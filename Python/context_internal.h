#ifndef Py_CONTEXT_INTERNAL_H
#define Py_CONTEXT_INTERNAL_H


#define _PyHamtNode_Array_size 32


typedef struct {
    PyObject_HEAD
} _PyHamtNode_BaseNode;


typedef struct {
    PyObject_HEAD
    _PyHamtNode_BaseNode *a_array[_PyHamtNode_Array_size];
    Py_ssize_t a_count;
} PyHamtNode_Array;


typedef struct {
    PyObject_VAR_HEAD
    uint32_t b_bitmap;
    PyObject *b_array[1];
} PyHamtNode_Bitmap;


typedef struct {
    PyObject_VAR_HEAD
    int32_t c_hash;
    PyObject *c_array[1];
} PyHamtNode_Collision;


typedef struct {
    PyObject_HEAD
    _PyHamtNode_BaseNode *h_root;
    PyObject *h_weakreflist;
    Py_ssize_t h_count;
} PyHamtObject;


PyAPI_DATA(PyTypeObject) PyHamt_Type;
PyAPI_DATA(PyTypeObject) _PyHamt_ArrayNode_Type;
PyAPI_DATA(PyTypeObject) _PyHamt_BitmapNode_Type;
PyAPI_DATA(PyTypeObject) _PyHamt_CollisionNode_Type;


#endif /* !Py_CONTEXT_INTERNAL_H */
