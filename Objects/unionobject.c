// types.Union -- used to represent e.g. Union[int, str], int | str
#include "Python.h"
#include "pycore_object.h"
#include "structmember.h"


typedef struct {
    PyObject_HEAD;
    PyObject *args;
} unionobject;

static void
unionobject_dealloc(PyObject *self)
{
    unionobject *alias = (unionobject *)self;

    _PyObject_GC_UNTRACK(self);
    Py_XDECREF(alias->args);
    self->ob_type->tp_free(self);
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
union_traverse(PyObject *self, visitproc visit, void *arg)
{
    unionobject *alias = (unionobject *)self;
    Py_VISIT(alias->args);
    return 0;
}

static PyMemberDef union_members[] = {
    {"__args__", T_OBJECT, offsetof(unionobject, args), READONLY},
    {0}
};

static PyObject *
union_getattro(PyObject *self, PyObject *name)
{
    return PyObject_GenericGetAttr(self, name);
}

static PyObject *
union_instancecheck(PyObject *self, PyObject *instance)
{
    unionobject *alias = (unionobject *) self;
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        if (PyType_Check(arg)) {
            if (PyObject_IsInstance(instance, arg) != 0)
            {
                return self;
            }
        }
    }
    return instance;
}

static PyObject *
union_subclasscheck(PyObject *self, PyObject *instance)
{
    unionobject *alias = (unionobject *) self;
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        if (PyType_Check(arg)) {
            if (PyType_IsSubtype((PyTypeObject *)instance, (PyTypeObject *)arg) != 0)
            {
                return self;
            }
        }
    }
    return instance;
}

static int
is_typing_name(PyObject *obj, char *name)
{
    PyTypeObject *type = Py_TYPE(obj);
    if (strcmp(type->tp_name, name) != 0) {
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

static PyMethodDef union_methods[] = {
    {"__instancecheck__", union_instancecheck, METH_O},
    {"__subclasscheck__", union_subclasscheck, METH_O},
    {0}};

static PyObject *
union_richcompare(PyObject *a, PyObject *b, int op)
{
    PyObject* b_set = NULL;
    unionobject *aa = (unionobject *)a;
    PyObject* a_set = PySet_New(aa->args);

    if (is_typing_name(b, "_UnionGenericAlias")) {
        PyObject* b_args = PyObject_GetAttrString(b, "__args__");
        b_set = PySet_New(b_args);

    }

    PyTypeObject *type = Py_TYPE(b);
    if (type == &Py_UnionType) {
        unionobject *bb = (unionobject *)b;
        b_set = PySet_New(bb->args);
    }

    if (b_set == NULL) {
        Py_RETURN_FALSE;
    }

    return PyObject_RichCompare(a_set, b_set, Py_EQ);
}

static PyObject*
flatten_args(PyObject* args) {
    int arg_length = PyTuple_GET_SIZE(args);
    int total_args = 0;
    // Get number of args once it's flattened.
    for (int i = 0; i < arg_length; i++) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        PyTypeObject* arg_type = Py_TYPE(arg);
        if (arg_type == &Py_UnionType) {
            unionobject *alias = (unionobject *)arg;
            total_args = total_args + PyTuple_GET_SIZE(alias->args);
        } else {
            total_args++;
        }
    }
    // Create new tuple of flattened args.
    PyObject *flattened_args = PyTuple_New(total_args);
    int pos = 0;
    for (int i = 0; i < arg_length; i++) {
        PyObject *arg = PyTuple_GET_ITEM(args, i);
        PyTypeObject* arg_type = Py_TYPE(arg);
        if (arg_type == &Py_UnionType) {
            unionobject *alias = (unionobject *)arg;
            PyObject* nested_args = alias->args;
            int nested_arg_length = PyTuple_GET_SIZE(nested_args);
            for (int j = 0; j < nested_arg_length; j++) {
                PyObject* nested_arg = PyTuple_GET_ITEM(nested_args, j);
                PyTuple_SET_ITEM(flattened_args, pos, nested_arg);
                pos++;
            }
        } else {
            PyTuple_SET_ITEM(flattened_args, pos, arg);
            pos++;
        }
    }

    return flattened_args;
}

static PyObject*
dedup_and_flatten_args(PyObject* args) {
    args = flatten_args(args);
    int arg_length = PyTuple_GET_SIZE(args);
    PyObject* temp[arg_length];
    for (int i = 0; i < arg_length ; i++) { temp[i] = NULL; }

    // Add unique elements to an array.
    int temp_count = 0;
    for(int i = 0; i < arg_length; i++) {
        int is_duplicate = 0;
        PyObject* i_tuple = PyTuple_GET_ITEM(args, i);
        for(int j = i + 1; j < arg_length; j++) {
            PyObject* j_tuple = PyTuple_GET_ITEM(args, j);
            if (i_tuple == j_tuple) is_duplicate = 1;
        }
        if (!is_duplicate) {
            temp[temp_count] = i_tuple;
            temp_count++;
        }
    }

    // Create new tuple from deduplicated array.
    PyObject* temp_arg = NULL;
    PyObject* new_args = PyTuple_New(temp_count);
    for(int k = 0; k < arg_length; k++) {
        temp_arg = temp[k];

        if (temp_arg != NULL)
            PyTuple_SET_ITEM(new_args, k, temp_arg);
    }

    return new_args;
}


static int
is_typevar(PyObject *obj)
{
    return is_typing_name(obj, "TypeVar");
}

static int
is_genericalias(PyObject *obj)
{
    return is_typing_name(obj, "GenericAlias");
}

static int
is_new_type(PyObject *obj)
{
    if (!PyObject_IsInstance(obj, (PyObject *)&PyFunction_Type)) {
        return 0;
    }
    PyObject *module = PyObject_GetAttrString(obj, "__module__");
    if (module == NULL) {
        return 0;
    }
    return PyUnicode_Check(module) && _PyUnicode_EqualToASCIIString(module, "typing");
}

static int
is_unionable(PyObject *obj)
{
    if (obj == Py_None) {
        return 1;
    }
    return (
        is_genericalias(obj) ||
        is_typevar(obj) ||
        is_new_type(obj) ||
        (PyObject_IsInstance(obj, (PyObject *)&PyType_Type) == 1) ||
    (PyObject_IsInstance(obj, (PyObject *)&Py_UnionType) == 1));
}

static PyObject *
union_new(PyTypeObject* self, PyObject* param) {
    // Check param is a PyType or GenericAlias
    if ((param == NULL) || !is_unionable((PyObject *)param)  || !is_unionable((PyObject *)self))
    {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *param_type = param == Py_None ? (PyObject *)Py_TYPE(param) : param;
    PyTypeObject *self_type = (PyObject *)self == Py_None ? Py_TYPE(self) : self;
    PyObject *tuple=PyTuple_Pack(2, self_type, param_type);
    PyObject *newUnionType = Py_Union(tuple);
    Py_DECREF(param_type);
    Py_DECREF(self_type);
    Py_DECREF(tuple);
    return newUnionType;
}


static PyObject *
type_or(PyTypeObject* self, PyObject* param) {
    return union_new(self, param);
}

static PyNumberMethods union_as_number = {
        .nb_or = (binaryfunc)type_or, // Add __or__ function
};

static int
union_repr_item(_PyUnicodeWriter *writer, PyObject *p)
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
union_repr(PyObject *self)
{
    unionobject *alias = (unionobject *)self;
    Py_ssize_t len = PyTuple_GET_SIZE(alias->args);


    _PyUnicodeWriter writer;
    _PyUnicodeWriter_Init(&writer);
     for (Py_ssize_t i = 0; i < len; i++) {
        if (i > 0) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, " | ", 3) < 0) {
                goto error;
            }
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


PyTypeObject Py_UnionType = {
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
    .tp_hash = union_hash,
    .tp_traverse = union_traverse,
    .tp_getattro = union_getattro,
    .tp_members = union_members,
    .tp_methods = union_methods,
    .tp_richcompare = union_richcompare,
    .tp_as_number = &union_as_number,
    .tp_repr = union_repr,
};



PyObject *
Py_Union(PyObject *args)
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

    unionobject *alias = PyObject_GC_New(unionobject, &Py_UnionType);
    if (alias == NULL) {
        Py_DECREF(args);
        return NULL;
    }

    PyObject* new_args = dedup_and_flatten_args(args);
    alias->args = new_args;
    _PyObject_GC_TRACK(alias);
    return (PyObject *) alias;
}

PyObject *
Py_Union_New(PyTypeObject* self, PyObject* param) {
    PyObject* new_union = union_new(self, param);
    return new_union;
}
