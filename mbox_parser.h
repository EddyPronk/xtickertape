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

#ifndef MBOX_PARSER_H
#define MBOX_PARSER_H

#ifndef lint
static const char cvs_MBOX_PARSER_H[] = "$Id: mbox_parser.h,v 1.2 1999/09/12 13:57:21 phelps Exp $";
#endif /* lint */

#include <stdio.h>

typedef struct mbox_parser *mbox_parser_t;

/*
 * Allocates a new RFC-822 mailbox parser.  This takes a strings as
 * input, which it parses as a mailbox (commented e-mail address) into 
 * e-mail address and user name portions
 *
 * return values:
 *     success: a valid mbox_parser_t
 *     failure: NULL
 */
mbox_parser_t mbox_parser_alloc();

/* Frees the resources consumed by the receiver */
void mbox_parser_free(mbox_parser_t self);

/* Prints out debugging information about the receiver */
void mbox_parser_debug(mbox_parser_t self, FILE *out);

/* Parses `mailbox' as an RFC 822 mailbox, separating out the name and
 * e-mail address portions which can subsequently be accessed with the 
 * mbox_parser_get_name() and mbox_parser_get_email() functions.
 *
 * return values:
 *     success: 0
 *     failure: -1
 */
int mbox_parser_parse(mbox_parser_t self, char *mailbox);

/*
 * Answers a pointer to the e-mail address (only valid after
 * a successful call to mbox_parser_parse).
 */
char *mbox_parser_get_email(mbox_parser_t self);

/*
 * Answers a pointer to the name (only valid after a successful call
 * to mbox_parser_parse).  This will return NULL if no name was present in
 * the most recently parsed mailbox.
 */
char *mbox_parser_get_name(mbox_parser_t self);
   
#endif /* MBOX_PARSER_H */
