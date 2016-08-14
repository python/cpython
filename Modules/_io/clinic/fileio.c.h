/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_io_FileIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the file.\n"
"\n"
"A closed file cannot be used for further I/O operations.  close() may be\n"
"called more than once without error.");

#define _IO_FILEIO_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io_FileIO_close, METH_NOARGS, _io_FileIO_close__doc__},

static PyObject *
_io_FileIO_close_impl(fileio *self);

static PyObject *
_io_FileIO_close(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_close_impl(self);
}

PyDoc_STRVAR(_io_FileIO___init____doc__,
"FileIO(file, mode=\'r\', closefd=True, opener=None)\n"
"--\n"
"\n"
"Open a file.\n"
"\n"
"The mode can be \'r\' (default), \'w\', \'x\' or \'a\' for reading,\n"
"writing, exclusive creation or appending.  The file will be created if it\n"
"doesn\'t exist when opened for writing or appending; it will be truncated\n"
"when opened for writing.  A FileExistsError will be raised if it already\n"
"exists when opened for creating. Opening a file for creating implies\n"
"writing so this mode behaves in a similar way to \'w\'.Add a \'+\' to the mode\n"
"to allow simultaneous reading and writing. A custom opener can be used by\n"
"passing a callable as *opener*. The underlying file descriptor for the file\n"
"object is then obtained by calling opener with (*name*, *flags*).\n"
"*opener* must return an open file descriptor (passing os.open as *opener*\n"
"results in functionality similar to passing None).");

static int
_io_FileIO___init___impl(fileio *self, PyObject *nameobj, const char *mode,
                         int closefd, PyObject *opener);

static int
_io_FileIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"file", "mode", "closefd", "opener", NULL};
    static _PyArg_Parser _parser = {"O|siO:FileIO", _keywords, 0};
    PyObject *nameobj;
    const char *mode = "r";
    int closefd = 1;
    PyObject *opener = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &nameobj, &mode, &closefd, &opener)) {
        goto exit;
    }
    return_value = _io_FileIO___init___impl((fileio *)self, nameobj, mode, closefd, opener);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the underlying file descriptor (an integer).");

#define _IO_FILEIO_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)_io_FileIO_fileno, METH_NOARGS, _io_FileIO_fileno__doc__},

static PyObject *
_io_FileIO_fileno_impl(fileio *self);

static PyObject *
_io_FileIO_fileno(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_fileno_impl(self);
}

PyDoc_STRVAR(_io_FileIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"True if file was opened in a read mode.");

#define _IO_FILEIO_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io_FileIO_readable, METH_NOARGS, _io_FileIO_readable__doc__},

static PyObject *
_io_FileIO_readable_impl(fileio *self);

static PyObject *
_io_FileIO_readable(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_readable_impl(self);
}

PyDoc_STRVAR(_io_FileIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"True if file was opened in a write mode.");

#define _IO_FILEIO_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io_FileIO_writable, METH_NOARGS, _io_FileIO_writable__doc__},

static PyObject *
_io_FileIO_writable_impl(fileio *self);

static PyObject *
_io_FileIO_writable(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_writable_impl(self);
}

PyDoc_STRVAR(_io_FileIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"True if file supports random-access.");

#define _IO_FILEIO_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io_FileIO_seekable, METH_NOARGS, _io_FileIO_seekable__doc__},

static PyObject *
_io_FileIO_seekable_impl(fileio *self);

static PyObject *
_io_FileIO_seekable(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_seekable_impl(self);
}

PyDoc_STRVAR(_io_FileIO_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n"
"Same as RawIOBase.readinto().");

#define _IO_FILEIO_READINTO_METHODDEF    \
    {"readinto", (PyCFunction)_io_FileIO_readinto, METH_O, _io_FileIO_readinto__doc__},

static PyObject *
_io_FileIO_readinto_impl(fileio *self, Py_buffer *buffer);

static PyObject *
_io_FileIO_readinto(fileio *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (!PyArg_Parse(arg, "w*:readinto", &buffer)) {
        goto exit;
    }
    return_value = _io_FileIO_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_readall__doc__,
"readall($self, /)\n"
"--\n"
"\n"
"Read all data from the file, returned as bytes.\n"
"\n"
"In non-blocking mode, returns as much as is immediately available,\n"
"or None if no data is available.  Return an empty bytes object at EOF.");

#define _IO_FILEIO_READALL_METHODDEF    \
    {"readall", (PyCFunction)_io_FileIO_readall, METH_NOARGS, _io_FileIO_readall__doc__},

static PyObject *
_io_FileIO_readall_impl(fileio *self);

static PyObject *
_io_FileIO_readall(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_readall_impl(self);
}

PyDoc_STRVAR(_io_FileIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as bytes.\n"
"\n"
"Only makes one system call, so less data may be returned than requested.\n"
"In non-blocking mode, returns None if no data is available.\n"
"Return an empty bytes object at EOF.");

#define _IO_FILEIO_READ_METHODDEF    \
    {"read", (PyCFunction)_io_FileIO_read, METH_VARARGS, _io_FileIO_read__doc__},

static PyObject *
_io_FileIO_read_impl(fileio *self, Py_ssize_t size);

static PyObject *
_io_FileIO_read(fileio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!PyArg_ParseTuple(args, "|O&:read",
        _PyIO_ConvertSsize_t, &size)) {
        goto exit;
    }
    return_value = _io_FileIO_read_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write buffer b to file, return number of bytes written.\n"
"\n"
"Only makes one system call, so not all of the data may be written.\n"
"The number of bytes actually written is returned.  In non-blocking mode,\n"
"returns None if the write would block.");

#define _IO_FILEIO_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io_FileIO_write, METH_O, _io_FileIO_write__doc__},

static PyObject *
_io_FileIO_write_impl(fileio *self, Py_buffer *b);

static PyObject *
_io_FileIO_write(fileio *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer b = {NULL, NULL};

    if (!PyArg_Parse(arg, "y*:write", &b)) {
        goto exit;
    }
    return_value = _io_FileIO_write_impl(self, &b);

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

PyDoc_STRVAR(_io_FileIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Move to new file position and return the file position.\n"
"\n"
"Argument offset is a byte count.  Optional argument whence defaults to\n"
"SEEK_SET or 0 (offset from start of file, offset should be >= 0); other values\n"
"are SEEK_CUR or 1 (move relative to current position, positive or negative),\n"
"and SEEK_END or 2 (move relative to end of file, usually negative, although\n"
"many platforms allow seeking beyond the end of a file).\n"
"\n"
"Note that not all file objects are seekable.");

#define _IO_FILEIO_SEEK_METHODDEF    \
    {"seek", (PyCFunction)_io_FileIO_seek, METH_VARARGS, _io_FileIO_seek__doc__},

static PyObject *
_io_FileIO_seek_impl(fileio *self, PyObject *pos, int whence);

static PyObject *
_io_FileIO_seek(fileio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *pos;
    int whence = 0;

    if (!PyArg_ParseTuple(args, "O|i:seek",
        &pos, &whence)) {
        goto exit;
    }
    return_value = _io_FileIO_seek_impl(self, pos, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_FileIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Current file position.\n"
"\n"
"Can raise OSError for non seekable files.");

#define _IO_FILEIO_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io_FileIO_tell, METH_NOARGS, _io_FileIO_tell__doc__},

static PyObject *
_io_FileIO_tell_impl(fileio *self);

static PyObject *
_io_FileIO_tell(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_tell_impl(self);
}

#if defined(HAVE_FTRUNCATE)

PyDoc_STRVAR(_io_FileIO_truncate__doc__,
"truncate($self, size=None, /)\n"
"--\n"
"\n"
"Truncate the file to at most size bytes and return the truncated size.\n"
"\n"
"Size defaults to the current file position, as returned by tell().\n"
"The current file position is changed to the value of size.");

#define _IO_FILEIO_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)_io_FileIO_truncate, METH_VARARGS, _io_FileIO_truncate__doc__},

static PyObject *
_io_FileIO_truncate_impl(fileio *self, PyObject *posobj);

static PyObject *
_io_FileIO_truncate(fileio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *posobj = NULL;

    if (!PyArg_UnpackTuple(args, "truncate",
        0, 1,
        &posobj)) {
        goto exit;
    }
    return_value = _io_FileIO_truncate_impl(self, posobj);

exit:
    return return_value;
}

#endif /* defined(HAVE_FTRUNCATE) */

PyDoc_STRVAR(_io_FileIO_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"True if the file is connected to a TTY device.");

#define _IO_FILEIO_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io_FileIO_isatty, METH_NOARGS, _io_FileIO_isatty__doc__},

static PyObject *
_io_FileIO_isatty_impl(fileio *self);

static PyObject *
_io_FileIO_isatty(fileio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_FileIO_isatty_impl(self);
}

#ifndef _IO_FILEIO_TRUNCATE_METHODDEF
    #define _IO_FILEIO_TRUNCATE_METHODDEF
#endif /* !defined(_IO_FILEIO_TRUNCATE_METHODDEF) */
/*[clinic end generated code: output=51924bc0ee11d58e input=a9049054013a1b77]*/
