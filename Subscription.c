/* $Id: Subscription.c,v 1.7 1998/10/16 04:15:02 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sanity.h"
#include "Subscription.h"

#define BUFFERSIZE 8192
#define SEPARATORS ":\n"

/* Sanity checking strings */
#ifdef SANITY
static char *sanity_value = "Subscription";
static char *sanity_freed = "Freed Subscription";
#endif /* SANITY */

/* Static function definitions */
static Subscription GetFromGroupFileLine(char *line, SubscriptionCallback callback, void *context);
static Subscription GetFromGroupFile(FILE *file, SubscriptionCallback callback, void *context);


/* The subscription data type */
struct Subscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */
    char *group;
    char *expression;
    int inMenu;
    int hasNazi;
    int minTime;
    int maxTime;
    SubscriptionCallback callback;
    void *context;
};


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
}


/* Create a subscription from a line of the group file */
static Subscription GetFromGroupFileLine(char *line, SubscriptionCallback callback, void *context)
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
	char buffer[BUFFERSIZE];

	sprintf(buffer, "TICKERTAPE == \"%s\"", group);
	return Subscription_alloc(
	    group, buffer, inMenu, hasNazi, minTime, maxTime, callback, context);
    }

    return Subscription_alloc(
	group, expression, inMenu, hasNazi, minTime, maxTime, callback, context);
}

/* Read the next subscription from file (answers NULL if EOF) */
static Subscription GetFromGroupFile(FILE *file, SubscriptionCallback callback, void *context)
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


/* Read Subscriptions from the group file 'groups' and add them to 'list' */
void Subscription_readFromGroupFile(FILE *groups, List list, SubscriptionCallback callback, void *context)
{
    Subscription subscription;

    while ((subscription = GetFromGroupFile(groups, callback, context)) != NULL)
    {
	List_addLast(list, subscription);
    }
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

/* Answers the adjusted timeout for a message in this group */
void Subscription_adjustTimeout(Subscription self, Message message)
{
    unsigned long timeout;

    SANITY_CHECK(self);
    timeout = Message_getTimeout(message);
    if (timeout < self -> minTime)
    {
	Message_setTimeout(message, self -> minTime);
	return;
    }

    if (timeout > self -> maxTime)
    {
	Message_setTimeout(message, self -> maxTime);
	return;
    }
}


/* Delivers a Message received because of the receiver */
void Subscription_deliverMessage(Subscription self, Message message)
{
    SANITY_CHECK(self);

    /* Make sure the timeout conforms */
    Subscription_adjustTimeout(self, message);

    if (self -> callback != NULL)
    {
	(*self -> callback)(message, self -> context);
    }
}
