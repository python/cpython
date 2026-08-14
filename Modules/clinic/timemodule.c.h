/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(time_time__doc__,
"time($module, /)\n"
"--\n"
"\n"
"Return the current time in seconds since the Epoch.\n"
"\n"
"Fractions of a second may be present if the system clock provides\n"
"them.");

#define TIME_TIME_METHODDEF    \
    {"time", (PyCFunction)time_time, METH_NOARGS, time_time__doc__},

static double
time_time_impl(PyObject *module);

static PyObject *
time_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = time_time_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_time_ns__doc__,
"time_ns($module, /)\n"
"--\n"
"\n"
"Return the current time in nanoseconds since the Epoch.");

#define TIME_TIME_NS_METHODDEF    \
    {"time_ns", (PyCFunction)time_time_ns, METH_NOARGS, time_time_ns__doc__},

static PyObject *
time_time_ns_impl(PyObject *module);

static PyObject *
time_time_ns(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_time_ns_impl(module);
}

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime__doc__,
"clock_gettime($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as a float.");

#define TIME_CLOCK_GETTIME_METHODDEF    \
    {"clock_gettime", (PyCFunction)time_clock_gettime, METH_O, time_clock_gettime__doc__},

static PyObject *
time_clock_gettime_impl(PyObject *module, clockid_t clk_id);

static PyObject *
time_clock_gettime(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(arg, &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_impl(module, clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#if defined(HAVE_CLOCK_GETTIME)

PyDoc_STRVAR(time_clock_gettime_ns__doc__,
"clock_gettime_ns($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as nanoseconds (int).");

#define TIME_CLOCK_GETTIME_NS_METHODDEF    \
    {"clock_gettime_ns", (PyCFunction)time_clock_gettime_ns, METH_O, time_clock_gettime_ns__doc__},

static PyObject *
time_clock_gettime_ns_impl(PyObject *module, clockid_t clk_id);

static PyObject *
time_clock_gettime_ns(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    clockid_t clk_id;

    if (!time_clockid_converter(arg, &clk_id)) {
        goto exit;
    }
    return_value = time_clock_gettime_ns_impl(module, clk_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETTIME) */

#if defined(HAVE_CLOCK_SETTIME)

PyDoc_STRVAR(time_clock_settime__doc__,
"clock_settime($module, clk_id, time, /)\n"
"--\n"
"\n"
"Set the time of the specified clock clk_id.");

#define TIME_CLOCK_SETTIME_METHODDEF    \
    {"clock_settime", _PyCFunction_CAST(time_clock_settime), METH_FASTCALL, time_clock_settime__doc__},

static PyObject *
time_clock_settime_impl(PyObject *module, int clk_id, PyObject *obj);

static PyObject *
time_clock_settime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int clk_id;
    PyObject *obj;

    if (!_PyArg_CheckPositional("clock_settime", nargs, 2, 2)) {
        goto exit;
    }
    clk_id = PyLong_AsInt(args[0]);
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    obj = args[1];
    return_value = time_clock_settime_impl(module, clk_id, obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_SETTIME) */

#if defined(HAVE_CLOCK_SETTIME)

PyDoc_STRVAR(time_clock_settime_ns__doc__,
"clock_settime_ns($module, clk_id, time, /)\n"
"--\n"
"\n"
"Set the time of the specified clock clk_id with nanoseconds.");

#define TIME_CLOCK_SETTIME_NS_METHODDEF    \
    {"clock_settime_ns", _PyCFunction_CAST(time_clock_settime_ns), METH_FASTCALL, time_clock_settime_ns__doc__},

static PyObject *
time_clock_settime_ns_impl(PyObject *module, int clk_id, PyObject *obj);

static PyObject *
time_clock_settime_ns(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int clk_id;
    PyObject *obj;

    if (!_PyArg_CheckPositional("clock_settime_ns", nargs, 2, 2)) {
        goto exit;
    }
    clk_id = PyLong_AsInt(args[0]);
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    obj = args[1];
    return_value = time_clock_settime_ns_impl(module, clk_id, obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_SETTIME) */

#if defined(HAVE_CLOCK_GETRES)

PyDoc_STRVAR(time_clock_getres__doc__,
"clock_getres($module, clk_id, /)\n"
"--\n"
"\n"
"Return the resolution (precision) of the specified clock clk_id.");

#define TIME_CLOCK_GETRES_METHODDEF    \
    {"clock_getres", (PyCFunction)time_clock_getres, METH_O, time_clock_getres__doc__},

static double
time_clock_getres_impl(PyObject *module, int clk_id);

static PyObject *
time_clock_getres(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int clk_id;
    double _return_value;

    clk_id = PyLong_AsInt(arg);
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = time_clock_getres_impl(module, clk_id);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_CLOCK_GETRES) */

#if defined(HAVE_PTHREAD_GETCPUCLOCKID)

PyDoc_STRVAR(time_pthread_getcpuclockid__doc__,
"pthread_getcpuclockid($module, thread_id, /)\n"
"--\n"
"\n"
"Return the clk_id of a thread\'s CPU time clock.");

#define TIME_PTHREAD_GETCPUCLOCKID_METHODDEF    \
    {"pthread_getcpuclockid", (PyCFunction)time_pthread_getcpuclockid, METH_O, time_pthread_getcpuclockid__doc__},

static PyObject *
time_pthread_getcpuclockid_impl(PyObject *module, unsigned long thread_id);

static PyObject *
time_pthread_getcpuclockid(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned long thread_id;

    if (!PyIndex_Check(arg)) {
        _PyArg_BadArgument("pthread_getcpuclockid", "argument", "int", arg);
        goto exit;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(arg, &thread_id, sizeof(unsigned long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    return_value = time_pthread_getcpuclockid_impl(module, thread_id);

exit:
    return return_value;
}

#endif /* defined(HAVE_PTHREAD_GETCPUCLOCKID) */

PyDoc_STRVAR(time_sleep__doc__,
"sleep($module, seconds, /)\n"
"--\n"
"\n"
"Delay execution for a given number of seconds.\n"
"\n"
"The argument may be a floating-point number for subsecond precision.");

#define TIME_SLEEP_METHODDEF    \
    {"sleep", (PyCFunction)time_sleep, METH_O, time_sleep__doc__},

PyDoc_STRVAR(time_gmtime__doc__,
"gmtime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert seconds since the Epoch to a time tuple expressing UTC.\n"
"\n"
"That is, Greenwich Mean Time.  When \'seconds\' is not passed in, convert\n"
"the current time instead.\n"
"\n"
"If the platform supports the tm_gmtoff and tm_zone, they are available\n"
"as attributes only.");

#define TIME_GMTIME_METHODDEF    \
    {"gmtime", _PyCFunction_CAST(time_gmtime), METH_FASTCALL, time_gmtime__doc__},

static PyObject *
time_gmtime_impl(PyObject *module, PyObject *ot);

static PyObject *
time_gmtime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ot = Py_None;

    if (!_PyArg_CheckPositional("gmtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = args[0];
skip_optional:
    return_value = time_gmtime_impl(module, ot);

exit:
    return return_value;
}

PyDoc_STRVAR(time_localtime__doc__,
"localtime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert seconds since the Epoch to a time tuple expressing local time.\n"
"\n"
"When \'seconds\' is not passed in, convert the current time instead.");

#define TIME_LOCALTIME_METHODDEF    \
    {"localtime", _PyCFunction_CAST(time_localtime), METH_FASTCALL, time_localtime__doc__},

static PyObject *
time_localtime_impl(PyObject *module, PyObject *ot);

static PyObject *
time_localtime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ot = Py_None;

    if (!_PyArg_CheckPositional("localtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = args[0];
skip_optional:
    return_value = time_localtime_impl(module, ot);

exit:
    return return_value;
}

PyDoc_STRVAR(time_asctime__doc__,
"asctime($module, time_tuple=<unrepresentable>, /)\n"
"--\n"
"\n"
"Convert a time tuple to a string, e.g. \'Sat Jun 06 16:26:11 1998\'.\n"
"\n"
"When the time tuple is not present, current time as returned by\n"
"localtime() is used.");

#define TIME_ASCTIME_METHODDEF    \
    {"asctime", _PyCFunction_CAST(time_asctime), METH_FASTCALL, time_asctime__doc__},

static PyObject *
time_asctime_impl(PyObject *module, PyObject *tup);

static PyObject *
time_asctime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *tup = NULL;

    if (!_PyArg_CheckPositional("asctime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    tup = args[0];
skip_optional:
    return_value = time_asctime_impl(module, tup);

exit:
    return return_value;
}

PyDoc_STRVAR(time_ctime__doc__,
"ctime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert a time in seconds since the Epoch to a string in local time.\n"
"\n"
"This is equivalent to asctime(localtime(seconds)).  When \'seconds\' is\n"
"not passed in, convert the current time instead.");

#define TIME_CTIME_METHODDEF    \
    {"ctime", _PyCFunction_CAST(time_ctime), METH_FASTCALL, time_ctime__doc__},

static PyObject *
time_ctime_impl(PyObject *module, PyObject *ot);

static PyObject *
time_ctime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ot = Py_None;

    if (!_PyArg_CheckPositional("ctime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ot = args[0];
skip_optional:
    return_value = time_ctime_impl(module, ot);

exit:
    return return_value;
}

#if defined(HAVE_MKTIME)

PyDoc_STRVAR(time_mktime__doc__,
"mktime($module, time_tuple, /)\n"
"--\n"
"\n"
"Convert a time tuple in local time to seconds since the Epoch.\n"
"\n"
"Note that mktime(gmtime(0)) will not generally return zero for most\n"
"time zones; instead the returned value will either be equal to that of\n"
"the timezone or altzone attributes on the time module.");

#define TIME_MKTIME_METHODDEF    \
    {"mktime", (PyCFunction)time_mktime, METH_O, time_mktime__doc__},

#endif /* defined(HAVE_MKTIME) */

#if defined(HAVE_WORKING_TZSET)

PyDoc_STRVAR(time_tzset__doc__,
"tzset($module, /)\n"
"--\n"
"\n"
"Initialize, or reinitialize, the local timezone.\n"
"\n"
"The value is stored in os.environ[\'TZ\'].  The TZ environment variable\n"
"should be specified in standard Unix timezone format as documented in\n"
"the tzset man page (eg. \'US/Eastern\', \'Europe/Amsterdam\').  Unknown\n"
"timezones will silently fall back to UTC.  If the TZ environment\n"
"variable is not set, the local timezone is set to the systems best\n"
"guess of wallclock time.  Changing the TZ environment variable without\n"
"calling tzset *may* change the local timezone used by methods such as\n"
"localtime, but this behaviour should not be relied on.");

#define TIME_TZSET_METHODDEF    \
    {"tzset", (PyCFunction)time_tzset, METH_NOARGS, time_tzset__doc__},

static PyObject *
time_tzset_impl(PyObject *module);

static PyObject *
time_tzset(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_tzset_impl(module);
}

#endif /* defined(HAVE_WORKING_TZSET) */

PyDoc_STRVAR(time_monotonic__doc__,
"monotonic($module, /)\n"
"--\n"
"\n"
"Monotonic clock, cannot go backward.");

#define TIME_MONOTONIC_METHODDEF    \
    {"monotonic", (PyCFunction)time_monotonic, METH_NOARGS, time_monotonic__doc__},

static double
time_monotonic_impl(PyObject *module);

static PyObject *
time_monotonic(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = time_monotonic_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_monotonic_ns__doc__,
"monotonic_ns($module, /)\n"
"--\n"
"\n"
"Monotonic clock, cannot go backward, as nanoseconds.");

#define TIME_MONOTONIC_NS_METHODDEF    \
    {"monotonic_ns", (PyCFunction)time_monotonic_ns, METH_NOARGS, time_monotonic_ns__doc__},

static PyObject *
time_monotonic_ns_impl(PyObject *module);

static PyObject *
time_monotonic_ns(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_monotonic_ns_impl(module);
}

PyDoc_STRVAR(time_perf_counter__doc__,
"perf_counter($module, /)\n"
"--\n"
"\n"
"Performance counter for benchmarking.");

#define TIME_PERF_COUNTER_METHODDEF    \
    {"perf_counter", (PyCFunction)time_perf_counter, METH_NOARGS, time_perf_counter__doc__},

static double
time_perf_counter_impl(PyObject *module);

static PyObject *
time_perf_counter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = time_perf_counter_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_perf_counter_ns__doc__,
"perf_counter_ns($module, /)\n"
"--\n"
"\n"
"Performance counter for benchmarking as nanoseconds.");

#define TIME_PERF_COUNTER_NS_METHODDEF    \
    {"perf_counter_ns", (PyCFunction)time_perf_counter_ns, METH_NOARGS, time_perf_counter_ns__doc__},

static PyObject *
time_perf_counter_ns_impl(PyObject *module);

static PyObject *
time_perf_counter_ns(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_perf_counter_ns_impl(module);
}

PyDoc_STRVAR(time_process_time__doc__,
"process_time($module, /)\n"
"--\n"
"\n"
"Process time for profiling.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_PROCESS_TIME_METHODDEF    \
    {"process_time", (PyCFunction)time_process_time, METH_NOARGS, time_process_time__doc__},

static double
time_process_time_impl(PyObject *module);

static PyObject *
time_process_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = time_process_time_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(time_process_time_ns__doc__,
"process_time_ns($module, /)\n"
"--\n"
"\n"
"Process time for profiling as nanoseconds.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_PROCESS_TIME_NS_METHODDEF    \
    {"process_time_ns", (PyCFunction)time_process_time_ns, METH_NOARGS, time_process_time_ns__doc__},

static PyObject *
time_process_time_ns_impl(PyObject *module);

static PyObject *
time_process_time_ns(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_process_time_ns_impl(module);
}

#if defined(HAVE_THREAD_TIME)

PyDoc_STRVAR(time_thread_time__doc__,
"thread_time($module, /)\n"
"--\n"
"\n"
"Thread time for profiling.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_THREAD_TIME_METHODDEF    \
    {"thread_time", (PyCFunction)time_thread_time, METH_NOARGS, time_thread_time__doc__},

static double
time_thread_time_impl(PyObject *module);

static PyObject *
time_thread_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    double _return_value;

    _return_value = time_thread_time_impl(module);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_THREAD_TIME) */

#if defined(HAVE_THREAD_TIME)

PyDoc_STRVAR(time_thread_time_ns__doc__,
"thread_time_ns($module, /)\n"
"--\n"
"\n"
"Thread time for profiling as nanoseconds.\n"
"\n"
"That is the sum of the kernel and user-space CPU time.");

#define TIME_THREAD_TIME_NS_METHODDEF    \
    {"thread_time_ns", (PyCFunction)time_thread_time_ns, METH_NOARGS, time_thread_time_ns__doc__},

static PyObject *
time_thread_time_ns_impl(PyObject *module);

static PyObject *
time_thread_time_ns(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_thread_time_ns_impl(module);
}

#endif /* defined(HAVE_THREAD_TIME) */

PyDoc_STRVAR(time_get_clock_info__doc__,
"get_clock_info($module, name, /)\n"
"--\n"
"\n"
"Get information of the specified clock.");

#define TIME_GET_CLOCK_INFO_METHODDEF    \
    {"get_clock_info", (PyCFunction)time_get_clock_info, METH_O, time_get_clock_info__doc__},

static PyObject *
time_get_clock_info_impl(PyObject *module, const char *name);

static PyObject *
time_get_clock_info(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("get_clock_info", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t name_length;
    name = PyUnicode_AsUTF8AndSize(arg, &name_length);
    if (name == NULL) {
        goto exit;
    }
    if (strlen(name) != (size_t)name_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = time_get_clock_info_impl(module, name);

exit:
    return return_value;
}

#ifndef TIME_CLOCK_GETTIME_METHODDEF
    #define TIME_CLOCK_GETTIME_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_METHODDEF) */

#ifndef TIME_CLOCK_GETTIME_NS_METHODDEF
    #define TIME_CLOCK_GETTIME_NS_METHODDEF
#endif /* !defined(TIME_CLOCK_GETTIME_NS_METHODDEF) */

#ifndef TIME_CLOCK_SETTIME_METHODDEF
    #define TIME_CLOCK_SETTIME_METHODDEF
#endif /* !defined(TIME_CLOCK_SETTIME_METHODDEF) */

#ifndef TIME_CLOCK_SETTIME_NS_METHODDEF
    #define TIME_CLOCK_SETTIME_NS_METHODDEF
#endif /* !defined(TIME_CLOCK_SETTIME_NS_METHODDEF) */

#ifndef TIME_CLOCK_GETRES_METHODDEF
    #define TIME_CLOCK_GETRES_METHODDEF
#endif /* !defined(TIME_CLOCK_GETRES_METHODDEF) */

#ifndef TIME_PTHREAD_GETCPUCLOCKID_METHODDEF
    #define TIME_PTHREAD_GETCPUCLOCKID_METHODDEF
#endif /* !defined(TIME_PTHREAD_GETCPUCLOCKID_METHODDEF) */

#ifndef TIME_MKTIME_METHODDEF
    #define TIME_MKTIME_METHODDEF
#endif /* !defined(TIME_MKTIME_METHODDEF) */

#ifndef TIME_TZSET_METHODDEF
    #define TIME_TZSET_METHODDEF
#endif /* !defined(TIME_TZSET_METHODDEF) */

#ifndef TIME_THREAD_TIME_METHODDEF
    #define TIME_THREAD_TIME_METHODDEF
#endif /* !defined(TIME_THREAD_TIME_METHODDEF) */

#ifndef TIME_THREAD_TIME_NS_METHODDEF
    #define TIME_THREAD_TIME_NS_METHODDEF
#endif /* !defined(TIME_THREAD_TIME_NS_METHODDEF) */
/*[clinic end generated code: output=75f2a96aa9a21a15 input=a9049054013a1b77]*/
