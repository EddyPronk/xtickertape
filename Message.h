/* $Id: Message.h,v 1.2 1997/02/10 07:14:54 phelps Exp $ */

#ifndef MESSAGE_H
#define MESSAGE_H

#include <X11/Xlib.h>

/* The Message pointer type */
typedef struct Message_t *Message;

/* Creates and returns a new message */
Message Message_alloc(char *group, char *user, char *string, unsigned int timeout);

/* Frees the memory used by the receiver */
void Message_free(Message self);

/* Answers the receiver's group */
char *Message_getGroup(Message self);

/* Answers the receiver's user */
char *Message_getUser(Message self);

/* Answers the receiver's string */
char *Message_getString(Message self);

/* Answers the receiver's timout */
unsigned long Message_getTimeout(Message self);

/* Prints debugging information */
void Message_debug(Message self);

#endif /* MESSAGE_H */
