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

   Description: 
             Represents a Scroller and control panel and manages the
	     connection between them, the notification service and the 
	     user

****************************************************************/

#ifndef TICKERTAPE_H
#define TICKERTAPE_H

#ifndef lint
static const char cvs_TICKERTAPE_H[] = "$Id: tickertape.h,v 1.22 2004/08/02 22:24:17 phelps Exp $";
#endif /* lint */

typedef struct tickertape *tickertape_t;

#include <X11/Intrinsic.h>

/* The structure of the application shell resources */
typedef struct
{
    /* The version tag from the app-defaults file */
    char *version_tag;

    /* The name of the metamail executable */
    char *metamail;

    /* The number of messages to record in the send history */
    int send_history_count;
} XTickertapeRec;

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service */
tickertape_t tickertape_alloc(
    XTickertapeRec *resources,
    elvin_handle_t handle,
    char *user, char *domain,
    char *ticker_dir,
    char *config_file,
    char *groups_file,
    char *usenet_file,
    char *keys_file,
    char *keys_dir,
    Widget top,
    elvin_error_t error);

/* Frees the resources used by the Tickertape */
void tickertape_free(tickertape_t self);

/* Prints out debugging information about the Tickertape */
void tickertape_debug(tickertape_t self);

/* Answers the tickertape's user name */
char *tickertape_user_name(tickertape_t self);

/* Answers the tickertape's domain name */
char *tickertape_domain_name(tickertape_t self);

/* Reloads the tickertape's keys file */
void tickertape_reload_keys(tickertape_t self);

/* Reloads the tickertape's groups file */
void tickertape_reload_groups(tickertape_t self);

/* Reloads the tickertape's usenet file */
void tickertape_reload_usenet(tickertape_t self);

/* Reloads all of tickertape's config files */
void tickertape_reload_all(tickertape_t self);

/* Show the previous item in the history */
void tickertape_history_prev(tickertape_t self);

/* Show the next item in the history */
void tickertape_history_next(tickertape_t self);

/* Quit the application */
void tickertape_quit(tickertape_t self);

/* Displays a message's MIME attachment */
int tickertape_show_attachment(tickertape_t self, message_t message);

#endif /* TICKERTAPE_H */
