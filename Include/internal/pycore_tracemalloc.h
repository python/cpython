#ifndef Py_INTERNAL_TRACEMALLOC_H
#define Py_INTERNAL_TRACEMALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_hashtable.h"     // _Py_hashtable_t


struct _PyTraceMalloc_Config {
    /* Module initialized?
       Variable protected by the GIL */
    enum {
        TRACEMALLOC_NOT_INITIALIZED,
        TRACEMALLOC_INITIALIZED,
        TRACEMALLOC_FINALIZED
    } initialized;

    /* Is tracemalloc tracing memory allocations?
       Variable protected by the TABLES_LOCK(). */
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

    PyMutex tables_lock;
    /* Size in bytes of currently traced memory.
       Protected by TABLES_LOCK(). */
    size_t traced_memory;
    /* Peak size in bytes of traced memory.
       Protected by TABLES_LOCK(). */
    size_t peak_traced_memory;
    /* Hash table used as a set to intern filenames:
       PyObject* => PyObject*.
       Protected by the TABLES_LOCK(). */
    _Py_hashtable_t *filenames;
    /* Buffer to store a new traceback in traceback_new().
       Protected by the TABLES_LOCK(). */
    struct tracemalloc_traceback *traceback;
    /* Hash table used as a set to intern tracebacks:
       traceback_t* => traceback_t*
       Protected by the TABLES_LOCK(). */
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


// Get the traceback where a memory block was allocated.
//
// Return a tuple of (filename: str, lineno: int) tuples.
//
// Return None if the tracemalloc module is disabled or if the memory block
// is not tracked by tracemalloc.
//
// Raise an exception and return NULL on error.
//
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(PyObject*) _PyTraceMalloc_GetTraceback(
    unsigned int domain,
    uintptr_t ptr);

/* Return non-zero if tracemalloc is tracing */
extern int _PyTraceMalloc_IsTracing(void);

/* Clear the tracemalloc traces */
extern void _PyTraceMalloc_ClearTraces(void);

/* Clear the tracemalloc traces */
extern PyObject* _PyTraceMalloc_GetTraces(void);

/* Clear tracemalloc traceback for an object */
extern PyObject* _PyTraceMalloc_GetObjectTraceback(PyObject *obj);

/* Initialize tracemalloc */
extern PyStatus _PyTraceMalloc_Init(void);

/* Start tracemalloc */
extern int _PyTraceMalloc_Start(int max_nframe);

/* Stop tracemalloc */
extern void _PyTraceMalloc_Stop(void);

/* Get the tracemalloc traceback limit */
extern int _PyTraceMalloc_GetTracebackLimit(void);

/* Get the memory usage of tracemalloc in bytes */
extern size_t _PyTraceMalloc_GetMemory(void);

/* Get the current size and peak size of traced memory blocks as a 2-tuple */
extern PyObject* _PyTraceMalloc_GetTracedMemory(void);

/* Set the peak size of traced memory blocks to the current size */
extern void _PyTraceMalloc_ResetPeak(void);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_TRACEMALLOC_H
