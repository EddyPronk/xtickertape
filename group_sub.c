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
static const char cvsid[] = "$Id: group_sub.c,v 1.27 2001/05/06 23:06:10 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "group_sub.h"

#define F_VERSION "xtickertape.version"
#define F_TICKERTAPE "TICKERTAPE"
#define F_USER "USER"
#define F_TIMEOUT "TIMEOUT"
#define F_TICKERTEXT "TICKERTEXT"
#define F_REPLACEMENT "REPLACEMENT"
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

    /* The receiver's producer keys */
    elvin_keys_t producer_keys;

    /* The receiver's consumer keys */
    elvin_keys_t consumer_keys;

    /* Non-zero if the receiver should appear in the groups menu */
    int in_menu;

    /* Non-zero if the receiver's mime-enabled messages should automatically appear */
    int has_nazi;

    /* The minimum number of minutes to display a message in the receiver's group */
    int min_time;

    /* The maximum number of minutes to display a message in the receiver's group */
    int max_time;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* A convenient error context */
    elvin_error_t error;

    /* The receiver's subscription */
    elvin_subscription_t subscription;

    /* The receiver's control panel */ 
    control_panel_t control_panel;

    /* The receiver's control panel info (for unsubscribing/retitling) */
    void *control_panel_rock;

    /* The receiver's callback */
    group_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;

    /* Non-zero if the receiver is waiting on a change to the subscription */
    int is_pending;
};




/*
 *
 * Static functions
 *
 */


/* Delivers a notification which matches the receiver's subscription expression */
static void notify_cb(
    elvin_handle_t handle,
    elvin_subscription_t subscription,
    elvin_notification_t notification,
    int is_secure,
    void *rock,
    elvin_error_t error)
{
    group_sub_t self = (group_sub_t)rock;
    message_t message;
    elvin_basetypes_t type;
    elvin_value_t value;
    char *user;
    char *text;
    int timeout;
    char *mime_type;
    elvin_opaque_t mime_args;
    char *tag;
    char *message_id;
    char *reply_id;

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }
    
    /* Get the user from the notification (if provided) */
    if (elvin_notification_get(notification, F_USER, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	user = value.s;
    }
    else
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if (elvin_notification_get(notification, F_TICKERTEXT, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	text = value.s;
    }
    else
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    timeout = 0;
    if (elvin_notification_get(notification, F_TIMEOUT, &type, &value, error))
    {
	switch (type)
	{
	    case ELVIN_INT32:
	    {
		timeout = value.i;
		break;
	    }

	    case ELVIN_INT64:
	    {
		timeout = (int)value.h;
		break;
	    }

	    case ELVIN_REAL64:
	    {
		timeout = (int)(0.5 + value.d);
		break;
	    }

	    case ELVIN_STRING:
	    {
		timeout = atoi(value.s);
		break;
	    }

	    default:
	    {
		break;
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
    if (elvin_notification_get(notification, F_MIME_TYPE, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	mime_type = value.s;
    }
    else
    {
	mime_type = NULL;
    }

    /* Get the MIME args (if provided) */
    mime_args.data = NULL;
    mime_args.length = 0;

    if (elvin_notification_get(notification, F_MIME_ARGS, &type, &value, error))
    {
	if (type == ELVIN_STRING)
	{
	    mime_args.data = value.s;
	    mime_args.length = strlen(value.s);
	}
	else if (type == ELVIN_OPAQUE)
	{
	    mime_args.data = value.o.data;
	    mime_args.length = value.o.length;
	}
	else
	{
	    mime_args.data = NULL;
	    mime_args.length = 0;
	}
    }

    /* Get the replacement tag (if provided) */
    if (elvin_notification_get(notification, F_REPLACEMENT, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	tag = value.s;
    }
    else
    {
	tag = NULL;
    }

    /* Get the message id (if provided) */
    if (elvin_notification_get(notification, F_MESSAGE_ID, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	message_id = value.s;
    }
    else 
    {
	message_id = NULL;
    }

    /* Get the reply id (if provided) */
    if (elvin_notification_get(notification, F_IN_REPLY_TO, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	reply_id = value.s;
    }
    else
    {
	reply_id = NULL;
    }

    /* Construct a message */
    message = message_alloc(
	self -> name,
	self -> name, user, text, (unsigned long) timeout,
	mime_type, mime_args.data, mime_args.length,
	tag, message_id, reply_id);

    /* Deliver the message */
    (*self -> callback)(self -> rock, message, self -> has_nazi);

    /* Clean up */
    message_free(message);
}

/* Sends a message_t using the receiver's information */
static void send_message(group_sub_t self, message_t message)
{
    elvin_notification_t notification;
    unsigned int timeout;
    char *message_id;
    char *reply_id;
    char *mime_args;
    char *mime_type;
    size_t mime_length;
    
    /* Pull information out of the message */
    timeout = message_get_timeout(message);
    message_id = message_get_id(message);
    reply_id = message_get_reply_id(message);
    mime_type = message_get_mime_type(message);
    mime_length = message_get_mime_args(message, &mime_args);

    /* Allocate a new notification */
    if ((notification = elvin_notification_alloc(self -> error)) == NULL)
    {
	fprintf(stderr, "elvin_notification_alloc(): failed\n");
	abort();
    }

    /* Add an xtickertape version tag */
    if (elvin_notification_add_string(
	notification,
	F_VERSION,
	VERSION,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    /* Add the TICKERTAPE field */
    if (elvin_notification_add_string(
	notification,
	F_TICKERTAPE,
	self -> name,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    /* Add the USER field */
    if (elvin_notification_add_string(
	notification,
	F_USER,
	message_get_user(message),
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    /* Add the TICKERTEXT field */
    if (elvin_notification_add_string(
	notification,
	F_TICKERTEXT,
	message_get_string(message),
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    /* Add the TIMEOUT field */
    if (elvin_notification_add_int32(
	notification,
	F_TIMEOUT,
	timeout,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_int32(): failed\n");
	abort();
    }

    /* Add mime information if both mime_args and mime_type are provided */
    if ((mime_length != 0) && (mime_type != NULL))
    {
	if (elvin_notification_add_string(
		notification,
		F_MIME_ARGS,
		mime_args,
		self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    abort();
	}

	if (elvin_notification_add_string(
	    notification,
	    F_MIME_TYPE,
	    mime_type,
	    self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    abort();
	}
    }

    /* Add the Message-ID field */
    if (elvin_notification_add_string(
	notification,
	F_MESSAGE_ID,
	message_id,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }


    /* Add the In-Reply-To field if relevant */
    if (reply_id != NULL)
    {
	if (elvin_notification_add_string(
	    notification,
	    F_IN_REPLY_TO,
	    reply_id,
	    self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    abort();
	}
    }

    /* Send the notification */
    elvin_xt_notify(
	self -> handle,
	notification, 
	1, /* deliver_insecure */
	self -> producer_keys,
	self -> error);

    /* And clean up */
    elvin_notification_free(notification, self -> error);
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
    elvin_keys_t producer_keys,
    elvin_keys_t consumer_keys,
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

    /* Copy the producer keys (if there are any) */
    if (producer_keys != NULL)
    {
	self -> producer_keys = elvin_keys_clone(producer_keys, NULL);
    }
    else 
    {
	self -> producer_keys = NULL;
    }

    /* Copy the consumer keys (if there are any) */
    if (consumer_keys != NULL)
    {
	self -> consumer_keys = elvin_keys_clone(consumer_keys, NULL);
    }
    else
    {
	self -> consumer_keys = NULL;
    }
    /* Initialize everything else to a sane value */
    self -> in_menu = in_menu;
    self -> has_nazi = has_nazi;
    self -> min_time = min_time;
    self -> max_time = max_time;
    self -> control_panel = NULL;
    self -> control_panel_rock = NULL;
    self -> handle = NULL;
    self -> error = NULL;
    self -> subscription = NULL;
    self -> callback = callback;
    self -> rock = rock;
    self -> is_pending = 0;

    return self;
}

/* Releases resources used by a Subscription */
void group_sub_free(group_sub_t self)
{
    if (self -> name)
    {
	free(self -> name);
	self -> name = NULL;
    }

    if (self -> expression)
    {
	free(self -> expression);
	self -> expression = NULL;
    }

    if (self -> producer_keys)
    {
	elvin_keys_free(self -> producer_keys, NULL);
	self -> producer_keys = NULL;
    }

    if (self -> consumer_keys)
    {
	elvin_keys_free(self -> consumer_keys, NULL);
	self -> consumer_keys = NULL;
    }

    /* Don't free a pending subscription */
    if (self -> is_pending)
    {
	return;
    }

    free(self);
}

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


/* Callback for a subscribe request */
static void subscribe_cb(
    elvin_handle_t handle, int result,
    elvin_subscription_t subscription, void *rock,
    elvin_error_t error)
{
    group_sub_t self = (group_sub_t)rock;
    self -> subscription = subscription;
    self -> is_pending = 0;

    /* Unsubscribe if we were pending when we were freed */
    if (self -> expression == NULL)
    {
	group_sub_set_connection(self, NULL, error);
    }
}

/* Callback for an unsubscribe request */
static void unsubscribe_cb(
    elvin_handle_t handle, int result,
    elvin_subscription_t subscription, void *rock,
    elvin_error_t error)
{
    group_sub_t self = (group_sub_t)rock;
    self -> is_pending = 0;

    /* Free the receiver if it was pending when it was freed */
    if (self -> expression == NULL) 
    {
	group_sub_free(self);
    }
}

/* Sets the receiver's connection */
void group_sub_set_connection(group_sub_t self, elvin_handle_t handle, elvin_error_t error)
{
    if ((self -> handle != NULL) && (self -> subscription != NULL))
    {
	if (elvin_xt_delete_subscription(
	    self -> handle, self -> subscription,
	    unsubscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_xt_delete_subscription(): failed\n");
	    abort();
	}

	self -> is_pending = 1;
    }

    self -> handle = handle;
    self -> error = error;

    if (self -> handle != NULL)
    {
	if (elvin_xt_add_subscription(
	    self -> handle, self -> expression,
	    self -> consumer_keys,
	    (self -> consumer_keys == NULL) ? 1 : 0,
	    notify_cb, self,
	    subscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_xt_add_subscription(): failed\n");
	    abort();
	}

	self -> is_pending = 1;
    }
}


/* Registers the receiver with the control panel */
void group_sub_set_control_panel(group_sub_t self, control_panel_t control_panel)
{
    /* If it's the same control panel we had before then bail */
    if (self -> control_panel == control_panel)
    {
	return;
    }

    if ((self -> control_panel != NULL) && (self -> control_panel_rock != NULL))
    {
	control_panel_remove_subscription(self -> control_panel, self -> control_panel_rock);
	self -> control_panel_rock = NULL;
    }

    self -> control_panel = control_panel;

    if (self -> control_panel != NULL)
    {
	if (self -> in_menu)
	{
	    self -> control_panel_rock = control_panel_add_subscription(
		control_panel, self -> name, self -> name,
		(control_panel_callback_t)send_message, self);
	}
	else
	{
	    self -> control_panel_rock = NULL;
	}
    }
}

/* Makes the receiver visible in the control panel's group menu iff
 * inMenu is set, and makes sure it appears at the proper index */
void group_sub_set_control_panel_index(
    group_sub_t self,
    control_panel_t control_panel,
    int *index)
{
    /* If the control panel is changing, then simply forward this to set_control_panel */
    if (self -> control_panel != control_panel)
    {
	group_sub_set_control_panel(self, control_panel);
    }
    else
    {
	/* If we were in the control panel's menu but aren't now, then remove us */
	if ((self -> control_panel_rock != NULL) && (! self -> in_menu))
	{
	    control_panel_remove_subscription(self -> control_panel, self -> control_panel_rock);
	    self -> control_panel_rock = NULL;
	}

	/* If we weren't in the control panel's menu but are now, then add us */
	if ((self -> control_panel_rock == NULL) && (self -> in_menu))
	{
	    self -> control_panel_rock = control_panel_add_subscription(
		control_panel, self -> name, self -> name,
		(control_panel_callback_t)send_message, self);
	}
    }

    /* If the receiver is now in the control panel's group menu, make
     * sure it's at the appropriate index */
    if (self -> control_panel_rock != NULL)
    {
	control_panel_set_index(
	    self -> control_panel,
	    self -> control_panel_rock,
	    *index);

	(*index)++;
    }
}
