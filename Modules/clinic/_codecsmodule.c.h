/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_codecs_register__doc__,
"register($module, search_function, /)\n"
"--\n"
"\n"
"Register a codec search function.\n"
"\n"
"Search functions are expected to take one argument, the encoding name in\n"
"all lower case letters, and either return None, or a tuple of functions\n"
"(encoder, decoder, stream_reader, stream_writer) (or a CodecInfo object).");

#define _CODECS_REGISTER_METHODDEF    \
    {"register", (PyCFunction)_codecs_register, METH_O, _codecs_register__doc__},

PyDoc_STRVAR(_codecs_unregister__doc__,
"unregister($module, search_function, /)\n"
"--\n"
"\n"
"Unregister a codec search function and clear the registry\'s cache.\n"
"\n"
"If the search function is not registered, do nothing.");

#define _CODECS_UNREGISTER_METHODDEF    \
    {"unregister", (PyCFunction)_codecs_unregister, METH_O, _codecs_unregister__doc__},

PyDoc_STRVAR(_codecs_lookup__doc__,
"lookup($module, encoding, /)\n"
"--\n"
"\n"
"Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.");

#define _CODECS_LOOKUP_METHODDEF    \
    {"lookup", (PyCFunction)_codecs_lookup, METH_O, _codecs_lookup__doc__},

static PyObject *
_codecs_lookup_impl(PyObject *module, const char *encoding);

static PyObject *
_codecs_lookup(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *encoding;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("lookup", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t encoding_length;
    encoding = PyUnicode_AsUTF8AndSize(arg, &encoding_length);
    if (encoding == NULL) {
        goto exit;
    }
    if (strlen(encoding) != (size_t)encoding_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _codecs_lookup_impl(module, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_encode__doc__,
"encode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Encodes obj using the codec registered for encoding.\n"
"\n"
"The default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that encoding\n"
"errors raise a ValueError.  Other possible values are \'ignore\', \'replace\'\n"
"and \'backslashreplace\' as well as any other name registered with\n"
"codecs.register_error that can handle ValueErrors.");

#define _CODECS_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)(void(*)(void))_codecs_encode, METH_FASTCALL|METH_KEYWORDS, _codecs_encode__doc__},

static PyObject *
_codecs_encode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors);

static PyObject *
_codecs_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "encode", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (!PyUnicode_Check(args[1])) {
            _PyArg_BadArgument("encode", "argument 'encoding'", "str", args[1]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(args[1], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("encode", "argument 'errors'", "str", args[2]);
        goto exit;
    }
    Py_ssize_t errors_length;
    errors = PyUnicode_AsUTF8AndSize(args[2], &errors_length);
    if (errors == NULL) {
        goto exit;
    }
    if (strlen(errors) != (size_t)errors_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = _codecs_encode_impl(module, obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_decode__doc__,
"decode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decodes obj using the codec registered for encoding.\n"
"\n"
"Default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that encoding\n"
"errors raise a ValueError.  Other possible values are \'ignore\', \'replace\'\n"
"and \'backslashreplace\' as well as any other name registered with\n"
"codecs.register_error that can handle ValueErrors.");

#define _CODECS_DECODE_METHODDEF    \
    {"decode", (PyCFunction)(void(*)(void))_codecs_decode, METH_FASTCALL|METH_KEYWORDS, _codecs_decode__doc__},

static PyObject *
_codecs_decode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors);

static PyObject *
_codecs_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decode", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    obj = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (!PyUnicode_Check(args[1])) {
            _PyArg_BadArgument("decode", "argument 'encoding'", "str", args[1]);
            goto exit;
        }
        Py_ssize_t encoding_length;
        encoding = PyUnicode_AsUTF8AndSize(args[1], &encoding_length);
        if (encoding == NULL) {
            goto exit;
        }
        if (strlen(encoding) != (size_t)encoding_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(args[2])) {
        _PyArg_BadArgument("decode", "argument 'errors'", "str", args[2]);
        goto exit;
    }
    Py_ssize_t errors_length;
    errors = PyUnicode_AsUTF8AndSize(args[2], &errors_length);
    if (errors == NULL) {
        goto exit;
    }
    if (strlen(errors) != (size_t)errors_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = _codecs_decode_impl(module, obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_escape_decode__doc__,
"escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_DECODE_METHODDEF    \
    {"escape_decode", (PyCFunction)(void(*)(void))_codecs_escape_decode, METH_FASTCALL, _codecs_escape_decode__doc__},

static PyObject *
_codecs_escape_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors);

static PyObject *
_codecs_escape_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("escape_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        PyBuffer_FillInfo(&data, args[0], (void *)ptr, len, 1, 0);
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&data, 'C')) {
            _PyArg_BadArgument("escape_decode", "argument 1", "contiguous buffer", args[0]);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("escape_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_escape_encode__doc__,
"escape_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_ENCODE_METHODDEF    \
    {"escape_encode", (PyCFunction)(void(*)(void))_codecs_escape_encode, METH_FASTCALL, _codecs_escape_encode__doc__},

static PyObject *
_codecs_escape_encode_impl(PyObject *module, PyObject *data,
                           const char *errors);

static PyObject *
_codecs_escape_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *data;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyBytes_Check(args[0])) {
        _PyArg_BadArgument("escape_encode", "argument 1", "bytes", args[0]);
        goto exit;
    }
    data = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("escape_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_escape_encode_impl(module, data, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_decode__doc__,
"utf_7_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_DECODE_METHODDEF    \
    {"utf_7_decode", (PyCFunction)(void(*)(void))_codecs_utf_7_decode, METH_FASTCALL, _codecs_utf_7_decode__doc__},

static PyObject *
_codecs_utf_7_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyObject *
_codecs_utf_7_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_7_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_7_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_7_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_7_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_decode__doc__,
"utf_8_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_DECODE_METHODDEF    \
    {"utf_8_decode", (PyCFunction)(void(*)(void))_codecs_utf_8_decode, METH_FASTCALL, _codecs_utf_8_decode__doc__},

static PyObject *
_codecs_utf_8_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyObject *
_codecs_utf_8_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_8_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_8_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_8_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_8_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_decode__doc__,
"utf_16_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_DECODE_METHODDEF    \
    {"utf_16_decode", (PyCFunction)(void(*)(void))_codecs_utf_16_decode, METH_FASTCALL, _codecs_utf_16_decode__doc__},

static PyObject *
_codecs_utf_16_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyObject *
_codecs_utf_16_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_16_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_decode__doc__,
"utf_16_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_DECODE_METHODDEF    \
    {"utf_16_le_decode", (PyCFunction)(void(*)(void))_codecs_utf_16_le_decode, METH_FASTCALL, _codecs_utf_16_le_decode__doc__},

static PyObject *
_codecs_utf_16_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_16_le_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_le_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_16_le_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_le_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_le_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_decode__doc__,
"utf_16_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_DECODE_METHODDEF    \
    {"utf_16_be_decode", (PyCFunction)(void(*)(void))_codecs_utf_16_be_decode, METH_FASTCALL, _codecs_utf_16_be_decode__doc__},

static PyObject *
_codecs_utf_16_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_16_be_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_be_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_16_be_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_be_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_be_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_ex_decode__doc__,
"utf_16_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_EX_DECODE_METHODDEF    \
    {"utf_16_ex_decode", (PyCFunction)(void(*)(void))_codecs_utf_16_ex_decode, METH_FASTCALL, _codecs_utf_16_ex_decode__doc__},

static PyObject *
_codecs_utf_16_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyObject *
_codecs_utf_16_ex_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_16_ex_decode", nargs, 1, 4)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_16_ex_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_ex_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyLong_AsInt(args[2]);
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[3]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_ex_decode_impl(module, &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_decode__doc__,
"utf_32_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_DECODE_METHODDEF    \
    {"utf_32_decode", (PyCFunction)(void(*)(void))_codecs_utf_32_decode, METH_FASTCALL, _codecs_utf_32_decode__doc__},

static PyObject *
_codecs_utf_32_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyObject *
_codecs_utf_32_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_32_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_decode__doc__,
"utf_32_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_DECODE_METHODDEF    \
    {"utf_32_le_decode", (PyCFunction)(void(*)(void))_codecs_utf_32_le_decode, METH_FASTCALL, _codecs_utf_32_le_decode__doc__},

static PyObject *
_codecs_utf_32_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_32_le_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_le_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_32_le_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_le_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_le_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_decode__doc__,
"utf_32_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_DECODE_METHODDEF    \
    {"utf_32_be_decode", (PyCFunction)(void(*)(void))_codecs_utf_32_be_decode, METH_FASTCALL, _codecs_utf_32_be_decode__doc__},

static PyObject *
_codecs_utf_32_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_32_be_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_be_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_32_be_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_be_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_be_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_ex_decode__doc__,
"utf_32_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_EX_DECODE_METHODDEF    \
    {"utf_32_ex_decode", (PyCFunction)(void(*)(void))_codecs_utf_32_ex_decode, METH_FASTCALL, _codecs_utf_32_ex_decode__doc__},

static PyObject *
_codecs_utf_32_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyObject *
_codecs_utf_32_ex_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!_PyArg_CheckPositional("utf_32_ex_decode", nargs, 1, 4)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("utf_32_ex_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_ex_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyLong_AsInt(args[2]);
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[3]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_ex_decode_impl(module, &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_decode__doc__,
"unicode_escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"unicode_escape_decode", (PyCFunction)(void(*)(void))_codecs_unicode_escape_decode, METH_FASTCALL, _codecs_unicode_escape_decode__doc__},

static PyObject *
_codecs_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                   const char *errors);

static PyObject *
_codecs_unicode_escape_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("unicode_escape_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        PyBuffer_FillInfo(&data, args[0], (void *)ptr, len, 1, 0);
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&data, 'C')) {
            _PyArg_BadArgument("unicode_escape_decode", "argument 1", "contiguous buffer", args[0]);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("unicode_escape_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_unicode_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_decode__doc__,
"raw_unicode_escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"raw_unicode_escape_decode", (PyCFunction)(void(*)(void))_codecs_raw_unicode_escape_decode, METH_FASTCALL, _codecs_raw_unicode_escape_decode__doc__},

static PyObject *
_codecs_raw_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                       const char *errors);

static PyObject *
_codecs_raw_unicode_escape_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("raw_unicode_escape_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        PyBuffer_FillInfo(&data, args[0], (void *)ptr, len, 1, 0);
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&data, 'C')) {
            _PyArg_BadArgument("raw_unicode_escape_decode", "argument 1", "contiguous buffer", args[0]);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("raw_unicode_escape_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_raw_unicode_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_decode__doc__,
"latin_1_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_DECODE_METHODDEF    \
    {"latin_1_decode", (PyCFunction)(void(*)(void))_codecs_latin_1_decode, METH_FASTCALL, _codecs_latin_1_decode__doc__},

static PyObject *
_codecs_latin_1_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors);

static PyObject *
_codecs_latin_1_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("latin_1_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("latin_1_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("latin_1_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_latin_1_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_decode__doc__,
"ascii_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_DECODE_METHODDEF    \
    {"ascii_decode", (PyCFunction)(void(*)(void))_codecs_ascii_decode, METH_FASTCALL, _codecs_ascii_decode__doc__},

static PyObject *
_codecs_ascii_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors);

static PyObject *
_codecs_ascii_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("ascii_decode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("ascii_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ascii_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_ascii_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_decode__doc__,
"charmap_decode($module, data, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_DECODE_METHODDEF    \
    {"charmap_decode", (PyCFunction)(void(*)(void))_codecs_charmap_decode, METH_FASTCALL, _codecs_charmap_decode__doc__},

static PyObject *
_codecs_charmap_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors, PyObject *mapping);

static PyObject *
_codecs_charmap_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    PyObject *mapping = Py_None;

    if (!_PyArg_CheckPositional("charmap_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("charmap_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("charmap_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mapping = args[2];
skip_optional:
    return_value = _codecs_charmap_decode_impl(module, &data, errors, mapping);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_decode__doc__,
"mbcs_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_DECODE_METHODDEF    \
    {"mbcs_decode", (PyCFunction)(void(*)(void))_codecs_mbcs_decode, METH_FASTCALL, _codecs_mbcs_decode__doc__},

static PyObject *
_codecs_mbcs_decode_impl(PyObject *module, Py_buffer *data,
                         const char *errors, int final);

static PyObject *
_codecs_mbcs_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("mbcs_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("mbcs_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("mbcs_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_mbcs_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_decode__doc__,
"oem_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_DECODE_METHODDEF    \
    {"oem_decode", (PyCFunction)(void(*)(void))_codecs_oem_decode, METH_FASTCALL, _codecs_oem_decode__doc__},

static PyObject *
_codecs_oem_decode_impl(PyObject *module, Py_buffer *data,
                        const char *errors, int final);

static PyObject *
_codecs_oem_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("oem_decode", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("oem_decode", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("oem_decode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[2]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_oem_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_decode__doc__,
"code_page_decode($module, codepage, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_DECODE_METHODDEF    \
    {"code_page_decode", (PyCFunction)(void(*)(void))_codecs_code_page_decode, METH_FASTCALL, _codecs_code_page_decode__doc__},

static PyObject *
_codecs_code_page_decode_impl(PyObject *module, int codepage,
                              Py_buffer *data, const char *errors, int final);

static PyObject *
_codecs_code_page_decode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int codepage;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!_PyArg_CheckPositional("code_page_decode", nargs, 2, 4)) {
        goto exit;
    }
    codepage = _PyLong_AsInt(args[0]);
    if (codepage == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("code_page_decode", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    if (args[2] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[2])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[2], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("code_page_decode", "argument 3", "str or None", args[2]);
        goto exit;
    }
    if (nargs < 4) {
        goto skip_optional;
    }
    final = _PyLong_AsInt(args[3]);
    if (final == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_code_page_decode_impl(module, codepage, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(_codecs_readbuffer_encode__doc__,
"readbuffer_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_READBUFFER_ENCODE_METHODDEF    \
    {"readbuffer_encode", (PyCFunction)(void(*)(void))_codecs_readbuffer_encode, METH_FASTCALL, _codecs_readbuffer_encode__doc__},

static PyObject *
_codecs_readbuffer_encode_impl(PyObject *module, Py_buffer *data,
                               const char *errors);

static PyObject *
_codecs_readbuffer_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("readbuffer_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (PyUnicode_Check(args[0])) {
        Py_ssize_t len;
        const char *ptr = PyUnicode_AsUTF8AndSize(args[0], &len);
        if (ptr == NULL) {
            goto exit;
        }
        PyBuffer_FillInfo(&data, args[0], (void *)ptr, len, 1, 0);
    }
    else { /* any bytes-like object */
        if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
            goto exit;
        }
        if (!PyBuffer_IsContiguous(&data, 'C')) {
            _PyArg_BadArgument("readbuffer_encode", "argument 1", "contiguous buffer", args[0]);
            goto exit;
        }
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("readbuffer_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_readbuffer_encode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_encode__doc__,
"utf_7_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_ENCODE_METHODDEF    \
    {"utf_7_encode", (PyCFunction)(void(*)(void))_codecs_utf_7_encode, METH_FASTCALL, _codecs_utf_7_encode__doc__},

static PyObject *
_codecs_utf_7_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_utf_7_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_7_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_7_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_7_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_7_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_encode__doc__,
"utf_8_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_ENCODE_METHODDEF    \
    {"utf_8_encode", (PyCFunction)(void(*)(void))_codecs_utf_8_encode, METH_FASTCALL, _codecs_utf_8_encode__doc__},

static PyObject *
_codecs_utf_8_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_utf_8_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_8_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_8_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_8_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_8_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_encode__doc__,
"utf_16_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_ENCODE_METHODDEF    \
    {"utf_16_encode", (PyCFunction)(void(*)(void))_codecs_utf_16_encode, METH_FASTCALL, _codecs_utf_16_encode__doc__},

static PyObject *
_codecs_utf_16_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyObject *
_codecs_utf_16_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!_PyArg_CheckPositional("utf_16_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_16_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyLong_AsInt(args[2]);
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_encode_impl(module, str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_encode__doc__,
"utf_16_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_ENCODE_METHODDEF    \
    {"utf_16_le_encode", (PyCFunction)(void(*)(void))_codecs_utf_16_le_encode, METH_FASTCALL, _codecs_utf_16_le_encode__doc__},

static PyObject *
_codecs_utf_16_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_16_le_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_16_le_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_16_le_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_le_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_le_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_encode__doc__,
"utf_16_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_ENCODE_METHODDEF    \
    {"utf_16_be_encode", (PyCFunction)(void(*)(void))_codecs_utf_16_be_encode, METH_FASTCALL, _codecs_utf_16_be_encode__doc__},

static PyObject *
_codecs_utf_16_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_16_be_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_16_be_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_16_be_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_16_be_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_16_be_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_encode__doc__,
"utf_32_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_ENCODE_METHODDEF    \
    {"utf_32_encode", (PyCFunction)(void(*)(void))_codecs_utf_32_encode, METH_FASTCALL, _codecs_utf_32_encode__doc__},

static PyObject *
_codecs_utf_32_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyObject *
_codecs_utf_32_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!_PyArg_CheckPositional("utf_32_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_32_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    byteorder = _PyLong_AsInt(args[2]);
    if (byteorder == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_encode_impl(module, str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_encode__doc__,
"utf_32_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_ENCODE_METHODDEF    \
    {"utf_32_le_encode", (PyCFunction)(void(*)(void))_codecs_utf_32_le_encode, METH_FASTCALL, _codecs_utf_32_le_encode__doc__},

static PyObject *
_codecs_utf_32_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_32_le_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_32_le_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_32_le_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_le_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_le_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_encode__doc__,
"utf_32_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_ENCODE_METHODDEF    \
    {"utf_32_be_encode", (PyCFunction)(void(*)(void))_codecs_utf_32_be_encode, METH_FASTCALL, _codecs_utf_32_be_encode__doc__},

static PyObject *
_codecs_utf_32_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_32_be_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("utf_32_be_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("utf_32_be_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("utf_32_be_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_utf_32_be_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_encode__doc__,
"unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"unicode_escape_encode", (PyCFunction)(void(*)(void))_codecs_unicode_escape_encode, METH_FASTCALL, _codecs_unicode_escape_encode__doc__},

static PyObject *
_codecs_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                   const char *errors);

static PyObject *
_codecs_unicode_escape_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("unicode_escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("unicode_escape_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("unicode_escape_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_unicode_escape_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_encode__doc__,
"raw_unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"raw_unicode_escape_encode", (PyCFunction)(void(*)(void))_codecs_raw_unicode_escape_encode, METH_FASTCALL, _codecs_raw_unicode_escape_encode__doc__},

static PyObject *
_codecs_raw_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                       const char *errors);

static PyObject *
_codecs_raw_unicode_escape_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("raw_unicode_escape_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("raw_unicode_escape_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("raw_unicode_escape_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_raw_unicode_escape_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_encode__doc__,
"latin_1_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_ENCODE_METHODDEF    \
    {"latin_1_encode", (PyCFunction)(void(*)(void))_codecs_latin_1_encode, METH_FASTCALL, _codecs_latin_1_encode__doc__},

static PyObject *
_codecs_latin_1_encode_impl(PyObject *module, PyObject *str,
                            const char *errors);

static PyObject *
_codecs_latin_1_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("latin_1_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("latin_1_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("latin_1_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_latin_1_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_encode__doc__,
"ascii_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_ENCODE_METHODDEF    \
    {"ascii_encode", (PyCFunction)(void(*)(void))_codecs_ascii_encode, METH_FASTCALL, _codecs_ascii_encode__doc__},

static PyObject *
_codecs_ascii_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_ascii_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("ascii_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("ascii_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("ascii_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_ascii_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_encode__doc__,
"charmap_encode($module, str, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_ENCODE_METHODDEF    \
    {"charmap_encode", (PyCFunction)(void(*)(void))_codecs_charmap_encode, METH_FASTCALL, _codecs_charmap_encode__doc__},

static PyObject *
_codecs_charmap_encode_impl(PyObject *module, PyObject *str,
                            const char *errors, PyObject *mapping);

static PyObject *
_codecs_charmap_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    PyObject *mapping = Py_None;

    if (!_PyArg_CheckPositional("charmap_encode", nargs, 1, 3)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("charmap_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("charmap_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    mapping = args[2];
skip_optional:
    return_value = _codecs_charmap_encode_impl(module, str, errors, mapping);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_build__doc__,
"charmap_build($module, map, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_BUILD_METHODDEF    \
    {"charmap_build", (PyCFunction)_codecs_charmap_build, METH_O, _codecs_charmap_build__doc__},

static PyObject *
_codecs_charmap_build_impl(PyObject *module, PyObject *map);

static PyObject *
_codecs_charmap_build(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *map;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("charmap_build", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    map = arg;
    return_value = _codecs_charmap_build_impl(module, map);

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_encode__doc__,
"mbcs_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_ENCODE_METHODDEF    \
    {"mbcs_encode", (PyCFunction)(void(*)(void))_codecs_mbcs_encode, METH_FASTCALL, _codecs_mbcs_encode__doc__},

static PyObject *
_codecs_mbcs_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyObject *
_codecs_mbcs_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("mbcs_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("mbcs_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("mbcs_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_mbcs_encode_impl(module, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_encode__doc__,
"oem_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_ENCODE_METHODDEF    \
    {"oem_encode", (PyCFunction)(void(*)(void))_codecs_oem_encode, METH_FASTCALL, _codecs_oem_encode__doc__},

static PyObject *
_codecs_oem_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyObject *
_codecs_oem_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("oem_encode", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("oem_encode", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    str = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("oem_encode", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_oem_encode_impl(module, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_encode__doc__,
"code_page_encode($module, code_page, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_ENCODE_METHODDEF    \
    {"code_page_encode", (PyCFunction)(void(*)(void))_codecs_code_page_encode, METH_FASTCALL, _codecs_code_page_encode__doc__},

static PyObject *
_codecs_code_page_encode_impl(PyObject *module, int code_page, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_code_page_encode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int code_page;
    PyObject *str;
    const char *errors = NULL;

    if (!_PyArg_CheckPositional("code_page_encode", nargs, 2, 3)) {
        goto exit;
    }
    code_page = _PyLong_AsInt(args[0]);
    if (code_page == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("code_page_encode", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    str = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    if (args[2] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[2])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[2], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("code_page_encode", "argument 3", "str or None", args[2]);
        goto exit;
    }
skip_optional:
    return_value = _codecs_code_page_encode_impl(module, code_page, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(_codecs_register_error__doc__,
"register_error($module, errors, handler, /)\n"
"--\n"
"\n"
"Register the specified error handler under the name errors.\n"
"\n"
"handler must be a callable object, that will be called with an exception\n"
"instance containing information about the location of the encoding/decoding\n"
"error and must return a (replacement, new position) tuple.");

#define _CODECS_REGISTER_ERROR_METHODDEF    \
    {"register_error", (PyCFunction)(void(*)(void))_codecs_register_error, METH_FASTCALL, _codecs_register_error__doc__},

static PyObject *
_codecs_register_error_impl(PyObject *module, const char *errors,
                            PyObject *handler);

static PyObject *
_codecs_register_error(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *errors;
    PyObject *handler;

    if (!_PyArg_CheckPositional("register_error", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("register_error", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t errors_length;
    errors = PyUnicode_AsUTF8AndSize(args[0], &errors_length);
    if (errors == NULL) {
        goto exit;
    }
    if (strlen(errors) != (size_t)errors_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    handler = args[1];
    return_value = _codecs_register_error_impl(module, errors, handler);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_lookup_error__doc__,
"lookup_error($module, name, /)\n"
"--\n"
"\n"
"lookup_error(errors) -> handler\n"
"\n"
"Return the error handler for the specified error handling name or raise a\n"
"LookupError, if no handler exists under this name.");

#define _CODECS_LOOKUP_ERROR_METHODDEF    \
    {"lookup_error", (PyCFunction)_codecs_lookup_error, METH_O, _codecs_lookup_error__doc__},

static PyObject *
_codecs_lookup_error_impl(PyObject *module, const char *name);

static PyObject *
_codecs_lookup_error(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("lookup_error", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(arg, &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _codecs_lookup_error_impl(module, name);

exit:
    return return_value;
}

#ifndef _CODECS_MBCS_DECODE_METHODDEF
    #define _CODECS_MBCS_DECODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_DECODE_METHODDEF) */

#ifndef _CODECS_OEM_DECODE_METHODDEF
    #define _CODECS_OEM_DECODE_METHODDEF
#endif /* !defined(_CODECS_OEM_DECODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_DECODE_METHODDEF
    #define _CODECS_CODE_PAGE_DECODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_DECODE_METHODDEF) */

#ifndef _CODECS_MBCS_ENCODE_METHODDEF
    #define _CODECS_MBCS_ENCODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_ENCODE_METHODDEF) */

#ifndef _CODECS_OEM_ENCODE_METHODDEF
    #define _CODECS_OEM_ENCODE_METHODDEF
#endif /* !defined(_CODECS_OEM_ENCODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_ENCODE_METHODDEF
    #define _CODECS_CODE_PAGE_ENCODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_ENCODE_METHODDEF) */
/*[clinic end generated code: output=557c3b37e4c492ac input=a9049054013a1b77]*/
