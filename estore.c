
/*
 * rcvstore.c -- asynchronously add mail to a folder
 *
 * $Id: estore.c,v 2.1 2000/05/08 06:38:41 phelps Exp $
 */

#include <h/mh.h>
#include <fcntl.h>
#include <h/signals.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <zotnet/mts/mts.h>
#include "parse_mail.h"

#define MAX_PACKET_SIZE 8192

static struct swit switches[] = {
#define CRETSW         0
    { "create",	0 },
#define NCRETSW        1
    { "nocreate", 0 },
#define UNSEENSW       2
    { "unseen", 0 },
#define NUNSEENSW      3
    { "nounseen", 0 },
#define PUBSW          4
    { "public",	0 },
#define NPUBSW         5
    { "nopublic",  0 },
#define ZEROSW         6
    { "zero",	0 },
#define NZEROSW        7
    { "nozero",	0 },
#define SEQSW          8
    { "sequence name", 0 },
#define HOSTSW	9
    { "host name", 0 },
#define PORTSW	10
    { "port number", 0 },
#define VERSIONSW      11
    { "version", 0 },
#define HELPSW        12
    { "help", 0 },
    { NULL, 0 }
};

extern int errno;

#define DEFAULT_HOST "elvin.dstc.edu.au"
#define DEFAULT_PORT 12304

/*
 * name of temporary file to store incoming message
 */
static char *tmpfilenam = NULL;


/* Copy a message from the input file to the output file and parse its 
 * headers at the same time */
int
cpydatanfn (lexer_t lexer, int in, int out, char *ifile, char *ofile)
{
    int i;
    char buffer[BUFSIZ];

    while ((i = read(in, buffer, sizeof(buffer))) > 0) {
	if (write(out, buffer, i) != i) {
	    adios(ofile, "error writing");
	}

	lex(lexer, buffer, i);
    }

    if (i < 0)
	adios(ifile, "error reading");

    return lex(lexer, buffer, 0);
}

/* Send a UDP packet to the given host and port */
static void
send_packet(char *hostname, int port, char *buffer, int length)
{
    struct sockaddr_in name;
    int fd;

    /* Initialize the name */
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(port);

    /* See if the hostname is in dot notation */
    if ((name.sin_addr.s_addr = inet_addr(hostname)) == (unsigned long)-1)
    {
	struct hostent *host;

	/* Check with the name service */
	if ((host = gethostbyname(hostname)) == NULL)
	{
	    return;
	}

	name.sin_addr = *(struct in_addr *)host -> h_addr;
    }

    /* Make a socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
	return;
    }

    /* Send the packet */
    sendto(fd, buffer, length, 0, (struct sockaddr *)&name, sizeof(name));
    close(fd);
}


int
main (int argc, char **argv)
{
    int publicsw = -1, zerosw = 0;
    int create = 1, unseensw = 1;
    int fd, msgnum, seqp = 0;
    char *cp, *maildir, *folder = NULL, buf[BUFSIZ];
    char **argp, **arguments, *seqs[NUMATTRS+1];
    char packet[MAX_PACKET_SIZE];
    struct msgs *mp;
    struct stat st;
    char *user = NULL;
    char *host = NULL;
    int port = 0;
    struct lexer lexer;
    int ok;

#ifdef LOCALE
    setlocale(LC_ALL, "");
#endif
    invo_name = r1bindex (argv[0], '/');

    /* read user profile/context */
    context_read();

    mts_init (invo_name);
    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    /* parse arguments */
    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
	    case AMBIGSW: 
		ambigsw (cp, switches);
		done (1);
	    case UNKWNSW: 
		adios (NULL, "-%s unknown", cp);

	    case HELPSW: 
		snprintf (buf, sizeof(buf), "%s [+folder] [switches]",
			  invo_name);
		print_help (buf, switches, 1);
		done (1);
	    case VERSIONSW:
		print_version(invo_name);
		done (1);

	    case SEQSW: 
		if (!(cp = *argp++) || *cp == '-')
		    adios (NULL, "missing argument name to %s", argp[-2]);

		/* check if too many sequences specified */
		if (seqp >= NUMATTRS)
		    adios (NULL, "too many sequences (more than %d) specified", NUMATTRS);
		seqs[seqp++] = cp;
		continue;

	    case HOSTSW:
		if (!(cp = *argp++) || *cp == '-')
		    adios (NULL, "missing argument to %s", argp[-2]);

		/* check if too many hosts specified */
		if (host) {
		    adios (NULL, "too many hosts specified");
		}
		host = cp;
		continue;

	    case PORTSW:
		if (!(cp = *argp++) || *cp == '-')
		    adios (NULL, "missing argument to %s", argp[-2]);

		/* check if too many ports specified */
		if (port)
		    adios (NULL, "too many ports specified");
		port = atoi(cp);
		continue;
		    
	    case UNSEENSW:
		unseensw = 1;
		continue;
	    case NUNSEENSW:
		unseensw = 0;
		continue;

	    case PUBSW: 
		publicsw = 1;
		continue;
	    case NPUBSW: 
		publicsw = 0;
		continue;

	    case ZEROSW: 
		zerosw++;
		continue;
	    case NZEROSW: 
		zerosw = 0;
		continue;

	    case CRETSW: 
		create++;
		continue;
	    case NCRETSW: 
		create = 0;
		continue;
	    }
	}
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		adios (NULL, "only one folder at a time!");
	    else
		folder = path (cp + 1, *cp == '+' ? TFOLDER : TSUBCWF);
	} else {
	    adios (NULL, "usage: %s [+folder] [switches]", invo_name);
	}
    }

    seqs[seqp] = NULL;	/* NULL terminate list of sequences */

    if (!context_find ("path"))
	free (path ("./", TFOLDER));

    /* if no folder is given, use default folder */
    if (!folder)
	folder = getfolder (0);
    maildir = m_maildir (folder);

    /* check if folder exists */
    if (stat (maildir, &st) == NOTOK) {
	if (errno != ENOENT)
	    adios (maildir, "error on folder");
	if (!create)
	    adios (NULL, "folder %s doesn't exist", maildir);
	if (!makedir (maildir))
	    adios (NULL, "unable to create folder %s", maildir);
    }

    if (chdir (maildir) == NOTOK)
	adios (maildir, "unable to change directory to");

    /* ignore a few signals */
    SIGNAL (SIGHUP, SIG_IGN);
    SIGNAL (SIGINT, SIG_IGN);
    SIGNAL (SIGQUIT, SIG_IGN);
    SIGNAL (SIGTERM, SIG_IGN);

    /* create a temporary file */
    tmpfilenam = m_scratch ("", invo_name);
    if ((fd = creat (tmpfilenam, m_gmprot ())) == NOTOK)
	adios (tmpfilenam, "unable to create");
    chmod (tmpfilenam, m_gmprot());

#if 0
    /* copy the message from stdin into temp file */
    cpydata (fileno (stdin), fd, "standard input", tmpfilenam);
#else
    /* Initialize the lexer */
    lexer_init(&lexer, packet, MAX_PACKET_SIZE);

    /* Set up the unotify header of the packet */
    user = getenv("USER");
    user = user ? user : getenv("LOGNAME");
    lexer_append_unotify_header(&lexer, user ? user : "doofus", folder);

    /* copy the message from stdin into temp file and construct an
     * elvin notification from its headers at the same time */
    ok = cpydatanfn (&lexer, fileno (stdin), fd, "standard input", tmpfilenam);
#endif

    if (fstat (fd, &st) == NOTOK) {
	unlink (tmpfilenam);
	adios (tmpfilenam, "unable to fstat");
    }
    if (close (fd) == NOTOK)
	adios (tmpfilenam, "error closing");

    /* don't add file if it is empty */
    if (st.st_size == 0) {
	unlink (tmpfilenam);
	advise (NULL, "empty file");
	done (0);
    }

    /*
     * read folder and create message structure
     */
    if (!(mp = folder_read (folder)))
	adios (NULL, "unable to read folder %s", folder);

    /*
     * Link message into folder, and possibly add
     * to the Unseen-Sequence's.
     */
    if ((msgnum = folder_addmsg (&mp, tmpfilenam, 0, unseensw, 0)) == -1)
	done (1);

    /*
     * Add the message to any extra sequences
     * that have been specified.
     */
    for (seqp = 0; seqs[seqp]; seqp++) {
	if (!seq_addmsg (mp, seqs[seqp], msgnum, publicsw, zerosw))
	    done (1);
    }

    seq_setunseen (mp, 0);	/* synchronize any Unseen-Sequence's      */
    seq_save (mp);		/* synchronize and save message sequences */
    folder_free (mp);		/* free folder/message structure          */

    context_save ();		/* save the global context file           */
    unlink (tmpfilenam);	/* remove temporary file                  */
    tmpfilenam = NULL;

    /* Send the elvin notification */
    if (ok == 0) {
	if (lexer_append_unotify_footer(&lexer, msgnum) == 0) {
	    send_packet(host ? host : DEFAULT_HOST,
			port ? port : DEFAULT_PORT,
			packet, lexer.point - packet);
	}
    }

    return done (0);
}

/*
 * Clean up and exit
 */
int
done(int status)
{
    if (tmpfilenam && *tmpfilenam)
	unlink (tmpfilenam);
    exit (status);
    return 1;  /* dead code to satisfy the compiler */
}
