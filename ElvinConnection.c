/* $Id: ElvinConnection.c,v 1.4 1997/02/14 10:52:32 phelps Exp $ */


#include <stdio.h>
#include <stdlib.h>
#ifdef ELVIN
#include <elvin.h>
#endif /* ELVIN */

#include "ElvinConnection.h"

#define BUFFERSIZE 8192

struct ElvinConnection_t
{
#ifdef ELVIN
    static elvin_t connection;
#endif /* ELVIN */
    ElvinConnectionCallback callback;
    void *context;
    List subscriptions;
    char buffer[BUFFERSIZE];
    char *bpointer;
};



/* Set up callbacks */
#ifdef ELVIN
static void (*quench_function)(elvin_t connection, char *quench) = NULL;

static void error(elvin_t connection, void *arg, elvin_error_code_t code, char *message)
{
    fprintf(stderr, "*** Elvin error %d (%s): exiting\n", code, message);
    exit(0);
}
#endif


/* Add the subscription info for an 'group' to the buffer */
void subscribeToItem(char *item, ElvinConnection self)
{
    char *out = self -> bpointer;
    char *in;

    if (List_first(self -> subscriptions) != item)
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

    in = item;
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
    printf("expression = \"%s\"\n", self -> buffer);
}

/* Posts a message to the callback */
void postMessage(ElvinConnection self, Message message)
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
#ifdef ELVIN
    if ((self -> connection = elvin_connect(hostname, port, quench_function, error, NULL)) == 0)
    {
	fprintf(stderr, "*** Unable to connect to elvin server at %s:%d\n", hostname, port);
	exit(0);
    }
#endif /* ELVIN */

    self -> callback = callback;
    self -> context = context;
    self -> subscriptions = subscriptions;
    subscribe(self);

    {
	Message message = Message_alloc("bob", "dave", "susan", 1);
	postMessage(self, message);
    }
    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
#ifdef ELVIN
    /* Disconnect from elvin server */
    elvin_disconnect(self -> connection);
#endif

    /* Free our memory */
    free(self);
}





