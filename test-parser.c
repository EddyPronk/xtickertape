#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
    elvin_error_t error = NULL;
    parser_t parser;
    char buffer[2048];
    size_t length;
    int fd;

    if ((parser = parser_alloc(parsed, (void *)13, error)) == NULL)
    {
	fprintf(stderr, "parser_alloc(): failed\n");
	abort();
    }

    /* Read stdin and put it through the parser */
    fd = STDIN_FILENO;
    while (1)
    {
	if ((length = read(fd, buffer, 2048)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	if (! parser_parse(parser, buffer, length, error))
	{
	    exit(1);
	}

	if (length < 1)
	{
	    /* Clean up */
	    if (! parser_free(parser, error))
	    {
		fprintf(stderr, "parser_free(): failed\n");
		exit(1);
	    }
	    
	    elvin_memory_report();
	    exit(0);
	}
    }
}
