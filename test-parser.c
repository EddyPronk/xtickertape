#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "ast.h"
#include "subscription.h"
#include "parser.h"

/* Callback for completion of the parsing */
static int parsed(void *rock, uint32_t count, subscription_t *subs, elvin_error_t error)
{
    uint32_t i;

    printf("parsed (count=%u)\n", count);
    for (i = 0; i < count; i++)
    {
	if (! subscription_free(subs[i], error))
	{
	    return 0;
	}
    }

    return ELVIN_FREE(subs, error);
}

/* For testing purposes */
int main(int argc, char *argv[])
{
    elvin_error_t error = elvin_error_alloc();
    parser_t parser;
    int i;

    /* Allocate a new parser */
    if ((parser = parser_alloc(parsed, (void *)13, error)) == NULL)
    {
	fprintf(stderr, "parser_alloc(): failed\n");
	abort();
    }

    /* Assume the args are filenames */
    for (i = 1; i < argc; i++)
    {
	char *filename = argv[i];
	int fd;

	/* Open the file */
	if ((fd = open(argv[i], O_RDONLY)) < 0)
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

    /* Clean up */
    if (! parser_free(parser, error))
    {
	fprintf(stderr, "parser_free(): failed\n");
	elvin_error_fprintf(stderr, "en", error);
	exit(1);
    }

    exit(0);
}
