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
static const char cvsid[] = "$Id: usenet_sub.c,v 1.7 1999/11/18 07:14:53 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elvin/elvin.h>
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

#define ONE_SUB \
"ELVIN_CLASS == \"NEWSWATCHER\" && " \
"ELVIN_SUBCLASS == \"MONITOR\" && (%s)"

#define PATTERN_ONLY "%sregex(NEWSGROUPS, \"%s\")"
#define PATTERN_PLUS "(%sregex(NEWSGROUPS, \"%s\"))"

#define USENET_PREFIX "usenet: %s"
#define NEWS_URL "news://%s/%s"

/* The structure of a usenet subscription */
struct usenet_sub
{
    /* The receiver's subscription expression */
    char *expression;

    /* The length of the subscription expression */
    size_t expression_size;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* The reciever's subscription id */
    int64_t subscription_id;

    /* The receiver's callback */
    usenet_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;
};


/* Delivers a notification which matches the receiver's subscription expression */
static void notify_cb(
    elvin_handle_t handle,
    int64_t subscription_id,
    elvin_notification_t notification,
    int is_secure,
    void *rock,
    dstc_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;
    message_t message;
    av_tuple_t tuple;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *buffer = NULL;

    /* If we don't have a callback than bail out now */
    if (self -> callback == NULL)
    {
	return;
    }

    /* Get the newsgroups to which the message was posted */
    tuple = elvin_notification_lookup(notification, NEWSGROUPS, error);
    if (tuple != NULL && tuple -> type == ELVIN_STRING)
    {
	string = tuple -> value.s;
    }
    else
    {
	string = "news";
    }

    /* Prepent `usenet:' to the beginning of the group field */
    if ((newsgroups = (char *)malloc(strlen(USENET_PREFIX) + strlen(string) - 1)) == NULL)
    {
	return;
    }

    sprintf(newsgroups, USENET_PREFIX, string);

    /* Get the name from the FROM_NAME field (if provided) */
    tuple = elvin_notification_lookup(notification, FROM_NAME, error);
    if (tuple != NULL && tuple -> type == ELVIN_STRING)
    {
	name = tuple -> value.s;
    }
    else
    {
	tuple = elvin_notification_lookup(notification, FROM_EMAIL, error);
	if (tuple != NULL && tuple -> type == ELVIN_STRING)
	{
	    name = tuple -> value.s;
	}
	else
	{
	    tuple = elvin_notification_lookup(notification, FROM, error);
	    if (tuple != NULL && tuple -> type == ELVIN_STRING)
	    {
		name = tuple -> value.s;
	    }
	    else
	    {
		name = "anonymous";
	    }
	}
    }

    /* Get the SUBJECT field (if provided) */
    tuple = elvin_notification_lookup(notification, SUBJECT, error);
    if (tuple != NULL && tuple -> type == ELVIN_STRING)
    {
	subject = tuple -> value.s;
    }
    else
    {
	subject = "[no subject]";
    }

    /* Get the MIME_ARGS field (if provided) */
    tuple = elvin_notification_lookup(notification, MIME_ARGS, error);
    if (tuple != NULL && tuple -> type == ELVIN_STRING)
    {
	mime_args = tuple -> value.s;

	/* Get the MIME_TYPE field (if provided) */
	tuple = elvin_notification_lookup(notification, MIME_TYPE, error);
	if (tuple != NULL && tuple -> type == ELVIN_STRING)
	{
	    mime_type = tuple -> value.s;
	}
	else
	{
	    mime_type = URL_MIME_TYPE;
	}
    }
    /* No MIME_ARGS provided.  Construct one using the Message-ID field */
    else
    {
	tuple = elvin_notification_lookup(notification, MESSAGE_ID, error);
	if (tuple != NULL && tuple -> type == ELVIN_STRING)
	{
	    char *message_id = tuple -> value.s;
	    char *news_host;

	    tuple = elvin_notification_lookup(notification, X_NNTP_HOST, error);
	    if (tuple != NULL && tuple -> type == ELVIN_STRING)
	    {
		news_host = tuple -> value.s;
	    }
	    else
	    {
		news_host = "news";
	    }

	    if ((buffer = (char *)malloc(
		strlen(NEWS_URL) +
		strlen(news_host) +
		strlen(message_id) - 3)) == NULL)
	    {
		mime_type = NULL;
		mime_args = NULL;
	    }
	    else
	    {
		sprintf(buffer, NEWS_URL, news_host, message_id);
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


    /* Construct a message out of all of that */
    if ((message = message_alloc(
	NULL,
	newsgroups, name,
	subject, 60,
	mime_type, mime_args,
	0, 0)) != NULL)
    {
	/* Deliver the message */
	(*self -> callback)(self -> rock, message);
	message_free(message);
    }

    /* Clean up */
    elvin_notification_free(notification, error);
    free(newsgroups);
    if (buffer != NULL)
    {
	free(buffer);
    }
}

/* Construct a portion of the subscription expression */
static char *alloc_expr(usenet_sub_t self, struct usenet_expr *expression)
{
    char *field_name;
    char *format;
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
	    fprintf(stderr, "*** internal error\n");
	    return NULL;
	}
    }

    /* Look up the string representation for the operator */
    switch (expression -> operator)
    {
	/* matches */
	case O_MATCHES:
	{
	    format = "regex(%s, \"%s\")";
	    break;
	}

	/* not [matches] */
	case O_NOT:
	{
	    format = "!regex(%s, \"%s\")";
	    break;
	}

	/* = */
	case O_EQ:
	{
	    if (expression -> field == F_XPOSTS)
	    {
		format = "%s == %s";
		break;
	    }

	    format = "%s == \"%s\"";
	    break;
	}

	/* != */
	case O_NEQ:
	{
	    if (expression -> field == F_XPOSTS)
	    {
		format = "%s != %s";
		break;
	    }

	    format = "%s != \"%s\"";
	    break;
	}

	/* < */
	case O_LT:
	{
	    format = "%s < %s";
	    break;
	}

	/* > */
	case O_GT:
	{
	    format = "%s > %s";
	    break;
	}

	/* <= */
	case O_LE:
	{
	    format = "%s <= %s";
	    break;
	}

	/* >= */
	case O_GE:
	{
	    printf("GE\n");
	    format = "%s >= %s";
	    break;
	}

	default:
	{
	    fprintf(stderr, "*** Internal error\n");
	    return NULL;
	}
    }

    /* Allocate space for the result */
    if ((result = (char *)malloc(
	strlen(format) + strlen(field_name) + strlen(expression -> pattern) - 3)) == NULL)
    {
	return NULL;
    }

    /* Construct the string */
    sprintf(result, format, field_name, expression -> pattern);
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
    size_t size;

    /* Shortcut -- if there are no expressions then we don't need parens */
    if (count == 0)
    {
	if ((result = (char *)malloc(
	    strlen(PATTERN_ONLY) + strlen(not_string) + strlen(pattern) - 3)) == NULL)
	{
	    return NULL;
	}

	sprintf(result, PATTERN_ONLY, not_string, pattern);
	return result;
    }

    /* Otherwise we have to do this the hard way.  Allocate space for
     * the initial pattern and then extend it for each of the expressions */
    size = strlen(PATTERN_PLUS) + strlen(not_string) + strlen(pattern) - 3;
    if ((result = (char *)malloc(size)) == NULL)
    {
	return NULL;
    }

    sprintf(result, PATTERN_PLUS, not_string, pattern);

    /* Insert the expressions */
    for (pointer = expressions; pointer < expressions + count; pointer++)
    {
	/* Get the string for the expression */
	char *expr = alloc_expr(self, pointer);
	size_t new_size = size + strlen(expr) + 4;

	/* Expand the buffer */
	if ((result = (char *)realloc(result, new_size)) == NULL)
	{
	    free(expr);
	    return NULL;
	}

	/* Overwrite the trailing right paren with the new stuff */
	strcpy(result + size - 2, " && ");
	strcpy(result + size + 2, expr);
	result[new_size - 2] = ')';
	result[new_size - 1] = '\0';
	size = new_size;
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
    self -> subscription_id = 0;
    self -> callback = callback;
    self -> rock = rock;
    return self;
}
    
/* Releases the resources consumed by the receiver */
void usenet_sub_free(usenet_sub_t self)
{
    if (self -> expression != NULL)
    {
	free(self -> expression);
    }

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
	self -> expression_size = sizeof(ONE_SUB) + entry_length - 2;
	if ((self -> expression = (char *)malloc(self -> expression_size)) == NULL)
	{
	    free(entry_sub);
	    return -1;
	}

	sprintf(self -> expression, ONE_SUB, entry_sub);
    }
    /* Otherwise we need to extend the existing expression */
    else
    {
	size_t new_size;

	/* Expand the expression buffer */
	new_size = self -> expression_size + entry_length + 4;
	if ((self -> expression = (char *)realloc(self -> expression, new_size)) == NULL)
	{
	    return -1;
	}

	/* Overwrite the trailing right paren with the new stuff */
	strcpy(self -> expression + self -> expression_size - 2, " || ");
	strcpy(self -> expression + self -> expression_size + 2, entry_sub);
	self -> expression[new_size - 2] = ')';
	self -> expression[new_size - 1] = '\0';
	self -> expression_size = new_size;
    }

    /* If we're connected then resubscribe */
    if (self -> handle != NULL)
    {
	fprintf(stderr, "*** Hmmm\n");
	abort();
    }

    /* Clean up */
    free(entry_sub);
    return 0;
}

/* Callback for a subscribe request */
static void subscribe_cb(
    elvin_handle_t handle, int result,
    int64_t subscription_id, void *rock,
    dstc_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;

    printf("subscribe_cb (result=%d, sub_id=%lld)\n", result, subscription_id);
    if (self -> subscription_id != 0)
    {
	printf("uh oh... self -> subscription_id = %lld\n", self -> subscription_id);
    }

    self -> subscription_id = subscription_id;
}

/* Callback for an unsubscribe request */
static void unsubscribe_cb(
    elvin_handle_t handle, int result,
    int64_t subscription_id, void *rock,
    dstc_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;

    printf("unsubscribe_cb (result=%d, sub_id=%lld)\n", result, subscription_id);
    if (self -> subscription_id != subscription_id)
    {
	printf("uh oh... self -> subscription_id = %lld\n", self -> subscription_id);
    }

    self -> subscription_id = 0;
}

/* Sets the receiver's elvin connection */
void usenet_sub_set_connection(usenet_sub_t self, elvin_handle_t handle, dstc_error_t error)
{
    /* Disconnect from the old connection */
    if ((self -> handle != NULL) && (self -> subscription_id != 0))
    {
	if (elvin_async_delete_subscription(
	    self -> handle, self -> subscription_id,
	    unsubscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_async_delete_subscription(): failed\n");
	    abort();
	}
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
	    abort();
	}
    }
}
