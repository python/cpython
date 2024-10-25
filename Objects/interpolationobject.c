/* Interpolation object implementation */
#include "Python.h"
#include <stddef.h>

#include "pycore_initconfig.h"      // _PyStatus_OK
#include "pycore_stackref.h"        // _PyStackRef
#include "pycore_global_objects.h"  // _Py_STR
#include "pycore_runtime.h"         // _Py_STR

#include "pycore_interpolation.h"

/*[clinic input]
module templatelib
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=dbb2236a3de68808]*/

#include "clinic/interpolationobject.c.h"

/*[clinic input]
class templatelib.Interpolation "interpolationobject *" "&_PyInterpolation_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=5b183514b4d7e5af]*/

int
_conversion_converter(PyObject *arg, PyObject **Py_UNUSED(unused))
{
    if (arg == Py_None) {
        return 1;
    }

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("Interpolation", "argument 'conv'", "str", arg);
        return 0;
    }

    Py_ssize_t len;
    const char *conv = PyUnicode_AsUTF8AndSize(arg, &len);
    if (len != 1 || !(conv[0] == 'a' || conv[0] == 'r' || conv[0] == 's')) {
        PyErr_SetString(PyExc_ValueError,
            "Interpolation() argument 'conv' must be one of 's', 'a' or 'r'");
        return 0;
    }

    return 1;
}

typedef struct {
    PyObject_HEAD
    PyObject *value;
    PyObject *expr;
    PyObject *conv;
    PyObject *format_spec;
} interpolationobject;

/*[clinic input]
@classmethod
templatelib.Interpolation.__new__ as interpolation_new

    value: object
    expr: object(subclass_of='&PyUnicode_Type')
    conv: object(converter='_conversion_converter') = None
    format_spec: object(subclass_of='&PyUnicode_Type', c_default='&_Py_STR(empty)') = ""
[clinic start generated code]*/

static PyObject *
interpolation_new_impl(PyTypeObject *type, PyObject *value, PyObject *expr,
                       PyObject *conv, PyObject *format_spec)
/*[clinic end generated code: output=417d59bccab99648 input=348d81ee06c4be20]*/
{
    interpolationobject *self = (interpolationobject *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    Py_XSETREF(self->value, Py_NewRef(value));
    Py_XSETREF(self->expr, Py_NewRef(expr));
    Py_XSETREF(self->conv, Py_NewRef(conv));
    Py_XSETREF(self->format_spec, Py_NewRef(format_spec));
    return (PyObject *) self;
}

static void
interpolation_dealloc(interpolationobject *self)
{
    Py_CLEAR(self->value);
    Py_CLEAR(self->expr);
    Py_CLEAR(self->conv);
    Py_CLEAR(self->format_spec);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
interpolation_repr(interpolationobject *self)
{
    return PyUnicode_FromFormat("%s(%R, %R, %R, %R)",
                                _PyType_Name(Py_TYPE(self)),
                                self->value, self->expr,
                                self->conv, self->format_spec);
}

static PyObject *
interpolation_compare(interpolationobject *self, PyObject *other, int op)
{
    if (op == Py_LT || op == Py_LE || op == Py_GT || op == Py_GE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!PyObject_TypeCheck(other, &_PyInterpolation_Type)) {
        return (op == Py_EQ) ? Py_False : Py_True;
    }

    interpolationobject *other_i = (interpolationobject *) other;

    int valueeq = PyObject_RichCompareBool(self->value, other_i->value, Py_EQ);
    if (valueeq == -1) {
        return NULL;
    }
    int expreq = PyUnicode_Compare(self->expr, other_i->expr);
    if (expreq == -1 && PyErr_Occurred()) {
        return NULL;
    }
    int conveq = PyObject_RichCompareBool(self->conv, other_i->conv, Py_EQ); // conv might be Py_None
    if (conveq == -1) {
        return NULL;
    }
    int formatspeceq = PyUnicode_Compare(self->format_spec, other_i->format_spec);
    if (formatspeceq == -1 && PyErr_Occurred()) {
        return NULL;
    }

    int eq = valueeq && expreq == 0 && conveq && formatspeceq == 0;
    return PyBool_FromLong(op == Py_EQ ? eq : !eq);
}

static Py_hash_t
interpolation_hash(interpolationobject *self)
{
    PyObject *tuple = PyTuple_Pack(4, self->value, self->expr, self->conv, self->format_spec);
    if (!tuple) {
        return -1;
    }

    Py_hash_t hash = PyObject_Hash(tuple);
    Py_DECREF(tuple);
    return hash;
}

static PyMemberDef interpolation_members[] = {
    {"value", Py_T_OBJECT_EX, offsetof(interpolationobject, value), Py_READONLY, "Value"},
    {"expr", Py_T_OBJECT_EX, offsetof(interpolationobject, expr), Py_READONLY, "Expr"},
    {"conv", Py_T_OBJECT_EX, offsetof(interpolationobject, conv), Py_READONLY, "Conversion"},
    {"format_spec", Py_T_OBJECT_EX, offsetof(interpolationobject, format_spec), Py_READONLY, "Format specifier"},
    {NULL}
};

PyTypeObject _PyInterpolation_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "templatelib.Interpolation",
    .tp_doc = PyDoc_STR("Interpolation object"),
    .tp_basicsize = sizeof(interpolationobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | _Py_TPFLAGS_MATCH_SELF,
    .tp_new = (newfunc) interpolation_new,
    .tp_dealloc = (destructor) interpolation_dealloc,
    .tp_repr = (reprfunc) interpolation_repr,
    .tp_richcompare = (richcmpfunc) interpolation_compare,
    .tp_hash = (hashfunc) interpolation_hash,
    .tp_members = interpolation_members,
};

static PyObject *
_get_match_args(void)
{
    PyObject *value = NULL, *expr = NULL, *conv = NULL, *format_spec = NULL;
    PyObject *tuple = NULL;

    value = PyUnicode_FromString("value");
    if (!value) {
        goto error;
    }
    expr = PyUnicode_FromString("expr");
    if (!expr) {
        goto error;
    }
    conv = PyUnicode_FromString("conv");
    if (!conv) {
        goto error;
    }
    format_spec = PyUnicode_FromString("format_spec");
    if (!format_spec) {
        goto error;
    }

    tuple = PyTuple_Pack(4, value, expr, conv, format_spec);

error:
    Py_XDECREF(value);
    Py_XDECREF(expr);
    Py_XDECREF(conv);
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
_PyInterpolation_FromStackRefSteal(_PyStackRef *values)
{
    PyObject *args = PyTuple_New(4);
    if (!args) {
        goto error;
    }

    PyTuple_SET_ITEM(args, 0, PyStackRef_AsPyObjectSteal(values[0]));
    PyTuple_SET_ITEM(args, 1, PyStackRef_AsPyObjectSteal(values[1]));

    PyObject *conv = PyStackRef_AsPyObjectSteal(values[2]);
    PyTuple_SET_ITEM(args, 2, conv ? conv : Py_NewRef(Py_None));

    PyObject *format_spec = PyStackRef_AsPyObjectSteal(values[3]);
    PyTuple_SET_ITEM(args, 3, format_spec ? format_spec : &_Py_STR(empty));

    PyObject *interpolation = PyObject_CallObject((PyObject *) &_PyInterpolation_Type, args);
    if (!interpolation) {
        Py_DECREF(args);
        goto error;
    }
    return interpolation;

error:
    PyStackRef_CLOSE(values[0]);
    PyStackRef_CLOSE(values[1]);
    PyStackRef_XCLOSE(values[2]);
    PyStackRef_XCLOSE(values[3]);
    return NULL;
}
