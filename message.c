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
static const char cvsid[] = "$Id: message.c,v 1.13 2001/05/07 13:25:20 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include "message.h"


#ifdef DEBUG
static long message_count;
#endif /* DEBUG */

struct message
{
    /* The receiver's reference count */
    int ref_count;

    /* The time when the message was created */
    time_t creation_time;

    /* A string which identifies the subscription info in the control panel */
    char *info;

    /* The receiver's group */
    char *group;

    /* The receiver's user */
    char *user;

    /* The receiver's string (tickertext) */
    char *string;

    /* The receiver's MIME type information */
    char *mime_type;

    /* The receiver's MIME arg */
    char *mime_args;

    /* The length of the receiver's mime arg */
    size_t mime_length;

    /* The lifetime of the receiver in seconds */
    unsigned long timeout;

    /* The message's replacement tag */
    char *tag;

    /* The identifier for this message */
    char *id;

    /* The identifier for the message for which this is a reply */
    char *reply_id;
};


/* Creates and returns a new message */
message_t message_alloc(
    char *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mime_type,
    char *mime_args,
    size_t mime_length,
    char *tag,
    char *id,
    char *reply_id)
{
    message_t self;

    /* Allocate some space for the message_t */
    if ((self = (message_t)malloc(sizeof(struct message))) == NULL)
    {
	return NULL;
    }

    /* Initialize the contents to sane values */
    self -> ref_count = 1;
    self -> info = (info == NULL) ? NULL : strdup(info);
    self -> group = strdup(group);
    self -> user = strdup(user);
    self -> string = strdup(string);

    self -> mime_type = (mime_type == NULL) ? NULL : strdup(mime_type);
    self -> mime_length = mime_length;
    if (mime_length == 0)
    {
	self -> mime_args = NULL;
    }
    else
    {
	/* Allocate some room for a copy of the mime args */
	if ((self -> mime_args = (char *)malloc(mime_length + 1)) == NULL)
	{
	    self -> mime_length = 0;
	}
	else
	{
	    /* Copy the mime args and null terminate them */
	    memcpy(self -> mime_args, mime_args, mime_length);
	    self -> mime_args[mime_length] = '\0';
	}
    }

    self -> timeout = timeout;

    self -> tag = (tag == NULL) ? NULL : strdup(tag);
    self -> id = (id == NULL) ? NULL : strdup(id);
    self -> reply_id = (reply_id == NULL) ? NULL : strdup(reply_id);

    /* Record the time of creation */
    if (time(&self -> creation_time) == (time_t)-1)
    {
	perror("time(): failed");
    }

#ifdef DEBUG
    printf("allocated message_t %p (%ld)\n", self, ++message_count);
    message_debug(self);
#endif /* DEBUG */

    return self;
}

/* Allocates another reference to the message_t */
message_t message_alloc_reference(message_t self)
{
    self -> ref_count++;
    return self;
}

/* Frees the memory used by the receiver */
void message_free(message_t self)
{
    /* Decrement the reference count */
    if (--self -> ref_count > 0)
    {
	return;
    }

#ifdef DEBUG
    printf("freeing message_t %p (%ld):\n", self, --message_count);
    message_debug(self);
#endif /* DEBUG */

    /* Out of references -- release the hounds! */
    if (self -> info != NULL)
    {
	free(self -> info);
	self -> info = NULL;
    }

    if (self -> group != NULL)
    {
	free(self -> group);
	self -> group = NULL;
    }

    if (self -> user != NULL)
    {
	free(self -> user);
	self -> user = NULL;
    }

    if (self -> string != NULL)
    {
	free(self -> string);
	self -> string = NULL;
    }

    if (self -> mime_type != NULL)
    {
	free(self -> mime_type);
	self -> mime_type = NULL;
    }

    if (self -> mime_args != NULL)
    {
	free(self -> mime_args);
	self -> mime_args = NULL;
    }

    if (self -> tag != NULL)
    {
	free(self -> tag);
	self -> tag = NULL;
    }

    if (self -> id != NULL)
    {
	free(self -> id);
	self -> id = NULL;
    }

    if (self -> reply_id != NULL)
    {
	free(self -> reply_id);
	self -> reply_id = NULL;
    }

    free(self);
}


#ifdef DEBUG
/* Prints debugging information */
void message_debug(message_t self)
{
    printf("message_t (%p)\n", self);
    printf("  ref_count=%d\n", self -> ref_count);
    printf("  info = \"%s\"\n", (self -> info == NULL) ? "<null>" : self -> info);
    printf("  group = \"%s\"\n", self -> group);
    printf("  user = \"%s\"\n", self -> user);
    printf("  string = \"%s\"\n", self -> string);
    printf("  mime_type = \"%s\"\n", (self -> mime_type == NULL) ? "<null>" : self -> mime_type);
    printf("  mime_args = \"%s\"\n", (self -> mime_args == NULL) ? "<null>" : self -> mime_args);
    printf("  timeout = %ld\n", self -> timeout);
    printf("  tag = \"%s\"\n", (self -> tag == NULL) ? "<null>" : self -> tag);
    printf("  id = \"%s\"\n", (self -> id == NULL) ? "<null>" : self -> id);
    printf("  reply_id = \"%s\"\n", (self -> reply_id == NULL) ? "<null>" : self -> reply_id);
}
#endif /* DEBUG */

/* Answers the Subscription matched to generate the receiver */
char *message_get_info(message_t self)
{
    return self -> info;
}

/* Answers the receiver's creation time */
time_t *message_get_creation_time(message_t self)
{
    return &self -> creation_time;
}

/* Answers the receiver's group */
char *message_get_group(message_t self)
{
    return self -> group;
}

/* Answers the receiver's user */
char *message_get_user(message_t self)
{
    return self -> user;
}


/* Answers the receiver's string */
char *message_get_string(message_t self)
{
    return self -> string;
}

/* Answers the receiver's timout */
unsigned long message_get_timeout(message_t self)
{
    return self -> timeout;
}

/* Sets the receiver's timeout */
void message_set_timeout(message_t self, unsigned long timeout)
{
    self -> timeout = timeout;
}

/* Answers non-zero if the receiver has a MIME attachment */
int message_has_attachment(message_t self)
{
    return (self -> mime_args != NULL);
}

/* Answers the receiver's MIME-type string */
char *message_get_mime_type(message_t self)
{
    return self -> mime_type;
}

/* Answers the receiver's MIME arguments */
size_t message_get_mime_args(message_t self, char **mime_args_out)
{
    *mime_args_out = self -> mime_args;
    return self -> mime_length;
}

/* Answers the receiver's tag */
char *message_get_tag(message_t self)
{
    return self -> tag;
}

/* Answers the receiver's id */
char *message_get_id(message_t self)
{
    return self -> id;
}

/* Answers the id of the message_t for which this is a reply */
char *message_get_reply_id(message_t self)
{
    return self -> reply_id;
}

