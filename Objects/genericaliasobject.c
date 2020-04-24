// types.GenericAlias -- used to represent e.g. list[int].

#include "Python.h"
#include "pycore_object.h"
#include "structmember.h"         // PyMemberDef

typedef struct {
    PyObject_HEAD
    PyObject *origin;
    PyObject *args;
    PyObject *parameters;
} gaobject;

static void
ga_dealloc(PyObject *self)
{
    gaobject *alias = (gaobject *)self;

    _PyObject_GC_UNTRACK(self);
    Py_XDECREF(alias->origin);
    Py_XDECREF(alias->args);
    Py_XDECREF(alias->parameters);
    self->ob_type->tp_free(self);
}

static int
ga_traverse(PyObject *self, visitproc visit, void *arg)
{
    gaobject *alias = (gaobject *)self;
    Py_VISIT(alias->origin);
    Py_VISIT(alias->args);
    Py_VISIT(alias->parameters);
    return 0;
}

static int
ga_repr_item(_PyUnicodeWriter *writer, PyObject *p)
{
    _Py_IDENTIFIER(__module__);
    _Py_IDENTIFIER(__qualname__);
    _Py_IDENTIFIER(__origin__);
    _Py_IDENTIFIER(__args__);
    PyObject *qualname = NULL;
    PyObject *module = NULL;
    PyObject *r = NULL;
    PyObject *tmp;
    int err;

    if (p == Py_Ellipsis) {
        // The Ellipsis object
        r = PyUnicode_FromString("...");
        goto done;
    }

    if (_PyObject_LookupAttrId(p, &PyId___origin__, &tmp) < 0) {
        goto done;
    }
    if (tmp != NULL) {
        Py_DECREF(tmp);
        if (_PyObject_LookupAttrId(p, &PyId___args__, &tmp) < 0) {
            goto done;
        }
        if (tmp != NULL) {
            Py_DECREF(tmp);
            // It looks like a GenericAlias
            goto use_repr;
        }
    }

    if (_PyObject_LookupAttrId(p, &PyId___qualname__, &qualname) < 0) {
        goto done;
    }
    if (qualname == NULL) {
        goto use_repr;
    }
    if (_PyObject_LookupAttrId(p, &PyId___module__, &module) < 0) {
        goto done;
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
        goto done;
    }
    else {
        r = PyUnicode_FromFormat("%S.%S", module, qualname);
        goto done;
    }

use_repr:
    r = PyObject_Repr(p);

done:
    Py_XDECREF(qualname);
    Py_XDECREF(module);
    if (r == NULL) {
        // error if any of the above PyObject_Repr/PyUnicode_From* fail
        err = -1;
    }
    else {
        err = _PyUnicodeWriter_WriteStr(writer, r);
        Py_DECREF(r);
    }
    return err;
}

static PyObject *
ga_repr(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);

    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);

    if (ga_repr_item(&writer, alias->origin) < 0) {
        goto error;
    }
    if (_PyUnicodeWriter_WriteASCIIString(&writer, "[", 1) < 0) {
        goto error;
    }
    for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0) {
                goto error;
            }
        }
        PyObject *p = PyTuple_GET_ITEM(alias->args, i);
        if (ga_repr_item(&writer, p) < 0) {
            goto error;
        }
    }
    if (len == 0) {
        // for something like tuple[()] we should print a "()"
        if (_PyUnicodeWriter_WriteASCIIString(&writer, "()", 2) < 0) {
            goto error;
        }
    }
    if (_PyUnicodeWriter_WriteASCIIString(&writer, "]", 1) < 0) {
        goto error;
    }
    return _PyUnicodeWriter_Finish(&writer);
error:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}

// isinstance(obj, TypeVar) without importing typing.py.
// Returns -1 for errors.
static int
is_typevar(PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (strcmp(type->tp_name, "TypeVar") != 0) {
        return 0;
    }
    PyObject *module = PyObject_GetAttrString((PyObject *)type, "__module__");
    if (module == NULL) {
        return -1;
    }
    int res = PyUnicode_Check(module)
        && _PyUnicode_EqualToASCIIString(module, "typing");
    Py_DECREF(module);
    return res;
}

// Index of item in self[:len], or -1 if not found (self is a tuple)
static Py_ssize_t
tuple_index(PyObject *self, Py_ssize_t len, PyObject *item)
{
    for (Py_ssize_t i = 0; i < len; i++) {
        if (PyTuple_GET_ITEM(self, i) == item) {
            return i;
        }
    }
    return -1;
}

// tuple(t for t in args if isinstance(t, TypeVar))
static PyObject *
make_parameters(PyObject *args)
{
    Py_ssize_t len = PyTuple_GET_SIZE(args);
    PyObject *parameters = PyTuple_New(len);
    if (parameters == NULL)
        return NULL;
    Py_ssize_t iparam = 0;
    for (Py_ssize_t iarg = 0; iarg < len; iarg++) {
        PyObject *t = PyTuple_GET_ITEM(args, iarg);
        int typevar = is_typevar(t);
        if (typevar < 0) {
            Py_XDECREF(parameters);
            return NULL;
        }
        if (typevar) {
            if (tuple_index(parameters, iparam, t) < 0) {
                Py_INCREF(t);
                PyTuple_SET_ITEM(parameters, iparam, t);
                iparam++;
            }
        }
    }
    if (iparam < len) {
        if (_PyTuple_Resize(&parameters, iparam) < 0) {
            Py_XDECREF(parameters);
            return NULL;
        }
    }
    return parameters;
}

static PyObject *
ga_getitem(PyObject *self, PyObject *item)
{
    gaobject *alias = (gaobject *)self;
    // do a lookup for __parameters__ so it gets populated (if not already)
    if (alias->parameters == NULL) {
        alias->parameters = make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    Py_ssize_t nparams = PyTuple_GET_SIZE(alias->parameters);
    if (nparams == 0) {
        return PyErr_Format(PyExc_TypeError,
                            "There are no type variables left in %R",
                            self);
    }
    int is_tuple = PyTuple_Check(item);
    Py_ssize_t nitem = is_tuple ? PyTuple_GET_SIZE(item) : 1;
    if (nitem != nparams) {
        return PyErr_Format(PyExc_TypeError,
                            "Too %s arguments for %R",
                            nitem > nparams ? "many" : "few",
                            self);
    }
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    PyObject *newargs = PyTuple_New(nargs);
    if (newargs == NULL)
        return NULL;
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        int typevar = is_typevar(arg);
        if (typevar < 0) {
            Py_DECREF(newargs);
            return NULL;
        }
        if (typevar) {
            Py_ssize_t iparam = tuple_index(alias->parameters, nparams, arg);
            assert(iparam >= 0);
            if (is_tuple) {
                arg = PyTuple_GET_ITEM(item, iparam);
            }
            else {
                assert(iparam == 0);
                arg = item;
            }
        }
        Py_INCREF(arg);
        PyTuple_SET_ITEM(newargs, iarg, arg);
    }
    PyObject *res = Py_GenericAlias(alias->origin, newargs);
    Py_DECREF(newargs);
    return res;
}

static PyMappingMethods ga_as_mapping = {
    .mp_subscript = ga_getitem,
};

static Py_hash_t
ga_hash(PyObject *self)
{
    gaobject *alias = (gaobject *)self;
    // TODO: Hash in the hash for the origin
    Py_hash_t h0 = PyObject_Hash(alias->origin);
    if (h0 == -1) {
        return -1;
    }
    Py_hash_t h1 = PyObject_Hash(alias->args);
    if (h1 == -1) {
        return -1;
    }
    return h0 ^ h1;
}

static PyObject *
ga_call(PyObject *self, PyObject *args, PyObject *kwds)
{
    gaobject *alias = (gaobject *)self;
    PyObject *obj = PyObject_Call(alias->origin, args, kwds);
    if (obj != NULL) {
        if (PyObject_SetAttrString(obj, "__orig_class__", self) < 0) {
            if (!PyErr_ExceptionMatches(PyExc_AttributeError) &&
                !PyErr_ExceptionMatches(PyExc_TypeError))
            {
                Py_DECREF(obj);
                return NULL;
            }
            PyErr_Clear();
        }
    }
    return obj;
}

static const char* const attr_exceptions[] = {
    "__origin__",
    "__args__",
    "__parameters__",
    "__mro_entries__",
    "__reduce_ex__",  // needed so we don't look up object.__reduce_ex__
    "__reduce__",
    NULL,
};

static PyObject *
ga_getattro(PyObject *self, PyObject *name)
{
    gaobject *alias = (gaobject *)self;
    if (PyUnicode_Check(name)) {
        for (const char * const *p = attr_exceptions; ; p++) {
            if (*p == NULL) {
                return PyObject_GetAttr(alias->origin, name);
            }
            if (_PyUnicode_EqualToASCIIString(name, *p)) {
                break;
            }
        }
    }
    return PyObject_GenericGetAttr(self, name);
}

static PyObject *
ga_richcompare(PyObject *a, PyObject *b, int op)
{
    if (Py_TYPE(a) != &Py_GenericAliasType ||
        Py_TYPE(b) != &Py_GenericAliasType ||
        (op != Py_EQ && op != Py_NE))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (op == Py_NE) {
        PyObject *eq = ga_richcompare(a, b, Py_EQ);
        if (eq == NULL)
            return NULL;
        Py_DECREF(eq);
        if (eq == Py_True) {
            Py_RETURN_FALSE;
        }
        else {
            Py_RETURN_TRUE;
        }
    }

    gaobject *aa = (gaobject *)a;
    gaobject *bb = (gaobject *)b;
    int eq = PyObject_RichCompareBool(aa->origin, bb->origin, Py_EQ);
    if (eq < 0) {
        return NULL;
    }
    if (!eq) {
        Py_RETURN_FALSE;
    }
    return PyObject_RichCompare(aa->args, bb->args, Py_EQ);
}

static PyObject *
ga_mro_entries(PyObject *self, PyObject *args)
{
    gaobject *alias = (gaobject *)self;
    return PyTuple_Pack(1, alias->origin);
}

static PyObject *
ga_instancecheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "isinstance() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_subclasscheck(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyErr_SetString(PyExc_TypeError,
                    "issubclass() argument 2 cannot be a parameterized generic");
    return NULL;
}

static PyObject *
ga_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    gaobject *alias = (gaobject *)self;
    return Py_BuildValue("O(OO)", Py_TYPE(alias),
                         alias->origin, alias->args);
}

static PyMethodDef ga_methods[] = {
    {"__mro_entries__", ga_mro_entries, METH_O},
    {"__instancecheck__", ga_instancecheck, METH_O},
    {"__subclasscheck__", ga_subclasscheck, METH_O},
    {"__reduce__", ga_reduce, METH_NOARGS},
    {0}
};

static PyMemberDef ga_members[] = {
    {"__origin__", T_OBJECT, offsetof(gaobject, origin), READONLY},
    {"__args__", T_OBJECT, offsetof(gaobject, args), READONLY},
    {0}
};

static PyObject *
ga_parameters(PyObject *self, void *unused)
{
    gaobject *alias = (gaobject *)self;
    if (alias->parameters == NULL) {
        alias->parameters = make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    Py_INCREF(alias->parameters);
    return alias->parameters;
}

static PyGetSetDef ga_properties[] = {
    {"__parameters__", ga_parameters, (setter)NULL, "Type variables in the GenericAlias.", NULL},
    {0}
};

static PyObject *
ga_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (!_PyArg_NoKwnames("GenericAlias", kwds)) {
        return NULL;
    }
    if (!_PyArg_CheckPositional("GenericAlias", PyTuple_GET_SIZE(args), 2, 2)) {
        return NULL;
    }
    PyObject *origin = PyTuple_GET_ITEM(args, 0);
    PyObject *arguments = PyTuple_GET_ITEM(args, 1);
    return Py_GenericAlias(origin, arguments);
}

// TODO:
// - argument clinic?
// - __doc__?
// - cache?
PyTypeObject Py_GenericAliasType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.GenericAlias",
    .tp_doc = "Represent a PEP 585 generic type\n"
              "\n"
              "E.g. for t = list[int], t.origin is list and t.args is (int,).",
    .tp_basicsize = sizeof(gaobject),
    .tp_dealloc = ga_dealloc,
    .tp_repr = ga_repr,
    .tp_as_mapping = &ga_as_mapping,
    .tp_hash = ga_hash,
    .tp_call = ga_call,
    .tp_getattro = ga_getattro,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = ga_traverse,
    .tp_richcompare = ga_richcompare,
    .tp_methods = ga_methods,
    .tp_members = ga_members,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = ga_new,
    .tp_free = PyObject_GC_Del,
    .tp_getset = ga_properties,
};

PyObject *
Py_GenericAlias(PyObject *origin, PyObject *args)
{
    if (!PyTuple_Check(args)) {
        args = PyTuple_Pack(1, args);
        if (args == NULL) {
            return NULL;
        }
    }
    else {
        Py_INCREF(args);
    }

    gaobject *alias = PyObject_GC_New(gaobject, &Py_GenericAliasType);
    if (alias == NULL) {
        Py_DECREF(args);
        return NULL;
    }

    Py_INCREF(origin);
    alias->origin = origin;
    alias->args = args;
    alias->parameters = NULL;
    _PyObject_GC_TRACK(alias);
    return (PyObject *)alias;
}
