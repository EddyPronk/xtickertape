/* $Id: MailSubscription.h,v 1.1 1998/10/30 08:13:09 phelps Exp $
 * Copyright!
 *
 * Receives notifications from elvinmail and transforms them into
 * Messages suitable for tickertape viewing
 */

#ifndef MAIL_SUBSCRIPTION_H
#define MAIL_SUBSCRIPTION_H

/* The MailSubscription data type */
typedef struct MailSubscription_t *MailSubscription;

#include "Message.h"
#include "ElvinConnection.h"

/* The MailSubscription callback type */
typedef void (*MailSubscriptionCallback)(void *context, Message message);


/* Exported functions */

/* Answers a new MailSubscription */
MailSubscription MailSubscription_alloc(
    char *user,
    MailSubscriptionCallback callback, void *context);

/* Releases the resources consumed by the receiver */
void MailSubscription_free(MailSubscription self);

/* Prints debugging information about the receiver */
void MailSubscription_debug(MailSubscription self);

/* Sets the receiver's ElvinConnection */
void MailSubscription_setConnection(MailSubscription self, ElvinConnection connection);

#endif /* MAIL_SUBSCRIPTION_H */
