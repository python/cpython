#ifndef Py_TOKENIZER_CURSOR_H
#define Py_TOKENIZER_CURSOR_H

#include "source.h"

typedef struct {
    /* The source must remain initialized at this address while in use. */
    const _PyTok_SourceText *source;
    _PyTok_Off pos;
    _PyTok_Off line_start;
    _PyTok_Off line_end;
    int lineno;
} _PyTok_Cursor;

/* Move to the start of a 1-based line. Both setters preserve the cursor on
   error. */
PyAPI_FUNC(int) _PyTok_CursorSetLine(_PyTok_Cursor *, int);
/* Move to an offset. A line boundary selects the following line. */
PyAPI_FUNC(int) _PyTok_CursorSetOffset(_PyTok_Cursor *, _PyTok_Off);

static inline void
_PyTok_CursorInit(_PyTok_Cursor *cursor, const _PyTok_SourceText *source)
{
    *cursor = (_PyTok_Cursor){
        .source = source,
    };
}

/* Read one byte from the current line, including its terminating newline.
   EOF marks the line boundary, not necessarily the end of the source. It is
   also returned if advancing would make the byte column unrepresentable. */
static inline int
_PyTok_CursorAdvance(_PyTok_Cursor *cursor)
{
    assert(cursor->source != NULL);
    assert(cursor->pos >= cursor->line_start);
    assert(cursor->pos <= cursor->line_end);
    assert(cursor->line_end <= cursor->source->len);
    if (cursor->pos >= cursor->line_end) {
        return EOF;
    }
    if (cursor->pos - cursor->line_start >= INT_MAX) {
        return EOF;
    }
    return Py_CHARMASK(cursor->source->bytes[cursor->pos++]);
}

/* Return the byte at a nonnegative distance within the current line, or EOF
   if the distance reaches or crosses the line boundary. */
static inline int
_PyTok_CursorPeek(const _PyTok_Cursor *cursor, int distance)
{
    assert(cursor->source != NULL);
    assert(cursor->pos >= cursor->line_start);
    assert(cursor->pos <= cursor->line_end);
    assert(cursor->line_end <= cursor->source->len);
    assert(distance >= 0);
    if (distance < 0 ||
            distance >= cursor->line_end - cursor->pos) {
        return EOF;
    }
    return Py_CHARMASK(cursor->source->bytes[cursor->pos + distance]);
}

#endif
