/* t-string Interpolation object implementation */

#include "Python.h"
#include "pycore_initconfig.h"      // _PyStatus_OK
#include "pycore_interpolation.h"

static int
_conversion_converter(PyObject *arg, PyObject **conversion)
{
    if (arg == Py_None) {
        return 1;
    }

    if (!PyUnicode_Check(arg)) {
        PyErr_Format(PyExc_TypeError,
            "%.200s() %.200s must be %.50s, not %.50s",
            "Interpolation", "argument 'conversion'", "str",
            arg == Py_None ? "None" : Py_TYPE(arg)->tp_name);
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
    interpolationobject *self = (interpolationobject *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    Py_XSETREF(self->value, Py_NewRef(value));
    Py_XSETREF(self->expression, Py_NewRef(expression));
    Py_XSETREF(self->conversion, Py_NewRef(conversion));
    Py_XSETREF(self->format_spec, Py_NewRef(format_spec));
    return (PyObject *) self;
}

static void
interpolation_dealloc(interpolationobject *self)
{
    PyObject_GC_UnTrack(self);
    Py_CLEAR(self->value);
    Py_CLEAR(self->expression);
    Py_CLEAR(self->conversion);
    Py_CLEAR(self->format_spec);
    Py_TYPE(self)->tp_free(self);
}

static int
interpolation_traverse(interpolationobject *self, visitproc visit, void *arg)
{
    Py_VISIT(self->value);
    Py_VISIT(self->expression);
    Py_VISIT(self->conversion);
    Py_VISIT(self->format_spec);
    return 0;
}

static PyObject *
interpolation_repr(interpolationobject *self)
{
    return PyUnicode_FromFormat("%s(%R, %R, %R, %R)",
                                _PyType_Name(Py_TYPE(self)),
                                self->value, self->expression,
                                self->conversion, self->format_spec);
}

static PyMemberDef interpolation_members[] = {
    {"value", Py_T_OBJECT_EX, offsetof(interpolationobject, value), Py_READONLY, "Value"},
    {"expression", Py_T_OBJECT_EX, offsetof(interpolationobject, expression), Py_READONLY, "Expression"},
    {"conversion", Py_T_OBJECT_EX, offsetof(interpolationobject, conversion), Py_READONLY, "Conversion"},
    {"format_spec", Py_T_OBJECT_EX, offsetof(interpolationobject, format_spec), Py_READONLY, "Format specifier"},
    {NULL}
};

PyTypeObject _PyInterpolation_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "string.templatelib.Interpolation",
    .tp_doc = PyDoc_STR("Interpolation object"),
    .tp_basicsize = sizeof(interpolationobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | _Py_TPFLAGS_MATCH_SELF,
    .tp_new = (newfunc) interpolation_new,
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = (destructor) interpolation_dealloc,
    .tp_free = PyObject_GC_Del,
    .tp_repr = (reprfunc) interpolation_repr,
    .tp_members = interpolation_members,
    .tp_traverse = (traverseproc) interpolation_traverse,
};

static PyObject *
_get_match_args(void)
{
    PyObject *value = NULL, *expression = NULL, *conversion = NULL, *format_spec = NULL;
    PyObject *tuple = NULL;

    value = PyUnicode_FromString("value");
    if (!value) {
        goto error;
    }
    expression = PyUnicode_FromString("expression");
    if (!expression) {
        goto error;
    }
    conversion = PyUnicode_FromString("conversion");
    if (!conversion) {
        goto error;
    }
    format_spec = PyUnicode_FromString("format_spec");
    if (!format_spec) {
        goto error;
    }

    tuple = PyTuple_Pack(4, value, expression, conversion, format_spec);

error:
    Py_XDECREF(value);
    Py_XDECREF(expression);
    Py_XDECREF(conversion);
    Py_XDECREF(format_spec);
    return tuple;

}

PyStatus
_PyInterpolation_InitTypes(PyInterpreterState *interp)
{
    PyObject *tuple = _get_match_args();
    if (!tuple) {
        goto error;
    }

    int status = PyDict_SetItemString(_PyType_GetDict(&_PyInterpolation_Type),
        "__match_args__",
        tuple);
    Py_DECREF(tuple);

    if (status < 0) {
        goto error;
    }
    return _PyStatus_OK();

error:
    return _PyStatus_ERR("Can't initialize interpolation types");
}

PyObject *
_PyInterpolation_FromStackRefStealOnSuccess(_PyStackRef *values)
{
    interpolationobject *interpolation = (interpolationobject *) _PyInterpolation_Type.tp_alloc(&_PyInterpolation_Type, 0);
    if (!interpolation) {
        return NULL;
    }

    interpolation->value = PyStackRef_AsPyObjectSteal(values[0]);
    interpolation->expression = PyStackRef_AsPyObjectSteal(values[1]);

    if (PyStackRef_IsNull(values[2])) {
        interpolation->conversion = Py_NewRef(Py_None);
    } else {
        interpolation->conversion = PyStackRef_AsPyObjectSteal(values[2]);
    }

    if (PyStackRef_IsNull(values[3])) {
        interpolation->format_spec = Py_NewRef(&_Py_STR(empty));
    } else {
        interpolation->format_spec = PyStackRef_AsPyObjectSteal(values[3]);
    }

    return (PyObject *) interpolation;
}

PyObject *
_PyInterpolation_GetValue(PyObject *interpolation)
{
    return ((interpolationobject *) interpolation)->value;
}
