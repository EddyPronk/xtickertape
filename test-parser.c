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
#include "sexp.h"
#include "parser.h"

env_t root_env = NULL;


/* Parser callback */
int parsed(void *rock, parser_t parser, sexp_t sexp, elvin_error_t error)
{
    sexp_t result;

    /* Evaluate the s-expression */
    if (sexp_eval(sexp, root_env, &result, error) == 0)
    {
	elvin_error_fprintf(stderr, error);
    }
    else
    {
	/* Print it */
	sexp_print(result); printf("\n");

	/* Free it */
	if (sexp_free(result, error) == 0)
	{
	    return 0;
	}
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


/* The `lambda' primitive function */
static int prim_lambda(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    sexp_t arg_list;
    sexp_t body;

    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract it */
    if ((arg_list = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((body = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure that we have a valid body */
    if (sexp_get_type(body) != SEXP_NIL && sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad thingo");
	return 0;
    }

    /* Create a lambda node from the args and body */
    if ((*result = lambda_alloc(arg_list, body, error)) == NULL)
    {
	return 0;
    }

    return 1;
}

/* The `quote' primitive function */
static int prim_quote(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract it */
    if ((*result = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Go to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure that there are no more */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Increase the reference count */
    return sexp_alloc_ref(*result, error);
}

/* The `car' primitive function */
static int prim_car(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    sexp_t cons, value;

    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract the cons cell */
    if ((cons = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure there are no more ags */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Evaluate the arg */
    if (sexp_eval(cons, env, &value, error) == 0)
    {
	return 0;
    }

    /* Make sure the cons really is a cons cell */
    if (sexp_get_type(value) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad cons");
	return 0;
    }

    /* Extract the car of the cons cell */
    if ((*result = cons_car(value, error)) == NULL)
    {
	return 0;
    }

    /* Free our reference to the cons cell */
    if (sexp_free(value, error) == 0)
    {
	return 0;
    }

    /* Grab a reference to it */
    return sexp_alloc_ref(*result, error);
}

/* The `car' primitive function */
static int prim_cdr(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    sexp_t cons, value;

    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract the cons cell */
    if ((cons = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure there are no more ags */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Evaluate the arg */
    if (sexp_eval(cons, env, &value, error) == 0)
    {
	return 0;
    }

    /* Make sure the cons really is a cons cell */
    if (sexp_get_type(value) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad cons");
	return 0;
    }

    /* Extract the car of the cons cell */
    if ((*result = cons_cdr(value, error)) == NULL)
    {
	return 0;
    }

    /* Free our reference to the cons cell */
    if (sexp_free(value, error) == 0)
    {
	return 0;
    }

    /* Grab a reference to it */
    return sexp_alloc_ref(*result, error);
}

/* The `cons' primitive function */
static int prim_cons(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    sexp_t car, cdr;
    sexp_t car_value, cdr_value;

    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract the car */
    if ((car = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure there are no more args */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Extract the cdr */
    if ((cdr = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure it's nil */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Evaluate the car */
    if (sexp_eval(car, env, &car_value, error) == 0)
    {
	return 0;
    }

    /* Evaluate the cdr */
    if (sexp_eval(cdr, env, &cdr_value, error) == 0)
    {
	return 0;
    }

    /* Construct a cons cell */
    if ((*result = cons_alloc(car_value, cdr_value, error)) == NULL)
    {
	return 0;
    }

    /* Free our references to the car and cdr */
    if (sexp_free(car, error) == 0 || sexp_free(cdr, error) == 0)
    {
	return 0;
    }

    return 1;
}


/* The `setq' primitive function */
static int prim_setq(env_t env, sexp_t args, sexp_t *result, elvin_error_t error)
{
    sexp_t symbol;
    sexp_t value;

    /* Make sure we have at least one arg */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract it */
    if ((symbol = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure it's a symbol */
    if (sexp_get_type(symbol) != SEXP_SYMBOL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "bad symbol");
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure we have at least one more arg left */
    if (sexp_get_type(args) != SEXP_CONS)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too few args");
	return 0;
    }

    /* Extract it */
    if ((value = cons_car(args, error)) == NULL)
    {
	return 0;
    }

    /* Move on to the next arg */
    if ((args = cons_cdr(args, error)) == NULL)
    {
	return 0;
    }

    /* Make sure we're now out of args */
    if (sexp_get_type(args) != SEXP_NIL)
    {
	ELVIN_ERROR_ELVIN_NOT_YET_IMPLEMENTED(error, "too many args");
	return 0;
    }

    /* Evaluate the value */
    if (! sexp_eval(value, env, result, error))
    {
	return 0;
    }

    /* Set it */
    if (! env_set(env, symbol, *result, error))
    {
	return 0;
    }

    return 1;
}

/* Initializes the Lisp evaluation engine */
static env_t root_env_alloc(elvin_error_t error)
{
    env_t env;

    if ((env = env_alloc(40, NULL, error)) == NULL)
    {
	return NULL;
    }

    /* Register some constants */
    if (env_set_symbol(env, "t", "t", error) == 0 ||
	env_set_float(env, "pi", M_PI, error) == 0)
    {
	env_free(env, NULL);
	return NULL;
    }

    /* Register the built-in functions */
    if (env_set_builtin(env, "car", prim_car, error) == 0 ||
	env_set_builtin(env, "cdr", prim_cdr, error) == 0 ||
	env_set_builtin(env, "cons", prim_cons, error) == 0 ||
	env_set_builtin(env, "lambda", prim_lambda, error) == 0 ||
	env_set_builtin(env, "quote", prim_quote, error) == 0 ||
	env_set_builtin(env, "setq", prim_setq, error) == 0)
    {
	env_free(env, NULL);
	return NULL;
    }

    /* FIX THIS: define some built-in functions */
    return env;
}

/* For testing purposes */
int main(int argc, char *argv[])
{
    elvin_error_t error;
    parser_t parser;
    int i;

    /* Grab an error context */
    if (! (error = elvin_error_alloc()))
    {
	fprintf(stderr, "elvin_error_alloc(): failed\n");
	exit(1);
    }

    /* Allocate the root environment */
    if ((root_env = root_env_alloc(error)) == NULL)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Allocate a new parser */
    if ((parser = parser_alloc(parsed, NULL, error)) == NULL)
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
	fprintf(stderr, "parser_free(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Free the root environment */
    if (env_free(root_env, error) == 0)
    {
	fprintf(stderr, "env_free(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    elvin_error_free(error);

    /* Report on memory usage */
    elvin_memory_report();
    exit(0);
}
