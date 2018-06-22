/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(warnings_warn__doc__,
"warn($module, /, message, category=None, stacklevel=1, source=None)\n"
"--\n"
"\n"
"Issue a warning, or maybe ignore it or raise an exception.");

#define WARNINGS_WARN_METHODDEF    \
    {"warn", (PyCFunction)warnings_warn, METH_FASTCALL|METH_KEYWORDS, warnings_warn__doc__},

static PyObject *
warnings_warn_impl(PyObject *module, PyObject *message, PyObject *category,
                   Py_ssize_t stacklevel, PyObject *source);

static PyObject *
warnings_warn(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"message", "category", "stacklevel", "source", NULL};
    static _PyArg_Parser _parser = {"O|OnO:warn", _keywords, 0};
    PyObject *message;
    PyObject *category = Py_None;
    Py_ssize_t stacklevel = 1;
    PyObject *source = Py_None;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &message, &category, &stacklevel, &source)) {
        goto exit;
    }
    return_value = warnings_warn_impl(module, message, category, stacklevel, source);

exit:
    return return_value;
}
/*[clinic end generated code: output=86369ece63001d78 input=a9049054013a1b77]*/
