/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_SINGLETON()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()

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
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__BufferedIOBase_readinto_impl(self, &buffer);
    Py_END_CRITICAL_SECTION();

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
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__BufferedIOBase_readinto1_impl(self, &buffer);
    Py_END_CRITICAL_SECTION();

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
    {"detach", _PyCFunction_CAST(_io__BufferedIOBase_detach), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_detach__doc__},

static PyObject *
_io__BufferedIOBase_detach_impl(PyObject *self, PyTypeObject *cls);

static PyObject *
_io__BufferedIOBase_detach(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "detach() takes no arguments");
        return NULL;
    }
    return _io__BufferedIOBase_detach_impl(self, cls);
}

PyDoc_STRVAR(_io__BufferedIOBase_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read and return up to n bytes.\n"
"\n"
"If the size argument is omitted, None, or negative, read and\n"
"return all data until EOF.\n"
"\n"
"If the size argument is positive, and the underlying raw stream is\n"
"not \'interactive\', multiple raw reads may be issued to satisfy\n"
"the byte count (unless EOF is reached first).\n"
"However, for interactive raw streams (as well as sockets and pipes),\n"
"at most one raw read will be issued, and a short result does not\n"
"imply that EOF is imminent.\n"
"\n"
"Return an empty bytes object on EOF.\n"
"\n"
"Return None if the underlying raw stream was open in non-blocking\n"
"mode and no data is available at the moment.");

#define _IO__BUFFEREDIOBASE_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io__BufferedIOBase_read), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_read__doc__},

static PyObject *
_io__BufferedIOBase_read_impl(PyObject *self, PyTypeObject *cls,
                              int Py_UNUSED(size));

static PyObject *
_io__BufferedIOBase_read(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    int size = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    size = PyLong_AsInt(args[0]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_posonly:
    return_value = _io__BufferedIOBase_read_impl(self, cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_read1__doc__,
"read1($self, size=-1, /)\n"
"--\n"
"\n"
"Read and return up to size bytes, with at most one read() call to the underlying raw stream.\n"
"\n"
"Return an empty bytes object on EOF.\n"
"A short result does not imply that EOF is imminent.");

#define _IO__BUFFEREDIOBASE_READ1_METHODDEF    \
    {"read1", _PyCFunction_CAST(_io__BufferedIOBase_read1), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_read1__doc__},

static PyObject *
_io__BufferedIOBase_read1_impl(PyObject *self, PyTypeObject *cls,
                               int Py_UNUSED(size));

static PyObject *
_io__BufferedIOBase_read1(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "read1",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int size = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    size = PyLong_AsInt(args[0]);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_posonly:
    return_value = _io__BufferedIOBase_read1_impl(self, cls, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__BufferedIOBase_write__doc__,
"write($self, b, /)\n"
"--\n"
"\n"
"Write buffer b to the IO stream.\n"
"\n"
"Return the number of bytes written, which is always\n"
"the length of b in bytes.\n"
"\n"
"Raise BlockingIOError if the buffer is full and the\n"
"underlying raw stream cannot accept more data at the moment.");

#define _IO__BUFFEREDIOBASE_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_io__BufferedIOBase_write), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__BufferedIOBase_write__doc__},

static PyObject *
_io__BufferedIOBase_write_impl(PyObject *self, PyTypeObject *cls,
                               PyObject *Py_UNUSED(b));

static PyObject *
_io__BufferedIOBase_write(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *b;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    b = args[0];
    return_value = _io__BufferedIOBase_write_impl(self, cls, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)_io__Buffered___sizeof__, METH_NOARGS, _io__Buffered___sizeof____doc__},

static PyObject *
_io__Buffered___sizeof___impl(buffered *self);

static PyObject *
_io__Buffered___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered___sizeof___impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered__dealloc_warn__doc__,
"_dealloc_warn($self, source, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED__DEALLOC_WARN_METHODDEF    \
    {"_dealloc_warn", (PyCFunction)_io__Buffered__dealloc_warn, METH_O, _io__Buffered__dealloc_warn__doc__},

static PyObject *
_io__Buffered__dealloc_warn_impl(buffered *self, PyObject *source);

static PyObject *
_io__Buffered__dealloc_warn(PyObject *self, PyObject *source)
{
    PyObject *return_value = NULL;

    return_value = _io__Buffered__dealloc_warn_impl((buffered *)self, source);

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_simple_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SIMPLE_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io__Buffered_simple_flush, METH_NOARGS, _io__Buffered_simple_flush__doc__},

static PyObject *
_io__Buffered_simple_flush_impl(buffered *self);

static PyObject *
_io__Buffered_simple_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_simple_flush_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io__Buffered_closed_DOCSTR)
#  define _io__Buffered_closed_DOCSTR NULL
#endif
#if defined(_IO__BUFFERED_CLOSED_GETSETDEF)
#  undef _IO__BUFFERED_CLOSED_GETSETDEF
#  define _IO__BUFFERED_CLOSED_GETSETDEF {"closed", (getter)_io__Buffered_closed_get, (setter)_io__Buffered_closed_set, _io__Buffered_closed_DOCSTR},
#else
#  define _IO__BUFFERED_CLOSED_GETSETDEF {"closed", (getter)_io__Buffered_closed_get, NULL, _io__Buffered_closed_DOCSTR},
#endif

static PyObject *
_io__Buffered_closed_get_impl(buffered *self);

static PyObject *
_io__Buffered_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_closed_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io__Buffered_close, METH_NOARGS, _io__Buffered_close__doc__},

static PyObject *
_io__Buffered_close_impl(buffered *self);

static PyObject *
_io__Buffered_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_close_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_DETACH_METHODDEF    \
    {"detach", (PyCFunction)_io__Buffered_detach, METH_NOARGS, _io__Buffered_detach__doc__},

static PyObject *
_io__Buffered_detach_impl(buffered *self);

static PyObject *
_io__Buffered_detach(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_detach_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io__Buffered_seekable, METH_NOARGS, _io__Buffered_seekable__doc__},

static PyObject *
_io__Buffered_seekable_impl(buffered *self);

static PyObject *
_io__Buffered_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_seekable_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io__Buffered_readable, METH_NOARGS, _io__Buffered_readable__doc__},

static PyObject *
_io__Buffered_readable_impl(buffered *self);

static PyObject *
_io__Buffered_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_readable_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io__Buffered_writable, METH_NOARGS, _io__Buffered_writable__doc__},

static PyObject *
_io__Buffered_writable_impl(buffered *self);

static PyObject *
_io__Buffered_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_writable_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io__Buffered_name_DOCSTR)
#  define _io__Buffered_name_DOCSTR NULL
#endif
#if defined(_IO__BUFFERED_NAME_GETSETDEF)
#  undef _IO__BUFFERED_NAME_GETSETDEF
#  define _IO__BUFFERED_NAME_GETSETDEF {"name", (getter)_io__Buffered_name_get, (setter)_io__Buffered_name_set, _io__Buffered_name_DOCSTR},
#else
#  define _IO__BUFFERED_NAME_GETSETDEF {"name", (getter)_io__Buffered_name_get, NULL, _io__Buffered_name_DOCSTR},
#endif

static PyObject *
_io__Buffered_name_get_impl(buffered *self);

static PyObject *
_io__Buffered_name_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_name_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io__Buffered_mode_DOCSTR)
#  define _io__Buffered_mode_DOCSTR NULL
#endif
#if defined(_IO__BUFFERED_MODE_GETSETDEF)
#  undef _IO__BUFFERED_MODE_GETSETDEF
#  define _IO__BUFFERED_MODE_GETSETDEF {"mode", (getter)_io__Buffered_mode_get, (setter)_io__Buffered_mode_set, _io__Buffered_mode_DOCSTR},
#else
#  define _IO__BUFFERED_MODE_GETSETDEF {"mode", (getter)_io__Buffered_mode_get, NULL, _io__Buffered_mode_DOCSTR},
#endif

static PyObject *
_io__Buffered_mode_get_impl(buffered *self);

static PyObject *
_io__Buffered_mode_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_mode_get_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)_io__Buffered_fileno, METH_NOARGS, _io__Buffered_fileno__doc__},

static PyObject *
_io__Buffered_fileno_impl(buffered *self);

static PyObject *
_io__Buffered_fileno(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_fileno_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io__Buffered_isatty, METH_NOARGS, _io__Buffered_isatty__doc__},

static PyObject *
_io__Buffered_isatty_impl(buffered *self);

static PyObject *
_io__Buffered_isatty(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_isatty_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io__Buffered_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io__Buffered_flush, METH_NOARGS, _io__Buffered_flush__doc__},

static PyObject *
_io__Buffered_flush_impl(buffered *self);

static PyObject *
_io__Buffered_flush(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_flush_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
_io__Buffered_peek(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_peek_impl((buffered *)self, size);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_read_impl((buffered *)self, n);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_read1(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_read1_impl((buffered *)self, n);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_readinto(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_readinto_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_readinto1(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_WRITABLE) < 0) {
        _PyArg_BadArgument("readinto1", "argument", "read-write bytes-like object", arg);
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_readinto1_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_readline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io__Buffered_readline_impl((buffered *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io__Buffered_tell, METH_NOARGS, _io__Buffered_tell__doc__},

static PyObject *
_io__Buffered_tell_impl(buffered *self);

static PyObject *
_io__Buffered_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_tell_impl((buffered *)self);
    Py_END_CRITICAL_SECTION();

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
_io__Buffered_seek(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    whence = PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_seek_impl((buffered *)self, targetobj, whence);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io__Buffered_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO__BUFFERED_TRUNCATE_METHODDEF    \
    {"truncate", _PyCFunction_CAST(_io__Buffered_truncate), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _io__Buffered_truncate__doc__},

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyTypeObject *cls, PyObject *pos);

static PyObject *
_io__Buffered_truncate(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    PyObject *pos = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    pos = args[0];
skip_optional_posonly:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io__Buffered_truncate_impl((buffered *)self, cls, pos);
    Py_END_CRITICAL_SECTION();

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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedReader",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedWriter",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
_io_BufferedWriter_write(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer buffer = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &buffer, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_BufferedWriter_write_impl((buffered *)self, &buffer);
    Py_END_CRITICAL_SECTION();

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
    PyTypeObject *base_tp = clinic_state()->PyBufferedRWPair_Type;
    PyObject *reader;
    PyObject *writer;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(raw), &_Py_ID(buffer_size), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"raw", "buffer_size", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "BufferedRandom",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *raw;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
/*[clinic end generated code: output=3ee17211d2010462 input=a9049054013a1b77]*/
