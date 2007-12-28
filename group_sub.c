/***********************************************************************

  Copyright (C) 1999-2004 by Mantara Software (ABN 17 105 665 594).
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

#ifndef lint
static const char cvsid[] = "$Id: group_sub.c,v 1.63 2007/12/28 12:46:52 phelps Exp $";
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
#include "key_table.h"
#include "group_sub.h"

#define F3_VERSION "org.tickertape.message"
#define F3_USER_AGENT "User-Agent"
#define F3_GROUP "Group"
#define F2_TICKERTAPE "TICKERTAPE"
#define F3_FROM "From"
#define F2_USER "USER"
#define F3_MESSAGE "Message"
#define F2_TICKERTEXT "TICKERTEXT"
#define F3_TIMEOUT "Timeout"
#define F2_TIMEOUT "TIMEOUT"
#define F3_REPLACEMENT_ID "Replacement-Id"
#define F2_REPLACEMENT "REPLACEMENT"
#define F3_MESSAGE_ID "Message-Id"
#define F3_IN_REPLY_TO "In-Reply-To"
#define F3_THREAD_ID "Thread-Id"
#define F3_ATTACHMENT "Attachment"
#define F2_MIME_ARGS "MIME_ARGS"
#define F2_MIME_TYPE "MIME_TYPE"

#define ATTACHMENT_HEADER_FMT \
    "MIME-Version: 1.0\n" \
    "Content-Type: %s; charset=us-ascii\n\n"

/* The size of the MIME args buffer */
#define BUFFER_SIZE (32)

/* libelvin compatibility hackery */
#if !defined(ELVIN_VERSION_AT_LEAST)
#  define ELVIN_RETURN_TYPE void
#  define ELVIN_RETURN_FAILURE
#  define ELVIN_RETURN_SUCCESS
#  define ELVIN_NOTIFICATION_ALLOC(client, error) \
    elvin_notification_alloc(error)
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
#  define ELVIN_RETURN_TYPE int
#  define ELVIN_RETURN_FAILURE 0
#  define ELVIN_RETURN_SUCCESS 1
#  define ELVIN_NOTIFICATION_ALLOC elvin_notification_alloc
#else
#  error "Unsupported libelvin version"
#endif


/* The group subscription data type */
struct group_sub
{
    /* The name of the receiver's group */
    char *name;

    /* The receiver's subscription expression */
    char *expression;

    /* The table of keys */
    key_table_t key_table;

    /* The names of the keys */
    char **key_names;

    /* The number of key names */
    int key_count;

    /* Non-zero if one of the key_names refers to a private key */
    int has_private_key;

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
    int version;

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }

    /* See if there's a version number */
    if (elvin_notification_get(notification, F3_VERSION, &type, &value, error) &&
        type == ELVIN_INT32)
    {
        version = (int32_t)value.i;
    }
    else
    {
        version = -1;
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
                if (version < 3001 && value.i <= 60) 
                {
                    timeout = value.i * 60;
                }
                else
                {
                    timeout = value.i;
                }

		break;
	    }

	    case ELVIN_INT64:
	    {
                if (version < 3001 && value.h <= 60)
                {
                    timeout = (int)value.h * 60;
                }
                else
                {
                    timeout = (int)value.h;
                }

		break;
	    }

	    case ELVIN_REAL64:
	    {
                if (version < 3001 && value.d <= 60)
                {
                    timeout = (int)(0.5 + 60 * value.d);
                }
                else
                {
                    timeout = (int)(0.5 + value.d);
                }

		break;
	    }

	    case ELVIN_STRING:
	    {
                timeout = atoi((char *)value.s);
                if (version < 3001 && timeout <= 60)
                {
                    timeout *= 60;
                }

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
		timeout = value.i * 60;
		break;
	    }

	    case ELVIN_INT64:
	    {
		timeout = (int)value.h * 60;
		break;
	    }

	    case ELVIN_REAL64:
	    {
		timeout = (int)(0.5 + value.d * 60);
		break;
	    }

	    case ELVIN_STRING:
	    {
		timeout = atoi((char *)value.s) * 60;
		break;
	    }

	    default:
	    {
		break;
	    }
	}
    }

    /* If the timeout was illegible, then set it to 10 minutes */
    timeout = (timeout == 0) ? 10 * 60 : timeout;

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
    if (elvin_notification_get(notification, F3_ATTACHMENT, &type, &value, error))
    {
	if (type == ELVIN_STRING)
	{
	    attachment = value.s;
	    length = strlen(value.s);
	}
	else if (type == ELVIN_OPAQUE)
	{
	    attachment = value.o.data;
	    length = value.o.length;
	}
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
    if (elvin_notification_get(notification, F3_REPLACEMENT_ID, &type, &value, error) &&
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
    int32_t version = -1;
    int timeout = 0;
    char *attachment = NULL;
    uint32_t length = 0;
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

    /* Get the 'org.tickertape.message' field */
    if (! elvin_notification_get_int32(notification, F3_VERSION, &found, &version, error))
    {
        fprintf(stderr, "elvin_notification_get_int32(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Get the `From' field from the notification */
    if (! elvin_notification_get_string(notification, F3_FROM, &found, &user, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
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
                if (version < 3001 && value.i <= 60)
                {
                    timeout = value.i * 60;
                }
                else
                {
                    timeout = value.i;
                }

		break;
	    }

	    case ELVIN_INT64:
	    {
                if (version < 3001 && value.h <= 60)
                {
                    timeout = (int)value.h * 60;
                }
                else
                {
                    timeout = (int)value.h;
                }

		break;
	    }

	    case ELVIN_REAL64:
	    {
                if (version < 3001 && value.d <= 60)
                {
                    timeout = (int)(0.5 + 60 * value.d);
                }
                else
                {
                    timeout = (int)(0.5 + value.d);
                }

		break;
	    }

	    case ELVIN_STRING:
	    {
		timeout = atoi(value.s);
                if (version < 3001 && timeout < 60)
                {
                    timeout *= 60;
                }

		break;
	    }

	    default:
	    {
		break;
	    }
	}
    }

    /* If the timeout was zero, then set it to 10 minutes */
    timeout = (timeout == 0) ? 600 : timeout;

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
    if (!elvin_notification_get(notification, F3_ATTACHMENT, &found, &type, &value, error))
    {
        fprintf(stderr, "elvin_notification_get(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    if (found)
    {
        if (type == ELVIN_STRING)
        {
            attachment = value.s;
            length = strlen(value.s);
        }
        else if (type == ELVIN_OPAQUE)
        {
            attachment = value.o.data;
            length = value.o.length;
        }
    }
    else
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
    if (! elvin_notification_get_string(notification, F3_REPLACEMENT_ID, &found, &tag, error))
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
    elvin_keys_t keys;
    
    /* Pull information out of the message */
    timeout = message_get_timeout(message);
    message_id = message_get_id(message);
    reply_id = message_get_reply_id(message);
    thread_id = message_get_thread_id(message);
    length = message_get_attachment(message, &attachment);
    message_decode_attachment(message, &mime_type, &mime_args);

    /* Allocate a new notification */
    if ((notification = ELVIN_NOTIFICATION_ALLOC(client, self -> error)) == NULL)
    {
	fprintf(stderr, PACKAGE ": elvin_notification_alloc(): failed\n");
	elvin_error_fprintf(stderr, self -> error);
	exit(1);
    }

    /* Add the xtickertape version number */
    if (elvin_notification_add_int32(
            notification,
            F3_VERSION,
            3001,
            self -> error) == 0)
    {
        fprintf(stderr, "elvin_notification_add_int32(): failed\n");
        elvin_error_fprintf(stderr, self -> error);
        exit(1);
    }

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
	    timeout == 60 ? 61 : timeout,
	    self -> error) == 0 ||
	elvin_notification_add_int32(
	    notification,
	    F2_TIMEOUT,
	    timeout / 60,
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
		F3_ATTACHMENT,
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

    /* Look up the keys to use */
    key_table_diff(
        NULL, NULL, 0,
        self -> key_table, self -> key_names, self -> key_count,
        1, &keys, NULL);

    if (!elvin_async_notify(
            self -> handle,
            notification,
            (self -> key_count == 0), keys,
            self -> error))
    {
        fprintf(stderr, "elvin_async_notify failed\n");
        elvin_error_fprintf(stderr, self -> error);
        elvin_error_clear(self -> error);
    }

    /* And clean up */
    elvin_notification_free(notification, self -> error);
    if (buffer != NULL)
    {
	free(buffer);
    }

    if (keys)
    {
        if (! elvin_keys_free(keys, self -> error))
        {
            fprintf(stderr, "elvin_keys_free failed\n");
            elvin_error_fprintf(stderr, self -> error);
            abort();
        }
    }
}


/* Updates a group sub's keys to match a new list */
static void group_sub_update_keys(
    group_sub_t self,
    key_table_t old_keys,
    key_table_t new_keys,
    char **key_names,
    int key_count,
    elvin_keys_t *keys_to_add_out,
    elvin_keys_t *keys_to_remove_out)
{
    char **old_names;
    int i;

    self -> key_table = new_keys;

    /* Compute the required changes */
    key_table_diff(
        old_keys,
        self -> key_names,
        self -> key_count,
        new_keys,
        key_names,
        key_count,
        0,
        keys_to_add_out,
        keys_to_remove_out);

    /* Hang on to the old key names */
    old_names = self -> key_names;

    /* Duplicate the new key names */
    if (key_count == 0)
    {
        self -> key_names = NULL;
    }
    else if ((self -> key_names = (char **)malloc(key_count * sizeof(char *))) == NULL) 
    {
        abort();
    }

    for (i = 0; i < key_count; i++)
    {
        if ((self -> key_names[i] = strdup(key_names[i])) == NULL)
        {
            abort();
        }
    }

    /* Discard the old key names */
    for (i = 0; i < self -> key_count; i++)
    {
        free(old_names[i]);
    }

    if (old_names != NULL) {
        free(old_names);
    }

    /* Record the new number of keys. */
    self -> key_count = key_count;
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
    key_table_t key_table,
    char **key_names,
    int key_count,
    group_sub_callback_t callback,
    void *rock)
{
    group_sub_t self;
    int i;

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

    /* Allocate room for a copy of the list of key names */
    if (key_count)
    {
        if ((self -> key_names = (char **)malloc(key_count * sizeof(char *))) == NULL)
        {
            abort();
        }

        /* Copy each key name */
        for (i = 0; i < key_count; i++)
        {
            if ((self -> key_names[i] = strdup(key_names[i])) == NULL)
            {
                abort();
            }
        }

        self -> key_count = key_count;
    }

    /* Copy the rest of the initializers */
    self -> key_table = key_table;
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
    int i;

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

    if (self -> key_names)
    {
        for (i = 0; i < self -> key_count; i++)
        {
            free(self -> key_names[i]);
        }
        free(self -> key_names);
        self -> key_names = NULL;
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
 * name, expression, in_menu, has_nazi, min_time, max_time, keys,
 * callback and rock */
void group_sub_update_from_sub(
    group_sub_t self,
    group_sub_t subscription,
    key_table_t old_keys,
    key_table_t new_keys)
{
    char *expression = NULL;
    elvin_keys_t keys_to_add = NULL;
    elvin_keys_t keys_to_remove = NULL;
    int accept_insecure;

    if (self != subscription)
    {
        /* Update the subscription name */
        if (strcmp(self -> name, subscription -> name) != 0)
        {
            free(self -> name);
            if ((self -> name = strdup(subscription -> name)) == NULL)
            {
                /* FIX THIS: error reporting?! */
                abort();
            }
        }

        /* Update the expression if it has changed */
        if (strcmp(self -> expression, subscription -> expression) != 0)
        {
            free(self -> expression);
            if ((self -> expression = strdup(subscription -> expression)) == NULL)
            {
                /* FIX THIS: error reporting? */
                abort();
            }

            /* We have a new epxression */
            expression = self -> expression;
        }

        /* Copy various fields */
        self -> in_menu = subscription -> in_menu;
        self -> has_nazi = subscription -> has_nazi;
        self -> min_time = subscription -> min_time;
        self -> max_time = subscription -> max_time;
        self -> callback = subscription -> callback;
        self -> rock = subscription -> rock;
    }

    group_sub_update_keys(self,
                          old_keys,
                          new_keys,
                          subscription -> key_names,
                          subscription -> key_count,
                          &keys_to_add, &keys_to_remove);
    accept_insecure = (self -> key_count == 0);

    /* Update the subscription if necessary */
    if (expression != NULL ||
        keys_to_add != NULL ||
        keys_to_remove != NULL)
    {
	/* Modify the subscription on the server */
        if (! elvin_async_modify_subscription(
                self -> handle,
                self -> subscription,
                expression,
                keys_to_add,
                keys_to_remove,
                &accept_insecure,
                NULL, NULL,
                NULL, NULL,
                NULL))
        {
            fprintf(stderr, "elvin_async_modify_subscription failed\n");
            abort();
        }
    }

    /* Clean up */
    if (keys_to_add)
    {
        elvin_keys_free(keys_to_add, NULL);
    }

    if (keys_to_remove)
    {
        elvin_keys_free(keys_to_remove, NULL);
    }
}


/* Callback for a subscribe request */
static ELVIN_RETURN_TYPE subscribe_cb(
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
static ELVIN_RETURN_TYPE unsubscribe_cb(
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
void group_sub_set_connection(
    group_sub_t self,
    elvin_handle_t handle,
    elvin_error_t error)
{
    elvin_keys_t keys;

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
        /* Compute the keys */
        key_table_diff(
            NULL, NULL, 0,
            self -> key_table, self -> key_names, self -> key_count,
            0, &keys, NULL);

        if (!elvin_async_add_subscription(
                self -> handle,
                self -> expression,
                keys, (self -> key_count == 0),
                notify_cb, self,
                subscribe_cb, self,
                error))
        {
            fprintf(stderr, "elvin_async_add_subscription() failed\n");
            elvin_error_fprintf(stderr, error);
        }

        if (keys)
        {
            if (!elvin_keys_free(keys, error))
            {
                fprintf(stderr, "elvin_keys_free failed\n");
                elvin_error_fprintf(stderr, error);
                exit(1);
            }
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
