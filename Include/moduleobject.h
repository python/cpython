#ifndef Py_MODULEOBJECT_H
#define Py_MODULEOBJECT_H
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

/* Module object interface */

extern DL_IMPORT(PyTypeObject) PyModule_Type;

#define PyModule_Check(op) ((op)->ob_type == &PyModule_Type)

extern DL_IMPORT(PyObject *) PyModule_New Py_PROTO((char *));
extern DL_IMPORT(PyObject *) PyModule_GetDict Py_PROTO((PyObject *));
extern DL_IMPORT(char *) PyModule_GetName Py_PROTO((PyObject *));
extern DL_IMPORT(char *) PyModule_GetFilename Py_PROTO((PyObject *));
extern DL_IMPORT(void) _PyModule_Clear Py_PROTO((PyObject *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_MODULEOBJECT_H */
