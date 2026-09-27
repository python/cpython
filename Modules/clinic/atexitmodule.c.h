/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(atexit_register__doc__,
"register($module, func, /, *args, **kwargs)\n"
"--\n"
"\n"
"Register a function to be executed upon normal program termination\n"
"\n"
"    func - function to be called at exit\n"
"    args - optional arguments to pass to func\n"
"    kwargs - optional keyword arguments to pass to func\n"
"\n"
"    func is returned to facilitate usage as a decorator.");

#define ATEXIT_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(atexit_register), METH_VARARGS|METH_KEYWORDS, atexit_register__doc__},

static PyObject *
atexit_register_impl(PyObject *module, PyObject *func, PyObject *args,
                     PyObject *kwargs);

static PyObject *
atexit_register(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *__clinic_args = NULL;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("register", PyTuple_GET_SIZE(args), 1, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    func = PyTuple_GET_ITEM(args, 0);
    __clinic_args = PyTuple_GetSlice(args, 1, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    if (kwargs == NULL) {
        __clinic_kwargs = PyDict_New();
        if (__clinic_kwargs == NULL) {
            goto exit;
        }
    }
    else {
        __clinic_kwargs = Py_NewRef(kwargs);
    }
    return_value = atexit_register_impl(module, func, __clinic_args, __clinic_kwargs);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);
    /* Cleanup for kwargs */
    Py_XDECREF(__clinic_kwargs);

    return return_value;
}
/*[clinic end generated code: output=8fd8f691fdbc4e0a input=a9049054013a1b77]*/
