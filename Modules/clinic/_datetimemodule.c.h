/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

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
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 3, 3, 0, argsbuf);
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
datetime_date_replace(PyDateTime_Date *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 3, 0, argsbuf);
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
    return_value = datetime_date_replace_impl(self, year, month, day);

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
datetime_time_replace(PyDateTime_Time *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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
    PyObject *tzinfo = HASTZINFO(self) ? self->tzinfo : Py_None;
    int fold = TIME_GET_FOLD(self);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 5, 0, argsbuf);
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
    return_value = datetime_time_replace_impl(self, hour, minute, second, microsecond, tzinfo, fold);

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
datetime_datetime_now(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    tz = args[0];
skip_optional_pos:
    return_value = datetime_datetime_now_impl(type, tz);

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
datetime_datetime_replace(PyDateTime_DateTime *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 9
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
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
    PyObject *tzinfo = HASTZINFO(self) ? self->tzinfo : Py_None;
    int fold = DATE_GET_FOLD(self);

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 8, 0, argsbuf);
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
    return_value = datetime_datetime_replace_impl(self, year, month, day, hour, minute, second, microsecond, tzinfo, fold);

exit:
    return return_value;
}
/*[clinic end generated code: output=c7a04b865b1e0890 input=a9049054013a1b77]*/
