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
static const char cvsid[] = "$Id: parser.c,v 2.24 2000/11/05 08:42:00 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <elvin/elvin.h>
#include <elvin/convert.h>
#include <elvin/memory.h>
#include "errors.h"
#include "atom.h"
#include "parser.h"

#define INITIAL_BUFFER_SIZE 256
#define INITIAL_STACK_SIZE 32

/* The type of a lexer state */
typedef int (*lexer_state_t)(parser_t self, int ch, elvin_error_t error);

/* Transforms an escape sequence (ex: `\n') into a character */
static int translate_esc_code(int ch)
{
    switch (ch)
    {
	/* Alert */
	case 'a':
	{
	    return '\a';
	}

	/* Backspace */
	case 'b':
	{
	    return '\b';
	}

	/* Form feed */
	case 'f':
	{
	    return '\f';
	}

	/* Newline */
	case 'n':
	{
	    return '\n';
	}

	/* Carriage return */
	case 'r':
	{
	    return '\r';
	}

	/* Horizontal tab */
	case 't':
	{
	    return '\t';
	}

	/* Vertical tab */
	case 'v':
	{
	    return '\v';
	}

	/* Anything else is simply itself */
	default:
	{
	    return ch;
	}
    }
}

/* Test ch to see if it's valid as a non-initial ID character */
static int is_id_char(int ch)
{
    static char table[] =
    {
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x00 */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x10 */
	0, 1, 0, 1,  1, 1, 1, 0,  0, 0, 1, 1,  0, 1, 1, 1, /* 0x20 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x30 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x40 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1, /* 0x50 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x60 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0  /* 0x70 */
    };

    /* Use a table for quick lookup of those tricky symbolic chars */
    return (ch < 0 || ch > 127) ? 0 : table[ch];
}

/* The parser data structure */
struct parser
{
    /* The receiver's callback */
    parser_callback_t callback;

    /* The callback's user-supplied argument */
    void *rock;
    
    /* The receiver's state stack */
    int *state_stack;

    /* The end of the receiver's state stack */
    int *state_end;

    /* The top of the state stack */
    int *state_top;

    /* The receiver's value stack */
    atom_t *value_stack;

    /* The top of the value stack */
    atom_t *value_top;

    /* The receiver's lexical state */
    lexer_state_t state;

    /* The token construction buffer */
    uchar *token;

    /* The character past the end of the token construction buffer */
    uchar *token_end;

    /* A pointer to the next free character in the token buffer */
    uchar *point;

    /* The offset to the beginning of the current number */
    int offset;
};


/* The reduction function type */
typedef atom_t (*reduction_t)(parser_t self, elvin_error_t error);

/* The reduction functions */
static atom_t make_list(parser_t self, elvin_error_t error);
static atom_t make_dot_list(parser_t self, elvin_error_t error);
static atom_t make_nil(parser_t self, elvin_error_t error);
static atom_t make_quote(parser_t self, elvin_error_t error);
static atom_t identity(parser_t self, elvin_error_t error);
static atom_t extend_cons(parser_t self, elvin_error_t error);
static atom_t make_cons(parser_t self, elvin_error_t error);

#include "grammar.h"

/* Returns a string representation of most terminal types */
static uchar *terminal_string(terminal_t terminal)
{
    static char *strings[] =
    {
	"[eof]", "(", ")", ".", NULL
    };

    return (uchar *)strings[terminal];
}


/* Lexer states */
static int lex_start(parser_t self, int ch, elvin_error_t error);
static int lex_comment(parser_t self, int ch, elvin_error_t error);
static int lex_string(parser_t self, int ch, elvin_error_t error);
static int lex_string_esc(parser_t self, int ch, elvin_error_t error);
static int lex_char(parser_t self, int ch, elvin_error_t error);
static int lex_signed(parser_t self, int ch, elvin_error_t error);
static int lex_float_pre(parser_t self, int ch, elvin_error_t error);
static int lex_dot(parser_t self, int ch, elvin_error_t error);
static int lex_float(parser_t self, int ch, elvin_error_t error);
static int lex_integer(parser_t self, int ch, elvin_error_t error);
static int lex_exp_pre(parser_t self, int ch, elvin_error_t error);
static int lex_exp_signed(parser_t self, int ch, elvin_error_t error);
static int lex_exp(parser_t self, int ch, elvin_error_t error);
static int lex_symbol(parser_t self, int ch, elvin_error_t error);
static int lex_symbol_esc(parser_t self, int ch, elvin_error_t error);

/* Increase the size of the stack */
static int grow_stack(parser_t self, elvin_error_t error)
{
    size_t length = (self -> state_end - self -> state_stack) * 2;
    int *state_stack;
    atom_t *value_stack;

    /* Allocate memory for the new state stack */
    if ((state_stack = (int *)ELVIN_REALLOC(
	self -> state_stack, length * sizeof(int), error)) == NULL)
    {
	return 0;
    }

    /* Allocate memory for the new value stack */
    if ((value_stack = (atom_t *)ELVIN_REALLOC(
	self -> value_stack, length * sizeof(atom_t), error)) == NULL)
    {
	return 0;
    }

    /* Update the state stack's pointers */
    self -> state_end = state_stack + length;
    self -> state_top = self -> state_top - self -> state_stack + state_stack;
    self -> state_stack = state_stack;

    /* Update the value stack's pointers */
    self -> value_top = self -> value_top - self -> value_stack + value_stack;
    self -> value_stack = value_stack;
    return 1;
}

/* Push a state and value onto the stack */
static int push(parser_t self, int state, atom_t value, elvin_error_t error)
{
    /* Grow the stack if necessary */
    if (! (self -> state_top < self -> state_end))
    {
	if (grow_stack(self, error) == 0)
	{
	    return 0;
	}
    }

    /* The state stack is pre-increment */
    *(++self -> state_top) = state;

    /* The value stack is post-increment */
    *(self -> value_top++) = value;

    return 1;
}

/* Moves the top of the stack back `count' spaces */
static void pop(parser_t self, int count, elvin_error_t error)
{
    /* Sanity check */
    if (self -> state_stack > self -> state_top - count)
    {
	fprintf(stderr, "popped off the top of the stack\n");
	abort();
    }

    self -> state_top -= count;
    self -> value_top -= count;
}

/* Puts n elements back onto the stack */
static void unpop(parser_t self, int count, elvin_error_t error)
{
    self -> state_top += count;
    self -> value_top += count;
}

/* Answers the top of the stack */
static int top(parser_t self)
{
    return *(self -> state_top);
}

/* Free everything on the stack */
static void clean_stack(parser_t self, elvin_error_t error)
{
    atom_t *pointer;

    /* Go through everything on the stack */
    for (pointer = self -> value_stack; pointer < self -> value_top; pointer++)
    {
	if (*pointer != NULL)
	{
	    atom_free(*pointer, error);
	}
    }

    /* Reset the stack pointers */
    self -> state_top = self -> state_stack;
    self -> value_top = self -> value_stack;
}

/* This is called when we have an actual expression */
static int accept_input(parser_t self, elvin_error_t error)
{
    atom_t sexp = self -> value_top[0];
    int result;

    /* Call the callback */
    result = self -> callback(self -> rock, self, sexp, error);

    /* Free the sexp */
    result = atom_free(sexp, result ? error : NULL) && result;
    return result;
}


/* Move the parser state along as far as it can go with the addition
 * of another token */
static int shift_reduce(parser_t self, terminal_t terminal, atom_t value, elvin_error_t error)
{
    int action = sr_table[top(self)][terminal];

    /* Watch for the mighty EOF */
    if ((terminal == TT_EOF) && (self -> state_stack == self -> state_top))
    {
	return 1;
    }

    /* Do the shift (we know that we can't do any interesting reductions... */
    if (IS_SHIFT(action))
    {
	if (push(self, SHIFT_GOTO(action), value, error) == 0)
	{
	    return 0;
	}
    }

    /* Watch for errors */
    if (IS_ERROR(action))
    {
	if (value == NULL)
	{
	    ELVIN_ERROR_LISP_PARSE_ERROR(error, terminal_string(terminal));
	    return 0;
	}

	ELVIN_ERROR_LISP_PARSE_ERROR(error, self -> token);
	return 0;
    }

    /* Reduce as many times as possible */
    while (IS_REDUCE(action = sr_table[top(self)][terminal]))
    {
	struct production *production;
	int reduction;
	atom_t result;

	/* Locate the production rule to use to do the reduction */
	reduction = REDUCTION(action);
	production = productions + reduction;

	/* Point the stack at the beginning of the components of the reduction */
	pop(self, production -> count, error);

	/* Reduce by calling the production rule's function */
	if ((result = production -> reduction(self, error)) == NULL)
	{
	    /* Put the args back onto the stack */
	    unpop(self, production -> count, error);
	    return 0;
	}

	/* Push the result onto the stack */
	if (push(self, REDUCE_GOTO(top(self), production), result, error) == 0)
	{
	    return 0;
	}
    }

    /* Can we accept? */
    if (top(self) == 1)
    {
	/* Set up the stack */
	pop(self, 1, error);
	return accept_input(self, error);
    }

    return 1;
}


/* Update the parser state to reflect the reading of a QUOTE token */
static int accept_quote(parser_t self, elvin_error_t error)
{
    return shift_reduce(self, TT_QUOTE, NULL, error);
}

/* Update the parser state to reflect the reading of a LPAREN token */
static int accept_lparen(parser_t self, elvin_error_t error)
{
    return shift_reduce(self, TT_LPAREN, NULL, error);
}

/* Update the parser state to reflect the reading of a RPAREN token */
static int accept_rparen(parser_t self, elvin_error_t error)
{
    return shift_reduce(self, TT_RPAREN, NULL, error);
}

/* Update the parser state to reflect the reading of an EOF token */
static int accept_eof(parser_t self, elvin_error_t error)
{
    return shift_reduce(self, TT_EOF, NULL, error);
}

/* Update the parser state to reflect the reading of a STRING token */
static int accept_string(parser_t self, uchar *string, elvin_error_t error)
{
    atom_t atom;

    /* Construct a string from the token */
    if ((atom = string_alloc(string, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}

/* Update the parser state to reflect the reading of a CHAR token */
static int accept_char(parser_t self, int ch, elvin_error_t error)
{
    atom_t atom;

    /* Construct a character atom from the character */
    if ((atom = char_alloc(ch, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}

/* Update the parser state to reflect the reading of a DOT token */
static int accept_dot(parser_t self, elvin_error_t error)
{
    return shift_reduce(self, TT_DOT, NULL, error);
}

/* Update the parser state to reflect the reading of an INT32 token */
static int accept_int32(parser_t self, int32_t value, elvin_error_t error)
{
    atom_t atom;

    /* Construct an atom from the token */
    if ((atom = int32_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}

/* Transform a string into an integer and accept it */
static int accept_int32_string(parser_t self, uchar *string, elvin_error_t error)
{
    int32_t value;

    if (elvin_string_to_int32((char *)string, &value, error) == 0)
    {
	return 0;
    }

    return accept_int32(self, value, error);
}

/* Update the parser state to reflect the reading of an INT64 token */
static int accept_int64(parser_t self, int64_t value, elvin_error_t error)
{
    atom_t atom;

    /* Construct an atom from the token */
    if ((atom = int64_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}

/* Transform a string into an int64 and accept it */
static int accept_int64_string(parser_t self, uchar *string, elvin_error_t error)
{
    int64_t value;

    if (elvin_string_to_int64((char *)string, &value, error) == 0)
    {
	return 0;
    }

    return accept_int64(self, value, error);
}


/* Update the parser state to reflect the reading of an REAL64 token */
static int accept_real64(parser_t self, double value, elvin_error_t error)
{
    atom_t atom;

    /* Construct an atom from the token */
    if ((atom = float_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}

/* Translate the string into a double-precision floating point number and accept it */
static int accept_real64_string(parser_t self, uchar *string, elvin_error_t error)
{
    double value;

    errno = 0;
    value = strtod((char *)string, NULL);
    if (errno != 0)
    {
	if (errno == ERANGE)
	{
	    ELVIN_ERROR_LISP_OVERFLOW(error, string);
	    return 0;
	}

	/* Otherwise bail out ungracefully */
	perror("strtod(): failed");
	abort();
    }

    return accept_real64(self, value, error);
}

/* Update the parser state to reflect the reading of an SYMBOL token */
static int accept_symbol(parser_t self, char *string, elvin_error_t error)
{
    atom_t atom;

    /* Check for the magic `nil' token */
    if (strcmp(string, "nil") == 0)
    {
	if ((atom = nil_alloc(error)) == NULL)
	{
	    return 0;
	}
    }
    /* Otherwise construct an atom from the token */
    else if ((atom = symbol_alloc(string, error)) == NULL)
    {
	return 0;
    }

    /* Do the parser thing */
    return shift_reduce(self, TT_ATOM, atom, error);
}


/* Expands the token buffer */
static int grow_buffer(parser_t self, elvin_error_t error)
{
    size_t length = (self -> token_end - self -> token) * 2;
    uchar *token;

    /* Allocate a bigger buffer */
    if ((token = (uchar *)ELVIN_REALLOC(self -> token, length, error)) == NULL)
    {
	return 0;
    }

    /* Update the pointers */
    self -> point = self -> point - self -> token + token;
    self -> token = token;
    self -> token_end = token + length;
    return 1;
}

/* Append a character to the end of the token, growing the token if necessary */
static int append_char(parser_t self, int ch, elvin_error_t error)
{
    /* Double the size of the buffer if it isn't big enough */
    if (! (self -> point < self -> token_end))
    {
	if (grow_buffer(self, error) == 0)
	{
	    return 0;
	}
    }

    /* Append the character to the end of the buffer */
    *(self -> point++) = ch;
    return 1;
}

/* Awaiting the first character of a token */
static int lex_start(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* Watch for a quoted string */
	case '"':
	{
	    self -> state = lex_string;
	    self -> point = self -> token;
	    return 1;
	}

	case '\'':
	{
	    /* Accept the QUOTE */
	    if (accept_quote(self, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Watch for a LPAREN token */
	case '(':
	{
	    /* Accept the LPAREN */
	    if (accept_lparen(self, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Watch for a RPAREN token */
	case ')':
	{
	    /* Accept the RPAREN */
	    if (accept_rparen(self, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Watch for a signed number */
	case '+':
	case '-':
	{
	    /* Record the sign */
	    self -> point = self -> token;
	    if (append_char(self, ch, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_signed;
	    return 1;
	}

	/* Watch for a semicolon */
	case ';':
	{
	    /* Ignore comments */
	    self -> state = lex_comment;
	    return 1;
	}

	/* Watch for a dot or decimal number */
	case '.':
	{
	    /* Record the dot */
	    self -> point = self -> token;
	    if (append_char(self, ch, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_dot;
	    return 1;
	}

	/* Watch for a character */
	case '?':
	{
	    self -> state = lex_char;
	    return 1;
	}

	/* Watch for an escaped symbol */
	case '\\':
	{
	    self -> point = self -> token;
	    self -> state = lex_symbol_esc;
	    return 1;
	}

	/* Watch for EOF */
	case EOF:
	{
	    /* Accept the EOF token */
	    if (accept_eof(self, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}
    }

    /* Watch for whitespace */
    if (isspace(ch))
    {
	self -> state = lex_start;
	return 1;
    }

    /* Watch for a number */
    if (isdigit(ch))
    {
	self -> point = self -> token;
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_integer;
	return 1;
    }

    /* Watch for identifier characters */
    if (is_id_char(ch))
    {
	self -> point = self -> token;
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_symbol;
	return 1;
    }

    /* Anything else is trouble */
    self -> point = self -> token;
    if (append_char(self, ch, error) == 0)
    {
	return 0;
    }

    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    ELVIN_ERROR_LISP_INVALID_TOKEN(error, self -> token);
    return 0;
}

/* Skipping a comment */
static int lex_comment(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for the end of input */
    if (ch == EOF)
    {
	return lex_start(self, ch, error);
    }

    /* Watch for the end of the line */
    if (ch == '\n')
    {
	self -> state = lex_start;
	return 1;
    }

    return 1;
}

/* Reading string characters */
static int lex_string(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* Watch for the end of input */
	case EOF:
	{
	    ELVIN_ERROR_LISP_UNTERM_STRING(error);
	    return 0;
	}

	/* Watch for the closing quote */
	case '"':
	{
	    /* Null-terminate the token */
	    if (append_char(self, 0, error) == 0)
	    {
		return 0;
	    }

	    /* Accept it */
	    if (accept_string(self, self -> token, error) == 0)
	    {
		return 0;
	    }
	    
	    self -> state = lex_start;
	    return 1;
	}

	/* Watch for an escape character */
	case '\\':
	{
	    self -> state = lex_string_esc;
	    return 1;
	}
    }

    /* Anything else gets appended to the token */
    if (append_char(self, ch, error) == 0)
    {
	return 0;
    }

    self -> state = lex_string;
    return 1;
}

/* Reading an escaped character in a string */
static int lex_string_esc(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	ELVIN_ERROR_LISP_UNTERM_STRING(error);
	return 0;
    }

    /* Newlines are, oddly, a continuation and ignored */
    if (ch == '\n')
    {
	self -> state = lex_string;
	return 1;
    }

    /* Record the character */
    if (append_char(self, translate_esc_code(ch), error) == 0)
    {
	return 0;
    }

    self -> state = lex_string;
    return 1;
}

/* Reading the character after a `?' */
static int lex_char(parser_t self, int ch, elvin_error_t error)
{
    /* Don't permit `?' at the end of the file */
    if (ch == EOF)
    {
	ELVIN_ERROR_LISP_UNTERM_SYMBOL(error);
	return 0;
    }

    if (accept_char(self, ch, error) == 0)
    {
	return 0;
    }

    self -> state = lex_start;
    return 1;
}


/* Reading a number beginning with `+' or `-' */
static int lex_signed(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a decimal point */
    if (ch == '.')
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_float_pre;
	return 1;
    }

    /* Watch for digits */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_integer;
	return 1;
    }

    /* Anything else is a symbol */
    return lex_symbol(self, ch, error);
}

/* Reading the first character after a signed decimal */
static int lex_float_pre(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a digit */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_float;
	return 1;
    }
    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    ELVIN_ERROR_LISP_INVALID_TOKEN(error, self -> token);
    return 0;
}

/* Reading the first character after a dot */
static int lex_dot(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a digit */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_float;
	return 1;
    }

    /* Anything else is the start of a new token after a DOT */
    if (accept_dot(self, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading additional digits of a number */
static int lex_integer(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* Watch for a decimal point */
	case '.':
	{
	    if (append_char(self, ch, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_float;
	    return 1;
	}

	/* Watch for a trailing 'L' */
	case 'l':
	case 'L':
	{
	    /* Null-terminate the number */
	    if (append_char(self, 0, error) == 0)
	    {
		return 0;
	    }

	    /* Accept the int64 token */
	    if (accept_int64_string(self, self -> token, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Watch for an exponent */
	case 'e':
	case 'E':
	{
	    if (append_char(self, ch, error) == 0)
	    {
		return 0;
	    }

	    self -> state = lex_exp_pre;
	    return 1;
	}
    }

    /* Watch for additional digits */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_integer;
	return 1;
    }

    /* Null-terminate the token string */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    /* Accept the integer token */
    if (accept_int32_string(self, self -> token, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading the decimal portion of a floating-point number */
static int lex_float(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for the beginning of the exponent */
    if (tolower(ch) == 'e')
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_exp_pre;
	return 1;
    }

    /* Watch for additional digits */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_float;
	return 1;
    }

    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    /* Accept the token */
    if (accept_real64_string(self, self -> token, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading the first character after the 'e' in an exponent */
static int lex_exp_pre(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a sign */
    if ((ch == '+') || (ch == '-'))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_exp_signed;
	return 1;
    }

    /* Watch for a digit */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Anything else is an error */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    ELVIN_ERROR_LISP_INVALID_TOKEN(error, self -> token);
    return 0;
}

/* Reading the first character after an exponent's sign */
static int lex_exp_signed(parser_t self, int ch, elvin_error_t error)
{
    /* Watch a digit */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    /* Accept the token */
    if (accept_real64_string(self, self -> token, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading additional characters in the exponent */
static int lex_exp(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for additional digits */
    if (isdigit(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    /* Anything else is the start of the next token */
    if (accept_real64_string(self, self -> token, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading additional characters of a symbol */
static int lex_symbol(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for the symbol escape character */
    if (ch == '\\')
    {
	self -> state = lex_symbol_esc;
	return 1;
    }

    /* Watch for more id characters */
    if (is_id_char(ch))
    {
	if (append_char(self, ch, error) == 0)
	{
	    return 0;
	}

	self -> state = lex_symbol;
	return 1;
    }

    /* Null-terminate the token */
    if (append_char(self, 0, error) == 0)
    {
	return 0;
    }

    /* Accept the symbol */
    if (accept_symbol(self, (char *)self -> token, error) == 0)
    {
	return 0;
    }

    /* Send the character to the next token */
    return lex_start(self, ch, error);
}

/* Reading an escaped character in a symbol */
static int lex_symbol_esc(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	ELVIN_ERROR_LISP_UNTERM_SYMBOL(error);
	return 0;
    }

    /* Anything else is fair game */
    if (append_char(self, ch, error) == 0)
    {
	return 0;
    }

    self -> state = lex_symbol;
    return 1;
}


/* Make a list out of cons cells by reversing them */
static atom_t make_list(parser_t self, elvin_error_t error)
{
    atom_t nil;

    /* Allocate a nil for the end of the list */
    if ((nil = nil_alloc(error)) == NULL)
    {
	return NULL;
    }

    return cons_reverse(self -> value_top[1], nil, error);
}

/* Make a list out of cons cells by reversing them */
static atom_t make_dot_list(parser_t self, elvin_error_t error)
{
    return cons_reverse(self -> value_top[1], self -> value_top[3], error);
}

/* `LPAREN RPAREN' reduces to `nil' */
static atom_t make_nil(parser_t self, elvin_error_t error)
{
    return nil_alloc(error);
}

/* QUOTE <expression> is (quote <expr>) */
static atom_t make_quote(parser_t self, elvin_error_t error)
{
    atom_t quote, nil;
    atom_t cons;

    /* Grab nil */
    if ((nil = nil_alloc(error)) == NULL)
    {
	return NULL;
    }

    /* Wrap the list in an expression */
    if ((cons = cons_alloc(self -> value_top[1], nil, error)) == NULL)
    {
	return NULL;
    }

    /* Allocate the `quote' symbol */
    if ((quote = symbol_alloc("quote", error)) == NULL)
    {
	return NULL;
    }

    /* Prepend it to the list */
    return cons_alloc(quote, cons, error);
}

/* No transformation required for this reduction */
static atom_t identity(parser_t self, elvin_error_t error)
{
    return self -> value_top[0];
}

/* Build a list backwards (it's more efficient that way :-P) */
static atom_t extend_cons(parser_t self, elvin_error_t error)
{
    return cons_alloc(self -> value_top[0], self -> value_top[1], error);
}

/* Create a cons cell which only has a car so far */
static atom_t make_cons(parser_t self, elvin_error_t error)
{
    atom_t nil;

    if ((nil = nil_alloc(error)) == NULL)
    {
	return NULL;
    }

    return cons_alloc(nil, self -> value_top[0], error);
}


/*
 * Allocates and initializes a new parser_t for esh's lisp-like
 * language.  This constructs a thread-safe parser which may be used
 * to convert input characters into lisp atoms and lists.  Whenever
 * the parser completes reading an s-expression it calls the callback
 * function.
 *
 * return values:
 *     success: a valid parser_t
 *     failure: NULL
 */
parser_t parser_alloc(parser_callback_t callback, void *rock, elvin_error_t error)
{
    parser_t self;

    /* Allocate memory for the new parser_t */
    if ((self = (parser_t)ELVIN_MALLOC(sizeof(struct parser), error)) == NULL)
    {
	return NULL;
    }

    /* Initialize sane state */
    self -> callback = callback;
    self -> rock = rock;
    self -> state_stack = NULL;
    self -> state_end = NULL;
    self -> state_top = NULL;
    self -> value_stack = NULL;
    self -> value_top = NULL;
    self -> state = lex_start;
    self -> token = NULL;
    self -> token_end = NULL;
    self -> point = NULL;
    self -> offset = 0;

    /* Allocate memory for the token buffer */
    if ((self -> token = (uchar *)ELVIN_MALLOC(INITIAL_BUFFER_SIZE, error)) == NULL)
    {
	parser_free(self, error);
	return NULL;
    }

    /* Allocate memory for the state stack */
    if ((self -> state_stack = (int *)ELVIN_CALLOC(
	INITIAL_STACK_SIZE, sizeof(int), error)) == NULL)
    {
	parser_free(self, error);
	return NULL;
    }

    /* Allocate memory for the value stack */
    if ((self -> value_stack = (atom_t *)ELVIN_CALLOC(
	INITIAL_STACK_SIZE, sizeof(atom_t), error)) == NULL)
    {
	parser_free(self, error);
	return NULL;
    }

    self -> token_end = self -> token + INITIAL_BUFFER_SIZE;

    self -> state_end = self -> state_stack + INITIAL_STACK_SIZE;
    self -> state_top = self -> state_stack;
    (*self -> state_top) = 0;

    self -> value_top = self -> value_stack;
    return self;
}

/* Frees the resources consumed by the parser */
int parser_free(parser_t self, elvin_error_t error)
{
    int result;

    if (self -> token != NULL)
    {
	result = ELVIN_FREE(self -> token, result ? error : NULL) && result;
    }

    if (self -> state_stack != NULL)
    {
	result = ELVIN_FREE(self -> state_stack, result ? error : NULL) && result;
    }

    if (self -> value_stack != NULL)
    {
	result = ELVIN_FREE(self -> value_stack, result ? error : NULL) && result;
    }

    return ELVIN_FREE(self, result ? error : NULL) && result;
}

/* The parser will read characters from `buffer' and use them generate 
 * lisp s-expressions.  Each time an s-expression is completed, the
 * parser's callback is called with that s-expression.  If an error is 
 * encountered, the buffer pointer will point to the character where
 * the error was first noticed (and the length will be updated
 * accordingly).  The parser does *not* reset its state   
 * between calls to parser_read_buffer(), so it is possible to
 * construct an s-expression which is much longer than the buffer size 
 * and it is also not necessary to ensure that a complete s-expression 
 * is in the buffer when this function is called.  The parser state
 * *is* reset whenever an error is encountered 
 *
 * return values:
 *     success: 0
 *     failure: -1 (and buffer will point to first error character)
 */
int parser_read_buffer(
    parser_t self,
    const char *buffer,
    ssize_t length,
    elvin_error_t error)
{
    const char *pointer;

    /* Watch for EOF */
    if (length == 0)
    {
	return self -> state(self, EOF, error);
    }

    /* Scan and parse them characters! */
    for (pointer = buffer; pointer < buffer + length; pointer++)
    {
	/* Send the next character into the parser and watch for errors */
	if (self -> state(self, *pointer, error) == 0)
	{
	    /* Error!  Clean up and reset the state */
	    clean_stack(self, error);
	    self -> state = lex_start;
	    self -> point = NULL;
	    return 0;
	}
    }

    return 1;
}
