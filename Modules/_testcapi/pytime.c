#include "parts.h"

#ifdef MS_WINDOWS
#  include <winsock2.h>           // struct timeval
#endif

static PyObject *
test_pytime_fromseconds(PyObject *self, PyObject *args)
{
    int seconds;
    if (!PyArg_ParseTuple(args, "i", &seconds)) {
        return NULL;
    }
    _PyTime_t ts = _PyTime_FromSeconds(seconds);
    return _PyTime_AsNanosecondsObject(ts);
}

static int
check_time_rounding(int round)
{
    if (round != _PyTime_ROUND_FLOOR
        && round != _PyTime_ROUND_CEILING
        && round != _PyTime_ROUND_HALF_EVEN
        && round != _PyTime_ROUND_UP)
    {
        PyErr_SetString(PyExc_ValueError, "invalid rounding");
        return -1;
    }
    return 0;
}

static PyObject *
test_pytime_fromsecondsobject(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    _PyTime_t ts;
    if (_PyTime_FromSecondsObject(&ts, obj, round) == -1) {
        return NULL;
    }
    return _PyTime_AsNanosecondsObject(ts);
}

static PyObject *
test_pytime_assecondsdouble(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    _PyTime_t ts;
    if (_PyTime_FromNanosecondsObject(&ts, obj) < 0) {
        return NULL;
    }
    double d = _PyTime_AsSecondsDouble(ts);
    return PyFloat_FromDouble(d);
}

static PyObject *
test_PyTime_AsTimeval(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    struct timeval tv;
    if (_PyTime_AsTimeval(t, &tv, round) < 0) {
        return NULL;
    }

    PyObject *seconds = PyLong_FromLongLong(tv.tv_sec);
    if (seconds == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", seconds, (long)tv.tv_usec);
}

static PyObject *
test_PyTime_AsTimeval_clamp(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    struct timeval tv;
    _PyTime_AsTimeval_clamp(t, &tv, round);

    PyObject *seconds = PyLong_FromLongLong(tv.tv_sec);
    if (seconds == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", seconds, (long)tv.tv_usec);
}

#ifdef HAVE_CLOCK_GETTIME
static PyObject *
test_PyTime_AsTimespec(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    struct timespec ts;
    if (_PyTime_AsTimespec(t, &ts) == -1) {
        return NULL;
    }
    return Py_BuildValue("Nl", _PyLong_FromTime_t(ts.tv_sec), ts.tv_nsec);
}

static PyObject *
test_PyTime_AsTimespec_clamp(PyObject *self, PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj)) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    struct timespec ts;
    _PyTime_AsTimespec_clamp(t, &ts);
    return Py_BuildValue("Nl", _PyLong_FromTime_t(ts.tv_sec), ts.tv_nsec);
}
#endif

static PyObject *
test_PyTime_AsMilliseconds(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    _PyTime_t ms = _PyTime_AsMilliseconds(t, round);
    _PyTime_t ns = _PyTime_FromNanoseconds(ms);
    return _PyTime_AsNanosecondsObject(ns);
}

static PyObject *
test_PyTime_AsMicroseconds(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    _PyTime_t t;
    if (_PyTime_FromNanosecondsObject(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    _PyTime_t us = _PyTime_AsMicroseconds(t, round);
    _PyTime_t ns = _PyTime_FromNanoseconds(us);
    return _PyTime_AsNanosecondsObject(ns);
}

static PyObject *
test_pytime_object_to_time_t(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_time_t", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    if (_PyTime_ObjectToTime_t(obj, &sec, round) == -1) {
        return NULL;
    }
    return _PyLong_FromTime_t(sec);
}

static PyObject *
test_pytime_object_to_timeval(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    long usec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_timeval", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    if (_PyTime_ObjectToTimeval(obj, &sec, &usec, round) == -1) {
        return NULL;
    }
    return Py_BuildValue("Nl", _PyLong_FromTime_t(sec), usec);
}

static PyObject *
test_pytime_object_to_timespec(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    long nsec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi:pytime_object_to_timespec", &obj, &round)) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    if (_PyTime_ObjectToTimespec(obj, &sec, &nsec, round) == -1) {
        return NULL;
    }
    return Py_BuildValue("Nl", _PyLong_FromTime_t(sec), nsec);
}

static PyMethodDef test_methods[] = {
    {"PyTime_AsMicroseconds",       test_PyTime_AsMicroseconds,     METH_VARARGS},
    {"PyTime_AsMilliseconds",       test_PyTime_AsMilliseconds,     METH_VARARGS},
    {"PyTime_AsSecondsDouble",      test_pytime_assecondsdouble,    METH_VARARGS},
#ifdef HAVE_CLOCK_GETTIME
    {"PyTime_AsTimespec",           test_PyTime_AsTimespec,         METH_VARARGS},
    {"PyTime_AsTimespec_clamp",     test_PyTime_AsTimespec_clamp,   METH_VARARGS},
#endif
    {"PyTime_AsTimeval",            test_PyTime_AsTimeval,          METH_VARARGS},
    {"PyTime_AsTimeval_clamp",      test_PyTime_AsTimeval_clamp,    METH_VARARGS},
    {"PyTime_FromSeconds",          test_pytime_fromseconds,        METH_VARARGS},
    {"PyTime_FromSecondsObject",    test_pytime_fromsecondsobject,  METH_VARARGS},
    {"pytime_object_to_time_t",     test_pytime_object_to_time_t,   METH_VARARGS},
    {"pytime_object_to_timespec",   test_pytime_object_to_timespec, METH_VARARGS},
    {"pytime_object_to_timeval",    test_pytime_object_to_timeval,  METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_PyTime(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
