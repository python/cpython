/* prepare_protocol.h - the protocol for preparing values for SQLite
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

#ifndef PYSQLITE_PREPARE_PROTOCOL_H
#define PYSQLITE_PREPARE_PROTOCOL_H
#define PY_SSIZE_T_CLEAN
#include "Python.h"

typedef struct
{
    PyObject_HEAD
} pysqlite_PrepareProtocol;

extern PyTypeObject *pysqlite_PrepareProtocolType;

int pysqlite_prepare_protocol_init(pysqlite_PrepareProtocol* self, PyObject* args, PyObject* kwargs);
void pysqlite_prepare_protocol_dealloc(pysqlite_PrepareProtocol* self);

int pysqlite_prepare_protocol_setup_types(PyObject *module);

#define UNKNOWN (-1)
#endif
