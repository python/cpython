// types.Union -- used to represent e.g. Union[int, str], int | str
#include "Python.h"
#include "pycore_unionobject.h"
#include "structmember.h"


typedef struct {
    PyObject_HEAD
    PyObject *args;
} unionobject;

static void
unionobject_dealloc(PyObject *self)
{
    unionobject *alias = (unionobject *)self;

    Py_XDECREF(alias->args);
    Py_TYPE(self)->tp_free(self);
}

static Py_hash_t
union_hash(PyObject *self)
{
    unionobject *alias = (unionobject *)self;
    Py_hash_t h1 = PyObject_Hash(alias->args);
    if (h1 == -1) {
        return -1;
    }
    return h1;
}

static int
is_generic_alias_in_args(PyObject *args) {
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(args, iarg);
        if (PyObject_TypeCheck(arg, &Py_GenericAliasType)) {
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
        if (arg == Py_None) {
            arg = (PyObject *)&_PyNone_Type;
        }
        if (PyType_Check(arg) && PyObject_IsInstance(instance, arg) != 0) {
            Py_RETURN_TRUE;
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
        if (PyType_Check(arg) && (PyType_IsSubtype((PyTypeObject *)instance, (PyTypeObject *)arg) != 0)) {
            Py_RETURN_TRUE;
        }
    }
   Py_RETURN_FALSE;
}

static int
is_typing_module(PyObject *obj) {
    PyObject *module = PyObject_GetAttrString(obj, "__module__");
    if (module == NULL) {
        return -1;
    }
    int is_typing = PyUnicode_Check(module) && _PyUnicode_EqualToASCIIString(module, "typing");
    Py_DECREF(module);
    return is_typing;
}

static int
is_typing_name(PyObject *obj, char *name)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (strcmp(type->tp_name, name) != 0) {
        return 0;
    }
    return is_typing_module(obj);
}

static PyObject *
union_richcompare(PyObject *a, PyObject *b, int op)
{
    PyObject *result = NULL;
    if (op != Py_EQ && op != Py_NE) {
        result = Py_NotImplemented;
        Py_INCREF(result);
        return result;
    }

    PyTypeObject *type = Py_TYPE(b);

    PyObject* a_set = PySet_New(((unionobject*)a)->args);
    if (a_set == NULL) {
        return NULL;
    }
    PyObject* b_set = PySet_New(NULL);
    if (b_set == NULL) {
        goto exit;
    }

    // Populate b_set with the data from the right object
    int is_typing_union = is_typing_name(b, "_UnionGenericAlias");
    if (is_typing_union < 0) {
        goto exit;
    }
    if (is_typing_union) {
        PyObject *b_args = PyObject_GetAttrString(b, "__args__");
        if (b_args == NULL) {
            goto exit;
        }
        if (!PyTuple_CheckExact(b_args)) {
            Py_DECREF(b_args);
            PyErr_SetString(PyExc_TypeError, "__args__ argument of typing.Union object is not a tuple");
            goto exit;
        }
        Py_ssize_t b_arg_length = PyTuple_GET_SIZE(b_args);
        for (Py_ssize_t i = 0; i < b_arg_length; i++) {
            PyObject* arg = PyTuple_GET_ITEM(b_args, i);
            if (arg == (PyObject *)&_PyNone_Type) {
                arg = Py_None;
            }
            if (PySet_Add(b_set, arg) == -1) {
                Py_DECREF(b_args);
                goto exit;
            }
        }
        Py_DECREF(b_args);
    } else if (type == &_Py_UnionType) {
        PyObject* args = ((unionobject*) b)->args;
        Py_ssize_t arg_length = PyTuple_GET_SIZE(args);
        for (Py_ssize_t i = 0; i < arg_length; i++) {
            PyObject* arg = PyTuple_GET_ITEM(args, i);
            if (PySet_Add(b_set, arg) == -1) {
                goto exit;
            }
        }
    } else {
        if (PySet_Add(b_set, b) == -1) {
            goto exit;
        }
    }
    result = PyObject_RichCompare(a_set, b_set, op);
exit:
    Py_XDECREF(a_set);
    Py_XDECREF(b_set);
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
        PyTypeObject* arg_type = Py_TYPE(arg);
        if (arg_type == &_Py_UnionType) {
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
        PyTypeObject* arg_type = Py_TYPE(arg);
        if (arg_type == &_Py_UnionType) {
            PyObject* nested_args = ((unionobject*)arg)->args;
            Py_ssize_t nested_arg_length = PyTuple_GET_SIZE(nested_args);
            for (Py_ssize_t j = 0; j < nested_arg_length; j++) {
                PyObject* nested_arg = PyTuple_GET_ITEM(nested_args, j);
                Py_INCREF(nested_arg);
                PyTuple_SET_ITEM(flattened_args, pos, nested_arg);
                pos++;
            }
        } else {
            Py_INCREF(arg);
            PyTuple_SET_ITEM(flattened_args, pos, arg);
            pos++;
        }
    }
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
        return NULL;
    }
    // Add unique elements to an array.
    Py_ssize_t added_items = 0;
    for (Py_ssize_t i = 0; i < arg_length; i++) {
        int is_duplicate = 0;
        PyObject* i_element = PyTuple_GET_ITEM(args, i);
        for (Py_ssize_t j = i + 1; j < arg_length; j++) {
            PyObject* j_element = PyTuple_GET_ITEM(args, j);
            int is_ga = PyObject_TypeCheck(i_element, &Py_GenericAliasType) &&
                        PyObject_TypeCheck(j_element, &Py_GenericAliasType);
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

static int
is_unionable(PyObject *obj)
{
    if (obj == Py_None) {
        return 1;
    }
    PyTypeObject *type = Py_TYPE(obj);
    return (
        is_typevar(obj) ||
        is_new_type(obj) ||
        is_special_form(obj) ||
        PyType_Check(obj) ||
        PyObject_TypeCheck(obj, &Py_GenericAliasType) ||
        type == &_Py_UnionType);
}

PyObject *
_Py_union_type_or(PyObject* self, PyObject* param)
{
    PyObject *tuple = PyTuple_Pack(2, self, param);
    if (tuple == NULL) {
        return NULL;
    }
    PyObject *new_union = _Py_Union(tuple);
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

static PyMemberDef union_members[] = {
        {"__args__", T_OBJECT, offsetof(unionobject, args), READONLY},
        {0}
};

static PyMethodDef union_methods[] = {
        {"__instancecheck__", union_instancecheck, METH_O},
        {"__subclasscheck__", union_subclasscheck, METH_O},
        {0}};

static PyNumberMethods union_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

PyTypeObject _Py_UnionType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.Union",
    .tp_doc = "Represent a PEP 604 union type\n"
              "\n"
              "E.g. for int | str",
    .tp_basicsize = sizeof(unionobject),
    .tp_dealloc = unionobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_hash = union_hash,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_members = union_members,
    .tp_methods = union_methods,
    .tp_richcompare = union_richcompare,
    .tp_as_number = &union_as_number,
    .tp_repr = union_repr,
};

PyObject *
_Py_Union(PyObject *args)
{
    assert(PyTuple_CheckExact(args));

    unionobject* result = NULL;

    // Check arguments are unionable.
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(args, iarg);
        if (arg == NULL) {
            return NULL;
        }
        int is_arg_unionable = is_unionable(arg);
        if (is_arg_unionable < 0) {
            return NULL;
        }
        if (!is_arg_unionable) {
            Py_INCREF(Py_NotImplemented);
            return Py_NotImplemented;
        }
    }

    result = PyObject_New(unionobject, &_Py_UnionType);
    if (result == NULL) {
        return NULL;
    }

    result->args = dedup_and_flatten_args(args);
    if (result->args == NULL) {
        Py_DECREF(result);
        return NULL;
    }
    return (PyObject*)result;
}
