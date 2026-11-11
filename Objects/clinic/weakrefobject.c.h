/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static Py_hash_t
weakref_hash_impl(PyWeakReference *self);

static Py_hash_t
weakref_hash(PyObject *self)
{
    Py_hash_t return_value = -1;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = weakref_hash_impl((PyWeakReference *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(weakref_ref_new__doc__,
"ReferenceType(object, callback=None, /)\n"
"--\n"
"\n"
"Create a weak reference to object.\n"
"\n"
"Calling the reference returns object, or None once it has been garbage\n"
"collected.  If callback is given, it is called with the reference as\n"
"its only argument when object is about to be finalized.");

static PyObject *
weakref_ref_new_impl(PyTypeObject *type, PyObject *object,
                     PyObject *callback);

static PyObject *
weakref_ref_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &_PyWeakref_RefType;
    PyObject *object;
    PyObject *callback = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("ReferenceType", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ReferenceType", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    object = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    callback = PyTuple_GET_ITEM(args, 1);
skip_optional:
    return_value = weakref_ref_new_impl(type, object, callback);

exit:
    return return_value;
}

static PyObject *
weakref_ref_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *object;
    PyObject *callback = NULL;

    assert(_PyType_CAST(type)->tp_new == weakref_ref_new);
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("ReferenceType", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ReferenceType", nargs, 1, 2)) {
        goto exit;
    }
    object = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    callback = args[1];
skip_optional:
    return_value = weakref_ref_new_impl(_PyType_CAST(type), object, callback);

exit:
    return return_value;
}

static int
weakref_ref_init_impl(PyWeakReference *self, PyObject *object,
                      PyObject *callback);

static int
weakref_ref_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    PyTypeObject *base_tp = &_PyWeakref_RefType;
    PyObject *object;
    PyObject *callback = Py_None;

    if ((Py_IS_TYPE(self, base_tp) ||
         Py_TYPE(self)->tp_new == base_tp->tp_new) &&
        !_PyArg_NoKeywords("ReferenceType", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("ReferenceType", PyTuple_GET_SIZE(args), 1, 2)) {
        goto exit;
    }
    object = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    callback = PyTuple_GET_ITEM(args, 1);
skip_optional:
    return_value = weakref_ref_init_impl((PyWeakReference *)self, object, callback);

exit:
    return return_value;
}
/*[clinic end generated code: output=774282aa1017bab5 input=a9049054013a1b77]*/
