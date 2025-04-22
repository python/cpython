/*
 *  atexit - allow programmer to define multiple exit functions to be executed
 *  upon normal program termination.
 *
 *   Translated from atexit.py by Collin Winter.
 +   Copyright 2007 Python Software Foundation.
 */

#include "Python.h"
#include "pycore_atexit.h"        // export _Py_AtExit()
#include "pycore_initconfig.h"    // _PyStatus_NO_MEMORY
#include "pycore_interp.h"        // PyInterpreterState.atexit
#include "pycore_pystate.h"       // _PyInterpreterState_GET

/* ===================================================================== */
/* Callback machinery. */

static inline struct atexit_state*
get_atexit_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->atexit;
}


int
PyUnstable_AtExit(PyInterpreterState *interp,
                  atexit_datacallbackfunc func, void *data)
{
    PyThreadState *tstate = _PyThreadState_GET();
    _Py_EnsureTstateNotNULL(tstate);
    assert(tstate->interp == interp);

    atexit_callback *callback = PyMem_Malloc(sizeof(atexit_callback));
    if (callback == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    callback->func = func;
    callback->data = data;
    callback->next = NULL;

    struct atexit_state *state = &interp->atexit;
    _PyAtExit_LockCallbacks(state);
    atexit_callback *top = state->ll_callbacks;
    if (top == NULL) {
        state->ll_callbacks = callback;
    }
    else {
        callback->next = top;
        state->ll_callbacks = callback;
    }
    _PyAtExit_UnlockCallbacks(state);
    return 0;
}


/* Clear all callbacks without calling them */
static void
atexit_cleanup(struct atexit_state *state)
{
    PyList_Clear(state->callbacks);
}


PyStatus
_PyAtExit_Init(PyInterpreterState *interp)
{
    struct atexit_state *state = &interp->atexit;
    // _PyAtExit_Init() must only be called once
    assert(state->callbacks == NULL);

    state->callbacks = PyList_New(0);
    if (state->callbacks == NULL) {
        return _PyStatus_NO_MEMORY();
    }
    return _PyStatus_OK();
}

void
_PyAtExit_Fini(PyInterpreterState *interp)
{
    // In theory, there shouldn't be any threads left by now, so we
    // won't lock this.
    struct atexit_state *state = &interp->atexit;
    atexit_cleanup(state);
    Py_CLEAR(state->callbacks);

    atexit_callback *next = state->ll_callbacks;
    state->ll_callbacks = NULL;
    while (next != NULL) {
        atexit_callback *callback = next;
        next = callback->next;
        atexit_datacallbackfunc exitfunc = callback->func;
        void *data = callback->data;
        // It was allocated in _PyAtExit_AddCallback().
        PyMem_Free(callback);
        exitfunc(data);
    }
}

static void
atexit_callfuncs(struct atexit_state *state)
{
    assert(!PyErr_Occurred());
    assert(state->callbacks != NULL);
    assert(PyList_CheckExact(state->callbacks));

    // Create a copy of the list for thread safety
    PyObject *copy = PyList_GetSlice(state->callbacks, 0, PyList_GET_SIZE(state->callbacks));
    if (copy == NULL)
    {
        PyErr_FormatUnraisable("Exception ignored while "
                               "copying atexit callbacks");
        return;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(copy); ++i) {
        // We don't have to worry about evil borrowed references, because
        // no other threads can access this list.
        PyObject *tuple = PyList_GET_ITEM(copy, i);
        assert(PyTuple_CheckExact(tuple));

        PyObject *func = PyTuple_GET_ITEM(tuple, 0);
        PyObject *args = PyTuple_GET_ITEM(tuple, 1);
        PyObject *kwargs = PyTuple_GET_ITEM(tuple, 2);

        PyObject *res = PyObject_Call(func,
                                      args,
                                      kwargs == Py_None ? NULL : kwargs);
        if (res == NULL) {
            PyErr_FormatUnraisable(
                "Exception ignored in atexit callback %R", func);
        }
        else {
            Py_DECREF(res);
        }
    }

    Py_DECREF(copy);
    atexit_cleanup(state);

    assert(!PyErr_Occurred());
}


void
_PyAtExit_Call(PyInterpreterState *interp)
{
    struct atexit_state *state = &interp->atexit;
    atexit_callfuncs(state);
}


/* ===================================================================== */
/* Module methods. */


PyDoc_STRVAR(atexit_register__doc__,
"register($module, func, /, *args, **kwargs)\n\
--\n\
\n\
Register a function to be executed upon normal program termination\n\
\n\
    func - function to be called at exit\n\
    args - optional arguments to pass to func\n\
    kwargs - optional keyword arguments to pass to func\n\
\n\
    func is returned to facilitate usage as a decorator.");

static PyObject *
atexit_register(PyObject *module, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) == 0) {
        PyErr_SetString(PyExc_TypeError,
                "register() takes at least 1 argument (0 given)");
        return NULL;
    }

    PyObject *func = PyTuple_GET_ITEM(args, 0);
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                "the first argument must be callable");
        return NULL;
    }
    PyObject *func_args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    PyObject *func_kwargs = kwargs;

    if (func_kwargs == NULL)
    {
        func_kwargs = Py_None;
    }
    PyObject *callback = PyTuple_Pack(3, func, func_args, func_kwargs);
    if (callback == NULL)
    {
        return NULL;
    }

    struct atexit_state *state = get_atexit_state();
    // atexit callbacks go in a LIFO order
    if (PyList_Insert(state->callbacks, 0, callback) < 0)
    {
        Py_DECREF(callback);
        return NULL;
    }
    Py_DECREF(callback);

    return Py_NewRef(func);
}

PyDoc_STRVAR(atexit_run_exitfuncs__doc__,
"_run_exitfuncs($module, /)\n\
--\n\
\n\
Run all registered exit functions.\n\
\n\
If a callback raises an exception, it is logged with sys.unraisablehook.");

static PyObject *
atexit_run_exitfuncs(PyObject *module, PyObject *Py_UNUSED(dummy))
{
    struct atexit_state *state = get_atexit_state();
    atexit_callfuncs(state);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_clear__doc__,
"_clear($module, /)\n\
--\n\
\n\
Clear the list of previously registered exit functions.");

static PyObject *
atexit_clear(PyObject *module, PyObject *Py_UNUSED(dummy))
{
    atexit_cleanup(get_atexit_state());
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_ncallbacks__doc__,
"_ncallbacks($module, /)\n\
--\n\
\n\
Return the number of registered exit functions.");

static PyObject *
atexit_ncallbacks(PyObject *module, PyObject *Py_UNUSED(dummy))
{
    struct atexit_state *state = get_atexit_state();
    assert(state->callbacks != NULL);
    assert(PyList_CheckExact(state->callbacks));
    return PyLong_FromSsize_t(PyList_GET_SIZE(state->callbacks));
}

static int
atexit_unregister_locked(PyObject *callbacks, PyObject *func)
{
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(callbacks); ++i) {
        PyObject *tuple = PyList_GET_ITEM(callbacks, i);
        assert(PyTuple_CheckExact(tuple));
        PyObject *to_compare = PyTuple_GET_ITEM(tuple, 0);
        int cmp = PyObject_RichCompareBool(func, to_compare, Py_EQ);
        if (cmp < 0)
        {
            return -1;
        }
        if (cmp == 1) {
            // We found a callback!
            if (PyList_SetSlice(callbacks, i, i + 1, NULL) < 0) {
                return -1;
            }
            --i;
        }
    }

    return 0;
}

PyDoc_STRVAR(atexit_unregister__doc__,
"unregister($module, func, /)\n\
--\n\
\n\
Unregister an exit function which was previously registered using\n\
atexit.register\n\
\n\
    func - function to be unregistered");

static PyObject *
atexit_unregister(PyObject *module, PyObject *func)
{
    struct atexit_state *state = get_atexit_state();
    int result;
    Py_BEGIN_CRITICAL_SECTION(state->callbacks);
    result = atexit_unregister_locked(state->callbacks, func);
    Py_END_CRITICAL_SECTION();
    return result < 0 ? NULL : Py_None;
}


static PyMethodDef atexit_methods[] = {
    {"register", _PyCFunction_CAST(atexit_register), METH_VARARGS|METH_KEYWORDS,
        atexit_register__doc__},
    {"_clear", atexit_clear, METH_NOARGS, atexit_clear__doc__},
    {"unregister", atexit_unregister, METH_O, atexit_unregister__doc__},
    {"_run_exitfuncs", atexit_run_exitfuncs, METH_NOARGS,
        atexit_run_exitfuncs__doc__},
    {"_ncallbacks", atexit_ncallbacks, METH_NOARGS,
        atexit_ncallbacks__doc__},
    {NULL, NULL}        /* sentinel */
};


/* ===================================================================== */
/* Initialization function. */

PyDoc_STRVAR(atexit__doc__,
"allow programmer to define multiple exit functions to be executed\n\
upon normal program termination.\n\
\n\
Two public functions, register and unregister, are defined.\n\
");

static PyModuleDef_Slot atexitmodule_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef atexitmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "atexit",
    .m_doc = atexit__doc__,
    .m_size = 0,
    .m_methods = atexit_methods,
    .m_slots = atexitmodule_slots,
};

PyMODINIT_FUNC
PyInit_atexit(void)
{
    return PyModuleDef_Init(&atexitmodule);
}
