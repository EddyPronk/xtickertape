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

#ifndef VM_H
#define VM_H

#ifndef lint
static const char cvs_VM_H[] = "$Id: vm.h,v 2.13 2000/11/28 00:33:59 phelps Exp $";
#endif /* lint */

/* Objects are really handles to the world outside the VM */
typedef void **object_t;

/* The virtual machine type */
typedef struct vm *vm_t;

/* The type of a primitive function */
typedef int (*prim_t)(vm_t vm, uint32_t argc, elvin_error_t error);


/* The types of objects */
typedef enum
{
    /* Atomic types are 0-127 */
    SEXP_NIL = 0,
    SEXP_CHAR,
    SEXP_INTEGER,
    SEXP_LONG,
    SEXP_FLOAT,
    SEXP_STRING,
    SEXP_SYMBOL,
    SEXP_SUBR,
    SEXP_SPECIAL,

    /* Combined types are 128-255 */
    SEXP_CONS = 128,
    SEXP_LAMBDA,
    SEXP_ENV
} object_type_t;

/* Allocates and initializes a new virtual machine */
vm_t vm_alloc(elvin_error_t error);

/* Frees a virtual machine */
int vm_free(vm_t self, elvin_error_t error);


/* Swaps the top two elements on the stack */
int vm_swap(vm_t self, elvin_error_t error);

/* Pops the top of the vm's stack */
int vm_pop(vm_t self, object_t *result, elvin_error_t error);

/* Rotates the stack so that the top is `count' places back */
int vm_roll(vm_t self, uint32_t count, elvin_error_t error);

/* Rotates the stack so that the item `count' places back is on top */
int vm_unroll(vm_t self, uint32_t count, elvin_error_t error);

/* Duplicates the top of the stack and puts it onto the stack */
int vm_dup(vm_t self, elvin_error_t error);

/* Returns the type of the top item on the stack (without popping it) */
int vm_type(vm_t self, object_type_t *result, elvin_error_t error);

/* Push nil onto the vm's stack */
int vm_push_nil(vm_t self, elvin_error_t error);

/* Pushes an integer onto the vm's stack */
int vm_push_integer(vm_t self, int32_t value, elvin_error_t error);

/* Pushes a long integer onto the vm's stack */
int vm_push_long(vm_t self, int64_t value, elvin_error_t error);

/* Pushes a float onto the vm's stack */
int vm_push_float(vm_t self, double value, elvin_error_t error);

/* Pushes a string onto the vm's stack */
int vm_push_string(vm_t self, char *value, elvin_error_t error);

/* Pushes a symbol onto the vm's stack */
int vm_push_symbol(vm_t self, char *name, elvin_error_t error);

/* Pushes a char onto the vm's stack */
int vm_push_char(vm_t self, int value, elvin_error_t error);

/* Pushes a special form onto the vm's stack */
int vm_push_special_form(vm_t self, prim_t func, elvin_error_t error);

/* Pushes a primitive subroutine onto the vm's stack */
int vm_push_subr(vm_t self, prim_t func, elvin_error_t error);

/* Makes a symbol out of the string on the top of the stack */
int vm_make_symbol(vm_t self, elvin_error_t error);

/* Creates a cons cell out of the top two elements on the stack */
int vm_make_cons(vm_t self, elvin_error_t error);

/* Replaces the top of the stack with its car */
int vm_car(vm_t self, elvin_error_t error);

/* Replaces the top of the stack with its cdr */
int vm_cdr(vm_t self, elvin_error_t error);

/* Reverses the pointers in a list that was constructed upside-down */
int vm_unwind_list(vm_t self, elvin_error_t error);

/* Makes a lambda out of the current environment and stack */
int vm_make_lambda(vm_t self, elvin_error_t error);


/* Assigns the value on the top of the stack to the variable up one
 * place, leaving only the value on the stack */
int vm_assign(vm_t self, elvin_error_t error);

/* Compares the top two items on the stack for pointer equality */
int vm_eq(vm_t self, elvin_error_t error);

/* Adds the top two elements of the stack together */
int vm_add(vm_t self, elvin_error_t error);

/* Evaluates the top of the stack, leaving the result in its place */
int vm_eval(vm_t self, elvin_error_t error);

/* Catch non-local returns */
int vm_catch(vm_t self, elvin_error_t error);

/* Perform garbage collection */
int vm_gc(vm_t self, elvin_error_t error);


/* Prints the top of the stack onto stdout */
int vm_print(vm_t self, elvin_error_t error);

/* For debugging only */
int vm_print_state(vm_t self, elvin_error_t error);


#endif
