/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_ctypes_sizeof__doc__,
"sizeof($module, obj, /)\n"
"--\n"
"\n"
"Return the size in bytes of a C instance.");

#define _CTYPES_SIZEOF_METHODDEF    \
    {"sizeof", (PyCFunction)_ctypes_sizeof, METH_O, _ctypes_sizeof__doc__},

PyDoc_STRVAR(_ctypes_byref__doc__,
"byref($module, obj, offset=0, /)\n"
"--\n"
"\n"
"Return a pointer lookalike to a C instance, only usable as function argument.");

#define _CTYPES_BYREF_METHODDEF    \
    {"byref", _PyCFunction_CAST(_ctypes_byref), METH_FASTCALL, _ctypes_byref__doc__},

static PyObject *
_ctypes_byref_impl(PyObject *module, PyObject *obj, Py_ssize_t offset);

static PyObject *
_ctypes_byref(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    Py_ssize_t offset = 0;

    if (!_PyArg_CheckPositional("byref", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("byref", "argument 1", (clinic_state()->PyCData_Type)->tp_name, args[0]);
        goto exit;
    }
    obj = args[0];
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
        offset = ival;
    }
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_byref_impl(module, obj, offset);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_addressof__doc__,
"addressof($module, obj, /)\n"
"--\n"
"\n"
"Return the address of the C instance internal buffer");

#define _CTYPES_ADDRESSOF_METHODDEF    \
    {"addressof", (PyCFunction)_ctypes_addressof, METH_O, _ctypes_addressof__doc__},

static PyObject *
_ctypes_addressof_impl(PyObject *module, PyObject *obj);

static PyObject *
_ctypes_addressof(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *obj;

    if (!PyObject_TypeCheck(arg, clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("addressof", "argument", (clinic_state()->PyCData_Type)->tp_name, arg);
        goto exit;
    }
    obj = arg;
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_addressof_impl(module, obj);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_ctypes_resize__doc__,
"resize($module, obj, size, /)\n"
"--\n"
"\n");

#define _CTYPES_RESIZE_METHODDEF    \
    {"resize", _PyCFunction_CAST(_ctypes_resize), METH_FASTCALL, _ctypes_resize__doc__},

static PyObject *
_ctypes_resize_impl(PyObject *module, CDataObject *obj, Py_ssize_t size);

static PyObject *
_ctypes_resize(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    CDataObject *obj;
    Py_ssize_t size;

    if (!_PyArg_CheckPositional("resize", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], clinic_state()->PyCData_Type)) {
        _PyArg_BadArgument("resize", "argument 1", (clinic_state()->PyCData_Type)->tp_name, args[0]);
        goto exit;
    }
    obj = (CDataObject *)args[0];
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
    Py_BEGIN_CRITICAL_SECTION(obj);
    return_value = _ctypes_resize_impl(module, obj, size);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=23c74aced603977d input=a9049054013a1b77]*/
