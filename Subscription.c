/* $Id: Subscription.c,v 1.2 1997/02/17 01:21:55 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Subscription.h"

#define BUFFERSIZE 8192
#define SEPARATORS ":\n"

/* The subscription data type */
struct Subscription_t
{
    char *group;
    int inMenu;
    int hasNazi;
    int minTime;
    int maxTime;
};


/* Answers a new Subscription */
Subscription Subscription_alloc(char *group, int inMenu, int autoMime, int minTime, int maxTime)
{
    Subscription self = (Subscription) malloc(sizeof(struct Subscription_t));
    self -> group = strdup(group);
    self -> inMenu = inMenu;
    self -> hasNazi = autoMime;
    self -> minTime = minTime;
    self -> maxTime = maxTime;
    return self;
}

/* Releases resources used by a Subscription */
void Subscription_free(Subscription self)
{
    if (self -> group)
    {
	free(self -> group);
    }

    free(self);
}

/* Prints debugging information */
void Subscription_debug(Subscription self)
{
    printf("Subscription (0x%p)\n", self);
    printf("  group = \"%s\"\n", self -> group ? self -> group : "<none>");
    printf("  inMenu = %s\n", self -> inMenu ? "true" : "false");
    printf("  hasNazi = %s\n", self -> hasNazi ? "true" : "false");
    printf("  minTime = %d\n", self -> minTime);
    printf("  maxTime = %d\n", self -> maxTime);
}


/* Create a subscription from a line of the group file */
Subscription getFromGroupFileLine(char *line)
{
    char *group = strtok(line, SEPARATORS);
    char *pointer = strtok(NULL, SEPARATORS);
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

    return Subscription_alloc(group, inMenu, hasNazi, minTime, maxTime);
}

/* Read the next subscription from file (answers NULL if EOF) */
Subscription getFromGroupFile(FILE *file)
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
	    return getFromGroupFileLine(pointer);
	}
    }

    return NULL;
}


/* Read Subscriptions from the group file 'groups' and add them to 'list' */
void Subscription_readFromGroupFile(FILE *groups, List list)
{
    Subscription subscription;

    while ((subscription = getFromGroupFile(groups)) != NULL)
    {
	List_addLast(list, subscription);
    }
}


/* Answers the receiver's group */
char *Subscription_getGroup(Subscription self)
{
    return self -> group;
}

/* Answers if the receiver should appear in the Control Panel menu */
int Subscription_isInMenu(Subscription self)
{
    return self -> inMenu;
}

/* Answers true if the receiver should automatically show mime messages */
int Subscription_isAutoMime(Subscription self)
{
    return self -> hasNazi;
}

/* Answers the adjusted timeout for a message in this group */
int Subscription_adjustTimeout(Subscription self, int timeout)
{
    if (timeout < self -> minTime)
    {
	return self -> minTime;
    }

    if (timeout > self -> maxTime)
    {
	return self -> maxTime;
    }

    return timeout;
}
