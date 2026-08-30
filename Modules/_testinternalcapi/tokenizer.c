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

static PyObject *
test_tokenizer_source(PyObject *Py_UNUSED(module),
                      PyObject *Py_UNUSED(args))
{
    _PyTok_SourceText source;
    _PyTok_SourceInit(&source);

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

    Py_ssize_t view_len;
    const char *view = _PyTok_SourceSpanView(
        &source, _PyTok_SpanFromBounds(6, 8), &view_len);
    if (check(view != NULL && view_len == 2 &&
                  memcmp(view, "\xce\xb2", 2) == 0,
              "wrong source span view") < 0 ||
            check_system_error(
                _PyTok_SourceSpanView(
                    &source, _PyTok_SpanFromBounds(0, source.len + 1),
                    &view_len) == NULL,
                "accepted invalid source span") < 0) {
        goto error;
    }

    _PyTok_SourceClear(&source);
    if (_PyTok_SourceAppendLine(&source, "tail", 4, 0) < 0 ||
            check_system_error(
                _PyTok_SourceAppendLine(&source, "x\n", 2, 0) < 0,
                "appended after unterminated source line") < 0) {
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
    {NULL},
};

int
_PyTestInternalCapi_Init_Tokenizer(PyObject *module)
{
    return PyModule_AddFunctions(module, test_methods);
}
