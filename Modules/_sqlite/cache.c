/* cache .c - a LRU cache
 *
 * Copyright (C) 2004-2006 Gerhard Häring <gh@ghaering.de>
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

#include "cache.h"
#include <limits.h>

/* only used internally */
pysqlite_Node* pysqlite_new_node(PyObject* key, PyObject* data)
{
    pysqlite_Node* node;

    node = (pysqlite_Node*) (pysqlite_NodeType.tp_alloc(&pysqlite_NodeType, 0));
    if (!node) {
        return NULL;
    }

    Py_INCREF(key);
    node->key = key;

    Py_INCREF(data);
    node->data = data;

    node->prev = NULL;
    node->next = NULL;

    return node;
}

void pysqlite_node_dealloc(pysqlite_Node* self)
{
    Py_DECREF(self->key);
    Py_DECREF(self->data);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

int pysqlite_cache_init(pysqlite_Cache* self, PyObject* args, PyObject* kwargs)
{
    PyObject* factory;
    int size = 10;

    self->factory = NULL;

    if (!PyArg_ParseTuple(args, "O|i", &factory, &size)) {
        return -1;
    }

    /* minimum cache size is 5 entries */
    if (size < 5) {
        size = 5;
    }
    self->size = size;
    self->first = NULL;
    self->last = NULL;

    self->mapping = PyDict_New();
    if (!self->mapping) {
        return -1;
    }

    Py_INCREF(factory);
    self->factory = factory;

    self->decref_factory = 1;

    return 0;
}

void pysqlite_cache_dealloc(pysqlite_Cache* self)
{
    pysqlite_Node* node;
    pysqlite_Node* delete_node;

    if (!self->factory) {
        /* constructor failed, just get out of here */
        return;
    }

    /* iterate over all nodes and deallocate them */
    node = self->first;
    while (node) {
        delete_node = node;
        node = node->next;
        Py_DECREF(delete_node);
    }

    if (self->decref_factory) {
        Py_DECREF(self->factory);
    }
    Py_DECREF(self->mapping);

    Py_TYPE(self)->tp_free((PyObject*)self);
}

PyObject* pysqlite_cache_get(pysqlite_Cache* self, PyObject* args)
{
    PyObject* key = args;
    pysqlite_Node* node;
    pysqlite_Node* ptr;
    PyObject* data;

    node = (pysqlite_Node*)PyDict_GetItem(self->mapping, key);
    if (node) {
        /* an entry for this key already exists in the cache */

        /* increase usage counter of the node found */
        if (node->count < LONG_MAX) {
            node->count++;
        }

        /* if necessary, reorder entries in the cache by swapping positions */
        if (node->prev && node->count > node->prev->count) {
            ptr = node->prev;

            while (ptr->prev && node->count > ptr->prev->count) {
                ptr = ptr->prev;
            }

            if (node->next) {
                node->next->prev = node->prev;
            } else {
                self->last = node->prev;
            }
            if (node->prev) {
                node->prev->next = node->next;
            }
            if (ptr->prev) {
                ptr->prev->next = node;
            } else {
                self->first = node;
            }

            node->next = ptr;
            node->prev = ptr->prev;
            if (!node->prev) {
                self->first = node;
            }
            ptr->prev = node;
        }
    } else {
        /* There is no entry for this key in the cache, yet. We'll insert a new
         * entry in the cache, and make space if necessary by throwing the
         * least used item out of the cache. */

        if (PyDict_Size(self->mapping) == self->size) {
            if (self->last) {
                node = self->last;

                if (PyDict_DelItem(self->mapping, self->last->key) != 0) {
                    return NULL;
                }

                if (node->prev) {
                    node->prev->next = NULL;
                }
                self->last = node->prev;
                node->prev = NULL;

                Py_DECREF(node);
            }
        }

        data = PyObject_CallFunction(self->factory, "O", key);

        if (!data) {
            return NULL;
        }

        node = pysqlite_new_node(key, data);
        if (!node) {
            return NULL;
        }
        node->prev = self->last;

        Py_DECREF(data);

        if (PyDict_SetItem(self->mapping, key, (PyObject*)node) != 0) {
            Py_DECREF(node);
            return NULL;
        }

        if (self->last) {
            self->last->next = node;
        } else {
            self->first = node;
        }
        self->last = node;
    }

    Py_INCREF(node->data);
    return node->data;
}

PyObject* pysqlite_cache_display(pysqlite_Cache* self, PyObject* args)
{
    pysqlite_Node* ptr;
    PyObject* prevkey;
    PyObject* nextkey;
    PyObject* fmt_args;
    PyObject* template;
    PyObject* display_str;

    ptr = self->first;

    while (ptr) {
        if (ptr->prev) {
            prevkey = ptr->prev->key;
        } else {
            prevkey = Py_None;
        }
        Py_INCREF(prevkey);

        if (ptr->next) {
            nextkey = ptr->next->key;
        } else {
            nextkey = Py_None;
        }
        Py_INCREF(nextkey);

        fmt_args = Py_BuildValue("OOO", prevkey, ptr->key, nextkey);
        if (!fmt_args) {
            return NULL;
        }
        template = PyUnicode_FromString("%s <- %s ->%s\n");
        if (!template) {
            Py_DECREF(fmt_args);
            return NULL;
        }
        display_str = PyUnicode_Format(template, fmt_args);
        Py_DECREF(template);
        Py_DECREF(fmt_args);
        if (!display_str) {
            return NULL;
        }
        PyObject_Print(display_str, stdout, Py_PRINT_RAW);
        Py_DECREF(display_str);

        Py_DECREF(prevkey);
        Py_DECREF(nextkey);

        ptr = ptr->next;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef cache_methods[] = {
    {"get", (PyCFunction)pysqlite_cache_get, METH_O,
        PyDoc_STR("Gets an entry from the cache or calls the factory function to produce one.")},
    {"display", (PyCFunction)pysqlite_cache_display, METH_NOARGS,
        PyDoc_STR("For debugging only.")},
    {NULL, NULL}
};

PyTypeObject pysqlite_NodeType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME "Node",                             /* tp_name */
        sizeof(pysqlite_Node),                          /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)pysqlite_node_dealloc,              /* tp_dealloc */
        0,                                              /* tp_print */
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
        (initproc)0,                                    /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

PyTypeObject pysqlite_CacheType = {
        PyVarObject_HEAD_INIT(NULL, 0)
        MODULE_NAME ".Cache",                           /* tp_name */
        sizeof(pysqlite_Cache),                         /* tp_basicsize */
        0,                                              /* tp_itemsize */
        (destructor)pysqlite_cache_dealloc,             /* tp_dealloc */
        0,                                              /* tp_print */
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
        cache_methods,                                  /* tp_methods */
        0,                                              /* tp_members */
        0,                                              /* tp_getset */
        0,                                              /* tp_base */
        0,                                              /* tp_dict */
        0,                                              /* tp_descr_get */
        0,                                              /* tp_descr_set */
        0,                                              /* tp_dictoffset */
        (initproc)pysqlite_cache_init,                  /* tp_init */
        0,                                              /* tp_alloc */
        0,                                              /* tp_new */
        0                                               /* tp_free */
};

extern int pysqlite_cache_setup_types(void)
{
    int rc;

    pysqlite_NodeType.tp_new = PyType_GenericNew;
    pysqlite_CacheType.tp_new = PyType_GenericNew;

    rc = PyType_Ready(&pysqlite_NodeType);
    if (rc < 0) {
        return rc;
    }

    rc = PyType_Ready(&pysqlite_CacheType);
    return rc;
}
