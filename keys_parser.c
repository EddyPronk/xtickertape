/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2002.
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
static const char cvsid[] = "$Id: keys_parser.c,v 1.1 2002/04/14 20:44:01 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "keys_parser.h"

#define INITIAL_TOKEN_SIZE 64

#define TYPE_ERROR_MSG "expecting `public' or `private', got `%s'"
#define FORMAT_ERROR_MSG "expecting `hex-inline', `hex-file' or `binary-file', got `%s'"
#define FILE_ERROR_MSG "could not open file `%s': %s"
#define HEX_ERROR_MSG "key `%s' is not hexadecimal"

/* The type of a lexer state */
typedef int (*lexer_state_t)(keys_parser_t self, int ch);

/* The structure of a keys_parser */
struct keys_parser
{
    /* The callback for when we've completed a key entry */
    keys_parser_callback_t callback;

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

    /* The name of the current key */
    char *name;

    /* The kind of the key. */
    int is_private;

    /* Where the key is (inline or a separate file). */
    int is_inline;

    /* Whether the key is encoded hexadecimally or raw binary data. */
    int is_hex;

    /* The key data (in raw binary form). */
    char *key_data;

    /* The length of the key data (in bytes). */
    int key_length;
};

/* Lexer state declarations */
static int lex_start(keys_parser_t self, int ch);
static int lex_comment(keys_parser_t self, int ch);
static int lex_name(keys_parser_t self, int ch);
static int lex_name_esc(keys_parser_t self, int ch);
static int lex_type(keys_parser_t self, int ch);
static int lex_format(keys_parser_t self, int ch);
static int lex_data(keys_parser_t self, int ch);

/* Prints a consistent error message */
static void parse_error(keys_parser_t self, char *message)
{
    fprintf(stderr, "%s: parse error line %d: %s\n",
            self -> tag, self -> line_num, message);
}

/* Calls the callback with the given key */
static int accept_key(keys_parser_t self)
{
    int result;

    if (! self -> is_inline)
    {
        int fd;
        struct stat file_stat;

        /* Open the key file. */
        if ((fd = open(self -> key_data, O_RDONLY)) < 0)
        {
            char *error_string = strerror(errno);
	    char *buffer;

	    if ((buffer = (char *)malloc(strlen(FILE_ERROR_MSG)
					 + strlen(self -> token)
					 + strlen(error_string) - 1)) != NULL)
	    {
		sprintf(buffer, FILE_ERROR_MSG, self -> token, error_string);
		parse_error(self, buffer);
		free(buffer);
	    }

            return -1;
        }
	    
        /* Find out how large the key is. */
	if (fstat(fd, &file_stat) < 0)
        {
            close(fd);
            return -1;
        }

        /* Allocate enough space to hold it. */
        self -> key_data = realloc(self -> key_data, file_stat.st_size);
        if (self -> key_data == NULL)
        {
            close(fd);
            return -1;
        }

        /* Read it in. */
        if ((self -> key_length = read(fd, self -> key_data, file_stat.st_size)) < 0)
        {
            close(fd);
            return -1;
        }

        /* Close the file. */
        close(fd);
    }

    if (self -> is_hex)
    {
        char *hex;
        int hex_index, binary_index, value, first_nibble;

        /* Move the hex data aside so we can store the real data. */
        hex = self -> key_data;

        /* Allocate some memory to store the key contents in. */
        self -> key_data = malloc(self -> key_length / 2);
        if (self -> key_data == NULL)
        {
            free(hex);
            return -1;
        }

        /* Convert hex into binary. */
        first_nibble = 1;
        binary_index = 0;
        for (hex_index = 0; hex_index < self -> key_length; hex_index++)
        {
	    int ch = hex[hex_index];

            /* Skip white space. */
            if (isspace(ch)
            {
                continue;
            }

            /* Are we starting a new byte? */
            if (first_nibble)
            {
                value = 0;
            }
            else
            {
                value *= 16;
            }

            /* Calculate the value of this nibble. */
            if (ch >= 'a' && ch <= 'f')
            {
                value += ch - 'a' + 10;
            }
            else if (ch >= 'A' && ch <= 'F')
            {
                value += ch - 'A' + 10;
            }
            else
            {
                value += ch - '0';
            }

            if (!first_nibble)
            {
                /* Move on to the next byte of output. */
                self -> key_data[binary_index++] = value;
            }

            /* Move on to the next nibble of input. */
            first_nibble = ! first_nibble;
        }

        /* Store the length of the real data. */
        self -> key_length = binary_index;

        /* Clean up the hex data. */
        free(hex);
    }

    /* Call the callback with the information */
    if (self -> callback != NULL)
    {
        result = (self -> callback)(
	    self -> rock, self -> name,
            self -> key_data, self -> key_length,
            self -> is_private);
    }

    /* Clean up */
    if (self -> key_data != NULL)
    {
	free(self -> key_data);
	self -> key_data = NULL;
    }

    free(self -> name);
    return result;
}

/* Appends a character to the end of the token */
static int append_char(keys_parser_t self, int ch)
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
static int lex_start(keys_parser_t self, int ch)
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

    /* Watch for a `:' (for an empty key name?) */
    if (ch == ':')
    {
	self -> name = strdup("");
	self -> token_pointer = self -> token;
	self -> state = lex_type;
	return 0;
    }


    /* Anything else is part of the key name */
    self -> token_pointer = self -> token;
    self -> state = lex_name;
    return append_char(self, ch);
}

/* Reading a comment (to end-of-line) */
static int lex_comment(keys_parser_t self, int ch)
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
static int lex_name(keys_parser_t self, int ch)
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
	self -> state = lex_type;
	return 0;
    }

    /* Anything else is part of the group name */
    return append_char(self, ch);
}

/* Reading an escaped character in the key name */
static int lex_name_esc(keys_parser_t self, int ch)
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

/* Reading the public/private option */
static int lex_type(keys_parser_t self, int ch)
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

    /* Watch for the end of the type token */
    if (ch == ':')
    {
	/* Null-terminate the token */
	if (append_char(self, '\0') < 0)
	{
	    return -1;
	}

	/* Make sure the token is either "public" or "private" */
	if (strcasecmp(self -> token, "public") == 0)
	{
	    self -> is_private = 0;
	}
	else if (strcasecmp(self -> token, "private") == 0)
	{
	    self -> is_private = 1;
	}
	else
	{
	    char *buffer = (char *)malloc(strlen(TYPE_ERROR_MSG)
                                          + strlen(self -> token) - 1);
	    if (buffer != NULL)
	    {
		sprintf(buffer, TYPE_ERROR_MSG, self -> token);
		parse_error(self, buffer);
		free(buffer);
	    }

	    return -1;
	}

	/* Move along to the key format */
	self -> token_pointer = self -> token;
	self -> state = lex_format;
	return 0;
    }

    /* Anything else is part of the token */
    return append_char(self, ch);
}

/* Reading the key format */
static int lex_format(keys_parser_t self, int ch)
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

	/* Make sure the token is "hex-inline", "hex-file" or "binary-file" */
	if (strcasecmp(self -> token, "hex-inline") == 0)
	{
	    self -> is_inline = 1;
            self -> is_hex = 1;
	}
	else if (strcasecmp(self -> token, "hex-file") == 0)
	{
	    self -> is_inline = 0;
            self -> is_hex = 1;
	}
        else if (strcasecmp(self -> token, "binary-file") == 0)
        {
            self -> is_inline = 0;
            self -> is_hex = 0;
        }
	else
	{
	    char *buffer = (char*)malloc(strlen(FORMAT_ERROR_MSG)
                                         + strlen(self -> token) - 1);

	    if (buffer != NULL)
	    {
		sprintf(buffer, FORMAT_ERROR_MSG, self -> token);
		parse_error(self, buffer);
		free(buffer);
	    }
	    return -1;
	}

	/* Move along to the key data */
	self -> token_pointer = self -> token;
	self -> state = lex_data;
	return 0;
    }

    /* Anything else is part of the token */
    return append_char(self, ch);
}

/* Reading the key data (content or filename) */
static int lex_data(keys_parser_t self, int ch)
{
    /* Watch for EOF or linefeed */
    if (ch == EOF || ch == '\n')
    {
	/* Null-terminate the key string */
	if (append_char(self, 0) < 0)
	{
	    return -1;
	}

        self -> key_data = strdup(self -> token);
        self -> token_pointer = self -> token;

	/* Accept the key */
	if (accept_key(self) < 0)
	{
	    return -1;
	}

	/* Go on to the next key */
	return lex_start(self, ch);
    }

    /* Anything else is part of the key */
    if (append_char(self, ch) < 0)
    {
	return -1;
    }

    self -> state = lex_data;
    return 0;
}

/* Parses the given character, ignoring CRs and counting LFs */
static int parse_char(keys_parser_t self, int ch)
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

/* Allocates and initializes a new keys file parser */
keys_parser_t keys_parser_alloc(keys_parser_callback_t callback,
                                void *rock, char *tag)
{
    keys_parser_t self;

    /* Allocate memory for the new keys_parser */
    if ((self = (keys_parser_t)malloc(sizeof(struct keys_parser))) == NULL)
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
    self -> key_data = NULL;
    self -> callback = callback;
    self -> rock = rock;
    self -> token_pointer = self -> token;
    self -> token_end = self -> token + INITIAL_TOKEN_SIZE;
    self -> state = lex_start;
    self -> line_num = 1;
    return self;
}

/* Frees the resources consumed by the receiver */
void keys_parser_free(keys_parser_t self)
{
    free(self -> tag);
    free(self -> token);
    free(self);
}

/* Parses the given buffer, calling callbacks for each subscription
 * expression that is successfully read.  A zero-length buffer is
 * interpreted as an end-of-input marker */
int keys_parser_parse(keys_parser_t self, char *buffer, size_t length)
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
