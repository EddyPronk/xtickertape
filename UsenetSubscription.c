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
static const char cvsid[] = "$Id: UsenetSubscription.c,v 1.18 1999/05/22 08:35:07 phelps Exp $";
#endif /* lint */

#include <stdlib.h>
#include "UsenetSubscription.h"
#include "FileStreamTokenizer.h"
#include "StringBuffer.h"
#include "sanity.h"
#include "usenet.h"

#ifdef SANITY
static char *sanity_value = "UsenetSubscription";
static char *sanity_freed = "Freed UsenetSubscription";
#endif /* SANITY */

typedef enum
{
    Error = -1,
    Equals,
    NotEquals,
    LessThan,
    LessThanEquals,
    GreaterThan,
    GreaterThanEquals,
    Matches,
    NotMatches
} Operation;


/* The UsenetSubscription data type */
struct UsenetSubscription_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's subscription expression */
    char *expression;

    /* The receiver's ElvinConnection */
    ElvinConnection connection;

    /* The receiver's ElvinConnection information */
    void *connectionInfo;

    /* The receiver's callback function */
    UsenetSubscriptionCallback callback;

    /* The receiver's callback context */
    void *context;
};



/*
 *
 * Static function headers
 *
 */
static void DeleteTrailingWhitespace(char *string, char *whitespace);
static char *TranslateField(char *field);
static Operation TranslateOperation(char *operation);
static void ReadNextTuple(FileStreamTokenizer tokenizer, StringBuffer buffer);
static void ReadNextLine(FileStreamTokenizer tokenizer, char *token, StringBuffer buffer);
static void ReadUsenetFile(FILE *file, StringBuffer buffer);
static void HandleNotify(UsenetSubscription self, en_notify_t notification);

/*
 *
 * Static functions
 *
 */

/* Inserts a '\0' in the string at the beginning of the final bit of whitespace */
static void DeleteTrailingWhitespace(char *string, char *whitespace)
{
    char *marker = NULL;
    char *pointer;

    /* Beware the NULL pointer */
    if (string == NULL)
    {
	return;
    }

    /* Find the beginning of the last bit of whitespace */
    for (pointer = string; *pointer != '\0'; pointer++)
    {
	/* Are we looking at a whitespace character? */
	if (strchr(whitespace, *pointer) == NULL)
	{
	    /* No.  Clear the marker */
	    marker = NULL;
	}
	else
	{
	    /* Yes.  Update the marker if this is the start of some whitespace */
	    marker = (marker == NULL) ? pointer : marker;
	}
    }

    /* Kill the trailing whitespace */
    if (marker != NULL)
    {
	*marker = '\0';
    }
}

/* Answers the Elvin field which matches the field from the usenet file */
static char *TranslateField(char *field)
{
    /* Bail if no field provided */
    if (field == NULL)
    {
	return NULL;
    }

    /* Use first character of field for quick lookup */
    switch (*field)
    {
	/* email */
	case 'e':
	{
	    if (strcmp(field, "email") == 0)
	    {
		return "FROM_EMAIL";
	    }

	    break;
	}

	/* from */
	case 'f':
	{
	    if (strcmp(field, "from") == 0)
	    {
		return "FROM_NAME";
	    }

	    break;
	}

	/* keywords */
	case 'k':
	{
	    if (strcmp(field, "keywords") == 0)
	    {
		return "KEYWORDS";
	    }

	    break;
	}

	/* newsgroups */
	case 'n':
	{
	    if (strcmp(field, "newsgroups") == 0)
	    {
		return "NEWSGROUPS";
	    }

	    break;
	}

	/* subject */
	case 's':
	{
	    if (strcmp(field, "subject") == 0)
	    {
		return "SUBJECT";
	    }

	    break;
	}

	/* x-posts */
	case 'x':
	{
	    if (strcmp(field, "x-posts") == 0)
	    {
		return "CROSSPOSTS";
	    }

	    break;
	}
    }

    return NULL;
}


/* Answers the operation (index) corresponding to the given operation, or -1 if none */
static Operation TranslateOperation(char *operation)
{
    /* Bail if no operation provided */
    if (operation == NULL)
    {
	return Error;
    }

    /* Use first character of operation for quick lookup */
    switch (*operation)
    {
	/* = */
	case '=':
	{
	    if (strcmp(operation, "=") == 0)
	    {
		return Equals;
	    }

	    break;
	}

	/* != */
	case '!':
	{
	    if (strcmp(operation, "!=") == 0)
	    {
		return NotEquals;
	    }

	    break;
	}

	/* < or <= */
	case '<':
	{
	    if (strcmp(operation, "<") == 0)
	    {
		return LessThan;
	    }

	    if (strcmp(operation, "<=") == 0)
	    {
		return LessThanEquals;
	    }

	    break;
	}

	/* > or >= */
	case '>':
	{
	    if (strcmp(operation, ">") == 0)
	    {
		return GreaterThan;
	    }

	    if (strcmp(operation, ">=") == 0)
	    {
		return GreaterThanEquals;
	    }

	    break;
	}

	/* matches */
	case 'm':
	{
	    if (strcmp(operation, "matches") == 0)
	    {
		return Matches;
	    }

	    break;
	}

	/* not */
	case 'n':
	{
	    if (strcmp(operation, "not") == 0)
	    {
		return NotMatches;
	    }

	    break;
	}
    }

    return Error;
}

/* Reads the next tuple from the FileStreamTokenizer into the StringBuffer */
static void ReadNextTuple(FileStreamTokenizer tokenizer, StringBuffer buffer)
{
    char *token;
    char *field;
    Operation operation;
    char *pattern;

    /* Read the field */
    token = FileStreamTokenizer_next(tokenizer);
    if ((field = TranslateField(token)) == NULL)
    {
	fprintf(stderr, "*** Unrecognized field \"%s\"\n", token);
	exit(1);
    }

    /* Read the operation */
    token = FileStreamTokenizer_next(tokenizer);
    if ((operation = TranslateOperation(token)) == Error)
    {
	fprintf(stderr, "** Unrecognized operation \"%s\"\n", token);
	return;
    }

    /* Read the pattern */
    FileStreamTokenizer_skipWhitespace(tokenizer);
    pattern = FileStreamTokenizer_nextWithSpec(tokenizer, "\r", "/\n");
    DeleteTrailingWhitespace(pattern, " \t");

    if ((pattern == NULL) || (*pattern == '/') || (*field == '\n'))
    {
	fprintf(stderr, "*** Unexpected end of usenet file line\n");
	return;
    }

    /* Construct our portion of the subscription */
    StringBuffer_append(buffer, " && ");

    switch (operation)
    {
	case Matches:
	{
	    /* Construct our portion of the subscription */
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " matches(\"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\")");
	    return;
	}

	case NotMatches:
	{
	    StringBuffer_append(buffer, "! ");
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " matches(\"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\")");
	    return;
	}

	case Equals:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " == \"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\"");
	    return;
	}

	case NotEquals:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " != \"");
	    StringBuffer_append(buffer, pattern);
	    StringBuffer_append(buffer, "\"");
	    return;
	}

	case LessThan:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " < ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case LessThanEquals:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " <= ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case GreaterThan:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " > ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	case GreaterThanEquals:
	{
	    StringBuffer_append(buffer, field);
	    StringBuffer_append(buffer, " >= ");
	    StringBuffer_append(buffer, pattern);
	    return;
	}

	/* This should never get called */
	case Error:
	{
	    return;
	}
    }
}


/* Scans a non-empty, non-comment line of the usenet file and adds its 
 * contents to the usenet subscription */
static void ReadNextLine(FileStreamTokenizer tokenizer, char *token, StringBuffer buffer)
{
    /* Read the group: [not] groupname */
    if (strcmp(token, "not") == 0)
    {
	if ((token = FileStreamTokenizer_next(tokenizer)) == NULL)
	{
	    fprintf(stderr, "*** Invalid usenet line containing \"%s\"\n", token);
	    exit(1);
	}

	StringBuffer_append(buffer, "( !NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }
    else
    {
	StringBuffer_append(buffer, "( NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }

    /* Read tuples */
    while (1)
    {
	token = FileStreamTokenizer_next(tokenizer);

	/* End of file or line? */
	if ((token == NULL) || (*token == '\n'))
	{
	    StringBuffer_append(buffer, " )");
	    return;
	}

	/* Look for our tuple separator */
	if (*token == '/')
	{
	    ReadNextTuple(tokenizer, buffer);
	}
	else
	{
	    fprintf(stderr, "*** Invalid usenet file\n");
	    exit(1);
	}
    }
}

/* Scans the usenet file to construct a single subscription to cover
 * all of the usenet news stuff, which is written into buffer */
static void ReadUsenetFile(FILE *file, StringBuffer buffer)
{
    FileStreamTokenizer tokenizer = FileStreamTokenizer_alloc(file, " \t\r", "/\n");
    char *token;
    int isFirst = 1;

    /* Write the beginning of the subscription expression */
    StringBuffer_append(buffer, "ELVIN_CLASS == \"NEWSWATCHER\" && ");
    StringBuffer_append(buffer, "ELVIN_SUBCLASS == \"MONITOR\" && ( ");

    /* Locate a non-empty line which doesn't begin with a '#' */
    while ((token = FileStreamTokenizer_next(tokenizer)) != NULL)
    {
	/* Comment line */
	if (*token == '#')
	{
	    FileStreamTokenizer_skipToEndOfLine(tokenizer);
	}
	/* Useful line? */
	else if (*token != '\n')
	{
	    if (isFirst)
	    {
		isFirst = 0;
	    }
	    else
	    {
		StringBuffer_append(buffer, " || ");
	    }

	    ReadNextLine(tokenizer, token, buffer);
	}
    }

    /* If we read no expressions, then clear the buffer */
    if (isFirst)
    {
	StringBuffer_clear(buffer);
    }
    else
    {
	/* Write the end of the expression */
	StringBuffer_append(buffer, " )");
    }

    FileStreamTokenizer_free(tokenizer);
}


/* Transforms a usenet notification into a Message and delivers it */
static void HandleNotify(UsenetSubscription self, en_notify_t notification)
{
    Message message;
    StringBuffer buffer;
    en_type_t type;
    char *string;
    char *mimeType;
    char *mimeArgs;
    char *name;
    char *email;
    char *newsgroups;
    char *pointer;

    /* Get the name from the FROM field (if provided) */
    if ((en_search(notification, "FROM_NAME", &type, (void **)&name) != 0) ||
	(type != EN_STRING))
    {
	name = "anonymous";
    }

    /* Get the e-mail address (if provided) */
    if ((en_search(notification, "FROM_EMAIL", &type, (void **)&email) != 0) ||
	(type != EN_STRING))
    {
	email = "anonymous";
    }

    /* Get the newsgroups to which the message was posted */
    if ((en_search(notification, "NEWSGROUPS", &type, (void **)&newsgroups) != 0) ||
	(type != EN_STRING))
    {
	newsgroups = "news";
    }

    /* Locate the first newsgroup to which the message was posted */
    for (pointer = newsgroups; *pointer != '\0'; pointer++)
    {
	if (*pointer == ',')
	{
	    *pointer = '\0';
	}

	break;
    }

    /* Construct the USER field */
    buffer = StringBuffer_alloc();

    /* If the name and e-mail addresses are identical, just use one as the user name */
    if (strcmp(name, email) == 0)
    {
	StringBuffer_append(buffer, name);
	StringBuffer_append(buffer, ": ");
	StringBuffer_append(buffer, newsgroups);
    }
    /* Otherwise construct the user name from the name and e-mail field */
    else
    {
	StringBuffer_append(buffer, name);
	StringBuffer_append(buffer, " <");
	StringBuffer_append(buffer, email);
	StringBuffer_append(buffer, ">: ");
	StringBuffer_append(buffer, newsgroups);
    }

    /* Construct the text of the Message */

    /* Get the SUBJECT field (if provided) */
    if ((en_search(notification, "SUBJECT", &type, (void **)&string) != 0) ||
	(type != EN_STRING))
    {
	string = "[no subject]";
    }


    /* Construct the mime attachment information (if provided) */

    /* Get the MIME_ARGS field to use as the mime args (if provided) */
    if ((en_search(notification, "MIME_ARGS", &type, (void **)&mimeArgs) == 0) &&
	(type == EN_STRING))
    {
	/* Get the MIME_TYPE field (if provided) */
	if ((en_search(notification, "MIME_TYPE", &type, (void **)&mimeType) != 0) ||
	    (type != EN_STRING))
	{
	    mimeType = "x-elvin/url";
	}
    }
    /* No MIME_ARGS provided.  Use the Message-Id field */
    else if ((en_search(notification, "Message-Id", &type, (void **)&mimeArgs) == 0) &&
	    (type == EN_STRING))
    {
	mimeType = "x-elvin/news";
    }
    /* No Message-Id field provied either */
    else
    {
	mimeType = NULL;
	mimeArgs = NULL;
    }

    /* Construct a Message out of all of that */
    message = Message_alloc(
	NULL,
	"usenet", StringBuffer_getBuffer(buffer),
	string, 60,
	mimeType, mimeArgs,
	0, 0);

    /* Deliver the Message */
    (*self -> callback)(self -> context, message);
    StringBuffer_free(buffer);
}






/*
 *
 * Exported functions
 *
 */

/* Write a default `usenet' file onto the output stream */
int UsenetSubscription_writeDefaultUsenetFile(FILE *out)
{
    /* Write a string into the file */
    return fwrite(defaultUsenetFile, 1, strlen(defaultUsenetFile), out);
}

/* Read the UsenetSubscription from the given file */
UsenetSubscription UsenetSubscription_readFromUsenetFile(
    FILE *usenet, UsenetSubscriptionCallback callback, void *context)
{
    UsenetSubscription subscription;
    StringBuffer buffer = StringBuffer_alloc();

    /* Get the subscription expressions for each individual line */
    ReadUsenetFile(usenet, buffer);

    /* If no usenet expressions found, then return NULL */
    if (StringBuffer_length(buffer) == 0)
    {
	subscription = NULL;
    }
    else
    {
	subscription = UsenetSubscription_alloc(StringBuffer_getBuffer(buffer), callback, context);
    }

    StringBuffer_free(buffer);
    return subscription;
}



/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context)
{
    UsenetSubscription self;

    /* Allocate memory for the receiver */
    if ((self = (UsenetSubscription)malloc(sizeof(struct UsenetSubscription_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    self -> expression = strdup(expression);
    self -> connection = NULL;
    self -> connectionInfo = NULL;
    self -> callback = callback;
    self -> context = context;
    return self;
}

/* Releases the resources used by an UsenetSubscription */
void UsenetSubscription_free(UsenetSubscription self)
{
    SANITY_CHECK(self);

    /* Free the receiver's expression if it has one */
    if (self -> expression)
    {
	free(self -> expression);
    }

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
}

/* Prints debugging information */
void UsenetSubscription_debug(UsenetSubscription self)
{
    SANITY_CHECK(self);

    printf("UsenetSubscription (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  expression = \"%s\"\n", self -> expression);
    printf("  connection = %p\n", self -> connection);
    printf("  connectionInfo = %p\n", self -> connectionInfo);
    printf("  callback = %p\n", self -> callback);
    printf("  context = %p\n", self -> context);
}

/* Sets the receiver's ElvinConnection */
void UsenetSubscription_setConnection(UsenetSubscription self, ElvinConnection connection)
{
    SANITY_CHECK(self);

    /* Shortcut if we're already subscribed */
    if (self -> connection == connection)
    {
	return;
    }

    /* Unsubscribe from the old connection */
    if (self -> connection != NULL)
    {
	ElvinConnection_unsubscribe(self -> connection, self -> connectionInfo);
    }

    self -> connection = connection;

    /* Subscribe to the new connection */
    if (self -> connection != NULL)
    {
	self -> connectionInfo = ElvinConnection_subscribe(
	    self -> connection, self -> expression,
	    (NotifyCallback)HandleNotify, self);
    }
}

