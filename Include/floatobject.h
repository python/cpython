#ifndef Py_FLOATOBJECT_H
#define Py_FLOATOBJECT_H
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

/* Float object interface */

/*
PyFloatObject represents a (double precision) floating point number.
*/

typedef struct {
	PyObject_HEAD
	double ob_fval;
} PyFloatObject;

extern DL_IMPORT(PyTypeObject) PyFloat_Type;

#define PyFloat_Check(op) ((op)->ob_type == &PyFloat_Type)

extern DL_IMPORT(PyObject *) PyFloat_FromString Py_PROTO((PyObject*, char**));
extern DL_IMPORT(PyObject *) PyFloat_FromDouble Py_PROTO((double));
extern DL_IMPORT(double) PyFloat_AsDouble Py_PROTO((PyObject *));

/* Macro, trading safety for speed */
#define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)

#ifdef __cplusplus
}
#endif
#endif /* !Py_FLOATOBJECT_H */
