/* row.c - an enhanced tuple for database rows
 *
 * Copyright (C) 2005-2010 Gerhard Häring <gh@ghaering.de>
 *
 * This file is part of pysqlite.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "row.h"
#include "cursor.h"

void pysqlite_row_dealloc(pysqlite_Row* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    Py_XDECREF(self->data);
    Py_XDECREF(self->description);

    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
pysqlite_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    pysqlite_Row *self;
    PyObject* data;
    pysqlite_Cursor* cursor;

    assert(type != NULL && type->tp_alloc != NULL);

    if (!_PyArg_NoKeywords("Row", kwargs))
        return NULL;
    if (!PyArg_ParseTuple(args, "OO", &cursor, &data))
        return NULL;

    if (!PyObject_TypeCheck((PyObject*)cursor, pysqlite_CursorType)) {
        PyErr_SetString(PyExc_TypeError, "instance of cursor required for first argument");
        return NULL;
    }

    if (!PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "tuple required for second argument");
        return NULL;
    }

    self = (pysqlite_Row *) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(data);
    self->data = data;

    Py_INCREF(cursor->description);
    self->description = cursor->description;

    return (PyObject *) self;
}

PyObject* pysqlite_row_item(pysqlite_Row* self, Py_ssize_t idx)
{
   PyObject* item = PyTuple_GetItem(self->data, idx);
   Py_XINCREF(item);
   return item;
}

static int
equal_ignore_case(PyObject *left, PyObject *right)
{
    int eq = PyObject_RichCompareBool(left, right, Py_EQ);
    if (eq) { /* equal or error */
        return eq;
    }
    if (!PyUnicode_Check(left) || !PyUnicode_Check(right)) {
        return 0;
    }
    if (!PyUnicode_IS_ASCII(left) || !PyUnicode_IS_ASCII(right)) {
        return 0;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(left);
    if (PyUnicode_GET_LENGTH(right) != len) {
        return 0;
    }
    const Py_UCS1 *p1 = PyUnicode_1BYTE_DATA(left);
    const Py_UCS1 *p2 = PyUnicode_1BYTE_DATA(right);
    for (; len; len--, p1++, p2++) {
        if (Py_TOLOWER(*p1) != Py_TOLOWER(*p2)) {
            return 0;
        }
    }
    return 1;
}

PyObject* pysqlite_row_subscript(pysqlite_Row* self, PyObject* idx)
{
    Py_ssize_t _idx;
    Py_ssize_t nitems, i;
    PyObject* item;

    if (PyLong_Check(idx)) {
        _idx = PyNumber_AsSsize_t(idx, PyExc_IndexError);
        if (_idx == -1 && PyErr_Occurred())
            return NULL;
        if (_idx < 0)
           _idx += PyTuple_GET_SIZE(self->data);
        item = PyTuple_GetItem(self->data, _idx);
        Py_XINCREF(item);
        return item;
    } else if (PyUnicode_Check(idx)) {
        nitems = PyTuple_Size(self->description);

        for (i = 0; i < nitems; i++) {
            PyObject *obj;
            obj = PyTuple_GET_ITEM(self->description, i);
            obj = PyTuple_GET_ITEM(obj, 0);
            int eq = equal_ignore_case(idx, obj);
            if (eq < 0) {
                return NULL;
            }
            if (eq) {
                /* found item */
                item = PyTuple_GetItem(self->data, i);
                Py_INCREF(item);
                return item;
            }
        }

        PyErr_SetString(PyExc_IndexError, "No item with that key");
        return NULL;
    }
    else if (PySlice_Check(idx)) {
        return PyObject_GetItem(self->data, idx);
    }
    else {
        PyErr_SetString(PyExc_IndexError, "Index must be int or string");
        return NULL;
    }
}

static Py_ssize_t
pysqlite_row_length(pysqlite_Row* self)
{
    return PyTuple_GET_SIZE(self->data);
}

PyObject* pysqlite_row_keys(pysqlite_Row* self, PyObject *Py_UNUSED(ignored))
{
    PyObject* list;
    Py_ssize_t nitems, i;

    list = PyList_New(0);
    if (!list) {
        return NULL;
    }
    nitems = PyTuple_Size(self->description);

    for (i = 0; i < nitems; i++) {
        if (PyList_Append(list, PyTuple_GET_ITEM(PyTuple_GET_ITEM(self->description, i), 0)) != 0) {
            Py_DECREF(list);
            return NULL;
        }
    }

    return list;
}

static PyObject* pysqlite_iter(pysqlite_Row* self)
{
    return PyObject_GetIter(self->data);
}

static Py_hash_t pysqlite_row_hash(pysqlite_Row *self)
{
    return PyObject_Hash(self->description) ^ PyObject_Hash(self->data);
}

static PyObject* pysqlite_row_richcompare(pysqlite_Row *self, PyObject *_other, int opid)
{
    if (opid != Py_EQ && opid != Py_NE)
        Py_RETURN_NOTIMPLEMENTED;

    if (PyObject_TypeCheck(_other, pysqlite_RowType)) {
        pysqlite_Row *other = (pysqlite_Row *)_other;
        int eq = PyObject_RichCompareBool(self->description, other->description, Py_EQ);
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

static PyMethodDef row_methods[] = {
    {"keys", (PyCFunction)pysqlite_row_keys, METH_NOARGS,
        PyDoc_STR("Returns the keys of the row.")},
    {NULL, NULL}
};

static PyType_Slot row_slots[] = {
    {Py_tp_dealloc, pysqlite_row_dealloc},
    {Py_tp_hash, pysqlite_row_hash},
    {Py_tp_methods, row_methods},
    {Py_tp_richcompare, pysqlite_row_richcompare},
    {Py_tp_iter, pysqlite_iter},
    {Py_mp_length, pysqlite_row_length},
    {Py_mp_subscript, pysqlite_row_subscript},
    {Py_sq_length, pysqlite_row_length},
    {Py_sq_item, pysqlite_row_item},
    {Py_tp_new, pysqlite_row_new},
    {0, NULL},
};

static PyType_Spec row_spec = {
    .name = MODULE_NAME ".Row",
    .basicsize = sizeof(pysqlite_Row),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = row_slots,
};

PyTypeObject *pysqlite_RowType = NULL;

extern int pysqlite_row_setup_types(PyObject *module)
{
    pysqlite_RowType = (PyTypeObject *)PyType_FromModuleAndSpec(module, &row_spec, NULL);
    if (pysqlite_RowType == NULL) {
        return -1;
    }
    return 0;
}
