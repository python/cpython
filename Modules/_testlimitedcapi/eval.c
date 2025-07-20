#include "parts.h"
#include "util.h"

static PyObject *
eval_get_func_name(PyObject *self, PyObject *func)
{
    return PyUnicode_FromString(PyEval_GetFuncName(func));
}

static PyObject *
eval_get_func_desc(PyObject *self, PyObject *func)
{
    return PyUnicode_FromString(PyEval_GetFuncDesc(func));
}

static PyObject *
eval_getlocals(PyObject *module, PyObject *Py_UNUSED(args))
{
    return Py_XNewRef(PyEval_GetLocals());
}

static PyObject *
eval_getglobals(PyObject *module, PyObject *Py_UNUSED(args))
{
    return Py_XNewRef(PyEval_GetGlobals());
}

static PyObject *
eval_getbuiltins(PyObject *module, PyObject *Py_UNUSED(args))
{
    return Py_XNewRef(PyEval_GetBuiltins());
}

static PyObject *
eval_getframe(PyObject *module, PyObject *Py_UNUSED(args))
{
    return Py_XNewRef(PyEval_GetFrame());
}

static PyObject *
eval_getframe_builtins(PyObject *module, PyObject *Py_UNUSED(args))
{
    return PyEval_GetFrameBuiltins();
}

static PyObject *
eval_getframe_globals(PyObject *module, PyObject *Py_UNUSED(args))
{
    return PyEval_GetFrameGlobals();
}

static PyObject *
eval_getframe_locals(PyObject *module, PyObject *Py_UNUSED(args))
{
    return PyEval_GetFrameLocals();
}

static PyObject *
eval_get_recursion_limit(PyObject *module, PyObject *Py_UNUSED(args))
{
    int limit = Py_GetRecursionLimit();
    return PyLong_FromLong(limit);
}

static PyObject *
eval_set_recursion_limit(PyObject *module, PyObject *args)
{
    int limit;
    if (!PyArg_ParseTuple(args, "i", &limit)) {
        return NULL;
    }
    Py_SetRecursionLimit(limit);
    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"eval_get_func_name", eval_get_func_name, METH_O, NULL},
    {"eval_get_func_desc", eval_get_func_desc, METH_O, NULL},
    {"eval_getlocals", eval_getlocals, METH_NOARGS},
    {"eval_getglobals", eval_getglobals, METH_NOARGS},
    {"eval_getbuiltins", eval_getbuiltins, METH_NOARGS},
    {"eval_getframe", eval_getframe, METH_NOARGS},
    {"eval_getframe_builtins", eval_getframe_builtins, METH_NOARGS},
    {"eval_getframe_globals", eval_getframe_globals, METH_NOARGS},
    {"eval_getframe_locals", eval_getframe_locals, METH_NOARGS},
    {"eval_get_recursion_limit", eval_get_recursion_limit, METH_NOARGS},
    {"eval_set_recursion_limit", eval_set_recursion_limit, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Eval(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
