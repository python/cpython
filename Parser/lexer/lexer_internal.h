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
#define tok_nextc _PyLexer_nextc
#define tok_backup _PyLexer_backup

static inline int
tok_failed(const struct tok_state *tok)
{
    return tok->done != E_OK && tok->done != E_EOF &&
           tok->done != E_INTERACT_STOP;
}

int _PyLexer_nextc(struct tok_state *);
void _PyLexer_backup(struct tok_state *, int);
int _PyLexer_record_ftstring_comment(
    struct tok_state *, ftstring_state *, const char *, const char *);
int _PyLexer_finish_ftstring_expr(
    struct tok_state *, ftstring_state *, struct token *);
int _PyLexer_check_string_prefixes(struct tok_state *, int, int, int, int, int);
int _PyLexer_scan_number(struct tok_state *, struct token *, int, int);
int _PyLexer_scan_fstring_start(struct tok_state *, struct token *, int);
int _PyLexer_scan_string(struct tok_state *, struct token *, int);
int _PyLexer_get_normal(struct tok_state *, ftstring_state *, struct token *);
int _PyLexer_get_ftstring(struct tok_state *, ftstring_state *, struct token *);

#endif
