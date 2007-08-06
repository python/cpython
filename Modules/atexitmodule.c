/*
 *  atexit - allow programmer to define multiple exit functions to be executed
 *  upon normal program termination.
 *
 *   Translated from atexit.py by Collin Winter.
 +   Copyright 2007 Python Software Foundation.
 */

#include "Python.h"

/* Forward declaration (for atexit_cleanup) */
static PyObject *atexit_clear(PyObject*);

/* ===================================================================== */
/* Callback machinery. */

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} atexit_callback;

static atexit_callback **atexit_callbacks;
static int ncallbacks = 0;
static int callback_len = 32;

/* Installed into pythonrun.c's atexit mechanism */

void
atexit_callfuncs(void)
{
    PyObject *exc_type = NULL, *exc_value, *exc_tb, *r;
    atexit_callback *cb;
    int i;
    
    if (ncallbacks == 0)
        return;
        
    for (i = ncallbacks - 1; i >= 0; i--)
    {
        cb = atexit_callbacks[i];
        if (cb == NULL)
            continue;

        r = PyObject_Call(cb->func, cb->args, cb->kwargs);
        Py_XDECREF(r);
        if (r == NULL) {
            /* Maintain the last exception, but don't leak if there are
               multiple exceptions. */
            if (exc_type) {
                Py_DECREF(exc_type);
                Py_XDECREF(exc_value);
                Py_XDECREF(exc_tb);    
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

void
atexit_cleanup(void)
{
    PyObject *r = atexit_clear(NULL);
    Py_DECREF(r);
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
        atexit_callback **r;
        callback_len += 16;
        r = (atexit_callback**)PyMem_Realloc(atexit_callbacks,
                                      sizeof(atexit_callback*) * callback_len);
        if (r == NULL)
            return PyErr_NoMemory();
        atexit_callbacks = r;
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

PyDoc_STRVAR(atexit_run_exitfuncs__doc__,
"_run_exitfuncs() -> None\n\
\n\
Run all registered exit functions.");

static PyObject *
atexit_run_exitfuncs(PyObject *self)
{
    atexit_callfuncs();
    if (PyErr_Occurred())
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_clear__doc__,
"_clear() -> None\n\
\n\
Clear the list of previously registered exit functions.");

static PyObject *
atexit_clear(PyObject *self)
{
    atexit_callback *cb;
    int i;
    
    for (i = 0; i < ncallbacks; i++)
    {
        cb = atexit_callbacks[i];
        if (cb == NULL)
            continue;
        
        atexit_delete_cb(i);
    }
    ncallbacks = 0;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_unregister__doc__,
"unregister(func) -> None\n\
\n\
Unregister a exit function which was previously registered using\n\
atexit.register\n\
\n\
    func - function to be unregistered");

static PyObject *
atexit_unregister(PyObject *self, PyObject *func)
{
    atexit_callback *cb;
    int i, eq;
    
    for (i = 0; i < ncallbacks; i++)
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
        atexit_clear__doc__},
    {"unregister", (PyCFunction) atexit_unregister, METH_O,
        atexit_unregister__doc__},
    {"_run_exitfuncs", (PyCFunction) atexit_run_exitfuncs, METH_NOARGS,
        atexit_run_exitfuncs__doc__},
    {NULL, NULL}        /* sentinel */
};

/* ===================================================================== */
/* Initialization function. */

PyDoc_STRVAR(atexit__doc__,
"allow programmer to define multiple exit functions to be executed\
upon normal program termination.\n\
\n\
Two public functions, register and unregister, are defined.\n\
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
    /* Register a callback that will free
       atexit_callbacks, otherwise valgrind will report memory leaks. */
    Py_AtExit(atexit_cleanup);
}
