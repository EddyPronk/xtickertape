/* $Id: ElvinConnection.h,v 1.5 1997/05/31 03:42:24 phelps Exp $ */

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

typedef struct ElvinConnection_t *ElvinConnection;

#include "List.h"
#include "Message.h"


/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(char *hostname, int port, List subscriptions);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

/* Answers the file descriptor for the elvin connection */
int ElvinConnection_getFD(ElvinConnection self);

/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, Message message);

/* Call this when the connection has data available */
void ElvinConnection_read(ElvinConnection self);

#endif /* ELVINCONNECTION_H */
