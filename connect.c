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
static const char cvsid[] = "$Id: connect.c,v 1.3 1999/10/06 08:38:47 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <elvin3/elvin.h>
#include <elvin3/subscription.h>
#include "connect.h"

/* The minimum number of milliseconds to wait before attempting to
 * reconnect to the elvin server (1 second) */
#define INITIAL_PAUSE 1000

/* The maximum number of seconds to wait before attempting to
 * reconnect to the elvin server (5 minutes) */
#define MAX_PAUSE (5 * 60 * 1000)


/* The receiver's state machine for connectivity status */
enum connection_state
{
    never_connected_state,
    connected_state,
    lost_connection_state,
    reconnect_failed_state,
    disconnected_state
};

typedef enum connection_state connection_state_t;

/* A tuple of subscription information */
typedef struct subscription_tuple *subscription_tuple_t;
struct subscription_tuple
{
    /* The next subscription_tuple_t in the list */
    subscription_tuple_t next;

    /* The elvin subscription id */
    uint32 id;

    /* The subscription expression (in case we lose our elvin connection) */
    char *expression;

    /* Who to call when we get a match */
    notify_callback_t callback;

    /* The argument for the above callback */
    void *rock;
};

/* Initializes and returns a newly allocated tuple */
static subscription_tuple_t subscription_tuple_alloc(
    char *expression, notify_callback_t callback, void *rock)
{
    subscription_tuple_t self;

    /* Allocate some room */
    if ((self = (subscription_tuple_t)malloc(sizeof(struct subscription_tuple))) == NULL)
    {
	return NULL;
    }

    /* Copy the subscription expression */
    if ((self -> expression = strdup(expression)) == NULL)
    {
	free(self);
	return NULL;
    }

    /* Initialize the rest of the tuples values */
    self -> id = 0;
    self -> next = NULL;
    self -> callback = callback;
    self -> rock = rock;
    return self;
}

/* Frees the resources consumed by a subscription_tuple_t */
static void subscription_tuple_free(subscription_tuple_t self)
{
    /* Free the subscription expression */
    if (self -> expression != NULL)
    {
	free(self -> expression);
    }

    /* Free ourselves */
    free(self);
}


/* Callback for a notification */
static void handle_notify(elvin_t connection, void *rock, uint32 id, en_notify_t notification)
{
    subscription_tuple_t self = (subscription_tuple_t)rock;

    (*self -> callback)(self -> rock, notification);
}

/* Subscribe to the tuple's subscription expression.
 * Returns 0 on success, -1 on failure */
static int subscription_tuple_subscribe(subscription_tuple_t self, elvin_t elvin)
{
    self -> id = elvin_add_subscription(elvin, self -> expression, handle_notify, self, 0);
    return (self -> id != 0) ? 0 : -1;
}

/* Unsubscribes to the tuple's subscription expression */
static void subscription_tuple_unsubscribe(subscription_tuple_t self, elvin_t elvin)
{
    if (self -> id != 0)
    {
	elvin_del_subscription(elvin, self -> id);
	self -> id = 0;
    }
}


/* The connection data structure */
struct connection
{
    /* The host on which the elvin server is running */
    char *host;

    /* The port to which the elvin server listens */
    int port;

    /* Our elvin connection */
    elvin_t elvin;

    /* The state of our connection to the elvin server */
    connection_state_t state;

    /* The duration to wait before attempting to reconnect */
    unsigned long retry_pause;

    /* The receiver's application context */
    XtAppContext app_context;
    
    /* The data returned from the register function we need to pass to unregister */
    XtInputId input_id;

    /* The disconnect callback function */
    disconnect_callback_t disconnect_callback;

    /* The disconnect callback context */
    void *disconnect_rock;

    /* The reconnect callback function */
    reconnect_callback_t reconnect_callback;

    /* The reconnect callback context */
    void *reconnect_rock;

    /* The list of subscriptions */
    subscription_tuple_t subscriptions;
};


/* Callback for quench expressions (we don't use these) */
static void (*quench_callback)(elvin_t elvin, void *arg, char *quench) = NULL;


/*
 *
 * Static function headers 
 *
 */
static void read_input(connection_t self);
static void do_connect(connection_t self);
static void error(elvin_t elvin, void *arg, elvin_error_code_t code, char *message);


/*
 *
 * Static functions
 *
 */

/* Call this when the connection has data available */
static void read_input(connection_t self)
{
    if (self -> state == connected_state)
    {
	elvin_dispatch(self -> elvin);
    }
}

/* Attempt to connect to the elvin server */
static void do_connect(connection_t self)
{
    subscription_tuple_t tuple;

    /* Try to connect */
    if ((self -> elvin = elvin_connect(
	EC_NAMEDHOST, self -> host, self -> port,
	quench_callback, NULL, error, self, 1)) == NULL)
    {
	int pause = self -> retry_pause;

	/* Is this the first time we've tried to connect? */
	if (self -> state == never_connected_state)
	{
	    (*self -> disconnect_callback)(self -> disconnect_rock, self);
	}

	XtAppAddTimeOut(
	    self -> app_context, pause,
	    (XtTimerCallbackProc)do_connect, (XtPointer)self);
	self -> retry_pause = (pause * 2 > MAX_PAUSE) ? MAX_PAUSE : pause * 2;
	self -> state = reconnect_failed_state;
	return;
    }

    /* Success: call the reconnect callback */
    if (self -> state != never_connected_state)
    {
	(*self -> reconnect_callback)(self -> reconnect_rock, self);
    }

    /* Start listening for input on the elvin socket */
    self -> state = connected_state;
    self -> input_id = XtAppAddInput(
	self -> app_context, elvin_get_socket(self -> elvin), 
	(XtPointer)XtInputReadMask,
	(XtInputCallbackProc)read_input, (XtPointer)self);

    /* Reconnect all of our subscription tuples */
    for (tuple = self -> subscriptions; tuple != NULL; tuple = tuple -> next)
    {
	/* Be careful in case we encounter an error while trying to subscribe */
	if (self -> state == connected_state)
	{
	    subscription_tuple_subscribe(tuple, self -> elvin);
	}
    }
}

/* Callback for elvin errors */
static void error(elvin_t elvin, void *arg, elvin_error_code_t code, char *message)
{
    connection_t self = (connection_t) arg;
    subscription_tuple_t tuple;

    /* Unregister our file descriptor */
    if (self -> input_id != 0)
    {
	XtRemoveInput(self -> input_id);
	self -> input_id = 0;
    }

    /* Go through each tuple and clear its subscription id */
    for (tuple = self -> subscriptions; tuple != NULL; tuple = tuple -> next)
    {
	tuple -> id = 0;
    }

    /* Call the disconnect_callback */
    self -> state = lost_connection_state;
    self -> retry_pause = INITIAL_PAUSE;
    (*self -> disconnect_callback)(self -> disconnect_rock, self);

    /* Try to reconnect */
    do_connect(self);
}


/* Answers a new connection_t */
connection_t connection_alloc(
    char *hostname, int port, XtAppContext app_context,
    disconnect_callback_t disconnect_callback, void *disconnect_rock,
    reconnect_callback_t reconnect_callback, void *reconnect_rock)
{
    connection_t self;

    /* Allocate memory for the new connection_t */
    self = (connection_t)malloc(sizeof(struct connection));

    self -> host = strdup(hostname);
    self -> port = port;
    self -> elvin = NULL;
    self -> state = never_connected_state;
    self -> retry_pause = INITIAL_PAUSE;
    self -> app_context = app_context;
    self -> input_id = 0;
    self -> disconnect_callback = disconnect_callback;
    self -> disconnect_rock = disconnect_rock;
    self -> reconnect_callback = reconnect_callback;
    self -> reconnect_rock = reconnect_rock;
    self -> subscriptions = NULL;
    do_connect(self);

    return self;
}


/* Releases the resources used by the connection_t */
void connection_free(connection_t self)
{
    subscription_tuple_t tuple;

    /* Free each of the tuples */
    tuple = self -> subscriptions;
    while (tuple != NULL)
    {
	subscription_tuple_t next = tuple -> next;
	subscription_tuple_free(tuple);
	tuple = next;
    }

    /* Unregister our input handler */
    if (self -> input_id != 0)
    {
	XtRemoveInput(self -> input_id);
	self -> input_id = 0;
    }

    /* Disconnect from elvin server */
    if (self -> state == connected_state)
    {
	elvin_disconnect(self -> elvin);
	self -> state = disconnected_state;
    }

    /* Clean up easy stuff */
    free(self -> host);
    free(self);
}


/* Answers the receiver's host */
char *connection_host(connection_t self)
{
    return self -> host;
}

/* Answers the receiver's port */
int connection_port(connection_t self)
{
    return self -> port;
}


/* Registers a callback for when the given expression is matched */
void *connection_subscribe(
    connection_t self, char *expression,
    notify_callback_t callback, void *rock)
{
    subscription_tuple_t tuple;

    /* Allocate a new tuple */
    if ((tuple = subscription_tuple_alloc(expression, callback, rock)) == NULL)
    {
	return NULL;
    }

    /* If we're connected, then try to subscribe */
    if (self -> state == connected_state)
    {
	/* If we get an error and we're still connected then we've got a bogus expression */
	if ((subscription_tuple_subscribe(tuple, self -> elvin) < 0) &&
	    (self -> state == connected_state))
	{
	    free(tuple);
	    return NULL;
	}
    }

    /* If we're not connected, then check to see that the expression is valid
     * so that we can resubscribe to it later */
    if (self -> state != connected_state)
    {
	es_ptree_t parse_tree;
	char *error;

	/* See if we can parse the expression */
	if ((parse_tree = sub_parse(expression, &error)) == NULL)
	{
	    es_free_ptree(parse_tree);
	    free(tuple);
	    return NULL;
	}

	/* Release the parse tree */
	es_free_ptree(parse_tree);
    }

    /* Add it to our list of subscriptions */
    tuple -> next = self -> subscriptions;
    self -> subscriptions = tuple;

    /* Return it */
    return tuple;
}

/* Removes the tuple from the list of subscriptions.
 * Returns 0 on success -1 if not found */
static int connection_remove_tuple(connection_t self, subscription_tuple_t tuple)
{
    subscription_tuple_t probe;

    /* If we have no subscriptions then bail out now */
    if (self -> subscriptions == NULL)
    {
	return -1;
    }

    /* If the first tuple is a match then we have a special case */
    if (self -> subscriptions == tuple)
    {
	self -> subscriptions = tuple -> next;
	return 0;
    }

    /* Otherwise check the rest of the list */
    for (probe = self -> subscriptions; probe -> next != NULL; probe = probe -> next)
    {
	/* Watch for a match */
	if (probe -> next == tuple)
	{
	    probe -> next = tuple -> next;
	    return 0;
	}
    }

    /* Not found */
    return -1;
}

/* Unregisters a callback (info was returned by connection_subscribe) */
void connection_unsubscribe(connection_t self, void *info)
{
    subscription_tuple_t tuple = (subscription_tuple_t)info;

    /* Remove the tuple from the list of subscriptions */
    if (connection_remove_tuple(self, tuple) < 0)
    {
	return;
    }

    /* Unsubscribe and free the tuple */
    subscription_tuple_unsubscribe(tuple, self -> elvin);
    subscription_tuple_free(tuple);
}


/* Sends a message by posting an Elvin event */
void connection_publish(connection_t self, en_notify_t notification)
{
    /* FIX THIS: should we enqueue the notification if we're not connected? */
    /* Don't even try if we're not connected */
    if (self -> state == connected_state)
    {
	elvin_notify(self -> elvin, notification);
    }
}
