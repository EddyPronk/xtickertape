/* $Id: BridgeConnection.c,v 1.8 1997/02/17 01:53:10 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include "BridgeConnection.h"
#include "Subscription.h"

#define BUFFERSIZE 4096
/*#define CONNECTING*/
#define EOL '\2'
#define SEPARATOR '|'

struct BridgeConnection_t
{
    TickertapeWidget tickertape;
    int fd;
    FILE *in;
    FILE *out;
    List subscriptions;
};


/*
 * Internal functions
 */

/* Sends a subscription for a single item expression to the bridge */
static void sendItemSubscribe(Subscription subscription, BridgeConnection self);

/* Sends a subscription expression to the bridge */
static void subscribe(BridgeConnection self);

/* Reads from the input stream until a separator character is encountered */
static int readToSeparator(BridgeConnection self, char *buffer);



/* Sends a subscription for a single item expression to the bridge */
static void sendItemSubscribe(Subscription subscription, BridgeConnection self)
{
    /* Get the name of the group */
    char *name = Subscription_getGroup(subscription);

    /* Check to see if it's the first item */
    if (subscription == List_first(self -> subscriptions))
    {
	fprintf(self -> out, "+TICKERTAPE == \"%s\"", name);
    }
    else
    {
	fprintf(self -> out, " || TICKERTAPE == \"%s\"", name);
    }
}

/* Sends a subscription expression to the bridge */
static void subscribe(BridgeConnection self)
{
    List_doWith(self -> subscriptions, sendItemSubscribe, self);
    fputc(EOL, self -> out);
    fflush(self -> out);
}

/* Reads from the input stream until a separator character is encountered */
static int readToSeparator(BridgeConnection self, char *buffer)
{
    while (1)
    {
	char ch = fgetc(self -> in);

	if (feof(self -> in) || ferror(self -> in))
	{
	    return -1;
	}

	if (ch == SEPARATOR)
	{
	    *buffer = '\0';
	    return 0;
	}

	if (ch == EOL)
	{
	    *buffer = '\0';
	    return EOL;
	}

	*buffer++ = ch;
    }
}






/*
 * Public functions
 */

/* Answers a new BridgeConnection */
BridgeConnection BridgeConnection_alloc(
    TickertapeWidget tickertape,
    char *hostname,
    int port,
    List subscriptions)
{
    BridgeConnection self;
#ifdef CONNECTING
    struct hostent *host;
    struct sockaddr_in address;
#endif /* CONNECTING */

    /* Allocate memory for the object */
    self = (BridgeConnection) malloc(sizeof(struct BridgeConnection_t));
    self -> tickertape = tickertape;
    self -> subscriptions = subscriptions;

#ifdef CONNECTING 
    /* Look up the host name */
#ifdef DEBUG
    fprintf(stderr, "  hostname[%s]", hostname); fflush(stderr);
#endif
    if ((host = gethostbyname(hostname)) == NULL)
    {
	fprintf(stderr, "*** can't find host \"%s\"\n", hostname);
	exit(1);
    }

    /* Construct an address */
    memcpy(&address.sin_addr, host -> h_addr_list[0], host -> h_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    /* Make a socket */
#ifdef DEBUG
    fprintf(stderr, "  socket"); fflush(stderr);
#endif
    if ((self -> fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf(stderr, "*** unable to create socket\n");
	exit(1);
    }

    /* Connect the socket to the bridge */
    if (connect(self -> fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
	fprintf(stderr, "*** unable to connect to %s:%u\n", hostname, port);
	exit(1);
    }

    /* Construct files from the fd */
#ifdef DEBUG    
    fprintf(stderr, "  streams"); fflush(stderr);
#endif /* DEBUG */
    self -> in = fdopen(self -> fd, "r");
    self -> out = fdopen(self -> fd, "a");
#else /* CONNECTING */
    self -> fd = STDIN_FILENO;
    self -> in = stdin;
    self -> out = stdout;
#endif /* CONNECTING */

#ifdef DEBUG
    fprintf(stderr, "  subscribe"); fflush(stderr);
#endif
    subscribe(self);

#ifdef DEBUG
    fprintf(stderr, "\n");
#endif    
    return self;
}


/* Releases the resources used by the BridgeConnection */
void BridgeConnection_free(BridgeConnection self)
{
    close(self -> fd);
    free(self);
}




/* Answers the receiver's file descriptor */
int BridgeConnection_getFD(BridgeConnection self)
{
    return self -> fd;
}


/* Sends a Message to the bridge */
void BridgeConnection_send(BridgeConnection self, Message message)
{
    fprintf(self -> out, "*TICKERTAPE|%s|USER|%s|TICKERTEXT|%s|TIMEOUT|%ld%c",
	    Message_getGroup(message),
	    Message_getUser(message),
	    Message_getString(message),
	    Message_getTimeout(message) / 60,
	    EOL);
    fflush(self -> out);
}

/* Call this when the connection has data available */
void BridgeConnection_read(BridgeConnection self)
{
    char key[BUFFERSIZE];
    char value[BUFFERSIZE];
    int result;
    char *group = NULL;
    char *user = NULL;
    char *string = NULL;
    int timeout = 0;

    while (1)
    {
	/* Read key value pairs and see if they're what we want */
	result = readToSeparator(self, key);
	if (result != 0)
	{
	    fprintf(stderr, "*** read error occurred: exiting...\n");
	    exit(1);
	}

	result = readToSeparator(self, value);

	if (strcmp(key, "TICKERTAPE") == 0)
	{
	    group = strdup(value);
	}
	else if (strcmp(key, "USER") == 0)
	{
	    user = strdup(value);
	}
	else if (strcmp(key, "TICKERTEXT") == 0)
	{
	    string = strdup(value);
	}
	else if (strcmp(key, "TIMEOUT") == 0)
	{
	    timeout = 60 * atoi(value);
	}
	
	/* FIX THIS: this is just hideous. Use a hashtable? */
	if (result != 0)
	{
	    if (result == EOL)
	    {
		if ((group != NULL) && (user != NULL) && (string != NULL) && (timeout != 0))
		{
		    Message message = Message_alloc(group, user, string, timeout);
		    free(group);
		    free(user);
		    free(string);
		    Message_debug(message);
		    TtAddMessage(self -> tickertape, message);
		    return;
		}
	    }
	    else
	    {
		if (feof(self -> in))
		{
		    fprintf(stderr, "*** connection closed by remote host\n");
		    exit(1);
		}
		else
		{
		    fprintf(stderr, "*** Error encounter in connection with bridge\n");
		    exit(1);
		}
	    }
	}
    }
}
