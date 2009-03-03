/* parse_mail.h */

#ifndef PARSE_MAIL_H
#define PARSE_MAIL_H

typedef struct lexer *lexer_t;
typedef int (*lexer_state_t)(lexer_t self, int ch);

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
void lexer_init(lexer_t self, char *buffer, ssize_t length);

/* Writes a UNotify packet header */
int lexer_append_unotify_header(lexer_t self, const char *user,
                                const char *folder, const char *group);

/* Run the buffer through the lexer */
int lex(lexer_t self, char *buffer, ssize_t length);

/* Writes the UNotify packet footer */
int lexer_append_unotify_footer(lexer_t self, int msg_num);

/* Returns the size of the buffer */
size_t lexer_size(lexer_t self);

#endif /* PARSE_MAIL_H */
