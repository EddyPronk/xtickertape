/* $Id: main.c,v 1.8 1997/02/12 07:33:52 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include "Message.h"
#include "BridgeConnection.h"
#include "Tickertape.h"



#define CONNECTING 1
#define HOSTNAME "fatcat.dstc.edu.au"
#define PORT 8800
#define PERIOD (1000000 / FREQUENCY)

#if 0
/* Reads a single token of the message */
int readToken(FILE *in, char *buffer)
{
    char *pointer = buffer;

    while (1)
    {
	char ch = fgetc(in);

	if (feof(in) || ferror(in))
	{
	    *pointer = '\0';
	    return -1;
	}

	if (ch == '\2')
	{
	    *pointer = '\0';
	    return -1;
	}

	if (ch == '|')
	{
	    *pointer = '\0';
	    return 0;
	}

	*pointer++ = ch;
    }
}

/* Read and parse the keywords of a message from the Elvin Bridge */
Hashtable getKeysAndValues(FILE *in)
{
    char key[4096];
    char value[4096];
    Hashtable hashtable = Hashtable_alloc(127);
    int done = 0;

    while (! done)
    {
	if (readToken(in, key) < 0)
	{
	    char *pointer = key;

	    fprintf(stderr, "*** read key with no value! (errno=%d)\n", errno);
	    
	    while (*pointer != '\0')
	    {
		printf("%c (0x%x)", *pointer, *pointer);
		pointer++;
	    }
	    exit(1);
	}

	done = (readToken(in, value) < 0);
	printf("key=\"%s\"	value=\"%s\"\n", key, value);
	Hashtable_put(hashtable, key, strdup(value));
    }

    return hashtable;
}

/* Constructs a message from stuff read from the Elvin bridge */
Message getMessage(FILE *in)
{
    Hashtable table;
    Message message;

    table = getKeysAndValues(in);
    message = Message_alloc(
	Hashtable_get(table, "TICKERTAPE"),
	Hashtable_get(table, "USER"),
	Hashtable_get(table, "TICKERTEXT"),
	60 * atoi(Hashtable_get(table, "TIMEOUT")));
    Hashtable_do(table, free);
    Hashtable_free(table);

    Message_debug(message);
    return message;
}


int main(int argc, char *argv[])
{
#ifdef CONNECTING
    char *hostname = HOSTNAME;
    int port = PORT;
    int fd;
    struct sockaddr_in address;
    struct hostent *host;
    FILE *in, *out;
#endif /* CONNECTING */
    Message message;

    /* Get the address of the server */
#ifdef CONNECTING
    host = gethostbyname(hostname);
    memcpy(&address.sin_addr, host -> h_addr_list[0], host -> h_length);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    /* Make us a socket */
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
	fprintf(stderr, "*** unable to open socket\n");
	return 1;
    }

    /* Connect to the socket to obtain stuff */
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)))
    {
	fprintf(stderr, "*** unable to connect to %s:%d\n", hostname, port);
	return 1;
    }

    /* Make files outta it */
    in = fdopen(fd, "r");
    out = fdopen(fd, "a");

    /* Send a subscription request */
    printf("sending subscription request\n");
    fprintf(out, "TICKERTAPE == \"phelps\" || TICKERTAPE == \"Chat\" || TICKERTAPE == \"Coffee\" || TICKERTAPE == \"Rooms\" || TICKERTAPE == \"level7\" || TICKERTAPE == \"email\" || TICKERTAPE == \"Files\" || TICKERTAPE == \"rwall\" || TICKERTAPE == \"elvin_news\"%c", '\2');
    fflush(out);
#endif /* CONNECTING */

    /* Having successfully done all of that, make a window */
    scroller = Scroller_alloc(NULL, 0);
    message = Message_alloc("tickertape", "internal",
			    "subscribing to \"phelps\" and \"Chat\"...", 60);
    Scroller_addMessage(scroller, message);

    /* set up the timer handler */
    setTimer();


    printf("waiting for server to say something\n");
    /* Wait for reply */

    while (1)
    {
	fd_set readSet;
	fd_set writeSet;
	fd_set exceptSet;

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_ZERO(&exceptSet);

#ifdef CONNECTING
	FD_SET(fd, &readSet);
#endif /* CONNECTING */

	Scroller_fdSet(scroller, &readSet, &writeSet, &exceptSet);
	if (select(FD_SETSIZE, &readSet, &writeSet, &exceptSet, NULL) < 0)
	{
	    /* Ignore interrupted selects */
	    if (errno != EINTR)
	    {
		fprintf(stderr, "*** Select failed! (errno=%d)\n", errno);
		return 1;
	    }
	}
	else
	{
	    /* Stop the timer */
	    clearTimer();

#ifdef CONNECTING
	    if (FD_ISSET(fd, &readSet))
	    {
		Message message = getMessage(in);
		Scroller_addMessage(scroller, message);
	    }
#endif /* CONNECTING */
	    Scroller_handle(scroller, &readSet, &writeSet, &exceptSet);

	    /* Restart the timer */
	    setTimer();
	}
    }

    return 0;
}


#endif /* 0 */

int main(int argc, char *argv[])
{
    BridgeConnection connection;
    List subscriptions;
    Widget top;
    TickertapeWidget tickertape;
    XtAppContext context;

    /* Connect to the bridge */
    subscriptions = List_alloc();
    List_addLast(subscriptions, "Chat");
    List_addLast(subscriptions, "Coffee");
    List_addLast(subscriptions, "Rooms");
    List_addLast(subscriptions, "level7");
    List_addLast(subscriptions, "email");
    List_addLast(subscriptions, "Files");
    List_addLast(subscriptions, "rwall");
    List_addLast(subscriptions, "elvin_news");
    List_addLast(subscriptions, "phelps");

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv,
	NULL, NULL);

    /* Create the tickertape widget */
    tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"ticker", tickertapeWidgetClass, top,
	NULL, 0);

    /* listen for messages from the bridge */
    connection = BridgeConnection_alloc(tickertape, "fatcat", 8800, subscriptions);
    XtAppAddInput(
	context,
	BridgeConnection_getFD(connection),
	XtInputReadMask,
	BridgeConnection_read,
	connection);

    /* Create the widget */
    XtRealizeWidget(top);
    XtAppMainLoop(context);

    return 0;
}
