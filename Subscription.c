/* $Id: Subscription.c,v 1.9 1998/10/21 02:44:52 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include "sanity.h"
#include "Subscription.h"

#define BUFFERSIZE 8192
#define SEPARATORS ":\n"

/* Sanity checking strings */
#ifdef SANITY
static char *sanity_value = "Subscription";
static char *sanity_freed = "Freed Subscription";
#endif /* SANITY */

/* The subscription data type */
struct Subscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The name of the receiver's group */
    char *group;

    /* The receiver's subscription expression */
    char *expression;

    /* Non-zero if the receiver should appear in the groups menu */
    int inMenu;

    /* Non-zero if the receiver's mime-enabled messages should automatically appear */
    int hasNazi;

    /* The minimum number of minutes to display a message in the receiver's group */
    int minTime;

    /* The maximum number of minutes to display a message in the receiver's group */
    int maxTime;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information (for unsubscribing) */
    void *connectionInfo;

    /* The receiver's ControlPanel */ 
    ControlPanel controlPanel;

    /* The receiver's ControlPanel info (for unsubscribing/retitling) */
    void *controlPanelInfo;

    /* The receiver's callback and argument */
    SubscriptionCallback callback;
    void *context;
};



/*
 *
 * Static function headers
 *
 */
static Subscription GetFromGroupFileLine(
    char *line, SubscriptionCallback callback, void *context);
static Subscription GetFromGroupFile(
    FILE *file, SubscriptionCallback callback, void *context);
static void HandleNotify(Subscription self, en_notify_t notification);
static void SendMessage(Subscription self, Message message);


/*
 *
 * Static functions
 *
 */

/* Create a subscription from a line of the group file */
static Subscription GetFromGroupFileLine(
    char *line, SubscriptionCallback callback, void *context)
{
    char *group = strtok(line, SEPARATORS);
    char *pointer = strtok(NULL, SEPARATORS);
    char *expression;
    int inMenu, hasNazi, minTime, maxTime;

    if (strcasecmp(pointer, "no menu") == 0)
    {
	inMenu = 0;
    }
    else if (strcasecmp(pointer, "menu") == 0)
    {
	inMenu = 1;
    }
    else
    {
	printf("unrecognized menu designation \"%s\" ignored\n", pointer);
	inMenu = 0;
    }

    pointer = strtok(NULL, SEPARATORS);
    if (strcasecmp(pointer, "auto") == 0)
    {
	hasNazi = 1;
    }
    else if (strcasecmp(pointer, "manual") == 0)
    {
	hasNazi = 0;
    }
    else
    {
	printf("unrecognized auto/manual designation \"%s\" ignored\n", pointer);
	hasNazi = 0;
    }

    pointer = strtok(NULL, SEPARATORS);
    minTime = atoi(pointer);
    if (minTime == 0)
    {
	printf("invalid minimum time: \"%s\"\n", pointer);
	minTime = 1;
    }

    pointer = strtok(NULL, SEPARATORS);
    maxTime = atoi(pointer);
    if (maxTime == 0)
    {
	printf("invalid maximum time: \"%s\"\n", pointer);
	maxTime = 60;
    }

    expression = strtok(NULL, "\n");
    if (expression == NULL)
    {
	char *buffer = (char *)alloca(strlen(group) + sizeof("TICKERTAPE == \"\""));

	sprintf(buffer, "TICKERTAPE == \"%s\"", group);
	return Subscription_alloc(
	    group, buffer, inMenu, hasNazi, minTime, maxTime, callback, context);
    }

    return Subscription_alloc(
	group, expression,
	inMenu, hasNazi,
	minTime, maxTime,
	callback, context);
}


/* Read the next subscription from file (answers NULL if EOF) */
static Subscription GetFromGroupFile(
    FILE *file, SubscriptionCallback callback, void *context)
{
    char buffer[BUFFERSIZE];
    char *pointer;

    /* find a non-empty line that doesn't begin with a '#' */
    while ((pointer = fgets(buffer, BUFFERSIZE, file)) != NULL)
    {
	/* skip whitespace */
	while ((*pointer == ' ') || (*pointer == '\t'))
	{
	    pointer++;
	}

	/* check for empty line or comment */
	if ((*pointer != '\0') && (*pointer != '\n') && (*pointer != '#'))
	{
	    return GetFromGroupFileLine(pointer, callback, context);
	}
    }

    return NULL;
}


/* Delivers a notification which matches the receiver's subscription expression */
static void HandleNotify(Subscription self, en_notify_t notification)
{
    Message message;
    en_type_t type;
    char *user;
    char *text;
    int32 *timeout;
    int32 value;
    char *mimeType;
    char *mimeArgs;
    SANITY_CHECK(self);

    /* If we don't have a callback then just quit now */
    if (self -> callback == NULL)
    {
	return;
    }
    
    /* Get the user from the notification (if provided) */
    if ((en_search(notification, "USER", &type, (void **)&user) != 0) ||
	(type != EN_STRING))
    {
	user = "anonymous";
    }

    /* Get the text of the notification (if provided) */
    if ((en_search(notification, "TICKERTEXT", &type, (void **)&text) != 0) ||
	(type != EN_STRING))
    {
	text = "";
    }

    /* Get the timeout for the notification (if provided) */
    if ((en_search(notification, "TIMEOUT", &type, (void **)&timeout) != 0) ||
	(type != EN_INT32))
    {
	timeout = &value;
	value = 0;

	if (type == EN_STRING)
	{
	    char *timeoutString;
	    en_search(notification, "TIMEOUT", &type, (void **)&timeoutString);
	    value = atoi(timeoutString);
	    printf("value=%d\n", value);
	}

	value = (value == 0) ? 10 : value;
    }

    /* Make sure the timeout conforms */
    if (*timeout < self -> minTime)
    {
	*timeout = self -> minTime;
    }
    else if (*timeout > self -> maxTime)
    {
	*timeout = self -> maxTime;
    }

    /* Get the MIME type (if provided) */
    if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) ||
	(type != EN_STRING))
    {
	mimeType = NULL;
    }

    /* Get the MIME args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) != 0) ||
	(type != EN_STRING))
    {
	mimeArgs = NULL;
    }

    /* Construct a message */
    message = Message_alloc(
	self -> controlPanelInfo, self -> group,
	user, text, *timeout,
	mimeType, mimeArgs);

    /* Deliver the message */
    (*self -> callback)(self -> context, message);
}

/* Sends a Message via this Subscription */
static void SendMessage(Subscription self, Message message)
{
    en_notify_t notification;
    int32 timeout;
    SANITY_CHECK(self);

    timeout = Message_getTimeout(message);
    notification = en_new();
    en_add_string(notification, "TICKERTAPE", self -> group);
    en_add_string(notification, "USER", Message_getUser(message));
    en_add_int32(notification, "TIMEOUT", timeout);
    en_add_string(notification, "TICKERTEXT", Message_getString(message));

    ElvinConnection_send(self -> connection, notification);
    en_free(notification);
}


/*
 *
 * Exported functions
 *
 */

/* Read Subscriptions from the group file 'groups' and add them to 'list' */
List Subscription_readFromGroupFile(
    FILE *groups, SubscriptionCallback callback, void *context)
{
    List list = List_alloc();
    Subscription subscription;

    while ((subscription = GetFromGroupFile(
	groups, callback, context)) != NULL)
    {
	List_addLast(list, subscription);
    }

    return list;
}


/* Answers a new Subscription */
Subscription Subscription_alloc(
    char *group,
    char *expression,
    int inMenu,
    int autoMime,
    int minTime,
    int maxTime,
    SubscriptionCallback callback,
    void *context)
{
    Subscription self = (Subscription) malloc(sizeof(struct Subscription_t));
#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> group = strdup(group);
    self -> expression = strdup(expression);
    self -> inMenu = inMenu;
    self -> hasNazi = autoMime;
    self -> minTime = minTime;
    self -> maxTime = maxTime;
    self -> controlPanel = NULL;
    self -> controlPanelInfo = NULL;
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> callback = callback;
    self -> context = context;

    return self;
}

/* Releases resources used by a Subscription */
void Subscription_free(Subscription self)
{
    SANITY_CHECK(self);

    if (self -> group)
    {
	free(self -> group);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#endif /* SANITY */
    free(self);
}

/* Prints debugging information */
void Subscription_debug(Subscription self)
{
    SANITY_CHECK(self);
    printf("Subscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */    
    printf("  group = \"%s\"\n", self -> group ? self -> group : "<none>");
    printf("  expression = \"%s\"\n", self -> expression ? self -> expression : "<none>");
    printf("  inMenu = %s\n", self -> inMenu ? "true" : "false");
    printf("  hasNazi = %s\n", self -> hasNazi ? "true" : "false");
    printf("  minTime = %d\n", self -> minTime);
    printf("  maxTime = %d\n", self -> maxTime);
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  controlPanel = %p\n", self -> controlPanel);
    printf("  controlPanelInfo = %p\n", self -> controlPanelInfo);
    printf("  callback = %p\n", self -> callback);
    printf("  context = %p\n", self -> context);
}


/* Answers the receiver's group */
char *Subscription_getGroup(Subscription self)
{
    SANITY_CHECK(self);
    return self -> group;
}


/* Answers the receiver's subscription expression */
char *Subscription_getExpression(Subscription self)
{
    SANITY_CHECK(self);
    return self -> expression;
}


/* Answers if the receiver should appear in the Control Panel menu */
int Subscription_isInMenu(Subscription self)
{
    SANITY_CHECK(self);
    return self -> inMenu;
}


/* Answers true if the receiver should automatically show mime messages */
int Subscription_isAutoMime(Subscription self)
{
    SANITY_CHECK(self);
    return self -> hasNazi;
}


/* Sets the receiver's ElvinConnection */
void Subscription_setConnection(Subscription self, ElvinConnection connection)
{
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    if (self -> connection != NULL)
    {
	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, self -> expression,
	    (NotifyCallback)HandleNotify, self);
    }
}


/* Registers the receiver with the ControlPanel */
void Subscription_setControlPanel(
    Subscription self, ControlPanel controlPanel)
{
    SANITY_CHECK(self);

    /* If it's the same control panel we had before then bail */
    if (self -> controlPanel == controlPanel)
    {
	return;
    }

    if (self -> controlPanel != NULL)
    {
	ControlPanel_removeSubscription(self -> controlPanel, self -> controlPanelInfo);
    }

    self -> controlPanel = controlPanel;

    if ((self -> controlPanel != NULL) && (self -> inMenu))
    {
	self -> controlPanelInfo = ControlPanel_addSubscription(
	    controlPanel,
	    self -> group,
	    (ControlPanelCallback) SendMessage,
	    self);
    }
}
