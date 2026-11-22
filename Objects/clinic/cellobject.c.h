/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(cell_new__doc__,
"cell([contents])\n"
"--\n"
"\n"
"Create a new cell object.\n"
"\n"
"  contents\n"
"    the contents of the cell. If not specified, the cell will be empty,\n"
"    and further attempts to access its cell_contents attribute will\n"
"    raise a ValueError.");

static PyObject *
cell_new_impl(PyTypeObject *type, PyObject *contents);

static PyObject *
cell_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyCell_Type;
    PyObject *contents = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("cell", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("cell", PyTuple_GET_SIZE(args), 0, 1)) {
        goto exit;
    }
    if (PyTuple_GET_SIZE(args) < 1) {
        goto skip_optional;
    }
    contents = PyTuple_GET_ITEM(args, 0);
skip_optional:
    return_value = cell_new_impl(type, contents);

exit:
    return return_value;
}

static PyObject *
cell_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *contents = NULL;

    assert(_PyType_CAST(type)->tp_new == cell_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("cell", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("cell", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    contents = args[0];
skip_optional:
    return_value = cell_new_impl(_PyType_CAST(type), contents);

exit:
    return return_value;
}
/*[clinic end generated code: output=687470647f1a75f8 input=a9049054013a1b77]*/
