/* $Id: ElvinConnection.h,v 1.2 1997/02/13 08:13:42 phelps Exp $ */

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

typedef struct ElvinConnection_t *ElvinConnection;

#include "List.h"
#include "Message.h"

/* The format for an the callback function */
typedef void (*ElvinConnectionCallback_t)(Message message, void *context);

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname,
    int port,
    List subscriptions,
    ElvinConnectionCallback_t callback,
    void *context);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

#endif /* ELVINCONNECTION_H */
