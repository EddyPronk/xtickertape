/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: parser.c,v 2.1 2000/07/05 05:23:44 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#define INITIAL_TOKEN_BUFFER_SIZE 32


/* Test ch to see if it's valid as a non-initial ID character */
static int is_id_char(int ch)
{
    static char table[] =
    {
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x00 */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x10 */
	0, 1, 0, 1,  1, 1, 1, 0,  0, 0, 1, 1,  0, 1, 1, 1, /* 0x20 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 0, 0,  1, 1, 1, 1, /* 0x30 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x40 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1, /* 0x50 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x60 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0  /* 0x70 */
    };

    /* Use a table for quick lookup of those tricky symbolic chars */
    return (ch < 0 || ch > 127) ? 0 : table[ch];
}


/* FIX THIS: this goes in parser.h */
/* The parser type */
typedef struct parser *parser_t;

/* The lexer_state_t type */
typedef int (*lexer_state_t)(parser_t parser, int ch);

/* The structure of the configuration file parser */
struct parser
{
    /* The current lexical state */
    lexer_state_t state;

    /* The tag for error messages */
    char *tag;

    /* The current line number */
    int line_num;

    /* The token construction buffer */
    char *token;

    /* The next character in the token buffer */
    char *point;

    /* The end of the token buffer */
    char *token_end;


    /* The callback for when we've completed a subscription entry */
/*    parser_callback_t callback;*/
    void *callback;
    
    /* The client-supplied arg for the callback */
    void *rock;
};


/* Lexer state function headers */
static int lex_start(parser_t self, int ch);
static int lex_comment(parser_t self, int ch);
static int lex_bang(parser_t self, int ch);
static int lex_ampersand(parser_t self, int ch);
static int lex_eq(parser_t self, int ch);
static int lex_lt(parser_t self, int ch);
static int lex_gt(parser_t self, int ch);
static int lex_vbar(parser_t self, int ch);
static int lex_zero(parser_t self, int ch);
static int lex_decimal(parser_t self, int ch);
static int lex_float_first(parser_t self, int ch);
static int lex_dq_string(parser_t self, int ch);
static int lex_dq_string_esc(parser_t self, int ch);
static int lex_sq_string(parser_t self, int ch);
static int lex_sq_string_esc(parser_t self, int ch);
static int lex_id(parser_t self, int ch);
static int lex_id_esc(parser_t self, int ch);

/* Expands the token buffer */
static int grow_buffer(parser_t self)
{
    size_t length = (self -> token_end - self -> token) * 2;
    char *token;

    /* Make the buffer bigger */
    if (! (token = (char *)realloc(self -> token, length)))
    {
	return -1;
    }

    /* Update the pointers */
    self -> point = self -> point - self -> token + token;
    self -> token_end = token + length;
    self -> token = token;
    return 0;
}

/* Appends a character to the end of the token */
static int append_char(parser_t self, int ch)
{
    /* Make sure there's room */
    if (! (self -> point < self -> token_end))
    {
	if (grow_buffer(self) < 0)
	{
	    return -1;
	}
    }

    *(self -> point++) = ch;
    return 0;
}

/* Handle the first character of a new token */
static int lex_start(parser_t self, int ch)
{
    switch (ch)
    {
	/* The end of the input file */
	case EOF:
	{
	    printf("<EOF>\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* BANG or NEQ */
	case '!':
	{
	    self -> state = lex_bang;
	    return 0;
	}

	/* COMMENT */
	case '#':
	{
	    self -> point = self -> token;
	    return lex_comment(self, ch);
	}
	

	/* Double-quoted string */
	case '"':
	{
	    self -> point = self -> token;
	    self -> state = lex_dq_string;
	    return 0;
	}

	/* AND */
	case '&':
	{
	    self -> state = lex_ampersand;
	    return 0;
	}

	/* Single-quoted string */
	case '\'':
	{
	    self -> point = self -> token;
	    self -> state = lex_sq_string;
	    return 0;
	}

	/* LPAREN */
	case '(':
	{
	    printf("LPAREN\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* RPAREN */
	case ')':
	{
	    printf("RPAREN\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* COMMA */
	case ',':
	{
	    printf("COMMA\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* Zero can lead to many different kinds of numbers */
	case '0':
	{
	    self -> point = self -> token;
	    if (append_char(self, ch) < 0)
	    {
		return -1;
	    }

	    self -> state = lex_zero;
	    return 0;
	}

	/* COLON */
	case ':':
	{
	    printf("COLON\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* SEMI */
	case ';':
	{
	    printf("SEMI\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* LT or LE */
	case '<':
	{
	    self -> state = lex_lt;
	    return 0;
	}

	/* ASSIGN or EQ */
	case '=':
	{
	    self -> state = lex_eq;
	    return 0;
	}

	/* GT or GE */
	case '>':
	{
	    self -> state = lex_gt;
	    return 0;
	}

	/* LBRACKET */
	case '[':
	{
	    printf("LBRACKET\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* ID with quoted first character */
	case '\\':
	{
	    self -> point = self -> token;
	    self -> state = lex_id_esc;
	    return 0;
	}

	/* RBRACKET */
	case ']':
	{
	    printf("RBRACKET\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* ID */
	case '_':
	{
	    self -> point = self -> token;
	    if (append_char(self, ch) < 0)
	    {
		return -1;
	    }

	    self -> state = lex_id;
	    return 0;
	}

	/* OR */
	case '|':
	{
	    self -> state = lex_vbar;
	    return 0;
	}

	/* LBRACE */
	case '{':
	{
	    printf("LBRACE\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* RBRACE */
	case '}':
	{
	    printf("RBRACE\n");
	    self -> state = lex_start;
	    return 0;
	}
    }

    /* Ignore whitespace */
    if (isspace(ch))
    {
	self -> state = lex_start;
	return 0;
    }

    /* Watch for a number */
    if (isdigit(ch))
    {
	self -> point = self -> token;
	if (append_char(self, ch) < 0)
	{
	    return -1;
	}

	self -> state = lex_decimal;
	return 1;
    }

    /* Watch for identifiers */
    if (isalpha(ch))
    {
	self -> point = self -> token;
	if (append_char(self, ch) < 0)
	{
	    return -1;
	}

	self -> state = lex_id;
	return 0;
    }

    /* Anything else is bogus */
    printf("bad token character: `%c'\n", ch);
    return -1;
}


/* Reading a line comment */
static int lex_comment(parser_t self, int ch)
{
    /* Watch for the end of file */
    if (ch == EOF)
    {
	return lex_start(self, ch);
    }

    /* Watch for the end of the line */
    if (ch == '\n')
    {
	self -> state = lex_start;
	return 0;
    }

    /* Ignore everything else */
    self -> state = lex_comment;
    return 0;
}

static int lex_bang(parser_t self, int ch)
{
    fprintf(stderr, "lex_bang: not yet implemented\n");
    abort();
}

/* Reading the character after a `&' */
static int lex_ampersand(parser_t self, int ch)
{
    /* Another `&' is an AND */
    if (ch == '&')
    {
	printf("AND\n");
	self -> state = lex_start;
	return 0;
    }

    /* Otherwise we've got a problem */
    fprintf(stderr, "Bad token: `&'\n");
    abort();
}

/* Reading the character after an initial `=' */
static int lex_eq(parser_t self, int ch)
{
    /* Is there an other `=' for an EQ token? */
    if (ch == '=')
    {
	printf("EQ\n");
	self -> state = lex_start;
	return 0;
    }

    /* Nope.  This must be an ASSIGN. */
    printf("ASSIGN\n");
    return lex_start(self, ch);
}

/* Reading the character after a `<' */
static int lex_lt(parser_t self, int ch)
{
    switch (ch)
    {
	/* LE */
	case '=':
	{
	    printf("LE\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* Anything else is the next character */
	default:
	{
	    printf("LT\n");
	    return lex_start(self, ch);
	}
    }
}

/* Reading the character after a `>' */
static int lex_gt(parser_t self, int ch)
{
    switch (ch)
    {
	/* GE */
	case '=':
	{
	    printf("GE\n");
	    self -> state = lex_start;
	    return 0;
	}

	/* GT */
	default:
	{
	    printf("GT\n");
	    return lex_start(self, ch);
	}
    }
}

/* Reading the character after a `|' */
static int lex_vbar(parser_t self, int ch)
{
    /* Another `|' is an OR token */
    if (ch == '|')
    {
	printf("OR\n");
	self -> state = lex_start;
	return 0;
    }

    /* We're not doing simple math yet */
    fprintf(stderr, "bogus token: `|'\n");
    abort();
}

static int lex_zero(parser_t self, int ch)
{
    fprintf(stderr, "lex_zero: not yet implemented\n");
    abort();
}

/* We've read one or more digits of a decimal number */
static int lex_decimal(parser_t self, int ch)
{
    /* Watch for additional digits */
    if (isdigit(ch))
    {
	if (append_char(self, ch) < 0)
	{
	    return -1;
	}

	self -> state = lex_decimal;
	return 0;
    }

    /* Watch for a decimal point */
    if (ch == '.')
    {
	if (append_char(self, ch) < 0)
	{
	    return -1;
	}

	self -> state = lex_float_first;
	return 0;
    }

    /* Null-terminate the number */
    if (append_char(self, 0) < 0)
    {
	return -1;
    }

    /* Watch for a trailing 'L' to indicate an int64 value */
    if (ch == 'l' || ch == 'L')
    {
	printf("INT64: %s\n", self -> token);
	self -> state = lex_start;
	return 0;
    }

    /* Otherwise accept the int32 token */
    printf("INT32: %s\n", self -> token);
    return lex_start(self, ch);
}


/* Reading the first digit of the decimal portion of a floating point number */
static int lex_float_first(parser_t self, int ch)
{
    fprintf(stderr, "lex_float_first: not yet implemented\n");
    abort();
}

/* Reading the characters of a double-quoted string */
static int lex_dq_string(parser_t self, int ch)
{
    /* Watch for special characters */
    switch (ch)
    {
	/* Unterminated string constant */
	case EOF:
	{
	    fprintf(stderr, "Unterminated string constant\n");
	    abort();
	}

	/* An escaped character */
	case '\\':
	{
	    self -> state = lex_dq_string_esc;
	    return 0;
	}

	/* End of the string */
	case '"':
	{
	    /* Null-terminate the string */
	    if (append_char(self, 0) < 0)
	    {
		return -1;
	    }

	    /* Accept the string */
	    printf("STRING: `%s'\n", self -> token);

	    /* Go back to the start state */
	    self -> state = lex_start;
	    return 0;
	}

	/* Normal string character */
	default:
	{
	    if (append_char(self, ch) < 0)
	    {
		return -1;
	    }

	    self -> state = lex_dq_string;
	    return 0;
	}
    }
}

/* Reading an escaped character in a double-quoted string */
static int lex_dq_string_esc(parser_t self, int ch)
{
    fprintf(stderr, "lex_dq_string_esc: not yet implemented\n");
    abort();
}

/* Reading the characters of a single-quoted string */
static int lex_sq_string(parser_t self, int ch)
{
    /* Watch for special characters */
    switch (ch)
    {
	/* Unterminated string constant */
	case EOF:
	{
	    fprintf(stderr, "Unterminated string constant\n");
	    abort();
	}

	/* An escaped character */
	case '\\':
	{
	    self -> state = lex_sq_string_esc;
	    return 0;
	}

	/* End of the string */
	case '\'':
	{
	    /* Null-terminate the string */
	    if (append_char(self, 0) < 0)
	    {
		return -1;
	    }

	    /* Accept the string */
	    printf("STRING: `%s'\n", self -> token);

	    /* Go back to the start state */
	    self -> state = lex_start;
	    return 0;
	}

	/* Normal string character */
	default:
	{
	    if (append_char(self, ch) < 0)
	    {
		return -1;
	    }

	    self -> state = lex_sq_string;
	    return 0;
	}
    }
}

/* Reading an escaped character in a single-quoted string */
static int lex_sq_string_esc(parser_t self, int ch)
{
    fprintf(stderr, "lex_sq_string_esc: not yet implemented\n");
    abort();
}

/* Reading an identifier (2nd character on) */
static int lex_id(parser_t self, int ch)
{
    /* Watch for an escaped id char */
    if (ch == '\\')
    {
	self -> state = lex_id_esc;
	return 0;
    }

    /* Watch for additional id characters */
    if (is_id_char(ch))
    {
	if (append_char(self, ch) < 0)
	{
	    return -1;
	}

	self -> state = lex_id;
	return 0;
    }

    /* This is the end of the id */
    if (append_char(self, 0) < 0)
    {
	return -1;
    }

    /* Accept it */
    printf("ID: `%s'\n", self -> token);

    /* Run ch through the start state */
    return lex_start(self, ch);
}

static int lex_id_esc(parser_t self, int ch)
{
    fprintf(stderr, "lex_id_esc: not yet implemented\n");
    abort();
}

void parser_free(parser_t self);

/* Allocate space for a parser and initialize its contents */
parser_t parser_alloc()
{
    parser_t self;

    /* Allocate some memory for the new parser */
    if ((self = (parser_t)malloc(sizeof(struct parser))) == NULL)
    {
	return NULL;
    }

    /* Initialize all of the fields to sane values */
    memset(self, 0, sizeof(struct parser));

    /* Allocate room for the token buffer */
    if ((self -> token = (char *)malloc(INITIAL_TOKEN_BUFFER_SIZE)) == NULL)
    {
	parser_free(self);
	return NULL;
    }

    /* Initialize the rest of the values */
    self -> token_end = self -> token + INITIAL_TOKEN_BUFFER_SIZE;
    self -> line_num = 1;
    self -> state = lex_start;
    return self;
}

/* Frees the resources consumed by the parser */
void parser_free(parser_t self)
{
    /* Free the token buffer */
    if (self -> token)
    {
	free(self -> token);
    }

    free(self);
}


/* Parses a single character, counting LFs */
static int parse_char(parser_t self, int ch)
{
    if (self -> state(self, ch) < 0)
    {
	return -1;
    }

    if (ch == '\n')
    {
	self -> line_num++;
    }

    return 0;
}

/* Runs the characters in the buffer through the parser */
int parser_parse(parser_t self, char *buffer, size_t length)
{
    char *pointer;
    int ch;

    /* Length of 0 is an EOF */
    if (length == 0)
    {
	return parse_char(self, EOF);
    }

    for (pointer = buffer; pointer < buffer + length; pointer++)
    {
	if (parse_char(self, *pointer) < 0)
	{
	    return -1;
	}
    }

    return 0;
}


/* For testing purposes */
int main(int argc, char *argv[])
{
    parser_t parser;
    char buffer[2048];
    size_t length;
    int fd;

    if ((parser = parser_alloc()) == NULL)
    {
	fprintf(stderr, "parser_alloc(): failed\n");
	abort();
    }

    /* Read stdin and put it through the parser */
    fd = STDIN_FILENO;
    while (1)
    {
	if ((length = read(fd, buffer, 2048)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	if (parser_parse(parser, buffer, length) < 0)
	{
	    exit(1);
	}

	if (length < 1)
	{
	    printf("-- end of input --\n");
	    exit(0);
	}
    }
}
