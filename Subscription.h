/* $Id: Subscription.h,v 1.4 1998/10/21 01:58:08 phelps Exp $ */

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

/* Answers the receiver's group */
char *Subscription_getGroup(Subscription self);

/* Answers the receiver's subscription expression */
char *Subscription_getExpression(Subscription self);

/* Answers true if the receiver should appear in the Control Panel menu */
int Subscription_isInMenu(Subscription self);

/* Answers true if the receiver should automatically show mime messages */
int Subscription_isAutoMime(Subscription self);

#endif /* SUBSCRIPTION_H */
