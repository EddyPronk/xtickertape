/* $Id: BridgeConnection.h,v 1.2 1997/02/14 10:52:29 phelps Exp $ */

#ifndef BRIDGECONNECTION_H
#define BRIDGECONNECTION_H

typedef struct BridgeConnection_t *BridgeConnection;

#include "List.h"
#include "Tickertape.h"

/* Answers a new BridgeConnection */
BridgeConnection BridgeConnection_alloc(
    TickertapeWidget target,
    char *hostname,
    int port,
    List subscriptions);

/* Releases the resources used by the BridgeConnection */
void BridgeConnection_free(BridgeConnection self);


/* Answers the receiver's file descriptor */
int BridgeConnection_getFD(BridgeConnection self);


/* Sends a Message to the bridge */
void BridgeConnection_send(BridgeConnection self, Message message);

/* Call this when the connection has data available */
void BridgeConnection_read(BridgeConnection self);

#endif /* BRIDGECONNECTION_H */
