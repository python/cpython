/* sqlitecompat.h - compatibility macros
 *
 * Copyright (C) 2006-2010 Gerhard Häring <gh@ghaering.de>
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

#include "Python.h"

#ifndef PYSQLITE_COMPAT_H
#define PYSQLITE_COMPAT_H

/* define Py_ssize_t for pre-2.5 versions of Python */

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
typedef int (*lenfunc)(PyObject*);
#endif


/* define PyDict_CheckExact for pre-2.4 versions of Python */
#ifndef PyDict_CheckExact
#define PyDict_CheckExact(op) ((op)->ob_type == &PyDict_Type)
#endif

/* define Py_CLEAR for pre-2.4 versions of Python */
#ifndef Py_CLEAR
#define Py_CLEAR(op)				\
        do {                            	\
                if (op) {			\
                        PyObject *tmp = (PyObject *)(op);	\
                        (op) = NULL;		\
                        Py_DECREF(tmp);		\
                }				\
        } while (0)
#endif

#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
    PyObject_HEAD_INIT(type) size,
#endif

#ifndef Py_TYPE
#define Py_TYPE(ob) ((ob)->ob_type)
#endif

#endif
