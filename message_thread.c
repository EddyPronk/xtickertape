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
static const char cvsid[] = "$Id: message_thread.c,v 1.1 1999/08/01 06:44:45 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include "message_thread.h"

struct message_thread
{
    /* The next message in the thread */
    message_thread_t next;

    /* The receiver's message */
    Message message;

    /* The receiver's child threads */
    message_thread_t children;

    /* Non-zero if this thread has been killed */
    int is_killed;
};


/* Allocates and initializes a new message_thread */
static message_thread_t message_thread_alloc(Message message)
{
    message_thread_t self;

    /* Allocate memory for a new message_thread */
    if ((self = (message_thread_t) malloc(sizeof(struct message_thread))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> next = NULL;
    self -> message = message;
    self -> children = NULL;
    self -> is_killed = 0;

    return self;
}

/* Releases the resources consumed by the receiver */
static void message_thread_free(message_thread_t self)
{
    free(self);
}

/* Finds the message_thread whose message id matches */
static message_thread_t message_thread_find(message_thread_t self, unsigned long id)
{
    message_thread_t result;

    /* Watch for end of list */
    if (self == NULL)
    {
	return NULL;
    }

    /* Check for a match at this node */
    if (Message_getId(self -> message) == id)
    {
	return self;
    }

    /* Check children */
    if ((result = message_thread_find(self -> children, id)) != NULL)
    {
	return result;
    }

    /* Otherwise check the rest of the list */
    return message_thread_find(self -> next, id);
}

/* Adds a Message to the end of a thread */
static message_thread_t message_thread_append(
    message_thread_t self,
    message_thread_t node)
{
    message_thread_t thread = self;

    /* If the list is empty, then return the new node */
    if (thread == NULL)
    {
	return node;
    }

    /* Otherwise find the last node in the thread and append the new node to it */
    while (1)
    {
	message_thread_t next = thread -> next;
	if (next == NULL)
	{
	    thread -> next = node;
	    return self;
	}

	thread = next;
    }
}

/* Adds a Message to a thread */
message_thread_t message_thread_add(message_thread_t self, Message message)
{
    message_thread_t node;
    unsigned long reply_id;

    /* If the message doesn't have an id then discard it */
    if (Message_getId(message) == 0)
    {
	return self;
    }

    /* Create a tree node to contain the new message */
    node = message_thread_alloc(message);

    /* If the message has reply id, then try to find its thread */
    if ((reply_id = Message_getReplyId(message)) != 0)
    {
	message_thread_t thread;

	/* If we can find the thread, then add the message to it */
	if ((thread = message_thread_find(self, reply_id)) != NULL)
	{
	    thread -> children = message_thread_append(thread -> children, node);
	    return self;
	}
    }

    /* Otherwise we add the new node to the top-level */
    return message_thread_append(self, node);
}


/* Prints out indented debugging information */
static void debug(message_thread_t self, int indent)
{
    if (self != NULL)
    {
	int i;

	/* Indent */
	for (i = 0; i < indent; i++)
	{
	    printf("  ");
	}

	printf("message %p\n", self -> message);
	debug(self -> children, indent + 1);
	debug(self -> next, indent);
    }
}

/* Prints debugging information */
void message_thread_debug(message_thread_t self)
{
    printf("message_thread:\n");
    debug(self, 1);
}
