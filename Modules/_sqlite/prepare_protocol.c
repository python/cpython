/* prepare_protocol.c - the protocol for preparing values for SQLite
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

#include "prepare_protocol.h"

static int
pysqlite_prepare_protocol_init(pysqlite_PrepareProtocol *self, PyObject *args,
                               PyObject *kwargs)
{
    return 0;
}

static int
pysqlite_prepare_protocol_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static void
pysqlite_prepare_protocol_dealloc(pysqlite_PrepareProtocol *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyType_Slot type_slots[] = {
    {Py_tp_dealloc, pysqlite_prepare_protocol_dealloc},
    {Py_tp_init, pysqlite_prepare_protocol_init},
    {Py_tp_traverse, pysqlite_prepare_protocol_traverse},
    {0, NULL},
};

static PyType_Spec type_spec = {
    .name = MODULE_NAME ".PrepareProtocol",
    .basicsize = sizeof(pysqlite_PrepareProtocol),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = type_slots,
};

PyTypeObject *pysqlite_PrepareProtocolType = NULL;

int
pysqlite_prepare_protocol_setup_types(PyObject *module)
{
    pysqlite_PrepareProtocolType = (PyTypeObject *)PyType_FromModuleAndSpec(module, &type_spec, NULL);
    if (pysqlite_PrepareProtocolType == NULL) {
        return -1;
    }
    return 0;
}
