/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* The control_panel_t datatype */
typedef struct control_panel *control_panel_t;

#include "message.h"
#include "tickertape.h"

/* The control_panel_t callback type */
typedef void (*control_panel_callback_t)(void *context, message_t message);

/* Allocates and initializes a new control_panel_t */
control_panel_t
control_panel_alloc(tickertape_t tickertape,
                    Widget parent,
                    unsigned int send_history_count);


/* Releases the resources used by the receiver */
void
control_panel_free(control_panel_t self);


/* Displays a message in the status line */
void
control_panel_set_status(control_panel_t self, char *message);


/* Displays a message in the status line */
void
control_panel_set_status_message(control_panel_t self, message_t message);


/* This is called when the elvin connection status changes */
void
control_panel_set_connected(control_panel_t self, int is_connected);


/* Adds a subscription to the receiver.  Returns information which is
 * needed in order to later remove or re-index the subscription */
void *
control_panel_add_subscription(control_panel_t self,
                               char *tag,
                               char *title,
                               control_panel_callback_t callback,
                               void *rock);


/* Removes a subscription from the receiver */
void
control_panel_remove_subscription(control_panel_t self, void *rock);


/* Adds a message to the control panel's history */
void
control_panel_add_message(control_panel_t self, message_t message);


/* Kills the thread rooted at message */
void
control_panel_kill_thread(control_panel_t self, message_t message);


/* Changes the location of the subscription in the control panel's menu */
void
control_panel_set_index(control_panel_t self, void *rock, int index);


/* Changes the title of a subscription */
void
control_panel_retitle_subscription(control_panel_t self,
                                   void *rock,
                                   char *title);


/* Makes the control_panel visible and selects the given message in
 * the history list */
void
control_panel_select(control_panel_t self, message_t message);


/* Makes the control_panel visible */
void
control_panel_show(control_panel_t self);


/* Handle notifications */
void
control_panel_handle_notify(control_panel_t self, Widget widget);


/* Show the previous item the user has sent */
void
control_panel_history_prev(control_panel_t self);


/* Show the next item the user has sent */
void
control_panel_history_next(control_panel_t self);


#endif /* CONTROLPANEL_H */
