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
static const char cvs_USENET_PARSER_H[] = "$Id: usenet_parser.h,v 1.1 1999/10/03 06:11:27 phelps Exp $";
#endif /* lint */

/* The usenet parser data type */
typedef struct usenet_parser *usenet_parser_t;

/* Allocates and initializes a new usenet subscription parser */
usenet_parser_t usenet_parser_alloc(char *tag);

/* Frees the resources consumed by the receiver */
void usenet_parser_free(usenet_parser_t self);

/* Parses the given buffer, calling callbacks for each usenet
 * subscription expression that is successfully read.  A zero-length
 * buffer is interpreted as an end-of-input marker */
int usenet_parser_parse(usenet_parser_t self, char *buffer, ssize_t length);

#endif /* USENET_PARSER_H */
