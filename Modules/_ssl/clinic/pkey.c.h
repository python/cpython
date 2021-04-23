/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_ssl_PrivateKey_from_file__doc__,
"from_file($type, /, path, *, password=None, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_PRIVATEKEY_FROM_FILE_METHODDEF    \
    {"from_file", (PyCFunction)(void(*)(void))_ssl_PrivateKey_from_file, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_PrivateKey_from_file__doc__},

static PyObject *
_ssl_PrivateKey_from_file_impl(PyTypeObject *type, PyObject *path,
                               PyObject *password, int format);

static PyObject *
_ssl_PrivateKey_from_file(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"path", "password", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "from_file", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *path;
    PyObject *password = Py_None;
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_FSConverter(args[0], &path)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        password = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    format = _PyLong_AsInt(args[2]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_PrivateKey_from_file_impl(type, path, password, format);

exit:
    return return_value;
}

PyDoc_STRVAR(_ssl_PrivateKey_from_buffer__doc__,
"from_buffer($type, /, buffer, *, password=None, format=FILETYPE_PEM)\n"
"--\n"
"\n");

#define _SSL_PRIVATEKEY_FROM_BUFFER_METHODDEF    \
    {"from_buffer", (PyCFunction)(void(*)(void))_ssl_PrivateKey_from_buffer, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, _ssl_PrivateKey_from_buffer__doc__},

static PyObject *
_ssl_PrivateKey_from_buffer_impl(PyTypeObject *type, Py_buffer *buffer,
                                 PyObject *password, int format);

static PyObject *
_ssl_PrivateKey_from_buffer(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"buffer", "password", "format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "from_buffer", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer buffer = {NULL, NULL};
    PyObject *password = Py_None;
    int format = PY_SSL_FILETYPE_PEM;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("from_buffer", "argument 'buffer'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        password = args[1];
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    format = _PyLong_AsInt(args[2]);
    if (format == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _ssl_PrivateKey_from_buffer_impl(type, &buffer, password, format);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}
/*[clinic end generated code: output=0af76c275a830b38 input=a9049054013a1b77]*/
