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
static const char cvsid[] = "$Id: vm.c,v 2.6 2000/11/18 04:07:16 phelps Exp $";
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
#define FLAGS_MASK 0x00000FFF



/* The structure of the virtual machine */
struct vm
{
    /* All external pointers into the VM */
    object_t root_set;

    /* The symbol table */
    object_t symbol_table;

    /* The virtual machine's heap */
    object_t heap;

    /* The next free spot in the heap */
    object_t heap_next;

    /* The bottom of the stack */
    object_t stack;

    /* --- CURRENT STATE --- */

    /* The current environment */
    object_t env;

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
	vm_free(self, NULL);
	return NULL;
    }
    memset(self -> heap, 0, HEAP_BLOCK_SIZE * POINTER_SIZE);

    /* Point to the first free part of the heap */
    self -> heap_next = self -> heap + 1;

    /* Allocate a symbol table */
    if (! (self -> symbol_table = ELVIN_MALLOC(SYMTAB_SIZE * POINTER_SIZE, error)))
    {
	vm_free(self, NULL);
	return NULL;
    }
    memset(self -> symbol_table, 0, SYMTAB_SIZE * POINTER_SIZE);

    /* Allocate an initial stack */
    if (! (self -> stack = ELVIN_MALLOC(STACK_SIZE * POINTER_SIZE, error)))
    {
	vm_free(self, NULL);
	return NULL;
    }

    /* Make an empty cons cell to be our root environment */
    if (! vm_push_nil(self, error) ||
	! vm_push_nil(self, error) ||
	! vm_make_cons(self, error) ||
	! vm_pop(self, &self -> env, error))
    {
	vm_free(self, NULL);
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
int vm_top(vm_t self, object_t *result, elvin_error_t error)
{
    if (self -> top < 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    *result = self -> stack[self -> top - 1];
    return 1;
}

/* Pops and returns the top of the stack */
int vm_pop(vm_t self, object_t *result, elvin_error_t error)
{
    /* Make sure the stack doesn't underflow */
    if (self -> top < 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    /* Pop the result if there's a place to put it */
    self -> top--;
    if (result)
    {
	*result = self -> stack[self -> top];
    }

    return 1;
}

/* Pushes an object onto the vm's stack */
int vm_push(vm_t self, object_t object, elvin_error_t error)
{
    /* FIX THIS: check bounds on stack */
    self -> stack[self -> top++] = object;
    return 1;
}

/* Swaps the top two elements on the stack */
int vm_swap(vm_t self, elvin_error_t error)
{
    object_t swap;

    if (self -> top < 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    swap = self -> stack[self -> top - 1];
    self -> stack[self -> top - 1] = self -> stack[self -> top - 2];
    self -> stack[self -> top - 2] = swap;
    return 1;
}

/* Rotates the stack so that the top is `count' places back */
int vm_rotate(vm_t self, uint32_t count, elvin_error_t error)
{
    object_t top;
    uint32_t i;

    if (self -> top < count + 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow");
	return 0;
    }

    /* Grab the top of the stack */
    top = self -> stack[self -> top - 1];

    /* Push the remaining elements down */
    for (i = 0; i < count; i++)
    {
	self -> stack[self -> top - 1 - i] = self -> stack[self -> top - 2 - i];
    }

    /* Push the top into the appropriate place */
    self -> stack[self -> top - 1 - count] = top;
    return 1;
}

/* Duplicates the top of the stack and puts it onto the stack */
int vm_dup(vm_t self, elvin_error_t error)
{
    object_t top;

    return vm_top(self, &top, error) && vm_push(self, top, error);
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
    header = (size & 0xFFFF) << 16 | (type & 0xF) << 12 | (flags & 0xFFF);
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
    object_t object;

    /* Create a new float object on the heap */
    if (! vm_new(self, sizeof(double) / POINTER_SIZE, SEXP_FLOAT, 0, error))
    {
	return 0;
    }

    /* Find the object on the top of the stack */
    if (! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Copy the value bytes into place */
    memcpy(object + 1, &value, sizeof(double));
    return 1;
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

    /* Find the object on the top of the stack */
    if (! vm_top(self, &object, error))
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
    if (! vm_top(self, &string, error))
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
    object_t cons;

    /* Make a new cons cell */
    if (! vm_new(self, 2, SEXP_CONS, 0, error))
    {
	return 0;
    }

    /* Pop it into our cons cell */
    if (! vm_pop(self, &cons, error))
    {
	return 0;
    }

    /* Pop its cdr */
    if (! vm_pop(self, (object_t *)(cons + 2), error))
    {
	return 0;
    }

    /* And its car */
    if (! vm_pop(self, (object_t *)(cons + 1), error))
    {
	return 0;
    }
    
    /* Push the cons cell back onto the stack */
    return vm_push(self, cons, error);
}

/* Pops a cons cell off the top of the stack and pushes its car on instead */
int vm_car(vm_t self, elvin_error_t error)
{
    object_t cons;
    return vm_pop(self, &cons, error) && vm_push(self, (object_t)cons[1], error);
}

/* Sets the car of a cons cell */
int vm_set_car(vm_t self, elvin_error_t error)
{
    object_t cons, car;

    /* Extract the car and cdr */
    if (! vm_pop(self, &car, error) || ! vm_top(self, &cons, error))
    {
	return 0;
    }

    /* Assign */
    cons[1] = car;
    return 1;
}

/* Sets the cdr of a cons cell */
int vm_set_cdr(vm_t self, elvin_error_t error)
{
    object_t cons, cdr;

    /* Extract the car and cdr */
    if (! vm_pop(self, &cdr, error) || ! vm_top(self, &cons, error))
    {
	return 0;
    }

    /* Assign */
    cons[2] = cdr;
    return 1;
}


/* Pops a cons cell off the top of the stack and pushes its car on instead */
int vm_cdr(vm_t self, elvin_error_t error)
{
    object_t cons;
    return vm_pop(self, &cons, error) && vm_push(self, (object_t)cons[2], error);
}

/* Reverses the pointers in a list that was constructed upside-down */
int vm_unwind_list(vm_t self, elvin_error_t error)
{
    object_t cons, car, cdr;

    /* Pop end of the list off the top of the stack */
    if (! vm_pop(self, &cdr, error))
    {
	return 0;
    }

    /* Pop the rest of the list */
    if (! vm_pop(self, &cons, error))
    {
	return 0;
    }

    /* Repeat until we hit the initial nil */
    while (cons != NULL)
    {
	car = (object_t)cons[1];
	cons[1] = cons[2];
	cons[2] = cdr;

	cdr = cons;
	cons = car;
    }

    /* Push the final cons cell back onto the stack */
    return vm_push(self, cdr, error);
}



/* Assigns the value on the top of the stack to the variable up one
 * place, leaving only the value on the stack */
int vm_assign(vm_t self, elvin_error_t error)
{
    /* FIX THIS: this is wrong for non-root environments */
    /* Duplicate the value and roll it up before the symbol */
    return
	vm_dup(self, error) &&
	vm_rotate(self, 2, error) &&
	vm_make_cons(self, error) &&
	vm_push(self, self -> env, error) &&
	vm_car(self, error) &&
	vm_make_cons(self, error) &&
	vm_push(self, self -> env, error) &&
	vm_swap(self, error) &&
	vm_set_car(self, error) &&
	vm_pop(self, NULL, error);
}

/* Locates a cons cell in an alist */
static int do_assoc(vm_t self, object_t alist, object_t symbol, elvin_error_t error)
{
    object_t cons = alist;

    while (object_type(cons) == SEXP_CONS)
    {
	object_t car = (object_t)cons[1];

	/* Make sure we have a cons cell */
	if (object_type(car) == SEXP_CONS)
	{
	    /* Do we have a match? */
	    if (symbol == (object_t)car[1])
	    {
		return vm_push(self, car, error);
	    }
	}

	cons = (object_t)cons[2];
    }

    /* Not found */
    return 0;
}

/* Looks up the value of the symbol on the top of the stack in the
 * current environment */
static int lookup(vm_t self, elvin_error_t error)
{
    object_t symbol;
    object_t env = self -> env;

    /* Get the symbol from the stack */
    if (! vm_pop(self, &symbol, error))
    {
	return 0;
    }

    /* Look for it in this environment */
    while (env)
    {
	if (do_assoc(self, (object_t)env[1], symbol, error))
	{
	    return vm_cdr(self, error);
	}

	env = (object_t)env[2];
    }

    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "undefined symbol()");
    return 0;
}

/* Evaluates the top of the stack, leaving the result in its place */
int vm_eval(vm_t self, elvin_error_t error)
{
    object_t object;

    /* Find the top item on the stack */
    if (! vm_top(self, &object, error))
    {
	return 1;
    }

    switch (object_type(object))
    {
	/* Most objects evaluate to themselves */
	case SEXP_NIL:
	case SEXP_CHAR:
	case SEXP_INTEGER:
	case SEXP_LONG:
	case SEXP_FLOAT:
	case SEXP_STRING:
	case SEXP_PRIM:
	case SEXP_LAMBDA:
	{
	    return 1;
	}

	/* Symbols get looked up in the environment */
	case SEXP_SYMBOL:
	{
	    return lookup(self, error);
	}

	/* Cons cells evaluate to function calls */
	case SEXP_CONS:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "eval[cons]");
	    return 0;
	}

	/* Nothing else should be getting put onto the stack */
	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "eval[unknown]");
	    return 0;
	}
    }
}



/* Prints a single object */
static void do_print(object_t object)
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
	    printf("%f", *(double *)(object + 1));
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
	    printf("%s", (char *)(object + 1));
	    break;
	}

	case SEXP_CONS:
	{
	    printf("(");
	    do_print((object_t)object[1]);

	    object = (object_t)object[2];
	    while (object_type(object) == SEXP_CONS)
	    {
		printf(" ");
		do_print((object_t)object[1]);
		object = (object_t)object[2];
	    }

	    /* Watch for a dotted list */
	    if (object != NULL)
	    {
		printf(" . ");
		do_print(object);
	    }
	    printf(")");
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

/* Prints the top of the stack onto stdout */
int vm_print(vm_t self, elvin_error_t error)
{
    object_t result;

    /* Pop the top off the stack */
    if (! vm_pop(self, &result, error))
    {
	return 0;
    }

    /* Print it */
    do_print(result);
    printf("\n");
    return 1;
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

