/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h> /* perror, printf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* free, malloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* memset, strdup */
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h> /* isspace */
#endif
#ifdef HAVE_TIME_H
# include <time.h> /* time */
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h> /* time */
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h> /* assert */
#endif
#include "replace.h"
#include "globals.h"
#include "utils.h"
#include "message.h"

/* The number of bytes required to hold a timestamp string, not
 * including the terminating NUL character. */
#define TIMESTAMP_SIZE (sizeof("YYYY-MM-DDTHH:MM:SS.uuuuuu+HHMM"))
#define TIMESTAMP_LEN (TIMESTAMP_SIZE - 1)

#ifdef DEBUG
static long message_count;
#endif /* DEBUG */

#define CONTENT_TYPE "Content-Type:"

struct message {
    /* The receiver's reference count */
    int ref_count;

    /* The time when the message was created */
    struct timeval creation_time;

    /* A string which identifies the subscription info in the control panel */
    const char *info;

    /* The receiver's group */
    const char *group;

    /* The receiver's user */
    const char *user;

    /* The receiver's string (tickertext) */
    const char *string;

    /* The lifetime of the receiver in seconds */
    unsigned long timeout;

    /* The message's replacement tag */
    const char *tag;

    /* The identifier for this message */
    const char *id;

    /* The identifier for the message for which this is a reply */
    const char *reply_id;

    /* The identifier for the thread for which this is a reply */
    const char *thread_id;

    /* Non-zero if the message has been killed */
    int is_killed;

    /* The receiver's MIME attachment */
    const char *attachment;

    /* The length of the receiver's MIME attachment */
    size_t length;

    /* The buffer in which the actual string data is kept. */
    char data[1];
};

typedef enum {
    ST_START = 0,
    ST_CR,
    ST_CRLF,
    ST_CRLFCR,
    ST_BODY
} state_t;

struct escape_seq {
    const char *seq;
    size_t len;
};

#define DECLARE_ESC(seq) \
    { seq, sizeof(seq) - 1 }

/* The string values for the escape sequences to use for ASCII
 * characters 0-31. */
static const struct escape_seq escapes[32] = {
    DECLARE_ESC("\\0"),
    DECLARE_ESC("\\001"),
    DECLARE_ESC("\\002"),
    DECLARE_ESC("\\003"),
    DECLARE_ESC("\\004"),
    DECLARE_ESC("\\005"),
    DECLARE_ESC("\\006"),
    DECLARE_ESC("\\a"),

    DECLARE_ESC("\\b"),
    DECLARE_ESC("\\t"),
    DECLARE_ESC("\\n"),
    DECLARE_ESC("\\v"),
    DECLARE_ESC("\\f"),
    DECLARE_ESC("\\r"),
    DECLARE_ESC("\\016"),
    DECLARE_ESC("\\017"),

    DECLARE_ESC("\\020"),
    DECLARE_ESC("\\021"),
    DECLARE_ESC("\\022"),
    DECLARE_ESC("\\023"),
    DECLARE_ESC("\\024"),
    DECLARE_ESC("\\025"),
    DECLARE_ESC("\\026"),
    DECLARE_ESC("\\027"),

    DECLARE_ESC("\\030"),
    DECLARE_ESC("\\031"),
    DECLARE_ESC("\\032"),
    DECLARE_ESC("\\033"),
    DECLARE_ESC("\\034"),
    DECLARE_ESC("\\035"),
    DECLARE_ESC("\\036"),
    DECLARE_ESC("\\037")
};

/* Copies a MIME attachment, replacing \r or \r\n in the header with \n */
static void
cleanse_header(const char *attachment,
               size_t length,
               char *copy,
               size_t *length_out)
{
    const char *end = attachment + length;
    const char *in = attachment;
    char *out = copy;
    state_t state;
    int ch;

    /* Go through one character at a time */
    state = ST_START;
    for (in = attachment; in < end; in++) {
        ch = *in;

        switch (state) {
        case ST_START:
            if (ch == '\r') {
                *out++ = '\n';
                state = ST_CR;
            } else if (ch == '\n') {
                *out++ = ch;
                state = ST_CRLF;
            } else {
                *out++ = ch;
            }

            break;

        case ST_CR:
            if (ch == '\r') {
                *out++ = '\n';
                state = ST_BODY;
            } else if (ch == '\n') {
                state = ST_CRLF;
            } else {
                *out++ = ch;
                state = ST_START;
            }

            break;

        case ST_CRLF:
            if (ch == '\r') {
                *out++ = '\n';
                state = ST_CRLFCR;
            } else if (ch == '\n') {
                *out++ = ch;
                state = ST_BODY;
            } else {
                *out++ = ch;
                state = ST_START;
            }

            break;

        case ST_CRLFCR:
            if (ch == '\n') {
                state = ST_BODY;
            } else {
                *out++ = ch;
                state = ST_BODY;
            }

            break;

        case ST_BODY:
            *out++ = ch;
            break;

        default:
            abort();
        }
    }

    *length_out = out - copy;
}

/* Return the number of bytes required to hold the cooked version of a
 * string.  See cook_string, below. */
static size_t
cooked_size(const char *string)
{
    const char *in;
    size_t len;
    int ch;

    /* Count one for the initial double-quote. */
    len = 1;

    /* Visit each character in the string. */
    for (in = string; (ch = *(unsigned char *)in) != '\0'; in++) {
        ASSERT(ch >= 0);
        if (ch < 32) {
            /* Convert control characters to escape sequences. */
            len += escapes[ch].len;
        } else if (ch == '"' || ch == '\\') {
            /* Also escape backslashes and double-quotes. */
            len += 2;
        } else {
            /* Anything else will be copied verbatim. */
            len++;
        }
    }

    /* Count one more for the trailing double-quote. */
    return len + 1;
}

/* Write string into buffer, wrapping it in double-quotes and escaping
 * any non-printing ASCII characters.  See also cooked_size. */
static void
cook_string(const char *string, char **buffer)
{
    const char *in;
    char *out = *buffer;
    int ch;

    /* Start with a double-quote character. */
    *out++ = '"';

    /* Visit each character in the string. */
    for (in = string; (ch = *(unsigned char *)in) != '\0'; in++) {
        /* We've just converted from an unsigned char. */
        ASSERT(ch >= 0);

        if (ch < 32) {
            /* Convert control characters to escape sequences. */
            memcpy(out, escapes[ch].seq, escapes[ch].len);
            out += escapes[ch].len;
        } else if (ch == '"' || ch == '\\') {
            /* Also escape backslahes and quotes */
            *out++ = '\\';
            *out++ = ch;
        } else {
            /* Copy anything else verbatim. */
            /* FIXME: this should deal with other non-printing Unicode
             * code points. */
            *out++ = ch;
        }
    }

    /* End with a double-quote character. */
    *out++ = '"';

    /* Update the pointer. */
    *buffer = out;
}

static char *
append_data(char **point, const char *string, size_t size)
{
    char *result = *point;

    if (string == NULL) {
        return NULL;
    }

    memcpy(result, string, size);
    *point = result + size;
    return result;
}

/* Creates and returns a new message */
message_t
message_alloc(const char *info,
              const char *group,
              const char *user,
              const char *string,
              unsigned int timeout,
              const char *attachment,
              size_t length,
              const char *tag,
              const char *id,
              const char *reply_id,
              const char *thread_id)
{
    message_t self;
    size_t info_size, group_size, user_size, string_size;
    size_t tag_size, id_size, reply_size, thread_size, len;
    char *point;

    /* Make sure the mandatory fields have values. */
    ASSERT(group != NULL);
    ASSERT(user != NULL);
    ASSERT(string != NULL);

    /* Measure each of the strings, including the NUL terminator. */
    info_size = (info == NULL) ? 0 : strlen(info) + 1;
    group_size = strlen(group) + 1;
    user_size = strlen(user) + 1;
    string_size = strlen(string) + 1;
    tag_size = (tag == NULL) ? 0 : strlen(tag) + 1;
    id_size = (id == NULL) ? 0 : strlen(id) + 1;
    reply_size = (reply_id == NULL) ? 0 : strlen(reply_id) + 1;
    thread_size = (thread_id == NULL) ? 0 : strlen(thread_id) + 1;

    /* Compute the total number of bytes needed to hold all of the
     * string data. */
    len = info_size + group_size + user_size + string_size + tag_size +
        id_size + reply_size + thread_size + length;

    /* Allocate space for the message_t, including space for all of
     * the strings. */
    self = malloc(sizeof(struct message) + len - 1);
    if (self == NULL) {
        return NULL;
    }

    /* Record the time the message was created. */
    if (gettimeofday(&self->creation_time, NULL) < 0) {
        perror("gettimeofday failed");
    }

    /* Copy each of the strings info the data array. */
    point = self->data;
    self->ref_count = 0;
    self->info = append_data(&point, info, info_size);
    self->group = append_data(&point, group, group_size);
    self->user = append_data(&point, user, user_size);
    self->string = append_data(&point, string, string_size);
    self->timeout = timeout;
    self->tag = append_data(&point, tag, tag_size);
    self->id = append_data(&point, id, id_size);
    self->reply_id = append_data(&point, reply_id, reply_size);
    self->thread_id = append_data(&point, thread_id, thread_size);
    self->is_killed = 0;

    /* Check our addition. */
    ASSERT(point + length == self->data + len);

    if (length == 0) {
        self->attachment = NULL;
        self->length = 0;
    } else {
        /* Clean up the attachment's linefeeds for metamail. */
        cleanse_header(attachment, length, point, &self->length);
        self->attachment = point;
        point += self->length;
    }

    /* Check our addition again. */
    ASSERT(point <= self->data + len);

#ifdef DEBUG
    printf("allocated message_t %p (%ld)\n", self, ++message_count);
    message_debug(self);
#endif /* DEBUG */

    /* Allocate a reference to the caller. */
    self->ref_count++;
    return self;
}

/* Allocates another reference to the message_t */
message_t
message_alloc_reference(message_t self)
{
    self->ref_count++;
    return self;
}

/* Frees the memory used by the receiver */
void
message_free(message_t self)
{
    /* Decrement the reference count */
    if (--self->ref_count > 0) {
        return;
    }

#ifdef DEBUG
    printf("freeing message_t %p (%ld):\n", self, --message_count);
    message_debug(self);
#endif /* DEBUG */

    free(self);
}

/* Answers the Subscription matched to generate the receiver */
const char *
message_get_info(message_t self)
{
    return self->info;
}

/* Answers the receiver's creation time */
time_t *
message_get_creation_time(message_t self)
{
    return &self->creation_time.tv_sec;
}

/* Answers the receiver's group */
const char *
message_get_group(message_t self)
{
    return self->group;
}

/* Answers the receiver's user */
const char *
message_get_user(message_t self)
{
    return self->user;
}

/* Answers the receiver's string */
const char *
message_get_string(message_t self)
{
    return self->string;
}

/* Answers the receiver's timout */
unsigned long
message_get_timeout(message_t self)
{
    return self->timeout;
}

/* Sets the receiver's timeout */
void
message_set_timeout(message_t self, unsigned long timeout)
{
    self->timeout = timeout;
}

/* Answers non-zero if the receiver has a MIME attachment */
int
message_has_attachment(message_t self)
{
    return self->attachment != NULL;
}

/* Answers the receiver's MIME arguments */
size_t
message_get_attachment(message_t self, const char **attachment_out)
{
    *attachment_out = self->attachment;
    return self->length;
}

/* Decodes the attachment into a content type, character set and body */
int
message_decode_attachment(message_t self, char **type_out, char **body_out)
{
    const char *point = self->attachment;
    const char *mark = point;
    const char *end = point + self->length;
    size_t ct_length = sizeof(CONTENT_TYPE) - 1;

    *type_out = NULL;
    *body_out = NULL;

    /* If no attachment then return lots of NULLs */
    if (point == NULL) {
        return 0;
    }

    /* Look through the attachment's MIME header */
    while (point < end) {
        /* End of line? */
        if (*point == '\n') {
            /* Is this the Content-Type line? */
            if (mark + ct_length < point &&
                strncasecmp(mark, CONTENT_TYPE, ct_length) == 0) {
                const char *p;

                /* Skip to the beginning of the value */
                mark += ct_length;
                while (mark < point && isspace(*(unsigned char *)mark)) {
                    mark++;
                }

                /* Look for a ';' in the content type */
                p = memchr(mark, ';', point - mark);
                if (p == NULL) {
                    p = point;
                }

                /* If multiple content-types are given then use the last */
                if (*type_out != NULL) {
                    free(*type_out);
                }

                /* Allocate a string to hold the content type */
                *type_out = malloc(p - mark + 1);
                if (*type_out == NULL) {
                    return -1;
                }
                memcpy(*type_out, mark, p - mark);
                (*type_out)[p - mark] = 0;
            } else if (point - mark == 0) {
                point++;

                /* Trim any CRs and LFs from the end of the body */
                while (point < end - 1 &&
                       (*(end - 1) == '\r' || *(end - 1) == '\n')) {
                    end--;
                }

                /* Allocate space for a copy of the body and
                 * null-terminate it */
                *body_out = malloc(end - point + 1);
                memcpy(*body_out, point, end - point);
                (*body_out)[end - point] = 0;
                return 0;
            }

            mark = point + 1;
        }

        point++;
    }

    /* No body found */
    return 0;
}

/* Answers the receiver's tag */
const char *
message_get_tag(message_t self)
{
    return self->tag;
}

/* Answers the receiver's id */
const char *
message_get_id(message_t self)
{
    return self->id;
}

/* Answers the id of the message_t for which this is a reply */
const char *
message_get_reply_id(message_t self)
{
    return self->reply_id;
}

/* Answers the thread id of the message for which this is a reply */
const char *
message_get_thread_id(message_t self)
{
    return self->thread_id;
}

/* Answers non-zero if the mesage has been killed */
int
message_is_killed(message_t self)
{
    return self->is_killed;
}

/* Set the message's killed status */
void
message_set_killed(message_t self, int is_killed)
{
    self->is_killed = is_killed;
}

static size_t
measure_string_field(const char *name, const char *value)
{
    return (value == NULL) ? 0 : strlen(name) + 2 + cooked_size(value) + 1;
}

static void
write_string_field(const char *name, const char *value, char **buffer)
{
    char *out = *buffer;
    size_t len;

    /* Skip this entry if it has no value. */
    if (value == NULL) {
        return;
    }

    /* Append the name, followed by a colon and a space. */
    len = strlen(name);
    memcpy(out, name, len);
    out += len;
    *out++ = ':';
    *out++ = ' ';

    /* Append the cooked string. */
    cook_string(value, &out);
    *out++ = '\n';

    /* Update the pointer. */
    *buffer = out;
}

/* Writes a timestamp string into buffer and returns a pointer to it.
 * The resulting timestamp should be exactly TIMESTAMP_LEN bytes, not
 * including the NUL-terminator. */
static char *
write_timestamp(message_t self, char *buffer, size_t buflen)
{
    const struct tm *tm;
    int utc_off;
    size_t len;
    char sign;

    /* Convert the timestamp to local time and determine the offset of
     * local time from UTC at that point in time. */
    tm = localtime_offset(&self->creation_time.tv_sec, &utc_off);
    if (utc_off < 0) {
        sign = '-';
        utc_off = -utc_off;
    } else {
        sign = '+';
    }

    /* Write the timestamp into the buffer. */
    len = snprintf(buffer, buflen,
                   "%04d-%02d-%02dT%02d:%02d:%02d.%06ld%c%02d%02d",
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min, tm->tm_sec,
                   (long)self->creation_time.tv_usec,
                   sign, utc_off / 3600, utc_off / 60 % 60);

    ASSERT(len == MAX(buflen, TIMESTAMP_LEN));
    return buffer;
}

static char *
write_message(message_t self, char *buffer, size_t buflen)
{
    char timebuf[TIMESTAMP_SIZE];
    char *out;
    size_t len;

    len = snprintf(buffer, buflen, "$time %s\n",
                   write_timestamp(self, timebuf, sizeof(timebuf)));
    out = buffer + len;

    /* Append each of the strings. */
    write_string_field("Group", self->group, &out);
    write_string_field("From", self->user, &out);
    write_string_field("Message", self->string, &out);
    write_string_field("Message-Id", self->id, &out);
    write_string_field("In-Reply-To", self->reply_id, &out);
    write_string_field("Thread-Id", self->thread_id, &out);
    write_string_field("Attachment", self->attachment, &out);

    ASSERT(out - buffer == buflen);
    return buffer;
}

message_part_t
message_part_from_string(const char *string)
{
    if (string == NULL) {
        return MSGPART_TEXT;
    } else if (strcmp(string, "id") == 0) {
        return MSGPART_ID;
    } else if (strcmp(string, "text") == 0) {
        return MSGPART_TEXT;
    } else if (strcmp(string, "all") == 0) {
        return MSGPART_ALL;
    } else if (strcmp(string, "link") == 0) {
        return MSGPART_LINK;
    } else {
        return MSGPART_TEXT;
    }
}

const char *
message_part_to_string(message_part_t part)
{
    switch (part) {
    case MSGPART_ID:
        return "id";
    case MSGPART_TEXT:
        return "text";
    case MSGPART_ALL:
        return "all";
    case MSGPART_LINK:
        return "link";
    default:
        return NULL;
    }
}

size_t
message_part_size(message_t self, message_part_t part)
{
    switch (part) {
    case MSGPART_NONE:
        return 0;

    case MSGPART_ID:
        return (self->id == NULL) ? 0 : strlen(self->id);

    case MSGPART_TEXT:
        return strlen(self->string);

    case MSGPART_LINK:
        return self->length;
        
    case MSGPART_ALL:
        return sizeof("$time YYYY-MM-DDTHH:MM:SS.uuuuuu+HHMM\n") - 1 +
            measure_string_field("Group", self->group) +
            measure_string_field("From", self->user) +
            measure_string_field("Message", self->string) +
            measure_string_field("Message-Id", self->id) +
            measure_string_field("In-Reply-To", self->reply_id) +
            measure_string_field("Thread-Id", self->thread_id) +
            measure_string_field("Attachment", self->attachment);
    }

    return 0;
}

char *
message_get_part(message_t self, message_part_t part,
                 char *buffer, size_t buflen)
{
    switch (part) {
    case MSGPART_NONE:
        return NULL;

    case MSGPART_ID:
        if (self->id == NULL) {
            return NULL;
        }
        strncpy(buffer, self->id, buflen);
        return buffer;

    case MSGPART_TEXT:
        ASSERT(self->string != NULL);
        strncpy(buffer, self->string, buflen);
        return buffer;

    case MSGPART_LINK:
        if (self->attachment == NULL) {
            return NULL;
        }
        memcpy(buffer, self->attachment, MIN(buflen, self->length));
        return buffer;

    case MSGPART_ALL:
        return write_message(self, buffer, buflen);
    }

    return NULL;
}

#ifdef DEBUG
/* Prints debugging information */
void
message_debug(message_t self)
{
    char timebuf[TIMESTAMP_SIZE];

    printf("message_t (%p)\n", self);
    printf("  ref_count=%d\n", self->ref_count);
    printf("  creation_time=%s\n",
           write_timestamp(self, timebuf, sizeof(timebuf)));
    printf("  info=\"%s\"\n", (self->info == NULL) ? "<null>" : self->info);
    printf("  group=\"%s\"\n", self->group);
    printf("  user=\"%s\"\n", self->user);
    printf("  string=\"%s\"\n", self->string);
    printf("  attachment=%p [%zu]\n", self->attachment, self->length);
    printf("  timeout=%ld\n", self->timeout);
    printf("  tag=\"%s\"\n", (self->tag == NULL) ? "<null>" : self->tag);
    printf("  id=\"%s\"\n", (self->id == NULL) ? "<null>" : self->id);
    printf("  reply_id=\"%s\"\n",
           (self->reply_id == NULL) ? "<null>" : self->reply_id);
    printf("  thread_id=\"%s\"\n",
           (self->thread_id == NULL) ? "<null>" : self->thread_id);
}
#endif /* DEBUG */

/**********************************************************************/
