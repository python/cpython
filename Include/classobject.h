#ifndef Py_CLASSOBJECT_H
#define Py_CLASSOBJECT_H
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

/* Class object interface */

#ifdef WITH_THREAD
#include "thread.h"
#else
#define get_thread_ident() 1L
#endif

/* Revealing some structures (not for general use) */

typedef struct {
	PyObject_HEAD
	PyObject	*cl_bases;	/* A tuple of class objects */
	PyObject	*cl_dict;	/* A dictionary */
	PyObject	*cl_name;	/* A string */
	/* The following three are functions or NULL */
	PyObject	*cl_getattr;
	PyObject	*cl_setattr;
	PyObject	*cl_delattr;
} PyClassObject;

typedef struct {
	PyObject_HEAD
	PyClassObject	*in_class;	/* The class object */
	PyObject	*in_dict;	/* A dictionary */
} PyInstanceObject;

extern DL_IMPORT(PyTypeObject) PyClass_Type, PyInstance_Type, PyMethod_Type;

#define PyClass_Check(op) ((op)->ob_type == &PyClass_Type)
#define PyInstance_Check(op) ((op)->ob_type == &PyInstance_Type)
#define PyMethod_Check(op) ((op)->ob_type == &PyMethod_Type)

extern PyObject *PyClass_New Py_PROTO((PyObject *, PyObject *, PyObject *));
extern PyObject *PyInstance_New Py_PROTO((PyObject *, PyObject *, PyObject *));
extern PyObject *PyMethod_New Py_PROTO((PyObject *, PyObject *, PyObject *));

extern PyObject *PyMethod_Function Py_PROTO((PyObject *));
extern PyObject *PyMethod_Self Py_PROTO((PyObject *));
extern PyObject *PyMethod_Class Py_PROTO((PyObject *));

extern int PyClass_IsSubclass Py_PROTO((PyObject *, PyObject *));

extern PyObject *PyInstance_DoBinOp
	Py_PROTO((PyObject *, PyObject *,
		  char *, char *,
		  PyObject * (*) Py_PROTO((PyObject *, PyObject *)) ));

#ifdef __cplusplus
}
#endif
#endif /* !Py_CLASSOBJECT_H */
