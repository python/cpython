/* Iterators (the basic kind, over a sequence) */

extern DL_IMPORT(PyTypeObject) PyIter_Type;

#define PyIter_Check(op) ((op)->ob_type == &PyIter_Type)

extern DL_IMPORT(PyObject *) PyIter_New(PyObject *);

extern DL_IMPORT(PyTypeObject) PyCallIter_Type;

#define PyCallIter_Check(op) ((op)->ob_type == &PyCallIter_Type)

extern DL_IMPORT(PyObject *) PyCallIter_New(PyObject *, PyObject *);
