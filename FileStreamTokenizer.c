/* $Id: FileStreamTokenizer.c,v 1.1 1998/10/22 07:07:24 phelps Exp $ */

#include "FileStreamTokenizer.h"
#include <stdlib.h>
#include <alloca.h>
#include <string.h>
#include "sanity.h"

#define INITIAL_BUFFER_SIZE 256

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

    /* The receiver's "special" characters (tokens all by themselves) */
    char *special;

    /* Non-zero if the receiver has a remembered character */
    int hasBacktrack;

    /* The receiver's remembered character */
    int backtrack;
};


/*
 *
 * Exported functions
 *
 */

/* Answers a new FileStreamTokenizer */
FileStreamTokenizer FileStreamTokenizer_alloc(FILE *file, char *special)
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
    self -> special = strdup(special);
    self -> hasBacktrack = 0;
    self -> backtrack = '\0';
    return self;
}

/* Frees the resources consumed by a FileStreamTokenizer */
void FileStreamTokenizer_free(FileStreamTokenizer self)
{
    SANITY_CHECK(self);

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
    printf("  hasBacktrack = %s\n", (self -> hasBacktrack) ? "true" : "false");
    printf("  backtrack = \'%c\'\n", self -> backtrack);
}


/* Answers the receiver's next token (dynamically allocated) or NULL
 * if at the end of the file */
char *FileStreamTokenizer_next(FileStreamTokenizer self)
{
    int bufferSize = INITIAL_BUFFER_SIZE;
    char *buffer;
    char *end;
    char *pointer;
    int ch;

    /* If we're at the end of the file, then simply exit */
    ch = fgetc(self -> file);
    if (ch == EOF)
    {
	return NULL;
    }

    /* Skip whitespace (but don't skip special characters) */
    while ((ch == EOF) || (ch == ' ') || (ch == '\t') || (ch == '\n'))
    {
	/* If the whitespace character is a special character, then return it */
	if (strchr(self -> special, ch) != NULL)
	{
	    char *result = (char *)malloc(2);
	    result[0] = ch;
	    result[1] = '\0';
	    return result;
	}

	ch = fgetc(self -> file);
    }

    /* If the whitespace character is a special character, then return it */
    if (strchr(self -> special, ch) != NULL)
    {
	char *result = (char *)malloc(2);
	result[0] = ch;
	result[1] = '\0';
	return result;
    }

    /* Ok, we've got a real token here.  Make a buffer for it and start copying it */
    bufferSize = INITIAL_BUFFER_SIZE;
    buffer = (char *)alloca(bufferSize);
    end = buffer + bufferSize;
    pointer = buffer;

    /* Copy the first character into the buffer */
    *pointer = ch;
    pointer++;

    /* Keep going until we get the whole token */
    while (1)
    {
	char *newBuffer;

	/* Keep going until we run out of space */
	while (pointer < end)
	{
	    ch = fgetc(self -> file);

	    /* If we run into whitespace or a special character, then we're done */
	    if ((ch == EOF) || (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\0') ||
		(strchr(self -> special, ch) != NULL))
	    {
		ungetc(ch, self -> file);
		*pointer = '\0';
		return strdup(buffer);
	    }

	    *pointer = ch;
	    pointer++;
	}

	/* Copy the buffer into a larger buffer */
	newBuffer = (char *)alloca(bufferSize * 2);
	strncpy(newBuffer, buffer, bufferSize);
	bufferSize *= 2;
	pointer = newBuffer + (pointer - buffer);
	buffer = newBuffer;
	end = buffer + bufferSize;
    }
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
