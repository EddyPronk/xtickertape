/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2002.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: show-url.c,v 1.11 2002/10/17 22:58:29 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned int)(stat_val) >> 8)
#endif

/* Environment variables */
#define ENV_BROWSER "BROWSER"
#define DEF_BROWSER "netscape -raise -remote \"openURL(%%s,new-window)\":lynx"
#define MAX_URL_SIZE 4095
#define INIT_CMD_SIZE 128

/* Options */
#define OPTIONS "b:dhv"

#if defined(HAVE_GETOPT_LONG)
/* The list of long options */
static struct option long_options[] =
{
    { "browser", required_argument, NULL, 'b' },
    { "debug", optional_argument, NULL, 'd' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, '\0' }
};
#endif

/* A table converting a number into a hex nibble */
static char hex[] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

/* Characters which should be escaped */
static char no_esc[] =
{
    2, 1, 2, 0,  1, 0, 1, 1,  1, 2, 1, 0,  2, 0, 0, 0,  /* 0x20 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 1, 1,  /* 0x30 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x40 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 1, 0,  /* 0x50 */
    2, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x60 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 2,  /* 0x70 */
};

/* Characters which need to be escaped inside double-quotes */
static char dq_esc[] =
{
    2, 2, 2, 0,  1, 0, 0, 0,  0, 2, 0, 0,  2, 0, 0, 0,  /* 0x20 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x30 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x40 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x50 */
    2, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x60 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 2,  /* 0x70 */
};

/* Characters which need to be escaped inside single-quotes */
static char sq_esc[] =
{
    2, 0, 2, 0,  0, 0, 0, 2,  0, 2, 0, 0,  2, 0, 0, 0,  /* 0x20 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x30 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x40 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x50 */
    2, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x60 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 2,  /* 0x70 */
};

/* The buffer used to construct the command */
static char *cmd_buffer = NULL;

/* The next character in the command buffer */
static size_t cmd_index = 0;

/* The end of the command buffer */
static size_t cmd_length = 0;

/* The verbosity level */
static int verbosity = 0;


/* Print out a usage message */
static void usage(int argc, char *argv[])
{
    fprintf(stderr,
	    "usage: %s [OPTION]... filename\n"
	    "  -b browser,     --browser=browser\n"
	    "  -d,             --debug[=level]\n"
	    "  -v,             --version\n"
	    "  -h,             --help\n", argv[0]);
}

/* Appends a character to the command buffer */
static void append_char(int ch)
{
    /* Make sure there's enough room */
    if (! (cmd_index < cmd_length))
    {
	if (2 < verbosity)
	{
	    printf("growing buffer\n");
	}

	/* Double the buffer size */
	cmd_length *= 2;
	if ((cmd_buffer = (char *)realloc(cmd_buffer, cmd_length)) == NULL)
	{
	    perror("realloc() failed");
	    exit(1);
	}
    }

    cmd_buffer[cmd_index++] = ch;
}

/* Appends an URL to the command buffer, apply escapes as appropriate */
static void append_url(char *url, int quote_count)
{
    char *point;
    char *esc_table;

    /* Figure out which escape table to use */
    switch (quote_count)
    {
	case 0:
	{
	    esc_table = no_esc;
	    break;
	}

	case 1:
	{
	    esc_table = sq_esc;
	    break;
	}

	case 2:
	{
	    esc_table = dq_esc;
	    break;
	}

	default:
	{
	    /* Funny kind of quoting */
	    abort();
	}
    }

    point = url;
    while (1)
    {
	int ch = *point;
	int esc_type;

	/* Watch for the end of line/input */
	if (ch == '\0' || ch == '\n')
	{
	    return;
	}

	/* Figure out how to escape this character */
	esc_type = (ch < 32 || ch > 127) ? 2 : esc_table[ch - 32];
	switch (esc_type)
	{
	    /* No escape required */
	    case 0:
	    {
		append_char(ch);
		break;
	    }

	    /* Escape with a backslash */
	    case 1:
	    {
		append_char('\\');
		append_char(ch);
		break;
	    }

	    /* URL escape */
	    case 2:
	    {
		append_char('%');
		append_char(hex[(ch >> 4) & 0xf]);
		append_char(hex[ch & 0xf]);
		break;
	    }

	    /* Trouble */
	    default:
	    {
		abort();
	    }
	}

	point++;
    }
}

/* Invoke the browser on the given URL */
char *invoke(char *browser, char *url)
{
    int did_subst = 0;
    char *point = browser;
    int quote_count = 0;

    /* Reset the buffer */
    cmd_index = 0;

    /* Copy from the browser string */
    while (1)
    {
	int ch = *point;

	switch (ch)
	{
	    /* Watch for the end of the browser string */
	    case '\0':
	    case ':':
	    {
		int status;

		/* Insert the URL if we haven't done so already */
		if (! did_subst)
		{
		    append_char(' ');
		    append_url(url, quote_count);
		}

		/* Null-terminate the command */
		append_char('\0');

		/* Invoke the command */
		if (0 < verbosity)
		{
		    printf("exec: %s\n", cmd_buffer);
		    fflush(stdout);
		}

		if ((status = system(cmd_buffer)) < 0)
		{
		    perror("fork() failed");
		    exit(1);
		}

		/* If successful return NULL */
		if (WEXITSTATUS(status) == 0)
		{
		    if (1 < verbosity)
		    {
			printf("ok\n");
		    }

		    return NULL;
		}

		if (1 < verbosity)
		{
		    printf("failed: %d\n", WEXITSTATUS(status));
		}

		return ch == '\0' ? point : point + 1;
	    }

	    /* Watch for double-quotes */
	    case '"':
	    {
		/* Toggle double-quotes if appropriate */
		if (quote_count == 2)
		{
		    quote_count = 0;
		}
		else if (quote_count == 0)
		{
		    quote_count = 2;
		}

		append_char(ch);
		break;
	    }

	    /* Watch for single-quotes */
	    case '\'':
	    {
		/* Toggle single-quotes if appropriate */
		if (quote_count == 1)
		{
		    quote_count = 0;
		}
		else if (quote_count == 0)
		{
		    quote_count = 1;
		}

		append_char(ch);
		break;
	    }

	    /* Watch for %-escapes */
	    case '%':
	    {
		ch = point[1];

		/* Watch for the URL substitution */
		if (ch == 's')
		{
		    append_url(url, quote_count);

		    /* Skip ahead */
		    did_subst = 1;
		    point++;
		    break;
		}

		/* Watch for odd EOF */
		if (ch == '\0')
		{
		    append_char('%');
		    break;
		}

		/* Otherwise drop the initial % */
		append_char(ch);
		point++;
		break;
	    }

	    default:
	    {
		append_char(ch);
		break;
	    }
	}

	point++;
    }
}


/* Read the URL from a file and use it to invoke a browser */
int main(int argc, char *argv[])
{
    char *browser;
    char buffer[MAX_URL_SIZE + 1];
    char *filename;
    FILE *file;
    size_t length;
    char *point;

    /* Get the browser string from the environment */
    if ((browser = getenv(ENV_BROWSER)) == NULL)
    {
	browser = DEF_BROWSER;
    }

    while (1)
    {
	int choice;

#if defined(HAVE_GETOPT_LONG)
	choice = getopt_long(argc, argv, OPTIONS, long_options, NULL);
#else
	choice = getopt(argc, argv, OPTIONS);
#endif

	/* End of options? */
	if (choice < 0)
	{
	    break;
	}

	/* Which option is it? */
	switch (choice)
	{
	    /* --browser= or -b */
	    case 'b':
	    {
		browser = optarg;
		break;
	    }

	    /* --debug= or -d */
	    case 'd':
	    {
		if (optarg == NULL)
		{
		    verbosity++;
		}
		else
		{
		    verbosity = atoi(optarg);
		}

		break;
	    }

	    /* --help or -h */
	    case 'h':
	    {
		usage(argc, argv);
		exit(0);
	    }

	    /* --version or -v */
	    case 'v':
	    {
		printf(PACKAGE " show-url version " VERSION "\n");
		exit(0);
	    }

	    /* Unsupported option */
	    case '?':
	    {
		usage(argc, argv);
		exit(1);
	    }

	    /* Trouble */
	    default:
	    {
		abort();
	    }
	}
    }

    /* Make sure there's a filename */
    if (! (optind < argc))
    {
	usage(argc, argv);
	exit(1);
    }

    /* Record the filename */
    filename = argv[optind++];

    /* Make sure there are no more arguments */
    if (optind < argc)
    {
	usage(argc, argv);
	exit(1);
    }

    if (1 < verbosity)
    {
	printf("reading url from %s\n", filename);
    }

    /* Read the URL from the file */
    if ((file = fopen(filename, "r")) == NULL)
    {
	perror("Unable to open URL file");
	exit(1);
    }

    /* Read up to MAX_URL_SIZE bytes of it */
    length = fread(buffer, 1, MAX_URL_SIZE, file);
    buffer[length] = 0;

    if (1 < verbosity)
    {
	printf("raw url: %s\n", buffer);
    }

    /* Clean up */
    fclose(file);

    /* Initialize the command buffer */
    cmd_length = INIT_CMD_SIZE;
    if ((cmd_buffer = (char *)malloc(cmd_length)) == NULL)
    {
	perror("malloc() failed");
	exit(1);
    }

    /* Attempt to open a browser */
    point = browser;
    while (*point != '\0')
    {
	point = invoke(point, buffer);
	if (point == NULL)
	{
	    exit(0);
	}
    }

    exit(1);
}
