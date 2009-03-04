/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h> /* EOF, fprintf  */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* calloc, free, malloc */
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h> /* isspace */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* strcmp, strdup */
#endif
#include "replace.h"
#include "usenet_parser.h"

#define INITIAL_TOKEN_SIZE 64
#define INITIAL_EXPR_MAX 8

/* The type of a lexer state */
typedef int (*lexer_state_t)(usenet_parser_t self, int ch);

#define FIELD_ERROR_MSG "unknown field \"%s\""
#define OP_ERROR_MSG "unknown operator \"%s\""


#define T_BODY "BODY"
#define T_FROM "from"
#define T_EMAIL "email"
#define T_SUBJECT "subject"
#define T_KEYWORDS "keywords"
#define T_XPOSTS "x-posts"

#define T_MATCHES "matches"
#define T_NOT "not"
#define T_EQ "="
#define T_NEQ "!="
#define T_LT "<"
#define T_GT ">"
#define T_LE "<="
#define T_GE ">="

/* The structure of a usenet_parser */
struct usenet_parser {
    /* The callback for when we've parsed a line */
    usenet_parser_callback_t callback;

    /* The user data for the callback */
    void *rock;

    /* The tag for error messages */
    char *tag;

    /* The buffer in which the token is assembled */
    char *token;

    /* The next character in the token buffer */
    char *token_pointer;

    /* A reference point within the token */
    char *token_mark;

    /* The end of the token buffer */
    char *token_end;

    /* The current lexical state */
    lexer_state_t state;

    /* The current line-number within the input */
    int line_num;

    /* This is set if the group pattern is negated */
    int has_not;

    /* The name of the usenet group whose entry we're currently building */
    char *group;

    /* The field name of the current expression */
    field_name_t field;

    /* The operator of the current expression */
    op_name_t operator;

    /* The usenet_expr patterns of the current group */
    struct usenet_expr *expressions;

    /* The next expression in the array */
    struct usenet_expr *expr_pointer;

    /* The end of the expression array */
    struct usenet_expr *expr_end;
};


/* Lexer state declarations */
static int
lex_start(usenet_parser_t self, int ch);
static int
lex_start_esc(usenet_parser_t self, int ch);
static int
lex_comment(usenet_parser_t self, int ch);
static int
lex_group(usenet_parser_t self, int ch);
static int
lex_group_esc(usenet_parser_t self, int ch);
static int
lex_group_ws(usenet_parser_t self, int ch);
static int
lex_field_start(usenet_parser_t self, int ch);
static int
lex_field(usenet_parser_t self, int ch);
static int
lex_field_esc(usenet_parser_t self, int ch);
static int
lex_op_start(usenet_parser_t self, int ch);
static int
lex_op(usenet_parser_t self, int ch);
static int
lex_op_esc(usenet_parser_t self, int ch);
static int
lex_pattern_start(usenet_parser_t self, int ch);
static int
lex_pattern(usenet_parser_t self, int ch);
static int
lex_pattern_esc(usenet_parser_t self, int ch);
static int
lex_pattern_ws(usenet_parser_t self, int ch);


/* Prints a consistent error message */
static void
parse_error(usenet_parser_t self, char *message)
{
    fprintf(stderr, "%s: parse error line %d: %s\n",
            self->tag, self->line_num, message);
}

/* This is called when a complete group entry has been read */
static int
accept_group(usenet_parser_t self, int has_not, char *group)
{
    int result;
    struct usenet_expr *pointer;

    /* Call the callback */
    result = (self->callback)(self->rock, has_not, group,
                              self->expressions,
                              self->expr_pointer - self->expressions);

    /* Clean up */
    for (pointer = self->expressions; pointer < self->expr_pointer;
         pointer++) {
        free(pointer->pattern);
    }

    self->expr_pointer = self->expressions;
    return result;
}

/* This is called when a complete expression has been read */
static int
accept_expression(usenet_parser_t self,
                  field_name_t field,
                  op_name_t operator,
                  char *pattern)
{
    /* Make sure there's room in the expressions table */
    if (!(self->expr_pointer < self->expr_end)) {
        /* Grow the expressions array */
        struct usenet_expr *new_array;
        size_t length = (self->expr_end - self->expressions) * 2;

        /* Try to allocate more memory */
        new_array = realloc(self->expressions,
                            sizeof(struct usenet_expr) * length);
        if (new_array == NULL) {
            return -1;
        }

        /* Update the other pointers */
        self->expr_pointer = new_array + (self->expr_pointer -
                                          self->expressions);
        self->expressions = new_array;
        self->expr_end = self->expressions + length;
    }

    /* Plug in the values */
    self->expr_pointer->field = field;
    self->expr_pointer->operator = operator;
    self->expr_pointer->pattern = strdup(pattern);
    self->expr_pointer++;
    return 0;
}

/* Appends a character to the end of the token */
static int
append_char(usenet_parser_t self, int ch)
{
    /* Grow the token buffer if necessary */
    if (!(self->token_pointer < self->token_end)) {
        char *new_token;
        size_t length = (self->token_end - self->token) * 2;

        /* Try to allocate more memory */
        new_token = realloc(self->token, length);
        if (new_token == NULL) {
            return -1;
        }

        /* Update the other pointers */
        self->token_pointer = new_token + (self->token_pointer - self->token);
        self->token_mark = new_token + (self->token_mark - self->token);
        self->token = new_token;
        self->token_end = self->token + length;
    }

    *(self->token_pointer++) = ch;
    return 0;
}

/* Answers the field_name_t corresponding to the given string (or
 * F_NONE if none match) */
static field_name_t
translate_field(char *field)
{
    switch (*field) {
    case 'B':
        /* BODY */
        if (strcmp(field, T_BODY) == 0) {
            return F_BODY;
        }

        break;

    case 'e':
        /* email */
        if (strcmp(field, T_EMAIL) == 0) {
            return F_EMAIL;
        }

        break;

    case 'f':
        /* from */
        if (strcmp(field, T_FROM) == 0) {
            return F_FROM;
        }

        break;

    case 'k':
        /* keywords */
        if (strcmp(field, T_KEYWORDS) == 0) {
            return F_KEYWORDS;
        }

        break;

    case 's':
        /* subject */
        if (strcmp(field, T_SUBJECT) == 0) {
            return F_SUBJECT;
        }

        break;

    case 'x':
        /* x-posts */
        if (strcmp(field, T_XPOSTS) == 0) {
            return F_XPOSTS;
        }

        break;
    }

    /* Not a valid field name */
    return F_NONE;
}

/* Answers the op_name_t corresponding to the given string (or O_NONE if none
  match) */
static op_name_t
translate_op(char *operator)
{
    switch (*operator) {
    case '!':
        /* != */
        if (strcmp(operator, T_NEQ) == 0) {
            return O_NEQ;
        }

        break;

    case '<':
        /* < or <= */
        if (strcmp(operator, T_LT) == 0) {
            return O_LT;
        }

        if (strcmp(operator, T_LE) == 0) {
            return O_LE;
        }

    case '=':
        /* = */
        if (strcmp(operator, T_EQ) == 0) {
            return O_EQ;
        }

        break;

    case '>':
        /* > or >= */
        if (strcmp(operator, T_GT) == 0) {
            return O_GT;
        }

        if (strcmp(operator, T_GE) == 0) {
            return O_GE;
        }

        break;

    case 'm':
        /* matches */
        if (strcmp(operator, T_MATCHES) == 0) {
            return O_MATCHES;
        }

        break;

    case 'n':
        /* not */
        if (strcmp(operator, T_NOT) == 0) {
            return O_NOT;
        }

        break;
    }

    /* Not a valid operator name */
    return O_NONE;
}

/* Awaiting the first character of a usenet subscription */
static int
lex_start(usenet_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF) {
        self->state = lex_start;
        return 0;
    }

    /* Watch for comments */
    if (ch == '#') {
        self->state = lex_comment;
        return 0;
    }

    /* Backslashes escape stuff */
    if (ch == '\\') {
        self->state = lex_start_esc;
        return 0;
    }

    /* Watch for blank lines */
    if (ch == '\n') {
        self->state = lex_start;
        self->line_num++;
        return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch)) {
        self->state = lex_start;
        return 0;
    }

    /* Anything else is part of the group pattern */
    self->has_not = 0;
    self->token_pointer = self->token;
    self->state = lex_group;
    return append_char(self, ch);
}

/* Reading the character after a '\' at the start of a usenet subscription */
static int
lex_start_esc(usenet_parser_t self, int ch)
{
    /* Watch for EOF */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Anything else is part of the group pattern */
    self->has_not = 0;
    self->token_pointer = self->token;
    self->state = lex_group;
    return append_char(self, ch);
}

/* Reading a comment (to end-of-line) */
static int
lex_comment(usenet_parser_t self, int ch)
{
    /* Watch for end-of-file */
    if (ch == EOF) {
        return lex_start(self, ch);
    }

    /* Watch for end of line */
    if (ch == '\n') {
        self->state = lex_start;
        self->line_num++;
        return 0;
    }

    /* Ignore everything else */
    return 0;
}

/* Reading the group (or its preceding `not') */
static int
lex_group(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline */
    if (ch == EOF || ch == '\n') {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* This is a nice, short group entry */
        if (accept_group(self, self->has_not, self->token) < 0) {
            return -1;
        }

        return lex_start(self, ch);
    }

    /* Watch for other whitespace */
    if (isspace(ch)) {
        self->state = lex_group_ws;

        /* Null-terminate the token */
        return append_char(self, '\0');
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_group_esc;
        return 0;
    }

    /* Watch for the separator character */
    if (ch == '/') {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* Record the group name for later use */
        self->group = strdup(self->token);
        self->token_pointer = self->token;
        self->state = lex_field_start;
        return 0;
    }

    /* Anything else is part of the group pattern */
    return append_char(self, ch);
}

/* Reading the character after a '//' in the group */
static int
lex_group_esc(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Anything else is part of the group pattern */
    self->state = lex_group;
    return append_char(self, ch);
}

/* We've got some whitespace in the group pattern.  It could mean that
 * the "not" keyword was used or it could mean that we've just got
 * some space between the pattern and the separator character */
static int
lex_group_ws(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline */
    if (ch == EOF || ch == '\n') {
        /* This is a nice, short group entry */
        if (accept_group(self, self->has_not, self->token) < 0) {
            return -1;
        }

        return lex_start(self, ch);
    }

    /* Skip additional whitespace */
    if (isspace(ch)) {
        return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_group_esc;
        return 0;
    }

    /* Watch for the separator character */
    if (ch == '/') {
        /* Record the group name for later */
        self->group = strdup(self->token);
        self->token_pointer = self->token;
        self->state = lex_field_start;
        return 0;
    }

    /* Anything else wants to be part of the pattern.  Make sure that
     * the token we've already read is "not" and that we're not doing
     * the double-negative thing */
    if (strcmp(self->token, T_NOT) != 0) {
        parse_error(self, "expected '/' or newline");
        return -1;
    }

    /* Watch for double-negative */
    if (self->has_not) {
        parse_error(self, "double negative not permitted");
        return -1;
    }

    /* This is a negated group pattern */
    self->has_not = 1;
    self->token_pointer = self->token;

    /* Anything else is part of the group pattern */
    self->state = lex_group;
    return append_char(self, ch);
}

/* We've just read the '/' after the group pattern */
static int
lex_field_start(usenet_parser_t self, int ch)
{
    /* Skip other whitespace */
    if (isspace(ch)) {
        return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_field_esc;
        return 0;
    }

    /* Anything else is the start of the field name */
    self->state = lex_field;
    return lex_field(self, ch);
}

/* We're reading the field portion of an expression */
static int
lex_field(usenet_parser_t self, int ch)
{
    /* Watch for EOF or LF in the middle of the field */
    if ((ch == EOF) || (ch == '\n')) {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_field_esc;
        return 0;
    }

    /* Watch for a stray separator */
    if (ch == '/') {
        parse_error(self, "incomplete expression");
        return -1;
    }

    /* Watch for whitespace */
    if (isspace(ch)) {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* Look up the field */
        self->field = translate_field(self->token);
        if (self->field == F_NONE) {
            char *buffer =
                malloc(strlen(FIELD_ERROR_MSG) + strlen(self->token) - 1);

            if (buffer != NULL) {
                sprintf(buffer, FIELD_ERROR_MSG, self->token);
                parse_error(self, buffer);
                free(buffer);
            }

            return -1;
        }

        self->token_pointer = self->token;
        self->state = lex_op_start;
        return 0;
    }

    /* Anything else is part of the field name */
    return append_char(self, ch);
}

/* Reading an escaped character in a field */
static int
lex_field_esc(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline in the middle of the field */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Anything else is part of the field name */
    self->state = lex_field;
    return append_char(self, ch);
}

/* Reading the whitespace before an operator */
static int
lex_op_start(usenet_parser_t self, int ch)
{
    /* Skip additional whitespace */
    if (isspace(ch)) {
        return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_op_esc;
        return 0;
    }

    /* Anything else is part of the operator name */
    self->state = lex_op;
    return lex_op(self, ch);
}

/* Reading the operator name */
static int
lex_op(usenet_parser_t self, int ch)
{
    /* Watch for EOF or linefeed in the middle of the expression */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_op_esc;
        return 0;
    }

    /* Watch for a stray separator */
    if (ch == '/') {
        parse_error(self, "incomplete expression");
        return -1;
    }

    /* Watch for more whitespace */
    if (isspace(ch)) {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* Look up the operator */
        self->operator = translate_op(self->token);
        if (self->operator == O_NONE) {
            char *buffer = malloc(strlen(OP_ERROR_MSG) + strlen(
                                      self->token) - 1);

            if (buffer != NULL) {
                sprintf(buffer, OP_ERROR_MSG, self->token);
                parse_error(self, buffer);
                free(buffer);
            }

            return -1;
        }

        /* Make sure there's agreement between the field and the operator */
        if (self->field == F_XPOSTS) {
            if ((self->operator == O_MATCHES) || (self->operator == O_NOT)) {
                parse_error(self, "illegal field/operator combination");
                return -1;
            }
        } else {
            if ((self->operator == O_LT) || (self->operator == O_GT) ||
                (self->operator == O_LE) || (self->operator == O_GE)) {
                parse_error(self, "illegal field/operator combination");
                return -1;
            }
        }

        /* Get ready to read the pattern */
        self->token_pointer = self->token;
        self->state = lex_pattern_start;
        return 0;
    }

    /* Anything else is part of the operator */
    return append_char(self, ch);
}

/* Reading an escaped character in the operator name */
static int
lex_op_esc(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Anything else is part of the operator name */
    self->state = lex_op;
    return append_char(self, ch);
}

/* Reading whitespace before the pattern */
static int
lex_pattern_start(usenet_parser_t self, int ch)
{
    /* Skip additional whitespace */
    if (isspace(ch)) {
        return 0;
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_pattern_esc;
        return 0;
    }

    /* Anything else is part of the operator name */
    self->state = lex_pattern;
    return lex_pattern(self, ch);
}

/* Reading the pattern itself */
static int
lex_pattern(usenet_parser_t self, int ch)
{
    /* Watch for EOF or linefeed */
    if ((ch == EOF) || (ch == '\n')) {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* Do something interesting with the expression */
        if (accept_expression(self, self->field, self->operator,
                              self->token) < 0) {
            return -1;
        }

        /* This is also the end of the group entry */
        if (accept_group(self, self->has_not, self->group) < 0) {
            return -1;
        }

        free(self->group);

        /* Let the start state deal with the EOF or linefeed */
        return lex_start(self, ch);
    }

    /* Watch for the escape character */
    if (ch == '\\') {
        self->state = lex_pattern_esc;
        return 0;
    }

    /* Watch for other whitespace */
    if (isspace(ch)) {
        /* Record the position of the whitespace for later trimming */
        self->token_mark = self->token_pointer;
        self->state = lex_pattern_ws;
        return append_char(self, ch);
    }

    /* Watch for a separator */
    if (ch == '/') {
        /* Null-terminate the token */
        if (append_char(self, '\0') < 0) {
            return -1;
        }

        /* Do something interesting with the expression */
        self->token_pointer = self->token;
        self->state = lex_field_start;
        return accept_expression(self, self->field, self->operator,
                                 self->token);
    }

    /* Anything else is part of the pattern */
    return append_char(self, ch);
}

/* Reading an escaped character in a pattern */
static int
lex_pattern_esc(usenet_parser_t self, int ch)
{
    /* Watch for EOF or newline */
    if (ch == EOF || ch == '\n') {
        parse_error(self, "unexpected end of line");
        return -1;
    }

    /* Anything else is part of the pattern */
    self->state = lex_pattern;
    return append_char(self, ch);
}

/* Reading trailing whitespace after a pattern */
static int
lex_pattern_ws(usenet_parser_t self, int ch)
{
    /* Watch for EOF and newline */
    if ((ch == EOF) || (ch == '\n')) {
        /* Trim off the trailing whitespace */
        *self->token_mark = '\0';

        /* Do something interesting with the expression */
        if (accept_expression(self, self->field, self->operator,
                              self->token) < 0) {
            return -1;
        }

        /* This is also the end of the group entry */
        if (accept_group(self, self->has_not, self->group) < 0) {
            return -1;
        }

        free(self->group);

        /* Let the start state deal with the EOF or newline */
        return lex_start(self, ch);
    }

    /* Skip any other whitespace */
    if (isspace(ch)) {
        return append_char(self, ch);
    }

    /* Watch for an escape character */
    if (ch == '\\') {
        self->state = lex_pattern_esc;
        return 0;
    }

    /* Watch for a separator */
    if (ch == '/') {
        /* Trim off the trailing whitespace */
        *self->token_mark = '\0';

        /* Do something interesting with the expression */
        self->token_pointer = self->token;
        self->state = lex_field_start;
        return accept_expression(self, self->field, self->operator,
                                 self->token);
    }

    /* Anything else is more of the pattern */
    self->state = lex_pattern;
    return append_char(self, ch);
}

/* Allocates and initializes a new usenet file parser */
usenet_parser_t
usenet_parser_alloc(usenet_parser_callback_t callback, void *rock, char *tag)
{
    usenet_parser_t self;

    /* Allocate memory for the new usenet_parser */
    self = malloc(sizeof(struct usenet_parser));
    if (self == NULL) {
        return NULL;
    }

    /* Copy the tag */
    self->tag = strdup(tag);
    if (self->tag == NULL) {
        free(self);
        return NULL;
    }

    /* Allocate room for the token buffer */
    self->token = malloc(INITIAL_TOKEN_SIZE);
    if (self->token == NULL) {
        free(self->tag);
        free(self);
        return NULL;
    }

    /* Allocate room for the expression array */
    self->expressions = calloc(INITIAL_EXPR_MAX, sizeof(struct usenet_expr));
    if (self->expressions == NULL) {
        free(self->token);
        free(self->tag);
        free(self);
        return NULL;
    }

    /* Initialize everything else to a sane value */
    self->callback = callback;
    self->rock = rock;
    self->token_pointer = self->token;
    self->token_mark = self->token;
    self->token_end = self->token + INITIAL_TOKEN_SIZE;
    self->state = lex_start;
    self->line_num = 1;
    self->has_not = 0;
    self->group = NULL;
    self->field = F_NONE;
    self->operator = O_NONE;
    self->expr_pointer = self->expressions;
    self->expr_end = self->expressions + INITIAL_EXPR_MAX;
    return self;
}

/* Frees the resources consumed by the receiver */
void
usenet_parser_free(usenet_parser_t self)
{
    free(self->tag);
    free(self->token);
    free(self->expressions);
    free(self);
}

/* Parses the given buffer, calling callbacks for each usenet
 * subscription expression that is successfully read.  A zero-length
 * buffer is interpreted as an end-of-input marker */
int
usenet_parser_parse(usenet_parser_t self, char *buffer, size_t length)
{
    char *end = buffer + length;
    char *pointer;

    /* Length of 0 indicates EOF */
    if (length == 0) {
        return self->state(self, EOF);
    }

    /* Parse the buffer */
    for (pointer = buffer; pointer < end; pointer++) {
        if (self->state(self, *(unsigned char *)pointer) < 0) {
            return -1;
        }
    }

    return 0;
}

/**********************************************************************/
