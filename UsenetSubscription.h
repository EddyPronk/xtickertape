/* $Id: UsenetSubscription.h,v 1.1 1998/10/22 04:26:01 phelps Exp $
 * COPYRIGHT!
 *
 * Receives notifications from the NewsWatcher and transforms them
 * into Messages suitable for viewing on a Tickertape
 */

#ifndef USENET_SUBSCRIPTION_H
#define USENET_SUBSCRIPTION_H

/* The UsenetSubscription data type */
typedef struct UsenetSubscription_t *UsenetSubscription;

#include "Message.h"
#include "ElvinConnection.h"

typedef void (*UsenetSubscriptionCallback)(void *context, Message message);


/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context);

/* Releases the resources used by an UsenetSubscription */
void UsenetSubscription_free(UsenetSubscription self);

/* Prints debugging information */
void UsenetSubscription_debug(UsenetSubscription self);

/* Sets the receiver's ElvinConnection */
void UsenetSubscription_setConnection(UsenetSubscription self, ElvinConnection connection);

#endif /* USENET_SUBSCRIPTION_H */
