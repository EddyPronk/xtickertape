/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2003.
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

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: group_sub.c,v 1.48 2003/01/27 17:50:42 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* exit, fprintf, snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* abort, atoi, exit, free, malloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memcpy, memset, strcpy, strlen */
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h> /* assert */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include "globals.h"
#include "replace.h"
#include "group_sub.h"

#define F3_USER_AGENT "User-Agent"
#define F3_GROUP "Group"
#define F2_TICKERTAPE "TICKERTAPE"
#define F3_FROM "From"
#define F2_USER "USER"
#define F3_MESSAGE "Message"
#define F2_TICKERTEXT "TICKERTEXT"
#define F3_TIMEOUT "Timeout"
#define F2_TIMEOUT "TIMEOUT"
#define F3_REPLACES "Replaces"
#define F2_REPLACEMENT "REPLACEMENT"
#define F3_MESSAGE_ID "Message-Id"
#define F3_IN_REPLY_TO "In-Reply-To"
#define F3_THREAD_ID "Thread-Id"
#define F3_MIME_ATTACHMENT "MIME-Attachment"
#define F2_MIME_ARGS "MIME_ARGS"
#define F2_MIME_TYPE "MIME_TYPE"

#define ATTACHMENT_HEADER_FMT \
    "MIME-Version: 1.0\n" \
    "Content-Type: %s; charset=us-ascii\n\n"

/* The size of the MIME args buffer */
#define BUFFER_SIZE (32)

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
#  define ELVIN_RETURN_FAILURE
#  define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
#  define ELVIN_RETURN_FAILURE 0
#  define ELVIN_RETURN_SUCCESS 1
#endif


/* The group subscription data type */
struct group_sub
{
    /* The name of the receiver's group */
    char *name;

    /* The receiver's subscription expression */
    char *expression;

    /* The receiver's notification keys */
    elvin_keys_t notification_keys;

    /* The receiver's subscription keys */
    elvin_keys_t subscription_keys;

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

#if ! defined(ELVIN_VERSION_AT_LEAST)    
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
    char *attachment = NULL;
    uint32_t length = 0;
    char *mime_type;
    char *buffer = NULL;
    size_t header_length = 0;
    elvin_opaque_t mime_args;
    char *tag;
    char *message_id;
    char *reply_id;
    char *thread_id;
    int found;

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }
    
    /* Get the user from the notification (if provided) */
    if (elvin_notification_get(notification, F3_FROM, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	user = (char *)value.s;
    }
    else if (elvin_notification_get(notification, F2_USER, &type, &value, error) &&
	     type == ELVIN_STRING)
    {
	user = (char *)value.s;
    }
    else
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if (elvin_notification_get(notification, F3_MESSAGE, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	text = (char *)value.s;
    }
    else if (elvin_notification_get(notification, F2_TICKERTEXT, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	text = (char *)value.s;
    }
    else
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    timeout = 0;
    if (elvin_notification_get(notification, F3_TIMEOUT, &type, &value, error))
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
		timeout = atoi((char *)value.s);
		break;
	    }

	    default:
	    {
		break;
	    }
	}
    }
    else if (elvin_notification_get(notification, F2_TIMEOUT, &type, &value, error))
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
		timeout = atoi((char *)value.s);
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

    /* Get the `Attachment' field from the notification */
    if (elvin_notification_get(notification, F3_MIME_ATTACHMENT, &type, &value, error) &&
	type == ELVIN_OPAQUE)
    {
	attachment = value.o.data;
	length = value.o.length;
    }
    else
    {
	/* Get the MIME type (if provided) */
	if (elvin_notification_get(notification, F2_MIME_TYPE, &type, &value, error) &&
	    type == ELVIN_STRING)
	{
	    mime_type = (char *)value.s;
	    header_length = sizeof(ATTACHMENT_HEADER_FMT) - 3 + strlen(mime_type);
	}
	else
	{
	    mime_type = NULL;
	}

	/* Get the MIME args (if provided) */
	found = elvin_notification_get(notification, F2_MIME_ARGS, &type, &value, error);
	if (mime_type != NULL && found && type == ELVIN_STRING)
	{
	    length = header_length + strlen(value.s);
	    if ((buffer = malloc(length + 1)) == NULL)
	    {
		length = 0;
	    }
	    else
	    {
		attachment = buffer;
		snprintf(buffer, header_length + 1, ATTACHMENT_HEADER_FMT, mime_type);
		strcpy(buffer + header_length, value.s);
	    }
	}
	else if (mime_type != NULL && found && type == ELVIN_OPAQUE)
	{
	    length = header_length + value.o.length;
	    if ((buffer = malloc(length + 1)) == NULL)
	    {
		length = 0;
	    }
	    else
	    {
		attachment = buffer;
		snprintf(buffer, header_length + 1, ATTACHMENT_HEADER_FMT, mime_type);
		memcpy(buffer + header_length, value.o.data, value.o.length);
	    }
	}
	else
	{
	    mime_args.data = NULL;
	    mime_args.length = 0;
	}
    }

    /* Get the replacement tag (if provided) */
    if (elvin_notification_get(notification, F3_REPLACES, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	tag = (char *)value.s;
    }
    else if (elvin_notification_get(notification, F2_REPLACEMENT, &type, &value, error) &&
	     type == ELVIN_STRING)
    {
	tag = (char *)value.s;
    }
    else
    {
	tag = NULL;
    }

    /* Get the message id (if provided) */
    if (elvin_notification_get(notification, F3_MESSAGE_ID, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	message_id = (char *)value.s;
    }
    else 
    {
	message_id = NULL;
    }

    /* Get the reply id (if provided) */
    if (elvin_notification_get(notification, F3_IN_REPLY_TO, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	reply_id = (char *)value.s;
    }
    else
    {
	reply_id = NULL;
    }

    /* Get the thread-id (if provided) */
    if (elvin_notification_get(notification, F3_THREAD_ID, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	thread_id = (char *)value.s;
    }
    else
    {
	thread_id = NULL;
    }

    /* Construct a message */
    message = message_alloc(
	self -> name,
	self -> name, user, text, (unsigned long) timeout,
	attachment, length,
	tag, message_id, reply_id,
	thread_id);

    /* Deliver the message */
    (*self -> callback)(self -> rock, message, self -> has_nazi);

    /* Clean up */
    message_free(message);

    if (buffer != NULL)
    {
	free(buffer);
    }
}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* Delivers a notification which matches the receiver's subscription expression */
static int notify_cb(
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
    int timeout = 0;
    uchar *attachment;
    uint32_t length;
    char *mime_type;
    size_t header_length;
    elvin_opaque_t mime_args;
    char *buffer = NULL;
    char *tag;
    char *message_id;
    char *reply_id;
    char *thread_id;
    int found;

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return 1;
    }

    /* Get the `From' field from the notification */
    if (! elvin_notification_get_string(notification, F3_FROM, &found, &user, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* If that didn't work then try the old `USER' field */
    if (! found)
    {
	if (! elvin_notification_get_string(notification, F2_USER, &found, &user, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}
    }

    /* Use a default user if none was provided in the notification */
    if (! found)
    {
	user = "anonymous";
    }

    /* Get the `Message' field from the notification */
    if (! elvin_notification_get_string(notification, F3_MESSAGE, &found, &text, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Try the `TICKERTEXT' field if no `Message' found */
    if (! found) {
	if (! elvin_notification_get_string(notification, F2_TICKERTEXT, &found, &text, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}
    }

    /* Default to an empty message if none provided */
    if (! found)
    {
	text = "";
    }

    /* Get the `Timeout' field from the notification */
    if (! elvin_notification_get(notification, F3_TIMEOUT, &found, &type, &value, error))
    {
	fprintf(stderr, "elvin_notification_get(): failed");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Try the `TIMEOUT' field for backward compatibility */
    if (! found)
    {
	if (! elvin_notification_get(notification, F2_TIMEOUT, &found, &type, &value, error))
	{
	    fprintf(stderr, "elvin_notification_get(): failed");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}
    }

    /* Be overly generous with the timeout field's type */
    if (found)
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

    /* If the timeout was zero, then set it to 10 minutes */
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

    /* Get the `Attachment' field from the notification */
    if (! elvin_notification_get_opaque(
	    notification,
	    F3_MIME_ATTACHMENT,
	    &found,
	    &attachment,
	    &length,
	    error))
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    if (! found)
    {
	/* Try the backward compatible `MIME-TYPE' field */
	if (! elvin_notification_get_string(
		notification,
		F2_MIME_TYPE,
		NULL, &mime_type,
		error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	/* Look for mime args if we have a mime type */
	if (mime_type != NULL)
	{
	    header_length = sizeof(ATTACHMENT_HEADER_FMT) - 3 + strlen(mime_type);

	    /* Try the backward compatible `MIME_ARGS' field */
	    if (! elvin_notification_get(
		    notification,
		    F2_MIME_ARGS,
		    &found, &type, &value,
		    error))
	    {
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    /* Accept string attachments */
	    if (mime_type != NULL && found && type == ELVIN_STRING)
	    {
		length = header_length + strlen(value.s);
		if ((buffer = malloc(length + 1)) == NULL)
		{
		    length = 0;
		}
		else
		{
		    attachment = buffer;
		    snprintf(buffer, header_length + 1, ATTACHMENT_HEADER_FMT, mime_type);
		    strcpy(buffer + header_length, value.s);
		}
	    }
	    /* And accept opaque attachments */
	    else if (mime_type != NULL && found && type == ELVIN_OPAQUE)
	    {
		length = header_length + value.o.length;
		if ((buffer = malloc(length + 1)) == NULL)
		{
		    length = 0;
		}
		else
		{
		    attachment = buffer;
		    snprintf(buffer, header_length + 1, ATTACHMENT_HEADER_FMT, mime_type);
		    memcpy(buffer + header_length, value.o.data, value.o.length);
		}
	    }
	    /* But not other kinds */
	    else
	    {
		mime_args.data = NULL;
		mime_args.length = 0;
	    }
	}
    }

    /* Get the `Replaces' field from the notification */
    if (! elvin_notification_get_string(notification, F3_REPLACES, &found, &tag, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Try the backward compatible `REPLACEMENT' field */
    if (! found)
    {
	if (! elvin_notification_get_string(notification, F2_REPLACEMENT, NULL, &tag, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed\n");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}
    }

    /* Get the `Message-Id' from the notification */
    if (! elvin_notification_get_string(notification, F3_MESSAGE_ID, NULL, &message_id, error)) {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Get the `In-Reply-To' field from the notification */
    if (! elvin_notification_get_string(notification, F3_IN_REPLY_TO, NULL, &reply_id, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Get the `Thread-Id' field from the notification */
    if (! elvin_notification_get_string(notification, F3_THREAD_ID, NULL, &thread_id, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Construct a message */
    message = message_alloc(
	self -> name,
	self -> name, user, text, (unsigned long) timeout,
	attachment, length,
	tag, message_id,
	reply_id, thread_id);

    /* Deliver the message */
    (*self -> callback)(self -> rock, message, self -> has_nazi);

    /* Clean up */
    message_free(message);

    if (buffer != NULL)
    {
	free(buffer);
    }

    return 1;
}
#else
#error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */

/* Sends a message_t using the receiver's information */
static void send_message(group_sub_t self, message_t message)
{
    elvin_notification_t notification;
    unsigned int timeout;
    char *message_id;
    char *reply_id;
    char *thread_id;
    char *attachment;
    uint32_t length;
    char *buffer = NULL;
    char *mime_type;
    char *mime_args;
    
    /* Pull information out of the message */
    timeout = message_get_timeout(message);
    message_id = message_get_id(message);
    reply_id = message_get_reply_id(message);
    thread_id = message_get_thread_id(message);
    length = message_get_attachment(message, &attachment);
    message_decode_attachment(message, &mime_type, &mime_args);

#if ! defined(ELVIN_VERSION_AT_LEAST)
    /* Allocate a new notification */
    if ((notification = elvin_notification_alloc(self -> error)) == NULL)
    {
	fprintf(stderr, PACKAGE ": elvin_notification_alloc() failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
    /* Allocate a new notification */
    if ((notification = elvin_notification_alloc(client, self -> error)) == NULL)
    {
	fprintf(stderr, PACKAGE ": elvin_notification_alloc(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }
#else
#error "Unsupported Elvin library version"
#endif

    /* Add an xtickertape user agent tag */
    if (elvin_notification_add_string(
	    notification,
	    F3_USER_AGENT,
	    PACKAGE "-" VERSION,
	    self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the `Group' field and the backward compatible `TICKERTAPE'
     * field */
    if (elvin_notification_add_string(
	    notification,
	    F3_GROUP,
	    self -> name,
	    self -> error) == 0 ||
	elvin_notification_add_string(
	    notification,
	    F2_TICKERTAPE,
	    self -> name,
	    self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the `From' field and the backward compatible `USER'
     * field */
    if (elvin_notification_add_string(
	    notification,
	    F3_FROM,
	    message_get_user(message),
	    self -> error) == 0 ||
	elvin_notification_add_string(
	    notification,
	    F2_USER,
	    message_get_user(message),
	    self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the `Message' field and the backward compatible
     * `TICKERTEXT' field */
    if (elvin_notification_add_string(
	    notification,
	    F3_MESSAGE,
	    message_get_string(message),
	    self -> error) == 0 ||
	elvin_notification_add_string(
	    notification,
	    F2_TICKERTEXT,
	    message_get_string(message),
	    self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the `Timeout' field and the backward compatible `TIMEOUT'
     * field */
    if (elvin_notification_add_int32(
	    notification,
	    F3_TIMEOUT,
	    timeout,
	    self -> error) == 0 ||
	elvin_notification_add_int32(
	    notification,
	    F2_TIMEOUT,
	    timeout,
	    self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_int32(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the attachment if one was provided */
    if (attachment != NULL)
    {
	if (elvin_notification_add_opaque(
		notification,
		F3_MIME_ATTACHMENT,
		attachment,
		length,
		self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_opaque(): failed\n");
	    elvin_error_fprintf(stderr, self -> error);
	    exit(1);
	}
    }

    if (mime_args != NULL)
    {
	if (elvin_notification_add_string(
		notification,
		F2_MIME_ARGS,
		mime_args,
		self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    elvin_error_fprintf(stderr, self -> error);
	    exit(1);
	}

	free(mime_args);
    }

    if (mime_type != NULL)
    {
	if (elvin_notification_add_string(
		notification,
		F2_MIME_TYPE,
		mime_type,
		self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    elvin_error_fprintf(stderr, self -> error);
	    exit(1);
	}

	free(mime_type);
    }

    /* Add the Message-ID field */
    if (elvin_notification_add_string(
	notification,
	F3_MESSAGE_ID,
	message_id,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }


    /* Add the In-Reply-To field if relevant */
    if (reply_id != NULL)
    {
	if (elvin_notification_add_string(
	    notification,
	    F3_IN_REPLY_TO,
	    reply_id,
	    self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    elvin_error_fprintf(stderr, self -> error);
	    exit(1);
	}
    }

    /* Add the Thread-Id field if relevant */
    if (thread_id != NULL)
    {
	if (elvin_notification_add_string(
		notification,
		F3_THREAD_ID,
		thread_id,
		self -> error) == 0)
	{
	    fprintf(stderr, "elvin_notification_add_string(): failed\n");
	    elvin_error_fprintf(stderr, self -> error);
	    exit(1);
	}
    }

    /* Send the notification */
    elvin_async_notify(
	self -> handle,
	notification, 
	(self -> notification_keys == NULL) ? 1 : 0,
	self -> notification_keys,
	self -> error);

    /* And clean up */
    elvin_notification_free(notification, self -> error);
    if (buffer != NULL)
    {
	free(buffer);
    }
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
    elvin_keys_t notification_keys,
    elvin_keys_t subscription_keys,
    group_sub_callback_t callback,
    void *rock)
{
    group_sub_t self;

    /* Allocate memory for the new group_sub_t */
    if ((self = (group_sub_t)malloc(sizeof(struct group_sub))) == NULL)
    {
	return NULL;
    }
    memset(self, 0, sizeof(struct group_sub));

    /* Copy the name string */
    if ((self -> name = strdup(name)) == NULL)
    {
	group_sub_free(self);
	return NULL;
    }

    /* Copy the subscription expression */
    if ((self -> expression = strdup(expression)) == NULL)
    {
	group_sub_free(self);
	return NULL;
    }

    /* Copy the notification keys (if there are any) */
    if (notification_keys != NULL)
    {
	self -> notification_keys = elvin_keys_clone(notification_keys, NULL);
    }

    /* Copy the subscription keys (if there are any) */
    if (subscription_keys != NULL)
    {
	self -> subscription_keys = elvin_keys_clone(subscription_keys, NULL);
    }

    /* Copy the rest of the initializers */
    self -> in_menu = in_menu;
    self -> has_nazi = has_nazi;
    self -> min_time = min_time;
    self -> max_time = max_time;
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
	self -> name = NULL;
    }

    if (self -> expression)
    {
	free(self -> expression);
	self -> expression = NULL;
    }

    if (self -> notification_keys)
    {
	elvin_keys_free(self -> notification_keys, NULL);
	self -> notification_keys = NULL;
    }

    if (self -> subscription_keys)
    {
	elvin_keys_free(self -> subscription_keys, NULL);
	self -> subscription_keys = NULL;
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

/* Compares two key blocks (possibly NULL) and returns whether they
 * are equivalent.  It would be convenient if libelvin could do
 * this. */
static int keys_equal(elvin_keys_t self, elvin_keys_t keys)
{
#if defined(ELVIN_VERSION_AT_LEAST)
#if ELVIN_VERSION_AT_LEAST(4, 1, -1)
    int matched;
#else
#error "Unsupported Elvin library version"
#endif
#endif

    /* Shortcut: if self is NULL then the decision is equal */
    if (self == NULL)
    {
	return keys == NULL ? 1 : 0;
    }

    /* If keys is NULL then they can't match */
    if (keys == NULL)
    {
	return 0;
    }

    /* Are the key blocks subsets of each other? */
#if ! defined(ELVIN_VERSION_AT_LEAST)
    return elvin_keys_contains_all(self, keys, NULL) &&
	elvin_keys_contains_all(keys, self, NULL);
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
    /* Is self a superset of keys? */
    if (! elvin_keys_contains_all(self, keys, &matched, NULL))
    {
	return -1;
    }

    if (! matched)
    {
	return 0;
    }

    /* Is keys a superset of self? */
    if (! elvin_keys_contains_all(keys, self, &matched, NULL))
    {
	return -1;
    }

    return matched;
#else
#error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */
}

/* Updates the receiver to look just like subscription in terms of
 * name, in_menu, autoMime, min_time, max_time, keys, callback and
 * rock, but NOT expression */
void group_sub_update_from_sub(group_sub_t self, group_sub_t subscription)
{
    int sub_changed = 0;

    /* Release any dynamically allocated strings in the receiver */
    free(self -> name);

    /* Compare the notification keys */
    if (! keys_equal(self -> notification_keys, subscription -> notification_keys))
    {
	sub_changed = 1;

	/* Free the old key block */
	if (self -> notification_keys != NULL)
	{
	    elvin_keys_free(self -> notification_keys, NULL);
	    self -> notification_keys = NULL;
	}

	/* Clone the new key block */
	if (subscription -> notification_keys != NULL)
	{
	    self -> notification_keys = elvin_keys_clone(
		subscription -> notification_keys, NULL);
	}
    }

    /* Compare the subscription keys */
    if (! keys_equal(self -> subscription_keys, subscription -> subscription_keys))
    {
	sub_changed = 1;

	/* Free the old key block */
	if (self -> subscription_keys != NULL)
	{
	    elvin_keys_free(self -> subscription_keys, NULL);
	    self -> subscription_keys = NULL;
	}

	/* Clone the new key block */
	if (subscription -> subscription_keys != NULL)
	{
	    self -> subscription_keys = elvin_keys_clone(
		subscription -> subscription_keys, NULL);
	}
    }

    /* Update the key blocks */
    if (sub_changed)
    {
	/* Modify the subscription on the server */
	/* FIX THIS: blech! */
	abort();
    }
    
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
static 
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
int
#endif
subscribe_cb(
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

    return ELVIN_RETURN_SUCCESS;
}

/* Callback for an unsubscribe request */
static 
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
int
#endif
unsubscribe_cb(
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

    return ELVIN_RETURN_SUCCESS;
}

/* Sets the receiver's connection */
void group_sub_set_connection(group_sub_t self, elvin_handle_t handle, elvin_error_t error)
{
    if ((self -> handle != NULL) && (self -> subscription != NULL))
    {
	if (elvin_async_delete_subscription(
	    self -> handle, self -> subscription,
	    unsubscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_async_delete_subscription(): failed\n");
	    exit(1);
	}

	self -> is_pending = 1;
    }

    self -> handle = handle;
    self -> error = error;

    if (self -> handle != NULL)
    {
	if (elvin_async_add_subscription(
	    self -> handle, self -> expression,
	    self -> subscription_keys,
	    (self -> subscription_keys == NULL) ? 1 : 0,
	    notify_cb, self,
	    subscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_async_add_subscription(): failed\n");
	    exit(1);
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
