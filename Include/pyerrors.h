#ifndef Py_ERRORS_H
#define Py_ERRORS_H
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

/* Error handling definitions */

void PyErr_SetNone Py_PROTO((PyObject *));
void PyErr_SetObject Py_PROTO((PyObject *, PyObject *));
void PyErr_SetString Py_PROTO((PyObject *, char *));
PyObject *PyErr_Occurred Py_PROTO((void));
void PyErr_Clear Py_PROTO((void));
void PyErr_Fetch Py_PROTO((PyObject **, PyObject **, PyObject **));
void PyErr_Restore Py_PROTO((PyObject *, PyObject *, PyObject *));

/* Predefined exceptions */

extern DL_IMPORT PyObject *PyExc_AccessError;
extern DL_IMPORT PyObject *PyExc_AttributeError;
extern DL_IMPORT PyObject *PyExc_ConflictError;
extern DL_IMPORT PyObject *PyExc_EOFError;
extern DL_IMPORT PyObject *PyExc_IOError;
extern DL_IMPORT PyObject *PyExc_ImportError;
extern DL_IMPORT PyObject *PyExc_IndexError;
extern DL_IMPORT PyObject *PyExc_KeyError;
extern DL_IMPORT PyObject *PyExc_KeyboardInterrupt;
extern DL_IMPORT PyObject *PyExc_MemoryError;
extern DL_IMPORT PyObject *PyExc_NameError;
extern DL_IMPORT PyObject *PyExc_OverflowError;
extern DL_IMPORT PyObject *PyExc_RuntimeError;
extern DL_IMPORT PyObject *PyExc_SyntaxError;
extern DL_IMPORT PyObject *PyExc_SystemError;
extern DL_IMPORT PyObject *PyExc_SystemExit;
extern DL_IMPORT PyObject *PyExc_TypeError;
extern DL_IMPORT PyObject *PyExc_ValueError;
extern DL_IMPORT PyObject *PyExc_ZeroDivisionError;

/* Convenience functions */

extern int PyErr_BadArgument Py_PROTO((void));
extern PyObject *PyErr_NoMemory Py_PROTO((void));
extern PyObject *PyErr_SetFromErrno Py_PROTO((PyObject *));

extern void PyErr_BadInternalCall Py_PROTO((void));

extern int PyErr_CheckSignals Py_PROTO((void)); /* In sigcheck.c or signalmodule.c */

#ifdef __cplusplus
}
#endif
#endif /* !Py_ERRORS_H */
