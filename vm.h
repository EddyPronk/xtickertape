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
static const char cvs_VM_H[] = "$Id: vm.h,v 2.1 2000/11/17 09:47:39 phelps Exp $";
#endif /* lint */

/* Objects are really handles to the world outside the VM */
typedef struct vm_object *vm_object_t;

/* The virtual machine type */
typedef struct vm *vm_t;


/* The types of objects */
typedef enum
{
    /* Atomic types are 0-7 */
    SEXP_NIL = 0,
    SEXP_INT32,
    SEXP_INT64,
    SEXP_FLOAT,
    SEXP_STRING,
    SEXP_CHAR,

    /* Combined types are 8-15 */
    SEXP_SYMBOL = 8,
    SEXP_CONS,
    SEXP_BUILTIN,
    SEXP_LAMBDA,
    SEXP_ENV,
    SEXP_ARRAY
} object_type_t;

/* Allocates and initializes a new virtual machine */
vm_t vm_alloc(elvin_error_t error);

/* Frees a virtual machine */
int vm_free(vm_t vm, elvin_error_t error);


/* Returns an object's type */
object_type_t vm_object_type(vm_object_t object);

/* Gets a reference to `nil' */
int nil_alloc(vm_object_t result, elvin_error_t error);

/* Gets a new 31-bit integer object */
int integer_alloc(int32_t value, vm_object_t result, elvin_error_t error);

/* Returns an integer's value */
int32_t integer_value(vm_object_t object, elvin_error_t error);

/* Allocates and initializes a new long integer object */
int long_alloc(int64_t value, vm_object_t result, elvin_error_t error);

/* Returns the value of a long integer object */
int64_t long_value(vm_object_t object);

/* Allocates and initializes a new float object */
int float_alloc(double value, vm_object_t result, elvin_error_t error);

/* Returns the value of a float */
double float_value(vm_object_t object);

/* Allocates and initializes a new string object */
int string_alloc(char *value, vm_object_t result, elvin_error_t error);

/* Returns the string as a char * */
char *string_value(vm_object_t object);

/* Allocates and initializes a new character object */
int char_alloc(char value, vm_object_t result, elvin_error_t error);

/* Returns the char's value */
char char_value(vm_object_t object);

/* Allocates and initializes a new symbol object */
int symbol_alloc(char *name, vm_object_t result, elvin_error_t error);

/* Returns the symbol's name */
char *symbol_name(vm_object_t object);

/* Allocates and initializes a new cons cell */
int cons_alloc(vm_object_t car, vm_object_t cdr, vm_object_t result, elvin_error_t error);

/* Returns the car of a cons cell */
void cons_car(vm_object_t object, vm_object_t result);

/* Returns the cdr of a cons cell */
void cons_cdr(vm_object_t object, vm_object_t result);

/* Allocates and initializes a new primitive subroutine object */
int builtin_alloc(char *name, void *function, vm_object_t result, elvin_error_t error);

/* Allocates and initializes a new lambda object */
int lambda_alloc(
    vm_object_t env,
    vm_object_t arg_list,
    vm_object_t body,
    vm_object_t result,
    elvin_error_t error);

/* Returns an object's type */
object_type_t vm_object_type(vm_object_t object);

/* Allocates and initializes a new array object */
int array_alloc(uint32_t size, vm_object_t result, elvin_error_t error);

/* Sets a value in an array */
int array_set(vm_object_t array, uint32_t index, vm_object_t value, elvin_error_t error);

/* Gets a value from an array */
int array_get(vm_object_t array, uint32_t index, vm_object_t result, elvin_error_t error);

#endif
