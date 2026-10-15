/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static PyObject *
weakref_ref_new_impl(PyTypeObject *type, PyObject *ob, PyObject *callback);

static PyObject *
weakref_ref_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &_PyWeakref_RefType;
    PyObject *ob;
    PyObject *callback = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("ref", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ref", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    ob = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    callback = PyTuple_GET_ITEM(args, 1);
skip_optional:
    return_value = weakref_ref_new_impl(type, ob, callback);

exit:
    return return_value;
}

static PyObject *
weakref_ref_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *ob;
    PyObject *callback = NULL;

    assert(Py_Is(_PyType_CAST(type), &_PyWeakref_RefType));
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("ref", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ref", nargs, 1, 2)) {
        goto exit;
    }
    ob = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    callback = args[1];
skip_optional:
    return_value = weakref_ref_new_impl(_PyType_CAST(type), ob, callback);

exit:
    return return_value;
}
/*[clinic end generated code: output=e376f863eef4ceb7 input=a9049054013a1b77]*/
