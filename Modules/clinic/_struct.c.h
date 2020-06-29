/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(Struct___init____doc__,
"Struct(format)\n"
"--\n"
"\n"
"Create a compiled struct object.\n"
"\n"
"Return a new Struct object which writes and reads binary data according to\n"
"the format string.\n"
"\n"
"See help(struct) for more on format strings.");

static int
Struct___init___impl(PyStructObject *self, PyObject *format);

static int
Struct___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "Struct", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *format;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    format = fastargs[0];
    return_value = Struct___init___impl((PyStructObject *)self, format);

exit:
    return return_value;
}

PyDoc_STRVAR(Struct_unpack__doc__,
"unpack($self, buffer, /)\n"
"--\n"
"\n"
"Return a tuple containing unpacked values.\n"
"\n"
"Unpack according to the format string Struct.format. The buffer\'s size\n"
"in bytes must be Struct.size.\n"
"\n"
"See help(struct) for more on format strings.");

#define STRUCT_UNPACK_METHODDEF    \
    {"unpack", (PyCFunction)Struct_unpack, METH_O, Struct_unpack__doc__},

static PyObject *
Struct_unpack_impl(PyStructObject *self, Py_buffer *buffer);

static PyObject *
Struct_unpack(PyStructObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("unpack", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = Struct_unpack_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(Struct_unpack_from__doc__,
"unpack_from($self, /, buffer, offset=0)\n"
"--\n"
"\n"
"Return a tuple containing unpacked values.\n"
"\n"
"Values are unpacked according to the format string Struct.format.\n"
"\n"
"The buffer\'s size in bytes, starting at position offset, must be\n"
"at least Struct.size.\n"
"\n"
"See help(struct) for more on format strings.");

#define STRUCT_UNPACK_FROM_METHODDEF    \
    {"unpack_from", (PyCFunction)(void(*)(void))Struct_unpack_from, METH_FASTCALL|METH_KEYWORDS, Struct_unpack_from__doc__},

static PyObject *
Struct_unpack_from_impl(PyStructObject *self, Py_buffer *buffer,
                        Py_ssize_t offset);

static PyObject *
Struct_unpack_from(PyStructObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"buffer", "offset", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "unpack_from", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("unpack_from", "argument 'buffer'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
skip_optional_pos:
    return_value = Struct_unpack_from_impl(self, &buffer, offset);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(Struct_iter_unpack__doc__,
"iter_unpack($self, buffer, /)\n"
"--\n"
"\n"
"Return an iterator yielding tuples.\n"
"\n"
"Tuples are unpacked from the given bytes source, like a repeated\n"
"invocation of unpack_from().\n"
"\n"
"Requires that the bytes length be a multiple of the struct size.");

#define STRUCT_ITER_UNPACK_METHODDEF    \
    {"iter_unpack", (PyCFunction)Struct_iter_unpack, METH_O, Struct_iter_unpack__doc__},

PyDoc_STRVAR(_clearcache__doc__,
"_clearcache($module, /)\n"
"--\n"
"\n"
"Clear the internal cache.");

#define _CLEARCACHE_METHODDEF    \
    {"_clearcache", (PyCFunction)_clearcache, METH_NOARGS, _clearcache__doc__},

static PyObject *
_clearcache_impl(PyObject *module);

static PyObject *
_clearcache(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _clearcache_impl(module);
}

PyDoc_STRVAR(calcsize__doc__,
"calcsize($module, format, /)\n"
"--\n"
"\n"
"Return size in bytes of the struct described by the format string.");

#define CALCSIZE_METHODDEF    \
    {"calcsize", (PyCFunction)calcsize, METH_O, calcsize__doc__},

static Py_ssize_t
calcsize_impl(PyObject *module, PyStructObject *s_object);

static PyObject *
calcsize(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyStructObject *s_object = NULL;
    Py_ssize_t _return_value;

    if (!cache_struct_converter(arg, &s_object)) {
        goto exit;
    }
    _return_value = calcsize_impl(module, s_object);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);

    return return_value;
}

PyDoc_STRVAR(unpack__doc__,
"unpack($module, format, buffer, /)\n"
"--\n"
"\n"
"Return a tuple containing values unpacked according to the format string.\n"
"\n"
"The buffer\'s size in bytes must be calcsize(format).\n"
"\n"
"See help(struct) for more on format strings.");

#define UNPACK_METHODDEF    \
    {"unpack", (PyCFunction)(void(*)(void))unpack, METH_FASTCALL, unpack__doc__},

static PyObject *
unpack_impl(PyObject *module, PyStructObject *s_object, Py_buffer *buffer);

static PyObject *
unpack(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyStructObject *s_object = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!_PyArg_CheckPositional("unpack", nargs, 2, 2)) {
        goto exit;
    }
    if (!cache_struct_converter(args[0], &s_object)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("unpack", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    return_value = unpack_impl(module, s_object, &buffer);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(unpack_from__doc__,
"unpack_from($module, format, /, buffer, offset=0)\n"
"--\n"
"\n"
"Return a tuple containing values unpacked according to the format string.\n"
"\n"
"The buffer\'s size, minus offset, must be at least calcsize(format).\n"
"\n"
"See help(struct) for more on format strings.");

#define UNPACK_FROM_METHODDEF    \
    {"unpack_from", (PyCFunction)(void(*)(void))unpack_from, METH_FASTCALL|METH_KEYWORDS, unpack_from__doc__},

static PyObject *
unpack_from_impl(PyObject *module, PyStructObject *s_object,
                 Py_buffer *buffer, Py_ssize_t offset);

static PyObject *
unpack_from(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "buffer", "offset", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "unpack_from", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyStructObject *s_object = NULL;
    Py_buffer buffer = {NULL, NULL};
    Py_ssize_t offset = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!cache_struct_converter(args[0], &s_object)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("unpack_from", "argument 'buffer'", "contiguous buffer", args[1]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        offset = ival;
    }
skip_optional_pos:
    return_value = unpack_from_impl(module, s_object, &buffer, offset);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(iter_unpack__doc__,
"iter_unpack($module, format, buffer, /)\n"
"--\n"
"\n"
"Return an iterator yielding tuples unpacked from the given bytes.\n"
"\n"
"The bytes are unpacked according to the format string, like\n"
"a repeated invocation of unpack_from().\n"
"\n"
"Requires that the bytes length be a multiple of the format struct size.");

#define ITER_UNPACK_METHODDEF    \
    {"iter_unpack", (PyCFunction)(void(*)(void))iter_unpack, METH_FASTCALL, iter_unpack__doc__},

static PyObject *
iter_unpack_impl(PyObject *module, PyStructObject *s_object,
                 PyObject *buffer);

static PyObject *
iter_unpack(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyStructObject *s_object = NULL;
    PyObject *buffer;

    if (!_PyArg_CheckPositional("iter_unpack", nargs, 2, 2)) {
        goto exit;
    }
    if (!cache_struct_converter(args[0], &s_object)) {
        goto exit;
    }
    buffer = args[1];
    return_value = iter_unpack_impl(module, s_object, buffer);

exit:
    /* Cleanup for s_object */
    Py_XDECREF(s_object);

    return return_value;
}
/*[clinic end generated code: output=8089792d8ed0c1be input=a9049054013a1b77]*/
