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
static const char cvs_PARSER_H[] = "$Id: parser.h,v 1.2 2000/07/10 13:44:51 phelps Exp $";
#endif /* lint */

/* The parser data type */
typedef struct parser *parser_t;

/* The parser's callback type */
typedef int (*parser_callback_t)(
    void *rock,
    uint32_t count,
    subscription_t *subscription,
    elvin_error_t error);

/* Allocate space for a parser and initialize its contents */
parser_t parser_alloc(
    parser_callback_t callback,
    void *rock,
    elvin_error_t error);

/* Frees the resources consumed by the parser */
int parser_free(parser_t self, elvin_error_t error);

/* Runs the characters in the buffer through the parser */
int parser_parse(parser_t self, char *buffer, size_t length, elvin_error_t error);

#endif /* PARSER_H */
