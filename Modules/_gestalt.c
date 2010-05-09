/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Macintosh Gestalt interface */

#include "Python.h"

#include <Carbon/Carbon.h>

/* Convert a 4-char string object argument to an OSType value */
static int
convert_to_OSType(PyObject *v, OSType *pr)
{
    uint32_t tmp;
    if (!PyUnicode_Check(v) || PyUnicode_GetSize(v) != 4) {
    PyErr_SetString(PyExc_TypeError,
                    "OSType arg must be string of 4 chars");
    return 0;
    }
    memcpy((char *)&tmp, _PyUnicode_AsString(v), 4);
    *pr = (OSType)ntohl(tmp);
    return 1;
}

static PyObject *
gestalt_gestalt(PyObject *self, PyObject *args)
{
    OSErr iErr;
    OSType selector;
    SInt32 response;
    if (!PyArg_ParseTuple(args, "O&", convert_to_OSType, &selector))
        return NULL;
    iErr = Gestalt(selector, &response);
    if (iErr != 0) {
    PyErr_SetString(PyExc_OSError,
                    "non-zero exit code!");
    return NULL;
    }
    return PyLong_FromLong(response);
}

static struct PyMethodDef gestalt_methods[] = {
    {"gestalt", gestalt_gestalt, METH_VARARGS},
    {NULL, NULL} /* Sentinel */
};

static struct PyModuleDef gestaltmodule = {
    PyModuleDef_HEAD_INIT,
    "_gestalt",
    NULL,
    -1,
    gestalt_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__gestalt(void)
{
    return PyModule_Create(&gestaltmodule);
}
