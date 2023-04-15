// TypeVar, TypeVarTuple, and ParamSpec
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_typevarobject.h"
#include "structmember.h"

/*[clinic input]
class typevar "typevarobject *" "&_PyTypeVar_Type"
class paramspec "paramspecobject *" "&_PyParamSpec_Type"
class paramspecargs "paramspecargsobject *" "&_PyParamSpecArgs_Type"
class paramspeckwargs "paramspeckwargsobject *" "&_PyParamSpecKwargs_Type"
class typevartuple "typevartupleobject *" "&_PyTypeVarTuple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=74cb9c15a049111b]*/

typedef struct {
    PyObject_HEAD
    const char *name;
    PyObject *bound;
    PyObject *constraints;
    bool covariant;
    bool contravariant;
    bool autovariance;
} typevarobject;

typedef struct {
    PyObject_HEAD
    const char *name;
} typevartupleobject;

typedef struct {
    PyObject_HEAD
    const char *name;
    PyObject *bound;
    bool covariant;
    bool contravariant;
    bool autovariance;
} paramspecobject;

#include "clinic/typevarobject.c.h"

static void typevarobject_dealloc(PyObject *self)
{
    typevarobject *tv = (typevarobject *)self;

    _PyObject_GC_UNTRACK(self);

    free((void *)tv->name);
    Py_XDECREF(tv->bound);
    Py_XDECREF(tv->constraints);

    Py_TYPE(self)->tp_free(self);
}

static int typevarobject_traverse(PyObject *self, visitproc visit, void *arg)
{
    typevarobject *tv = (typevarobject *)self;
    Py_VISIT(tv->bound);
    Py_VISIT(tv->constraints);
    return 0;
}

static PyObject *typevarobject_repr(PyObject *self)
{
    typevarobject *tv = (typevarobject *)self;

    if (tv->autovariance) {
        return PyUnicode_FromFormat("%s", tv->name);
    }

    char variance = tv->covariant ? '+' : tv->contravariant ? '-' : '~';
    return PyUnicode_FromFormat("%c%s", variance, tv->name);
}

static PyMemberDef typevar_members[] = {
    {"__name__", T_STRING, offsetof(typevarobject, name), READONLY},
    {"__bound__", T_OBJECT, offsetof(typevarobject, bound), READONLY},
    {"__constraints__", T_OBJECT, offsetof(typevarobject, constraints), READONLY},
    {"__covariant__", T_BOOL, offsetof(typevarobject, covariant), READONLY},
    {"__contravariant__", T_BOOL, offsetof(typevarobject, contravariant), READONLY},
    {"__autovariance__", T_BOOL, offsetof(typevarobject, autovariance), READONLY},
    {0}
};

static typevarobject *typevarobject_alloc(const char *name, PyObject *bound,
                                          PyObject *constraints,
                                          bool covariant, bool contravariant,
                                          bool autovariance)
{
    typevarobject *tv = PyObject_GC_New(typevarobject, &_PyTypeVar_Type);
    if (tv == NULL) {
        return NULL;
    }

    tv->name = strdup(name);
    if (tv->name == NULL) {
        Py_DECREF(tv);
        return NULL;
    }

    tv->bound = Py_XNewRef(bound);
    tv->constraints = Py_XNewRef(constraints);

    tv->covariant = covariant;
    tv->contravariant = contravariant;
    tv->autovariance = autovariance;

    _PyObject_GC_TRACK(tv);
    return tv;
}

/*[clinic input]
@classmethod
typevar.__new__ as typevar_new

    name: str
    *constraints: object
    *
    bound: object = None
    covariant: bool = False
    contravariant: bool = False
    autovariance: bool = False

Create a TypeVar.
[clinic start generated code]*/

static PyObject *
typevar_new_impl(PyTypeObject *type, const char *name, PyObject *constraints,
                 PyObject *bound, int covariant, int contravariant,
                 int autovariance)
/*[clinic end generated code: output=e74ea8371ab8103a input=9d08a995b997a11b]*/
{
    if (covariant && contravariant) {
        PyErr_SetString(PyExc_ValueError,
                        "Bivariant types are not supported.");
        return NULL;
    }

    if (autovariance && (covariant || contravariant)) {
        PyErr_SetString(PyExc_ValueError,
                        "Variance cannot be specified with autovariance.");
        return NULL;
    }

    if (!PyTuple_CheckExact(constraints)) {
        PyErr_SetString(PyExc_TypeError,
                        "constraints must be a tuple");
        return NULL;
    }
    Py_ssize_t n_constraints = PyTuple_GET_SIZE(constraints);
    if (n_constraints == 1) {
        PyErr_SetString(PyExc_TypeError,
                        "A single constraint is not allowed");
        return NULL;
    } else if (n_constraints == 0) {
        constraints = NULL;
    }

    if (Py_IsNone(bound)) {
        bound = NULL;
    }

    return (PyObject *)typevarobject_alloc(name, bound, constraints,
                                           covariant, contravariant,
                                           autovariance);
}

/*[clinic input]
typevar.__typing_subst__ as typevar_typing_subst

    arg: object

[clinic start generated code]*/

static PyObject *
typevar_typing_subst_impl(typevarobject *self, PyObject *arg)
/*[clinic end generated code: output=c76ced134ed8f4e1 input=6b70a4bb2da838de]*/
{
    // TODO _type_check
    // TODO reject Unpack[]
    return Py_NewRef(arg);
}

static PyMethodDef typevar_methods[] = {
    TYPEVAR_TYPING_SUBST_METHODDEF
    {0}
};

PyTypeObject _PyTypeVar_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.TypeVar",
    .tp_basicsize = sizeof(typevarobject),
    .tp_dealloc = typevarobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = typevarobject_traverse,
    .tp_repr = typevarobject_repr,
    .tp_members = typevar_members,
    .tp_methods = typevar_methods,
    .tp_new = typevar_new,
};

typedef struct {
    PyObject_HEAD
    PyObject *__origin__;
} paramspecargsobject;

static void paramspecargsobject_dealloc(PyObject *self)
{
    paramspecargsobject *psa = (paramspecargsobject *)self;

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(psa->__origin__);

    Py_TYPE(self)->tp_free(self);
}

static int paramspecargsobject_traverse(PyObject *self, visitproc visit, void *arg)
{
    paramspecargsobject *psa = (paramspecargsobject *)self;
    Py_VISIT(psa->__origin__);
    return 0;
}

static PyObject *paramspecargsobject_repr(PyObject *self)
{
    paramspecargsobject *psa = (paramspecargsobject *)self;

    return PyUnicode_FromFormat("%R.args", psa->__origin__);
}

static PyObject *paramspecargsobject_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!Py_IS_TYPE(b, &_PyParamSpecArgs_Type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return PyObject_RichCompare(
        ((paramspecargsobject *)a)->__origin__,
        ((paramspecargsobject *)b)->__origin__,
        op
    );
}

static PyMemberDef paramspecargs_members[] = {
    {"__origin__", T_OBJECT, offsetof(paramspecargsobject, __origin__), READONLY},
    {0}
};

static paramspecargsobject *paramspecargsobject_new(PyObject *origin)
{
    paramspecargsobject *psa = PyObject_GC_New(paramspecargsobject, &_PyParamSpecArgs_Type);
    if (psa == NULL) {
        return NULL;
    }
    psa->__origin__ = Py_NewRef(origin);
    _PyObject_GC_TRACK(psa);
    return psa;
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
    return (PyObject *)paramspecargsobject_new(origin);
}

PyTypeObject _PyParamSpecArgs_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.ParamSpecArgs",
    .tp_basicsize = sizeof(paramspecargsobject),
    .tp_dealloc = paramspecargsobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_repr = paramspecargsobject_repr,
    .tp_richcompare = paramspecargsobject_richcompare,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = paramspecargsobject_traverse,
    .tp_members = paramspecargs_members,
    .tp_new = paramspecargs_new,
};

typedef struct {
    PyObject_HEAD
    PyObject *__origin__;
} paramspeckwargsobject;

static void paramspeckwargsobject_dealloc(PyObject *self)
{
    paramspeckwargsobject *psk = (paramspeckwargsobject *)self;

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(psk->__origin__);

    Py_TYPE(self)->tp_free(self);
}

static int paramspeckwargsobject_traverse(PyObject *self, visitproc visit, void *arg)
{
    paramspeckwargsobject *psk = (paramspeckwargsobject *)self;
    Py_VISIT(psk->__origin__);
    return 0;
}

static PyObject *paramspeckwargsobject_repr(PyObject *self)
{
    paramspeckwargsobject *psk = (paramspeckwargsobject *)self;

    return PyUnicode_FromFormat("%R.kwargs", psk->__origin__);
}

static PyObject *paramspeckwargsobject_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!Py_IS_TYPE(b, &_PyParamSpecKwargs_Type)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return PyObject_RichCompare(
        ((paramspeckwargsobject *)a)->__origin__,
        ((paramspeckwargsobject *)b)->__origin__,
        op
    );
}

static PyMemberDef paramspeckwargs_members[] = {
    {"__origin__", T_OBJECT, offsetof(paramspeckwargsobject, __origin__), READONLY},
    {0}
};

static paramspeckwargsobject *paramspeckwargsobject_new(PyObject *origin)
{
    paramspeckwargsobject *psk = PyObject_GC_New(paramspeckwargsobject, &_PyParamSpecKwargs_Type);
    if (psk == NULL) {
        return NULL;
    }
    psk->__origin__ = Py_NewRef(origin);
    _PyObject_GC_TRACK(psk);
    return psk;
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
    return (PyObject *)paramspeckwargsobject_new(origin);
}

PyTypeObject _PyParamSpecKwargs_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.ParamSpecKwargs",
    .tp_basicsize = sizeof(paramspeckwargsobject),
    .tp_dealloc = paramspeckwargsobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_repr = paramspeckwargsobject_repr,
    .tp_richcompare = paramspeckwargsobject_richcompare,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = paramspeckwargsobject_traverse,
    .tp_members = paramspeckwargs_members,
    .tp_new = paramspeckwargs_new,
};

static void paramspecobject_dealloc(PyObject *self)
{
    paramspecobject *ps = (paramspecobject *)self;

    _PyObject_GC_UNTRACK(self);

    free((void *)ps->name);
    Py_XDECREF(ps->bound);

    Py_TYPE(self)->tp_free(self);
}

static int paramspecobject_traverse(PyObject *self, visitproc visit, void *arg)
{
    paramspecobject *ps = (paramspecobject *)self;
    Py_VISIT(ps->bound);
    return 0;
}

static PyObject *paramspecobject_repr(PyObject *self)
{
    paramspecobject *ps = (paramspecobject *)self;

    if (ps->autovariance) {
        return PyUnicode_FromFormat("%s", ps->name);
    }

    char variance = ps->covariant ? '+' : ps->contravariant ? '-' : '~';
    return PyUnicode_FromFormat("%c%s", variance, ps->name);
}

static PyMemberDef paramspec_members[] = {
    {"__name__", T_STRING, offsetof(paramspecobject, name), READONLY},
    {"__bound__", T_OBJECT, offsetof(paramspecobject, bound), READONLY},
    {"__covariant__", T_BOOL, offsetof(paramspecobject, covariant), READONLY},
    {"__contravariant__", T_BOOL, offsetof(paramspecobject, contravariant), READONLY},
    {"__autovariance__", T_BOOL, offsetof(paramspecobject, autovariance), READONLY},
    {0}
};

static PyObject *paramspecobject_args(PyObject *self, void *unused)
{
    return (PyObject *)paramspecargsobject_new(self);
}

static PyObject *paramspecobject_kwargs(PyObject *self, void *unused)
{
    return (PyObject *)paramspeckwargsobject_new(self);
}

static PyGetSetDef paramspec_getset[] = {
    {"args", (getter)paramspecobject_args, NULL, "Represents positional arguments.", NULL},
    {"kwargs", (getter)paramspecobject_kwargs, NULL, "Represents keyword arguments.", NULL},
    {0},
};

static paramspecobject *paramspecobject_alloc(const char *name, PyObject *bound, bool covariant,
                                              bool contravariant, bool autovariance)
{
    paramspecobject *ps = PyObject_GC_New(paramspecobject, &_PyParamSpec_Type);
    if (ps == NULL) {
        return NULL;
    }
    ps->name = strdup(name);
    if (ps->name == NULL) {
        Py_DECREF(ps);
        return NULL;
    }
    ps->bound = Py_NewRef(bound);
    ps->covariant = covariant;
    ps->contravariant = contravariant;
    ps->autovariance = autovariance;
    _PyObject_GC_TRACK(ps);
    return ps;
}

/*[clinic input]
@classmethod
paramspec.__new__ as paramspec_new

    name: str
    *
    bound: object = None
    covariant: bool = False
    contravariant: bool = False
    autovariance: bool = False

Create a ParamSpec object.
[clinic start generated code]*/

static PyObject *
paramspec_new_impl(PyTypeObject *type, const char *name, PyObject *bound,
                   int covariant, int contravariant, int autovariance)
/*[clinic end generated code: output=d1693f0a5be7d69d input=09c84e07c5c0722e]*/
{
    if (covariant && contravariant) {
        PyErr_SetString(PyExc_ValueError, "Bivariant types are not supported.");
        return NULL;
    }
    if (autovariance && (covariant || contravariant)) {
        PyErr_SetString(PyExc_ValueError, "Variance cannot be specified with autovariance.");
        return NULL;
    }
    // TODO typing._type_check on bound
    return (PyObject *)paramspecobject_alloc(name, bound, covariant, contravariant, autovariance);
}


/*[clinic input]
paramspec.__typing_subst__ as paramspec_typing_subst

    arg: object

[clinic start generated code]*/

static PyObject *
paramspec_typing_subst_impl(paramspecobject *self, PyObject *arg)
/*[clinic end generated code: output=803e1ade3f13b57d input=4e0005d24023e896]*/
{
    if (PyTuple_Check(arg)) {
        return Py_NewRef(arg);
    } else if (PyList_Check(arg)) {
        return PyList_AsTuple(arg);
    } else {
        // TODO: typing._is_param_expr
        // (in particular _ConcatenateGenericAlias)
        return Py_NewRef(arg);
    }
}

static PyMethodDef paramspec_methods[] = {
    PARAMSPEC_TYPING_SUBST_METHODDEF
    {0}
};


// TODO:
// - args/kwargs
// - pickling
// - __typing_prepare_subst__
PyTypeObject _PyParamSpec_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.ParamSpec",
    .tp_basicsize = sizeof(paramspecobject),
    .tp_dealloc = paramspecobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_repr = paramspecobject_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = paramspecobject_traverse,
    .tp_members = paramspec_members,
    .tp_methods = paramspec_methods,
    .tp_getset = paramspec_getset,
    .tp_new = paramspec_new,
};

static void typevartupleobject_dealloc(PyObject *self)
{
    typevartupleobject *tvt = (typevartupleobject *)self;

    free((void *)tvt->name);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *typevartupleobject_iter(PyObject *self)
{
    PyObject *typing = NULL;
    PyObject *unpack = NULL;
    PyObject *unpacked = NULL;
    PyObject *tuple = NULL;
    PyObject *result = NULL;

    typing = PyImport_ImportModule("typing");
    if (typing == NULL) {
        goto exit;
    }
    unpack = PyObject_GetAttrString(typing, "Unpack");
    if (unpack == NULL) {
        goto exit;
    }
    unpacked = PyObject_GetItem(unpack, self);
    if (unpacked == NULL) {
        goto exit;
    }
    tuple = PyTuple_Pack(1, unpacked);
    if (tuple == NULL) {
        goto exit;
    }
    result = PyObject_GetIter(tuple);
exit:
    Py_XDECREF(typing);
    Py_XDECREF(unpack);
    Py_XDECREF(unpacked);
    Py_XDECREF(tuple);
    return result;
}

static PyObject *typevartupleobject_repr(PyObject *self)
{
    typevartupleobject *tvt = (typevartupleobject *)self;

    return PyUnicode_FromFormat("%s", tvt->name);
}

static PyMemberDef typevartuple_members[] = {
    {"__name__", T_STRING, offsetof(typevartupleobject, name), READONLY},
    {0}
};

static typevartupleobject *typevartupleobject_alloc(const char *name)
{
    typevartupleobject *tvt = PyObject_New(typevartupleobject, &_PyTypeVarTuple_Type);
    if (tvt == NULL) {
        return NULL;
    }
    tvt->name = strdup(name);
    if (tvt->name == NULL) {
        Py_DECREF(tvt);
        return NULL;
    }
    return tvt;
}

/*[clinic input]
@classmethod
typevartuple.__new__

    name: str

Create a new TypeVarTuple with the given name.
[clinic start generated code]*/

static PyObject *
typevartuple_impl(PyTypeObject *type, const char *name)
/*[clinic end generated code: output=a5a5bc3437a27749 input=d89424a0e967cee6]*/
{
    return (PyObject *)typevartupleobject_alloc(name);
}

/*[clinic input]
typevartuple.__typing_subst__ as typevartuple_typing_subst

    arg: object

[clinic start generated code]*/

static PyObject *
typevartuple_typing_subst_impl(typevartupleobject *self, PyObject *arg)
/*[clinic end generated code: output=814316519441cd76 input=670c4e0a36e5d8c0]*/
{
    PyErr_SetString(PyExc_TypeError, "Substitution of bare TypeVarTuple is not supported");
    return NULL;
}

static PyMethodDef typevartuple_methods[] = {
    TYPEVARTUPLE_TYPING_SUBST_METHODDEF
    {0}
};

// TODO:
// - Iterable (for Unpack)
// - Pickling
// - __typing_subst__/__typing_prepare_subst__
PyTypeObject _PyTypeVarTuple_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.TypeVarTuple",
    .tp_basicsize = sizeof(typevartupleobject),
    .tp_dealloc = typevartupleobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_iter = typevartupleobject_iter,
    .tp_repr = typevartupleobject_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_members = typevartuple_members,
    .tp_methods = typevartuple_methods,
    .tp_new = typevartuple,
};

PyObject *_Py_make_typevar(const char *name, PyObject *bound_or_constraints) {
    if (PyTuple_CheckExact(bound_or_constraints)) {
        return (PyObject *)typevarobject_alloc(name, NULL, bound_or_constraints, false, false, true);
    } else {
        return (PyObject *)typevarobject_alloc(name, bound_or_constraints, NULL, false, false, true);
    }
}

PyObject *_Py_make_paramspec(const char *name) {
    return (PyObject *)paramspecobject_alloc(name, NULL, false, false, true);
}

PyObject *_Py_make_typevartuple(const char *name) {
    return (PyObject *)typevartupleobject_alloc(name);
}
