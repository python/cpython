
static void
_ensure_current_cause(PyThreadState *tstate, PyObject *cause)
{
    if (cause == NULL) {
        return;
    }
    PyObject *exc = _PyErr_GetRaisedException(tstate);
    assert(exc != NULL);
    PyObject *ctx = PyException_GetContext(exc);
    if (ctx == NULL) {
        PyException_SetContext(exc, Py_NewRef(cause));
    }
    else {
        Py_DECREF(ctx);
    }
    assert(PyException_GetCause(exc) == NULL);
    PyException_SetCause(exc, Py_NewRef(cause));
    _PyErr_SetRaisedException(tstate, exc);
}


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

/* NotShareableError extends TypeError */

static int
_init_notshareableerror(exceptions_t *state)
{
    const char *name = "interpreters.NotShareableError";
    PyObject *base = PyExc_TypeError;
    PyObject *ns = NULL;
    PyObject *exctype = PyErr_NewException(name, base, ns);
    if (exctype == NULL) {
        return -1;
    }
    state->PyExc_NotShareableError = exctype;
    return 0;
}

static void
_fini_notshareableerror(exceptions_t *state)
{
    Py_CLEAR(state->PyExc_NotShareableError);
}

static PyObject *
get_notshareableerror_type(PyThreadState *tstate)
{
    _PyXI_state_t *local = _PyXI_GET_STATE(tstate->interp);
    if (local == NULL) {
        PyErr_Clear();
        return NULL;
    }
    return local->exceptions.PyExc_NotShareableError;
}

static void
_ensure_notshareableerror(PyThreadState *tstate,
                          PyObject *cause, int force, PyObject *msgobj)
{
    PyObject *ctx = _PyErr_GetRaisedException(tstate);
    PyObject *exctype = get_notshareableerror_type(tstate);
    if (exctype != NULL) {
        if (!force && ctx != NULL && Py_TYPE(ctx) == (PyTypeObject *)exctype) {
            // A NotShareableError instance is already set.
            assert(cause == NULL);
            _PyErr_SetRaisedException(tstate, ctx);
        }
    }
    else {
        exctype = PyExc_TypeError;
    }
    _PyErr_SetObject(tstate, exctype, msgobj);
    // We have to set the context manually since _PyErr_SetObject() doesn't.
    _PyErr_ChainExceptions1Tstate(tstate, ctx);
    _ensure_current_cause(tstate, cause);
}

static void
set_notshareableerror(PyThreadState *tstate, PyObject *cause, int force, const char *msg)
{
    PyObject *msgobj = PyUnicode_FromString(msg);
    if (msgobj == NULL) {
        assert(_PyErr_Occurred(tstate));
    }
    else {
        _ensure_notshareableerror(tstate, cause, force, msgobj);
        Py_DECREF(msgobj);
    }
}

static void
format_notshareableerror_v(PyThreadState *tstate, PyObject *cause, int force,
                           const char *format, va_list vargs)
{
    PyObject *msgobj = PyUnicode_FromFormatV(format, vargs);
    if (msgobj == NULL) {
        assert(_PyErr_Occurred(tstate));
    }
    else {
        _ensure_notshareableerror(tstate, cause, force, msgobj);
        Py_DECREF(msgobj);
    }
}

static void
format_notshareableerror(PyThreadState *tstate, PyObject *cause, int force,
                         const char *format, ...)
{
    va_list vargs;
    va_start(vargs, format);
    format_notshareableerror_v(tstate, cause, force, format, vargs);
    va_end(vargs);
}


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
    if (_init_notshareableerror(state) < 0) {
        goto error;
    }
    return 0;

error:
    fini_heap_exctypes(state);
    return -1;
}

static void
fini_heap_exctypes(exceptions_t *state)
{
    _fini_notshareableerror(state);
}
