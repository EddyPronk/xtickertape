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
static const char cvsid[] = "$Id: ast.c,v 1.6 2000/07/10 00:12:10 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "ast.h"
#include "subscription.h"

/* The types of AST nodes */
typedef enum ast_type
{
    AST_INT32,
    AST_INT64,
    AST_FLOAT,
    AST_STRING,
    AST_LIST,
    AST_ID,
    AST_UNARY,
    AST_BINOP,
    AST_FUNCTION,
    AST_BLOCK,
     AST_SUB
} ast_type_t;

/* The supported functions */
typedef enum ast_func
{
    FUNC_FORMAT
} ast_func_t;

/* The structure of a unary operator node */
struct unary
{
    /* The operator */
    ast_unary_t op;

    /* The operator's arg */
    ast_t value;
};

/* The structure of a binary operator node */
struct binop
{
    /* The operation type */
    ast_binop_t op;

    /* The operation's left-hand value */
    ast_t lvalue;

    /* The operation's right-handle value */
    ast_t rvalue;
};

/* The structure of a function node */
struct func
{
    /* The function operator */
    ast_func_t op;

    /* The operation's args */
    ast_t args;
};

/* A union of all of the various kinds of ASTs */
union ast_union
{
    int32_t int32;
    int64_t int64;
    double real64;
    uchar *string;
    ast_t list;
    ast_t block;
    char *id;
    struct unary unary;
    struct binop binop;
    struct func func;
    subscription_t sub;
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

/* Allocates and initializes a new float AST node */
ast_t ast_float_alloc(double value, elvin_error_t error)
{
    ast_t self;

    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_FLOAT;
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

/* Allocates and initialzes a new unary AST node */
ast_t ast_unary_alloc(ast_unary_t op, ast_t value, elvin_error_t error)
{
    ast_t self;

    /* Allocate some memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_UNARY;
    self -> next = NULL;
    self -> value.unary.op = op;
    self -> value.unary.value = value;
    return self;
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
    self -> next = NULL;
    self -> value.binop.op = op;
    self -> value.binop.lvalue = lvalue;
    self -> value.binop.rvalue = rvalue;
    return self;
}

/* Allocates and initializes a new function AST node */
ast_t ast_function_alloc(ast_t id, ast_t args, elvin_error_t error)
{
    ast_t self;
    char *name;
    
    /* Allocate memory */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_FUNCTION;
    self -> next = NULL;

    /* Get the function's name */
    name = id -> value.id;

    /* Figure out which function it is and check the args */
    switch (*name)
    {
	/* format */
	case 'f':
	{
	    if (strcmp(name, "format") == 0)
	    {
		self -> value.func.op = FUNC_FORMAT;
		self -> value.func.args = args;
		return self;
	    }

	    break;
	}
    }

    fprintf(stderr, "bogus function name: `%s'\n", name);
    ELVIN_FREE(self, NULL);
    return NULL;
}

/* Allocates and initializes a new block AST node */
ast_t ast_block_alloc(ast_t body, elvin_error_t error)
{
    ast_t self;

    /* Allocate a new node */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	return NULL;
    }

    self -> type = AST_BLOCK;
    self -> next = NULL;
    self -> value.block = body;
    return self;
}



/* Converts a list of strings into an elvin_keys_t */
static int eval_producer_keys(
    ast_t value,
    elvin_keys_t *keys_out,
    elvin_error_t error)
{
    elvin_keys_t keys;
    elvin_key_t key;
    ast_t ast;

    /* Don't accidentally return anything interesting */
    *keys_out = NULL;

    /* Make sure we were given a list */
    if (value -> type != AST_LIST)
    {
	fprintf(stderr, "keys must be lists\n");
	return 0;
    }

    /* If the list is empty then don't bother creating a key set */
    if (value -> value.list == NULL)
    {
	return 1;
    }

    /* Allocate some room for the key set */
    if (! (keys = elvin_keys_alloc(7, error)))
    {
	return 0;
    }

    /* Go through the list and add keys */
    for (ast = value -> value.list; ast != NULL; ast = ast -> next)
    {
	/* FIX THIS: we should allow functions to read from files */
	/* Make sure the key is a string */
	if (ast -> type != AST_STRING)
	{
	    fprintf(stderr, "bad producer key type: %d\n", ast -> type);
	    return 0;
	}

	/* Convert it to a producer key */
	if (! (key = elvin_keyraw_alloc(
	    ast -> value.string,
	    strlen(ast -> value.string),
	    error)))
	{
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Add it to the key set */
	if (! elvin_keys_add(keys, key, error))
	{
	    elvin_key_free(key, NULL);
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Release our reference to the key */
	if (! elvin_key_free(key, error))
	{
	    return 0;
	}
    }

    /* Success! */
    *keys_out = keys;
    return 1;
}

/* Converts a list of strings into an elvin_keys_t */
static int eval_consumer_keys(
    value_t *value,
    elvin_keys_t *keys_out,
    elvin_error_t error)
{
    elvin_keys_t keys;
    elvin_key_t key;
    uint32_t i;

    /* Don't accidentally return anything interesting */
    *keys_out = NULL;

    /* If the list is empty then don't bother creating a key set */
    if (value -> value.list.count == 0)
    {
	return 1;
    }

    /* Allocate some room for the key set */
    if (! (keys = elvin_keys_alloc(value -> value.list.count, error)))
    {
	return 0;
    }

    /* Go through the list and add keys */
    for (i = 0; i < value -> value.list.count; i++)
    {
	value_t *item = &value -> value.list.items[i];

	if (item -> type != VALUE_STRING)
	{
	    fprintf(stderr, "bad consumer key type: %d\n", item -> type);
	    return 0;
	}

	/* Convert it into a consumer key */
	if (! (key = elvin_keyprime_from_hexstring(item -> value.string, error)))
	{
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Add it to the key set */
	if (! elvin_keys_add(keys, key, error))
	{
	    elvin_key_free(key, NULL);
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Release our reference to the key */
	if (! elvin_key_free(key, error))
	{
	    return 0;
	}
    }

    /* Success! */
    *keys_out = keys;
    return 1;
}

/* Allocates and initializes a new subscription AST node */
ast_t ast_sub_alloc(ast_t tag, ast_t statements, elvin_error_t error)
{
    subscription_t sub;
    ast_t self, ast, next;
    char *name = NULL;
    char *subscription = NULL;
    int in_menu = 1;
    int auto_mime = 0;
    elvin_keys_t producer_keys = NULL;
    elvin_keys_t consumer_keys = NULL;
    value_t group;
    value_t user;
    value_t message;
    value_t timeout;
    value_t mime_type;
    value_t mime_args;
    value_t message_id;
    value_t reply_id;

    /* Go through the statements, perform sanity checks on them and
     * then use them to construct a usable subscription_t */
    for (ast = statements; ast != NULL; ast = next)
    {
	char *field;
	ast_t rvalue;
	value_t value;

	next = ast -> next;

	/* Pull apart the assignment node */
	field = ast -> value.binop.lvalue -> value.id;
	rvalue = ast -> value.binop.rvalue;

	/* Figure out which field we're assigning */
	switch (*field)
	{
	    /* auto-mime */
	    case 'a':
	    {
		if (strcmp(field, "auto-mime") == 0)
		{
		    /* Evaluate the rvalue */
		    if (! ast_eval(rvalue, NULL, &value, error))
		    {
			fprintf(stderr, "trouble evaluating `auto-mime'\n");
			abort();
		    }

		    /* Check the resulting type */
		    if (value.type != VALUE_INT32)
		    {
			fprintf(stderr, "bogus value for `auto-mime'\n");
			abort();
		    }

		    auto_mime = value.value.int32;
		    continue;
		}

		break;
	    }

	    /* consumer-keys */
	    case 'c':
	    {
		if (strcmp(field, "consumer-keys") == 0)
		{
		    /* Evaluate the rvalue */
		    if (! ast_eval(rvalue, NULL, &value, error))
		    {
			fprintf(stderr, "trouble evaluating `consumer-keys'\n");
			abort();
		    }

		    /* Check the resulting type */
		    if (value.type != VALUE_LIST)
		    {
			fprintf(stderr, "bogus value for `consumer-keys'\n");
			abort();
		    }

		    /* Convert the list into a set of keys */
		    if (! eval_consumer_keys(&value, &consumer_keys, error))
		    {
			fprintf(stderr, "bad keys\n");
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
		    rvalue -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* in-menu */
	    case 'i':
	    {
		if (strcmp(field, "in-menu") == 0)
		{
		    /* Evaluate the rvalue */
		    if (! ast_eval(rvalue, NULL, &value, error))
		    {
			fprintf(stderr, "trouble evaluating `in-menu'\n");
			abort();
		    }

		    /* Check the resulting type */
		    if (value.type != VALUE_INT32)
		    {
			fprintf(stderr, "bogus value for `in-menu'\n");
			abort();
		    }

		    in_menu = value.value.int32;
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
		    rvalue -> next = NULL;
		    continue;
		}

		if (strcmp(field, "message-id") == 0)
		{
		    message_id = value;
		    rvalue -> next = NULL;
		    continue;
		}

		if (strcmp(field, "mime-args") == 0)
		{
		    mime_args = value;
		    rvalue -> next = NULL;
		    continue;
		}

		if (strcmp(field, "mime-type") == 0)
		{
		    mime_type = value;
		    rvalue -> next = NULL;
		    continue;
		}

		break;
	    }

	    /* name */
	    case 'n':
	    {
		if (strcmp(field, "name") == 0)
		{
		    /* Evaluate the rvalue to get the name */
		    if (! ast_eval(rvalue, NULL, &value, error))
		    {
			fprintf(stderr, "trouble evaluating `name'\n");
			abort();
		    }

		    /* Check the type of the result */
		    if (value.type != VALUE_STRING)
		    {
			fprintf(stderr, "`name' must be a string\n");
			abort();
		    }

		    /* Hang on to that string */
		    name = value.value.string;
		    continue;
		}

		break;
	    }

	    /* producer-keys */
	    case 'p':
	    {
		if (strcmp(field, "producer-keys") == 0)
		{
		    if (! eval_producer_keys(rvalue, &producer_keys, error))
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
		    rvalue -> next = NULL;
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
		    if (rvalue -> type != AST_STRING)
		    {
			fprintf(stderr, "`subscription' must be a string\n");
			abort();
		    }

		    /* Steal the string from the AST */
		    subscription = rvalue -> value.string;
		    rvalue -> value.string = NULL;
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
		    rvalue -> next = NULL;
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
		    rvalue -> next = NULL;
		    continue;
		}

		break;
	    }
	}

	fprintf(stderr, "bogus field name: `%s'\n", field);
	abort();
    }

    /* Allocate a subscription */
    if (! (sub = subscription_alloc(
	tag -> value.id, name, subscription, 
	in_menu, auto_mime,
	producer_keys, consumer_keys,
	group, user, message, timeout,
	mime_type, mime_args,
	message_id, reply_id,
	error)))
    {
	return NULL;
    }

    /* Allocate memory for the node */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	subscription_free(sub, NULL);
	return NULL;
    }

    self -> type = AST_SUB;
    self -> next = NULL;
    self -> value.sub = sub;
    return self;
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
    ast_t ast;

    /* Find the end of the list */
    for (ast = list; ast -> next != NULL; ast = ast -> next);

    /* Append the item */
    ast -> next = item;
    return list;
}


/* Evaluates an AST with the given notification */
int ast_eval(
    ast_t self,
    elvin_notification_t env,
    value_t *value_out,
    elvin_error_t error)
{
    switch (self -> type)
    {
	case AST_INT32:
	{
	    value_out -> type = VALUE_INT32;
	    value_out -> value.int32 = self -> value.int32;
	    return 1;
	}

	case AST_INT64:
	{
	    value_out -> type = VALUE_INT64;
	    value_out -> value.int64 = self -> value.int64;
	    return 1;
	}

	case AST_FLOAT:
	{
	    value_out -> type = VALUE_FLOAT;
	    value_out -> value.real64 = self -> value.real64;
	    return 1;
	}

	case AST_STRING:
	{
	    if (! (value_out -> value.string = ELVIN_STRDUP(self -> value.string, error)))
	    {
		return 0;
	    }

	    value_out -> type = VALUE_STRING;
	    return 1;
	}

	case AST_LIST:
	{
	    uint32_t count, i;
	    value_t *items;
	    ast_t ast;

	    /* Figure out how long the list needs to be */
	    count = 0;
	    for (ast = self -> value.list; ast != NULL; ast = ast -> next)
	    {
		count++;
	    }

	    /* Short-cut -- nothing to alloc if no items */
	    if (count == 0)
	    {
		value_out -> type = VALUE_LIST;
		value_out -> value.list.count = 0;
		value_out -> value.list.items = NULL;
		return 1;
	    }

	    /* Allocate an array to hold the items in the list */
	    if (! (items = (value_t *)ELVIN_MALLOC(count * sizeof(value_t), error)))
	    {
		return 0;
	    }

	    /* Evaluate each ast item and put it into the resulting list */
	    ast = self -> value.list;
	    for (i = 0; i < count; i++)
	    {
		if (! ast_eval(ast, env, &items[i], error))
		{
		    return 0;
		}

		ast = ast -> next;
	    }

	    /* And we're done */
	    value_out -> type = VALUE_LIST;
	    value_out -> value.list.count = count;
	    value_out -> value.list.items = items;
	    return 1;
	}

	case AST_ID:
	{
	    /* If we have no environment then everything evalutes to nil */
	    if (! env)
	    {
		value_out -> type = VALUE_NONE;
		return 1;
	    }

	    /* Otherwise look up the value in the notification */
	    printf("punt!\n");
	    abort();
	}

	case AST_UNARY:
	{
	    switch (self -> value.unary.op)
	    {
		case AST_NOT:
		{
		    /* Evaluate the child */
		    if (! ast_eval(self -> value.unary.value, env, value_out, error))
		    {
			return 0;
		    }

		    /* And negate its value */
		    switch (value_out -> type)
		    {
			case VALUE_NONE:
			{
			    value_out -> type = VALUE_INT32;
			    value_out -> value.int32 = 1;
			}

			case VALUE_INT32:
			{
			    value_out -> value.int32 = ! value_out -> value.int32;
			    return 1;
			}

			case VALUE_INT64:
			{
			    value_out -> value.int64 = ! value_out -> value.int64;
			    return 1;
			}

			case VALUE_FLOAT:
			{
			    value_out -> value.real64 = ! value_out -> value.real64;
			    return 1;
			}

			case VALUE_STRING:
			{
			    value_out -> type = VALUE_NONE;
			    return ELVIN_FREE(value_out -> value.string, error);
			}

			case VALUE_OPAQUE:
			{
			    value_out -> type = VALUE_NONE;
			    return ELVIN_FREE(value_out -> value.opaque.data, error);
			}

			case VALUE_LIST:
			{
			    printf("how do I negate a list?!\n");
			    abort();
			}

			case VALUE_BLOCK:
			{
			    printf("how do I negate a block!?\n");
			    abort();
			}
		    }

		    fprintf(stderr, "doh!\n");
		    abort();
		}
	    }

	    /* Otherwise we're in trouble */
	    printf("foon!\n");
	    abort();
	}

	case AST_BINOP:
	{
	    printf("ast_eval: binop\n");
	    abort();
	}

	case AST_FUNCTION:
	{
	    printf("ast_eval: function\n");
	    abort();
	}

	case AST_BLOCK:
	{
	    printf("ast_eval: block\n");
	    abort();
	}

	case AST_SUB:
	{
	    printf("ast_eval: sub\n");
	    abort();
	}
    }

    return 0;
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
	case AST_FLOAT:
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
