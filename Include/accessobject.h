#ifndef Py_ACCESSOBJECT_H
#define Py_ACCESSOBJECT_H
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

/* Access object interface */

/* Access mode bits (note similarity with UNIX permissions) */
#define AC_R		0444
#define AC_W		0222

#define AC_PRIVATE	0700
#define AC_R_PRIVATE	0400
#define AC_W_PRIVATE	0200

#define AC_PROTECTED	0070
#define AC_R_PROTECTED	0040
#define AC_W_PROTECTED	0020

#define AC_PUBLIC	0007
#define AC_R_PUBLIC	0004
#define AC_W_PUBLIC	0002

extern DL_IMPORT(PyTypeObject) PyAccess_Type;

#define PyAccess_Check(v) ((v)->ob_type == &PyAccess_Type)

PyObject *PyAccess_FromValue Py_PROTO((PyObject *, PyObject *, PyTypeObject *, int));
PyObject *PyAccess_AsValue Py_PROTO((PyObject *, PyObject *));
int PyAccess_SetValue Py_PROTO((PyObject *, PyObject *, PyObject *));

void PyAccess_SetOwner Py_PROTO((PyObject *, PyObject *));
PyObject *PyAccess_Clone Py_PROTO((PyObject *));
int PyAccess_HasValue Py_PROTO((PyObject *));

extern DL_IMPORT(PyTypeObject) PyAnyNumber_Type, PyAnySequence_Type, PyAnyMapping_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_ACCESSOBJECT_H */
