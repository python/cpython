#ifndef Py_INTERNAL_TSTATE_H
#define Py_INTERNAL_TSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_mimalloc.h"      // struct _mimalloc_thread_state


// Every PyThreadState is actually allocated as a _PyThreadStateImpl. The
// PyThreadState fields are exposed as part of the C API, although most fields
// are intended to be private. The _PyThreadStateImpl fields not exposed.
typedef struct _PyThreadStateImpl {
    // semi-public fields are in PyThreadState.
    PyThreadState base;

#ifdef Py_GIL_DISABLED
    struct _mimalloc_thread_state mimalloc;
#endif

} _PyThreadStateImpl;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TSTATE_H */
