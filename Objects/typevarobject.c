// TypeVar, TypeVarTuple, and ParamSpec
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_typevarobject.h"
#include "pycore_unionobject.h"   // _Py_union_type_or
#include "structmember.h"

/*[clinic input]
class typevar "typevarobject *" "&_PyTypeVar_Type"
class paramspec "paramspecobject *" "&_PyParamSpec_Type"
class paramspecargs "paramspecargsobject *" "&_PyParamSpecArgs_Type"
class paramspeckwargs "paramspeckwargsobject *" "&_PyParamSpecKwargs_Type"
class typevartuple "typevartupleobject *" "&_PyTypeVarTuple_Type"
class Generic "PyObject *" "&PyGeneric_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9b0f1a94d4a27c0c]*/

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

static PyObject *call_typing_func_object(const char *name, PyObject *args) {
    PyObject *typing = PyImport_ImportModule("typing");
    if (typing == NULL) {
        return NULL;
    }
    PyObject *func = PyObject_GetAttrString(typing, name);
    if (func == NULL) {
        Py_DECREF(typing);
        return NULL;
    }
    PyObject *result = PyObject_CallObject(func, args);
    Py_DECREF(func);
    Py_DECREF(typing);
    return result;
}

static PyObject *type_check(PyObject *arg) {
    // Calling typing.py here leads to bootstrapping problems
    if (Py_IsNone(arg)) {
        return Py_NewRef(Py_TYPE(arg));
    }
    // TODO: real error message
    PyObject *args = PyTuple_Pack(2, arg, Py_None);
    if (args == NULL) {
        return NULL;
    }
    return call_typing_func_object("_type_check", args);
}

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

    if (Py_IsNone(bound)) {
        bound = NULL;
    }
    if (bound != NULL) {
        bound = type_check(bound);
        if (bound == NULL) {
            return NULL;
        }
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

    PyObject *tv = (PyObject *)typevarobject_alloc(name, bound, constraints,
                                                   covariant, contravariant,
                                                   autovariance);
    Py_XDECREF(bound);
    return tv;
}

/*[clinic input]
typevar.__typing_subst__ as typevar_typing_subst

    arg: object

[clinic start generated code]*/

static PyObject *
typevar_typing_subst_impl(typevarobject *self, PyObject *arg)
/*[clinic end generated code: output=c76ced134ed8f4e1 input=6b70a4bb2da838de]*/
{
    PyObject *args = PyTuple_Pack(2, self, arg);
    if (args == NULL) {
        return NULL;
    }
    return call_typing_func_object("_typevar_subst", args);
}

/*[clinic input]
typevar.__reduce__ as typevar_reduce

[clinic start generated code]*/

static PyObject *
typevar_reduce_impl(typevarobject *self)
/*[clinic end generated code: output=02e5c55d7cf8a08f input=de76bc95f04fb9ff]*/
{
    return PyUnicode_FromString(self->name);
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
    TYPEVAR_REDUCE_METHODDEF
    {"__mro_entries__", typevar_mro_entries, METH_O},
    {0}
};

static PyNumberMethods typevar_as_number = {
    .nb_or = _Py_union_type_or, // Add __or__ function
};

PyDoc_STRVAR(typevar_doc,
"Type variable.\n\
\n\
Usage::\n\
\n\
  T = TypeVar('T')  # Can be anything\n\
  A = TypeVar('A', str, bytes)  # Must be str or bytes\n\
\n\
Type variables exist primarily for the benefit of static type\n\
checkers.  They serve as the parameters for generic types as well\n\
as for generic function definitions.  See class Generic for more\n\
information on generic types.  Generic functions work as follows:\n\
\n\
  def repeat(x: T, n: int) -> List[T]:\n\
      '''Return a list containing n references to x.'''\n\
      return [x]*n\n\
\n\
  def longest(x: A, y: A) -> A:\n\
      '''Return the longest of two strings.'''\n\
      return x if len(x) >= len(y) else y\n\
\n\
The latter example's signature is essentially the overloading\n\
of (str, str) -> str and (bytes, bytes) -> bytes.  Also note\n\
that if the arguments are instances of some subclass of str,\n\
the return type is still plain str.\n\
\n\
At runtime, isinstance(x, T) and issubclass(C, T) will raise TypeError.\n\
\n\
Type variables defined with covariant=True or contravariant=True\n\
can be used to declare covariant or contravariant generic types.\n\
See PEP 484 for more details. By default generic types are invariant\n\
in all type variables.\n\
\n\
Type variables can be introspected. e.g.:\n\
\n\
  T.__name__ == 'T'\n\
  T.__constraints__ == ()\n\
  T.__covariant__ == False\n\
  T.__contravariant__ = False\n\
  A.__constraints__ == (str, bytes)\n\
\n\
Note that only type variables defined in global scope can be pickled.\n\
");

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
    .tp_as_number = &typevar_as_number,
    .tp_doc = typevar_doc,
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

    if (Py_IS_TYPE(psa->__origin__, &_PyParamSpec_Type)) {
        return PyUnicode_FromFormat("%s.args",
            ((paramspecobject *)psa->__origin__)->name);
    }
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

PyDoc_STRVAR(paramspecargs_doc,
"The args for a ParamSpec object.\n\
\n\
Given a ParamSpec object P, P.args is an instance of ParamSpecArgs.\n\
\n\
ParamSpecArgs objects have a reference back to their ParamSpec:\n\
\n\
    P.args.__origin__ is P\n\
\n\
This type is meant for runtime introspection and has no special meaning to\n\
static type checkers.\n\
");

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
    .tp_doc = paramspecargs_doc,
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

    if (Py_IS_TYPE(psk->__origin__, &_PyParamSpec_Type)) {
        return PyUnicode_FromFormat("%s.kwargs",
            ((paramspecobject *)psk->__origin__)->name);
    }
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

PyDoc_STRVAR(paramspeckwargs_doc,
"The kwargs for a ParamSpec object.\n\
\n\
Given a ParamSpec object P, P.kwargs is an instance of ParamSpecKwargs.\n\
\n\
ParamSpecKwargs objects have a reference back to their ParamSpec:\n\
\n\
    P.kwargs.__origin__ is P\n\
\n\
This type is meant for runtime introspection and has no special meaning to\n\
static type checkers.\n\
");

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
    .tp_doc = paramspeckwargs_doc,
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
    ps->bound = Py_XNewRef(bound);
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
    if (bound != NULL) {
        bound = type_check(bound);
        if (bound == NULL) {
            return NULL;
        }
    }
    PyObject *ps = (PyObject *)paramspecobject_alloc(
        name, bound, covariant, contravariant, autovariance);
    Py_XDECREF(bound);
    return ps;
}


/*[clinic input]
paramspec.__typing_subst__ as paramspec_typing_subst

    arg: object

[clinic start generated code]*/

static PyObject *
paramspec_typing_subst_impl(paramspecobject *self, PyObject *arg)
/*[clinic end generated code: output=803e1ade3f13b57d input=4e0005d24023e896]*/
{
    PyObject *args = PyTuple_Pack(2, self, arg);
    if (args == NULL) {
        return NULL;
    }
    return call_typing_func_object("_paramspec_subst", args);
}

/*[clinic input]
paramspec.__typing_prepare_subst__ as paramspec_typing_prepare_subst

    alias: object
    args: object

[clinic start generated code]*/

static PyObject *
paramspec_typing_prepare_subst_impl(paramspecobject *self, PyObject *alias,
                                    PyObject *args)
/*[clinic end generated code: output=95449d630a2adb9a input=4375e2ffcb2ad635]*/
{
    PyObject *args_tuple = PyTuple_Pack(3, self, alias, args);
    if (args_tuple == NULL) {
        return NULL;
    }
    return call_typing_func_object("_paramspec_prepare_subst", args_tuple);
}

/*[clinic input]
paramspec.__reduce__ as paramspec_reduce

[clinic start generated code]*/

static PyObject *
paramspec_reduce_impl(paramspecobject *self)
/*[clinic end generated code: output=b83398674416db27 input=5bf349f0d5dd426c]*/
{
    return PyUnicode_FromString(self->name);
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
    PARAMSPEC_REDUCE_METHODDEF
    {"__mro_entries__", paramspec_mro_entries, METH_O},
    {0}
};

static PyNumberMethods paramspec_as_number = {
    .nb_or = _Py_union_type_or, // Add __or__ function
};

PyDoc_STRVAR(paramspec_doc,
"Parameter specification variable.\n\
\n\
Usage::\n\
\n\
    P = ParamSpec('P')\n\
\n\
Parameter specification variables exist primarily for the benefit of static\n\
type checkers.  They are used to forward the parameter types of one\n\
callable to another callable, a pattern commonly found in higher order\n\
functions and decorators.  They are only valid when used in ``Concatenate``,\n\
or as the first argument to ``Callable``, or as parameters for user-defined\n\
Generics.  See class Generic for more information on generic types.  An\n\
example for annotating a decorator::\n\
\n\
    T = TypeVar('T')\n\
    P = ParamSpec('P')\n\
\n\
    def add_logging(f: Callable[P, T]) -> Callable[P, T]:\n\
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
Parameter specification variables defined with covariant=True or\n\
contravariant=True can be used to declare covariant or contravariant\n\
generic types.  These keyword arguments are valid, but their actual semantics\n\
are yet to be decided.  See PEP 612 for details.\n\
\n\
Parameter specification variables can be introspected. e.g.:\n\
\n\
    P.__name__ == 'P'\n\
    P.__bound__ == None\n\
    P.__covariant__ == False\n\
    P.__contravariant__ == False\n\
\n\
Note that only parameter specification variables defined in global scope can\n\
be pickled.\n\
");

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
    .tp_as_number = &paramspec_as_number,
    .tp_doc = paramspec_doc,
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

/*[clinic input]
typevartuple.__typing_prepare_subst__ as typevartuple_typing_prepare_subst

    alias: object
    args: object

[clinic start generated code]*/

static PyObject *
typevartuple_typing_prepare_subst_impl(typevartupleobject *self,
                                       PyObject *alias, PyObject *args)
/*[clinic end generated code: output=ff999bc5b02036c1 input=a211b05f2eeb4306]*/
{
    PyObject *args_tuple = PyTuple_Pack(3, self, alias, args);
    if (args_tuple == NULL) {
        return NULL;
    }
    return call_typing_func_object("_typevartuple_prepare_subst", args_tuple);
}

/*[clinic input]
typevartuple.__reduce__ as typevartuple_reduce

[clinic start generated code]*/

static PyObject *
typevartuple_reduce_impl(typevartupleobject *self)
/*[clinic end generated code: output=3215bc0477913d20 input=3018a4d66147e807]*/
{
    return PyUnicode_FromString(self->name);
}

static PyObject *
typevartuple_mro_entries(PyObject *self, PyObject *args)
{
    PyErr_SetString(PyExc_TypeError,
                    "Cannot subclass an instance of TypeVarTuple");
    return NULL;
}

static PyMethodDef typevartuple_methods[] = {
    TYPEVARTUPLE_TYPING_SUBST_METHODDEF
    TYPEVARTUPLE_TYPING_PREPARE_SUBST_METHODDEF
    TYPEVARTUPLE_REDUCE_METHODDEF
    {"__mro_entries__", typevartuple_mro_entries, METH_O},
    {0}
};

PyDoc_STRVAR(typevartuple_doc,
"Type variable tuple.\n\
\n\
Usage:\n\
\n\
  Ts = TypeVarTuple('Ts')  # Can be given any name\n\
\n\
Just as a TypeVar (type variable) is a placeholder for a single type,\n\
a TypeVarTuple is a placeholder for an *arbitrary* number of types. For\n\
example, if we define a generic class using a TypeVarTuple:\n\
\n\
  class C(Generic[*Ts]): ...\n\
\n\
Then we can parameterize that class with an arbitrary number of type\n\
arguments:\n\
\n\
  C[int]       # Fine\n\
  C[int, str]  # Also fine\n\
  C[()]        # Even this is fine\n\
\n\
For more details, see PEP 646.\n\
\n\
Note that only TypeVarTuples defined in global scope can be pickled.\n\
");

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
    .tp_doc = typevartuple_doc,
};

PyObject *_Py_make_typevar(const char *name, PyObject *bound_or_constraints) {
    if (bound_or_constraints != NULL && PyTuple_CheckExact(bound_or_constraints)) {
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

PyDoc_STRVAR(generic_doc,
"Abstract base class for generic types.\n\
\n\
A generic type is typically declared by inheriting from\n\
this class parameterized with one or more type variables.\n\
For example, a generic mapping type might be defined as::\n\
\n\
    class Mapping(Generic[KT, VT]):\n\
        def __getitem__(self, key: KT) -> VT:\n\
            ...\n\
        # Etc.\n\
\n\
This class can then be used as follows::\n\
\n\
    def lookup_name(mapping: Mapping[KT, VT], key: KT, default: VT) -> VT:\n\
        try:\n\
            return mapping[key]\n\
        except KeyError:\n\
            return default\n\
");

PyDoc_STRVAR(generic_class_getitem_doc,
"Parameterizes a generic class.\n\
\n\
At least, parameterizing a generic class is the *main* thing this method\n\
does. For example, for some generic class `Foo`, this is called when we\n\
do `Foo[int]` - there, with `cls=Foo` and `params=int`.\n\
\n\
However, note that this method is also called when defining generic\n\
classes in the first place with `class Foo(Generic[T]): ...`.\n\
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
    Py_DECREF(func);
    Py_DECREF(typing);
    Py_DecRef(new_args);
    return result;
error:
    Py_XDECREF(typing);
    Py_XDECREF(func);
    Py_XDECREF(new_args);
    return NULL;
}

static PyObject *
generic_init_subclass(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    return call_typing_args_kwargs("_generic_init_subclass", cls, args, kwargs);
}

static PyObject *
generic_class_getitem(PyTypeObject *cls, PyObject *args, PyObject *kwargs)
{
    return call_typing_args_kwargs("_generic_class_getitem", cls, args, kwargs);
}

PyObject *
_Py_subscript_generic(PyObject *params)
{
    PyObject *args = PyTuple_Pack(2, &_PyGeneric_Type, params);
    if (args == NULL) {
        return NULL;
    }
    return call_typing_func_object("_generic_class_getitem", args);
}

static PyMethodDef generic_methods[] = {
    {"__class_getitem__", (PyCFunction)(void (*)(void))generic_class_getitem,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     generic_class_getitem_doc},
    {"__init_subclass__", (PyCFunction)(void (*)(void))generic_init_subclass,
     METH_VARARGS | METH_KEYWORDS | METH_CLASS,
     PyDoc_STR("Function to initialize subclasses.")},
    {NULL} /* Sentinel */
};

PyTypeObject _PyGeneric_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.Generic",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = generic_doc,
    .tp_methods = generic_methods,
    .tp_new = PyType_GenericNew,
};
