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

#ifdef lint
static const char cvsid[] = "$Id: sexp.c,v 2.8 2000/11/09 07:35:32 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/error.h>
#include <elvin/errors/elvin.h>
#include "errors.h"
#include "sexp.h"

#define INIT_SYMBOL_TABLE_SIZE 100

/* The symbol table */
static elvin_hashtable_t symbol_table = NULL;

/* A cons cell has two values */
struct cons
{
    sexp_t car;
    sexp_t cdr;
};

/* A built-in function has a name and a function */
struct builtin
{
    char *name;
    builtin_t function;
};

/* A lambda expression */
struct lambda
{
    env_t env;
    sexp_t arg_list;
    sexp_t body;
};

/* An sexp can be many things */
struct sexp
{
    /* The type determines all */
    sexp_type_t type;

    /* The number of references to this sexp */
    uint32_t ref_count;

    /* Everything else is part of the big union */
    union
    {
	int32_t i;
	int64_t h;
	double d;
	char *s;
	struct cons c;
	struct builtin b;
	struct lambda l;
    } value;
};

/* An environment is a hashtable and a parent */
struct env
{
    /* The number of references to this environment */
    int ref_count;

    /* The parent environment */
    env_t parent;

    /* A mapping of symbols to their values */
    elvin_hashtable_t map;
};


/* Some predefined sexps */
static struct sexp nil = { SEXP_NIL, 1, { 0 } };
static struct sexp minus_one = { SEXP_INT32, 1, { -1 } };
static struct sexp numbers[] =
{
    { SEXP_INT32, 1, { 0 } },
    { SEXP_INT32, 1, { 1 } },
    { SEXP_INT32, 1, { 2 } },
    { SEXP_INT32, 1, { 3 } },
    { SEXP_INT32, 1, { 4 } },
    { SEXP_INT32, 1, { 5 } },
    { SEXP_INT32, 1, { 6 } },
    { SEXP_INT32, 1, { 7 } },
    { SEXP_INT32, 1, { 8 } },
    { SEXP_INT32, 1, { 9 } }
};

/* Hash function for a symbol */
static uint32_t symbol_hashfunc(elvin_hashtable_t table, elvin_hashkey_t key)
{
    char *start = (char *)key;
    char *pointer;
    uint32_t hash = 0;

    /* Treat the pointer as a fixed-length byte array */
    for (pointer = start; pointer < start + sizeof(void *); pointer++)
    {
	hash <<= 1;
	hash ^= *pointer;
    }

    return hash;
}

/* Equality function for a symbol */
static int symbol_hashequal(
    elvin_hashtable_t table,
    elvin_hashkey_t key1,
    elvin_hashkey_t key2)
{
    return key1 == key2;
}

/* Grabs a reference to a symbol */
static int symbol_hashcopy(
    elvin_hashtable_t table,
    elvin_hashkey_t *copy_out,
    elvin_hashkey_t key,
    elvin_error_t error)
{
    sexp_t sexp = (sexp_t)key;
    sexp -> ref_count++;
    *copy_out = key;
    return 1;
}

/* Frees a reference to a symbol */
static int symbol_hashfree(elvin_hashkey_t key, elvin_error_t error)
{
    return sexp_free((sexp_t)key, error);
}

/* Grabs a reference to an sexp */
static int sexp_hashcopy(elvin_hashtable_t table,
			 elvin_hashdata_t *copy_out,
			 elvin_hashdata_t data,
			 elvin_error_t error)
{
    sexp_t sexp = (sexp_t)data;
    sexp -> ref_count++;
    *copy_out = data;
    return 1;
}

/* Frees a reference to an sexp */
static int sexp_hashfree(elvin_hashdata_t data, elvin_error_t error)
{
    return sexp_free((sexp_t)data, error);
}


/* Initialize the interpreter */
int interp_init(elvin_error_t error)
{
    /* No, so create one */
    if ((symbol_table = elvin_string_hash_create(
	     INIT_SYMBOL_TABLE_SIZE,
	     NULL,
	     NULL,
	     error)) == NULL)
    {
	return 0;
    }

    return 1;
}

/* Shut down the interpreter */
int interp_close(elvin_error_t error)
{
    return elvin_hash_free(symbol_table, error);
}



/* Answers the unique nil instance */
sexp_t nil_alloc(elvin_error_t error)
{
    nil.ref_count++;
    return &nil;
}


/* Allocates a new sexp */
sexp_t sexp_alloc(sexp_type_t type, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate some memory for the new sexp */
    if ((sexp = (sexp_t)ELVIN_MALLOC(sizeof(struct sexp), error)) == NULL)
    {
	return NULL;
    }

    sexp -> ref_count = 1;
    sexp -> type = type;
    return sexp;
}

/* Allocates and initializes a new 32-bit integer sexp */
sexp_t int32_alloc(int32_t value, elvin_error_t error)
{
    sexp_t sexp;

    /* See if we already have the value cached */
    if (0 <= value && value < 10)
    {
	numbers[value].ref_count++;
	return &numbers[value];
    }

    /* We also cache -1 */
    if (value == -1)
    {
	minus_one.ref_count++;
	return &minus_one;
    }

    /* Allocate a new sexp */
    if ((sexp = sexp_alloc(SEXP_INT32, error)) == NULL)
    {
	return NULL;
    }

    sexp -> value.i = value;
    return sexp;
}

/* Answers the integer's value */
int32_t int32_value(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.i;
}

/* Allocates and initializes a new 64-bit integer sexp */
sexp_t int64_alloc(int64_t value, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_INT64, error)) == NULL)
    {
	return NULL;
    }

    sexp -> value.h = value;
    return sexp;
}

/* Answers the integer's value */
int64_t int64_value(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.h;
}

/* Allocates and initializes a new float sexp */
sexp_t float_alloc(double value, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_FLOAT, error)) == NULL)
    {
	return NULL;
    }

    sexp -> value.d = value;
    return sexp;
}


/* Allocates and initializes a new string sexp */
sexp_t string_alloc(char *value, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_STRING, error)) == NULL)
    {
	return NULL;
    }

    /* Duplicate the value */
    if ((sexp -> value.s = ELVIN_STRDUP(value, error)) == NULL)
    {
	ELVIN_FREE(sexp, NULL);
	return NULL;
    }

    return sexp;
}

/* Answers the string's characters */
char *string_value(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.s;
}

/* Allocates and initializes a new char sexp */
sexp_t char_alloc(char ch, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_CHAR, error)) == NULL)
    {
	return NULL;
    }

    sexp -> value.i = (int32_t)ch;
    return sexp;
}

/* Answers the char's char */
char char_value(sexp_t sexp, elvin_error_t error)
{
    return (char)sexp->value.i;
}


/* Allocates and initializes a new symbol sexp */
sexp_t symbol_alloc(char *name, elvin_error_t error)
{
    sexp_t sexp;

    /* Check for the symbol in the symbol table */
    if ((sexp = elvin_hash_get(symbol_table, (elvin_hashkey_t)name, error)) != NULL)
    {
	sexp -> ref_count++;
	return sexp;
    }

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_SYMBOL, error)) == NULL)
    {
	return NULL;
    }

    /* Duplicate the value */
    if ((sexp -> value.s = ELVIN_STRDUP(name, error)) == NULL)
    {
	ELVIN_FREE(sexp, NULL);
	return NULL;
    }

    /* Store it in the symbol table */
    if (elvin_hash_add(
	    symbol_table,
	    (elvin_hashkey_t)name,
	    (elvin_hashdata_t)sexp,
	    error) == 0) {
	ELVIN_FREE(sexp, NULL);
	return NULL;
    }

    return sexp;
}

/* Answers the symbol's name */
char *symbol_name(sexp_t sexp, elvin_error_t error)
{
    /* FIX THIS: complain if the sexp is not a symbol */
    return sexp -> value.s;
}

/* Allocates and initializes a new cons sexp */
sexp_t cons_alloc(sexp_t car, sexp_t cdr, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_CONS, error)) == NULL)
    {
	return NULL;
    }

    /* Copy the car and cdr into place */
    sexp -> value.c.car = car;
    sexp -> value.c.cdr = cdr;
    return sexp;
}

/* Answers the car of a cons sexp */
sexp_t cons_car(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.c.car;
}

/* Answers the cdr of a cons sexp */
sexp_t cons_cdr(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.c.cdr;
}

/* Reverses a collection of cons cells */
sexp_t cons_reverse(sexp_t sexp, sexp_t end, elvin_error_t error)
{
    sexp_t cdr = end;

    /* Keep going until the entire list is reversed */
    while (1)
    {
	sexp_t car = sexp -> value.c.car;

	/* Rotate the cdr into the car and end into the cdr */
	sexp -> value.c.car = sexp -> value.c.cdr;
	sexp -> value.c.cdr = cdr;

	/* See if we're done */
	if (car -> type == SEXP_NIL)
	{
	    return sexp;
	}

	cdr = sexp;
	sexp = car;
    }
}

/* Allocates and initializes a new built-in sexp */
sexp_t builtin_alloc(char *name, builtin_t function, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_BUILTIN, error)) == NULL)
    {
	return NULL;
    }

    /* Record the name */
    if ((sexp -> value.b.name = ELVIN_STRDUP(name, error)) == NULL)
    {
	ELVIN_FREE(sexp, NULL);
	return NULL;
    }

    /* Record the function */
    sexp -> value.b.function = function;
    return sexp;
}

/* Allocates and initializes a new lambda sexp */
sexp_t lambda_alloc(env_t env, sexp_t arg_list, sexp_t body, elvin_error_t error)
{
    sexp_t sexp;

    /* Allocate an sexp */
    if ((sexp = sexp_alloc(SEXP_LAMBDA, error)) == NULL)
    {
	return NULL;
    }

    /* Record the interesting bits */
    sexp -> value.l.env = env;
    sexp -> value.l.arg_list = arg_list;
    sexp -> value.l.body = body;
    return sexp;
}


/* Returns an sexp's type */
sexp_type_t sexp_get_type(sexp_t sexp)
{
    return sexp -> type;
}

/* Allocates another reference to an sexp */
int sexp_alloc_ref(sexp_t sexp, elvin_error_t error)
{
    sexp -> ref_count++;
    return 1;
}

/* Frees an sexp */
int sexp_free(sexp_t sexp, elvin_error_t error)
{
    /* Delete a reference to the sexp */
    if (--sexp -> ref_count > 0)
    {
	return 1;
    }

    /* Then free the sexp */
    switch (sexp -> type)
    {
	case SEXP_NIL:
	{
	    fprintf(stderr, PACKAGE ": attempted to free `nil'\n");
	    abort();
	}

	case SEXP_STRING:
	{
	    int result;

	    result = ELVIN_FREE(sexp -> value.s, error);
	    return ELVIN_FREE(sexp, result ? error : NULL) && result;
	}

	case SEXP_SYMBOL:
	{
	    int result;
	    char *name;

	    /* Get the symbol's name */
	    if ((name = symbol_name(sexp, error)) == NULL)
	    {
		return 0;
	    } 

	    /* Remove the symbol from the symbol table */
	    result = elvin_hash_delete(symbol_table, (elvin_hashdata_t)name, error);
	    result = ELVIN_FREE(sexp -> value.s, result ? error : NULL) && result;
	    return ELVIN_FREE(sexp, result ? error : NULL) && result;
	}

	case SEXP_CONS:
	{
	    int result;

	    result = sexp_free(sexp -> value.c.car, error);
	    result = sexp_free(sexp -> value.c.cdr, result ? error : NULL) && result;
	    return ELVIN_FREE(sexp, result ? error : NULL) && result;
	}

	case SEXP_BUILTIN:
	{
	    int result;

	    result = ELVIN_FREE(sexp -> value.b.name, error);
	    return ELVIN_FREE(sexp, result ? error : NULL) && result;
	}

	case SEXP_LAMBDA:
	{
	    int result;

	    result = env_free(sexp -> value.l.env, error);
	    result = sexp_free(sexp -> value.l.arg_list, error);
	    result = sexp_free(sexp -> value.l.body, result ? error : NULL) && result;
	    return ELVIN_FREE(sexp, result ? error : NULL) && result;
	}

	default:
	{
	    return ELVIN_FREE(sexp, error);
	}
    }
}

/* Prints the body of a list */
static void print_cdrs(sexp_t sexp)
{
    while (sexp -> type == SEXP_CONS)
    {
	/* Print the car */
	fputc(' ', stdout);
	sexp_print(sexp -> value.c.car);
	sexp = sexp -> value.c.cdr;
    }

    /* Watch for dotted lists */
    if (sexp -> type != SEXP_NIL)
    {
	fputs(" . ", stdout);
	sexp_print(sexp);
    }
}

/* Prints an sexp to stdout */
int sexp_print(sexp_t sexp)
{
    switch (sexp -> type)
    {
	case SEXP_NIL:
	{
	    printf("nil");
	    return 1;
	}

	case SEXP_INT32:
	{
	    printf("%d", sexp -> value.i);
	    return 1;
	}

	case SEXP_INT64:
	{
	    printf("%" INT64_PRINT, sexp -> value.h);
	    return 1;
	}

	case SEXP_FLOAT:
	{
	    printf("%f", sexp -> value.d);
	    return 1;
	}

	case SEXP_STRING:
	{
	    printf("\"%s\"", sexp -> value.s);
	    return 1;
	}

	case SEXP_CHAR:
	{
	    printf("?%c", sexp -> value.i);
	    return 1;
	}

	case SEXP_SYMBOL:
	{
	    printf("%s", sexp -> value.s);
	    return 1;
	}

	case SEXP_CONS:
	{
	    fputc('(', stdout);
	    sexp_print(sexp -> value.c.car);
	    print_cdrs(sexp -> value.c.cdr);
	    fputc(')', stdout);
	    return 1;
	}

	case SEXP_LAMBDA:
	{
	    printf("(lambda ");
	    sexp_print(sexp -> value.l.arg_list);
	    print_cdrs(sexp -> value.l.body);
	    fputc(')', stdout);
	    return 1;
	}

	case SEXP_BUILTIN:
	{
	    printf("<built-in %s>", sexp -> value.b.name);
	    return 1;
	}

	default:
	{
	    fprintf(stderr, "unknown sexp type: %d\n", sexp -> type);
	    return 0;
	}
    }
}


/* Evaluates a function */
int funcall(sexp_t function, env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    /* What we do depends on the type */
    switch (function->type)
    {
	/* Built-in function? */
	case SEXP_BUILTIN:
	{
	    return function->value.b.function(env, args, result, error);
	}

	/* Lambda expression? */
	case SEXP_LAMBDA:
	{

	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "lambda_eval");
	    return 0;
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a function");
	    return 0;
	}
    }
}

/* Evaluates an sexp */
int sexp_eval(sexp_t sexp, env_t env, sexp_t *result, elvin_error_t error)
{
    switch (sexp -> type)
    {
	/* Most things evaluate to themselves */
	case SEXP_NIL:
	case SEXP_INT32:
	case SEXP_INT64:
	case SEXP_FLOAT:
	case SEXP_STRING:
	case SEXP_CHAR:
	case SEXP_BUILTIN:
	case SEXP_LAMBDA:
	{
	    sexp->ref_count++;
	    *result = sexp;
	    return 1;
	}

	/* Symbols get extracted from the environment */
	case SEXP_SYMBOL:
	{
	    /* Look up the symbol's value in the environment */
	    if (env_get(env, sexp, result, error) == 0)
	    {
		return 0;
	    }

	    /* Increment its reference count */
	    (*result) -> ref_count++;
	    return 1;
	}

	case SEXP_CONS:
	{
	    sexp_t function;
	    int r;

	    /* First we evaluate the car to get the function */
	    if (sexp_eval(cons_car(sexp, error), env, &function, error) == 0)
	    {
		return 0;
	    }

	    /* Then we call the function with the args */
	    r = funcall(function, env, cons_cdr(sexp, error), result, error);

	    /* Free our reference to the function */
	    if (sexp_free(function, error) == 0)
	    {
		return 0;
	    }

	    return r;
	}

	default:
	{
	    /* FIX THIS: set a better error */
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "sexp_eval");
	    return 0;
	}
    }
}

/* Allocates and initializes an environment */
env_t env_alloc(uint32_t size, env_t parent, elvin_error_t error)
{
    env_t env;

    /* Allocate some memory for the new environment */
    if ((env = (env_t)ELVIN_MALLOC(sizeof(struct env), error)) == NULL)
    {
	return NULL;
    }

    /* Allocate a hashtable to map symbols to their values */
    if ((env -> map = elvin_hash_alloc(
	     size,
	     symbol_hashfunc,
	     symbol_hashequal,
	     symbol_hashcopy,
	     symbol_hashfree,
	     sexp_hashcopy,
	     sexp_hashfree,
	     error)) == NULL)
    {
	ELVIN_FREE(env, NULL);
	return NULL;
    }

    env -> ref_count = 1;
    env -> parent = parent;
    return env;
}

/* Allocates another reference to the environment */
int env_alloc_ref(env_t env, elvin_error_t error)
{
    env -> ref_count++;
    return 1;
}

/* Frees an environment and all of its references */
int env_free(env_t env, elvin_error_t error)
{
    int result = 1;

    /* Decrement the reference count */
    if (--env -> ref_count > 0)
    {
	return 1;
    }

    /* Free the hashtable */
    if (env -> map)
    {
	result = elvin_hash_free(env -> map, result ? error : NULL) && result;
    }

    /* Free the environment */
    return ELVIN_FREE(env, result ? error : NULL) && result;
}

/* Looks up a symbol's value in the environment */
int env_get(env_t env, sexp_t symbol, sexp_t *result, elvin_error_t error)
{
    /* Keep checking until we run out of parent environments */
    while (env != NULL)
    {
	/* Look up the value in the environment */
	if ((*result = (sexp_t)elvin_hash_get(
		 env -> map,
		 (elvin_hashkey_t)symbol,
		 error)) != NULL)
	{
	    return 1;
	}

	env = env -> parent;
    }

    ELVIN_ERROR_LISP_SYM_UNDEF(error, symbol_name(symbol, error));
    return 0;
}

/* Sets a symbol's value in the environment */
int env_set(env_t env, sexp_t symbol, sexp_t value, elvin_error_t error)
{
    /* Make sure it isn't in the hashtable */
    elvin_hash_delete(env -> map, (elvin_hashkey_t)symbol, error);

    /* Use it to register the value in the map (automatically increases the ref_count) */
    if (elvin_hash_add(
	    env -> map,
	    (elvin_hashkey_t)symbol,
	    (elvin_hashdata_t)value,
	    error) == 0)
    {
	return 0;
    }

    return 1;
}

/* Sets the named symbol's value */
int env_set_value(env_t env, char *name, sexp_t value, elvin_error_t error)
{
    sexp_t symbol;
    int result;

    /* Locate the symbol */
    if ((symbol = symbol_alloc(name, error)) == NULL)
    {
	return 0;
    }

    /* Set its value */
    result = env_set(env, symbol, value, error);

    /* Lose our reference to the symbol */
    return sexp_free(symbol, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the int32 value in env */
int env_set_int32(env_t env, char *name, int32_t value, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create an int32 sexp */
    if ((sexp = int32_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}


/* Sets the named symbol's value to the int64 value in env */
int env_set_int64(env_t env, char *name, int64_t value, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create an int64 sexp */
    if ((sexp = int64_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the float value in env */
int env_set_float(env_t env, char *name, double value, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create a float sexp */
    if ((sexp = float_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}


/* Sets the named symbol's value in the environment */
int env_set_string(env_t env, char *name, char *value, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create a string */
    if ((sexp = string_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the symbol in env */
int env_set_symbol(env_t env, char *name, char *value, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create a symbol */
    if ((sexp = symbol_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the built-in function in env */
int env_set_builtin(env_t env, char *name, builtin_t function, elvin_error_t error)
{
    sexp_t sexp;
    int result;

    /* Create a built-in */
    if ((sexp = builtin_alloc(name, function, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, sexp, error);

    /* Lose our reference to it */
    return sexp_free(sexp, result ? error : NULL) && result;
}
