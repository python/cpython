#ifndef Py_MARSHAL_H
#define Py_MARSHAL_H
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

/* Interface for marshal.c */

DL_IMPORT(void) PyMarshal_WriteLongToFile Py_PROTO((long, FILE *));
DL_IMPORT(void) PyMarshal_WriteShortToFile Py_PROTO((int, FILE *));
DL_IMPORT(void) PyMarshal_WriteObjectToFile Py_PROTO((PyObject *, FILE *));
DL_IMPORT(PyObject *) PyMarshal_WriteObjectToString Py_PROTO((PyObject *));

DL_IMPORT(long) PyMarshal_ReadLongFromFile Py_PROTO((FILE *));
DL_IMPORT(int) PyMarshal_ReadShortFromFile Py_PROTO((FILE *));
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromFile Py_PROTO((FILE *));
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromString Py_PROTO((char *, int));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
