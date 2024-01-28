/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(mmap_mmap_mprotect__doc__,
"mprotect($self, prot, start=0, length=-1, /)\n"
"--\n"
"\n");

#define MMAP_MMAP_MPROTECT_METHODDEF    \
    {"mprotect", _PyCFunction_CAST(mmap_mmap_mprotect), METH_FASTCALL, mmap_mmap_mprotect__doc__},

static PyObject *
mmap_mmap_mprotect_impl(mmap_object *self, int prot, Py_ssize_t start,
                        Py_ssize_t length);

static PyObject *
mmap_mmap_mprotect(mmap_object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int prot;
    Py_ssize_t start = 0;
    Py_ssize_t length = -1;

    if (!_PyArg_CheckPositional("mprotect", nargs, 1, 3)) {
        goto exit;
    }
    prot = PyLong_AsInt(args[0]);
    if (prot == -1 && PyErr_Occurred()) {
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
skip_optional:
    return_value = mmap_mmap_mprotect_impl(self, prot, start, length);

exit:
    return return_value;
}
/*[clinic end generated code: output=3acd77c6d0a50f49 input=a9049054013a1b77]*/
