#ifndef Py_ITEROBJECT_H
#define Py_ITEROBJECT_H
/* Iterators (the basic kind, over a sequence) */
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_DATA(PyTypeObject) PySeqIter_Type;

#define PySeqIter_Check(op) (Py_Type(op) == &PySeqIter_Type)

PyAPI_FUNC(PyObject *) PySeqIter_New(PyObject *);

PyAPI_DATA(PyTypeObject) PyCallIter_Type;

#define PyCallIter_Check(op) (Py_Type(op) == &PyCallIter_Type)

PyAPI_FUNC(PyObject *) PyCallIter_New(PyObject *, PyObject *);

PyObject* _PyZip_CreateIter(PyObject* args);

#ifdef __cplusplus
}
#endif
#endif /* !Py_ITEROBJECT_H */

