#ifndef Py_TOKENIZER_TYPES_H
#define Py_TOKENIZER_TYPES_H

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
