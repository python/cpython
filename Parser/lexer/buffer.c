#include "Python.h"
#include "errcode.h"

#include "state.h"

/* Traverse and remember all f-string buffers, in order to be able to restore
   them after reallocating tok->buf */
void
_PyLexer_remember_fstring_buffers(struct tok_state *tok)
{
    int index;
    tokenizer_mode *mode;

    for (index = tok->tok_mode_stack_index; index >= 0; --index) {
        mode = &(tok->tok_mode_stack[index]);
        mode->start_offset = mode->start == NULL ? -1 : mode->start - tok->buf;
        mode->multi_line_start_offset = mode->multi_line_start == NULL ? -1 : mode->multi_line_start - tok->buf;
    }
}

/* Traverse and restore all f-string buffers after reallocating tok->buf */
void
_PyLexer_restore_fstring_buffers(struct tok_state *tok)
{
    int index;
    tokenizer_mode *mode;

    for (index = tok->tok_mode_stack_index; index >= 0; --index) {
        mode = &(tok->tok_mode_stack[index]);
        mode->start = mode->start_offset < 0 ? NULL : tok->buf + mode->start_offset;
        mode->multi_line_start = mode->multi_line_start_offset < 0 ? NULL : tok->buf + mode->multi_line_start_offset;
    }
}

int
_PyLexer_tok_reserve_buf(struct tok_state *tok, Py_ssize_t size)
{
    Py_ssize_t cur = tok->cur - tok->buf;
    Py_ssize_t oldsize = tok->inp - tok->buf;
    Py_ssize_t newsize = oldsize + Py_MAX(size, oldsize >> 1);
    if (newsize > tok->end - tok->buf) {
        char *newbuf = tok->buf;
        Py_ssize_t start = tok->start == NULL ? -1 : tok->start - tok->buf;
        Py_ssize_t line_start = tok->start == NULL ? -1 : tok->line_start - tok->buf;
        Py_ssize_t multi_line_start = tok->multi_line_start - tok->buf;
        _PyLexer_remember_fstring_buffers(tok);
        newbuf = (char *)PyMem_Realloc(newbuf, newsize);
        if (newbuf == NULL) {
            tok->done = E_NOMEM;
            return 0;
        }
        tok->buf = newbuf;
        tok->cur = tok->buf + cur;
        tok->inp = tok->buf + oldsize;
        tok->end = tok->buf + newsize;
        tok->start = start < 0 ? NULL : tok->buf + start;
        tok->line_start = line_start < 0 ? NULL : tok->buf + line_start;
        tok->multi_line_start = multi_line_start < 0 ? NULL : tok->buf + multi_line_start;
        _PyLexer_restore_fstring_buffers(tok);
    }
    return 1;
}
