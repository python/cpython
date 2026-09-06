#include "Python.h"
#include "buffer.h"
#include "state.h"

void
_PyLexer_SaveBufferPointers(struct tok_state *tok, const char *base,
                            _PyLexer_BufferPointers *pointers)
{
    pointers->buf_from_base = tok->buf - base;
    pointers->cur_from_buf = tok->cur - tok->buf;
    pointers->inp_from_buf = tok->inp - tok->buf;
    pointers->start_from_buf = tok->start == NULL
        ? -1 : tok->start - tok->buf;
    pointers->line_start_from_buf = tok->line_start == NULL
        ? -1 : tok->line_start - tok->buf;
}

void
_PyLexer_RestoreBufferPointers(struct tok_state *tok, char *base,
                               const _PyLexer_BufferPointers *pointers)
{
    tok->buf = base + pointers->buf_from_base;
    tok->cur = tok->buf + pointers->cur_from_buf;
    tok->inp = tok->buf + pointers->inp_from_buf;
    tok->start = pointers->start_from_buf < 0
        ? NULL : tok->buf + pointers->start_from_buf;
    tok->line_start = pointers->line_start_from_buf < 0
        ? NULL : tok->buf + pointers->line_start_from_buf;
}
