#ifndef Py_TOKENIZER_SOURCE_H
#define Py_TOKENIZER_SOURCE_H

#include "Python.h"

typedef Py_ssize_t _PyTok_Off;

/* Half-open byte offsets into a _PyTok_SourceText. */
typedef struct {
    _PyTok_Off start;
    _PyTok_Off end;
} _PyTok_Span;

/* Lines are 1-based and byte columns are 0-based. */
typedef struct {
    int lineno;
    int byte_col;
} _PyTok_Loc;

typedef enum {
    _PYTOK_AFFINITY_LEFT,
    _PYTOK_AFFINITY_RIGHT,
} _PyTok_Affinity;

/* The half-open range includes the terminating newline when present. */
typedef struct {
    _PyTok_Off start;
    _PyTok_Off end;
    unsigned implicit_newline : 1;
    unsigned contains_nul : 1;
} _PyTok_Line;

typedef struct {
    char *bytes;
    _PyTok_Off len;
    _PyTok_Off cap;
    _PyTok_Off *line_checkpoints;
    unsigned char *implicit_lines;
    int nlines;
    int checkpoints_cap;
    Py_ssize_t implicit_cap;
} _PyTok_SourceText;

PyAPI_FUNC(void) _PyTok_SourceInit(_PyTok_SourceText *);
/* Clear invalidates all cursors, spans, and views for the source. */
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
/* Look up a 1-based line. Empty and newline-terminated sources have an empty
   virtual line at EOF. */
PyAPI_FUNC(int) _PyTok_SourceLine(
    const _PyTok_SourceText *, int, _PyTok_Line *);
/* Return false for invalid line numbers and the virtual EOF line. */
PyAPI_FUNC(int) _PyTok_SourceLineIsImplicit(
    const _PyTok_SourceText *, int);
/* At a line boundary, left affinity selects the preceding line at its end;
   right affinity selects the following line at byte column zero. */
PyAPI_FUNC(int) _PyTok_SourceLocation(
    const _PyTok_SourceText *, _PyTok_Off, _PyTok_Affinity, _PyTok_Loc *);

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

static inline _PyTok_Off
_PyTok_SourceFindLineEnd(const _PyTok_SourceText *source, _PyTok_Off start)
{
    if (source->bytes == NULL || start < 0 || start >= source->len) {
        PyErr_SetString(PyExc_SystemError,
                        "corrupt tokenizer source line index");
        return -1;
    }
    const char *newline = memchr(
        source->bytes + start, '\n', source->len - start);
    if (newline == NULL) {
        PyErr_SetString(PyExc_SystemError,
                        "corrupt tokenizer source line index");
        return -1;
    }
    return newline - source->bytes + 1;
}

#endif
