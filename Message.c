/* $Id: Message.c,v 1.3 1997/02/10 08:07:33 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Message.h"

struct Message_t
{
    char *group;
    char *user;
    char *string;
    unsigned long timeout; /* in seconds */
};


/* Creates and returns a new message */
Message Message_alloc(char *group, char *user, char *string, unsigned int timeout)
{
    Message message = (Message) malloc(sizeof(struct Message_t));

    message -> group = strdup(group);
    message -> user = strdup(user);
    message -> string = strdup(string);
    message -> timeout = timeout;
    return message;
}

/* Frees the memory used by the receiver */
void Message_free(Message self)
{
    free(self -> string);
    free(self);
}

/* Answers the receiver's group */
char *Message_getGroup(Message self)
{
    return self -> group;
}

/* Answers the receiver's user */
char *Message_getUser(Message self)
{
    return self -> user;
}


/* Answers the receiver's string */
char *Message_getString(Message self)
{
    return self -> string;
}

/* Answers the receiver's timout */
unsigned long Message_getTimeout(Message self)
{
    return self -> timeout;
}

/* Prints debugging information */
void Message_debug(Message self)
{
    printf("Message (0x%p)\n", self);
    printf("  group = \"%s\"\n", self -> group);
    printf("  user = \"%s\"\n", self -> user);
    printf("  string = \"%s\"\n", self -> string);
    printf("  timeout = %ld\n", self -> timeout);
}


