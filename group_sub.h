/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2000.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
             Manages tickertape group subscriptions

****************************************************************/

#ifndef GROUP_SUB_H
#define GROUP_SUB_H

#ifndef lint
static const char cvs_GROUP_SUB_H[] = "$Id: group_sub.h,v 1.9 2001/08/25 14:04:43 phelps Exp $";
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
    elvin_keys_t producer_keys,
    elvin_keys_t consumer_keys,
    group_sub_callback_t callback,
    void *rock);

/* Releases resources used by the receiver */
void group_sub_free(group_sub_t self);

/* Answers the receiver's subscription expression */
char *group_sub_expression(group_sub_t self);

/* Updates the receiver to look just like subscription in terms of
 * group, in_menu, has_nazi, min_time, max_time, callback and rock,
 * but NOT expression */
void group_sub_update_from_sub(group_sub_t self, group_sub_t subscription);

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
