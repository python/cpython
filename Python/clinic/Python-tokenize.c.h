/*[clinic input]
preserve
[clinic start generated code]*/

static PyObject *
tokenizeriter_new_impl(PyTypeObject *type, const char *source);

static PyObject *
tokenizeriter_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"source", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "tokenizeriter", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    const char *source;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("tokenizeriter", "argument 'source'", "str", fastargs[0]);
        goto exit;
    }
    Py_ssize_t source_length;
    source = PyUnicode_AsUTF8AndSize(fastargs[0], &source_length);
    if (source == NULL) {
        goto exit;
    }
    if (strlen(source) != (size_t)source_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = tokenizeriter_new_impl(type, source);

exit:
    return return_value;
}
/*[clinic end generated code: output=dfcd64774e01bfe6 input=a9049054013a1b77]*/
