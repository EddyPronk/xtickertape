
/*
 * estore.c -- asynchronously add mail to a folder
 *             and send out an elvin notification
 *
 * This code is mostly stolen from rcvstore.c
 *
 * $Id: estore.c,v 1.1 1999/01/11 07:31:36 phelps Exp $
 */

#include <elvin3/elvin.h>
#include <h/mh.h>
#include <fcntl.h>
#include <h/signals.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>

/* Default elvin server */
#define HOST "elvin"
#define PORT 5678

static struct swit switches[] =
{
#define CRETSW 0
    { "create", 0 },
#define NCRETSW 1
    { "nocreate", 0 },
#define UNSEENSW 2
    { "unseen", 0 },
#define NUNSEENSW 3
    { "nounseen", 0 },
#define PUBSW 4
    { "public", 0 },
#define NPUBSW 5
    { "nopublic", 0 },
#define ZEROSW 6
    { "zero", 0 },
#define NZEROSW 7
    { "nozero", 0 },
#define SEQSW 8
    { "sequence name", 0 },
#define HOSTSW 9
    { "host name", 0 },
#define PORTSW 10
    { "port number", 0 },
#define USERSW 11
    { "user name", 0 },
#define VERSIONSW 12
    { "version", 0 },
#define HELPSW 13
    { "help", 4 },
    { NULL, 0 }
};


/* No quench callback */
static void (*QuenchCallback)(elvin_t elvin, void *data, char *quench) = NULL;

/* No error callback */
static void (*ErrorCallback)(
    elvin_t elvin, void *data,
    elvin_error_code_t code, char *message) = NULL;

/* The states of the lexer */
typedef enum
{
    StartState,
    WhitespaceState,
    FieldNameState,
    FieldBodyState,
    MustFoldState,
    FoldState,
    EndState,
    ErrorState
} LexerState;

extern int errno;

/* Name of temporary file in which to store incoming message */
static char *tmpfilename = NULL;

/* The lexer state */
static LexerState lexer_state = StartState;

/* The lexer's field-name buffer */
static char *field_name_buffer = NULL;
static char *field_name_pointer = NULL;
static char *field_name_end;

/* The lexer's field-body buffer */
static char *field_body_buffer = NULL;
static char *field_body_pointer = NULL;
static char *field_body_end;

/* The elvin notification */
en_notify_t notification = NULL;


/* Appends a character to the input buffer */
static void FieldNameAppend(int ch)
{
    /* Make sure we have enough space for the next character */
    if (! (field_name_pointer < field_name_end))
    {
	/* Double the field name space */
	size_t length = 2 * (field_name_end - field_name_buffer);
	char *buffer = realloc(field_name_buffer, length);
	field_name_end = buffer + length;
	field_name_pointer = buffer + (field_name_pointer - field_name_buffer);
	field_name_buffer = buffer;
    }

    /* Append the character */
    *(field_name_pointer++) = (char)ch;
}

/* Clears out the field name buffer */
static void FieldNameClear()
{
    /* Make sure we have a buffer! */
    if (field_name_buffer == NULL)
    {
	field_name_buffer = (char *)malloc(16);
	field_name_end = field_name_buffer + 16;
    }

    field_name_pointer = field_name_buffer;
}

/* Appends a character to the input buffer */
static void FieldBodyAppend(int ch)
{
    /* Make sure we have enough space for the next character */
    if (! (field_body_pointer < field_body_end))
    {
	/* Double the field body space */
	size_t length = 2 * (field_body_end - field_body_buffer);
	char *buffer = realloc(field_body_buffer, length);
	field_body_end = buffer + length;
	field_body_pointer = buffer + (field_body_pointer - field_body_buffer);
	field_body_buffer = buffer;
    }

    /* Append the character */
    *(field_body_pointer++) = (char)ch;
}

/* Clears out the field body buffer */
static void FieldBodyClear()
{
    /* Make sure we have a buffer! */
    if (field_body_buffer == NULL)
    {
	field_body_buffer = (char *)malloc(256);
	field_body_end = field_body_buffer + 256;
    }

    field_body_pointer = field_body_buffer;
}


/* Update the state of the Lexer according to the input character */
static void Lex(int ch)
{
    switch (lexer_state)
    {
	/* Start state */
	case StartState:
	{
	    /* Initialize the buffers and notification */
	    notification = en_new();
	    FieldNameClear();
	    FieldBodyClear();

	    /* Can't start with a space or colon */
	    if ((ch <= ' ') || (ch == ':'))
	    {
		lexer_state = ErrorState;
		return;
	    }

	    /* CR indicates end of headers */
	    if (ch == '\n')
	    {
		lexer_state = EndState;
		return;
	    }

	    /* Anything else takes us to the FieldNameState */
	    FieldNameAppend(toupper(ch));
	    lexer_state = FieldNameState;
	    return;
	}

	/* Reading whitespace at beginning of body */
	case WhitespaceState:
	{
	    /* If we get a CR, then go to MustFoldState */
	    if (ch == '\n')
	    {
		lexer_state = MustFoldState;
		return;
	    }

	    /* If linear whitespace, then stay in this state */
	    if ((ch == ' ') || (ch == '\t'))
	    {
		return;
	    }

	    /* For anything else, go to the FieldBodyState */
	    FieldBodyAppend(ch);
	    lexer_state = FieldBodyState;
	    return;
	}

	/* Reading field name */
	case FieldNameState:
	{
	    /* Spaces and control characters are not permitted in field names */
	    if (ch < ' ')
	    {
		lexer_state = ErrorState;
		return;
	    }

	    /* Colon indicates the end of the field name */
	    if (ch == ':')
	    {
		FieldNameAppend('\0');
		lexer_state = WhitespaceState;
		return;
	    }

	    /* SPECIAL CASE: If we've encounter a space on the first
	     * field name and it's "From", then permit it */
	    if ((ch == ' ') && (en_count(notification) == 0))
	    {
		FieldNameAppend('\0');
		if (strcmp(field_name_buffer, "From") != 0)
		{
		    lexer_state = ErrorState;
		    return;
		}

		/* Make this a lowercase 'from' so that we can
		 * distinguish it from any later From: fields */
		*field_name_buffer = 'f';
		lexer_state = WhitespaceState;
		return;
	    }

	    /* Anything else is part of the field name.  Convert it to lowercase */
	    FieldNameAppend(tolower(ch));
	    return;
	}

	/* We're reading the field body.  Keep reading until CR */
	case FieldBodyState:
	{
	    if (ch == '\n')
	    {
		lexer_state = FoldState;
		return;
	    }

	    FieldBodyAppend(ch);
	    return;
	}

	/* We've read an empty field body.  See if it's merely folded */
	case MustFoldState:
	{
	    /* If we get linear whitespace, then we're ok */
	    if ((ch == ' ') || (ch == '\t'))
	    {
		lexer_state = WhitespaceState;
		return;
	    }

	    /* Otherwise, we're in error */
	    lexer_state = ErrorState;
	    return;
	}

	/* We've read a CR.  Either start reading a header or read folded body */
	case FoldState:
	{
	    /* Linear whitespace means folded body */
	    if ((ch == ' ') || (ch == '\t'))
	    {
		FieldBodyAppend(' ');
		lexer_state = WhitespaceState;
		return;
	    }

	    /* Add the field's name and body to the notification */
	    FieldBodyAppend('\0');
	    en_add_string(notification, field_name_buffer, field_body_buffer);
	    FieldBodyClear();

	    /* A linefeed means that we're at the end of the headers */
	    if (ch == '\n')
	    {
		lexer_state = EndState;
		return;
	    }

	    /* A colon puts us in an error state */
	    if (ch == ':')
	    {
		lexer_state = ErrorState;
		return;
	    }

	    /* Anything else is the start of a field name */
	    FieldNameClear();
	    FieldNameAppend(toupper(ch));
	    lexer_state = FieldNameState;
	    return;
	}

	/* There's no escape from the end state */
	case EndState:
	{
	    return;
	}

	/* There is no escape from the error state */
	case ErrorState:
	{
	    return;
	}
    }
}


/* Copy FILE in to FILE out and parse its contents as we go */
static void CopyAndParse(FILE *in, FILE *out, char *in_name, char *out_name)
{
    int ch;

    /* Read a character */
    while ((ch = fgetc(in)) != EOF)
    {
	/* Write it to the output file */
	if (fputc(ch, out) == EOF)
	{
	    adios(out_name, "error writing");
	}

	/* Update the state of the lexer */
	Lex(ch);
    }

    Lex(EOF);
}



/* Parse args, perform actions and exit */
int main(int argc, char *argv[])
{
    int publicsw = -1;
    int zerosw = 0;
    int create = 1;
    int unseensw = 1;
    int fd;
    FILE *file;
    int msgnum;
    int seqp = 0;
    char *cp;
    char *maildir;
    char *folder = NULL;
    char buf[BUFSIZ];
    char **argp;
    char **arguments;
    char *seqs[NUMATTRS + 1];
    struct msgs *mp;
    struct stat st;
    char *host = NULL;
    int port = 0;
    char *user = NULL;

#ifdef LOCALE
    setlocale(LC_ALL, "");
#endif /* LOCALE */

    invo_name = r1bindex(argv[0], '/');

    /* Read the user profile and context */
    context_read();

    /* Parse the command line */
    mts_init(invo_name);
    arguments = getarguments(invo_name, argc, argv, 1);
    argp = arguments;

    while (cp = *(argp++))
    {
	if (*cp == '-')
	{
	    switch (smatch(++cp, switches))
	    {
		case AMBIGSW:
		{
		    ambigsw(cp, switches);
		    done(1);
		}

		case UNKWNSW:
		{
		    adios(NULL, "-%s unknown", cp);
		}

		case HELPSW:
		{
		    snprintf(buf, sizeof(buf), "%s [+folder] [switches]", invo_name);
		    print_help(buf, switches, 1);
		    done(1);
		}

		case SEQSW:
		{
		    if (! (cp = *(argp++)) || (*cp == '-'))
		    {
			adios(NULL, "missing argument name to %s", argp[-2]);
		    }

		    if (seqp < NUMATTRS)
		    {
			seqs[seqp++] = cp;
		    }
		    else
		    {
			adios(NULL, "only %d sequences allowed!", NUMATTRS);
		    }

		    continue;
		}

		case HOSTSW:
		{
		    if (! (cp = *(argp++)) || (*cp == '-'))
		    {
			adios(NULL, "missing argument to %s", argp[-2]);
		    }

		    if (host != NULL)
		    {
			adios(NULL, "only one elvin host allowed!");
		    }

		    host = cp;
		    continue;
		}

		case PORTSW:
		{
		    if (! (cp = *(argp++)) || (*cp == '-'))
		    {
			adios(NULL, "missing argument to %s", argp[-2]);
		    }

		    if (port != 0)
		    {
			adios(NULL, "only one elvin port allowed!");
		    }

		    port = atoi(cp);
		    continue;
		}

		case USERSW:
		{
		    if (! (cp = *(argp++)) || (*cp == '-'))
		    {
			adios(NULL, "missing argument to %s", argp[-2]);
		    }

		    if (user != NULL)
		    {
			adios(NULL, "only one user name allowed!\n");
		    }

		    user = cp;
		    continue;
		}

		case UNSEENSW:
		{
		    unseensw = 1;
		    continue;
		}

		case NUNSEENSW:
		{
		    unseensw = 0;
		    continue;
		}

		case PUBSW:
		{
		    publicsw = 1;
		    continue;
		}

		case NPUBSW:
		{
		    publicsw = 0;
		    continue;
		}

		case ZEROSW:
		{
		    zerosw = 1;
		    continue;
		}

		case NZEROSW:
		{
		    zerosw = 0;
		    continue;
		}

		case CRETSW:
		{
		    create = 1;
		    continue;
		}

		case NCRETSW:
		{
		    create = 0;
		    continue;
		}
	    }
	}

	/* Folder (or subfolder) name? */
	if ((*cp == '+') || (*cp == '@'))
	{
	    if (folder)
	    {
		adios(NULL, "only one folder at a time!");
	    }
	    else
	    {
		folder = path(cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	    }
	}
	else
	{
	    adios(NULL, "usage: %s [+folder] [switches]", invo_name);
	}
    }

    /* Make sure we have a hostname */
    if (host == NULL)
    {
	host = HOST;
    }

    /* Make sure we have a port number */
    if (port == 0)
    {
	port = PORT;
    }

    /* Make sure we have a user name */
    if (user == NULL)
    {
	user = getpwuid(getuid()) -> pw_name;
    }

    /* NULL terminate the list of sequences */
    seqs[seqp] = NULL;

    /* Look up our path */
    if (! context_find("path"))
    {
	free(path("./", TFOLDER));
    }

    /* If no folder is given then use the default folder */
    if (! folder)
    {
	folder = getfolder(0);
    }

    maildir = m_maildir(folder);

    /* Check if folder exists */
    if (stat(maildir, &st) == NOTOK)
    {
	/* Something went wrong */
	if (errno != ENOENT)
	{
	    adios(maildir, "error on folder");
	}

	/* Quit if we're not supposed to create missing folders */
	if (! create)
	{
	    adios(NULL, "folder %s doesn't exist", maildir);
	}

	/* Try to create the folder */
	if (! makedir(maildir))
	{
	    adios(NULL, "unable to create folder %s", maildir);
	}
    }

    /* Try to change directory to the folder */
    if (chdir(maildir) == NOTOK)
    {
	adios(maildir, "unable to change directory to");
    }

    /* Make us difficult to interrupt */
    SIGNAL(SIGHUP, SIG_IGN);
    SIGNAL(SIGINT, SIG_IGN);
    SIGNAL(SIGQUIT, SIG_IGN);
    SIGNAL(SIGTERM, SIG_IGN);

    /* Create a temporary file */
    tmpfilename = m_scratch("", invo_name);
    if ((fd = creat(tmpfilename, m_gmprot())) == NOTOK)
    {
	adios(tmpfilename, "unable to create");
    }

    /* Create a FILE abstraction over the temp file */
    if ((file = fdopen(fd, "w")) == NULL)
    {
	/* Copy the message from stdin into the temporary file */
	cpydata(fileno(stdin), fd, "standard input", tmpfilename);

	/* Make sure our copy went smoothly */
	if (fstat(fd, &st) == NOTOK)
	{
	    unlink(tmpfilename);
	    adios(tmpfilename, "unable to fstat");
	}

	/* Close it */
	if (close(fd) == NOTOK)
	{
	    adios(tmpfilename, "error closing");
	}
    }
    else
    {
	/* Copy the message and parse it as we go */
	CopyAndParse(stdin, file, "standard input", tmpfilename);
	fflush(file);

	/* Make sure our copy went smoothly */
	if (fstat(fd, &st) == NOTOK)
	{
	    unlink(tmpfilename);
	    adios(tmpfilename, "unable to fstat");
	}

	/* Close the file */
	if (fclose(file) == NOTOK)
	{
	    adios(tmpfilename, "error closing");
	}
    }


    /* Throw away empty files */
    if (st.st_size == 0)
    {
	unlink(tmpfilename);
	advise(NULL, "empty file");
	done(0);
    }

    /* Read folder and create message structure */
    if (! (mp = folder_read(folder)))
    {
	adios(NULL, "unable to read folder %s", folder);
    }

    /* Link message into folder and possibly add to the
     * Unseen sequences */
    if ((msgnum = folder_addmsg(&mp, tmpfilename, 0, unseensw, 0)) == -1)
    {
	done(1);
    }

    /* Add the message to any extra sequences that
     * have been specified */
    for (seqp = 0; seqs[seqp]; seqp++)
    {
	if (! seq_addmsg(mp, seqs[seqp], msgnum, publicsw, zerosw))
	{
	    done(1);
	}
    }

    /* Synchronize any unseen sequences */
    seq_setunseen(mp, 0);

    /* Synchronize and save message sequences */
    seq_save(mp);

    /* Free the folder/message structure */
    folder_free(mp);

    /* Save the global context file */
    context_save();

    /* Remove the temporary file */
    unlink(tmpfilename);
    tmpfilename = NULL;

    /* If we have successfully parsed the headers, then send the notification */
    if (lexer_state == EndState)
    {
	elvin_t elvin;

	/* Add some useful fields */
	en_add_string(notification, "elvinmail", "1.0");
	en_add_string(notification, "folder", folder);
	en_add_int32(notification, "index", msgnum);
	en_add_string(notification, "user", user);

	/* Try to send the notification */
	if ((elvin = elvin_connect(
	    EC_NAMEDHOST, host, port,
	    QuenchCallback, NULL, ErrorCallback, NULL, 1)) != NULL)
	{
	    elvin_notify(elvin, notification);
	    en_free(notification);
	    elvin_disconnect(elvin);
	}
    }

    done(0);
}


/* Clean up and exit */
void done(int status)
{
    /* Delete the temporary file if it's still around */
    if (tmpfilename && *tmpfilename)
    {
	unlink(tmpfilename);
    }

    exit(status);
}
