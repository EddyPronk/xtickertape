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
static const char cvsid[] = "$Id: show-url.c,v 1.2 2002/10/04 16:43:47 phelps Exp $";
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

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned int)(stat_val) >> 8)
#endif

/* Environment variables */
#define ENV_BROWSER "BROWSER"
#define DEF_BROWSER "netscape -raise -remote \"openURL(%%s,new-window)\":lynx"
#define MAX_URL_SIZE 4095
#define INIT_CMD_SIZE 128

/* A table converting a number into a hex nibble */
static char hex[16] =
{
    '0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

/* A table of characters which should be escaped */
static char do_esc[128] =
{
    1, 0, 1, 0,  1, 0, 1, 1,  1, 1, 1, 0,  0, 0, 0, 0,  /* 0x20 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 0, 1, 0,  /* 0x30 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x40 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  /* 0x50 */
    1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  /* 0x60 */
    0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 1,  /* 0x70 */
};

/* The buffer used to construct the command */
static char *cmd_buffer = NULL;

/* The next character in the command buffer */
static char *cmd_point = NULL;

/* The end of the command buffer */
static size_t cmd_length = 0;


/* Print out a usage message */
static void usage(int argc, char *argv[])
{
    fprintf(stderr, "usage: %s filename | url\n", argv[0]);
}

/* Appends a character to the command buffer */
static void append_char(int ch)
{
    /* Make sure there's enough room */
    if (! (cmd_point < cmd_buffer + cmd_length))
    {
	/* Double the buffer size */
	cmd_length *= 2;
	if ((cmd_buffer = (char *)realloc(cmd_buffer, cmd_length)) == NULL)
	{
	    perror("realloc() failed");
	    exit(1);
	}
    }

    *(cmd_point++) = ch;
}

/* Appends an URL to the command buffer, apply escapes as appropriate */
static void append_url(char *url)
{
    char *point;

    point = url;
    while (1)
    {
	int ch = *point;

	/* Watch for the end of line/input */
	if (ch == '0' || ch == '\n')
	{
	    return;
	}

	/* Escape interesting shell characters */
	if (ch < 32 || ch > 127 || do_esc[ch - 32])
	{
	    append_char('\\');
	    append_char(ch);
	}
	/* Escape other stuff that can confuse netscape */
	else if (ch == ' ' || ch == ')' || ch == ',')
	{
	    append_char('%');
	    append_char(hex[(ch >> 4) & 0xf]);
	    append_char(hex[ch & 0xf]);
	}
	else
	{
	    append_char(ch);
	}

	point++;
    }
}

/* Invoke the browser on the given URL */
char *invoke(char *browser, char *url)
{
    int did_subst = 0;
    char *point = browser;
    cmd_point = cmd_buffer;

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
		    append_url(url);
		}

		/* Null-terminate the command */
		append_char('\0');

		/* Invoke the command */
		printf("%s\n", cmd_buffer);
		if ((status = system(cmd_buffer)) < 0)
		{
		    perror("fork() failed");
		    exit(1);
		}

		/* If successful return NULL */
		if (WEXITSTATUS(status) == 0)
		{
		    return NULL;
		}

		return ch == '\0' ? point : point + 1;
	    }

	    /* Watch for %s */
	    case '%':
	    {
		if (point[1] == 's')
		{
		    append_url(url);

		    /* Skip ahead */
		    did_subst = 1;
		    point++;
		}

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
    FILE *file;
    size_t length;
    char *point;

    /* Get the browser string from the environment */
    if ((browser = getenv(ENV_BROWSER)) == NULL)
    {
	browser = DEF_BROWSER;
    }

    /* Check that we have exactly one argument */
    if (argc != 2)
    {
	usage(argc, argv);
	exit(1);
    }

    /* Read the URL from the file */
    if ((file = fopen(argv[1], "r")) == NULL)
    {
	perror("Unable to open URL file");
	exit(1);
    }

    /* Read up to MAX_URL_SIZE bytes of it */
    length = fread(buffer, 1, MAX_URL_SIZE, file);
    buffer[length] = 0;

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
