/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Buffer object interface */

/* Note: the object's structure is private */

#ifndef Py_BUFFEROBJECT_H
#define Py_BUFFEROBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


extern DL_IMPORT(PyTypeObject) PyBuffer_Type;

#define PyBuffer_Check(op) ((op)->ob_type == &PyBuffer_Type)

#define Py_END_OF_BUFFER	(-1)

extern DL_IMPORT(PyObject *) PyBuffer_FromObject(PyObject *base,
                                                 int offset, int size);
extern DL_IMPORT(PyObject *) PyBuffer_FromReadWriteObject(PyObject *base,
                                                          int offset,
                                                          int size);

extern DL_IMPORT(PyObject *) PyBuffer_FromMemory(void *ptr, int size);
extern DL_IMPORT(PyObject *) PyBuffer_FromReadWriteMemory(void *ptr, int size);

extern DL_IMPORT(PyObject *) PyBuffer_New(int size);

#ifdef __cplusplus
}
#endif
#endif /* !Py_BUFFEROBJECT_H */
