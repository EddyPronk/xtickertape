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
static const char cvsid[] = "$Id: sexp.c,v 2.17 2000/11/17 04:04:31 phelps Exp $";
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
    sexp_t env;
    sexp_t arg_list;
    sexp_t body;
};

/* A (non-root) environment */
struct env
{
    sexp_t parent;
    sexp_t arg_names;
    uint32_t arg_count;
    sexp_t args[1];
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
	int64_t h;
	double d;
	char *s;
	char ch;
	struct cons c;
	struct builtin b;
	struct lambda l;
	struct env e;
    } value;
};


/* Some predefined sexps */
static struct sexp nil = { SEXP_NIL, 1, { 0 } };


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
    /* Wrap up the value as a pointer */
    return (sexp_t)((value << 1) | 1);
}

/* Answers the integer's value */
int32_t int32_value(sexp_t sexp, elvin_error_t error)
{
    return ((int32_t)sexp) >> 1;
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

/* Returns the float's value */
double float_value(sexp_t sexp, elvin_error_t error)
{
    return sexp -> value.d;
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

    sexp -> value.ch = ch;
    return sexp;
}

/* Answers the char's char */
char char_value(sexp_t sexp, elvin_error_t error)
{
    return sexp->value.ch;
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
	if (sexp_get_type(car) == SEXP_NIL)
	{
	    return sexp;
	}

	cdr = sexp;
	sexp = car;
    }
}

/* Locates an entry in an alist */
int assoc(sexp_t symbol, sexp_t alist, sexp_t *result, elvin_error_t error)
{
    /* Keep going until we run out of elements or find a match */
    while (sexp_get_type(alist) == SEXP_CONS)
    {
	*result = cons_car(alist, error);

	/* Does the association match? */
	if (cons_car(*result, error) == symbol)
	{
	    return 1;
	}

	/* Go on to the next association */
	alist = cons_cdr(alist, error);
    }

    /* No match.  Return nil */
    *result = nil_alloc(error);
    return 1;
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
sexp_t lambda_alloc(sexp_t env, sexp_t arg_list, sexp_t body, elvin_error_t error)
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
    if (((int32_t)sexp) & 1)
    {
	return SEXP_INT32;
    }

    return sexp -> type;
}

/* Allocates another reference to an sexp */
int sexp_alloc_ref(sexp_t sexp, elvin_error_t error)
{
    sexp_type_t type = sexp_get_type(sexp);

    if (type == SEXP_INT32)
    {
	return 1;
    }

    sexp -> ref_count++;
    return 1;
}

/* Frees an sexp */
int sexp_free(sexp_t sexp, elvin_error_t error)
{
    sexp_type_t type = sexp_get_type(sexp);

    /* Never free an int32 */
    if (type == SEXP_INT32)
    {
	return 1;
    }

    /* Delete a reference to the sexp */
    if (--sexp -> ref_count > 0)
    {
	return 1;
    }

    /* Then free the sexp */
    switch (type)
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

	    result = sexp_free(sexp -> value.l.env, error);
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
    while (sexp_get_type(sexp) == SEXP_CONS)
    {
	/* Print the car */
	fputc(' ', stdout);
	sexp_print(sexp -> value.c.car);
	sexp = sexp -> value.c.cdr;
    }

    /* Watch for dotted lists */
    if (sexp_get_type(sexp) != SEXP_NIL)
    {
	fputs(" . ", stdout);
	sexp_print(sexp);
    }
}

/* Prints an sexp to stdout */
int sexp_print(sexp_t sexp)
{
    switch (sexp_get_type(sexp))
    {
	case SEXP_NIL:
	{
	    printf("nil");
	    return 1;
	}

	case SEXP_INT32:
	{
	    printf("%d", int32_value(sexp, NULL));
	    return 1;
	}

	case SEXP_INT64:
	{
	    printf("%" INT64_PRINT "L", sexp -> value.h);
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
	    printf("?%c", sexp -> value.ch);
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


/* Returns the length of an sexp (list or string) */
int sexp_length(sexp_t sexp, uint32_t *length_out, elvin_error_t error)
{
    switch (sexp_get_type(sexp))
    {
	/* Strings have lengths */
	case SEXP_STRING:
	{
	    return strlen(sexp -> value.s);
	}

	/* We let `nil' count as a zero-length list */
	case SEXP_NIL:
	{
	    *length_out = 0;
	    return 1;
	}

	/* Only properly formatted lists have lengths */
	case SEXP_CONS:
	{
	    *length_out = 0;
	    while (sexp_get_type(sexp) == SEXP_CONS)
	    {
		sexp = cons_cdr(sexp, error);
		(*length_out)++;
	    }

	    /* Make sure we've got a proper list */
	    if (sexp_get_type(sexp) != SEXP_NIL)
	    {
		ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bogus list");
		return 0;
	    }

	    return 1;
	}

	/* Nothing else works */
	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad type");
	    return 0;
	}
    }
}

/* Initialize an environment from an arg list and a set of args */
static int init_env_with_args(
    sexp_t env,
    sexp_t eval_env,
    sexp_t arg_list,
    sexp_t args,
    elvin_error_t error)
{
    /* Go through each of the args in arg_list */
    while (sexp_get_type(arg_list) == SEXP_CONS)
    {
	sexp_t value;

	/* Make sure we have enough args */
	if (sexp_get_type(args) != SEXP_CONS)
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	    return 0;
	}

	/* Evaluate the value */
	if (sexp_eval(cons_car(args, error), eval_env, &value, error) == 0)
	{
	    return 0;
	}

	/* Assign it in the environment */
	if (env_set(env, cons_car(arg_list, error), value, error) == 0)
	{
	    return 0;
	}

	/* Go on to the next arg */
	arg_list = cons_cdr(arg_list, error);
	args = cons_cdr(args, error);
    }

    /* Make sure we terminate cleanly */
    if (sexp_get_type(arg_list) != SEXP_NIL || sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad args");
	return 0;
    }

    return 1;
}


/* Evaluates a function */
static int funcall(
    sexp_t function,
    sexp_t env,
    sexp_t args,
    sexp_t *result,
    elvin_error_t error)
{
    /* What we do depends on the type */
    switch (sexp_get_type(function))
    {
	/* Built-in function? */
	case SEXP_BUILTIN:
	{
	    return function -> value.b.function(env, args, result, error);
	}

	/* Lambda expression? */
	case SEXP_LAMBDA:
	{
	    sexp_t lambda_env;
	    sexp_t body;

	    /* Create an environment for execution */
	    if ((lambda_env = env_alloc(
		     function -> value.l.env,
		     function -> value.l.arg_list,
		     args,
		     error)) == NULL)
	    {
		return 0;
	    }

	    /* Evaluate each sexp in the body of the lambda */
	    body = function -> value.l.body;
	    while (sexp_get_type(body) == SEXP_CONS)
	    {
		if (sexp_eval(cons_car(body, error), lambda_env, result, error) == 0)
		{
		    return 0;
		}

		body = cons_cdr(body, error);
	    }

	    /* Make sure we end cleanly */
	    if (sexp_get_type(body) != SEXP_NIL)
	    {
		ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "dotted func body");
		sexp_free(lambda_env, NULL);
		return 0;
	    }

	    /* Free the environment */
	    if (sexp_free(lambda_env, error) == 0)
	    {
		return 0;
	    }

	    return 1;
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a function");
	    return 0;
	}
    }
}

/* Evaluates an sexp */
int sexp_eval(sexp_t sexp, sexp_t env, sexp_t *result, elvin_error_t error)
{
    switch (sexp_get_type(sexp))
    {
	/* Most things evaluate to themselves */
	case SEXP_NIL:
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

	/* Integers evaluate to themselves but don't do reference counting */
	case SEXP_INT32:
	{
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
	    sexp_alloc_ref(*result, error);
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
sexp_t root_env_alloc(elvin_error_t error)
{
    /* For now, the root environment is an a-list */
    return cons_alloc(nil_alloc(error), nil_alloc(error), error);
}

/* Allocates a non-root environment with the given arg list and values */
sexp_t env_alloc(sexp_t parent, sexp_t arg_names, sexp_t args, elvin_error_t error)
{
    uint32_t length, i;
    sexp_t sexp;

    /* Count the number of args in the arg_list (and verify that it ends nicely) */
    if (! sexp_length(arg_names, &length, error))
    {
	return NULL;
    }

    /* Create the environment with enough space for the values */
    if ((sexp = (sexp_t)ELVIN_MALLOC(sizeof(struct sexp) + length * sizeof(sexp_t), error)) == NULL)
    {
	return NULL;
    }

    /* Record the type */
    sexp -> type = SEXP_ENV;
    sexp -> ref_count = 1;

    /* Copy the values into place */
    for (i = 0; i < length; i++)
    {
	sexp_t arg;

	/* Make sure we have a list */
	if (sexp_get_type(args) != SEXP_CONS)
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	    return NULL;
	}

	/* Copy the arg */
	arg = cons_car(args, error);
	sexp_alloc_ref(arg, error);
	sexp -> value.e.args[i] = arg;

	/* Move on to the next arg */
	args = cons_cdr(args, error);
    }

    /* Make sure we're ok */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return NULL;
    }

    /* Record the parent */
    sexp -> value.e.parent = parent;
    sexp_alloc_ref(parent, error);

    /* Record the arg names */
    sexp -> value.e.arg_names = arg_names;
    sexp_alloc_ref(arg_names, error);

    /* Record the number of args */
    sexp -> value.e.arg_count = length;
    return sexp;
}


/* Returns the index into the list of the given value */
int list_index(sexp_t list, sexp_t value, uint32_t *result, elvin_error_t error)
{
    *result = 0;

    /* Look through the list starting from the beginning */
    while (sexp_get_type(list) == SEXP_CONS)
    {
	/* Only do pointer equality for now */
	if (cons_car(list, error) == value)
	{
	    return 1;
	}

	list = cons_cdr(list, error);
	(*result)++;
    }

    return 0;
}

/* Looks up a symbol's value in the environment */
int env_get(sexp_t env, sexp_t symbol, sexp_t *result, elvin_error_t error)
{
    /* Keep checking until we get to the root environment */
    while (sexp_get_type(env) == SEXP_ENV)
    {
	uint32_t i;

	/* Check through the args to see if we find a match */
	if (list_index(env -> value.e.arg_names, symbol, &i, error) != 0)
	{
	    *result = env -> value.e.args[i];
	    return 1;
	}

	env = env -> value.e.parent;
    }

    /* Sanity check: the root environment should be a cons cell */
    if (sexp_get_type(env) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bogus environment");
	return 0;
    }

    /* Check in the root environment */
    if (assoc(symbol, cons_cdr(env, error), result, error) == 0)
    {
	return 0;
    }

    /* Did we find a match */
    if (*result != &nil)
    {
	*result = cons_cdr(*result, error);
	return 1;
    }

    /* No match */
    ELVIN_ERROR_LISP_SYM_UNDEF(error, symbol_name(symbol, error));
    return 0;
}

/* Sets a symbol's value in the environment */
int env_set(sexp_t env, sexp_t symbol, sexp_t value, elvin_error_t error)
{
    sexp_t pair;
    sexp_t alist;

    /* Hunt for the variable in non-root environments */
    while (sexp_get_type(env) == SEXP_ENV)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "env_set");
	return 0;
    }

    /* Sanity check: the root environment should be a cons cell */
    if (sexp_get_type(env) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bogus environment");
	return 0;
    }

    /* Create a new association */
    if ((pair = cons_alloc(symbol, value, error)) == NULL)
    {
	return 0;
    }

    /* Prepend it to the list */
    /* FIX THIS: this is a hideous hack -- it doesn't remove the old assocation! */
    alist = cons_alloc(pair, cons_cdr(env, error), error);
    env -> value.c.cdr = alist;

    /* Allocate some references */
    sexp_alloc_ref(symbol, error);
    sexp_alloc_ref(value, error);
    return 1;
}

/* Sets a symbol's value in the appropriate environment */
int env_assign(sexp_t env, sexp_t symbol, sexp_t value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "env_assign");
    return 0;
}

/* Sets the named symbol's value */
int env_set_value(sexp_t env, char *name, sexp_t value, elvin_error_t error)
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
int env_set_int32(sexp_t env, char *name, int32_t value, elvin_error_t error)
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
int env_set_int64(sexp_t env, char *name, int64_t value, elvin_error_t error)
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
int env_set_float(sexp_t env, char *name, double value, elvin_error_t error)
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
int env_set_string(sexp_t env, char *name, char *value, elvin_error_t error)
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
int env_set_symbol(sexp_t env, char *name, char *value, elvin_error_t error)
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
int env_set_builtin(sexp_t env, char *name, builtin_t function, elvin_error_t error)
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
