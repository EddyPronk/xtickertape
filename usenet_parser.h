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

   Description: 
             Manages tickertape group subscriptions and can parse the
	     groups file

****************************************************************/

#ifndef USENET_PARSER_H
#define USENET_PARSER_H

#ifndef lint
static const char cvs_USENET_PARSER_H[] = "$Id: usenet_parser.h,v 1.5 1999/10/04 11:54:05 phelps Exp $";
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
