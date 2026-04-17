/* Sentinel object implementation */

#include "Python.h"
#include "pycore_ceval.h"         // _PyThreadState_GET()
#include "pycore_interpframe.h"   // _PyFrame_IsIncomplete()
#include "pycore_object.h"        // _PyObject_GC_TRACK/UNTRACK()
#include "pycore_sentinelobject.h"
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectBorrow()
#include "pycore_typeobject.h"    // _Py_BaseObject_RichCompare()
#include "pycore_unionobject.h"   // _Py_union_from_tuple()
#include "structmember.h"         // PyMemberDef

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *module;
} sentinelobject;

#define sentinelobject_CAST(op) \
    (assert(_PySentinel_Check(op)), _Py_CAST(sentinelobject *, (op)))

/*[clinic input]
class sentinel "sentinelobject *" "&PySentinel_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8b88f8268d3b5775]*/

#include "clinic/sentinelobject.c.h"

static PyObject *
sentinel_get_caller_module(void)
{
    _PyInterpreterFrame *f = _PyThreadState_GET()->current_frame;
    while (f && _PyFrame_IsIncomplete(f)) {
        f = f->previous;
    }
    if (f != NULL && !PyStackRef_IsNull(f->f_funcobj)) {
        PyObject *module = PyFunction_GetModule(
            PyStackRef_AsPyObjectBorrow(f->f_funcobj));
        if (module != NULL && module != Py_None) {
            return Py_NewRef(module);
        }
        PyErr_Clear();
    }
    return PyUnicode_FromString("builtins");
}

/*[clinic input]
@classmethod
sentinel.__new__ as sentinel_new

    name: object(subclass_of='&PyUnicode_Type')
    /
[clinic start generated code]*/

static PyObject *
sentinel_new_impl(PyTypeObject *type, PyObject *name)
/*[clinic end generated code: output=4af55c6048bed30d input=3ab75704f39c119c]*/
{
    Py_INCREF(name);
    PyObject *module = sentinel_get_caller_module();
    if (module == NULL) {
        Py_DECREF(name);
        return NULL;
    }

    sentinelobject *self = PyObject_GC_New(sentinelobject, type);
    if (self == NULL) {
        Py_DECREF(name);
        Py_DECREF(module);
        return NULL;
    }
    self->name = name;
    self->module = module;
    _PyObject_GC_TRACK(self);
    return (PyObject *)self;
}

static void
sentinel_dealloc(PyObject *op)
{
    _PyObject_GC_UNTRACK(op);
    sentinelobject *self = sentinelobject_CAST(op);
    Py_CLEAR(self->name);
    Py_CLEAR(self->module);
    Py_TYPE(op)->tp_free(op);
}

static int
sentinel_traverse(PyObject *op, visitproc visit, void *arg)
{
    sentinelobject *self = sentinelobject_CAST(op);
    Py_VISIT(self->name);
    Py_VISIT(self->module);
    return 0;
}

static int
sentinel_clear(PyObject *op)
{
    sentinelobject *self = sentinelobject_CAST(op);
    Py_CLEAR(self->name);
    Py_CLEAR(self->module);
    return 0;
}

static PyObject *
sentinel_repr(PyObject *op)
{
    sentinelobject *self = sentinelobject_CAST(op);
    return Py_NewRef(self->name);
}

static PyObject *
sentinel_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_NewRef(self);
}

static PyObject *
sentinel_deepcopy(PyObject *self, PyObject *Py_UNUSED(memo))
{
    return Py_NewRef(self);
}

static PyObject *
sentinel_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    sentinelobject *self = sentinelobject_CAST(op);
    return Py_NewRef(self->name);
}

static PyObject *
sentinel_or(PyObject *self, PyObject *other)
{
    PyObject *args = PyTuple_Pack(2, self, other);
    if (args == NULL) {
        return NULL;
    }
    PyObject *result = _Py_union_from_tuple(args);
    Py_DECREF(args);
    return result;
}

static PyMethodDef sentinel_methods[] = {
    {"__copy__", sentinel_copy, METH_NOARGS, NULL},
    {"__deepcopy__", sentinel_deepcopy, METH_O, NULL},
    {"__reduce__", sentinel_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

static PyMemberDef sentinel_members[] = {
    {"__name__", Py_T_OBJECT_EX, offsetof(sentinelobject, name), Py_READONLY},
    {"__module__", Py_T_OBJECT_EX, offsetof(sentinelobject, module), Py_READONLY},
    {NULL}
};

static PyNumberMethods sentinel_as_number = {
    .nb_or = sentinel_or,
};

PyDoc_STRVAR(sentinel_doc,
"sentinel(name, /)\n"
"--\n\n"
"Create a unique sentinel object with the given name.");

PyTypeObject PySentinel_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "sentinel",
    .tp_basicsize = sizeof(sentinelobject),
    .tp_dealloc = sentinel_dealloc,
    .tp_repr = sentinel_repr,
    .tp_as_number = &sentinel_as_number,
    .tp_hash = PyObject_GenericHash,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
                | Py_TPFLAGS_HAVE_GC,
    .tp_doc = sentinel_doc,
    .tp_traverse = sentinel_traverse,
    .tp_clear = sentinel_clear,
    .tp_richcompare = _Py_BaseObject_RichCompare,
    .tp_methods = sentinel_methods,
    .tp_members = sentinel_members,
    .tp_new = sentinel_new,
    .tp_free = PyObject_GC_Del,
};
