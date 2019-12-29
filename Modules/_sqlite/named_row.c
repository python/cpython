/* named_row.c - an row_factory with attribute access for database rows
 *
 * Copyright (C) 2019 Clinton James <clinton@jidn.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 */

#include "named_row.h"
#include "cursor.h"

PyDoc_STRVAR(named_row__doc__,
    "NamedRow is an optimized row_factory for column name access.\n"
    "\n"
    "NamedRow supports mapping by attribute, column name, and index.\n"
    "Iteration gives field/value pairs similar to dict.items().\n"
    "For attribute names that would be illegal, either alter the SQL\n"
    "using the 'AS' statement (SELECT count(*) AS count FROM ...) \n"
    "or index by name as in `row['count(*)']`.\n"
    );

static void
named_row_dealloc(pysqlite_NamedRow* self)
{
    Py_XDECREF(self->data);
    Py_XDECREF(self->fields);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject*
named_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject* data;
    pysqlite_Cursor* cursor;

    assert(type != NULL && type->tp_alloc != NULL);

    if (!_PyArg_NoKeywords("NamedRow", kwargs)) {
        return NULL;
    }
    if (!PyArg_ParseTuple(args, "OO", &cursor, &data)) {
        return NULL;
    }

    if (!PyObject_TypeCheck((PyObject*)cursor, &pysqlite_CursorType)) {
        PyErr_SetString(PyExc_TypeError,
                        "instance of cursor required for first argument");
        return NULL;
    }

    if (!PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "tuple required for second argument");
        return NULL;
    }

    pysqlite_NamedRow *self = (pysqlite_NamedRow *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    Py_INCREF(data);
    self->data = data;

    Py_INCREF(cursor->description);
    self->fields = cursor->description;

    return (PyObject *) self;
}

// Get data value by index
static PyObject*
named_row_item(pysqlite_NamedRow* self, Py_ssize_t idx)
{
   PyObject* item = PyTuple_GetItem(self->data, idx);
   Py_XINCREF(item);
   return item;
}

// Find index from field name
static Py_ssize_t
named_row_find_index(pysqlite_NamedRow* self, PyObject* name)
{
    Py_ssize_t name_len = PyUnicode_GET_LENGTH(name);
    Py_ssize_t nitems = PyTuple_Size(self->fields);

    for (Py_ssize_t i = 0; i < nitems; i++) {
        PyObject* obj = PyTuple_GET_ITEM(PyTuple_GET_ITEM(self->fields, i), 0);

        if (name_len != PyUnicode_GET_LENGTH(obj)) {
            continue;
        }

        Py_ssize_t len = name_len;
        const Py_UCS1 *p1 = PyUnicode_1BYTE_DATA(name);
        const Py_UCS1 *p2 = PyUnicode_1BYTE_DATA(obj);
        for (; len; len--, p1++, p2++) {
            if (*p1 == *p2) {
                return i;
            }
        }
    }
    return -1;
}

static PyObject*
named_row_getattro(pysqlite_NamedRow* self, PyObject* attr_obj)
{
    // Calling function checked `name` is PyUnicode, no need to check here.
    Py_INCREF(attr_obj);
    Py_ssize_t idx = named_row_find_index(self, attr_obj);
    Py_DECREF(attr_obj);

    if (idx < 0) {
        return PyObject_GenericGetAttr((PyObject *)self, attr_obj);
    }

    PyObject* value = PyTuple_GET_ITEM(self->data, idx);
    Py_INCREF(value);
    return value;
}

static int
named_row_setattro(PyObject* self, PyObject* name, PyObject* value)
{
    PyErr_Format(PyExc_TypeError,
                 "'%.50s' object does not support item assignment",
                 Py_TYPE(self)->tp_name);
    return -1;
}

// Find the data value by either number or string
static PyObject*
named_row_index(pysqlite_NamedRow* self, PyObject* index)
{
    PyObject* item;

    if (PyLong_Check(index)) {
        // Process integer index
        Py_ssize_t idx = PyNumber_AsSsize_t(index, PyExc_IndexError);
        if (idx == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if (idx < 0) {
           idx += PyTuple_GET_SIZE(self->data);
        }
        item = PyTuple_GetItem(self->data, idx);
        Py_XINCREF(item);
        return item;
    }
    else if (PyUnicode_Check(index)) {
        // Process string index, dict like
        Py_ssize_t idx = named_row_find_index(self, index);

        if (idx < 0) {
            PyErr_SetString(PyExc_IndexError, "No item with that key");
            return NULL;
        }
        item = PyTuple_GetItem(self->data, idx);
        Py_INCREF(item);
        return item;
    }
    else if (PySlice_Check(index)) {
        return PyObject_GetItem(self->data, index);
    }
    else {
        PyErr_SetString(PyExc_IndexError, "Index must be int or str");
        return NULL;
    }
}

static Py_ssize_t
named_row_length(pysqlite_NamedRow* self)
{
    return PyTuple_GET_SIZE(self->data);
}

// Check for a field name in NamedRow
static int
named_row_contains(pysqlite_NamedRow* self, PyObject* name)
{
    if (PyUnicode_Check(name)) {
        return named_row_find_index(self, name) < 0 ? 0 : 1;
    }
    return -1;
}

static Py_hash_t
named_row_hash(pysqlite_NamedRow* self)
{
    return PyObject_Hash(self->fields) ^ PyObject_Hash(self->data);
}

static PyObject*
named_row_richcompare(pysqlite_NamedRow *self, PyObject *_other, int opid)
{
    if (opid != Py_EQ && opid != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (PyObject_TypeCheck(_other, &pysqlite_NamedRowType)) {
        pysqlite_NamedRow *other = (pysqlite_NamedRow *)_other;
        int eq = PyObject_RichCompareBool(self->fields, other->fields, Py_EQ);
        if (eq < 0) {
            return NULL;
        }
        if (eq) {
            return PyObject_RichCompare(self->data, other->data, opid);
        }
        return PyBool_FromLong(opid != Py_EQ);
    }
    Py_RETURN_NOTIMPLEMENTED;
}

PyMappingMethods named_row_as_mapping = {
    .mp_length = (lenfunc)named_row_length,
    .mp_subscript = (binaryfunc)named_row_index,
};

static PySequenceMethods named_row_as_sequence = {
    .sq_length = (lenfunc)named_row_length,
    .sq_item = (ssizeargfunc)named_row_item,
    .was_sq_slice = (ssizeargfunc)named_row_item,
    .sq_contains = (objobjproc)named_row_contains,
};

static PyObject* named_row_iterator(pysqlite_NamedRow* self);

PyTypeObject pysqlite_NamedRowType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name        = MODULE_NAME ".NamedRow",
    .tp_doc         = named_row__doc__,
    .tp_flags       = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    .tp_basicsize   = sizeof(pysqlite_NamedRow),
    .tp_dealloc     = (destructor)named_row_dealloc,
    .tp_as_sequence = &named_row_as_sequence,
    .tp_hash        = (hashfunc)named_row_hash,
    .tp_getattro    = (getattrofunc)named_row_getattro,
    .tp_setattro    = (setattrofunc)named_row_setattro,
    .tp_richcompare = (richcmpfunc)named_row_richcompare,
    .tp_iter        = (getiterfunc)named_row_iterator,
};

extern int pysqlite_named_row_setup_types(void)
{
    pysqlite_NamedRowType.tp_new = named_row_new;
    pysqlite_NamedRowType.tp_as_mapping = &named_row_as_mapping;
    pysqlite_NamedRowType.tp_as_sequence = &named_row_as_sequence;
    return PyType_Ready(&pysqlite_NamedRowType);
}


/***************** Iterator *****************/

typedef struct _NamedRowIter{
    PyObject_HEAD
    Py_ssize_t idx;
    Py_ssize_t len;
    pysqlite_NamedRow *row;
} NamedRowIter;

static void
named_row_iter_dealloc(NamedRowIter *it)
{
    Py_XDECREF(it->row);
}

static PyObject*
named_row_iter_iter(PyObject *self)
{
    Py_INCREF(self);
    return self;
}

static PyObject*
named_row_iter_next(PyObject *self)
{
    NamedRowIter *p = (NamedRowIter *)self;
    if (p->idx >= p->len)
    {
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
    PyObject *item = PyTuple_New(2);

    // Get the field name
    PyObject *cursor_descript_field = PyTuple_GET_ITEM(p->row->fields, p->idx);
    PyObject *key = PyTuple_GET_ITEM(cursor_descript_field, 0);
    Py_INCREF(key);
    PyTuple_SET_ITEM(item, 0, key);

    // Get the value
    PyObject *value = PyTuple_GET_ITEM(p->row->data, p->idx);
    Py_INCREF(value);
    PyTuple_SET_ITEM(item, 1, value);
    (p->idx)++;
    return item;
}

PyTypeObject NamedRowIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name        = MODULE_NAME ".NamedRowIter",
    .tp_doc         = "Internal NamedRow iterator",
    .tp_flags       = Py_TPFLAGS_DEFAULT,
    .tp_basicsize   = sizeof(NamedRowIter),
    .tp_dealloc     = (destructor)named_row_iter_dealloc,
    .tp_iter        = named_row_iter_iter,
    .tp_iternext    = named_row_iter_next,
};

static PyObject*
named_row_iterator(pysqlite_NamedRow* self)
{
    NamedRowIter *iter = PyObject_New(NamedRowIter, &NamedRowIterType);
    if (!iter) {
        return NULL;
    }
    Py_INCREF(self);
    iter->idx = 0;
    iter->len = PyTuple_GET_SIZE(self->data);
    iter->row = self;
    return (PyObject *)iter;
}
