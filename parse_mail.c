#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PACKET_SIZE 8192
#define BUFFER_SIZE 1024

#define UNOTIFY_TYPECODE 1
#define STRING_TYPECODE 4
#define PROTO_VERSION_MAJOR 4
#define PROTO_VERSION_MINOR 0


typedef int (*lexer_state_t)(int ch);

/* Forward declarations for the state machine states */
static int lex_error(int ch);
static int lex_start(int ch);
static int lex_first_name(int ch);
static int lex_name(int ch);
static int lex_dash(int ch);
static int lex_ws(int ch);
static int lex_body(int ch);
static int lex_fold(int ch);
static int lex_end(int ch);


static char buffer[MAX_PACKET_SIZE];
static char *point = buffer;
static char *end = buffer + MAX_PACKET_SIZE;
static char *count_point = NULL;
static char *length_point = NULL;
static int count = 0;
static lexer_state_t state = lex_start;


/* Writes an int32 into a buffer */
static void write_int32(char *buffer, int value)
{
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) && 0xff;
    buffer[3] = (value >> 0) & 0xff;
}

/* Append an int32 to the outgoing buffer */
static int append_int32(int value)
{
    /* Make sure there's room */
    if (point + 4 < end)
    {
	write_int32(point, value);
	point += 4;
	return 0;
    }

    return -1;
}


/* Append a character to the current string */
static int append_char(int ch)
{
    if (point < end)
    {
	*(point++) = ch;
	return 0;
    }

    return -1;
}

/* Begin an attribute name */
static int begin_name()
{
    if (point + 4 < end)
    {
	/* Remember where the length goes */
	length_point = point;
	point += 4;
	return 0;
    }

    return -1;
}

/* End an attribute name */
static int end_name()
{
    char *name = length_point + 4;

    /* Write the length */
    write_int32(length_point, point - name);

    /* Get to a 4-byte boundary */
    while ((int)point & 3)
    {
	*(point++) = '\0';
    }

    return 0;
}


/* Begin an attribute string value */
static int begin_string_value()
{
    if (point + 8 < end)
    {
	/* Write the string typecode */
	append_int32(STRING_TYPECODE);

	/* Remember where the length goes */
	length_point = point;
	point += 4;
	return 0;
    }

    return -1;
}

/* End an attribute string value */
static int end_string_value()
{
    char *value = length_point + 4;

    /* Write the length */
    write_int32(length_point, point - value);

    /* Get to a 4-byte boundary */
    while ((int)point & 3)
    {
	*(point++) = '\0';
    }

    count++;
    return 0;
}

/* Ignore additional input */
static int lex_error(int ch)
{
    return -1;
}

/* Get started */
static int lex_start(int ch)
{
    /* Bitch and moan if we start with a space or a colon */
    if (ch == ':' || isspace(ch))
    {
	state = lex_error;
	return -1;
    }

    /* Start a name field */
    if (begin_name() < 0)
    {
	state = lex_error;
	return -1;
    }

    /* Anything else is part of the first field's name */
    if (append_char(toupper(ch)) < 0)
    {
	state = lex_error;
	return -1;
    }

    state = lex_first_name;
    return 0;
}

/* Reading the first field name */
static int lex_first_name(int ch)
{
    /* SPECIAL CASE: the first line of may an out-of-band `From' line
     * in which the `From' doesn't end in a colon. */
    if (ch == ' ')
    {
	/* FIX THIS: Make sure we've read `From' */
	if (end_name() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_ws;
	return 0;
    }

    /* Other whitespace is an error */
    if (isspace(ch))
    {
	state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':')
    {
	if (end_name() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(tolower(ch)) < 0)
    {
	state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
    }

    state = lex_first_name;
    return 0;
}

/* Reading a field name */
static int lex_name(int ch)
{
    if (isspace(ch))
    {
	state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':')
    {
	if (end_name() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(tolower(ch)) < 0)
    {
	state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
    }

    state = lex_name;
    return 0;
}

/* We've read a `-' in a name */
static int lex_dash(int ch)
{
    /* Spaces are bogus in field names */
    if (isspace(ch))
    {
	state = lex_error;
	return -1;
    }

    /* A colon indicates the end of the field name */
    if (ch == ':')
    {
	if (end_name() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(toupper(ch)) < 0)
    {
	state = lex_error;
	return -1;
    }

    /* Watch for additional dashes */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
    }

    state = lex_name;
    return 0;
}

/* Skip over whitespace */
static int lex_ws(int ch)
{
    /* Try to fold on a LF */
    if (ch == '\n')
    {
	if (begin_string_value() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_fold;
	return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch))
    {
	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the body */
    if (begin_string_value() < 0)
    {
	state = lex_error;
	return -1;
    }

    return lex_body(ch);
}

/* Read the body */
static int lex_body(int ch)
{
    /* Try to fold on LF */
    if (ch == '\n')
    {
	state = lex_fold;
	return 0;
    }

    /* Anything else is part of the body */
    if (append_char(ch) < 0)
    {
	state = lex_error;
	return -1;
    }

    state = lex_body;
    return 0;
}

/* Try to fold */
static int lex_fold(int ch)
{
    /* A linefeed means that we're out of headers */
    if (ch == '\n')
    {
	if (end_string_value() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch))
    {
	if (append_char(' ') < 0)
	{
	    state = lex_error;
	    return -1;
	}

	state = lex_body;
	return 0;
    }

    if (end_string_value() < 0)
    {
	state = lex_error;
	return -1;
    }

    /* A colon here is an error */
    if (ch == ':')
    {
	state = lex_error;
	return 0;
    }

    /* Anything else is the start of the next field */
    if (begin_name() < 0)
    {
	state = lex_error;
	return -1;
    }

    /* Make sure the first letter is always uppercase */
    if (append_char(toupper(ch)) < 0)
    {
	state = lex_error;
	return -1;
    }

    state = lex_name;
    return 0;
}

static int lex_end(int ch)
{
    state = lex_end;
    return 0;
}



/* Run the buffer through the lexer */
static int lex(char *in, ssize_t length)
{
    char *point;

    /* Watch for the end-of-input marker */
    if (length == 0)
    {
	return state(EOF);
    }

    for (point = in; point< in + length; point++)
    {
	if (state(*point) < 0)
	{
	    state = lex_error;
	    return -1;
	}
    }

    return 0;
}


static void dump_buffer()
{
    fwrite(buffer, 1, point - buffer, stdout);
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    char buffer[BUFFER_SIZE];
    int fd = STDIN_FILENO;

    /* Write the header into the outgoing buffer */
    append_int32(UNOTIFY_TYPECODE);
    append_int32(PROTO_VERSION_MAJOR);
    append_int32(PROTO_VERSION_MINOR);

    /* The next 4 bytes is where the number of attributes goes */
    count_point = point;
    point += 4;

    /* Read the attributes and add them to the notification */
    while (1)
    {
	ssize_t length;
	int result;

	/* Read some more */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	/* Run it through the lexer */
	result = lex(buffer, length);

	/* Check for end of input */
	if (length == 0)
	{
	    if (result < 0)
	    {
		exit(1);
	    }

	    /* Write the number of attributes field */
	    write_int32(count_point, count);

	    /* No keys (yet) */
	    append_int32(0);

	    /* What have we got? */
	    dump_buffer();
	    exit(0);
	}
    }
}
