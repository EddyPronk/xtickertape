/* $Id: ElvinConnection.h,v 1.3 1997/02/14 10:52:32 phelps Exp $ */

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

typedef struct ElvinConnection_t *ElvinConnection;

#include "List.h"
#include "Message.h"

/* The format for an the callback function */
typedef void (*ElvinConnectionCallback)(Message message, void *context);

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname,
    int port,
    List subscriptions,
    ElvinConnectionCallback callback,
    void *context);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

#endif /* ELVINCONNECTION_H */
