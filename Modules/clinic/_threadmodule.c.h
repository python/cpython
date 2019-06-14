/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_thread__is_main_interpreter__doc__,
"_is_main_interpreter($module, /)\n"
"--\n"
"\n"
"Return True if the current interpreter is the main Python interpreter.");

#define _THREAD__IS_MAIN_INTERPRETER_METHODDEF    \
    {"_is_main_interpreter", (PyCFunction)_thread__is_main_interpreter, METH_NOARGS, _thread__is_main_interpreter__doc__},

static PyObject *
_thread__is_main_interpreter_impl(PyObject *module);

static PyObject *
_thread__is_main_interpreter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__is_main_interpreter_impl(module);
}
/*[clinic end generated code: output=505840d1b9101789 input=a9049054013a1b77]*/
