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
static const char cvsid[] = "$Id: group_sub.c,v 1.52 2004/08/02 15:18:42 phelps Exp $";
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

/* libelvin compatibility hackery */
#if !defined(ELVIN_VERSION_AT_LEAST)
#  define ELVIN_RETURN_TYPE void
#  define ELVIN_RETURN_FAILURE
#  define ELVIN_RETURN_SUCCESS
#  define ELVIN_SHA1_DIGESTLEN SHA1DIGESTLEN
#  define elvin_sha1_digest(client, data, length, public_key, error) \
    elvin_sha1digest(data, length, public_key)
#  define ELVIN_NOTIFICATION_ALLOC(client, error) \
    elvin_notification_alloc(error)
#  define ELVIN_KEYS_ALLOC(client, error) \
    elvin_keys_alloc(error)
#  define ELVIN_KEYS_ADD(keys, scheme, key_set_index, bytes, length, rock, error) \
    elvin_keys_add(keys, scheme, key_set_index, bytes, length, error)
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
#  define ELVIN_RETURN_TYPE int
#  define ELVIN_RETURN_FAILURE 0
#  define ELVIN_RETURN_SUCCESS 1
#  define ELVIN_NOTIFICATION_ALLOC elvin_notification_alloc
#  define ELVIN_KEYS_ALLOC elvin_keys_alloc
#  define ELVIN_KEYS_ADD elvin_keys_add
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

#if 0
/* Adds a subscription key to an elvin_keys_t */
static void add_named_key(
    elvin_keys_t keys,
    int has_private_key,
    int is_for_notify,
    key_table_t key_table,
    char *name)
{
    char *data;
    int length;
    int is_private;
    char digest[ELVIN_SHA1_DIGESTLEN];

    /* Look up the key */
    if (key_table_lookup(key_table, name, &data, &length, &is_private) < 0)
    {
        printf("unknown key %s\n", name);
        return;
    }

    if (is_private)
    {
        if (! ELVIN_KEYS_ADD(
                keys,
                ELVIN_KEY_SCHEME_SHA1_DUAL,
                is_for_notify ? 
                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX :
                ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX,
                data, length, NULL,
                NULL))
        {
            fprintf(stderr, "elvin_keys_add failed for key %s\n", name);
        }

        /* Digest it to get a public key */
        if (! elvin_sha1_digest(client, data, length, digest, NULL))
        {
            abort();
        }

        /* Arrange to add the public key too */
        data = digest;
        length = ELVIN_SHA1_DIGESTLEN;
    }

    /* Add the public key */
    if (has_private_key)
    {
        /* We needn't bother adding the public key to the SHA1 dual
         * block if we don't have a private key since we can't match
         * without one */
        if (! ELVIN_KEYS_ADD(
                keys,
                ELVIN_KEY_SCHEME_SHA1_DUAL,
                is_for_notify ?
                ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX :
                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
                data, length, NULL,
                NULL))
        {
            fprintf(stderr, "elvin_keys_add failed for key %s\n", name);
        }
    }

    /* Add it as a normal key too */
    if (! ELVIN_KEYS_ADD(
            keys,
            is_for_notify ?
            ELVIN_KEY_SCHEME_SHA1_CONSUMER :
            ELVIN_KEY_SCHEME_SHA1_PRODUCER,
            is_for_notify ?
            ELVIN_KEY_SHA1_PRODUCER_INDEX :
            ELVIN_KEY_SHA1_CONSUMER_INDEX,
            data, length, NULL,
            NULL))
    {
        fprintf(stderr, "elvin_keys_add failed for key %s\n", name);
    }
}
#endif

#if 0
static void remove_named_key(
    elvin_keys_t keys,
    int has_private_key,
    int is_for_notify,
    key_table_t key_table,
    char *name)
{
    char *data;
    int length;
    int is_private;
    char digest[ELVIN_SHA1_DIGESTLEN];

    /* Look up the key */
    if (key_table_lookup(key_table, name, &data, &length, &is_private) < 0)
    {
        printf("unknown key %s\n", name);
        return;
    }

    if (is_private)
    {
        if (! elvin_keys_remove(
                keys,
                ELVIN_KEY_SCHEME_SHA1_DUAL,
                is_for_notify ?
                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX :
                ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX,
                data, length, NULL))
        {
            fprintf(stderr, "elvin_keys_remove failed for key %s\n", name);
        }

        /* Digest it to get the public key */
        if (! elvin_sha1_digest(client, data, length, digest, NULL))
        {
            abort();
        }

        /* Arrange to remove the public key too */
        data = digest;
        length = ELVIN_SHA1_DIGESTLEN;
    }

    /* Remove the public key */
    if (has_private_key)
    {
        /* We won't have registered the public key with the SHA1 dual
         * block unless we have at least one private key to make a
         * match possible. */
        if (! elvin_keys_remove(
                keys,
                ELVIN_KEY_SCHEME_SHA1_DUAL,
                is_for_notify ?
                ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX :
                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
                data, length, NULL))
        {
            fprintf(stderr, "elvin_keys_remove failed for key %s\n", name);
        }
    }

    /* Remove the normal key */
    if (! elvin_keys_remove(
            keys,
            ELVIN_KEY_SCHEME_SHA1_PRODUCER,
            is_for_notify ?
            ELVIN_KEY_SHA1_CONSUMER_INDEX :
            ELVIN_KEY_SHA1_PRODUCER_INDEX,
            data, length, NULL))
    {
        fprintf(stderr, "elvin_keys_remove failed for key %s\n", name);
    }
}
#endif

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
    if (elvin_notification_get(notification, F3_MIME_ATTACHMENT, &type, &value, error))
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
    char *attachment;
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
    elvin_keys_t keys;
    int i;
    
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

#if 0
    /* Put together the list of keys */
    if (self -> key_count == 0)
    {
        keys = NULL;
    }
    else
    {
        if ((keys = ELVIN_KEYS_ALLOC(client, self -> error)) == NULL)
        {
            fprintf(stderr, "elvin_keys_alloc failed\n");
            elvin_error_fprintf(stderr, self -> error);
            abort();
        }

        for (i = 0; i < self -> key_count; i++)
        {
            add_named_key(
                keys,
                self -> has_private_key, 1,
                self -> key_table, self -> key_names[i]);
        }
    }

#else
    key_table_diff(
        NULL, NULL, 0,
        self -> key_table, self -> key_names, self -> key_count,
        1, &keys, NULL);
#endif

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


/* Returns the index of name is in key_names, or -1 if not present */
static int key_index(char **key_names, int key_count, char *name)
{
    int i;

    for (i = 0; i < key_count; i++)
    {
        if (strcmp(key_names[i], name) == 0)
        {
            return i;
        }
    }

    return -1;
}

#if 0
/* Updates a group sub's keys to match a new list */
static void group_sub_update_keys(
    group_sub_t self,
    char **key_names,
    int key_count,
    elvin_keys_t *keys_to_add_out,
    elvin_keys_t *keys_to_remove_out)
{
    char **old_keys;
    int old_count;
    int index;
    int i;
    char *data;
    int length;
    int is_private;
    int has_private_key;

    /* Decide whether or not any of the key_names refers to a private key */
    has_private_key = 0;
    for (i = 0; i < key_count; i++)
    {
        if (key_table_lookup(
                self -> key_table, key_names[i],
                NULL, NULL, &is_private) < 0)
        {
            continue;
        }

        if (is_private)
        {
            has_private_key = 1;
            break;
        }
    }

    /* Remember our old set of keys */
    old_keys = self -> key_names;
    old_count = self -> key_count;

    /* Allocate memory for a new set */
    if (!key_count)
    {
        self -> key_names = NULL;
    }
    else
    {
        if ((self -> key_names = (char **)malloc(key_count * sizeof(char *))) == NULL)
        {
            abort();
        }
    }

    /* Go through the new set of keys */
    for (i = 0; i < key_count; i++)
    {
        /* See if we have this key */
        index = key_index(old_keys, old_count, key_names[i]);

        if (index < 0)
        {
            /* New key */
            if ((self -> key_names[i] = strdup(key_names[i])) == NULL)
            {
                abort();
            }

            /* Are we returning keys to add */
            if (keys_to_add_out)
            {
                /* Make sure we have a table */
                if (! (*keys_to_add_out))
                {
                    if ((*keys_to_add_out = ELVIN_KEYS_ALLOC(client, NULL)) == NULL)
                    {
                        abort();
                    }
                }

                /* Add the key */
                add_named_key(*keys_to_add_out,
                              has_private_key, 0,
                              self -> key_table, key_names[i]);
            }
        }
        else
        {
            self -> key_names[i] = old_keys[index];
            old_keys[index] = old_keys[--old_count];

            /* Will we need to add a public key? */
            if (! self -> has_private_key && has_private_key)
            {
                /* Are we returning keys to add? */
                if (keys_to_add_out)
                {
                    /* Make sure we have a table */
                    if (! (*keys_to_add_out))
                    {
                        if ((*keys_to_add_out = ELVIN_KEYS_ALLOC(client, NULL)) == NULL)
                        {
                            abort();
                        }
                    }

                    /* Look up the key */
                    if (key_table_lookup(
                            self -> key_table, key_names[i],
                            &data, &length, &is_private) < 0) {
                        fprintf(stderr, "Group %s refers to unknown key %s\n",
                                self -> name, key_names[i]);
                    }
                    else
                    {
                        /* Sanity check: we shouldn't get here if the key is private */
                        assert(!is_private);

                        /* Add the key to the SHA1 dual key block */
                        if (!ELVIN_KEYS_ADD(
                                *keys_to_add_out,
                                ELVIN_KEY_SCHEME_SHA1_DUAL,
                                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
                                data, length, NULL,
                                NULL))
                        {
                            fprintf(stderr, "elvin_keys_add_failed for group %s key %s\n",
                                    self -> name, key_names[i]);
                        }
                    }
                }
            }
            else if (self -> has_private_key && ! has_private_key)
            {
                /* Are we returning keys to remove? */
                if (keys_to_remove_out)
                {
                    /* Make sure we have a table */
                    if (! (*keys_to_remove_out))
                    {
                        if ((*keys_to_remove_out = ELVIN_KEYS_ALLOC(client, NULL)) == NULL)
                        {
                            abort();
                        }
                    }

                    /* Look up the key */
                    if (key_table_lookup(
                            self -> key_table, key_names[i],
                            &data, &length, &is_private) < 0)
                    {
                        fprintf(stderr, "Group %s refers to unknown key %s\n",
                                self -> name, key_names[i]);
                    }
                    else
                    {
                        /* Sanity check: we shouldn't get here if the key is private */
                        assert(!is_private);

                        /* Remove the key from the SHA1 dual block */
                        if (!ELVIN_KEYS_ADD(
                                *keys_to_remove_out,
                                ELVIN_KEY_SCHEME_SHA1_DUAL,
                                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
                                data, length, NULL,
                                NULL))
                        {
                            fprintf(stderr, "elvin_keys_remove failed for gorup %s key %s\n",
                                    self -> name, key_names[i]);
                        }
                    }
                }
            }
        }
    }
    self -> key_count = key_count;

    /* Discard the old keys */
    for (i = 0; i < old_count; i++)
    {
        /* Are we returning keys to remove? */
        if (keys_to_remove_out)
        {
            /* Make sure we have a table */
            if (! (*keys_to_remove_out))
            {
                if ((*keys_to_remove_out = ELVIN_KEYS_ALLOC(client, NULL)) == NULL)
                {
                    abort();
                }
            }

            add_named_key(
                *keys_to_remove_out,
                self -> has_private_key, 0,
                self -> key_table, old_keys[i]);
        }

        free(old_keys[i]);
    }

    self -> has_private_key = has_private_key;

    /* Clean up */
    if (old_keys)
    {
        free(old_keys);
    }
}
#else
/* Updates a group sub's keys to match a new list */
static void group_sub_update_keys(
    group_sub_t self,
    char **key_names,
    int key_count,
    elvin_keys_t *keys_to_add_out,
    elvin_keys_t *keys_to_remove_out)
{
    int i;

    /* Compute the required changes */
    key_table_diff(
        self -> key_table,
        self -> key_names,
        self -> key_count,
        self -> key_table,
        key_names,
        key_count,
        0,
        keys_to_add_out,
        keys_to_remove_out);

    /* Discard the old key names */
    for (i = 0; i < self -> key_count; i++)
    {
        free(self -> key_names[i]);
    }
    free(self -> key_names);

    /* Duplicate the new key names */
    if ((self -> key_names = (char **)malloc(key_count * sizeof(char *))) == NULL) 
    {
        abort();
    }

    self -> key_count = key_count;
    for (i = 0; i < key_count; i++)
    {
        if ((self -> key_names[i] = strdup(key_names[i])) == NULL)
        {
            abort();
        }
    }
}
#endif
                                 


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
    int i, is_private;

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

#if 0
            /* Look up the key */
            if (key_table_lookup(key_table, key_names[i], NULL, NULL, &is_private) < 0)
            {
                fprintf(stderr, "Unknown key \"%s\" in group \"%s\"\n", key_names[i], name);
            }
            else
            {
                self -> has_private_key |= is_private;
            }
#else
            self -> has_private_key = 1;
#endif
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
void group_sub_update_from_sub(group_sub_t self, group_sub_t subscription)
{
    char *expression = NULL;
    elvin_keys_t keys_to_add = NULL;
    elvin_keys_t keys_to_remove = NULL;
    int accept_insecure;

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

    group_sub_update_keys(self, subscription -> key_names, subscription -> key_count,
                          &keys_to_add, &keys_to_remove);
    accept_insecure = (self -> key_count == 0);

    /* Copy various fields */
    self -> in_menu = subscription -> in_menu;
    self -> has_nazi = subscription -> has_nazi;
    self -> min_time = subscription -> min_time;
    self -> max_time = subscription -> max_time;
    self -> callback = subscription -> callback;
    self -> rock = subscription -> rock;

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
    int i;

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
#if 0
        if (self -> key_count == 0)
        {
            keys = NULL;
        }
        else
        {
            if ((keys = ELVIN_KEYS_ALLOC(client, NULL)) == NULL)
            {
                abort();
            }

            for (i = 0; i < self -> key_count; i++)
            {
                add_named_key(
                    keys,
                    self -> has_private_key, 0,
                    self -> key_table, self -> key_names[i]);
            } 
        }
#else
        /* Compute the keys */
        key_table_diff(
            NULL, NULL, 0,
            self -> key_table, self -> key_names, self -> key_count,
            0, &keys, NULL);
#endif

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
