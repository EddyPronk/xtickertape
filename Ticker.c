/*
 * $Id: Ticker.c,v 1.16 1998/10/29 03:40:14 phelps Exp $
 * COPYRIGHT!
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include "sanity.h"
#include "Ticker.h"
#include "Hash.h"
#include "StringBuffer.h"
#include "Tickertape.h"
#include "Control.h"
#include "Subscription.h"
#include "UsenetSubscription.h"
#include "ElvinConnection.h"
#include "version.h"


#ifdef SANITY
static char *sanity_value = "Ticker";
static char *sanity_freed = "Freed Ticker";
#endif /* SANITY */

/* The Tickertape data type */
struct Tickertape_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The user's name */
    char *user;

    /* The group file from which we read our subscriptions */
    char *groupsFile;

    /* The usenet file from which we read our usenet subscription */
    char *usenetFile;

    /* The top-level widget */
    Widget top;

    /* The receiver's subscriptions (from the groups file) */
    List subscriptions;

    /* The receiver's usenet subscription (from the usenet file) */
    UsenetSubscription usenetSubscription;

#ifdef ORBIT
    /* The receiver's Orbit-related subscriptions */
    Hashtable orbitSubscriptionsById;
#endif /* ORBIT */
    /* The elvin connection */
    ElvinConnection connection;

    /* The control panel */
    ControlPanel controlPanel;

    /* The TickertapeWidget */
    TickertapeWidget tickertape;
};


/*
 *
 * Static function headers
 *
 */
static void Click(Widget widget, Tickertape self, Message message);
static void ReceiveMessage(Tickertape self, Message message);
static void InitializeUserInterface(Tickertape self);
static List ReadGroupsFile(Tickertape self);
static void PublishStartupNotification(Tickertape self);
static void Disconnect(Tickertape self, ElvinConnection connection);
static void Reconnect(Tickertape self, ElvinConnection connection);
#ifdef ORBIT
static void OrbitCallback(Tickertape self, en_notify_t notification);
static void SubscribeToOrbit(Tickertape self);
#endif /* ORBIT */


/*
 *
 * Static functions
 *
 */

/* Callback for a mouse click in the tickertape scroller */
static void Click(Widget widget, Tickertape self, Message message)
{
    SANITY_CHECK(self);
    ControlPanel_show(self -> controlPanel, message);
}

/* Receive a Message matched by Subscription */
static void ReceiveMessage(Tickertape self, Message message)
{
    SANITY_CHECK(self);
    TtAddMessage(self -> tickertape, message);
}


/* Initializes the User Interface */
static void InitializeUserInterface(Tickertape self)
{
    self -> controlPanel = ControlPanel_alloc(self -> top, self -> user);
    List_doWith(
	self -> subscriptions,
	Subscription_setControlPanel,
	self -> controlPanel);

    self -> tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"scroller", tickertapeWidgetClass, self -> top,
	NULL);
    XtAddCallback((Widget)self -> tickertape, XtNcallback, (XtCallbackProc)Click, self);
    XtRealizeWidget(self -> top);
}


/* Read from the Groups file.  Returns a List of subscriptions if
 * successful, NULL otherwise */
static List ReadGroupsFile(Tickertape self)
{
    FILE *file;
    List subscriptions;
    
    /* No groups file, no subscriptions list */
    if (self -> groupsFile == NULL)
    {
	return List_alloc();
    }

    /* Open the groups file and read */
    if ((file = fopen(self -> groupsFile, "r")) == NULL)
    {
	fprintf(stderr, "*** unable to open groups file %s\n", self -> groupsFile);
	return List_alloc();
    }

    /* Read the subscriptions */
    subscriptions = Subscription_readFromGroupFile(
	file, (SubscriptionCallback)ReceiveMessage, self);
    fclose(file);

    return subscriptions;
}


/* Read from the usenet file.  Returns a Usenet subscription. */
static UsenetSubscription ReadUsenetFile(Tickertape self)
{
    FILE *file;
    UsenetSubscription subscription;

    if ((file = fopen(self -> usenetFile, "r")) == NULL)
    {
	fprintf(stderr, "*** unable to open usenet file %s\n", self -> usenetFile);
	return NULL;
    }

    /* Read the file */
    subscription = UsenetSubscription_readFromUsenetFile(
	file, (SubscriptionCallback)ReceiveMessage, self);
    fclose(file);

    return subscription;
}


/* Publishes a notification indicating that the receiver has started */
static void PublishStartupNotification(Tickertape self)
{
    en_notify_t notification;
    SANITY_CHECK(self);

    /* If we haven't managed to connect, then don't try sending */
    if (self -> connection == NULL)
    {
	return;
    }

    notification = en_new();
    en_add_string(notification, "tickertape.startup", VERSION);
    en_add_string(notification, "user", self -> user);
    ElvinConnection_send(self -> connection, notification);
    en_free(notification);
}

/* This is called when we get our elvin connection back */
static void Reconnect(Tickertape self, ElvinConnection connection)
{
    StringBuffer buffer;
    Message message;
    SANITY_CHECK(self);

    /* Construct a reconnect message */
    buffer = StringBuffer_alloc();
    StringBuffer_append(buffer, "Connected to elvin server at ");
    StringBuffer_append(buffer, ElvinConnection_host(connection));
    StringBuffer_appendChar(buffer, ':');
    StringBuffer_appendInt(buffer, ElvinConnection_port(connection));
    StringBuffer_appendChar(buffer, '.');

    /* Display the message on the scroller */
    message = Message_alloc(
	NULL,
	"internal", "tickertape",
	StringBuffer_getBuffer(buffer), 10,
	NULL, NULL,
	0, 0);
    ReceiveMessage(self, message);

    /* Republish the startup notification */
    PublishStartupNotification(self);
}

/* This is called when we lose our elvin connection */
static void Disconnect(Tickertape self, ElvinConnection connection)
{
    StringBuffer buffer;
    Message message;
    SANITY_CHECK(self);

    /* Construct a disconnect message */
    buffer = StringBuffer_alloc();

    /* If this is called in the middle of ElvinConnection_alloc, then
     * self -> connection will be NULL.  Let the user know that we've
     * never managed to connect. */
    if (self -> connection == NULL)
    {
	StringBuffer_append(buffer, "Unable to connect to elvin server at ");
    }
    else
    {
	StringBuffer_append(buffer, "Lost connection to elvin server at ");
    }

    StringBuffer_append(buffer, ElvinConnection_host(connection));
    StringBuffer_appendChar(buffer, ':');
    StringBuffer_appendInt(buffer, ElvinConnection_port(connection));

    if (self -> connection == NULL)
    {
	StringBuffer_appendChar(buffer, '.');
    }
    else
    {
	StringBuffer_append(buffer, ".  Attempting to reconnect.");
    }

    /* Display the message on the scroller */
    message = Message_alloc(
	NULL,
	"internal", "tickertape",
	StringBuffer_getBuffer(buffer), 10,
	NULL, NULL,
	0, 0);
    ReceiveMessage(self, message);
    StringBuffer_free(buffer);
}



#ifdef ORBIT
/*
 *
 * Orbit-related functions
 *
 */

/* Callback for when we match the Orbit notification */
static void OrbitCallback(Tickertape self, en_notify_t notification)
{
    en_type_t type;
    char *id;
    char *tickertape;
    SANITY_CHECK(self);

    /* Get the id of the zone (if provided) */
    if ((en_search(notification, "zone.id", &type, (void **)&id) != 0) || (type != EN_STRING))
    {
	/* Can't subscribe without a zone id */
	return;
    }

    /* Get the status (if provided) */
    if ((en_search(notification, "tickertape", &type, (void **)&tickertape) != 0) ||
	(type != EN_STRING))
    {
	/* Not provided or not a string -- can't determine a useful course of action */
	return;
    }

    /* See if we're subscribing */
    if (strcasecmp(tickertape, "true") == 0)
    {
	OrbitSubscription subscription;
	char *title;

	/* Get the title of the zone (if provided) */
	if ((en_search(notification, "zone.title", &type, (void **)&title) != 0) ||
	    (type != EN_STRING))
	{
	    title = "Untitled Zone";
	}

	/* If we already have a subscription, then update the title and quit */
	if ((subscription = Hashtable_get(self -> orbitSubscriptionsById, id)) != NULL)
	{
	    OrbitSubscription_setTitle(subscription, title);
	    return;
	}

	/* Otherwise create a new Subscription and record it in the table */
	subscription = OrbitSubscription_alloc(
	    title, id,
	    (OrbitSubscriptionCallback)ReceiveMessage, self);
	Hashtable_put(self -> orbitSubscriptionsById, id, subscription);

	/* Register the subscription with the ElvinConnection and the ControlPanel */
	OrbitSubscription_setConnection(subscription, self -> connection);
	OrbitSubscription_setControlPanel(subscription, self -> controlPanel);
    }

    /* See if we're unsubscribing */
    if (strcasecmp(tickertape, "false") == 0)
    {
	OrbitSubscription subscription = Hashtable_remove(self -> orbitSubscriptionsById, id);
	if (subscription != NULL)
	{
	    OrbitSubscription_setConnection(subscription, NULL);
	    OrbitSubscription_setControlPanel(subscription, NULL);
	}
    }
}


/* Subscribes to the Orbit meta-subscription */
static void SubscribeToOrbit(Tickertape self)
{
    StringBuffer buffer;
    SANITY_CHECK(self);

    /* Construct the subscription expression */
    buffer = StringBuffer_alloc();
    StringBuffer_append(buffer, "exists(orbit.view_update) && exists(tickertape) && ");
    StringBuffer_append(buffer, "user == \"");
    StringBuffer_append(buffer, self -> user);
    StringBuffer_append(buffer, "\"");

    /* Subscribe to the meta-subscription */
    ElvinConnection_subscribe(
	self -> connection, StringBuffer_getBuffer(buffer),
	(NotifyCallback)OrbitCallback, self);

    StringBuffer_free(buffer);
}

#endif /* ORBIT */

/*
 *
 * Exported function definitions 
 *
 */

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service
 * specified by host and port */
Tickertape Tickertape_alloc(
    char *user,
    char *groupsFile, char *usenetFile,
    char *host, int port,
    Widget top)
{
    Tickertape self = (Tickertape) malloc(sizeof(struct Tickertape_t));
#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> user = strdup(user);
    self -> groupsFile = strdup(groupsFile);
    self -> usenetFile = strdup(usenetFile);
    self -> top = top;
    self -> subscriptions = ReadGroupsFile(self);
    self -> usenetSubscription = ReadUsenetFile(self);
    self -> connection = NULL;

    InitializeUserInterface(self);

    /* Connect to elvin and subscribe */
    self -> connection = ElvinConnection_alloc(
	host, port, 	XtWidgetToApplicationContext(top),
	(DisconnectCallback)Disconnect, self,
	(ReconnectCallback)Reconnect, self);
    List_doWith(self -> subscriptions, Subscription_setConnection, self -> connection);

    /* Subscribe to the Usenet subscription if we have one */
    if (self -> usenetSubscription != NULL)
    {
	UsenetSubscription_setConnection(self -> usenetSubscription, self -> connection);
    }

#ifdef ORBIT
    /* Listen for Orbit-related notifications and alert the world to our presence */
    self -> orbitSubscriptionsById = Hashtable_alloc(37);
    SubscribeToOrbit(self);
#endif /* ORBIT */

    PublishStartupNotification(self);
    return self;
}

/* Free the resources consumed by a Tickertape */
void Tickertape_free(Tickertape self)
{
    SANITY_CHECK(self);

    if (self -> user)
    {
	free(self -> user);
    }

    if (self -> groupsFile)
    {
	free(self -> groupsFile);
    }

    /* How do we free a Widget? */

    if (self -> subscriptions)
    {
	List_free(self -> subscriptions);
    }

#ifdef ORBIT
    if (self -> orbitSubscriptionsById)
    {
	Hashtable_free(self -> orbitSubscriptionsById);
    }
#endif /* ORBIT */

    if (self -> connection)
    {
	ElvinConnection_free(self -> connection);
    }

    if (self -> controlPanel)
    {
	ControlPanel_free(self -> controlPanel);
    }

    /* How do we free a TickertapeWidget? */

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else
    free(self);
#endif /* SANITY */
}


/* Handle the notify action */
void Tickertape_handleNotify(Tickertape self, Widget widget)
{
    /* Pass this on to the control panel */
    ControlPanel_handleNotify(self -> controlPanel, widget);
}


/* Handle the quit action */
void Tickertape_handleQuit(Tickertape self, Widget widget)
{
    /* If we get the quit action from the top-level widget or the tickertape then quit */
    if ((widget == self -> top) || (widget == (Widget) self -> tickertape))
    {
	XtDestroyApplicationContext(XtWidgetToApplicationContext(widget));
	exit(0);
    }

    /* Otherwise close the control panel window */
    XtPopdown(widget);
}

/* Debugging */
void Tickertape_debug(Tickertape self)
{
    SANITY_CHECK(self);

    printf("Tickertape\n");
    printf("  user = \"%s\"\n", self -> user);
    printf("  groupsFile = \"%s\"\n", self -> groupsFile);
    printf("  usenetFile = \"%s\"\n", self -> usenetFile);
    printf("  top = 0x%p\n", self -> top);
    printf("  subscriptions = 0x%p\n", self -> subscriptions);
#ifdef ORBIT
    printf("  orbitSubscriptionsById = 0x%p\n", self -> orbitSubscriptionsById);
#endif /* ORBIT */
    printf("  connection = 0x%p\n", self -> connection);
    printf("  controlPanel = 0x%p\n", self -> controlPanel);
    printf("  tickertape = 0x%p\n", self -> tickertape);
}

