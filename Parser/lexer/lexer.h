#ifndef _PY_LEXER_LEXER_H_
#define _PY_LEXER_LEXER_H_

#include "state.h"

int update_fstring_expr(struct tok_state *tok, char cur);

extern int _PyTokenizer_Get(struct tok_state *, struct token *);

#endif
