/* $Id: BridgeConnection.h,v 1.1 1997/02/12 07:34:06 phelps Exp $ */

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


/* Call this when the connection has data available */
void BridgeConnection_read(BridgeConnection self);

#endif /* BRIDGECONNECTION_H */
