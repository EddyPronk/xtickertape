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

#ifndef USENET_PARSER_H
#define USENET_PARSER_H

#ifndef lint
static const char cvs_USENET_PARSER_H[] = "$Id: usenet_parser.h,v 1.10 2009/03/09 05:26:27 phelps Exp $";
#endif /* lint */

/* The usenet parser data type */
typedef struct usenet_parser *usenet_parser_t;

/* The field type */
enum field_name
{
    F_NONE,
    F_BODY,
    F_FROM,
    F_EMAIL,
    F_SUBJECT,
    F_KEYWORDS,
    F_XPOSTS
};

typedef enum field_name field_name_t;


/* The operator type */
enum op_name
{
    O_NONE,
    O_MATCHES,
    O_NOT,
    O_EQ,
    O_NEQ,
    O_LT,
    O_GT,
    O_LE,
    O_GE
};

typedef enum op_name op_name_t;


/* The structure of an expression used in the callback */
struct usenet_expr
{
    /* The field */
    field_name_t field;

    /* The operator */
    op_name_t operator;

    /* The pattern */
    char *pattern;
};


/* The usenet parser callback type */
typedef int (*usenet_parser_callback_t)(
    void *rock, int has_not, char *pattern,
    struct usenet_expr *expressions, size_t expr_count);

/* Allocates and initializes a new usenet subscription parser */
usenet_parser_t usenet_parser_alloc(usenet_parser_callback_t callback, void *rock, char *tag);

/* Frees the resources consumed by the receiver */
void usenet_parser_free(usenet_parser_t self);

/* Parses the given buffer, calling callbacks for each usenet
 * subscription expression that is successfully read.  A zero-length
 * buffer is interpreted as an end-of-input marker */
int usenet_parser_parse(usenet_parser_t self, char *buffer, size_t length);

#endif /* USENET_PARSER_H */
