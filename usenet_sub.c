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
static const char cvsid[] = "$Id: usenet_sub.c,v 1.4 1999/10/05 05:39:22 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usenet_sub.h"


/* Some notification field names */
#define BODY "BODY"
#define FROM_NAME "FROM_NAME"
#define FROM_EMAIL "FROM_EMAIL"
#define FROM "From"
#define KEYWORDS "keywords"
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

#define PATTERN_ONLY "%sNEWSGROUPS matches(\"%s\")"
#define PATTERN_PLUS "(%sNEWSGROUPS matches(\"%s\"))"

#define USENET_PREFIX "usenet: %s"
#define NEWS_URL "news://%s/%s"

/* The structure of a usenet subscription */
struct usenet_sub
{
    /* The receiver's subscription expression */
    char *expression;

    /* The length of the subscription expression */
    size_t expression_size;

    /* The receiver's elvin connection */
    connection_t connection;

    /* The receiver's connection info */
    void *connection_info;

    /* The receiver's callback */
    usenet_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;
};


/* Callbacks for matching notifications */
static void handle_notify(usenet_sub_t self, en_notify_t notification)
{
    message_t message;
    en_type_t type;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *buffer = NULL;

    /* Get the newsgroups to which the message was posted */
    if ((en_search(notification, NEWSGROUPS, &type, (void **)&string) != 0) ||
	(type != EN_STRING))
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
    if ((en_search(notification, FROM_NAME, &type, (void **)&name) != 0) ||
	(type != EN_STRING))
    {
	/* Otherwise try to use FROM_EMAIL */
	if ((en_search(notification, FROM_EMAIL, &type, (void **)&name) != 0) ||
	    (type != EN_STRING))
	{
	    /* Otherwise try the From field */
	    if ((en_search(notification, FROM, &type, (void **)&name) != 0) ||
		(type != EN_STRING))
	    {
		name = "anonymous";
	    }
	}
    }

    /* Get the SUBJECT field (if provided) */
    if ((en_search(notification, SUBJECT, &type, (void **)&subject) != 0) ||
	(type != EN_STRING))
    {
	subject = "[no subject]";
    }

    /* Get the MIME_ARGS field (if provided) */
    if ((en_search(notification, MIME_ARGS, &type, (void **)&mime_args) == 0) &&
	(type == EN_STRING))
    {
	/* Get the MIME_TYPE field (if provided) */
	if ((en_search(notification, MIME_TYPE, &type, (void **)&mime_type) != 0) ||
	    (type != EN_STRING))
	{
	    mime_type = URL_MIME_TYPE;
	}
    }
    /* No MIME_ARGS provided.  Construct one using the Message-Id field */
    else if ((en_search(notification, MESSAGE_ID, &type, (void **)&mime_args) == 0) &&
	     (type == EN_STRING))
    {
	char *newshost;

	if ((en_search(notification, X_NNTP_HOST, &type, (void **)&newshost) != 0) ||
	    (type != EN_STRING))
	{
	    newshost = "news";
	}

	if ((buffer = (char *)malloc(
	    strlen(NEWS_URL) + strlen(newshost) + strlen(mime_args) - 3)) == NULL)
	{
	    free(newsgroups);
	    return;
	}

	sprintf(buffer, NEWS_URL, newshost, mime_args);

	mime_args = buffer;
	mime_type = NEWS_MIME_TYPE;
    }
    /* No Message-Id field provided either */
    else
    {
	mime_type = NULL;
	mime_args = NULL;
    }

    /* Construct a message out of all of that */
    message = message_alloc(
	NULL,
	newsgroups, name,
	subject, 60,
	mime_type, mime_args,
	0, 0);

    /* Deliver the message */
    (*self -> callback)(self -> rock, message);

    /* Clean up */
    free(newsgroups);
    if (buffer != NULL)
    {
	free(buffer);
    }

    message_free(message);
    en_free(notification);
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
	    format = "%s matches(\"%s\")";
	    break;
	}

	/* not [matches] */
	case O_NOT:
	{
	    format = "!%s matches(\"%s\")";
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
    self -> connection = NULL;
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
    if (self -> connection != NULL)
    {
	fprintf(stderr, "*** Hmmm\n");
	exit(1);
    }

    /* Clean up */
    free(entry_sub);
    return 0;
}

/* Sets the receiver's elvin connection */
void usenet_sub_set_connection(usenet_sub_t self, connection_t connection)
{
    /* Disconnect from the old connection */
    if (self -> connection != NULL)
    {
	connection_unsubscribe(self -> connection, self -> connection_info);
    }

    /* Connect to the new one */
    self -> connection = connection;
    if ((self -> connection != NULL) && (self -> expression != NULL))
    {
	self -> connection_info = connection_subscribe(
	    self -> connection, self -> expression,
	    (notify_callback_t)handle_notify, self);
    }
}
