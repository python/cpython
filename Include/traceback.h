#ifndef Py_TRACEBACK_H
#define Py_TRACEBACK_H
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

/* Traceback interface */

struct _frame;

int PyTraceBack_Here Py_PROTO((struct _frame *));
PyObject *PyTraceBack_Fetch Py_PROTO((void));
int PyTraceBack_Store Py_PROTO((PyObject *));
int PyTraceBack_Print Py_PROTO((PyObject *, PyObject *));

/* Reveale traceback type so we can typecheck traceback objects */
extern PyTypeObject PyTraceback_Type;
#define PyTraceback_Check(v) ((v)->ob_type == &PyTraceback_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_TRACEBACK_H */
