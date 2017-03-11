/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(func_new__doc__,
"function(code, globals, name=None, argdefs=None, closure=None)\n"
"--\n"
"\n"
"Create a function object.\n"
"\n"
"  code\n"
"    a code object\n"
"  globals\n"
"    the globals dictionary\n"
"  name\n"
"    a string that overrides the name from the code object\n"
"  argdefs\n"
"    a tuple that specifies the default argument values\n"
"  closure\n"
"    a tuple that supplies the bindings for free variables");

static PyObject *
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyObject *name, PyObject *defaults, PyObject *closure);

static PyObject *
func_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"code", "globals", "name", "argdefs", "closure", NULL};
    static _PyArg_Parser _parser = {"O!O!|OOO:function", _keywords, 0};
    PyCodeObject *code;
    PyObject *globals;
    PyObject *name = Py_None;
    PyObject *defaults = Py_None;
    PyObject *closure = Py_None;

    if (!_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &_parser,
        &PyCode_Type, &code, &PyDict_Type, &globals, &name, &defaults, &closure)) {
        goto exit;
    }
    return_value = func_new_impl(type, code, globals, name, defaults, closure);

exit:
    return return_value;
}
/*[clinic end generated code: output=a6ab29e4dd33010a input=a9049054013a1b77]*/
