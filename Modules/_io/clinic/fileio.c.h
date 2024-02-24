/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_io_FileIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the file.\n"
"\n"
"A closed file cannot be used for further I/O operations.  close() may be\n"
"called more than once without error.");

#define _IO_FILEIO_CLOSE_METHODDEF    \
    {"close", _PyCFunction_CAST(_io_FileIO_close), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_FileIO_close__doc__},

static PyObject *
_io_FileIO_close_impl(fileio *self, PyTypeObject *cls);

static PyObject *
_io_FileIO_close(fileio *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "close() takes no arguments");
        return NULL;
    }
    return _io_FileIO_close_impl(self, cls);
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(file), &_Py_ID(mode), &_Py_ID(closefd), &_Py_ID(opener), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "mode", "closefd", "opener", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "FileIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
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
            _PyArg_BadArgument("FileIO", "argument 'mode'", "str", fastargs[1]);
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
        closefd = PyObject_IsTrue(fastargs[2]);
        if (closefd < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    opener = fastargs[3];
skip_optional_pos:
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
    {"readinto", _PyCFunction_CAST(_io_FileIO_readinto), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_FileIO_readinto__doc__},

static PyObject *
_io_FileIO_readinto_impl(fileio *self, PyTypeObject *cls, Py_buffer *buffer);

static PyObject *
_io_FileIO_readinto(fileio *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "readinto",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_buffer buffer = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument 1", "read-write bytes-like object", args[0]);
        goto exit;
    }
    return_value = _io_FileIO_readinto_impl(self, cls, &buffer);

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
    {"read", _PyCFunction_CAST(_io_FileIO_read), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_FileIO_read__doc__},

static PyObject *
_io_FileIO_read_impl(fileio *self, PyTypeObject *cls, Py_ssize_t size);

static PyObject *
_io_FileIO_read(fileio *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "read",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t size = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &size)) {
        goto exit;
    }
skip_optional_posonly:
    return_value = _io_FileIO_read_impl(self, cls, size);

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
    {"write", _PyCFunction_CAST(_io_FileIO_write), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_FileIO_write__doc__},

static PyObject *
_io_FileIO_write_impl(fileio *self, PyTypeObject *cls, Py_buffer *b);

static PyObject *
_io_FileIO_write(fileio *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "write",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_buffer b = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &b, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = _io_FileIO_write_impl(self, cls, &b);

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
    {"seek", _PyCFunction_CAST(_io_FileIO_seek), METH_FASTCALL, _io_FileIO_seek__doc__},

static PyObject *
_io_FileIO_seek_impl(fileio *self, PyObject *pos, int whence);

static PyObject *
_io_FileIO_seek(fileio *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pos;
    int whence = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
    }
    pos = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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
    {"truncate", _PyCFunction_CAST(_io_FileIO_truncate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_FileIO_truncate__doc__},

static PyObject *
_io_FileIO_truncate_impl(fileio *self, PyTypeObject *cls, PyObject *posobj);

static PyObject *
_io_FileIO_truncate(fileio *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *posobj = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    posobj = args[0];
skip_optional_posonly:
    return_value = _io_FileIO_truncate_impl(self, cls, posobj);

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
/*[clinic end generated code: output=e3d9446b4087020e input=a9049054013a1b77]*/
