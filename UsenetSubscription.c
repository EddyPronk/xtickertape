/* $Id */

#include "UsenetSubscription.h"
#include <stdlib.h>
#include <alloca.h>
#include "FileStreamTokenizer.h"
#include "List.h"
#include "Hash.h"
#include "sanity.h"

#define BUFFERSIZE 8192
#define SEPARATORS " \t\n"

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


/* The mapping from fields in the usenet file to the elvin notification equivalents */
static Hashtable fieldMap = NULL;


/*
 *
 * Static function headers
 *
 */
static void HandleNotify(UsenetSubscription self, en_notify_t notification);

/*
 *
 * Static functions
 *
 */

/* Copies characters from inString into outString */
void AppendString(char *inString, char **outString)
{
    char *pointer;

    for (pointer = inString; *pointer != '\0'; pointer++)
    {
	**outString = *pointer;
	(*outString)++;
    }
}


/* Answers the field map (from field to subscription thingy) */
static Hashtable GetFieldMap()
{
    if (fieldMap == NULL)
    {
	fieldMap = Hashtable_alloc(13);
	Hashtable_put(fieldMap, "from", "FROM_NAME");
	Hashtable_put(fieldMap, "email", "FROM_EMAIL");
	Hashtable_put(fieldMap, "subject", "SUBJECT");
	Hashtable_put(fieldMap, "keywords", "KEYWORDS");
	Hashtable_put(fieldMap, "x-posts", "CROSS_POSTS");
	Hashtable_put(fieldMap, "newsgroups", "NEWSGROUPS");
    }

    return fieldMap;
}

/* Reads the next tuple from the FileStreamTokenizer */
static char *GetTupleFromTokenizer(FileStreamTokenizer tokenizer)
{
    char *field;
    char *operation;
    char *pattern;
    char *elvinField;

    /* Read the field */
    field = FileStreamTokenizer_next(tokenizer);
    if ((field == NULL) || (*field == '/') || (*field == '\n'))
    {
	return NULL;
    }

    if ((elvinField = Hashtable_get(GetFieldMap(), field)) == NULL)
    {
	fprintf(stderr, "*** Unrecognized field \"%s\"\n", field);
	exit(1);
    }
    free(field);

    /* Read the operation */
    operation = FileStreamTokenizer_next(tokenizer);
    if ((operation == NULL) || (*operation == '/') || (*field == '\n'))
    {
	return NULL;
    }

    /* Read the pattern */
    pattern = FileStreamTokenizer_next(tokenizer);
    if ((pattern == NULL) || (*pattern == '/') || (*field == '\n'))
    {
	return NULL;
    }

    printf("  field=\"%s\", operation=\"%s\", pattern=\"%s\"\n", elvinField, operation, pattern);
    free(operation);
    free(pattern);

    return strdup("bob");
}


/* Scans a non-empty, non-comment line of the usenet file and answers
 * a malloc'ed string corresponding to that line's part of the
 * subscription expression */
static char *GetFromUsenetFileLine(char *token, FileStreamTokenizer tokenizer)
{
    List list = List_alloc();
    int isGroupNegated = 0;
    char *group;

    /* Read the group: [not] groupname */
    if (strcmp(token, "not") == 0)
    {
	isGroupNegated = 1;
	if ((group = FileStreamTokenizer_next(tokenizer)) == NULL)
	{
	    fprintf(stderr, "*** Invalid usenet line containing \"%s\"\n", token);
	    return NULL;
	}

	printf("group: !\"%s\"\n", group);
    }
    else
    {
	group = token;
	printf("group: \"%s\"\n", group);
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

	/* Token separator? */
	if (*tok == '/')
	{
	    char *expression;
	    free(tok);

	    expression = GetTupleFromTokenizer(tokenizer);
	    if (expression == NULL)
	    {
		fprintf(stderr, "*** Invalid usenet line for group %s\n", group);
		exit(1);
	    }

	    List_addLast(list, expression);
	}
	else
	{
	    fprintf(stderr, "*** Invalid usenet line for group %s\n", group);
	    exit(1);
	}
    }

    return strdup("hi");
}

/* Scans the next line of the usenet file and answers a malloc'ed
 * string corresponding to that line's part of the subscription
 * expression */
static char *GetFromUsenetFile(FILE *file)
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
	    char *result = GetFromUsenetFileLine(token, tokenizer);
	    free(token);
	    return result;
	}
    }

    FileStreamTokenizer_free(tokenizer);
    fclose(file);

    return NULL;
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
    List list = List_alloc();
    char *prefix = "ELVIN_CLASS == \"NEWSWATCHER\" && ELVIN_SUBCLASS == \"MONITOR\" && ( ";
    char *suffix = " )";
    char *conjunction = " || ";
    char *expression;
    char *pointer;
    int size = 0;

    /* Get the subscription expressions for each individual line */
    while ((expression = GetFromUsenetFile(usenet)) != NULL)
    {
	if (List_isEmpty(list))
	{
	    size = strlen(expression);
	}
	else
	{
	    size += sizeof(conjunction) + strlen(expression);
	}

	List_addLast(list, expression);
    }

    /* If the List is empty, then return NULL now */
    if (List_isEmpty(list))
    {
/*	List_free(list);*/
	return NULL;
    }

    /* Construct one ubersubscription for all Usenet news */
    expression = (char *)alloca(sizeof(prefix) + sizeof(suffix) + size);
    pointer = expression;

    /* Copy the prefix into the expression */
    AppendString(prefix, &pointer);

    /* Copy in each of the List elements */
    List_doWith(list, AppendString, &pointer);
/*    List_do(list, free);*/
/*    List_free(list);*/
    
    /* Copy in the suffix */
    AppendString(suffix, &pointer);

    printf("expression: %s\n", expression);
    return UsenetSubscription_alloc("ELVIN_CLASS == \"NEWSWATCHER\"", callback, context);
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

