/* $Id: Subscription.h,v 1.5 1998/10/24 04:56:46 phelps Exp $ */

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

/* The subscription data type */
typedef struct Subscription_t *Subscription;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include "List.h"
#include "Message.h"
#include "ElvinConnection.h"
#include "Control.h"

/* The format for the callback function */
typedef void (*SubscriptionCallback)(void *context, Message message);



/* Answer a List containing the subscriptions in the given file or NULL if unable */
List Subscription_readFromGroupFile(
    FILE *groups, SubscriptionCallback callback, void *context);

/* Answers a new Subscription */
Subscription Subscription_alloc(
    char *group,
    char *expression,
    int inMenu,
    int autoMime,
    int minTime,
    int maxTime,
    SubscriptionCallback callback,
    void *context);

/* Releases resources used by a Subscription */
void Subscription_free(Subscription self);

/* Prints debugging information */
void Subscription_debug(Subscription self);

/* Sets the receiver's ElvinConnection */
void Subscription_setConnection(Subscription self, ElvinConnection connection);

/* Registers the receiver with the ControlPanel */
void Subscription_setControlPanel(Subscription self, ControlPanel controlPanel);

#endif /* SUBSCRIPTION_H */
