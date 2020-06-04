/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(datetime_date_today__doc__,
"today($type, /)\n"
"--\n"
"\n"
"Return new date from current time.\n"
"\n"
"Same as self.__class__.fromtimestamp(time.time())");

#define DATETIME_DATE_TODAY_METHODDEF    \
    {"today", (PyCFunction)datetime_date_today, METH_NOARGS|METH_CLASS, datetime_date_today__doc__},

static PyObject *
datetime_date_today_impl(PyTypeObject *type);

static PyObject *
datetime_date_today(PyTypeObject *type, PyObject *Py_UNUSED(ignored))
{
    return datetime_date_today_impl(type);
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

PyDoc_STRVAR(datetime_date_fromordinal__doc__,
"fromordinal($type, ordinal, /)\n"
"--\n"
"\n"
"int -> date corresponding to a proleptic Gregorian ordinal.\n"
"\n"
"Raises ValueError if the ordinal is out of range.");

#define DATETIME_DATE_FROMORDINAL_METHODDEF    \
    {"fromordinal", (PyCFunction)datetime_date_fromordinal, METH_O|METH_CLASS, datetime_date_fromordinal__doc__},

static PyObject *
datetime_date_fromordinal_impl(PyTypeObject *type, int ordinal);

static PyObject *
datetime_date_fromordinal(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    int ordinal;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    ordinal = _PyLong_AsInt(arg);
    if (ordinal == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromordinal_impl(type, ordinal);

exit:
    return return_value;
}

PyDoc_STRVAR(datetime_date_fromisoformat__doc__,
"fromisoformat($type, date_string, /)\n"
"--\n"
"\n"
"str -> Construct a date from the output of date.isoformat()");

#define DATETIME_DATE_FROMISOFORMAT_METHODDEF    \
    {"fromisoformat", (PyCFunction)datetime_date_fromisoformat, METH_O|METH_CLASS, datetime_date_fromisoformat__doc__},

PyDoc_STRVAR(datetime_date_fromisocalendar__doc__,
"fromisocalendar($type, /, year, week, day)\n"
"--\n"
"\n"
"int, int, int -> Construct a date from the ISO year, week number and weekday.\n"
"\n"
"This is the inverse of the date.isocalendar() function");

#define DATETIME_DATE_FROMISOCALENDAR_METHODDEF    \
    {"fromisocalendar", (PyCFunction)(void(*)(void))datetime_date_fromisocalendar, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_date_fromisocalendar__doc__},

static PyObject *
datetime_date_fromisocalendar_impl(PyTypeObject *type, int year, int week,
                                   int day);

static PyObject *
datetime_date_fromisocalendar(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"year", "week", "day", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "fromisocalendar", 0};
    PyObject *argsbuf[3];
    int year;
    int week;
    int day;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    year = _PyLong_AsInt(args[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    week = _PyLong_AsInt(args[1]);
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (PyFloat_Check(args[2])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    day = _PyLong_AsInt(args[2]);
    if (day == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = datetime_date_fromisocalendar_impl(type, year, week, day);

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
    static const char * const _keywords[] = {"year", "week", "weekday", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "IsoCalendarDate", 0};
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
    year = _PyLong_AsInt(fastargs[0]);
    if (year == -1 && PyErr_Occurred()) {
        goto exit;
    }
    week = _PyLong_AsInt(fastargs[1]);
    if (week == -1 && PyErr_Occurred()) {
        goto exit;
    }
    weekday = _PyLong_AsInt(fastargs[2]);
    if (weekday == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = iso_calendar_date_new_impl(type, year, week, weekday);

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
    {"now", (PyCFunction)(void(*)(void))datetime_datetime_now, METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_datetime_now__doc__},

static PyObject *
datetime_datetime_now_impl(PyTypeObject *type, PyObject *tz);

static PyObject *
datetime_datetime_now(PyTypeObject *type, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tz", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "now", 0};
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
/*[clinic end generated code: output=f61310936e3d8091 input=a9049054013a1b77]*/
