/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2002.
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
             Parses the Tickertape keys file

****************************************************************/

#ifndef KEYS_PARSER_H
#define KEYS_PARSER_H

#ifndef lint
static const char cvs_KEYS_PARSER_H[] = "$Id: keys_parser.h,v 1.2 2003/01/11 13:25:37 phelps Exp $";
#endif /* lint */

#include <elvin/elvin.h>

/* The keys parser data type */
typedef struct keys_parser *keys_parser_t;

/* The keys_parser callback type */
typedef int (*keys_parser_callback_t)(
    void *rock,
    char *name,
    char *key_data,
    int key_length,
    int is_private);

/* Allocates and initializes a new keys file parser */
keys_parser_t keys_parser_alloc(
    char *tickerdir,
    keys_parser_callback_t callback,
    void *rock,
    char *tag);

/* Frees the resources consumed by the receiver */
void keys_parser_free(keys_parser_t self);

/* Parses the given buffer, calling callbacks for each subscription
 * expression that is successfully read.  A zero-length buffer is
 * interpreted as an end-of-input marker */
int keys_parser_parse(keys_parser_t self, char *buffer, size_t length);

#endif /* KEYS_PARSER_H */
