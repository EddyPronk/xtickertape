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

#ifndef GROUPS_PARSER_H
#define GROUPS_PARSER_H

#ifndef lint
static const char cvs_GROUPS_PARSER_H[] = "$Id: groups_parser.h,v 1.1 1999/10/02 13:28:40 phelps Exp $";
#endif /* lint */

/* The groups parser data type */
typedef struct groups_parser *groups_parser_t;

#include "Subscription.h"

/* Allocates and initializes a new groups_parser_t */
groups_parser_t groups_parser_alloc();

/* Frees the resources consumed by the receiver */
void groups_parser_free(groups_parser_t self);

/* Parses the given file into an array of subscriptions */
int groups_parser_parse(groups_parser_t self, char *tag, char *buffer, ssize_t length);

/* Answers the subscriptions that we've parsed (or NULL if an error occurred */
Subscription *groups_parser_get_subscriptions(groups_parser_t self);

#endif /* GROUPS_PARSER_H */
