/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2000.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
static const char cvsid[] = "$Id: groups_parser.c,v 1.17 2001/08/25 14:04:43 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <elvin/elvin.h>
#include "groups_parser.h"

#define INITIAL_TOKEN_SIZE 64

#define MENU_ERROR_MSG "expecting `menu' or `no menu', got `%s'"
#define NAZI_ERROR_MSG "expecting `auto' or `manual', got `%s'"
#define TIMEOUT_ERROR_MSG "illegal timeout value `%s'"
#define PRODUCER_KEYS_MSG "illegal producer key: `%s'"
#define EXTRA_ERROR_MSG "superfluous characters: `%s'"


/* The type of a lexer state */
typedef int (*lexer_state_t)(groups_parser_t self, int ch);

/* The structure of a groups_parser */
struct groups_parser
{
    /* The callback for when we've completed a subscription entry */
    groups_parser_callback_t callback;

    /* The client data for the callback */
    void *rock;

    /* The tag for error messages */
    char *tag;

    /* The buffer in which the token is assembled */
    char *token;

    /* The next character in the token */
    char *token_pointer;

    /* The end of the token buffer */
    char *token_end;

    /* The current lexical state */
    lexer_state_t state;

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

    /* The producer keys */
    elvin_keys_t producer_keys;

    /* The consumer keys */
    elvin_keys_t consumer_keys;
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
static int lex_producer_keys_ws(groups_parser_t self, int ch);
static int lex_producer_keys(groups_parser_t self, int ch);
static int lex_producer_keys_esc(groups_parser_t self, int ch);
static int lex_consumer_keys_ws(groups_parser_t self, int ch);
static int lex_consumer_keys(groups_parser_t self, int ch);
static int lex_consumer_keys_esc(groups_parser_t self, int ch);
static int lex_superfluous(groups_parser_t self, int ch);


/* Calls the callback with the given information */
static int accept_subscription(groups_parser_t self)
{
    int result;

    /* Make sure there is a callback */
    if (self -> callback == NULL)
    {
	return 0;
    }

    /* Call it with the information */
    result = (self -> callback)(
	self -> rock, self -> name,
	self -> in_menu, self -> has_nazi,
	self -> min_time, self -> max_time,
	self -> producer_keys,
	self -> consumer_keys);

    /* Clean up */
    if (self -> producer_keys != NULL)
    {
	elvin_keys_free(self -> producer_keys, NULL);
	self -> producer_keys = NULL;
    }

    if (self -> consumer_keys != NULL)
    {
	elvin_keys_free(self -> consumer_keys, NULL);
	self -> consumer_keys = NULL;
    }

    free(self -> name);
    return result;
}

/* Adds a producer key to the current list */
static int accept_producer_key(groups_parser_t self, char *string)
{
#if 0
    elvin_key_t key;

    /* Make sure we have a key set */
    if (self -> producer_keys == NULL)
    {
	if ((self -> producer_keys = elvin_keys_alloc(7, NULL)) == NULL)
	{
	    abort();
	}
    }

    /* Create our producer key */
    if ((key = elvin_keyraw_alloc((uchar *)string, strlen(string), NULL)) == NULL)
    {
	abort();
    }

    /* Add it to the keys */
    if (elvin_keys_add(self -> producer_keys, key, NULL) == 0)
    {
	abort();
    }
#endif

    return 0;
}

/* Adds a consumer key to the current list */
static int accept_consumer_key(groups_parser_t self, char *string)
{
#if 0
    elvin_key_t key;

    /* Make sure we have a key set */
    if (self -> consumer_keys == NULL)
    {
	if ((self -> consumer_keys = elvin_keys_alloc(7, NULL)) == NULL)
	{
	    abort();
	}
    }

    /* Create our consumer key */
    if ((key = elvin_keyprime_from_hexstring(string, NULL)) == NULL)
    {
	abort();
    }

    /* Add it to the keys */
    if (elvin_keys_add(self -> consumer_keys, key, NULL) == 0)
    {
	abort();
    }
#endif
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
	self -> state = lex_start;
	return 0;
    }

    /* Watch for comments */
    if (ch == '#')
    {
	self -> state = lex_comment;
	return 0;
    }

    /* Watch for blank lines */
    if (ch == '\n')
    {
	self -> state = lex_start;
	return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch))
    {
	self -> state = lex_start;
	return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\')
    {
	self -> token_pointer = self -> token;
	self -> state = lex_name_esc;
	return 0;
    }

    /* Watch for a `:' (for the empty group?) */
    if (ch == ':')
    {
	self -> name = strdup("");
	self -> token_pointer = self -> token;
	self -> state = lex_menu;
	return 0;
    }


    /* Anything else is part of the group name */
    self -> token_pointer = self -> token;
    self -> state = lex_name;
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
	self -> state = lex_start;
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
	self -> state = lex_name_esc;
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
	self -> state = lex_menu;
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
	self -> state = lex_name;
	return 0;
    }

    /* Anything else is part of the name */
    self -> state = lex_name;
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
	if (ELVIN_STRCASECMP(self -> token, "menu") == 0)
	{
	    self -> in_menu = 1;
	}
	else if (ELVIN_STRCASECMP(self -> token, "no menu") == 0)
	{
	    self -> in_menu = 0;
	}
	else
	{
	    char *buffer = (char *)malloc(strlen(MENU_ERROR_MSG) + strlen(self -> token) - 1);
	    if (buffer != NULL)
	    {
		sprintf(buffer, MENU_ERROR_MSG, self -> token);
		parse_error(self, buffer);
		free(buffer);
	    }

	    return -1;
	}

	/* Move along to the mime nazi option */
	self -> token_pointer = self -> token;
	self -> state = lex_nazi;
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
	if (ELVIN_STRCASECMP(self -> token, "auto") == 0)
	{
	    self -> has_nazi = 1;
	}
	else if (ELVIN_STRCASECMP(self -> token, "manual") == 0)
	{
	    self -> has_nazi = 0;
	}
	else
	{
	    char *buffer = (char*)malloc(strlen(NAZI_ERROR_MSG) + strlen(self -> token) - 1);

	    if (buffer != NULL)
	    {
		sprintf(buffer, NAZI_ERROR_MSG, self -> token);
		parse_error(self, buffer);
		free(buffer);
	    }
	    return -1;
	}

	/* Move along to the minimum-time token */
	self -> token_pointer = self -> token;
	self -> state = lex_min_time;
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
	self -> state = lex_max_time;
	return 0;
    }

    /* Make sure we keep getting digits */
    if (isdigit(ch))
    {
	return append_char(self, ch);
    }

    /* Otherwise we go to the error state for a bit */
    self -> state = lex_bad_time;
    return append_char(self, ch);
}

/* Reading the maximum timeout token */
static int lex_max_time(groups_parser_t self, int ch)
{
    /* Watch for end of token */
    if ((ch == EOF) || (ch == '\n'))
    {
	/* Null-terminate the token */
	if (append_char(self, 0) < 0)
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

    /* If we get a `:' then look for producer (raw) keys */
    if (ch == ':')
    {
	/* Null-terminate the token */
	if (append_char(self, 0) < 0)
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

	/* Prepare to read a key */
	self -> token_pointer = self -> token;
	self -> state = lex_producer_keys_ws;
	return 0;
    }

    /* Otherwise we go to the error state for a bit */
    self -> state = lex_bad_time;
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
	buffer = (char *)malloc(strlen(TIMEOUT_ERROR_MSG) + strlen(self -> token) - 1);
	if (buffer != NULL)
	{
	    sprintf(buffer, TIMEOUT_ERROR_MSG, self -> token);
	    parse_error(self, buffer);
	    free(buffer);
	}

	return -1;
    }

    /* Append additional characters to the token for context */
    return append_char(self, ch);
}


/* Skipping whitespace in a producer key */
static int lex_producer_keys_ws(groups_parser_t self, int ch)
{
    /* Watch for the end of the line */
    if (ch == EOF || ch == '\n')
    {
	/* Accept the subscription */
	if (accept_subscription(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next subscription */
	return lex_start(self, ch);
    }

    /* Throw away whitespace */
    if (isspace(ch))
    {
	self -> state = lex_producer_keys_ws;
	return 0;
    }

    /* Watch for a `:' */
    if (ch == ':')
    {
	self -> token_pointer = self -> token;
	self -> state = lex_consumer_keys_ws;
	return 0;
    }

    /* Send anything else through the producer keys state */
    return lex_producer_keys(self, ch);
}


/* Reading the producer (raw) keys */
static int lex_producer_keys(groups_parser_t self, int ch)
{
    /* Watch for EOF or linefeed */
    if (ch == EOF || ch == '\n')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

	/* Accept it */
	if (accept_producer_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Accept the subscription */
	if (accept_subscription(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next subscription */
	return lex_start(self, ch);
    }

    /* Watch for `:' */
    if (ch == ':')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

	/* Accept it */
	if (accept_producer_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Prepare to read consumer keys */
	self -> token_pointer = self -> token;
	self -> state = lex_consumer_keys;
	return 0;
    }

    /* Watch for `,' */
    if (ch == ',')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

	/* Accept it */
	if (accept_producer_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Get set up for the next key */
	self -> token_pointer = self -> token;
	self -> state = lex_producer_keys_ws;
	return 0;
    }

    /* Watch for an esc-char */
    if (ch == '\\')
    {
	self -> state = lex_producer_keys_esc;
	return 0;
    }

    /* Anything else is part of the key */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_producer_keys;
    return 0;
}

/* Reading the character after a `\' in a key */
static int lex_producer_keys_esc(groups_parser_t self, int ch)
{
    /* We can't escape an EOF */
    if (ch == EOF)
    {
	/* Pretend we never saw the backslash */
	return lex_producer_keys(self, ch);
    }

    /* Watch for an end-of-line */
    if (ch == '\n')
    {
	self -> state = lex_producer_keys_ws;
	return 0;
    }

    /* Anything else is the character itself? */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_producer_keys;
    return 0;
}


/* Skipping whitespace in a consumer key */
static int lex_consumer_keys_ws(groups_parser_t self, int ch)
{
    /* Watch for the end of the line */
    if (ch == EOF || ch == '\n')
    {
	/* Accept the subscription */
	if (accept_subscription(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next subscription */
	return lex_start(self, ch);
    }

    /* Throw away whitespace */
    if (isspace(ch))
    {
	self -> state = lex_consumer_keys_ws;
	return 0;
    }

    /* Watch for yet another `:' */
    if (ch == ':')
    {
	self -> token_pointer = self -> token;
	return lex_superfluous(self, ch);
    }

    /* Send anything else through the consumer keys state */
    return lex_consumer_keys(self, ch);
}

/* Reading the consumer (prime) keys */
static int lex_consumer_keys(groups_parser_t self, int ch)
{
    /* Watch for EOF or linefeed */
    if (ch == EOF || ch == '\n')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

	/* Accept it */
	if (accept_consumer_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Accept the subscription */
	if (accept_subscription(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next subscription */
	return lex_start(self, ch);
    }

    /* Watch for a bogus `:' */
    if (ch == ':')
    {
	self -> token_pointer = self -> token;
	return lex_superfluous(self, ch);
    }

    /* Watch for `,' */
    if (ch == ',')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

	/* Accept it */
	if (accept_consumer_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Set up for the next key */
	self -> token_pointer = self -> token;
	self -> state = lex_consumer_keys_ws;
	return 0;
    }

    /* Watch for escapes */
    if (ch == '\\')
    {
	self -> state = lex_consumer_keys_esc;
	return 0;
    }

    /* Anything else is part of the key */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_consumer_keys;
    return 0;
}

/* Reading an escaped character in a consumer key */
static int lex_consumer_keys_esc(groups_parser_t self, int ch)
{
    /* We can't escape an EOF */
    if (ch == EOF)
    {
	/* Pretend we never saw the backslash */
	return lex_consumer_keys(self, ch);
    }

    /* Watch for an end-of-line */
    if (ch == '\n')
    {
	self -> state = lex_consumer_keys_ws;
	return 0;
    }

    /* Anything else is the character itself? */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_consumer_keys;
    return 0;
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
	buffer = (char *)malloc(strlen(EXTRA_ERROR_MSG) + strlen(self -> token) - 1);
	if (buffer != NULL)
	{
	    sprintf(buffer, EXTRA_ERROR_MSG, self -> token);
	    parse_error(self, buffer);
	    free(buffer);
	}

	return -1;
    }

    /* Append additional characters to the token for context */
    self -> state = lex_superfluous;
    return append_char(self, ch);
}

/* Parses the given character, ignoring CRs and counting LFs */
static int parse_char(groups_parser_t self, int ch)
{
    /* Ignore CRs */
    if (ch == '\r')
    {
	return 0;
    }

    /* Parse the character */
    if (self -> state(self, ch) < 0)
    {
	return -1;
    }

    /* Count LFs */
    if (ch == '\n')
    {
	self -> line_num++;
    }

    return 0;
}




/* Allocates and initializes a new groups file parser */
groups_parser_t groups_parser_alloc(groups_parser_callback_t callback, void *rock, char *tag)
{
    groups_parser_t self;

    /* Allocate memory for the new groups_parser */
    if ((self = (groups_parser_t)malloc(sizeof(struct groups_parser))) == NULL)
    {
	return NULL;
    }

    /* Copy the tag string */
    if ((self -> tag = strdup(tag)) == NULL)
    {
	free(self);
	return NULL;
    }

    /* Allocate room for the token buffer */
    if ((self -> token = (char *)malloc(INITIAL_TOKEN_SIZE)) == NULL)
    {
	free(self -> tag);
	free(self);
	return NULL;
    }

    /* Initialize everything else to sane values */
    self -> producer_keys = NULL;
    self -> consumer_keys = NULL;
    self -> callback = callback;
    self -> rock = rock;
    self -> token_pointer = self -> token;
    self -> token_end = self -> token + INITIAL_TOKEN_SIZE;
    self -> state = lex_start;
    self -> line_num = 1;
    return self;
}

/* Frees the resources consumed by the receiver */
void groups_parser_free(groups_parser_t self)
{
    free(self -> tag);
    free(self -> token);
    free(self);
}

/* Parses the given buffer, calling callbacks for each subscription
 * expression that is successfully read.  A zero-length buffer is
 * interpreted as an end-of-input marker */
int groups_parser_parse(groups_parser_t self, char *buffer, size_t length)
{
    char *pointer;

    /* Length of 0 indicates EOF */
    if (length == 0)
    {
	return parse_char(self, EOF);
    }

    /* Parse the buffer */
    for (pointer = buffer; pointer < buffer + length; pointer++)
    {
	if (parse_char(self, *pointer) < 0)
	{
	    return -1;
	}
    }

    return 0;
}

