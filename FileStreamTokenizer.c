/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
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
static const char cvsid[] = "$Id: FileStreamTokenizer.c,v 1.9 1999/05/06 00:34:30 phelps Exp $";
#endif /* lint */

#include <stdlib.h>
#include <string.h>
#include "FileStreamTokenizer.h"
#include "StringBuffer.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "FileStreamTokenizer";
static char *freed_value = "Freed FileStreamTokenizer";
#endif /* SANITY */


/* The FileStreamTokenizer data type */
struct FileStreamTokenizer_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's FILE */
    FILE *file;

    /* The receiver's StringBuffer */
    StringBuffer buffer;

    /* The receiver's whitespace characters (not to be included as part of tokens) */
    char *whitespace;

    /* The receiver's "special" characters (tokens all by themselves) */
    char *special;
};


/*
 *
 * Exported functions
 *
 */

/* Answers a new FileStreamTokenizer */
FileStreamTokenizer FileStreamTokenizer_alloc(FILE *file, char *whitespace, char *special)
{
    FileStreamTokenizer self;

    /* Allocate space for the receiver */
    if ((self = (FileStreamTokenizer) malloc(sizeof(struct FileStreamTokenizer_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif SANITY

    self -> file = file;
    self -> buffer = StringBuffer_alloc();
    self -> whitespace = strdup(whitespace);
    self -> special = strdup(special);
    return self;
}

/* Frees the resources consumed by a FileStreamTokenizer */
void FileStreamTokenizer_free(FileStreamTokenizer self)
{
    SANITY_CHECK(self);

    if (self -> whitespace)
    {
	free(self -> whitespace);
    }

    if (self -> special)
    {
	free(self -> special);
    }

#ifdef SANITY
    self -> sanity_check = freed_value;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}

/* Prints debugging information about a FileStreamTokenizer */
void FileStreamTokenizer_debug(FileStreamTokenizer self)
{
    SANITY_CHECK(self);

    printf("FileStreamTokenizer (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  file = %p\n", self -> file);
    printf("  special = \"%s\"\n", self -> special);
}


/* Skip over the given whitespace characters */
void FileStreamTokenizer_skip(FileStreamTokenizer self, char *whitespace)
{
    char ch;
    
    while ((ch = fgetc(self -> file)) != EOF)
    {
	/* If we encounter a non-whitespace character, then put it back and return */
	if (strchr(whitespace, ch) == NULL)
	{
	    ungetc(ch, self -> file);
	    return;
	}
    }
}


/* Skips over whitespace */
void FileStreamTokenizer_skipWhitespace(FileStreamTokenizer self)
{
    FileStreamTokenizer_skip(self, self -> whitespace);
}




/* Answers the receiver's next token or NULL if at the end of the file.
 * NB: this token exists in the receiver's internal buffer and will be
 * overwritten when the next token is read */
char *FileStreamTokenizer_nextWithSpec(
    FileStreamTokenizer self, char *whitespace, char *special)
{
    int ch;

    /* If we're at the end of the file, then simply exit */
    ch = fgetc(self -> file);
    if (ch == EOF)
    {
	return NULL;
    }

    /* Put the character back */
    ungetc(ch, self -> file);

    /* Skip over whitespace */
    FileStreamTokenizer_skip(self, whitespace);

    /* Clear out the StringBuffer */
    StringBuffer_clear(self -> buffer);

    /* See if the next character is a special character */
    ch = fgetc(self -> file);
    if (strchr(special, ch) != NULL)
    {
	StringBuffer_appendChar(self -> buffer, ch);
	return StringBuffer_getBuffer(self -> buffer);
    }

    /* Put the character back */
    ungetc(ch, self -> file);

    /* Ok, so we're reading a real token.  Keep going until we hit whitespace or special */
    while ((ch = fgetc(self -> file)) != EOF)
    {
	/* If we get a special character or whitespace, then put it back and return */
	if ((strchr(whitespace, ch) != NULL) || (strchr(special, ch) != NULL))
	{
	    ungetc(ch, self -> file);
	    return StringBuffer_getBuffer(self -> buffer);
	}

	StringBuffer_appendChar(self -> buffer, ch);
    }

    /* If we get here, then we've reached the end of the file */
    return StringBuffer_getBuffer(self -> buffer);
}

/* Answers the receiver's next token or NULL if at the end of the file.
 * NB: this token exists in the receiver's internal buffer and will be
 * overwritten when the next token is read */
char *FileStreamTokenizer_next(FileStreamTokenizer self)
{
    return FileStreamTokenizer_nextWithSpec(self, self -> whitespace, self -> special);
}


/* Skips to the end of the current line */
void FileStreamTokenizer_skipToEndOfLine(FileStreamTokenizer self)
{
    while (1)
    {
	int ch = fgetc(self -> file);
	if ((ch == EOF) || (ch == '\n'))
	{
	    return;
	}
    }
}
