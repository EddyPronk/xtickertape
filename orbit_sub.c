/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
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

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: orbit_sub.c,v 1.2 1999/10/04 13:59:02 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include "orbit_sub.h"

/* The orbit_sub data type */
struct orbit_sub
{
    /* The receiver's title */
    char *title;

    /* The receiver's zone id */
    char *id;

    /* The receiver's connection */
    connection_t connection;

    /* The receiver's connection information */
    void *connection_info;

    /* The receiver's ControlPanel */
    ControlPanel control_panel;

    /* The receiver's ControlPanel information */
    void *control_panel_info;

    /* The receiver's callback function */
    orbit_sub_callback_t callback;

    /* The receiver's callback context */
    void *rock;
};


/*
 *
 * Static functions
 *
 */

/* Transforms a notification into a message_t and delivers it */
static void handle_notify(orbit_sub_t self, en_notify_t notification)
{
    message_t message;
    en_type_t type;
    char *user;
    char *text;
    int32 *timeout_p;
    int32 timeout;
    char *mime_type;
    char *mime_args;
    char *message_id;
    char *reply_id;

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }

    /* Get the user from the notification (if provided) */
    if ((en_search(notification, "USER", &type, (void **)&user) != 0) ||
	(type != EN_STRING))
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if ((en_search(notification, "TICKERTEXT", &type, (void **)&text) != 0) ||
	(type != EN_STRING))
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    if ((en_search(notification, "TIMEOUT", &type, (void **)&timeout_p) != 0) ||
	(type != EN_INT32))
    {
	timeout_p = &timeout;
	timeout = 0;

	/* Check to see if it's in ascii format */
	if (type == EN_STRING)
	{
	    char *timeoutString;
	    en_search(notification, "TIMEOUT", &type, (void **)&timeoutString);
	    timeout = atoi(timeoutString);
	}

	timeout = (timeout == 0) ? 10 : timeout;
    }

    /* Get the MIME type (if provided) */
    if ((en_search(notification, "MIME_TYPE", &type, (void **)&mime_type) != 0) ||
	(type != EN_STRING))
    {
	mime_type = NULL;
    }

    /* Get the MIME args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mime_args) != 0) ||
	(type != EN_STRING))
    {
	mime_args = NULL;
    }

    /* Get the message id (if provided) */
    if ((en_search(notification, "Message-Id", &type, (void **)&message_id) != 0) ||
	(type != EN_STRING))
    {
        message_id = NULL;
    }

    /* Get the reply id (if provided) */
    if ((en_search(notification, "In-Reply-To", &type, (void **)&reply_id) != 0) ||
	(type != EN_STRING))
    {
        reply_id = NULL;
    }

    /* Construct the message_t */
    message = message_alloc(
	self -> control_panel_info, self -> title,
	user, text, *timeout_p,
	mime_type, mime_args,
	message_id, reply_id);

    /* Deliver the message_t */
    (*self -> callback)(self -> rock, message);

    /* Release our reference to the message */
    message_free(message);
}

/* Constructs a notification out of a message_t and delivers it to the connection_t */
void send_message(orbit_sub_t self, message_t message)
{
    en_notify_t notification;
    int32 timeout;
    char *message_id;
    char *reply_id;
    char *mime_args;
    char *mime_type;

    timeout = message_get_timeout(message);
    message_id = message_get_id(message);
    reply_id = message_get_reply_id(message);
    mime_args = message_get_mime_args(message);
    mime_type = message_get_mime_type(message);

    notification = en_new();
    en_add_string(notification, "zone.id", self -> id);
    en_add_string(notification, "USER", message_get_user(message));
    en_add_string(notification, "TICKERTEXT", message_get_string(message));
    en_add_int32(notification, "TIMEOUT", timeout);
    en_add_string(notification, "Message-Id", message_id);

    if (reply_id != NULL)
    {
	en_add_string(notification, "In-Reply-To", reply_id);
    }

    /* Add mime information if both mime_args and mime_type are provided */
    if ((mime_args != NULL) && (mime_type != NULL))
    {
	en_add_string(notification, "MIME_ARGS", mime_args);
	en_add_string(notification, "MIME_TYPE", mime_type);
    }

    connection_publish(self -> connection, notification);
    en_free(notification);
}


/*
 *
 * Exported functions
 *
 */

/* Allocates and initializes a new orbit_sub_t */
orbit_sub_t orbit_sub_alloc(char *title, char *id, orbit_sub_callback_t callback, void *rock)
{
    orbit_sub_t self;

    /* Allocate space for the new orbit_sub_t */
    if ((self = (orbit_sub_t)malloc(sizeof(struct orbit_sub))) == NULL)
    {
	return NULL;
    }

    /* Copy the title string */
    if ((self -> title = strdup(title)) == NULL)
    {
	free(self);
	return NULL;
    }

    /* Copy the id string */
    if ((self -> id = strdup(id)) == NULL)
    {
	free(self -> title);
	free(self);
	return NULL;
    }

    /* Initialize everything else to sane values */
    self -> connection = NULL;
    self -> connection_info = NULL;
    self -> control_panel = NULL;
    self -> control_panel_info = NULL;
    self -> callback = callback;
    self -> rock = rock;

    return self;
}

/* Releases resources used by the receiver */
void orbit_sub_free(orbit_sub_t self)
{
    /* Free the receiver's title if it has one */
    if (self -> title)
    {
	free(self -> title);
    }

    /* Free the receiver's id if it has one */
    if (self -> id)
    {
	free(self -> id);
    }

    free(self);
}

#ifdef DEBUG
/* Prints debugging information */
void orbit_sub_debug(orbit_sub_t self)
{
    printf("orbit_sub_t (%p)\n", self);
    printf("  title = \"%s\"\n", self -> title);
    printf("  id = \"%s\"\n", self -> id);
    printf("  connection = %p\n", self -> connection);
    printf("  connection_info = %p\n", self -> connection_info);
    printf("  control_panel = %p\n", self -> control_panel);
    printf("  control_panel_info = %p\n", self -> control_panel_info);
    printf("  callback = %p\n", self -> control_panel);
    printf("  rock = %p\n", self -> rock);
}
#endif /* DEBUG */


/* Answers the receiver's zone id */
char *orbit_sub_get_id(orbit_sub_t self)
{
    return self -> id;
}

/* Sets the receiver's title */
void orbit_sub_set_title(orbit_sub_t self, char *title)
{
    /* Check for identical titles and don't let the title be set to NULL */
    if ((title == NULL) || ((self -> title != NULL) && (strcmp(self -> title, title) == 0)))
    {
	return;
    }

    /* Release the old title */
    if (self -> title != NULL)
    {
	free(self -> title);
    }

    self -> title = strdup(title);

    /* Update our menu item */
    if (self -> control_panel != NULL)
    {
	ControlPanel_retitleSubscription(
	    self -> control_panel,
	    self -> control_panel_info,
	    self -> title);
    }
}

/* Answers the receiver's title */
char *orbit_sub_get_title(orbit_sub_t self)
{
    return self -> title;
}


/* Sets the receiver's connection */
void orbit_sub_set_connection(orbit_sub_t self, connection_t connection)
{
    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from old connection */
    if (self -> connection != NULL)
    {
	connection_unsubscribe(self -> connection, self -> connection_info);
    }

    self -> connection = connection;

    /* Subscribe to the new connection */
    if (self -> connection != NULL)
    {
	char *buffer = (char *)malloc(
	    sizeof("exists(TICKERTEXT) && zone.id == \"\"") + strlen(self -> id));

	sprintf(buffer, "exists(TICKERTEXT) && zone.id == \"%s\"\n", self -> id);
	self -> connection_info = connection_subscribe(
	    self -> connection, buffer,
	    (notify_callback_t)handle_notify, self);
	free(buffer);
    }
}

/* Sets the receiver's ControlPanel */
void orbit_sub_set_control_panel(orbit_sub_t self, ControlPanel control_panel)
{
    /* Shortcut if we're with the same ControlPanel */
    if (self -> control_panel == control_panel)
    {
	return;
    }

    /* Unregister with the old ControlPanel */
    if (self -> control_panel != NULL)
    {
	ControlPanel_removeSubscription(self -> control_panel, self -> control_panel_info);
    }

    self -> control_panel = control_panel;

    /* Register with the new ControlPanel */
    if (self -> control_panel != NULL)
    {
	self -> control_panel_info = ControlPanel_addSubscription(
	    control_panel, self -> title,
	    (ControlPanelCallback)send_message, self);
    }
}

