#include "Python.h"

#include "source.h"

void
_PyTok_SourceInit(_PyTok_SourceText *source)
{
    *source = (_PyTok_SourceText){0};
}

void
_PyTok_SourceClear(_PyTok_SourceText *source)
{
    PyMem_Free(source->bytes);
    PyMem_Free(source->implicit_lines);
    _PyTok_SourceInit(source);
}

void
_PyTok_SourceDiscard(_PyTok_SourceText *source)
{
    assert(source->base_offset <= PY_SSIZE_T_MAX - source->len);
    source->base_offset += source->len;
    source->len = 0;
    if (source->bytes != NULL) {
        source->bytes[0] = '\0';
    }
    if (source->implicit_lines != NULL) {
        Py_ssize_t used = source->nlines / 8 + (source->nlines % 8 != 0);
        memset(source->implicit_lines, 0, Py_MIN(used, source->implicit_cap));
    }
    source->nlines = 0;
}

static int
reserve_bytes(_PyTok_SourceText *source, Py_ssize_t needed)
{
    if (needed <= source->cap) {
        return 0;
    }
    Py_ssize_t cap = source->cap > 0 ? source->cap : BUFSIZ;
    while (cap < needed) {
        if (cap > PY_SSIZE_T_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }
    char *bytes;
#ifdef Py_DEBUG
    /* Moving on growth makes stale interior pointers fail in debug builds. */
    bytes = PyMem_Malloc(cap);
    if (bytes != NULL && source->len > 0) {
        memcpy(bytes, source->bytes, source->len);
    }
#else
    bytes = PyMem_Realloc(source->bytes, cap);
#endif
    if (bytes == NULL) {
        PyErr_NoMemory();
        return -1;
    }
#ifdef Py_DEBUG
    if (source->bytes != NULL) {
        memset(source->bytes, 0xDD, source->cap);
        PyMem_Free(source->bytes);
    }
#endif
    source->bytes = bytes;
    source->cap = cap;
    return 0;
}

static int
reserve_implicit_lines(_PyTok_SourceText *source, int nlines)
{
    Py_ssize_t needed = nlines / 8 + (nlines % 8 != 0);
    if (needed <= source->implicit_cap) {
        return 0;
    }
    Py_ssize_t cap = source->implicit_cap > 0 ? source->implicit_cap : 16;
    while (cap < needed) {
        if (cap > PY_SSIZE_T_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }
    unsigned char *lines = PyMem_Realloc(source->implicit_lines, cap);
    if (lines == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    memset(lines + source->implicit_cap, 0, cap - source->implicit_cap);
    source->implicit_lines = lines;
    source->implicit_cap = cap;
    return 0;
}

static int
validate_line(const _PyTok_SourceText *source, const char *bytes,
              Py_ssize_t len, int implicit_newline)
{
    if (len <= 0 || bytes == NULL ||
            (source->nlines > 0 &&
             (source->len == 0 || source->bytes[source->len - 1] != '\n'))) {
        PyErr_SetString(PyExc_SystemError, "invalid tokenizer source line");
        return -1;
    }
    const char *newline = memchr(bytes, '\n', len);
    if ((newline != NULL && newline != bytes + len - 1) ||
            (implicit_newline && newline == NULL)) {
        PyErr_SetString(PyExc_SystemError, "invalid tokenizer source line");
        return -1;
    }
    if (source->nlines == INT_MAX ||
            (source->nlines == INT_MAX - 1 && newline != NULL)) {
        PyErr_SetString(PyExc_OverflowError, "too many tokenizer source lines");
        return -1;
    }
    return 0;
}

_PyTok_Off
_PyTok_SourceAppendLine(_PyTok_SourceText *source, const char *bytes,
                        Py_ssize_t len, int implicit_newline)
{
    if (validate_line(source, bytes, len, implicit_newline) < 0) {
        return -1;
    }
    if (source->len > PY_SSIZE_T_MAX - len - 1 ||
            source->base_offset > PY_SSIZE_T_MAX - source->len - len) {
        PyErr_NoMemory();
        return -1;
    }
    int nlines = source->nlines + 1;
    if ((implicit_newline && reserve_implicit_lines(source, nlines) < 0) ||
            reserve_bytes(source, source->len + len + 1) < 0) {
        return -1;
    }

    _PyTok_Off start = source->len;
    memcpy(source->bytes + start, bytes, len);
    source->len += len;
    source->bytes[source->len] = '\0';
    if (implicit_newline) {
        source->implicit_lines[(nlines - 1) / 8] |=
            (unsigned char)(1U << ((nlines - 1) & 7));
    }
    source->nlines = nlines;
    return source->base_offset + start;
}

const char *
_PyTok_SourceLineView(const _PyTok_SourceText *source, Py_ssize_t lineno,
                      Py_ssize_t *len)
{
    assert(len != NULL);
    const char *line = _PyTok_SourceData(source);
    const char *end = line + source->len;
    while (lineno > 1) {
        const char *newline = memchr(line, '\n', end - line);
        if (newline == NULL) {
            break;
        }
        line = newline + 1;
        lineno--;
    }
    const char *newline = memchr(line, '\n', end - line);
    *len = (newline != NULL ? newline : end) - line;
    return line;
}

int
_PyTok_SourceLineIsImplicit(const _PyTok_SourceText *source, int lineno)
{
    if (lineno < 1 || lineno > source->nlines ||
            (lineno - 1) / 8 >= source->implicit_cap) {
        return 0;
    }
    return (source->implicit_lines[(lineno - 1) / 8] >>
            ((lineno - 1) & 7)) & 1;
}
