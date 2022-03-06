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
    {"expandtabs", (PyCFunction)stringlib_expandtabs, METH_FASTCALL|METH_KEYWORDS, stringlib_expandtabs__doc__},

static PyObject *
stringlib_expandtabs_impl(PyObject *self, int tabsize);

static PyObject *
stringlib_expandtabs(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"tabsize", NULL};
    static _PyArg_Parser _parser = {"|i:expandtabs", _keywords, 0};
    int tabsize = 8;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &tabsize)) {
        goto exit;
    }
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
    {"ljust", (PyCFunction)stringlib_ljust, METH_FASTCALL, stringlib_ljust__doc__},

static PyObject *
stringlib_ljust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_ljust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_ParseStack(args, nargs, "n|c:ljust",
        &width, &fillchar)) {
        goto exit;
    }
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
    {"rjust", (PyCFunction)stringlib_rjust, METH_FASTCALL, stringlib_rjust__doc__},

static PyObject *
stringlib_rjust_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_rjust(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_ParseStack(args, nargs, "n|c:rjust",
        &width, &fillchar)) {
        goto exit;
    }
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
    {"center", (PyCFunction)stringlib_center, METH_FASTCALL, stringlib_center__doc__},

static PyObject *
stringlib_center_impl(PyObject *self, Py_ssize_t width, char fillchar);

static PyObject *
stringlib_center(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t width;
    char fillchar = ' ';

    if (!_PyArg_ParseStack(args, nargs, "n|c:center",
        &width, &fillchar)) {
        goto exit;
    }
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

    if (!PyArg_Parse(arg, "n:zfill", &width)) {
        goto exit;
    }
    return_value = stringlib_zfill_impl(self, width);

exit:
    return return_value;
}
/*[clinic end generated code: output=336620159a1fc70d input=a9049054013a1b77]*/
