#include "parts.h"

#include "../../Parser/tokenizer/cursor.h"

static int
check(int condition, const char *message)
{
    if (condition) {
        return 0;
    }
    PyErr_SetString(PyExc_AssertionError, message);
    return -1;
}

static int
check_system_error(int failed, const char *message)
{
    if (!failed || !PyErr_ExceptionMatches(PyExc_SystemError)) {
        PyErr_SetString(PyExc_AssertionError, message);
        return -1;
    }
    PyErr_Clear();
    return 0;
}

static int
same_cursor(const _PyTok_Cursor *left, const _PyTok_Cursor *right)
{
    return left->source == right->source &&
        left->pos == right->pos &&
        left->line_start == right->line_start &&
        left->line_end == right->line_end &&
        left->lineno == right->lineno;
}

static PyObject *
test_tokenizer_source(PyObject *Py_UNUSED(module),
                      PyObject *Py_UNUSED(args))
{
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);

    _PyTok_Loc loc;
    _PyTok_Line line;
    if (check(_PyTok_SourceLocation(
                  &source, 0, _PYTOK_AFFINITY_RIGHT, &loc) == 0,
              "cannot locate empty source") < 0 ||
            check(loc.lineno == 1 && loc.byte_col == 0,
                  "wrong empty source location") < 0 ||
            check(_PyTok_SourceLine(&source, 1, &line) == 0,
                  "cannot find empty source line") < 0 ||
            check(line.start == 0 && line.end == 0,
                  "wrong empty source line") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "", 0, 0) < 0,
                "accepted empty source line") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "a\nb\n", 4, 0) < 0,
                "accepted multiple source lines") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "a", 1, 1) < 0,
                "accepted missing implicit newline") < 0) {
        goto error;
    }

    if (check(_PyTok_SourceAppendLine(&source, "alpha\n", 6, 0) == 0,
              "wrong first source offset") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, "\xce\xb2\n", 3, 1) == 6,
                  "wrong second source offset") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, "nul\0x\n", 6, 0) == 9,
                  "wrong third source offset") < 0) {
        goto error;
    }

    int marker_line = 257;
    int final_line = 300;
    _PyTok_Off marker_start = -1;
    for (int lineno = 4; lineno <= final_line; lineno++) {
        const char *text = lineno == marker_line ? "marker\n" : "x\n";
        Py_ssize_t len = (Py_ssize_t)strlen(text);
        _PyTok_Off start = _PyTok_SourceAppendLine(
            &source, text, len, lineno == final_line);
        if (start < 0) {
            goto error;
        }
        if (lineno == marker_line) {
            marker_start = start;
        }
    }

    if (check(source.nlines == final_line, "wrong source line count") < 0 ||
            check(_PyTok_SourceLine(&source, marker_line, &line) == 0,
                  "cannot find late source line") < 0 ||
            check(line.start == marker_start &&
                      line.end == marker_start + 7,
                  "wrong late source line") < 0 ||
            check(!line.implicit_newline && !line.contains_nul,
                  "wrong late source flags") < 0 ||
            check(_PyTok_SourceLine(&source, 2, &line) == 0,
                  "cannot find second source line") < 0 ||
            check(line.start == 6 && line.end == 9 &&
                      line.implicit_newline && !line.contains_nul,
                  "wrong second source line") < 0 ||
            check(!_PyTok_SourceLineIsImplicit(&source, 1) &&
                      _PyTok_SourceLineIsImplicit(&source, 2),
                  "wrong early implicit newline flags") < 0 ||
            check(_PyTok_SourceLine(&source, 3, &line) == 0,
                  "cannot find third source line") < 0 ||
            check(line.contains_nul, "missing null byte flag") < 0 ||
            check(_PyTok_SourceLine(&source, final_line, &line) == 0,
                  "cannot find final source line") < 0 ||
            check(line.implicit_newline &&
                      _PyTok_SourceLineIsImplicit(&source, final_line),
                  "missing late implicit newline flag") < 0) {
        goto error;
    }

    Py_ssize_t view_len;
    const char *view = _PyTok_SourceSpanView(
        &source, _PyTok_SpanFromBounds(6, 8), &view_len);
    if (check(view != NULL && view_len == 2 &&
                  memcmp(view, "\xce\xb2", 2) == 0,
              "wrong source span view") < 0 ||
            check(_PyTok_SourceLocation(
                      &source, marker_start,
                      _PYTOK_AFFINITY_LEFT, &loc) == 0,
                  "cannot locate left line boundary") < 0 ||
            check(loc.lineno == marker_line - 1 && loc.byte_col == 2,
                  "wrong left boundary location") < 0 ||
            check(_PyTok_SourceLocation(
                      &source, marker_start,
                      _PYTOK_AFFINITY_RIGHT, &loc) == 0,
                  "cannot locate right line boundary") < 0 ||
            check(loc.lineno == marker_line && loc.byte_col == 0,
                  "wrong right boundary location") < 0 ||
            check(_PyTok_SourceLocation(
                      &source, marker_start + 1,
                      _PYTOK_AFFINITY_RIGHT, &loc) == 0,
                  "cannot locate late source byte") < 0 ||
            check(loc.lineno == marker_line && loc.byte_col == 1,
                  "wrong late source location") < 0) {
        goto error;
    }

    if (check(_PyTok_SourceLocation(
                  &source, source.len, _PYTOK_AFFINITY_LEFT, &loc) == 0,
              "cannot locate left EOF") < 0 ||
            check(loc.lineno == final_line && loc.byte_col == 2,
                  "wrong left EOF location") < 0 ||
            check(_PyTok_SourceLocation(
                      &source, source.len,
                      _PYTOK_AFFINITY_RIGHT, &loc) == 0,
                  "cannot locate right EOF") < 0 ||
            check(loc.lineno == final_line + 1 && loc.byte_col == 0,
                  "wrong right EOF location") < 0 ||
            check(_PyTok_SourceLine(&source, final_line + 1, &line) == 0,
                  "cannot find virtual EOF line") < 0 ||
            check(line.start == source.len && line.end == source.len,
                  "wrong virtual EOF line") < 0 ||
            check(!_PyTok_SourceLineIsImplicit(&source, 0) &&
                      !_PyTok_SourceLineIsImplicit(
                          &source, final_line + 1),
                  "virtual or invalid line is implicit") < 0) {
        goto error;
    }

    view = _PyTok_SourceSpanView(
        &source, _PyTok_SpanFromBounds(0, source.len + 1), &view_len);
    if (check_system_error(view == NULL, "accepted invalid source span") < 0 ||
            check_system_error(
                _PyTok_SourceLocation(
                    &source, source.len + 1,
                    _PYTOK_AFFINITY_RIGHT, &loc) < 0,
                "accepted invalid source offset") < 0 ||
            check_system_error(
                _PyTok_SourceLine(&source, final_line + 2, &line) < 0,
                "accepted invalid source line") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    _PyTok_SourceInit(&source);
    if (_PyTok_SourceAppendLine(&source, "tail", 4, 0) < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "x\n", 2, 0) < 0,
                "appended after unterminated source line") < 0 ||
            check(_PyTok_SourceLocation(
                      &source, source.len,
                      _PYTOK_AFFINITY_RIGHT, &loc) == 0,
                  "cannot locate unterminated EOF") < 0 ||
            check(loc.lineno == 1 && loc.byte_col == 4,
                  "wrong unterminated EOF location") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    Py_RETURN_NONE;

error:
    _PyTok_SourceClear(&source);
    return NULL;
}

static PyObject *
test_tokenizer_cursor(PyObject *Py_UNUSED(module),
                      PyObject *Py_UNUSED(args))
{
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);
    if (_PyTok_SourceAppendLine(&source, "ab\n", 3, 0) < 0 ||
            _PyTok_SourceAppendLine(&source, "cd\n", 3, 0) < 0) {
        goto error;
    }

    _PyTok_Cursor cursor;
    _PyTok_CursorInit(&cursor, &source);
    if (_PyTok_CursorSetOffset(&cursor, source.len) < 0 ||
            check(cursor.lineno == 3 && cursor.pos == source.len,
                  "wrong cursor at virtual EOF") < 0 ||
            _PyTok_CursorSetLine(&cursor, 1) < 0) {
        goto error;
    }

    char large[BUFSIZ + 1];
    memset(large, 'z', sizeof(large));
    large[sizeof(large) - 1] = '\n';
    if (_PyTok_SourceAppendLine(&source, large, sizeof(large), 0) < 0) {
        goto error;
    }

    if (check(_PyTok_CursorPeek(&cursor, 0) == 'a',
              "wrong cursor peek after relocation") < 0 ||
            check(_PyTok_CursorPeek(&cursor, 1) == 'b',
                  "wrong distant cursor peek") < 0 ||
            check(_PyTok_CursorAdvance(&cursor) == 'a',
                  "wrong first cursor byte") < 0 ||
            check(_PyTok_CursorAdvance(&cursor) == 'b',
                  "wrong second cursor byte") < 0 ||
            check(_PyTok_CursorAdvance(&cursor) == '\n',
                  "wrong final cursor byte") < 0 ||
            check(_PyTok_CursorAdvance(&cursor) == EOF,
                  "cursor advanced past line") < 0 ||
            check(_PyTok_CursorSetOffset(&cursor, 2) == 0,
                  "cannot seek cursor offset") < 0 ||
            check(_PyTok_CursorAdvance(&cursor) == '\n',
                  "wrong cursor byte after seek") < 0 ||
            check(_PyTok_CursorSetOffset(&cursor, 3) == 0,
                  "cannot seek line boundary") < 0 ||
            check(cursor.lineno == 2 && cursor.line_start == 3 &&
                      _PyTok_CursorAdvance(&cursor) == 'c',
                  "wrong cursor at line boundary") < 0 ||
            check(_PyTok_CursorSetLine(&cursor, 3) == 0,
                  "cannot advance cursor to final line") < 0 ||
            check(cursor.line_start == 6 &&
                      _PyTok_CursorAdvance(&cursor) == 'z',
                  "wrong cursor byte on final line") < 0) {
        goto error;
    }

    _PyTok_Cursor saved = cursor;
    if (check_system_error(
            _PyTok_CursorSetOffset(&cursor, source.len + 1) < 0,
            "accepted invalid cursor offset") < 0 ||
            check(same_cursor(&cursor, &saved),
                  "invalid offset changed cursor") < 0 ||
            check_system_error(
                _PyTok_CursorSetLine(&cursor, source.nlines + 2) < 0,
                "accepted invalid cursor line") < 0 ||
            check(same_cursor(&cursor, &saved),
                  "invalid line changed cursor") < 0 ||
            check(_PyTok_CursorSetOffset(&cursor, source.len) == 0,
                  "cannot set cursor to EOF") < 0 ||
            check(cursor.lineno == 4 && cursor.pos == source.len,
                  "wrong cursor at EOF") < 0) {
        goto error;
    }

#if SIZEOF_VOID_P > 4
    char byte = 0;
    _PyTok_SourceText huge_source = {
        .bytes = &byte,
        .len = (_PyTok_Off)INT_MAX + 1,
    };
    _PyTok_Cursor huge_cursor = {
        .source = &huge_source,
        .pos = INT_MAX,
        .line_end = (_PyTok_Off)INT_MAX + 1,
        .lineno = 1,
    };
    if (check(_PyTok_CursorAdvance(&huge_cursor) == EOF &&
                  huge_cursor.pos == INT_MAX,
              "cursor advanced past maximum column") < 0) {
        goto error;
    }
#endif

    _PyTok_SourceClear(&source);
    Py_RETURN_NONE;

error:
    _PyTok_SourceClear(&source);
    return NULL;
}

static PyMethodDef test_methods[] = {
    {"test_tokenizer_source", test_tokenizer_source, METH_NOARGS},
    {"test_tokenizer_cursor", test_tokenizer_cursor, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Tokenizer(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
