/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_io__BufferedIOBase_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFEREDIOBASE_READINTO_METHODDEF    \
    {"readinto", (PyCFunction)_io__BufferedIOBase_readinto, METH_O, _io__BufferedIOBase_readinto__doc__},

static PyObject *
_io__BufferedIOBase_readinto_impl(PyObject *self, Py_buffer *buffer);

static PyObject *
_io__BufferedIOBase_readinto(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        PyErr_Clear();
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("readinto", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io__BufferedIOBase_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFEREDIOBASE_READINTO1_METHODDEF    \
    {"readinto1", (PyCFunction)_io__BufferedIOBase_readinto1, METH_O, _io__BufferedIOBase_readinto1__doc__},

static PyObject *
_io__BufferedIOBase_readinto1_impl(PyObject *self, Py_buffer *buffer);

static PyObject *
_io__BufferedIOBase_readinto1(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        PyErr_Clear();
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("readinto1", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io__BufferedIOBase_readinto1_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n"
"Disconnect this buffer from its underlying raw stream and return it.\n"
"\n"
"After the raw stream has been detached, the buffer is in an unusable\n"
"state.");

#define _IO__BUFFEREDIOBASE_DETACH_METHODDEF    \
    {"detach", (PyCFunction)_io__BufferedIOBase_detach, METH_NOARGS, _io__BufferedIOBase_detach__doc__},

static PyObject *
_io__BufferedIOBase_detach_impl(PyObject *self);

static PyObject *
_io__BufferedIOBase_detach(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__BufferedIOBase_detach_impl(self);
}

PyDoc_STRVAR(_io__Buffered_peek__doc__,
"peek($self, size=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_PEEK_METHODDEF    \
    {"peek", _PyCFunction_CAST(_io__Buffered_peek), METH_FASTCALL, _io__Buffered_peek__doc__},

static PyObject *
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size);

static PyObject *
_io__Buffered_peek(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = 0;

    if (!_PyArg_CheckPositional("peek", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        size = ival;
    }
skip_optional:
    return_value = _io__Buffered_peek_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__Buffered_read), METH_FASTCALL, _io__Buffered_read__doc__},

static PyObject *
_io__Buffered_read_impl(buffered *self, Py_ssize_t n);

static PyObject *
_io__Buffered_read(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &n)) {
        goto exit;
    }
skip_optional:
    return_value = _io__Buffered_read_impl(self, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io__Buffered_read1), METH_FASTCALL, _io__Buffered_read1__doc__},

static PyObject *
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n);

static PyObject *
_io__Buffered_read1(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        n = ival;
    }
skip_optional:
    return_value = _io__Buffered_read1_impl(self, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READINTO_METHODDEF    \
    {"readinto", (PyCFunction)_io__Buffered_readinto, METH_O, _io__Buffered_readinto__doc__},

static PyObject *
_io__Buffered_readinto_impl(buffered *self, Py_buffer *buffer);

static PyObject *
_io__Buffered_readinto(buffered *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        PyErr_Clear();
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("readinto", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io__Buffered_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readinto1__doc__,
"readinto1($self, buffer, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READINTO1_METHODDEF    \
    {"readinto1", (PyCFunction)_io__Buffered_readinto1, METH_O, _io__Buffered_readinto1__doc__},

static PyObject *
_io__Buffered_readinto1_impl(buffered *self, Py_buffer *buffer);

static PyObject *
_io__Buffered_readinto1(buffered *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        PyErr_Clear();
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("readinto1", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io__Buffered_readinto1_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io__Buffered_readline), METH_FASTCALL, _io__Buffered_readline__doc__},

static PyObject *
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size);

static PyObject *
_io__Buffered_readline(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &size)) {
        goto exit;
    }
skip_optional:
    return_value = _io__Buffered_readline_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seek__doc__,
"seek($self, target, whence=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io__Buffered_seek), METH_FASTCALL, _io__Buffered_seek__doc__},

static PyObject *
_io__Buffered_seek_impl(buffered *self, PyObject *targetobj, int whence);

static PyObject *
_io__Buffered_seek(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *targetobj;
    int whence = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    targetobj = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = _PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io__Buffered_seek_impl(self, targetobj, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io__Buffered_truncate), METH_FASTCALL, _io__Buffered_truncate__doc__},

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyObject *pos);

static PyObject *
_io__Buffered_truncate(buffered *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pos = Py_None;

    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    pos = args[0];
skip_optional:
    return_value = _io__Buffered_truncate_impl(self, pos);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedReader___init____doc__,
"BufferedReader(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"Create a new buffered reader using the given readable raw IO object.");

static int
_io_BufferedReader___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedReader___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "BufferedReader", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedReader___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedWriter___init____doc__,
"BufferedWriter(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"A buffer for a writeable sequential RawIO object.\n"
"\n"
"The constructor creates a BufferedWriter for the given writeable raw\n"
"stream. If the buffer_size is not given, it defaults to\n"
"DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedWriter___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedWriter___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "BufferedWriter", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedWriter___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedWriter_write__doc__,
"write($self, buffer, /)\n"
"--\n"
"\n");

#define _IO_BUFFEREDWRITER_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io_BufferedWriter_write, METH_O, _io_BufferedWriter_write__doc__},

static PyObject *
_io_BufferedWriter_write_impl(buffered *self, Py_buffer *buffer);

static PyObject *
_io_BufferedWriter_write(buffered *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&buffer, 'C')) {
        _PyArg_BadArgument("write", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io_BufferedWriter_write_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_BufferedRWPair___init____doc__,
"BufferedRWPair(reader, writer, buffer_size=DEFAULT_BUFFER_SIZE, /)\n"
"--\n"
"\n"
"A buffered reader and writer object together.\n"
"\n"
"A buffered reader object and buffered writer object put together to\n"
"form a sequential IO object that can read and write. This is typically\n"
"used with a socket or two-way pipe.\n"
"\n"
"reader and writer are RawIOBase objects that are readable and\n"
"writeable respectively. If the buffer_size is omitted it defaults to\n"
"DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedRWPair___init___impl(rwpair *self, PyObject *reader,
                                 PyObject *writer, Py_ssize_t buffer_size);

static int
_io_BufferedRWPair___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyObject *reader;
    PyObject *writer;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if ((Py_IS_TYPE(self, &PyBufferedRWPair_Type) ||
         Py_TYPE(self)->tp_new == PyBufferedRWPair_Type.tp_new) &&
        !_PyArg_NoKeywords("BufferedRWPair", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("BufferedRWPair", PyTuple_GET_SIZE(args), 2, 3)) {
        goto exit;
    }
    reader = PyTuple_GET_ITEM(args, 0);
    writer = PyTuple_GET_ITEM(args, 1);
    if (PyTuple_GET_SIZE(args) < 3) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(PyTuple_GET_ITEM(args, 2));
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional:
    return_value = _io_BufferedRWPair___init___impl((rwpair *)self, reader, writer, buffer_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BufferedRandom___init____doc__,
"BufferedRandom(raw, buffer_size=DEFAULT_BUFFER_SIZE)\n"
"--\n"
"\n"
"A buffered interface to random access streams.\n"
"\n"
"The constructor creates a reader and writer for a seekable stream,\n"
"raw, given in the first argument. If the buffer_size is omitted it\n"
"defaults to DEFAULT_BUFFER_SIZE.");

static int
_io_BufferedRandom___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size);

static int
_io_BufferedRandom___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "BufferedRandom", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    raw = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(fastargs[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        buffer_size = ival;
    }
skip_optional_pos:
    return_value = _io_BufferedRandom___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}
/*[clinic end generated code: output=820461c6b0e29e48 input=a9049054013a1b77]*/
