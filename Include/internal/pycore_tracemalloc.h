#ifndef Py_INTERNAL_TRACEMALLOC_H
#define Py_INTERNAL_TRACEMALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_hashtable.h"     // _Py_hashtable_t


/* Trace memory blocks allocated by PyMem_RawMalloc() */
#define TRACE_RAW_MALLOC


struct _PyTraceMalloc_Config {
    /* Module initialized?
       Variable protected by the GIL */
    enum {
        TRACEMALLOC_NOT_INITIALIZED,
        TRACEMALLOC_INITIALIZED,
        TRACEMALLOC_FINALIZED
    } initialized;

    /* Is tracemalloc tracing memory allocations?
       Variable protected by the GIL */
    int tracing;

    /* limit of the number of frames in a traceback, 1 by default.
       Variable protected by the GIL. */
    int max_nframe;
};


/* Pack the frame_t structure to reduce the memory footprint on 64-bit
   architectures: 12 bytes instead of 16. */
#if defined(_MSC_VER)
#pragma pack(push, 4)
#endif

struct
#ifdef __GNUC__
__attribute__((packed))
#endif
tracemalloc_frame {
    /* filename cannot be NULL: "<unknown>" is used if the Python frame
       filename is NULL */
    PyObject *filename;
    unsigned int lineno;
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

struct tracemalloc_traceback {
    Py_uhash_t hash;
    /* Number of frames stored */
    uint16_t nframe;
    /* Total number of frames the traceback had */
    uint16_t total_nframe;
    struct tracemalloc_frame frames[1];
};


struct _tracemalloc_runtime_state {
    struct _PyTraceMalloc_Config config;

    /* Protected by the GIL */
    struct {
        PyMemAllocatorEx mem;
        PyMemAllocatorEx raw;
        PyMemAllocatorEx obj;
    } allocators;

#if defined(TRACE_RAW_MALLOC)
    PyThread_type_lock tables_lock;
#endif
    /* Size in bytes of currently traced memory.
       Protected by TABLES_LOCK(). */
    size_t traced_memory;
    /* Peak size in bytes of traced memory.
       Protected by TABLES_LOCK(). */
    size_t peak_traced_memory;
    /* Hash table used as a set to intern filenames:
       PyObject* => PyObject*.
       Protected by the GIL */
    _Py_hashtable_t *filenames;
    /* Buffer to store a new traceback in traceback_new().
       Protected by the GIL. */
    struct tracemalloc_traceback *traceback;
    /* Hash table used as a set to intern tracebacks:
       traceback_t* => traceback_t*
       Protected by the GIL */
    _Py_hashtable_t *tracebacks;
    /* pointer (void*) => trace (trace_t*).
       Protected by TABLES_LOCK(). */
    _Py_hashtable_t *traces;
    /* domain (unsigned int) => traces (_Py_hashtable_t).
       Protected by TABLES_LOCK(). */
    _Py_hashtable_t *domains;

    struct tracemalloc_traceback empty_traceback;

    Py_tss_t reentrant_key;
};

#define _tracemalloc_runtime_state_INIT \
    { \
        .config = { \
            .initialized = TRACEMALLOC_NOT_INITIALIZED, \
            .tracing = 0, \
            .max_nframe = 1, \
        }, \
        .reentrant_key = Py_tss_NEEDS_INIT, \
    }


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_TRACEMALLOC_H
