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
    static _PyArg_Parser _parser = {"|O:now", _keywords, 0};
    PyObject *tz = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &tz)) {
        goto exit;
    }
    return_value = datetime_datetime_now_impl(type, tz);

exit:
    return return_value;
}
/*[clinic end generated code: output=b3d746843403a8ce input=a9049054013a1b77]*/
