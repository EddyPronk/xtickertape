#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_PACKET_SIZE 8192
#define BUFFER_SIZE 1024


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
static char *end = buffer + MAX_PACKET_SIZE;
static lexer_state_t state = lex_start;



/* Append a character to the current string */
static int append_char(int ch)
{
    putchar(ch);
    return 0;
}

/* Terminate a string */
static int terminate_string()
{
    printf("[0]");
    return 0;
}

/* Ignore additional input */
static int lex_error(int ch)
{
    return 0;
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

    printf("->");

    /* Anything else is part of the first field's name */
    if (append_char(toupper(ch)) < 0)
    {
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
	if (terminate_string() < 0)
	{
	    return -1;
	}

	/* FIX THIS: Make sure we've read `From' */
	printf("<-\n");

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
	if (terminate_string() < 0)
	{
	    return -1;
	}

	printf("<-\n");
	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(tolower(ch)) < 0)
    {
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
	if (terminate_string() < 0)
	{
	    return -1;
	}

	printf("<-\n");
	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(tolower(ch)) < 0)
    {
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
	if (terminate_string() < 0)
	{
	    return -1;
	}

	printf("<-\n");
	state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(toupper(ch)) < 0)
    {
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
	printf("=>");
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
    printf("=>");
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
	if (terminate_string() < 0)
	{
	    return -1;
	}

	printf("<=\n");
	state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch))
    {
	if (append_char(' ') < 0)
	{
	    return -1;
	}

	state = lex_body;
	return 0;
    }

    if (terminate_string() < 0)
    {
	return -1;
    }

    printf("<=\n");

    /* A colon here is an error */
    if (ch == ':')
    {
	state = lex_error;
	return 0;
    }

    /* Anything else is the start of the next field */
    printf("->");
    if (append_char(toupper(ch)) < 0)
    {
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
	    return -1;
	}
    }

    return 0;
}


int main(int argc, char *argv[])
{
    char buffer[BUFFER_SIZE];
    int fd = STDIN_FILENO;

    while (1)
    {
	ssize_t length;

	/* Read some more */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	/* Run it through the lexer */
	if (lex(buffer, length) < 0)
	{
	    exit(1);
	}

	/* Check for end of input */
	if (length == 0)
	{
	    exit(0);
	}
    }
}
