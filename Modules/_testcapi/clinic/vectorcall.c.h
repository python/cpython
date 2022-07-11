/*[clinic input]
preserve
[clinic start generated code]*/

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
/*[clinic end generated code: output=c2836830bee23928 input=a9049054013a1b77]*/
