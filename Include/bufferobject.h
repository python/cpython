
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
