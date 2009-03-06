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
 *   Represents a Scroller and control panel and manages the
 *   connection between them, the notification service and the user
 */


#ifndef TICKERTAPE_H
#define TICKERTAPE_H

typedef struct tickertape *tickertape_t;

#include <X11/Intrinsic.h>

/* The structure of the application shell resources */
typedef struct {
    /* The version tag from the app-defaults file */
    const char *version_tag;

    /* The name of the metamail executable */
    const char *metamail;

    /* The number of messages to record in the send history */
    int send_history_count;
} XTickertapeRec;

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service */
tickertape_t
tickertape_alloc(XTickertapeRec *resources,
                 elvin_handle_t handle,
                 const char *user,
                 const char *domain,
                 const char *ticker_dir,
                 const char *config_file,
                 const char *groups_file,
                 const char *usenet_file,
                 const char *keys_file,
                 const char *keys_dir,
                 Widget top,
                 elvin_error_t error);


/* Frees the resources used by the Tickertape */
void
tickertape_free(tickertape_t self);


/* Prints out debugging information about the Tickertape */
void
tickertape_debug(tickertape_t self);


/* Answers the tickertape's user name */
const char *
tickertape_user_name(tickertape_t self);


/* Answers the tickertape's domain name */
const char *
tickertape_domain_name(tickertape_t self);


/* Reloads the tickertape's keys file */
void
tickertape_reload_keys(tickertape_t self);


/* Reloads the tickertape's groups file */
void
tickertape_reload_groups(tickertape_t self);


/* Reloads the tickertape's usenet file */
void
tickertape_reload_usenet(tickertape_t self);


/* Reloads all of tickertape's config files */
void
tickertape_reload_all(tickertape_t self);


/* Show the previous item in the history */
void
tickertape_history_prev(tickertape_t self);


/* Show the next item in the history */
void
tickertape_history_next(tickertape_t self);


/* Quit the application */
void
tickertape_quit(tickertape_t self);


/* Displays a message's MIME attachment */
int
tickertape_show_attachment(tickertape_t self, message_t message);


#endif /* TICKERTAPE_H */
