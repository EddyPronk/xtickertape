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
static const char cvsid[] = "$Id: ast.c,v 1.1 2000/07/06 14:00:01 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "ast.h"

/* The types of AST nodes */
typedef enum ast_type
{
    AST_INT32,
    AST_INT64,
    AST_REAL64,
    AST_STRING,
    AST_LIST,
    AST_ID,
    AST_BINOP,
    AST_SUBSCRIPTION
} ast_type_t;

/* The structure of a binary operation node */
struct binop
{
    /* The operation type */
    ast_binop_t op;

    /* The operation's left-hand value */
    ast_t lvalue;

    /* The operation's right-handle value */
    ast_t rvalue;
};

/* A union of all of the various kinds of ASTs */
union ast_union
{
    int32_t int32;
    int64_t int64;
    double real64;
    char *string;
    ast_t list;
    char *id;
    struct binop binop;
};


/* An AST is a very big union */
struct ast
{
    /* The discriminator of the union */
    ast_type_t type;

    /* The next ast in the list */
    ast_t next;

    /* The AST's type-specific data */
    union ast_union value;
};


ast_t ast_int32_alloc(int32_t value, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_INT32;
    self -> next = NULL;
    self -> value.int32 = value;
    return self;
}

/* Allocates and initializes a new int64 AST node */
ast_t ast_int64_alloc(int64_t value, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_INT64;
    self -> next = NULL;
    self -> value.int64 = value;
    return self;
}

/* Allocates and initializes a new real64 AST node */
ast_t ast_real64_alloc(double value, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_REAL64;
    self -> next = NULL;
    self -> value.real64 = value;
    return self;
}

/* Allocates and initializes a new string AST node */
ast_t ast_string_alloc(char *value, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_STRING;
    self -> next = NULL;
    if (! (self -> value.string = ELVIN_STRDUP(value, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

    return self;
}

/* Allocates and initializes a new list AST node */
ast_t ast_list_alloc(ast_t list, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_LIST;
    self -> next = NULL;
    self -> value.list = list;
    return self;
}


/* Allocates and initializes a new id AST node */
ast_t ast_id_alloc(char *name, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_ID;
    self -> next = NULL;
    if (! (self -> value.id = ELVIN_STRDUP(name, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

    return self;
}

/* Allocates and initializes a new function AST node */
ast_t ast_function_alloc(ast_t id, ast_t args, elvin_error_t error)
{
    printf("ast_function_alloc(): not yet implemented\n");
    return id;
}

/* Allocates and initializes a new binary operation AST node */
ast_t ast_binop_alloc(
    ast_binop_t op,
    ast_t lvalue,
    ast_t rvalue,
    elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_BINOP;
    self -> value.binop.op = op;
    self -> value.binop.lvalue = lvalue;
    self -> value.binop.rvalue = rvalue;
    return self;
}

/* Evaluates a value to either true or false */
static int eval_boolean(ast_t value, int *result_out, elvin_error_t error)
{
    /* Make sure the boolean is an ID */
    if (value -> type != AST_ID)
    {
	fprintf(stderr, "bad boolean\n");
	return 0;
    }

    /* Is it `true'? */
    if (strcmp(value -> value.id, "true") == 0)
    {
	*result_out = 1;
	return 1;
    }

    /* Is it `false'? */
    if (strcmp(value -> value.id, "false") == 0)
    {
	*result_out = 0;
	return 1;
    }

    fprintf(stderr, "bad boolean: `%s'\n", value -> value.id);
    return 0;
}

/* Converts a list of strings into an elvin_keys_t */
static int eval_keys(ast_t value, elvin_keys_t *keys_out, elvin_error_t error)
{
    printf("eval_keys(): not yet implemented\n");
    keys_out = NULL;
    return 1;
}

/* Allocates and initializes a new subscription AST node */
ast_t ast_sub_alloc(ast_t tag, ast_t statements, elvin_error_t error)
{
    ast_t ast, next;
    char *name = NULL;
    char *subscription = NULL;
    int in_menu = 1;
    int auto_mime = 0;
    elvin_keys_t producer_keys = NULL;
    elvin_keys_t consumer_keys = NULL;
    ast_t group = NULL;
    ast_t user = NULL;
    ast_t message = NULL;
    ast_t timeout = NULL;
    ast_t mime_type = NULL;
    ast_t mime_args = NULL;
    ast_t message_id = NULL;
    ast_t reply_id = NULL;

    /* Go through the statements, perform sanity checks on them and
     * then use them to construct a usable subscription_t */
    for (ast = statements; ast != NULL; ast = next)
    {
	char *field;
	ast_t value;

	next = ast -> next;

	/* Take the assignment node apart */
	field = ast -> value.binop.lvalue -> value.id;
	value = ast -> value.binop.rvalue;

	/* Figure out which field we're assigning */
	switch (*field)
	{
	    /* auto-mime */
	    case 'a':
	    {
		if (strcmp(field, "auto-mime") == 0)
		{
		    /* Value must be either true or false */
		    if (! eval_boolean(value, &auto_mime, error))
		    {
			abort();
		    }

		    continue;
		}

		break;
	    }

	    /* consumer-keys */
	    case 'c':
	    {
		if (strcmp(field, "consumer-keys") == 0)
		{
		    if (! eval_keys(value, &consumer_keys, error))
		    {
			abort();
		    }

		    continue;
		}

		break;
	    }

	    /* group */
	    case 'g':
	    {
		if (strcmp(field, "group") == 0)
		{
		    /* Just steal the AST */
		    group = value;
		    value -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* in-menu */
	    case 'i':
	    {
		if (strcmp(field, "in-menu") == 0)
		{
		    /* Value must be either `true' or `false' */
		    if (! eval_boolean(value, &in_menu, error))
		    {
			abort();
		    }

		    continue;
		}

		break;
	    }

	    /* message, message-id, mime-args, mime-type */
	    case 'm':
	    {
		if (strcmp(field, "message") == 0)
		{
		    message = value;
		    value -> next = NULL;
		    continue;
		}

		if (strcmp(field, "message-id") == 0)
		{
		    message_id = value;
		    value -> next = NULL;
		    continue;
		}

		if (strcmp(field, "mime-args") == 0)
		{
		    mime_args = value;
		    value -> next = NULL;
		    continue;
		}

		if (strcmp(field, "mime-type") == 0)
		{
		    mime_type = value;
		    value -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* name */
	    case 'n':
	    {
		if (strcmp(field, "name") == 0)
		{
		    /* Make sure the name is a simple string */
		    if (value -> type != AST_STRING)
		    {
			fprintf(stderr, "`name' must be a string\n");
			abort();
		    }

		    /* Steal the string from the AST */
		    name = value -> value.string;
		    value -> value.string = NULL;
		    continue;
		}

		break;
	    }

	    /* producer-keys */
	    case 'p':
	    {
		if (strcmp(field, "producer-keys") == 0)
		{
		    if (! eval_keys(value, &producer_keys, error))
		    {
			abort();
		    }

		    continue;
		}

		break;
	    }

	    /* reply-id */
	    case 'r':
	    {
		if (strcmp(field, "reply-id") == 0)
		{
		    /* Steal the string from the AST */
		    reply_id = value;
		    value -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* subscription */
	    case 's':
	    {
		if (strcmp(field, "subscription") == 0)
		{
		    /* Make sure the subscription is a simple string */
		    if (value -> type != AST_STRING)
		    {
			fprintf(stderr, "`subscription' must be a string\n");
			abort();
		    }

		    /* Steal the string from the AST */
		    subscription = value -> value.string;
		    value -> value.string = NULL;
		    continue;
		}

		break;
	    }

	    /* timeout */
	    case 't':
	    {
		if (strcmp(field, "timeout") == 0)
		{
		    timeout = value;
		    value -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* user */
	    case 'u':
	    {
		if (strcmp(field, "user") == 0)
		{
		    /* Just steal the AST */
		    user = value;
		    value -> next = NULL;
		    continue;
		}

		break;
	    }
	}

	fprintf(stderr, "bogus field name: `%s'\n", field);
	abort();
    }

    return tag;
}


/* Frees the resources consumed by the ast */
int ast_free(ast_t self, elvin_error_t error)
{
    fprintf(stderr, "ast_free(): not yet implemented\n");
    abort();
}

/* Appends another element to the end of an AST list */
ast_t ast_append(ast_t list, ast_t item, elvin_error_t error)
{
    /* Find the end of the list */
    item -> next = list;
    return item;
}


/* Allocates and returns a string representation of the AST */
char *ast_to_string(ast_t self, elvin_error_t error)
{
    /* Which kind of ast node are we? */
    switch (self -> type)
    {
	/* Simple integers */
	case AST_INT32:
	{
	    char buffer[12];

	    sprintf(buffer, "%d", self -> value.int32);
	    return ELVIN_STRDUP(buffer, error);
	}

	/* Big integers */
	case AST_INT64:
	{
	    char buffer[21];

	    sprintf(buffer, "%" INT64_PRINT, self -> value.int64);
	    return ELVIN_STRDUP(buffer, error);
	}

	/* Floating-point numbers (not really reals) */
	case AST_REAL64:
	{
	    /* FIX THIS: how big does this have to be? */
	    char buffer[64];

	    sprintf(buffer, "%#.8g", self -> value.real64);
	    return ELVIN_STRDUP(buffer, error);
	}

	/* Strings print as themselves */
	case AST_STRING:
	{
	    return ELVIN_STRDUP(self -> value.string, error);
	}

	/* Identifiers don't print, but we'll pretend they do */
	case AST_ID:
	{
	    return ELVIN_STRDUP(self -> value.id, error);
	}

	case AST_BINOP:
	default:
	{
	    fprintf(stderr, "ast_to_string(): unexpected type: %d\n", self -> type);
	    return NULL;
	}
    }
}
