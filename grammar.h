#define IS_ERROR(action) ((action) == 0)
#define IS_ACCEPT(action) ((action) == 96)
#define IS_REDUCE(action) (0 < (action) && (action) < 34)
#define IS_SHIFT(action) (34 <= (action) && (action) < 96)
#define REDUCTION(action) (action)
#define REDUCE_GOTO(state, production) \
    (goto_table[state][production -> nonterm_type])
#define SHIFT_GOTO(action) ((action) - 34)

typedef enum
{
    TT_EOF = 0,
    TT_LBRACE,
    TT_RBRACE,
    TT_SEMI,
    TT_ID,
    TT_COLON,
    TT_ASSIGN,
    TT_OR,
    TT_AND,
    TT_EQ,
    TT_NEQ,
    TT_LT,
    TT_LE,
    TT_GT,
    TT_GE,
    TT_BANG,
    TT_COMMA,
    TT_STRING,
    TT_INT32,
    TT_INT64,
    TT_FLOAT,
    TT_LPAREN,
    TT_RPAREN,
    TT_LBRACKET,
    TT_RBRACKET
} terminal_t;

struct production
{
    reduction_t reduction;
    int nonterm_type;
    int count;
};

static struct production productions[34] =
{
    /* 0: <config-file> ::= <subscription-list> */
    { identity, 0, 1 },

    /* 1: <subscription-list> ::= <subscription-list> <subscription> */
    { append, 1, 2 },

    /* 2: <subscription-list> ::= <subscription> */
    { identity, 1, 1 },

    /* 3: <subscription> ::= <tag> LBRACE <statements> RBRACE SEMI */
    { make_sub, 2, 5 },

    /* 4: <subscription> ::= <tag> LBRACE RBRACE SEMI */
    { make_default_sub, 2, 4 },

    /* 5: <tag> ::= ID COLON */
    { identity, 3, 2 },

    /* 6: <statements> ::= <statements> <statement> */
    { append, 4, 2 },

    /* 7: <statements> ::= <statement> */
    { identity, 4, 1 },

    /* 8: <statement> ::= ID ASSIGN <disjunction> SEMI */
    { make_assignment, 5, 4 },

    /* 9: <disjunction> ::= <disjunction> OR <conjunction> */
    { extend_disjunction, 6, 3 },

    /* 10: <disjunction> ::= <conjunction> */
    { identity, 6, 1 },

    /* 11: <conjunction> ::= <conjunction> AND <term> */
    { extend_conjunction, 7, 3 },

    /* 12: <conjunction> ::= <term> */
    { identity, 7, 1 },

    /* 13: <term> ::= <term> EQ <value> */
    { make_eq, 8, 3 },

    /* 14: <term> ::= <term> NEQ <value> */
    { make_neq, 8, 3 },

    /* 15: <term> ::= <term> LT <value> */
    { make_lt, 8, 3 },

    /* 16: <term> ::= <term> LE <value> */
    { make_le, 8, 3 },

    /* 17: <term> ::= <term> GT <value> */
    { make_gt, 8, 3 },

    /* 18: <term> ::= <term> GE <value> */
    { make_ge, 8, 3 },

    /* 19: <term> ::= <value> */
    { identity, 8, 1 },

    /* 20: <value> ::= BANG <value> */
    { make_not, 9, 2 },

    /* 21: <values> ::= <values> COMMA <disjunction> */
    { extend_values, 10, 3 },

    /* 22: <values> ::= <disjunction> */
    { identity, 10, 1 },

    /* 23: <value> ::= ID */
    { identity, 9, 1 },

    /* 24: <value> ::= STRING */
    { identity, 9, 1 },

    /* 25: <value> ::= INT32 */
    { identity, 9, 1 },

    /* 26: <value> ::= INT64 */
    { identity, 9, 1 },

    /* 27: <value> ::= FLOAT */
    { identity, 9, 1 },

    /* 28: <value> ::= LBRACE <values> RBRACE */
    { make_list, 9, 3 },

    /* 29: <value> ::= LBRACE RBRACE */
    { make_empty_list, 9, 2 },

    /* 30: <value> ::= ID LPAREN <values> RPAREN */
    { make_function, 9, 4 },

    /* 31: <value> ::= ID LPAREN RPAREN */
    { make_noarg_function, 9, 3 },

    /* 32: <value> ::= LPAREN <disjunction> RPAREN */
    { identity2, 9, 3 },

    /* 33: <value> ::= LBRACKET <disjunction> RBRACKET */
    { make_block, 9, 3 }
};

#define ERR 0
#define ACC 96
#define R(x) (x)
#define S(x) (x + 34)

static unsigned int sr_table[62][25] =
{
    { ERR, ERR, ERR, ERR, S(4), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ACC, ERR, ERR, ERR, S(4), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { R(2), ERR, ERR, ERR, R(2), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, S(6), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, ERR, ERR, ERR, S(7), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { R(1), ERR, ERR, ERR, R(1), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, S(10), ERR, S(11), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, R(5), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, S(13), ERR, S(11), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, R(7), ERR, R(7), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, ERR, S(14), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, ERR, ERR, ERR, ERR, S(15), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, R(6), ERR, R(6), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, ERR, S(16), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { R(4), ERR, ERR, ERR, R(4), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { R(3), ERR, ERR, ERR, R(3), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, ERR, S(30), ERR, ERR, ERR, S(31), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, R(10), R(10), ERR, ERR, ERR, R(10), S(32), ERR, ERR, ERR, ERR, ERR, ERR, ERR, R(10), ERR, ERR, ERR, ERR, ERR, R(10), ERR, R(10) },
    { ERR, ERR, R(12), R(12), ERR, ERR, ERR, R(12), R(12), S(33), S(34), S(35), S(36), S(37), S(38), ERR, R(12), ERR, ERR, ERR, ERR, ERR, R(12), ERR, R(12) },
    { ERR, ERR, R(19), R(19), ERR, ERR, ERR, R(19), R(19), R(19), R(19), R(19), R(19), R(19), R(19), ERR, R(19), ERR, ERR, ERR, ERR, ERR, R(19), ERR, R(19) },
    { ERR, S(21), S(41), ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, ERR, R(23), R(23), ERR, ERR, ERR, R(23), R(23), R(23), R(23), R(23), R(23), R(23), R(23), ERR, R(23), ERR, ERR, ERR, ERR, S(42), R(23), ERR, R(23) },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, ERR, R(24), R(24), ERR, ERR, ERR, R(24), R(24), R(24), R(24), R(24), R(24), R(24), R(24), ERR, R(24), ERR, ERR, ERR, ERR, ERR, R(24), ERR, R(24) },
    { ERR, ERR, R(25), R(25), ERR, ERR, ERR, R(25), R(25), R(25), R(25), R(25), R(25), R(25), R(25), ERR, R(25), ERR, ERR, ERR, ERR, ERR, R(25), ERR, R(25) },
    { ERR, ERR, R(26), R(26), ERR, ERR, ERR, R(26), R(26), R(26), R(26), R(26), R(26), R(26), R(26), ERR, R(26), ERR, ERR, ERR, ERR, ERR, R(26), ERR, R(26) },
    { ERR, ERR, R(27), R(27), ERR, ERR, ERR, R(27), R(27), R(27), R(27), R(27), R(27), R(27), R(27), ERR, R(27), ERR, ERR, ERR, ERR, ERR, R(27), ERR, R(27) },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, ERR, R(8), ERR, R(8), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, ERR, R(22), ERR, ERR, ERR, ERR, S(31), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, R(22), ERR, ERR, ERR, ERR, ERR, R(22), ERR, ERR },
    { ERR, ERR, S(54), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(55), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR },
    { ERR, ERR, R(29), R(29), ERR, ERR, ERR, R(29), R(29), R(29), R(29), R(29), R(29), R(29), R(29), ERR, R(29), ERR, ERR, ERR, ERR, ERR, R(29), ERR, R(29) },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), S(57), S(29), ERR },
    { ERR, ERR, R(20), R(20), ERR, ERR, ERR, R(20), R(20), R(20), R(20), R(20), R(20), R(20), R(20), ERR, R(20), ERR, ERR, ERR, ERR, ERR, R(20), ERR, R(20) },
    { ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(31), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(58), ERR, ERR },
    { ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(31), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(59) },
    { ERR, ERR, R(9), R(9), ERR, ERR, ERR, R(9), S(32), ERR, ERR, ERR, ERR, ERR, ERR, ERR, R(9), ERR, ERR, ERR, ERR, ERR, R(9), ERR, R(9) },
    { ERR, ERR, R(11), R(11), ERR, ERR, ERR, R(11), R(11), S(33), S(34), S(35), S(36), S(37), S(38), ERR, R(11), ERR, ERR, ERR, ERR, ERR, R(11), ERR, R(11) },
    { ERR, ERR, R(13), R(13), ERR, ERR, ERR, R(13), R(13), R(13), R(13), R(13), R(13), R(13), R(13), ERR, R(13), ERR, ERR, ERR, ERR, ERR, R(13), ERR, R(13) },
    { ERR, ERR, R(14), R(14), ERR, ERR, ERR, R(14), R(14), R(14), R(14), R(14), R(14), R(14), R(14), ERR, R(14), ERR, ERR, ERR, ERR, ERR, R(14), ERR, R(14) },
    { ERR, ERR, R(15), R(15), ERR, ERR, ERR, R(15), R(15), R(15), R(15), R(15), R(15), R(15), R(15), ERR, R(15), ERR, ERR, ERR, ERR, ERR, R(15), ERR, R(15) },
    { ERR, ERR, R(16), R(16), ERR, ERR, ERR, R(16), R(16), R(16), R(16), R(16), R(16), R(16), R(16), ERR, R(16), ERR, ERR, ERR, ERR, ERR, R(16), ERR, R(16) },
    { ERR, ERR, R(17), R(17), ERR, ERR, ERR, R(17), R(17), R(17), R(17), R(17), R(17), R(17), R(17), ERR, R(17), ERR, ERR, ERR, ERR, ERR, R(17), ERR, R(17) },
    { ERR, ERR, R(18), R(18), ERR, ERR, ERR, R(18), R(18), R(18), R(18), R(18), R(18), R(18), R(18), ERR, R(18), ERR, ERR, ERR, ERR, ERR, R(18), ERR, R(18) },
    { ERR, ERR, R(28), R(28), ERR, ERR, ERR, R(28), R(28), R(28), R(28), R(28), R(28), R(28), R(28), ERR, R(28), ERR, ERR, ERR, ERR, ERR, R(28), ERR, R(28) },
    { ERR, S(21), ERR, ERR, S(22), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(23), ERR, S(24), S(25), S(26), S(27), S(28), ERR, S(29), ERR },
    { ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, S(55), ERR, ERR, ERR, ERR, ERR, S(61), ERR, ERR },
    { ERR, ERR, R(31), R(31), ERR, ERR, ERR, R(31), R(31), R(31), R(31), R(31), R(31), R(31), R(31), ERR, R(31), ERR, ERR, ERR, ERR, ERR, R(31), ERR, R(31) },
    { ERR, ERR, R(32), R(32), ERR, ERR, ERR, R(32), R(32), R(32), R(32), R(32), R(32), R(32), R(32), ERR, R(32), ERR, ERR, ERR, ERR, ERR, R(32), ERR, R(32) },
    { ERR, ERR, R(33), R(33), ERR, ERR, ERR, R(33), R(33), R(33), R(33), R(33), R(33), R(33), R(33), ERR, R(33), ERR, ERR, ERR, ERR, ERR, R(33), ERR, R(33) },
    { ERR, ERR, R(21), ERR, ERR, ERR, ERR, S(31), ERR, ERR, ERR, ERR, ERR, ERR, ERR, ERR, R(21), ERR, ERR, ERR, ERR, ERR, R(21), ERR, ERR },
    { ERR, ERR, R(30), R(30), ERR, ERR, ERR, R(30), R(30), R(30), R(30), R(30), R(30), R(30), R(30), ERR, R(30), ERR, ERR, ERR, ERR, ERR, R(30), ERR, R(30) }
};

#undef ERR
#undef R
#undef S

static unsigned int goto_table[62][11] =
{
    { 0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 5, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 8, 9, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 17, 18, 19, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 39, 18, 19, 20, 40 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 43, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 44, 18, 19, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 45, 18, 19, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 46, 19, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 47, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 48, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 49, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 51, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 52, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 53, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 39, 18, 19, 20, 56 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 60, 18, 19, 20, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

