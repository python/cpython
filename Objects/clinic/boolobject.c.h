/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(bool_new__doc__,
"bool(object=False, /)\n"
"--\n"
"\n"
"Returns True when the argument is true, False otherwise.\n"
"\n"
"The builtins True and False are the only two instances of the class\n"
"bool.  The class bool is a subclass of the class int, and cannot be\n"
"subclassed.");

static PyObject *
bool_new_impl(PyTypeObject *type, PyObject *x);

static PyObject *
bool_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyBool_Type;
    PyObject *x = Py_False;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("bool", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("bool", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    x = PyTuple_GET_ITEM(args, 0);
skip_optional:
    return_value = bool_new_impl(type, x);

exit:
    return return_value;
}

static PyObject *
bool_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *x = Py_False;

    assert(_PyType_CAST(type)->tp_new == bool_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("bool", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("bool", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    x = args[0];
skip_optional:
    return_value = bool_new_impl(_PyType_CAST(type), x);

exit:
    return return_value;
}
/*[clinic end generated code: output=c7be32f9d1f3e07a input=a9049054013a1b77]*/
