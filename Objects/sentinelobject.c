/* Sentinel object implementation */

#include "Python.h"
#include "descrobject.h"          // PyMemberDef
#include "pycore_ceval.h"         // _PyThreadState_GET()
#include "pycore_interpframe.h"   // _PyFrame_IsIncomplete()
#include "pycore_object.h"        // _PyObject_GC_TRACK/UNTRACK()
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectBorrow()
#include "pycore_tuple.h"         // _PyTuple_FromPair
#include "pycore_typeobject.h"    // _Py_BaseObject_RichCompare()
#include "pycore_unionobject.h"   // _Py_union_type_or()

typedef struct {
    PyObject_HEAD
    PyObject *name;
    PyObject *module;
} sentinelobject;

#define sentinelobject_CAST(op) \
    (assert(PySentinel_Check(op)), _Py_CAST(sentinelobject *, (op)))

/*[clinic input]
class sentinel "sentinelobject *" "&PySentinel_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8b88f8268d3b5775]*/

#include "clinic/sentinelobject.c.h"


static PyObject *
caller(void)
{
    _PyInterpreterFrame *f = _PyThreadState_GET()->current_frame;
    if (f == NULL || PyStackRef_IsNull(f->f_funcobj)) {
        assert(!PyErr_Occurred());
        Py_RETURN_NONE;
    }
    PyFunctionObject *func = _PyFrame_GetFunction(f);
    assert(PyFunction_Check(func));
    PyObject *r = PyFunction_GetModule((PyObject *)func);
    if (!r) {
        assert(!PyErr_Occurred());
        Py_RETURN_NONE;
    }
    return Py_NewRef(r);
}

static PyObject *
sentinel_new_with_module(PyTypeObject *type, PyObject *name, PyObject *module)
{
    assert(PyUnicode_Check(name));

    sentinelobject *self = PyObject_GC_New(sentinelobject, type);
    if (self == NULL) {
        return NULL;
    }
    self->name = Py_NewRef(name);
    self->module = Py_NewRef(module);
    _PyObject_GC_TRACK(self);
    return (PyObject *)self;
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
    PyObject *module = caller();
    PyObject *self = sentinel_new_with_module(type, name, module);
    Py_DECREF(module);
    return self;
}

PyObject *
PySentinel_New(const char *name, const char *module_name)
{
    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        return NULL;
    }
    PyObject *module_obj = module_name == NULL
        ? Py_None
        : PyUnicode_FromString(module_name);
    if (module_obj == NULL) {
        Py_DECREF(name_obj);
        return NULL;
    }

    PyObject *sentinel = sentinel_new_with_module(
        &PySentinel_Type, name_obj, module_obj);
    Py_DECREF(module_obj);
    Py_DECREF(name_obj);
    return sentinel;
}

static int
sentinel_clear(PyObject *op)
{
    sentinelobject *self = sentinelobject_CAST(op);
    Py_CLEAR(self->name);
    Py_CLEAR(self->module);
    return 0;
}

static void
sentinel_dealloc(PyObject *op)
{
    _PyObject_GC_UNTRACK(op);
    (void)sentinel_clear(op);
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
    .nb_or = _Py_union_type_or,
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
