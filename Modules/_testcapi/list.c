// Use pycore_list.h
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"

#include "pycore_list.h"          // _Py_LIST_SMALL_ALLOCATED


static PyObject *
list_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_GET_SIZE(obj));
}


static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GET_ITEM(obj, i));
}


static PyObject *
list_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyList_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}


static PyObject *
list_clear(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Clear(obj));
}


static PyObject *
list_extend(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *arg;
    if (!PyArg_ParseTuple(args, "OO", &obj, &arg)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(arg);
    RETURN_INT(PyList_Extend(obj, arg));
}


static PyObject*
test_list_api(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* list;
    int i;

    /* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
    list = PyList_New(NLIST);
    if (list == (PyObject*)NULL)
        return (PyObject*)NULL;
    /* list = range(NLIST) */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyLong_FromLong(i);
        if (anint == (PyObject*)NULL) {
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        PyList_SET_ITEM(list, i, anint);
    }
    /* list.reverse(), via PyList_Reverse() */
    i = PyList_Reverse(list);   /* should not blow up! */
    if (i != 0) {
        Py_DECREF(list);
        return (PyObject*)NULL;
    }
    /* Check that list == range(29, -1, -1) now */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyList_GET_ITEM(list, i);
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            PyErr_SetString(PyExc_AssertionError,
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
    }
    Py_DECREF(list);
#undef NLIST

    Py_RETURN_NONE;
}


static PyObject *
_list_get_allocated(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    PyListObject *list = (PyListObject*)obj;
    return PyLong_FromSsize_t(list->allocated);
}


typedef struct {
    PyObject_HEAD
    PyListBuilder *builder;
} BuilderObject;


static inline int
builder_check(BuilderObject *self)
{
    if (self->builder == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "operation on finished/discarded builder");
        return -1;
    }
    return 0;
}


static PyObject *
builder_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    BuilderObject *self = (BuilderObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->builder = NULL;
    return (PyObject*)self;
}


// Test PyListBuilder_Create()
static int
builder_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyListBuilder() takes no keyword arguments");
        return -1;
    }

    Py_ssize_t size = 0;
    if (!PyArg_ParseTuple(args, "|n", &size)) {
        return -1;
    }

    BuilderObject *self = (BuilderObject *)self_raw;
    PyListBuilder_Discard(self->builder);

    self->builder = PyListBuilder_Create(size);
    if (self->builder == NULL) {
        return -1;
    }

    return 0;
}


// Test PyListBuilder_Discard()
static void
builder_dealloc(PyObject *self_raw)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->builder) {
        PyListBuilder_Discard(self->builder);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


// Test PyListBuilder_Append()
static PyObject*
builder_append(PyObject *self_raw, PyObject *item)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PyListBuilder_Append(self->builder, item) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListBuilder_AppendSteal()
static PyObject*
builder_append_steal(PyObject *self_raw, PyObject *item)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PyListBuilder_AppendSteal(self->builder, Py_NewRef(item)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListBuilder_Extend()
static PyObject*
builder_extend(PyObject *self_raw, PyObject *iterable)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PyListBuilder_Extend(self->builder, iterable) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListBuilder_ExtendArray()
static PyObject*
builder_extend_array(PyObject *self_raw, PyObject *tuple)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (!PyTuple_Check(tuple)) {
        PyErr_Format(PyExc_TypeError,
                     "expect tuple, got %T", tuple);
        return NULL;
    }
    PyObject **items = &PyTuple_GET_ITEM(tuple, 0);
    Py_ssize_t len = PyTuple_GET_SIZE(tuple);

    if (PyListBuilder_ExtendArray(self->builder, items, len) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListBuilder_ExtendArraySteal()
static PyObject*
builder_extend_array_steal(PyObject *self_raw, PyObject *tuple)
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (!PyTuple_Check(tuple)) {
        PyErr_Format(PyExc_TypeError,
                     "expect tuple, got %T", tuple);
        return NULL;
    }
    PyObject **items = &PyTuple_GET_ITEM(tuple, 0);
    Py_ssize_t len = PyTuple_GET_SIZE(tuple);

    for (Py_ssize_t i = 0; i < len; i++) {
        Py_INCREF(items[i]);
    }

    if (PyListBuilder_ExtendArraySteal(self->builder, items, len) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListBuilder_Finish()
static PyObject*
builder_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    PyObject *list = PyListBuilder_Finish(self->builder);
    self->builder = NULL;
    return list;
}


// Test PyListBuilder_Discard()
static PyObject*
builder_discard(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    BuilderObject *self = (BuilderObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    PyListBuilder_Discard(self->builder);
    self->builder = NULL;
    Py_RETURN_NONE;
}


static PyMethodDef builder_methods[] = {
    {"append", _PyCFunction_CAST(builder_append), METH_O},
    {"append_steal", _PyCFunction_CAST(builder_append_steal), METH_O},
    {"extend", _PyCFunction_CAST(builder_extend), METH_O},
    {"extend_array", _PyCFunction_CAST(builder_extend_array), METH_O},
    {"extend_array_steal", _PyCFunction_CAST(builder_extend_array_steal), METH_O},
    {"finish", _PyCFunction_CAST(builder_finish), METH_NOARGS},
    {"discard", _PyCFunction_CAST(builder_discard), METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot Builder_Type_slots[] = {
    {Py_tp_new, builder_new},
    {Py_tp_init, builder_init},
    {Py_tp_dealloc, builder_dealloc},
    {Py_tp_methods, builder_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Builder_spec = {
    .name = "_testcapi.PyListBuilder",
    .basicsize = sizeof(BuilderObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Builder_Type_slots,
};


static PyMethodDef test_methods[] = {
    {"list_get_size", list_get_size, METH_O},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},
    {"test_list_api", test_list_api, METH_NOARGS},
    {"_list_get_allocated", _list_get_allocated, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    PyTypeObject *builder_type = (PyTypeObject *)PyType_FromSpec(&Builder_spec);
    if (builder_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, builder_type) < 0) {
        Py_DECREF(builder_type);
        return -1;
    }
    Py_DECREF(builder_type);

    if (PyModule_AddIntConstant(m, "LIST_SMALL_ALLOCATED", _Py_LIST_SMALL_ALLOCATED) < 0) {
        return -1;
    }

    return 0;
}
