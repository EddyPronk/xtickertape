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
             Represents a Scroller and control panel and manages the
	     connection between them, the notification service and the 
	     user

****************************************************************/

#ifndef TICKERTAPE_H
#define TICKERTAPE_H

#ifndef lint
static const char cvs_TICKERTAPE_H[] = "$Id: tickertape.h,v 1.20 2003/05/15 20:23:03 phelps Exp $";
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

/* Show the previous item in the history */
void tickertape_history_prev(tickertape_t self);

/* Show the next item in the history */
void tickertape_history_next(tickertape_t self);

/* Quit the application */
void tickertape_quit(tickertape_t self);

/* Displays a message's MIME attachment */
int tickertape_show_attachment(tickertape_t self, message_t message);

#endif /* TICKERTAPE_H */
