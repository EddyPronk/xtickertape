#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define MAX_PACKET_SIZE 16384
#define BUFFER_SIZE 4096

#define UNOTIFY_PACKET 1
#define INT32_TYPECODE 1
#define STRING_TYPECODE 4
#define PROTO_VERSION_MAJOR 4
#define PROTO_VERSION_MINOR 0
#define VERSION_MAJOR 2
#define VERSION_MINOR 0

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 13131
#define N_VERSION_MAJOR "elvinmail"
#define N_VERSION_MINOR "elvinmail.minor"
#define N_FOLDER "folder"
#define N_USER "user"
#define N_INDEX "index"

typedef struct lexer *lexer_t;
typedef int (*lexer_state_t)(lexer_t self, int ch);

/* Forward declarations for the state machine states */
static int lex_error(lexer_t self, int ch);
static int lex_start(lexer_t self, int ch);
static int lex_first_name(lexer_t self, int ch);
static int lex_name(lexer_t self, int ch);
static int lex_dash(lexer_t self, int ch);
static int lex_ws(lexer_t self, int ch);
static int lex_body(lexer_t self, int ch);
static int lex_body_ws(lexer_t self, int ch);
static int lex_fold(lexer_t self, int ch);
static int lex_skip_body(lexer_t self, int ch);
static int lex_skip_fold(lexer_t self, int ch);
static int lex_end(lexer_t self, int ch);

/* A bunch of state information for the lexer */
struct lexer
{
    /* The current lexical state */
    lexer_state_t state;

    /* The packet buffer */
    char *buffer;

    /* The end of the packet buffer */
    char *end;

    /* The next character in the packet buffer */
    char *point;

    /* The number of attributes in the buffer */
    int count;

    /* The position of the first name in the buffer */
    char *first_name_point;

    /* The position of the attribute count in the buffer */
    char *count_point;

    /* The position of the length of the current string */
    char *length_point;
};

char *hostname = DEFAULT_HOST;
int port = DEFAULT_PORT;

/* The length-prefixed string `From' */
static char from_string[] = { 0, 0, 0, 4, 'F', 'r', 'o', 'm' };

/* Initializes a lexer's state */
static void lexer_init(lexer_t self, char *buffer, ssize_t length)
{
    self -> state = lex_start;
    self -> buffer = buffer;
    self -> end = buffer + length;
    self -> point = buffer;
    self -> count = 0;
    self -> count_point = NULL;
    self -> length_point = NULL;
}

/* Writes an int32 into a buffer */
static void write_int32(char *buffer, int value)
{
    unsigned char *point = (unsigned char *)buffer;
    point[0] = (value >> 24) & 0xff;
    point[1] = (value >> 16) & 0xff;
    point[2] = (value >> 8) & 0xff;
    point[3] = (value >> 0) & 0xff;
}

/* Reads an int32 from a buffer */
static int read_int32(char *buffer)
{
    unsigned char *point = (unsigned char *)buffer;
    return (point[0] << 24) | (point[1] << 16) | (point[2] << 8) | (point[3] << 0);
}

/* Append an int32 to the outgoing buffer */
static int append_int32(lexer_t self, int value)
{
    /* Make sure there's room */
    if (self -> point + 4 < self -> end)
    {
	write_int32(self -> point, value);
	self -> point += 4;
	return 0;
    }

    return -1;
}

/* Begin writing a string */
static int begin_string(lexer_t self)
{
    if (self -> point + 4 < self -> end)
    {
	self -> length_point = self -> point;
	self -> point += 4;
	return 0;
    }

    return -1;
}

/* Finish writing a string */
static int end_string(lexer_t self)
{
    char *string = self -> length_point + 4;

    /* Write the length */
    write_int32(self -> length_point, self -> point - string);

    /* Pad the string out to a 4-byte boundary */
    while ((int)self -> point & 3)
    {
	*(self -> point++) = '\0';
    }

    return 0;
}


/* Append a character to the current string */
static int append_char(lexer_t self, int ch)
{
    if (self -> point < self -> end)
    {
	*(self -> point++) = ch;
	return 0;
    }

    return -1;
}

/* Appends a C string to the buffer */
static int append_string(lexer_t self, char *string)
{
    char *point;

    begin_string(self);
    for (point = string; *point != '\0'; point++)
    {
	if (! (self -> point < self -> end))
	{
	    return -1;
	}

	*self -> point++ = *point;
    }

    end_string(self);
    return 0;
}

/* Compares two length-prefixed strings for equality */
static int lstring_eq(char *string1, char *string2)
{
    int len, i;

    /* First compare the lengths */
    len = read_int32(string1);
    if (read_int32(string2) != len)
    {
	return 0;
    }

    string1 += 4;
    string2 += 4;

    /* Then compare all chars */
    for (i = 0; i < len; i++)
    {
	if (string1[i] != string2[i])
	{
	    return 0;
	}
    }

    return 1;
}

/* Appends an int32 attribute to the buffer */
static int append_int32_tuple(lexer_t self, char *name, int value)
{
    /* Write the name */
    if (append_string(self, name) < 0)
    {
	return -1;
    }

    /* Write the value's type */
    if (append_int32(self, INT32_TYPECODE) < 0)
    {
	return -1;
    }

    /* Write the value */
    if (append_int32(self, value) < 0)
    {
	return -1;
    }

    self -> count++;
    return 0;
}

/* Appends a string attribute to the buffer */
static int append_string_tuple(lexer_t self, char *name, char *value)
{
    /* Write the name */
    if (append_string(self, name) < 0)
    {
	return -1;
    }

    /* Write the value's type */
    if (append_int32(self, STRING_TYPECODE) < 0)
    {
	return -1;
    }

    /* Write the value */
    if (append_string(self, value) < 0)
    {
	return -1;
    }

    self -> count++;
    return 0;
}



/* Writes a UNotify packet header */
static int lexer_append_unotify_header(lexer_t self, char *user, char *folder)
{
    /* Write the packet type */
    if (append_int32(self, UNOTIFY_PACKET) < 0)
    {
	return -1;
    }

    /* Write the protocol major version number */
    if (append_int32(self, PROTO_VERSION_MAJOR) < 0)
    {
	return -1;
    }

    /* Write the protocol minor version number */
    if (append_int32(self, PROTO_VERSION_MINOR) < 0)
    {
	return -1;
    }

    /* The number of attributes will go here */
    self -> count_point = self -> point;
    self -> point += 4;

    /* Write the notification format version numbers */
    if (append_int32_tuple(self, N_VERSION_MAJOR, VERSION_MAJOR) < 0)
    {
	return -1;
    }

    if (append_int32_tuple(self, N_VERSION_MINOR, VERSION_MINOR) < 0)
    {
	return -1;
    }

    /* Write the username */
    if (append_string_tuple(self, N_USER, user) < 0)
    {
	return -1;
    }

    /* Write the folder's name */
    if (append_string_tuple(self, N_FOLDER, folder) < 0)
    {
	return -1;
    }

    /* The first name will go next */
    self -> first_name_point = self -> point;
    return 0;
}

/* Writes the UNotify packet footer */
static int lexer_append_unotify_footer(lexer_t self)
{
    /* Record the number of attributes */
    write_int32(self -> count_point, self -> count);

    /* No keys */
    return append_int32(self, 0);
}



/* Begin an attribute name */
static int begin_name(lexer_t self)
{
    return begin_string(self);
}

/* End an attribute name */
static int end_name(lexer_t self)
{
    char *name;

    /* Tidy up the string */
    if (end_string(self) < 0)
    {
	return -1;
    }

    /* Check if we've already seen this name */
    name = self -> first_name_point;
    while (name < self -> length_point)
    {
	int length;

	/* Look for a match */
	if (lstring_eq(name, self -> length_point))
	{
	    /* Already have this attribute.  Discard the new one. */
	    self -> point = self -> length_point;
	    self -> state = lex_skip_body;
	    return 1;
	}

	/* Skip to the next name */
	length = read_int32(name);
	/* FIX THIS: need a better constant here */
	name += length + (MAX_PACKET_SIZE - length) % 4 + 8;
	length = read_int32(name);
	name += length + (MAX_PACKET_SIZE - length) % 4 + 4;
    }

    return 0;
}


/* Begin an attribute string value */
static int begin_string_value(lexer_t self)
{
    if (self -> point + 4 < self -> end)
    {
	/* Write the string typecode */
	append_int32(self, STRING_TYPECODE);

	return begin_string(self);
    }

    return -1;
}

/* End an attribute string value */
static int end_string_value(lexer_t self)
{
    end_string(self);
    self -> count++;
    return 0;
}

/* Ignore additional input */
static int lex_error(lexer_t self, int ch)
{
    return -1;
}

/* Get started */
static int lex_start(lexer_t self, int ch)
{
    /* Bitch and moan if we start with a space or a colon */
    if (ch == ':' || isspace(ch))
    {
	self -> state = lex_error;
	return -1;
    }

    /* Start a name field */
    if (begin_name(self) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Anything else is part of the first field's name */
    if (append_char(self, toupper(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_first_name;
    return 0;
}

/* Reading the first field name */
static int lex_first_name(lexer_t self, int ch)
{
    /* SPECIAL CASE: the first line of may an out-of-band `From' line
     * in which the `From' doesn't end in a colon. */
    if (ch == ' ')
    {
	/* Complete the name string */
	if (end_name(self) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	/* Make sure it matches `From' */
	if (! lstring_eq(self -> length_point, from_string))
	{
	    self -> state = lex_error;
	    return -1;
	}

	/* Convert the first character to lowercase */
	self -> length_point[4] = 'f';

	self -> state = lex_ws;
	return 0;
    }

    /* Other whitespace is an error */
    if (isspace(ch))
    {
	self -> state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':')
    {
	if (end_name(self) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	self -> state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, tolower(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_first_name;
    return 0;
}

/* Reading a field name */
static int lex_name(lexer_t self, int ch)
{
    if (isspace(ch))
    {
	self -> state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':')
    {
	int result;

	if ((result = end_name(self)) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result)
	{
	    self -> state = lex_skip_body;
	    return 0;
	}

	self -> state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, tolower(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_name;
    return 0;
}

/* We've read a `-' in a name */
static int lex_dash(lexer_t self, int ch)
{
    /* Spaces are bogus in field names */
    if (isspace(ch))
    {
	self -> state = lex_error;
	return -1;
    }

    /* A colon indicates the end of the field name */
    if (ch == ':')
    {
	int result;

	if ((result = end_name(self)) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result)
	{
	    self -> state = lex_skip_body;
	    return 0;
	}

	self -> state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, toupper(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for additional dashes */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_name;
    return 0;
}

/* Skip over whitespace */
static int lex_ws(lexer_t self, int ch)
{
    /* Try to fold on a LF */
    if (ch == '\n')
    {
	if (begin_string_value(self) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	self -> state = lex_fold;
	return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch))
    {
	self -> state = lex_ws;
	return 0;
    }

    /* Anything else is part of the body */
    if (begin_string_value(self) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    return lex_body(self, ch);
}

/* Read the body */
static int lex_body(lexer_t self, int ch)
{
    /* Try to fold on LF */
    if (ch == '\n')
    {
	self -> state = lex_fold;
	return 0;
    }

    /* Compress whitespace */
    if (isspace(ch))
    {
	self -> state = lex_body_ws;
	return 0;
    }

    /* Anything else is part of the body */
    if (append_char(self, ch) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    self -> state = lex_body;
    return 0;
}

/* Compressing whitespace in the body */
static int lex_body_ws(lexer_t self, int ch)
{
    /* Watch for linefeeds */
    if (ch == '\n')
    {
	self -> state = lex_fold;
	return 0;
    }

    /* Discard other whitespace */
    if (isspace(ch))
    {
	self -> state = lex_body_ws;
	return 0;
    }

    /* Insert an actual space first */
    if (append_char(self, ' ') < 0)
    {
	return -1;
    }

    /* Anything else is part of the body */
    return lex_body(self, ch);
}

/* Try to fold */
static int lex_fold(lexer_t self, int ch)
{
    /* A linefeed means that we're out of headers */
    if (ch == '\n')
    {
	if (end_string_value(self) < 0)
	{
	    self -> state = lex_error;
	    return -1;
	}

	self -> state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch))
    {
	self -> state = lex_body_ws;
	return 0;
    }

    if (end_string_value(self) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* A colon here is an error */
    if (ch == ':')
    {
	self -> state = lex_error;
	return 0;
    }

    /* Anything else is the start of the next field */
    if (begin_name(self) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Make sure the first letter is always uppercase */
    if (append_char(self, toupper(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_name;
    return 0;
}

/* Discard the body */
static int lex_skip_body(lexer_t self, int ch)
{
    /* Try to fold on LF */
    if (ch == '\n')
    {
	self -> state = lex_skip_fold;
	return 0;
    }

    /* Anything else is part of the body */
    self -> state = lex_skip_body;
    return 0;
}

/* Try to fold */
static int lex_skip_fold(lexer_t self, int ch)
{
    /* A linefeed means the end of the headers */
    if (ch == '\n')
    {
	self -> state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch))
    {
	self -> state = lex_skip_body;
	return 0;
    }

    /* Anything else is the beginning of the next name */
    if (begin_name(self) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Make sure the first letter is always uppercase */
    if (append_char(self, toupper(ch)) < 0)
    {
	self -> state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-')
    {
	self -> state = lex_dash;
	return 0;
    }

    self -> state = lex_name;
    return 0;
}

/* Skip everything after the headers */
static int lex_end(lexer_t self, int ch)
{
    self -> state = lex_end;
    return 0;
}



/* Run the buffer through the lexer */
static int lex(lexer_t self, char *buffer, ssize_t length)
{
    char *point;

    /* Watch for the end-of-input marker */
    if (length == 0)
    {
	return self -> state(self, EOF);
    }

    for (point = buffer; point < buffer + length; point++)
    {
	if (self -> state(self, *point) < 0)
	{
	    return -1;
	}
    }

    return 0;
}

static int dump_buffer(char *buffer, int length)
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
	while ((host = gethostbyname(hostname)) == NULL)
	{
	    switch (h_errno)
	    {
		case HOST_NOT_FOUND:
		{
		    fprintf(stderr, "Error: unknown host: %s\n", hostname);
		    exit(1);
		}

		case TRY_AGAIN:
		{
		    break;
		}

		default:
		{
		    fprintf(stderr, "Error: internal error: %s\n", hostname);
		    return -1;
		}
	    }
	}

	name.sin_addr = *(struct in_addr *)host -> h_addr;
    }

    /* Create a socket */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
	perror("socket(): failed");
	return -1;
    }

    /* Send the packet */
    if (sendto(fd, buffer, length, 0, (struct sockaddr *)&name, sizeof(name)) < 0)
    {
	perror("sendto(): failed");
	return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct lexer lexer;
    char packet[MAX_PACKET_SIZE];
    char buffer[BUFFER_SIZE];
    int fd = STDIN_FILENO;

    if (argc > 1)
    {
	if ((fd = open(argv[1], O_RDONLY)) < 0)
	{
	    perror("open(): failed");
	    exit(1);
	}
    }

    /* Initialize the lexer */
    lexer_init(&lexer, packet, MAX_PACKET_SIZE);

    /* Write the header */
    if (lexer_append_unotify_header(&lexer, "phelps", "Inbox") < 0)
    {
	abort();
    }

    /* Read the attributes and add them to the notification */
    while (1)
    {
	ssize_t length;
	int result;

	/* Read some more */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    perror("read(): failed");
	    exit(1);
	}

	/* Run it through the lexer */
	result = lex(&lexer, buffer, length);

	/* Check for end of input */
	if (length == 0)
	{
	    if (result < 0)
	    {
		exit(1);
	    }

	    /* Write the footer */
	    if (lexer_append_unotify_footer(&lexer) < 0)
	    {
		abort();
	    }

	    /* What have we got? */
	    dump_buffer(lexer.buffer, lexer.point - lexer.buffer);
	    close(fd);
	    exit(0);
	}
    }
}
