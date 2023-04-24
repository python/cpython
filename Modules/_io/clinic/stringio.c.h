/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


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
_io_StringIO_read(stringio *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io_StringIO_read_impl(self, size);

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
_io_StringIO_readline(stringio *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _io_StringIO_readline_impl(self, size);

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
_io_StringIO_truncate_impl(stringio *self, Py_ssize_t size);

static PyObject *
_io_StringIO_truncate(stringio *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = self->pos;

    if (!_PyArg_CheckPositional("truncate", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &size)) {
        goto exit;
    }
skip_optional:
    return_value = _io_StringIO_truncate_impl(self, size);

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
_io_StringIO_seek(stringio *self, PyObject *const *args, Py_ssize_t nargs)
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
    whence = _PyLong_AsInt(args[1]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
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
/*[clinic end generated code: output=533f20ae9b773126 input=a9049054013a1b77]*/
