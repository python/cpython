#ifndef Py_TOKENIZER_SOURCE_H
#define Py_TOKENIZER_SOURCE_H

#include "Python.h"

#include "types.h"

typedef struct {
    char *bytes;
    _PyTok_Off base_offset;
    _PyTok_Off len;
    _PyTok_Off cap;
    unsigned char *implicit_lines;
    int nlines;
    Py_ssize_t implicit_cap;
} _PyTok_SourceText;

static inline const char *
_PyTok_SourceData(const _PyTok_SourceText *source)
{
    return source->bytes != NULL ? source->bytes : "";
}

PyAPI_FUNC(void) _PyTok_SourceInit(_PyTok_SourceText *);
/* Clear invalidates all spans and views for the source. */
PyAPI_FUNC(void) _PyTok_SourceClear(_PyTok_SourceText *);
/* Discard the retained window and invalidate its spans and views.
   Keep its allocation and advance the logical base to the end of the window. */
PyAPI_FUNC(void) _PyTok_SourceDiscard(_PyTok_SourceText *);
/* Append one nonempty logical line and return its start offset. The input may
   contain one newline, as its final byte. An unterminated line must be the
   final line. implicit_newline means that the final newline was synthesized.
   The input must not point into source storage. */
PyAPI_FUNC(_PyTok_Off) _PyTok_SourceAppendLine(
    _PyTok_SourceText *source, const char *bytes, Py_ssize_t len,
    int implicit_newline);
/* Return borrowed bytes excluding '\n', writing the byte length to *len.
   Line numbers are 1-based and clamp to the first or final line; a trailing
   '\n' adds an empty final line. The view need not be NUL-terminated.
   This does not set an exception. Append, discard, and clear invalidate the view. */
PyAPI_FUNC(const char *) _PyTok_SourceLineView(
    const _PyTok_SourceText *source, Py_ssize_t lineno, Py_ssize_t *len);
/* Return false for invalid line numbers and the virtual EOF line. */
PyAPI_FUNC(int) _PyTok_SourceLineIsImplicit(
    const _PyTok_SourceText *, int);

#endif
