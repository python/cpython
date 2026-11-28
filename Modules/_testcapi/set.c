#include "parts.h"
#include "util.h"

static PyObject *
set_get_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySet_GET_SIZE(obj));
}


static PyObject*
test_set_type_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }

    // Ensure that following tests don't modify the object,
    // to ensure that Py_DECREF() will not crash.
    assert(Py_TYPE(obj) == &PyList_Type);
    assert(Py_SIZE(obj) == 0);

    // bpo-39573: Test Py_SET_TYPE() and Py_SET_SIZE() functions.
    Py_SET_TYPE(obj, &PyList_Type);
    Py_SET_SIZE(obj, 0);

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


static PyObject*
_set_get_table_size(PyObject *self_raw, PyObject *arg)
{
    PySetObject *set = _PySet_CAST(arg);
    Py_ssize_t table_size = set->mask + 1;
    return PyLong_FromSsize_t(table_size);
}


typedef struct {
    PyObject_HEAD
    PySetBuilder *builder;
} WriterObject;


static inline int
builder_check(WriterObject *self)
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
    WriterObject *self = (WriterObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->builder = NULL;
    return (PyObject*)self;
}


// Test PySetBuilder_CreateFrozenSet()
static int
builder_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PySetBuilder() takes no keyword arguments");
        return -1;
    }

    Py_ssize_t size;
    PyObject *type;
    if (!PyArg_ParseTuple(args, "On", &type, &size)) {
        return -1;
    }

    int set_type;
    if (type == _PyObject_CAST(&PySet_Type)) {
        set_type = 1;
    }
    else if (type == _PyObject_CAST(&PyFrozenSet_Type)) {
        set_type = 0;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "expect set or frozenset type");
        return -1;
    }

    WriterObject *self = (WriterObject *)self_raw;
    PySetBuilder_Discard(self->builder);

    if (set_type) {
        self->builder = PySetBuilder_CreateSet(size);
    }
    else {
        self->builder = PySetBuilder_CreateFrozenSet(size);
    }
    if (self->builder == NULL) {
        return -1;
    }

    return 0;
}


// Test PySetBuilder_Discard()
static void
builder_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    PySetBuilder_Discard(self->builder);
    tp->tp_free(self);
    Py_DECREF(tp);
}


// Test PySetBuilder_Add()
static PyObject*
builder_add(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PySetBuilder_Add(self->builder, item) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PySetBuilder_AddSteal()
static PyObject*
builder_add_steal(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PySetBuilder_AddSteal(self->builder, Py_NewRef(item)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PySetBuilder_Update()
static PyObject*
builder_update(PyObject *self_raw, PyObject *iterable)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    if (PySetBuilder_Update(self->builder, iterable) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
_builder_update_array(PyObject *self_raw, PyObject *tuple, int steal)
{
    WriterObject *self = (WriterObject *)self_raw;
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

    if (steal) {
        for (Py_ssize_t i = 0; i < len; i++) {
            Py_INCREF(items[i]);
        }
    }

    int res;
    if (steal) {
        res = PySetBuilder_UpdateArraySteal(self->builder, items, len);
    }
    else {
        res = PySetBuilder_UpdateArray(self->builder, items, len);
    }
    if (res < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PySetBuilder_UpdateArray()
static PyObject*
builder_update_array(PyObject *self, PyObject *tuple)
{
    return _builder_update_array(self, tuple, 0);
}


// Test PySetBuilder_UpdateArraySteal()
static PyObject*
builder_update_array_steal(PyObject *self, PyObject *tuple)
{
    return _builder_update_array(self, tuple, 1);
}


static PyObject*
builder_get_table_size(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    PyObject *set = _PyObject_CAST(self->builder);
    return _set_get_table_size(self_raw, set);
}


// Test PySetBuilder_Finish()
static PyObject*
builder_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    PyObject *set = PySetBuilder_Finish(self->builder);
    self->builder = NULL;
    return set;
}


// Test PySetBuilder_Discard()
static PyObject*
builder_discard(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (builder_check(self) < 0) {
        return NULL;
    }

    PySetBuilder_Discard(self->builder);
    self->builder = NULL;
    Py_RETURN_NONE;
}


static PyMethodDef builder_methods[] = {
    {"add", _PyCFunction_CAST(builder_add), METH_O},
    {"add_steal", _PyCFunction_CAST(builder_add_steal), METH_O},
    {"update", _PyCFunction_CAST(builder_update), METH_O},
    {"update_array", _PyCFunction_CAST(builder_update_array), METH_O},
    {"update_array_steal", _PyCFunction_CAST(builder_update_array_steal), METH_O},
    {"get_table_size", _PyCFunction_CAST(builder_get_table_size), METH_NOARGS},
    {"finish", _PyCFunction_CAST(builder_finish), METH_NOARGS},
    {"discard", _PyCFunction_CAST(builder_discard), METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_new, builder_new},
    {Py_tp_init, builder_init},
    {Py_tp_dealloc, builder_dealloc},
    {Py_tp_methods, builder_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Writer_spec = {
    .name = "_testcapi.PySetBuilder",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyMethodDef test_methods[] = {
    {"set_get_size", set_get_size, METH_O},
    {"test_set_type_size", test_set_type_size, METH_NOARGS},
    {"_set_get_table_size", _set_get_table_size, METH_O},
    {NULL},
};


int
_PyTestCapi_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    PyTypeObject *builder_type = (PyTypeObject *)PyType_FromSpec(&Writer_spec);
    if (builder_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, builder_type) < 0) {
        Py_DECREF(builder_type);
        return -1;
    }
    Py_DECREF(builder_type);

    if (PyModule_AddIntMacro(m, PySet_MINSIZE) < 0) {
        return -1;
    }

    return 0;
}
