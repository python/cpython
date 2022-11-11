#include "parts.h"

#include "datetime.h"             // PyDateTimeAPI

#ifdef MS_WINDOWS
#  include <winsock2.h>           // struct timeval
#endif

static int test_run_counter = 0;

static PyObject *
test_datetime_capi(PyObject *self, PyObject *args)
{
    if (PyDateTimeAPI) {
        if (test_run_counter) {
            /* Probably regrtest.py -R */
            Py_RETURN_NONE;
        }
        else {
            PyErr_SetString(PyExc_AssertionError,
                            "PyDateTime_CAPI somehow initialized");
            return NULL;
        }
    }
    test_run_counter++;
    PyDateTime_IMPORT;

    if (PyDateTimeAPI) {
        Py_RETURN_NONE;
    }
    return NULL;
}

/* Functions exposing the C API type checking for testing */
#define MAKE_DATETIME_CHECK_FUNC(check_method, exact_method)    \
do {                                                            \
    PyObject *obj;                                              \
    int exact = 0;                                              \
    if (!PyArg_ParseTuple(args, "O|p", &obj, &exact)) {         \
        return NULL;                                            \
    }                                                           \
    int rv = exact?exact_method(obj):check_method(obj);         \
    if (rv) {                                                   \
        Py_RETURN_TRUE;                                         \
    }                                                           \
    Py_RETURN_FALSE;                                            \
} while (0)                                                     \

static PyObject *
datetime_check_date(PyObject *self, PyObject *args)
{
    MAKE_DATETIME_CHECK_FUNC(PyDate_Check, PyDate_CheckExact);
}

static PyObject *
datetime_check_time(PyObject *self, PyObject *args)
{
    MAKE_DATETIME_CHECK_FUNC(PyTime_Check, PyTime_CheckExact);
}

static PyObject *
datetime_check_datetime(PyObject *self, PyObject *args)
{
    MAKE_DATETIME_CHECK_FUNC(PyDateTime_Check, PyDateTime_CheckExact);
}

static PyObject *
datetime_check_delta(PyObject *self, PyObject *args)
{
    MAKE_DATETIME_CHECK_FUNC(PyDelta_Check, PyDelta_CheckExact);
}

static PyObject *
datetime_check_tzinfo(PyObject *self, PyObject *args)
{
    MAKE_DATETIME_CHECK_FUNC(PyTZInfo_Check, PyTZInfo_CheckExact);
}
#undef MAKE_DATETIME_CHECK_FUNC


/* Makes three variations on timezone representing UTC-5:
   1. timezone with offset and name from PyDateTimeAPI
   2. timezone with offset and name from PyTimeZone_FromOffsetAndName
   3. timezone with offset (no name) from PyTimeZone_FromOffset
*/
static PyObject *
make_timezones_capi(PyObject *self, PyObject *args)
{
    PyObject *offset = PyDelta_FromDSU(0, -18000, 0);
    PyObject *name = PyUnicode_FromString("EST");

    PyObject *est_zone_capi = PyDateTimeAPI->TimeZone_FromTimeZone(offset, name);
    PyObject *est_zone_macro = PyTimeZone_FromOffsetAndName(offset, name);
    PyObject *est_zone_macro_noname = PyTimeZone_FromOffset(offset);

    Py_DecRef(offset);
    Py_DecRef(name);

    PyObject *rv = PyTuple_New(3);
    if (rv == NULL) {
        return NULL;
    }

    PyTuple_SET_ITEM(rv, 0, est_zone_capi);
    PyTuple_SET_ITEM(rv, 1, est_zone_macro);
    PyTuple_SET_ITEM(rv, 2, est_zone_macro_noname);

    return rv;
}

static PyObject *
get_timezones_offset_zero(PyObject *self, PyObject *args)
{
    PyObject *offset = PyDelta_FromDSU(0, 0, 0);
    PyObject *name = PyUnicode_FromString("");

    // These two should return the UTC singleton
    PyObject *utc_singleton_0 = PyTimeZone_FromOffset(offset);
    PyObject *utc_singleton_1 = PyTimeZone_FromOffsetAndName(offset, NULL);

    // This one will return +00:00 zone, but not the UTC singleton
    PyObject *non_utc_zone = PyTimeZone_FromOffsetAndName(offset, name);

    Py_DecRef(offset);
    Py_DecRef(name);

    PyObject *rv = PyTuple_New(3);
    PyTuple_SET_ITEM(rv, 0, utc_singleton_0);
    PyTuple_SET_ITEM(rv, 1, utc_singleton_1);
    PyTuple_SET_ITEM(rv, 2, non_utc_zone);

    return rv;
}

static PyObject *
get_timezone_utc_capi(PyObject *self, PyObject *args)
{
    int macro = 0;
    if (!PyArg_ParseTuple(args, "|p", &macro)) {
        return NULL;
    }
    if (macro) {
        Py_INCREF(PyDateTime_TimeZone_UTC);
        return PyDateTime_TimeZone_UTC;
    }
    Py_INCREF(PyDateTimeAPI->TimeZone_UTC);
    return PyDateTimeAPI->TimeZone_UTC;
}

static PyObject *
get_date_fromdate(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int year, month, day;

    if (!PyArg_ParseTuple(args, "piii", &macro, &year, &month, &day)) {
        return NULL;
    }

    if (macro) {
        rv = PyDate_FromDate(year, month, day);
    }
    else {
        rv = PyDateTimeAPI->Date_FromDate(
                year, month, day,
                PyDateTimeAPI->DateType);
    }
    return rv;
}

static PyObject *
get_datetime_fromdateandtime(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int year, month, day;
    int hour, minute, second, microsecond;

    if (!PyArg_ParseTuple(args, "piiiiiii",
                          &macro,
                          &year, &month, &day,
                          &hour, &minute, &second, &microsecond)) {
        return NULL;
    }

    if (macro) {
        rv = PyDateTime_FromDateAndTime(
                year, month, day,
                hour, minute, second, microsecond);
    }
    else {
        rv = PyDateTimeAPI->DateTime_FromDateAndTime(
                year, month, day,
                hour, minute, second, microsecond,
                Py_None,
                PyDateTimeAPI->DateTimeType);
    }
    return rv;
}

static PyObject *
get_datetime_fromdateandtimeandfold(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int year, month, day;
    int hour, minute, second, microsecond, fold;

    if (!PyArg_ParseTuple(args, "piiiiiiii",
                          &macro,
                          &year, &month, &day,
                          &hour, &minute, &second, &microsecond,
                          &fold)) {
        return NULL;
    }

    if (macro) {
        rv = PyDateTime_FromDateAndTimeAndFold(
                year, month, day,
                hour, minute, second, microsecond,
                fold);
    }
    else {
        rv = PyDateTimeAPI->DateTime_FromDateAndTimeAndFold(
                year, month, day,
                hour, minute, second, microsecond,
                Py_None,
                fold,
                PyDateTimeAPI->DateTimeType);
    }
    return rv;
}

static PyObject *
get_time_fromtime(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int hour, minute, second, microsecond;

    if (!PyArg_ParseTuple(args, "piiii",
                          &macro,
                          &hour, &minute, &second, &microsecond))
    {
        return NULL;
    }

    if (macro) {
        rv = PyTime_FromTime(hour, minute, second, microsecond);
    }
    else {
        rv = PyDateTimeAPI->Time_FromTime(
                hour, minute, second, microsecond,
                Py_None,
                PyDateTimeAPI->TimeType);
    }
    return rv;
}

static PyObject *
get_time_fromtimeandfold(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int hour, minute, second, microsecond, fold;

    if (!PyArg_ParseTuple(args, "piiiii",
                          &macro,
                          &hour, &minute, &second, &microsecond,
                          &fold)) {
        return NULL;
    }

    if (macro) {
        rv = PyTime_FromTimeAndFold(hour, minute, second, microsecond, fold);
    }
    else {
        rv = PyDateTimeAPI->Time_FromTimeAndFold(
                hour, minute, second, microsecond,
                Py_None,
                fold,
                PyDateTimeAPI->TimeType);
    }
    return rv;
}

static PyObject *
get_delta_fromdsu(PyObject *self, PyObject *args)
{
    PyObject *rv = NULL;
    int macro;
    int days, seconds, microseconds;

    if (!PyArg_ParseTuple(args, "piii",
                          &macro,
                          &days, &seconds, &microseconds)) {
        return NULL;
    }

    if (macro) {
        rv = PyDelta_FromDSU(days, seconds, microseconds);
    }
    else {
        rv = PyDateTimeAPI->Delta_FromDelta(
                days, seconds, microseconds, 1,
                PyDateTimeAPI->DeltaType);
    }

    return rv;
}

static PyObject *
get_date_fromtimestamp(PyObject *self, PyObject *args)
{
    PyObject *tsargs = NULL, *ts = NULL, *rv = NULL;
    int macro = 0;

    if (!PyArg_ParseTuple(args, "O|p", &ts, &macro)) {
        return NULL;
    }

    // Construct the argument tuple
    if ((tsargs = PyTuple_Pack(1, ts)) == NULL) {
        return NULL;
    }

    // Pass along to the API function
    if (macro) {
        rv = PyDate_FromTimestamp(tsargs);
    }
    else {
        rv = PyDateTimeAPI->Date_FromTimestamp(
                (PyObject *)PyDateTimeAPI->DateType, tsargs
        );
    }

    Py_DECREF(tsargs);
    return rv;
}

static PyObject *
get_datetime_fromtimestamp(PyObject *self, PyObject *args)
{
    int macro = 0;
    int usetz = 0;
    PyObject *tsargs = NULL, *ts = NULL, *tzinfo = Py_None, *rv = NULL;
    if (!PyArg_ParseTuple(args, "OO|pp", &ts, &tzinfo, &usetz, &macro)) {
        return NULL;
    }

    // Construct the argument tuple
    if (usetz) {
        tsargs = PyTuple_Pack(2, ts, tzinfo);
    }
    else {
        tsargs = PyTuple_Pack(1, ts);
    }

    if (tsargs == NULL) {
        return NULL;
    }

    // Pass along to the API function
    if (macro) {
        rv = PyDateTime_FromTimestamp(tsargs);
    }
    else {
        rv = PyDateTimeAPI->DateTime_FromTimestamp(
                (PyObject *)PyDateTimeAPI->DateTimeType, tsargs, NULL
        );
    }

    Py_DECREF(tsargs);
    return rv;
}

static PyObject *
test_PyDateTime_GET(PyObject *self, PyObject *obj)
{
    int year, month, day;

    year = PyDateTime_GET_YEAR(obj);
    month = PyDateTime_GET_MONTH(obj);
    day = PyDateTime_GET_DAY(obj);

    return Py_BuildValue("(iii)", year, month, day);
}

static PyObject *
test_PyDateTime_DATE_GET(PyObject *self, PyObject *obj)
{
    int hour = PyDateTime_DATE_GET_HOUR(obj);
    int minute = PyDateTime_DATE_GET_MINUTE(obj);
    int second = PyDateTime_DATE_GET_SECOND(obj);
    int microsecond = PyDateTime_DATE_GET_MICROSECOND(obj);
    PyObject *tzinfo = PyDateTime_DATE_GET_TZINFO(obj);

    return Py_BuildValue("(iiiiO)", hour, minute, second, microsecond, tzinfo);
}

static PyObject *
test_PyDateTime_TIME_GET(PyObject *self, PyObject *obj)
{
    int hour = PyDateTime_TIME_GET_HOUR(obj);
    int minute = PyDateTime_TIME_GET_MINUTE(obj);
    int second = PyDateTime_TIME_GET_SECOND(obj);
    int microsecond = PyDateTime_TIME_GET_MICROSECOND(obj);
    PyObject *tzinfo = PyDateTime_TIME_GET_TZINFO(obj);

    return Py_BuildValue("(iiiiO)", hour, minute, second, microsecond, tzinfo);
}

static PyObject *
test_PyDateTime_DELTA_GET(PyObject *self, PyObject *obj)
{
    int days = PyDateTime_DELTA_GET_DAYS(obj);
    int seconds = PyDateTime_DELTA_GET_SECONDS(obj);
    int microseconds = PyDateTime_DELTA_GET_MICROSECONDS(obj);

    return Py_BuildValue("(iii)", days, seconds, microseconds);
}

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
    {"PyDateTime_DATE_GET",         test_PyDateTime_DATE_GET,       METH_O},
    {"PyDateTime_DELTA_GET",        test_PyDateTime_DELTA_GET,      METH_O},
    {"PyDateTime_GET",              test_PyDateTime_GET,            METH_O},
    {"PyDateTime_TIME_GET",         test_PyDateTime_TIME_GET,       METH_O},
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
    {"datetime_check_date",         datetime_check_date,            METH_VARARGS},
    {"datetime_check_datetime",     datetime_check_datetime,        METH_VARARGS},
    {"datetime_check_delta",        datetime_check_delta,           METH_VARARGS},
    {"datetime_check_time",         datetime_check_time,            METH_VARARGS},
    {"datetime_check_tzinfo",       datetime_check_tzinfo,          METH_VARARGS},
    {"get_date_fromdate",           get_date_fromdate,              METH_VARARGS},
    {"get_date_fromtimestamp",      get_date_fromtimestamp,         METH_VARARGS},
    {"get_datetime_fromdateandtime", get_datetime_fromdateandtime,  METH_VARARGS},
    {"get_datetime_fromdateandtimeandfold", get_datetime_fromdateandtimeandfold, METH_VARARGS},
    {"get_datetime_fromtimestamp",  get_datetime_fromtimestamp,     METH_VARARGS},
    {"get_delta_fromdsu",           get_delta_fromdsu,              METH_VARARGS},
    {"get_time_fromtime",           get_time_fromtime,              METH_VARARGS},
    {"get_time_fromtimeandfold",    get_time_fromtimeandfold,       METH_VARARGS},
    {"get_timezone_utc_capi",       get_timezone_utc_capi,          METH_VARARGS},
    {"get_timezones_offset_zero",   get_timezones_offset_zero,      METH_NOARGS},
    {"make_timezones_capi",         make_timezones_capi,            METH_NOARGS},
    {"pytime_object_to_time_t",     test_pytime_object_to_time_t,   METH_VARARGS},
    {"pytime_object_to_timespec",   test_pytime_object_to_timespec, METH_VARARGS},
    {"pytime_object_to_timeval",    test_pytime_object_to_timeval,  METH_VARARGS},
    {"test_datetime_capi",          test_datetime_capi,             METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_Time(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
