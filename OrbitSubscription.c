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
static const char cvsid[] = "$Id: OrbitSubscription.c,v 1.17 1999/08/22 06:22:41 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include "OrbitSubscription.h"
#include "StringBuffer.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "OrbitSubscription";
static char *sanity_freed = "Freed OrbitSubscription";
#endif /* SANITY */


/* The OrbitSubscription data type */
struct OrbitSubscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's title */
    char *title;

    /* The receiver's zone id */
    char *id;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information */
    void *connectionInfo;

    /* The receiver's ControlPanel */
    ControlPanel controlPanel;

    /* The receiver's ControlPanel information */
    void *controlPanelInfo;

    /* The receiver's callback function */
    OrbitSubscriptionCallback callback;

    /* The receiver's callback context */
    void *context;
};


/*
 *
 * Static function headers
 *
 */

static void HandleNotify(OrbitSubscription self, en_notify_t notification);
void SendMessage(OrbitSubscription self, Message message);


/*
 *
 * Static functions
 *
 */

/* Transforms a notification into a Message and delivers it */
static void HandleNotify(OrbitSubscription self, en_notify_t notification)
{
    Message message;
    en_type_t type;
    char *user;
    char *text;
    int32 *timeout_p;
    int32 timeout;
    char *mimeType;
    char *mimeArgs;
    char *messageId;
    char *replyId;
    SANITY_CHECK(self);

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }

    /* Get the user from the notification (if provided) */
    if ((en_search(notification, "USER", &type, (void **)&user) != 0) ||
	(type != EN_STRING))
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if ((en_search(notification, "TICKERTEXT", &type, (void **)&text) != 0) ||
	(type != EN_STRING))
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    if ((en_search(notification, "TIMEOUT", &type, (void **)&timeout_p) != 0) ||
	(type != EN_INT32))
    {
	timeout_p = &timeout;
	timeout = 0;

	/* Check to see if it's in ascii format */
	if (type == EN_STRING)
	{
	    char *timeoutString;
	    en_search(notification, "TIMEOUT", &type, (void **)&timeoutString);
	    timeout = atoi(timeoutString);
	}

	timeout = (timeout == 0) ? 10 : timeout;
    }

    /* Get the MIME type (if provided) */
    if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) ||
	(type != EN_STRING))
    {
	mimeType = NULL;
    }

    /* Get the MIME args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) != 0) ||
	(type != EN_STRING))
    {
	mimeArgs = NULL;
    }

    /* Get the message id (if provided) */
    if ((en_search(notification, "Message-Id", &type, (void **)&messageId) != 0) ||
	(type != EN_STRING))
    {
        messageId = NULL;
    }

    /* Get the reply id (if provided) */
    if ((en_search(notification, "In-Reply-To", &type, (void **)&replyId) != 0) ||
	(type != EN_STRING))
    {
        replyId = NULL;
    }

    /* Construct the Message */
    message = Message_alloc(
	self -> controlPanelInfo, self -> title,
	user, text, *timeout_p,
	mimeType, mimeArgs,
	messageId, replyId);

    /* Deliver the Message */
    (*self -> callback)(self -> context, message);

    /* Release our reference to the message */
    Message_free(message);
}

/* Constructs a notification out of a Message and delivers it to the ElvinConection */
void SendMessage(OrbitSubscription self, Message message)
{
    en_notify_t notification;
    int32 timeout;
    char *messageId;
    char *replyId;
    char *mimeArgs;
    char *mimeType;
    SANITY_CHECK(self);

    timeout = Message_getTimeout(message);
    messageId = Message_getId(message);
    replyId = Message_getReplyId(message);
    mimeArgs = Message_getMimeArgs(message);
    mimeType = Message_getMimeType(message);

    notification = en_new();
    en_add_string(notification, "zone.id", self -> id);
    en_add_string(notification, "USER", Message_getUser(message));
    en_add_string(notification, "TICKERTEXT", Message_getString(message));
    en_add_int32(notification, "TIMEOUT", timeout);
    en_add_string(notification, "Message-Id", messageId);

    if (replyId != NULL)
    {
	en_add_string(notification, "In-Reply-To", replyId);
    }

    /* Add mime information if both mimeArgs and mimeType are provided */
    if ((mimeArgs != NULL) && (mimeType != NULL))
    {
	en_add_string(notification, "MIME_ARGS", mimeArgs);
	en_add_string(notification, "MIME_TYPE", mimeType);
    }

    ElvinConnection_send(self -> connection, notification);
    en_free(notification);
}


/*
 *
 * Exported functions
 *
 */

/* Answers a new OrbitSubscription */
OrbitSubscription OrbitSubscription_alloc(
    char *title,
    char *id,
    OrbitSubscriptionCallback callback,
    void *context)
{
    OrbitSubscription self = (OrbitSubscription) malloc(sizeof(struct OrbitSubscription_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> title = strdup(title);
    self -> id = strdup(id);
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> controlPanel = NULL;
    self -> controlPanelInfo = NULL;
    self -> callback = callback;
    self -> context = context;

    return self;
}

/* Releases the resouces used by an OrbitSubscription */
void OrbitSubscription_free(OrbitSubscription self)
{
    SANITY_CHECK(self);

    /* Free the receiver's title if it has one */
    if (self -> title)
    {
	free(self -> title);
    }

    /* Free the receiver's id if it has one */
    if (self -> id)
    {
	free(self -> id);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */    
    free(self);
#endif /* SANITY */    
}


/* Prints debugging information */
void OrbitSubscription_debug(OrbitSubscription self)
{
    SANITY_CHECK(self);

    printf("OrbitSubscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */    
    printf("  title = \"%s\"\n", self -> title);
    printf("  id = \"%s\"\n", self -> id);
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  controlPanel = %p\n", self -> controlPanel);
    printf("  controlPanelInfo = %p\n", self -> controlPanelInfo);
    printf("  callback = %p\n", self -> controlPanel);
    printf("  context = %p\n", self -> context);
}


/* Sets the receiver's title */
void OrbitSubscription_setTitle(OrbitSubscription self, char *title)
{
    SANITY_CHECK(self);

    /* Release the old title */
    if (self -> title != NULL)
    {
	free(self -> title);
    }

    self -> title = strdup(title);

    /* Update our menu item */
    if (self -> controlPanel != NULL)
    {
	ControlPanel_retitleSubscription(self -> controlPanel, self -> controlPanelInfo, self -> title);
    }
}

/* Sets the receiver's title */
char *OrbitSubscription_getTitle(OrbitSubscription self)
{
    SANITY_CHECK(self);
    return self -> title;
}


/* Sets the receiver's ElvinConnection */
void OrbitSubscription_setConnection(OrbitSubscription self, ElvinConnection connection)
{
    SANITY_CHECK(self);

    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from old connection */
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    /* Subscribe to the new connection */
    if (self -> connection != NULL)
    {
	StringBuffer buffer = StringBuffer_alloc();
	StringBuffer_append(buffer, "exists(TICKERTEXT) && zone.id == \"");
	StringBuffer_append(buffer, self -> id);
	StringBuffer_append(buffer, "\"");
	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, StringBuffer_getBuffer(buffer),
	    (NotifyCallback)HandleNotify, self);
	StringBuffer_free(buffer);
    }
}

/* Sets the receiver's ControlPanel */
void OrbitSubscription_setControlPanel(OrbitSubscription self, ControlPanel controlPanel)
{
    SANITY_CHECK(self);

    /* Shortcut if we're with the same ControlPanel */
    if (self -> controlPanel == controlPanel)
    {
	return;
    }

    /* Unregister with the old ControlPanel */
    if (self -> controlPanel != NULL)
    {
	ControlPanel_removeSubscription(self -> controlPanel, self -> controlPanelInfo);
    }

    self -> controlPanel = controlPanel;

    /* Register with the new ControlPanel */
    if (self -> controlPanel != NULL)
    {
	self -> controlPanelInfo = ControlPanel_addSubscription(
	    controlPanel, self -> title,
	    (ControlPanelCallback)SendMessage, self);
    }
}

