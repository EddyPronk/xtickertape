/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2003.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
static const char cvsid[] = "$Id: message.c,v 1.22 2003/01/27 17:33:14 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* perror, printf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* free, malloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memset, strdup */
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h> /* isspace */
#endif
#ifdef HAVE_TIME_H
#include <time.h> /* time */
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h> /* assert */
#endif
#include "replace.h"
#include "message.h"


#ifdef DEBUG
static long message_count;
#endif /* DEBUG */

#define CONTENT_TYPE "Content-Type:"

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

    /* The receiver's MIME attachment */
    char *attachment;

    /* The length of the receiver's MIME attachment */
    size_t length;

    /* The lifetime of the receiver in seconds */
    unsigned long timeout;

    /* The message's replacement tag */
    char *tag;

    /* The identifier for this message */
    char *id;

    /* The identifier for the message for which this is a reply */
    char *reply_id;

    /* The identifier for the thread for which this is a reply */
    char *thread_id;

    /* Non-zero if the message has been killed */
    int is_killed;
};


/* Creates and returns a new message */
message_t message_alloc(
    char *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *attachment,
    size_t length,
    char *tag,
    char *id,
    char *reply_id,
    char *thread_id)
{
    message_t self;

    /* Allocate some space for the message_t */
    if ((self = (message_t)malloc(sizeof(struct message))) == NULL)
    {
	return NULL;
    }

    /* Set its contents to sane values */
    memset(self, 0, sizeof(struct message));

    /* Initialize the contents to sane values */
    self -> ref_count = 1;

    if (info != NULL)
    {
	self -> info = strdup(info);
    }

    assert(group != NULL);
    self -> group = strdup(group);

    assert(user != NULL);
    self -> user = strdup(user);

    assert(string != NULL);
    self -> string = strdup(string);

    if (length == 0)
    {
	self -> attachment = NULL;
	self -> length = 0;
    }
    else
    {
	/* Allocate some room for a copy of the mime args */
	if ((self -> attachment = malloc(length)) == NULL)
	{
	    self -> length = 0;
	}
	else
	{
	    /* Make a copy of the attachment */
	    memcpy(self -> attachment, attachment, length);
	    self -> length = length;
	}
    }

    self -> timeout = timeout;

    self -> tag = (tag == NULL) ? NULL : strdup(tag);
    self -> id = (id == NULL) ? NULL : strdup(id);
    self -> reply_id = (reply_id == NULL) ? NULL : strdup(reply_id);
    self -> thread_id = (thread_id == NULL) ? NULL : strdup(thread_id);

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

    if (self -> attachment != NULL)
    {
	free(self -> attachment);
	self -> attachment = NULL;
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

    if (self -> thread_id != NULL)
    {
	free(self -> thread_id);
	self -> thread_id = NULL;
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
    printf("  attachment = \"%p\" [%d]\n", self -> attachment, self -> length);
    printf("  timeout = %ld\n", self -> timeout);
    printf("  tag = \"%s\"\n", (self -> tag == NULL) ? "<null>" : self -> tag);
    printf("  id = \"%s\"\n", (self -> id == NULL) ? "<null>" : self -> id);
    printf("  reply_id = \"%s\"\n", (self -> reply_id == NULL) ? "<null>" : self -> reply_id);
    printf("  thread_id = \"%s\"\n", (self -> thread_id == NULL) ? "<null>" : self -> thread_id);
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
    return (self -> attachment != NULL);
}

/* Answers the receiver's MIME arguments */
size_t message_get_attachment(message_t self, char **attachment_out)
{
    *attachment_out = self -> attachment;
    return self -> length;
}

/* Decodes the attachment into a content type, character set and body */
int message_decode_attachment(
    message_t self,
    char **type_out,
    char **body_out,
    size_t *length_out)
{
    unsigned char *point = (unsigned char *)self -> attachment;
    unsigned char *mark = point;
    unsigned char *end = point + self -> length;
    size_t ct_length = sizeof(CONTENT_TYPE) - 1;

    *type_out = NULL;
    *body_out = NULL;
    *length_out = 0;

    /* If no attachment then return lots of NULLs */
    if (self -> attachment == NULL)
    {
	return 0;
    }

    /* Look through the attachment's MIME header */
    while (point < end)
    {
	/* End of line? */
	if (*point == '\r' || *point == '\n')
	{
	    /* Is this the Content-Type line? */
	    if (ct_length < point - mark &&
		strncasecmp(mark, CONTENT_TYPE, ct_length) == 0)
	    {
		unsigned char *p;

		/* Skip to the beginning of the value */
		mark += ct_length;
		while (mark < point && isspace(*mark))
		{
		    mark++;
		}

		/* Look for a ';' in the content type */
		if ((p = memchr(mark, ';', point - mark)) == NULL)
		{
		    p = point;
		}

		/* If multiple content-types are given then use the last */
		if (*type_out != NULL)
		{
		    free(*type_out);
		}

		/* Allocate a string to hold the content type */
		if ((*type_out = (char *)malloc(p - mark + 1)) == NULL)
		{
		    return -1;
		}
		memcpy(*type_out, mark, p - mark);
		(*type_out)[p - mark] = 0;
	    }
	    else if (point - mark == 0)
	    {
		/* If the CR-LF linefeed is used then skip the LF */
		if (*point == '\r' && point + 1 < end && *(point + 1) == '\n')
		{
		    point += 2;
		}
		else
		{
		    point += 1;
		}

		*body_out = (char *)point;
		*length_out = end - point;
		return 0;
	    }

	    /* If the CR-LF linefeed is used then skip the LF */
	    if (*point == '\r' && point + 1 < end && *(point + 1) == '\n')
	    {
		mark = point + 2;
	    }
	    else
	    {
		mark = point + 1;
	    }
	}

	point++;
    }

    /* No body found */
    return 0;
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

/* Answers the thread id of the message for which this is a reply */
char *message_get_thread_id(message_t self)
{
    return self -> thread_id;
}

/* Answers non-zero if the mesage has been killed */
int message_is_killed(message_t self)
{
    return self -> is_killed;
}

/* Set the message's killed status */
void message_set_killed(message_t self, int is_killed)
{
    self -> is_killed = is_killed;
}

