#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
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
#include <alloca.h>
#include <assert.h>
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif
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

typedef enum
{
    ENC_BASE64,
    ENC_QPRINT
} enc_t;

typedef enum
{
    CSET_OTHER,
    CSET_US_ASCII,
    CSET_UTF_8,
    CSET_ISO_8859_1
} charset_t;

/* Forward declarations for the state machine states */
static int lex_error(lexer_t self, int ch);
static int lex_start(lexer_t self, int ch);
static int lex_name(lexer_t self, int ch);
static int lex_dash(lexer_t self, int ch);
static int lex_ws(lexer_t self, int ch);
static int lex_body(lexer_t self, int ch);
static int lex_body_ws(lexer_t self, int ch);
static int lex_fold(lexer_t self, int ch);
static int lex_skip_body(lexer_t self, int ch);
static int lex_skip_fold(lexer_t self, int ch);
static int lex_end(lexer_t self, int ch);

static const char nibbles[] =
{
    /* 0x30 */  0,  1,  2,  3,  4,  5,  6,  7,   8,  9, -1, -1, -1, -1, -1, -1,
    /* 0x40 */ -1, 10, 11, 12, 13, 14, 15, -1,  -1, -1, -1, -1, -1, -1, -1, -1,
    /* 0x50 */ -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, -1, -1, -1, -1, -1,
    /* 0x60 */ -1, 10, 11, 12, 13, 14, 15, -1
};

static const char base64s[] =
{
    /* 0x20 */ -1, -1, -1, -1, -1, -1, -1, -1,  -1, -1, -1, 62, -1, -1, -1, 63,
    /* 0x30 */ 52, 53, 54, 55, 56, 57, 58, 59,  60, 61, -1, -1, -1, -1, -1, -1,
    /* 0x40 */ -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    /* 0x50 */ 15, 16, 17, 18, 19, 20, 21, 22,  23, 24, 25, -1, -1, -1, -1, -1,
    /* 0x60 */ -1, 26, 27, 28, 29, 30, 31, 32,  33, 34, 35, 36, 37, 38, 39, 40,
    /* 0x70 */ 41, 42, 43, 44, 45, 46, 47, 48,  49, 50, 51, -1, -1, -1, -1, -1,
};

/*
  especials = "(" / ")" / "<" / ">" / "@" / "," / ";" / ":" / "\" /
              <"> / "/" / "[" / "]" / "?" / "." / "="
*/
static const char especials[] =
{
    /* 0x20 */ 0, 0, 1, 0,  0, 0, 0, 0,   1, 1, 0, 0,  1, 0, 1, 1,
    /* 0x30 */ 0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 1, 1,  1, 1, 1, 1,
    /* 0x40 */ 1, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0,
    /* 0x50 */ 0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 1,  1, 1, 0, 0,
    /* 0x60 */ 0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0,
    /* 0x70 */ 0, 0, 0, 0,  0, 0, 0, 0,   0, 0, 0, 0,  0, 0, 0, 0
};

/* Returns the value corresponding to a hex digit, or -1 if the
 * character is not a hex digit */
static int
nibblevalue(int ch)
{
    return (0x30 <= ch && ch < 0x68) ? nibbles[ch - 0x30] : -1;
}

/* Returns the numeric value associated with a base64 character */
static int
base64value(int ch)
{
    return (0x20 <= ch && ch < 0x80) ? base64s[ch - 0x20] : -1;
}

/* Returns non-zero if a character is a valid RFC 1522 encoded word
 * token character */
static int
istoken(int ch)
{
    return 0x21 <= ch && ch < 0x7f && !especials[ch - 0x20];
}

/* Returns non-zero if a character is a valid RFC 1522 encoded-text
 * character */
static int
isenc(int ch)
{
    return 0x21 <= ch && ch < 0x7f && ch != '?';
}

/* Decode a base64-encoded string */
static ssize_t
base64_decode(const char *string, char *buffer, size_t buflen)
{
    const char *in = string;
    char *out = buffer;
    char *end = buffer + buflen;
    int val, num, ch, flag;

    /* Walk the input string in four-byte strides */
    flag = 0;
    while (out < end) {
	ch = *in++;

	/* End of input? */
	if (ch == 0) {
	    *out = ch;
	    return out - buffer;
	}

	/* Watch for premature '=' */
	if (flag) {
	    return -1;
	}

	if ((val = base64value(ch)) < 0) {
	    return -1;
	} else {
	    num = val << 18;
	}

	ch = *in++;
	if ((val = base64value(ch)) < 0) {
	    return -1;
	} else {
	    num |= val << 12;
	}

	ch = *in++;
	if (ch == '=') {
	    flag++;
	} else if (flag || (val = base64value(ch)) < 0) {
	    return -1;
	} else {
	    num |= val << 6;
	}

	ch = *in++;
	if (ch == '=') {
	    flag++;
	} else if (flag || (val = base64value(ch)) < 0) {
	    return -1;
	} else {
	    num |= val;
	}

	/* Record the result */
	*out++ = (num >> 16) & 0xff;
	if (out < end && flag < 2) {
	    *out++ = (num >> 8) & 0xff;
	}
	if (out < end && flag < 1) {
	    *out++ = num & 0xff;
	}
    }

    /* Out of buffer */
    printf("invalid base64 #5: %s\n", string);
    return -1;
}

/* Decode an RFC 1521 Quoted-Printable string */
static ssize_t
qprint_decode(const char *string, char *buffer, size_t buflen)
{
    const char *in = string;
    char *out = buffer;
    char *end = buffer + buflen;
    int ch, state, digit, num;

    state = 0;
    while (out < end) {
	ch = *in++;

	if (state == 0) {
	    switch (ch) {
	    case '\0':
		/* End of the string */
		*out = ch;
		return out - buffer;

	    case '_':
		/* Underscores become spaces */
		*out++ = ' ';
		break;

	    case '=':
		/* Hex-encoded character */
		num = 0;
		state++;
		break;

	    default:
		*out++ = ch;
		break;
	    }
	} else if (state == 1) {
	    if ((digit = nibblevalue(ch)) < 0) {
		return -1;
	    }
	    num = digit;
	    state++;
	} else {
	    assert(state == 2);
	    if ((digit = nibblevalue(ch)) < 0) {
		return -1;
	    }
	    ch = num << 4 | digit;
	    *out++ = ch;
	    state = 0;
	}
    }

    /* Out of buffer space: fail */
    return -1;
}

/* "Convert" a US-ASCII string to UTF-8 */
static ssize_t
ascii_to_utf8(const char *string, char *buffer, size_t buflen)
{
    const char *in = string;
    char *out = buffer;
    char *end = buffer + buflen;
    int ch;

    while (out < end) {
	/* Copy the character */
	ch = *(unsigned char *)in++;
	if (ch == 0) {
	    *out = ch;
	    return out - buffer;
	} else if (ch & 0x80) {
	    return -1;
	} else {
	    *out++ = ch;
	}
    }

    /* Out of buffer */
    return -1;
}

/* Convert an ISO-8859-1 string to UTF-8 */
static ssize_t
iso88591_to_utf8(const char *string, char *buffer, size_t buflen)
{
    const char *in = string;
    char *out = buffer;
    char *end = buffer + buflen;
    int ch;

    while (out < end) {
	ch = *(const unsigned char *)in++;

	if (ch == 0) {
	    *out = 0;
	    return out - buffer;
	} else if (ch < 0x80) {
	    *out++ = ch;
	} else {
	    *out++ = 0xc0 | ((ch >> 6) & 0x1f);
	    if (out < end) {
		*out++ = 0x80 | (ch & 0x3f);
	    }
	}
    }

    /* Buffer overflow */
    return -1;
}

/* "Convert" a UTF-8 string to UTF-8 */
static ssize_t
utf8_to_utf8(const char *string, char *buffer, size_t buflen)
{
    const char *in = string;
    char *out = buffer;
    char *end = buffer + buflen;
    int ch, state;

    /* Copy from source to dest, verifying that the characters are
     * valid UTF-8 */
    state = 0;
    while (out < end) {
	/* Copy a character */
	ch = *(unsigned char *)in++;
	*out++ = ch;

	/* Make sure it's legal */
	if (state == 0) {
	    if (ch == 0) {
		return out - buffer - 1;
	    } else if (~ch & 0x80) {
		state = 0;
	    } else if (~ch & 0x40) {
		return -1;
	    } else if (~ch & 0x20) {
		state = 1;
	    } else if (~ch & 0x10) {
		state = 2;
	    } else if (~ch & 0x08) {
		state = 3;
	    } else if (~ch & 0x04) {
		state = 4;
	    } else if (~ch & 0x02) {
		state = 5;
	    } else {
		return -1;
	    }
	} else if ((ch & 0xc0) != 0x80) {
	    return -1;
	} else {
	    state--;
	}
    }

    /* Out of buffer */
    return -1;
}

#if defined(HAVE_ICONV)
/* Convert a string to UTF-8 */
static ssize_t
other_to_utf8(iconv_t cd, const char *string, size_t slen, char *buffer, size_t buflen)
{
    size_t len;
    char *in = (char *)string;
    char *out = buffer;

    len = iconv(cd, &in, &slen, &out, &buflen);
    if (len == (size_t)-1 || slen != 0) {
	return -1;
    } else {
	return out - buffer;
    }
}
#endif /* HAVE_ICONV */


/* Attempt to decode an RFC 1522 encoded token */
static int
rfc1522_decode(const char *charset,
	       const char *encoding,
	       const char *text,
	       char *buffer, size_t buflen)
{
    char buf[256];
    enc_t enc;
    charset_t cset;
    ssize_t len;
    int i;
#if defined(HAVE_ICONV)
    iconv_t cd = (iconv_t)-1;
#endif /* HAVE_ICONV */

    /* Bail if we don't understand the encoding */
    if (strcasecmp(encoding, "B") == 0) {
	enc = ENC_BASE64;
    } else if (strcasecmp(encoding, "Q") == 0) {
	enc = ENC_QPRINT;
    } else {
	return -1;
    }

    /* Convert the charset name to uppercase */
    for (i = 0; i + 1 < sizeof(buf) && charset[i]; i++) {
	buf[i] = (0x61 <= charset[i] && charset[i] < 0x7b) ?
	    charset[i] - 0x20 : charset[i];
    }
    buf[i] = 0;

    /* See if we recognize the character set.  We'll handle UTF-8,
     * ISO-8859-1 and US-ASCII ourselves since they're trivial
     * conversions. */
    if (strcmp(buf, "ISO-8859-1") == 0) {
	cset = CSET_ISO_8859_1;
    } else if (strcmp(buf, "UTF-8") == 0) {
	cset = CSET_UTF_8;
    } else if (strcmp(buf, "US-ASCII") == 0) {
	cset = CSET_US_ASCII;
    } else {
#if defined(HAVE_ICONV)
	cset = CSET_OTHER;
	if ((cd = iconv_open("UTF-8", buf)) == (iconv_t)-1) {
	    return -1;
	}
#else /* !HAVE_ICONV */
	return -1;
#endif /* HAVE_ICONV */
    }

    /* We recognize the encoding and character set, so we should be
     * good to go.  Decode the string. */
    switch (enc) {
    case ENC_BASE64:
	len = base64_decode(text, buf, sizeof(buf));
	break;

    case ENC_QPRINT:
	len = qprint_decode(text, buf, sizeof(buf));
	break;
    }

    /* Bail out if the decoding failed */
    if (len < 0) {
#if defined(HAVE_ICONV)
	if (cd != (iconv_t)-1) {
	    iconv_close(cd);
	}
#endif /* HAVE_ICONV */
	return -1;
    }

    /* Translate the decoded string to UTF-8 (or verify that it's
     * US-ASCII or UTF-8) */
    switch (cset) {
    case CSET_ISO_8859_1:
	len = iso88591_to_utf8(buf, buffer, buflen);
	break;

    case CSET_UTF_8:
	len = utf8_to_utf8(buf, buffer, buflen);
	break;

    case CSET_US_ASCII:
	len = ascii_to_utf8(buf, buffer, buflen);
	break;

    case CSET_OTHER:
#if defined(HAVE_ICONV)
	assert(cset == CSET_OTHER);
	len = other_to_utf8(cd, buf, len, buffer, buflen);
	break;
#else /* !HAVE_ICONV */
	abort();
#endif /* HAVE_ICONV */
    }

#if defined(HAVE_ICONV)
    /* Close the conversion descriptor */
    if (cd != (iconv_t)-1) {
	(void)iconv_close(cd);
    }
#endif /* HAVE_ICONV */

    return len;
}

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


/* Determine whether or not we have a field with a given name */
static int
have_name(lexer_t self, const char *lstring)
{
    char *name;
    int length;

    /* Check if we've already seen this name */
    name = self->first_name_point;
    while (name < self->length_point) {
	/* Look for a match */
	if (lstring_eq(name, lstring)) {
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
    if (begin_string(self) < 0) {
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

    self->state = lex_name;
    return 0;
}

/* Reading a field name */
static int lex_name(lexer_t self, int ch)
{
    /* The first line of a the message may be an out-of-band
     * 'From' line which doesn't end with a ':'.  In addition, MH
     * will sometimes prepend additional header lines to a stored
     * message and we don't want to choke on those, so we let that
     * specific field slide. */
    if (ch == ' ') {
	/* Complete the name string */
	if (end_string(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Is it a 'From' line? */
	if (lstring_eq(self->length_point, from_string)) {
	    /* Convert the first character to lowercase */
	    self->length_point[4] = 'f';

	    /* Discard the field if we've seen it before */
	    if (have_name(self, self->length_point)) {
		self->point = self->length_point;
		self->state = lex_skip_body;
		return 0;
	    }

	    /* Otherwise read the field body */
	    self->state = lex_ws;
	    return 0;
	}
    }

    /* If we find whitespace in a header field name there's a good
     * chance that we're actualy in the message body, so just pretend
     * that we are. */
    if (isspace(ch)) {
	self->point = self->length_point;
	self->state = lex_end;
	return 0;
    }

    /* A colon is the end of the field name */
    if (ch == ':') {
	/* Finish the string */
	if (end_string(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Discard the field if we've seen it before */
	if (have_name(self, self->length_point)) {
	    self->point = self->length_point;
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
    /* If we find whitespace in a header field name there's a good
     * chance that we're actualy in the message body, so just pretend
     * that we are. */
    if (isspace(ch)) {
	self->point = self->length_point;
	self->state = lex_end;
	return 0;
    }

    /* A colon indicates the end of the field name */
    if (ch == ':') {
	/* Finish the string */
	if (end_string(self) < 0) {
	    self->state = lex_error;
	    return -1;
	}

	/* Discard the field if we've seen it before */
	if (have_name(self, self->length_point)) {
	    self->point = self->length_point;
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

    /* Reset the RFC 1522 word state */
    self->rfc1522_state = 0;
    return lex_body(self, ch);
}

/* Read the body */
static int lex_body(lexer_t self, int ch)
{
    int len;
    char buffer[256];

    /* Watch for RFC 1522-encoded words */
    switch (self->rfc1522_state) {
    case 0:
    case 10:
	self->rfc1522_state = (ch == '=') ? (self->rfc1522_state + 1) : 0;
	self->enc_word_point = self->point;
	break;

    case 1:
    case 11:
	self->rfc1522_state = (ch == '?') ? (self->rfc1522_state + 1) : (ch == '=') ? 1 : 0;
	break;

    case 2:
    case 12:
	self->rfc1522_state = istoken(ch) ? (self->rfc1522_state + 1) : (ch == '=') ? 1 : 0;
	break;

    case 3:
    case 13:
    case 5:
    case 15:
	if (ch == '?') {
	    self->rfc1522_state++;
	} else if (!istoken(ch)) {
	    self->rfc1522_state = 0;
	}
	break;

    case 4:
    case 14:
	self->enc_enc_point = self->point;
	self->rfc1522_state = istoken(ch) ? (self->rfc1522_state + 1) : (ch == '=') ? 1 : 0;
	break;

    case 6:
    case 16:
	self->enc_text_point = self->point;
	self->rfc1522_state = isenc(ch) ? (self->rfc1522_state + 1) : (ch == '=') ? 1 : 0;
	break;

    case 7:
    case 17:
	if (ch == '?') {
	    self->rfc1522_state++;
	} else if (!isenc(ch)) {
	    self->point[0] = 0;
	    self->rfc1522_state = (ch == '=') ? 1 : 0;
	}
	break;

    case 8:
    case 18:
	self->rfc1522_state = (ch == '=') ? (self->rfc1522_state + 1) : 0;
	break;

    case 9:
    case 19:
#ifdef DEBUG
	*self->point = 0;
	printf("encoded word: \"%s\"\n", self->enc_word_point);
#endif /* DEBUG */

	/* Null-terminate the component strings */
	assert(self->enc_text_point[-1] == '?');
	self->enc_text_point[-1] = 0;
	assert(self->enc_enc_point[-1] == '?');
	self->enc_enc_point[-1] = 0;
	assert(self->point[-2] == '?');
	self->point[-2] = 0;

	/* Decode the string */
	len = rfc1522_decode(self->enc_word_point + 2,
			     self->enc_enc_point,
			     self->enc_text_point,
			     buffer, sizeof(buffer));
	if (len < 0) {
	    /* Unable to decode; revert the string and carry on */
#ifdef DEBUG
	    printf("decoding failed\n");
#endif /* DEBUG */
	    self->enc_text_point[-1] = '?';
	    self->enc_enc_point[-1] = '?';
	    self->point[-2] = '?';
	    self->rfc1522_state = 0;
	} else {
	    /* Decoded; copy it into place.  If we're in state 19 then
	     * we need to overwrite the preceding space too. */
	    if (self->rfc1522_state == 19) {
		self->enc_word_point--;
	    }
	    memcpy(self->enc_word_point, buffer, len);
	    self->point = self->enc_word_point + len;
#ifdef DEBUG
	    *self->point = 0;
	    printf("decoded w%crd: \"%s\"\n", 
		   self->rfc1522_state == 19 ? '*' : 'o',
		   self->enc_word_point);
#endif /* DEBUG */
	    self->rfc1522_state = isspace(ch) ? 10 : 0;
	}
	break;

    default:
	abort();
    }

    /* Try to fold on LF */
    if (isspace(ch)) {
	self->state = (ch == '\n') ? lex_fold : lex_body_ws;
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

    /* Colons aren't allowed here, so if we see one we're probably
     * just in the message body. */
    if (ch == ':') {
	self->point = self->length_point;
	self->state = lex_end;
	return 0;
    }

    /* Anything else is the start of the next field */
    if (begin_string(self) < 0) {
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
    if (begin_string(self) < 0) {
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

    for (point = buffer; point < buffer + length && self->state != lex_end; point++) {
	if (self->state(self, *point) < 0) {
	    return -1;
	}
    }

    return 0;
}
