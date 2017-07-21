// Fuzz PyUnicode_FromStringAndSize as a proxy for unicode(str).
int fuzz_builtin_unicode(const char* data, size_t size) {
    PyObject* s = PyUnicode_FromStringAndSize(data, size);
    if (PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_UnicodeDecodeError)) {
        PyErr_Clear();
    }
    Py_XDECREF(s);
    return 0;
}
