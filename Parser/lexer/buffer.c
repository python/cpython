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
    pointers->multi_line_start_from_buf = tok->multi_line_start == NULL
        ? -1 : tok->multi_line_start - tok->buf;
    for (int index = tok->tok_mode_stack_index; index > 0; --index) {
        tokenizer_mode *mode = &tok->tok_mode_stack[index];
        mode->start_offset = mode->start == NULL ? -1 : mode->start - tok->buf;
        mode->multi_line_start_offset = mode->multi_line_start == NULL
            ? -1 : mode->multi_line_start - tok->buf;
    }
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
    tok->multi_line_start = pointers->multi_line_start_from_buf < 0
        ? NULL : tok->buf + pointers->multi_line_start_from_buf;
    for (int index = tok->tok_mode_stack_index; index > 0; --index) {
        tokenizer_mode *mode = &tok->tok_mode_stack[index];
        mode->start = mode->start_offset < 0
            ? NULL : tok->buf + mode->start_offset;
        mode->multi_line_start = mode->multi_line_start_offset < 0
            ? NULL : tok->buf + mode->multi_line_start_offset;
    }
}
