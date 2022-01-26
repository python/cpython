/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the console object.\n"
"\n"
"A closed console object cannot be used for further I/O operations.\n"
"close() may be called more than once without error.");

#define _IO__WINDOWSCONSOLEIO_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io__WindowsConsoleIO_close, METH_NOARGS, _io__WindowsConsoleIO_close__doc__},

static PyObject *
_io__WindowsConsoleIO_close_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_close(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_close_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO___init____doc__,
"_WindowsConsoleIO(file, mode=\'r\', closefd=True, opener=None)\n"
"--\n"
"\n"
"Open a console buffer by file descriptor.\n"
"\n"
"The mode can be \'rb\' (default), or \'wb\' for reading or writing bytes. All\n"
"other mode characters will be ignored. Mode \'b\' will be assumed if it is\n"
"omitted. The *opener* parameter is always ignored.");

static int
_io__WindowsConsoleIO___init___impl(winconsoleio *self, PyObject *nameobj,
                                    const char *mode, int closefd,
                                    PyObject *opener);

static int
_io__WindowsConsoleIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"file", "mode", "closefd", "opener", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_WindowsConsoleIO", 0};
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *nameobj;
    const char *mode = "r";
    int closefd = 1;
    PyObject *opener = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 4, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    nameobj = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[1]) {
        if (!PyUnicode_Check(fastargs[1])) {
            _PyArg_BadArgument("_WindowsConsoleIO", "argument 'mode'", "str", fastargs[1]);
            goto exit;
        }
        Py_ssize_t mode_length;
        mode = PyUnicode_AsUTF8AndSize(fastargs[1], &mode_length);
        if (mode == NULL) {
            goto exit;
        }
        if (strlen(mode) != (size_t)mode_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        closefd = _PyLong_AsInt(fastargs[2]);
        if (closefd == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    opener = fastargs[3];
skip_optional_pos:
    return_value = _io__WindowsConsoleIO___init___impl((winconsoleio *)self, nameobj, mode, closefd, opener);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the underlying file descriptor (an integer).");

#define _IO__WINDOWSCONSOLEIO_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)_io__WindowsConsoleIO_fileno, METH_NOARGS, _io__WindowsConsoleIO_fileno__doc__},

static PyObject *
_io__WindowsConsoleIO_fileno_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_fileno(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_fileno_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"True if console is an input buffer.");

#define _IO__WINDOWSCONSOLEIO_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io__WindowsConsoleIO_readable, METH_NOARGS, _io__WindowsConsoleIO_readable__doc__},

static PyObject *
_io__WindowsConsoleIO_readable_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_readable(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_readable_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"True if console is an output buffer.");

#define _IO__WINDOWSCONSOLEIO_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io__WindowsConsoleIO_writable, METH_NOARGS, _io__WindowsConsoleIO_writable__doc__},

static PyObject *
_io__WindowsConsoleIO_writable_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_writable(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_writable_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n"
"Same as RawIOBase.readinto().");

#define _IO__WINDOWSCONSOLEIO_READINTO_METHODDEF    \
    {"readinto", (PyCFunction)_io__WindowsConsoleIO_readinto, METH_O, _io__WindowsConsoleIO_readinto__doc__},

static PyObject *
_io__WindowsConsoleIO_readinto_impl(winconsoleio *self, Py_buffer *buffer);

static PyObject *
_io__WindowsConsoleIO_readinto(winconsoleio *self, PyObject *arg)
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
    return_value = _io__WindowsConsoleIO_readinto_impl(self, &buffer);

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_readall__doc__,
"readall($self, /)\n"
"--\n"
"\n"
"Read all data from the console, returned as bytes.\n"
"\n"
"Return an empty bytes object at EOF.");

#define _IO__WINDOWSCONSOLEIO_READALL_METHODDEF    \
    {"readall", (PyCFunction)_io__WindowsConsoleIO_readall, METH_NOARGS, _io__WindowsConsoleIO_readall__doc__},

static PyObject *
_io__WindowsConsoleIO_readall_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_readall(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_readall_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as bytes.\n"
"\n"
"Only makes one system call when size is a positive integer,\n"
"so less data may be returned than requested.\n"
"Return an empty bytes object at EOF.");

#define _IO__WINDOWSCONSOLEIO_READ_METHODDEF    \
    {"read", (PyCFunction)(void(*)(void))_io__WindowsConsoleIO_read, METH_FASTCALL, _io__WindowsConsoleIO_read__doc__},

static PyObject *
_io__WindowsConsoleIO_read_impl(winconsoleio *self, Py_ssize_t size);

static PyObject *
_io__WindowsConsoleIO_read(winconsoleio *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &size)) {
        goto exit;
    }
skip_optional:
    return_value = _io__WindowsConsoleIO_read_impl(self, size);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write buffer b to file, return number of bytes written.\n"
"\n"
"Only makes one system call, so not all of the data may be written.\n"
"The number of bytes actually written is returned.");

#define _IO__WINDOWSCONSOLEIO_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io__WindowsConsoleIO_write, METH_O, _io__WindowsConsoleIO_write__doc__},

static PyObject *
_io__WindowsConsoleIO_write_impl(winconsoleio *self, Py_buffer *b);

static PyObject *
_io__WindowsConsoleIO_write(winconsoleio *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer b = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&b, 'C')) {
        _PyArg_BadArgument("write", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _io__WindowsConsoleIO_write_impl(self, &b);

exit:
    /* Cleanup for b */
    if (b.obj) {
       PyBuffer_Release(&b);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_io__WindowsConsoleIO_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"Always True.");

#define _IO__WINDOWSCONSOLEIO_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io__WindowsConsoleIO_isatty, METH_NOARGS, _io__WindowsConsoleIO_isatty__doc__},

static PyObject *
_io__WindowsConsoleIO_isatty_impl(winconsoleio *self);

static PyObject *
_io__WindowsConsoleIO_isatty(winconsoleio *self, PyObject *Py_UNUSED(ignored))
{
    return _io__WindowsConsoleIO_isatty_impl(self);
}

#endif /* defined(MS_WINDOWS) */

#ifndef _IO__WINDOWSCONSOLEIO_CLOSE_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_CLOSE_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_CLOSE_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_FILENO_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_FILENO_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_FILENO_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_READABLE_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_READABLE_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_READABLE_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_WRITABLE_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_WRITABLE_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_WRITABLE_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_READINTO_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_READINTO_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_READINTO_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_READALL_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_READALL_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_READALL_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_READ_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_READ_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_READ_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_WRITE_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_WRITE_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_WRITE_METHODDEF) */

#ifndef _IO__WINDOWSCONSOLEIO_ISATTY_METHODDEF
    #define _IO__WINDOWSCONSOLEIO_ISATTY_METHODDEF
#endif /* !defined(_IO__WINDOWSCONSOLEIO_ISATTY_METHODDEF) */
/*[clinic end generated code: output=48080572ffee22f5 input=a9049054013a1b77]*/
