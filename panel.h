/***************************************************************

  Copyright (C) 1999-2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

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
****************************************************************/

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#ifndef lint
static const char cvs_CONTROL_PANEL_H[] = "$Id: panel.h,v 1.13 2004/08/02 22:24:16 phelps Exp $";
#endif /* lint */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* The control_panel_t datatype */
typedef struct control_panel *control_panel_t;

#include "message.h"
#include "tickertape.h"

/* The control_panel_t callback type */
typedef void (*control_panel_callback_t)(void *context, message_t message);

/* Allocates and initializes a new control_panel_t */
control_panel_t control_panel_alloc(
    tickertape_t tickertape,
    Widget parent,
    unsigned int send_history_count);

/* Releases the resources used by the receiver */
void control_panel_free(control_panel_t self);

/* Displays a message in the status line */
void control_panel_set_status(
    control_panel_t self,
    char *message);

/* Displays a message in the status line */
void control_panel_set_status_message(
    control_panel_t self,
    message_t message);

/* This is called when the elvin connection status changes */
void control_panel_set_connected(
    control_panel_t self,
    int is_connected);

/* Adds a subscription to the receiver.  Returns information which is
 * needed in order to later remove or re-index the subscription */
void *control_panel_add_subscription(
    control_panel_t self, char *tag, char *title,
    control_panel_callback_t callback, void *rock);

/* Removes a subscription from the receiver */
void control_panel_remove_subscription(control_panel_t self, void *rock);

/* Adds a message to the control panel's history */
void control_panel_add_message(control_panel_t self, message_t message);

/* Kills the thread rooted at message */
void control_panel_kill_thread(control_panel_t self, message_t message);

/* Changes the location of the subscription in the control panel's menu */
void control_panel_set_index(control_panel_t self, void *rock, int index);

/* Changes the title of a subscription */
void control_panel_retitle_subscription(control_panel_t self, void *rock, char *title);

/* Makes the control_panel visible and selects the given message in
 * the history list */
void control_panel_select(control_panel_t self, message_t message);

/* Makes the control_panel visible */
void control_panel_show(control_panel_t self);

/* Handle notifications */
void control_panel_handle_notify(control_panel_t self, Widget widget);

/* Show the previous item the user has sent */
void control_panel_history_prev(control_panel_t self);

/* Show the next item the user has sent */
void control_panel_history_next(control_panel_t self);

#endif /* CONTROLPANEL_H */
