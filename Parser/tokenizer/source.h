#ifndef Py_TOKENIZER_SOURCE_H
#define Py_TOKENIZER_SOURCE_H

#include "Python.h"

typedef Py_ssize_t _PyTok_Off;

/* Spans use half-open logical byte offsets into decoded input. Their backing
   storage may retain only the current input window. */
typedef struct {
    _PyTok_Off start;
    _PyTok_Off end;
} _PyTok_Span;

/* Lines are 1-based and byte columns are 0-based. */
typedef struct {
    int lineno;
    int byte_col;
} _PyTok_Loc;

typedef struct {
    char *bytes;
    _PyTok_Off len;
    _PyTok_Off cap;
    unsigned char *implicit_lines;
    int nlines;
    Py_ssize_t implicit_cap;
} _PyTok_SourceText;

PyAPI_FUNC(void) _PyTok_SourceInit(_PyTok_SourceText *);
/* Clear invalidates all spans and views for the source. */
PyAPI_FUNC(void) _PyTok_SourceClear(_PyTok_SourceText *);
/* Append one nonempty logical line and return its start offset. The input may
   contain one newline, as its final byte. An unterminated line must be the
   final line. implicit_newline means that the final newline was synthesized.
   The input must not point into source storage. */
PyAPI_FUNC(_PyTok_Off) _PyTok_SourceAppendLine(
    _PyTok_SourceText *source, const char *bytes, Py_ssize_t len,
    int implicit_newline);
/* The returned view is invalidated by SourceAppendLine and SourceClear. */
PyAPI_FUNC(const char *) _PyTok_SourceSpanView(
    const _PyTok_SourceText *, _PyTok_Span, Py_ssize_t *);
/* Return false for invalid line numbers and the virtual EOF line. */
PyAPI_FUNC(int) _PyTok_SourceLineIsImplicit(
    const _PyTok_SourceText *, int);

static inline _PyTok_Span
_PyTok_SpanFromBounds(_PyTok_Off start, _PyTok_Off end)
{
    return (_PyTok_Span){start, end};
}

static inline int
_PyTok_SpanIsValid(_PyTok_Span span)
{
    return span.start >= 0 && span.end >= span.start;
}

#endif
