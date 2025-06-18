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
        mode->start_offset = mode->start - tok->buf;
        mode->multi_line_start_offset = mode->multi_line_start - tok->buf;
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
        mode->start = tok->buf + mode->start_offset;
        mode->multi_line_start = tok->buf + mode->multi_line_start_offset;
    }
}

/* Read a line of text from TOK into S, using the stream in TOK.
   Return NULL on failure, else S.

   On entry, tok->decoding_buffer will be one of:
     1) NULL: need to call tok->decoding_readline to get a new line
     2) PyUnicodeObject *: decoding_feof has called tok->decoding_readline and
       stored the result in tok->decoding_buffer
     3) PyByteArrayObject *: previous call to tok_readline_recode did not have enough room
       (in the s buffer) to copy entire contents of the line read
       by tok->decoding_readline.  tok->decoding_buffer has the overflow.
       In this case, tok_readline_recode is called in a loop (with an expanded buffer)
       until the buffer ends with a '\n' (or until the end of the file is
       reached): see tok_nextc and its calls to tok_reserve_buf.
*/
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
