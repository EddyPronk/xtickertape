/* $Id: Message.c,v 1.4 1997/05/31 03:42:26 phelps Exp $ */

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
    unsigned long timeout; /* in seconds */
};


/* Creates and returns a new message */
Message Message_alloc(char *group, char *user, char *string, unsigned int timeout)
{
    Message message = (Message) malloc(sizeof(struct Message_t));

#ifdef SANITY
    message -> sanity_check = sanity_value;
#endif /* SANITY */
    message -> group = strdup(group);
    message -> user = strdup(user);
    message -> string = strdup(string);
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
    free(self -> string);
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


/* Prints debugging information */
void Message_debug(Message self)
{
    SANITY_CHECK(self);

    printf("Message (0x%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  group = \"%s\"\n", self -> group);
    printf("  user = \"%s\"\n", self -> user);
    printf("  string = \"%s\"\n", self -> string);
    printf("  timeout = %ld\n", self -> timeout);
}


