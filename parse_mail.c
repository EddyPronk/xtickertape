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
static int lex_skip_body(int ch);
static int lex_skip_fold(int ch);
static int lex_end(int ch);


static char buffer[MAX_PACKET_SIZE];
static char *point = buffer;
static char *end = buffer + MAX_PACKET_SIZE;
static char *first_name_point = NULL;
static char *count_point = NULL;
static char *length_point = NULL;
static int count = 0;
static lexer_state_t state = lex_start;

/* The length-prefixed string `From' */
static char from_string[] = { 0, 0, 0, 4, 'F', 'r', 'o', 'm' };



/* Writes an int32 into a buffer */
static void write_int32(char *buffer, int value)
{
    buffer[0] = (value >> 24) & 0xff;
    buffer[1] = (value >> 16) & 0xff;
    buffer[2] = (value >> 8) && 0xff;
    buffer[3] = (value >> 0) & 0xff;
}

/* Reads an int32 from a buffer */
static int read_int32(char *buffer)
{
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3] << 0);
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

/* Begin writing a string */
static int begin_string()
{
    if (point + 4 < end)
    {
	length_point = point;
	point += 4;
	return 0;
    }

    return -1;
}

/* Finish writing a string */
static int end_string()
{
    char *string = length_point + 4;

    /* Write the length */
    write_int32(length_point, point - string);

    /* Pad the string out to a 4-byte boundary */
    while ((int)point & 3)
    {
	*(point++) = '\0';
    }

    return 0;
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


/* Compares two length-prefixed strings for equality */
static int lstring_eq(char *string1, char *string2)
{
    int len, i;

    /* First compare the lengths */
    len = read_int32(string1);
    if (read_int32(string2) != len)
    {
	return 0;
    }

    string1 += 4;
    string2 += 4;

    /* Then compare all chars */
    for (i = 0; i < len; i++)
    {
	if (string1[i] != string2[i])
	{
	    return 0;
	}
    }

    return 1;
}



/* Begin an attribute name */
static int begin_name()
{
    return begin_string();
}

/* End an attribute name */
static int end_name()
{
    char *name;

    /* Tidy up the string */
    if (end_string() < 0)
    {
	return -1;
    }

    /* Check if we've already seen this name */
    name = first_name_point;
    while (name < length_point)
    {
	/* Look for a match */
	if (lstring_eq(length_point, name))
	{
	    /* Uh oh!  Discard the new name */
	    point = length_point;
	    state = lex_skip_body;
	    return 1;
	}

	name += read_int32(name) + 4;
    }

    return 0;
}


/* Begin an attribute string value */
static int begin_string_value()
{
    if (point + 4 < end)
    {
	/* Write the string typecode */
	append_int32(STRING_TYPECODE);

	return begin_string();
    }

    return -1;
}

/* End an attribute string value */
static int end_string_value()
{
    end_string();
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

    /* Watch for an initial dash */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
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
	/* Complete the name string */
	if (end_name() < 0)
	{
	    state = lex_error;
	    return -1;
	}

	/* Make sure it matches `From' */
	if (! lstring_eq(length_point, from_string))
	{
	    state = lex_error;
	    return -1;
	}

	/* Convert the first character to lowercase */
	length_point[4] = 'f';

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
	int result;

	if ((result = end_name()) < 0)
	{
	    state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result)
	{
	    state = lex_skip_body;
	    return 0;
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
	int result;

	if ((result = end_name()) < 0)
	{
	    state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result)
	{
	    state = lex_skip_body;
	    return 0;
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

    /* Watch for an initial dash */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
    }

    state = lex_name;
    return 0;
}

/* Discard the body */
static int lex_skip_body(int ch)
{
    /* Try to fold on LF */
    if (ch == '\n')
    {
	state = lex_skip_fold;
	return 0;
    }

    /* Anything else is part of the body */
    state = lex_skip_body;
    return 0;
}

/* Try to fold */
static int lex_skip_fold(int ch)
{
    /* A linefeed means the end of the headers */
    if (ch == '\n')
    {
	state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch))
    {
	state = lex_skip_body;
	return 0;
    }

    /* Anything else is the beginning of the next name */
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

    /* Watch for an initial dash */
    if (ch == '-')
    {
	state = lex_dash;
	return 0;
    }

    state = lex_name;
    return 0;
}

/* Skip everything after the headers */
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
    first_name_point = point;

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
