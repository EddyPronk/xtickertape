/* -*- mode: c; c-file-style: "elvin" -*- */
/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2003.
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
static const char cvsid[] = "$Id: groups_parser.c,v 1.30 2004/02/02 22:01:19 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* fprintf, snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* atoi, free, malloc, realloc */
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h> /* isdigit */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memcpy, memset, strdup, strlen */
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h> /* strcasecmp */
#endif
#include "globals.h"
#include "replace.h"
#include "groups_parser.h"

#define INITIAL_TOKEN_SIZE 64

#define MENU_ERROR_MSG "expecting `menu' or `no menu', got `%s'"
#define NAZI_ERROR_MSG "expecting `auto' or `manual', got `%s'"
#define TIMEOUT_ERROR_MSG "illegal timeout value `%s'"
#define KEY_ERROR_MSG "unknown key: `%s'"
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

    /* The number keys */
    int key_count;

    /* The keys names */
    char **key_names;
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

    /* Make sure there is a callback */
    if (self -> callback == NULL)
    {
	return 0;
    }

    /* Call it with the information */
    result = self -> callback(
	self -> rock, self -> name,
	self -> in_menu, self -> has_nazi,
	self -> min_time, self -> max_time,
        self -> key_names, self -> key_count);

    /* Clean up */
    for (i = 0; i < self -> key_count; i++)
    {
        free(self -> key_names[i]);
    }

    if (self -> key_names)
    {
        free(self -> key_names);
        self -> key_names = NULL;
        self -> key_count = 0;
    }

    free(self -> name);
    return result;
}

/* Prints a consistent error message */
static void parse_error(groups_parser_t self, char *message)
{
    fprintf(stderr, "%s: parse error line %d: %s\n", self -> tag, self -> line_num, message);
}

/* Adds a key to the current list */
static int accept_key(groups_parser_t self, char *name)
{
    char **new_names;

    /* Make room in the list of keys */
    if ((new_names = (char **)realloc(
             self->key_names,
             (self->key_count + 1) * sizeof(char *))) == NULL)
    {
        return -1;
    }
    self -> key_names = new_names;

    /* Record the new key name */
    if ((new_names[self->key_count++] = strdup(name)) == NULL)
    {
        return -1;
    }

    return 0;
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
	    size_t length = strlen(MENU_ERROR_MSG) + strlen(self -> token) - 1;
	    char *buffer;

	    if ((buffer = (char *)malloc(length)) != NULL)
	    {
		snprintf(buffer, length, MENU_ERROR_MSG, self -> token);
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
	    size_t length = strlen(NAZI_ERROR_MSG) + strlen(self -> token) - 1;
	    char *buffer;

	    if ((buffer = (char *)malloc(length)) != NULL)
	    {
		snprintf(buffer, length, NAZI_ERROR_MSG, self -> token);
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
	size_t length;
	char *buffer;

	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Generate an error message */
	length = strlen(TIMEOUT_ERROR_MSG) + strlen(self -> token) - 1;
	if ((buffer = (char *)malloc(length)) != NULL)
	{
	    snprintf(buffer, length, TIMEOUT_ERROR_MSG, self -> token);
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
	size_t length;
	char *buffer;

	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Generate an error message */
	length = strlen(EXTRA_ERROR_MSG) + strlen(self -> token) - 1;
	if ((buffer = (char *)malloc(length)) != NULL)
	{
	    snprintf(buffer, length, EXTRA_ERROR_MSG, self -> token);
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
    char *tag)
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
    unsigned char *end = (unsigned char *)buffer + length;
    unsigned char *pointer;

    /* Length of 0 indicates EOF */
    if (length == 0)
    {
	return parse_char(self, EOF);
    }

    /* Parse the buffer */
    for (pointer = (unsigned char *)buffer; pointer < end; pointer++)
    {
	if (parse_char(self, *pointer) < 0)
	{
	    return -1;
	}
    }

    return 0;
}
