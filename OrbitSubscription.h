/* $Id: OrbitSubscription.h,v 1.2 1998/10/21 01:58:08 phelps Exp $
 * COPYRIGHT!
 *
 * Manages an Orbit-specific zone-related subscription
 */

#ifndef ORBIT_SUBSCRIPTION_H
#define ORBIT_SUBSCRIPTION_H

/* The OrbitSubscription data type */
typedef struct OrbitSubscription_t *OrbitSubscription;

#include "Message.h"
#include "ElvinConnection.h"
#include "Control.h"

typedef void (*OrbitSubscriptionCallback)(void *context, Message message);



/* Answers a new OrbitSubscription */
OrbitSubscription OrbitSubscription_alloc(
    char *title, char *id,
    OrbitSubscriptionCallback callback, void *context);

/* Releases resources used by an OrbitSubscription */
void OrbitSubscription_free(OrbitSubscription self);

/* Prints debugging information */
void OrbitSubscription_debug(OrbitSubscription self);

/* Sets the receiver's title */
void OrbitSubscription_setTitle(OrbitSubscription self, char *title);

/* Sets the receiver's title */
char *OrbitSubscription_getTitle(OrbitSubscription self);

/* Sets the receiver's ElvinConnection */
void OrbitSubscription_setConnection(OrbitSubscription self, ElvinConnection connection);

/* Sets the receiver's ControlPanel */
void OrbitSubscription_setControlPanel(OrbitSubscription self, ControlPanel controlPanel);

#endif /* ORBIT_SUBSCRIPTION_H */
