#ifndef _PY_LEXER_H_
#define _PY_LEXER_H_

#include "object.h"
#include "../tokenizer/source.h"
#include "../tokenizer/tokenizer.h"

#define MAXINDENT 100       /* Max indentation level */
#define MAXLEVEL 200        /* Max parentheses level */
#define MAXFTSTRINGLEVEL 150
#define FTSTRING_STACK_INLINE_CAPACITY 1

typedef enum {
    FTSTRING_MODE_MIDDLE,
    FTSTRING_MODE_EXPRESSION,
    FTSTRING_MODE_FORMAT_SPEC,
} ftstring_mode;

typedef enum {
    FSTRING,
    RAW_FSTRING,
    TSTRING,
    RAW_TSTRING,
} ftstring_kind;

#define MAX_EXPR_NESTING 3

typedef struct _tokenizer_comments {
    Py_ssize_t count;
    Py_ssize_t capacity;
    _PyTok_Span spans[];
} tokenizer_comments;

typedef struct _ftstring_state {
    ftstring_mode mode;
    ftstring_kind kind;
    char quote;
    unsigned char quote_size;
    unsigned char debug_expr;
    unsigned char replacement_depth;
    int paren_level;
    _PyTok_Off start;
    _PyTok_Loc start_loc;
    _PyTok_Span expr_span;
    tokenizer_comments *comments;
} ftstring_state;

static inline int
_PyLexer_IsTString(ftstring_kind kind)
{
    return kind == TSTRING || kind == RAW_TSTRING;
}

static inline int
_PyLexer_IsRawString(ftstring_kind kind)
{
    return kind == RAW_FSTRING || kind == RAW_TSTRING;
}

/* Tokenizer state */
struct tok_state {
    _PyTok_Off buf_offset;
    _PyTok_Off cur;
    _PyTok_Off inp;
    _PyTok_Off start;
    _PyTok_Off line_start;
    _PyTok_SourceText source;
    int done;           /* E_OK normally, E_EOF at EOF, otherwise error code */
    FILE *fp;           /* Rest of input; NULL if tokenizing a string */
    int indent;         /* Current indentation index */
    int indstack[MAXINDENT];            /* Stack of indents */
    int atbol;          /* Nonzero if at begin of new line */
    int pendin;         /* Pending indents (if > 0) or dedents (if < 0) */
    int lineno;         /* Current line number */
    _PyTok_Loc start_loc;
    _PyTokenizer_Diagnostic diagnostic;
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
    struct _PyTok_Reader *reader;

    int type_comments;      /* Whether to look for type comments */

    ftstring_state *ftstring_stack;
    ftstring_state ftstring_stack_inline[FTSTRING_STACK_INLINE_CAPACITY];
    int ftstring_depth;
    int ftstring_capacity;
    int tok_extra_tokens;
    int comment_newline;
    int implicit_newline;
#ifdef Py_DEBUG
    int debug;
#endif
};

static inline ftstring_state *
_PyLexer_CurrentFTString(struct tok_state *tok)
{
    assert(tok->ftstring_stack != NULL);
    assert(tok->ftstring_depth >= 0 && tok->ftstring_depth <= tok->ftstring_capacity);
    if (tok->ftstring_depth == 0) {
        return NULL;
    }
    return &tok->ftstring_stack[tok->ftstring_depth - 1];
}

static inline char
_PyLexer_StringPrefix(ftstring_kind kind)
{
    return _PyLexer_IsTString(kind) ? 't' : 'f';
}

static inline int
_PyLexer_FTStringBracketDepth(const struct tok_state *tok,
                              const ftstring_state *state)
{
    return tok->level - state->paren_level;
}

static inline _PyTok_Off
_PyLexer_BufferOffset(const struct tok_state *tok, const char *position)
{
    const char *base = _PyTok_SourceData(&tok->source);
    assert(position >= base && position <= base + tok->source.len);
    return tok->source.base_offset + (position - base);
}

static inline const char *
_PyLexer_BufferPointer(const struct tok_state *tok, _PyTok_Off offset)
{
    assert(offset >= tok->source.base_offset);
    assert(offset - tok->source.base_offset <= tok->source.len);
    return _PyTok_SourceData(&tok->source) + (offset - tok->source.base_offset);
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
    assert(tok->line_start >= 0);
    assert(tok->cur >= tok->line_start);
    Py_ssize_t column = tok->cur - tok->line_start;
    assert(column <= INT_MAX);
    return (int)column;
}

int _PyLexer_token_setup(struct tok_state *tok, struct token *token, int type,
                         _PyTok_Off start, _PyTok_Off end);

ftstring_state *_PyLexer_PushFTString(struct tok_state *);
void _PyLexer_PopFTString(struct tok_state *);


#endif
