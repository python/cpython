#ifndef Py_FILEOBJECT_H
#define Py_FILEOBJECT_H
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

/* File object interface */

extern DL_IMPORT PyTypeObject PyFile_Type;

#define PyFile_Check(op) ((op)->ob_type == &PyFile_Type)

extern PyObject *PyFile_FromString Py_PROTO((char *, char *));
extern void PyFile_SetBufSize Py_PROTO((PyObject *, int));
extern PyObject *PyFile_FromFile
	Py_PROTO((FILE *, char *, char *, int (*)Py_FPROTO((FILE *))));
extern FILE *PyFile_AsFile Py_PROTO((PyObject *));
extern PyObject *PyFile_Name Py_PROTO((PyObject *));
extern PyObject *PyFile_GetLine Py_PROTO((PyObject *, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEOBJECT_H */
