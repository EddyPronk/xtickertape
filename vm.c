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
static const char cvsid[] = "$Id: vm.c,v 2.15 2000/11/28 00:34:15 phelps Exp $";
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
#define TYPE_MASK 0x0000FF00
#define FLAGS_MASK 0x000000FF

#define IS_POINTERS_FLAG 0x00008000
#define MIGRATED_FLAG 0x00000080

#define OBJECT_SIZE(flags) (((flags) >> 16) & 0xFFFF)

static void do_print(object_t object);


/* The structure of the virtual machine */
struct vm
{
    /* All external pointers into the VM */
    object_t root_set;

    /* The symbol table */
    object_t *symbol_table;

    /* The virtual machine's heap */
    object_t *heap;

    /* The next free spot in the heap */
    object_t *heap_next;

    /* The bottom of the stack */
    object_t stack;

    /* --- CURRENT STATE --- */

    /* The current stack frame */
    uint32_t fp;

    /* The current stack position */
    uint32_t sp;

    /* The current environment */
    object_t env;

    /* The expression we're evaluating */
    object_t expr;

    /* The program counter */
    uint32_t pc;
};


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
    return (header >> 8) & 0xFF;
}

/* Returns the object n places back on the stack */
int vm_peek(vm_t self, uint32_t count, object_t *result, elvin_error_t error)
{
    if (self -> sp < count + 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    *result = self -> stack[self -> sp - count - 1];
    return 1;
}

/* Returns the object on the top of the stack */
int vm_top(vm_t self, object_t *result, elvin_error_t error)
{
    return vm_peek(self, 0, result, error);
}

/* Pops and returns the top of the stack */
int vm_pop(vm_t self, object_t *result, elvin_error_t error)
{
    /* Make sure the stack doesn't underflow */
    if (self -> sp < 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    /* Pop the result if there's a place to put it */
    self -> sp--;
    if (result)
    {
	*result = self -> stack[self -> sp];
    }

    return 1;
}

/* Pushes an object onto the vm's stack */
int vm_push(vm_t self, object_t object, elvin_error_t error)
{
    if (! (self -> sp < STACK_SIZE))
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack overflow");
	return 0;
    }

    self -> stack[self -> sp++] = object;
    return 1;
}

/* Swaps the top two elements on the stack */
int vm_swap(vm_t self, elvin_error_t error)
{
    object_t swap;

    if (self -> sp < 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow!");
	return 0;
    }

    swap = self -> stack[self -> sp - 1];
    self -> stack[self -> sp - 1] = self -> stack[self -> sp - 2];
    self -> stack[self -> sp - 2] = swap;
    return 1;
}

/* Rotates the stack so that the top is `count' places back */
int vm_roll(vm_t self, uint32_t count, elvin_error_t error)
{
    object_t top;

    if (self -> sp < count + 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow");
	return 0;
    }

    /* Grab the top of the stack */
    top = self -> stack[self -> sp - 1];

    /* Push the remaining elements up */
    memmove(
	self -> stack + self -> sp - count, 
	self -> stack + self -> sp - count - 1,
	count * POINTER_SIZE);

    /* Push the top into the appropriate place */
    self -> stack[self -> sp - count - 1] = top;
    return 1;
}

/* Rotates the stack so that the item `count' places back is on top */
int vm_unroll(vm_t self, uint32_t count, elvin_error_t error)
{
    object_t top;

    if (self -> sp < count + 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "stack underflow");
	return 0;
    }

    /* Grab the item from the stack */
    top = self -> stack[self -> sp - count - 1];

    /* Push the remaining elements down */
    memmove(
	self -> stack + self -> sp - count - 1,
	self -> stack + self -> sp - count,
	count * POINTER_SIZE);

    /* Put the item onto the top */
    self -> stack[self -> sp - 1] = top;
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
    object = (object_t)(self -> heap_next);
    self -> heap_next += size + 1;

    /* Write the object header (it pretends to be an integer) */
    header = (size & 0xFFFF) << 16 | (type & 0xFF) << 8 | (flags & 0xFF);
    object[0] = (void *)header;

    /* Clear the rest of it */
    memset(object + 1, 0, size * POINTER_SIZE);

    /* Push the object onto the stack */
    return vm_push(self, object, error);
}

/* Returns the type of the object on the top of the stack */
int vm_type(vm_t self, object_type_t *result, elvin_error_t error)
{
    object_t object;

    /* Grab the top of the stack */
    if (! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Return its type */
    *result = object_type(object);
    return 1;
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

/* Pushes a symbol onto the vm's stack */
int vm_push_symbol(vm_t self, char *name, elvin_error_t error)
{
    object_t *pointer;
    uint32_t length;
    uint32_t size;

    /* Go through the symbol table to see if we already have one */
    for (pointer = self -> symbol_table; *pointer; pointer++)
    {
	/* If we get a match then we're done */
	if (strcmp((char *)(*pointer + 1), name) == 0)
	{
	    return vm_push(self, *pointer, error);
	}
    }

    /* No symbol yet.  Make a new one. */
    length = strlen(name);
    size = 1 + length / POINTER_SIZE;
    if (! vm_new(self, size, SEXP_SYMBOL, size * POINTER_SIZE - length, error))
    {
	return 0;
    }

    /* Find it on the top of the stack */
    if (! vm_top(self, pointer, error))
    {
	return 0;
    }

    /* Copy the string bytes into place */
    memcpy(*pointer + 1, name, length + 1);

    /* Null-terminate the symbol table */
    *(pointer + 1) = NULL;
    return 1;
}

/* Pushes a char onto the vm's stack */
int vm_push_char(vm_t self, int value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_push_char");
    return 0;
}

/* Pushes a special form onto the vm's stack */
int vm_push_special_form(vm_t self, prim_t func, elvin_error_t error)
{
    object_t object;

    /* Create a new special form object on the heap */
    if (! vm_new(self, 1, SEXP_SPECIAL, 0, error) ||
	! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Copy the function pointer into the object */
    object[1] = (object_t)func;
    return 1;
}

/* Pushes a primitive subroutine onto the vm's stack */
int vm_push_subr(vm_t self, prim_t func, elvin_error_t error)
{
    object_t object;

    /* Create a new special form object on the heap */
    if (! vm_new(self, 1, SEXP_SUBR, 0, error) ||
	! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Copy the function pointer into the object */
    object[1] = (object_t)func;
    return 1;
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
	    self -> sp--;

	    /* Push the symbol on */
	    return vm_push(self, *pointer, error);
	}

	/* Keep looking */
	pointer++;
    }

    /* No such symbol.  Transform the string into a symbol. */
    header = (uint32_t)*string;
    header = (header & ~TYPE_MASK) | (SEXP_SYMBOL << 8);
    *string = (void *)header;

    /* Write it on the end of the symbol table */
    *pointer = string;
    *(pointer + 1) = NULL;
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
    object_t object;

    /* Pop the top off the stack */
    if (! vm_pop(self, &object, error))
    {
	return 0;
    }

    /* We can work with a cons cell or nil */
    switch (object_type(object))
    {
	/* (car nil) -> nil */
	case SEXP_NIL:
	{
	    return vm_push(self, object, error);
	}

	case SEXP_CONS:
	{
	    return vm_push(self, (object_t)object[1], error);
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a cons");
	    return 0;
	}
    }
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

/* Pops a cons cell off the top of the stack and pushes its car on instead */
int vm_cdr(vm_t self, elvin_error_t error)
{
    object_t object;

    /* Pop the top of the stack */
    if (! vm_pop(self, &object, error))
    {
	return 0;
    }

    /* We can work with nil or a cons cell */
    switch (object_type(object))
    {
	/* (cdr nil) -> nil */
	case SEXP_NIL:
	{
	    return vm_push(self, object, error);
	}

	case SEXP_CONS:
	{
	    return vm_push(self, (object_t)object[2], error);
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a cons");
	    return 0;
	}
    }
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

/* Makes a lambda out of the current environment and stack */
int vm_make_lambda(vm_t self, elvin_error_t error)
{
    object_t object;
    uint32_t count = 0;

    /* Move the arg list to the top of the stack and look at it */
    if (! vm_swap(self, error) ||
	! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Unroll the arg list onto the stack */
    while (object_type(object) == SEXP_CONS)
    {
	count++;

	/* Extract the car of the arg list */
	if (! vm_dup(self, error) ||
	    ! vm_car(self, error) ||
	    ! vm_top(self, &object, error))
	{
	    return 0;
	}

	/* Complain if we're not given symbols */
	if (object_type(object) != SEXP_SYMBOL)
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad arg name");
	    return 0;
	}

	/* Extract the cdr */
	if (! vm_swap(self, error) ||
	    ! vm_cdr(self, error) ||
	    ! vm_top(self, &object, error))
	{
	    return 0;
	}
    }

    /* Make sure we've ended well */
    if (object_type(object) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad arg list");
	return 0;
    }

    /* Pop off the nil */
    if (! vm_pop(self, NULL, error))
    {
	return 0;
    }

    /* Create an object big enough for the args plus an environment and body */
    if (! vm_new(self, 2 + count, SEXP_LAMBDA, 0, error))
    {
	return 0;
    }

    /* Grab the object off the top of the stack */
    if (! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Record the environment */
    object[1] = self -> env;

    /* Record the body and arg list */
    memcpy(
	object + 2,
	self -> stack + self -> sp - count - 2,
	POINTER_SIZE * (count + 1));

    /* Fix the stack so that only the lambda is on top */
    self -> stack[self -> sp - count - 2] = object;
    self -> sp -= count + 1;

    return 1;
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

/* Assigns the value on the top of the stack to the variable up one
 * place, leaving only the value on the stack */
int vm_assign(vm_t self, elvin_error_t error)
{
    object_t symbol;
    object_t env = self -> env;

    /* Get the symbol */
    if (! vm_peek(self, 1, &symbol, error))
    {
	return 0;
    }

    /* If we already have an entry for it then just change its value */
    while (1)
    {
	/* Look in the current environment for its value */
	if (do_assoc(self, (object_t)env[1], symbol, error))
	{
	    return
		vm_swap(self, error) &&
		vm_set_cdr(self, error) &&
		vm_cdr(self, error);
	}

	/* If this is the root environment then just set the variable */
	if (env[2] == NULL)
	{
	    return
		vm_dup(self, error) &&
		vm_roll(self, 2, error) &&
		vm_make_cons(self, error) &&
		vm_push(self, self -> env, error) &&
		vm_car(self, error) &&
		vm_make_cons(self, error) &&
		vm_push(self, self -> env, error) &&
		vm_swap(self, error) &&
		vm_set_car(self, error) &&
		vm_pop(self, NULL, error);
	}

	env = (object_t)env[2];
    }
}

/* Puts `t' on the top of the stack if the top two items are pointers
 * to the same object, `nil' otherwise */
int vm_eq(vm_t self, elvin_error_t error)
{
    object_t arg1, arg2;

    /* Pop the args */
    if (! vm_pop(self, &arg2, error) ||
	! vm_pop(self, &arg1, error))
    {
	return 0;
    }

    /* If they're not the same then push nil */
    if (arg1 == arg2)
    {
	/* FIX THIS: this is a slow way to say true */
	return vm_push_symbol(self, "t", error);
    }
    else
    {
	return vm_push_nil(self, error);
    }
}


/* Adds the top two elements of the stack together */
int vm_add(vm_t self, elvin_error_t error)
{
    object_t arg1, arg2;

    /* Pop the args */
    if (! vm_pop(self, &arg2, error) ||
	! vm_pop(self, &arg1, error))
    {
	return 0;
    }

    /* Watch for promotion rules */
    switch (object_type(arg1))
    {
	case SEXP_INTEGER:
	{
	    int32_t value = ((int32_t)arg1) >> 1;

	    switch (object_type(arg2))
	    {
		case SEXP_INTEGER:
		{
		    /* FIX THIS: watch for promotion */
		    return vm_push_integer(self, value + ((int32_t)arg2 >> 1), error);
		}

		case SEXP_LONG:
		{
		    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "+long");
		    return 0;
		}

		case SEXP_FLOAT:
		{
		    return vm_push_float(self, value + *(double *)(arg2 + 1), error);
		}

		default:
		{
		    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a number");
		    return 0;
		}
	    }
	}

	case SEXP_LONG:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "+long");
	    return 0;
	}

	case SEXP_FLOAT:
	{
	    double value = *(double *)(arg1 + 1);
	    switch (object_type(arg2))
	    {
		case SEXP_INTEGER:
		{
		    return vm_push_float(self, value + ((int32_t)arg2 >> 1), error);
		}

		case SEXP_LONG:
		{
		    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "+long");
		    return 0;
		}

		case SEXP_FLOAT:
		{
		    return vm_push_float(self, value + *(double *)(arg2 + 1), error);
		}

		default:
		{
		    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a number");
		    return 0;
		}
	    }
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "not a number");
	    return 0;
	}
    }
}

/* Gets the top of the stack evaluated */
static int eval_setup(vm_t self, elvin_error_t error)
{
    object_t object;
    uint32_t sp;

    /* Find the top item on the stack */
    if (! vm_top(self, &object, error))
    {
	return 0;
    }

    /* Evaluate the object based upon its type */
    switch (object_type(object))
    {
	/* Most values evaluate to themselves */
	case SEXP_NIL:
	case SEXP_CHAR:
	case SEXP_INTEGER:
	case SEXP_LONG:
	case SEXP_FLOAT:
	case SEXP_STRING:
	case SEXP_SUBR:
	case SEXP_SPECIAL:
	case SEXP_LAMBDA:
	{
	    return 1;
	}

	/* Look up symbols in the current environment */
	case SEXP_SYMBOL:
	{
	    return lookup(self, error);
	}

	/* Cons cells create a new stack frame */
	case SEXP_CONS:
	{
	    /* Record the current stack pointer */
	    sp = self -> sp;

	    /* Push the current state */
	    if (! vm_push_integer(self, self -> fp, error) ||
		! vm_push(self, self -> env, error) ||
		! vm_push(self, self -> expr, error) ||
		! vm_push_integer(self, self -> pc, error))
	    {
		self -> sp = sp;
		return 0;
	    }

	    /* Set up our state to evaluate the new thing */
	    self -> fp = sp;
	    self -> expr = object;
	    self -> pc = 0;
	    return 1;
	}

	/* Anything else doesn't evaluate */
	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "eval[unknown]");
	    return 0;
	}
    }
}

/* Returns the top of the stack from a function call */
static int do_return(vm_t self, elvin_error_t error)
{
    object_t object;

    /* Get the object off of the stack */
    if (! vm_pop(self, &object, error))
    {
	return 0;
    }

    /* Restore the old state */
    self -> sp = self -> fp;
    self -> fp = ((uint32_t)(self -> stack[self -> sp])) >> 1;
    self -> env = self -> stack[self -> sp + 1];
    self -> expr = self -> stack[self -> sp + 2];
    self -> pc = ((uint32_t)(self -> stack[self -> sp + 3])) >> 1;

    /* Push the result onto the stack */
    if (! vm_pop(self, NULL, error) || ! vm_push(self, object, error))
    {
	return 0;
    }

    return 1;
}

/* Call a function object */
static int funcall(vm_t self, elvin_error_t error)
{
    object_t object;

    /* Peek at the stack to get the function */
    if (! vm_peek(self, self -> pc - 1, &object, error))
    {
	return 0;
    }

    /* Decide what to do based on its type */
    switch (object_type(object))
    {
	case SEXP_SPECIAL:
	case SEXP_SUBR:
	{
	    /* Call the primitive function */
	    if (! ((prim_t)object[1])(self, self -> pc - 1, error))
	    {
		return 0;
	    }

	    /* Return the top of the stack as the result */
	    return do_return(self, error);
	}

	case SEXP_LAMBDA:
	{
	    uint32_t count, i;

	    /* Check number of args */
	    count = OBJECT_SIZE((uint32_t)object[0]) - 2;
	    if (count != self -> pc - 1)
	    {
		ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong argc");
		return 0;
	    }

	    /* Start with an empty alist */
	    if (! vm_push_nil(self, error))
	    {
		return 0;
	    }

	    /* Add a cons pair for each item */
	    for (i = count; 0 < i; i--)
	    {
		/* Make a cons cell out of the arg name and value and add it to the alist */
		if (! vm_push(self, object[i + 2], error) ||
		    ! vm_unroll(self, 2, error) ||
		    ! vm_make_cons(self, error) ||
		    ! vm_swap(self, error) ||
		    ! vm_make_cons(self, error))
		{
		    return 0;
		}
	    }

	    /* Add the parent environment and make it the current environment */
	    if (! vm_push(self, object[1], error) ||
		! vm_make_cons(self, error) ||
		! vm_pop(self, &self -> env, error))
	    {
		return 0;
	    }
	
	    /* Make the body be the current expression and reset the pc */
	    self -> expr = object[2];
	    self -> pc = 0;
	    return 1;
	}

	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad function");
	    return 0;
	}
    }
}

/* Evaluate an arg of the expression */
static int eval_next_arg(vm_t self, elvin_error_t error)
{
    /* Double-check that we have a cons cell */
    switch (object_type(self -> expr))
    {
	/* Watch for the end of the expression */
	case SEXP_NIL:
	{
	    return funcall(self, error);
	}

	/* Evaluate the next arg */
	case SEXP_CONS:
	{
	    self -> pc++;

	    /* Push the car of the expression onto the stack */
	    if (! vm_push(self, self -> expr[1], error))
	    {
		return 0;
	    }

	    /* Just keep the cdr of the expression */
	    self -> expr = self -> expr[2];

	    /* Prepare to evaluate the arg */
	    if (! eval_setup(self, error))
	    {
		return 0;
	    }

	    return 1;
	}

	/* Bogus arg list */
	default:
	{
	    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad arg list");
	    return 0;
	}
    }
}

/* Perform the next calculation based on the VM state */
static int run(vm_t self, uint32_t sp_exit, elvin_error_t error)
{
    /* Keep evaluating until... when? */
    while (sp_exit < self -> sp)
    {
	switch (self -> pc)
	{
	    /* Always evaluate the first arg */
	    case 0:
	    {
		if (! eval_next_arg(self, error))
		{
		    self -> sp = sp_exit - 1;
		    return 0;
		}

		break;
	    }

	    /* Do we evaluate the rest of the args? */
	    case 1:
	    {
		object_t object;

		/* The function should be on the top of the stack */
		if (! vm_top(self, &object, error))
		{
		    self -> sp = sp_exit - 1;
		    return 0;
		}

		/* Do we need to evaluate the args? */
		switch (object_type(object))
		{
		    /* Don't evaluate the args, just push them onto the stack */
		    case SEXP_SPECIAL:
		    {
			while (object_type(self -> expr) == SEXP_CONS)
			{
			    if (! vm_push(self, self -> expr[1], error))
			    {
				self -> sp = sp_exit - 1;
				return 0;
			    }

			    self -> expr = self -> expr[2];
			    self -> pc++;
			}

			/* Make sure we have a null-terminated list */
			if (object_type(self -> expr) != SEXP_NIL)
			{
			    self -> sp = sp_exit - 1;
			    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad args");
			    return 0;
			}

			/* Roll the function to the top of the stack */
			if (! funcall(self, error))
			{
			    self -> sp = sp_exit - 1;
			    return 0;
			}

			break;
		    }

		    /* Evaluate the args and push the results onto the stack */
		    case SEXP_SUBR:
		    case SEXP_LAMBDA:
		    {
			if (! eval_next_arg(self, error))
			{
			    self -> sp = sp_exit - 1;
			    return 0;
			}

			break;
		    }

		    /* Bogus function.  Bail! */
		    default:
		    {
			self -> sp = sp_exit - 1;
			ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bogus func");
			return 0;
		    }
		}

		break;
	    }

	    /* We must be evaluating args */
	    default:
	    {
		if (! eval_next_arg(self, error))
		{
		    self -> sp = sp_exit - 1;
		    return 0;
		}
	    }
	}
    }

    return 1;
}


/* Evaluates the top of the stack, leaving the result in its place */
int vm_eval(vm_t self, elvin_error_t error)
{
    uint32_t sp = self -> sp;

    /* Set up to evaluate the top of the stack */
    if (! eval_setup(self, error))
    {
	return 0;
    }

    /* Run until we're done */
    return run(self, sp, error);
}

/* Catch non-local returns */
int vm_catch(vm_t self, elvin_error_t error)
{
    uint32_t fp = self -> fp;

    /* Rotate the body to the top of the stack */
    if (! vm_unroll(self, 1, error))
    {
	return 0;
    }

    /* If eval goes well then just drop out */
    if (vm_eval(self, error))
    {
	return 1;
    }

    /* DO MAGIC HERE */
    printf("CATCH NOT YET IMPLEMENTED\n");
    exit(1);
}


/* Migrate an object into a new heap */
static void migrate(vm_t self, object_t *heap, object_t **end, object_t *object)
{
    uint32_t flags;
    uint32_t size;

    /* If the object is nil or an integer then do nothing */
    if (*object == NULL || ((uint32_t)*object & 1) == 1)
    {
	return;
    }

    /* If the object has already been migrated then update the pointer */
    flags = (uint32_t)(**object);
    if (MIGRATED_FLAG & flags)
    {
	/* Update the pointer from the forwarding address */
	*object = *(*object + 1);
	return;
    }

    /* Copy the object to the new heap */
    size = OBJECT_SIZE(flags) + 1;
    memcpy(*end, *object, size * POINTER_SIZE);

    /* Leave a forwarding address */
    flags |= MIGRATED_FLAG;
    **object = (object_t)flags;
    *(*object + 1) = (object_t)*end;
    
    /* Update the object pointer */
    *object = (object_t)*end;
    (*end) += size;
}

/* Perform garbage collection */
int vm_gc(vm_t self, elvin_error_t error)
{
    object_t *heap;
    object_t *point;
    object_t *end;
    uint32_t i;

    /* Create a new heap to copy stuff to */
    if (! (heap = (object_t *)ELVIN_MALLOC(HEAP_BLOCK_SIZE * POINTER_SIZE, error)))
    {
	return 0;
    }
    end = heap;

    /* Use the current VM state to migrate the root set */
    migrate(self, heap, &end, &self -> env);
    migrate(self, heap, &end, &self -> expr);

    /* Migrate the stack */
    for (i = 0; i < self -> sp; i++)
    {
 	migrate(self, heap, &end, (object_t *)&(self -> stack[i]));
    }

    /* We've got our root set.  Now migrate the transitive closure of that. */
    point = heap;
    while (point < end)
    {
	uint32_t flags = (uint32_t)(*(point++));
	object_t *next = point + OBJECT_SIZE(flags);

	/* If the object contains pointers then migrate them */
	if (flags & IS_POINTERS_FLAG)
	{
	    while (point < next)
	    {
		migrate(self, heap, &end, point);
		point++;
	    }
	}

	point = next;
    }

    /* Remember where the end of the heap is */
    self -> heap_next = end;

    /* Clean up the symbol table */
    end = self -> symbol_table;
    for (point = end; *point; point++)
    {
	uint32_t flags;

	/* Keep the symbol if it's been migrated */
	flags = (uint32_t)(**point);
	if (flags & MIGRATED_FLAG)
	{
	    *(end++) = *(*point + 1);
	}
    }

    *end = NULL;

    /* Swap the new heap into place */
    ELVIN_FREE(self -> heap, NULL);
    self -> heap = heap;

    /* Push the amount of used space onto the stack */
    return vm_push_integer(self, self -> heap_next - self -> heap, error);
}



/* Prints a single object */
static void do_print(object_t object)
{
    uint32_t header;

    /* Is the object nil? */
    if (object == NULL)
    {
	printf("nil");
	return;
    }

    /* Is it an integer masquerading as a pointer? */
    if (((int32_t)object & 1) == 1)
    {
	printf("%d", ((int32_t)object) >> 1);
	return;
    }

    /* Grab the header */
    header = (uint32_t)(*object);
    if (header & MIGRATED_FLAG)
    {
	printf("<migrated object>");
	return;
    }

    switch ((header >> 8) &0xFF)
    {
	case SEXP_NIL:
	case SEXP_INTEGER:
	{
	    printf("uh oh\n");
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

	case SEXP_SUBR:
	{
	    printf("<subr>");
	    break;
	}

	case SEXP_SPECIAL:
	{
	    printf("<special form>");
	    break;
	}

	case SEXP_LAMBDA:
	{
	    printf("<closure>");
	    break;
	}

	case SEXP_ENV:
	{
	    printf("<env>");
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
int vm_print_state(vm_t self, elvin_error_t error)
{
    uint32_t i;

    /* Print the state */
    printf("fp=%d, sp=%d, pc=%d heap-size: %d\n",
	   self -> fp, self -> sp, self -> pc,
	   self -> heap_next - self -> heap);
    printf("env="); do_print(self -> env); printf("\n");
    printf("expr="); do_print(self -> expr); printf("\n");
    printf("stack:\n");

    /* print each stack item */
    for (i = 0; i < self -> sp; i++)
    {
	printf("  %d: ", i);
	do_print(self -> stack[i]);
	printf("\n");
    }

    fflush(stdout);
    return 1;
}

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
    if (! (self -> heap = (object_t *)ELVIN_MALLOC(
	       HEAP_BLOCK_SIZE * POINTER_SIZE, error)))
    {
	vm_free(self, NULL);
	return NULL;
    }

    /* Point to the first free part of the heap */
    self -> heap_next = self -> heap + 1;

    /* Allocate a symbol table */
    if (! (self -> symbol_table = (object_t *)ELVIN_MALLOC(
	       SYMTAB_SIZE * POINTER_SIZE, error)))
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
    int result = 1;

    /* Free the root set */
    /* Free the symbol table */
    result = ELVIN_FREE(self -> symbol_table, result ? error : NULL) && result;

    /* Free the heap */
    result = ELVIN_FREE(self -> heap, result ? error : NULL) && result;

    /* Free the stack */
    result = ELVIN_FREE(self -> stack, result ? error : NULL) && result;

    /* Free the VM itself */
    return ELVIN_FREE(self, result ? error : NULL) && result;
}

