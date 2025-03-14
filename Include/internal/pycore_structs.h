#ifndef Py_INTERNAL_STRUCTS_H
#define Py_INTERNAL_STRUCTS_H
#ifdef __cplusplus
extern "C" {
#endif

/* This is the "core of the core".
 * All the mostly widely used structs are defined here.
 * This file must not import any other pycore_ files */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Abstract tree node. */
typedef struct {
    PyObject_HEAD
} PyHamtNode;


/* An HAMT immutable mapping collection. */
typedef struct {
    PyObject_HEAD
    PyHamtNode *h_root;
    PyObject *h_weakreflist;
    Py_ssize_t h_count;
} PyHamtObject;

typedef struct {
    PyObject_VAR_HEAD
    uint32_t b_bitmap;
    PyObject *b_array[1];
} PyHamtNode_Bitmap;

#include "pycore_context.h"

// Define this to get precise tracking of stackrefs.
// #define Py_STACKREF_DEBUG 1

typedef union _PyStackRef {
#if !defined(Py_GIL_DISABLED) && defined(Py_STACKREF_DEBUG)
    uint64_t index;
#else
    uintptr_t bits;
#endif
} _PyStackRef;


#ifdef __cplusplus
}
#endif
#endif /* Py_INTERNAL_STRUCTS_H */
