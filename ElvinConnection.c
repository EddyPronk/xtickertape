/* $Id: ElvinConnection.c,v 1.28 1998/10/29 03:38:59 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <elvin3/subscription.h>
#include "sanity.h"
#include "ElvinConnection.h"
#include "List.h"

#define INITIAL_PAUSE 1000 /* 1 second */
#define MAX_PAUSE (5 * 60 * 1000) /* 5 minutes */

#ifdef SANITY
static char *sanity_value = "ElvinConnection";
static char *sanity_freed = "Freed ElvinConnection";
#endif /* SANITY */


/* The receiver's state machine for connectivity status */
typedef enum
{
    NeverConnected,
    Connected,
    LostConnection,
    ReconnectFailed,
    Disconnected
} ConnectionState;

/* A tuple of subscription expression, callback, context for the Hashtable */
typedef struct SubscriptionTuple_t
{
    uint32 id;
    char *expression;
    NotifyCallback callback;
    void *context;
} *SubscriptionTuple;


/* The ElvinConnection data structure */
struct ElvinConnection_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The host and port of the elvin server */
    char *host;
    int port;

    /* Our elvin connection */
    elvin_t elvin;

    /* The state of our connection to the elvin server */
    ConnectionState state;

    /* The duration to wait before attempting to reconnect */
    unsigned long retryPause;

    /* The receiver's application context */
    XtAppContext app_context;
    
    /* The data returned from the register function we need to pass to unregister */
    XtInputId inputId;

    /* The disconnect callback function */
    DisconnectCallback disconnectCallback;

    /* The disconnect callback context */
    void *disconnectContext;

    /* The reconnect callback function */
    ReconnectCallback reconnectCallback;

    /* The reconnect callback context */
    void *reconnectContext;

    /* A Hashtable mapping expressions to callbacks */
    List subscriptions;
};


/* Callback for quench expressions (we don't use these) */
static void (*QuenchCallback)(elvin_t elvin, void *arg, char *quench) = NULL;


/*
 *
 * Static function headers 
 *
 */
static void Notify(
    elvin_t connection, void *context,
    uint32 subscriptionId, en_notify_t notification);
static void ReadInput(ElvinConnection self);
static void Resubscribe(SubscriptionTuple tuple, ElvinConnection self);
static void Connect(ElvinConnection self);
static void Error(elvin_t elvin, void *arg, elvin_error_code_t code, char *message);


/*
 *
 * Static functions
 *
 */

/* Callback for a notification */
static void Notify(
    elvin_t connection, void *context,
    uint32 subscriptionId, en_notify_t notification)
{
    SubscriptionTuple tuple = (SubscriptionTuple)context;

    (*tuple -> callback)(tuple -> context, notification);
}


/* Call this when the connection has data available */
static void ReadInput(ElvinConnection self)
{
    SANITY_CHECK(self);

    if (self -> state == Connected)
    {
	elvin_dispatch(self -> elvin);
    }
}


/* Resubscribe a subscription */
static void Resubscribe(SubscriptionTuple tuple, ElvinConnection self)
{
    SANITY_CHECK(self);

    if (self -> state == Connected)
    {
	tuple -> id = elvin_add_subscription(self -> elvin, tuple -> expression, Notify, tuple, 0);
    }
}

/* Attempt to connect to the elvin server */
static void Connect(ElvinConnection self)
{
    if ((self -> elvin = elvin_connect(
	EC_NAMEDHOST, self -> host, self -> port,
	QuenchCallback, NULL, Error, self, 1)) == NULL)
    {
	int pause = self -> retryPause;

	/* Is this the first time we've tried to connect? */
	if (self -> state == NeverConnected)
	{
	    (*self -> disconnectCallback)(self -> disconnectContext, self);
	}

	XtAppAddTimeOut(
	    self -> app_context, pause,
	    (XtTimerCallbackProc)Connect, (XtPointer)self);
	self -> retryPause = (pause * 2 > MAX_PAUSE) ? MAX_PAUSE : pause * 2;
	self -> state = ReconnectFailed;
    }
    else
    {
	if (self -> state != NeverConnected)
	{
	    (*self -> reconnectCallback)(self -> reconnectContext, self);
	}

	self -> state = Connected;

	self -> inputId = XtAppAddInput(
	    self -> app_context, elvin_get_socket(self -> elvin), 
	    (XtPointer)XtInputReadMask,
	    (XtInputCallbackProc)ReadInput, (XtPointer)self);
	List_doWith(self -> subscriptions, Resubscribe, self);
    }
}

/* Callback for elvin errors */
static void Error(elvin_t elvin, void *arg, elvin_error_code_t code, char *message)
{
    ElvinConnection self = (ElvinConnection) arg;
    SANITY_CHECK(self);

    /* Unregister our file descriptor */
    if (self -> inputId != 0)
    {
	XtRemoveInput(self -> inputId);
	self -> inputId = 0;
    }

    /* Notify the user that we've lost our connection */
    self -> state = LostConnection;
    (*self -> disconnectCallback)(self -> disconnectContext, self);

    /* Try to reconnect */
    Connect(self);
}


/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname, int port, XtAppContext app_context,
    DisconnectCallback disconnectCallback, void *disconnectContext,
    ReconnectCallback reconnectCallback, void *reconnectContext)
{
    ElvinConnection self;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(struct ElvinConnection_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> host = hostname;
    self -> port = port;
    self -> state = NeverConnected;
    self -> retryPause = INITIAL_PAUSE;
    self -> app_context = app_context;
    self -> inputId = 0;
    self -> disconnectCallback = disconnectCallback;
    self -> disconnectContext = disconnectContext;
    self -> reconnectCallback = reconnectCallback;
    self -> reconnectContext = reconnectContext;
    self -> subscriptions = List_alloc();
    Connect(self);

    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
    SANITY_CHECK(self);

    /* Disconnect from elvin server */
    if (self -> state == Connected)
    {
	elvin_disconnect(self -> elvin);
	self -> state = Disconnected;
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else
    free(self);
#endif /* SANITY */
}

/* Answers the receiver's host */
char *ElvinConnection_host(ElvinConnection self)
{
    return self -> host;
}

/* Answers the receiver's port */
int ElvinConnection_port(ElvinConnection self)
{
    return self -> port;
}



/* Registers a callback for when the given expression is matched */
void *ElvinConnection_subscribe(
    ElvinConnection self, char *expression,
    NotifyCallback callback, void *context)
{
    SubscriptionTuple tuple;
    es_ptree_t parseTree;
    char *error;
    SANITY_CHECK(self);

    /* Check the expression to see if it's valid before we make a tuple for it */
    if ((parseTree = sub_parse(expression, &error)) == NULL)
    {
	fprintf(stderr, "*** Error %s\n", error);
	return NULL;
    }
    es_free_ptree(parseTree);

    tuple = malloc(sizeof(struct SubscriptionTuple_t));
    tuple -> expression = strdup(expression);
    tuple -> callback = callback;
    tuple -> context = context;
    Resubscribe(tuple, self);
    List_addLast(self -> subscriptions, tuple);

    return tuple;
}

/* Unregisters a callback (info was returned by ElvinConnection_subscribe) */
void ElvinConnection_unsubscribe(ElvinConnection self, void *info)
{
    SubscriptionTuple tuple;
    SANITY_CHECK(self);

    /* Remove the tuple from the List of subscriptions (if not there then quit) */
    if ((tuple = List_remove(self -> subscriptions, info)) == NULL)
    {
	return;
    }
    
    /* Only need to unsubscribe if we're connected */
    if (self -> state == Connected)
    {
	elvin_del_subscription(self -> elvin, tuple -> id);
    }

    /* Release the tuple's memory */
    free(tuple -> expression);
    free(tuple);
}


/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, en_notify_t notification)
{
    SANITY_CHECK(self);

    /* FIX THIS: should we enqueue the notification if we're not connected? */
    /* Don't even try if we're not connected */
    if (self -> state == Connected)
    {
	elvin_notify(self -> elvin, notification);
    }
}
