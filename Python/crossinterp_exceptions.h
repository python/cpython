
/* InterpreterError extends Exception */

static PyTypeObject _PyExc_InterpreterError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "InterpreterError",
    .tp_doc = PyDoc_STR("An interpreter was not found."),
    //.tp_base = (PyTypeObject *)PyExc_BaseException,
};
PyObject *PyExc_InterpreterError = (PyObject *)&_PyExc_InterpreterError;

/* InterpreterNotFoundError extends InterpreterError */

static PyTypeObject _PyExc_InterpreterNotFoundError = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "InterpreterNotFoundError",
    .tp_doc = PyDoc_STR("An interpreter was not found."),
    .tp_base = &_PyExc_InterpreterError,
};
PyObject *PyExc_InterpreterNotFoundError = (PyObject *)&_PyExc_InterpreterNotFoundError;


/* lifecycle */

static int
init_exceptions(PyInterpreterState *interp)
{
    _PyExc_InterpreterError.tp_base = (PyTypeObject *)PyExc_BaseException;
    if (_PyStaticType_InitBuiltin(interp, &_PyExc_InterpreterError) < 0) {
        return -1;
    }
    if (_PyStaticType_InitBuiltin(interp, &_PyExc_InterpreterNotFoundError) < 0) {
        return -1;
    }
    return 0;
}

static void
fini_exceptions(PyInterpreterState *interp)
{
    _PyStaticType_Dealloc(interp, &_PyExc_InterpreterNotFoundError);
    _PyStaticType_Dealloc(interp, &_PyExc_InterpreterError);
}
