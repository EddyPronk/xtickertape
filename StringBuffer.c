/* $Id: StringBuffer.c,v 1.1 1998/10/23 02:01:39 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "StringBuffer.h"
#include "sanity.h"

#ifdef SANITY
char *sanity_value = "StringBuffer";
char *sanity_freed = "Freed StringBuffer";
#endif /* SANITY */

#define INITIAL_BUFFER_SIZE 1024

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


/* Appends a null-terminated character array to the receiver */
void StringBuffer_append(StringBuffer self, char *string)
{
    char *in = string;

    /* Keep copying until we reach the null character */
    while (1)
    {
	/* Keep copying until we get to the end of the buffer */
	while (self -> pointer < self -> end)
	{
	    char ch = *(in++);

	    /* When we get the null character we're done */
	    if (ch == '\0')
	    {
		return;
	    }

	    *(self -> pointer++) = ch;
	}

	/* We've reached the end of the buffer, so grow */
	Grow(self);
    }
}


/* Answers a pointer to the receiver's null-terminated character array */
char *StringBuffer_getBuffer(StringBuffer self)
{
    /* Make sure we're null-terminated */
    *(self -> pointer) = '\0';
    return self -> buffer;
}
