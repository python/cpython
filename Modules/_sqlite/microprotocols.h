/* microprotocols.c - definitions for minimalist and non-validating protocols
 *
 * Copyright (C) 2003-2004 Federico Di Gregorio <fog@debian.org>
 *
 * This file is part of psycopg and was adapted for pysqlite. Federico Di
 * Gregorio gave the permission to use it within pysqlite under the following
 * license:
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

#ifndef PSYCOPG_MICROPROTOCOLS_H
#define PSYCOPG_MICROPROTOCOLS_H 1

#include <Python.h>

/** adapters registry **/

extern PyObject *psyco_adapters;

/** the names of the three mandatory methods **/

#define MICROPROTOCOLS_GETQUOTED_NAME "getquoted"
#define MICROPROTOCOLS_GETSTRING_NAME "getstring"
#define MICROPROTOCOLS_GETBINARY_NAME "getbinary"

/** exported functions **/

/* used by module.c to init the microprotocols system */
extern int pysqlite_microprotocols_init(PyObject *dict);
extern int pysqlite_microprotocols_add(
    PyTypeObject *type, PyObject *proto, PyObject *cast);
extern PyObject *pysqlite_microprotocols_adapt(
    PyObject *obj, PyObject *proto, PyObject *alt);

extern PyObject *
    pysqlite_adapt(pysqlite_Cursor* self, PyObject *args);
#define pysqlite_adapt_doc \
    "adapt(obj, protocol, alternate) -> adapt obj to given protocol. Non-standard."

#endif /* !defined(PSYCOPG_MICROPROTOCOLS_H) */
