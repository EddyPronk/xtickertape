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

/* The `car' subroutine */
static int prim_car(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    return vm_car(vm, error);
}

/* The `cdr' subroutine */
static int prim_cdr(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    return vm_cdr(vm, error);
}

/* The `cons' subroutine */
static int prim_cons(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    /* Make a cons cell out of the top two elements on the stack */
    return vm_make_cons(vm, error);
}

/* The `eq' subroutine */
static int prim_eq(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 2)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    return vm_eq(vm, error);
}

/* The `if' special form */
static int prim_if(vm_t vm, uint32_t argc, elvin_error_t error)
{
    object_type_t type;

    if (argc != 3)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    /* Rotate the condition to the top of the stack and evaluate it */
    if (! vm_unroll(vm, 2, error) || ! vm_eval(vm, error))
    {
	return 0;
    }

    /* Is it nil? */
    if (! vm_type(vm, &type, error))
    {
	return 0;
    }

    /* If the value is not nil then we need to pop an extra time */
    if (type != SEXP_NIL)
    {
	if (! vm_pop(vm, NULL, error))
	{
	    return 0;
	}
    }

    return vm_pop(vm, NULL, error) && vm_eval(vm, error);
}

/* The `gc' subroutine */
static int prim_gc(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 0)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    return vm_gc(vm, error);
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

/* Quotes its arg */
static int prim_quote(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "wrong number of args");
	return 0;
    }

    /* Our result is what's on the stack */
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



/* Defines a subroutine in the root environment */
static int define_subr(vm_t vm, char *name, prim_t func, elvin_error_t error)
{
    return
	vm_push_symbol(vm, name, error) &&
	vm_push_subr(vm, func, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error);
}

/* Defines a special form in the root environment */
static int define_special(vm_t vm, char *name, prim_t func, elvin_error_t error)
{
    return
	vm_push_string(vm, name, error) &&
	vm_make_symbol(vm, error) &&
	vm_push_special_form(vm, func, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error);
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

	! define_subr(vm, "car", prim_car, error) ||
	! define_subr(vm, "cdr", prim_cdr, error) ||
	! define_subr(vm, "cons", prim_cons, error) ||
	! define_subr(vm, "eq", prim_eq, error) ||
	! define_special(vm, "if", prim_if, error) ||
	! define_subr(vm, "gc", prim_gc, error) ||
	! define_special(vm, "lambda", prim_lambda, error) ||
	! define_special(vm, "quote", prim_quote, error) ||
	! define_special(vm, "setq", prim_setq, error) ||
	! define_subr(vm, "+", prim_plus, error) ||
	! define_special(vm, "print-state", prim_print_state, error))
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
