/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(picklebuf_new__doc__,
"PickleBuffer(buffer, /)\n"
"--\n"
"\n"
"Wrapper for potentially out-of-band buffers.");

static PyObject *
picklebuf_new_impl(PyTypeObject *type, PyObject *base);

static PyObject *
picklebuf_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PyPickleBuffer_Type;
    PyObject *base;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("PickleBuffer", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("PickleBuffer", PyTuple_GET_SIZE(args), 1, 1)) {
        goto exit;
    }
    base = PyTuple_GET_ITEM(args, 0);
    return_value = picklebuf_new_impl(type, base);

exit:
    return return_value;
}

static PyObject *
picklebuf_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *base;

    assert(_PyType_CAST(type)->tp_new == picklebuf_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("PickleBuffer", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("PickleBuffer", nargs, 1, 1)) {
        goto exit;
    }
    base = args[0];
    return_value = picklebuf_new_impl(_PyType_CAST(type), base);

exit:
    return return_value;
}
/*[clinic end generated code: output=4c32a9c4ea5b9bfe input=a9049054013a1b77]*/
