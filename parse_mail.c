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
#include "parse_mail.h"

#define UNOTIFY_PACKET 32
#define INT32_TYPECODE 1
#define STRING_TYPECODE 4
#define PROTO_VERSION_MAJOR 4
#define PROTO_VERSION_MINOR 0
#define VERSION_MAJOR 2
#define VERSION_MINOR 0

#define N_VERSION_MAJOR "elvinmail"
#define N_VERSION_MINOR "elvinmail.minor"
#define N_FOLDER "folder"
#define N_USER "user"
#define N_INDEX "index"

#define ALIGN_4(x) ((((x) + 3) >> 2) << 2)

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

/* The length-prefixed string `From' */
static const char *from_string = "\0\0\0\4From";

/* Initializes a lexer's state */
void lexer_init(lexer_t self, char *buffer, ssize_t length)
{
    self->state = lex_start;
    self->buffer = buffer;
    self->end = buffer + length;
    self->point = buffer;
    self->count = 0;
    self->count_point = NULL;
    self->length_point = NULL;
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
static int read_int32(const char *buffer)
{
    unsigned char *point = (unsigned char *)buffer;
    return (point[0] << 24) | (point[1] << 16) | (point[2] << 8) | (point[3] << 0);
}

/* Append an int32 to the outgoing buffer */
static int append_int32(lexer_t self, int value)
{
    /* Make sure there's room */
    if (self->point + 4 < self->end) {
	write_int32(self->point, value);
	self->point += 4;
	return 0;
    }

    return -1;
}

/* Begin writing a string */
static int begin_string(lexer_t self)
{
    if (self->point + 4 < self->end) {
	self->length_point = self->point;
	self->point += 4;
	return 0;
    }

    return -1;
}

/* Finish writing a string */
static int end_string(lexer_t self)
{
    char *string = self->length_point + 4;

    /* Write the length */
    write_int32(self->length_point, self->point - string);

    /* Pad the string out to a 4-byte boundary */
    while ((intptr_t)(self->point) & 3) {
	*(self->point++) = '\0';
    }

    return 0;
}


/* Append a character to the current string */
static int append_char(lexer_t self, int ch)
{
    if (self->point < self->end) {
	*(self->point++) = ch;
	return 0;
    }

    return -1;
}

/* Appends a C string to the buffer */
static int append_string(lexer_t self, char *string)
{
    char *point;

    begin_string(self);
    for (point = string; *point != '\0'; point++) {
	if (self->point >= self->end) {
	    return -1;
	}

	*self->point++ = *point;
    }

    end_string(self);
    return 0;
}

/* Compares two length-prefixed strings for equality */
static int lstring_eq(const char *string1, const char *string2)
{
    int len, i;

    /* First compare the lengths */
    len = read_int32(string1);
    if (read_int32(string2) != len) {
	return 0;
    }

    string1 += 4;
    string2 += 4;

    /* Then compare all chars */
    for (i = 0; i < len; i++) {
	if (string1[i] != string2[i]) {
	    return 0;
	}
    }

    return 1;
}

/* Appends an int32 attribute to the buffer */
static int append_int32_tuple(lexer_t self, char *name, int value)
{
    /* Write the name */
    if (append_string(self, name) < 0) {
	return -1;
    }

    /* Write the value's type */
    if (append_int32(self, INT32_TYPECODE) < 0) {
	return -1;
    }

    /* Write the value */
    if (append_int32(self, value) < 0) {
	return -1;
    }

    self->count++;
    return 0;
}

/* Appends a string attribute to the buffer */
static int append_string_tuple(lexer_t self, char *name, char *value)
{
    /* Write the name */
    if (append_string(self, name) < 0) {
	return -1;
    }

    /* Write the value's type */
    if (append_int32(self, STRING_TYPECODE) < 0) {
	return -1;
    }

    /* Write the value */
    if (append_string(self, value) < 0) {
	return -1;
    }

    self->count++;
    return 0;
}



/* Writes a UNotify packet header */
int lexer_append_unotify_header(lexer_t self, char *user, char *folder)
{
    /* Write the packet type */
    if (append_int32(self, UNOTIFY_PACKET) < 0) {
	return -1;
    }

    /* Write the protocol major version number */
    if (append_int32(self, PROTO_VERSION_MAJOR) < 0) {
	return -1;
    }

    /* Write the protocol minor version number */
    if (append_int32(self, PROTO_VERSION_MINOR) < 0) {
	return -1;
    }

    /* The number of attributes will go here */
    self->count_point = self->point;
    self->point += 4;

    /* Write the notification format version numbers */
    if (append_int32_tuple(self, N_VERSION_MAJOR, VERSION_MAJOR) < 0) {
	return -1;
    }

    if (append_int32_tuple(self, N_VERSION_MINOR, VERSION_MINOR) < 0) {
	return -1;
    }

    /* Write the username */
    if (append_string_tuple(self, N_USER, user) < 0) {
	return -1;
    }

    /* Write the folder's name */
    if (folder) {
	if (append_string_tuple(self, N_FOLDER, folder) < 0) {
	    return -1;
	}
    }

    /* The first name will go next */
    self->first_name_point = self->point;
    return 0;
}

/* Writes the UNotify packet footer */
int lexer_append_unotify_footer(lexer_t self, int msg_num)
{
    /* Append the message number (if provided) */
    if (! (msg_num < 0)) {
	if (append_int32_tuple(self, N_INDEX, msg_num) < 0) {
	    return -1;
	}
    }

    /* Record the number of attributes */
    write_int32(self->count_point, self->count);

    /* We'll happily deliver_insecure */
    if (append_int32(self, 1) < 0) {
	return -1;
    }

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
    int length;

    /* Tidy up the string */
    if (end_string(self) < 0) {
	return -1;
    }

    /* Check if we've already seen this name */
    name = self->first_name_point;
    while (name < self->length_point) {
	/* Look for a match */
	if (lstring_eq(name, self -> length_point)) {
	    /* Already have this attribute.  Discard the new one. */
	    self->point = self -> length_point;
	    self->state = lex_skip_body;
	    return 1;
	}

	/* Skip to the next name */
	length = read_int32(name);
	name += ALIGN_4(length) + 8;
	length = read_int32(name);
	name += ALIGN_4(length) + 4;
    }

    return 0;
}


/* Begin an attribute string value */
static int begin_string_value(lexer_t self)
{
    if (self->point + 4 < self->end) {
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
    self->count++;
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
    if (ch == ':' || isspace(ch)) {
	self->state = lex_error;
	return -1;
    }

    /* Start a name field */
    if (begin_name(self) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Anything else is part of the first field's name */
    if (append_char(self, toupper(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_first_name;
    return 0;
}

/* Reading the first field name */
static int lex_first_name(lexer_t self, int ch)
{
    /* SPECIAL CASE: the first line of may an out-of-band `From' line
     * in which the `From' doesn't end in a colon. */
    if (ch == ' ') {
	/* Complete the name string */
	if (end_name(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Make sure it matches `From' */
	if (! lstring_eq(self->length_point, from_string)) {
	    self->state = lex_error;
	    return -1;
	}

	/* Convert the first character to lowercase */
	self->length_point[4] = 'f';

	self->state = lex_ws;
	return 0;
    }

    /* Other whitespace is an error */
    if (isspace(ch)) {
	self->state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':') {
	if (end_name(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	self->state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, tolower(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_first_name;
    return 0;
}

/* Reading a field name */
static int lex_name(lexer_t self, int ch)
{
    int result;

    if (isspace(ch)) {
	self->state = lex_error;
	return -1;
    }

    /* A colon is the end of the field name */
    if (ch == ':') {
	if ((result = end_name(self)) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result) {
	    self->state = lex_skip_body;
	    return 0;
	}

	self->state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, tolower(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for dashes */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_name;
    return 0;
}

/* We've read a `-' in a name */
static int lex_dash(lexer_t self, int ch)
{
    int result;

    /* Spaces are bogus in field names */
    if (isspace(ch)) {
	self->state = lex_error;
	return -1;
    }

    /* A colon indicates the end of the field name */
    if (ch == ':') {
	if ((result = end_name(self)) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Watch for repeated names */
	if (result) {
	    self->state = lex_skip_body;
	    return 0;
	}

	self->state = lex_ws;
	return 0;
    }

    /* Anything else is part of the field name */
    if (append_char(self, toupper(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for additional dashes */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_name;
    return 0;
}

/* Skip over whitespace */
static int lex_ws(lexer_t self, int ch)
{
    /* Try to fold on a LF */
    if (ch == '\n') {
	if (begin_string_value(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	self->state = lex_fold;
	return 0;
    }

    /* Skip other whitespace */
    if (isspace(ch)) {
	self->state = lex_ws;
	return 0;
    }

    /* Anything else is part of the body */
    if (begin_string_value(self) < 0) {
	self->state = lex_error;
	return -1;
    }

    return lex_body(self, ch);
}

/* Read the body */
static int lex_body(lexer_t self, int ch)
{
    /* Try to fold on LF */
    if (ch == '\n') {
	self->state = lex_fold;
	return 0;
    }

    /* Compress whitespace */
    if (isspace(ch)) {
	self->state = lex_body_ws;
	return 0;
    }

    /* Anything else is part of the body */
    if (append_char(self, ch) < 0) {
	self->state = lex_error;
	return -1;
    }

    self->state = lex_body;
    return 0;
}

/* Compressing whitespace in the body */
static int lex_body_ws(lexer_t self, int ch)
{
    /* Watch for linefeeds */
    if (ch == '\n') {
	self->state = lex_fold;
	return 0;
    }

    /* Discard other whitespace */
    if (isspace(ch)) {
	self->state = lex_body_ws;
	return 0;
    }

    /* Insert an actual space first */
    if (append_char(self, ' ') < 0) {
	return -1;
    }

    /* Anything else is part of the body */
    return lex_body(self, ch);
}

/* Try to fold */
static int lex_fold(lexer_t self, int ch)
{
    /* A linefeed means that we're out of headers */
    if (ch == '\n') {
	if (end_string_value(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	self->state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch)) {
	self->state = lex_body_ws;
	return 0;
    }

    if (end_string_value(self) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* A colon here is an error */
    if (ch == ':') {
	self->state = lex_error;
	return 0;
    }

    /* Anything else is the start of the next field */
    if (begin_name(self) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Make sure the first letter is always uppercase */
    if (append_char(self, toupper(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_name;
    return 0;
}

/* Discard the body */
static int lex_skip_body(lexer_t self, int ch)
{
    /* Try to fold on LF */
    if (ch == '\n') {
	self->state = lex_skip_fold;
	return 0;
    }

    /* Anything else is part of the body */
    self->state = lex_skip_body;
    return 0;
}

/* Try to fold */
static int lex_skip_fold(lexer_t self, int ch)
{
    /* A linefeed means the end of the headers */
    if (ch == '\n') {
	self->state = lex_end;
	return 0;
    }

    /* Other whitespace means a folded body */
    if (isspace(ch)) {
	self->state = lex_skip_body;
	return 0;
    }

    /* Anything else is the beginning of the next name */
    if (begin_name(self) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Make sure the first letter is always uppercase */
    if (append_char(self, toupper(ch)) < 0) {
	self->state = lex_error;
	return -1;
    }

    /* Watch for an initial dash */
    if (ch == '-') {
	self->state = lex_dash;
	return 0;
    }

    self->state = lex_name;
    return 0;
}

/* Skip everything after the headers */
static int lex_end(lexer_t self, int ch)
{
    self->state = lex_end;
    return 0;
}

/* Returns the size of the buffer */
size_t lexer_size(lexer_t self)
{
    return self->point - self->buffer;
}

/* Run the buffer through the lexer */
int lex(lexer_t self, char *buffer, ssize_t length)
{
    char *point;

    /* Watch for the end-of-input marker */
    if (length == 0) {
	return self->state(self, EOF);
    }

    for (point = buffer; point < buffer + length; point++) {
	if (self->state(self, *point) < 0) {
	    return -1;
	}
    }

    return 0;
}
