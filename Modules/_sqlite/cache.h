/* cache.h - definitions for the LRU cache
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

#ifndef PYSQLITE_CACHE_H
#define PYSQLITE_CACHE_H
#include "Python.h"

/* The LRU cache is implemented as a combination of a doubly-linked with a
 * dictionary. The list items are of type 'Node' and the dictionary has the
 * nodes as values. */

typedef struct _Node
{
    PyObject_HEAD
    PyObject* key;
    PyObject* data;
    long count;
    struct _Node* prev;
    struct _Node* next;
} Node;

typedef struct
{
    PyObject_HEAD
    int size;

    /* a dictionary mapping keys to Node entries */
    PyObject* mapping;

    /* the factory callable */
    PyObject* factory;

    Node* first;
    Node* last;

    /* if set, decrement the factory function when the Cache is deallocated.
     * this is almost always desirable, but not in the pysqlite context */
    int decref_factory;
} Cache;

extern PyTypeObject NodeType;
extern PyTypeObject CacheType;

int node_init(Node* self, PyObject* args, PyObject* kwargs);
void node_dealloc(Node* self);

int cache_init(Cache* self, PyObject* args, PyObject* kwargs);
void cache_dealloc(Cache* self);
PyObject* cache_get(Cache* self, PyObject* args);

int cache_setup_types(void);

#endif
