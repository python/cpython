/* Test pycore_time.h */

#include "parts.h"

#include "pycore_time.h"          // _PyTime_FromSeconds()

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
    PyTime_t ts = _PyTime_FromSeconds(seconds);
    return _PyTime_AsLong(ts);
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
    PyTime_t ts;
    if (_PyTime_FromSecondsObject(&ts, obj, round) == -1) {
        return NULL;
    }
    return _PyTime_AsLong(ts);
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
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
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
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
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
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
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
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
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
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    PyTime_t ms = _PyTime_AsMilliseconds(t, round);
    return _PyTime_AsLong(ms);
}

static PyObject *
test_PyTime_AsMicroseconds(PyObject *self, PyObject *args)
{
    PyObject *obj;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
        return NULL;
    }
    PyTime_t t;
    if (_PyTime_FromLong(&t, obj) < 0) {
        return NULL;
    }
    if (check_time_rounding(round) < 0) {
        return NULL;
    }
    PyTime_t us = _PyTime_AsMicroseconds(t, round);
    return _PyTime_AsLong(us);
}

static PyObject *
test_pytime_object_to_time_t(PyObject *self, PyObject *args)
{
    PyObject *obj;
    time_t sec;
    int round;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
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
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
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
    if (!PyArg_ParseTuple(args, "Oi", &obj, &round)) {
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

static PyMethodDef TestMethods[] = {
    {"_PyTime_AsMicroseconds",    test_PyTime_AsMicroseconds,     METH_VARARGS},
    {"_PyTime_AsMilliseconds",    test_PyTime_AsMilliseconds,     METH_VARARGS},
#ifdef HAVE_CLOCK_GETTIME
    {"_PyTime_AsTimespec",        test_PyTime_AsTimespec,         METH_VARARGS},
    {"_PyTime_AsTimespec_clamp",  test_PyTime_AsTimespec_clamp,   METH_VARARGS},
#endif
    {"_PyTime_AsTimeval",         test_PyTime_AsTimeval,          METH_VARARGS},
    {"_PyTime_AsTimeval_clamp",   test_PyTime_AsTimeval_clamp,    METH_VARARGS},
    {"_PyTime_FromSeconds",       test_pytime_fromseconds,        METH_VARARGS},
    {"_PyTime_FromSecondsObject", test_pytime_fromsecondsobject,  METH_VARARGS},
    {"_PyTime_ObjectToTime_t",    test_pytime_object_to_time_t,   METH_VARARGS},
    {"_PyTime_ObjectToTimespec",  test_pytime_object_to_timespec, METH_VARARGS},
    {"_PyTime_ObjectToTimeval",   test_pytime_object_to_timeval,  METH_VARARGS},
    {NULL, NULL} /* sentinel */
};

int
_PyTestInternalCapi_Init_PyTime(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }
    return 0;
}
