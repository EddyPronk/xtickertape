/* $Id: ElvinConnection.c,v 1.1 1997/02/07 08:48:42 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "ElvinConnection.h"
#include "List.h"

struct ElvinConnection_t
{
    int fd;
    FILE *in;
    FILE *out;
};





/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(char *hostname, int port, List subscriptions)
{
    ElvinConnection self;
    struct hostent *host;
    struct sockaddr_in address;

    /* Allocate memory for the new ElvinConnection */
    self = (ElvinConnection) malloc(sizeof(ElvinConnection_t));

    /* Construct an address for the Elvin server */
    host = gethostbyname(hostname);
    memcpy(&address.sin_addr, host -> h_addr_list[0], host -> h_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    /* Make a socket */
    if ((self -> fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf(stderr, "*** unable to create a socket\n");
	exit(1);
    }

    /* Connect to the socket */
    if (connect(self -> fd, (struct sockaddr *)&address, sizeof(address)))
    {
	fprintf(stderr, "*** unable to connect to %s:%d\n", hostname, port);
	exit(1);
    }

    /* Create files to read and write with */
    self -> in = fdopen(fd, "r");
    self -> out = fdopen(fd, "a");

    List_doWith(subscriptions, ElvinConnection_subscribeInternal, self);

    return self;
}


/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self)
{
    /* Flush and close the socket */
    fflush(out);
    close(fd);

    /* Free our memory */
    free(self);
}



