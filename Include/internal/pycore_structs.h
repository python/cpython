#ifndef Py_INTERNAL_STRUCTS_H
#define Py_INTERNAL_STRUCTS_H
#ifdef __cplusplus
extern "C" {
#endif

/* This files contains various key structs that are widely used
 * and do not depend on other headers. */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct {
    uint16_t value_and_backoff;
} _Py_BackoffCounter;

/* Each instruction in a code object is a fixed-width value,
 * currently 2 bytes: 1-byte opcode + 1-byte oparg.  The EXTENDED_ARG
 * opcode allows for larger values but the current limit is 3 uses
 * of EXTENDED_ARG (see Python/compile.c), for a maximum
 * 32-bit value.  This aligns with the note in Python/compile.c
 * (compiler_addop_i_line) indicating that the max oparg value is
 * 2**32 - 1, rather than INT_MAX.
 */
typedef union {
    uint16_t cache;
    struct {
        uint8_t code;
        uint8_t arg;
    } op;
    _Py_BackoffCounter counter;  // First cache entry of specializable op
} _Py_CODEUNIT;


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
