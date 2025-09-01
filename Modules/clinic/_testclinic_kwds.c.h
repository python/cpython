/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_long.h"          // _PyLong_UnsignedShort_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_tuple.h"         // _PyTuple_FromArray()

PyDoc_STRVAR(lone_kwds__doc__,
"lone_kwds($module, /, **kwds)\n"
"--\n"
"\n");

#define LONE_KWDS_METHODDEF    \
    {"lone_kwds", _PyCFunction_CAST(lone_kwds), METH_VARARGS|METH_KEYWORDS, lone_kwds__doc__},

static PyObject *
lone_kwds_impl(PyObject *module, PyObject *kwds);

static PyObject *
lone_kwds(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *kwds = NULL;

    if (!_PyArg_NoPositional("lone_kwds", args)) {
        goto exit;
    }
    kwds = (kwargs != NULL) ? Py_NewRef(kwargs) : PyDict_New();
    return_value = lone_kwds_impl(module, kwds);

exit:
    /* Cleanup for kwds */
    Py_XDECREF(kwds);

    return return_value;
}

PyDoc_STRVAR(kwds_with_pos_only__doc__,
"kwds_with_pos_only($module, a, b, /, **kwds)\n"
"--\n"
"\n");

#define KWDS_WITH_POS_ONLY_METHODDEF    \
    {"kwds_with_pos_only", _PyCFunction_CAST(kwds_with_pos_only), METH_VARARGS|METH_KEYWORDS, kwds_with_pos_only__doc__},

static PyObject *
kwds_with_pos_only_impl(PyObject *module, PyObject *a, PyObject *b,
                        PyObject *kwds);

static PyObject *
kwds_with_pos_only(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    PyObject *kwds = NULL;

    if (!_PyArg_CheckPositional("kwds_with_pos_only", PyTuple_GET_SIZE(args), 2, 2)) {
        goto exit;
    }
    a = PyTuple_GET_ITEM(args, 0);
    b = PyTuple_GET_ITEM(args, 1);
    kwds = (kwargs != NULL) ? Py_NewRef(kwargs) : PyDict_New();
    return_value = kwds_with_pos_only_impl(module, a, b, kwds);

exit:
    /* Cleanup for kwds */
    Py_XDECREF(kwds);

    return return_value;
}

PyDoc_STRVAR(kwds_with_stararg__doc__,
"kwds_with_stararg($module, /, *args, **kwds)\n"
"--\n"
"\n");

#define KWDS_WITH_STARARG_METHODDEF    \
    {"kwds_with_stararg", _PyCFunction_CAST(kwds_with_stararg), METH_VARARGS|METH_KEYWORDS, kwds_with_stararg__doc__},

static PyObject *
kwds_with_stararg_impl(PyObject *module, PyObject *args, PyObject *kwds);

static PyObject *
kwds_with_stararg(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;
    PyObject *kwds = NULL;

    __clinic_args = Py_NewRef(args);
    kwds = (kwargs != NULL) ? Py_NewRef(kwargs) : PyDict_New();
    return_value = kwds_with_stararg_impl(module, __clinic_args, kwds);

    /* Cleanup for args */
    Py_XDECREF(__clinic_args);
    /* Cleanup for kwds */
    Py_XDECREF(kwds);

    return return_value;
}

PyDoc_STRVAR(kwds_with_pos_only_and_stararg__doc__,
"kwds_with_pos_only_and_stararg($module, a, b, /, *args, **kwds)\n"
"--\n"
"\n");

#define KWDS_WITH_POS_ONLY_AND_STARARG_METHODDEF    \
    {"kwds_with_pos_only_and_stararg", _PyCFunction_CAST(kwds_with_pos_only_and_stararg), METH_VARARGS|METH_KEYWORDS, kwds_with_pos_only_and_stararg__doc__},

static PyObject *
kwds_with_pos_only_and_stararg_impl(PyObject *module, PyObject *a,
                                    PyObject *b, PyObject *args,
                                    PyObject *kwds);

static PyObject *
kwds_with_pos_only_and_stararg(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;
    PyObject *__clinic_args = NULL;
    PyObject *kwds = NULL;

    if (!_PyArg_CheckPositional("kwds_with_pos_only_and_stararg", PyTuple_GET_SIZE(args), 2, PY_SSIZE_T_MAX)) {
        goto exit;
    }
    a = PyTuple_GET_ITEM(args, 0);
    b = PyTuple_GET_ITEM(args, 1);
    __clinic_args = PyTuple_GetSlice(args, 2, PY_SSIZE_T_MAX);
    if (!__clinic_args) {
        goto exit;
    }
    kwds = (kwargs != NULL) ? Py_NewRef(kwargs) : PyDict_New();
    return_value = kwds_with_pos_only_and_stararg_impl(module, a, b, __clinic_args, kwds);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);
    /* Cleanup for kwds */
    Py_XDECREF(kwds);

    return return_value;
}
/*[clinic end generated code: output=0a1e2c2244a10a50 input=a9049054013a1b77]*/
