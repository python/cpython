
/* Tuple object interface */

#ifndef Py_TUPLEOBJECT_H
#define Py_TUPLEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/*
Another generally useful object type is an tuple of object pointers.
This is a mutable type: the tuple items can be changed (but not their
number).  Out-of-range indices or non-tuple objects are ignored.

*** WARNING *** PyTuple_SetItem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the tuple.  Similarly, PyTuple_GetItem does not increment the
returned item's reference count.
*/

typedef struct {
    PyObject_VAR_HEAD
    PyObject *ob_item[1];
} PyTupleObject;

extern DL_IMPORT(PyTypeObject) PyTuple_Type;

#define PyTuple_Check(op) ((op)->ob_type == &PyTuple_Type)

extern DL_IMPORT(PyObject *) PyTuple_New(int size);
extern DL_IMPORT(int) PyTuple_Size(PyObject *);
extern DL_IMPORT(PyObject *) PyTuple_GetItem(PyObject *, int);
extern DL_IMPORT(int) PyTuple_SetItem(PyObject *, int, PyObject *);
extern DL_IMPORT(PyObject *) PyTuple_GetSlice(PyObject *, int, int);
extern DL_IMPORT(int) _PyTuple_Resize(PyObject **, int, int);

/* Macro, trading safety for speed */
#define PyTuple_GET_ITEM(op, i) (((PyTupleObject *)(op))->ob_item[i])
#define PyTuple_GET_SIZE(op)    (((PyTupleObject *)(op))->ob_size)

/* Macro, *only* to be used to fill in brand new tuples */
#define PyTuple_SET_ITEM(op, i, v) (((PyTupleObject *)(op))->ob_item[i] = v)

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
