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

#ifndef lint
static const char cvsid[] = "$Id: parser.c,v 2.19 2000/07/11 07:12:10 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/convert.h>
#include <elvin/errors/unix.h>
#include "errors.h"
#include "ast.h"
#include "subscription.h"
#include "parser.h"

#define INITIAL_TOKEN_BUFFER_SIZE 64
#define INITIAL_STACK_DEPTH 8
#define BUFFER_SIZE 4096

/* Test ch to see if it's valid as a non-initial ID character */
static int is_id_char(int ch)
{
    static char table[] =
    {
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x00 */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* 0x10 */
	0, 1, 0, 1,  1, 1, 1, 0,  0, 0, 1, 1,  0, 1, 1, 1, /* 0x20 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 0, 0,  1, 1, 1, 1, /* 0x30 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x40 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0,  0, 0, 1, 1, /* 0x50 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, /* 0x60 */
	1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 0,  1, 0, 1, 0  /* 0x70 */
    };

    /* Use a table for quick lookup of those tricky symbolic chars */
    return (ch < 0 || ch > 127) ? 0 : table[ch];
}


/* The list of token names */
static char *token_names[] =
{
    "[eof]", "{", "}", ";", "[id]", ":", "=",
    "||", "&&", "==", "!=", "<", "<=", ">", ">=",
    "!", ",", "[string]", "[int32]", "[int64]", "[float]",
    "(", ")", "[", "]"
};

/* The lexer_state_t type */
typedef int (*lexer_state_t)(parser_t parser, int ch, elvin_error_t error);

/* The structure of the configuration file parser */
struct parser
{
    /* The tag for error messages */
    char *tag;

    /* The current line number */
    int line_num;

    /* The state stack */
    int *state_stack;

    /* The top of the state stack */
    int *state_top;

    /* The end of the state stack */
    int *state_end;

    /* The value stack */
    ast_t *value_stack;

    /* The top of the value stack */
    ast_t *value_top;

    /* The current lexical state */
    lexer_state_t state;

    /* The token construction buffer */
    char *token;

    /* The next character in the token buffer */
    char *point;

    /* The end of the token buffer */
    char *token_end;

    /* The callback for when we've completed a subscription entry */
    parser_callback_t callback;
    
    /* The client-supplied arg for the callback */
    void *rock;
};



/* Reduction function declarations */
typedef ast_t (*reduction_t)(parser_t self, elvin_error_t error);
static ast_t identity(parser_t self, elvin_error_t error);
static ast_t identity2(parser_t self, elvin_error_t error);
static ast_t append(parser_t self, elvin_error_t error);
static ast_t make_sub(parser_t self, elvin_error_t error);
static ast_t make_default_sub(parser_t self, elvin_error_t error);
static ast_t make_assignment(parser_t self, elvin_error_t error);
static ast_t extend_disjunction(parser_t self, elvin_error_t error);
static ast_t extend_conjunction(parser_t self, elvin_error_t error);
static ast_t make_eq(parser_t self, elvin_error_t error);
static ast_t make_neq(parser_t self, elvin_error_t error);
static ast_t make_lt(parser_t self, elvin_error_t error);
static ast_t make_le(parser_t self, elvin_error_t error);
static ast_t make_gt(parser_t self, elvin_error_t error);
static ast_t make_ge(parser_t self, elvin_error_t error);
static ast_t make_not(parser_t self, elvin_error_t error);
static ast_t extend_values(parser_t self, elvin_error_t error);
static ast_t make_list(parser_t self, elvin_error_t error);
static ast_t make_empty_list(parser_t self, elvin_error_t error);
static ast_t make_function(parser_t self, elvin_error_t error);
static ast_t make_noarg_function(parser_t self, elvin_error_t error);
static ast_t make_block(parser_t self, elvin_error_t error);

/* Lexer state function headers */
static int lex_start(parser_t self, int ch, elvin_error_t error);
static int lex_comment(parser_t self, int ch, elvin_error_t error);
static int lex_bang(parser_t self, int ch, elvin_error_t error);
static int lex_ampersand(parser_t self, int ch, elvin_error_t error);
static int lex_eq(parser_t self, int ch, elvin_error_t error);
static int lex_lt(parser_t self, int ch, elvin_error_t error);
static int lex_gt(parser_t self, int ch, elvin_error_t error);
static int lex_vbar(parser_t self, int ch, elvin_error_t error);
static int lex_zero(parser_t self, int ch, elvin_error_t error);
static int lex_decimal(parser_t self, int ch, elvin_error_t error);
static int lex_octal(parser_t self, int ch, elvin_error_t error);
static int lex_hex(parser_t self, int ch, elvin_error_t error);
static int lex_float_first(parser_t self, int ch, elvin_error_t error);
static int lex_float(parser_t self, int ch, elvin_error_t error);
static int lex_exp_first(parser_t self, int ch, elvin_error_t error);
static int lex_exp_sign(parser_t self, int ch, elvin_error_t error);
static int lex_exp(parser_t self, int ch, elvin_error_t error);
static int lex_dq_string(parser_t self, int ch, elvin_error_t error);
static int lex_dq_string_esc(parser_t self, int ch, elvin_error_t error);
static int lex_sq_string(parser_t self, int ch, elvin_error_t error);
static int lex_sq_string_esc(parser_t self, int ch, elvin_error_t error);
static int lex_id(parser_t self, int ch, elvin_error_t error);
static int lex_id_esc(parser_t self, int ch, elvin_error_t error);

/* Include the parser tables */
#include "grammar.h"

/* Makes a deeper stack */
static int grow_stack(parser_t self, elvin_error_t error)
{
    size_t length = (self -> state_end - self -> state_stack) * 2;
    int *state_stack;
    ast_t *value_stack;

    /* Allocate memory for the new state stack */
    if (! (state_stack = (int *)ELVIN_REALLOC(self -> state_stack, length * sizeof(int), error)))
    {
	return 0;
    }

    /* Update the state stack's pointers */
    self -> state_top = self -> state_top - self -> state_stack + state_stack;
    self -> state_stack = state_stack;
    self -> state_end = state_stack + length;

    /* Allocate memory for the new value stack */
    if (! (value_stack = (ast_t *)ELVIN_REALLOC(self -> value_stack, length * sizeof(ast_t), error)))
    {
	return 0;
    }

    /* Update the value stack's pointers */
    self -> value_top = value_stack + (self -> value_top - self -> value_stack);
    self -> value_stack = value_stack;
    return 1;
}

/* Pushes a state and value onto the stack */
static int push(parser_t self, int state, ast_t value, elvin_error_t error)
{
    /* Make sure there's enough room on the stack */
    if (! (self -> state_top + 1 < self -> state_end))
    {
	if (! grow_stack(self, error))
	{
	    return 0;
	}
    }

    /* The state stack is pre-increment */
    *(++self->state_top) = state;

    /* The value stack is post-increment */
    *(self->value_top++) = value;
    return 1;
}

/* Moves the top of the stack back `count' positions */
static void pop(parser_t self, int count, elvin_error_t error)
{
#ifdef DEBUG
    /* Sanity check */
    if (self -> state_stack > self -> state_top - count)
    {
	fprintf(stderr, "popped off the top of the stack\n");
	abort();
    }
#endif

    /* Adjust the appropriate pointers */
    self -> state_top -= count;
    self -> value_top -= count;
}

/* Puts n elements back onto the stack */
static void unpop(parser_t self, int count, elvin_error_t error)
{
    fprintf(stderr, "unpop: not yet implemented\n");
    abort();
}

/* Answers the top of the state stack */
static int top(parser_t self)
{
    return *(self->state_top);
}

/* Returns the first value */
static ast_t identity(parser_t self, elvin_error_t error)
{
    return self -> value_top[0];
}

/* Returns the second value */
static ast_t identity2(parser_t self, elvin_error_t error)
{
    return self -> value_top[1];
}

/* Appends a node to the end of an AST list */
static ast_t append(parser_t self, elvin_error_t error)
{
    ast_t list, value, result;

    /* Pull the children values off the stack */
    list = self -> value_top[0];
    value = self -> value_top[1];

    /* Append the value to the end of the list */
    if (! (result = ast_append(list, value, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(value, error))
    {
	return NULL;
    }

    return result;
}


/* <subscription> ::= <tag> LBRACE <statements> RBRACE SEMI */
static ast_t make_sub(parser_t self, elvin_error_t error)
{
    ast_t tag, statements, result;

    /* Extract the tag and statements from the stack */
    tag = self -> value_top[0];
    statements = self -> value_top[2];

    /* Create a subscription node */
    if (! (result = ast_sub_alloc(tag, statements, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(tag, error))
    {
	return NULL;
    }

    if (! ast_free(statements, error))
    {
	return NULL;
    }

    return result;
}

/* <subscription> ::= <tag> LBRACE RBRACE SEMI */
static ast_t make_default_sub(parser_t self, elvin_error_t error)
{
    ast_t tag, result;

    /* Extract the tag from the stack */
    tag = self -> value_top[0];

    /* Create an empty subscription node */
    if (! (result = ast_sub_alloc(tag, NULL, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(tag, error))
    {
	return NULL;
    }

    return result;
}

/* <statement> ::= ID ASSIGN <disjunction> SEMI */
static ast_t make_assignment(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the components of the assignment node */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Make a new assignment node */
    if (! (result = ast_binary_alloc(AST_ASSIGN, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <disjunction> ::= <disjunction> OR <conjunction> */
static ast_t extend_disjunction(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the components of the disjunction */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Make a new disjunction node */
    if (! (result = ast_binary_alloc(AST_OR, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <conjunction> ::= <conjunction> AND <term> */
static ast_t extend_conjunction(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the components of the conjunction */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Make a new conjunction node */
    if (! (result = ast_binary_alloc(AST_AND, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> EQ <value> */
static ast_t make_eq(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the halves of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Make a new equality node */
    if (! (result = ast_binary_alloc(AST_EQ, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> NEQ <value> */
static ast_t make_neq(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, eq, result;

    /* Look up the halves of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Make an equality node */
    if (! (eq = ast_binary_alloc(AST_EQ, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Wrap it in a not node */
    if (! (result = ast_unary_alloc(AST_NOT, eq, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    if (! ast_free(eq, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> LT <value> */
static ast_t make_lt(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the children of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Construct the comparison node */
    if (! (result = ast_binary_alloc(AST_LT, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> LE <value> */
static ast_t make_le(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, gt, result;

    /* Look up the children of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Construct the comparison node */
    if (! (gt = ast_binary_alloc(AST_GT, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Wrap it in a negation node */
    if (! (result = ast_unary_alloc(AST_NOT, gt, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    if (! ast_free(gt, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> GT <value> */
static ast_t make_gt(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, result;

    /* Look up the children of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Construct the comparison node */
    if (! (result = ast_binary_alloc(AST_GT, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    return result;
}

/* <term> ::= <term> GE <value> */
static ast_t make_ge(parser_t self, elvin_error_t error)
{
    ast_t lvalue, rvalue, lt, result;

    /* Look up the children of the comparison */
    lvalue = self -> value_top[0];
    rvalue = self -> value_top[2];

    /* Construct the comparison node */
    if (! (lt = ast_binary_alloc(AST_LT, lvalue, rvalue, error)))
    {
	return NULL;
    }

    /* Wrap it in a not node */
    if (! (result = ast_unary_alloc(AST_NOT, lt, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(lvalue, error))
    {
	return NULL;
    }

    if (! ast_free(rvalue, error))
    {
	return NULL;
    }

    if (! ast_free(lt, error))
    {
	return NULL;
    }

    return result;
}

/* <value> ::= BANG <value> */
static ast_t make_not(parser_t self, elvin_error_t error)
{
    ast_t value, result;

    /* Get the value to negate */
    value = self -> value_top[1];

    /* Create a new not node */
    if (! (result = ast_unary_alloc(AST_NOT, value, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(value, error))
    {
	return NULL;
    }
    
    return result;
}


/* <values> ::= <values> COMMA <disjunction> */
static ast_t extend_values(parser_t self, elvin_error_t error)
{
    ast_t list, value, result;

    /* Get the children */
    list = self -> value_top[0];
    value = self -> value_top[2];

    /* Extend the list */
    if (! (result = ast_append(list, value, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(value, error))
    {
	return NULL;
    }

    return result;
}

/* <value> ::= LBRACE <values> RBRACE */
static ast_t make_list(parser_t self, elvin_error_t error)
{
    ast_t value, result;

    /* Get the child */
    value = self -> value_top[1];

    /* Create the list node */
    if (! (result = ast_list_alloc(value, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(value, error))
    {
	return NULL;
    }

    return result;
}

/* <value> ::= LBRACE RBRACE */
static ast_t make_empty_list(parser_t self, elvin_error_t error)
{
    return ast_list_alloc(NULL, error);
}


/* <value> ::= ID LPAREN <values> RPAREN */
static ast_t make_function(parser_t self, elvin_error_t error)
{
    ast_t id, args, result;

    /* Look up the components of the function */
    id = self -> value_top[0];
    args = self -> value_top[2];

    /* Create the function node */
    if (! (result = ast_function_alloc(id, args, error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(id, error))
    {
	return NULL;
    }

    if (! ast_free(args, error))
    {
	return NULL;
    }

    return result;
}

/* <value> ::= ID LPAREN RPAREN */
static ast_t make_noarg_function(parser_t self, elvin_error_t error)
{
    fprintf(stderr, "make_noarg_function(): not yet implemented\n");
    return NULL;
}

/* <block> ::= LBRACKET <value> RBRACKET */
static ast_t make_block(parser_t self, elvin_error_t error)
{
    ast_t value, result;

    /* Look up the value */
    value = self -> value_top[1];

    /* Create the block */
    if (! (result = ast_block_alloc(self -> value_top[1], error)))
    {
	return NULL;
    }

    /* Clean up */
    if (! ast_free(value, error))
    {
	return NULL;
    }

    return result;
}


/* Frees an array of subscriptions */
static int subscription_array_free(
    uint32_t count,
    subscription_t *subs,
    elvin_error_t error)
{
    uint32_t i;

    for (i = 0; i < count; i++)
    {
	if (! subscription_free(subs[i], error))
	{
	    return 0;
	}
    }

    return ELVIN_FREE(subs, error);
}

/* <config-file> ::= <subscription-list> */
static int do_accept(parser_t self, elvin_error_t error)
{
    uint32_t count;
    subscription_t *subs;

    /* Evaluate the subscription list to get a list of subscriptions */
    if (! ast_eval_sub_list(self -> value_stack[0], NULL, &count, &subs, error))
    {
	return 0;
    }

    /* Reset the parser */
    if (! parser_reset(self, error))
    {
	return 0;
    }

    /* If we have a callback then give it the subscriptions */
    if (self -> callback)
    {
	if (! self -> callback(self -> rock, count, subs, error))
	{
	    subscription_array_free(count, subs, NULL);
	    return 0;
	}

	return 1;
    }

    return subscription_array_free(count, subs, NULL);
}

/* Accepts another token and performs as many parser transitions as
 * possible with the data it has seen so far */
static int shift_reduce(parser_t self, terminal_t terminal, ast_t value, elvin_error_t error)
{
    int action;
    struct production *production;
    int reduction;
    ast_t result;

    /* Reduce as many times as possible */
    while (IS_REDUCE(action = sr_table[top(self)][terminal]))
    {
	/* Locate the production rule to use */
	reduction = REDUCTION(action);
	production = productions + reduction;

	/* Point the stack at the beginning of the components */
	pop(self, production -> count, error);

	/* Reduce by calling the production's reduction function */
	if (! (result = production -> reduction(self, error)))
	{
	    /* Something went wrong -- put the args back on the stack */
	    unpop(self, production -> count, error);
	    return 0;
	}

	/* Push the result back onto the stack */
	if (! push(self, REDUCE_GOTO(top(self), production), result, error))
	{
	    return 0;
	}
    }

    /* See if we can shift */
    if (IS_SHIFT(action))
    {
	return push(self, SHIFT_GOTO(action), value, error);
    }

    /* See if we can accept */
    if (IS_ACCEPT(action))
    {
	return do_accept(self, error);
    }

    /* Everything else is an error */
    if (value == NULL)
    {
	ELVIN_ERROR_XTICKERTAPE_PARSE_ERROR(
	    error, self -> tag, self -> line_num,
	    token_names[terminal]);
	return 0;
    }

    ELVIN_ERROR_XTICKERTAPE_PARSE_ERROR(
	error, self -> tag, self -> line_num,
	self -> token);

    /* Clean up */
    ast_free(value, NULL);
    return 0;
}

/* Accepts token which has no useful value */
static int accept_token(parser_t self, terminal_t terminal, elvin_error_t error)
{
    return shift_reduce(self, terminal, NULL, error);
}

/* Accepts an ID token */
static int accept_id(parser_t self, char *name, elvin_error_t error)
{
    ast_t node;

    /* Make an AST node out of the identifier */
    if (! (node = ast_id_alloc(name, error)))
    {
	return 0;
    }

    return shift_reduce(self, TT_ID, node, error);
}

/* Accepts an INT32 token */
static int accept_int32(parser_t self, int32_t value, elvin_error_t error)
{
    ast_t node;

    /* Make an AST node out of the int32 */
    if (! (node = ast_int32_alloc(value, error)))
    {
	return 0;
    }

    return shift_reduce(self, TT_INT32, node, error);
}

/* Accepts a string as an int32 token */
static int accept_int32_string(parser_t self, char *string, elvin_error_t error)
{
    int32_t value;

    if (! elvin_string_to_int32(string, &value, error))
    {
	return 0;
    }

    return accept_int32(self, value, error);
}

/* Accepts an INT64 token */
static int accept_int64(parser_t self, int64_t value, elvin_error_t error)
{
    ast_t node;

    /* Make an AST out of the int64 */
    if (! (node = ast_int64_alloc(value, error)))
    {
	return 0;
    }

    return shift_reduce(self, TT_INT64, node, error);
}

/* Accepts a string as an int64 token */
static int accept_int64_string(parser_t self, char *string, elvin_error_t error)
{
    int64_t value;

    if (! elvin_string_to_int64(string, &value, error))
    {
	return 0;
    }

    return accept_int64(self, value, error);
}

/* Accepts a FLOAT token */
static int accept_float(parser_t self, double value, elvin_error_t error)
{
    ast_t node;

    /* Make an AST out of the float */
    if (! (node = ast_float_alloc(value, error)))
    {
	return 0;
    }

    return shift_reduce(self, TT_FLOAT, node, error);
}

/* Accepts a string as a float token */
static int accept_float_string(parser_t self, char *string, elvin_error_t error)
{
    double value;

    if (! elvin_string_to_real64(string, &value, error))
    {
	return 0;
    }

    return accept_float(self, value, error);
}

/* Accepts an STRING token */
static int accept_string(parser_t self, char *string, elvin_error_t error)
{
    ast_t node;

    /* Make an AST out of the string */
    if (! (node = ast_string_alloc(string, error)))
    {
	return 0;
    }

    return shift_reduce(self, TT_STRING, node, error);
}



/* Expands the token buffer */
static int grow_buffer(parser_t self, elvin_error_t error)
{
    size_t length = (self -> token_end - self -> token) * 2;
    char *token;

    /* Make the buffer bigger */
    if (! (token = (char *)ELVIN_REALLOC(self -> token, length, error)))
    {
	return 0;
    }

    /* Update the pointers */
    self -> point = self -> point - self -> token + token;
    self -> token_end = token + length;
    self -> token = token;
    return 1;
}

/* Appends a character to the end of the token */
static int append_char(parser_t self, int ch, elvin_error_t error)
{
    /* Make sure there's room */
    if (! (self -> point < self -> token_end))
    {
	if (! grow_buffer(self, error))
	{
	    return 0;
	}
    }

    *(self -> point++) = ch;
    return 1;
}

/* Handle the first character of a new token */
static int lex_start(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* The end of the input file */
	case EOF:
	{
	    if (! accept_token(self, TT_EOF, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* BANG or NEQ */
	case '!':
	{
	    self -> state = lex_bang;
	    return 1;
	}

	/* COMMENT */
	case '#':
	{
	    self -> point = self -> token;
	    return lex_comment(self, ch, error);
	}
	

	/* Double-quoted string */
	case '"':
	{
	    self -> point = self -> token;
	    self -> state = lex_dq_string;
	    return 1;
	}

	/* AND */
	case '&':
	{
	    self -> state = lex_ampersand;
	    return 1;
	}

	/* Single-quoted string */
	case '\'':
	{
	    self -> point = self -> token;
	    self -> state = lex_sq_string;
	    return 1;
	}

	/* LPAREN */
	case '(':
	{
	    if (! accept_token(self, TT_LPAREN, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* RPAREN */
	case ')':
	{
	    if (! accept_token(self, TT_RPAREN, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* COMMA */
	case ',':
	{
	    if (! accept_token(self, TT_COMMA, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Zero can lead to many different kinds of numbers */
	case '0':
	{
	    self -> point = self -> token;
	    if (! append_char(self, ch, error))
	    {
		return 0;
	    }

	    self -> state = lex_zero;
	    return 1;
	}

	/* COLON */
	case ':':
	{
	    if (! accept_token(self, TT_COLON, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* SEMI */
	case ';':
	{
	    if (! accept_token(self, TT_SEMI, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* LT or LE */
	case '<':
	{
	    self -> state = lex_lt;
	    return 1;
	}

	/* ASSIGN or EQ */
	case '=':
	{
	    self -> state = lex_eq;
	    return 1;
	}

	/* GT or GE */
	case '>':
	{
	    self -> state = lex_gt;
	    return 1;
	}

	/* LBRACKET */
	case '[':
	{
	    if (! accept_token(self, TT_LBRACKET, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* ID with quoted first character */
	case '\\':
	{
	    self -> point = self -> token;
	    self -> state = lex_id_esc;
	    return 1;
	}

	/* RBRACKET */
	case ']':
	{
	    if (! accept_token(self, TT_RBRACKET, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* ID */
	case '_':
	{
	    self -> point = self -> token;
	    if (! append_char(self, ch, error))
	    {
		return 0;
	    }

	    self -> state = lex_id;
	    return 1;
	}

	/* OR */
	case '|':
	{
	    self -> state = lex_vbar;
	    return 1;
	}

	/* LBRACE */
	case '{':
	{
	    if (! accept_token(self, TT_LBRACE, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* RBRACE */
	case '}':
	{
	    if (! accept_token(self, TT_RBRACE, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}
    }

    /* Ignore whitespace */
    if (isspace(ch))
    {
	self -> state = lex_start;
	return 1;
    }

    /* Watch for a number */
    if (isdigit(ch))
    {
	self -> point = self -> token;
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_decimal;
	return 1;
    }

    /* Watch for identifiers */
    if (isalpha(ch))
    {
	self -> point = self -> token;
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_id;
	return 1;
    }

    /* Anything else is a bogus token */
    if (! append_char(self, ch, error))
    {
	return 0;
    }

    /* Null-terminate the string */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)self -> token);
    return 0;
}


/* Reading a line comment */
static int lex_comment(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for the end of file */
    if (ch == EOF)
    {
	return lex_start(self, ch, error);
    }

    /* Watch for the end of the line */
    if (ch == '\n')
    {
	self -> state = lex_start;
	return 1;
    }

    /* Ignore everything else */
    self -> state = lex_comment;
    return 1;
}

/* Reading the character after a `!' */
static int lex_bang(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for an `=' */
    if (ch == '=')
    {
	/* Accept an NEQ token */
	if (! accept_token(self, TT_NEQ, error))
	{
	    return 0;
	}

	/* Go back to the start state */
	self -> state = lex_start;
	return 1;
    }

    /* Otherwise accept a BANG token */
    if (! accept_token(self, TT_BANG, error))
    {
	return 0;
    }

    /* And scan ch again */
    return lex_start(self, ch, error);
}

/* Reading the character after a `&' */
static int lex_ampersand(parser_t self, int ch, elvin_error_t error)
{
    /* Another `&' is an AND */
    if (ch == '&')
    {
	if (! accept_token(self, TT_AND, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Otherwise we've got a problem */
    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)"&");
    return 0;
}

/* Reading the character after an initial `=' */
static int lex_eq(parser_t self, int ch, elvin_error_t error)
{
    /* Is there an other `=' for an EQ token? */
    if (ch == '=')
    {
	if (! accept_token(self, TT_EQ, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Nope.  This must be an ASSIGN. */
    if (! accept_token(self, TT_ASSIGN, error))
    {
	return 0;
    }

    return lex_start(self, ch, error);
}

/* Reading the character after a `<' */
static int lex_lt(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* LE */
	case '=':
	{
	    if (! accept_token(self, TT_LE, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* Anything else is the next character */
	default:
	{
	    if (! accept_token(self, TT_LT, error))
	    {
		return 0;
	    }

	    return lex_start(self, ch, error);
	}
    }
}

/* Reading the character after a `>' */
static int lex_gt(parser_t self, int ch, elvin_error_t error)
{
    switch (ch)
    {
	/* GE */
	case '=':
	{
	    if (! accept_token(self, TT_GE, error))
	    {
		return 0;
	    }

	    self -> state = lex_start;
	    return 1;
	}

	/* GT */
	default:
	{
	    if (! accept_token(self, TT_GT, error))
	    {
		return 0;
	    }

	    return lex_start(self, ch, error);
	}
    }
}

/* Reading the character after a `|' */
static int lex_vbar(parser_t self, int ch, elvin_error_t error)
{
    /* Another `|' is an OR token */
    if (ch == '|')
    {
	if (! accept_token(self, TT_OR, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* We're not doing simple math yet */
    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)"|");
    return 0;
}

/* Reading the character after an initial `0' */
static int lex_zero(parser_t self, int ch, elvin_error_t error)
{
    /* An `x' means that we've got a hex constant */
    if (ch == 'x' || ch == 'X')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_hex;
	return 1;
    }

    /* A decimal point means a floating point number follows */
    if (ch == '.')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_float_first;
	return 1;
    }

    /* A digit means that an octal number follows */
    if (isdigit(ch) && ch != '8' && ch != '9')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_octal;
	return 1;
    }

    /* A trailing 'L' means that we've got a 64-bit zero */
    if (ch == 'l' || ch == 'L')
    {
	if (! accept_int64(self, 0L, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Otherwise we've got a plain, simple, 32-bit zero */
    if (! accept_int32(self, 0, error))
    {
	return 0;
    }

    /* Run ch through the lexer start state */
    return lex_start(self, ch, error);
}

/* We've read one or more digits of a decimal number */
static int lex_decimal(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for additional digits */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_decimal;
	return 1;
    }

    /* Watch for a decimal point */
    if (ch == '.')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_float_first;
	return 1;
    }

    /* Null-terminate the number */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* Watch for a trailing 'L' to indicate an int64 value */
    if (ch == 'l' || ch == 'L')
    {
	if (! accept_int64_string(self, self -> token, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Otherwise accept the int32 token */
    if (! accept_int32_string(self, self -> token, error))
    {
	return 0;
    }

    return lex_start(self, ch, error);
}

/* Reading an octal integer */
static int lex_octal(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for more octal digits */
    if (isdigit(ch) && ch != '8' && ch != '9')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_octal;
	return 1;
    }

    /* Null-terminate the number */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* Watch for a trailing `L' */
    if (ch == 'l' || ch == 'L')
    {
	if (! accept_int64_string(self, self -> token, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Accept an int32 token */
    if (! accept_int32_string(self, self -> token, error))
    {
	return 0;
    }

    /* Run ch back through the start state */
    return lex_start(self, ch, error);
}

/* Reading a hex integer */
static int lex_hex(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for more digits */
    if (isxdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_hex;
	return 1;
    }

    /* Null-terminate the token */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* Watch for a trailing `L' indicating a 64-bit value */
    if (ch == 'l' || ch == 'L')
    {
	if (! accept_int64_string(self, self -> token, error))
	{
	    return 0;
	}

	self -> state = lex_start;
	return 1;
    }

    /* Otherwise accept an int32 token */
    if (! accept_int32_string(self, self -> token, error))
    {
	return 0;
    }

    /* Run ch back through the start state */
    return lex_start(self, ch, error);
}

/* Reading the first digit of the decimal portion of a floating point number */
static int lex_float_first(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a digit */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_float;
	return 1;
    }

    /* Otherwise null-terimate the token */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* And complain */
    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)self -> token);
    return 0;
}

/* We've read at least one digit of the decimal portion */
static int lex_float(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for the exponent */
    if (ch == 'e' || ch == 'E')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_exp_first;
	return 1;
    }

    /* Watch for more digits */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_float;
	return 1;
    }

    /* Otherwise we're done */
    if (! accept_float_string(self, self -> token, error))
    {
	return 0;
    }

    /* Run ch through the lexer start state */
    return lex_start(self, ch, error);
}

/* Reading the first character after the `e' in a float */
static int lex_exp_first(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for a sign */
    if (ch == '-' || ch == '+')
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_exp_sign;
	return 1;
    }

    /* Watch for digits of the exponent */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Otherwise we've got an error */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)self -> token);
    return 0;
}

/* Reading the first character after a `+' or `-' in an exponent of a float */
static int lex_exp_sign(parser_t self, int ch, elvin_error_t error)
{
    /* Must have a digit here */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Anything else is an error */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    ELVIN_ERROR_XTICKERTAPE_INVALID_TOKEN(
	error, (uchar *)self -> tag,
	self -> line_num, (uchar *)self -> token);
    return 0;
}

/* Reading the digits of an exponent */
static int lex_exp(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for more digits */
    if (isdigit(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_exp;
	return 1;
    }

    /* Null-terminate the string */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* Accept a float token */
    if (! accept_float_string(self, self -> token, error))
    {
	return 0;
    }

    /* Run ch back through the start state */
    return lex_start(self, ch, error);
}


/* Reading the characters of a double-quoted string */
static int lex_dq_string(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for special characters */
    switch (ch)
    {
	/* Unterminated string constant */
	case EOF:
	{
	    ELVIN_ERROR_XTICKERTAPE_UNTERM_STRING(
		error, (uchar *)self -> tag, self -> line_num);
	    return 0;
	}

	/* An escaped character */
	case '\\':
	{
	    self -> state = lex_dq_string_esc;
	    return 1;
	}

	/* End of the string */
	case '"':
	{
	    /* Null-terminate the string */
	    if (! append_char(self, 0, error))
	    {
		return 0;
	    }

	    /* Accept the string */
	    if (! accept_string(self, self -> token, error))
	    {
		return 0;
	    }

	    /* Go back to the start state */
	    self -> state = lex_start;
	    return 1;
	}

	/* Normal string character */
	default:
	{
	    if (! append_char(self, ch, error))
	    {
		return 0;
	    }

	    self -> state = lex_dq_string;
	    return 1;
	}
    }
}

/* Reading an escaped character in a double-quoted string */
static int lex_dq_string_esc(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	ELVIN_ERROR_XTICKERTAPE_UNTERM_STRING(
	    error, (uchar *)self -> tag, self -> line_num);
	return 0;
    }

    /* Append the character to the end of the string */
    if (! append_char(self, ch, error))
    {
	return 0;
    }

    self -> state = lex_dq_string;
    return 1;
}

/* Reading the characters of a single-quoted string */
static int lex_sq_string(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for special characters */
    switch (ch)
    {
	/* Unterminated string constant */
	case EOF:
	{
	    ELVIN_ERROR_XTICKERTAPE_UNTERM_STRING(
		error, (uchar *)self -> tag, self -> line_num);
	    return 0;
	}

	/* An escaped character */
	case '\\':
	{
	    self -> state = lex_sq_string_esc;
	    return 1;
	}

	/* End of the string */
	case '\'':
	{
	    /* Null-terminate the string */
	    if (! append_char(self, 0, error))
	    {
		return 0;
	    }

	    /* Accept the string */
	    if (! accept_string(self, self -> token, error))
	    {
		return 0;
	    }

	    /* Go back to the start state */
	    self -> state = lex_start;
	    return 1;
	}

	/* Normal string character */
	default:
	{
	    if (! append_char(self, ch, error))
	    {
		return 0;
	    }

	    self -> state = lex_sq_string;
	    return 1;
	}
    }
}

/* Reading an escaped character in a single-quoted string */
static int lex_sq_string_esc(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	ELVIN_ERROR_XTICKERTAPE_UNTERM_STRING(
	    error, (uchar *)self -> tag, self -> line_num);
	return 0;
    }

    /* Append the character to the end of the string */
    if (! append_char(self, ch, error))
    {
	return 0;
    }

    self -> state = lex_sq_string;
    return 1;
}

/* Reading an identifier (2nd character on) */
static int lex_id(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for an escaped id char */
    if (ch == '\\')
    {
	self -> state = lex_id_esc;
	return 1;
    }

    /* Watch for additional id characters */
    if (is_id_char(ch))
    {
	if (! append_char(self, ch, error))
	{
	    return 0;
	}

	self -> state = lex_id;
	return 1;
    }

    /* This is the end of the id */
    if (! append_char(self, 0, error))
    {
	return 0;
    }

    /* Accept it */
    if (! accept_id(self, self -> token, error))
    {
	return 0;
    }

    /* Run ch through the start state */
    return lex_start(self, ch, error);
}

static int lex_id_esc(parser_t self, int ch, elvin_error_t error)
{
    /* Watch for EOF */
    if (ch == EOF)
    {
	/* Add the backslash to the end of the token */
	if (! append_char(self, '\\', error))
	{
	    return 0;
	}

	/* Null-terminate the token */
	if (! append_char(self, 0, error))
	{
	    return 0;
	}

	ELVIN_ERROR_XTICKERTAPE_UNTERM_ID(
	    error, (uchar *)self -> tag, self -> line_num);
	return 0;
    }

    /* Append the character to the id */
    if (! append_char(self, ch, error))
    {
	return 0;
    }

    self -> state = lex_id;
    return 1;
}



/* Allocate space for a parser and initialize its contents */
parser_t parser_alloc(
    parser_callback_t callback,
    void *rock,
    elvin_error_t error)
{
    parser_t self;

    /* Allocate some memory for the new parser */
    if (! (self = (parser_t)ELVIN_MALLOC(sizeof(struct parser), error)))
    {
	return NULL;
    }

    /* Initialize all of the fields to sane values */
    memset(self, 0, sizeof(struct parser));

    /* Allocate room for the state stack */
    if (! (self -> state_stack = (int *)ELVIN_CALLOC(INITIAL_STACK_DEPTH, sizeof(int), error)))
    {
	parser_free(self, error);
	return NULL;
    }

    /* Allocate room for the value stack */
    if (! (self -> value_stack = (ast_t *)ELVIN_CALLOC(
	INITIAL_STACK_DEPTH,
	sizeof(ast_t),
	error)))
    {
	parser_free(self, error);
	return NULL;
    }

    /* Allocate room for the token buffer */
    if (! (self -> token = (char *)ELVIN_MALLOC(INITIAL_TOKEN_BUFFER_SIZE, error)))
    {
	parser_free(self, error);
	return NULL;
    }

    /* Initialize the rest of the values */
    self -> callback = callback;
    self -> rock = rock;

    self -> state_end = self -> state_stack + INITIAL_STACK_DEPTH;
    self -> state_top = self -> state_stack;
    *(self -> state_top) = 0;

    self -> value_top = self -> value_stack;
    self -> token_end = self -> token + INITIAL_TOKEN_BUFFER_SIZE;
    self -> line_num = 1;
    self -> state = lex_start;
    return self;
}

/* Frees the resources consumed by the parser */
int parser_free(parser_t self, elvin_error_t error)
{
    /* Free the state stack */
    if (self -> state_stack)
    {
	if (! ELVIN_FREE(self -> state_stack, error))
	{
	    return 0;
	}
    }

    /* Free the value stack */
    if (self -> value_stack)
    {
	if (! ELVIN_FREE(self -> value_stack, error))
	{
	    return 0;
	}
    }

    /* Free the token buffer */
    if (self -> token)
    {
	if (! ELVIN_FREE(self -> token, error))
	{
	    return 0;
	}
    }

    /* Free the parser itself */
    return ELVIN_FREE(self, error);
}

/* Resets the parser to accept new input */
int parser_reset(parser_t self, elvin_error_t error)
{
    ast_t *pointer;

    /* Free everything on the stack */
    for (pointer = self -> value_stack; pointer < self -> value_top; pointer++)
    {
	if (*pointer != NULL)
	{
	    if (! ast_free(*pointer, error))
	    {
		return 0;
	    }
	}
    }

    /* Reset the parser */
    self -> line_num = 1;
    self -> state_top = self -> state_stack;
    self -> value_top = self -> value_stack;
    self -> state = lex_start;
    return 1;
}

/* Parses a single character, counting LFs */
static int parse_char(parser_t self, int ch, elvin_error_t error)
{
    if (! self -> state(self, ch, error))
    {
	parser_reset(self, NULL);
	return 0;
    }

    if (ch == '\n')
    {
	self -> line_num++;
    }

    return 1;
}

/* Runs the characters in the buffer through the parser */
int parser_parse(parser_t self, char *buffer, size_t length, elvin_error_t error)
{
    char *pointer;

    /* Length of 0 is an EOF */
    if (length == 0)
    {
	return parse_char(self, EOF, error);
    }

    for (pointer = buffer; pointer < buffer + length; pointer++)
    {
	if (! parse_char(self, *pointer, error))
	{
	    return 0;
	}
    }

    return 1;
}

/* Parses the input from fd */
int parser_parse_file(parser_t self, int fd, char *tag, elvin_error_t error)
{
    self -> tag = tag;

    while (1)
    {
	char buffer[BUFFER_SIZE];
	ssize_t length;

	/* Read from the file descriptor */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    ELVIN_ERROR_UNIX_READ_FAILED(error, errno);
	    self -> tag = NULL;
	    return 0;
	}

	/* Parse what we've read so far */
	if (! parser_parse(self, buffer, length, error))
	{
	    self -> tag = NULL;
	    return 0;
	}

	/* Watch for EOF */
	if (length == 0)
	{
	    self -> tag = NULL;
	    return 1;
	}
    }
}
