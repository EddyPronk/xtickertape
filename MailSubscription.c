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
static const char cvsid[] = "$Id: MailSubscription.c,v 1.6 1999/05/06 00:34:31 phelps Exp $";
#endif /* lint */

#include <stdlib.h>
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */
#include "StringBuffer.h"
#include "MailSubscription.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "MailSubscription";
static char *sanity_freed = "Freed MailSubscription";
#endif /* SANITY */


/* The MailSubscription data type */
struct MailSubscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's user name */
    char *user;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information */
    void *connectionInfo;

    /* The receiver's callback function */
    MailSubscriptionCallback callback;

    /* The receiver's callback context */
    void *context;
};


/*
 *
 * Static function headers
 *
 */
static void GetNameFromEmail(StringBuffer buffer, char *email);
static void HandleNotify(MailSubscription self, en_notify_t notification);


/*
 *
 * Static functions
 *
 */

/* Locate the portion of an e-mail address and put in into the StringBuffer */
static void GetNameFromEmail(StringBuffer buffer, char *email)
{
    char *end = email + strlen(email) - 1;

    /* Is the e-mail address of the form Nice Name <email.address> ? */
    if (*end == '>')
    {
	char *pointer;

	/* Find the open '<' if it exists */
	for (pointer = end; pointer > email; pointer--)
	{
	    if (*pointer == '<')
	    {
		/* Found it.  Now trim spaces and quotes */
		while ((pointer > email) && ((*(pointer - 1) == ' ') || (*(pointer - 1) == '\"')))
		{
		    pointer--;
		}

		*pointer = '\0';

		for (pointer = email; *pointer != '\0'; pointer++)
		{
		    if ((*pointer != ' ') && (*pointer != '\"'))
		    {
			StringBuffer_append(buffer, pointer);
			return;
		    }
		}
	    }
	}
    }

    /* Is it of the form email.address (Nice Name) ? */
    if (*end == ')')
    {
	char *pointer;

	/* Skip everything up to a '(' */
	for (pointer = email; ((*pointer != '\0') && (*pointer != '(')); pointer++)
	{
	}

	/* If we've run out of string, then just copy the entire e-mail address */
	if (*pointer == '\0')
	{
	    StringBuffer_append(buffer, email);
	    return;
	}

	/* If we haven't run out of string, then copy everything up to a close paren */
	pointer++;
	while (*pointer != ')')
	{
	    StringBuffer_appendChar(buffer, *pointer);
	    pointer++;
	}

	return;
    }

    /* Not of the usual forms.  Just use the whole thing */
    StringBuffer_append(buffer, email);
}


/* Transforms an e-mail notification into a Message and delivers it */
static void HandleNotify(MailSubscription self, en_notify_t notification)
{
    Message message;
    StringBuffer buffer;
    en_type_t type;
    char *from;
    char *folder;
    char *subject;

    /* Create a StringBuffer in which to work */
    buffer = StringBuffer_alloc();

    /* Get the name from the "From" field */
    if ((en_search(notification, "From", &type, (void **)&from) != 0) || (type != EN_STRING))
    {
	from = NULL;
    }

    /* Get the folder from the "folder" field */
    if ((en_search(notification, "folder", &type, (void **)&folder) != 0) || (type != EN_STRING))
    {
	folder = NULL;
    }

    /* Get the subject from the "Subject" field */
    if ((en_search(notification, "Subject", &type, (void **)&subject) != 0) || (type != EN_STRING))
    {
	subject = "No subject";
    }

    /* Try to make the from field into something nice */
    if (from == NULL)
    {
#ifdef HAVE_ALLOCA
	from = "anonymous";
#else /* HAVE_ALLOCA */
	from = strdup("anonymous");
#endif /* HAVE_ALLOCA */
    }
    else
    {
	StringBuffer_clear(buffer);

	GetNameFromEmail(buffer, from);
#ifdef HAVE_ALLOCA
	from = (char *)alloca(StringBuffer_length(buffer) + 1);
	strcpy(from, StringBuffer_getBuffer(buffer));
#else /* HAVE_ALLOCA */
	from = strdup(StringBuffer_getBuffer(buffer));
#endif /* HAVE_ALLOCA */
    }

    /* If we did get a folder, then add a "+" to the beginning of it's name */
    StringBuffer_clear(buffer);
    if (folder != NULL)
    {
	StringBuffer_appendChar(buffer, '+');
	StringBuffer_append(buffer, folder);
	folder = StringBuffer_getBuffer(buffer);
    }
    else
    {
	folder = "mail";
    }

    /* Construct a Message and deliver it */
    message = Message_alloc(NULL, folder, from, subject, 60, NULL, NULL, 0, 0);
    (*self -> callback)(self -> context, message);

#ifndef HAVE_ALLOCA
    free(from);
#endif /* HAVE_ALLOCA */
    StringBuffer_free(buffer);
}

/*
 *
 * Exported functions
 *
 */

/* Answers a new MailSubscription */
MailSubscription MailSubscription_alloc(
    char *user,
    MailSubscriptionCallback callback, void *context)
{
    MailSubscription self;

    /* Allocate memory for the receiver */
    if ((self = (MailSubscription)malloc(sizeof(struct MailSubscription_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> user = strdup(user);
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> callback = callback;
    self -> context = context;
    return self;
}

/* Releases the resources consumed by the receiver */
void MailSubscription_free(MailSubscription self)
{
    SANITY_CHECK(self);

    if (self -> user != NULL)
    {
	free(self -> user);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else
    free(self);
#endif /* SANITY */
}

/* Prints debugging information about the receiver */
void MailSubscription_debug(MailSubscription self)
{
    SANITY_CHECK(self);

    printf("MailSubscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  callback = %p\n", self -> callback);
    printf("  context = %p\n", self -> context);
}


/* Sets the receiver's ElvinConnection */
void MailSubscription_setConnection(MailSubscription self, ElvinConnection connection)
{
    SANITY_CHECK(self);

    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from the old connection */
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    /* Subscribe with the new connection */
    if (self -> connection != NULL)
    {
	StringBuffer buffer = StringBuffer_alloc();
	StringBuffer_append(buffer, "exists(elvinmail) && user == \"");
	StringBuffer_append(buffer, self -> user);
	StringBuffer_append(buffer, "\"");

	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, StringBuffer_getBuffer(buffer),
	    (NotifyCallback)HandleNotify, self);

	StringBuffer_free(buffer);
    }
}
