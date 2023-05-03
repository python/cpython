// types.IntersectionType -- used to represent e.g. int & str
#include "Python.h"
#include "pycore_object.h"  // _PyObject_GC_TRACK/UNTRACK
#include "pycore_intersectionobject.h"
#include "structmember.h"


static PyObject *make_intersection(PyObject *);


typedef struct {
    PyObject_HEAD
    PyObject *args;
    PyObject *parameters;
} intersectionobject;

static void
intersectionobject_dealloc(PyObject *self)
{
    intersectionobject *alias = (intersectionobject *)self;

    _PyObject_GC_UNTRACK(self);

    Py_XDECREF(alias->args);
    Py_XDECREF(alias->parameters);
    Py_TYPE(self)->tp_free(self);
}

static int
intersection_traverse(PyObject *self, visitproc visit, void *arg)
{
    intersectionobject *alias = (intersectionobject *)self;
    Py_VISIT(alias->args);
    Py_VISIT(alias->parameters);
    return 0;
}

static Py_hash_t
intersection_hash(PyObject *self)
{
    intersectionobject *alias = (intersectionobject *)self;
    PyObject *args = PyFrozenSet_New(alias->args);
    if (args == NULL) {
        return (Py_hash_t)-1;
    }
    Py_hash_t hash = PyObject_Hash(args);
    Py_DECREF(args);
    return hash;
}

static PyObject *
intersection_richcompare(PyObject *a, PyObject *b, int op)
{
    if (!_PyIntersection_Check(b) || (op != Py_EQ && op != Py_NE)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    PyObject *a_set = PySet_New(((intersectionobject*)a)->args);
    if (a_set == NULL) {
        return NULL;
    }
    PyObject *b_set = PySet_New(((intersectionobject*)b)->args);
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
    if (_PyIntersection_Check(*obj)) {
        PyObject *args = ((intersectionobject *) *obj)->args;
        *size = PyTuple_GET_SIZE(args);
        return &PyTuple_GET_ITEM(args, 0);
    }
    else {
        *size = 1;
        return obj;
    }
}

PyObject *
_Py_intersection_type_and(PyObject* self, PyObject* other)
{
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

    PyObject *new_intersection = make_intersection(tuple);
    Py_DECREF(tuple);
    return new_intersection;
}

static int
intersection_repr_item(_PyUnicodeWriter *writer, PyObject *p)
{
    PyObject *qualname = NULL;
    PyObject *module = NULL;
    PyObject *tmp;
    PyObject *r = NULL;
    int err;

    if (p == (PyObject *)&_PyNone_Type) {
        return _PyUnicodeWriter_WriteASCIIString(writer, "None", 4);
    }

    if (_PyObject_LookupAttr(p, &_Py_ID(__origin__), &tmp) < 0) {
        goto exit;
    }

    if (tmp) {
        Py_DECREF(tmp);
        if (_PyObject_LookupAttr(p, &_Py_ID(__args__), &tmp) < 0) {
            goto exit;
        }
        if (tmp) {
            // It looks like a GenericAlias
            Py_DECREF(tmp);
            goto use_repr;
        }
    }

    if (_PyObject_LookupAttr(p, &_Py_ID(__qualname__), &qualname) < 0) {
        goto exit;
    }
    if (qualname == NULL) {
        goto use_repr;
    }
    if (_PyObject_LookupAttr(p, &_Py_ID(__module__), &module) < 0) {
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
intersection_repr(PyObject *self)
{
    intersectionobject *alias = (intersectionobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);

    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);
     for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0 && _PyUnicodeWriter_WriteASCIIString(&writer, " & ", 3) < 0) {
            goto error;
        }
        PyObject *p = PyTuple_GET_ITEM(alias->args, i);
        if (intersection_repr_item(&writer, p) < 0) {
            goto error;
        }
    }
    return _PyUnicodeWriter_Finish(&writer);
error:
    _PyUnicodeWriter_Dealloc(&writer);
    return NULL;
}

static PyMemberDef intersection_members[] = {
        {"__args__", T_OBJECT, offsetof(intersectionobject, args), READONLY},
        {0}
};

static PyObject *
intersection_getitem(PyObject *self, PyObject *item)
{
    intersectionobject *alias = (intersectionobject *)self;
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
        res = make_intersection(newargs);
    }
    else {
        res = Py_NewRef(PyTuple_GET_ITEM(newargs, 0));
        for (Py_ssize_t iarg = 1; iarg < nargs; iarg++) {
            PyObject *arg = PyTuple_GET_ITEM(newargs, iarg);
            Py_SETREF(res, PyNumber_And(res, arg));
            if (res == NULL) {
                break;
            }
        }
    }
    Py_DECREF(newargs);
    return res;
}

static PyMappingMethods intersection_as_mapping = {
    .mp_subscript = intersection_getitem,
};

static PyObject *
intersection_parameters(PyObject *self, void *Py_UNUSED(unused))
{
    intersectionobject *alias = (intersectionobject *)self;
    if (alias->parameters == NULL) {
        alias->parameters = _Py_make_parameters(alias->args);
        if (alias->parameters == NULL) {
            return NULL;
        }
    }
    return Py_NewRef(alias->parameters);
}

static PyGetSetDef intersection_properties[] = {
    {"__parameters__", intersection_parameters, (setter)NULL, "Type variables in the types.IntersectionType.", NULL},
    {0}
};

static PyNumberMethods intersection_as_number = {
        .nb_or = _Py_union_type_or, // Add __or__ function
        .nb_and = _Py_intersection_type_and, // Add __and__ function
};

static const char* const cls_attrs[] = {
        "__module__",  // Required for compatibility with typing module
        NULL,
};

static PyObject *
intersection_getattro(PyObject *self, PyObject *name)
{
    intersectionobject *alias = (intersectionobject *)self;
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
_Py_intersection_args(PyObject *self)
{
    assert(_PyIntersection_Check(self));
    return ((intersectionobject *) self)->args;
}

PyTypeObject _PyIntersection_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "types.IntersectionType",
    .tp_doc = PyDoc_STR("Represent an intersection type\n"
              "\n"
              "E.g. for A & B"),
    .tp_basicsize = sizeof(intersectionobject),
    .tp_dealloc = intersectionobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = intersection_traverse,
    .tp_hash = intersection_hash,
    .tp_getattro = intersection_getattro,
    .tp_members = intersection_members,
    .tp_richcompare = intersection_richcompare,
    .tp_as_mapping = &intersection_as_mapping,
    .tp_as_number = &intersection_as_number,
    .tp_repr = intersection_repr,
    .tp_getset = intersection_properties,
    .tp_new = make_intersection,
};

static PyObject *
make_intersection(PyObject *args)
{
    assert(PyTuple_CheckExact(args));

    intersectionobject *result = PyObject_GC_New(intersectionobject, &_PyIntersection_Type);
    if (result == NULL) {
        return NULL;
    }

    result->parameters = NULL;
    result->args = Py_NewRef(args);
    _PyObject_GC_TRACK(result);
    return (PyObject*)result;
}
