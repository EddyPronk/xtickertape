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
static const char cvsid[] = "$Id: StringBuffer.c,v 1.5 1998/12/24 05:48:29 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "StringBuffer.h"
#include "sanity.h"

#ifdef SANITY
char *sanity_value = "StringBuffer";
char *sanity_freed = "Freed StringBuffer";
#endif /* SANITY */

#define INITIAL_BUFFER_SIZE 64

struct StringBuffer_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's buffer */
    char *buffer;

    /* A pointer to the next character in the receiver's buffer */
    char *pointer;

    /* A pointer to the character after the end of the buffer */
    char *end;
};

/*
 *
 * Static function headers
 *
 */
static void Grow(StringBuffer self);


/*
 *
 * Static functions
 *
 */

/* Double the size of the receiver's buffer */
static void Grow(StringBuffer self)
{
    /* Double the size of the buffer with realloc */
    unsigned long length = (self -> end - self -> buffer) * 2;
    char *newBuffer = (char *)realloc(self -> buffer, length * 2);

    /* Update our pointers */
    self -> end = newBuffer + length;
    self -> pointer = newBuffer + (self -> pointer - self -> buffer);
    self -> buffer = newBuffer;
}



/*
 *
 * Exported functions
 *
 */

/* Allocates an empty StringBuffer */
StringBuffer StringBuffer_alloc()
{
    StringBuffer self;

    if ((self = (StringBuffer) malloc(sizeof(struct StringBuffer_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    if ((self -> buffer = (char *)malloc(INITIAL_BUFFER_SIZE)) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

    self -> pointer = self -> buffer;
    self -> end = self -> buffer + INITIAL_BUFFER_SIZE;

    return self;
}

/* Frees the resources used by the receiver */
void StringBuffer_free(StringBuffer self)
{
    free(self -> buffer);

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}

/* Prints out debugging information about the receiver */
void StringBuffer_debug(StringBuffer self)
{
    char *pointer;

    printf("StringBuffer (%p)\n", self);
    printf("  buffer = 0x%p\n", self -> buffer);
    printf("  pointer = 0x%p\n", self -> pointer);
    printf("  end = 0x%p\n", self -> end);
    printf("  contents: \"");
    pointer = self -> buffer;
    while (pointer < self -> pointer)
    {
	putchar(*(pointer++));
    }
    printf("\"\n");
}

/* Resets the buffer to its first character */
void StringBuffer_clear(StringBuffer self)
{
    self -> pointer = self -> buffer;
}

/* Answers the number of characters in the receiver's buffer */
unsigned long StringBuffer_length(StringBuffer self)
{
    return self -> pointer - self -> buffer;
}

/* Appends a single character to the receiver */
void StringBuffer_appendChar(StringBuffer self, char ch)
{
    if (! (self -> pointer < self -> end))
    {
	Grow(self);
    }

    *(self -> pointer++) = ch;
}

/* Appends an integer to the receiver */
void StringBuffer_appendInt(StringBuffer self, int value)
{
    /* FIX THIS: there must be a nicer way to do this... */
    char buffer[128];

    sprintf(buffer, "%d", value);
    StringBuffer_append(self, buffer);
}


/* Appends a null-terminated character array to the receiver */
void StringBuffer_append(StringBuffer self, char *string)
{
    char *pointer = string;

    /* Keep copying until we reach the null character */
    for (pointer = string; *pointer != '\0'; pointer++)
    {
	StringBuffer_appendChar(self, *pointer);
    }
}


/* Answers a pointer to the receiver's null-terminated character array */
char *StringBuffer_getBuffer(StringBuffer self)
{
    /* Make sure we're null-terminated */
    *(self -> pointer) = '\0';
    return self -> buffer;
}
