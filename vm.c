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
static const char cvsid[] = "$Id: vm.c,v 2.1 2000/11/17 09:47:39 phelps Exp $";
#endif

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/error.h>
#include <elvin/errors/elvin.h>
#include "vm.h"

/* The heap is composed of blocks this big */
#define HEAP_BLOCK_SIZE (4 * 1024)
#define OUT_OF_MEMORY "Out of memory"

/* Objects are just pointers */
typedef void **object_t;

/* Handles allow us to refer to GC objects from outside GC space */
struct vm_object
{
    object_t object;
};

/* The heap is composed of a linked list of chunks of memory */
typedef struct vm_heap *vm_heap_t;
struct vm_heap
{
    /* The next block of the heap */
    vm_heap_t next;

    /* The heap's bytes */
    char bytes[1];
};


/* Allocates and initializes a new heap block */
vm_heap_t vm_heap_alloc(elvin_error_t error)
{
    vm_heap_t heap;

    /* Heaps are fixed sized block */
    if (! (heap = ELVIN_MALLOC(HEAP_BLOCK_SIZE, error)))
    {
	return NULL;
    }

    /* Initialize the one field we guarantee to have set */
    heap -> next = NULL;
    return heap;
}


/* The structure of the virtual machine */
struct vm
{
    /* All external pointers into the VM */
    vm_heap_t root_set;

    /* The virtual machine's heap */
    vm_heap_t heap;

    /* The next free spot in the heap */
    object_t heap_next;

    /* The bottom of the stack */
    vm_object_t stack;

    /* The current stack frame */
    vm_object_t sp;

    /* The top of the stack */
    vm_object_t top;

    /* The current evaluation environment */
    vm_object_t env;

    /* The expression currently being evaluated */
    vm_object_t expr;

    /* The program counter (index into the expression) */
    uint32_t pc;
};


/* Allocates and initializes a new virtual machine */
vm_t vm_alloc(elvin_error_t error)
{
    vm_t vm;

    /* Allocate some room for the VM itself */
    if (! (vm = ELVIN_MALLOC(sizeof(struct vm), error)))
    {
	return NULL;
    }

    /* Initialize its fields to safe values */
    memset(vm, 0, sizeof(struct vm));

    /* Allocate an initial heap */
    if (! (vm -> heap = vm_heap_alloc(error)))
    {
	ELVIN_FREE(vm, NULL);
	return NULL;
    }

    return vm;
}

/* Frees a virtual machine */
int vm_free(vm_t vm, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "vm_free");
    return 0;
}


/* Allocates a block of memory on the heap.  Note that size should be
 * the number of object references, not the size in bytes */
int vm_new(
    vm_t vm,
    object_type_t type,
    uint16_t length,
    uint8_t flags,
    vm_object_t result,
    elvin_error_t error)
{
    uint32_t header;

    /* Make sure there's enough room in the heap */
    if (! (vm -> heap_next + length + 1 < (object_t)(vm -> heap + HEAP_BLOCK_SIZE)))
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "out of memory");
	return 0;
    }

    /* Record the pointer in the result field */
    result -> object = vm -> heap_next;

    /* Update the heap to point to the next object */
    vm -> heap_next += length + 1;

    /* Construct the header */
    header = (type & 0xf) << 24 | (flags & 0xf) << 16 | (length & 0xFFFF);
    result -> object[0] = (void *)header;

    /* Fill in the rest of the fields with nil */
    memset(result -> object + 1, 0, length * 4);
    return 1;
}



/* Returns an object's type */
object_type_t vm_object_type(vm_object_t object)
{
    fprintf(stderr, "not yet implemented: vm_object_type()\n");
    abort();
}

/* Gets a reference to `nil' */
int nil_alloc(vm_object_t result, elvin_error_t error)
{
    result -> object = NULL;
    return 1;
}

/* Gets a new 31-bit integer object */
int integer_alloc(int32_t value, vm_object_t result, elvin_error_t error)
{
    /* FIX THIS: watch for overflow */
    result -> object = (void *)((value << 1) | 1);
    return 1;
}

/* Returns an integer's value */
int32_t integer_value(vm_object_t object, elvin_error_t error)
{
    return ((int32_t)object) >> 1;
}

/* Allocates and initializes a new long integer object */
int long_alloc(int64_t value, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "long_alloc()");
    return 0;
}

/* Returns the value of a long integer object */
int64_t long_value(vm_object_t object)
{
    return 0L;
}

/* Allocates and initializes a new float object */
int float_alloc(double value, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "float_alloc()");
    return 0;
}


/* Returns the value of a float */
double float_value(vm_object_t object)
{
    return 0.0;
}

/* Allocates and initializes a new string object */
int string_alloc(char *value, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "string_alloc()");
    return 0;
}

/* Returns the string as a char * */
char *string_value(vm_object_t object)
{
    return "";
}

/* Allocates and initializes a new character object */
int char_alloc(char value, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "char_alloc()");
    return 0;
}

/* Returns the char's value */
char char_value(vm_object_t object)
{
    return 0;
}

/* Allocates and initializes a new symbol object */
int symbol_alloc(char *name, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "symbol_alloc()");
    return 0;
}

/* Returns the symbol's name */
char *symbol_name(vm_object_t object)
{
    return "";
}

/* Allocates and initializes a new cons cell */
int cons_alloc(vm_object_t car, vm_object_t cdr, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "cons_alloc()");
    return 0;
}

/* Returns the car of a cons cell */
void cons_car(vm_object_t result, vm_object_t object)
{
    abort();
}

/* Returns the cdr of a cons cell */
void cons_cdr(vm_object_t result, vm_object_t object)
{
    abort();
}


/* Allocates and initializes a new primitive subroutine object */
int builtin_alloc(char *name, void *function, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "builtin_alloc()");
    return 0;
}

/* Allocates and initializes a new lambda object */
int lambda_alloc(
    vm_object_t env,
    vm_object_t arg_list,
    vm_object_t body,
    vm_object_t result,
    elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "lambda_alloc()");
    return 0;
}

/* Allocates and initializes a new array object */
int array_alloc(uint32_t size, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "array_alloc()");
    return 0;
}

/* Sets a value in an array */
int array_set(vm_object_t array, uint32_t index, vm_object_t value, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "array_set()");
    return 0;
}

/* Gets a value from an array */
int array_get(vm_object_t array, uint32_t index, vm_object_t result, elvin_error_t error)
{
    ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "array_get()");
    return 0;
}

