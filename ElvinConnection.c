/* $Id: ElvinConnection.c,v 1.19 1998/10/16 03:46:04 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#include <elvin3/elvin.h>
#include <elvin3/element.h>

#include "sanity.h"
#include "ElvinConnection.h"
#include "Subscription.h"

#define BUFFERSIZE 8192
#define INITIAL_PAUSE 1000 /* 1 second */
#define MAX_PAUSE (5 * 60 * 1000) /* 5 minutes */

#ifdef SANITY
static char *sanity_value = "ElvinConnection";
static char *sanity_freed = "Freed ElvinConnection";
#endif /* SANITY */

/* Static function headers */
static void ReceiveCallback(elvin_t elvin, void *object, uint32 id, en_notify_t notification);
static void ReceiveMetaCallback(elvin_t elvin, void *object, uint32 id, en_notify_t notification);
static void SubscribeToItem(Subscription subscription, ElvinConnection self);
static void SubscribeToMeta(ElvinConnection self);
static void ReadInput(ElvinConnection self);
static void PublishStartupNotification(ElvinConnection self);
static void Connect(ElvinConnection self);
static void Reconnect(ElvinConnection self);
static void ErrorCallback(elvin_t elvin, void *arg, elvin_error_code_t code, char *message);

typedef enum
{
    NeverConnected,
    Connected,
    LostConnection,
    ReconnectFailed
} ConnectionState;


/* The ElvinConnection data structure */
struct ElvinConnection_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The host and port of the elvin server */
    char *host;
    int port;

    /* The name of our user */
    char *user;

    /* Our elvin connection */
    elvin_t elvin;

    /* The state of our connection to the elvin server */
    ConnectionState state;

    /* The duration to wait before attempting to reconnect */
    unsigned long retryPause;
    
    /* The function to call to register the receiver with an event loop */
    EventLoopRegisterInputFunc registerFunc;

    /* The function to call to unregister the receiver with an event loop */
    EventLoopUnregisterInputFunc unregisterFunc;

    /* The function to register a timer callback with an event loop */
    EventLoopRegisterTimerFunc registerTimerFunc;

    /* The data returned from the register function we need to pass to unregister */
    void *registrationData;

    /* The special subscription we notify about elvin errors */
    Subscription errorSubscription;

    /* Our current list of subscriptions */
    List subscriptions;
};




/* Callback for quench expressions */
static void (*QuenchCallback)(elvin_t elvin, void *arg, char *quench) = NULL;

/* Callback for subscription thingo */
static void ReceiveCallback(elvin_t elvin, void *object, uint32 id, en_notify_t notification)
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
    if ((en_search(notification, "USER", &type, (void **)&user) != 0) || (type != EN_STRING))
    {
	user = "nobody";
    }

    /* Get the text of the notification (if any is provided) */
    if ((en_search(notification, "TICKERTEXT", &type, (void **)&text) != 0) || (type != EN_STRING))
    {
	text = "no message";
    }

    /* Get the timeout for the notification (if any is provided) */
    if ((en_search(notification, "TIMEOUT", &type, (void **)&timeout) != 0) || (type != EN_INT32))
    {
	*timeout = 10;
    }

    /* Get the MIME type of the notification (if provided) */
    if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) || (type != EN_STRING))
    {
	mimeType = NULL;
    }

    /* Get the MIME args of the notification (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) != 0) || (type != EN_STRING))
    {
	mimeArgs = NULL;
    }

    /* Deliver the notification to its subscription */
    Subscription_deliverMessage(
	subscription,
	Message_alloc(group, user, text, *timeout, mimeType, mimeArgs));
}

/* Receive callbacks about the meta subscription */
static void ReceiveMetaCallback(elvin_t elvin, void *object, uint32 id, en_notify_t notification)
{
    ElvinConnection self = (ElvinConnection) object;

    SANITY_CHECK(self);
    fprintf(stderr, "whoot!\n");
}




/* Subscribe to a tickertape "group" */
static void SubscribeToItem(Subscription subscription, ElvinConnection self)
{
    SANITY_CHECK(self);

    /* Don't try to subscribe if we've lost our connection */
    if (self -> state == Connected)
    {
	elvin_add_subscription(
	    self -> elvin, Subscription_getExpression(subscription),
	    ReceiveCallback, subscription, 0);
    }
}

/* Subscribe to the tickertape metagroup */
static void SubscribeToMeta(ElvinConnection self)
{
    char buffer[BUFFERSIZE];
    SANITY_CHECK(self);

    /* Don't try to subscribe if we've lost our elvin connection */
    if (self -> state != Connected)
    {
	return;
    }

    /* Listen for Orbit-related stuff */
    sprintf(
	buffer,
	"exists(orbit.view_update) && exists(tickertape) && user == \"%s\"",
	self -> user);
    elvin_add_subscription(self -> elvin, buffer, ReceiveMetaCallback, self, 0);
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

/* Publish a notification to say that we're up and running */
static void PublishStartupNotification(ElvinConnection self)
{
    en_notify_t notification;
    SANITY_CHECK(self);

    notification = en_new();
    en_add_string(notification, "tickertape.startup_notification", "1.0");
    en_add_string(notification, "user", self -> user);
    elvin_notify(self -> elvin, notification);
    en_free(notification);
}

/* Attempt to connect to the elvin server */
static void Connect(ElvinConnection self)
{
    if ((self -> elvin = elvin_connect(
	EC_NAMEDHOST, self -> host, self -> port,
	QuenchCallback, NULL, ErrorCallback, self, 1)) == NULL)
    {
	int pause = self -> retryPause;

	/* Is this the first time we've tried to connect? */
	if (self -> state == NeverConnected)
	{
	    char buffer[1024];

	    sprintf(buffer, "Unable to connect to elvin server %s:%d", self -> host, self -> port);
	    Subscription_deliverMessage(
		self -> errorSubscription,
		Message_alloc("tickertape", "tickertape", buffer, 10, NULL, NULL));
	}

	(*self -> registerTimerFunc)(pause, Connect, self);
	self -> retryPause = (pause * 2 > MAX_PAUSE) ? MAX_PAUSE : pause * 2;
	self -> state = ReconnectFailed;
    }
    else
    {
	if (self -> state != NeverConnected)
	{
	    char buffer[1024];
	    self -> retryPause = INITIAL_PAUSE;

	    sprintf(buffer, "Connected to %s:%d", self -> host, self -> port);
	    Subscription_deliverMessage(
		self -> errorSubscription,
		Message_alloc("tickertape", "tickertape", buffer, 10, NULL, NULL));
	}
	self -> state = Connected;

	self -> registrationData = (*self -> registerFunc)(elvin_get_socket(self -> elvin), ReadInput, self);
	List_doWith(self -> subscriptions, SubscribeToItem, self);

	/* Subscribe to changes in our subscription information */
	SubscribeToMeta(self);
	PublishStartupNotification(self);
    }
}

/* Callback for elvin errors */
static void ErrorCallback(elvin_t elvin, void *arg, elvin_error_code_t code, char *message)
{
    ElvinConnection self = (ElvinConnection) arg;
    char buffer[1024];
    SANITY_CHECK(self);

    /* Unregister our file descriptor */
    if (self -> registrationData != NULL)
    {
	(*self -> unregisterFunc)(self -> registrationData);
	self -> registrationData = NULL;
    }

    /* Notify the user that we've lost our connection */
    sprintf(buffer, "Lost connection to elvin server %s:%d, attempting to reconnect",
	    self -> host, self -> port);
    Subscription_deliverMessage(
	self -> errorSubscription,
	Message_alloc("tickertape", "tickertape", buffer, 10, NULL, NULL));
    self -> state = LostConnection;

    /* Try to reconnect */
    Connect(self);
}


/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname, int port, char *user,
    List subscriptions, Subscription errorSubscription)
{
    ElvinConnection self;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(struct ElvinConnection_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> host = hostname;
    self -> port = port;
    self -> user = strdup(user);
    self -> state = NeverConnected;
    self -> retryPause = INITIAL_PAUSE;
    self -> registerFunc = NULL;
    self -> unregisterFunc = NULL;
    self -> registerTimerFunc = NULL;
    self -> errorSubscription = errorSubscription;
    self -> subscriptions = subscriptions;

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
    if (self -> state == Connected)
    {
	elvin_disconnect(self -> elvin);
    }

    /* Free our memory */
    free(self);
}


/* Registers the connection with an event loop */
void ElvinConnection_register(
    ElvinConnection self,
    EventLoopRegisterInputFunc registerFunc,
    EventLoopUnregisterInputFunc unregisterFunc,
    EventLoopRegisterTimerFunc registerTimerFunc)
{
    self -> registerFunc = registerFunc;
    self -> unregisterFunc = unregisterFunc;
    self -> registerTimerFunc = registerTimerFunc;
    Connect(self);
}


/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, Message message)
{
    en_notify_t notification;
    int32 timeout;

    SANITY_CHECK(self);

    /* Don't even try if we're not connected */
    if (self -> state != Connected)
    {
	return;
    }

    timeout = Message_getTimeout(message);
    notification = en_new();
    en_add_string(notification, "TICKERTAPE", Message_getGroup(message));
    en_add_string(notification, "USER", Message_getUser(message));
    en_add_int32(notification, "TIMEOUT", timeout);
    en_add_string(notification, "TICKERTEXT", Message_getString(message));

    if (elvin_notify(self -> elvin, notification) < 0)
    {
	fprintf(stderr, "*** Unable to send message\n");
	exit(0);
    }

    en_free(notification);
}

