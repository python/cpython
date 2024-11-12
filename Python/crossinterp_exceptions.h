
/* InterpreterError extends Exception */

static PyTypeObject _PyExc_InterpreterError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "interpreters.InterpreterError",
    .tp_doc = PyDoc_STR("A cross-interpreter operation failed"),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    //.tp_traverse = ((PyTypeObject *)PyExc_Exception)->tp_traverse,
    //.tp_clear = ((PyTypeObject *)PyExc_Exception)->tp_clear,
    //.tp_base = (PyTypeObject *)PyExc_Exception,
};
PyObject *PyExc_InterpreterError = (PyObject *)&_PyExc_InterpreterError;

/* InterpreterNotFoundError extends InterpreterError */

static PyTypeObject _PyExc_InterpreterNotFoundError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "interpreters.InterpreterNotFoundError",
    .tp_doc = PyDoc_STR("An interpreter was not found"),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    //.tp_traverse = ((PyTypeObject *)PyExc_Exception)->tp_traverse,
    //.tp_clear = ((PyTypeObject *)PyExc_Exception)->tp_clear,
    .tp_base = &_PyExc_InterpreterError,
};
PyObject *PyExc_InterpreterNotFoundError = (PyObject *)&_PyExc_InterpreterNotFoundError;


/* lifecycle */

static int
init_static_exctypes(exceptions_t *state, PyInterpreterState *interp)
{
    assert(state == &_PyXI_GET_STATE(interp)->exceptions);
    PyTypeObject *base = (PyTypeObject *)PyExc_Exception;

    // PyExc_InterpreterError
    _PyExc_InterpreterError.tp_base = base;
    _PyExc_InterpreterError.tp_traverse = base->tp_traverse;
    _PyExc_InterpreterError.tp_clear = base->tp_clear;
    if (_PyStaticType_InitBuiltin(interp, &_PyExc_InterpreterError) < 0) {
        goto error;
    }
    state->PyExc_InterpreterError = (PyObject *)&_PyExc_InterpreterError;

    // PyExc_InterpreterNotFoundError
    _PyExc_InterpreterNotFoundError.tp_traverse = base->tp_traverse;
    _PyExc_InterpreterNotFoundError.tp_clear = base->tp_clear;
    if (_PyStaticType_InitBuiltin(interp, &_PyExc_InterpreterNotFoundError) < 0) {
        goto error;
    }
    state->PyExc_InterpreterNotFoundError =
            (PyObject *)&_PyExc_InterpreterNotFoundError;

    return 0;

error:
    fini_static_exctypes(state, interp);
    return -1;
}

static void
fini_static_exctypes(exceptions_t *state, PyInterpreterState *interp)
{
    assert(state == &_PyXI_GET_STATE(interp)->exceptions);
    if (state->PyExc_InterpreterNotFoundError != NULL) {
        state->PyExc_InterpreterNotFoundError = NULL;
        _PyStaticType_FiniBuiltin(interp, &_PyExc_InterpreterNotFoundError);
    }
    if (state->PyExc_InterpreterError != NULL) {
        state->PyExc_InterpreterError = NULL;
        _PyStaticType_FiniBuiltin(interp, &_PyExc_InterpreterError);
    }
}

static int
init_heap_exctypes(exceptions_t *state)
{
    PyObject *exctype;

    /* NotShareableError extends ValueError */
    const char *name = "interpreters.NotShareableError";
    PyObject *base = PyExc_ValueError;
    PyObject *ns = NULL;
    exctype = PyErr_NewException(name, base, ns);
    if (exctype == NULL) {
        goto error;
    }
    state->PyExc_NotShareableError = exctype;

    return 0;

error:
    fini_heap_exctypes(state);
    return -1;
}

static void
fini_heap_exctypes(exceptions_t *state)
{
    Py_CLEAR(state->PyExc_NotShareableError);
}
