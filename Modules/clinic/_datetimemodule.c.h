/*[clinic input]
preserve
[clinic start generated code]*/

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
    {"now", _PyCFunction_CAST(datetime_datetime_now), METH_FASTCALL|METH_KEYWORDS|METH_CLASS, datetime_datetime_now__doc__},

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
/*[clinic end generated code: output=1a3da7479e443e17 input=a9049054013a1b77]*/
