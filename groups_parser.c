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
static const char cvsid[] = "$Id: groups_parser.c,v 1.19 2002/04/14 22:33:16 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <elvin/elvin.h>
#include <elvin/sha1.h>
#include "key_table.h"
#include "groups_parser.h"

#define INITIAL_TOKEN_SIZE 64

#define MENU_ERROR_MSG "expecting `menu' or `no menu', got `%s'"
#define NAZI_ERROR_MSG "expecting `auto' or `manual', got `%s'"
#define TIMEOUT_ERROR_MSG "illegal timeout value `%s'"
#define PRODUCER_KEYS_MSG "illegal producer key: `%s'"
#define EXTRA_ERROR_MSG "superfluous characters: `%s'"


/* libvin 4.0 has a poorly named macro */
#if ! defined(ELVIN_SHA1DIGESTLEN)
#define ELVIN_SHA1DIGESTLEN SHA1DIGESTLEN
#endif

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

    /* The table of keys to use */
    key_table_t key_table;

    /* The number of private keys */
    int private_key_count;

    /* The private keys */
    char **private_keys;

    /* The length of each private key */
    int *private_key_lengths;

    /* The number of public keys */
    int public_key_count;

    /* The public keys */
    char **public_keys;

    /* The length of each public key */
    int *public_key_lengths;
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
static int lex_keys_ws(groups_parser_t self, int ch);
static int lex_keys(groups_parser_t self, int ch);
static int lex_superfluous(groups_parser_t self, int ch);


/* Calls the callback with the given information */
static int accept_subscription(groups_parser_t self)
{
    int result;
    int i;
    elvin_keys_t notification_keys = NULL;
    elvin_keys_t subscription_keys = NULL;

    /* Make sure there is a callback */
    if (self -> callback == NULL)
    {
	return 0;
    }

    /* If there are keys then make a key block */
    if (self -> private_key_count != 0 || self -> public_key_count != 0)
    {
	/* Construct a notification key block */
	if ((notification_keys = elvin_keys_alloc(NULL)) == NULL)
	{
	    abort();
	}

	/* Construct a subscription key block */
	if ((subscription_keys = elvin_keys_alloc(NULL)) == NULL)
	{
	    abort();
	}

	/* Add the private keys to the notification block */
	for (i = 0; i < self -> private_key_count; i++)
	{
	    if (self -> private_key_count != 0)
	    {
		/* SHA1 dual */
		if (! elvin_keys_add(notification_keys,
				     ELVIN_KEY_SCHEME_SHA1_DUAL,
				     ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
				     self -> private_keys[i],
				     self -> private_key_lengths[i],
				     NULL))
		{
		    return -1;
		}
	    }

	    /* SHA1 consumer */
	    if (! elvin_keys_add(notification_keys,
				 ELVIN_KEY_SCHEME_SHA1_CONSUMER,
				 ELVIN_KEY_SHA1_CONSUMER_INDEX,
				 self -> public_keys[i],
				 self -> public_key_lengths[i],
				 NULL))
	    {
		return -1;
	    }
	}

	/* Add the public keys to the notification key blcok */
	for (i = 0; i < self -> public_key_count; i++)
	{
	    /* SHA1 dual */
	    if (! elvin_keys_add(notification_keys,
				 ELVIN_KEY_SCHEME_SHA1_DUAL,
				 ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
				 self -> public_keys[i],
				 self -> public_key_lengths[i],
				 NULL))
	    {
		return -1;
	    }
	}

	/* Add the public keys to the subscription key block */
	for (i = 0; i < self -> private_key_count; i++)
	{
	    /* SHA1 dual */
	    if (! elvin_keys_add(subscription_keys,
				 ELVIN_KEY_SCHEME_SHA1_DUAL,
				 ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX,
				 self -> private_keys[i],
				 self -> private_key_lengths[i],
				 NULL))
	    {
		return -1;
	    }
	}

	/* Add the public keys to the subscription key block */
	for (i = 0; i < self -> public_key_count; i++)
	{
	    if (self -> private_key_count != 0)
	    {
		/* SHA1 dual */
		if (! elvin_keys_add(subscription_keys,
				     ELVIN_KEY_SCHEME_SHA1_DUAL,
				     ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
				     self -> public_keys[i],
				     self -> public_key_lengths[i],
				     NULL))
		{
		    return -1;
		}
	    }

	    /* SHA1 producer */
	    if (! elvin_keys_add(subscription_keys,
				 ELVIN_KEY_SCHEME_SHA1_PRODUCER,
				 ELVIN_KEY_SHA1_PRODUCER_INDEX,
				 self -> public_keys[i],
				 self -> public_key_lengths[i],
				 NULL))
	    {
		return -1;
	    }
	}
    }

    /* Call it with the information */
    result = (self -> callback)(
	self -> rock, self -> name,
	self -> in_menu, self -> has_nazi,
	self -> min_time, self -> max_time,
	notification_keys,
	subscription_keys);

    /* Free the private keys */
    for (i = 0; i < self -> private_key_count; i++)
    {
	free(self -> private_keys[i]);
    }

    if (self -> private_keys != NULL)
    {
	free(self -> private_keys);
	self -> private_keys = NULL;

	free(self -> private_key_lengths);
	self -> private_key_lengths = NULL;

	self -> private_key_count = 0;
    }

    /* Free the public keys */
    for (i = 0; i < self -> public_key_count; i++)
    {
	free(self -> public_keys[i]);
    }

    if (self -> public_keys != NULL)
    {
	free(self -> public_keys);
	self -> public_keys = NULL;

	free(self -> public_key_lengths);
	self -> public_key_lengths = NULL;

	self -> public_key_count = 0;
    }

    /* Clean up */
    free(self -> name);
    return result;
}

/* Adds a key to the current list */
static int accept_key(groups_parser_t self, char *name)
{
    char *data;
    int length;
    int is_private;
    void *new_names;
    void *new_lengths;
    char public_key[ELVIN_SHA1DIGESTLEN];

    /* Look up the key in the table */
    if (key_table_lookup(
	    self -> key_table, name,
	    &data, &length, &is_private) < 0)
    {
	/* FIX THIS: report that the key is unknown */
	return -1;
    }

    if (is_private)
    {
	/* Grow the array of private keys */
	if ((new_names = realloc(
		 self -> private_keys,
		 (self -> private_key_count + 1) * sizeof(char *))) == NULL)
	{
	    return -1;
	}

	self -> private_keys = new_names;

	/* Grow the array of private key lengths */
	if ((new_lengths = realloc(
		 self -> private_key_lengths,
		 (self -> private_key_count + 1) * sizeof(char *))) == NULL)
	{
	    return -1;
	}

	self -> private_key_lengths = new_lengths;

	/* Store the new private key */
	if ((self -> private_keys[self -> private_key_count] = malloc(length)) == NULL)
	{
	    return -1;
	}
	memcpy(self -> private_keys[self -> private_key_count], data, length);

	self -> private_key_lengths[self -> private_key_count] = length;
	self -> private_key_count++;

#if ! defined(ELVIN_VERSION_AT_LEAST)
	/* Calculate the public key */
	if (! elvin_sha1digest(data, length, public_key))
	{
	    return -1;
	}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
	/* Calculate the public key */
	if (! elvin_sha1digest(data, length, public_key, NULL))
	{
	    return -1;
	}
#endif /* ELVIN_VERSION_AT_LEAST */

	length = ELVIN_SHA1DIGESTLEN;
	data = public_key;
    }

    /* Grow the array of public keys */
    if ((new_names = realloc(self -> public_keys, (self -> public_key_count + 1) * sizeof(char *))) == NULL)
    {
	return -1;
    }
    self -> public_keys = new_names;

    /* Grow the array of public key lengths */
    if ((new_lengths = realloc(self -> public_key_lengths, (self -> public_key_count + 1) * sizeof(int))) == NULL)
    {
	return -1;
    }
    self -> public_key_lengths = new_lengths;

    /* Store the new public key */
    if ((self -> public_keys[self -> public_key_count] = malloc(length)) == NULL)
    {
	return -1;
    }
    memcpy(self -> public_keys[self -> public_key_count], data, length);

    self -> public_key_lengths[self -> public_key_count] = length;
    self -> public_key_count++;
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

    /* If we get a `:' then look for keys */
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
	self -> state = lex_keys_ws;
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

/* Skipping whitespace in a key name */
static int lex_keys_ws(groups_parser_t self, int ch)
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
	self -> state = lex_keys_ws;
	return 0;
    }

    /* Watch for yet another `:' */
    if (ch == ':')
    {
	self -> token_pointer = self -> token;
	return lex_superfluous(self, ch);
    }

    /* Send anything else through the keys state */
    return lex_keys(self, ch);
}

/* Reading the key names */
static int lex_keys(groups_parser_t self, int ch)
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
	if (accept_key(self, self -> token) < 0)
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
	if (accept_key(self, self -> token) < 0)
	{
	    return -1;
	}

	/* Set up for the next key */
	self -> token_pointer = self -> token;
	self -> state = lex_keys_ws;
	return 0;
    }

    /* Anything else is part of the key */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_keys;
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
groups_parser_t groups_parser_alloc(
    groups_parser_callback_t callback,
    void *rock,
    char *tag,
    key_table_t keys)
{
    groups_parser_t self;

    /* Allocate memory for the new groups_parser */
    if ((self = (groups_parser_t)malloc(sizeof(struct groups_parser))) == NULL)
    {
	return NULL;
    }
    memset(self, 0, sizeof(struct groups_parser));

    /* Copy the tag string */
    if ((self -> tag = strdup(tag)) == NULL)
    {
	groups_parser_free(self);
	return NULL;
    }

    /* Allocate room for the token buffer */
    if ((self -> token = (char *)malloc(INITIAL_TOKEN_SIZE)) == NULL)
    {
	groups_parser_free(self);
	return NULL;
    }

    /* Initialize everything else to sane values */
    self -> callback = callback;
    self -> rock = rock;
    self -> token_pointer = self -> token;
    self -> token_end = self -> token + INITIAL_TOKEN_SIZE;
    self -> state = lex_start;
    self -> line_num = 1;
    self -> key_table = keys;
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

