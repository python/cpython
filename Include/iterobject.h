#ifndef Py_ITEROBJECT_H
#define Py_ITEROBJECT_H
/* Iterators (the basic kind, over a sequence) */
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(PyTypeObject) PySeqIter_Type;

#define PySeqIter_Check(op) ((op)->ob_type == &PySeqIter_Type)

extern DL_IMPORT(PyObject *) PySeqIter_New(PyObject *);

extern DL_IMPORT(PyTypeObject) PyCallIter_Type;

#define PyCallIter_Check(op) ((op)->ob_type == &PyCallIter_Type)

extern DL_IMPORT(PyObject *) PyCallIter_New(PyObject *, PyObject *);
#ifdef __cplusplus
}
#endif
#endif /* !Py_ITEROBJECT_H */

