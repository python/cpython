/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(warnings_warn__doc__,
"warn($module, /, message, category=None, stacklevel=1, source=None)\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.");

#define WARNINGS_WARN_METHODDEF    \
    {"warn", (PyCFunction)(void(*)(void))warnings_warn, METH_FASTCALL|METH_KEYWORDS, warnings_warn__doc__},

static PyObject *
warnings_warn_impl(PyObject *module, PyObject *message, PyObject *category,
                   Py_ssize_t stacklevel, PyObject *source);

static PyObject *
warnings_warn(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"message", "category", "stacklevel", "source", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "warn", 0};
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *message;
    PyObject *category = Py_None;
    Py_ssize_t stacklevel = 1;
    PyObject *source = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 4, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    message = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        category = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            stacklevel = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    source = args[3];
skip_optional_pos:
    return_value = warnings_warn_impl(module, message, category, stacklevel, source);

exit:
    return return_value;
}
/*[clinic end generated code: output=eb9997fa998fdbad input=a9049054013a1b77]*/
