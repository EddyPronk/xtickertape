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

#ifndef AST_H
#define AST_H

#ifndef lint
static const char cvs_AST_H[] = "$Id: ast.h,v 1.8 2000/07/11 07:11:52 phelps Exp $";
#endif /* lint */

/* The ast data type */
typedef struct ast *ast_t;

/* The value data type */
typedef struct value value_t;

/* This shouldn't be here */
typedef struct subscription *subscription_t;



/* The types of the values */
typedef enum value_type
{
    VALUE_NONE,
    VALUE_INT32,
    VALUE_INT64,
    VALUE_FLOAT,
    VALUE_STRING,
    VALUE_OPAQUE,
    VALUE_LIST,
    VALUE_BLOCK
} value_type_t;

/* A list is an array of other values */
typedef struct list list_t;
struct list
{
    /* The number of items in the list */
    uint32_t count;

    /* The items */
    value_t *items;
};


/* Allocates and returns a string representation of the value */
char *value_to_string(value_t *self, elvin_error_t error);

/* Frees a value's contents */
int value_free(value_t *value, elvin_error_t error);

/* Copes a value */
int value_copy(value_t *self, value_t *copy, elvin_error_t error);

/* An environment is really just a hashtable */
typedef elvin_hashtable_t env_t;

/* Allocates and initializes a new environment */
env_t env_alloc(elvin_error_t error);

/* Frees an env */
int env_free(env_t env, elvin_error_t error);

/* Looks up a value in the environment */
value_t *env_get(env_t self, char *name, elvin_error_t error);

/* Adds a value to the environment */
int env_put(env_t self, char *name, value_t *value, elvin_error_t error);


/* The value union contains all possible types values */
typedef union value_union
{
    int32_t int32;
    int64_t int64;
    double real64;
    uchar *string;
    elvin_opaque_t opaque;
    list_t list;
    ast_t block;
} value_union_t;

struct value
{
    /* Hack to make values look like av_tuples */
    void *ignored;

    /* The value's type */
    value_type_t type;

    /* And it's value */
    value_union_t value;
};


/* The unary operators */
enum ast_unary
{
    AST_NOT
};
typedef enum ast_unary ast_unary_t;

/* The binary operators */
enum ast_binary
{
    AST_ASSIGN,
    AST_OR,
    AST_AND,
    AST_EQ,
    AST_LT,
    AST_GT
};

typedef enum ast_binary ast_binary_t;


/* Allocates and initializes a new int32 AST node */
ast_t ast_int32_alloc(int32_t value, elvin_error_t error);

/* Allocates and initializes a new int64 AST node */
ast_t ast_int64_alloc(int64_t value, elvin_error_t error);

/* Allocates and initializes a new float AST node */
ast_t ast_float_alloc(double value, elvin_error_t error);

/* Allocates and initializes a new string AST node */
ast_t ast_string_alloc(char *value, elvin_error_t error);

/* Allocates and initializes a new list AST node */
ast_t ast_list_alloc(ast_t list, elvin_error_t error);

/* Allocates and initializes a new id AST node */
ast_t ast_id_alloc(char *name, elvin_error_t error);

/* Allocates and initializes a new function AST node */
ast_t ast_function_alloc(ast_t id, ast_t args, elvin_error_t error);

/* Allocates and initializes a new block AST node */
ast_t ast_block_alloc(ast_t body, elvin_error_t error);

/* Allocates and initialzes a new unary AST node */
ast_t ast_unary_alloc(ast_unary_t op, ast_t value, elvin_error_t error);

/* Allocates and initializes a new binary operation AST node */
ast_t ast_binary_alloc(
    ast_binary_t op,
    ast_t lvalue,
    ast_t rvalue,
    elvin_error_t error);

/* Allocates and initializes a new subscription AST node */
ast_t ast_sub_alloc(ast_t tag, ast_t statements, elvin_error_t error);

/* Clones the ast (actually gets another reference to it) */
ast_t ast_clone(ast_t self, elvin_error_t error);

/* Frees the resources consumed by the ast */
int ast_free(ast_t self, elvin_error_t error);


/* Appends another element to the end of an AST list */
ast_t ast_append(ast_t list, ast_t item, elvin_error_t error);


/* Evaluates an AST in the given environment */
int ast_eval(
    ast_t self,
    elvin_hashtable_t env,
    value_t *value_out,
    elvin_error_t error);

/* Evaluates a list of subscriptions */
int ast_eval_sub_list(
    ast_t self,
    elvin_hashtable_t env,
    uint32_t *count_out,
    subscription_t **subs_out,
    elvin_error_t error);

/* Allocates and returns a string representation of the AST */
char *ast_to_string(ast_t self, elvin_error_t error);

#endif /* AST_H */
