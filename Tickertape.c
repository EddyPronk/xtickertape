/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: Tickertape.c,v 1.41 1999/08/09 10:47:00 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <X11/Intrinsic.h>
#include "sanity.h"
#include "Tickertape.h"
#include "Hash.h"
#include "StringBuffer.h"
#include "Scroller.h"
#include "Control.h"
#include "Subscription.h"
#include "UsenetSubscription.h"
#include "MailSubscription.h"
#include "ElvinConnection.h"
#ifdef ORBIT
#include "OrbitSubscription.h"
#endif /* ORBIT */

#ifdef SANITY
static char *sanity_value = "Ticker";
static char *sanity_freed = "Freed Ticker";
#endif /* SANITY */

#define DEFAULT_TICKERDIR ".ticker"
#define DEFAULT_GROUPS_FILE "groups"
#define DEFAULT_USENET_FILE "usenet"


/* The Tickertape data type */
struct Tickertape_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The user's name */
    char *user;

    /* The receiver's ticker directory */
    char *tickerDir;

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

    /* The receiver's mail subscription */
    MailSubscription mailSubscription;

#ifdef ORBIT
    /* The receiver's Orbit-related subscriptions */
    Hashtable orbitSubscriptionsById;
#endif /* ORBIT */
    /* The elvin connection */
    ElvinConnection connection;

    /* The control panel */
    ControlPanel controlPanel;

    /* The ScrollerWidget */
    ScrollerWidget scroller;
};


/*
 *
 * Static function headers
 *
 */
static int mkdirhier(char *dirname);
static void PublishStartupNotification(Tickertape self);
static void Click(Widget widget, Tickertape self, Message message);
static void ReceiveMessage(Tickertape self, Message message);
static List ReadGroupsFile(Tickertape self);
static void ReloadGroups(Tickertape self);
static void ReloadUsenet(Tickertape self);
static void InitializeUserInterface(Tickertape self);
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

/* Makes the named directory and any parent directories needed */
static int mkdirhier(char *dirname)
{
    char *pointer;
    struct stat statbuf;

    /* End the recursion */
    if (*dirname == '\0')
    {
	return 0;
    }

    /* Find the last '/' in the directory name */
    if ((pointer = strrchr(dirname, '/')) != NULL)
    {
	int result;

	/* Call ourselves recursively to make sure the parent
	 * directory exists */
	*pointer = '\0';
	result = mkdirhier(dirname);
	*pointer = '/';

	/* If we couldn't create the parent directory, then fail */
	if (result < 0)
	{
	    return result;
	}
    }

    /* If a file named `dirname' exists then make sure it's a directory */
    if (stat(dirname, &statbuf) == 0)
    {
	if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	{
	    errno = EEXIST;
	    return -1;
	}
	
	return 0;
    }

    /* If the directory doesn't exist then try to create it */
    if (errno == ENOENT)
    {
	return mkdir(dirname, 0777);
    }

    return -1;
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

/* Callback for a mouse click in the scroller */
static void Click(Widget widget, Tickertape self, Message message)
{
    SANITY_CHECK(self);
    ControlPanel_show(self -> controlPanel, message);
}

/* Receive a Message matched by Subscription */
static void ReceiveMessage(Tickertape self, Message message)
{
    SANITY_CHECK(self);
    ScAddMessage(self -> scroller, message);

    printf("new message received:\n");
    Message_debug(message);
    ControlPanel_addHistoryMessage(self -> controlPanel, message);
}

/* Read from the Groups file.  Returns a List of subscriptions if
 * successful, NULL otherwise */
static List ReadGroupsFile(Tickertape self)
{
    FILE *file;
    List subscriptions;

    /* Open the groups file and read */
    if ((file = Tickertape_groupsFile(self)) == NULL)
    {
	return List_alloc();
    }

    /* Read the file */
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

    /* Open the usenet file and read */
    if ((file = Tickertape_usenetFile(self)) == NULL)
    {
	return NULL;
    }

    /* Read the file */
    subscription = UsenetSubscription_readFromUsenetFile(
	file, (SubscriptionCallback)ReceiveMessage, self);
    fclose(file);

    return subscription;
}




/* Adds the subscription into the Hashtable using the expression as its key */
static void MapByExpression(Subscription subscription, Hashtable hashtable)
{
    Hashtable_put(hashtable, Subscription_expression(subscription), subscription);
}

/* Make sure that we're subscribed to the right stuff, but share elvin
 * subscriptions whenever possible */
static void UpdateElvin(
    Subscription subscription,
    Tickertape self,
    Hashtable hashtable,
    List list)
{
    Subscription match;
    SANITY_CHECK(self);

    /* Is there already a subscription for this? */
    if ((match = Hashtable_remove(hashtable, Subscription_expression(subscription))) == NULL)
    {
	/* No subscription yet.  Subscribe via elvin */
	Subscription_setConnection(subscription, self -> connection);
    }
    else
    {
	/* Subscription found.  Replace the new subscription with the old one */
	Subscription_updateFromSubscription(match, subscription);
	List_replaceAll(list, subscription, match);
	Subscription_free(subscription);
    }
}

/* Request from the ControlPanel to reload groups file */
static void ReloadGroups(Tickertape self)
{
    Hashtable hashtable;
    List newSubscriptions;
    int index;
    SANITY_CHECK(self);

    /* Read the new list of subscriptions */
    newSubscriptions = ReadGroupsFile(self);

    /* Reuse any elvin subscriptions if we can, create new ones we need */
    hashtable = Hashtable_alloc(37);
    List_doWith(self -> subscriptions, MapByExpression, hashtable);
    List_doWithWithWith(newSubscriptions, UpdateElvin, self, hashtable, newSubscriptions);

    /* Delete any elvin subscriptions we're no longer listening to,
     * remove them from the ControlPanel and free the corresponding
     * Subscriptions. */
    Hashtable_doWith(hashtable, Subscription_setConnection, NULL);
    Hashtable_doWith(hashtable, Subscription_setControlPanel, NULL);
    Hashtable_do(hashtable, Subscription_free);
    Hashtable_free(hashtable);
    List_free(self -> subscriptions);

    /* Set the list of group subscriptions to the new list */
    self -> subscriptions = newSubscriptions;

    /* Make sure that exactly those Subscriptions which should appear
     * the in groups menu do, and that they're in the right order */
    index = 0;
    List_doWithWith(
	self -> subscriptions,
	Subscription_updateControlPanelIndex,
	self -> controlPanel,
	&index);
}


/* Request from the ControlPanel to reload usenet file */
static void ReloadUsenet(Tickertape self)
{
    UsenetSubscription subscription;
    SANITY_CHECK(self);

    /* Read the new usenet subscription */
    subscription = ReadUsenetFile(self);

    /* Stop listening to old subscription */
    if (self -> usenetSubscription != NULL)
    {
	UsenetSubscription_setConnection(self -> usenetSubscription, NULL);
	UsenetSubscription_free(self -> usenetSubscription);
    }

    /* Start listening to the new subscription */
    self -> usenetSubscription = subscription;

    if (self -> usenetSubscription != NULL)
    {
	UsenetSubscription_setConnection(self -> usenetSubscription, self -> connection);
    }
}


/* Initializes the User Interface */
static void InitializeUserInterface(Tickertape self)
{
    self -> controlPanel = ControlPanel_alloc(
	self -> top, self -> user,
	(ReloadCallback)ReloadGroups, self,
	(ReloadCallback)ReloadUsenet, self);

    List_doWith(
	self -> subscriptions,
	Subscription_setControlPanel,
	self -> controlPanel);

    self -> scroller = (ScrollerWidget) XtVaCreateManagedWidget(
	"scroller", scrollerWidgetClass, self -> top,
	NULL);
    XtAddCallback((Widget)self -> scroller, XtNcallback, (XtCallbackProc)Click, self);
    XtRealizeWidget(self -> top);
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
    char *user, char *tickerDir,
    char *groupsFile, char *usenetFile,
    char *host, int port,
    Widget top)
{
    Tickertape self = (Tickertape) malloc(sizeof(struct Tickertape_t));
#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> user = strdup(user);
    self -> tickerDir = (tickerDir == NULL) ? NULL : strdup(tickerDir);
    self -> groupsFile = (groupsFile == NULL) ? NULL : strdup(groupsFile);
    self -> usenetFile = (usenetFile == NULL) ? NULL : strdup(usenetFile);
    self -> top = top;
    self -> messages = NULL;
    self -> subscriptions = ReadGroupsFile(self);
    self -> usenetSubscription = ReadUsenetFile(self);
    self -> mailSubscription = MailSubscription_alloc(
	user, (MailSubscriptionCallback)ReceiveMessage, self);
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

    /* Subscribe to the Mail subscription if we have one */
    if (self -> mailSubscription != NULL)
    {
	MailSubscription_setConnection(self -> mailSubscription, self -> connection);
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

    /* How do we free a ScrollerWidget? */

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else
    free(self);
#endif /* SANITY */
}

/* Debugging */
void Tickertape_debug(Tickertape self)
{
    SANITY_CHECK(self);

    printf("Tickertape\n");
    printf("  user = \"%s\"\n", self -> user);
    printf("  tickerDir = \"%s\"\n", (self -> tickerDir == NULL) ? "null" : self -> tickerDir);
    printf("  groupsFile = \"%s\"\n", (self -> groupsFile == NULL) ? "null" : self -> groupsFile);
    printf("  usenetFile = \"%s\"\n", (self -> usenetFile == NULL) ? "null" : self -> usenetFile);
    printf("  top = 0x%p\n", self -> top);
    printf("  subscriptions = 0x%p\n", self -> subscriptions);
#ifdef ORBIT
    printf("  orbitSubscriptionsById = 0x%p\n", self -> orbitSubscriptionsById);
#endif /* ORBIT */
    printf("  connection = 0x%p\n", self -> connection);
    printf("  controlPanel = 0x%p\n", self -> controlPanel);
    printf("  scroller = 0x%p\n", self -> scroller);
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
    if ((widget == self -> top) || (widget == (Widget) self -> scroller))
    {
	XtDestroyApplicationContext(XtWidgetToApplicationContext(widget));
	exit(0);
    }

    /* Otherwise close the control panel window */
    XtPopdown(widget);
}

/* Answers the receiver's tickerDir filename */
char *Tickertape_tickerDir(Tickertape self)
{
    /* See if we've already looked it up */
    if (self -> tickerDir != NULL)
    {
	return self -> tickerDir;
    }

    /* Use the TICKERDIR environment variable if it is set */
    if ((self -> tickerDir = getenv("TICKERDIR")) == NULL)
    {
	char *home;

	/* Else look up the user's home directory (default to '/' if not set) */
	if ((home = getenv("HOME")) == NULL)
	{
	    home = "/";
	}

	/* And append /.ticker to it to construct the directory name */
	self -> tickerDir = (char *)malloc(strlen(home) + sizeof(DEFAULT_TICKERDIR) + 1);
	strcpy(self -> tickerDir, home);
	strcat(self -> tickerDir, "/");
	strcat(self -> tickerDir, DEFAULT_TICKERDIR);
    }

    /* Make sure the TICKERDIR exists 
     * Note: we're being clever here and assuming that nothing will
     * call Tickertape_tickerDir() if both groupsFile and usenetFile
     * are set */
    if (mkdirhier(self -> tickerDir) < 0)
    {
	perror("unable to create tickertape directory");
    }

    return self -> tickerDir;
}

/* Answers the receiver's groups file filename */
char *Tickertape_groupsFilename(Tickertape self)
{
    if (self -> groupsFile == NULL)
    {
	char *dir = Tickertape_tickerDir(self);
	self -> groupsFile = (char *)malloc(strlen(dir) + sizeof(DEFAULT_GROUPS_FILE) + 1);
	strcpy(self -> groupsFile, dir);
	strcat(self -> groupsFile, "/");
	strcat(self -> groupsFile, DEFAULT_GROUPS_FILE);
    }

    return self -> groupsFile;
}

/* Answers the receiver's usenet filename */
char *Tickertape_usenetFilename(Tickertape self)
{
    if (self -> usenetFile == NULL)
    {
	char *dir = Tickertape_tickerDir(self);
	self -> usenetFile = (char *)malloc(strlen(dir) + sizeof(DEFAULT_USENET_FILE) + 1);
	strcpy(self -> usenetFile, dir);
	strcat(self -> usenetFile, "/");
	strcat(self -> usenetFile, DEFAULT_USENET_FILE);
    }

    return self -> usenetFile;
}


/* Answers the receiver's groups file */
FILE *Tickertape_groupsFile(Tickertape self)
{
    char *filename = Tickertape_groupsFilename(self);
    FILE *file;
    int result;

    /* Try to open the file */
    if ((file = fopen(filename, "r")) != NULL)
    {
	return file;
    }

    /* If failure was not due to a missing file, then fail */
    if (errno != ENOENT)
    {
	return NULL;
    }

    /* Try to create the file */
    if ((file = fopen(filename, "w")) == NULL)
    {
	return NULL;
    }

    /* Try to write a default groups file for the current user */
    result = Subscription_writeDefaultGroupsFile(file, self -> user);
    fclose(file);

    if (result < 0)
    {
	unlink(filename);
	return NULL;
    }

    /* Try to open the newly created file */
    return fopen(filename, "r");
}

/* Answers the receiver's usenet file */
FILE *Tickertape_usenetFile(Tickertape self)
{
    char *filename = Tickertape_usenetFilename(self);
    FILE *file;
    int result;

    /* Try to open the file */
    if ((file = fopen(filename, "r")) != NULL)
    {
	return file;
    }

    /* If failure was not due to a missing file, then fail */
    if (errno != ENOENT)
    {
	return NULL;
    }

    /* Try to create the file */
    if ((file = fopen(filename, "w")) == NULL)
    {
	return NULL;
    }

    /* Try to write a default usenet file for the current user */
    result = UsenetSubscription_writeDefaultUsenetFile(file);
    fclose(file);

    if (result < 0)
    {
	unlink(filename);
	return NULL;
    }

    /* Try to open the newly created file */
    return fopen(filename, "r");
}
