/* $Id: ElvinConnection.c,v 1.6 1997/02/25 06:41:15 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <elvin.h>

#include "ElvinConnection.h"
#include "Subscription.h"

#define BUFFERSIZE 8192

struct ElvinConnection_t
{
    elvin_t connection;
    uint32 subid;
    List subscriptions;
    char buffer[BUFFERSIZE];
    char *bpointer;
    ElvinConnectionCallback callback;
    void *context;
};



/* Callback for quench expressions */
static void (*quenchCallback)(elvin_t connection, char *quench) = NULL;

/* Callback for subscription thingo */
static void receiveCallback(elvin_t connection, uint32 id, en_notify_t notify)
{
    printf("receiveCallback\n");
    exit(0);
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
    char *out = self -> bpointer;
    char *in;

    if (List_first(self -> subscriptions) != subscription)
    {
	in = " || TICKERTAPE == \"";
    }
    else
    {
	in = "TICKERTAPE == \"";
    }
    while (*in != '\0')
    {
	*out++ = *in++;
    }

    in = Subscription_getGroup(subscription);
    while (*in != '\0')
    {
	*out++ = *in++;
    }

    *out++ = '"';
    self -> bpointer = out;
}

/* Send a subscription expression to the elvin server */
void subscribe(ElvinConnection self)
{
    self -> bpointer = self -> buffer;
    List_doWith(self -> subscriptions, subscribeToItem, self);
    *(self -> bpointer)++ = '\0';
#ifdef DEBUG
    printf("expression = \"%s\"\n", self -> buffer);
#endif /* DEBUG */
    self -> subid = elvin_add_subscription(self -> connection, self -> buffer, receiveCallback, 0);
    printf("subid = %d\n", self -> subid);
}


/* Posts a message to the callback */
static void postMessage(ElvinConnection self, Message message)
{
    if (self -> callback != NULL)
    {
	(*self -> callback)(message, self -> context);
    }
}

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname,
    int port,
    List subscriptions,
    ElvinConnectionCallback callback,
    void *context)
{
    ElvinConnection self;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(struct ElvinConnection_t));
    if ((self -> connection = elvin_connect(
	EC_NAMEDHOST, hostname, port,
	quenchCallback, errorCallback, NULL)) == NULL)
    {
	fprintf(stderr, "*** Unable to connect to elvin server at %s:%d\n", hostname, port);
	exit(0);
    }

    self -> callback = callback;
    self -> context = context;
    self -> subscriptions = subscriptions;
    subscribe(self);
    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
    /* Disconnect from elvin server */
    elvin_disconnect(self -> connection);

    /* Free our memory */
    free(self);
}


/* Answers the file descriptor for the elvin connection */
int ElvinConnection_getFD(ElvinConnection self)
{
    printf("fd = %d\n", elvin_get_socket(self -> connection));
    return elvin_get_socket(self -> connection);
}

/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, Message message)
{
    int32 timeout = Message_getTimeout(message) / 60;
    en_notify_t notify = en_new();
    en_add_string(notify, "TICKERTAPE", Message_getGroup(message));
    en_add_string(notify, "USER", Message_getUser(message));
    en_add_int32(notify, "TIMEOUT", timeout);
    en_add_string(notify, "TICKERTEXT", Message_getString(message));

    printf("sending message\n");
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
    en_notify_t notify;

    if (elvin_poll_notify(self -> connection, self -> subid, &notify) != 0)
    {
	printf("foo!\n");
	exit(1);
    }

    if (notify == NULL)
    {
	printf("notify: empty\n");
	exit(0);
    }
    else
    {
	printf("notify\"%s\"\n", en_notify_to_string(notify, " = ", "\n"));
    }
}
