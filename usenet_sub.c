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

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: usenet_sub.c,v 1.39 2004/08/02 22:24:17 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* fprintf, snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* exit, free, malloc, realloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* strlen */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "replace.h"
#include "usenet_sub.h"

/* Some notification field names */
#define BODY "BODY"
#define FROM_NAME "FROM_NAME"
#define FROM_EMAIL "FROM_EMAIL"
#define FROM "From"
#define KEYWORDS "KEYWORDS"
#define MESSAGE_ID "Message-ID"
#define MIME_ARGS "MIME_ARGS"
#define MIME_TYPE "MIME_TYPE"
#define NEWSGROUPS "NEWSGROUPS"
#define SUBJECT "SUBJECT"
#define URL_MIME_TYPE "x-elvin/url"
#define NEWS_MIME_TYPE "x-elvin/news"
#define XPOSTS "CROSS_POSTS"
#define X_NNTP_HOST "X-NNTP-Host"

#define F_MATCHES "regex(%s, \"%s\")"
#define F_NOT_MATCHES "!regex(%s, \"%s\")"
#define F_EQ "%s == %s"
#define F_STRING_EQ "%s == \"%s\""
#define F_NEQ "%s != %s"
#define F_STRING_NEQ "%s != \"%s\""
#define F_LT "%s < %s"
#define F_GT "%s > %s"
#define F_LE "%s <= %s"
#define F_GE "%s >= %s"

#define ONE_SUB "ELVIN_CLASS == \"NEWSWATCHER\" && (%s)"


#define PATTERN_ONLY "%sregex(NEWSGROUPS, \"%s\")"
#define PATTERN_PLUS "(%sregex(NEWSGROUPS, \"%s\"))"

#define USENET_PREFIX "usenet: %s"
#define NEWS_URL "news://%s/%s"

#define ATTACHMENT_FMT \
    "MIME-Version: 1.0\n" \
    "Content-Type: %s; charset=us-ascii\n" \
    "\n" \
    "%s\n"

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
#  define ELVIN_RETURN_FAILURE
#  define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
#  define ELVIN_RETURN_FAILURE 0
#  define ELVIN_RETURN_SUCCESS 1
#endif

/* The structure of a usenet subscription */
struct usenet_sub
{
    /* The receiver's subscription expression */
    char *expression;

    /* The length of the subscription expression */
    size_t expression_size;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* The reciever's subscription */
    elvin_subscription_t subscription;

    /* The receiver's callback */
    usenet_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;

    /* Non-zero if the receiver is waiting on a change to the subscription */
    int is_pending;
};

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
    usenet_sub_t self = (usenet_sub_t)rock;
    message_t message;
    elvin_basetypes_t type;
    elvin_value_t value;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *attachment = NULL;
    size_t length;
    char *buffer = NULL;

    /* If we don't have a callback than bail out now */
    if (self -> callback == NULL)
    {
	return;
    }

    /* Get the newsgroups to which the message was posted */
    if (elvin_notification_get(notification, NEWSGROUPS, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	string = value.s;
    }
    else
    {
	string = "news";
    }

    /* Prepend `usenet:' to the beginning of the group field */
    length = strlen(USENET_PREFIX) + strlen(string) - 1;
    if ((newsgroups = (char *)malloc(length)) == NULL)
    {
	return;
    }

    snprintf(newsgroups, length, USENET_PREFIX, string);

    /* Get the name from the FROM_NAME field (if provided) */
    if (elvin_notification_get(notification, FROM_NAME, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	name = value.s;
    }
    else
    {
	/* If no FROM_NAME field then try FROM_EMAIL */
	if (elvin_notification_get(notification, FROM_EMAIL, &type, &value, error) &&
	    type == ELVIN_STRING)
	{
	    name = value.s;
	}
	else
	{
	    /* If no FROM_EMAIL then try FROM */
	    if (elvin_notification_get(notification, FROM, &type, &value, error) &&
		type == ELVIN_STRING)
	    {
		name = value.s;
	    }
	    else
	    {
		name = "anonymous";
	    }
	}
    }

    /* Get the SUBJECT field (if provided) */
    if (elvin_notification_get(notification, SUBJECT, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	subject = value.s;
    }
    else
    {
	subject = "[no subject]";
    }

    /* Get the MIME_ARGS field (if provided) */
    if (elvin_notification_get(notification, MIME_ARGS, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	mime_args = value.s;

	/* Get the MIME_TYPE field (if provided) */
	if (elvin_notification_get(notification, MIME_TYPE, &type, &value, error) &&
	    type == ELVIN_STRING)
	{
	    mime_type = value.s;
	}
	else
	{
	    mime_type = URL_MIME_TYPE;
	}
    }
    /* No MIME_ARGS provided.  Construct one using the Message-ID field */
    else
    {
	if (elvin_notification_get(notification, MESSAGE_ID, &type, &value, error) &&
	    type == ELVIN_STRING)
	{
	    char *message_id = value.s;
	    char *news_host;

	    if (elvin_notification_get(notification, X_NNTP_HOST, &type, &value, error)
		&& type == ELVIN_STRING)
	    {
		news_host = value.s;
	    }
	    else
	    {
		news_host = "news";
	    }

	    length = strlen(NEWS_URL) + strlen(news_host) + strlen(message_id) - 3;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		mime_type = NULL;
		mime_args = NULL;
	    }
	    else
	    {
		snprintf(buffer, length, NEWS_URL, news_host, message_id);
		mime_args = buffer;
		mime_type = NEWS_MIME_TYPE;
	    }
	}
	else
	{
	    mime_type = NULL;
	    mime_args = NULL;
	}
    }

    /* Convert the mime type and args into an attachment */
    if (mime_type == NULL || mime_args == NULL)
    {
	attachment = NULL;
	length = 0;
    }
    else
    {
	length = strlen(ATTACHMENT_FMT) + strlen(mime_type) + strlen(mime_args) - 4;
	if ((attachment = malloc(length + 1)) == NULL)
	{
	    length = 0;
	}
	else
	{
	    snprintf(attachment, length + 1, ATTACHMENT_FMT, mime_type, mime_args);
	}
    }

    /* Construct a message out of all of that */
    if ((message = message_alloc(
	NULL, newsgroups, name, subject, 300,
	attachment, length - 1,
 	NULL, NULL, NULL, NULL)) != NULL)
    {
	/* Deliver the message */
	(*self -> callback)(self -> rock, message, False);
	message_free(message);
    }

    /* Clean up */
    free(newsgroups);
    if (buffer != NULL)
    {
	free(buffer);
    }

    if (attachment != NULL)
    {
	free(attachment);
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
    usenet_sub_t self = (usenet_sub_t)rock;
    message_t message;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *buffer = NULL;
    char *attachment = NULL;
    size_t length = 0;
    int found;

    /* If we don't have a callback than bail out now */
    if (self -> callback == NULL)
    {
	return 1;
    }

    /* Get the newsgroups to which the message was posted */
    if (! elvin_notification_get_string(notification, NEWSGROUPS, &found, &string, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Use a reasonable default */
    string = found ? string : "news";

    /* Prepend `usenet:' to the beginning of the group field */
    length = strlen(USENET_PREFIX) + strlen(string) - 1;
    if ((newsgroups = (char *)malloc(length)) == NULL)
    {
	return 0;
    }

    snprintf(newsgroups, length, USENET_PREFIX, string);

    /* Get the name from the FROM_NAME field (if provided) */
    if (! elvin_notification_get_string(notification, FROM_NAME, &found, &name, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    if (! found)
    {
	/* No FROM_NAME field, so try FROM_EMAIL */
	if (! elvin_notification_get_string(notification, FROM_EMAIL, &found, &name, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed\n");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	if (! found)
	{
	    /* No FROM_EMAIL, so try FROM */
	    if (! elvin_notification_get_string(notification, FROM, &found, &name, error))
	    {
		fprintf(stderr, "elvin_notification_get_string(): failed\n");
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    if (! found)
	    {
		/* Give up */
		name = "anonymous";
	    }
	}
    }

    /* Get the SUBJECT field (if provided) */
    if (! elvin_notification_get_string(notification, SUBJECT, &found, &subject, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Use a default if none found */
    subject = found ? subject : "[no subject]";

    /* Get the MIME_ARGS field (if provided) */
    if (! elvin_notification_get_string(notification, MIME_ARGS, &found, &mime_args, error))
    {
	fprintf(stderr, "elvin_notification_get_string(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Was the MIME_ARGS field provided? */
    if (found)
    {
	/* Get the MIME_TYPE field (if provided) */
	if (! elvin_notification_get_string(notification, MIME_TYPE, &found, &mime_type, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed\n");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	/* Use a default if none found */
	mime_type = found ? mime_type : URL_MIME_TYPE;
    }
    else
    {
	char *message_id;

	/* No MIME_ARGS.  Look for a message-id */
	if (! elvin_notification_get_string( notification, MESSAGE_ID, &found, &message_id, error))
	{
	    fprintf(stderr, "elvin_notification_get_string(): failed\n");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	if (found)
	{
	    char *news_host;

	    /* Look up the news host field */
	    if (! elvin_notification_get_string(
		    notification,
		    X_NNTP_HOST,
		    &found, &news_host,
		    error))
	    {
		fprintf(stderr, "elvin_notification_get_string(): failed\n");
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    news_host = found ? news_host : "news";

	    length = strlen(NEWS_URL) + strlen(news_host) + strlen(message_id) - 3;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		mime_type = NULL;
		mime_args = NULL;
	    }
	    else
	    {
		snprintf(buffer, length, NEWS_URL, news_host, message_id);
		mime_args = buffer;
		mime_type = NEWS_MIME_TYPE;
	    }
	}
	else
	{
	    mime_type = NULL;
	    mime_args = NULL;
	}
    }

    /* Convert the mime type and args into an attachment */
    if (mime_type == NULL || mime_args == NULL)
    {
	attachment = NULL;
	length = 0;
    }
    else
    {
	length = strlen(ATTACHMENT_FMT) - 4 + strlen(mime_type) + strlen(mime_args);
	if ((attachment = malloc(length + 1)) == NULL)
	{
	    length = 0;
	}
	else
	{
	    snprintf(attachment, length + 1, ATTACHMENT_FMT, mime_type, mime_args);
	}
    }

    /* Construct a message out of all of that */
    if ((message = message_alloc(
	NULL, newsgroups, name, subject, 60,
	attachment, length,
 	NULL, NULL, NULL, NULL)) != NULL)
    {
	/* Deliver the message */
	(*self -> callback)(self -> rock, message, False);
	message_free(message);
    }

    /* Clean up */
    free(newsgroups);
    if (buffer != NULL)
    {
	free(buffer);
    }

    return 1;
}
#else
#error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */

/* Construct a portion of the subscription expression */
static char *alloc_expr(usenet_sub_t self, struct usenet_expr *expression)
{
    char *field_name;
    char *format;
    size_t format_length;
    size_t length;
    char *result;

    /* Get the string representation for the field */
    switch (expression -> field)
    {
	/* body -> BODY */
	case F_BODY:
	{
	    field_name = BODY;
	    break;
	}

	/* from -> FROM_NAME */
	case F_FROM:
	{
	    field_name = FROM_NAME;
	    break;
	}

	/* email -> FROM_EMAIL */
	case F_EMAIL:
	{
	    field_name = FROM_EMAIL;
	    break;
	}

	/* subject -> SUBJECT */
	case F_SUBJECT:
	{
	    field_name = SUBJECT;
	    break;
	}

	/* keywords -> KEYWORDS */
	case F_KEYWORDS:
	{
	    field_name = KEYWORDS;
	    break;
	}

	/* xposts -> CROSS_POSTS */
	case F_XPOSTS:
	{
	    field_name = XPOSTS;
	    break;
	}

	/* Should never get here */
	default:
	{
	    fprintf(stderr, PACKAGE ": internal error\n");
	    return NULL;
	}
    }

    /* Look up the string representation for the operator */
    switch (expression -> operator)
    {
	/* matches */
	case O_MATCHES:
	{
	    format = F_MATCHES;
	    format_length = sizeof(F_MATCHES);
	    break;
	}

	/* not [matches] */
	case O_NOT:
	{
	    format = F_NOT_MATCHES;
	    format_length = sizeof(F_NOT_MATCHES);
	    break;
	}

	/* = */
	case O_EQ:
	{
	    if (expression -> field == F_XPOSTS)
	    {
		format = F_EQ;
		format_length = sizeof(F_EQ);
		break;
	    }

	    format = F_STRING_EQ;
	    format_length = sizeof(F_STRING_EQ);
	    break;
	}

	/* != */
	case O_NEQ:
	{
	    if (expression -> field == F_XPOSTS)
	    {
		format = F_NEQ;
		format_length = sizeof(F_NEQ);
		break;
	    }

	    format = F_STRING_NEQ;
	    format_length = sizeof(F_STRING_NEQ);
	    break;
	}

	/* < */
	case O_LT:
	{
	    format = F_LT;
	    format_length = sizeof(F_LT);
	    break;
	}

	/* > */
	case O_GT:
	{
	    format = F_GT;
	    format_length = sizeof(F_GT);
	    break;
	}

	/* <= */
	case O_LE:
	{
	    format = F_LE;
	    format_length = sizeof(F_LE);
	    break;
	}

	/* >= */
	case O_GE:
	{
	    format = F_GE;
	    format_length = sizeof(F_GE);
	    break;
	}

	default:
	{
	    fprintf(stderr, PACKAGE ": internal error\n");
	    return NULL;
	}
    }

    /* Allocate space for the result */
    length = format_length + strlen(field_name) + strlen(expression -> pattern) - 3;
    if ((result = (char *)malloc(length)) == NULL)
    {
	return NULL;
    }

    /* Construct the string */
    snprintf(result, length, format, field_name, expression -> pattern);
    return result;
}

/* Construct a subscription expression for the given group entry */
static char *alloc_sub(
    usenet_sub_t self, int has_not, char *pattern,
    struct usenet_expr *expressions, size_t count)
{
    struct usenet_expr *pointer;
    char *not_string = has_not ? "!" : "";
    char *result;
    size_t length;

    /* Shortcut -- if there are no expressions then we don't need parens */
    if (count == 0)
    {
	length = strlen(PATTERN_ONLY) + strlen(not_string) + strlen(pattern) - 3;
	if ((result = (char *)malloc(length)) == NULL)
	{
	    return NULL;
	}

	snprintf(result, length, PATTERN_ONLY, not_string, pattern);
	return result;
    }

    /* Otherwise we have to do this the hard way.  Allocate space for
     * the initial pattern and then extend it for each of the expressions */
    length = strlen(PATTERN_PLUS) + strlen(not_string) + strlen(pattern) - 3;
    if ((result = (char *)malloc(length)) == NULL)
    {
	return NULL;
    }

    snprintf(result, length, PATTERN_PLUS, not_string, pattern);

    /* Insert the expressions */
    for (pointer = expressions; pointer < expressions + count; pointer++)
    {
	/* Get the string for the expression */
	char *expr = alloc_expr(self, pointer);
	size_t new_length = length + strlen(expr) + 4;

	/* Expand the buffer */
	if ((result = (char *)realloc(result, new_length)) == NULL)
	{
	    free(expr);
	    return NULL;
	}

	/* Overwrite the trailing right paren with the new stuff */
	snprintf(result + length - 2, new_length - length + 2, " && %s)", expr);
	length = new_length;
	free(expr);
    }

    return result;
}

/* Allocates and initializes a new usenet_sub_t */
usenet_sub_t usenet_sub_alloc(usenet_sub_callback_t callback, void *rock)
{
    usenet_sub_t self;

    /* Allocate memory for the new subscription */
    if ((self = (usenet_sub_t)malloc(sizeof(struct usenet_sub))) == NULL)
    {
	return NULL;
    }

    /* Initialize its contents */
    self -> expression = NULL;
    self -> expression_size = 0;
    self -> handle = NULL;
    self -> subscription = NULL;
    self -> callback = callback;
    self -> rock = rock;
    self -> is_pending = 0;
    return self;
}
    
/* Releases the resources consumed by the receiver */
void usenet_sub_free(usenet_sub_t self)
{
    /* Free the subscription expression */
    if (self -> expression != NULL)
    {
	free(self -> expression);
	self -> expression = NULL;
    }

    /* Don't free a pending subscription */
    if (self -> is_pending)
    {
	return;
    }

    /* It should be safe to free the subscription now */
    free(self);
}


/* Adds a new entry to the usenet subscription */
int usenet_sub_add(
    usenet_sub_t self,
    int has_not,
    char *pattern,
    struct usenet_expr *expressions,
    size_t count)
{
    char *entry_sub;
    size_t entry_length;

    /* Figure out what the new entry is */
    if ((entry_sub = alloc_sub(self, has_not, pattern, expressions, count)) == NULL)
    {
	return -1;
    }

    /* And how long it is */
    entry_length = strlen(entry_sub);

    /* If we had no previous expression, then simply create one */
    if (self -> expression == NULL)
    {
	self -> expression_size = strlen(ONE_SUB) + entry_length - 1;
	if ((self -> expression = (char *)malloc(self -> expression_size)) == NULL)
	{
	    free(entry_sub);
	    return -1;
	}

	snprintf(self -> expression, self -> expression_size, ONE_SUB, entry_sub);
    }
    /* Otherwise we need to extend the existing expression */
    else
    {
	size_t new_size;
	char *new_expression;

	/* Expand the expression buffer */
	new_size = self -> expression_size + entry_length + 4;
	if ((new_expression = (char *)realloc(self -> expression, new_size)) == NULL)
	{
	    return -1;
	}

	/* Overwrite the trailing right paren with the new stuff */
	snprintf(new_expression + self -> expression_size - 2,
		 new_size - self -> expression_size + 2,
		 " || %s)", entry_sub);
	self -> expression = new_expression;
	self -> expression_size = new_size;
    }

    /* If we're connected then resubscribe */
    if (self -> handle != NULL)
    {
	fprintf(stderr, PACKAGE ": hmmm\n");
	abort();
    }

    /* Clean up */
    free(entry_sub);
    return 0;
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
    usenet_sub_t self = (usenet_sub_t)rock;
    self -> subscription = subscription;
    self -> is_pending = 0;

    /* Unsubscribe if we were pending when we were freed */
    if (self -> expression == NULL)
    {
	usenet_sub_set_connection(self, NULL, error);
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
    usenet_sub_t self = (usenet_sub_t)rock;
    self -> is_pending = 0;

    /* Free the receiver if it was pending when it was freed */
    if (self -> expression == NULL)
    {
	usenet_sub_free(self);
    }

    return ELVIN_RETURN_SUCCESS;
}


/* Sets the receiver's elvin connection */
void usenet_sub_set_connection(usenet_sub_t self, elvin_handle_t handle, elvin_error_t error)
{
    /* Disconnect from the old connection */
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

    /* Connect to the new one */
    self -> handle = handle;

    if ((self -> handle != NULL) && (self -> expression != NULL))
    {
	if (elvin_async_add_subscription(
		self -> handle, self -> expression, NULL, 1,
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
