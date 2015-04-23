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

    if (!PyArg_Parse(arg, "w*:readinto", &buffer))
        goto exit;
    return_value = _io__BufferedIOBase_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

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

    if (!PyArg_Parse(arg, "w*:readinto1", &buffer))
        goto exit;
    return_value = _io__BufferedIOBase_readinto1_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

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
    {"peek", (PyCFunction)_io__Buffered_peek, METH_VARARGS, _io__Buffered_peek__doc__},

static PyObject *
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size);

static PyObject *
_io__Buffered_peek(buffered *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = 0;

    if (!PyArg_ParseTuple(args, "|n:peek",
        &size))
        goto exit;
    return_value = _io__Buffered_peek_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ_METHODDEF    \
    {"read", (PyCFunction)_io__Buffered_read, METH_VARARGS, _io__Buffered_read__doc__},

static PyObject *
_io__Buffered_read_impl(buffered *self, Py_ssize_t n);

static PyObject *
_io__Buffered_read(buffered *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = -1;

    if (!PyArg_ParseTuple(args, "|O&:read",
        _PyIO_ConvertSsize_t, &n))
        goto exit;
    return_value = _io__Buffered_read_impl(self, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_read1__doc__,
"read1($self, size, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READ1_METHODDEF    \
    {"read1", (PyCFunction)_io__Buffered_read1, METH_O, _io__Buffered_read1__doc__},

static PyObject *
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n);

static PyObject *
_io__Buffered_read1(buffered *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t n;

    if (!PyArg_Parse(arg, "n:read1", &n))
        goto exit;
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

    if (!PyArg_Parse(arg, "w*:readinto", &buffer))
        goto exit;
    return_value = _io__Buffered_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

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

    if (!PyArg_Parse(arg, "w*:readinto1", &buffer))
        goto exit;
    return_value = _io__Buffered_readinto1_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READLINE_METHODDEF    \
    {"readline", (PyCFunction)_io__Buffered_readline, METH_VARARGS, _io__Buffered_readline__doc__},

static PyObject *
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size);

static PyObject *
_io__Buffered_readline(buffered *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!PyArg_ParseTuple(args, "|O&:readline",
        _PyIO_ConvertSsize_t, &size))
        goto exit;
    return_value = _io__Buffered_readline_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seek__doc__,
"seek($self, target, whence=0, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEK_METHODDEF    \
    {"seek", (PyCFunction)_io__Buffered_seek, METH_VARARGS, _io__Buffered_seek__doc__},

static PyObject *
_io__Buffered_seek_impl(buffered *self, PyObject *targetobj, int whence);

static PyObject *
_io__Buffered_seek(buffered *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *targetobj;
    int whence = 0;

    if (!PyArg_ParseTuple(args, "O|i:seek",
        &targetobj, &whence))
        goto exit;
    return_value = _io__Buffered_seek_impl(self, targetobj, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)_io__Buffered_truncate, METH_VARARGS, _io__Buffered_truncate__doc__},

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyObject *pos);

static PyObject *
_io__Buffered_truncate(buffered *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *pos = Py_None;

    if (!PyArg_UnpackTuple(args, "truncate",
        0, 1,
        &pos))
        goto exit;
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
    static char *_keywords[] = {"raw", "buffer_size", NULL};
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n:BufferedReader", _keywords,
        &raw, &buffer_size))
        goto exit;
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
    static char *_keywords[] = {"raw", "buffer_size", NULL};
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n:BufferedWriter", _keywords,
        &raw, &buffer_size))
        goto exit;
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

    if (!PyArg_Parse(arg, "y*:write", &buffer))
        goto exit;
    return_value = _io_BufferedWriter_write_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

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

    if ((Py_TYPE(self) == &PyBufferedRWPair_Type) &&
        !_PyArg_NoKeywords("BufferedRWPair", kwargs))
        goto exit;
    if (!PyArg_ParseTuple(args, "OO|n:BufferedRWPair",
        &reader, &writer, &buffer_size))
        goto exit;
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
    static char *_keywords[] = {"raw", "buffer_size", NULL};
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|n:BufferedRandom", _keywords,
        &raw, &buffer_size))
        goto exit;
    return_value = _io_BufferedRandom___init___impl((buffered *)self, raw, buffer_size);

exit:
    return return_value;
}
/*[clinic end generated code: output=2bbb5e239b4ffe6f input=a9049054013a1b77]*/
