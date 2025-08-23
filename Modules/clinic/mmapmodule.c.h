/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(mmap_mmap_flush__doc__,
"flush($self, offset=0, size=None, /)\n"
"--\n"
"\n"
"Flushes changes made to the in-memory copy of a file back to disk.\n"
"\n"
"If offset and size are specified, only the specified range will\n"
"be flushed. If not specified, the entire mapped region will be\n"
"flushed.");

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
    return_value = mmap_mmap_flush_impl((mmap_object *)self, offset, size);

exit:
    return return_value;
}
/*[clinic end generated code: output=c95c7ffe05f26c3a input=a9049054013a1b77]*/
