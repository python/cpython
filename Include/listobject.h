
/* List object interface */

/*
Another generally useful object type is an list of object pointers.
This is a mutable type: the list items can be changed, and items can be
added or removed.  Out-of-range indices or non-list objects are ignored.

*** WARNING *** PyList_SetItem does not increment the new item's reference
count, but does decrement the reference count of the item it replaces,
if not nil.  It does *decrement* the reference count if it is *not*
inserted in the list.  Similarly, PyList_GetItem does not increment the
returned item's reference count.
*/

#ifndef Py_LISTOBJECT_H
#define Py_LISTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject_VAR_HEAD
    PyObject **ob_item;
} PyListObject;

extern DL_IMPORT(PyTypeObject) PyList_Type;

#define PyList_Check(op) ((op)->ob_type == &PyList_Type)

extern DL_IMPORT(PyObject *) PyList_New(int size);
extern DL_IMPORT(int) PyList_Size(PyObject *);
extern DL_IMPORT(PyObject *) PyList_GetItem(PyObject *, int);
extern DL_IMPORT(int) PyList_SetItem(PyObject *, int, PyObject *);
extern DL_IMPORT(int) PyList_Insert(PyObject *, int, PyObject *);
extern DL_IMPORT(int) PyList_Append(PyObject *, PyObject *);
extern DL_IMPORT(PyObject *) PyList_GetSlice(PyObject *, int, int);
extern DL_IMPORT(int) PyList_SetSlice(PyObject *, int, int, PyObject *);
extern DL_IMPORT(int) PyList_Sort(PyObject *);
extern DL_IMPORT(int) PyList_Reverse(PyObject *);
extern DL_IMPORT(PyObject *) PyList_AsTuple(PyObject *);

/* Macro, trading safety for speed */
#define PyList_GET_ITEM(op, i) (((PyListObject *)(op))->ob_item[i])
#define PyList_SET_ITEM(op, i, v) (((PyListObject *)(op))->ob_item[i] = (v))
#define PyList_GET_SIZE(op)    (((PyListObject *)(op))->ob_size)

#ifdef __cplusplus
}
#endif
#endif /* !Py_LISTOBJECT_H */
