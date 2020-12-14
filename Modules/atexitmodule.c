/*
 *  atexit - allow programmer to define multiple exit functions to be executed
 *  upon normal program termination.
 *
 *   Translated from atexit.py by Collin Winter.
 +   Copyright 2007 Python Software Foundation.
 */

#include "Python.h"
#include "pycore_interp.h"        // PyInterpreterState.atexit_func
#include "pycore_pystate.h"       // _PyInterpreterState_GET

/* ===================================================================== */
/* Callback machinery. */

typedef struct {
    PyObject *func;
    PyObject *args;
    PyObject *kwargs;
} atexit_callback;

struct atexit_state {
    atexit_callback **atexit_callbacks;
    int ncallbacks;
    int callback_len;
};

static inline struct atexit_state*
get_atexit_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (struct atexit_state *)state;
}


static void
atexit_delete_cb(struct atexit_state *state, int i)
{
    atexit_callback *cb = state->atexit_callbacks[i];
    state->atexit_callbacks[i] = NULL;

    Py_DECREF(cb->func);
    Py_DECREF(cb->args);
    Py_XDECREF(cb->kwargs);
    PyMem_Free(cb);
}


/* Clear all callbacks without calling them */
static void
atexit_cleanup(struct atexit_state *state)
{
    atexit_callback *cb;
    for (int i = 0; i < state->ncallbacks; i++) {
        cb = state->atexit_callbacks[i];
        if (cb == NULL)
            continue;

        atexit_delete_cb(state, i);
    }
    state->ncallbacks = 0;
}

/* Installed into pylifecycle.c's atexit mechanism */

static void
atexit_callfuncs(PyObject *module, int ignore_exc)
{
    assert(!PyErr_Occurred());

    if (module == NULL) {
        return;
    }

    struct atexit_state *state = get_atexit_state(module);
    if (state->ncallbacks == 0) {
        return;
    }

    PyObject *exc_type = NULL, *exc_value, *exc_tb;
    for (int i = state->ncallbacks - 1; i >= 0; i--) {
        atexit_callback *cb = state->atexit_callbacks[i];
        if (cb == NULL) {
            continue;
        }

        PyObject *res = PyObject_Call(cb->func, cb->args, cb->kwargs);
        if (res == NULL) {
            if (ignore_exc) {
                _PyErr_WriteUnraisableMsg("in atexit callback", cb->func);
            }
            else {
                /* Maintain the last exception, but don't leak if there are
                   multiple exceptions. */
                if (exc_type) {
                    Py_DECREF(exc_type);
                    Py_XDECREF(exc_value);
                    Py_XDECREF(exc_tb);
                }
                PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
                if (!PyErr_GivenExceptionMatches(exc_type, PyExc_SystemExit)) {
                    PySys_WriteStderr("Error in atexit._run_exitfuncs:\n");
                    PyErr_NormalizeException(&exc_type, &exc_value, &exc_tb);
                    PyErr_Display(exc_type, exc_value, exc_tb);
                }
            }
        }
        else {
            Py_DECREF(res);
        }
    }

    atexit_cleanup(state);

    if (ignore_exc) {
        assert(!PyErr_Occurred());
    }
    else {
        if (exc_type) {
            PyErr_Restore(exc_type, exc_value, exc_tb);
        }
    }
}


void
_PyAtExit_Call(PyObject *module)
{
    atexit_callfuncs(module, 1);
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

    struct atexit_state *state = get_atexit_state(module);
    if (state->ncallbacks >= state->callback_len) {
        atexit_callback **r;
        state->callback_len += 16;
        r = (atexit_callback**)PyMem_Realloc(state->atexit_callbacks,
                                      sizeof(atexit_callback*) * state->callback_len);
        if (r == NULL)
            return PyErr_NoMemory();
        state->atexit_callbacks = r;
    }

    atexit_callback *callback = PyMem_Malloc(sizeof(atexit_callback));
    if (callback == NULL) {
        return PyErr_NoMemory();
    }

    callback->args = PyTuple_GetSlice(args, 1, PyTuple_GET_SIZE(args));
    if (callback->args == NULL) {
        PyMem_Free(callback);
        return NULL;
    }
    callback->func = Py_NewRef(func);
    callback->kwargs = Py_XNewRef(kwargs);

    state->atexit_callbacks[state->ncallbacks++] = callback;

    return Py_NewRef(func);
}

PyDoc_STRVAR(atexit_run_exitfuncs__doc__,
"_run_exitfuncs() -> None\n\
\n\
Run all registered exit functions.");

static PyObject *
atexit_run_exitfuncs(PyObject *module, PyObject *unused)
{
    atexit_callfuncs(module, 0);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_clear__doc__,
"_clear() -> None\n\
\n\
Clear the list of previously registered exit functions.");

static PyObject *
atexit_clear(PyObject *module, PyObject *unused)
{
    atexit_cleanup(get_atexit_state(module));
    Py_RETURN_NONE;
}

PyDoc_STRVAR(atexit_ncallbacks__doc__,
"_ncallbacks() -> int\n\
\n\
Return the number of registered exit functions.");

static PyObject *
atexit_ncallbacks(PyObject *module, PyObject *unused)
{
    struct atexit_state *state = get_atexit_state(module);
    return PyLong_FromSsize_t(state->ncallbacks);
}

static int
atexit_m_traverse(PyObject *module, visitproc visit, void *arg)
{
    struct atexit_state *state = (struct atexit_state *)PyModule_GetState(module);
    for (int i = 0; i < state->ncallbacks; i++) {
        atexit_callback *cb = state->atexit_callbacks[i];
        if (cb == NULL)
            continue;
        Py_VISIT(cb->func);
        Py_VISIT(cb->args);
        Py_VISIT(cb->kwargs);
    }
    return 0;
}

static int
atexit_m_clear(PyObject *module)
{
    struct atexit_state *state = (struct atexit_state *)PyModule_GetState(module);
    atexit_cleanup(state);
    return 0;
}

static void
atexit_free(PyObject *module)
{
    struct atexit_state *state = (struct atexit_state *)PyModule_GetState(module);
    atexit_cleanup(state);
    PyMem_Free(state->atexit_callbacks);
}

PyDoc_STRVAR(atexit_unregister__doc__,
"unregister(func) -> None\n\
\n\
Unregister an exit function which was previously registered using\n\
atexit.register\n\
\n\
    func - function to be unregistered");

static PyObject *
atexit_unregister(PyObject *module, PyObject *func)
{
    struct atexit_state *state = get_atexit_state(module);
    for (int i = 0; i < state->ncallbacks; i++)
    {
        atexit_callback *cb = state->atexit_callbacks[i];
        if (cb == NULL) {
            continue;
        }

        int eq = PyObject_RichCompareBool(cb->func, func, Py_EQ);
        if (eq < 0) {
            return NULL;
        }
        if (eq) {
            atexit_delete_cb(state, i);
        }
    }
    Py_RETURN_NONE;
}


static PyMethodDef atexit_methods[] = {
    {"register", (PyCFunction)(void(*)(void)) atexit_register, METH_VARARGS|METH_KEYWORDS,
        atexit_register__doc__},
    {"_clear", (PyCFunction) atexit_clear, METH_NOARGS,
        atexit_clear__doc__},
    {"unregister", (PyCFunction) atexit_unregister, METH_O,
        atexit_unregister__doc__},
    {"_run_exitfuncs", (PyCFunction) atexit_run_exitfuncs, METH_NOARGS,
        atexit_run_exitfuncs__doc__},
    {"_ncallbacks", (PyCFunction) atexit_ncallbacks, METH_NOARGS,
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

static int
atexit_exec(PyObject *module)
{
    struct atexit_state *state = get_atexit_state(module);
    state->callback_len = 32;
    state->ncallbacks = 0;
    state->atexit_callbacks = PyMem_New(atexit_callback*, state->callback_len);
    if (state->atexit_callbacks == NULL) {
        return -1;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    interp->atexit_module = module;
    return 0;
}

static PyModuleDef_Slot atexit_slots[] = {
    {Py_mod_exec, atexit_exec},
    {0, NULL}
};

static struct PyModuleDef atexitmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "atexit",
    .m_doc = atexit__doc__,
    .m_size = sizeof(struct atexit_state),
    .m_methods = atexit_methods,
    .m_slots = atexit_slots,
    .m_traverse = atexit_m_traverse,
    .m_clear = atexit_m_clear,
    .m_free = (freefunc)atexit_free
};

PyMODINIT_FUNC
PyInit_atexit(void)
{
    return PyModuleDef_Init(&atexitmodule);
}
