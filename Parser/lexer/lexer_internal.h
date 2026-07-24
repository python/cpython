#ifndef _PY_LEXER_INTERNAL_H_
#define _PY_LEXER_INTERNAL_H_

#include "lexer.h"

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

#ifdef Py_DEBUG
static inline tokenizer_mode *
TOK_GET_MODE(struct tok_state *tok)
{
    assert(tok->tok_mode_stack_index >= 0);
    assert(tok->tok_mode_stack_index < MAXFSTRINGLEVEL);
    return &tok->tok_mode_stack[tok->tok_mode_stack_index];
}

static inline tokenizer_mode *
TOK_NEXT_MODE(struct tok_state *tok)
{
    assert(tok->tok_mode_stack_index >= 0);
    assert(tok->tok_mode_stack_index + 1 < MAXFSTRINGLEVEL);
    return &tok->tok_mode_stack[++tok->tok_mode_stack_index];
}
#else
#define TOK_GET_MODE(tok) (&(tok)->tok_mode_stack[(tok)->tok_mode_stack_index])
#define TOK_NEXT_MODE(tok) (&(tok)->tok_mode_stack[++(tok)->tok_mode_stack_index])
#endif

#define FTSTRING_MIDDLE(tok_mode) ((tok_mode)->string_kind == TSTRING ? TSTRING_MIDDLE : FSTRING_MIDDLE)
#define FTSTRING_END(tok_mode) ((tok_mode)->string_kind == TSTRING ? TSTRING_END : FSTRING_END)
#define TOK_GET_STRING_PREFIX(tok) (TOK_GET_MODE(tok)->string_kind == TSTRING ? 't' : 'f')

#define tok_nextc _PyLexer_nextc
#define tok_backup _PyLexer_backup

int _PyLexer_nextc(struct tok_state *);
void _PyLexer_backup(struct tok_state *, int);
int _PyLexer_set_ftstring_expr(struct tok_state *, struct token *, char);
int _PyLexer_check_string_prefixes(struct tok_state *, int, int, int, int, int);
int _PyLexer_scan_number(struct tok_state *, struct token *, int, int);
int _PyLexer_scan_fstring_start(struct tok_state *, struct token *, int);
int _PyLexer_scan_string(struct tok_state *, struct token *, int);
int _PyLexer_get_normal_mode(struct tok_state *, tokenizer_mode *, struct token *);
int _PyLexer_get_fstring_mode(struct tok_state *, tokenizer_mode *, struct token *);

#endif
