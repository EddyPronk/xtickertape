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
static const char cvsid[] = "$Id: Message.c,v 1.16 1998/12/24 05:48:28 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sanity.h"
#include "Message.h"


#ifdef SANITY
static char *sanity_value = "Message";
static char *sanity_freed = "Freed Message";
#endif /* SANITY */

struct Message_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The Subscription which was matched to generate the receiver */
    void *info;

    /* The receiver's group */
    char *group;

    /* The receiver's user */
    char *user;

    /* The receiver's string (tickertext) */
    char *string;

    /* The receiver's MIME type information */
    char *mimeType;

    /* The receiver's MIME arg */
    char *mimeArgs;

    /* The lifetime of the receiver in seconds */
    unsigned long timeout;

    /* The identifier for this message */
    unsigned long id;

    /* The identifier for the message for which this is a reply */
    unsigned long replyId;
};


/* Creates and returns a new message */
Message Message_alloc(
    void *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mimeType,
    char *mimeArgs,
    unsigned long id,
    unsigned long replyId)
{
    Message self = (Message) malloc(sizeof(struct Message_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> info = info;
    self -> group = strdup(group);
    self -> user = strdup(user);
    self -> string = strdup(string);

    if (mimeType == NULL)
    {
	self -> mimeType = NULL;
    }
    else
    {
	self -> mimeType = strdup(mimeType);
    }

    if (mimeArgs == NULL)
    {
	self -> mimeArgs = NULL;
    }
    else
    {
	self -> mimeArgs = strdup(mimeArgs);
    }

    self -> timeout = timeout;
    self -> id = id;
    self -> replyId = replyId;

    return self;
}

/* Frees the memory used by the receiver */
void Message_free(Message self)
{
    SANITY_CHECK(self);

    if (self -> group)
    {
	free(self -> group);
    }

    if (self -> user)
    {
	free(self -> user);
    }

    if (self -> string)
    {
	free(self -> string);
    }

    if (self -> mimeType)
    {
	free(self -> mimeType);
    }

    if (self -> mimeArgs)
    {
	free(self -> mimeArgs);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}


/* Prints debugging information */
void Message_debug(Message self)
{
    SANITY_CHECK(self);

    printf("Message (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  info = 0x%p\n", self -> info);
    printf("  group = \"%s\"\n", self -> group);
    printf("  user = \"%s\"\n", self -> user);
    printf("  string = \"%s\"\n", self -> string);
    printf("  mimeType = \"%s\"\n", (self -> mimeType == NULL) ? "<null>" : self -> mimeType);
    printf("  mimeArgs = \"%s\"\n", (self -> mimeArgs == NULL) ? "<null>" : self -> mimeArgs);
    printf("  timeout = %ld\n", self -> timeout);
    printf("  id = %ld\n", self -> id);
    printf("  replyId = %ld\n", self -> replyId);
}



/* Answers the Subscription matched to generate the receiver */
void *Message_getInfo(Message self)
{
    SANITY_CHECK(self);
    return self -> info;
}

/* Answers the receiver's group */
char *Message_getGroup(Message self)
{
    SANITY_CHECK(self);
    return self -> group;
}

/* Answers the receiver's user */
char *Message_getUser(Message self)
{
    SANITY_CHECK(self);
    return self -> user;
}


/* Answers the receiver's string */
char *Message_getString(Message self)
{
    SANITY_CHECK(self);
    return self -> string;
}

/* Answers the receiver's timout */
unsigned long Message_getTimeout(Message self)
{
    SANITY_CHECK(self);
    return self -> timeout;
}

/* Sets the receiver's timeout */
void Message_setTimeout(Message self, unsigned long timeout)
{
    SANITY_CHECK(self);
    self -> timeout = timeout;
}

/* Answers non-zero if the receiver has a MIME attachment */
int Message_hasAttachment(Message self)
{
    return (self -> mimeArgs != NULL);
}

/* Answers the receiver's MIME-type string */
char *Message_getMimeType(Message self)
{
    SANITY_CHECK(self);
    return self -> mimeType;
}

/* Answers the receiver's MIME arguments */
char *Message_getMimeArgs(Message self)
{
    SANITY_CHECK(self);
    return self -> mimeArgs;
}

/* Answers the receiver's id */
unsigned long Message_getId(Message self)
{
    SANITY_CHECK(self);
    return self -> id;
}

/* Answers the id of the message for which this is a reply */
unsigned long Message_getReplyId(Message self)
{
    SANITY_CHECK(self);
    return self -> replyId;
}

