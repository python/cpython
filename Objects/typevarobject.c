// TypeVar, TypeVarTuple, ParamSpec, and TypeAlias
#include "Python.h"
#include "pycore_interpframe.h"   // _PyInterpreterFrame
#include "pycore_object.h"        // _PyObject_GC_TRACK/UNTRACK, PyAnnotateFormat
#include "pycore_typevarobject.h"
#include "pycore_unicodeobject.h" // _PyUnicode_EqualToASCIIString()
#include "pycore_unionobject.h"   // _Py_union_type_or, _Py_union_from_tuple
#include "structmember.h"

/*[clinic input]
class typevar "typevarobject *" "&_PyTypeVar_Type"
class paramspec "paramspecobject *" "&_PyParamSpec_Type"
class paramspecargs "paramspecattrobject *" "&_PyParamSpecArgs_Type"
class paramspeckwargs "paramspecattrobject *" "&_PyParamSpecKwargs_Type"
class typevartuple "typevartupleobject *" "&_PyTypeVarTuple_Type"
class typealias "typealiasobject *" "&_PyTypeAlias_Type"
class Generic "PyObject *" "&PyGeneric_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=aa86741931a0f55c]*/

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *bound;
    PyObject *evaluate_bound;
    PyObject *constraints;
    PyObject *evaluate_constraints;
    PyObject *default_value;
    PyObject *evaluate_default;
    bool covariant;
    bool contravariant;
    bool infer_variance;
} typevarobject;

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *default_value;
    PyObject *evaluate_default;
} typevartupleobject;

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *bound;
    PyObject *default_value;
    PyObject *evaluate_default;
    bool covariant;
    bool contravariant;
    bool infer_variance;
} paramspecobject;

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *type_params;
    PyObject *compute_value;
    PyObject *value;
    PyObject *module;
} typealiasobject;

#define typevarobject_CAST(op)      ((typevarobject *)(op))
#define typevartupleobject_CAST(op) ((typevartupleobject *)(op))
#define paramspecobject_CAST(op)    ((paramspecobject *)(op))
#define typealiasobject_CAST(op)    ((typealiasobject *)(op))

#include "clinic/typevarobject.c.h"

/* NoDefault is a marker object to indicate that a parameter has no default. */

static PyObject *
NoDefault_repr(PyObject *op)
{
    return PyUnicode_FromString("typing.NoDefault");
}

static PyObject *
NoDefault_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("NoDefault");
}

static PyMethodDef nodefault_methods[] = {
    {"__reduce__", NoDefault_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyObject *
nodefault_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args) || (kwargs && PyDict_GET_SIZE(kwargs))) {
        PyErr_SetString(PyExc_TypeError, "NoDefaultType takes no arguments");
        return NULL;
    }
    return &_Py_NoDefaultStruct;
}

static void
nodefault_dealloc(PyObject *nodefault)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref NoDefault out of existence. Instead,
     * since NoDefault is an immortal object, re-set the reference count.
     */
    _Py_SetImmortal(nodefault);
}

PyDoc_STRVAR(nodefault_doc,
"NoDefaultType()\n"
"--\n\n"
"The type of the NoDefault singleton.");

PyTypeObject _PyNoDefault_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "NoDefaultType",
    .tp_dealloc = nodefault_dealloc,
    .tp_repr = NoDefault_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = nodefault_doc,
    .tp_methods = nodefault_methods,
    .tp_new = nodefault_new,
};

PyObject _Py_NoDefaultStruct = _PyObject_HEAD_INIT(&_PyNoDefault_Type);

typedef struct {
    PyObject_HEAD
    PyObject *value;
} constevaluatorobject;

#define constevaluatorobject_CAST(op)   ((constevaluatorobject *)(op))

static void
constevaluator_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    constevaluatorobject *ce = constevaluatorobject_CAST(self);

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(ce->value);

    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
constevaluator_traverse(PyObject *self, visitproc visit, void *arg)
{
    constevaluatorobject *ce = constevaluatorobject_CAST(self);
    Py_VISIT(ce->value);
    return 0;
}

static int
constevaluator_clear(PyObject *self)
{
    constevaluatorobject *ce = constevaluatorobject_CAST(self);
    Py_CLEAR(ce->value);
    return 0;
}

static PyObject *
constevaluator_repr(PyObject *self)
{
    constevaluatorobject *ce = constevaluatorobject_CAST(self);
    return PyUnicode_FromFormat("<constevaluator %R>", ce->value);
}

static PyObject *
constevaluator_call(PyObject *self, PyObject *args, PyObject *kwargs)
{
    constevaluatorobject *ce = constevaluatorobject_CAST(self);
    if (!_PyArg_NoKeywords("constevaluator.__call__", kwargs)) {
        return NULL;
    }
    int format;
    if (!PyArg_ParseTuple(args, "i:constevaluator.__call__", &format)) {
        return NULL;
    }
    PyObject *value = ce->value;
    if (format == _Py_ANNOTATE_FORMAT_STRING) {
        PyUnicodeWriter *writer = PyUnicodeWriter_Create(5);  // cannot be <5
        if (writer == NULL) {
            return NULL;
        }
        if (PyTuple_Check(value)) {
            if (PyUnicodeWriter_WriteChar(writer, '(') < 0) {
                PyUnicodeWriter_Discard(writer);
                return NULL;
            }
            for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(value); i++) {
                PyObject *item = PyTuple_GET_ITEM(value, i);
                if (i > 0) {
                    if (PyUnicodeWriter_WriteUTF8(writer, ", ", 2) < 0) {
                        PyUnicodeWriter_Discard(writer);
                        return NULL;
                    }
                }
                if (_Py_typing_type_repr(writer, item) < 0) {
                    PyUnicodeWriter_Discard(writer);
                    return NULL;
                }
            }
            if (PyUnicodeWriter_WriteChar(writer, ')') < 0) {
                PyUnicodeWriter_Discard(writer);
                return NULL;
            }
        }
        else {
            if (_Py_typing_type_repr(writer, value) < 0) {
                PyUnicodeWriter_Discard(writer);
                return NULL;
            }
        }
        return PyUnicodeWriter_Finish(writer);
    }
    return Py_NewRef(value);
}

static PyObject *
constevaluator_alloc(PyObject *value)
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.constevaluator_type;
    assert(tp != NULL);
    constevaluatorobject *ce = PyObject_GC_New(constevaluatorobject, tp);
    if (ce == NULL) {
        return NULL;
    }
    ce->value = Py_NewRef(value);
    _PyObject_GC_TRACK(ce);
    return (PyObject *)ce;

}

PyDoc_STRVAR(constevaluator_doc,
"_ConstEvaluator()\n"
"--\n\n"
"Internal type for implementing evaluation functions.");

static PyType_Slot constevaluator_slots[] = {
    {Py_tp_doc, (void *)constevaluator_doc},
    {Py_tp_dealloc, constevaluator_dealloc},
    {Py_tp_traverse, constevaluator_traverse},
    {Py_tp_clear, constevaluator_clear},
    {Py_tp_repr, constevaluator_repr},
    {Py_tp_call, constevaluator_call},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {0, NULL},
};

PyType_Spec constevaluator_spec = {
    .name = "_typing._ConstEvaluator",
    .basicsize = sizeof(constevaluatorobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_DISALLOW_INSTANTIATION,
    .slots = constevaluator_slots,
};

int
_Py_typing_type_repr(PyUnicodeWriter *writer, PyObject *p)
{
    PyObject *qualname = NULL;
    PyObject *module = NULL;
    PyObject *r = NULL;
    int rc;

    if (p == Py_Ellipsis) {
        // The Ellipsis object
        r = PyUnicode_FromString("...");
        goto exit;
    }

    if (p == (PyObject *)&_PyNone_Type) {
        return PyUnicodeWriter_WriteUTF8(writer, "None", 4);
    }

    if ((rc = PyObject_HasAttrWithError(p, &_Py_ID(__origin__))) > 0 &&
        (rc = PyObject_HasAttrWithError(p, &_Py_ID(__args__))) > 0)
    {
        // It looks like a GenericAlias
        goto use_repr;
    }
    if (rc < 0) {
        goto exit;
    }

    if (PyObject_GetOptionalAttr(p, &_Py_ID(__qualname__), &qualname) < 0) {
        goto exit;
    }
    if (qualname == NULL) {
        goto use_repr;
    }
    if (PyObject_GetOptionalAttr(p, &_Py_ID(__module__), &module) < 0) {
        goto exit;
    }
    if (module == NULL || module == Py_None) {
        goto use_repr;
    }

    // Looks like a class
    if (PyUnicode_Check(module) &&
        _PyUnicode_EqualToASCIIString(module, "builtins"))
    {
        // builtins don't need a module name
        r = PyObject_Str(qualname);
        goto exit;
    }
    else {
        r = PyUnicode_FromFormat("%S.%S", module, qualname);
        goto exit;
    }

use_repr:
    r = PyObject_Repr(p);
exit:
    Py_XDECREF(qualname);
    Py_XDECREF(module);
    if (r == NULL) {
        return -1;
    }
    rc = PyUnicodeWriter_WriteStr(writer, r);
    Py_DECREF(r);
    return rc;
}


static PyObject *
call_typing_func_object(const char *name, PyObject **args, size_t nargs)
{
    PyObject *typing = PyImport_ImportModule("typing");
    if (typing == NULL) {
        return NULL;
    }
    PyObject *func = PyObject_GetAttrString(typing, name);
    if (func == NULL) {
        Py_DECREF(typing);
        return NULL;
    }
    PyObject *result = PyObject_Vectorcall(func, args, nargs, NULL);
    Py_DECREF(func);
    Py_DECREF(typing);
    return result;
}

static PyObject *
type_check(PyObject *arg, const char *msg)
{
    // Calling typing.py here leads to bootstrapping problems
    if (Py_IsNone(arg)) {
        return Py_NewRef(Py_TYPE(arg));
    }
    PyObject *message_str = PyUnicode_FromString(msg);
    if (message_str == NULL) {
        return NULL;
    }
    PyObject *args[2] = {arg, message_str};
    PyObject *result = call_typing_func_object("_type_check", args, 2);
    Py_DECREF(message_str);
    return result;
}

/*
 * Return a typing.Union. This is used as the nb_or (|) operator for
 * TypeVar and ParamSpec. We use this rather than _Py_union_type_or
 * (which would produce a types.Union) because historically TypeVar
 * supported unions with string forward references, and we want to
 * preserve that behavior. _Py_union_type_or only allows a small set
 * of types.
 */
static PyObject *
make_union(PyObject *self, PyObject *other)
{
    PyObject *args = PyTuple_Pack(2, self, other);
    if (args == NULL) {
        return NULL;
    }
    PyObject *u = _Py_union_from_tuple(args);
    Py_DECREF(args);
    return u;
}

static PyObject *
caller(void)
{
    _PyInterpreterFrame *f = _PyThreadState_GET()->current_frame;
    if (f == NULL) {
        Py_RETURN_NONE;
    }
    if (f == NULL || PyStackRef_IsNull(f->f_funcobj)) {
        Py_RETURN_NONE;
    }
    PyObject *r = PyFunction_GetModule(PyStackRef_AsPyObjectBorrow(f->f_funcobj));
    if (!r) {
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    return Py_NewRef(r);
}

static PyObject *
unpack(PyObject *self)
{
    PyObject *typing = PyImport_ImportModule("typing");
    if (typing == NULL) {
        return NULL;
    }
    PyObject *unpack = PyObject_GetAttrString(typing, "Unpack");
    if (unpack == NULL) {
        Py_DECREF(typing);
        return NULL;
    }
    PyObject *unpacked = PyObject_GetItem(unpack, self);
    Py_DECREF(typing);
    Py_DECREF(unpack);
    return unpacked;
}

static int
contains_typevartuple(PyTupleObject *params)
{
    Py_ssize_t n = PyTuple_GET_SIZE(params);
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.typevartuple_type;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *param = PyTuple_GET_ITEM(params, i);
        if (Py_IS_TYPE(param, tp)) {
            return 1;
        }
    }
    return 0;
}

static PyObject *
unpack_typevartuples(PyObject *params)
{
    assert(PyTuple_Check(params));
    // TypeVarTuple must be unpacked when passed to Generic, so we do that here.
    if (contains_typevartuple((PyTupleObject *)params)) {
        Py_ssize_t n = PyTuple_GET_SIZE(params);
        PyObject *new_params = PyTuple_New(n);
        if (new_params == NULL) {
            return NULL;
        }
        PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.typevartuple_type;
        for (Py_ssize_t i = 0; i < n; i++) {
            PyObject *param = PyTuple_GET_ITEM(params, i);
            if (Py_IS_TYPE(param, tp)) {
                PyObject *unpacked = unpack(param);
                if (unpacked == NULL) {
                    Py_DECREF(new_params);
                    return NULL;
                }
                PyTuple_SET_ITEM(new_params, i, unpacked);
            }
            else {
                PyTuple_SET_ITEM(new_params, i, Py_NewRef(param));
            }
        }
        return new_params;
    }
    else {
        return Py_NewRef(params);
    }
}

static void
typevar_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    typevarobject *tv = typevarobject_CAST(self);

    _PyObject_GC_UNTRACK(self);

    Py_DECREF(tv->name);
    Py_XDECREF(tv->bound);
    Py_XDECREF(tv->evaluate_bound);
    Py_XDECREF(tv->constraints);
    Py_XDECREF(tv->evaluate_constraints);
    Py_XDECREF(tv->default_value);
    Py_XDECREF(tv->evaluate_default);
    PyObject_ClearManagedDict(self);
    PyObject_ClearWeakRefs(self);

    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
typevar_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    typevarobject *tv = typevarobject_CAST(self);
    Py_VISIT(tv->bound);
    Py_VISIT(tv->evaluate_bound);
    Py_VISIT(tv->constraints);
    Py_VISIT(tv->evaluate_constraints);
    Py_VISIT(tv->default_value);
    Py_VISIT(tv->evaluate_default);
    PyObject_VisitManagedDict(self, visit, arg);
    return 0;
}

static int
typevar_clear(PyObject *op)
{
    typevarobject *self = typevarobject_CAST(op);
    Py_CLEAR(self->bound);
    Py_CLEAR(self->evaluate_bound);
    Py_CLEAR(self->constraints);
    Py_CLEAR(self->evaluate_constraints);
    Py_CLEAR(self->default_value);
    Py_CLEAR(self->evaluate_default);
    PyObject_ClearManagedDict(op);
    return 0;
}

static PyObject *
typevar_repr(PyObject *self)
{
    typevarobject *tv = typevarobject_CAST(self);

    if (tv->infer_variance) {
        return Py_NewRef(tv->name);
    }

    char variance = tv->covariant ? '+' : tv->contravariant ? '-' : '~';
    return PyUnicode_FromFormat("%c%U", variance, tv->name);
}

static PyMemberDef typevar_members[] = {
    {"__name__", _Py_T_OBJECT, offsetof(typevarobject, name), Py_READONLY},
    {"__covariant__", Py_T_BOOL, offsetof(typevarobject, covariant), Py_READONLY},
    {"__contravariant__", Py_T_BOOL, offsetof(typevarobject, contravariant), Py_READONLY},
    {"__infer_variance__", Py_T_BOOL, offsetof(typevarobject, infer_variance), Py_READONLY},
    {0}
};

static PyObject *
typevar_bound(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->bound != NULL) {
        return Py_NewRef(self->bound);
    }
    if (self->evaluate_bound == NULL) {
        Py_RETURN_NONE;
    }
    PyObject *bound = PyObject_CallNoArgs(self->evaluate_bound);
    self->bound = Py_XNewRef(bound);
    return bound;
}

static PyObject *
typevar_default(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->default_value != NULL) {
        return Py_NewRef(self->default_value);
    }
    if (self->evaluate_default == NULL) {
        return &_Py_NoDefaultStruct;
    }
    PyObject *default_value = PyObject_CallNoArgs(self->evaluate_default);
    self->default_value = Py_XNewRef(default_value);
    return default_value;
}

static PyObject *
typevar_constraints(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->constraints != NULL) {
        return Py_NewRef(self->constraints);
    }
    if (self->evaluate_constraints == NULL) {
        return PyTuple_New(0);
    }
    PyObject *constraints = PyObject_CallNoArgs(self->evaluate_constraints);
    self->constraints = Py_XNewRef(constraints);
    return constraints;
}

static PyObject *
typevar_evaluate_bound(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->evaluate_bound != NULL) {
        return Py_NewRef(self->evaluate_bound);
    }
    if (self->bound != NULL) {
        return constevaluator_alloc(self->bound);
    }
    Py_RETURN_NONE;
}

static PyObject *
typevar_evaluate_constraints(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->evaluate_constraints != NULL) {
        return Py_NewRef(self->evaluate_constraints);
    }
    if (self->constraints != NULL) {
        return constevaluator_alloc(self->constraints);
    }
    Py_RETURN_NONE;
}

static PyObject *
typevar_evaluate_default(PyObject *op, void *Py_UNUSED(closure))
{
    typevarobject *self = typevarobject_CAST(op);
    if (self->evaluate_default != NULL) {
        return Py_NewRef(self->evaluate_default);
    }
    if (self->default_value != NULL) {
        return constevaluator_alloc(self->default_value);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef typevar_getset[] = {
    {"__bound__", typevar_bound, NULL, NULL, NULL},
    {"__constraints__", typevar_constraints, NULL, NULL, NULL},
    {"__default__", typevar_default, NULL, NULL, NULL},
    {"evaluate_bound", typevar_evaluate_bound, NULL, NULL, NULL},
    {"evaluate_constraints", typevar_evaluate_constraints, NULL, NULL, NULL},
    {"evaluate_default", typevar_evaluate_default, NULL, NULL, NULL},
    {0}
};

static typevarobject *
typevar_alloc(PyObject *name, PyObject *bound, PyObject *evaluate_bound,
              PyObject *constraints, PyObject *evaluate_constraints,
              PyObject *default_value,
              bool covariant, bool contravariant, bool infer_variance,
              PyObject *module)
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.typevar_type;
    assert(tp != NULL);
    typevarobject *tv = PyObject_GC_New(typevarobject, tp);
    if (tv == NULL) {
        return NULL;
    }

    tv->name = Py_NewRef(name);

    tv->bound = Py_XNewRef(bound);
    tv->evaluate_bound = Py_XNewRef(evaluate_bound);
    tv->constraints = Py_XNewRef(constraints);
    tv->evaluate_constraints = Py_XNewRef(evaluate_constraints);
    tv->default_value = Py_XNewRef(default_value);
    tv->evaluate_default = NULL;

    tv->covariant = covariant;
    tv->contravariant = contravariant;
    tv->infer_variance = infer_variance;
    _PyObject_GC_TRACK(tv);

    if (module != NULL) {
        if (PyObject_SetAttrString((PyObject *)tv, "__module__", module) < 0) {
            Py_DECREF(tv);
            return NULL;
        }
    }

    return tv;
}

/*[clinic input]
@classmethod
typevar.__new__ as typevar_new

    name: object(subclass_of="&PyUnicode_Type")
    *constraints: tuple
    bound: object = None
    default as default_value: object(c_default="&_Py_NoDefaultStruct") = typing.NoDefault
    covariant: bool = False
    contravariant: bool = False
    infer_variance: bool = False

Create a TypeVar.
[clinic start generated code]*/

static PyObject *
typevar_new_impl(PyTypeObject *type, PyObject *name, PyObject *constraints,
                 PyObject *bound, PyObject *default_value, int covariant,
                 int contravariant, int infer_variance)
/*[clinic end generated code: output=d2b248ff074eaab6 input=1b5b62e40c92c167]*/
{
    if (covariant && contravariant) {
        PyErr_SetString(PyExc_ValueError,
                        "Bivariant types are not supported.");
        return NULL;
    }

    if (infer_variance && (covariant || contravariant)) {
        PyErr_SetString(PyExc_ValueError,
                        "Variance cannot be specified with infer_variance.");
        return NULL;
    }

    if (Py_IsNone(bound)) {
        bound = NULL;
    }
    if (bound != NULL) {
        bound = type_check(bound, "Bound must be a type.");
        if (bound == NULL) {
            return NULL;
        }
    }

    assert(PyTuple_CheckExact(constraints));
    Py_ssize_t n_constraints = PyTuple_GET_SIZE(constraints);
    if (n_constraints == 1) {
        PyErr_SetString(PyExc_TypeError,
                        "A single constraint is not allowed");
        Py_XDECREF(bound);
        return NULL;
    } else if (n_constraints == 0) {
        constraints = NULL;
    } else if (bound != NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Constraints cannot be combined with bound=...");
        Py_XDECREF(bound);
        return NULL;
    }
    PyObject *module = caller();
    if (module == NULL) {
        Py_XDECREF(bound);
        return NULL;
    }

    PyObject *tv = (PyObject *)typevar_alloc(name, bound, NULL,
                                             constraints, NULL,
                                             default_value,
                                             covariant, contravariant,
                                             infer_variance, module);
    Py_XDECREF(bound);
    Py_XDECREF(module);
    return tv;
}

/*[clinic input]
typevar.__typing_subst__ as typevar_typing_subst

    arg: object
    /

[clinic start generated code]*/

static PyObject *
typevar_typing_subst_impl(typevarobject *self, PyObject *arg)
/*[clinic end generated code: output=c76ced134ed8f4e1 input=9e87b57f0fc59b92]*/
{
    PyObject *args[2] = {(PyObject *)self, arg};
    PyObject *result = call_typing_func_object("_typevar_subst", args, 2);
    return result;
}

/*[clinic input]
typevar.__typing_prepare_subst__ as typevar_typing_prepare_subst

    alias: object
    args: object
    /

[clinic start generated code]*/

static PyObject *
typevar_typing_prepare_subst_impl(typevarobject *self, PyObject *alias,
                                  PyObject *args)
/*[clinic end generated code: output=82c3f4691e0ded22 input=201a750415d14ffb]*/
{
    PyObject *params = PyObject_GetAttrString(alias, "__parameters__");
    if (params == NULL) {
        return NULL;
    }
    Py_ssize_t i = PySequence_Index(params, (PyObject *)self);
    if (i == -1) {
        Py_DECREF(params);
        return NULL;
    }
    Py_ssize_t args_len = PySequence_Length(args);
    if (args_len == -1) {
        Py_DECREF(params);
        return NULL;
    }
    if (i < args_len) {
        // We already have a value for our TypeVar
        Py_DECREF(params);
        return Py_NewRef(args);
    }
    else if (i == args_len) {
        // If the TypeVar has a default, use it.
        PyObject *dflt = typevar_default((PyObject *)self, NULL);
        if (dflt == NULL) {
            Py_DECREF(params);
            return NULL;
        }
        if (dflt != &_Py_NoDefaultStruct) {
            PyObject *new_args = PyTuple_Pack(1, dflt);
            Py_DECREF(dflt);
            if (new_args == NULL) {
                Py_DECREF(params);
                return NULL;
            }
            PyObject *result = PySequence_Concat(args, new_args);
            Py_DECREF(params);
            Py_DECREF(new_args);
            return result;
        }
    }
    Py_DECREF(params);
    PyErr_Format(PyExc_TypeError,
                 "Too few arguments for %S; actual %d, expected at least %d",
                 alias, args_len, i + 1);
    return NULL;
}

/*[clinic input]
typevar.__reduce__ as typevar_reduce

[clinic start generated code]*/

static PyObject *
typevar_reduce_impl(typevarobject *self)
/*[clinic end generated code: output=02e5c55d7cf8a08f input=de76bc95f04fb9ff]*/
{
    return Py_NewRef(self->name);
}


/*[clinic input]
typevar.has_default as typevar_has_default

[clinic start generated code]*/

static PyObject *
typevar_has_default_impl(typevarobject *self)
/*[clinic end generated code: output=76bf0b8dc98b97dd input=31024aa030761cf6]*/
{
    if (self->evaluate_default != NULL ||
        (self->default_value != &_Py_NoDefaultStruct && self->default_value != NULL)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
typevar_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of TypeVar");
    return NULL;
}

static PyMethodDef typevar_methods[] = {
    TYPEVAR_TYPING_SUBST_METHODDEF
    TYPEVAR_TYPING_PREPARE_SUBST_METHODDEF
    TYPEVAR_REDUCE_METHODDEF
    TYPEVAR_HAS_DEFAULT_METHODDEF
    {"__mro_entries__", typevar_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(typevar_doc,
"Type variable.\n\
\n\
The preferred way to construct a type variable is via the dedicated\n\
syntax for generic functions, classes, and type aliases::\n\
\n\
    class Sequence[T]:  # T is a TypeVar\n\
        ...\n\
\n\
This syntax can also be used to create bound and constrained type\n\
variables::\n\
\n\
    # S is a TypeVar bound to str\n\
    class StrSequence[S: str]:\n\
        ...\n\
\n\
    # A is a TypeVar constrained to str or bytes\n\
    class StrOrBytesSequence[A: (str, bytes)]:\n\
        ...\n\
\n\
Type variables can also have defaults:\n\
\n\
    class IntDefault[T = int]:\n\
        ...\n\
\n\
However, if desired, reusable type variables can also be constructed\n\
manually, like so::\n\
\n\
   T = TypeVar('T')  # Can be anything\n\
   S = TypeVar('S', bound=str)  # Can be any subtype of str\n\
   A = TypeVar('A', str, bytes)  # Must be exactly str or bytes\n\
   D = TypeVar('D', default=int)  # Defaults to int\n\
\n\
Type variables exist primarily for the benefit of static type\n\
checkers.  They serve as the parameters for generic types as well\n\
as for generic function and type alias definitions.\n\
\n\
The variance of type variables is inferred by type checkers when they\n\
are created through the type parameter syntax and when\n\
``infer_variance=True`` is passed. Manually created type variables may\n\
be explicitly marked covariant or contravariant by passing\n\
``covariant=True`` or ``contravariant=True``. By default, manually\n\
created type variables are invariant. See PEP 484 and PEP 695 for more\n\
details.\n\
");

static PyType_Slot typevar_slots[] = {
    {Py_tp_doc, (void *)typevar_doc},
    {Py_tp_methods, typevar_methods},
    {Py_nb_or, make_union},
    {Py_tp_new, typevar_new},
    {Py_tp_dealloc, typevar_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, typevar_traverse},
    {Py_tp_clear, typevar_clear},
    {Py_tp_repr, typevar_repr},
    {Py_tp_members, typevar_members},
    {Py_tp_getset, typevar_getset},
    {0, NULL},
};

PyType_Spec typevar_spec = {
    .name = "typing.TypeVar",
    .basicsize = sizeof(typevarobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_MANAGED_DICT | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = typevar_slots,
};

typedef struct {
    PyObject_HEAD
    PyObject *__origin__;
} paramspecattrobject;

#define paramspecattrobject_CAST(op)    ((paramspecattrobject *)(op))

static void
paramspecattr_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    paramspecattrobject *psa = paramspecattrobject_CAST(self);

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(psa->__origin__);

    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
paramspecattr_traverse(PyObject *self, visitproc visit, void *arg)
{
    paramspecattrobject *psa = paramspecattrobject_CAST(self);
    Py_VISIT(psa->__origin__);
    return 0;
}

static int
paramspecattr_clear(PyObject *op)
{
    paramspecattrobject *self = paramspecattrobject_CAST(op);
    Py_CLEAR(self->__origin__);
    return 0;
}

static PyObject *
paramspecattr_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!Py_IS_TYPE(a, Py_TYPE(b))) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    paramspecattrobject *lhs = paramspecattrobject_CAST(a); // may be unsafe
    paramspecattrobject *rhs = (paramspecattrobject *)b;    // safe fast cast
    return PyObject_RichCompare(lhs->__origin__, rhs->__origin__, op);
}

static PyMemberDef paramspecattr_members[] = {
    {"__origin__", _Py_T_OBJECT, offsetof(paramspecattrobject, __origin__), Py_READONLY},
    {0}
};

static paramspecattrobject *
paramspecattr_new(PyTypeObject *tp, PyObject *origin)
{
    paramspecattrobject *psa = PyObject_GC_New(paramspecattrobject, tp);
    if (psa == NULL) {
        return NULL;
    }
    psa->__origin__ = Py_NewRef(origin);
    _PyObject_GC_TRACK(psa);
    return psa;
}

static PyObject *
paramspecargs_repr(PyObject *self)
{
    paramspecattrobject *psa = paramspecattrobject_CAST(self);
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.paramspec_type;
    if (Py_IS_TYPE(psa->__origin__, tp)) {
        return PyUnicode_FromFormat("%U.args",
            ((paramspecobject *)psa->__origin__)->name);
    }
    return PyUnicode_FromFormat("%R.args", psa->__origin__);
}


/*[clinic input]
@classmethod
paramspecargs.__new__ as paramspecargs_new

    origin: object

Create a ParamSpecArgs object.
[clinic start generated code]*/

static PyObject *
paramspecargs_new_impl(PyTypeObject *type, PyObject *origin)
/*[clinic end generated code: output=9a1463dc8942fe4e input=3596a0bb6183c208]*/
{
    return (PyObject *)paramspecattr_new(type, origin);
}

static PyObject *
paramspecargs_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of ParamSpecArgs");
    return NULL;
}

static PyMethodDef paramspecargs_methods[] = {
    {"__mro_entries__", paramspecargs_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(paramspecargs_doc,
"The args for a ParamSpec object.\n\
\n\
Given a ParamSpec object P, P.args is an instance of ParamSpecArgs.\n\
\n\
ParamSpecArgs objects have a reference back to their ParamSpec::\n\
\n\
    >>> P = ParamSpec(\"P\")\n\
    >>> P.args.__origin__ is P\n\
    True\n\
\n\
This type is meant for runtime introspection and has no special meaning\n\
to static type checkers.\n\
");

static PyType_Slot paramspecargs_slots[] = {
    {Py_tp_doc, (void *)paramspecargs_doc},
    {Py_tp_methods, paramspecargs_methods},
    {Py_tp_new, paramspecargs_new},
    {Py_tp_dealloc, paramspecattr_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, paramspecattr_traverse},
    {Py_tp_clear, paramspecattr_clear},
    {Py_tp_repr, paramspecargs_repr},
    {Py_tp_members, paramspecattr_members},
    {Py_tp_richcompare, paramspecattr_richcompare},
    {0, NULL},
};

PyType_Spec paramspecargs_spec = {
    .name = "typing.ParamSpecArgs",
    .basicsize = sizeof(paramspecattrobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = paramspecargs_slots,
};

static PyObject *
paramspeckwargs_repr(PyObject *self)
{
    paramspecattrobject *psk = paramspecattrobject_CAST(self);

    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.paramspec_type;
    if (Py_IS_TYPE(psk->__origin__, tp)) {
        return PyUnicode_FromFormat("%U.kwargs",
            ((paramspecobject *)psk->__origin__)->name);
    }
    return PyUnicode_FromFormat("%R.kwargs", psk->__origin__);
}

/*[clinic input]
@classmethod
paramspeckwargs.__new__ as paramspeckwargs_new

    origin: object

Create a ParamSpecKwargs object.
[clinic start generated code]*/

static PyObject *
paramspeckwargs_new_impl(PyTypeObject *type, PyObject *origin)
/*[clinic end generated code: output=277b11967ebaf4ab input=981bca9b0cf9e40a]*/
{
    return (PyObject *)paramspecattr_new(type, origin);
}

static PyObject *
paramspeckwargs_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of ParamSpecKwargs");
    return NULL;
}

static PyMethodDef paramspeckwargs_methods[] = {
    {"__mro_entries__", paramspeckwargs_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(paramspeckwargs_doc,
"The kwargs for a ParamSpec object.\n\
\n\
Given a ParamSpec object P, P.kwargs is an instance of ParamSpecKwargs.\n\
\n\
ParamSpecKwargs objects have a reference back to their ParamSpec::\n\
\n\
    >>> P = ParamSpec(\"P\")\n\
    >>> P.kwargs.__origin__ is P\n\
    True\n\
\n\
This type is meant for runtime introspection and has no special meaning\n\
to static type checkers.\n\
");

static PyType_Slot paramspeckwargs_slots[] = {
    {Py_tp_doc, (void *)paramspeckwargs_doc},
    {Py_tp_methods, paramspeckwargs_methods},
    {Py_tp_new, paramspeckwargs_new},
    {Py_tp_dealloc, paramspecattr_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, paramspecattr_traverse},
    {Py_tp_clear, paramspecattr_clear},
    {Py_tp_repr, paramspeckwargs_repr},
    {Py_tp_members, paramspecattr_members},
    {Py_tp_richcompare, paramspecattr_richcompare},
    {0, NULL},
};

PyType_Spec paramspeckwargs_spec = {
    .name = "typing.ParamSpecKwargs",
    .basicsize = sizeof(paramspecattrobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = paramspeckwargs_slots,
};

static void
paramspec_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    paramspecobject *ps = paramspecobject_CAST(self);

    _PyObject_GC_UNTRACK(self);

    Py_DECREF(ps->name);
    Py_XDECREF(ps->bound);
    Py_XDECREF(ps->default_value);
    Py_XDECREF(ps->evaluate_default);
    PyObject_ClearManagedDict(self);
    PyObject_ClearWeakRefs(self);

    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
paramspec_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    paramspecobject *ps = paramspecobject_CAST(self);
    Py_VISIT(ps->bound);
    Py_VISIT(ps->default_value);
    Py_VISIT(ps->evaluate_default);
    PyObject_VisitManagedDict(self, visit, arg);
    return 0;
}

static int
paramspec_clear(PyObject *op)
{
    paramspecobject *self = paramspecobject_CAST(op);
    Py_CLEAR(self->bound);
    Py_CLEAR(self->default_value);
    Py_CLEAR(self->evaluate_default);
    PyObject_ClearManagedDict(op);
    return 0;
}

static PyObject *
paramspec_repr(PyObject *self)
{
    paramspecobject *ps = paramspecobject_CAST(self);

    if (ps->infer_variance) {
        return Py_NewRef(ps->name);
    }

    char variance = ps->covariant ? '+' : ps->contravariant ? '-' : '~';
    return PyUnicode_FromFormat("%c%U", variance, ps->name);
}

static PyMemberDef paramspec_members[] = {
    {"__name__", _Py_T_OBJECT, offsetof(paramspecobject, name), Py_READONLY},
    {"__bound__", _Py_T_OBJECT, offsetof(paramspecobject, bound), Py_READONLY},
    {"__covariant__", Py_T_BOOL, offsetof(paramspecobject, covariant), Py_READONLY},
    {"__contravariant__", Py_T_BOOL, offsetof(paramspecobject, contravariant), Py_READONLY},
    {"__infer_variance__", Py_T_BOOL, offsetof(paramspecobject, infer_variance), Py_READONLY},
    {0}
};

static PyObject *
paramspec_args(PyObject *self, void *Py_UNUSED(closure))
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.paramspecargs_type;
    return (PyObject *)paramspecattr_new(tp, self);
}

static PyObject *
paramspec_kwargs(PyObject *self, void *Py_UNUSED(closure))
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.paramspeckwargs_type;
    return (PyObject *)paramspecattr_new(tp, self);
}

static PyObject *
paramspec_default(PyObject *op, void *Py_UNUSED(closure))
{
    paramspecobject *self = paramspecobject_CAST(op);
    if (self->default_value != NULL) {
        return Py_NewRef(self->default_value);
    }
    if (self->evaluate_default == NULL) {
        return &_Py_NoDefaultStruct;
    }
    PyObject *default_value = PyObject_CallNoArgs(self->evaluate_default);
    self->default_value = Py_XNewRef(default_value);
    return default_value;
}

static PyObject *
paramspec_evaluate_default(PyObject *op, void *Py_UNUSED(closure))
{
    paramspecobject *self = paramspecobject_CAST(op);
    if (self->evaluate_default != NULL) {
        return Py_NewRef(self->evaluate_default);
    }
    if (self->default_value != NULL) {
        return constevaluator_alloc(self->default_value);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef paramspec_getset[] = {
    {"args", paramspec_args, NULL, PyDoc_STR("Represents positional arguments."), NULL},
    {"kwargs", paramspec_kwargs, NULL, PyDoc_STR("Represents keyword arguments."), NULL},
    {"__default__", paramspec_default, NULL, "The default value for this ParamSpec.", NULL},
    {"evaluate_default", paramspec_evaluate_default, NULL, NULL, NULL},
    {0},
};

static paramspecobject *
paramspec_alloc(PyObject *name, PyObject *bound, PyObject *default_value, bool covariant,
                bool contravariant, bool infer_variance, PyObject *module)
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.paramspec_type;
    paramspecobject *ps = PyObject_GC_New(paramspecobject, tp);
    if (ps == NULL) {
        return NULL;
    }
    ps->name = Py_NewRef(name);
    ps->bound = Py_XNewRef(bound);
    ps->covariant = covariant;
    ps->contravariant = contravariant;
    ps->infer_variance = infer_variance;
    ps->default_value = Py_XNewRef(default_value);
    ps->evaluate_default = NULL;
    _PyObject_GC_TRACK(ps);
    if (module != NULL) {
        if (PyObject_SetAttrString((PyObject *)ps, "__module__", module) < 0) {
            Py_DECREF(ps);
            return NULL;
        }
    }
    return ps;
}

/*[clinic input]
@classmethod
paramspec.__new__ as paramspec_new

    name: object(subclass_of="&PyUnicode_Type")
    *
    bound: object = None
    default as default_value: object(c_default="&_Py_NoDefaultStruct") = typing.NoDefault
    covariant: bool = False
    contravariant: bool = False
    infer_variance: bool = False

Create a ParamSpec object.
[clinic start generated code]*/

static PyObject *
paramspec_new_impl(PyTypeObject *type, PyObject *name, PyObject *bound,
                   PyObject *default_value, int covariant, int contravariant,
                   int infer_variance)
/*[clinic end generated code: output=47ca9d63fa5a094d input=495e1565bc067ab9]*/
{
    if (covariant && contravariant) {
        PyErr_SetString(PyExc_ValueError, "Bivariant types are not supported.");
        return NULL;
    }
    if (infer_variance && (covariant || contravariant)) {
        PyErr_SetString(PyExc_ValueError, "Variance cannot be specified with infer_variance.");
        return NULL;
    }
    if (bound != NULL) {
        bound = type_check(bound, "Bound must be a type.");
        if (bound == NULL) {
            return NULL;
        }
    }
    PyObject *module = caller();
    if (module == NULL) {
        Py_XDECREF(bound);
        return NULL;
    }
    PyObject *ps = (PyObject *)paramspec_alloc(
        name, bound, default_value, covariant, contravariant, infer_variance, module);
    Py_XDECREF(bound);
    Py_DECREF(module);
    return ps;
}


/*[clinic input]
paramspec.__typing_subst__ as paramspec_typing_subst

    arg: object
    /

[clinic start generated code]*/

static PyObject *
paramspec_typing_subst_impl(paramspecobject *self, PyObject *arg)
/*[clinic end generated code: output=803e1ade3f13b57d input=2d5b5e3d4a717189]*/
{
    PyObject *args[2] = {(PyObject *)self, arg};
    PyObject *result = call_typing_func_object("_paramspec_subst", args, 2);
    return result;
}

/*[clinic input]
paramspec.__typing_prepare_subst__ as paramspec_typing_prepare_subst

    alias: object
    args: object
    /

[clinic start generated code]*/

static PyObject *
paramspec_typing_prepare_subst_impl(paramspecobject *self, PyObject *alias,
                                    PyObject *args)
/*[clinic end generated code: output=95449d630a2adb9a input=6df6f9fef3e150da]*/
{
    PyObject *args_array[3] = {(PyObject *)self, alias, args};
    PyObject *result = call_typing_func_object(
        "_paramspec_prepare_subst", args_array, 3);
    return result;
}

/*[clinic input]
paramspec.__reduce__ as paramspec_reduce

[clinic start generated code]*/

static PyObject *
paramspec_reduce_impl(paramspecobject *self)
/*[clinic end generated code: output=b83398674416db27 input=5bf349f0d5dd426c]*/
{
    return Py_NewRef(self->name);
}

/*[clinic input]
paramspec.has_default as paramspec_has_default

[clinic start generated code]*/

static PyObject *
paramspec_has_default_impl(paramspecobject *self)
/*[clinic end generated code: output=daaae7467a6a4368 input=2112e97eeb76cd59]*/
{
    if (self->evaluate_default != NULL ||
        (self->default_value != &_Py_NoDefaultStruct && self->default_value != NULL)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
paramspec_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of ParamSpec");
    return NULL;
}

static PyMethodDef paramspec_methods[] = {
    PARAMSPEC_TYPING_SUBST_METHODDEF
    PARAMSPEC_TYPING_PREPARE_SUBST_METHODDEF
    PARAMSPEC_HAS_DEFAULT_METHODDEF
    PARAMSPEC_REDUCE_METHODDEF
    {"__mro_entries__", paramspec_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(paramspec_doc,
"Parameter specification variable.\n\
\n\
The preferred way to construct a parameter specification is via the\n\
dedicated syntax for generic functions, classes, and type aliases,\n\
where the use of '**' creates a parameter specification::\n\
\n\
    type IntFunc[**P] = Callable[P, int]\n\
\n\
The following syntax creates a parameter specification that defaults\n\
to a callable accepting two positional-only arguments of types int\n\
and str:\n\
\n\
    type IntFuncDefault[**P = (int, str)] = Callable[P, int]\n\
\n\
For compatibility with Python 3.11 and earlier, ParamSpec objects\n\
can also be created as follows::\n\
\n\
    P = ParamSpec('P')\n\
    DefaultP = ParamSpec('DefaultP', default=(int, str))\n\
\n\
Parameter specification variables exist primarily for the benefit of\n\
static type checkers.  They are used to forward the parameter types of\n\
one callable to another callable, a pattern commonly found in\n\
higher-order functions and decorators.  They are only valid when used\n\
in ``Concatenate``, or as the first argument to ``Callable``, or as\n\
parameters for user-defined Generics. See class Generic for more\n\
information on generic types.\n\
\n\
An example for annotating a decorator::\n\
\n\
    def add_logging[**P, T](f: Callable[P, T]) -> Callable[P, T]:\n\
        '''A type-safe decorator to add logging to a function.'''\n\
        def inner(*args: P.args, **kwargs: P.kwargs) -> T:\n\
            logging.info(f'{f.__name__} was called')\n\
            return f(*args, **kwargs)\n\
        return inner\n\
\n\
    @add_logging\n\
    def add_two(x: float, y: float) -> float:\n\
        '''Add two numbers together.'''\n\
        return x + y\n\
\n\
Parameter specification variables can be introspected. e.g.::\n\
\n\
    >>> P = ParamSpec(\"P\")\n\
    >>> P.__name__\n\
    'P'\n\
\n\
Note that only parameter specification variables defined in the global\n\
scope can be pickled.\n\
");

static PyType_Slot paramspec_slots[] = {
    {Py_tp_doc, (void *)paramspec_doc},
    {Py_tp_members, paramspec_members},
    {Py_tp_methods, paramspec_methods},
    {Py_tp_getset, paramspec_getset},
    // Unions of ParamSpecs have no defined meaning, but they were allowed
    // by the Python implementation, so we allow them here too.
    {Py_nb_or, make_union},
    {Py_tp_new, paramspec_new},
    {Py_tp_dealloc, paramspec_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, paramspec_traverse},
    {Py_tp_clear, paramspec_clear},
    {Py_tp_repr, paramspec_repr},
    {0, 0},
};

PyType_Spec paramspec_spec = {
    .name = "typing.ParamSpec",
    .basicsize = sizeof(paramspecobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE
        | Py_TPFLAGS_MANAGED_DICT | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = paramspec_slots,
};

static void
typevartuple_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    typevartupleobject *tvt = typevartupleobject_CAST(self);

    Py_DECREF(tvt->name);
    Py_XDECREF(tvt->default_value);
    Py_XDECREF(tvt->evaluate_default);
    PyObject_ClearManagedDict(self);
    PyObject_ClearWeakRefs(self);

    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
unpack_iter(PyObject *self)
{
    PyObject *unpacked = unpack(self);
    if (unpacked == NULL) {
        return NULL;
    }
    PyObject *tuple = PyTuple_Pack(1, unpacked);
    if (tuple == NULL) {
        Py_DECREF(unpacked);
        return NULL;
    }
    PyObject *result = PyObject_GetIter(tuple);
    Py_DECREF(unpacked);
    Py_DECREF(tuple);
    return result;
}

static PyObject *
typevartuple_repr(PyObject *self)
{
    typevartupleobject *tvt = typevartupleobject_CAST(self);
    return Py_NewRef(tvt->name);
}

static PyMemberDef typevartuple_members[] = {
    {"__name__", _Py_T_OBJECT, offsetof(typevartupleobject, name), Py_READONLY},
    {0}
};

static typevartupleobject *
typevartuple_alloc(PyObject *name, PyObject *module, PyObject *default_value)
{
    PyTypeObject *tp = _PyInterpreterState_GET()->cached_objects.typevartuple_type;
    typevartupleobject *tvt = PyObject_GC_New(typevartupleobject, tp);
    if (tvt == NULL) {
        return NULL;
    }
    tvt->name = Py_NewRef(name);
    tvt->default_value = Py_XNewRef(default_value);
    tvt->evaluate_default = NULL;
    _PyObject_GC_TRACK(tvt);
    if (module != NULL) {
        if (PyObject_SetAttrString((PyObject *)tvt, "__module__", module) < 0) {
            Py_DECREF(tvt);
            return NULL;
        }
    }
    return tvt;
}

/*[clinic input]
@classmethod
typevartuple.__new__

    name: object(subclass_of="&PyUnicode_Type")
    *
    default as default_value: object(c_default="&_Py_NoDefaultStruct") = typing.NoDefault

Create a new TypeVarTuple with the given name.
[clinic start generated code]*/

static PyObject *
typevartuple_impl(PyTypeObject *type, PyObject *name,
                  PyObject *default_value)
/*[clinic end generated code: output=9d6b76dfe95aae51 input=e149739929a866d0]*/
{
    PyObject *module = caller();
    if (module == NULL) {
        return NULL;
    }
    PyObject *result = (PyObject *)typevartuple_alloc(name, module, default_value);
    Py_DECREF(module);
    return result;
}

/*[clinic input]
typevartuple.__typing_subst__ as typevartuple_typing_subst

    arg: object
    /

[clinic start generated code]*/

static PyObject *
typevartuple_typing_subst_impl(typevartupleobject *self, PyObject *arg)
/*[clinic end generated code: output=814316519441cd76 input=3fcf2dfd9eee7945]*/
{
    PyErr_SetString(PyExc_TypeError, "Substitution of bare TypeVarTuple is not supported");
    return NULL;
}

/*[clinic input]
typevartuple.__typing_prepare_subst__ as typevartuple_typing_prepare_subst

    alias: object
    args: object
    /

[clinic start generated code]*/

static PyObject *
typevartuple_typing_prepare_subst_impl(typevartupleobject *self,
                                       PyObject *alias, PyObject *args)
/*[clinic end generated code: output=ff999bc5b02036c1 input=685b149b0fc47556]*/
{
    PyObject *args_array[3] = {(PyObject *)self, alias, args};
    PyObject *result = call_typing_func_object(
        "_typevartuple_prepare_subst", args_array, 3);
    return result;
}

/*[clinic input]
typevartuple.__reduce__ as typevartuple_reduce

[clinic start generated code]*/

static PyObject *
typevartuple_reduce_impl(typevartupleobject *self)
/*[clinic end generated code: output=3215bc0477913d20 input=3018a4d66147e807]*/
{
    return Py_NewRef(self->name);
}


/*[clinic input]
typevartuple.has_default as typevartuple_has_default

[clinic start generated code]*/

static PyObject *
typevartuple_has_default_impl(typevartupleobject *self)
/*[clinic end generated code: output=4895f602f56a5e29 input=9ef3250ddb2c1851]*/
{
    if (self->evaluate_default != NULL ||
        (self->default_value != &_Py_NoDefaultStruct && self->default_value != NULL)) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyObject *
typevartuple_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of TypeVarTuple");
    return NULL;
}

static int
typevartuple_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    typevartupleobject *tvt = typevartupleobject_CAST(self);
    Py_VISIT(tvt->default_value);
    Py_VISIT(tvt->evaluate_default);
    PyObject_VisitManagedDict(self, visit, arg);
    return 0;
}

static int
typevartuple_clear(PyObject *self)
{
    typevartupleobject *tvt = typevartupleobject_CAST(self);
    Py_CLEAR(tvt->default_value);
    Py_CLEAR(tvt->evaluate_default);
    PyObject_ClearManagedDict(self);
    return 0;
}

static PyObject *
typevartuple_default(PyObject *op, void *Py_UNUSED(closure))
{
    typevartupleobject *self = typevartupleobject_CAST(op);
    if (self->default_value != NULL) {
        return Py_NewRef(self->default_value);
    }
    if (self->evaluate_default == NULL) {
        return &_Py_NoDefaultStruct;
    }
    PyObject *default_value = PyObject_CallNoArgs(self->evaluate_default);
    self->default_value = Py_XNewRef(default_value);
    return default_value;
}

static PyObject *
typevartuple_evaluate_default(PyObject *op, void *Py_UNUSED(closure))
{
    typevartupleobject *self = typevartupleobject_CAST(op);
    if (self->evaluate_default != NULL) {
        return Py_NewRef(self->evaluate_default);
    }
    if (self->default_value != NULL) {
        return constevaluator_alloc(self->default_value);
    }
    Py_RETURN_NONE;
}

static PyGetSetDef typevartuple_getset[] = {
    {"__default__", typevartuple_default, NULL, "The default value for this TypeVarTuple.", NULL},
    {"evaluate_default", typevartuple_evaluate_default, NULL, NULL, NULL},
    {0},
};

static PyMethodDef typevartuple_methods[] = {
    TYPEVARTUPLE_TYPING_SUBST_METHODDEF
    TYPEVARTUPLE_TYPING_PREPARE_SUBST_METHODDEF
    TYPEVARTUPLE_REDUCE_METHODDEF
    TYPEVARTUPLE_HAS_DEFAULT_METHODDEF
    {"__mro_entries__", typevartuple_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(typevartuple_doc,
"Type variable tuple. A specialized form of type variable that enables\n\
variadic generics.\n\
\n\
The preferred way to construct a type variable tuple is via the\n\
dedicated syntax for generic functions, classes, and type aliases,\n\
where a single '*' indicates a type variable tuple::\n\
\n\
    def move_first_element_to_last[T, *Ts](tup: tuple[T, *Ts]) -> tuple[*Ts, T]:\n\
        return (*tup[1:], tup[0])\n\
\n\
Type variables tuples can have default values:\n\
\n\
    type AliasWithDefault[*Ts = (str, int)] = tuple[*Ts]\n\
\n\
For compatibility with Python 3.11 and earlier, TypeVarTuple objects\n\
can also be created as follows::\n\
\n\
    Ts = TypeVarTuple('Ts')  # Can be given any name\n\
    DefaultTs = TypeVarTuple('Ts', default=(str, int))\n\
\n\
Just as a TypeVar (type variable) is a placeholder for a single type,\n\
a TypeVarTuple is a placeholder for an *arbitrary* number of types. For\n\
example, if we define a generic class using a TypeVarTuple::\n\
\n\
    class C[*Ts]: ...\n\
\n\
Then we can parameterize that class with an arbitrary number of type\n\
arguments::\n\
\n\
    C[int]       # Fine\n\
    C[int, str]  # Also fine\n\
    C[()]        # Even this is fine\n\
\n\
For more details, see PEP 646.\n\
\n\
Note that only TypeVarTuples defined in the global scope can be\n\
pickled.\n\
");

PyType_Slot typevartuple_slots[] = {
    {Py_tp_doc, (void *)typevartuple_doc},
    {Py_tp_members, typevartuple_members},
    {Py_tp_methods, typevartuple_methods},
    {Py_tp_getset, typevartuple_getset},
    {Py_tp_new, typevartuple},
    {Py_tp_iter, unpack_iter},
    {Py_tp_repr, typevartuple_repr},
    {Py_tp_dealloc, typevartuple_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, typevartuple_traverse},
    {Py_tp_clear, typevartuple_clear},
    {0, 0},
};

PyType_Spec typevartuple_spec = {
    .name = "typing.TypeVarTuple",
    .basicsize = sizeof(typevartupleobject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_MANAGED_DICT
        | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_MANAGED_WEAKREF,
    .slots = typevartuple_slots,
};

PyObject *
_Py_make_typevar(PyObject *name, PyObject *evaluate_bound, PyObject *evaluate_constraints)
{
    return (PyObject *)typevar_alloc(name, NULL, evaluate_bound, NULL, evaluate_constraints,
                                     NULL, false, false, true, NULL);
}

PyObject *
_Py_make_paramspec(PyThreadState *Py_UNUSED(ignored), PyObject *v)
{
    assert(PyUnicode_Check(v));
    return (PyObject *)paramspec_alloc(v, NULL, NULL, false, false, true, NULL);
}

PyObject *
_Py_make_typevartuple(PyThreadState *Py_UNUSED(ignored), PyObject *v)
{
    assert(PyUnicode_Check(v));
    return (PyObject *)typevartuple_alloc(v, NULL, NULL);
}

static PyObject *
get_type_param_default(PyThreadState *ts, PyObject *typeparam) {
    // Does not modify refcount of existing objects.
    if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.typevar_type)) {
        return typevar_default(typeparam, NULL);
    }
    else if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.paramspec_type)) {
        return paramspec_default(typeparam, NULL);
    }
    else if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.typevartuple_type)) {
        return typevartuple_default(typeparam, NULL);
    }
    else {
        PyErr_Format(PyExc_TypeError, "Expected a type param, got %R", typeparam);
        return NULL;
    }
}

static void
typealias_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    typealiasobject *ta = typealiasobject_CAST(self);
    Py_DECREF(ta->name);
    Py_XDECREF(ta->type_params);
    Py_XDECREF(ta->compute_value);
    Py_XDECREF(ta->value);
    Py_XDECREF(ta->module);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
typealias_get_value(typealiasobject *ta)
{
    if (ta->value != NULL) {
        return Py_NewRef(ta->value);
    }
    PyObject *result = PyObject_CallNoArgs(ta->compute_value);
    if (result == NULL) {
        return NULL;
    }
    ta->value = Py_NewRef(result);
    return result;
}

static PyObject *
typealias_repr(PyObject *self)
{
    typealiasobject *ta = (typealiasobject *)self;
    return Py_NewRef(ta->name);
}

static PyMemberDef typealias_members[] = {
    {"__name__", _Py_T_OBJECT, offsetof(typealiasobject, name), Py_READONLY},
    {0}
};

static PyObject *
typealias_value(PyObject *self, void *Py_UNUSED(closure))
{
    typealiasobject *ta = typealiasobject_CAST(self);
    return typealias_get_value(ta);
}

static PyObject *
typealias_evaluate_value(PyObject *self, void *Py_UNUSED(closure))
{
    typealiasobject *ta = typealiasobject_CAST(self);
    if (ta->compute_value != NULL) {
        return Py_NewRef(ta->compute_value);
    }
    assert(ta->value != NULL);
    return constevaluator_alloc(ta->value);
}

static PyObject *
typealias_parameters(PyObject *self, void *Py_UNUSED(closure))
{
    typealiasobject *ta = typealiasobject_CAST(self);
    if (ta->type_params == NULL) {
        return PyTuple_New(0);
    }
    return unpack_typevartuples(ta->type_params);
}

static PyObject *
typealias_type_params(PyObject *self, void *Py_UNUSED(closure))
{
    typealiasobject *ta = typealiasobject_CAST(self);
    if (ta->type_params == NULL) {
        return PyTuple_New(0);
    }
    return Py_NewRef(ta->type_params);
}

static PyObject *
typealias_module(PyObject *self, void *Py_UNUSED(closure))
{
    typealiasobject *ta = typealiasobject_CAST(self);
    if (ta->module != NULL) {
        return Py_NewRef(ta->module);
    }
    if (ta->compute_value != NULL) {
        PyObject* mod = PyFunction_GetModule(ta->compute_value);
        if (mod != NULL) {
            // PyFunction_GetModule() returns a borrowed reference,
            // and it may return NULL (e.g., for functions defined
            // in an exec()'ed block).
            return Py_NewRef(mod);
        }
    }
    Py_RETURN_NONE;
}

static PyGetSetDef typealias_getset[] = {
    {"__parameters__", typealias_parameters, NULL, NULL, NULL},
    {"__type_params__", typealias_type_params, NULL, NULL, NULL},
    {"__value__", typealias_value, NULL, NULL, NULL},
    {"evaluate_value", typealias_evaluate_value, NULL, NULL, NULL},
    {"__module__", typealias_module, NULL, NULL, NULL},
    {0}
};

static PyObject *
typealias_check_type_params(PyObject *type_params, int *err) {
    // Can return type_params or NULL without exception set.
    // Does not change the reference count of type_params,
    // sets `*err` to 1 when error happens and sets an exception,
    // otherwise `*err` is set to 0.
    *err = 0;
    if (type_params == NULL) {
        return NULL;
    }

    assert(PyTuple_Check(type_params));
    Py_ssize_t length = PyTuple_GET_SIZE(type_params);
    if (!length) {  // 0-length tuples are the same as `NULL`.
        return NULL;
    }

    PyThreadState *ts = _PyThreadState_GET();
    int default_seen = 0;
    for (Py_ssize_t index = 0; index < length; index++) {
        PyObject *type_param = PyTuple_GET_ITEM(type_params, index);
        PyObject *dflt = get_type_param_default(ts, type_param);
        if (dflt == NULL) {
            *err = 1;
            return NULL;
        }
        if (dflt == &_Py_NoDefaultStruct) {
            if (default_seen) {
                *err = 1;
                PyErr_Format(PyExc_TypeError,
                                "non-default type parameter '%R' "
                                "follows default type parameter",
                                type_param);
                return NULL;
            }
        } else {
            default_seen = 1;
            Py_DECREF(dflt);
        }
    }

    return type_params;
}

static PyObject *
typelias_convert_type_params(PyObject *type_params)
{
    if (
        type_params == NULL
        || Py_IsNone(type_params)
        || (PyTuple_Check(type_params) && PyTuple_GET_SIZE(type_params) == 0)
    ) {
        return NULL;
    }
    else {
        return type_params;
    }
}

static typealiasobject *
typealias_alloc(PyObject *name, PyObject *type_params, PyObject *compute_value,
                PyObject *value, PyObject *module)
{
    typealiasobject *ta = PyObject_GC_New(typealiasobject, &_PyTypeAlias_Type);
    if (ta == NULL) {
        return NULL;
    }
    ta->name = Py_NewRef(name);
    ta->type_params = Py_XNewRef(type_params);
    ta->compute_value = Py_XNewRef(compute_value);
    ta->value = Py_XNewRef(value);
    ta->module = Py_XNewRef(module);
    _PyObject_GC_TRACK(ta);
    return ta;
}

static int
typealias_traverse(PyObject *op, visitproc visit, void *arg)
{
    typealiasobject *self = typealiasobject_CAST(op);
    Py_VISIT(self->type_params);
    Py_VISIT(self->compute_value);
    Py_VISIT(self->value);
    Py_VISIT(self->module);
    return 0;
}

static int
typealias_clear(PyObject *op)
{
    typealiasobject *self = typealiasobject_CAST(op);
    Py_CLEAR(self->type_params);
    Py_CLEAR(self->compute_value);
    Py_CLEAR(self->value);
    Py_CLEAR(self->module);
    return 0;
}

/*[clinic input]
typealias.__reduce__ as typealias_reduce

[clinic start generated code]*/

static PyObject *
typealias_reduce_impl(typealiasobject *self)
/*[clinic end generated code: output=913724f92ad3b39b input=4f06fbd9472ec0f1]*/
{
    return Py_NewRef(self->name);
}

static PyObject *
typealias_subscript(PyObject *op, PyObject *args)
{
    typealiasobject *self = typealiasobject_CAST(op);
    if (self->type_params == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Only generic type aliases are subscriptable");
        return NULL;
    }
    return Py_GenericAlias(op, args);
}

static PyMethodDef typealias_methods[] = {
    TYPEALIAS_REDUCE_METHODDEF
    {0}
};


/*[clinic input]
@classmethod
typealias.__new__ as typealias_new

    name: object(subclass_of="&PyUnicode_Type")
    value: object
    *
    type_params: object = NULL

Create a TypeAliasType.
[clinic start generated code]*/

static PyObject *
typealias_new_impl(PyTypeObject *type, PyObject *name, PyObject *value,
                   PyObject *type_params)
/*[clinic end generated code: output=8920ce6bdff86f00 input=df163c34e17e1a35]*/
{
    if (type_params != NULL && !PyTuple_Check(type_params)) {
        PyErr_SetString(PyExc_TypeError, "type_params must be a tuple");
        return NULL;
    }

    int err = 0;
    PyObject *checked_params = typealias_check_type_params(type_params, &err);
    if (err) {
        return NULL;
    }

    PyObject *module = caller();
    if (module == NULL) {
        return NULL;
    }
    PyObject *ta = (PyObject *)typealias_alloc(name, checked_params, NULL, value,
                                               module);
    Py_DECREF(module);
    return ta;
}

PyDoc_STRVAR(typealias_doc,
"Type alias.\n\
\n\
Type aliases are created through the type statement::\n\
\n\
    type Alias = int\n\
\n\
In this example, Alias and int will be treated equivalently by static\n\
type checkers.\n\
\n\
At runtime, Alias is an instance of TypeAliasType. The __name__\n\
attribute holds the name of the type alias. The value of the type alias\n\
is stored in the __value__ attribute. It is evaluated lazily, so the\n\
value is computed only if the attribute is accessed.\n\
\n\
Type aliases can also be generic::\n\
\n\
    type ListOrSet[T] = list[T] | set[T]\n\
\n\
In this case, the type parameters of the alias are stored in the\n\
__type_params__ attribute.\n\
\n\
See PEP 695 for more information.\n\
");

static PyNumberMethods typealias_as_number = {
    .nb_or = _Py_union_type_or,
};

static PyMappingMethods typealias_as_mapping = {
    .mp_subscript = typealias_subscript,
};

PyTypeObject _PyTypeAlias_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.TypeAliasType",
    .tp_basicsize = sizeof(typealiasobject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc = typealias_doc,
    .tp_members = typealias_members,
    .tp_methods = typealias_methods,
    .tp_getset = typealias_getset,
    .tp_alloc = PyType_GenericAlloc,
    .tp_dealloc = typealias_dealloc,
    .tp_new = typealias_new,
    .tp_free = PyObject_GC_Del,
    .tp_iter = unpack_iter,
    .tp_traverse = typealias_traverse,
    .tp_clear = typealias_clear,
    .tp_repr = typealias_repr,
    .tp_as_number = &typealias_as_number,
    .tp_as_mapping = &typealias_as_mapping,
};

PyObject *
_Py_make_typealias(PyThreadState* unused, PyObject *args)
{
    assert(PyTuple_Check(args));
    assert(PyTuple_GET_SIZE(args) == 3);
    PyObject *name = PyTuple_GET_ITEM(args, 0);
    assert(PyUnicode_Check(name));
    PyObject *type_params = typelias_convert_type_params(PyTuple_GET_ITEM(args, 1));
    PyObject *compute_value = PyTuple_GET_ITEM(args, 2);
    assert(PyFunction_Check(compute_value));
    return (PyObject *)typealias_alloc(name, type_params, compute_value, NULL, NULL);
}

PyDoc_STRVAR(generic_doc,
"Abstract base class for generic types.\n\
\n\
On Python 3.12 and newer, generic classes implicitly inherit from\n\
Generic when they declare a parameter list after the class's name::\n\
\n\
    class Mapping[KT, VT]:\n\
        def __getitem__(self, key: KT) -> VT:\n\
            ...\n\
        # Etc.\n\
\n\
On older versions of Python, however, generic classes have to\n\
explicitly inherit from Generic.\n\
\n\
After a class has been declared to be generic, it can then be used as\n\
follows::\n\
\n\
    def lookup_name[KT, VT](mapping: Mapping[KT, VT], key: KT, default: VT) -> VT:\n\
        try:\n\
            return mapping[key]\n\
        except KeyError:\n\
            return default\n\
");

PyDoc_STRVAR(generic_class_getitem_doc,
"Parameterizes a generic class.\n\
\n\
At least, parameterizing a generic class is the *main* thing this\n\
method does. For example, for some generic class `Foo`, this is called\n\
when we do `Foo[int]` - there, with `cls=Foo` and `params=int`.\n\
\n\
However, note that this method is also called when defining generic\n\
classes in the first place with `class Foo[T]: ...`.\n\
");

static PyObject *
call_typing_args_kwargs(const char *name, PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    PyObject *typing = NULL, *func = NULL, *new_args = NULL;
    typing = PyImport_ImportModule("typing");
    if (typing == NULL) {
        goto error;
    }
    func = PyObject_GetAttrString(typing, name);
    if (func == NULL) {
        goto error;
    }
    assert(PyTuple_Check(args));
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    new_args = PyTuple_New(nargs + 1);
    if (new_args == NULL) {
        goto error;
    }
    PyTuple_SET_ITEM(new_args, 0, Py_NewRef((PyObject *)cls));
    for (Py_ssize_t i = 0; i < nargs; i++) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        PyTuple_SET_ITEM(new_args, i + 1, Py_NewRef(arg));
    }
    PyObject *result = PyObject_Call(func, new_args, kwargs);
    Py_DECREF(typing);
    Py_DECREF(func);
    Py_DECREF(new_args);
    return result;
error:
    Py_XDECREF(typing);
    Py_XDECREF(func);
    Py_XDECREF(new_args);
    return NULL;
}

static PyObject *
generic_init_subclass(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    return call_typing_args_kwargs("_generic_init_subclass",
                                   (PyTypeObject*)cls, args, kwargs);
}

static PyObject *
generic_class_getitem(PyObject *cls, PyObject *args, PyObject *kwargs)
{
    return call_typing_args_kwargs("_generic_class_getitem",
                                   (PyTypeObject*)cls, args, kwargs);
}

PyObject *
_Py_subscript_generic(PyThreadState* unused, PyObject *params)
{
    params = unpack_typevartuples(params);

    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->cached_objects.generic_type == NULL) {
        PyErr_SetString(PyExc_SystemError, "Cannot find Generic type");
        return NULL;
    }
    PyObject *args[2] = {(PyObject *)interp->cached_objects.generic_type, params};
    PyObject *result = call_typing_func_object("_GenericAlias", args, 2);
    Py_DECREF(params);
    return result;
}

static PyMethodDef generic_methods[] = {
    {"__class_getitem__", _PyCFunction_CAST(generic_class_getitem),
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     generic_class_getitem_doc},
    {"__init_subclass__", _PyCFunction_CAST(generic_init_subclass),
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Function to initialize subclasses.")},
    {NULL} /* Sentinel */
};

static void
generic_dealloc(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static int
generic_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyType_Slot generic_slots[] = {
    {Py_tp_doc, (void *)generic_doc},
    {Py_tp_methods, generic_methods},
    {Py_tp_dealloc, generic_dealloc},
    {Py_tp_alloc, PyType_GenericAlloc},
    {Py_tp_free, PyObject_GC_Del},
    {Py_tp_traverse, generic_traverse},
    {0, NULL},
};

PyType_Spec generic_spec = {
    .name = "typing.Generic",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .slots = generic_slots,
};

int _Py_initialize_generic(PyInterpreterState *interp)
{
#define MAKE_TYPE(name) \
    do { \
        PyTypeObject *name ## _type = (PyTypeObject *)PyType_FromSpec(&name ## _spec); \
        if (name ## _type == NULL) { \
            return -1; \
        } \
        interp->cached_objects.name ## _type = name ## _type; \
    } while(0)

    MAKE_TYPE(generic);
    MAKE_TYPE(typevar);
    MAKE_TYPE(typevartuple);
    MAKE_TYPE(paramspec);
    MAKE_TYPE(paramspecargs);
    MAKE_TYPE(paramspeckwargs);
    MAKE_TYPE(constevaluator);
#undef MAKE_TYPE
    return 0;
}

void _Py_clear_generic_types(PyInterpreterState *interp)
{
    Py_CLEAR(interp->cached_objects.generic_type);
    Py_CLEAR(interp->cached_objects.typevar_type);
    Py_CLEAR(interp->cached_objects.typevartuple_type);
    Py_CLEAR(interp->cached_objects.paramspec_type);
    Py_CLEAR(interp->cached_objects.paramspecargs_type);
    Py_CLEAR(interp->cached_objects.paramspeckwargs_type);
    Py_CLEAR(interp->cached_objects.constevaluator_type);
}

PyObject *
_Py_set_typeparam_default(PyThreadState *ts, PyObject *typeparam, PyObject *evaluate_default)
{
    if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.typevar_type)) {
        Py_XSETREF(((typevarobject *)typeparam)->evaluate_default, Py_NewRef(evaluate_default));
        return Py_NewRef(typeparam);
    }
    else if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.paramspec_type)) {
        Py_XSETREF(((paramspecobject *)typeparam)->evaluate_default, Py_NewRef(evaluate_default));
        return Py_NewRef(typeparam);
    }
    else if (Py_IS_TYPE(typeparam, ts->interp->cached_objects.typevartuple_type)) {
        Py_XSETREF(((typevartupleobject *)typeparam)->evaluate_default, Py_NewRef(evaluate_default));
        return Py_NewRef(typeparam);
    }
    else {
        PyErr_Format(PyExc_TypeError, "Expected a type param, got %R", typeparam);
        return NULL;
    }
}
