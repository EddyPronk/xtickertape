/***************************************************************

  Copyright (C) 1999-2002, 2004 by Mantara Software
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
             Manages tickertape group subscriptions and can parse the
	     groups file

****************************************************************/

#ifndef GROUPS_PARSER_H
#define GROUPS_PARSER_H

#ifndef lint
static const char cvs_GROUPS_PARSER_H[] = "$Id: groups_parser.h,v 1.10 2004/08/02 22:24:16 phelps Exp $";
#endif /* lint */

/* The groups parser data type */
typedef struct groups_parser *groups_parser_t;

/* The groups_parser callback type */
typedef int (*groups_parser_callback_t)(
    void *rock, char *name,
    int in_menu, int has_nazi,
    int min_time, int max_time,
    char **key_names,
    int key_name_count);

/* Allocates and initializes a new groups file parser */
groups_parser_t groups_parser_alloc(
    groups_parser_callback_t callback,
    void *rock,
    char *tag);

/* Frees the resources consumed by the receiver */
void groups_parser_free(groups_parser_t self);

/* Parses the given buffer, calling callbacks for each subscription
 * expression that is successfully read.  A zero-length buffer is
 * interpreted as an end-of-input marker */
int groups_parser_parse(groups_parser_t self, char *buffer, size_t length);

#endif /* GROUPS_PARSER_H */
