#ifndef _LEXER_BUFFER_H_
#define _LEXER_BUFFER_H_

#include "pyport.h"

struct tok_state;

typedef struct {
    Py_ssize_t buf_from_base;
    Py_ssize_t cur_from_buf;
    Py_ssize_t inp_from_buf;
    Py_ssize_t start_from_buf;
    Py_ssize_t line_start_from_buf;
} _PyLexer_BufferPointers;

void _PyLexer_SaveBufferPointers(
    struct tok_state *, const char *, _PyLexer_BufferPointers *);
void _PyLexer_RestoreBufferPointers(
    struct tok_state *, char *, const _PyLexer_BufferPointers *);

#endif
