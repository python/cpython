#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
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

/* Function object interface */

typedef struct {
	PyObject_HEAD
	PyObject *func_code;
	PyObject *func_globals;
	PyObject *func_name;
	int	func_argcount;
	PyObject *func_argdefs;
	PyObject *func_doc;
} PyFunctionObject;

extern DL_IMPORT PyTypeObject PyFunction_Type;

#define PyFunction_Check(op) ((op)->ob_type == &PyFunction_Type)

extern PyObject *PyFunction_New Py_PROTO((PyObject *, PyObject *));
extern PyObject *PyFunction_GetCode Py_PROTO((PyObject *));
extern PyObject *PyFunction_GetGlobals Py_PROTO((PyObject *));
extern PyObject *getfuncargstuff Py_PROTO((PyObject *, int *));
extern int     setfuncargstuff Py_PROTO((PyObject *, int, PyObject *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
