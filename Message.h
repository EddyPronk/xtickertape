/* $Id: Message.h,v 1.6 1998/10/21 01:58:07 phelps Exp $ */

#ifndef MESSAGE_H
#define MESSAGE_H

/* The Message pointer type */
typedef struct Message_t *Message;

/* Creates and returns a new message */
Message Message_alloc(
    void *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mimeType,
    char *mimeArgs);

/* Frees the memory used by the receiver */
void Message_free(Message self);

/* Answers the receiver's group */
char *Message_getGroup(Message self);

/* Answers the receiver's user */
char *Message_getUser(Message self);

/* Answers the receiver's string */
char *Message_getString(Message self);

/* Answers the receiver's timout in minutes */
unsigned long Message_getTimeout(Message self);

/* Sets the receiver's timeout in minutes*/
void Message_setTimeout(Message self, unsigned long timeout);

/* Answers the receiver's MIME-type string */
char *Message_getMimeType(Message self);

/* Answers the receiver's MIME arguments */
char *Message_getMimeArgs(Message self);

/* Answers the Subscription info for the receiver's subscription */
void *Message_getInfo(Message self);

/* Prints debugging information */
void Message_debug(Message self);

#endif /* MESSAGE_H */
