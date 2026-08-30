#ifndef _PY_LEXER_H_
#define _PY_LEXER_H_

#include "object.h"
#include "../tokenizer/source.h"

#define MAXINDENT 100       /* Max indentation level */
#define MAXLEVEL 200        /* Max parentheses level */
#define MAXFSTRINGLEVEL 150 /* Max f-string nesting level */

#define INSIDE_FSTRING(tok) (tok->tok_mode_stack_index > 0)
#define INSIDE_FSTRING_EXPR(tok) (tok->curly_bracket_expr_start_depth >= 0)
#define INSIDE_FSTRING_EXPR_AT_TOP(tok) \
    (tok->curly_bracket_depth - tok->curly_bracket_expr_start_depth == 1)

enum interactive_underflow_t {
    /* Normal mode of operation: return a new token when asked in interactive mode */
    IUNDERFLOW_NORMAL,
    /* Forcefully return ENDMARKER when asked for a new token in interactive mode. This
     * can be used to prevent the tokenizer to prompt the user for new tokens */
    IUNDERFLOW_STOP,
};

struct token {
    int level;
    _PyTok_Span span;
    _PyTok_Loc start_loc;
    _PyTok_Loc end_loc;
    PyObject *metadata;
};

enum tokenizer_mode_kind_t {
    TOK_REGULAR_MODE,
    TOK_FSTRING_MODE,
};

enum string_kind_t {
    FSTRING,
    TSTRING,
};

#define MAX_EXPR_NESTING 3

typedef struct _tokenizer_comments {
    Py_ssize_t count;
    Py_ssize_t capacity;
    _PyTok_Span spans[];
} tokenizer_comments;

typedef struct _tokenizer_mode {
    enum tokenizer_mode_kind_t kind;

    int curly_bracket_depth;
    int curly_bracket_expr_start_depth;

    char quote;
    int quote_size;
    int raw;
    _PyTok_Off start;
    _PyTok_Off multi_line_start;
    int first_line;

    _PyTok_Span expr_span;
    int in_debug;
    int in_format_spec;

    enum string_kind_t string_kind;
    tokenizer_comments *comments;
} tokenizer_mode;

/* Tokenizer state */
struct tok_state {
    /* Input state; buf <= cur <= inp */
    /* NB an entire line is held in the buffer */
    char *buf;          /* Owned for file/readline input; source-backed otherwise. */
    char *cur;          /* Next character in buffer */
    char *inp;          /* End of data in buffer */
    _PyTok_Off buf_offset; /* Logical offset of buf[0]. */
    int fp_interactive; /* If the file descriptor is interactive */
    char *interactive_src_start; /* The start of the source parsed so far in interactive mode */
    char *interactive_src_end; /* The end of the source parsed so far in interactive mode */
    const char *start;  /* Start of current token if not NULL */
    int done;           /* E_OK normally, E_EOF at EOF, otherwise error code */
    /* NB If done != E_OK, cur must be == inp!!! */
    FILE *fp;           /* Rest of input; NULL if tokenizing a string */
    int indent;         /* Current indentation index */
    int indstack[MAXINDENT];            /* Stack of indents */
    int atbol;          /* Nonzero if at begin of new line */
    int pendin;         /* Pending indents (if > 0) or dedents (if < 0) */
    const char *prompt;          /* For interactive prompting */
    int lineno;         /* Current line number */
    _PyTok_Loc start_loc;
    int level;          /* () [] {} Parentheses nesting level */
            /* Used to allow free continuations inside them */
    char parenstack[MAXLEVEL];
    int parenlinenostack[MAXLEVEL];
    int parencolstack[MAXLEVEL];
    PyObject *filename;
    PyObject *module;
    /* Stuff for checking on different tab sizes */
    int altindstack[MAXINDENT];         /* Stack of alternate indents */
    /* Stuff for PEP 0263 */
    char *encoding;         /* Source encoding. */
    const char* line_start;     /* pointer to start of current line */

    _PyTok_SourceText source;
    struct _PyTok_Reader *reader;

    int type_comments;      /* Whether to look for type comments */

    /* How to proceed when asked for a new token in interactive mode */
    enum interactive_underflow_t interactive_underflow;
    int report_warnings;
    // TODO: Factor this into its own thing
    tokenizer_mode tok_mode_stack[MAXFSTRINGLEVEL];
    int tok_mode_stack_index;
    int tok_extra_tokens;
    int comment_newline;
    int implicit_newline;
#ifdef Py_DEBUG
    int debug;
#endif
};

static inline _PyTok_Off
_PyLexer_BufferOffset(const struct tok_state *tok, const char *position)
{
    assert(tok->buf != NULL);
    assert(tok->inp >= tok->buf);
    assert(position >= tok->buf && position <= tok->inp);
    Py_ssize_t offset = position - tok->buf;
    assert(tok->buf_offset <= PY_SSIZE_T_MAX - offset);
    return tok->buf_offset + offset;
}

static inline char *
_PyLexer_BufferPointer(const struct tok_state *tok, _PyTok_Off offset)
{
    assert(tok->buf != NULL);
    assert(tok->inp >= tok->buf);
    assert(offset >= tok->buf_offset);
    assert(offset - tok->buf_offset <= tok->inp - tok->buf);
    return tok->buf + (offset - tok->buf_offset);
}

static inline const char *
_PyLexer_BufferSpanView(const struct tok_state *tok, _PyTok_Span span,
                        Py_ssize_t *length)
{
    assert(length != NULL);
    assert(_PyTok_SpanIsValid(span));
    *length = span.end - span.start;
    (void)_PyLexer_BufferPointer(tok, span.end);
    return _PyLexer_BufferPointer(tok, span.start);
}

static inline int
_PyLexer_ByteColumn(const struct tok_state *tok)
{
    assert(tok->line_start != NULL);
    assert(tok->cur >= tok->line_start);
    Py_ssize_t column = tok->cur - tok->line_start;
    assert(column <= INT_MAX);
    return (int)column;
}

static inline _PyTok_Span
_PyLexer_BufferSpan(const struct tok_state *tok, const char *start,
                    const char *end)
{
    if (start == NULL) {
        assert(end == NULL);
        return (_PyTok_Span){-1, -1};
    }
    assert(end != NULL);
    assert(start <= end);
    return _PyTok_SpanFromBounds(
        _PyLexer_BufferOffset(tok, start),
        _PyLexer_BufferOffset(tok, end));
}

int _PyLexer_token_setup(struct tok_state *tok, struct token *token, int type, const char *start, const char *end);

struct tok_state *_PyTokenizer_tok_new(void);
void _PyTokenizer_Free(struct tok_state *);
void _PyToken_Free(struct token *);
void _PyToken_Init(struct token *);


#endif
