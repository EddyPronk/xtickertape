/* $Id */

#include "UsenetSubscription.h"
#include <stdlib.h>
#include <alloca.h>
#include "FileStreamTokenizer.h"
#include "StringBuffer.h"
#include "List.h"
#include "sanity.h"

#ifdef SANITY
static char *sanity_value = "UsenetSubscription";
static char *sanity_freed = "Freed UsenetSubscription";
#endif /* SANITY */


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
static char *TranslateField(char *field);
static void GetTupleFromTokenizer(FileStreamTokenizer tokenizer, StringBuffer buffer);
static void GetFromUsenetFileLine(
    char *token, FileStreamTokenizer tokenizer, StringBuffer buffer, int *isFirst);
static int GetFromUsenetFile(FILE *file, StringBuffer buffer, int *isFirst_p);
static void HandleNotify(UsenetSubscription self, en_notify_t notification);

/*
 *
 * Static functions
 *
 */

/* Answers the Elvin field which matches the field from the usenet file */
static char *TranslateField(char *field)
{
    switch (*field)
    {
	/* email */
	case 'e':
	{
	    if (strcmp(field, "email") == 0)
	    {
		return "FROM_EMAIL";
	    }

	    return NULL;
	}

	/* from */
	case 'f':
	{
	    if (strcmp(field, "from") == 0)
	    {
		return "FROM_NAME";
	    }

	    return NULL;
	}

	/* keywords */
	case 'k':
	{
	    if (strcmp(field, "keywords") == 0)
	    {
		return "KEYWORDS";
	    }

	    return NULL;
	}

	/* newsgroups */
	case 'n':
	{
	    if (strcmp(field, "newsgroups") == 0)
	    {
		return "NEWSGROUPS";
	    }

	    return NULL;
	}

	/* subject */
	case 's':
	{
	    if (strcmp(field, "subject") == 0)
	    {
		return "SUBJECT";
	    }

	    return NULL;
	}

	/* x-posts */
	case 'x':
	{
	    if (strcmp(field, "x-posts") == 0)
	    {
		return "CROSSPOSTS";
	    }

	    return NULL;
	}

	/* No match */
	default:
	{
	    return NULL;
	}
    }
}

/* Reads the next tuple from the FileStreamTokenizer into the StringBuffer */
static void GetTupleFromTokenizer(FileStreamTokenizer tokenizer, StringBuffer buffer)
{
    char *field;
    char *operation;
    char *pattern;
    char *elvinField;

    /* Read the field */
    field = FileStreamTokenizer_next(tokenizer);
    if ((field == NULL) || (*field == '/') || (*field == '\n'))
    {
	return;
    }

    if ((elvinField = TranslateField(field)) == NULL)
    {
	fprintf(stderr, "*** Unrecognized field \"%s\"\n", field);
	exit(1);
    }
    free(field);

    /* Read the operation */
    operation = FileStreamTokenizer_next(tokenizer);
    if ((operation == NULL) || (*operation == '/') || (*field == '\n'))
    {
	return;
    }

    /* Read the pattern */
    pattern = FileStreamTokenizer_next(tokenizer);
    if ((pattern == NULL) || (*pattern == '/') || (*field == '\n'))
    {
	return;
    }

    switch (*operation)
    {
	/* matches */
	case 'm':
	{
	    if (strcmp(operation, "matches") == 0)
	    {
		/* Construct our portion of the subscription */
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " matches(\"");
		StringBuffer_append(buffer, pattern);
		StringBuffer_append(buffer, "\")");
		return;
	    }

	    break;
	}

	/* not */
	case 'n':
	{
	    if (strcmp(operation, "not") == 0)
	    {
		/* Construct our portion of the subscription */
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " !matches(\"");
		StringBuffer_append(buffer, pattern);
		StringBuffer_append(buffer, "\")");
		free(operation);
		free(pattern);
		return;
	    }

	    break;
	}

	/* = */
	case '=':
	{
	    if (strcmp(operation, "=") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " == \"");
		StringBuffer_append(buffer, pattern);
		StringBuffer_append(buffer, "\"");
		free(operation);
		free(pattern);
		return;
	    }

	    break;
	}

	/* != */
	case '!':
	{
	    if (strcmp(operation, "!=") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " != \"");
		StringBuffer_append(buffer, pattern);
		StringBuffer_append(buffer, "\"");
		free(operation);
		free(pattern);
		return;
	    }

	    break;
	}

	/* < or <= */
	case '<':
	{
	    if (strcmp(operation, "<") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " < ");
		StringBuffer_append(buffer, pattern);
		free(operation);
		free(pattern);
		return;
	    }

	    if (strcmp(operation, "<=") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " <= ");
		StringBuffer_append(buffer, pattern);
		free(operation);
		free(pattern);
		return;
	    }

	    break;
	}

	/* > or >= */
	case '>':
	{
	    if (strcmp(operation, ">") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " > ");
		StringBuffer_append(buffer, pattern);
		free(operation);
		free(pattern);
		return;
	    }

	    if (strcmp(operation, ">=") == 0)
	    {
		StringBuffer_append(buffer, " && ");
		StringBuffer_append(buffer, elvinField);
		StringBuffer_append(buffer, " >= ");
		StringBuffer_append(buffer, pattern);
		free(operation);
		free(pattern);
		return;
	    }

	    break;
	}
    }

    /* If we get here, then we have a bad operation */
    fprintf(stderr, "*** don't understand operation \"%s\"\n", operation);
    free(operation);
    free(pattern);
}


/* Scans a non-empty, non-comment line of the usenet file and answers
 * a malloc'ed string corresponding to that line's part of the
 * subscription expression */
static void GetFromUsenetFileLine(
    char *token, FileStreamTokenizer tokenizer, StringBuffer buffer, int *isFirst)
{
    int isGroupNegated = 0;

    /* Read the group: [not] groupname */
    if (strcmp(token, "not") == 0)
    {
	isGroupNegated = 1;
	if ((token = FileStreamTokenizer_next(tokenizer)) == NULL)
	{
	    fprintf(stderr, "*** Invalid usenet line containing \"%s\"\n", token);
	    return;
	}

	if (*isFirst)
	{
	    *isFirst = 0;
	}
	else
	{
	    StringBuffer_append(buffer, " || ");
	}

	StringBuffer_append(buffer, "( NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, "!");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }
    else
    {
	if (*isFirst)
	{
	    *isFirst = 0;
	}
	else
	{
	    StringBuffer_append(buffer, " || ");
	}

	StringBuffer_append(buffer, "( NEWSGROUPS matches(\"");
	StringBuffer_append(buffer, token);
	StringBuffer_append(buffer, "\")");
    }

    /* Read tuples */
    while (1)
    {
	char *tok = FileStreamTokenizer_next(tokenizer);

	/* End of file? */
	if (tok == NULL)
	{
	    break;
	}

	/* End of line? */
	if (*tok == '\n')
	{
	    free(tok);
	    break;
	}

	if (*tok == '/')
	{
	    free(tok);
	    
	    GetTupleFromTokenizer(tokenizer, buffer);
	}
	else
	{
	    fprintf(stderr, "*** Invalid usenet line for group %s\n", token);
	    exit(1);
	}
    }
    
    StringBuffer_append(buffer, " )");
}

/* Scans the next line of the usenet file and answers a malloc'ed
 * string corresponding to that line's part of the subscription
 * expression */
static int GetFromUsenetFile(FILE *file, StringBuffer buffer, int *isFirst_p)
{
    FileStreamTokenizer tokenizer = FileStreamTokenizer_alloc(file, "/\n");
    char *token;

    /* Locate a non-empty line which doesn't begin with a '#' */
    while ((token = FileStreamTokenizer_next(tokenizer)) != NULL)
    {
	/* Comment line */
	if (*token == '#')
	{
	    free(token);
	    FileStreamTokenizer_skipToEndOfLine(tokenizer);
	}
	/* Empty line? */
	else if (*token == '\n')
	{
	    free(token);
	}
	/* Useful line */
	else
	{
	    GetFromUsenetFileLine(token, tokenizer, buffer, isFirst_p);
	    free(token);
	    return 1;
	}
    }

    FileStreamTokenizer_free(tokenizer);
    fclose(file);

    return 0;
}


/* Transforms a usenet notification into a Message and delivers it */
static void HandleNotify(UsenetSubscription self, en_notify_t notification)
{
    Message message;
    en_type_t type;
    char *user;
    char *string;
    char *mimeType;
    char *mimeArgs;
    char *name;
    char *email;
    char *newsgroups;
    char *pointer;

    /* First set up the user name */

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

    /* If the name and e-mail addresses are identical, just use one as the user name */
    if (strcmp(name, email) == 0)
    {
	user = (char *)alloca(sizeof(": ") + strlen(name) + strlen(newsgroups));
	sprintf(user, "%s: %s", name, newsgroups);
    }
    /* Otherwise construct the user name from the name and e-mail field */
    else
    {
	user = (char *)alloca(sizeof(" <>: ") + strlen(name) + strlen(email) + strlen(newsgroups));
	sprintf(user, "%s <%s>: %s", name, email, newsgroups);
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
    message = Message_alloc(NULL, "usenet", user, string, 60, mimeType, mimeArgs, 0, 0);

    /* Deliver the Message */
    (*self -> callback)(self -> context, message);
}






/*
 *
 * Exported functions
 *
 */

/* Read the UsenetSubscription from the given file */
UsenetSubscription UsenetSubscription_readFromUsenetFile(
    FILE *usenet, UsenetSubscriptionCallback callback, void *context)
{
    UsenetSubscription subscription;
    StringBuffer buffer = StringBuffer_alloc();
    int isFirst = 1;

    StringBuffer_append(buffer, "ELVIN_CLASS == \"NEWSWATCHER\" && ");
    StringBuffer_append(buffer, "ELVIN_SUBCLASS == \"MONITOR\" && ( ");

    /* Get the subscription expressions for each individual line */
    while (GetFromUsenetFile(usenet, buffer, &isFirst) != 0);

    StringBuffer_append(buffer, " )");
    subscription = UsenetSubscription_alloc(StringBuffer_getBuffer(buffer), callback, context);
    StringBuffer_free(buffer);

    return subscription;
}



/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context)
{
    UsenetSubscription self = (UsenetSubscription) malloc(sizeof(struct UsenetSubscription_t));

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

