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
static const char cvs_AST_H[] = "$Id: ast.h,v 1.2 2000/07/07 05:57:36 phelps Exp $";
#endif /* lint */

/* The ast data type */
typedef struct ast *ast_t;

/* The supported binary operations */
enum ast_binop
{
    AST_ASSIGN,
    AST_OR,
    AST_AND,
    AST_EQ,
    AST_LT,
    AST_GT
};

typedef enum ast_binop ast_binop_t;


/* Allocates and initializes a new int32 AST node */
ast_t ast_int32_alloc(int32_t value, elvin_error_t error);

/* Allocates and initializes a new int64 AST node */
ast_t ast_int64_alloc(int64_t value, elvin_error_t error);

/* Allocates and initializes a new real64 AST node */
ast_t ast_real64_alloc(double value, elvin_error_t error);

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

/* Allocates and initializes a new binary operation AST node */
ast_t ast_binop_alloc(
    ast_binop_t op,
    ast_t lvalue,
    ast_t rvalue,
    elvin_error_t error);

/* Allocates and initializes a new subscription AST node */
ast_t ast_sub_alloc(ast_t tag, ast_t statements, elvin_error_t error);

/* Frees the resources consumed by the ast */
int ast_free(ast_t self, elvin_error_t error);


/* Appends another element to the end of an AST list */
ast_t ast_append(ast_t list, ast_t item, elvin_error_t error);

/* Allocates and returns a string representation of the AST */
char *ast_to_string(ast_t self, elvin_error_t error);

#endif /* AST_H */
