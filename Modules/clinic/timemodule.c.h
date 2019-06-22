/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(time_time__doc__,
"time($module, /)\n"
"--\n"
"\n"
"Return the current time in seconds since the Epoch.\n"
"\n"
"Fractions of a second may be present if the system clock provides them.");

#define TIME_TIME_METHODDEF    \
    {"time", (PyCFunction)time_time, METH_NOARGS, time_time__doc__},

static PyObject *
time_time_impl(PyObject *module);

static PyObject *
time_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_time_impl(module);
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

PyDoc_STRVAR(time_clock_gettime_ns__doc__,
"clock_gettime_ns($module, clk_id, /)\n"
"--\n"
"\n"
"Return the time of the specified clock clk_id as nanoseconds.");

#define TIME_CLOCK_GETTIME_NS_METHODDEF    \
    {"clock_gettime_ns", (PyCFunction)time_clock_gettime_ns, METH_O, time_clock_gettime_ns__doc__},

static PyObject *
time_clock_gettime_ns_impl(PyObject *module, int clk_id);

static PyObject *
time_clock_gettime_ns(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int clk_id;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    clk_id = _PyLong_AsInt(arg);
    if (clk_id == -1 && PyErr_Occurred()) {
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
    {"clock_settime", (PyCFunction)(void(*)(void))time_clock_settime, METH_FASTCALL, time_clock_settime__doc__},

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
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    clk_id = _PyLong_AsInt(args[0]);
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
    {"clock_settime_ns", (PyCFunction)(void(*)(void))time_clock_settime_ns, METH_FASTCALL, time_clock_settime_ns__doc__},

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
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    clk_id = _PyLong_AsInt(args[0]);
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

static PyObject *
time_clock_getres_impl(PyObject *module, int clk_id);

static PyObject *
time_clock_getres(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int clk_id;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    clk_id = _PyLong_AsInt(arg);
    if (clk_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = time_clock_getres_impl(module, clk_id);

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

    if (!_PyLong_UnsignedLong_Converter(arg, &thread_id)) {
        goto exit;
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
"The argument may be a floating point number for subsecond precision.");

#define TIME_SLEEP_METHODDEF    \
    {"sleep", (PyCFunction)time_sleep, METH_O, time_sleep__doc__},

PyDoc_STRVAR(time_gmtime__doc__,
"gmtime($module, seconds=None, /)\n"
"--\n"
"\n"
"Convert seconds since the Epoch to a time tuple expressing UTC (a.k.a. GMT).\n"
"\n"
"When \'seconds\' is not passed in, convert the current time instead.  If the\n"
"platform supports the tm_gmtoff and tm_zone, they are available as attributes\n"
"only.");

#define TIME_GMTIME_METHODDEF    \
    {"gmtime", (PyCFunction)(void(*)(void))time_gmtime, METH_FASTCALL, time_gmtime__doc__},

static PyObject *
time_gmtime_impl(PyObject *module, PyObject *seconds);

static PyObject *
time_gmtime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *seconds = NULL;

    if (!_PyArg_CheckPositional("gmtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    seconds = args[0];
skip_optional:
    return_value = time_gmtime_impl(module, seconds);

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
    {"localtime", (PyCFunction)(void(*)(void))time_localtime, METH_FASTCALL, time_localtime__doc__},

static PyObject *
time_localtime_impl(PyObject *module, PyObject *seconds);

static PyObject *
time_localtime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *seconds = NULL;

    if (!_PyArg_CheckPositional("localtime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    seconds = args[0];
skip_optional:
    return_value = time_localtime_impl(module, seconds);

exit:
    return return_value;
}

#if defined(HAVE_STRFTIME)

PyDoc_STRVAR(time_strftime__doc__,
"strftime($module, format, tuple=None, /)\n"
"--\n"
"\n"
"Convert a time tuple to a string according to a format specification.\n"
"\n"
"See the library reference manual for formatting codes. When the time tuple\n"
"is not present, current time as returned by localtime() is used.\n"
"\n"
"Commonly used format codes:\n"
"\n"
"%Y  Year with century as a decimal number.\n"
"%m  Month as a decimal number [01,12].\n"
"%d  Day of the month as a decimal number [01,31].\n"
"%H  Hour (24-hour clock) as a decimal number [00,23].\n"
"%M  Minute as a decimal number [00,59].\n"
"%S  Second as a decimal number [00,61].\n"
"%z  Time zone offset from UTC.\n"
"%a  Locale\'s abbreviated weekday name.\n"
"%A  Locale\'s full weekday name.\n"
"%b  Locale\'s abbreviated month name.\n"
"%B  Locale\'s full month name.\n"
"%c  Locale\'s appropriate date and time representation.\n"
"%I  Hour (12-hour clock) as a decimal number [01,12].\n"
"%p  Locale\'s equivalent of either AM or PM.\n"
"\n"
"Other codes may be available on your platform.  See documentation for\n"
"the C library strftime function.");

#define TIME_STRFTIME_METHODDEF    \
    {"strftime", (PyCFunction)(void(*)(void))time_strftime, METH_FASTCALL, time_strftime__doc__},

static PyObject *
time_strftime_impl(PyObject *module, PyObject *format_arg, PyObject *tup);

static PyObject *
time_strftime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *format_arg;
    PyObject *tup = NULL;

    if (!_PyArg_CheckPositional("strftime", nargs, 1, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strftime", 1, "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    format_arg = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    tup = args[1];
skip_optional:
    return_value = time_strftime_impl(module, format_arg, tup);

exit:
    return return_value;
}

#endif /* defined(HAVE_STRFTIME) */

PyDoc_STRVAR(time_strptime__doc__,
"strptime($module, string, format=None, /)\n"
"--\n"
"\n"
"Parse a string to a time tuple according to a format specification.\n"
"\n"
"See the library reference manual for formatting codes (same as\n"
"strftime()).\n"
"\n"
"Commonly used format codes:\n"
"\n"
"%Y  Year with century as a decimal number.\n"
"%m  Month as a decimal number [01,12].\n"
"%d  Day of the month as a decimal number [01,31].\n"
"%H  Hour (24-hour clock) as a decimal number [00,23].\n"
"%M  Minute as a decimal number [00,59].\n"
"%S  Second as a decimal number [00,61].\n"
"%z  Time zone offset from UTC.\n"
"%a  Locale\'s abbreviated weekday name.\n"
"%A  Locale\'s full weekday name.\n"
"%b  Locale\'s abbreviated month name.\n"
"%B  Locale\'s full month name.\n"
"%c  Locale\'s appropriate date and time representation.\n"
"%I  Hour (12-hour clock) as a decimal number [01,12].\n"
"%p  Locale\'s equivalent of either AM or PM.\n"
"\n"
"Other codes may be available on your platform.  See documentation for\n"
"the C library strftime function.");

#define TIME_STRPTIME_METHODDEF    \
    {"strptime", (PyCFunction)(void(*)(void))time_strptime, METH_FASTCALL, time_strptime__doc__},

static PyObject *
time_strptime_impl(PyObject *module, PyObject *string, PyObject *format);

static PyObject *
time_strptime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *string;
    PyObject *format = NULL;

    if (!_PyArg_CheckPositional("strptime", nargs, 1, 2)) {
        goto exit;
    }
    string = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    format = args[1];
skip_optional:
    return_value = time_strptime_impl(module, string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(time_asctime__doc__,
"asctime($module, tuple=None, /)\n"
"--\n"
"\n"
"Convert a time tuple to a string, e.g. \'Sat Jun 06 16:26:11 1998\'.\n"
"\n"
"When the time tuple is not present, current time as returned by localtime()\n"
"is used.");

#define TIME_ASCTIME_METHODDEF    \
    {"asctime", (PyCFunction)(void(*)(void))time_asctime, METH_FASTCALL, time_asctime__doc__},

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
"This is equivalent to asctime(localtime(seconds)). When the time tuple is\n"
"not present, current time as returned by localtime() is used.");

#define TIME_CTIME_METHODDEF    \
    {"ctime", (PyCFunction)(void(*)(void))time_ctime, METH_FASTCALL, time_ctime__doc__},

static PyObject *
time_ctime_impl(PyObject *module, PyObject *seconds);

static PyObject *
time_ctime(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *seconds = NULL;

    if (!_PyArg_CheckPositional("ctime", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    seconds = args[0];
skip_optional:
    return_value = time_ctime_impl(module, seconds);

exit:
    return return_value;
}

#if defined(HAVE_MKTIME)

PyDoc_STRVAR(time_mktime__doc__,
"mktime($module, tuple, /)\n"
"--\n"
"\n"
"Convert a time tuple in local time to seconds since the Epoch.\n"
"\n"
"Note that mktime(gmtime(0)) will not generally return zero for most\n"
"time zones; instead the returned value will either be equal to that\n"
"of the timezone or altzone attributes on the time module.");

#define TIME_MKTIME_METHODDEF    \
    {"mktime", (PyCFunction)time_mktime, METH_O, time_mktime__doc__},

#endif /* defined(HAVE_MKTIME) */

#if defined(HAVE_WORKING_TZSET)

PyDoc_STRVAR(time_tzset__doc__,
"tzset($module, /)\n"
"--\n"
"\n"
"Initialize, or reinitialize, the local timezone to os.environ[\'TZ\'].\n"
"\n"
"The TZ environment variable should be specified in\n"
"standard Unix timezone format as documented in the tzset man page\n"
"(eg. \'US/Eastern\', \'Europe/Amsterdam\'). Unknown timezones will silently\n"
"fall back to UTC. If the TZ environment variable is not set, the local\n"
"timezone is set to the systems best guess of wallclock time.\n"
"Changing the TZ environment variable without calling tzset *may* change\n"
"the local timezone used by methods such as localtime, but this behaviour\n"
"should not be relied on.");

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

static PyObject *
time_monotonic_impl(PyObject *module);

static PyObject *
time_monotonic(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_monotonic_impl(module);
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

static PyObject *
time_perf_counter_impl(PyObject *module);

static PyObject *
time_perf_counter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_perf_counter_impl(module);
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
"Process time for profiling: sum of the kernel and user-space CPU time.");

#define TIME_PROCESS_TIME_METHODDEF    \
    {"process_time", (PyCFunction)time_process_time, METH_NOARGS, time_process_time__doc__},

static PyObject *
time_process_time_impl(PyObject *module);

static PyObject *
time_process_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_process_time_impl(module);
}

PyDoc_STRVAR(time_process_time_ns__doc__,
"process_time_ns($module, /)\n"
"--\n"
"\n"
"Process time for profiling as nanoseconds:\n"
"\n"
"sum of the kernel and user-space CPU time.");

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
"Thread time for profiling: sum of the kernel and user-space CPU time.");

#define TIME_THREAD_TIME_METHODDEF    \
    {"thread_time", (PyCFunction)time_thread_time, METH_NOARGS, time_thread_time__doc__},

static PyObject *
time_thread_time_impl(PyObject *module);

static PyObject *
time_thread_time(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return time_thread_time_impl(module);
}

#endif /* defined(HAVE_THREAD_TIME) */

#if defined(HAVE_THREAD_TIME)

PyDoc_STRVAR(time_thread_time_ns__doc__,
"thread_time_ns($module, /)\n"
"--\n"
"\n"
"Thread time for profiling as nanoseconds:\n"
"\n"
"sum of the kernel and user-space CPU time.");

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
        _PyArg_BadArgument("get_clock_info", 0, "str", arg);
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

#ifndef TIME_STRFTIME_METHODDEF
    #define TIME_STRFTIME_METHODDEF
#endif /* !defined(TIME_STRFTIME_METHODDEF) */

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
/*[clinic end generated code: output=b5a6f84ba148fdc3 input=a9049054013a1b77]*/
