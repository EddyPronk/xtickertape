/* $Id */

#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include "UsenetSubscription.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "UsenetSubscription";
static char *sanity_freed = "Freed UsenetSubscription";
#endif /* SANITY */


/* The UsenetSubscription data type */
struct UsenetSubscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's subscription expression */
    char *expression;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information */
    void *connectionInfo;

    /* The receiver's callback function */
    UsenetSubscriptionCallback callback;

    /* The receiver's callback context */
    void *context;
};


/*
 *
 * Static function headers
 *
 */
static void HandleNotify(UsenetSubscription self, en_notify_t notification);

/*
 *
 * Static functions
 *
 */

/* Transforms a usenet notification into a Message and delivers it */
static void HandleNotify(UsenetSubscription self, en_notify_t notification)
{
    Message message;
    en_type_t type;
    char *user;
    char *string;
    char *mimeType;
    char *mimeArgs;
    char *name;
    char *email;
    char *newsgroups;
    char *pointer;

    /* First set up the user name */

    /* Get the name from the FROM field (if provided) */
    if ((en_search(notification, "FROM_NAME", &type, (void **)&name) != 0) ||
	(type != EN_STRING))
    {
	name = "anonymous";
    }

    /* Get the e-mail address (if provided) */
    if ((en_search(notification, "FROM_EMAIL", &type, (void **)&email) != 0) ||
	(type != EN_STRING))
    {
	email = "anonymous";
    }

    /* Get the newsgroups to which the message was posted */
    if ((en_search(notification, "NEWSGROUPS", &type, (void **)&newsgroups) != 0) ||
	(type != EN_STRING))
    {
	newsgroups = "news";
    }

    /* Locate the first newsgroup to which the message was posted */
    for (pointer = newsgroups; *pointer != '\0'; pointer++)
    {
	if (*pointer == ',')
	{
	    *pointer = '\0';
	}

	break;
    }

    /* If the name and e-mail addresses are identical, just use one as the user name */
    if (strcmp(name, email) == 0)
    {
	user = (char *)alloca(sizeof(": ") + strlen(name) + strlen(newsgroups));
	sprintf(user, "%s: %s", name, newsgroups);
    }
    /* Otherwise construct the user name from the name and e-mail field */
    else
    {
	user = (char *)alloca(sizeof(" <>: ") + strlen(name) + strlen(email) + strlen(newsgroups));
	sprintf(user, "%s <%s>: %s", name, email, newsgroups);
    }


    /* Construct the text of the Message */

    /* Get the SUBJECT field (if provided) */
    if ((en_search(notification, "SUBJECT", &type, (void **)&string) != 0) ||
	(type != EN_STRING))
    {
	string = "[no subject]";
    }


    /* Construct the mime attachment information (if provided) */

    /* Get the MIME_ARGS field to use as the mime args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) == 0) &&
	(type == EN_STRING))
    {
	/* Get the MIME_TYPE field (if provided) */
	if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) ||
	    (type != EN_STRING))
	{
	    mimeType = "x-elvin/url";
	}
    }
    /* No MIME_ARGS provided.  Use the Message-Id field */
    else if ((en_search(notification, "Message-Id", &type, (void **)&mimeArgs) == 0) &&
	    (type == EN_STRING))
    {
	mimeType = "x-elvin/news";
    }
    /* No Message-Id field provied either */
    else
    {
	mimeType = NULL;
	mimeArgs = NULL;
    }

    /* Construct a Message out of all of that */
    message = Message_alloc(NULL, "usenet", user, string, 60, mimeType, mimeArgs, 0, 0);

    /* Deliver the Message */
    (*self -> callback)(self -> context, message);
}


/*
 *
 * Exported functions
 *
 */

/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context)
{
    UsenetSubscription self = (UsenetSubscription) malloc(sizeof(struct UsenetSubscription_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> expression = strdup(expression);
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> callback = callback;
    self -> context = context;
    return self;
}

/* Releases the resources used by an UsenetSubscription */
void UsenetSubscription_free(UsenetSubscription self)
{
    SANITY_CHECK(self);

    /* Free the receiver's expression if it has one */
    if (self -> expression)
    {
	free(self -> expression);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}

/* Prints debugging information */
void UsenetSubscription_debug(UsenetSubscription self)
{
    SANITY_CHECK(self);

    printf("UsenetSubscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  expression = \"%s\"\n", self -> expression);
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  callback = %p\n", self -> callback);
    printf("  context = %p\n", self -> context);
}

/* Sets the receiver's ElvinConnection */
void UsenetSubscription_setConnection(UsenetSubscription self, ElvinConnection connection)
{
    SANITY_CHECK(self);

    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from the old connection */
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    /* Subscribe to the new connection */
    if (self -> connection != NULL)
    {
	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, self -> expression,
	    (NotifyCallback)HandleNotify, self);
    }
}

