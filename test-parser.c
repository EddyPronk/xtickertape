#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include <elvin/errors/elvin.h>
#include "errors.h"
#include "vm.h"
#include "parser.h"

/* Parser callback */
int parsed(vm_t vm, parser_t parser, void *rock, elvin_error_t error)
{
    if (! vm_eval(vm, error) || ! vm_print(vm, error))
    {
	elvin_error_fprintf(stdout, error);
	elvin_error_clear(error);
    }

    printf("> "); fflush(stdout);
    return 1;
}

/* Parse a file */
static int parse_file(parser_t parser, int fd, char *filename, elvin_error_t error)
{
    char buffer[4096];
    ssize_t length;

    /* Keep reading until we run dry */
    while (1)
    {
	/* Read some of the file */
	if ((length = read(fd, buffer, 4096)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	/* Run it through the parser */
	if (parser_read_buffer(parser, buffer, length, error) == 0)
	{
	    return 0;
	}

	/* See if that was the end of the file */
	if (length == 0)
	{
	    return 1;
	}
    }
}

/* The `lambda' special form */
static int prim_lambda(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* FIX THIS: we should allow lambdas with complex bodies */
    if (argc != 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    return vm_make_lambda(vm, error);
}

/* The `+' subroutine */
static int prim_plus(vm_t vm, uint32_t argc, elvin_error_t error)
{
    uint32_t i;

    /* Special case: no args -> 0 */
    if (argc == 0)
    {
	return vm_push_integer(vm, 0, error);
    }

    /* Add up all of the args */
    for (i = 1; i < argc; i++)
    {
	if (! vm_add(vm, error))
	{
	    return 0;
	}
    }

    return 1;
}

/* Prints out the stack */
static int prim_print_state(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 0)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    /* Print the stack */
    return vm_print_state(vm, error) && vm_push_nil(vm, error);
}

/* The `setq' special form */
static int prim_setq(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* FIX THIS: setq should allow multiple args */
    if (argc != 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    /* Evaluate the top of the stack and assign it */
    return vm_eval(vm, error) && vm_assign(vm, error);
}



/* For testing purposes */
int main(int argc, char *argv[])
{
    elvin_error_t error;
    parser_t parser;
    vm_t vm;
    int i;

    /* Grab an error context */
    if (! (error = elvin_error_alloc()))
    {
	fprintf(stderr, "elvin_error_alloc(): failed\n");
	exit(1);
    }

    /* Initialize the virtual machine */
    if (! (vm = vm_alloc(error)))
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Populate the root environment */
    if (! vm_push_string(vm, "nil", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_push_nil(vm, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error) ||

	! vm_push_string(vm, "t", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_dup(vm, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error) ||

	! vm_push_string(vm, "lambda", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_push_special_form(vm, prim_lambda, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error) ||

	! vm_push_string(vm, "setq", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_push_special_form(vm, prim_setq, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error) ||

	! vm_push_string(vm, "+", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_push_subr(vm, prim_plus, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error) ||

	! vm_push_string(vm, "print-state", error) ||
	! vm_make_symbol(vm, error) ||
	! vm_push_special_form(vm, prim_print_state, error) ||
	! vm_assign(vm, error) ||
	! vm_pop(vm, NULL, error))
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Allocate a new parser */
    if ((parser = parser_alloc(vm, parsed, NULL, error)) == NULL)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* If we have no args, then read from stdin */
    if (argc < 2)
    {
	/* Print the prompt */
	printf("> "); fflush(stdout);

	if (! parse_file(parser, STDIN_FILENO, "[stdin]", error))
	{
	    fprintf(stderr, "parse_file(): failed\n");
	    elvin_error_fprintf(stderr, error);
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
	    if (! parse_file(parser, fd, filename, error))
	    {
		fprintf(stderr, "parse_file(): failed\n");
		elvin_error_fprintf(stderr, error);
	    }

	    /* Close the file */
	    if (close(fd) < 0)
	    {
		perror("close(): failed");
		exit(1);
	    }
	}
    }

    /* Get rid of the parser */
    if (parser_free(parser, error) == 0)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Free the virtual machine */
    if (! vm_free(vm, error))
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* We don't need our error anymore */
    elvin_error_free(error);

    /* Report on memory usage */
    elvin_memory_report();
    exit(0);
}
