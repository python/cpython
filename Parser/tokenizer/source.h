#ifndef Py_TOKENIZER_SOURCE_H
#define Py_TOKENIZER_SOURCE_H

#include "Python.h"

typedef Py_ssize_t _PyTok_Off;

typedef struct {
    _PyTok_Off start;
    _PyTok_Off end;
} _PyTok_Span;

typedef struct {
    int lineno;
    int byte_col;
} _PyTok_Loc;

typedef enum {
    _PYTOK_AFFINITY_LEFT,
    _PYTOK_AFFINITY_RIGHT,
} _PyTok_Affinity;

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
/* Clear invalidates all cursors and span views for the source. */
PyAPI_FUNC(void) _PyTok_SourceClear(_PyTok_SourceText *);
/* Append one logical line. The implicit_newline flag means that its newline
   terminator was synthesized. The input must not point into source storage. */
PyAPI_FUNC(_PyTok_Off) _PyTok_SourceAppendLine(
    _PyTok_SourceText *source, const char *bytes, Py_ssize_t len,
    int implicit_newline);
/* The returned view is invalidated by SourceAppendLine and SourceClear. */
PyAPI_FUNC(const char *) _PyTok_SourceSpanView(
    const _PyTok_SourceText *, _PyTok_Span, Py_ssize_t *);
PyAPI_FUNC(int) _PyTok_SourceLine(
    const _PyTok_SourceText *, int, _PyTok_Line *);
/* At a line boundary, affinity selects the preceding or following line. */
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
