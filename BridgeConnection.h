/* $Id: BridgeConnection.h,v 1.3 1997/02/25 04:32:31 phelps Exp $ */

#ifndef BRIDGECONNECTION_H
#define BRIDGECONNECTION_H

typedef struct BridgeConnection_t *BridgeConnection;

#include "List.h"
#include "Message.h"

typedef void (*BridgeConnectionCallback)(Message message, void *context);

/* Answers a new BridgeConnection */
BridgeConnection BridgeConnection_alloc(
    char *hostname,
    int port,
    List subscriptions,
    BridgeConnectionCallback callback,
    void *context);

/* Releases the resources used by the BridgeConnection */
void BridgeConnection_free(BridgeConnection self);


/* Answers the receiver's file descriptor */
int BridgeConnection_getFD(BridgeConnection self);


/* Sends a Message to the bridge */
void BridgeConnection_send(BridgeConnection self, Message message);

/* Call this when the connection has data available */
void BridgeConnection_read(BridgeConnection self);

#endif /* BRIDGECONNECTION_H */
