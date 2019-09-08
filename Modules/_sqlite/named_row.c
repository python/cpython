/* named_row.c - an enhansed namedtuple for database rows
 *
 * Copyright (C) 2019 Clinton James <clinton@jidn.com>
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

#include "named_row.h"
#include "cursor.h"
/*
 * Space efficient.  Identical to Row
 * No fancy repr. Just fields?
 * Is hashable  hash(row)
 * Is unpackable
 * Much faster than NamedTuple
 * Allows for fields that would be invalid in Python, a['-dash']
 * Is sliceable,  row[1:3]
 * Iterator like Objects/dictobject.c:3435 dictiter_new
 * conversion to dictionary with dict(row)
 * for key, value in row:
 * queries like count(*) will return ('count(*)', #) so use 'count(*) AS qty'
 */

void named_row_dealloc(pysqlite_NamedRow* self)
{
    Py_XDECREF(self->data);
    Py_XDECREF(self->description);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
named_row_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    pysqlite_NamedRow *self;
    PyObject* data;
    pysqlite_Cursor* cursor;

    assert(type != NULL && type->tp_alloc != NULL);

    if (!_PyArg_NoKeywords("NamedRow", kwargs))
        return NULL;
    if (!PyArg_ParseTuple(args, "OO", &cursor, &data))
        return NULL;

    if (!PyObject_TypeCheck((PyObject*)cursor, &pysqlite_CursorType)) {
        PyErr_SetString(PyExc_TypeError, "instance of cursor required for first argument");
        return NULL;
    }

    if (!PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "tuple required for second argument");
        return NULL;
    }

    self = (pysqlite_NamedRow *) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    Py_INCREF(data);
    self->data = data;

    Py_INCREF(cursor->description);
    self->description = cursor->description;

    return (PyObject *) self;
}

PyObject* named_row_item(pysqlite_NamedRow* self, Py_ssize_t idx)
{
   PyObject* item = PyTuple_GetItem(self->data, idx);
   Py_XINCREF(item);
   return item;
}

Py_ssize_t named_row_find_index(pysqlite_NamedRow* self, const char* name)
{
    // Return the index into cursor.description matching name
    Py_ssize_t nitems = PyTuple_Size(self->description);

    for (Py_ssize_t i = 0; i < nitems; i++) {
        PyObject* obj = PyTuple_GET_ITEM(PyTuple_GET_ITEM(self->description, i), 0);
        const char *cursor_field = PyUnicode_AsUTF8(obj);
        if (!cursor_field) {
            continue;
        }

        const char* p1 = name;
        const char* p2 = cursor_field;

        while (1) {
            // Done checking if at the end of either string
            if ((*p1 == (char)0) || (*p2 == (char)0)) {
                break;
            }

            // case insensitive comparison  and accept '_' instead of space or dash
            if ((*p1 | 0x20) != (*p2 | 0x20) && *p1 != '_' && *p2 !=' ' && *p2 !='-') {
                break;
            }

            p1++;
            p2++;
        }

        // Found it if at the end of both strings
        if ((*p1 == (char)0) && (*p2 == (char)0)) {
            return i;
        }
    }
    return -1;
}

static PyObject* named_row_getattro(pysqlite_NamedRow* self, char* attr_name)
{
    Py_ssize_t idx = named_row_find_index(self, attr_name);
    if (idx < 0) {
        PyTypeObject *tp = Py_TYPE(self);
        /* TODO change 'FIELD' to 'field'*/
        PyErr_Format(PyExc_AttributeError, "'%.50s' object has no FIELD %d '%U'",
                                           tp->tp_name, idx, attr_name);
        return NULL;
    }
    PyObject *value = PyTuple_GET_ITEM(self->data, idx);
    Py_INCREF(value);
    return value;
}

PyObject* named_row_subscript(pysqlite_NamedRow* self, PyObject* idx)
{
    PyObject* item;

    // Process integer index
    if (PyLong_Check(idx)) {
        Py_ssize_t _idx = PyNumber_AsSsize_t(idx, PyExc_IndexError);
        if (_idx == -1 && PyErr_Occurred())
            return NULL;
        if (_idx < 0)
           _idx += PyTuple_GET_SIZE(self->data);
        item = PyTuple_GetItem(self->data, _idx);
        Py_XINCREF(item);
        return item;
    }
    // Process string, dict like
    else if (PyUnicode_Check(idx)) {
        const char* key = PyUnicode_AsUTF8(idx);
        if (key == NULL)
            return NULL;

        Py_ssize_t idx = named_row_find_index(self, key);
        if (idx < 0) {
            PyErr_SetString(PyExc_IndexError, "No item with that key");
            return NULL;
        }
        item = PyTuple_GetItem(self->data, idx);
        Py_INCREF(item);
        return item;
    }
    else if (PySlice_Check(idx)) {
        return PyObject_GetItem(self->data, idx);
    }
    else {
        PyErr_SetString(PyExc_IndexError, "Index must be int or string");
        return NULL;
    }
}

static Py_ssize_t named_row_length(pysqlite_NamedRow* self)
{
    return PyTuple_GET_SIZE(self->data);
}

static PyObject* named_row_iter(pysqlite_NamedRow* self)
{
    // Return a tuple of key/value tuples
    Py_ssize_t item_count = PyTuple_Size(self->data);
    PyObject* result = PyTuple_New(item_count);
    if (!result) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < item_count; i++) {
        PyObject *item = PyTuple_New(2);
        if (item == NULL) {
            Py_DECREF(result);
            return NULL;
        }

        // Get the field title
        PyObject *key = PyTuple_GET_ITEM(PyTuple_GET_ITEM(self->description, i), 0);
        Py_INCREF(key);
        PyTuple_SET_ITEM(item, 0, key);

        // Get the value
        PyObject *value = PyTuple_GET_ITEM(self->data, i);
        Py_INCREF(value);
        PyTuple_SET_ITEM(item, 1, value);

        // Add to result tuple
        PyTuple_SET_ITEM(result, i, item);
    }
    return result;
}

static int named_row_contains(pysqlite_NamedRow* self, PyObject* name)
{
    if (PyUnicode_Check(name)) {
        const char* key = PyUnicode_AsUTF8(name);
        if (key == NULL)
            return -1;

        Py_ssize_t idx = named_row_find_index(self, key);
        return idx >= 0 ? 1 : 0;
    }
    return -1;
}

static Py_hash_t named_row_hash(pysqlite_NamedRow* self)
{
    return PyObject_Hash(self->description) ^ PyObject_Hash(self->data);
}

static PyObject* named_row_richcompare(pysqlite_NamedRow *self, PyObject *_other, int opid)
{
    if (opid != Py_EQ && opid != Py_NE)
        Py_RETURN_NOTIMPLEMENTED;

    if (PyType_IsSubtype(Py_TYPE(_other), &pysqlite_NamedRowType)) {
        pysqlite_NamedRow *other = (pysqlite_NamedRow *)_other;
        PyObject *res = PyObject_RichCompare(self->description, other->description, opid);
        if ((opid == Py_EQ && res == Py_True)
            || (opid == Py_NE && res == Py_False)) {
            Py_DECREF(res);
            return PyObject_RichCompare(self->data, other->data, opid);
        }
    }
    Py_RETURN_NOTIMPLEMENTED;
}

PyMappingMethods named_row_as_mapping = {
    /* mp_length        */ (lenfunc)named_row_length,
    /* mp_subscript     */ (binaryfunc)named_row_subscript,
    /* mp_ass_subscript */ (objobjargproc)0,
};

static PySequenceMethods named_row_as_sequence = {
    /* sq_length      */    (lenfunc)named_row_length,
    /* sq_concat      */    0,
    /* sq_repeat      */    0,
    /* sq_item        */    (ssizeargfunc)named_row_item,
    /* sq_slice       */    (ssizeargfunc)named_row_item,
    /* sq_ass_item    */    0,
    /* sq_ass_slice   */    0,
    /* sq_sq_contains */    (objobjproc)named_row_contains,
};


PyTypeObject pysqlite_NamedRowType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    /* tp_name              */ MODULE_NAME ".NamedRow",
    /* tp_basicsize         */ sizeof(pysqlite_NamedRow),
    /* tp_itemsize          */ 0,
    /* tp_dealloc           */ (destructor)named_row_dealloc,
    /* tp_vectorcall_offset */ 0,
    /* tp_getattr           */ 0,
    /* tp_setattr           */ 0,
    /* tp_as_async          */ 0,
    /* tp_repr              */ 0,
    /* tp_as_number         */ 0,
    /* tp_as_sequence       */ &named_row_as_sequence,
    /* tp_as_mapping        */ 0,
    /* tp_hash              */ (hashfunc)named_row_hash,
    /* tp_call              */ 0,
    /* tp_str               */ 0,
    /* tp_getattro          */ 0, //(getattrofunc)named_row_getattro,
    /* tp_setattro          */ 0,
    /* tp_as_buffer         */ 0,
    /* tp_flags             */ Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    /* tp_doc               */ 0,  /* TODO */
    /* tp_traverse          */ (traverseproc)0,
    /* tp_clear             */ 0,
    /* tp_richcompare       */ (richcmpfunc)named_row_richcompare,
    /* tp_weaklistoffset    */ 0,
    /* tp_iter              */ (getiterfunc)named_row_iter,
    /* tp_iternext          */ 0,
    /* tp_methods           */ 0,
    /* tp_members           */ 0,
    /* tp_getset            */ 0,
    /* tp_base              */ 0,
    /* tp_dict              */ 0,
    /* tp_descr_get         */ 0,
    /* tp_descr_set         */ 0,
    /* tp_dictoffset        */ 0,
    /* tp_init              */ 0,
    /* tp_alloc             */ 0,
    /* tp_new               */ 0,
    /* tp_free              */ 0
};

extern int named_row_setup_types(void)
{
    pysqlite_NamedRowType.tp_new = named_row_new;
    pysqlite_NamedRowType.tp_as_mapping = &named_row_as_mapping;
    pysqlite_NamedRowType.tp_as_sequence = &named_row_as_sequence;
    return PyType_Ready(&pysqlite_NamedRowType);
}

/***************** Iterator *****************/
// Objects/tupleobject.c 1006
/*
typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    //named_row *it_seq;  // Set to NULL when iterator is exhausted
} named_row_iter_object;


static void named_row_iter_dealloc(named_row_iter_object *it)
{
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static PyObject* named_row_iter_traverse(named_row_iter_object *it, visitproc visit, void* arg)
{
    Py_Visit(it->it_seq);
    return 0;
}
*/
