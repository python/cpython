/*
 *  atexit - allow programmer to define multiple exit functions to be executed
 *  upon normal program termination.
 *
 *   Translated from atexit.py by Collin Winter.
 +   Copyright 2007 Python Software Foundation.
 */

#include "Python.h"

/* ===================================================================== */
/* Callback machinery. */

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} atexit_callback;

atexit_callback **atexit_callbacks;
int ncallbacks = 0;
int callback_len = 32;

/* Installed into pythonrun.c's atexit mechanism */

void
atexit_callfuncs(void)
{
    PyObject *exc_type = NULL, *exc_value, *exc_tb, *r;
    atexit_callback *cb;
    int i;
    
    if (ncallbacks == 0)
        return;
        
    for(i = ncallbacks - 1; i >= 0; i--)
    {
        cb = atexit_callbacks[i];
        if (cb == NULL)
            continue;

        r = PyObject_Call(cb->func, cb->args, cb->kwargs);
        Py_XDECREF(r);
        if (r == NULL) {
            if (exc_type) {
                Py_DECREF(exc_type);
                Py_DECREF(exc_value);
                Py_DECREF(exc_tb);    
            }
            PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
            if (!PyErr_ExceptionMatches(PyExc_SystemExit)) {
                PySys_WriteStderr("Error in atexit._run_exitfuncs:\n");
                PyErr_Display(exc_type, exc_value, exc_tb);
            }
        }
    }
    
    if (exc_type)
        PyErr_Restore(exc_type, exc_value, exc_tb);
}

void
atexit_delete_cb(int i)
{
    atexit_callback *cb = atexit_callbacks[i];
    atexit_callbacks[i] = NULL;
    Py_DECREF(cb->func);
    Py_DECREF(cb->args);
    Py_XDECREF(cb->kwargs);
    PyMem_Free(cb);    
}

/* ===================================================================== */
/* Module methods. */

PyDoc_STRVAR(atexit_register__doc__,
"register(func, *args, **kwargs) -> func\n\
\n\
Register a function to be executed upon normal program termination\n\
\n\
    func - function to be called at exit\n\
    args - optional arguments to pass to func\n\
    kwargs - optional keyword arguments to pass to func\n\
\n\
    func is returned to facilitate usage as a decorator.");

static PyObject *
atexit_register(PyObject *self, PyObject *args, PyObject *kwargs)
{
    atexit_callback *new_callback;
    PyObject *func = NULL;
    
    if (ncallbacks >= callback_len) {
        callback_len += 16;
        atexit_callbacks = PyMem_Realloc(atexit_callbacks,
                          sizeof(atexit_callback*) * callback_len);

    }
    
    if (PyTuple_GET_SIZE(args) == 0) {
        PyErr_SetString(PyExc_TypeError,
                "register() takes at least 1 argument (0 given)");
        return NULL; 
    }
    
    func = PyTuple_GET_ITEM(args, 0);
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                "the first argument must be callable");
        return NULL;
    }
    
    new_callback = PyMem_Malloc(sizeof(atexit_callback));
    if (new_callback == NULL)
        return PyErr_NoMemory();   

    new_callback->args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (new_callback->args == NULL) {
        PyMem_Free(new_callback);
        return NULL;
    }
    new_callback->func = func;
    new_callback->kwargs = kwargs;
    Py_INCREF(func);
    Py_XINCREF(kwargs);
    
    atexit_callbacks[ncallbacks++] = new_callback;
    
    Py_INCREF(func);
    return func;
}

static PyObject *
atexit_run_exitfuncs(PyObject *self)
{
    atexit_callfuncs();
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
atexit_clear(PyObject *self)
{
    atexit_callback *cb;
    int i;
    
    for(i = 0; i < ncallbacks; i++)
    {
        cb = atexit_callbacks[i];
        if (cb == NULL)
            continue;
        
        atexit_delete_cb(i);
    }
    ncallbacks = 0;
    Py_RETURN_NONE;
}

static PyObject *
atexit_unregister(PyObject *self, PyObject *func)
{
    atexit_callback *cb;
    int i, eq;
    
    for(i = 0; i < ncallbacks; i++)
    {
        cb = atexit_callbacks[i];
        if (cb == NULL)
            continue;
        
        eq = PyObject_RichCompareBool(cb->func, func, Py_EQ);
        if (eq < 0)
            return NULL;
        if (eq)
            atexit_delete_cb(i);
    }
    Py_RETURN_NONE;
}

static PyMethodDef atexit_methods[] = {
    {"register", (PyCFunction) atexit_register, METH_VARARGS|METH_KEYWORDS,
        atexit_register__doc__},
    {"_clear", (PyCFunction) atexit_clear, METH_NOARGS,
        NULL},
    {"unregister", (PyCFunction) atexit_unregister, METH_O,
        NULL},
    {"_run_exitfuncs", (PyCFunction) atexit_run_exitfuncs, METH_NOARGS,
        NULL},
    {NULL, NULL}        /* sentinel */
};

/* ===================================================================== */
/* Initialization function. */

PyDoc_STRVAR(atexit__doc__,
"atexit.py - allow programmer to define multiple exit functions to be executed\
upon normal program termination.\n\
\n\
One public function, register, is defined.\n\
");

PyMODINIT_FUNC
initatexit(void)
{
    PyObject *m;
    
    atexit_callbacks = PyMem_New(atexit_callback*, callback_len);
    if (atexit_callbacks == NULL)
        return;

    m = Py_InitModule3("atexit", atexit_methods, atexit__doc__);
    if (m == NULL)
        return;
    
    _Py_PyAtExit(atexit_callfuncs);
}
