/* $Id: Message.c,v 1.1 1997/02/05 06:24:12 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Message.h"

struct Message_t
{
    char *group;
    char *user;
    char *string;
    unsigned long length;
    unsigned long timeout;
    Font cacheFont;
    unsigned long width;
};


/* Creates and returns a new message */
Message Message_alloc(char *group, char *user, char *string, unsigned int timeout)
{
    Message message = (Message) malloc(sizeof(struct Message_t));

    message -> group = strdup(group);
    message -> user = strdup(user);
    message -> string = strdup(string);
    message -> length = strlen(string);
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

/* Answers the length of the receiver's string */
unsigned long Message_getStringLength(Message self)
{
    return self -> length;
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


