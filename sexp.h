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

#ifndef SEXP_H
#define SEXP_H

#ifndef lint
static const char cvs_SEXP_H[] = "$Id: sexp.h,v 2.4 2000/11/09 03:04:54 phelps Exp $";
#endif /* lint */

/* An env_t is an opaque struct as well */
typedef struct env *env_t;

/* The types of sexps */
typedef enum
{
    SEXP_NIL,
    SEXP_INT32,
    SEXP_INT64,
    SEXP_FLOAT,
    SEXP_STRING,
    SEXP_CHAR,
    SEXP_SYMBOL,
    SEXP_CONS,
    SEXP_BUILTIN,
    SEXP_LAMBDA
} sexp_type_t;

/* An sexp_t is a an opaque struct */
typedef struct sexp *sexp_t;

/* The built-in function type */
typedef int (*builtin_t)(env_t env, sexp_t args, sexp_t *result, elvin_error_t error);


/* Initialize the interpreter */
int interp_init(elvin_error_t error);

/* Shut down the interpreter */
int interp_close(elvin_error_t error);


/* Answers the unique nil instance */
sexp_t nil_alloc(elvin_error_t error);

/* Allocates and initializes a new symbol sexp */
sexp_t symbol_alloc(char *name, elvin_error_t error);

/* Allocates and initializes a new 32-bit integer sexp */
sexp_t int32_alloc(int32_t value, elvin_error_t error);

/* Allocates and initializes a new 64-bit integer sexp */
sexp_t int64_alloc(int64_t value, elvin_error_t error);

/* Allocates and initializes a new float sexp */
sexp_t float_alloc(double value, elvin_error_t error);

/* Allocates and initializes a new string sexp */
sexp_t string_alloc(char *value, elvin_error_t error);

/* Allocates and initializes a new char sexp */
sexp_t char_alloc(char ch, elvin_error_t error);

/* Allocates and initializes a new cons sexp */
sexp_t cons_alloc(sexp_t car, sexp_t cdr, elvin_error_t error);

/* Allocates and initializes a new built-in sexp */
sexp_t builtin_alloc(char *name, builtin_t function, elvin_error_t error);

/* Allocates and initializes a new lambda sexp */
sexp_t lambda_alloc(sexp_t arg_list, sexp_t body, elvin_error_t error);

/* Returns the sexp's type */
sexp_type_t sexp_get_type(sexp_t sexp);


/* Allocates another reference to an sexp */
int sexp_alloc_ref(sexp_t sexp, elvin_error_t error);

/* Frees an sexp */
int sexp_free(sexp_t sexp, elvin_error_t error);

/* Allocates and returns a string representing the sexp */
char *sexp_to_string(sexp_t sexp, elvin_error_t error);

/* Evaluates an sexp */
int sexp_eval(sexp_t sexp, env_t env, sexp_t *result, elvin_error_t error);

/* Returns the symbol's name */
char *symbol_name(sexp_t sexp, elvin_error_t error);

/* Returns the integer's value */
int32_t int32_value(sexp_t sexp, elvin_error_t error);

/* Returns the integer's value */
int64_t int64_value(sexp_t sexp, elvin_error_t error);

/* Returns the string's bytes */
char *string_value(sexp_t sexp, elvin_error_t error);

/* Returns the char's byte */
char char_value(sexp_t sexp, elvin_error_t error);

/* Answers the car of a cons sexp */
sexp_t cons_car(sexp_t sexp, elvin_error_t error);

/* Answers the cdr of a cons sexp */
sexp_t cons_cdr(sexp_t sexp, elvin_error_t error);

/* Reverse the elements of a list */
sexp_t cons_reverse(sexp_t sexp, sexp_t end, elvin_error_t error);


/* Allocates and returns an evalutaion environment */
env_t env_alloc(uint32_t size, env_t parent, elvin_error_t error);

/* Frees an environment and all of its references */
int env_free(env_t env, elvin_error_t error);

/* Looks up a symbol's value in the environment */
int env_get(env_t env, sexp_t sexp, sexp_t *result, elvin_error_t error);

/* Sets a symbol's value in the environment */
int env_set(env_t env, sexp_t sexp, sexp_t value, elvin_error_t error);


/* Sets the named symbol's value to the int32 value in env */
int env_set_int32(env_t env, char *name, int32_t value, elvin_error_t error);

/* Sets the named symbol's value to the int64 value in env */
int env_set_int64(env_t env, char *name, int64_t value, elvin_error_t error);

/* Sets the named symbol's value to the float value in env */
int env_set_float(env_t env, char *name, double value, elvin_error_t error);

/* Sets the named symbol's value to the string value in env */
int env_set_string(env_t env, char *name, char *value, elvin_error_t error);

/* Sets the named symbol's value to the symbol in env */
int env_set_symbol(env_t env, char *name, char *value, elvin_error_t error);

/* Sets the named symbol's value to the built-in function in env */
int env_set_builtin(env_t env, char *name, builtin_t function, elvin_error_t error);

#endif /* SEXP_H */
