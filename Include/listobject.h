#ifndef Py_LISTOBJECT_H
#define Py_LISTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

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

typedef struct {
	PyObject_VAR_HEAD
	PyObject **ob_item;
} PyListObject;

extern DL_IMPORT(PyTypeObject) PyList_Type;

#define PyList_Check(op) ((op)->ob_type == &PyList_Type)

extern DL_IMPORT(PyObject *) PyList_New Py_PROTO((int size));
extern DL_IMPORT(int) PyList_Size Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyList_GetItem Py_PROTO((PyObject *, int));
extern DL_IMPORT(int) PyList_SetItem Py_PROTO((PyObject *, int, PyObject *));
extern DL_IMPORT(int) PyList_Insert Py_PROTO((PyObject *, int, PyObject *));
extern DL_IMPORT(int) PyList_Append Py_PROTO((PyObject *, PyObject *));
extern DL_IMPORT(PyObject *) PyList_GetSlice Py_PROTO((PyObject *, int, int));
extern DL_IMPORT(int) PyList_SetSlice Py_PROTO((PyObject *, int, int, PyObject *));
extern DL_IMPORT(int) PyList_Sort Py_PROTO((PyObject *));
extern DL_IMPORT(int) PyList_Reverse Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyList_AsTuple Py_PROTO((PyObject *));

/* Macro, trading safety for speed */
#define PyList_GET_ITEM(op, i) (((PyListObject *)(op))->ob_item[i])
#define PyList_SET_ITEM(op, i, v) (((PyListObject *)(op))->ob_item[i] = (v))
#define PyList_GET_SIZE(op)    (((PyListObject *)(op))->ob_size)

#ifdef __cplusplus
}
#endif
#endif /* !Py_LISTOBJECT_H */
