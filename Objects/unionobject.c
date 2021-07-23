// types.Union -- used to represent e.g. Union[int, str], int | str
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_unionobject.h"
#include "structmember.h"


static PyObject *make_union(PyObject *);


typedef struct {
    PyObject_HEAD
    PyObject *args;
    PyObject *parameters;
} unionobject;

static void
unionobject_dealloc(PyObject *self)
{
    unionobject *alias = (unionobject *)self;

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(alias->args);
    Py_XDECREF(alias->parameters);
    Py_TYPE(self)->tp_free(self);
}

static int
union_traverse(PyObject *self, visitproc visit, void *arg)
{
    unionobject *alias = (unionobject *)self;
    Py_VISIT(alias->args);
    Py_VISIT(alias->parameters);
    return 0;
}

static Py_hash_t
union_hash(PyObject *self)
{
    unionobject *alias = (unionobject *)self;
    PyObject *args = PyFrozenSet_New(alias->args);
    if (args == NULL) {
        return (Py_hash_t)-1;
    }
    Py_hash_t hash = PyObject_Hash(args);
    Py_DECREF(args);
    return hash;
}

static int
is_generic_alias_in_args(PyObject *args)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(args, iarg);
        if (_PyGenericAlias_Check(arg)) {
            return 0;
        }
    }
    return 1;
}

static PyObject *
union_instancecheck(PyObject *self, PyObject *instance)
{
    unionobject *alias = (unionobject *) self;
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    if (!is_generic_alias_in_args(alias->args)) {
        PyErr_SetString(PyExc_TypeError,
            "isinstance() argument 2 cannot contain a parameterized generic");
        return NULL;
    }
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        if (PyType_Check(arg)) {
            int res = PyObject_IsInstance(instance, arg);
            if (res < 0) {
                return NULL;
            }
            if (res) {
                Py_RETURN_TRUE;
            }
        }
    }
    Py_RETURN_FALSE;
}

static PyObject *
union_subclasscheck(PyObject *self, PyObject *instance)
{
    if (!PyType_Check(instance)) {
        PyErr_SetString(PyExc_TypeError, "issubclass() arg 1 must be a class");
        return NULL;
    }
    unionobject *alias = (unionobject *)self;
    if (!is_generic_alias_in_args(alias->args)) {
        PyErr_SetString(PyExc_TypeError,
            "issubclass() argument 2 cannot contain a parameterized generic");
        return NULL;
    }
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        if (PyType_Check(arg)) {
            int res = PyObject_IsSubclass(instance, arg);
            if (res < 0) {
                return NULL;
            }
            if (res) {
                Py_RETURN_TRUE;
            }
        }
    }
    Py_RETURN_FALSE;
}

static int
is_typing_module(PyObject *obj)
{
    _Py_IDENTIFIER(__module__);
    PyObject *module;
    if (_PyObject_LookupAttrId(obj, &PyId___module__, &module) < 0) {
        return -1;
    }
    int is_typing = (module != NULL &&
                     PyUnicode_Check(module) &&
                     _PyUnicode_EqualToASCIIString(module, "typing"));
    Py_XDECREF(module);
    return is_typing;
}

static int
is_typing_name(PyObject *obj, const char *name)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (strcmp(type->tp_name, name) != 0) {
        return 0;
    }
    return is_typing_module((PyObject *)type);
}

static PyObject *
union_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!_PyUnion_Check(b) || (op != Py_EQ && op != Py_NE)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *a_set = PySet_New(((unionobject*)a)->args);
    if (a_set == NULL) {
        return NULL;
    }
    PyObject *b_set = PySet_New(((unionobject*)b)->args);
    if (b_set == NULL) {
        Py_DECREF(a_set);
        return NULL;
    }
    PyObject *result = PyObject_RichCompare(a_set, b_set, op);
    Py_DECREF(b_set);
    Py_DECREF(a_set);
    return result;
}

static PyObject*
flatten_args(PyObject* args)
{
    Py_ssize_t arg_length = PyTuple_GET_SIZE(args);
    Py_ssize_t total_args = 0;
    // Get number of total args once it's flattened.
    for (Py_ssize_t i = 0; i < arg_length; i++) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        if (_PyUnion_Check(arg)) {
            total_args += PyTuple_GET_SIZE(((unionobject*) arg)->args);
        } else {
            total_args++;
        }
    }
    // Create new tuple of flattened args.
    PyObject *flattened_args = PyTuple_New(total_args);
    if (flattened_args == NULL) {
        return NULL;
    }
    Py_ssize_t pos = 0;
    for (Py_ssize_t i = 0; i < arg_length; i++) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        if (_PyUnion_Check(arg)) {
            PyObject* nested_args = ((unionobject*)arg)->args;
            Py_ssize_t nested_arg_length = PyTuple_GET_SIZE(nested_args);
            for (Py_ssize_t j = 0; j < nested_arg_length; j++) {
                PyObject* nested_arg = PyTuple_GET_ITEM(nested_args, j);
                Py_INCREF(nested_arg);
                PyTuple_SET_ITEM(flattened_args, pos, nested_arg);
                pos++;
            }
        } else {
            if (arg == Py_None) {
                arg = (PyObject *)&_PyNone_Type;
            }
            Py_INCREF(arg);
            PyTuple_SET_ITEM(flattened_args, pos, arg);
            pos++;
        }
    }
    assert(pos == total_args);
    return flattened_args;
}

static PyObject*
dedup_and_flatten_args(PyObject* args)
{
    args = flatten_args(args);
    if (args == NULL) {
        return NULL;
    }
    Py_ssize_t arg_length = PyTuple_GET_SIZE(args);
    PyObject *new_args = PyTuple_New(arg_length);
    if (new_args == NULL) {
        Py_DECREF(args);
        return NULL;
    }
    // Add unique elements to an array.
    Py_ssize_t added_items = 0;
    for (Py_ssize_t i = 0; i < arg_length; i++) {
        int is_duplicate = 0;
        PyObject* i_element = PyTuple_GET_ITEM(args, i);
        for (Py_ssize_t j = 0; j < added_items; j++) {
            PyObject* j_element = PyTuple_GET_ITEM(new_args, j);
            int is_ga = _PyGenericAlias_Check(i_element) &&
                        _PyGenericAlias_Check(j_element);
            // RichCompare to also deduplicate GenericAlias types (slower)
            is_duplicate = is_ga ? PyObject_RichCompareBool(i_element, j_element, Py_EQ)
                : i_element == j_element;
            // Should only happen if RichCompare fails
            if (is_duplicate < 0) {
                Py_DECREF(args);
                Py_DECREF(new_args);
                return NULL;
            }
            if (is_duplicate)
                break;
        }
        if (!is_duplicate) {
            Py_INCREF(i_element);
            PyTuple_SET_ITEM(new_args, added_items, i_element);
            added_items++;
        }
    }
    Py_DECREF(args);
    _PyTuple_Resize(&new_args, added_items);
    return new_args;
}

static int
is_typevar(PyObject *obj)
{
    return is_typing_name(obj, "TypeVar");
}

static int
is_special_form(PyObject *obj)
{
    return is_typing_name(obj, "_SpecialForm");
}

static int
is_new_type(PyObject *obj)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (type != &PyFunction_Type) {
        return 0;
    }
    return is_typing_module(obj);
}

// Emulates short-circuiting behavior of the ``||`` operator
// while also checking negative values.
#define CHECK_RES(res) { \
    int result = res; \
    if (result) { \
        return result; \
    } \
}

// Returns 1 on true, 0 on false, and -1 on error.
static int
is_unionable(PyObject *obj)
{
    if (obj == Py_None ||
        PyType_Check(obj) ||
        _PyGenericAlias_Check(obj) ||
        _PyUnion_Check(obj))
    {
        return 1;
    }
    CHECK_RES(is_typevar(obj));
    CHECK_RES(is_new_type(obj));
    CHECK_RES(is_special_form(obj));
    return 0;
}

static int
is_args_unionable(PyObject *args)
{
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(args, iarg);
        int is_arg_unionable = is_unionable(arg);
        if (is_arg_unionable <= 0) {
            if (is_arg_unionable == 0) {
                PyErr_Format(PyExc_TypeError,
                             "Each union argument must be a type, got %.100R", arg);
            }
            return 0;
        }
    }
    return 1;
}

PyObject *
_Py_union_type_or(PyObject* self, PyObject* other)
{
    int r = is_unionable(self);
    if (r > 0) {
        r = is_unionable(other);
    }
    if (r < 0) {
        return NULL;
    }
    if (!r) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *tuple = PyTuple_Pack(2, self, other);
    if (tuple == NULL) {
        return NULL;
    }

    PyObject *new_union = make_union(tuple);
    Py_DECREF(tuple);
    return new_union;
}

static int
union_repr_item(_PyUnicodeWriter *writer, PyObject *p)
{
    _Py_IDENTIFIER(__module__);
    _Py_IDENTIFIER(__qualname__);
    _Py_IDENTIFIER(__origin__);
    _Py_IDENTIFIER(__args__);
    PyObject *qualname = NULL;
    PyObject *module = NULL;
    PyObject *tmp;
    PyObject *r = NULL;
    int err;

    if (p == (PyObject *)&_PyNone_Type) {
        return _PyUnicodeWriter_WriteASCIIString(writer, "None", 4);
    }

    if (_PyObject_LookupAttrId(p, &PyId___origin__, &tmp) < 0) {
        goto exit;
    }

    if (tmp) {
        Py_DECREF(tmp);
        if (_PyObject_LookupAttrId(p, &PyId___args__, &tmp) < 0) {
            goto exit;
        }
        if (tmp) {
            // It looks like a GenericAlias
            Py_DECREF(tmp);
            goto use_repr;
        }
    }

    if (_PyObject_LookupAttrId(p, &PyId___qualname__, &qualname) < 0) {
        goto exit;
    }
    if (qualname == NULL) {
        goto use_repr;
    }
    if (_PyObject_LookupAttrId(p, &PyId___module__, &module) < 0) {
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
    err = _PyUnicodeWriter_WriteStr(writer, r);
    Py_DECREF(r);
    return err;
}

static PyObject *
union_repr(PyObject *self)
{
    unionobject *alias = (unionobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);

    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);
     for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0 && _PyUnicodeWriter_WriteASCIIString(&writer, " | ", 3) < 0) {
            goto error;
        }
        PyObject *p = PyTuple_GET_ITEM(alias->args, i);
        if (union_repr_item(&writer, p) < 0) {
            goto error;
        }
    }
    return _PyUnicodeWriter_Finish(&writer);
error:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}

static PyObject *
union_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    unionobject *alias = (unionobject *)self;
    PyObject* from_args = PyObject_GetAttrString(self, "_from_args");
    if (from_args == NULL) {
        return NULL;
    }

    return Py_BuildValue("O(O)", from_args, alias->args);
}

static PyMemberDef union_members[] = {
        {"__args__", T_OBJECT, offsetof(unionobject, args), READONLY},
        {0}
};

static PyObject *
union_from_args(PyObject *cls, PyObject *args)
{
    if (!PyTuple_CheckExact(args)) {
        _PyArg_BadArgument("Union._from_args", "argument 'args'", "tuple", args);
        return NULL;
    }
    if (!PyTuple_GET_SIZE(args)) {
        PyErr_SetString(PyExc_ValueError, "args must be not empty");
        return NULL;
    }

    if (is_args_unionable(args) <= 0) {
        return NULL;
    }

    return make_union(args);
}

static PyMethodDef union_methods[] = {
        {"_from_args", union_from_args, METH_O | METH_CLASS},
        {"__instancecheck__", union_instancecheck, METH_O},
        {"__subclasscheck__", union_subclasscheck, METH_O},
        {"__reduce__", union_reduce, METH_NOARGS},
        {0}};


static PyObject *
union_getitem(PyObject *self, PyObject *item)
{
    unionobject *alias = (unionobject *)self;
    // Populate __parameters__ if needed.
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }

    PyObject *newargs = _Py_subs_parameters(self, alias->args, alias->parameters, item);
    if (newargs == NULL) {
        return NULL;
    }

    PyObject *res;
    Py_ssize_t nargs = PyTuple_GET_SIZE(newargs);
    if (nargs == 0) {
        res = make_union(newargs);
    }
    else {
        res = PyTuple_GET_ITEM(newargs, 0);
        Py_INCREF(res);
        for (Py_ssize_t iarg = 1; iarg < nargs; iarg++) {
            PyObject *arg = PyTuple_GET_ITEM(newargs, iarg);
            Py_SETREF(res, PyNumber_Or(res, arg));
            if (res == NULL) {
                break;
            }
        }
    }
    Py_DECREF(newargs);
    return res;
}

static PyMappingMethods union_as_mapping = {
    .mp_subscript = union_getitem,
};

static PyObject *
union_parameters(PyObject *self, void *Py_UNUSED(unused))
{
    unionobject *alias = (unionobject *)self;
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    Py_INCREF(alias->parameters);
    return alias->parameters;
}

static PyGetSetDef union_properties[] = {
    {"__parameters__", union_parameters, (setter)NULL, "Type variables in the types.Union.", NULL},
    {0}
};

static PyNumberMethods union_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

PyTypeObject _PyUnion_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.Union",
    .tp_doc = "Represent a PEP 604 union type\n"
              "\n"
              "E.g. for int | str",
    .tp_basicsize = sizeof(unionobject),
    .tp_dealloc = unionobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = union_traverse,
    .tp_hash = union_hash,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_members = union_members,
    .tp_methods = union_methods,
    .tp_richcompare = union_richcompare,
    .tp_as_mapping = &union_as_mapping,
    .tp_as_number = &union_as_number,
    .tp_repr = union_repr,
    .tp_getset = union_properties,
};

static PyObject *
make_union(PyObject *args)
{
    assert(PyTuple_CheckExact(args));

    args = dedup_and_flatten_args(args);
    if (args == NULL) {
        return NULL;
    }
    if (PyTuple_GET_SIZE(args) == 1) {
        PyObject *result1 = PyTuple_GET_ITEM(args, 0);
        Py_INCREF(result1);
        Py_DECREF(args);
        return result1;
    }

    unionobject *result = PyObject_GC_New(unionobject, &_PyUnion_Type);
    if (result == NULL) {
        Py_DECREF(args);
        return NULL;
    }

    result->parameters = NULL;
    result->args = args;
    _PyObject_GC_TRACK(result);
    return (PyObject*)result;
}
