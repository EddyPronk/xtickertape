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
static const char cvsid[] = "$Id: vm.c,v 2.2 2000/11/17 13:17:49 phelps Exp $";
#endif

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/error.h>
#include <elvin/errors/elvin.h>
#include "vm.h"

/* The heap can contain this many pointers */
#define HEAP_BLOCK_SIZE (1024)
#define STACK_SIZE (1024)
#define SYMTAB_SIZE (1024)
#define POINTER_SIZE (sizeof(void *))

#define LENGTH_MASK 0xFFFF0000
#define TYPE_MASK 0x0000F000
#define FLAGS_MASK 0x00000FFE
#define INTEGER_MASK 0x00000001


/* Objects are just pointers */
typedef void **object_t;

/* Handles allow us to refer to GC objects from outside GC space */
struct vm_object
{
    vm_object_t next;
    object_t object;
};


/* The structure of the virtual machine */
struct vm
{
    /* All external pointers into the VM */
    vm_object_t root_set;

    /* The symbol table */
    object_t symbol_table;

    /* The virtual machine's heap */
    object_t heap;

    /* The next free spot in the heap */
    object_t heap_next;

    /* The bottom of the stack */
    object_t stack;

    /* The current stack frame */
    uint32_t sp;

    /* The top of the stack */
    uint32_t top;

    /* The program counter */
    uint32_t pc;
};


/* Allocates and initializes a new virtual machine */
vm_t vm_alloc(elvin_error_t error)
{
    vm_t self;

    /* Allocate some room for the VM itself */
    if (! (self = ELVIN_MALLOC(sizeof(struct vm), error)))
    {
	return NULL;
    }

    /* Initialize its fields to safe values */
    memset(self, 0, sizeof(struct vm));

    /* Allocate an initial heap */
    if (! (self -> heap = ELVIN_MALLOC(HEAP_BLOCK_SIZE * POINTER_SIZE, error)))
    {
	ELVIN_FREE(self, NULL);
	return NULL;
    }
    memset(self -> heap, 0, HEAP_BLOCK_SIZE * POINTER_SIZE);

    /* Point to the first free part of the heap */
    self -> heap_next = self -> heap + 1;

    /* Allocate a symbol table */
    if (! (self -> symbol_table = ELVIN_MALLOC(SYMTAB_SIZE * POINTER_SIZE, error)))
    {
	ELVIN_FREE(self -> heap, NULL);
	ELVIN_FREE(self, NULL);
	return NULL;
    }
    memset(self -> symbol_table, 0, SYMTAB_SIZE * POINTER_SIZE);

    /* Allocate an initial stack */
    if (! (self -> stack = ELVIN_MALLOC(STACK_SIZE * POINTER_SIZE, error)))
    {
	ELVIN_FREE(self -> symbol_table, NULL);
	ELVIN_FREE(self -> heap, NULL);
	ELVIN_FREE(self, NULL);
	return NULL;
    }

    return self;
}

/* Frees a virtual machine */
int vm_free(vm_t self, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_free");
    return 0;
}


/* Returns the type of the given object */
static object_type_t object_type(object_t object)
{
    uint32_t header;

    /* Check for NIL */
    if (object == NULL)
    {
	return SEXP_NIL;
    }

    /* Check for encoded integers */
    if (((int32_t)object & 1) == 1)
    {
	return SEXP_INTEGER;
    }

    /* Otherwise look in the object's header */
    header = (uint32_t)(*object);
    return (header >> 12) & 0xF;
}

/* Returns the object on the top of the stack */
object_t vm_top(vm_t self, elvin_error_t error)
{
    if (self -> top < 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!\n");
	return NULL;
    }

    return self -> stack[self -> top - 1];
}



/* Swaps the top two elements on the stack */
int vm_swap(vm_t self, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_swap");
    return 0;
}

/* Pushes an object onto the vm's stack */
int vm_push(vm_t self, object_t object, elvin_error_t error)
{
    /* FIX THIS: check bounds on stack */
    self -> stack[self -> top++] = object;
    return 1;
}

/* Creates a new object in the heap and pushes it onto the stack */
int vm_new(
    vm_t self,
    uint32_t size,
    object_type_t type,
    uint16_t flags,
    elvin_error_t error)
{
    object_t object;
    uint32_t header;

    /* See if there's space on the heap */
    if (! (self -> heap_next + size + 1 < self -> heap + 1 + HEAP_BLOCK_SIZE))
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "out of memory\n");
	return 0;
    }

    /* Record the object pointer and update the heap poiner */
    object = self -> heap_next;
    self -> heap_next += size + 1;

    /* Write the object header (it pretends to be an integer) */
    header = (size & 0xFFFF) << 16 | (type & 0xF) << 12 | (flags & 0x7FF) << 1 | 1;
    object[0] = (void *)header;

    /* Push the object onto the stack */
    return vm_push(self, object, error);
}

/* Push nil onto the vm's stack */
int vm_push_nil(vm_t self, elvin_error_t error)
{
    return vm_push(self, NULL, error);
}

/* Pushes an integer onto the vm's stack */
int vm_push_integer(vm_t self, int32_t value, elvin_error_t error)
{
    return vm_push(self, (object_t)((value << 1) | 1), error);
}

/* Pushes a long integer onto the vm's stack */
int vm_push_long(vm_t self, int64_t value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_push_long");
    return 0;
}

/* Pushes a float onto the vm's stack */
int vm_push_float(vm_t self, double value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_push_float");
    return 0;
}

/* Pushes a string onto the vm's stack */
int vm_push_string(vm_t self, char *value, elvin_error_t error)
{
    uint32_t length = strlen(value);
    uint32_t size = 1 + length / POINTER_SIZE;
    object_t object;

    /* Create a string object on the heap */
    if (! vm_new(self, size, SEXP_STRING, size * POINTER_SIZE - length, error))
    {
	return 0;
    }

    /* Grab the object off of the top of the stack */
    if (! (object = vm_top(self, error)))
    {
	return 0;
    }

    /* Copy the string bytes into place */
    memcpy(object + 1, value, length + 1);
    return 1;
}

/* Pushes a char onto the vm's stack */
int vm_push_char(vm_t self, int value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_push_char");
    return 0;
}

/* Makes a symbol out of the string on the top of the stack */
int vm_make_symbol(vm_t self, elvin_error_t error)
{
    object_t string;
    object_t *pointer = (object_t *)self -> symbol_table;
    uint32_t header;

    /* Grab that string from the top of the stack */
    if (! (string = vm_top(self, error)))
    {
	return 0;
    }

    /* Keep going until we reach the end of the symbol table */
    while (*pointer != NULL)
    {
	/* Do we have a match? */
	if (strcmp((char *)(string + 1), (char *)(*pointer + 1)) == 0)
	{
	    /* Pop the string off the stack */
	    self -> top--;

	    /* Push the symbol on */
	    return vm_push(self, *pointer, error);
	}

	/* Keep looking */
	pointer++;
    }

    /* No such symbol.  Transform the string into a symbol. */
    header = (uint32_t)*string;
    header = (header & ~TYPE_MASK) | (SEXP_SYMBOL << 12);
    *string = (void *)header;

    /* Write it on the end of the symbol table */
    *pointer = string;
    return 1;
}


/* Creates a cons cell out of the top two elements on the stack */
int vm_make_cons(vm_t self, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_make_cons");
    return 0;
}

/* Reverses the pointers in a list that was constructed upside-down */
int vm_unwind_list(vm_t self, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_unwind_list");
    return 0;
}



/* Prints a single object */
void do_print(object_t object)
{
    switch (object_type(object))
    {
	case SEXP_NIL:
	{
	    printf("nil");
	    break;
	}

	case SEXP_INTEGER:
	{
	    printf("%d", ((int32_t)object) >> 1);
	    break;
	}

	case SEXP_LONG:
	{
	    printf("<long>");
	    break;
	}

	case SEXP_FLOAT:
	{
	    printf("<float>");
	    break;
	}

	case SEXP_STRING:
	{
	    printf("\"%s\"", (char *)(object + 1));
	    break;
	}

	case SEXP_CHAR:
	{
	    printf("<char>");
	    break;
	}

	case SEXP_SYMBOL:
	{
	    printf("%s [%p]\n", (char *)(object + 1), object);
	    break;
	}

	case SEXP_CONS:
	{
	    printf("<cons>");
	    break;
	}

	case SEXP_PRIM:
	{
	    printf("<prim>");
	    break;
	}

	case SEXP_LAMBDA:
	{
	    printf("<lambda>");
	    break;
	}

	case SEXP_ENV:
	{
	    printf("<env>");
	    break;
	}

	case SEXP_ARRAY:
	{
	    printf("<array>");
	    break;
	}

	default:
	{
	    printf("<unknown>");
	    break;
	}
    }
}

/* For debugging only */
int vm_print_stack(vm_t self, elvin_error_t error)
{
    uint32_t i;

    /* print each stack item */
    for (i = 0; i < self -> top; i++)
    {
	printf("%d: ", i);
	do_print(self -> stack[i]);
	printf("\n");
    }

    fflush(stdout);
    return 1;
}

