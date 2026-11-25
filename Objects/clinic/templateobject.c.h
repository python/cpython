/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_tuple.h"         // _PyTuple_ITEMS()

static PyObject *
template_new_impl(PyTypeObject *type, PyObject * const *args,
                  Py_ssize_t args_length);

static PyObject *
template_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &_PyTemplate_Type;
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("Template", kwargs)) {
        goto exit;
    }
    __clinic_args = _PyTuple_ITEMS(args);
    args_length = PyTuple_GET_SIZE(args);
    return_value = template_new_impl(type, __clinic_args, args_length);

exit:
    return return_value;
}

static PyObject *
template_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject * const *__clinic_args;
    Py_ssize_t args_length;

    assert(_PyType_CAST(type)->tp_new == template_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("Template", kwnames)) {
        goto exit;
    }
    __clinic_args = args;
    args_length = nargs;
    return_value = template_new_impl(_PyType_CAST(type), __clinic_args, args_length);

exit:
    return return_value;
}
/*[clinic end generated code: output=38d45f3b38a21517 input=a9049054013a1b77]*/
