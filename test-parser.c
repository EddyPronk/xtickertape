#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "errors.h"
#include "ast.h"
#include "subscription.h"
#include "parser.h"


/* Callback for completion of the parsing */
static int parsed(void *rock, uint32_t count, subscription_t *subs, elvin_error_t error)
{
    elvin_notification_t notification = (elvin_notification_t)rock;
    uint32_t i;

    /* Test each notification to see what message it generates */
    for (i = 0; i < count; i++)
    {
	message_t message;

	/* Get the subscription to transmute the message */
	if (! (message = subscription_transmute(subs[i], notification, error)))
	{
	    return 0;
	}
    }

    /* Clean up */
    for (i = 0; i < count; i++)
    {
	subscription_free(subs[i], error);
    }

    /* Free the subscription array */
    ELVIN_FREE(subs, error);
    return 1;
}

/* Constructs a simple notification */
static elvin_notification_t notification_alloc(elvin_error_t error)
{
    elvin_notification_t notification;

    /* Allocate a new notification */
    if (! (notification = elvin_notification_alloc(error)))
    {
	return NULL;
    }

    /* Populate it */
    elvin_notification_add_string(notification, "TICKERTAPE", "Chat", error);
    elvin_notification_add_string(notification, "USER", "phelps", error);
    elvin_notification_add_string(notification, "TICKERTEXT", "flonk", error);
    elvin_notification_add_int32(notification, "TIMEOUT", 10, error);
    return notification;
}

/* For testing purposes */
int main(int argc, char *argv[])
{
    elvin_error_t error;
    elvin_notification_t notification;
    parser_t parser;
    int i;

    /* Grab an error context */
    if (! (error = elvin_error_alloc()))
    {
	fprintf(stderr, "elvin_error_alloc(): failed\n");
	exit(1);
    }

    /* Construct a notification to play with */
    if (! (notification = notification_alloc(error)))
    {
	elvin_error_fprintf(stderr, "en", error);
	exit(1);
    }

    /* Allocate a new parser */
    if ((parser = parser_alloc(parsed, notification, error)) == NULL)
    {
	elvin_error_fprintf(stderr, "en", error);
	exit(1);
    }

    /* If we have no args, then read from stdin */
    if (argc < 2)
    {
	if (! parser_parse_file(parser, STDIN_FILENO, "[stdin]", error))
	{
	    fprintf(stderr, "parser_parse_file(): failed\n");
	    elvin_error_fprintf(stderr, "en", error);
	}
    }
    else
    {
	for (i = 1; i < argc; i++)
	{
	    char *filename = argv[i];
	    int fd;

	    fprintf(stderr, "--- parsing %s ---\n", filename);

	    /* Open the file */
	    if ((fd = open(filename, O_RDONLY)) < 0)
	    {
		perror("open(): failed");
		exit(1);
	    }

	    /* Parse its contents */
	    if (! parser_parse_file(parser, fd, filename, error))
	    {
		fprintf(stderr, "parser_parse_file(): failed\n");
		elvin_error_fprintf(stderr, "en", error);
	    }

	    /* Close the file */
	    if (close(fd) < 0)
	    {
		perror("close(): failed");
		exit(1);
	    }
	}
    }

    /* Clean up */
    if (! parser_free(parser, error))
    {
	fprintf(stderr, "parser_free(): failed\n");
	elvin_error_fprintf(stderr, "en", error);
	exit(1);
    }

    elvin_notification_free(notification, error);
    elvin_error_free(error);

    /* Report on memory usage */
    elvin_memory_report();
    exit(0);
}
