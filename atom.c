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
static const char cvsid[] = "$Id: atom.c,v 2.1 2000/11/01 02:07:02 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/error.h>
#include <elvin/errors/elvin.h>
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


/* The nil value is predefined */
static struct atom Qnil = { ATOM_NIL, { 0 } };

/* Answers the unique nil instance */
atom_t nil_alloc(elvin_error_t error)
{
    return &Qnil;
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

    atom -> type = type;
    return atom;
}

/* Allocates and initializes a new 32-bit integer atom */
atom_t int32_alloc(int32_t value, elvin_error_t error)
{
    atom_t atom;

    /* Allocate an atom */
    if ((atom = atom_alloc(ATOM_INT32, error)) == NULL)
    {
	return NULL;
    }

    atom -> value.i = value;
    return atom;
}

/* Answers the integer's value */
int32_t int32_value(atom_t atom)
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
int64_t int64_value(atom_t atom)
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
uchar *string_value(atom_t atom)
{
    return atom -> value.s;
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
char *symbol_name(atom_t atom)
{
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
atom_t cons_car(atom_t atom)
{
    return atom -> value.c.car;
}

/* Answers the cdr of a cons atom */
atom_t cons_cdr(atom_t atom)
{
    return atom -> value.c.cdr;
}

/* Reverses a collection of cons cells */
atom_t cons_reverse(atom_t atom, atom_t end)
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
    switch (atom -> type)
    {
	case ATOM_NIL:
	{
	    return 1;
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
	    printf("%e", atom -> value.d);
	    return 1;
	}

	case ATOM_STRING:
	{
	    printf("\"%s\"", atom -> value.s);
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
atom_t atom_eval(atom_t atom, elvin_error_t error)
{
    switch (atom -> type)
    {
	/* Most things evaluate to themselves */
	case ATOM_NIL:
	case ATOM_INT32:
	case ATOM_INT64:
	case ATOM_FLOAT:
	case ATOM_STRING:
	{
	    return atom;
	}

	/* Symbols get extracted from the environment */
	case ATOM_SYMBOL:
	{
	    return &Qnil;
	}

	case ATOM_CONS:
	{
	    return &Qnil;
	}
    }
}
