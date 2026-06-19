#include "Python.h"
#include "pycore_time.h"

static PyObject *
_timeout_enter(PyObject *self, PyObject *args)
{
    double seconds;
    if (!PyArg_ParseTuple(args, "d", &seconds))
        return NULL;

    PyThreadState *tstate = PyThreadState_Get();
    PyTime_t now;
    PyTime_Monotonic(&now);

    PyTime_t relative_deadline;
    if (_PyTime_FromSecondsDouble(seconds, _PyTime_ROUND_TIMEOUT, &relative_deadline) < 0) {
        PyErr_SetString(PyExc_OverflowError, "timeout value too large");
        return NULL;
    }
    const PyTime_t TIMEOUT_EPS = 1000000;
    PyTime_t deadline = now + relative_deadline + TIMEOUT_EPS;

    if (_PyTimeout_Push(tstate, deadline) < 0)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
_timeout_leave(PyObject *self, PyObject *noargs)
{
    PyThreadState *tstate = PyThreadState_Get();
    _PyTimeout_Pop(tstate);
    Py_RETURN_NONE;
}

static PyObject *
_test_timeout_init(PyObject *self, PyObject *args)
{
    double seconds;
    if (!PyArg_ParseTuple(args, "d", &seconds))
        return NULL;

    PyThreadState *tstate = PyThreadState_Get();
    PyTime_t now;
    PyTime_Monotonic(&now);
    PyTime_t deadline = now + (PyTime_t)(seconds * 1e9);


    (void)now;
    (void)deadline;
    PyTime_t fake_deadline = (PyTime_t)INT64_MAX;

    if (_PyTimeout_Push(tstate, fake_deadline) < 0)
        return NULL;

    Py_RETURN_NONE;
}

static int test_skip_interval = 0;
static PyObject *
_test_set_timeout_skip_interval(PyObject *self, PyObject *args)
{
    PyThreadState *tstate = PyThreadState_Get();
    _PyTimeoutBlock *block = tstate->timeout_block;
    if (block != NULL)
        block->skip_counter = 0;
    if (!PyArg_ParseTuple(args, "|i", &test_skip_interval))
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
_test_timeout_check(PyObject *self, PyObject *noargs)
{
    PyThreadState *tstate = PyThreadState_Get();
    int res = Py_CheckTimeOut(tstate, test_skip_interval);
    if (res != 0) {
        PyErr_Clear();
    }
    Py_RETURN_NONE;
}

static PyObject *
_test_benchmark_timeout_check_raw_loop(PyObject *self, PyObject *args)
{
    int interval, loops;
    if (!PyArg_ParseTuple(args, "ii", &interval, &loops))
        return NULL;

    PyThreadState *tstate = PyThreadState_Get();
    for (int i = 0; i < loops; ++i) {
        Py_CheckTimeOut(tstate, interval);
    }
    Py_RETURN_NONE;
}

static PyObject *
_test_timeout_cleanup(PyObject *self, PyObject *noargs)
{
    PyThreadState *tstate = PyThreadState_Get();

    _PyTimeout_Pop(tstate);
    Py_RETURN_NONE;
}

static PyMethodDef timeout_methods[] = {
    {"enter", _timeout_enter, METH_VARARGS, "Enter a timeout block."},
    {"leave", _timeout_leave, METH_NOARGS,  "Leave a timeout block."},
    {"_test_timeout_init", _test_timeout_init, METH_VARARGS,  "test _PyTimeout_Push C Api"},
    {"_test_timeout_check", _test_timeout_check, METH_NOARGS, "test Py_CheckTimeOut C Api"},
    {"_test_timeout_cleanup", _test_timeout_cleanup, METH_NOARGS,  "test _PyTimeout_Pop CApi"},
    {"_test_set_timeout_skip_interval", _test_set_timeout_skip_interval, METH_VARARGS,  "set test_skip_interval"},
    {"_test_benchmark_timeout_check_raw_loop", _test_benchmark_timeout_check_raw_loop, METH_VARARGS,  "test `Py_CheckTimeOut` CApi"},
    {NULL}
};

static struct PyModuleDef _timeoutmodule = {
    PyModuleDef_HEAD_INIT,
    "_timeout",
    NULL,
    -1,
    timeout_methods,
    NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC
PyInit__timeout(void)
{
    return PyModule_Create(&_timeoutmodule);
}