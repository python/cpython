#ifndef Py_PYTHONRUN_H
#define Py_PYTHONRUN_H
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

/* Interfaces to parse and execute pieces of python code */

void Py_Initialize Py_PROTO((void));

int PyRun_AnyFile Py_PROTO((FILE *, char *));

int PyRun_SimpleString Py_PROTO((char *));
int PyRun_SimpleFile Py_PROTO((FILE *, char *));
int PyRun_InteractiveOne Py_PROTO((FILE *, char *));
int PyRun_InteractiveLoop Py_PROTO((FILE *, char *));

struct _node *PyParser_SimpleParseString Py_PROTO((char *, int));
struct _node *PyParser_SimpleParseFile Py_PROTO((FILE *, char *, int));

PyObject *PyRun_String Py_PROTO((char *, int, PyObject *, PyObject *));
PyObject *PyRun_File Py_PROTO((FILE *, char *, int, PyObject *, PyObject *));

PyObject *Py_CompileString Py_PROTO((char *, char *, int));

void PyErr_Print Py_PROTO((void));

int Py_AtExit Py_PROTO((void (*func) Py_PROTO((void))));

void Py_Exit Py_PROTO((int));

void Py_Cleanup Py_PROTO((void));

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYTHONRUN_H */
