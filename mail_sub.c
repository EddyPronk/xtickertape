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
static const char cvsid[] = "$Id: mail_sub.c,v 1.15 2000/01/13 00:29:47 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elvin/elvin.h>
#include "message.h"
#include "mbox_parser.h"
#include "mail_sub.h"

#define MAIL_SUB "exists(elvinmail) && user == \"%s\""
#define FOLDER_FMT "+%s"

/* The fields in the notification which we access */
#define F_FROM "From"
#define F_FOLDER "folder"
#define F_SUBJECT "Subject"


/* The MailSubscription data type */
struct mail_sub
{
    /* The receiver's user name */
    char *user;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* The receiver's subscription id */
    int64_t subscription_id;

    /* The receiver's rfc822 mailbox parser */
    mbox_parser_t parser;

    /* The receiver's callback function */
    mail_sub_callback_t callback;

    /* The receiver's callback user data */
    void *rock;
};


/* Delivers a notification which matches the receiver's e-mail subscription */
static void notify_cb(
    elvin_handle_t handle,
    int64_t subscription_id,
    elvin_notification_t notification,
    int is_secure,
    void *rock,
    elvin_error_t error)
{
    mail_sub_t self = (mail_sub_t)rock;
    message_t message;
    elvin_basetypes_t type;
    elvin_value_t value;
    char *from;
    char *folder;
    char *subject;
    char *buffer = NULL;

    /* Get the name from the `From' field */
    if (elvin_notification_get(notification, F_FROM, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	from = (char *)value.s;

	/* Split the user name from the address. */
	if ((mbox_parser_parse(self -> parser, from)) == 0)
	{
	    from = mbox_parser_get_name(self -> parser);
	    if (*from == '\0')
	    {
		/* Otherwise resort to the e-mail address */
		from = mbox_parser_get_email(self -> parser);
	    }
	}
    }
    else
    {
	from = "anonymous";
    }

    /* Get the folder field */
    if (elvin_notification_get(notification, F_FOLDER, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	folder = (char *)value.s;

	/* Format the folder name to use as the group */
	if ((buffer = (char *)malloc(strlen(FOLDER_FMT) + strlen(folder) - 1)) != NULL)
	{
	    sprintf(buffer, FOLDER_FMT, folder);
	    folder = buffer;
	}
    }
    else
    {
	folder = "mail";
    }

    /* Get the subject field */
    if (elvin_notification_get(notification, F_SUBJECT, &type, &value, error) &&
	type == ELVIN_STRING)
    {
	subject = (char *)value.s;
    }
    else
    {
	subject = "No subject";
    }

    /* Construct a message_t out of all of that */
    message = message_alloc(
	NULL, folder, from, subject, 60,
	NULL, NULL,
	NULL, NULL, NULL);

    (*self -> callback)(self -> rock, message);

    /* Clean up */
    message_free(message);

    /* Free the folder name */
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

/* Answers a new mail_sub_t */
mail_sub_t mail_sub_alloc(char *user, mail_sub_callback_t callback, void *rock)
{
    mail_sub_t self;

    /* Allocate memory for the receiver */
    if ((self = (mail_sub_t)malloc(sizeof(struct mail_sub))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

    /* Allocate memory for the rfc822 mailbox parser */
    if ((self -> parser = mbox_parser_alloc()) == NULL)
    {
	free(self);
	return NULL;
    }

    self -> user = strdup(user);
    self -> handle = NULL;
    self -> subscription_id = 0;
    self -> callback = callback;
    self -> rock = rock;
    return self;
}

/* Releases the resources consumed by the receiver */
void mail_sub_free(mail_sub_t self)
{
    if (self -> user != NULL)
    {
	free(self -> user);
    }

    if (self -> parser != NULL)
    {
	mbox_parser_free(self -> parser);
    }

    free(self);
}

/* Callback for a subscribe request */
static void subscribe_cb(
    elvin_handle_t handle, int result,
    int64_t subscription_id, void *rock,
    elvin_error_t error)
{
    mail_sub_t self = (mail_sub_t)rock;
    self -> subscription_id = subscription_id;
}

/* Sets the receiver's connection */
void mail_sub_set_connection(mail_sub_t self, elvin_handle_t handle, elvin_error_t error)
{
    /* Unsubscribe from the old connection */
    if (self -> handle != NULL)
    {
	if (elvin_async_delete_subscription(
	    self -> handle, self -> subscription_id,
	    NULL, NULL,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_async_delete_subscription(): failed\n");
	    abort();
	}
    }

    self -> handle = handle;

    /* Subscribe using the new handle */
    if (self -> handle != NULL)
    {
	char *buffer;

	if ((buffer = (char *)malloc(strlen(MAIL_SUB) + strlen(self -> user) - 1)) == NULL)
	{
	    return;
	}

	sprintf(buffer, MAIL_SUB, self -> user);

	/* Subscribe to elvinmail notifications */
	if (elvin_async_add_subscription(
	    self -> handle, (uchar *)buffer, NULL, 1,
	    notify_cb, self,
	    subscribe_cb, self,
	    error) == 0)
	{
	    fprintf(stderr, "elvin_async_add_subscription(): failed\n");
	    abort();
	}

	/* Clean up */
	free(buffer);
    }
}
