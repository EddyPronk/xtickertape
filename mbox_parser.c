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
static const char cvsid[] = "$Id: mbox_parser.c,v 1.1 1999/09/12 07:34:26 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "mbox_parser.h"

/* The type of a lexer state */
typedef int (*lexer_state_t)(mbox_parser_t self, char ch);


/* The type of a parser state */
typedef enum parser_state parser_state_t;
enum parser_state
{
    PRE_ROUTE_ADDR,
    ROUTE_ADDR,
    POST_ROUTE_ADDR
};

/* Instance variables of a mbox_parser */
struct mbox_parser
{
    /* The current lexical state */
    lexer_state_t lexer_state;

    /* The current parse state */
    parser_state_t parser_state;

    /* A pointer to the character being scanned at the moment */
    char *pointer;

    /* One of the strings */
    char *name;

    /* The end of name */
    char *end_name;

    /* The other string */
    char *email;

    /* The end of email */
    char *end_email;
};


/* Static function declarations */
static int lex_start(mbox_parser_t self, char ch);
static int lex_string(mbox_parser_t self, char ch);
static int lex_string_esc(mbox_parser_t self, char ch);
static int lex_comment(mbox_parser_t self, char ch);
static int lex_comment_esc(mbox_parser_t self, char ch);
static int lex_domain_lit(mbox_parser_t self, char ch);
static int lex_domain_lit_esc(mbox_parser_t self, char ch);

/* Awaiting the first character of a token*/
static int lex_start(mbox_parser_t self, char ch)
{
    switch (ch)
    {
	/* Watch for a quoted-string */
	case '"':
	{
	    self -> lexer_state = lex_string;
	    return 0;
	}

	/* Watch for a comment */
	case '(':
	{
	    if (self -> end_name == NULL)
	    {
		self -> end_name = self -> pointer;
	    }

	    self -> email = self -> pointer + 1;
	    self -> lexer_state = lex_comment;
	    return 0;
	}

	/* Watch for a domain-literal */
	case '[':
	{
	    self -> lexer_state = lex_domain_lit;
	    return 0;
	}

	/* Watch for the start of a route-addr */
	case '<':
	{
	    self -> end_name = self -> pointer;
	    self -> email = self -> pointer + 1;

	    if (self -> parser_state != PRE_ROUTE_ADDR)
	    {
		return -1;
	    }

	    self -> parser_state = ROUTE_ADDR;
	    return 0;
	}

	/* Watch for the end of a route-addr */
	case '>':
	{
	    if (self -> parser_state != ROUTE_ADDR)
	    {
		return -1;
	    }

	    self -> end_email = self -> pointer;
	    self -> parser_state = POST_ROUTE_ADDR;
	    return 0;
	}

	/* Let anything else through */
	default:
	{
	    return 0;
	}
    }
}

/* Awaiting characters within the body of a string */
static int lex_string(mbox_parser_t self, char ch)
{
    switch (ch)
    {
	/* End of string? */
	case '"':
	{
	    self -> lexer_state = lex_start;
	    return 0;
	}

	/* Escape character? */
	case '\\':
	{
	    self -> lexer_state = lex_string_esc;
	    return 0;
	}

	/* Let anything else through */
	default:
	{
	    return 0;
	}
    }
}


/* Awaiting the character after a string escape */
static int lex_string_esc(mbox_parser_t self, char ch)
{
    self -> lexer_state = lex_string;
    return 0;
}

/* Awaiting a character within the body of a comment */
static int lex_comment(mbox_parser_t self, char ch)
{
    switch (ch)
    {
	/* End of comment? */
	case ')':
	{
	    self -> end_email = self -> pointer;
	    self -> lexer_state = lex_start;
	    return 0;
	}

	/* Escape character */
	case '\\':
	{
	    self -> lexer_state = lex_comment_esc;
	    return 0;
	}

	/* Bogus comment character? */
	case '(':
	{
	    return -1;
	}

	/* Anything else is part of the comment */
	default:
	{
	    return 0;
	}
    }
}

/* Awaiting an escaped comment character */
static int lex_comment_esc(mbox_parser_t self, char ch)
{
    self -> lexer_state = lex_string;
    return 0;
}

/* Awaiting a character within a domain-literal */
static int lex_domain_lit(mbox_parser_t self, char ch)
{
    switch (ch)
    {
	/* End of domain literal? */
	case ']':
	{
	    self -> lexer_state = lex_start;
	    return 0;
	}

	/* Escape character? */
	case '\\':
	{
	    self -> lexer_state = lex_domain_lit_esc;
	    return 0;
	}

	/* Illegal character? */
	case '[':
	{
	    return -1;
	}

	/* Anything else is permitted */
	default:
	{
	    return 0;
	}
    }
}

/* Awaiting an escaped domain-literal characer */
static int lex_domain_lit_esc(mbox_parser_t self, char ch)
{
    self -> lexer_state = lex_domain_lit;
    return 0;
}




/*
 * Allocates a new RFC-822 mailbox parser.  This takes a strings as
 * input, which it parses as a mailbox (commented e-mail address) into 
 * e-mail address and user name portions
 *
 * return values:
 *     success: a valid mbox_parser_t
 *     failure: NULL
 */
mbox_parser_t mbox_parser_alloc()
{
    mbox_parser_t self;

    /* Allocate memory for the new mbox_parser */
    if ((self = (mbox_parser_t) malloc(sizeof(struct mbox_parser))) == NULL)
    {
	return NULL;
    }

    /* Initialize its state */	
    self -> lexer_state = lex_start;
    self -> parser_state = PRE_ROUTE_ADDR;
    return self;
}

/* Frees the resources consumed by the receiver */
void mbox_parser_free(mbox_parser_t self)
{
    free(self);
}

/* Prints out debugging information about the receiver */
void mbox_parser_debug(mbox_parser_t self, FILE *out)
{
    fprintf(out, "mbox_parser_t %p\n", self);
    fprintf(out, "  lexer_state = %p\n", self -> lexer_state);
    fprintf(out, "  parser_state = %d\n", self -> parser_state);
}

/* Parses the given string (THIS MODIFIES THE STRING), and returns
 * pointers to the e-mail address and user name portions
 *
 * return values:
 *     success: 0
 *     failure: -1
 */
int mbox_parser_parse(mbox_parser_t self, char *mailbox)
{
    /* Initialize the receiver's state */
    self -> lexer_state = lex_start;
    self -> parser_state = PRE_ROUTE_ADDR;
    self -> name = mailbox;
    self -> end_name = NULL;
    self -> email = NULL;
    self -> end_email = NULL;

    /* Send each character to the lexer */
    for (self -> pointer = mailbox; *self -> pointer != '\0'; self -> pointer++)
    {
	if ((self -> lexer_state)(self, *self -> pointer) < 0)
	{
	    return -1;
	}
    }

    /* Must end in the start state (oddly enough) */
    if (self -> lexer_state != lex_start)
    {
	return -1;
    }

    /* Trim whitespace off the end of the name */
    if (self -> end_name == NULL)
    {
	self -> end_name = self -> pointer;
    }

    while (isspace((int) *(self -> end_name - 1)))
    {
	self -> end_name--;
    }

    *self -> end_name = '\0';

    /* Determine what was the e-mail address and what was the name */
    switch (self -> parser_state)
    {
	case PRE_ROUTE_ADDR:
	{
	    char *pointer;

	    /* End the email string */
	    if (self -> email != NULL)
	    {
		*self -> end_email = '\0';
	    }

	    /* Swap the name and e-mail strings */
	    pointer = self -> email;
	    self -> email = self -> name;
	    self -> name = pointer;
	    return 0;
	}

	case POST_ROUTE_ADDR:
	{
	    *self -> end_email = '\0';
	    return 0;
	}

	default:
	{
	    self -> email = NULL;
	    self -> name = NULL;
	    return -1;
	}
    }
}

/*
 * Answers a pointer to the e-mail address (only valid after
 * a successful call to mbox_parser_parse).
 */
char *mbox_parser_get_email(mbox_parser_t self)
{
    return self -> email;
}

/*
 * Answers a pointer to the name (only valid after a successful call
 * to mbox_parser_parse)
 */
char *mbox_parser_get_name(mbox_parser_t self)
{
    return self -> name;
}
