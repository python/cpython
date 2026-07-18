#include "Python.h"

#include "source.h"

#define LINE_CHECKPOINT_INTERVAL 256

void
_PyTok_SourceInit(_PyTok_SourceText *source)
{
    *source = (_PyTok_SourceText){0};
}

void
_PyTok_SourceClear(_PyTok_SourceText *source)
{
    PyMem_Free(source->bytes);
    PyMem_Free(source->line_checkpoints);
    PyMem_Free(source->implicit_lines);
    _PyTok_SourceInit(source);
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
reserve_checkpoints(_PyTok_SourceText *source, int needed)
{
    if (needed <= source->checkpoints_cap) {
        return 0;
    }
    int cap;
    if (source->checkpoints_cap == 0) {
        cap = 16;
    }
    else if (source->checkpoints_cap <= INT_MAX / 2) {
        cap = source->checkpoints_cap * 2;
    }
    else {
        PyErr_NoMemory();
        return -1;
    }
    _PyTok_Off *checkpoints = source->line_checkpoints;
    PyMem_Resize(checkpoints, _PyTok_Off, cap);
    if (checkpoints == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    source->line_checkpoints = checkpoints;
    source->checkpoints_cap = cap;
    return 0;
}

static int
reserve_implicit_lines(_PyTok_SourceText *source, int nlines)
{
    Py_ssize_t needed = ((Py_ssize_t)nlines + 7) / 8;
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
    if (source->len > PY_SSIZE_T_MAX - len - 1) {
        PyErr_NoMemory();
        return -1;
    }
    int nlines = source->nlines + 1;
    int checkpoint = ((nlines - 1) % LINE_CHECKPOINT_INTERVAL) == 0;
    int checkpoint_count = (nlines - 1) / LINE_CHECKPOINT_INTERVAL + 1;
    if ((checkpoint &&
         reserve_checkpoints(source, checkpoint_count) < 0) ||
            (implicit_newline && reserve_implicit_lines(source, nlines) < 0) ||
            reserve_bytes(source, source->len + len + 1) < 0) {
        return -1;
    }

    _PyTok_Off start = source->len;
    memcpy(source->bytes + start, bytes, len);
    source->len += len;
    source->bytes[source->len] = '\0';
    if (checkpoint) {
        source->line_checkpoints[checkpoint_count - 1] = start;
    }
    if (implicit_newline) {
        source->implicit_lines[(nlines - 1) / 8] |=
            (unsigned char)(1U << ((nlines - 1) & 7));
    }
    source->nlines = nlines;
    return start;
}

const char *
_PyTok_SourceSpanView(const _PyTok_SourceText *source, _PyTok_Span span,
                      Py_ssize_t *len)
{
    if (!_PyTok_SpanIsValid(span) || span.end > source->len || len == NULL) {
        PyErr_SetString(PyExc_SystemError, "invalid tokenizer source span");
        return NULL;
    }
    *len = span.end - span.start;
    return source->bytes == NULL ? "" : source->bytes + span.start;
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

static int
source_ends_in_newline(const _PyTok_SourceText *source)
{
    return source->len > 0 && source->bytes[source->len - 1] == '\n';
}

static int
eof_lineno(const _PyTok_SourceText *source)
{
    if (source->nlines == 0) {
        return 1;
    }
    return source->nlines + source_ends_in_newline(source);
}

int
_PyTok_SourceLine(const _PyTok_SourceText *source, int lineno,
                  _PyTok_Line *line)
{
    if (line == NULL || lineno < 1 || lineno > eof_lineno(source)) {
        PyErr_SetString(PyExc_SystemError, "invalid tokenizer source line");
        return -1;
    }
    if (lineno > source->nlines) {
        *line = (_PyTok_Line){
            .start = source->len,
            .end = source->len,
        };
        return 0;
    }

    int checkpoint = (lineno - 1) / LINE_CHECKPOINT_INTERVAL;
    int current = checkpoint * LINE_CHECKPOINT_INTERVAL + 1;
    _PyTok_Off start = source->line_checkpoints[checkpoint];
    while (current < lineno) {
        start = _PyTok_SourceFindLineEnd(source, start);
        if (start < 0) {
            return -1;
        }
        current++;
    }
    _PyTok_Off end = source->len;
    if (lineno < source->nlines) {
        end = _PyTok_SourceFindLineEnd(source, start);
        if (end < 0) {
            return -1;
        }
    }
    *line = (_PyTok_Line){
        .start = start,
        .end = end,
        .implicit_newline = _PyTok_SourceLineIsImplicit(source, lineno),
        .contains_nul = memchr(
            source->bytes + start, 0, end - start) != NULL,
    };
    return 0;
}

int
_PyTok_SourceLocation(const _PyTok_SourceText *source, _PyTok_Off offset,
                      _PyTok_Affinity affinity, _PyTok_Loc *loc)
{
    if (offset < 0 || offset > source->len || loc == NULL ||
            (affinity != _PYTOK_AFFINITY_LEFT &&
             affinity != _PYTOK_AFFINITY_RIGHT)) {
        PyErr_SetString(PyExc_SystemError, "invalid tokenizer source offset");
        return -1;
    }
    if (source->nlines == 0 ||
            (offset == source->len && source_ends_in_newline(source) &&
             affinity == _PYTOK_AFFINITY_RIGHT)) {
        *loc = (_PyTok_Loc){eof_lineno(source), 0};
        return 0;
    }

    _PyTok_Off key = offset;
    if (affinity == _PYTOK_AFFINITY_LEFT && key > 0) {
        key--;
    }
    int low = 0;
    int high = (source->nlines - 1) / LINE_CHECKPOINT_INTERVAL + 1;
    while (low < high) {
        int middle = low + (high - low) / 2;
        if (source->line_checkpoints[middle] <= key) {
            low = middle + 1;
        }
        else {
            high = middle;
        }
    }
    int checkpoint = low - 1;
    if (checkpoint < 0) {
        PyErr_SetString(PyExc_SystemError, "corrupt tokenizer source line index");
        return -1;
    }
    int lineno = checkpoint * LINE_CHECKPOINT_INTERVAL + 1;
    _PyTok_Off start = source->line_checkpoints[checkpoint];
    while (lineno < source->nlines) {
        _PyTok_Off end = _PyTok_SourceFindLineEnd(source, start);
        if (end < 0) {
            return -1;
        }
        if (offset < end ||
                (offset == end && affinity == _PYTOK_AFFINITY_LEFT)) {
            break;
        }
        start = end;
        lineno++;
    }
    _PyTok_Off byte_col = offset - start;
    if (byte_col > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError, "tokenizer column is too large");
        return -1;
    }
    *loc = (_PyTok_Loc){lineno, (int)byte_col};
    return 0;
}
