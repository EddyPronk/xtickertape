/***************************************************************

  Copyright (C) 1999, 2001, 2004 by Mantara Software
  (ABN 17 105 665 594). All Rights Reserved.

  This software is the property of the Mantara Software. All use
  of this software is permitted only under the terms of a
  license agreement with Mantara Software. If you do not have
  such a license, then you have no rights to use this software
  in any manner and should contact Mantara at the address below
  to determine an appropriate licensing arrangement.
  
     Mantara Software
     PO Box 1820
     Toowong QLD 4066
     Australia
     Tel: +61 7 3876 8844
     Fax: +61 7 3876 8843
     Email: licensing@mantara.com
  
     Web: http://www.mantara.com
  
  This software is being provided "AS IS" without warranty of
  any kind. In no event shall Mantara Software be liable for
  damage of any kind arising out of or in connection with the
  use or performance of this software.

****************************************************************/

#ifndef USENET_PARSER_H
#define USENET_PARSER_H

#ifndef lint
static const char cvs_USENET_PARSER_H[] = "$Id: usenet_parser.h,v 1.7 2004/08/02 22:24:17 phelps Exp $";
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
