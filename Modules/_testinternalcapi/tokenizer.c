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
    const char multiple_lines[] = "a\nb\n";
    const char first_line[] = "alpha\n";
    const char second_line[] = "\xce\xb2\n";
    const char expected[] = "alpha\n\xce\xb2\n";
    const char tail[] = "tail";
    const char terminated_line[] = "x\n";
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);

    if (check_line_view(&source, 1, "") < 0) {
        goto error;
    }

    if (check_system_error(
            _PyTok_SourceAppendLine(&source, "", 0) < 0,
            "accepted empty source line") < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(
                    &source, multiple_lines, sizeof(multiple_lines) - 1) < 0,
                "accepted multiple source lines") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, first_line, sizeof(first_line) - 1) == 0,
                  "wrong first source offset") < 0 ||
            check(_PyTok_SourceAppendLine(
                      &source, second_line, sizeof(second_line) - 1) ==
                      (Py_ssize_t)sizeof(first_line) - 1,
                  "wrong second source offset") < 0) {
        goto error;
    }

    if (check_line_view(&source, PY_SSIZE_T_MIN, "alpha") < 0 ||
            check_line_view(&source, 1, "alpha") < 0 ||
            check_line_view(&source, 2, "\xce\xb2") < 0 ||
            check_line_view(&source, 3, "") < 0 ||
            check_line_view(&source, PY_SSIZE_T_MAX, "") < 0) {
        goto error;
    }

    if (check(source.len == (Py_ssize_t)sizeof(expected) - 1 &&
                  memcmp(source.bytes, expected, sizeof(expected)) == 0,
              "wrong source contents") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    if (_PyTok_SourceAppendLine(&source, tail, sizeof(tail) - 1) < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(
                    &source, terminated_line, sizeof(terminated_line) - 1) < 0,
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
    enum { LINE_COUNT = 260 };
    const char first_line[] = "x\n";
    const char second_line[] = "y\n";
    const char tail[] = "tail";
    const char final_line[] = "z\n";
    const _PyTok_Off first_batch_len = LINE_COUNT * (sizeof(first_line) - 1);
    const _PyTok_Off second_batch_len = LINE_COUNT * (sizeof(second_line) - 1);
    const _PyTok_Off discarded_len = first_batch_len + second_batch_len;
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);
    for (int i = 0; i < LINE_COUNT; i++) {
        if (_PyTok_SourceAppendLine(&source, first_line, sizeof(first_line) - 1) < 0) {
            goto error;
        }
    }
    char *bytes = source.bytes;
    _PyTok_Off capacity = source.cap;
    _PyTok_SourceDiscard(&source);
    if (check(source.base_offset == first_batch_len && source.len == 0 &&
                  source.nlines == 0 && source.bytes == bytes &&
                  source.cap == capacity && source.bytes[0] == '\0',
              "discard did not preserve source allocation") < 0) {
        goto error;
    }
    for (int i = 0; i < LINE_COUNT; i++) {
        if (check(_PyTok_SourceAppendLine(
                      &source, second_line, sizeof(second_line) - 1) ==
                      first_batch_len + ((Py_ssize_t)sizeof(second_line) - 1) * i,
                  "wrong source offset after discard") < 0) {
            goto error;
        }
    }
    if (check(source.bytes == bytes && source.cap == capacity,
              "discarded allocation was not reused") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    if (check(_PyTok_SourceAppendLine(
                  &source, tail, sizeof(tail) - 1) == discarded_len,
              "wrong source offset after repeated discard") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    if (check(_PyTok_SourceAppendLine(
                  &source, final_line, sizeof(final_line) - 1) ==
                  discarded_len + (Py_ssize_t)sizeof(tail) - 1,
              "cannot append after discarding unterminated line") < 0) {
        goto error;
    }
    _PyTok_SourceDiscard(&source);
    source.base_offset = PY_SSIZE_T_MAX - 1;
    if (check(_PyTok_SourceAppendLine(
                  &source, final_line, sizeof(final_line) - 1) < 0 &&
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
