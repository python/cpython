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
    static _PyArg_Parser _parser = {"U|O:module", _keywords, 0};
    PyObject *name;
    PyObject *doc = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &name, &doc)) {
        goto exit;
    }
    return_value = module___init___impl((PyModuleObject *)self, name, doc);

exit:
    return return_value;
}
/*[clinic end generated code: output=7b1b324bf6a590d1 input=a9049054013a1b77]*/
