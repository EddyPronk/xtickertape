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
             Manages tickertape group subscriptions

****************************************************************/

#ifndef GROUP_SUB_H
#define GROUP_SUB_H

#ifndef lint
static const char cvs_GROUP_SUB_H[] = "$Id: group_sub.h,v 1.13 2004/08/02 22:24:16 phelps Exp $";
#endif /* lint */

/* The subscription data type */
typedef struct group_sub *group_sub_t;

#include "message.h"
#include "panel.h"

/* The format for the callback function */
typedef void (*group_sub_callback_t)(void *rock, message_t message, int show_attachment);

/* Allocates and initializes a new group_sub_t */
group_sub_t group_sub_alloc(
    char *group,
    char *expression,
    int in_menu,
    int has_nazi,
    int min_time,
    int max_time,
    key_table_t key_table,
    char **key_names,
    int key_count,
    group_sub_callback_t callback,
    void *rock);

/* Releases resources used by the receiver */
void group_sub_free(group_sub_t self);

/* Answers the receiver's subscription expression */
char *group_sub_expression(group_sub_t self);

/* Updates the receiver to look just like subscription in terms of
 * name, expression, in_menu, has_nazi, min_time, max_time, keys,
 * callback and rock */
void group_sub_update_from_sub(
    group_sub_t self,
    group_sub_t subscription,
    key_table_t old_keys,
    key_table_t new_keys);

/* Sets the receiver's connection */
void group_sub_set_connection(group_sub_t self, elvin_handle_t handle, elvin_error_t error);

/* Registers the receiver with the control panel */
void group_sub_set_control_panel(group_sub_t self, control_panel_t control_panel);

/* Makes the receiver visible in the control panel's group menu iff
 * inMenu is set, and makes sure it appears at the proper index */
void group_sub_set_control_panel_index(
    group_sub_t self,
    control_panel_t control_panel,
    int *index);

#endif /* GROUP_SUB_H */
