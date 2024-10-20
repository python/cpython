/* Interpolation object implementation */
#include "Python.h"
#include <stddef.h>

#include "pycore_initconfig.h"      // _PyStatus_OK
#include "pycore_typeobject.h"      // _PyStaticType_InitBuiltin
#include "pycore_stackref.h"        // _PyStackRef
#include "pycore_global_objects.h"  // _Py_STR
#include "pycore_runtime.h"         // _Py_STR
#include "pycore_object.h"          // _PyObject_GC_TRACK

#include "pycore_interpolation.h"

typedef struct {
    PyObject_HEAD
    PyObject *value;
    PyObject *expr;
    PyObject *conv;
    PyObject *format_spec;
} interpolationobject;

static interpolationobject *
interpolation_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    interpolationobject *self = (interpolationobject *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    static char *kwlist[] = {"value", "expr", "conv", "format_spec", NULL};

    PyObject *value, *expr;
    PyObject *conv = NULL;
    PyObject *format_spec = NULL;

    if (PyArg_ParseTupleAndKeywords(args, kwds, "OO|OO", kwlist,
                                    &value, &expr,
                                    &conv, &format_spec) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    Py_XSETREF(self->value, Py_NewRef(value));
    Py_XSETREF(self->expr, Py_NewRef(expr));
    Py_XSETREF(self->conv, Py_NewRef(conv ? conv : Py_None));
    Py_XSETREF(self->format_spec, format_spec ? Py_NewRef(format_spec) : &_Py_STR(empty));
    return self;
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
    .tp_members = interpolation_members,
};

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
