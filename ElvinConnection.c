/* $Id: ElvinConnection.c,v 1.2 1997/02/13 05:51:11 phelps Exp $ */


#include "ElvinConnection.h"
#include "List.h"

struct ElvinConnection_t
{
    static elvin_t connection;
};



/* Set up callbacks */
static void (*quench_function)(elvin_t connection, char *quench) = NULL;

static void error(elvin_t connection, void *arg, elvin_error_code_t code, char *message)
{
    fprintf(stderr, "*** Elvin error %d (%s): exiting\n", code, message);
    exit(0);
}



/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    TickertapeWidge widget,
    char *hostname,
    int port,
    List subscriptions)
{
    ElvinConnection self;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(ElvinConnection_t));
    if ((self -> connection = elvin_connect(hostname, port, quench_function, error, NULL)) == 0)
    {
	fprintf(stderr, "*** Unable to connect to elvin server at %s:%d\n", hostname, port);
	exit(0);
    }

    /* FIX THIS: should add subscriptions here... */

    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
    /* Disconnect from elvin server */
    elvin_disconnect(self -> connection);

    /* Free our memory */
    free(self);
}



