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
static const char cvsid[] = "$Id: atom.c,v 2.7 2000/11/06 12:33:47 phelps Exp $";
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
#include "atom.h"

/* A cons cell has two values */
struct cons
{
    atom_t car;
    atom_t cdr;
};

/* An atom can be many things */
struct atom
{
    /* The type determines all */
    atom_type_t type;

    /* The number of references to this atom */
    uint32_t ref_count;

    /* Everything else is part of the big union */
    union
    {
	int32_t i;
	int64_t h;
	double d;
	uchar *s;
	struct cons c;
    } value;
};

/* An environment is a hashtable and a parent */
struct env
{
    /* The parent environment */
    env_t parent;

    /* A mapping of symbols to their values */
    elvin_hashtable_t map;
};


/* Some predefined atoms */
static struct atom nil = { ATOM_NIL, 1, { 0 } };
static struct atom minus_one = { ATOM_INT32, 1, { -1 } };
static struct atom numbers[] =
{
    { ATOM_INT32, 1, { 0 } },
    { ATOM_INT32, 1, { 1 } },
    { ATOM_INT32, 1, { 2 } },
    { ATOM_INT32, 1, { 3 } },
    { ATOM_INT32, 1, { 4 } },
    { ATOM_INT32, 1, { 5 } },
    { ATOM_INT32, 1, { 6 } },
    { ATOM_INT32, 1, { 7 } },
    { ATOM_INT32, 1, { 8 } },
    { ATOM_INT32, 1, { 9 } }
};

/* Grabs a reference to an atom */
static int hashcopy(elvin_hashtable_t table,
		    elvin_hashdata_t *copy_out,
		    elvin_hashdata_t data,
		    elvin_error_t error)
{
    atom_t atom = (atom_t)data;
    atom -> ref_count++;
    *copy_out = data;
    return 1;
}

/* Frees a reference to an atom */
static int hashfree(elvin_hashdata_t data, elvin_error_t error)
{
    return atom_free((atom_t)data, error);
}


/* Answers the unique nil instance */
atom_t nil_alloc(elvin_error_t error)
{
    nil.ref_count++;
    return &nil;
}


/* Allocates a new atom */
atom_t atom_alloc(atom_type_t type, elvin_error_t error)
{
    atom_t atom;

    /* Allocate some memory for the new atom */
    if ((atom = (atom_t)ELVIN_MALLOC(sizeof(struct atom), error)) == NULL)
    {
	return NULL;
    }

    atom -> ref_count = 1;
    atom -> type = type;
    return atom;
}

/* Allocates and initializes a new 32-bit integer atom */
atom_t int32_alloc(int32_t value, elvin_error_t error)
{
    atom_t atom;

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

    /* Allocate a new atom */
    if ((atom = atom_alloc(ATOM_INT32, error)) == NULL)
    {
	return NULL;
    }

    atom -> value.i = value;
    return atom;
}

/* Answers the integer's value */
int32_t int32_value(atom_t atom, elvin_error_t error)
{
    return atom -> value.i;
}

/* Allocates and initializes a new 64-bit integer atom */
atom_t int64_alloc(int64_t value, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_INT64, error)) == NULL)
    {
	return NULL;
    }

    atom -> value.h = value;
    return atom;
}

/* Answers the integer's value */
int64_t int64_value(atom_t atom, elvin_error_t error)
{
    return atom -> value.h;
}

/* Allocates and initializes a new float atom */
atom_t float_alloc(double value, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_FLOAT, error)) == NULL)
    {
	return NULL;
    }

    atom -> value.d = value;
    return atom;
}


/* Allocates and initializes a new string atom */
atom_t string_alloc(uchar *value, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_STRING, error)) == NULL)
    {
	return NULL;
    }

    /* Duplicate the value */
    if ((atom -> value.s = ELVIN_USTRDUP(value, error)) == NULL)
    {
	ELVIN_FREE(atom, NULL);
	return NULL;
    }

    return atom;
}

/* Answers the string's characters */
uchar *string_value(atom_t atom, elvin_error_t error)
{
    return atom -> value.s;
}

/* Allocates and initializes a new char atom */
atom_t char_alloc(uchar ch, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_CHAR, error)) == NULL)
    {
	return NULL;
    }

    atom -> value.i = (int32_t)ch;
    return atom;
}

/* Answers the char's char */
uchar char_value(atom_t atom, elvin_error_t error)
{
    return (uchar)atom->value.i;
}


/* Allocates and initializes a new symbol atom */
atom_t symbol_alloc(char *name, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_SYMBOL, error)) == NULL)
    {
	return NULL;
    }

    /* Duplicate the value */
    if ((atom -> value.s = ELVIN_USTRDUP(name, error)) == NULL)
    {
	ELVIN_FREE(atom, NULL);
	return NULL;
    }

    return atom;
}

/* Answers the symbol's name */
char *symbol_name(atom_t atom, elvin_error_t error)
{
    /* FIX THIS: complain if the atom is not a symbol */
    return atom -> value.s;
}

/* Allocates and initializes a new cons atom */
atom_t cons_alloc(atom_t car, atom_t cdr, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_CONS, error)) == NULL)
    {
	return NULL;
    }

    /* Copy the car and cdr into place */
    atom -> value.c.car = car;
    atom -> value.c.cdr = cdr;
    return atom;
}

/* Answers the car of a cons atom */
atom_t cons_car(atom_t atom, elvin_error_t error)
{
    return atom -> value.c.car;
}

/* Answers the cdr of a cons atom */
atom_t cons_cdr(atom_t atom, elvin_error_t error)
{
    return atom -> value.c.cdr;
}

/* Reverses a collection of cons cells */
atom_t cons_reverse(atom_t atom, atom_t end, elvin_error_t error)
{
    atom_t cdr = end;

    /* Keep going until the entire list is reversed */
    while (1)
    {
	atom_t car = atom -> value.c.car;

	/* Rotate the cdr into the car and end into the cdr */
	atom -> value.c.car = atom -> value.c.cdr;
	atom -> value.c.cdr = cdr;

	/* See if we're done */
	if (car -> type == ATOM_NIL)
	{
	    return atom;
	}

	cdr = atom;
	atom = car;
    }
}


/* Frees an atom */
int atom_free(atom_t atom, elvin_error_t error)
{
    /* Delete a reference to the atom */
    if (--atom -> ref_count > 0)
    {
	return 1;
    }

    /* Then free the atom */
    switch (atom -> type)
    {
	case ATOM_NIL:
	{
	    fprintf(stderr, PACKAGE ": attempted to free `nil'\n");
	    abort();
	}

	case ATOM_STRING:
	case ATOM_SYMBOL:
	{
	    int result;

	    result = ELVIN_FREE(atom -> value.s, error);
	    return ELVIN_FREE(atom, result ? error : NULL) && result;
	}

	case ATOM_CONS:
	{
	    int result;

	    result = atom_free(atom -> value.c.car, error);
	    result = atom_free(atom -> value.c.cdr, result ? error : NULL) && result;
	    return ELVIN_FREE(atom, error);
	}

	default:
	{
	    return ELVIN_FREE(atom, error);
	}
    }
}

/* Prints the body of a list */
static void print_cdrs(atom_t atom)
{
    while (atom -> type == ATOM_CONS)
    {
	/* Print the car */
	fputc(' ', stdout);
	atom_print(atom -> value.c.car);
	atom = atom -> value.c.cdr;
    }

    /* Watch for dotted lists */
    if (atom -> type != ATOM_NIL)
    {
	fputs(" . ", stdout);
	atom_print(atom);
    }
}

/* Prints an atom to stdout */
int atom_print(atom_t atom)
{
    switch (atom -> type)
    {
	case ATOM_NIL:
	{
	    printf("nil");
	    return 1;
	}

	case ATOM_INT32:
	{
	    printf("%d", atom -> value.i);
	    return 1;
	}

	case ATOM_INT64:
	{
	    printf("%" INT64_PRINT, atom -> value.h);
	    return 1;
	}

	case ATOM_FLOAT:
	{
	    printf("%f", atom -> value.d);
	    return 1;
	}

	case ATOM_STRING:
	{
	    printf("\"%s\"", atom -> value.s);
	    return 1;
	}

	case ATOM_CHAR:
	{
	    printf("?%c", atom -> value.i);
	    return 1;
	}

	case ATOM_SYMBOL:
	{
	    printf("%s", atom -> value.s);
	    return 1;
	}

	case ATOM_CONS:
	{
	    fputc('(', stdout);
	    atom_print(atom -> value.c.car);
	    print_cdrs(atom -> value.c.cdr);
	    fputc(')', stdout);
	    return 1;
	}

	default:
	{
	    fprintf(stderr, "unknown atom type: %d\n", atom -> type);
	    return 0;
	}
    }
}


/* Evaluates an atom */
int atom_eval(atom_t atom, env_t env, atom_t *result, elvin_error_t error)
{
    switch (atom -> type)
    {
	/* Most things evaluate to themselves */
	case ATOM_NIL:
	case ATOM_INT32:
	case ATOM_INT64:
	case ATOM_FLOAT:
	case ATOM_STRING:
	case ATOM_CHAR:
	{
	    atom->ref_count++;
	    *result = atom;
	    return 1;
	}

	/* Symbols get extracted from the environment */
	case ATOM_SYMBOL:
	{
	    /* Look up the symbol's value in the environment */
	    if (env_get(env, atom, result, error) == 0)
	    {
		return 0;
	    }

	    /* Increment its reference count */
	    (*result) -> ref_count++;
	    return 1;
	}

	case ATOM_CONS:
	{
	    nil.ref_count++;
	    *result = &nil;
	    return 1;
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, (uchar *)"atom_eval");
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
    /* FIX THIS: we should use a pointer hash and a symbol table */
    if ((env -> map = elvin_string_hash_create(size, hashcopy, hashfree, error)) == NULL)
    {
	ELVIN_FREE(env, NULL);
	return NULL;
    }

    env -> parent = parent;
    return env;
}

/* Frees an environment and all of its references */
int env_free(env_t env, elvin_error_t error)
{
    int result = 1;

    /* Free the hashtable */
    if (env -> map)
    {
	result = elvin_hash_free(env -> map, result ? error : NULL) && result;
    }

    /* Free the environment */
    return ELVIN_FREE(env, result ? error : NULL) && result;
}

/* Looks up a symbol's value in the environment */
int env_get(env_t env, atom_t symbol, atom_t *result, elvin_error_t error)
{
    char *name;

    /* Bail if the atom isn't a symbol */
    if ((name = symbol_name(symbol, error)) == NULL)
    {
	return 0;
    }

    /* Keep checking until we run out of parent environments */
    while (env != NULL)
    {
	/* Look up the value in the environment */
	if ((*result = (atom_t)elvin_hash_get(env -> map, (elvin_hashkey_t)name, error)) != NULL)
	{
	    /* Found it! */
	    return 1;
	}

	env = env -> parent;
    }

    ELVIN_ERROR_LISP_SYM_UNDEF(error, (uchar *)name);
    return 0;
}

/* Sets a symbol's value in the environment */
int env_set(env_t env, atom_t symbol, atom_t value, elvin_error_t error)
{
    char *name;

    /* Look up the symbol's name */
    if ((name = symbol_name(symbol, error)) == NULL)
    {
	return 0;
    }

    /* Use it to register the value in the map (automatically increases the ref_count) */
    if (elvin_hash_add(
	    env -> map,
	    (elvin_hashkey_t)name,
	    (elvin_hashdata_t)value,
	    error) == 0)
    {
	return 0;
    }

    return 1;
}

/* Sets the named symbol's value */
int env_set_value(env_t env, char *name, atom_t value, elvin_error_t error)
{
    atom_t symbol;
    int result;

    /* Locate the symbol */
    if ((symbol = symbol_alloc(name, error)) == NULL)
    {
	return 0;
    }

    /* Set its value */
    result = env_set(env, symbol, value, error);

    /* Lose our reference to the symbol */
    return atom_free(symbol, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the int32 value in env */
int env_set_int32(env_t env, char *name, int32_t value, elvin_error_t error)
{
    atom_t atom;
    int result;

    /* Create an int32 atom */
    if ((atom = int32_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, atom, error);

    /* Lose our reference to it */
    return atom_free(atom, result ? error : NULL) && result;
}


/* Sets the named symbol's value to the int64 value in env */
int env_set_int64(env_t env, char *name, int64_t value, elvin_error_t error)
{
    atom_t atom;
    int result;

    /* Create an int64 atom */
    if ((atom = int64_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, atom, error);

    /* Lose our reference to it */
    return atom_free(atom, result ? error : NULL) && result;
}

/* Sets the named symbol's value to the float value in env */
int env_set_float(env_t env, char *name, double value, elvin_error_t error)
{
    atom_t atom;
    int result;

    /* Create a float atom */
    if ((atom = float_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, atom, error);

    /* Lose our reference to it */
    return atom_free(atom, result ? error : NULL) && result;
}


/* Sets the named symbol's value in the environment */
int env_set_string(env_t env, char *name, char *value, elvin_error_t error)
{
    atom_t atom;
    int result;

    /* Create a string */
    if ((atom = string_alloc(value, error)) == NULL)
    {
	return 0;
    }

    /* Register it */
    result = env_set_value(env, name, atom, error);

    /* Lose our reference to it */
    return atom_free(atom, result ? error : NULL) && result;
}
