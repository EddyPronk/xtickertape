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
static const char cvsid[] = "$Id: ast.c,v 1.7 2000/07/10 07:44:14 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "ast.h"
#include "subscription.h"

/* Allocates and returns a string representation of the value */
char *value_to_string(value_t *self, elvin_error_t error)
{
    char buffer[80];

    switch (self -> type)
    {
	case VALUE_NONE:
	{
	    return ELVIN_STRDUP("nil", error);
	}

	case VALUE_INT32:
	{
	    sprintf(buffer, "%d", self -> value.int32);
	    return ELVIN_STRDUP(buffer, error);
	}

	case VALUE_INT64:
	{
	    sprintf(buffer, "%" INT64_PRINT "L", self -> value.int64);
	    return ELVIN_STRDUP(buffer, error);
	}

	case VALUE_FLOAT:
	{
	    sprintf(buffer, "%#.8g", self -> value.real64);
	    return ELVIN_STRDUP(buffer, error);
	}

	case VALUE_STRING:
	{
	    return ELVIN_STRDUP(self -> value.string, error);
	}

	case VALUE_OPAQUE:
	{
	    return ELVIN_STRDUP("[opaque]", error);
	}

	case VALUE_BLOCK:
	{
	    return ELVIN_STRDUP("[block]", error);
	}

	case VALUE_LIST:
	{
	    return ELVIN_STRDUP("[list]", error);
	}

	default:
	{
	    fprintf(stderr, "value_to_string(): invalid type: %d\n", self -> type);
	    abort();
	}
    }
}

/* Copes a value */
int value_copy(value_t *self, value_t *copy, elvin_error_t error)
{
    copy -> type = self -> type;
    switch (self -> type)
    {
	case VALUE_NONE:
	{
	    return 1;
	}

	case VALUE_INT32:
	{
	    copy -> value.int32 = self -> value.int32;
	    return 1;
	}

	case VALUE_INT64:
	{
	    copy -> value.int64 = self -> value.int64;
	    return 1;
	}

	case VALUE_FLOAT:
	{
	    copy -> value.real64 = self -> value.real64;
	    return 1;
	}

	case VALUE_STRING:
	{
	    if (! (copy -> value.string = ELVIN_USTRDUP(self -> value.string, error)))
	    {
		return 0;
	    }

	    return 1;
	}

	case VALUE_OPAQUE:
	{
	    uint32_t length = self -> value.opaque.length;

	    copy -> value.opaque.length = length;
	    if (! (copy -> value.opaque.data = ELVIN_MALLOC(length, error)))
	    {
		return 0;
	    }

	    memcpy(copy -> value.opaque.data, self -> value.opaque.data, length);
	    return 1;
	}

	case VALUE_BLOCK:
	{
	    if (! (copy -> value.block = ast_clone(self -> value.block, error)))
	    {
		return 0;
	    }

	    return 1;
	}

	case VALUE_LIST:
	{
	    uint32_t count = self -> value.list.count;
	    uint32_t i;

	    copy -> value.list.count = count;

	    /* Shortcut for zero-length lists */
	    if (count < 1)
	    {
		return 1;
	    }

	    if (! (copy -> value.list.items = (value_t *)ELVIN_MALLOC(count * sizeof(value_t), error)))
	    {
		return 0;
	    }

	    /* Copy the items */
	    for (i = 0; i < count; i++)
	    {
		if (! value_copy(&self -> value.list.items[i], &copy -> value.list.items[i], error))
		{
		    /* FIX THIS: free the items we've copied so far */
		    return 0;
		}
	    }

	    return 1;
	}

	default:
	{
	    fprintf(stderr, "value_copy(): bad type: %d\n", self -> type);
	    abort();
	}
    }
}

/* Frees a value's contents */
int value_free(value_t *self, elvin_error_t error)
{
    switch (self -> type)
    {
	/* Nothing to do for these cases */
	case VALUE_NONE:
	case VALUE_INT32:
	case VALUE_INT64:
	case VALUE_FLOAT:
	{
	    return 1;
	}

	/* Free the string */
	case VALUE_STRING:
	{
	    return ELVIN_FREE(self -> value.string, error);
	}

	/* Free the opaque */
	case VALUE_OPAQUE:
	{
	    return ELVIN_FREE(self -> value.opaque.data, error);
	}

	/* Free a block */
	case VALUE_BLOCK:
	{
	    return ast_free(self -> value.block, error);
	}

	/* Frees a list */
	case VALUE_LIST:
	{
	    uint32_t count = self -> value.list.count;
	    uint32_t i;

	    /* Shortcut for empty lists */
	    if (count < 1)
	    {
		return 1;
	    }

	    /* Free each of the items */
	    for (i = 0; i < count; i++)
	    {
		if (! value_free(&self -> value.list.items[i], error))
		{
		    return 0;
		}
	    }

	    /* Free the list */
	    return ELVIN_FREE(self -> value.list.items, error);
	}

	/* We should never get here */
	default:
	{
	    fprintf(stderr, "value_free(): bad type %d\n", self -> type);
	    abort();
	}
    }
}



/* Copies a value for the hashtable */
static int value_hash_copy(
    elvin_hashtable_t table,
    elvin_hashdata_t *copy_out,
    elvin_hashdata_t data,
    elvin_error_t error)
{
    value_t *copy;
    value_t *self = (value_t *)data;

    /* Allocate some room for the value */
    if (! (copy = (value_t *)ELVIN_MALLOC(sizeof(value_t), error)))
    {
	return 0;
    }

    /* Copy the data */
    if (! value_copy(self, copy, error))
    {
	ELVIN_FREE(copy, NULL);
	return 0;
    }

    *copy_out = (elvin_hashdata_t)copy;
    return 1;
}

/* Frees a value from the hashtable */
static int value_hash_free(elvin_hashdata_t data, elvin_error_t error)
{
    value_t *self = (value_t *)data;
    if (! value_free(self, error))
    {
	return 0;
    }

    return ELVIN_FREE(self, error);
}


/* Allocates and initializes a new environment */
env_t env_alloc(elvin_error_t error)
{
    return elvin_string_hash_create(17, value_hash_copy, value_hash_free, error);
}

/* Looks up a value in the environment */
value_t *env_get(env_t self, char *name, elvin_error_t error)
{
    return (value_t *)elvin_hash_get(self, (elvin_hashkey_t)name, error);
}


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
    AST_BINARY,
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
struct binary
{
    /* The operation type */
    ast_binary_t op;

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
    struct binary binary;
    struct func func;
    subscription_t sub;
};


/* An AST is a very big union */
struct ast
{
    /* The discriminator of the union */
    ast_type_t type;

    /* The number of references to this ast node */
    int ref_count;

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
    self -> ref_count = 1;
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
    self -> ref_count = 1;
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
    self -> ref_count = 1;
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
    self -> ref_count = 1;
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
    self -> ref_count = 1;
    self -> next = NULL;

    /* Shortcut for the empty list */
    if (! list)
    {
	self -> value.list = NULL;
	return self;
    }

    /* Copy the list */
    if (! (self -> value.list = ast_clone(list, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

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
    self -> ref_count = 1;
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
    self -> ref_count = 1;
    self -> next = NULL;
    self -> value.unary.op = op;

    if (! (self -> value.unary.value = ast_clone(value, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

    return self;
}

/* Allocates and initializes a new binary operation AST node */
ast_t ast_binary_alloc(
    ast_binary_t op,
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

    self -> type = AST_BINARY;
    self -> ref_count = 1;
    self -> next = NULL;
    self -> value.binary.op = op;
    if (! (self -> value.binary.lvalue = ast_clone(lvalue, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

    if (! (self -> value.binary.rvalue = ast_clone(rvalue, error)))
    {
	ast_free(self -> value.binary.lvalue, NULL);
	ELVIN_FREE(self, NULL);
	return NULL;
    }

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
    self -> ref_count = 1;
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

		if (! (self -> value.func.args = ast_clone(args, error)))
		{
		    ELVIN_FREE(self, NULL);
		    return NULL;
		}

		return self;
	    }

	    break;
	}
    }

    fprintf(stderr, "Invalid function name: `%s'\n", name);
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
    self -> ref_count = 1;
    self -> next = NULL;

    if (! (self -> value.block = ast_clone(body, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }

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
    ast_t self, ast;
    elvin_hashtable_t env;
    subscription_t subscription;

    /* Make sure the tag is an ID */
    if (tag -> type != AST_ID)
    {
	return NULL;
    }

    /* Use a hashtable as our environment */
    if (! (env = env_alloc(error)))
    {
	return NULL;
    }

    /* Evaluate the statements to construct the subscription's environment */
    for (ast = statements; ast != NULL; ast = ast -> next)
    {
	value_t value;

	if (! ast_eval(ast, env, &value, error))
	{
	    /* FIX THIS: clean up */
	    return 0;
	}
    }

    /* Use the environment to initialize the subscription */
    if (! (subscription = subscription_alloc(tag -> value.string, env, error)))
    {
	return NULL;
    }

    /* Allocate some memory for a subscription ast */
    if (! (self = (ast_t)ELVIN_MALLOC(sizeof(struct ast), error)))
    {
	subscription_free(subscription, error);
	return NULL;
    }

    self -> type = AST_SUB;
    self -> ref_count = 1;
    self -> next = NULL;
    self -> value.sub = subscription;

    /* Clean up */
    if (! ast_free(tag, error))
    {
	return NULL;
    }

    if (! ast_free(statements, error))
    {
	return NULL;
    }

    return self;
}


/* Allocates another copy of the ast */
ast_t ast_clone(ast_t self, elvin_error_t error)
{
    self -> ref_count++;
    return self;
}

/* Frees the resources consumed by the ast */
int ast_free(ast_t self, elvin_error_t error)
{
    if (self == NULL)
    {
	return 1;
    }

    /* Check the reference count */
    if (--self -> ref_count > 0)
    {
	return 1;
    }

    /* Clean up based on type */
    switch (self -> type)
    {
	case AST_INT32:
	case AST_INT64:
	case AST_FLOAT:
	{
	    return ELVIN_FREE(self, error);
	}

	case AST_STRING:
	{
	    if (! ELVIN_FREE(self -> value.string, error))
	    {
		return 0;
	    }

	    return ELVIN_FREE(self, error);
	}

	case AST_LIST:
	{
	    fprintf(stderr, "list\n");
	    abort();
	}

	case AST_ID:
	{
	    if (! ELVIN_FREE(self -> value.id, error))
	    {
		return 0;
	    }

	    return ELVIN_FREE(self, error);
	}

	case AST_UNARY:
	{
	    fprintf(stderr, "unary\n");
	    abort();
	}

	case AST_BINARY:
	{
	    if (! ast_free(self -> value.binary.lvalue, error))
	    {
		return 0;
	    }

	    if (! ast_free(self -> value.binary.rvalue, error))
	    {
		return 0;
	    }

	    return ELVIN_FREE(self, error);
	}

	case AST_FUNCTION:
	case AST_BLOCK:
	case AST_SUB:
	{
	    fprintf(stderr, "ast_free(): type %d is not yet supported\n", self -> type);
	    abort();
	}

	default:
	{
	    fprintf(stderr, "ast_free(): invalid ast type: %d\n", self -> type);
	    abort();
	}
    }
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


/* Evaluates an int32 ast in the given environment */
static int eval_int32(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    value_out -> type = VALUE_INT32;
    value_out -> value.int32 = self -> value.int32;
    return 1;
}

/* Evaluates an int64 ast in the given environment */
static int eval_int64(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    value_out -> type = VALUE_INT64;
    value_out -> value.int64 = self -> value.int64;
    return 1;
}

/* Evaluates a float ast in the given environment */
static int eval_float(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    value_out -> type = VALUE_FLOAT;
    value_out -> value.real64 = self -> value.real64;
    return 1;
}

/* Evaluates a string ast in the given environment */
static int eval_string(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    uchar *result;

    /* Make a copy of the string */
    if (! (result = ELVIN_STRDUP(self -> value.string, error)))
    {
	return 0;
    }

    value_out -> type = VALUE_STRING;
    value_out -> value.string = result;
    return 1;
}

/* Evaluates a list ast in the given environment */
static int eval_list(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    uint32_t count, i;
    value_t *items;
    ast_t ast;

    /* Figure out how long the list is */
    count = 0;
    for (ast = self -> value.list; ast != NULL; ast = ast -> next)
    {
	count++;
    }

    /* Shortcut for zero-length lists */
    if (count < 1)
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

    /* Evaluate each ast item and put it into the list */
    ast = self -> value.list;
    for (i = 0; i < count; i++)
    {
	if (! ast_eval(ast, env, &items[i], error))
	{
	    /* FIX THIS: clean up properly */
	    abort();
	}

	ast = ast -> next;
    }

    /* Fill in the results */
    value_out -> type = VALUE_LIST;
    value_out -> value.list.count = count;
    value_out -> value.list.items = items;
    return 1;
}

/* Evaluates an id ast in the given environment */
static int eval_id(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    fprintf(stderr, "eval_id(): not yet implemented\n");
    abort();
}

/* Evaluates a unary operation ast in the given environment */
static int eval_unary(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    value_t value;

    /* Evaluate the child node */
    if (! ast_eval(self -> value.unary.value, env, &value, error))
    {
	return 0;
    }

    /* Which operation is this? */
    switch (self -> value.unary.op)
    {
	case AST_NOT:
	{
	    /* NONE is the same as `false' */
	    if (value.type == VALUE_NONE)
	    {
		value_out -> type = VALUE_INT32;
		value_out -> value.int32 = 1;
		return 1;
	    }

	    /* Otherwise the value becomes NONE */
	    if (! value_free(&value, error))
	    {
		return 0;
	    }

	    value_out -> type = VALUE_NONE;
	    return 1;
	}

	default:
	{
	    fprintf(stderr, "internal error!\n");
	    abort();
	}
    }
}


/* Evaluates an assignment operator */
static int eval_assign(
    ast_t lvalue,
    ast_t rvalue,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    uchar *name;

    /* The lvalue must be an id */
    if (lvalue -> type != AST_ID)
    {
	fprintf(stderr, "eval_assign(): bad lvalue\n");
	abort();
    }

    /* Get its name */
    name = lvalue -> value.id;

    /* Evaluate the right-hand side */
    if (! ast_eval(rvalue, env, value_out, error))
    {
	return 0;
    }

    /* If the environment already has an entry for that name then delete it */
    if (elvin_hash_get(env, (elvin_hashkey_t)name, error))
    {
	if (! elvin_hash_delete(env, (elvin_hashkey_t)name, error))
	{
	    return 0;
	}
    }

    /* Register the value with the name in the environment */
    if (! elvin_hash_add(env, (elvin_hashkey_t)name, (elvin_hashdata_t)value_out, error))
    {
	return 0;
    }

    return 1;
}

/* Evaluates a binary operation ast in the given environment */
static int eval_binary(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    ast_t lvalue, rvalue;

    /* Extract the lvalue and rvalue */
    lvalue = self -> value.binary.lvalue;
    rvalue = self -> value.binary.rvalue;

    /* What kind of operator is this again? */
    switch (self -> value.binary.op)
    {
	/* Assignment */
	case AST_ASSIGN:
	{
	    return eval_assign(lvalue, rvalue, env, value_out, error);
	}

	case AST_OR:
	case AST_AND:
	case AST_EQ:
	case AST_LT:
	case AST_GT:
	{
	    fprintf(stderr, "eval_binary(): type %d not yet supported\n", self -> value.binary.op);
	    return 0;
	}

	default:
	{
	    fprintf(stderr, "eval_binary(): invalid type: %d\n", self -> value.binary.op);
	    abort();
	}
    }
}

/* Evaluates a function operation ast in the given environment */
static int eval_function(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    fprintf(stderr, "eval_function(): not yet implemented\n");
    abort();
}

/* Evaluates a block ast in the given environment */
static int eval_block(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    value_out -> type = VALUE_BLOCK;
    if (! (value_out -> value.block = ast_clone(self, error))) {
	return 0;
    }

    return 1;
}

/* Evaluates a subscription ast in the given environment */
static int eval_sub(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    fprintf(stderr, "eval_sub(): not yet implemented\n");
    abort();
}

/* Evaluates an AST with the given notification */
int ast_eval(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error)
{
    switch (self -> type)
    {
	case AST_INT32:
	{
	    return eval_int32(self, env, value_out, error);
	}

	case AST_INT64:
	{
	    return eval_int64(self, env, value_out, error);
	}

	case AST_FLOAT:
	{
	    return eval_float(self, env, value_out, error);
	}

	case AST_STRING:
	{
	    return eval_string(self, env, value_out, error);
	}

	case AST_LIST:
	{
	    return eval_list(self, env, value_out, error);
	}

	case AST_ID:
	{
	    return eval_id(self, env, value_out, error);
	}

	case AST_UNARY:
	{
	    return eval_unary(self, env, value_out, error);
	}

	case AST_BINARY:
	{
	    return eval_binary(self, env, value_out, error);
	}

	case AST_FUNCTION:
	{
	    return eval_function(self, env, value_out, error);
	}

	case AST_BLOCK:
	{
	    return eval_block(self, env, value_out, error);
	}

	case AST_SUB:
	{
	    return eval_sub(self, env, value_out, error);
	}

	default:
	{
	    fprintf(stderr, "ast_eval(): bad type: %d\n", self -> type);
	    abort();
	}
    }
}

