/* $Id: ElvinConnection.h,v 1.10 1998/10/22 07:06:27 phelps Exp $ */

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

typedef struct ElvinConnection_t *ElvinConnection;

#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include <X11/Intrinsic.h>
#include "Message.h"

/* The format of the error callback function */
typedef void (*ErrorCallback)(void *context, char *message);
typedef void (*NotifyCallback)(void *context, en_notify_t notification);

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname, int port, XtAppContext app_context,
    ErrorCallback callback, void *context);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

/* Registers a callback for when the given expression is matched */
void *ElvinConnection_subscribe(
    ElvinConnection self, char *expression,
    NotifyCallback callback, void *context);

/* Unregisters a callback (info was returned by ElvinConnection_subscribe) */
void ElvinConnection_unsubscribe(ElvinConnection self, void *info);

/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, en_notify_t notification);

#endif /* ELVINCONNECTION_H */
