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

#ifndef PARSE_MAIL_H
#define PARSE_MAIL_H

typedef struct lexer *lexer_t;
typedef int (*lexer_state_t)(lexer_t self, int ch);

/* A bunch of state information for the lexer */
struct lexer {
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

    /* The RFC 1522-encoded word lexer state */
    int rfc1522_state;

    /* The start of the encoded word */
    char *enc_word_point;

    /* The start of the encoding field */
    char *enc_enc_point;

    /* The start of the encoded text */
    char *enc_text_point;

    /* Non-zero if the message is being posted to a tickertape group. */
    int send_to_tickertape;
};


/* Initializes a lexer_t */
void
lexer_init(lexer_t self, char *buffer, ssize_t length);


/* Writes a UNotify packet header */
int
lexer_append_unotify_header(lexer_t self,
                            const char *user,
                            const char *folder,
                            const char *group);


/* Run the buffer through the lexer */
int
lex(lexer_t self, char *buffer, ssize_t length);


/* Writes the UNotify packet footer */
int
lexer_append_unotify_footer(lexer_t self, int msg_num);


/* Returns the size of the buffer */
size_t
lexer_size(lexer_t self);


#endif /* PARSE_MAIL_H */
