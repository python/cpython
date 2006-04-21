/* row.c - an enhanced tuple for database rows
 *
 * Copyright (C) 2005-2006 Gerhard Häring <gh@ghaering.de>
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
#include "sqlitecompat.h"

void row_dealloc(Row* self)
{
    Py_XDECREF(self->data);
    Py_XDECREF(self->description);

    self->ob_type->tp_free((PyObject*)self);
}

int row_init(Row* self, PyObject* args, PyObject* kwargs)
{
    PyObject* data;
    Cursor* cursor;

    self->data = 0;
    self->description = 0;

    if (!PyArg_ParseTuple(args, "OO", &cursor, &data)) {
        return -1;
    }

    if (!PyObject_IsInstance((PyObject*)cursor, (PyObject*)&CursorType)) {
        PyErr_SetString(PyExc_TypeError, "instance of cursor required for first argument");
        return -1;
    }

    if (!PyTuple_Check(data)) {
        PyErr_SetString(PyExc_TypeError, "tuple required for second argument");
        return -1;
    }

    Py_INCREF(data);
    self->data = data;

    Py_INCREF(cursor->description);
    self->description = cursor->description;

    return 0;
}

PyObject* row_subscript(Row* self, PyObject* idx)
{
    long _idx;
    char* key;
    int nitems, i;
    char* compare_key;

    char* p1;
    char* p2;

    PyObject* item;

    if (PyInt_Check(idx)) {
        _idx = PyInt_AsLong(idx);
        item = PyTuple_GetItem(self->data, _idx);
        Py_XINCREF(item);
        return item;
    } else if (PyLong_Check(idx)) {
        _idx = PyLong_AsLong(idx);
        item = PyTuple_GetItem(self->data, _idx);
        Py_XINCREF(item);
        return item;
    } else if (PyString_Check(idx)) {
        key = PyString_AsString(idx);

        nitems = PyTuple_Size(self->description);

        for (i = 0; i < nitems; i++) {
            compare_key = PyString_AsString(PyTuple_GET_ITEM(PyTuple_GET_ITEM(self->description, i), 0));
            if (!compare_key) {
                return NULL;
            }

            p1 = key;
            p2 = compare_key;

            while (1) {
                if ((*p1 == (char)0) || (*p2 == (char)0)) {
                    break;
                }

                if ((*p1 | 0x20) != (*p2 | 0x20)) {
                    break;
                }

                p1++;
                p2++;
            }

            if ((*p1 == (char)0) && (*p2 == (char)0)) {
                /* found item */
                item = PyTuple_GetItem(self->data, i);
                Py_INCREF(item);
                return item;
            }

        }

        PyErr_SetString(PyExc_IndexError, "No item with that key");
        return NULL;
    } else if (PySlice_Check(idx)) {
        PyErr_SetString(PyExc_ValueError, "slices not implemented, yet");
        return NULL;
    } else {
        PyErr_SetString(PyExc_IndexError, "Index must be int or string");
        return NULL;
    }
}

Py_ssize_t row_length(Row* self, PyObject* args, PyObject* kwargs)
{
    return PyTuple_GET_SIZE(self->data);
}

static int row_print(Row* self, FILE *fp, int flags)
{
    return (&PyTuple_Type)->tp_print(self->data, fp, flags);
}


PyMappingMethods row_as_mapping = {
    /* mp_length        */ (lenfunc)row_length,
    /* mp_subscript     */ (binaryfunc)row_subscript,
    /* mp_ass_subscript */ (objobjargproc)0,
};


PyTypeObject RowType = {
        PyObject_HEAD_INIT(NULL)
        0,                                              /* ob_size */
        MODULE_NAME ".Row",                             /* tp_name */
        sizeof(Row),                                    /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)row_dealloc,                        /* tp_dealloc */
        (printfunc)row_print,                           /* tp_print */
        0,                                              /* tp_getattr */
        0,                                              /* tp_setattr */
        0,                                              /* tp_compare */
        0,                                              /* tp_repr */
        0,                                              /* tp_as_number */
        0,                                              /* tp_as_sequence */
        0,                                              /* tp_as_mapping */
        0,                                              /* tp_hash */
        0,                                              /* tp_call */
        0,                                              /* tp_str */
        0,                                              /* tp_getattro */
        0,                                              /* tp_setattro */
        0,                                              /* tp_as_buffer */
        Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,         /* tp_flags */
        0,                                              /* tp_doc */
        0,                                              /* tp_traverse */
        0,                                              /* tp_clear */
        0,                                              /* tp_richcompare */
        0,                                              /* tp_weaklistoffset */
        0,                                              /* tp_iter */
        0,                                              /* tp_iternext */
        0,                                              /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)row_init,                             /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int row_setup_types(void)
{
    RowType.tp_new = PyType_GenericNew;
    RowType.tp_as_mapping = &row_as_mapping;
    return PyType_Ready(&RowType);
}
