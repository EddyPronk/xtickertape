/* $Id: ElvinConnection.c,v 1.10 1998/02/10 23:40:48 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <elvin.h>
#include <element.h>

#include "sanity.h"
#include "ElvinConnection.h"
#include "Subscription.h"

#define BUFFERSIZE 8192

#ifdef SANITY
static char *sanity_value = "ElvinConnection";
static char *sanity_freed = "Freed ElvinConnection";
#endif /* SANITY */


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
static void receiveCallback(elvin_t connection, void *object, uint32 id, en_notify_t notify)
{
    Subscription subscription = (Subscription)object;
    en_type_t type;
    char *group;
    char *user;
    char *text;
    int32 *timeout;
    char *mimeType;
    char *mimeArgs;

    en_search(notify, "TICKERTAPE", &type, (void **)&group);
    en_search(notify, "USER", &type, (void **)&user);
    en_search(notify, "TICKERTEXT", &type, (void **)&text);
    en_search(notify, "TIMEOUT", &type, (void **)&timeout);
    if (en_search(notify, "MIME_TYPE", &type, (void **)&mimeType) != 0)
    {
	mimeType = NULL;
    }

    if (en_search(notify, "MIME_ARGS", &type, (void **)&mimeArgs) != 0)
    {
	mimeArgs = NULL;
    }

    Subscription_deliverMessage(
	subscription,
	Message_alloc(group, user, text, *timeout, mimeType, mimeArgs));
}

/* Callback for elvin errors */
static void errorCallback(elvin_t connection, void *arg, elvin_error_code_t code, char *message)
{
    fprintf(stderr, "*** Elvin error %d (%s): exiting\n", code, message);
    exit(0);
}


/* Add the subscription info for an 'group' to the buffer */
void subscribeToItem(Subscription subscription, ElvinConnection self)
{
    char buffer[BUFFERSIZE];

    SANITY_CHECK(self);
    sprintf(buffer, "TICKERTAPE == \"%s\"", Subscription_getGroup(subscription));
    elvin_add_subscription(self -> connection, buffer, receiveCallback, subscription, 0);
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
    if ((self -> connection = elvin_connect(
	EC_NAMEDHOST, hostname, port,
	quenchCallback, NULL,
	errorCallback, NULL,
	1)) == NULL)
    {
	fprintf(stderr, "*** Unable to connect to elvin server at %s:%d\n", hostname, port);
	exit(0);
    }

    self -> subscriptions = subscriptions;
    List_doWith(self -> subscriptions, subscribeToItem, self);
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
    en_notify_t notify;

    SANITY_CHECK(self);
    timeout = Message_getTimeout(message);
    notify = en_new();
    en_add_string(notify, "TICKERTAPE", Message_getGroup(message));
    en_add_string(notify, "USER", Message_getUser(message));
    en_add_int32(notify, "TIMEOUT", timeout);
    en_add_string(notify, "TICKERTEXT", Message_getString(message));

    if (elvin_notify(self -> connection, notify) < 0)
    {
	fprintf(stderr, "*** Unable to send message\n");
	exit(0);
    }

    /* FIX THIS: do we need to en_free()? */
}


/* Call this when the connection has data available */
void ElvinConnection_read(ElvinConnection self)
{
    SANITY_CHECK(self);
    elvin_dispatch(self -> connection);
}
