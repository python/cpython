#ifndef Py_MAPPINGOBJECT_H
#define Py_MAPPINGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
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

/* Dictionary object type -- mapping from hashable object to object */
/* NB Should be moved back to dictobject.h */

extern DL_IMPORT(PyTypeObject) PyDict_Type;

#define PyDict_Check(op) ((op)->ob_type == &PyDict_Type)

extern PyObject *PyDict_New Py_PROTO((void));
extern PyObject *PyDict_GetItem Py_PROTO((PyObject *mp, PyObject *key));
extern int PyDict_SetItem Py_PROTO((PyObject *mp, PyObject *key, PyObject *item));
extern int PyDict_DelItem Py_PROTO((PyObject *mp, PyObject *key));
extern void PyDict_Clear Py_PROTO((PyObject *mp));
extern int PyDict_Next
	Py_PROTO((PyObject *mp, int *pos, PyObject **key, PyObject **value));
extern PyObject *PyDict_Keys Py_PROTO((PyObject *mp));
extern PyObject *PyDict_Values Py_PROTO((PyObject *mp));
extern PyObject *PyDict_Items Py_PROTO((PyObject *mp));
extern int PyDict_Size Py_PROTO((PyObject *mp));

extern PyObject *PyDict_GetItemString Py_PROTO((PyObject *dp, char *key));
extern int PyDict_SetItemString Py_PROTO((PyObject *dp, char *key, PyObject *item));
extern int PyDict_DelItemString Py_PROTO((PyObject *dp, char *key));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MAPPINGOBJECT_H */
