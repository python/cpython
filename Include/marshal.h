/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Interface for marshal.c */

#ifndef Py_MARSHAL_H
#define Py_MARSHAL_H
#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(void) PyMarshal_WriteLongToFile(long, FILE *);
DL_IMPORT(void) PyMarshal_WriteShortToFile(int, FILE *);
DL_IMPORT(void) PyMarshal_WriteObjectToFile(PyObject *, FILE *);
DL_IMPORT(PyObject *) PyMarshal_WriteObjectToString(PyObject *);

DL_IMPORT(long) PyMarshal_ReadLongFromFile(FILE *);
DL_IMPORT(int) PyMarshal_ReadShortFromFile(FILE *);
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromFile(FILE *);
DL_IMPORT(PyObject *) PyMarshal_ReadObjectFromString(char *, int);

#ifdef __cplusplus
}
#endif
#endif /* !Py_MARSHAL_H */
