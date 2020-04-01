/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(module___init____doc__,
"module(name, doc=None)\n"
"--\n"
"\n"
"Create a module object.\n"
"\n"
"The name must be a string; the optional doc argument can have any type.");

static int
module___init___impl(PyModuleObject *self, PyObject *name, PyObject *doc);

static int
module___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"name", "doc", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "module", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    PyObject *name;
    PyObject *doc = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!PyUnicode_Check(fastargs[0])) {
        _PyArg_BadArgument("module", "argument 'name'", "str", fastargs[0]);
        goto exit;
    }
    if (PyUnicode_READY(fastargs[0]) == -1) {
        goto exit;
    }
    name = fastargs[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    doc = fastargs[1];
skip_optional_pos:
    return_value = module___init___impl((PyModuleObject *)self, name, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=680276bc3a496d7a input=a9049054013a1b77]*/
