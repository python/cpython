// typing.Union -- used to represent e.g. Union[int, str], int | str
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_typevarobject.h"  // _PyTypeAlias_Type
#include "pycore_unionobject.h"



static PyObject *make_union(PyObject *);


typedef struct {
    PyObject_HEAD
    PyObject *args;
    PyObject *parameters;
    PyObject *weakreflist;
} unionobject;

static void
unionobject_dealloc(PyObject *self)
{
    unionobject *alias = (unionobject *)self;

    _PyObject_GC_UNTRACK(self);
    if (alias->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)alias);
    }

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

static int
is_same(PyObject *left, PyObject *right)
{
    int is_ga = _PyGenericAlias_Check(left) && _PyGenericAlias_Check(right);
    return is_ga ? PyObject_RichCompareBool(left, right, Py_EQ) : left == right;
}

static int
contains(PyObject **items, Py_ssize_t size, PyObject *obj)
{
    for (int i = 0; i < size; i++) {
        int is_duplicate = is_same(items[i], obj);
        if (is_duplicate) {  // -1 or 1
            return is_duplicate;
        }
    }
    return 0;
}

static PyObject *
merge(PyObject **items1, Py_ssize_t size1,
      PyObject **items2, Py_ssize_t size2)
{
    PyObject *tuple = NULL;
    Py_ssize_t pos = 0;

    for (int i = 0; i < size2; i++) {
        PyObject *arg = items2[i];
        int is_duplicate = contains(items1, size1, arg);
        if (is_duplicate < 0) {
            Py_XDECREF(tuple);
            return NULL;
        }
        if (is_duplicate) {
            continue;
        }

        if (tuple == NULL) {
            tuple = PyTuple_New(size1 + size2 - i);
            if (tuple == NULL) {
                return NULL;
            }
            for (; pos < size1; pos++) {
                PyObject *a = items1[pos];
                PyTuple_SET_ITEM(tuple, pos, Py_NewRef(a));
            }
        }
        PyTuple_SET_ITEM(tuple, pos, Py_NewRef(arg));
        pos++;
    }

    if (tuple) {
        (void) _PyTuple_Resize(&tuple, pos);
    }
    return tuple;
}

static PyObject **
get_types(PyObject **obj, Py_ssize_t *size)
{
    if (*obj == Py_None) {
        *obj = (PyObject *)&_PyNone_Type;
    }
    if (_PyUnion_Check(*obj)) {
        PyObject *args = ((unionobject *) *obj)->args;
        *size = PyTuple_GET_SIZE(args);
        return &PyTuple_GET_ITEM(args, 0);
    }
    else {
        *size = 1;
        return obj;
    }
}

static int
is_unionable(PyObject *obj)
{
    if (obj == Py_None ||
        PyType_Check(obj) ||
        _PyGenericAlias_Check(obj) ||
        _PyUnion_Check(obj) ||
        Py_IS_TYPE(obj, &_PyTypeAlias_Type)) {
        return 1;
    }
    return 0;
}

PyObject *
_Py_union_type_or(PyObject* self, PyObject* other)
{
    if (!is_unionable(self) || !is_unionable(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    Py_ssize_t size1, size2;
    PyObject **items1 = get_types(&self, &size1);
    PyObject **items2 = get_types(&other, &size2);
    PyObject *tuple = merge(items1, size1, items2, size2);
    if (tuple == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return Py_NewRef(self);
    }

    PyObject *new_union = make_union(tuple);
    Py_DECREF(tuple);
    return new_union;
}

static int
union_repr_item(_PyUnicodeWriter *writer, PyObject *p)
{
    PyObject *qualname = NULL;
    PyObject *module = NULL;
    PyObject *r = NULL;
    int rc;

    if (p == (PyObject *)&_PyNone_Type) {
        return _PyUnicodeWriter_WriteASCIIString(writer, "None", 4);
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
    rc = _PyUnicodeWriter_WriteStr(writer, r);
    Py_DECREF(r);
    return rc;
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
        {"__args__", _Py_T_OBJECT, offsetof(unionobject, args), Py_READONLY},
        {0}
};

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
        res = Py_NewRef(PyTuple_GET_ITEM(newargs, 0));
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
    return Py_NewRef(alias->parameters);
}

static PyObject *
union_name(PyObject *Py_UNUSED(self), void *Py_UNUSED(ignored))
{
    return PyUnicode_FromString("Union");
}

static PyObject *
union_origin(PyObject *Py_UNUSED(self), void *Py_UNUSED(ignored))
{
    return Py_NewRef(&_PyUnion_Type);
}

static PyGetSetDef union_properties[] = {
    {"__name__", union_name, NULL,
     PyDoc_STR("Name of the type"), NULL},
    {"__qualname__", union_name, NULL,
     PyDoc_STR("Qualified name of the type"), NULL},
    {"__origin__", union_origin, NULL,
     PyDoc_STR("Always returns the type"), NULL},
    {"__parameters__", union_parameters, (setter)NULL,
     PyDoc_STR("Type variables in the types.UnionType."), NULL},
    {0}
};

static PyNumberMethods union_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
};

static const char* const cls_attrs[] = {
        "__module__",  // Required for compatibility with typing module
        NULL,
};

static PyObject *
union_getattro(PyObject *self, PyObject *name)
{
    unionobject *alias = (unionobject *)self;
    if (PyUnicode_Check(name)) {
        for (const char * const *p = cls_attrs; ; p++) {
            if (*p == NULL) {
                break;
            }
            if (_PyUnicode_EqualToASCIIString(name, *p)) {
                return PyObject_GetAttr((PyObject *) Py_TYPE(alias), name);
            }
        }
    }
    return PyObject_GenericGetAttr(self, name);
}

PyObject *
_Py_union_args(PyObject *self)
{
    assert(_PyUnion_Check(self));
    return ((unionobject *) self)->args;
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
    if (Py_IsNone(arg)) {
        // NoneType is immortal, so don't need an INCREF
        return (PyObject *)Py_TYPE(arg);
    }
    // Fast path to avoid calling into typing.py
    if (is_unionable(arg)) {
        return Py_NewRef(arg);
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

static int
add_object_to_union_args(PyObject *args_list, PyObject *args_set, PyObject *obj)
{
    if (Py_IS_TYPE(obj, &_PyUnion_Type)) {
        PyObject *args = ((unionobject *) obj)->args;
        Py_ssize_t size = PyTuple_GET_SIZE(args);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *arg = PyTuple_GET_ITEM(args, i);
            if (add_object_to_union_args(args_list, args_set, arg) < 0) {
                return -1;
            }
        }
        return 0;
    }
    PyObject *type = type_check(obj, "Union[arg, ...]: each arg must be a type.");
    if (type == NULL) {
        return -1;
    }
    if (PySet_Contains(args_set, type)) {
        Py_DECREF(type);
        return 0;
    }
    if (PyList_Append(args_list, type) < 0) {
        Py_DECREF(type);
        return -1;
    }
    if (PySet_Add(args_set, type) < 0) {
        Py_DECREF(type);
        return -1;
    }
    Py_DECREF(type);
    return 0;
}

PyObject *
_Py_union_from_tuple(PyObject *args)
{
    PyObject *args_list = PyList_New(0);
    if (args_list == NULL) {
        return NULL;
    }
    PyObject *args_set = PySet_New(NULL);
    if (args_set == NULL) {
        Py_DECREF(args_list);
        return NULL;
    }
    if (!PyTuple_CheckExact(args)) {
        if (add_object_to_union_args(args_list, args_set, args) < 0) {
            Py_DECREF(args_list);
            Py_DECREF(args_set);
            return NULL;
        }
    }
    else {
        Py_ssize_t size = PyTuple_GET_SIZE(args);
        for (Py_ssize_t i = 0; i < size; i++) {
            PyObject *arg = PyTuple_GET_ITEM(args, i);
            if (add_object_to_union_args(args_list, args_set, arg) < 0) {
                Py_DECREF(args_list);
                Py_DECREF(args_set);
                return NULL;
            }
        }
    }
    Py_DECREF(args_set);
    if (PyList_GET_SIZE(args_list) == 0) {
        Py_DECREF(args_list);
        PyErr_SetString(PyExc_TypeError, "Cannot take a Union of no types.");
        return NULL;
    }
    else if (PyList_GET_SIZE(args_list) == 1) {
        PyObject *result = PyList_GET_ITEM(args_list, 0);
        Py_INCREF(result);
        Py_DECREF(args_list);
        return result;
    }
    PyObject *args_tuple = PyList_AsTuple(args_list);
    Py_DECREF(args_list);
    if (args_tuple == NULL) {
        return NULL;
    }
    PyObject *u = make_union(args_tuple);
    Py_DECREF(args_tuple);
    return u;
}

static PyObject *
union_class_getitem(PyObject *cls, PyObject *args)
{
    return _Py_union_from_tuple(args);
}

static PyObject *
union_mro_entries(PyObject *self, PyObject *args)
{
    return PyErr_Format(PyExc_TypeError,
                        "Cannot subclass %R", self);
}

static PyMethodDef union_methods[] = {
    {"__mro_entries__", union_mro_entries, METH_O},
    {"__class_getitem__", union_class_getitem, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {0}
};

PyTypeObject _PyUnion_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.Union",
    .tp_doc = PyDoc_STR("Represent a union type\n"
              "\n"
              "E.g. for int | str"),
    .tp_basicsize = sizeof(unionobject),
    .tp_dealloc = unionobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = union_traverse,
    .tp_hash = union_hash,
    .tp_getattro = union_getattro,
    .tp_members = union_members,
    .tp_methods = union_methods,
    .tp_richcompare = union_richcompare,
    .tp_as_mapping = &union_as_mapping,
    .tp_as_number = &union_as_number,
    .tp_repr = union_repr,
    .tp_getset = union_properties,
    .tp_weaklistoffset = offsetof(unionobject, weakreflist),
};

static PyObject *
make_union(PyObject *args)
{
    assert(PyTuple_CheckExact(args));

    unionobject *result = PyObject_GC_New(unionobject, &_PyUnion_Type);
    if (result == NULL) {
        return NULL;
    }

    result->parameters = NULL;
    result->args = Py_NewRef(args);
    result->weakreflist = NULL;
    _PyObject_GC_TRACK(result);
    return (PyObject*)result;
}
