/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(mmap_mmap_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_CLOSE_METHODDEF    \
    {"close", (PyCFunction)mmap_mmap_close, METH_NOARGS, mmap_mmap_close__doc__},

static PyObject *
mmap_mmap_close_impl(mmap_object *self);

static PyObject *
mmap_mmap_close(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_close_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_read_byte__doc__,
"read_byte($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_BYTE_METHODDEF    \
    {"read_byte", (PyCFunction)mmap_mmap_read_byte, METH_NOARGS, mmap_mmap_read_byte__doc__},

static PyObject *
mmap_mmap_read_byte_impl(mmap_object *self);

static PyObject *
mmap_mmap_read_byte(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_read_byte_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_readline__doc__,
"readline($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READLINE_METHODDEF    \
    {"readline", (PyCFunction)mmap_mmap_readline, METH_NOARGS, mmap_mmap_readline__doc__},

static PyObject *
mmap_mmap_readline_impl(mmap_object *self);

static PyObject *
mmap_mmap_readline(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_readline_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_read__doc__,
"read($self, n=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(mmap_mmap_read), METH_FASTCALL, mmap_mmap_read__doc__},

static PyObject *
mmap_mmap_read_impl(mmap_object *self, Py_ssize_t num_bytes);

static PyObject *
mmap_mmap_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t num_bytes = PY_SSIZE_T_MAX;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!_Py_convert_optional_to_ssize_t(args[0], &num_bytes)) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_read_impl((mmap_object *)self, num_bytes);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_find__doc__,
"find($self, view, start=None, end=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_FIND_METHODDEF    \
    {"find", _PyCFunction_CAST(mmap_mmap_find), METH_FASTCALL, mmap_mmap_find__doc__},

static PyObject *
mmap_mmap_find_impl(mmap_object *self, Py_buffer *view, PyObject *start,
                    PyObject *end);

static PyObject *
mmap_mmap_find(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    PyObject *start = Py_None;
    PyObject *end = Py_None;

    if (!_PyArg_CheckPositional("find", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    start = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    end = args[2];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_find_impl((mmap_object *)self, &view, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_rfind__doc__,
"rfind($self, view, start=None, end=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RFIND_METHODDEF    \
    {"rfind", _PyCFunction_CAST(mmap_mmap_rfind), METH_FASTCALL, mmap_mmap_rfind__doc__},

static PyObject *
mmap_mmap_rfind_impl(mmap_object *self, Py_buffer *view, PyObject *start,
                     PyObject *end);

static PyObject *
mmap_mmap_rfind(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer view = {NULL, NULL};
    PyObject *start = Py_None;
    PyObject *end = Py_None;

    if (!_PyArg_CheckPositional("rfind", nargs, 1, 3)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &view, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    start = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    end = args[2];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_rfind_impl((mmap_object *)self, &view, start, end);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for view */
    if (view.obj) {
       PyBuffer_Release(&view);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write__doc__,
"write($self, bytes, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_METHODDEF    \
    {"write", (PyCFunction)mmap_mmap_write, METH_O, mmap_mmap_write__doc__},

static PyObject *
mmap_mmap_write_impl(mmap_object *self, Py_buffer *data);

static PyObject *
mmap_mmap_write(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_write_impl((mmap_object *)self, &data);
    Py_END_CRITICAL_SECTION();

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_write_byte__doc__,
"write_byte($self, byte, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_WRITE_BYTE_METHODDEF    \
    {"write_byte", (PyCFunction)mmap_mmap_write_byte, METH_O, mmap_mmap_write_byte__doc__},

static PyObject *
mmap_mmap_write_byte_impl(mmap_object *self, unsigned char value);

static PyObject *
mmap_mmap_write_byte(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned char value;

    {
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            value = (unsigned char) ival;
        }
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_write_byte_impl((mmap_object *)self, value);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_size__doc__,
"size($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SIZE_METHODDEF    \
    {"size", (PyCFunction)mmap_mmap_size, METH_NOARGS, mmap_mmap_size__doc__},

static PyObject *
mmap_mmap_size_impl(mmap_object *self);

static PyObject *
mmap_mmap_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_size_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if (defined(MS_WINDOWS) || defined(HAVE_MREMAP))

PyDoc_STRVAR(mmap_mmap_resize__doc__,
"resize($self, newsize, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_RESIZE_METHODDEF    \
    {"resize", (PyCFunction)mmap_mmap_resize, METH_O, mmap_mmap_resize__doc__},

static PyObject *
mmap_mmap_resize_impl(mmap_object *self, Py_ssize_t new_size);

static PyObject *
mmap_mmap_resize(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t new_size;

    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        new_size = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_resize_impl((mmap_object *)self, new_size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS) || defined(HAVE_MREMAP)) */

PyDoc_STRVAR(mmap_mmap_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_TELL_METHODDEF    \
    {"tell", (PyCFunction)mmap_mmap_tell, METH_NOARGS, mmap_mmap_tell__doc__},

static PyObject *
mmap_mmap_tell_impl(mmap_object *self);

static PyObject *
mmap_mmap_tell(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_tell_impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap_flush__doc__,
"flush($self, offset=0, size=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(mmap_mmap_flush), METH_FASTCALL, mmap_mmap_flush__doc__},

static PyObject *
mmap_mmap_flush_impl(mmap_object *self, Py_ssize_t offset, Py_ssize_t size);

static PyObject *
mmap_mmap_flush(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t offset = 0;
    Py_ssize_t size = -1;

    if (!_PyArg_CheckPositional("flush", nargs, 0, 2)) {
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
        offset = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
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
    return_value = mmap_mmap_flush_impl((mmap_object *)self, offset, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_seek__doc__,
"seek($self, pos, whence=0, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SEEK_METHODDEF    \
    {"seek", _PyCFunction_CAST(mmap_mmap_seek), METH_FASTCALL, mmap_mmap_seek__doc__},

static PyObject *
mmap_mmap_seek_impl(mmap_object *self, Py_ssize_t dist, int how);

static PyObject *
mmap_mmap_seek(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t dist;
    int how = 0;

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
        dist = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    how = PyLong_AsInt(args[1]);
    if (how == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_seek_impl((mmap_object *)self, dist, how);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)mmap_mmap_seekable, METH_NOARGS, mmap_mmap_seekable__doc__},

static PyObject *
mmap_mmap_seekable_impl(mmap_object *self);

static PyObject *
mmap_mmap_seekable(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return mmap_mmap_seekable_impl((mmap_object *)self);
}

PyDoc_STRVAR(mmap_mmap_move__doc__,
"move($self, dest, src, count, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MOVE_METHODDEF    \
    {"move", _PyCFunction_CAST(mmap_mmap_move), METH_FASTCALL, mmap_mmap_move__doc__},

static PyObject *
mmap_mmap_move_impl(mmap_object *self, Py_ssize_t dest, Py_ssize_t src,
                    Py_ssize_t cnt);

static PyObject *
mmap_mmap_move(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t dest;
    Py_ssize_t src;
    Py_ssize_t cnt;

    if (!_PyArg_CheckPositional("move", nargs, 3, 3)) {
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
        dest = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        src = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        cnt = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_move_impl((mmap_object *)self, dest, src, cnt);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(mmap_mmap___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)mmap_mmap___enter__, METH_NOARGS, mmap_mmap___enter____doc__},

static PyObject *
mmap_mmap___enter___impl(mmap_object *self);

static PyObject *
mmap_mmap___enter__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap___enter___impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(mmap_mmap___exit____doc__,
"__exit__($self, exc_type, exc_value, traceback, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(mmap_mmap___exit__), METH_FASTCALL, mmap_mmap___exit____doc__},

static PyObject *
mmap_mmap___exit___impl(mmap_object *self, PyObject *exc_type,
                        PyObject *exc_value, PyObject *traceback);

static PyObject *
mmap_mmap___exit__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *traceback;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = args[0];
    exc_value = args[1];
    traceback = args[2];
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap___exit___impl((mmap_object *)self, exc_type, exc_value, traceback);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(mmap_mmap___sizeof____doc__,
"__sizeof__($self, /)\n"
"--\n"
"\n");

#define MMAP_MMAP___SIZEOF___METHODDEF    \
    {"__sizeof__", (PyCFunction)mmap_mmap___sizeof__, METH_NOARGS, mmap_mmap___sizeof____doc__},

static PyObject *
mmap_mmap___sizeof___impl(mmap_object *self);

static PyObject *
mmap_mmap___sizeof__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap___sizeof___impl((mmap_object *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if (defined(MS_WINDOWS) && defined(Py_DEBUG))

PyDoc_STRVAR(mmap_mmap__protect__doc__,
"_protect($self, flNewProtect, start, length, /)\n"
"--\n"
"\n");

#define MMAP_MMAP__PROTECT_METHODDEF    \
    {"_protect", _PyCFunction_CAST(mmap_mmap__protect), METH_FASTCALL, mmap_mmap__protect__doc__},

static PyObject *
mmap_mmap__protect_impl(mmap_object *self, unsigned int flNewProtect,
                        Py_ssize_t start, Py_ssize_t length);

static PyObject *
mmap_mmap__protect(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int flNewProtect;
    Py_ssize_t start;
    Py_ssize_t length;

    if (!_PyArg_CheckPositional("_protect", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[0], &flNewProtect, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap__protect_impl((mmap_object *)self, flNewProtect, start, length);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* (defined(MS_WINDOWS) && defined(Py_DEBUG)) */

#if defined(HAVE_MADVISE)

PyDoc_STRVAR(mmap_mmap_madvise__doc__,
"madvise($self, option, start=0, length=None, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MADVISE_METHODDEF    \
    {"madvise", _PyCFunction_CAST(mmap_mmap_madvise), METH_FASTCALL, mmap_mmap_madvise__doc__},

static PyObject *
mmap_mmap_madvise_impl(mmap_object *self, int option, Py_ssize_t start,
                       PyObject *length_obj);

static PyObject *
mmap_mmap_madvise(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int option;
    Py_ssize_t start = 0;
    PyObject *length_obj = Py_None;

    if (!_PyArg_CheckPositional("madvise", nargs, 1, 3)) {
        goto exit;
    }
    option = PyLong_AsInt(args[0]);
    if (option == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        start = ival;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    length_obj = args[2];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = mmap_mmap_madvise_impl((mmap_object *)self, option, start, length_obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#endif /* defined(HAVE_MADVISE) */

#ifndef MMAP_MMAP_RESIZE_METHODDEF
    #define MMAP_MMAP_RESIZE_METHODDEF
#endif /* !defined(MMAP_MMAP_RESIZE_METHODDEF) */

#ifndef MMAP_MMAP___SIZEOF___METHODDEF
    #define MMAP_MMAP___SIZEOF___METHODDEF
#endif /* !defined(MMAP_MMAP___SIZEOF___METHODDEF) */

#ifndef MMAP_MMAP__PROTECT_METHODDEF
    #define MMAP_MMAP__PROTECT_METHODDEF
#endif /* !defined(MMAP_MMAP__PROTECT_METHODDEF) */

#ifndef MMAP_MMAP_MADVISE_METHODDEF
    #define MMAP_MMAP_MADVISE_METHODDEF
#endif /* !defined(MMAP_MMAP_MADVISE_METHODDEF) */
/*[clinic end generated code: output=381f6cf4986ac867 input=a9049054013a1b77]*/
