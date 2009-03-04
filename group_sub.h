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

/*
 * Description:
 *   Manages tickertape group subscriptions
 */

#ifndef GROUP_SUB_H
#define GROUP_SUB_H

/* The subscription data type */
typedef struct group_sub *group_sub_t;

#include "message.h"
#include "panel.h"

/* The format for the callback function */
typedef void (*group_sub_callback_t)(void *rock, message_t message,
                                     int show_attachment);

/* Allocates and initializes a new group_sub_t */
group_sub_t
group_sub_alloc(char *group,
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
void
group_sub_free(group_sub_t self);


/* Answers the receiver's subscription expression */
char *
group_sub_expression(group_sub_t self);


/* Updates the receiver to look just like subscription in terms of
 * name, expression, in_menu, has_nazi, min_time, max_time, keys,
 * callback and rock */
void
group_sub_update_from_sub(group_sub_t self,
                          group_sub_t subscription,
                          key_table_t old_keys,
                          key_table_t new_keys);


/* Sets the receiver's connection */
void
group_sub_set_connection(group_sub_t self,
                         elvin_handle_t handle,
                         elvin_error_t error);


/* Registers the receiver with the control panel */
void
group_sub_set_control_panel(group_sub_t self, control_panel_t control_panel);


/* Makes the receiver visible in the control panel's group menu iff
 * inMenu is set, and makes sure it appears at the proper index */
void
group_sub_set_control_panel_index(group_sub_t self,
                                  control_panel_t control_panel,
                                  int *index);


#endif /* GROUP_SUB_H */
