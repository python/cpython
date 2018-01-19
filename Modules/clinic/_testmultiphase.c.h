/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testmultiphase_Example_get_defining_module__doc__,
"get_defining_module($self, /)\n"
"--\n"
"\n"
"This method returns module of the defining class.");

#define _TESTMULTIPHASE_EXAMPLE_GET_DEFINING_MODULE_METHODDEF    \
    {"get_defining_module", (PyCFunction)_testmultiphase_Example_get_defining_module, METH_METHOD|METH_VARARGS|METH_KEYWORDS, _testmultiphase_Example_get_defining_module__doc__},

static PyObject *
_testmultiphase_Example_get_defining_module_impl(ExampleObject *self,
                                                 PyTypeObject *cls);

static PyObject *
_testmultiphase_Example_get_defining_module(ExampleObject *self, PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;

    return_value = _testmultiphase_Example_get_defining_module_impl(self, cls);

    return return_value;
}
/*[clinic end generated code: output=697cb81b3a5b31a2 input=a9049054013a1b77]*/
