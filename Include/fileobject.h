/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* File object interface */

#ifndef Py_FILEOBJECT_H
#define Py_FILEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(PyTypeObject) PyFile_Type;

#define PyFile_Check(op) ((op)->ob_type == &PyFile_Type)

extern DL_IMPORT(PyObject *) PyFile_FromString(char *, char *);
extern DL_IMPORT(void) PyFile_SetBufSize(PyObject *, int);
extern DL_IMPORT(PyObject *) PyFile_FromFile(FILE *, char *, char *,
                                             int (*)(FILE *));
extern DL_IMPORT(FILE *) PyFile_AsFile(PyObject *);
extern DL_IMPORT(PyObject *) PyFile_Name(PyObject *);
extern DL_IMPORT(PyObject *) PyFile_GetLine(PyObject *, int);
extern DL_IMPORT(int) PyFile_WriteObject(PyObject *, PyObject *, int);
extern DL_IMPORT(int) PyFile_SoftSpace(PyObject *, int);
extern DL_IMPORT(int) PyFile_WriteString(char *, PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEOBJECT_H */
