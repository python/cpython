#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
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
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Long (arbitrary precision) integer object interface */

typedef struct _longobject PyLongObject; /* Revealed in longintrepr.h */

extern DL_IMPORT(PyTypeObject) PyLong_Type;

#define PyLong_Check(op) ((op)->ob_type == &PyLong_Type)

extern PyObject *PyLong_FromLong Py_PROTO((long));
extern PyObject *PyLong_FromUnsignedLong Py_PROTO((unsigned long));
extern PyObject *PyLong_FromDouble Py_PROTO((double));
extern long PyLong_AsLong Py_PROTO((PyObject *));
extern unsigned long PyLong_AsUnsignedLong Py_PROTO((PyObject *));
extern double PyLong_AsDouble Py_PROTO((PyObject *));

#ifdef HAVE_LONG_LONG
extern PyObject *PyLong_FromLongLong Py_PROTO((long long ));
extern PyObject *PyLong_FromUnsignedLongLong Py_PROTO((unsigned long long));
extern long long PyLong_AsLongLong Py_PROTO((PyObject *));
extern unsigned long long PyLong_AsUnsignedLongLong Py_PROTO((PyObject *));
#endif

PyObject *PyLong_FromString Py_PROTO((char *, char **, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGOBJECT_H */
