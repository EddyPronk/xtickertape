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
static const char cvsid[] = "$Id: group_sub.c,v 1.1 1999/10/02 16:45:10 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "group_sub.h"

#define F_VERSION "xtickertape.version"
#define F_TICKERTAPE "TICKERTAPE"
#define F_USER "USER"
#define F_TIMEOUT "TIMEOUT"
#define F_TICKERTEXT "TICKERTEXT"
#define F_MESSAGE_ID "Message-Id"
#define F_IN_REPLY_TO "In-Reply-To"
#define F_MIME_ARGS "MIME_ARGS"
#define F_MIME_TYPE "MIME_TYPE"


/* The group subscription data type */
struct group_sub
{
    /* The name of the receiver's group */
    char *name;

    /* The receiver's subscription expression */
    char *expression;

    /* Non-zero if the receiver should appear in the groups menu */
    int in_menu;

    /* Non-zero if the receiver's mime-enabled messages should automatically appear */
    int has_nazi;

    /* The minimum number of minutes to display a message in the receiver's group */
    int min_time;

    /* The maximum number of minutes to display a message in the receiver's group */
    int max_time;

    /* The receiver's connection */
    connection_t connection;

    /* The receiver's connection information (for unsubscribing) */
    void *connection_info;

    /* The receiver's ControlPanel */ 
    ControlPanel control_panel;

    /* The receiver's ControlPanel info (for unsubscribing/retitling) */
    void *control_panel_info;

    /* The receiver's callback */
    group_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;
};




/*
 *
 * Static functions
 *
 */


/* Delivers a notification which matches the receiver's subscription expression */
static void handle_notify(group_sub_t self, en_notify_t notification)
{
    message_t message;
    en_type_t type;
    char *user;
    char *text;
    void *value;
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
    if ((en_search(notification, F_USER, &type, (void **)&user) != 0) ||
	(type != EN_STRING))
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if ((en_search(notification, F_TICKERTEXT, &type, (void **)&text) != 0) ||
	(type != EN_STRING))
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    if (en_search(notification, F_TIMEOUT, &type, (void **)&value) != 0)
    {
	timeout = 0;
    }
    else
    {
	switch (type)
	{
	    /* If it's a string, then use atoi to make it into an integer */
	    case EN_STRING:
	    {
		char *string = (char *)value;
		timeout = atoi(string);
		break;
	    }

	    /* If it's an integer, then just copy the value */
	    case EN_INT32:
	    {
		timeout = *((int32 *) value);
		break;
	    }

	    /* If it's a float, then round it to the nearest integer */
	    case EN_FLOAT:
	    {
		timeout = (0.5 + *((float *) value));
		break;
	    }

	    /* Just use a default for anything else */
	    default:
	    {
		timeout = 0;
	    }
	}
    }

    /* If the timeout was illegible, then set it to 10 minutes */
    timeout = (timeout == 0) ? 10 : timeout;

    /* Make sure the timeout conforms */
    if (timeout < self -> min_time)
    {
	timeout = self -> min_time;
    }
    else if (timeout > self -> max_time)
    {
	timeout = self -> max_time;
    }

    /* Get the MIME type (if provided) */
    if ((en_search(notification, F_MIME_TYPE, &type, (void **)&mime_type) != 0) ||
	(type != EN_STRING))
    {
	mime_type = NULL;
    }

    /* Get the MIME args (if provided) */
    if ((en_search(notification, F_MIME_ARGS, &type, (void **)&mime_args) != 0) ||
	(type != EN_STRING))
    {
	mime_args = NULL;
    }

    /* Get the message id (if provided) */
    if ((en_search(notification, F_MESSAGE_ID, &type, (void **)&message_id) != 0) ||
	(type != EN_STRING))
    {
	message_id = NULL;
    }

    /* Get the reply id (if provided) */
    if ((en_search(notification, F_IN_REPLY_TO, &type, (void **)&reply_id) != 0) ||
	(type != EN_STRING))
    {
	reply_id = NULL;
    }

    /* Construct a message */
    message = message_alloc(
	self -> control_panel_info, self -> name,
	user, text, (unsigned long) timeout,
	mime_type, mime_args,
	message_id, reply_id);

    /* Deliver the message */
    (*self -> callback)(self -> rock, message);

    /* Clean up */
    message_free(message);
    en_free(notification);
}

/* Sends a message_t using the receiver's information */
static void send_message(group_sub_t self, message_t message)
{
    en_notify_t notification;
    int32 timeout;
    char *message_id;
    char *reply_id;
    char *mime_args;
    char *mime_type;
    
    /* Pull information out of the message */
    timeout = message_get_timeout(message);
    message_id = message_get_id(message);
    reply_id = message_get_reply_id(message);
    mime_args = message_get_mime_args(message);
    mime_type = message_get_mime_type(message);

    /* Use it to construct a notification */
    notification = en_new();
    en_add_string(notification, F_VERSION, VERSION);
    en_add_string(notification, F_TICKERTAPE, self -> name);
    en_add_string(notification, F_USER, message_get_user(message));
    en_add_int32(notification, F_TIMEOUT, timeout);
    en_add_string(notification, F_TICKERTEXT, message_get_string(message));
    en_add_string(notification, F_MESSAGE_ID, message_id);

    if (reply_id != NULL)
    {
	en_add_string(notification, F_IN_REPLY_TO, reply_id);
    }

    /* Add mime information if both mime_args and mime_type are provided */
    if ((mime_args != NULL) && (mime_type != NULL))
    {
	en_add_string(notification, F_MIME_ARGS, mime_args);
	en_add_string(notification, F_MIME_TYPE, mime_type);
    }

    connection_publish(self -> connection, notification);
    en_free(notification);
}


/*
 *
 * Exported functions
 *
 */


/* Allocates and initializes a new group_sub_t */
group_sub_t group_sub_alloc(
    char *name,
    char *expression,
    int in_menu,
    int has_nazi,
    int min_time,
    int max_time,
    group_sub_callback_t callback,
    void *rock)
{
    group_sub_t self;

    /* Allocate memory for the new group_sub_t */
    if ((self = (group_sub_t)malloc(sizeof(struct group_sub))) == NULL)
    {
	return NULL;
    }

    /* Copy the name string */
    if ((self -> name = strdup(name)) == NULL)
    {
	free(self);
	return NULL;
    }

    /* Copy the subscription expression */
    if ((self -> expression = strdup(expression)) == NULL)
    {
	free(self -> name);
	free(self);
	return NULL;
    }

    /* Initialize everything else to a sane value */
    self -> in_menu = in_menu;
    self -> has_nazi = has_nazi;
    self -> min_time = min_time;
    self -> max_time = max_time;
    self -> control_panel = NULL;
    self -> control_panel_info = NULL;
    self -> connection = NULL;
    self -> connection_info = NULL;
    self -> callback = callback;
    self -> rock = rock;

    return self;
}

/* Releases resources used by a Subscription */
void group_sub_free(group_sub_t self)
{
    if (self -> name)
    {
	free(self -> name);
    }

    if (self -> expression)
    {
	free(self -> expression);
    }

    free(self);
}

#ifdef DEBUG
/* Prints debugging information */
void group_sub_debug(group_sub_t self)
{
    printf("group_sub_t (%p)\n", self);
    printf("  name = \"%s\"\n", self -> name ? self -> name : "<none>");
    printf("  expression = \"%s\"\n", self -> expression ? self -> expression : "<none>");
    printf("  in_menu = %s\n", self -> in_menu ? "true" : "false");
    printf("  has_nazi = %s\n", self -> has_nazi ? "true" : "false");
    printf("  min_time = %d\n", self -> min_time);
    printf("  max_time = %d\n", self -> max_time);
    printf("  connection = %p\n", self -> connection);
    printf("  connection_info = %p\n", self -> connection_info);
    printf("  control_panel = %p\n", self -> control_panel);
    printf("  control_panel_info = %p\n", self -> control_panel_info);
    printf("  callback = %p\n", self -> callback);
    printf("  rock = %p\n", self -> rock);
}
#endif /* DEBUG */


/* Answers the receiver's subscription expression */
char *group_sub_expression(group_sub_t self)
{
    return self -> expression;
}


/* Updates the receiver to look just like subscription in terms of
 * name, in_menu, autoMime, min_time, max_time, callback and rock,
 * but NOT expression */
void group_sub_update_from_sub(group_sub_t self, group_sub_t subscription)
{
    /* Release any dynamically allocated strings in the receiver */
    free(self -> name);
    
    /* Copy various fields */
    self -> name = strdup(subscription -> name);
    self -> in_menu = subscription -> in_menu;
    self -> has_nazi = subscription -> has_nazi;
    self -> min_time = subscription -> min_time;
    self -> max_time = subscription -> max_time;
    self -> callback = subscription -> callback;
    self -> rock = subscription -> rock;
}


/* Sets the receiver's connection */
void group_sub_set_connection(group_sub_t self, connection_t connection)
{
    if (self -> connection != NULL)
    {
	connection_unsubscribe(self -> connection, self -> connection_info);
    }

    self -> connection = connection;

    if (self -> connection != NULL)
    {
	self -> connection_info = connection_subscribe(
	    self -> connection, self -> expression,
	    (notify_callback_t)handle_notify, self);
    }
}


/* Registers the receiver with the ControlPanel */
void group_sub_set_control_panel(group_sub_t self, ControlPanel control_panel)
{
    /* If it's the same control panel we had before then bail */
    if (self -> control_panel == control_panel)
    {
	return;
    }

    if (self -> control_panel != NULL)
    {
	ControlPanel_removeSubscription(self -> control_panel, self -> control_panel_info);
	self -> control_panel_info = NULL;
    }

    self -> control_panel = control_panel;

    if (self -> control_panel != NULL)
    {
	if (self -> in_menu)
	{
	    self -> control_panel_info = ControlPanel_addSubscription(
		control_panel, self -> name,
		(ControlPanelCallback)send_message, self);
	}
	else
	{
	    self -> control_panel_info = NULL;
	}
    }
}

/* Makes the receiver visible in the ControlPanel's group menu iff
 * inMenu is set, and makes sure it appears at the proper index */
void group_sub_set_control_panel_index(
    group_sub_t self,
    ControlPanel control_panel,
    int *index)
{
    /* If the ControlPanel is changing, then simply forward this to setControlPanel */
    if (self -> control_panel != control_panel)
    {
	group_sub_set_control_panel(self, control_panel);
    }
    else
    {
	/* If we were in the ControlPanel's menu but aren't now, then remove us */
	if ((self -> control_panel_info != NULL) && (! self -> in_menu))
	{
	    ControlPanel_removeSubscription(self -> control_panel, self -> control_panel_info);
	    self -> control_panel_info = NULL;
	}

	/* If we weren't in the ControlPanel's menu but are now, then add us */
	if ((self -> control_panel_info == NULL) && (self -> in_menu))
	{
	    self -> control_panel_info = ControlPanel_addSubscription(
		control_panel, self -> name,
		(ControlPanelCallback)send_message, self);
	}
    }

    /* If the receiver is now in the ControlPanel's group menu, make
     * sure it's at the appropriate index */
    if (self -> control_panel_info != NULL)
    {
	ControlPanel_setSubscriptionIndex(self -> control_panel, self -> control_panel_info, *index);
	(*index)++;
    }
}
