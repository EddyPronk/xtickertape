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
static const char cvsid[] = "$Id: mbox_parser.c,v 1.5 1999/12/16 07:52:10 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "mbox_parser.h"

/* The type of a lexer state */
typedef int (*lexer_state_t)(mbox_parser_t self, int ch);


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

    /* The buffer in which the `name' string is constructed */
    char *name;

    /* The next free character in the `name' buffer */
    char *name_pointer;

    /* The buffer in which the `email' string is constructed */
    char *email;

    /* The next free character in the `email' buffer */
    char *email_pointer;
};


/* Static function declarations */
static int lex_start(mbox_parser_t self, int ch);
static int lex_white(mbox_parser_t self, int ch);
static int lex_string(mbox_parser_t self, int ch);
static int lex_string_esc(mbox_parser_t self, int ch);
static int lex_comment(mbox_parser_t self, int ch);
static int lex_comment_esc(mbox_parser_t self, int ch);
static int lex_ignored_comment(mbox_parser_t self, int ch);
static int lex_ignored_comment_esc(mbox_parser_t self, int ch);
static int lex_domain_lit(mbox_parser_t self, int ch);
static int lex_domain_lit_esc(mbox_parser_t self, int ch);

/* Awaiting the first character of a token*/
static int lex_start(mbox_parser_t self, int ch)
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
	    /* If we don't already have a name then use the comment as the name */
	    if (*self -> name_pointer == '\0')
	    {
		self -> lexer_state = lex_comment;
		return 0;
	    }

	    /* Otherwise throw the comment away */
	    self -> lexer_state = lex_ignored_comment;
	    return 0;
	}

	/* Watch for a domain-literal */
	case '[':
	{
	    *(self -> email_pointer++) = ch;
	    self -> lexer_state = lex_domain_lit;
	    return 0;
	}

	/* Watch for the start of a route-addr */
	case '<':
	{
	    char *pointer;

	    /* Don't allow multiple route-addrs */
	    if (self -> parser_state != PRE_ROUTE_ADDR)
	    {
		return -1;
	    }

	    /* We're looking at a `name <address>' format mailbox */
	    *self -> email_pointer = '\0';

	    /* Exchange the email and name buffers */
	    pointer = self -> name;
	    self -> name = self -> email;
	    self -> email = pointer;
	    self -> email_pointer = self -> name_pointer;

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

	    /* Null-terminate the email string */
	    *self -> email_pointer = '\0';
	    self -> parser_state = POST_ROUTE_ADDR;
	    return 0;
	}
    }

    /* Compress whitespace */
    if (isspace(ch))
    {
	self -> lexer_state = lex_white;
	return 0;
    }

    /* Copy anything else */
    *(self -> email_pointer++) = ch;
    return 0;
}

/* Skipping additional whitespace */
static int lex_white(mbox_parser_t self, int ch)
{
    switch (ch)
    {
	/* Watch for a quoted string */
	case '"':
	{
	    *(self -> email_pointer++) = ' ';
	    self -> lexer_state = lex_string;
	    return 0;
	}

	/* Watch for a comment */
	case '(':
	{
	    /* If we don't already have a name then use the comment */
	    if (*self -> name == '\0')
	    {
		self -> lexer_state = lex_comment;
		return 0;
	    }

	    /* Otherwise throw the comment away */
	    self -> lexer_state = lex_ignored_comment;
	    return 0;
	}

	/* Watch for a domain-literal */
	case '[':
	{
	    *(self -> email_pointer++) = ' ';
	    *(self -> email_pointer++) = ch;
	    self -> lexer_state = lex_domain_lit;
	    return 0;
	}

	/* Watch for the start of a route-addr */
	case '<':
	{
	    char *pointer;

	    /* Don't allow multiple route-addrs */
	    if (self -> parser_state != PRE_ROUTE_ADDR)
	    {
		return -1;
	    }

	    /* We're looking a a `name <address>' format mailbox */
	    *self -> email_pointer = '\0';

	    /* Exchange the email and name buffers */
	    pointer = self -> name;
	    self -> name = self -> email;
	    self -> email = pointer;
	    self -> email_pointer = self -> name_pointer;

	    self -> lexer_state = lex_start;
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

	    /* Null-terminate the email string */
	    *self -> email_pointer = '\0';

	    /* No longer skipping whitespace... */
	    self -> lexer_state = lex_start;
	    self -> parser_state = POST_ROUTE_ADDR;
	    return 0;
	}
    }

    /* Ignore additional whitespace */
    if (isspace(ch))
    {
	return 0;
    }

    /* Anything else gets a space in front of it */
    *(self -> email_pointer++) = ' ';
    *(self -> email_pointer++) = ch;
    self -> lexer_state = lex_start;
    return 0;
}

/* Awaiting characters within the body of a string */
static int lex_string(mbox_parser_t self, int ch)
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
	    *(self -> email_pointer++) = ch;
	    return 0;
	}
    }
}


/* Awaiting the character after a string escape */
static int lex_string_esc(mbox_parser_t self, int ch)
{
    *(self -> email_pointer++) = ch;
    self -> lexer_state = lex_string;
    return 0;
}

/* Awaiting a character within the body of a comment */
static int lex_comment(mbox_parser_t self, int ch)
{
    switch (ch)
    {
	/* End of comment? */
	case ')':
	{
	    *self -> name_pointer = '\0';
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
	    *(self -> name_pointer++) = ch;
	    return 0;
	}
    }
}

/* Awaiting an escaped comment character */
static int lex_comment_esc(mbox_parser_t self, int ch)
{
    *(self -> name_pointer++) = ch;
    self -> lexer_state = lex_string;
    return 0;
}

/* Awaiting a character within the body of a comment */
static int lex_ignored_comment(mbox_parser_t self, int ch)
{
    switch (ch)
    {
	/* end of comment? */
	case ')':
	{
	    self -> lexer_state = lex_start;
	    return 0;
	}

	/* escape character? */
	case '\\':
	{
	    self -> lexer_state = lex_ignored_comment_esc;
	    return 0;
	}

	/* bogus character? */
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
static int lex_ignored_comment_esc(mbox_parser_t self, int ch)
{
    self -> lexer_state = lex_ignored_comment;
    return 0;
}

/* Awaiting a character within a domain-literal */
static int lex_domain_lit(mbox_parser_t self, int ch)
{
    switch (ch)
    {
	/* End of domain literal? */
	case ']':
	{
	    *(self -> email_pointer++) = ch;
	    self -> lexer_state = lex_start;
	    return 0;
	}

	/* Escape character? */
	case '\\':
	{
	    *(self -> email_pointer++) = ch;
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
	    *(self -> email_pointer++) = ch;
	    return 0;
	}
    }
}

/* Awaiting an escaped domain-literal characer */
static int lex_domain_lit_esc(mbox_parser_t self, int ch)
{
    *(self -> email_pointer++) = ch;
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
    self -> name = NULL;
    self -> email = NULL;
    return self;
}

/* Frees the resources consumed by the receiver */
void mbox_parser_free(mbox_parser_t self)
{
    free(self);
}

#ifdef DEBUG
/* Prints out debugging information about the receiver */
void mbox_parser_debug(mbox_parser_t self, FILE *out)
{
    fprintf(out, "mbox_parser_t %p\n", self);
    fprintf(out, "  lexer_state = %p\n", self -> lexer_state);
    fprintf(out, "  parser_state = %d\n", self -> parser_state);
}
#endif /* DEBUG */

/* Parses `mailbox' as an RFC 822 mailbox, separating out the name and
 * e-mail address portions which can subsequently be accessed with the 
 * mbox_parser_get_name() and mbox_parser_get_email() functions.
 *
 * return values:
 *     success: 0
 *     failure: -1
 */
int mbox_parser_parse(mbox_parser_t self, char *mailbox)
{
    int length;
    char *pointer;

    /* Make sure we have enough room in our buffers */
    length = strlen(mailbox) + 1;

    self -> name = (char *)realloc(self -> name, length);
    self -> name_pointer = self -> name;
    *self -> name = '\0';

    self -> email = (char *)realloc(self -> email, length);
    self -> email_pointer = self -> email;
    *self -> email = '\0';

    /* Initialize the receiver's state */
    self -> lexer_state = lex_start;
    self -> parser_state = PRE_ROUTE_ADDR;

    /* Send each character to the lexer */
    for (pointer = mailbox; *pointer != '\0'; pointer++)
    {
	if ((self -> lexer_state)(self, *pointer) < 0)
	{
	    return -1;
	}
    }

    /* Must end in the start state (oddly enough) */
    if (self -> lexer_state != lex_start)
    {
	return -1;
    }

    /* Make sure the email address is null-terminated */
    *self -> email_pointer = '\0';

    return 0;
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
