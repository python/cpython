/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(delta_new__doc__,
"timedelta(days=0, seconds=0, microseconds=0, milliseconds=0, minutes=0,\n"
"          hours=0, weeks=0)\n"
"--\n"
"\n"
"Difference between two datetime values.\n"
"\n"
"All arguments are optional and default to 0.\n"
"Arguments may be integers or floats, and may be positive or negative.");

static PyObject *
delta_new_impl(PyTypeObject *type, PyObject *days, PyObject *seconds,
               PyObject *microseconds, PyObject *milliseconds,
               PyObject *minutes, PyObject *hours, PyObject *weeks);

static PyObject *
delta_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 7
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(days), &_Py_ID(seconds), &_Py_ID(microseconds), &_Py_ID(milliseconds), &_Py_ID(minutes), &_Py_ID(hours), &_Py_ID(weeks), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"days", "seconds", "microseconds", "milliseconds", "minutes", "hours", "weeks", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "timedelta",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[7];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *days = NULL;
    PyObject *seconds = NULL;
    PyObject *microseconds = NULL;
    PyObject *milliseconds = NULL;
    PyObject *minutes = NULL;
    PyObject *hours = NULL;
    PyObject *weeks = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 7, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        days = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        seconds = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        microseconds = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        milliseconds = fastargs[3];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        minutes = fastargs[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[5]) {
        hours = fastargs[5];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    weeks = fastargs[6];
skip_optional_pos:
    return_value = delta_new_impl(type, days, seconds, microseconds, milliseconds, minutes, hours, weeks);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date__doc__,
"date(year, month, day)\n"
"--\n"
"\n"
"Concrete date type.");

static PyObject *
datetime_date_impl(PyTypeObject *type, int year, int month, int day);

static PyObject *
datetime_date(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(month), &_Py_ID(day), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "date",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    int year;
    int month;
    int day;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    year = PyLong_AsInt(fastargs[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    month = PyLong_AsInt(fastargs[1]);
    if (month == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = PyLong_AsInt(fastargs[2]);
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_impl(type, year, month, day);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_today__doc__,
"today($type, /)\n"
"--\n"
"\n"
"Current date or datetime.\n"
"\n"
"Equivalent to fromtimestamp(time.time()).");

#define DATETIME_DATE_TODAY_METHODDEF    \
    {"today", (PyCFunction)datetime_date_today, METH_NOARGS|METH_CLASS, datetime_date_today__doc__},

static PyObject *
datetime_date_today_impl(PyTypeObject *type);

static PyObject *
datetime_date_today(PyObject *type, PyObject *Py_UNUSED(ignored))
{
    return datetime_date_today_impl((PyTypeObject *)type);
}

PyDoc_STRVAR(datetime_date_fromtimestamp__doc__,
"fromtimestamp($type, timestamp, /)\n"
"--\n"
"\n"
"Create a date from a POSIX timestamp.\n"
"\n"
"The timestamp is a number, e.g. created via time.time(), that is interpreted\n"
"as local time.");

#define DATETIME_DATE_FROMTIMESTAMP_METHODDEF    \
    {"fromtimestamp", (PyCFunction)datetime_date_fromtimestamp, METH_O|METH_CLASS, datetime_date_fromtimestamp__doc__},

static PyObject *
datetime_date_fromtimestamp_impl(PyTypeObject *type, PyObject *timestamp);

static PyObject *
datetime_date_fromtimestamp(PyObject *type, PyObject *timestamp)
{
    PyObject *return_value = NULL;

    return_value = datetime_date_fromtimestamp_impl((PyTypeObject *)type, timestamp);

    return return_value;
}

PyDoc_STRVAR(datetime_date_fromordinal__doc__,
"fromordinal($type, ordinal, /)\n"
"--\n"
"\n"
"Construct a date from a proleptic Gregorian ordinal.\n"
"\n"
"January 1 of year 1 is day 1.  Only the year, month and day are\n"
"non-zero in the result.");

#define DATETIME_DATE_FROMORDINAL_METHODDEF    \
    {"fromordinal", (PyCFunction)datetime_date_fromordinal, METH_O|METH_CLASS, datetime_date_fromordinal__doc__},

static PyObject *
datetime_date_fromordinal_impl(PyTypeObject *type, int ordinal);

static PyObject *
datetime_date_fromordinal(PyObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    int ordinal;

    ordinal = PyLong_AsInt(arg);
    if (ordinal == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromordinal_impl((PyTypeObject *)type, ordinal);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a date from a string in ISO 8601 format.");

#define DATETIME_DATE_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", (PyCFunction)datetime_date_fromisoformat, METH_O|METH_CLASS, datetime_date_fromisoformat__doc__},

static PyObject *
datetime_date_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyObject *
datetime_date_fromisoformat(PyObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", arg);
        goto exit;
    }
    string = arg;
    return_value = datetime_date_fromisoformat_impl((PyTypeObject *)type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_fromisocalendar__doc__,
"fromisocalendar($type, /, year, week, day)\n"
"--\n"
"\n"
"Construct a date from the ISO year, week number and weekday.\n"
"\n"
"This is the inverse of the date.isocalendar() function.");

#define DATETIME_DATE_FROMISOCALENDAR_METHODDEF    \
    {"fromisocalendar", _PyCFunction_CAST(datetime_date_fromisocalendar), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_date_fromisocalendar__doc__},

static PyObject *
datetime_date_fromisocalendar_impl(PyTypeObject *type, int year, int week,
                                   int day);

static PyObject *
datetime_date_fromisocalendar(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(week), &_Py_ID(day), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "week", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromisocalendar",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    int year;
    int week;
    int day;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    year = PyLong_AsInt(args[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    week = PyLong_AsInt(args[1]);
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = PyLong_AsInt(args[2]);
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromisocalendar_impl((PyTypeObject *)type, year, week, day);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given date format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATE_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_date_strptime), METH_FASTCALL|METH_CLASS, datetime_date_strptime__doc__},

static PyObject *
datetime_date_strptime_impl(PyTypeObject *type, PyObject *string,
                            PyObject *format);

static PyObject *
datetime_date_strptime(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strptime", "argument 1", "str", args[0]);
        goto exit;
    }
    string = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("strptime", "argument 2", "str", args[1]);
        goto exit;
    }
    format = args[1];
    return_value = datetime_date_strptime_impl((PyTypeObject *)type, string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_strftime__doc__,
"strftime($self, /, format)\n"
"--\n"
"\n"
"Format using strftime().\n"
"\n"
"Example: \"%d/%m/%Y, %H:%M:%S\".\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATE_STRFTIME_METHODDEF    \
    {"strftime", _PyCFunction_CAST(datetime_date_strftime), METH_FASTCALL|METH_KEYWORDS, datetime_date_strftime__doc__},

static PyObject *
datetime_date_strftime_impl(PyObject *self, PyObject *format);

static PyObject *
datetime_date_strftime(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(format), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "strftime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *format;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strftime", "argument 'format'", "str", args[0]);
        goto exit;
    }
    format = args[0];
    return_value = datetime_date_strftime_impl(self, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date___format____doc__,
"__format__($self, format, /)\n"
"--\n"
"\n"
"Formats self with strftime.");

#define DATETIME_DATE___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)datetime_date___format__, METH_O, datetime_date___format____doc__},

static PyObject *
datetime_date___format___impl(PyObject *self, PyObject *format);

static PyObject *
datetime_date___format__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *format;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__format__", "argument", "str", arg);
        goto exit;
    }
    format = arg;
    return_value = datetime_date___format___impl(self, format);

exit:
    return return_value;
}

static PyObject *
iso_calendar_date_new_impl(PyTypeObject *type, int year, int week,
                           int weekday);

static PyObject *
iso_calendar_date_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(week), &_Py_ID(weekday), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "week", "weekday", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "IsoCalendarDate",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    int year;
    int week;
    int weekday;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 3, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    year = PyLong_AsInt(fastargs[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    week = PyLong_AsInt(fastargs[1]);
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    weekday = PyLong_AsInt(fastargs[2]);
    if (weekday == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = iso_calendar_date_new_impl(type, year, week, weekday);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_replace__doc__,
"replace($self, /, year=unchanged, month=unchanged, day=unchanged)\n"
"--\n"
"\n"
"Return date with new specified fields.");

#define DATETIME_DATE_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_date_replace), METH_FASTCALL|METH_KEYWORDS, datetime_date_replace__doc__},

static PyObject *
datetime_date_replace_impl(PyDateTime_Date *self, int year, int month,
                           int day);

static PyObject *
datetime_date_replace(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(month), &_Py_ID(day), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int year = GET_YEAR(self);
    int month = GET_MONTH(self);
    int day = GET_DAY(self);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        year = PyLong_AsInt(args[0]);
        if (year == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        month = PyLong_AsInt(args[1]);
        if (month == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    day = PyLong_AsInt(args[2]);
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = datetime_date_replace_impl((PyDateTime_Date *)self, year, month, day);

exit:
    return return_value;
}

PyDoc_STRVAR(timezone_new__doc__,
"timezone(offset, name=<unrepresentable>)\n"
"--\n"
"\n"
"Fixed offset from UTC implementation of tzinfo.");

static PyObject *
timezone_new_impl(PyTypeObject *type, PyObject *offset, PyObject *name);

static PyObject *
timezone_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(offset), &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"offset", "name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "timezone",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *offset;
    PyObject *name = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyObject_TypeCheck(fastargs[0], DELTA_TYPE(NO_STATE))) {
        _PyArg_BadArgument("timezone", "argument 'offset'", (DELTA_TYPE(NO_STATE))->tp_name, fastargs[0]);
        goto exit;
    }
    offset = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyUnicode_Check(fastargs[1])) {
        _PyArg_BadArgument("timezone", "argument 'name'", "str", fastargs[1]);
        goto exit;
    }
    name = fastargs[1];
skip_optional_pos:
    return_value = timezone_new_impl(type, offset, name);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time__doc__,
"time(hour=0, minute=0, second=0, microsecond=0, tzinfo=None, *, fold=0)\n"
"--\n"
"\n"
"Time with time zone.\n"
"\n"
"All arguments are optional. tzinfo may be None, or an instance of\n"
"a tzinfo subclass. The remaining arguments may be ints.");

static PyObject *
datetime_time_impl(PyTypeObject *type, int hour, int minute, int second,
                   int microsecond, PyObject *tzinfo, int fold);

static PyObject *
datetime_time(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(hour), &_Py_ID(minute), &_Py_ID(second), &_Py_ID(microsecond), &_Py_ID(tzinfo), &_Py_ID(fold), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "time",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
    PyObject *tzinfo = Py_None;
    int fold = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        hour = PyLong_AsInt(fastargs[0]);
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        minute = PyLong_AsInt(fastargs[1]);
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        second = PyLong_AsInt(fastargs[2]);
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[3]) {
        microsecond = PyLong_AsInt(fastargs[3]);
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        tzinfo = fastargs[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    fold = PyLong_AsInt(fastargs[5]);
    if (fold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = datetime_time_impl(type, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given time format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_TIME_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_time_strptime), METH_FASTCALL|METH_CLASS, datetime_time_strptime__doc__},

static PyObject *
datetime_time_strptime_impl(PyTypeObject *type, PyObject *string,
                            PyObject *format);

static PyObject *
datetime_time_strptime(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strptime", "argument 1", "str", args[0]);
        goto exit;
    }
    string = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("strptime", "argument 2", "str", args[1]);
        goto exit;
    }
    format = args[1];
    return_value = datetime_time_strptime_impl((PyTypeObject *)type, string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_isoformat__doc__,
"isoformat($self, /, timespec=\'auto\')\n"
"--\n"
"\n"
"Return the time formatted according to ISO.\n"
"\n"
"The full format is \'HH:MM:SS.mmmmmm+zz:zz\'. By default, the fractional\n"
"part is omitted if self.microsecond == 0.\n"
"\n"
"The optional argument timespec specifies the number of additional\n"
"terms of the time to include. Valid options are \'auto\', \'hours\',\n"
"\'minutes\', \'seconds\', \'milliseconds\' and \'microseconds\'.");

#define DATETIME_TIME_ISOFORMAT_METHODDEF    \
    {"isoformat", _PyCFunction_CAST(datetime_time_isoformat), METH_FASTCALL|METH_KEYWORDS, datetime_time_isoformat__doc__},

static PyObject *
datetime_time_isoformat_impl(PyDateTime_Time *self, const char *timespec);

static PyObject *
datetime_time_isoformat(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(timespec), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timespec", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isoformat",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *timespec = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("isoformat", "argument 'timespec'", "str", args[0]);
        goto exit;
    }
    Py_ssize_t timespec_length;
    timespec = PyUnicode_AsUTF8AndSize(args[0], &timespec_length);
    if (timespec == NULL) {
        goto exit;
    }
    if (strlen(timespec) != (size_t)timespec_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = datetime_time_isoformat_impl((PyDateTime_Time *)self, timespec);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_strftime__doc__,
"strftime($self, /, format)\n"
"--\n"
"\n"
"Format using strftime().\n"
"\n"
"The date part of the timestamp passed to underlying strftime should not be used.\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_TIME_STRFTIME_METHODDEF    \
    {"strftime", _PyCFunction_CAST(datetime_time_strftime), METH_FASTCALL|METH_KEYWORDS, datetime_time_strftime__doc__},

static PyObject *
datetime_time_strftime_impl(PyDateTime_Time *self, PyObject *format);

static PyObject *
datetime_time_strftime(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(format), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"format", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "strftime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *format;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strftime", "argument 'format'", "str", args[0]);
        goto exit;
    }
    format = args[0];
    return_value = datetime_time_strftime_impl((PyDateTime_Time *)self, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___format____doc__,
"__format__($self, format, /)\n"
"--\n"
"\n"
"Formats self with strftime.");

#define DATETIME_TIME___FORMAT___METHODDEF    \
    {"__format__", (PyCFunction)datetime_time___format__, METH_O, datetime_time___format____doc__},

static PyObject *
datetime_time___format___impl(PyObject *self, PyObject *format);

static PyObject *
datetime_time___format__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *format;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("__format__", "argument", "str", arg);
        goto exit;
    }
    format = arg;
    return_value = datetime_time___format___impl(self, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_replace__doc__,
"replace($self, /, hour=unchanged, minute=unchanged, second=unchanged,\n"
"        microsecond=unchanged, tzinfo=unchanged, *, fold=unchanged)\n"
"--\n"
"\n"
"Return time with new specified fields.");

#define DATETIME_TIME_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_time_replace), METH_FASTCALL|METH_KEYWORDS, datetime_time_replace__doc__},

static PyObject *
datetime_time_replace_impl(PyDateTime_Time *self, int hour, int minute,
                           int second, int microsecond, PyObject *tzinfo,
                           int fold);

static PyObject *
datetime_time_replace(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(hour), &_Py_ID(minute), &_Py_ID(second), &_Py_ID(microsecond), &_Py_ID(tzinfo), &_Py_ID(fold), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int hour = TIME_GET_HOUR(self);
    int minute = TIME_GET_MINUTE(self);
    int second = TIME_GET_SECOND(self);
    int microsecond = TIME_GET_MICROSECOND(self);
    PyObject *tzinfo = HASTZINFO(self) ? ((PyDateTime_Time *)self)->tzinfo : Py_None;
    int fold = TIME_GET_FOLD(self);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 5, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        hour = PyLong_AsInt(args[0]);
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        minute = PyLong_AsInt(args[1]);
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        second = PyLong_AsInt(args[2]);
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        microsecond = PyLong_AsInt(args[3]);
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        tzinfo = args[4];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    fold = PyLong_AsInt(args[5]);
    if (fold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = datetime_time_replace_impl((PyDateTime_Time *)self, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a time from a string in ISO 8601 format.");

#define DATETIME_TIME_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", (PyCFunction)datetime_time_fromisoformat, METH_O|METH_CLASS, datetime_time_fromisoformat__doc__},

static PyObject *
datetime_time_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyObject *
datetime_time_fromisoformat(PyObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", arg);
        goto exit;
    }
    string = arg;
    return_value = datetime_time_fromisoformat_impl((PyTypeObject *)type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___reduce_ex____doc__,
"__reduce_ex__($self, proto, /)\n"
"--\n"
"\n");

#define DATETIME_TIME___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", (PyCFunction)datetime_time___reduce_ex__, METH_O, datetime_time___reduce_ex____doc__},

static PyObject *
datetime_time___reduce_ex___impl(PyDateTime_Time *self, int proto);

static PyObject *
datetime_time___reduce_ex__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int proto;

    proto = PyLong_AsInt(arg);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_time___reduce_ex___impl((PyDateTime_Time *)self, proto);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_time___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define DATETIME_TIME___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)datetime_time___reduce__, METH_NOARGS, datetime_time___reduce____doc__},

static PyObject *
datetime_time___reduce___impl(PyDateTime_Time *self);

static PyObject *
datetime_time___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return datetime_time___reduce___impl((PyDateTime_Time *)self);
}

PyDoc_STRVAR(datetime_datetime__doc__,
"datetime(year, month, day, hour=0, minute=0, second=0, microsecond=0,\n"
"         tzinfo=None, *, fold=0)\n"
"--\n"
"\n"
"A combination of a date and a time.\n"
"\n"
"The year, month and day arguments are required. tzinfo may be None, or an\n"
"instance of a tzinfo subclass. The remaining arguments may be ints.");

static PyObject *
datetime_datetime_impl(PyTypeObject *type, int year, int month, int day,
                       int hour, int minute, int second, int microsecond,
                       PyObject *tzinfo, int fold);

static PyObject *
datetime_datetime(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(month), &_Py_ID(day), &_Py_ID(hour), &_Py_ID(minute), &_Py_ID(second), &_Py_ID(microsecond), &_Py_ID(tzinfo), &_Py_ID(fold), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", "hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "datetime",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[9];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 3;
    int year;
    int month;
    int day;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int microsecond = 0;
    PyObject *tzinfo = Py_None;
    int fold = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 3, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    year = PyLong_AsInt(fastargs[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    month = PyLong_AsInt(fastargs[1]);
    if (month == -1 && PyErr_Occurred()) {
        goto exit;
    }
    day = PyLong_AsInt(fastargs[2]);
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[3]) {
        hour = PyLong_AsInt(fastargs[3]);
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[4]) {
        minute = PyLong_AsInt(fastargs[4]);
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[5]) {
        second = PyLong_AsInt(fastargs[5]);
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[6]) {
        microsecond = PyLong_AsInt(fastargs[6]);
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[7]) {
        tzinfo = fastargs[7];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    fold = PyLong_AsInt(fastargs[8]);
    if (fold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = datetime_datetime_impl(type, year, month, day, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_now__doc__,
"now($type, /, tz=None)\n"
"--\n"
"\n"
"Returns new datetime object representing current time local to tz.\n"
"\n"
"  tz\n"
"    Timezone object.\n"
"\n"
"If no tz is specified, uses local timezone.");

#define DATETIME_DATETIME_NOW_METHODDEF    \
    {"now", _PyCFunction_CAST(datetime_datetime_now), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_datetime_now__doc__},

static PyObject *
datetime_datetime_now_impl(PyTypeObject *type, PyObject *tz);

static PyObject *
datetime_datetime_now(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(tz), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "now",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *tz = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tz = args[0];
skip_optional_pos:
    return_value = datetime_datetime_now_impl((PyTypeObject *)type, tz);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_utcnow__doc__,
"utcnow($type, /)\n"
"--\n"
"\n"
"Return a new datetime representing UTC day and time.");

#define DATETIME_DATETIME_UTCNOW_METHODDEF    \
    {"utcnow", (PyCFunction)datetime_datetime_utcnow, METH_NOARGS|METH_CLASS, datetime_datetime_utcnow__doc__},

static PyObject *
datetime_datetime_utcnow_impl(PyTypeObject *type);

static PyObject *
datetime_datetime_utcnow(PyObject *type, PyObject *Py_UNUSED(ignored))
{
    return datetime_datetime_utcnow_impl((PyTypeObject *)type);
}

PyDoc_STRVAR(datetime_datetime_fromtimestamp__doc__,
"fromtimestamp($type, /, timestamp, tz=None)\n"
"--\n"
"\n"
"Create a datetime from a POSIX timestamp.\n"
"\n"
"The timestamp is a number, e.g. created via time.time(), that is interpreted\n"
"as local time.");

#define DATETIME_DATETIME_FROMTIMESTAMP_METHODDEF    \
    {"fromtimestamp", _PyCFunction_CAST(datetime_datetime_fromtimestamp), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_datetime_fromtimestamp__doc__},

static PyObject *
datetime_datetime_fromtimestamp_impl(PyTypeObject *type, PyObject *timestamp,
                                     PyObject *tzinfo);

static PyObject *
datetime_datetime_fromtimestamp(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(timestamp), &_Py_ID(tz), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"timestamp", "tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fromtimestamp",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *timestamp;
    PyObject *tzinfo = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    timestamp = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tzinfo = args[1];
skip_optional_pos:
    return_value = datetime_datetime_fromtimestamp_impl((PyTypeObject *)type, timestamp, tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_utcfromtimestamp__doc__,
"utcfromtimestamp($type, timestamp, /)\n"
"--\n"
"\n"
"Create a naive UTC datetime from a POSIX timestamp.");

#define DATETIME_DATETIME_UTCFROMTIMESTAMP_METHODDEF    \
    {"utcfromtimestamp", (PyCFunction)datetime_datetime_utcfromtimestamp, METH_O|METH_CLASS, datetime_datetime_utcfromtimestamp__doc__},

static PyObject *
datetime_datetime_utcfromtimestamp_impl(PyTypeObject *type,
                                        PyObject *timestamp);

static PyObject *
datetime_datetime_utcfromtimestamp(PyObject *type, PyObject *timestamp)
{
    PyObject *return_value = NULL;

    return_value = datetime_datetime_utcfromtimestamp_impl((PyTypeObject *)type, timestamp);

    return return_value;
}

PyDoc_STRVAR(datetime_datetime_strptime__doc__,
"strptime($type, string, format, /)\n"
"--\n"
"\n"
"Parse string according to the given date and time format (like time.strptime()).\n"
"\n"
"For a list of supported format codes, see the documentation:\n"
"    https://docs.python.org/3/library/datetime.html#format-codes");

#define DATETIME_DATETIME_STRPTIME_METHODDEF    \
    {"strptime", _PyCFunction_CAST(datetime_datetime_strptime), METH_FASTCALL|METH_CLASS, datetime_datetime_strptime__doc__},

static PyObject *
datetime_datetime_strptime_impl(PyTypeObject *type, PyObject *string,
                                PyObject *format);

static PyObject *
datetime_datetime_strptime(PyObject *type, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *string;
    PyObject *format;

    if (!_PyArg_CheckPositional("strptime", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strptime", "argument 1", "str", args[0]);
        goto exit;
    }
    string = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("strptime", "argument 2", "str", args[1]);
        goto exit;
    }
    format = args[1];
    return_value = datetime_datetime_strptime_impl((PyTypeObject *)type, string, format);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_combine__doc__,
"combine($type, /, date, time, tzinfo=<unrepresentable>)\n"
"--\n"
"\n"
"Construct a datetime from a given date and a given time.");

#define DATETIME_DATETIME_COMBINE_METHODDEF    \
    {"combine", _PyCFunction_CAST(datetime_datetime_combine), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_datetime_combine__doc__},

static PyObject *
datetime_datetime_combine_impl(PyTypeObject *type, PyObject *date,
                               PyObject *time, PyObject *tzinfo);

static PyObject *
datetime_datetime_combine(PyObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(date), &_Py_ID(time), &_Py_ID(tzinfo), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"date", "time", "tzinfo", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "combine",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 2;
    PyObject *date;
    PyObject *time;
    PyObject *tzinfo = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 2, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyObject_TypeCheck(args[0], DATE_TYPE(NO_STATE))) {
        _PyArg_BadArgument("combine", "argument 'date'", (DATE_TYPE(NO_STATE))->tp_name, args[0]);
        goto exit;
    }
    date = args[0];
    if (!PyObject_TypeCheck(args[1], TIME_TYPE(NO_STATE))) {
        _PyArg_BadArgument("combine", "argument 'time'", (TIME_TYPE(NO_STATE))->tp_name, args[1]);
        goto exit;
    }
    time = args[1];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tzinfo = args[2];
skip_optional_pos:
    return_value = datetime_datetime_combine_impl((PyTypeObject *)type, date, time, tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_fromisoformat__doc__,
"fromisoformat($type, string, /)\n"
"--\n"
"\n"
"Construct a date from a string in ISO 8601 format.");

#define DATETIME_DATETIME_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", (PyCFunction)datetime_datetime_fromisoformat, METH_O|METH_CLASS, datetime_datetime_fromisoformat__doc__},

static PyObject *
datetime_datetime_fromisoformat_impl(PyTypeObject *type, PyObject *string);

static PyObject *
datetime_datetime_fromisoformat(PyObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *string;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("fromisoformat", "argument", "str", arg);
        goto exit;
    }
    string = arg;
    return_value = datetime_datetime_fromisoformat_impl((PyTypeObject *)type, string);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_isoformat__doc__,
"isoformat($self, /, sep=\'T\', timespec=\'auto\')\n"
"--\n"
"\n"
"Return the time formatted according to ISO.\n"
"\n"
"The full format looks like \'YYYY-MM-DD HH:MM:SS.mmmmmm\'.\n"
"By default, the fractional part is omitted if self.microsecond == 0.\n"
"\n"
"If self.tzinfo is not None, the UTC offset is also attached, giving\n"
"a full format of \'YYYY-MM-DD HH:MM:SS.mmmmmm+HH:MM\'.\n"
"\n"
"Optional argument sep specifies the separator between date and\n"
"time, default \'T\'.\n"
"\n"
"The optional argument timespec specifies the number of additional\n"
"terms of the time to include. Valid options are \'auto\', \'hours\',\n"
"\'minutes\', \'seconds\', \'milliseconds\' and \'microseconds\'.");

#define DATETIME_DATETIME_ISOFORMAT_METHODDEF    \
    {"isoformat", _PyCFunction_CAST(datetime_datetime_isoformat), METH_FASTCALL|METH_KEYWORDS, datetime_datetime_isoformat__doc__},

static PyObject *
datetime_datetime_isoformat_impl(PyDateTime_DateTime *self, int sep,
                                 const char *timespec);

static PyObject *
datetime_datetime_isoformat(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(sep), &_Py_ID(timespec), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"sep", "timespec", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "isoformat",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int sep = 'T';
    const char *timespec = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (!PyUnicode_Check(args[0])) {
            _PyArg_BadArgument("isoformat", "argument 'sep'", "a unicode character", args[0]);
            goto exit;
        }
        if (PyUnicode_GET_LENGTH(args[0]) != 1) {
            PyErr_Format(PyExc_TypeError,
                "isoformat(): argument 'sep' must be a unicode character, "
                "not a string of length %zd",
                PyUnicode_GET_LENGTH(args[0]));
            goto exit;
        }
        sep = PyUnicode_READ_CHAR(args[0], 0);
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("isoformat", "argument 'timespec'", "str", args[1]);
        goto exit;
    }
    Py_ssize_t timespec_length;
    timespec = PyUnicode_AsUTF8AndSize(args[1], &timespec_length);
    if (timespec == NULL) {
        goto exit;
    }
    if (strlen(timespec) != (size_t)timespec_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
skip_optional_pos:
    return_value = datetime_datetime_isoformat_impl((PyDateTime_DateTime *)self, sep, timespec);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_replace__doc__,
"replace($self, /, year=unchanged, month=unchanged, day=unchanged,\n"
"        hour=unchanged, minute=unchanged, second=unchanged,\n"
"        microsecond=unchanged, tzinfo=unchanged, *, fold=unchanged)\n"
"--\n"
"\n"
"Return datetime with new specified fields.");

#define DATETIME_DATETIME_REPLACE_METHODDEF    \
    {"replace", _PyCFunction_CAST(datetime_datetime_replace), METH_FASTCALL|METH_KEYWORDS, datetime_datetime_replace__doc__},

static PyObject *
datetime_datetime_replace_impl(PyDateTime_DateTime *self, int year,
                               int month, int day, int hour, int minute,
                               int second, int microsecond, PyObject *tzinfo,
                               int fold);

static PyObject *
datetime_datetime_replace(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(year), &_Py_ID(month), &_Py_ID(day), &_Py_ID(hour), &_Py_ID(minute), &_Py_ID(second), &_Py_ID(microsecond), &_Py_ID(tzinfo), &_Py_ID(fold), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"year", "month", "day", "hour", "minute", "second", "microsecond", "tzinfo", "fold", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "replace",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[9];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int year = GET_YEAR(self);
    int month = GET_MONTH(self);
    int day = GET_DAY(self);
    int hour = DATE_GET_HOUR(self);
    int minute = DATE_GET_MINUTE(self);
    int second = DATE_GET_SECOND(self);
    int microsecond = DATE_GET_MICROSECOND(self);
    PyObject *tzinfo = HASTZINFO(self) ? ((PyDateTime_DateTime *)self)->tzinfo : Py_None;
    int fold = DATE_GET_FOLD(self);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 8, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        year = PyLong_AsInt(args[0]);
        if (year == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        month = PyLong_AsInt(args[1]);
        if (month == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        day = PyLong_AsInt(args[2]);
        if (day == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        hour = PyLong_AsInt(args[3]);
        if (hour == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        minute = PyLong_AsInt(args[4]);
        if (minute == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        second = PyLong_AsInt(args[5]);
        if (second == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[6]) {
        microsecond = PyLong_AsInt(args[6]);
        if (microsecond == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[7]) {
        tzinfo = args[7];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    fold = PyLong_AsInt(args[8]);
    if (fold == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = datetime_datetime_replace_impl((PyDateTime_DateTime *)self, year, month, day, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime_astimezone__doc__,
"astimezone($self, /, tz=None)\n"
"--\n"
"\n"
"Convert to local time in new timezone tz.");

#define DATETIME_DATETIME_ASTIMEZONE_METHODDEF    \
    {"astimezone", _PyCFunction_CAST(datetime_datetime_astimezone), METH_FASTCALL|METH_KEYWORDS, datetime_datetime_astimezone__doc__},

static PyObject *
datetime_datetime_astimezone_impl(PyDateTime_DateTime *self,
                                  PyObject *tzinfo);

static PyObject *
datetime_datetime_astimezone(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(tz), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tz", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "astimezone",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *tzinfo = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tzinfo = args[0];
skip_optional_pos:
    return_value = datetime_datetime_astimezone_impl((PyDateTime_DateTime *)self, tzinfo);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime___reduce_ex____doc__,
"__reduce_ex__($self, proto, /)\n"
"--\n"
"\n");

#define DATETIME_DATETIME___REDUCE_EX___METHODDEF    \
    {"__reduce_ex__", (PyCFunction)datetime_datetime___reduce_ex__, METH_O, datetime_datetime___reduce_ex____doc__},

static PyObject *
datetime_datetime___reduce_ex___impl(PyDateTime_DateTime *self, int proto);

static PyObject *
datetime_datetime___reduce_ex__(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int proto;

    proto = PyLong_AsInt(arg);
    if (proto == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_datetime___reduce_ex___impl((PyDateTime_DateTime *)self, proto);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_datetime___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define DATETIME_DATETIME___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)datetime_datetime___reduce__, METH_NOARGS, datetime_datetime___reduce____doc__},

static PyObject *
datetime_datetime___reduce___impl(PyDateTime_DateTime *self);

static PyObject *
datetime_datetime___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return datetime_datetime___reduce___impl((PyDateTime_DateTime *)self);
}
/*[clinic end generated code: output=69658acff6a43ac4 input=a9049054013a1b77]*/
