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

/*
 * Description:
 *   The model (in the MVC sense) of an elvin notification
 *   with xtickertape
 */

#ifndef MESSAGE_H
#define MESSAGE_H

/* The message_t type */
typedef struct message *message_t;


/* Portions of the message which may be copied via copy and paste. */
typedef enum {
    MSGPART_ID,
    MSGPART_TEXT,
    MSGPART_ALL,
    MSGPART_LINK
} message_part_t;


/* Creates and returns a new message */
message_t
message_alloc(const char *info,
              const char *group,
              const char *user,
              const char *string,
              const unsigned int timeout,
              const char *attachment,
              size_t length,
              const char *tag,
              const char *id,
              const char *reply_id,
              const char *thread_id);


/* Allocates another reference to the message_t */
message_t
message_alloc_reference(message_t self);


/* Frees the memory used by the receiver */
void
message_free(message_t self);


/* Prints debugging information */
void
message_debug(message_t self);


/* Answers the Subscription info for the receiver's subscription */
const char *
message_get_info(message_t self);


/* Answers the receiver's creation time */
time_t *
message_get_creation_time(message_t self);


/* Answers the receiver's group */
const char *
message_get_group(message_t self);


/* Answers the receiver's user */
const char *
message_get_user(message_t self);


/* Answers the receiver's string */
const char *
message_get_string(message_t self);


/* Answers the receiver's timout in seconds */
unsigned long
message_get_timeout(message_t self);


/* Sets the receiver's timeout in seconds*/
void
message_set_timeout(message_t self, unsigned long timeout);


/* Answers non-zero if the receiver has a MIME attachment */
int
message_has_attachment(message_t self);


/* Answers the length of the attachment, and a pointer to its bytes */
size_t
message_get_attachment(message_t self, const char **attachment_out);


/* Decodes the attachment into a content type, character set and body */
int
message_decode_attachment(message_t self, char **type_out, char **body_out);


/* Answers the receiver's tag */
const char *
message_get_tag(message_t self);


/* Answers the receiver's id */
const char *
message_get_id(message_t self);


/* Answers the id of the message for which this is a reply */
const char *
message_get_reply_id(message_t self);


/* Answers the thread id of the message for which this is a reply */
const char *
message_get_thread_id(message_t self);


/* Answers non-zero if the mesage has been killed */
int
message_is_killed(message_t self);


/* Set the message's killed status */
void
message_set_killed(message_t self, int is_killed);


/* Returns how many bytes are required to hold the named message
 * part. */
size_t
message_part_size(message_t self, message_part_t part);


/* Writes the message part into the supplied buffer, returning a
 * pointer to that buffer or NULL if no such part exists. */
char *
message_get_part(message_t self, message_part_t part,
                 char *buffer, size_t buflen);


#endif /* MESSAGE_H */
