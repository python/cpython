/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_io__IOBase_seek__doc__,
"seek($self, offset, whence=os.SEEK_SET, /)\n"
"--\n"
"\n"
"Change the stream position to the given byte offset.\n"
"\n"
"  offset\n"
"    The stream position, relative to \'whence\'.\n"
"  whence\n"
"    The relative position to seek from.\n"
"\n"
"The offset is interpreted relative to the position indicated by whence.\n"
"Values for whence are:\n"
"\n"
"* os.SEEK_SET or 0 -- start of stream (the default); offset should be zero or positive\n"
"* os.SEEK_CUR or 1 -- current stream position; offset may be negative\n"
"* os.SEEK_END or 2 -- end of stream; offset is usually negative\n"
"\n"
"Return the new absolute position.");

#define _IO__IOBASE_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io__IOBase_seek), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__IOBase_seek__doc__},

static PyObject *
_io__IOBase_seek_impl(PyObject *self, PyTypeObject *cls,
                      int Py_UNUSED(offset), int Py_UNUSED(whence));

static PyObject *
_io__IOBase_seek(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", "", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "seek",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    int offset;
    int whence = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    offset = PyLong_AsInt(args[0]);
    if (offset == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional_posonly;
    }
    whence = PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_posonly:
    return_value = _io__IOBase_seek_impl(self, cls, offset, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__IOBase_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Return current stream position.");

#define _IO__IOBASE_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io__IOBase_tell, METH_NOARGS, _io__IOBase_tell__doc__},

static PyObject *
_io__IOBase_tell_impl(PyObject *self);

static PyObject *
_io__IOBase_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_tell_impl(self);
}

PyDoc_STRVAR(_io__IOBase_truncate__doc__,
"truncate($self, size=None, /)\n"
"--\n"
"\n"
"Truncate file to size bytes.\n"
"\n"
"File pointer is left unchanged. Size defaults to the current IO position\n"
"as reported by tell(). Return the new size.");

#define _IO__IOBASE_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io__IOBase_truncate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__IOBase_truncate__doc__},

static PyObject *
_io__IOBase_truncate_impl(PyObject *self, PyTypeObject *cls,
                          PyObject *Py_UNUSED(size));

static PyObject *
_io__IOBase_truncate(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "truncate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *size = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    size = args[0];
skip_optional_posonly:
    return_value = _io__IOBase_truncate_impl(self, cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__IOBase_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Flush write buffers, if applicable.\n"
"\n"
"This is not implemented for read-only and non-blocking streams.");

#define _IO__IOBASE_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io__IOBase_flush, METH_NOARGS, _io__IOBase_flush__doc__},

static PyObject *
_io__IOBase_flush_impl(PyObject *self);

static PyObject *
_io__IOBase_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_flush_impl(self);
}

PyDoc_STRVAR(_io__IOBase_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Flush and close the IO object.\n"
"\n"
"This method has no effect if the file is already closed.");

#define _IO__IOBASE_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io__IOBase_close, METH_NOARGS, _io__IOBase_close__doc__},

static PyObject *
_io__IOBase_close_impl(PyObject *self);

static PyObject *
_io__IOBase_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_close_impl(self);
}

PyDoc_STRVAR(_io__IOBase_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"Return whether object supports random access.\n"
"\n"
"If False, seek(), tell() and truncate() will raise OSError.\n"
"This method may need to do a test seek().");

#define _IO__IOBASE_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io__IOBase_seekable, METH_NOARGS, _io__IOBase_seekable__doc__},

static PyObject *
_io__IOBase_seekable_impl(PyObject *self);

static PyObject *
_io__IOBase_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_seekable_impl(self);
}

PyDoc_STRVAR(_io__IOBase_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"Return whether object was opened for reading.\n"
"\n"
"If False, read() will raise OSError.");

#define _IO__IOBASE_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io__IOBase_readable, METH_NOARGS, _io__IOBase_readable__doc__},

static PyObject *
_io__IOBase_readable_impl(PyObject *self);

static PyObject *
_io__IOBase_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_readable_impl(self);
}

PyDoc_STRVAR(_io__IOBase_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"Return whether object was opened for writing.\n"
"\n"
"If False, write() will raise OSError.");

#define _IO__IOBASE_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io__IOBase_writable, METH_NOARGS, _io__IOBase_writable__doc__},

static PyObject *
_io__IOBase_writable_impl(PyObject *self);

static PyObject *
_io__IOBase_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_writable_impl(self);
}

PyDoc_STRVAR(_io__IOBase_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return underlying file descriptor if one exists.\n"
"\n"
"Raise OSError if the IO object does not use a file descriptor.");

#define _IO__IOBASE_FILENO_METHODDEF    \
    {"fileno", _PyCFunction_CAST(_io__IOBase_fileno), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__IOBase_fileno__doc__},

static PyObject *
_io__IOBase_fileno_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
_io__IOBase_fileno(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "fileno() takes no arguments");
        return NULL;
    }
    return _io__IOBase_fileno_impl(self, cls);
}

PyDoc_STRVAR(_io__IOBase_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"Return whether this is an \'interactive\' stream.\n"
"\n"
"Return False if it can\'t be determined.");

#define _IO__IOBASE_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io__IOBase_isatty, METH_NOARGS, _io__IOBase_isatty__doc__},

static PyObject *
_io__IOBase_isatty_impl(PyObject *self);

static PyObject *
_io__IOBase_isatty(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__IOBase_isatty_impl(self);
}

PyDoc_STRVAR(_io__IOBase_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Read and return a line from the stream.\n"
"\n"
"If size is specified, at most size bytes will be read.\n"
"\n"
"The line terminator is always b\'\\n\' for binary files; for text\n"
"files, the newlines argument to open can be used to select the line\n"
"terminator(s) recognized.");

#define _IO__IOBASE_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io__IOBase_readline), METH_FASTCALL, _io__IOBase_readline__doc__},

static PyObject *
_io__IOBase_readline_impl(PyObject *self, Py_ssize_t limit);

static PyObject *
_io__IOBase_readline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t limit = -1;

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &limit)) {
        goto exit;
    }
skip_optional:
    return_value = _io__IOBase_readline_impl(self, limit);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__IOBase_readlines__doc__,
"readlines($self, hint=-1, /)\n"
"--\n"
"\n"
"Return a list of lines from the stream.\n"
"\n"
"hint can be specified to control the number of lines read: no more\n"
"lines will be read if the total size (in bytes/characters) of all\n"
"lines so far exceeds hint.");

#define _IO__IOBASE_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_io__IOBase_readlines), METH_FASTCALL, _io__IOBase_readlines__doc__},

static PyObject *
_io__IOBase_readlines_impl(PyObject *self, Py_ssize_t hint);

static PyObject *
_io__IOBase_readlines(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t hint = -1;

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &hint)) {
        goto exit;
    }
skip_optional:
    return_value = _io__IOBase_readlines_impl(self, hint);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__IOBase_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n"
"Write a list of lines to stream.\n"
"\n"
"Line separators are not added, so it is usual for each of the\n"
"lines provided to have a line separator at the end.");

#define _IO__IOBASE_WRITELINES_METHODDEF    \
    {"writelines", (PyCFunction)_io__IOBase_writelines, METH_O, _io__IOBase_writelines__doc__},

PyDoc_STRVAR(_io__RawIOBase_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO__RAWIOBASE_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__RawIOBase_read), METH_FASTCALL, _io__RawIOBase_read__doc__},

static PyObject *
_io__RawIOBase_read_impl(PyObject *self, Py_ssize_t n);

static PyObject *
_io__RawIOBase_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = -1;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
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
    return_value = _io__RawIOBase_read_impl(self, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__RawIOBase_readall__doc__,
"readall($self, /)\n"
"--\n"
"\n"
"Read until EOF, using multiple read() call.");

#define _IO__RAWIOBASE_READALL_METHODDEF    \
    {"readall", (PyCFunction)_io__RawIOBase_readall, METH_NOARGS, _io__RawIOBase_readall__doc__},

static PyObject *
_io__RawIOBase_readall_impl(PyObject *self);

static PyObject *
_io__RawIOBase_readall(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _io__RawIOBase_readall_impl(self);
}
/*[clinic end generated code: output=5a22bc5db0ecaacb input=a9049054013a1b77]*/
