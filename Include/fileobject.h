#ifndef Py_FILEOBJECT_H
#define Py_FILEOBJECT_H
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

/* File object interface */

extern DL_IMPORT(PyTypeObject) PyFile_Type;

#define PyFile_Check(op) ((op)->ob_type == &PyFile_Type)

extern DL_IMPORT(PyObject *) PyFile_FromString Py_PROTO((char *, char *));
extern DL_IMPORT(void) PyFile_SetBufSize Py_PROTO((PyObject *, int));
extern DL_IMPORT(PyObject *) PyFile_FromFile
	Py_PROTO((FILE *, char *, char *, int (*)Py_FPROTO((FILE *))));
extern DL_IMPORT(FILE *) PyFile_AsFile Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFile_Name Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyFile_GetLine Py_PROTO((PyObject *, int));
extern DL_IMPORT(int) PyFile_WriteObject Py_PROTO((PyObject *, PyObject *, int));
extern DL_IMPORT(int) PyFile_SoftSpace Py_PROTO((PyObject *, int));
extern DL_IMPORT(int) PyFile_WriteString Py_PROTO((char *, PyObject *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEOBJECT_H */
