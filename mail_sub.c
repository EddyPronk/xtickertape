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
static const char cvsid[] = "$Id: mail_sub.c,v 1.7 1999/10/06 01:47:41 phelps Exp $";
#endif /* lint */

#include <stdlib.h>
#include "mbox_parser.h"
#include "mail_sub.h"

#define MAIL_SUB "exists(elvinmail) && user == \"%s\""
#define FOLDER_FMT "+%s"

/* The MailSubscription data type */
struct mail_sub
{
    /* The receiver's user name */
    char *user;

    /* The receiver's connection */
    connection_t connection;

    /* The receiver's connection information */
    void *connection_info;

    /* The receiver's rfc822 mailbox parser */
    mbox_parser_t parser;

    /* The receiver's callback function */
    mail_sub_callback_t callback;

    /* The receiver's callback user data */
    void *rock;
};


/* Transforms an e-mail notification into a message_t and delivers it */
static void handle_notify(mail_sub_t self, en_notify_t notification)
{
    message_t message;
    en_type_t type;
    char *from;
    char *folder;
    char *subject;
    char *buffer = NULL;

    /* Get the name from the "From" field */
    if ((en_search(notification, "From", &type, (void **)&from) != 0) || (type != EN_STRING))
    {
	from = "anonymous";
    }
    else
    {
	/* Split the user name from her address.  Note: this modifies from! */
	if ((mbox_parser_parse(self -> parser, from)) == 0)
	{
	    from = mbox_parser_get_name(self -> parser);
	    if (*from == '\0')
	    {
		/* Otherwise resort to email address */
		from = mbox_parser_get_email(self -> parser);
	    }
	}
    }

    /* Get the folder from the "folder" field */
    if ((en_search(notification, "folder", &type, (void **)&folder) != 0) || (type != EN_STRING))
    {
	/* It's silly, but we need to malloc so that we can free */
	folder = strdup("mail");
    }
    else
    {
	/* Format the folder name to use as the group */
	buffer = (char *)malloc(strlen(FOLDER_FMT) + strlen(folder) - 1);
	if (buffer != NULL)
	{
	    sprintf(buffer, FOLDER_FMT, folder);
	    folder = buffer;
	}
    }

    /* Get the subject from the "Subject" field */
    if ((en_search(notification, "Subject", &type, (void **)&subject) != 0) || (type != EN_STRING))
    {
	subject = "No subject";
    }

    /* Construct a message_t out of all of that */
    message = message_alloc(NULL, folder, from, subject, 60, NULL, NULL, 0, 0);
    (*self -> callback)(self -> rock, message);

    /* Clean up */
    message_free(message);
    en_free(notification);

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
    self -> connection = NULL;
    self -> connection_info = NULL;
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

/* Prints debugging information about the receiver */
void mail_subscription_debug(mail_sub_t self)
{
    printf("mail_sub_t (%p)\n", self);
    printf("  connection = %p\n", self -> connection);
    printf("  connection_info = %p\n", self -> connection_info);
    printf("  callback = %p\n", self -> callback);
    printf("  rock = %p\n", self -> rock);
}


/* Sets the receiver's connection */
void mail_sub_set_connection(mail_sub_t self, connection_t connection)
{
    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from the old connection */
    if (self -> connection != NULL)
    {
	connection_unsubscribe(self -> connection, self -> connection_info);
    }

    self -> connection = connection;

    /* Subscribe with the new connection */
    if (self -> connection != NULL)
    {
	char *buffer;

	if ((buffer = (char *)malloc(strlen(MAIL_SUB) + strlen(self -> user) - 1)) == NULL)
	{
	    return;
	}

	sprintf(buffer, MAIL_SUB, self -> user);

	/* Subscribe to elvinmail notifications */
	self -> connection_info = connection_subscribe(
	    self -> connection, buffer,
	    (notify_callback_t)handle_notify, self);

	free(buffer);
    }
}
