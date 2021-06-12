#include "Python.h"

typedef struct {
    PyObject_HEAD
    PyObject *mapping;
    Py_hash_t hash;
} jump_table;

static PyTypeObject JumpTable_Type;

PyObject *
_PyJumpTable_New(PyObject *mapping)
{
    PyObject *d = PyDict_Copy(mapping);
    jump_table *table = PyObject_GC_New(jump_table, &JumpTable_Type);
    if (table == NULL) {
        Py_DECREF(d);
        return NULL;
    }
    table->mapping = d;
    table->hash = -1;
    assert(Py_IS_TYPE(table, &JumpTable_Type));
    return (PyObject *)table;
}

/* Function copied from setobject.c. */
static Py_uhash_t
_shuffle_bits(Py_uhash_t h)
{
    return ((h ^ 89869747UL) ^ (h << 16)) * 3644798167UL;
}

/* based on frozenset.__hash__ */
static Py_hash_t
compute_hash(jump_table *jt)
{
    PyObject *d = jt->mapping;
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    Py_hash_t key_hash;
    Py_uhash_t result = 0;
    while (_PyDict_Next(d, &pos, &key, &value, &key_hash)) {
        Py_hash_t value_hash = PyObject_Hash(value);
        if (value_hash == -1) {
            return -1;
        }
        result ^= _shuffle_bits(key_hash);
        result ^= _shuffle_bits(_shuffle_bits(value_hash));
    }
    if (result == (Py_uhash_t)-1) {
        result = 0xc0ffee;
    }
    return result;
}


static Py_hash_t
jump_table_hash(PyObject *table)
{
    jump_table *jt = (jump_table *)table;
    if (jt->hash == -1) {
        jt->hash = compute_hash(jt);
    }
    return jt->hash;
}

int
_PyJumpTable_Check(PyObject *x)
{
    return Py_IS_TYPE(x, &JumpTable_Type);
}

PyObject *
_PyJumpTable_DictCopy(PyObject *mapping) {
    assert(Py_IS_TYPE(mapping, &JumpTable_Type));
    return PyDict_Copy(((jump_table *)mapping)->mapping);
}

int
_PyJumpTable_Get(PyObject *mapping, PyObject *key)
{
    PyObject *res;
    assert(Py_IS_TYPE(mapping, &JumpTable_Type));
    res = PyDict_GetItemWithError(((jump_table *)mapping)->mapping,
                                  key);
    if (res == NULL) {
        if (PyErr_Occurred()) {
            return -2;
        }
        else {
            return -1;
        }
    }
    assert(PyLong_CheckExact(res));
    return (int)PyLong_AsLong(res);
}

static PyObject *
jump_table_getitem(PyObject *mapping, PyObject *key)
{
    assert(Py_IS_TYPE(mapping, &JumpTable_Type));
    PyObject *mapping_mapping = ((jump_table *)mapping)->mapping;
    assert(Py_IS_TYPE(mapping_mapping, &PyDict_Type));
    binaryfunc subscript = PyDict_Type.tp_as_mapping->mp_subscript;
    return subscript(mapping_mapping, key);
}

static PyObject *
jump_table_richcompare(PyObject *self, PyObject *other, int op)
{
    if (!_PyJumpTable_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    return PyDict_Type.tp_richcompare(((jump_table *)self)->mapping,
                                      ((jump_table *)other)->mapping,
                                      op);
}

static PyObject *
jump_table_repr(jump_table *jt)
{
    return PyUnicode_FromFormat("jump_table(%.200R)", jt->mapping);
}

static int
jump_table_clear(jump_table *jt)
{
    Py_CLEAR(jt->mapping);
    return 0;
}

static int
jump_table_traverse(jump_table *jt, visitproc visit, void *arg)
{
    Py_VISIT(jt->mapping);
    return 0;
}

static void
jump_table_dealloc(jump_table *jt)
{
    PyObject_GC_UnTrack(jt);
    Py_XDECREF(jt->mapping);
    Py_TYPE(jt)->tp_free(jt);
}

static PyMappingMethods jump_table_as_mapping = {
    .mp_subscript = (binaryfunc)jump_table_getitem,
};

PyTypeObject JumpTable_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "jump_table",
    .tp_basicsize = sizeof(jump_table),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_dealloc = (destructor)jump_table_dealloc,
    .tp_clear = (inquiry)jump_table_clear,
    .tp_traverse = (traverseproc)jump_table_traverse,
    .tp_free = PyObject_GC_Del,
    .tp_new = NULL,
    .tp_as_mapping = &jump_table_as_mapping,
    .tp_hash = (hashfunc)jump_table_hash,
    .tp_repr = (reprfunc)jump_table_repr,
    .tp_richcompare = (richcmpfunc)jump_table_richcompare,
};
