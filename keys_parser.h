/***************************************************************

  Copyright (C) 2002-2004 by Mantara Software
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

   Description: 
             Parses the Tickertape keys file

****************************************************************/

#ifndef KEYS_PARSER_H
#define KEYS_PARSER_H

#ifndef lint
static const char cvs_KEYS_PARSER_H[] = "$Id: keys_parser.h,v 1.3 2004/08/02 22:24:16 phelps Exp $";
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
