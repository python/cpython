#ifndef _PY_LEXER_INTERNAL_H_
#define _PY_LEXER_INTERNAL_H_

#include "errcode.h"
#include "state.h"

#define is_potential_identifier_start(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || c == '_'\
               || (c >= 128))

#define is_potential_identifier_char(c) (\
              (c >= 'a' && c <= 'z')\
               || (c >= 'A' && c <= 'Z')\
               || (c >= '0' && c <= '9')\
               || c == '_'\
               || (c >= 128))

#define FTSTRING_MIDDLE(state) \
    (_PyLexer_IsTString((state)->kind) ? TSTRING_MIDDLE : FSTRING_MIDDLE)
#define FTSTRING_END(state) \
    (_PyLexer_IsTString((state)->kind) ? TSTRING_END : FSTRING_END)
#define tok_backup _PyLexer_backup

static inline int
tok_failed(const struct tok_state *tok)
{
    return tok->done != E_OK && tok->done != E_EOF &&
           tok->done != E_INTERACT_STOP;
}

int _PyLexer_refill(struct tok_state *);

static inline int
tok_nextc(struct tok_state *tok)
{
    while (tok->cur == tok->inp) {
        if (!_PyLexer_refill(tok)) {
            return EOF;
        }
    }
    assert(tok->cur >= tok->line_start);
    assert(tok->cur >= tok->source.base_offset);
    assert(tok->cur - tok->source.base_offset < tok->source.len);
    if (tok->cur - tok->line_start >= INT_MAX) {
        tok->done = E_COLUMNOVERFLOW;
        return EOF;
    }
    return Py_CHARMASK(
        tok->source.bytes[tok->cur++ - tok->source.base_offset]);
}

void _PyLexer_backup(struct tok_state *, int);
int _PyLexer_record_ftstring_comment(
    struct tok_state *, ftstring_state *, _PyTok_Off, _PyTok_Off);
int _PyLexer_finish_ftstring_expr(
    struct tok_state *, ftstring_state *, struct token *);
int _PyLexer_check_string_prefixes(struct tok_state *, int, int, int, int, int);
int _PyLexer_scan_number(struct tok_state *, struct token *, int, int);
int _PyLexer_scan_fstring_start(struct tok_state *, struct token *, int);
int _PyLexer_scan_string(struct tok_state *, struct token *, int);
int _PyLexer_get_normal(struct tok_state *, ftstring_state *, struct token *);
int _PyLexer_get_ftstring(struct tok_state *, ftstring_state *, struct token *);

#endif
