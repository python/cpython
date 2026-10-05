#include "parts.h"

#include "../../Parser/tokenizer/source.h"

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
check_line_view(const _PyTok_SourceText *source, Py_ssize_t lineno,
                const char *expected)
{
    Py_ssize_t len;
    const char *line = _PyTok_SourceLineView(source, lineno, &len);
    return check(len == (Py_ssize_t)strlen(expected) &&
                 memcmp(line, expected, len) == 0,
                 "wrong source line view");
}

static PyObject *
test_tokenizer_source(PyObject *Py_UNUSED(module),
                      PyObject *Py_UNUSED(args))
{
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);

    if (check_line_view(&source, 1, "") < 0) {
        goto error;
    }

    if (check_system_error(
            _PyTok_SourceAppendLine(&source, "", 0, 0) < 0,
            "accepted empty source line") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "a\nb\n", 4, 0) < 0,
                "accepted multiple source lines") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "a", 1, 1) < 0,
                "accepted missing implicit newline") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, "alpha\n", 6, 0) == 0,
                  "wrong first source offset") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, "\xce\xb2\n", 3, 1) == 6,
                  "wrong second source offset") < 0 ||
            check(!_PyTok_SourceLineIsImplicit(&source, 1) &&
                      _PyTok_SourceLineIsImplicit(&source, 2),
                  "wrong implicit newline flags") < 0) {
        goto error;
    }

    if (check_line_view(&source, PY_SSIZE_T_MIN, "alpha") < 0 ||
            check_line_view(&source, 1, "alpha") < 0 ||
            check_line_view(&source, 2, "\xce\xb2") < 0 ||
            check_line_view(&source, 3, "") < 0 ||
            check_line_view(&source, PY_SSIZE_T_MAX, "") < 0) {
        goto error;
    }

    if (check(source.len == 9 &&
                  memcmp(source.bytes, "alpha\n\xce\xb2\n", 10) == 0,
              "wrong source contents") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    if (_PyTok_SourceAppendLine(&source, "tail", 4, 0) < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "x\n", 2, 0) < 0,
                "appended after unterminated source line") < 0) {
        goto error;
    }
    if (check_line_view(&source, 1, "tail") < 0 ||
            check_line_view(&source, PY_SSIZE_T_MAX, "tail") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    Py_RETURN_NONE;

error:
    _PyTok_SourceClear(&source);
    return NULL;
}

static PyObject *
test_tokenizer_source_discard(PyObject *Py_UNUSED(module),
                             PyObject *Py_UNUSED(args))
{
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);
    for (int i = 0; i < 260; i++) {
        if (_PyTok_SourceAppendLine(&source, "x\n", 2, 1) < 0) {
            goto error;
        }
    }
    char *bytes = source.bytes;
    _PyTok_Off capacity = source.cap;
    _PyTok_SourceDiscard(&source);
    if (check(source.base_offset == 520 && source.len == 0 &&
                  source.nlines == 0 && source.bytes == bytes &&
                  source.cap == capacity && source.bytes[0] == '\0',
              "discard did not preserve source allocation") < 0) {
        goto error;
    }
    for (int i = 0; i < 260; i++) {
        if (check(_PyTok_SourceAppendLine(&source, "y\n", 2, 0) == 520 + 2 * i,
                  "wrong source offset after discard") < 0 ||
                check(!_PyTok_SourceLineIsImplicit(&source, i + 1),
                      "discard preserved implicit newline flag") < 0) {
            goto error;
        }
    }
    if (check(source.bytes == bytes && source.cap == capacity,
              "discarded allocation was not reused") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    if (check(_PyTok_SourceAppendLine(&source, "tail", 4, 0) == 1040,
              "wrong source offset after repeated discard") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    if (check(_PyTok_SourceAppendLine(&source, "z\n", 2, 0) == 1044,
              "cannot append after discarding unterminated line") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    source.base_offset = PY_SSIZE_T_MAX - 1;
    if (check(_PyTok_SourceAppendLine(&source, "z\n", 2, 0) < 0 &&
                  PyErr_ExceptionMatches(PyExc_MemoryError),
              "accepted overflowing logical source offset") < 0) {
        goto error;
    }
    PyErr_Clear();
    if (check(source.len == 0 && source.nlines == 0 &&
                  source.base_offset == PY_SSIZE_T_MAX - 1,
              "overflow changed retained source") < 0) {
        goto error;
    }
    _PyTok_SourceClear(&source);
    Py_RETURN_NONE;

error:
    _PyTok_SourceClear(&source);
    return NULL;
}

static PyMethodDef test_methods[] = {
    {"test_tokenizer_source", test_tokenizer_source, METH_NOARGS},
    {"test_tokenizer_source_discard", test_tokenizer_source_discard, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_Tokenizer(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
