#ifndef Py_TRACEBACK_H
#define Py_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Traceback interface */

struct _frame;

DL_IMPORT(int) PyTraceBack_Here Py_PROTO((struct _frame *));
DL_IMPORT(PyObject *) PyTraceBack_Fetch Py_PROTO((void));
DL_IMPORT(int) PyTraceBack_Store Py_PROTO((PyObject *));
DL_IMPORT(int) PyTraceBack_Print Py_PROTO((PyObject *, PyObject *));

/* Reveale traceback type so we can typecheck traceback objects */
extern DL_IMPORT(PyTypeObject) PyTraceBack_Type;
#define PyTraceBack_Check(v) ((v)->ob_type == &PyTraceBack_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_TRACEBACK_H */
