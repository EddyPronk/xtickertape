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

#ifndef PARSER_H
#define PARSER_H

#ifndef lint
static const char cvs_PARSER_H[] = "$Id: parser.h,v 1.6 2000/11/09 01:28:06 phelps Exp $";
#endif /* lint */

typedef struct parser *parser_t;


/* The format of a parser callback function. */
typedef int (*parser_callback_t)(void *rock, parser_t parser, sexp_t sexp, elvin_error_t error);


/*
 * Allocates and initializes a new parser_t for esh's lisp-like
 * language.  This constructs a thread-safe parser which may be used
 * to convert input characters into lisp atoms and lists.  Whenever
 * the parser completes reading an s-expression it calls the callback
 * function.
 *
 * return values:
 *     success: a valid parser_t
 *     failure: NULL
 */
parser_t parser_alloc(parser_callback_t callback, void *rock, elvin_error_t error);

/* Frees the resources consumed by the parser */
int parser_free(parser_t self, elvin_error_t error);


/* The parser will read characters from `buffer' and use them generate 
 * lisp s-expressions.  Each time an s-expression is completed, the
 * parser's callback is called with that s-expression.  If an error is 
 * encountered, the buffer pointer will point to the character where
 * the error was first noticed.  The parser does *not* reset its state 
 * between calls to parser_read_buffer(), so it is possible to
 * construct an s-expression which is much longer than the buffer size 
 * and it is also not necessary to ensure that a complete s-expression 
 * is in the buffer when this function is called.  The parser state
 * *is* reset whenever an error is encountered.  A buffer of zero
 * length is interpreted as the end of input.
 *
 * return values:
 *     success: 0
 *     failure: -1 (and buffer will point to first error character)
 */
int parser_read_buffer(
    parser_t self, 
    const char *buffer,
    ssize_t length,
    elvin_error_t error);

#endif /* PARSER_H */
