/* $Id: Subscription.h,v 1.2 1997/05/31 03:42:29 phelps Exp $ */

#include <unistd.h>
#include "List.h"
#include "Message.h"

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

/* The subscription data type */
typedef struct Subscription_t *Subscription;

/* The format for the callback function */
typedef void (*SubscriptionCallback)(Message message, void *context);

/* Answers a new Subscription */
/* Answers a new Subscription */
Subscription Subscription_alloc(
    char *group,
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

/* Read Subscriptions from the group file 'groups' and add them to 'list' */
void Subscription_readFromGroupFile(FILE *groups, List list, SubscriptionCallback callback, void *context);


/*
 * Accessors
 */

/* Answers the receiver's group */
char *Subscription_getGroup(Subscription self);

/* Answers true if the receiver should appear in the Control Panel menu */
int Subscription_isInMenu(Subscription self);

/* Answers true if the receiver should automatically show mime messages */
int Subscription_isAutoMime(Subscription self);

/* Answers the adjusted timeout for a message in this group */
void Subscription_adjustTimeout(Subscription self, Message message);

/* Delivers a Message received because of the receiver */
void Subscription_deliverMessage(Subscription self, Message message);

#endif /* SUBSCRIPTION_H */
