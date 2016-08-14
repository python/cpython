/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_io_StringIO_getvalue__doc__,
"getvalue($self, /)\n"
"--\n"
"\n"
"Retrieve the entire contents of the object.");

#define _IO_STRINGIO_GETVALUE_METHODDEF    \
    {"getvalue", (PyCFunction)_io_StringIO_getvalue, METH_NOARGS, _io_StringIO_getvalue__doc__},

static PyObject *
_io_StringIO_getvalue_impl(stringio *self);

static PyObject *
_io_StringIO_getvalue(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_getvalue_impl(self);
}

PyDoc_STRVAR(_io_StringIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Tell the current file position.");

#define _IO_STRINGIO_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io_StringIO_tell, METH_NOARGS, _io_StringIO_tell__doc__},

static PyObject *
_io_StringIO_tell_impl(stringio *self);

static PyObject *
_io_StringIO_tell(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_tell_impl(self);
}

PyDoc_STRVAR(_io_StringIO_read__doc__,
"read($self, size=None, /)\n"
"--\n"
"\n"
"Read at most size characters, returned as a string.\n"
"\n"
"If the argument is negative or omitted, read until EOF\n"
"is reached. Return an empty string at EOF.");

#define _IO_STRINGIO_READ_METHODDEF    \
    {"read", (PyCFunction)_io_StringIO_read, METH_VARARGS, _io_StringIO_read__doc__},

static PyObject *
_io_StringIO_read_impl(stringio *self, PyObject *arg);

static PyObject *
_io_StringIO_read(stringio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *arg = Py_None;

    if (!PyArg_UnpackTuple(args, "read",
        0, 1,
        &arg)) {
        goto exit;
    }
    return_value = _io_StringIO_read_impl(self, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_readline__doc__,
"readline($self, size=None, /)\n"
"--\n"
"\n"
"Read until newline or EOF.\n"
"\n"
"Returns an empty string if EOF is hit immediately.");

#define _IO_STRINGIO_READLINE_METHODDEF    \
    {"readline", (PyCFunction)_io_StringIO_readline, METH_VARARGS, _io_StringIO_readline__doc__},

static PyObject *
_io_StringIO_readline_impl(stringio *self, PyObject *arg);

static PyObject *
_io_StringIO_readline(stringio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *arg = Py_None;

    if (!PyArg_UnpackTuple(args, "readline",
        0, 1,
        &arg)) {
        goto exit;
    }
    return_value = _io_StringIO_readline_impl(self, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n"
"Truncate size to pos.\n"
"\n"
"The pos argument defaults to the current file position, as\n"
"returned by tell().  The current file position is unchanged.\n"
"Returns the new absolute position.");

#define _IO_STRINGIO_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)_io_StringIO_truncate, METH_VARARGS, _io_StringIO_truncate__doc__},

static PyObject *
_io_StringIO_truncate_impl(stringio *self, PyObject *arg);

static PyObject *
_io_StringIO_truncate(stringio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *arg = Py_None;

    if (!PyArg_UnpackTuple(args, "truncate",
        0, 1,
        &arg)) {
        goto exit;
    }
    return_value = _io_StringIO_truncate_impl(self, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Change stream position.\n"
"\n"
"Seek to character offset pos relative to position indicated by whence:\n"
"    0  Start of stream (the default).  pos should be >= 0;\n"
"    1  Current position - pos must be 0;\n"
"    2  End of stream - pos must be 0.\n"
"Returns the new absolute position.");

#define _IO_STRINGIO_SEEK_METHODDEF    \
    {"seek", (PyCFunction)_io_StringIO_seek, METH_VARARGS, _io_StringIO_seek__doc__},

static PyObject *
_io_StringIO_seek_impl(stringio *self, Py_ssize_t pos, int whence);

static PyObject *
_io_StringIO_seek(stringio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t pos;
    int whence = 0;

    if (!PyArg_ParseTuple(args, "n|i:seek",
        &pos, &whence)) {
        goto exit;
    }
    return_value = _io_StringIO_seek_impl(self, pos, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_write__doc__,
"write($self, s, /)\n"
"--\n"
"\n"
"Write string to file.\n"
"\n"
"Returns the number of characters written, which is always equal to\n"
"the length of the string.");

#define _IO_STRINGIO_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io_StringIO_write, METH_O, _io_StringIO_write__doc__},

PyDoc_STRVAR(_io_StringIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the IO object.\n"
"\n"
"Attempting any further operation after the object is closed\n"
"will raise a ValueError.\n"
"\n"
"This method has no effect if the file is already closed.");

#define _IO_STRINGIO_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io_StringIO_close, METH_NOARGS, _io_StringIO_close__doc__},

static PyObject *
_io_StringIO_close_impl(stringio *self);

static PyObject *
_io_StringIO_close(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_close_impl(self);
}

PyDoc_STRVAR(_io_StringIO___init____doc__,
"StringIO(initial_value=\'\', newline=\'\\n\')\n"
"--\n"
"\n"
"Text I/O implementation using an in-memory buffer.\n"
"\n"
"The initial_value argument sets the value of object.  The newline\n"
"argument is like the one of TextIOWrapper\'s constructor.");

static int
_io_StringIO___init___impl(stringio *self, PyObject *value,
                           PyObject *newline_obj);

static int
_io_StringIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"initial_value", "newline", NULL};
    static _PyArg_Parser _parser = {"|OO:StringIO", _keywords, 0};
    PyObject *value = NULL;
    PyObject *newline_obj = NULL;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &value, &newline_obj)) {
        goto exit;
    }
    return_value = _io_StringIO___init___impl((stringio *)self, value, newline_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be read.");

#define _IO_STRINGIO_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io_StringIO_readable, METH_NOARGS, _io_StringIO_readable__doc__},

static PyObject *
_io_StringIO_readable_impl(stringio *self);

static PyObject *
_io_StringIO_readable(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_readable_impl(self);
}

PyDoc_STRVAR(_io_StringIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be written.");

#define _IO_STRINGIO_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io_StringIO_writable, METH_NOARGS, _io_StringIO_writable__doc__},

static PyObject *
_io_StringIO_writable_impl(stringio *self);

static PyObject *
_io_StringIO_writable(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_writable_impl(self);
}

PyDoc_STRVAR(_io_StringIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be seeked.");

#define _IO_STRINGIO_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io_StringIO_seekable, METH_NOARGS, _io_StringIO_seekable__doc__},

static PyObject *
_io_StringIO_seekable_impl(stringio *self);

static PyObject *
_io_StringIO_seekable(stringio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_StringIO_seekable_impl(self);
}
/*[clinic end generated code: output=5dd5c2a213e75405 input=a9049054013a1b77]*/
