#ifndef _LEXER_BUFFER_H_
#define _LEXER_BUFFER_H_

#include "pyport.h"

void remember_fstring_buffers(struct tok_state *tok);
void restore_fstring_buffers(struct tok_state *tok);
int tok_reserve_buf(struct tok_state *tok, Py_ssize_t size);

#endif
