/* $Id: Message.c,v 1.6 1998/05/16 05:49:11 phelps Exp $ */

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
    char *group;
    char *user;
    char *string;
    char *mimeType;
    char *mimeArgs;
    unsigned long timeout; /* in seconds */
};


/* Creates and returns a new message */
Message Message_alloc(
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mimeType,
    char *mimeArgs)
{
    Message message = (Message) malloc(sizeof(struct Message_t));

#ifdef SANITY
    message -> sanity_check = sanity_value;
#endif /* SANITY */
    message -> group = strdup(group);
    message -> user = strdup(user);
    message -> string = strdup(string);

    if (mimeType == NULL)
    {
	message -> mimeType = NULL;
    }
    else
    {
	message -> mimeType = strdup(mimeType);
    }

    if (mimeArgs == NULL)
    {
	message -> mimeArgs = NULL;
    }
    else
    {
	message -> mimeArgs = strdup(mimeArgs);
    }

    message -> timeout = timeout;
    return message;
}

/* Frees the memory used by the receiver */
void Message_free(Message self)
{
    SANITY_CHECK(self);

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#endif /* SANITY */    
    free(self -> group);
    free(self -> user);
    free(self -> string);

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
    printf("  group = \"%s\"\n", self -> group);
    printf("  user = \"%s\"\n", self -> user);
    printf("  string = \"%s\"\n", self -> string);
    printf("  timeout = %ld\n", self -> timeout);
}


