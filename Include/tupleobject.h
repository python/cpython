#ifndef Py_TUPLEOBJECT_H
#define Py_TUPLEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Tuple object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

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

extern DL_IMPORT PyTypeObject PyTuple_Type;

#define PyTuple_Check(op) ((op)->ob_type == &PyTuple_Type)

extern PyObject *PyTuple_New Py_PROTO((int size));
extern int PyTuple_Size Py_PROTO((PyObject *));
extern PyObject *PyTuple_GetItem Py_PROTO((PyObject *, int));
extern int PyTuple_SetItem Py_PROTO((PyObject *, int, PyObject *));
extern PyObject *PyTuple_GetSlice Py_PROTO((PyObject *, int, int));
extern int resizetuple Py_PROTO((PyObject **, int, int));

/* Macro, trading safety for speed */
#define PyTuple_GET_ITEM(op, i) ((op)->ob_item[i])

#ifdef __cplusplus
}
#endif
#endif /* !Py_TUPLEOBJECT_H */
