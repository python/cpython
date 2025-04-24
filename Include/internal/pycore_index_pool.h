#ifndef Py_INTERNAL_INDEX_POOL_H
#define Py_INTERNAL_INDEX_POOL_H

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_GIL_DISABLED

#include "pycore_interp_structs.h"

// This contains code for allocating unique indices in an array. It is used by
// the free-threaded build to assign each thread a globally unique index into
// each code object's thread-local bytecode array.


// Allocate the smallest available index. Returns -1 on error.
extern int32_t _PyIndexPool_AllocIndex(_PyIndexPool *indices);

// Release `index` back to the pool
extern void _PyIndexPool_FreeIndex(_PyIndexPool *indices, int32_t index);

extern void _PyIndexPool_Fini(_PyIndexPool *indices);

#endif // Py_GIL_DISABLED

#ifdef __cplusplus
}
#endif
#endif // !Py_INTERNAL_INDEX_POOL_H
