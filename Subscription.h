/* $Id: Subscription.h,v 1.1 1997/02/17 00:29:42 phelps Exp $ */

#include <unistd.h>
#include "List.h"

/* The subscription data type */
typedef struct Subscription_t *Subscription;


/* Answers a new Subscription */
Subscription Subscription_alloc(char *group, int inMenu, int autoMime, int minTime, int maxTime);

/* Releases resources used by a Subscription */
void Subscription_free(Subscription self);

/* Prints debugging information */
void Subscription_debug(Subscription self);

/* Read Subscriptions from the group file 'groups' and add them to 'list' */
void Subscription_readFromGroupFile(FILE *groups, List list);


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
int Subscription_adjustTimeout(Subscription self, int timeout);
