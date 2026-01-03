/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_io_BytesIO_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be read.");

#define _IO_BYTESIO_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io_BytesIO_readable, METH_NOARGS, _io_BytesIO_readable__doc__},

static PyObject *
_io_BytesIO_readable_impl(bytesio *self);

static PyObject *
_io_BytesIO_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_readable_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be written.");

#define _IO_BYTESIO_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io_BytesIO_writable, METH_NOARGS, _io_BytesIO_writable__doc__},

static PyObject *
_io_BytesIO_writable_impl(bytesio *self);

static PyObject *
_io_BytesIO_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_writable_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n"
"Returns True if the IO object can be seeked.");

#define _IO_BYTESIO_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io_BytesIO_seekable, METH_NOARGS, _io_BytesIO_seekable__doc__},

static PyObject *
_io_BytesIO_seekable_impl(bytesio *self);

static PyObject *
_io_BytesIO_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_seekable_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Does nothing.");

#define _IO_BYTESIO_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io_BytesIO_flush, METH_NOARGS, _io_BytesIO_flush__doc__},

static PyObject *
_io_BytesIO_flush_impl(bytesio *self);

static PyObject *
_io_BytesIO_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_flush_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_getbuffer__doc__,
"getbuffer($self, /)\n"
"--\n"
"\n"
"Get a read-write view over the contents of the BytesIO object.");

#define _IO_BYTESIO_GETBUFFER_METHODDEF    \
    {"getbuffer", _PyCFunction_CAST(_io_BytesIO_getbuffer), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io_BytesIO_getbuffer__doc__},

static PyObject *
_io_BytesIO_getbuffer_impl(bytesio *self, PyTypeObject *cls);

static PyObject *
_io_BytesIO_getbuffer(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;

    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "getbuffer() takes no arguments");
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_getbuffer_impl((bytesio *)self, cls);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_getvalue__doc__,
"getvalue($self, /)\n"
"--\n"
"\n"
"Retrieve the entire contents of the BytesIO object.");

#define _IO_BYTESIO_GETVALUE_METHODDEF    \
    {"getvalue", (PyCFunction)_io_BytesIO_getvalue, METH_NOARGS, _io_BytesIO_getvalue__doc__},

static PyObject *
_io_BytesIO_getvalue_impl(bytesio *self);

static PyObject *
_io_BytesIO_getvalue(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_getvalue_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n"
"Always returns False.\n"
"\n"
"BytesIO objects are not connected to a TTY-like device.");

#define _IO_BYTESIO_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io_BytesIO_isatty, METH_NOARGS, _io_BytesIO_isatty__doc__},

static PyObject *
_io_BytesIO_isatty_impl(bytesio *self);

static PyObject *
_io_BytesIO_isatty(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_isatty_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n"
"Current file position, an integer.");

#define _IO_BYTESIO_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io_BytesIO_tell, METH_NOARGS, _io_BytesIO_tell__doc__},

static PyObject *
_io_BytesIO_tell_impl(bytesio *self);

static PyObject *
_io_BytesIO_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_tell_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as a bytes object.\n"
"\n"
"If the size argument is negative, read until EOF is reached.\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_BytesIO_read), METH_FASTCALL, _io_BytesIO_read__doc__},

static PyObject *
_io_BytesIO_read_impl(bytesio *self, Py_ssize_t size);

static PyObject *
_io_BytesIO_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_read_impl((bytesio *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size bytes, returned as a bytes object.\n"
"\n"
"If the size argument is negative or omitted, read until EOF is reached.\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io_BytesIO_read1), METH_FASTCALL, _io_BytesIO_read1__doc__},

static PyObject *
_io_BytesIO_read1_impl(bytesio *self, Py_ssize_t size);

static PyObject *
_io_BytesIO_read1(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("read1", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &size)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_read1_impl((bytesio *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Next line from the file, as a bytes object.\n"
"\n"
"Retain newline.  A non-negative size argument limits the maximum\n"
"number of bytes to return (an incomplete line may be returned then).\n"
"Return an empty bytes object at EOF.");

#define _IO_BYTESIO_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io_BytesIO_readline), METH_FASTCALL, _io_BytesIO_readline__doc__},

static PyObject *
_io_BytesIO_readline_impl(bytesio *self, Py_ssize_t size);

static PyObject *
_io_BytesIO_readline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_readline_impl((bytesio *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readlines__doc__,
"readlines($self, size=None, /)\n"
"--\n"
"\n"
"List of bytes objects, each a line from the file.\n"
"\n"
"Call readline() repeatedly and return a list of the lines so read.\n"
"The optional size argument, if given, is an approximate bound on the\n"
"total number of bytes in the lines returned.");

#define _IO_BYTESIO_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_io_BytesIO_readlines), METH_FASTCALL, _io_BytesIO_readlines__doc__},

static PyObject *
_io_BytesIO_readlines_impl(bytesio *self, PyObject *arg);

static PyObject *
_io_BytesIO_readlines(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *arg = Py_None;

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    arg = args[0];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_readlines_impl((bytesio *)self, arg);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_readinto__doc__,
"readinto($self, buffer, /)\n"
"--\n"
"\n"
"Read bytes into buffer.\n"
"\n"
"Returns number of bytes read (0 for EOF), or None if the object\n"
"is set not to block and has no data to read.");

#define _IO_BYTESIO_READINTO_METHODDEF    \
    {"readinto", (PyCFunction)_io_BytesIO_readinto, METH_O, _io_BytesIO_readinto__doc__},

static PyObject *
_io_BytesIO_readinto_impl(bytesio *self, Py_buffer *buffer);

static PyObject *
_io_BytesIO_readinto(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_readinto_impl((bytesio *)self, &buffer);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for buffer */
    if (buffer.obj) {
       PyBuffer_Release(&buffer);
    }

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_truncate__doc__,
"truncate($self, size=None, /)\n"
"--\n"
"\n"
"Truncate the file to at most size bytes.\n"
"\n"
"Size defaults to the current file position, as returned by tell().\n"
"The current file position is unchanged.  Returns the new size.");

#define _IO_BYTESIO_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io_BytesIO_truncate), METH_FASTCALL, _io_BytesIO_truncate__doc__},

static PyObject *
_io_BytesIO_truncate_impl(bytesio *self, PyObject *size);

static PyObject *
_io_BytesIO_truncate(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *size = Py_None;

    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    size = args[0];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_truncate_impl((bytesio *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n"
"Change stream position.\n"
"\n"
"Seek to byte offset pos relative to position indicated by whence:\n"
"     0  Start of stream (the default).  pos should be >= 0;\n"
"     1  Current position - pos may be negative;\n"
"     2  End of stream - pos usually negative.\n"
"Returns the new absolute position.");

#define _IO_BYTESIO_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(_io_BytesIO_seek), METH_FASTCALL, _io_BytesIO_seek__doc__},

static PyObject *
_io_BytesIO_seek_impl(bytesio *self, Py_ssize_t pos, int whence);

static PyObject *
_io_BytesIO_seek(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t pos;
    int whence = 0;

    if (!_PyArg_CheckPositional("seek", nargs, 1, 2)) {
        goto exit;
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
        pos = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    whence = PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_seek_impl((bytesio *)self, pos, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write bytes to file.\n"
"\n"
"Return the number of bytes written.");

#define _IO_BYTESIO_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io_BytesIO_write, METH_O, _io_BytesIO_write__doc__},

static PyObject *
_io_BytesIO_write_impl(bytesio *self, PyObject *b);

static PyObject *
_io_BytesIO_write(PyObject *self, PyObject *b)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_write_impl((bytesio *)self, b);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n"
"Write lines to the file.\n"
"\n"
"Note that newlines are not added.  lines can be any iterable object\n"
"producing bytes-like objects. This is equivalent to calling write() for\n"
"each element.");

#define _IO_BYTESIO_WRITELINES_METHODDEF    \
    {"writelines", (PyCFunction)_io_BytesIO_writelines, METH_O, _io_BytesIO_writelines__doc__},

static PyObject *
_io_BytesIO_writelines_impl(bytesio *self, PyObject *lines);

static PyObject *
_io_BytesIO_writelines(PyObject *self, PyObject *lines)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_writelines_impl((bytesio *)self, lines);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Disable all I/O operations.");

#define _IO_BYTESIO_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io_BytesIO_close, METH_NOARGS, _io_BytesIO_close__doc__},

static PyObject *
_io_BytesIO_close_impl(bytesio *self);

static PyObject *
_io_BytesIO_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO_close_impl((bytesio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_BytesIO___init____doc__,
"BytesIO(initial_bytes=b\'\')\n"
"--\n"
"\n"
"Buffered I/O implementation using an in-memory bytes buffer.");

static int
_io_BytesIO___init___impl(bytesio *self, PyObject *initvalue);

static int
_io_BytesIO___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(initial_bytes), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"initial_bytes", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BytesIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *initvalue = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    initvalue = fastargs[0];
skip_optional_pos:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BytesIO___init___impl((bytesio *)self, initvalue);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=daa81dfdae5ccc57 input=a9049054013a1b77]*/
