/* $Id: FileStreamTokenizer.c,v 1.4 1998/10/23 17:37:45 phelps Exp $ */

#include "FileStreamTokenizer.h"
#include <stdlib.h>
#include <string.h>
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
 * Static functions
 *
 */

/* Skips all whitespace characters */
static void SkipWhitespace(FileStreamTokenizer self, char *whitespace)
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


/* Answers the receiver's next token (dynamically allocated) or NULL
 * if at the end of the file */
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
    SkipWhitespace(self, self -> whitespace);

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

/* Answers the receiver's next token (dynamically allocated) or NULL
 * if at the end of the file */
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
