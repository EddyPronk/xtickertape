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
static const char cvsid[] = "$Id: show-url.c,v 1.14 2003/04/23 16:46:16 phelps Exp $";
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
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned int)(stat_val) >> 8)
#endif

/* Environment variables */
#define ENV_BROWSER "BROWSER"
#define DEF_BROWSER "netscape -raise -remote \"openURL(%s,new-window)\":lynx"
#define MAX_URL_SIZE 4095
#define INIT_CMD_SIZE 128
#define FILE_URL_PREFIX "file://"

/* Options */
#define OPTIONS "b:dhu:v"

#if defined(HAVE_GETOPT_LONG)
/* The list of long options */
static struct option long_options[] =
{
    { "browser", required_argument, NULL, 'b' },
    { "url", required_argument, NULL, 'u' },
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

/* The program name */
static char *progname = NULL;

/* The buffer used to construct the command */
static char *cmd_buffer = NULL;

/* The next character in the command buffer */
static size_t cmd_index = 0;

/* The end of the command buffer */
static size_t cmd_length = 0;

/* The verbosity level */
static int verbosity = 0;


/* Debugging printf */
void dprintf(int level, const char *format, ...)
{
    va_list ap;

    /* Bail if the statement is below the debugging threshold */
    if (verbosity < level) {
        return;
    }

    /* Do the varargs thing */
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fflush(stderr);
}

/* Print out a usage message */
static void usage()
{
    fprintf(stderr,
	    "usage: %s [OPTION]... filename\n"
	    "usage: %s [OPTION]... -u URL\n"
	    "  -b browser,     --browser=browser\n"
	    "  -d,             --debug[=level]\n"
	    "  -u,             --url=url\n"
	    "  -v,             --version\n"
	    "  -h,             --help\n", progname, progname);
}

/* Appends a character to the command buffer */
static void append_char(int ch)
{
    /* Make sure there's enough room */
    if (! (cmd_index < cmd_length))
    {
	dprintf(3, "growing buffer\n");

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
		dprintf(1, "exec: %s\n", cmd_buffer);

		if ((status = system(cmd_buffer)) < 0)
		{
		    perror("fork() failed");
		    exit(1);
		}

		/* If successful return NULL */
		if (WEXITSTATUS(status) == 0)
		{
		    dprintf(2, "ok\n");
		    return NULL;
		}

		dprintf(2, "failed: %d\n", WEXITSTATUS(status));
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
    struct stat statbuf;
    char *url = NULL;
    char *filename = NULL;
    FILE *file;
    size_t length;
    char *point;

    /* Extract the program name from argv[0] */
    if ((progname = strrchr(argv[0], '/')) == NULL)
    {
	progname = argv[0];
    }
    else
    {
	progname++;
    }

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
		usage();
		exit(0);
	    }

	    /* --url= or -u */
	    case 'u':
	    {
		/* Determine if we should view a local file or a remote one */
		if (stat(optarg, &statbuf) < 0)
		{
		    dprintf(2, "%s: unable to stat file: %s\n", progname, optarg);
		    url = strdup(optarg);
		}
		else if ((statbuf.st_mode & S_IRUSR) == 0)
		{
		    dprintf(2, "%s: unable to read file: %s\n", progname, optarg);
		    url = strdup(optarg);
		}
		else
		{
		    dprintf(3, "%s: creating file URL: %s\n", progname, optarg);
		    if (optarg[0] == '/')
		    {
			length = sizeof(FILE_URL_PREFIX) + strlen(optarg);
			if ((url = (char *)malloc(length)) == NULL)
			{
			    perror("malloc() failed");
			    exit(1);
			}

			snprintf(url, length, FILE_URL_PREFIX "%s", optarg);
		    }
		    else
		    {
			/* Construct an absolute path */
			if (getcwd(buffer, MAX_URL_SIZE) == NULL)
			{
			    perror("Unable to determine current directory");
			    exit(1);
			}

			length = sizeof(FILE_URL_PREFIX) + strlen(buffer) + 1 + strlen(optarg);
			if ((url = (char *)malloc(length)) == NULL)
			{
			    perror("malloc() failed");
			    exit(1);
			}

			snprintf(url, length, FILE_URL_PREFIX "%s/%s", buffer, optarg);
		    }
		}

		dprintf(2, "%s: raw URL: %s\n", progname, url);
		break;
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
		usage();
		exit(1);
	    }

	    /* Trouble */
	    default:
	    {
		abort();
	    }
	}
    }

    /* If no URL provided then make sure there's a filename */
    if (url == NULL)
    {
	if (! (optind < argc))
	{
	    usage();
	    exit(1);
	}

	/* Get the filename */
	filename = argv[optind++];
    }

    /* Make sure there are no more arguments */
    if (optind < argc)
    {
	usage();
	exit(1);
    }

    /* Extract the URL from the file */
    if (filename != NULL)
    {
	dprintf(3, "%s: reading URL from %s\n", progname, filename);

	/* Read the URL from the file */
	if ((file = fopen(filename, "r")) == NULL)
	{
	    perror("Unable to open URL file");
	    exit(1);
	}

	/* Read up to MAX_URL_SIZE bytes of it */
	length = fread(buffer, 1, MAX_URL_SIZE, file);
	buffer[length] = 0;

	dprintf(2, "%s: raw URL: %s\n", progname, buffer);

	/* Clean up */
	fclose(file);

	/* Duplicate its contents */
	if ((url = strdup(buffer)) == NULL)
	{
	    perror("malloc() failed");
	    exit(1);
	}

	strcpy(url, buffer);
    }

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
	point = invoke(point, url);
	if (point == NULL)
	{
	    exit(0);
	}
    }

    /* Clean up */
    free(url);
    exit(1);
}
