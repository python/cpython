#ifndef Py_LONGOBJECT_H
#define Py_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Long (arbitrary precision) integer object interface */

typedef struct _longobject PyLongObject; /* Revealed in longintrepr.h */

extern DL_IMPORT(PyTypeObject) PyLong_Type;

#define PyLong_Check(op) ((op)->ob_type == &PyLong_Type)

extern DL_IMPORT(PyObject *) PyLong_FromLong Py_PROTO((long));
extern DL_IMPORT(PyObject *) PyLong_FromUnsignedLong Py_PROTO((unsigned long));
extern DL_IMPORT(PyObject *) PyLong_FromDouble Py_PROTO((double));
extern DL_IMPORT(long) PyLong_AsLong Py_PROTO((PyObject *));
extern DL_IMPORT(unsigned long) PyLong_AsUnsignedLong Py_PROTO((PyObject *));
extern DL_IMPORT(double) PyLong_AsDouble Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyLong_FromVoidPtr Py_PROTO((void *));
extern DL_IMPORT(void *) PyLong_AsVoidPtr Py_PROTO((PyObject *));

#ifdef HAVE_LONG_LONG

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

/* Hopefully this is portable... */
#ifndef LONG_MAX
#define LONG_MAX 2147483647L
#endif
#ifndef ULONG_MAX
#define ULONG_MAX 4294967295U
#endif
#ifndef LONGLONG_MAX
#define LONGLONG_MAX 9223372036854775807LL
#endif
#ifndef ULONGLONG_MAX
#define ULONGLONG_MAX 0xffffffffffffffffULL
#endif
#ifndef LONG_LONG
#define LONG_LONG long long
#endif

extern DL_IMPORT(PyObject *) PyLong_FromLongLong Py_PROTO((LONG_LONG));
extern DL_IMPORT(PyObject *) PyLong_FromUnsignedLongLong Py_PROTO((unsigned LONG_LONG));
extern DL_IMPORT(LONG_LONG) PyLong_AsLongLong Py_PROTO((PyObject *));
extern DL_IMPORT(unsigned LONG_LONG) PyLong_AsUnsignedLongLong Py_PROTO((PyObject *));
#endif /* HAVE_LONG_LONG */

DL_IMPORT(PyObject *) PyLong_FromString Py_PROTO((char *, char **, int));
DL_IMPORT(PyObject *) PyLong_FromUnicode Py_PROTO((Py_UNICODE*, int, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_LONGOBJECT_H */
