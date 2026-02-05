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
_io_StringIO_getvalue(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_getvalue_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
_io_StringIO_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_tell_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n"
"Read at most size characters, returned as a string.\n"
"\n"
"If the argument is negative or omitted, read until EOF\n"
"is reached. Return an empty string at EOF.");

#define _IO_STRINGIO_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_io_StringIO_read), METH_FASTCALL, _io_StringIO_read__doc__},

static PyObject *
_io_StringIO_read_impl(stringio *self, Py_ssize_t size);

static PyObject *
_io_StringIO_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io_StringIO_read_impl((stringio *)self, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_io_StringIO_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n"
"Read until newline or EOF.\n"
"\n"
"Returns an empty string if EOF is hit immediately.");

#define _IO_STRINGIO_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_io_StringIO_readline), METH_FASTCALL, _io_StringIO_readline__doc__},

static PyObject *
_io_StringIO_readline_impl(stringio *self, Py_ssize_t size);

static PyObject *
_io_StringIO_readline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io_StringIO_readline_impl((stringio *)self, size);
    Py_END_CRITICAL_SECTION();

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
    {"truncate", _PyCFunction_CAST(_io_StringIO_truncate), METH_FASTCALL, _io_StringIO_truncate__doc__},

static PyObject *
_io_StringIO_truncate_impl(stringio *self, PyObject *pos);

static PyObject *
_io_StringIO_truncate(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_truncate_impl((stringio *)self, pos);
    Py_END_CRITICAL_SECTION();

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
    {"seek", _PyCFunction_CAST(_io_StringIO_seek), METH_FASTCALL, _io_StringIO_seek__doc__},

static PyObject *
_io_StringIO_seek_impl(stringio *self, Py_ssize_t pos, int whence);

static PyObject *
_io_StringIO_seek(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io_StringIO_seek_impl((stringio *)self, pos, whence);
    Py_END_CRITICAL_SECTION();

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

static PyObject *
_io_StringIO_write_impl(stringio *self, PyObject *obj);

static PyObject *
_io_StringIO_write(PyObject *self, PyObject *obj)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_write_impl((stringio *)self, obj);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

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
_io_StringIO_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_close_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
        .ob_item = { &_Py_ID(initial_value), &_Py_ID(newline), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"initial_value", "newline", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "StringIO",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *value = NULL;
    PyObject *newline_obj = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        value = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    newline_obj = fastargs[1];
skip_optional_pos:
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
_io_StringIO_readable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_readable_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
_io_StringIO_writable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_writable_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
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
_io_StringIO_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_seekable_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO___getstate____doc__,
"__getstate__($self, /)\n"
"--\n"
"\n");

#define _IO_STRINGIO___GETSTATE___METHODDEF    \
    {"__getstate__", (PyCFunction)_io_StringIO___getstate__, METH_NOARGS, _io_StringIO___getstate____doc__},

static PyObject *
_io_StringIO___getstate___impl(stringio *self);

static PyObject *
_io_StringIO___getstate__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO___getstate___impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_io_StringIO___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define _IO_STRINGIO___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)_io_StringIO___setstate__, METH_O, _io_StringIO___setstate____doc__},

static PyObject *
_io_StringIO___setstate___impl(stringio *self, PyObject *state);

static PyObject *
_io_StringIO___setstate__(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO___setstate___impl((stringio *)self, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io_StringIO_closed_DOCSTR)
#  define _io_StringIO_closed_DOCSTR NULL
#endif
#if defined(_IO_STRINGIO_CLOSED_GETSETDEF)
#  undef _IO_STRINGIO_CLOSED_GETSETDEF
#  define _IO_STRINGIO_CLOSED_GETSETDEF {"closed", (getter)_io_StringIO_closed_get, (setter)_io_StringIO_closed_set, _io_StringIO_closed_DOCSTR},
#else
#  define _IO_STRINGIO_CLOSED_GETSETDEF {"closed", (getter)_io_StringIO_closed_get, NULL, _io_StringIO_closed_DOCSTR},
#endif

static PyObject *
_io_StringIO_closed_get_impl(stringio *self);

static PyObject *
_io_StringIO_closed_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_closed_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io_StringIO_line_buffering_DOCSTR)
#  define _io_StringIO_line_buffering_DOCSTR NULL
#endif
#if defined(_IO_STRINGIO_LINE_BUFFERING_GETSETDEF)
#  undef _IO_STRINGIO_LINE_BUFFERING_GETSETDEF
#  define _IO_STRINGIO_LINE_BUFFERING_GETSETDEF {"line_buffering", (getter)_io_StringIO_line_buffering_get, (setter)_io_StringIO_line_buffering_set, _io_StringIO_line_buffering_DOCSTR},
#else
#  define _IO_STRINGIO_LINE_BUFFERING_GETSETDEF {"line_buffering", (getter)_io_StringIO_line_buffering_get, NULL, _io_StringIO_line_buffering_DOCSTR},
#endif

static PyObject *
_io_StringIO_line_buffering_get_impl(stringio *self);

static PyObject *
_io_StringIO_line_buffering_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_line_buffering_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(_io_StringIO_newlines_DOCSTR)
#  define _io_StringIO_newlines_DOCSTR NULL
#endif
#if defined(_IO_STRINGIO_NEWLINES_GETSETDEF)
#  undef _IO_STRINGIO_NEWLINES_GETSETDEF
#  define _IO_STRINGIO_NEWLINES_GETSETDEF {"newlines", (getter)_io_StringIO_newlines_get, (setter)_io_StringIO_newlines_set, _io_StringIO_newlines_DOCSTR},
#else
#  define _IO_STRINGIO_NEWLINES_GETSETDEF {"newlines", (getter)_io_StringIO_newlines_get, NULL, _io_StringIO_newlines_DOCSTR},
#endif

static PyObject *
_io_StringIO_newlines_get_impl(stringio *self);

static PyObject *
_io_StringIO_newlines_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _io_StringIO_newlines_get_impl((stringio *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=bccc25ef8e6ce9ef input=a9049054013a1b77]*/
