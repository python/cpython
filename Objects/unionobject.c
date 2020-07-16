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
    // Py_XDECREF(alias->origin);
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
    unionobject *alias = (unionobject *) self;
    return PyObject_GenericGetAttr(alias, name);
}

// TODO: MM: Implement this for isinstance checks
static PyObject *
union_instancecheck(PyObject *self, PyObject *instance)
{
    unionobject *alias = (unionobject *) self;
    Py_ssize_t nargs = PyTuple_GET_SIZE(alias->args);
    int retval;
    for (Py_ssize_t iarg = 0; iarg < nargs; iarg++) {
        PyObject *arg = PyTuple_GET_ITEM(alias->args, iarg);
        if (PyType_Check(arg)) {
            retval = PyObject_IsInstance(instance, arg);
        }
    }
    return self;
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
    {0}
};

static PyObject *
union_richcompare(PyObject *a, PyObject *b, int op)
{
    unionobject *aa = (unionobject *)a;
    if (is_typing_name(b, "_UnionGenericAlias")) {
        PyObject* b_args = PyObject_GetAttrString(b, "__args__");
        printf("\n: Aa->args");
        // PyObject_Print(b_args, stdout, 0);
        PyObject_Print(aa->args, stdout, 0);
        return PyObject_RichCompare(aa->args, b_args, Py_EQ);
    }
    PyTypeObject *type = Py_TYPE(b);
    if (strcmp(type->tp_name, "typing.Union") != 0) {
        unionobject *bb = (unionobject *)a;
        return PyObject_RichCompare(aa->args, bb->args, Py_EQ);
    }
    Py_RETURN_FALSE;
}


PyTypeObject Py_UnionType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.Union",
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

    alias->args = args;
    _PyObject_GC_TRACK(alias);
    return (PyObject *) alias;
}
