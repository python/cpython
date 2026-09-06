#ifndef _PY_LEXER_LEXER_H_
#define _PY_LEXER_LEXER_H_

#include "state.h"

int _PyLexer_update_ftstring_expr(struct tok_state *tok, char cur);

int _PyTokenizer_Get(struct tok_state *, struct token *);

/* The view points into the current input window. The next
   _PyTokenizer_Get() call may discard it. */
static inline const char *
_PyToken_TextView(const struct tok_state *tok, const struct token *token,
                  Py_ssize_t *length)
{
    assert(length != NULL);
    if (token->span.start < 0) {
        assert(token->span.start == -1 && token->span.end == -1);
        *length = 0;
        return "";
    }
    assert(_PyTok_SpanIsValid(token->span));
    assert(tok->buf != NULL);
    assert(tok->inp >= tok->buf);
    assert(token->span.start >= tok->buf_offset);
    assert(token->span.end - tok->buf_offset <= tok->inp - tok->buf);
    *length = token->span.end - token->span.start;
    return tok->buf + (token->span.start - tok->buf_offset);
}

#endif
