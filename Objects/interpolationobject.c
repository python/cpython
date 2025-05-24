/* t-string Interpolation object implementation */

#include "Python.h"
#include "pycore_initconfig.h"    // _PyStatus_OK
#include "pycore_interpolation.h"
#include "pycore_typeobject.h"    // _PyType_GetDict

static int
_conversion_converter(PyObject *arg, PyObject **conversion)
{
    if (arg == Py_None) {
        return 1;
    }

    if (!PyUnicode_Check(arg)) {
        PyErr_Format(PyExc_TypeError,
            "Interpolation() argument 'conversion' must be str, not %T",
            arg);
        return 0;
    }

    Py_ssize_t len;
    const char *conv_str = PyUnicode_AsUTF8AndSize(arg, &len);
    if (len != 1 || !(conv_str[0] == 'a' || conv_str[0] == 'r' || conv_str[0] == 's')) {
        PyErr_SetString(PyExc_ValueError,
            "Interpolation() argument 'conversion' must be one of 's', 'a' or 'r'");
        return 0;
    }

    *conversion = arg;
    return 1;
}

#include "clinic/interpolationobject.c.h"

/*[clinic input]
class Interpolation "interpolationobject *" "&_PyInterpolation_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=161c64a16f9c4544]*/

typedef struct {
    PyObject_HEAD
    PyObject *value;
    PyObject *expression;
    PyObject *conversion;
    PyObject *format_spec;
} interpolationobject;

#define interpolationobject_CAST(op) \
    (assert(_PyInterpolation_CheckExact(op)), _Py_CAST(interpolationobject*, (op)))

/*[clinic input]
@classmethod
Interpolation.__new__ as interpolation_new

    value: object
    expression: object(subclass_of='&PyUnicode_Type')
    conversion: object(converter='_conversion_converter') = None
    format_spec: object(subclass_of='&PyUnicode_Type', c_default='&_Py_STR(empty)') = ""
[clinic start generated code]*/

static PyObject *
interpolation_new_impl(PyTypeObject *type, PyObject *value,
                       PyObject *expression, PyObject *conversion,
                       PyObject *format_spec)
/*[clinic end generated code: output=6488e288765bc1a9 input=d91711024068528c]*/
{
    interpolationobject *self = PyObject_GC_New(interpolationobject, type);
    if (!self) {
        return NULL;
    }

    self->value = Py_NewRef(value);
    self->expression = Py_NewRef(expression);
    self->conversion = Py_NewRef(conversion);
    self->format_spec = Py_NewRef(format_spec);
    PyObject_GC_Track(self);
    return (PyObject *) self;
}

static void
interpolation_dealloc(PyObject *op)
{
    PyObject_GC_UnTrack(op);
    Py_TYPE(op)->tp_clear(op);
    Py_TYPE(op)->tp_free(op);
}

static int
interpolation_clear(PyObject *op)
{
    interpolationobject *self = interpolationobject_CAST(op);
    Py_CLEAR(self->value);
    Py_CLEAR(self->expression);
    Py_CLEAR(self->conversion);
    Py_CLEAR(self->format_spec);
    return 0;
}

static int
interpolation_traverse(PyObject *op, visitproc visit, void *arg)
{
    interpolationobject *self = interpolationobject_CAST(op);
    Py_VISIT(self->value);
    Py_VISIT(self->expression);
    Py_VISIT(self->conversion);
    Py_VISIT(self->format_spec);
    return 0;
}

static PyObject *
interpolation_repr(PyObject *op)
{
    interpolationobject *self = interpolationobject_CAST(op);
    return PyUnicode_FromFormat("%s(%R, %R, %R, %R)",
                                _PyType_Name(Py_TYPE(self)), self->value, self->expression,
                                self->conversion, self->format_spec);
}

static PyMemberDef interpolation_members[] = {
    {"value", Py_T_OBJECT_EX, offsetof(interpolationobject, value), Py_READONLY, "Value"},
    {"expression", Py_T_OBJECT_EX, offsetof(interpolationobject, expression), Py_READONLY, "Expression"},
    {"conversion", Py_T_OBJECT_EX, offsetof(interpolationobject, conversion), Py_READONLY, "Conversion"},
    {"format_spec", Py_T_OBJECT_EX, offsetof(interpolationobject, format_spec), Py_READONLY, "Format specifier"},
    {NULL}
};

static PyObject*
interpolation_reduce(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    interpolationobject *self = interpolationobject_CAST(op);
    return Py_BuildValue("(O(OOOO))", (PyObject *)Py_TYPE(op),
                         self->value, self->expression,
                         self->conversion, self->format_spec);
}

static PyMethodDef interpolation_methods[] = {
    {"__reduce__", interpolation_reduce, METH_NOARGS,
        PyDoc_STR("__reduce__() -> (cls, state)")},
    {"__class_getitem__", Py_GenericAlias,
        METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL, NULL},
};

PyTypeObject _PyInterpolation_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string.templatelib.Interpolation",
    .tp_doc = PyDoc_STR("Interpolation object"),
    .tp_basicsize = sizeof(interpolationobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_new = interpolation_new,
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = interpolation_dealloc,
    .tp_clear = interpolation_clear,
    .tp_free = PyObject_GC_Del,
    .tp_repr = interpolation_repr,
    .tp_members = interpolation_members,
    .tp_methods = interpolation_methods,
    .tp_traverse = interpolation_traverse,
};

PyStatus
_PyInterpolation_InitTypes(PyInterpreterState *interp)
{
    PyObject *tuple = Py_BuildValue("(ssss)", "value", "expression", "conversion", "format_spec");
    if (!tuple) {
        goto error;
    }

    PyObject *dict = _PyType_GetDict(&_PyInterpolation_Type);
    if (!dict) {
        Py_DECREF(tuple);
        goto error;
    }

    int status = PyDict_SetItemString(dict, "__match_args__", tuple);
    Py_DECREF(tuple);
    if (status < 0) {
        goto error;
    }
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("Can't initialize interpolation types");
}

PyObject *
_PyInterpolation_Build(PyObject *value, PyObject *str, int conversion, PyObject *format_spec)
{
    interpolationobject *interpolation = PyObject_GC_New(interpolationobject, &_PyInterpolation_Type);
    if (!interpolation) {
        return NULL;
    }

    interpolation->value = Py_NewRef(value);
    interpolation->expression = Py_NewRef(str);
    interpolation->format_spec = Py_NewRef(format_spec);
    interpolation->conversion = NULL;

    if (conversion == 0) {
        interpolation->conversion = Py_None;
    }
    else {
        switch (conversion) {
            case FVC_ASCII:
                interpolation->conversion = _Py_LATIN1_CHR('a');
                break;
            case FVC_REPR:
                interpolation->conversion = _Py_LATIN1_CHR('r');
                break;
            case FVC_STR:
                interpolation->conversion = _Py_LATIN1_CHR('s');
                break;
            default:
                PyErr_SetString(PyExc_SystemError,
                    "Interpolation() argument 'conversion' must be one of 's', 'a' or 'r'");
                Py_DECREF(interpolation);
                return NULL;
        }
    }

    PyObject_GC_Track(interpolation);
    return (PyObject *) interpolation;
}

PyObject *
_PyInterpolation_GetValueRef(PyObject *interpolation)
{
    return Py_NewRef(interpolationobject_CAST(interpolation)->value);
}
