/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_pyobject_fastcalldict__doc__,
"pyobject_fastcalldict($module, func, func_args, kwargs, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYOBJECT_FASTCALLDICT_METHODDEF    \
    {"pyobject_fastcalldict", _PyCFunction_CAST(_testcapi_pyobject_fastcalldict), METH_FASTCALL, _testcapi_pyobject_fastcalldict__doc__},

static PyObject *
_testcapi_pyobject_fastcalldict_impl(PyObject *module, PyObject *func,
                                     PyObject *func_args, PyObject *kwargs);

static PyObject *
_testcapi_pyobject_fastcalldict(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *func_args;
    PyObject *__clinic_kwargs;

    if (!_PyArg_CheckPositional("pyobject_fastcalldict", nargs, 3, 3)) {
        goto exit;
    }
    func = args[0];
    func_args = args[1];
    __clinic_kwargs = args[2];
    return_value = _testcapi_pyobject_fastcalldict_impl(module, func, func_args, __clinic_kwargs);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyobject_vectorcall__doc__,
"pyobject_vectorcall($module, func, func_args, kwnames, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYOBJECT_VECTORCALL_METHODDEF    \
    {"pyobject_vectorcall", _PyCFunction_CAST(_testcapi_pyobject_vectorcall), METH_FASTCALL, _testcapi_pyobject_vectorcall__doc__},

static PyObject *
_testcapi_pyobject_vectorcall_impl(PyObject *module, PyObject *func,
                                   PyObject *func_args, PyObject *kwnames);

static PyObject *
_testcapi_pyobject_vectorcall(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *func_args;
    PyObject *__clinic_kwnames;

    if (!_PyArg_CheckPositional("pyobject_vectorcall", nargs, 3, 3)) {
        goto exit;
    }
    func = args[0];
    func_args = args[1];
    __clinic_kwnames = args[2];
    return_value = _testcapi_pyobject_vectorcall_impl(module, func, func_args, __clinic_kwnames);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyvectorcall_call__doc__,
"pyvectorcall_call($module, func, argstuple, kwargs=<unrepresentable>, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYVECTORCALL_CALL_METHODDEF    \
    {"pyvectorcall_call", _PyCFunction_CAST(_testcapi_pyvectorcall_call), METH_FASTCALL, _testcapi_pyvectorcall_call__doc__},

static PyObject *
_testcapi_pyvectorcall_call_impl(PyObject *module, PyObject *func,
                                 PyObject *argstuple, PyObject *kwargs);

static PyObject *
_testcapi_pyvectorcall_call(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *argstuple;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("pyvectorcall_call", nargs, 2, 3)) {
        goto exit;
    }
    func = args[0];
    argstuple = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    __clinic_kwargs = args[2];
skip_optional:
    return_value = _testcapi_pyvectorcall_call_impl(module, func, argstuple, __clinic_kwargs);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_VectorCallClass_set_vectorcall__doc__,
"set_vectorcall($self, type, /)\n"
"--\n"
"\n"
"Set self\'s vectorcall function for `type` to one that returns \"vectorcall\"");

#define _TESTCAPI_VECTORCALLCLASS_SET_VECTORCALL_METHODDEF    \
    {"set_vectorcall", (PyCFunction)_testcapi_VectorCallClass_set_vectorcall, METH_O, _testcapi_VectorCallClass_set_vectorcall__doc__},

static PyObject *
_testcapi_VectorCallClass_set_vectorcall_impl(PyObject *self,
                                              PyTypeObject *type);

static PyObject *
_testcapi_VectorCallClass_set_vectorcall(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyTypeObject *type;

    if (!PyObject_TypeCheck(arg, &PyType_Type)) {
        _PyArg_BadArgument("set_vectorcall", "argument", (&PyType_Type)->tp_name, arg);
        goto exit;
    }
    type = (PyTypeObject *)arg;
    return_value = _testcapi_VectorCallClass_set_vectorcall_impl(self, type);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_make_vectorcall_class__doc__,
"make_vectorcall_class($module, base=<unrepresentable>, /)\n"
"--\n"
"\n"
"Create a class whose instances return \"tpcall\" when called.\n"
"\n"
"When the \"set_vectorcall\" method is called on an instance, a vectorcall\n"
"function that returns \"vectorcall\" will be installed.");

#define _TESTCAPI_MAKE_VECTORCALL_CLASS_METHODDEF    \
    {"make_vectorcall_class", _PyCFunction_CAST(_testcapi_make_vectorcall_class), METH_FASTCALL, _testcapi_make_vectorcall_class__doc__},

static PyObject *
_testcapi_make_vectorcall_class_impl(PyObject *module, PyTypeObject *base);

static PyObject *
_testcapi_make_vectorcall_class(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base = NULL;

    if (!_PyArg_CheckPositional("make_vectorcall_class", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    if (!PyObject_TypeCheck(args[0], &PyType_Type)) {
        _PyArg_BadArgument("make_vectorcall_class", "argument 1", (&PyType_Type)->tp_name, args[0]);
        goto exit;
    }
    base = (PyTypeObject *)args[0];
skip_optional:
    return_value = _testcapi_make_vectorcall_class_impl(module, base);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_has_vectorcall_flag__doc__,
"has_vectorcall_flag($module, type, /)\n"
"--\n"
"\n"
"Return true iff Py_TPFLAGS_HAVE_VECTORCALL is set on the class.");

#define _TESTCAPI_HAS_VECTORCALL_FLAG_METHODDEF    \
    {"has_vectorcall_flag", (PyCFunction)_testcapi_has_vectorcall_flag, METH_O, _testcapi_has_vectorcall_flag__doc__},

static int
_testcapi_has_vectorcall_flag_impl(PyObject *module, PyTypeObject *type);

static PyObject *
_testcapi_has_vectorcall_flag(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyTypeObject *type;
    int _return_value;

    if (!PyObject_TypeCheck(arg, &PyType_Type)) {
        _PyArg_BadArgument("has_vectorcall_flag", "argument", (&PyType_Type)->tp_name, arg);
        goto exit;
    }
    type = (PyTypeObject *)arg;
    _return_value = _testcapi_has_vectorcall_flag_impl(module, type);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=210ae67caab177ba input=a9049054013a1b77]*/
