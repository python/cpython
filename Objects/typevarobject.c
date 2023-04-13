// TypeVar, TypeVarTuple, and ParamSpec
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_typevarobject.h"
#include "structmember.h"

/*[clinic input]
class typevar "typevarobject *" "&_PyTypeVar_Type"
class paramspec "paramspecobject *" "&_PyParamSpec_Type"
class typevartuple "typevartupleobject *" "&_PyTypeVarTuple_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d1b368d1521582f6]*/

#include "clinic/typevarobject.c.h"

typedef struct {
    PyObject_HEAD
    const char *name;
    PyObject *bound;
    PyObject *constraints;
    bool covariant;
    bool contravariant;
    bool autovariance;
} typevarobject;

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

    if (constraints != NULL) {
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
            PyErr_SetString(PyExc_TypeError,
                            "constraints must be a non-empty tuple");
            return NULL;
        }
    }

    return (PyObject *)typevarobject_alloc(name, bound, constraints,
                                           covariant, contravariant,
                                           autovariance);
}

PyTypeObject _PyTypeVar_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.TypeVar",
    .tp_basicsize = sizeof(typevarobject),
    .tp_dealloc = typevarobject_dealloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = typevarobject_traverse,
    .tp_repr = typevarobject_repr,
    .tp_members = typevar_members,
    .tp_new = typevar_new,
};

typedef struct {
    PyObject_HEAD
    const char *name;
    PyObject *bound;
    bool covariant;
    bool contravariant;
    bool autovariance;
} paramspecobject;

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

static paramspecobject *paramspecobject_alloc(const char *name, PyObject *bound, bool covariant,
                                              bool contravariant, bool autovariance)
{
    paramspecobject *ps = PyObject_New(paramspecobject, &_PyParamSpec_Type);
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

// TODO:
// - args/kwargs
// - pickling
// - __typing_prepare_subst__
PyTypeObject _PyParamSpec_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.ParamSpec",
    .tp_basicsize = sizeof(paramspecobject),
    .tp_dealloc = paramspecobject_dealloc,
    .tp_free = PyObject_GC_Del,
    .tp_repr = paramspecobject_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_traverse = paramspecobject_traverse,
    .tp_members = paramspec_members,
    .tp_new = paramspec_new,
};

typedef struct {
    PyObject_HEAD
    const char *name;
} typevartupleobject;

static void typevartupleobject_dealloc(PyObject *self)
{
    typevartupleobject *tvt = (typevartupleobject *)self;

    free((void *)tvt->name);
    Py_TYPE(self)->tp_free(self);
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

// TODO:
// - Iterable (for Unpack)
// - Pickling
// - __typing_subst__/__typing_prepare_subst__
PyTypeObject _PyTypeVarTuple_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.TypeVarTuple",
    .tp_basicsize = sizeof(typevartupleobject),
    .tp_dealloc = typevartupleobject_dealloc,
    .tp_repr = typevartupleobject_repr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE,
    .tp_members = typevartuple_members,
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
