#include "parts.h"


static int
pytime_from_nanoseconds(PyTime_t *tp, PyObject *obj)
{
    if (!PyLong_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "expect int, got %T", obj);
        return -1;
    }

    long long nsec = PyLong_AsLongLong(obj);
    if (nsec == -1 && PyErr_Occurred()) {
        return -1;
    }

    Py_BUILD_ASSERT(sizeof(long long) == sizeof(PyTime_t));
    *tp = (PyTime_t)nsec;
    return 0;
}


static PyObject *
test_pytime_assecondsdouble(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    PyTime_t ts;
    if (pytime_from_nanoseconds(&ts, obj) < 0) {
        return NULL;
    }
    double d = PyTime_AsSecondsDouble(ts);
    return PyFloat_FromDouble(d);
}


static PyObject*
pytime_as_float(PyTime_t t)
{
    return PyFloat_FromDouble(PyTime_AsSecondsDouble(t));
}



static PyObject*
test_pytime_monotonic(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res = PyTime_Monotonic(&t);
    if (res < 0) {
        assert(t == 0);
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyObject*
test_pytime_monotonic_raw(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = PyTime_MonotonicRaw(&t);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        assert(t == 0);
        PyErr_SetString(PyExc_RuntimeError, "PyTime_MonotonicRaw() failed");
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyObject*
test_pytime_perf_counter(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res = PyTime_PerfCounter(&t);
    if (res < 0) {
        assert(t == 0);
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyObject*
test_pytime_perf_counter_raw(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = PyTime_PerfCounterRaw(&t);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        assert(t == 0);
        PyErr_SetString(PyExc_RuntimeError, "PyTime_PerfCounterRaw() failed");
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyObject*
test_pytime_time(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res = PyTime_Time(&t);
    if (res < 0) {
        assert(t == 0);
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyObject*
test_pytime_time_raw(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(args))
{
    PyTime_t t;
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = PyTime_TimeRaw(&t);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        assert(t == 0);
        PyErr_SetString(PyExc_RuntimeError, "PyTime_TimeRaw() failed");
        return NULL;
    }
    assert(res == 0);
    return pytime_as_float(t);
}


static PyMethodDef test_methods[] = {
    {"PyTime_AsSecondsDouble", test_pytime_assecondsdouble, METH_VARARGS},
    {"PyTime_Monotonic", test_pytime_monotonic, METH_NOARGS},
    {"PyTime_MonotonicRaw", test_pytime_monotonic_raw, METH_NOARGS},
    {"PyTime_PerfCounter", test_pytime_perf_counter, METH_NOARGS},
    {"PyTime_PerfCounterRaw", test_pytime_perf_counter_raw, METH_NOARGS},
    {"PyTime_Time", test_pytime_time, METH_NOARGS},
    {"PyTime_TimeRaw", test_pytime_time_raw, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Time(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    Py_BUILD_ASSERT(sizeof(long long) == sizeof(PyTime_t));
    if (PyModule_AddObject(m, "PyTime_MIN", PyLong_FromLongLong(PyTime_MIN)) < 0) {
        return 1;
    }
    if (PyModule_AddObject(m, "PyTime_MAX", PyLong_FromLongLong(PyTime_MAX)) < 0) {
        return 1;
    }
    return 0;
}
