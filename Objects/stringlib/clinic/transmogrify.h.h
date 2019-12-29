/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(stringlib_expandtabs__doc__,
"expandtabs($self, /, tabsize=8)\n"
"--\n"
"\n"
"Return a copy where all tab characters are expanded using spaces.\n"
"\n"
"If tabsize is not given, a tab size of 8 characters is assumed.");

#define STRINGLIB_EXPANDTABS_METHODDEF    \
    {"expandtabs", (PyCFunction)(void(*)(void))stringlib_expandtabs, METH_FASTCALL|METH_KEYWORDS, stringlib_expandtabs__doc__},

static PyObject *
stringlib_expandtabs_impl(PyObject *self, int tabsize);

static PyObject *
stringlib_expandtabs(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tabsize", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "expandtabs", 0};
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int tabsize = 8;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    tabsize = _PyLong_AsInt(args[0]);
    if (tabsize == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = stringlib_expandtabs_impl(self, tabsize);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_ljust__doc__,
"ljust($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a left-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_LJUST_METHODDEF    \
    {"ljust", (PyCFunction)(void(*)(void))stringlib_ljust, METH_FASTCALL, stringlib_ljust__doc__},

static PyObject *
stringlib_ljust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_ljust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_CheckPositional("ljust", nargs, 1, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[1]) && PyBytes_GET_SIZE(args[1]) == 1) {
        fillchar = PyBytes_AS_STRING(args[1])[0];
    }
    else if (PyByteArray_Check(args[1]) && PyByteArray_GET_SIZE(args[1]) == 1) {
        fillchar = PyByteArray_AS_STRING(args[1])[0];
    }
    else {
        _PyArg_BadArgument("ljust", "argument 2", "a byte string of length 1", args[1]);
        goto exit;
    }
skip_optional:
    return_value = stringlib_ljust_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_rjust__doc__,
"rjust($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a right-justified string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_RJUST_METHODDEF    \
    {"rjust", (PyCFunction)(void(*)(void))stringlib_rjust, METH_FASTCALL, stringlib_rjust__doc__},

static PyObject *
stringlib_rjust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_rjust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_CheckPositional("rjust", nargs, 1, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[1]) && PyBytes_GET_SIZE(args[1]) == 1) {
        fillchar = PyBytes_AS_STRING(args[1])[0];
    }
    else if (PyByteArray_Check(args[1]) && PyByteArray_GET_SIZE(args[1]) == 1) {
        fillchar = PyByteArray_AS_STRING(args[1])[0];
    }
    else {
        _PyArg_BadArgument("rjust", "argument 2", "a byte string of length 1", args[1]);
        goto exit;
    }
skip_optional:
    return_value = stringlib_rjust_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_center__doc__,
"center($self, width, fillchar=b\' \', /)\n"
"--\n"
"\n"
"Return a centered string of length width.\n"
"\n"
"Padding is done using the specified fill character.");

#define STRINGLIB_CENTER_METHODDEF    \
    {"center", (PyCFunction)(void(*)(void))stringlib_center, METH_FASTCALL, stringlib_center__doc__},

static PyObject *
stringlib_center_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_center(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_CheckPositional("center", nargs, 1, 2)) {
        goto exit;
    }
    if (PyFloat_Check(args[0])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (PyBytes_Check(args[1]) && PyBytes_GET_SIZE(args[1]) == 1) {
        fillchar = PyBytes_AS_STRING(args[1])[0];
    }
    else if (PyByteArray_Check(args[1]) && PyByteArray_GET_SIZE(args[1]) == 1) {
        fillchar = PyByteArray_AS_STRING(args[1])[0];
    }
    else {
        _PyArg_BadArgument("center", "argument 2", "a byte string of length 1", args[1]);
        goto exit;
    }
skip_optional:
    return_value = stringlib_center_impl(self, width, fillchar);

exit:
    return return_value;
}

PyDoc_STRVAR(stringlib_zfill__doc__,
"zfill($self, width, /)\n"
"--\n"
"\n"
"Pad a numeric string with zeros on the left, to fill a field of the given width.\n"
"\n"
"The original string is never truncated.");

#define STRINGLIB_ZFILL_METHODDEF    \
    {"zfill", (PyCFunction)stringlib_zfill, METH_O, stringlib_zfill__doc__},

static PyObject *
stringlib_zfill_impl(PyObject *self, Py_ssize_t width);

static PyObject *
stringlib_zfill(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(arg);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        width = ival;
    }
    return_value = stringlib_zfill_impl(self, width);

exit:
    return return_value;
}
/*[clinic end generated code: output=15be047aef999b4e input=a9049054013a1b77]*/
