/* $Id: ElvinConnection.c,v 1.15 1998/10/15 06:22:45 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <elvin3/elvin.h>
#include <elvin3/element.h>

#include "sanity.h"
#include "ElvinConnection.h"
#include "Subscription.h"

#define BUFFERSIZE 8192

#ifdef SANITY
static char *sanity_value = "ElvinConnection";
static char *sanity_freed = "Freed ElvinConnection";
#endif /* SANITY */

/* Static function headers */
static void ReceiveCallback(elvin_t connection, void *object, uint32 id, en_notify_t notify);
static void ErrorCallback(elvin_t connection, void *arg, elvin_error_code_t code, char *message);
static void SubscribeToItem(Subscription subscription, ElvinConnection self);


/* The ElvinConnection data structure */
struct ElvinConnection_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */
    elvin_t connection;
    uint32 subid;
    List subscriptions;
};




/* Callback for quench expressions */
static void (*quenchCallback)(elvin_t connection, void *arg, char *quench) = NULL;

/* Callback for subscription thingo */
static void ReceiveCallback(elvin_t connection, void *object, uint32 id, en_notify_t notify)
{
    Subscription subscription = (Subscription)object;
    en_type_t type;
    char *group;
    char *user;
    char *text;
    int32 *timeout;
    char *mimeType;
    char *mimeArgs;

    /* Get the group from the subscription */
    group = Subscription_getGroup(subscription);

    /* Get the user from the notification if one is provided */
    if ((en_search(notify, "USER", &type, (void **)&user) != 0) || (type != EN_STRING))
    {
	user = "nobody";
    }

    if ((en_search(notify, "TICKERTEXT", &type, (void **)&text) != 0) || (type != EN_STRING))
    {
	text = "no message";
    }

    if ((en_search(notify, "TIMEOUT", &type, (void **)&timeout) != 0) || (type != EN_INT32))
    {
	*timeout = 10;
    }

    if ((en_search(notify, "MIME_TYPE", &type, (void **)&mimeType) != 0) || (type != EN_STRING))
    {
	mimeType = NULL;
    }

    if ((en_search(notify, "MIME_ARGS", &type, (void **)&mimeArgs) != 0) || (type != EN_STRING))
    {
	mimeArgs = NULL;
    }

    Subscription_deliverMessage(
	subscription,
	Message_alloc(group, user, text, *timeout, mimeType, mimeArgs));
}

/* Callback for elvin errors */
static void ErrorCallback(elvin_t connection, void *arg, elvin_error_code_t code, char *message)
{
    fprintf(stderr, "*** Elvin error %d (%s): exiting\n", code, message);
    exit(0);
}


/* Add the subscription info for an 'group' to the buffer */
static void SubscribeToItem(Subscription subscription, ElvinConnection self)
{
    SANITY_CHECK(self);
    elvin_add_subscription(
	self -> connection, Subscription_getExpression(subscription),
	ReceiveCallback, subscription, 0);
}


/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(char *hostname, int port, List subscriptions)
{
    ElvinConnection self;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(struct ElvinConnection_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

#ifdef DEBUG
    fprintf(stderr, "connecting to elvin at %s:%d...", hostname, port);
#endif /* DEBUG */
    if ((self -> connection = elvin_connect(
	EC_NAMEDHOST, hostname, port,
	quenchCallback, NULL,
	ErrorCallback, NULL,
	1)) == NULL)
    {
	fprintf(stderr, "*** Unable to connect to elvin server at %s:%d\n", hostname, port);
	exit(0);
    }
#ifdef DEBUG
    fprintf(stderr, "  done\n");
#endif /* DEBUG */
    self -> subscriptions = subscriptions;
    List_doWith(self -> subscriptions, SubscribeToItem, self);
    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
    SANITY_CHECK(self);

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    /* Disconnect from elvin server */
    elvin_disconnect(self -> connection);

    /* Free our memory */
    free(self);
}


/* Answers the file descriptor for the elvin connection */
int ElvinConnection_getFD(ElvinConnection self)
{
    SANITY_CHECK(self);
#ifdef DEBUG
    printf("fd = %d\n", elvin_get_socket(self -> connection));
#endif /*DEBUG */
    return elvin_get_socket(self -> connection);
}

/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, Message message)
{
    int32 timeout;
    en_notify_t notification;

    SANITY_CHECK(self);
    timeout = Message_getTimeout(message);
    notification = en_new();
    en_add_string(notification, "TICKERTAPE", Message_getGroup(message));
    en_add_string(notification, "USER", Message_getUser(message));
    en_add_int32(notification, "TIMEOUT", timeout);
    en_add_string(notification, "TICKERTEXT", Message_getString(message));

    if (elvin_notify(self -> connection, notification) < 0)
    {
	fprintf(stderr, "*** Unable to send message\n");
	exit(0);
    }

    en_free(notification);
}

/* Call this when the connection has data available */
void ElvinConnection_read(ElvinConnection self)
{
    SANITY_CHECK(self);
    elvin_dispatch(self -> connection);
}
