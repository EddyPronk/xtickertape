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
static const char cvsid[] = "$Id: groups_parser.c,v 1.1 1999/10/02 13:28:39 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "groups_parser.h"

#define INITIAL_TOKEN_SIZE 64
#define BUFFER_SIZE 2048

/* The type of a lexer state */
typedef int (*lexer_state_t)(groups_parser_t self, int ch);

/* The structure of a groups_parser */
struct groups_parser
{
    /* The buffer in which the token is assembled */
    char *token;

    /* The next character in the token */
    char *token_pointer;

    /* The end of the token buffer */
    char *token_end;

    /* The tag for error messages */
    char *tag;

    /* The list of subscriptions */
    Subscription *subscriptions;

    /* The number of subscriptions thus far */
    int subscription_count;

    /* The current lexical state */
    lexer_state_t lexer_state;

    /* The current line-number within the file */
    int line_num;

    /* The name of the current group */
    char *name;

    /* Non-zero if the group should be in the menu */
    int in_menu;

    /* Non-zero if the group has the auto-mime option set */
    int has_nazi;

    /* The minimum timeout value for the current group */
    int min_time;

    /* The maximum timeout value for the current group */
    int max_time;
};


/* Lexer state declarations */
static int lex_start(groups_parser_t self, int ch);
static int lex_comment(groups_parser_t self, int ch);
static int lex_name(groups_parser_t self, int ch);
static int lex_name_esc(groups_parser_t self, int ch);
static int lex_menu(groups_parser_t self, int ch);
static int lex_nazi(groups_parser_t self, int ch);
static int lex_min_time(groups_parser_t self, int ch);
static int lex_max_time(groups_parser_t self, int ch);
static int lex_bad_time(groups_parser_t self, int ch);
static int lex_superfluous(groups_parser_t self, int ch);


/* Constructs a subscription from the given information and appends it
 * to the end of the array */
static int accept_subscription(groups_parser_t self)
{
    Subscription subscription = Subscription_alloc(
	self -> name,
	"HELLO DOLLY",
	self -> in_menu,
	self -> has_nazi,
	self -> min_time,
	self -> max_time,
	NULL,
	NULL);

    Subscription_debug(subscription);
    return 0;
}

/* Prints a consistent error message */
static void parse_error(groups_parser_t self, char *message)
{
    fprintf(stderr, "%s: parse error line %d: %s\n", self -> tag, self -> line_num, message);
}

/* Appends a character to the end of the token */
static int append_char(groups_parser_t self, int ch)
{
    /* Grow the token buffer if necessary */
    if (! (self -> token_pointer < self -> token_end))
    {
	char *new_token;
	size_t length = (self -> token_end - self -> token) * 2;

	/* Try to allocate more memory */
	if ((new_token = (char *)realloc(self -> token, length)) == NULL)
	{
	    return -1;
	}

	/* Update the other pointers */
	self -> token_pointer = new_token + (self -> token_pointer - self -> token);
	self -> token = new_token;
	self -> token_end = self -> token + length;
    }

    *(self -> token_pointer++) = ch;
    return 0;
}

/* Awaiting the first character of a group subscription */
static int lex_start(groups_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	printf("<EOF>\n");
	return 0;
    }

    /* Watch for comments */
    if (ch == '#')
    {
	self -> lexer_state = lex_comment;
	return 0;
    }

    /* Watch for blank lines */
    if (ch == '\n')
    {
	self -> lexer_state = lex_start;
	self -> line_num++;
	return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch))
    {
	self -> lexer_state = lex_start;
	return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\')
    {
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_name_esc;
	return 0;
    }

    /* Watch for a `:' (for the empty group?) */
    if (ch == ':')
    {
	self -> name = strdup("");
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_menu;
	return 0;
    }


    /* Anything else is part of the group name */
    self -> token_pointer = self -> token;
    self -> lexer_state = lex_name;
    return append_char(self, ch);
}

/* Reading a comment (to end-of-line) */
static int lex_comment(groups_parser_t self, int ch)
{
    /* Watch for end-of-file */
    if (ch == EOF)
    {
	return lex_start(self, ch);
    }

    /* Watch for end-of-line */
    if (ch == '\n')
    {
	self -> lexer_state = lex_start;
	self -> line_num++;
	return 0;
    }

    /* Ignore everything else */
    return 0;
}

/* Reading characters in the group name */
static int lex_name(groups_parser_t self, int ch)
{
    /* EOF is an error */
    if (ch == EOF)
    {
	parse_error(self, "unexpected end of file");
	return -1;
    }

    /* Linefeed is an error */
    if (ch == '\n')
    {
	parse_error(self, "unexpected end of line");
	return -1;
    }

    /* Watch for the magical escape character */
    if (ch == '\\')
    {
	self -> lexer_state = lex_name_esc;
	return 0;
    }

    /* Watch for the end of the name */
    if (ch == ':')
    {
	/* Null-terminate the name */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	self -> name = strdup(self -> token);
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_menu;
	return 0;
    }

    /* Anything else is part of the group name */
    return append_char(self, ch);
}

/* Reading an escaped character in the group name */
static int lex_name_esc(groups_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	parse_error(self, "unexpected end of file");
	return -1;
    }

    /* Linefeeds are ignored */
    if (ch == '\n')
    {
	self -> lexer_state = lex_name;
	return 0;
    }

    /* Anything else is part of the name */
    self -> lexer_state = lex_name;
    return append_char(self, ch);
}

/* Reading the menu/no menu option */
static int lex_menu(groups_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	parse_error(self, "unexpected end of file");
	return -1;
    }

    /* Watch for end of line */
    if (ch == '\n')
    {
	parse_error(self, "unexpected end of line");
	return -1;
    }

    /* Watch for the end of the menu token */
    if (ch == ':')
    {
	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Make sure the token is either "menu" or "no menu" */
	if (strcasecmp(self -> token, "menu") == 0)
	{
	    self -> in_menu = 1;
	}
	else if (strcasecmp(self -> token, "no menu") == 0)
	{
	    self -> in_menu = 0;
	}
	else
	{
	    char *buffer = (char *)malloc(
		sizeof("expecting \"menu\" or \"no menu\", got \"\"") + strlen(self -> token));
	    sprintf(buffer, "expecting \"menu\" or \"no menu\", got \"%s\"", self -> token);
	    parse_error(self, buffer);
	    free(buffer);
	    return -1;
	}

	/* Move along to the mime nazi option */
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_nazi;
	return 0;
    }

    /* Anything else is part of the token */
    return append_char(self, ch);
}

/* Reading the auto-mime option */
static int lex_nazi(groups_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	parse_error(self, "unexpected end of file");
	return -1;
    }

    /* Watch for end of line */
    if (ch == '\n')
    {
	parse_error(self, "unexpected end of line");
	return -1;
    }

    /* Watch for the end of the token */
    if (ch == ':')
    {
	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Make sure the token is either "auto" or "manual" */
	if (strcasecmp(self -> token, "auto") == 0)
	{
	    self -> has_nazi = 1;
	}
	else if (strcasecmp(self -> token, "manual") == 0)
	{
	    self -> has_nazi = 0;
	}
	else
	{
	    char *buffer = (char *)malloc(
		sizeof("expecting \"auto\" or \"manual\", got \"\"") + strlen(self -> token));
	    sprintf(buffer, "expecting \"auto\" or \"manual\", got \"%s\"", self -> token);
	    parse_error(self, buffer);
	    free(buffer);
	    return -1;
	}

	/* Move along to the minimum-time token */
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_min_time;
	return 0;
    }

    /* Anything else is part of the token */
    return append_char(self, ch);
}

/* Reading the minimum timeout token */
static int lex_min_time(groups_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	parse_error(self, "unexpected end of file");
	return -1;
    }

    /* Watch for end of line */
    if (ch == '\n')
    {
	parse_error(self, "unexpected end of line");
	return -1;
    }

    /* Watch for the end of the token */
    if (ch == ':')
    {
	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Read the min-time from the token */
	self -> min_time = atoi(self -> token);
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_max_time;
	return 0;
    }

    /* Make sure we keep getting digits */
    if (isdigit(ch))
    {
	return append_char(self, ch);
    }

    /* Otherwise we go to the error state for a bit */
    self -> lexer_state = lex_bad_time;
    return append_char(self, ch);
}

/* Reading the maximum timeout token */
static int lex_max_time(groups_parser_t self, int ch)
{
    /* Watch for end of token */
    if ((ch == EOF) || (ch == '\n'))
    {
	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Determine the max timeout */
	if (*self -> token == '\0')
	{
	    self -> max_time = -1;
	}
	else
	{
	    self -> max_time = atoi(self -> token);
	}

	/* Construct a subscription and carry on */
	if (accept_subscription(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next subscription */
	return lex_start(self, ch);
    }

    /* Everything else should be a digit */
    if (isdigit(ch))
    {
	return append_char(self, ch);
    }

    /* If we get a ':' then indicate that there's extra stuff on the line */
    if (ch == ':')
    {
	self -> token_pointer = self -> token;
	self -> lexer_state = lex_superfluous;
	return append_char(self, ch);
    }

    /* Otherwise we go to the error state for a bit */
    self -> lexer_state = lex_bad_time;
    return append_char(self, ch);
}


/* Reading min or max time after a non-digit */
static int lex_bad_time(groups_parser_t self, int ch)
{
    /* Watch for EOF, end of line or ':' as end of token */
    if ((ch == EOF) || (ch == '\n') || (ch == ':'))
    {
	char *buffer;

	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Generate an error message */
	buffer = (char *)malloc(sizeof("illegal timeout value \"\"") + strlen(self -> token));
	sprintf(buffer, "illegal timeout value \"%s\"", self -> token);
	parse_error(self, buffer);
	free(buffer);
	return -1;
    }

    /* Append additional characters to the token for context */
    return append_char(self, ch);
}

/* Reading extraneous stuff at the end of the line */
static int lex_superfluous(groups_parser_t self, int ch)
{
    /* Watch for EOF or linefeed */
    if ((ch == EOF) || (ch == '\n'))
    {
	char *buffer;

	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Generate an error message */
	buffer = (char *)malloc(sizeof("superfluous characters: \"\"") + strlen(self -> token));
	sprintf(buffer, "superfluous characters: \"%s\"", self -> token);
	parse_error(self, buffer);
	free(buffer);
	return -1;
    }

    /* Append additional characters to the token for context */
    return append_char(self, ch);
}




/* Allocates and initializes a new groups file parser */
groups_parser_t groups_parser_alloc()
{
    groups_parser_t self;

    /* Allocate memory for the new groups_parser */
    if ((self = (groups_parser_t)malloc(sizeof(struct groups_parser))) == NULL)
    {
	return NULL;
    }

    /* Allocate room for the token buffer */
    if ((self -> token = (char *)malloc(INITIAL_TOKEN_SIZE)) == NULL)
    {
	free(self);
	return NULL;
    }

    /* Initialize everything else to sane values */
    self -> token_pointer = self -> token;
    self -> token_end = self -> token + INITIAL_TOKEN_SIZE;
    self -> lexer_state = lex_start;
    self -> subscriptions = NULL;
    self -> subscription_count = 0;
    self -> line_num = 1;
    return self;
}

/* Frees the resources consumed by the receiver */
void groups_parser_free(groups_parser_t self)
{
    free(self -> token);
    free(self);
}

/* Parses the given file into an array of subscriptions */
int groups_parser_parse(groups_parser_t self, char *tag, char *buffer, int length)
{
    char *pointer;

    /* Initialize the receiver's state to do some parsing */
    self -> tag = tag;

    /* Parse the buffer */
    for (pointer = buffer; pointer < buffer + length; pointer++)
    {
	if ((self -> lexer_state)(self, *pointer) < 0)
	{
	    return -1;
	}
    }

    return 0;
}

/* Answers the subscriptions that we've parsed (or NULL if an error occurred */
Subscription *groups_parser_get_subscriptions(groups_parser_t self)
{
    /* Send an EOF through the lexer to clean up */
    if ((self -> lexer_state)(self, EOF) < 0)
    {
	return NULL;
    }

    /* Return the resulting array */
    self -> subscriptions = NULL;
    self -> subscription_count = 0;
    return NULL;
}
