/* $Id: Message.c,v 1.8 1998/10/21 01:58:07 phelps Exp $ */

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
};


/* Creates and returns a new message */
Message Message_alloc(
    void *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mimeType,
    char *mimeArgs)
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
    return self;
}

/* Frees the memory used by the receiver */
void Message_free(Message self)
{
    SANITY_CHECK(self);

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#endif /* SANITY */

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

    free(self);
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
    printf("  timeout = %ld\n", self -> timeout);
}


