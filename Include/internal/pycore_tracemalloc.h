#ifndef Py_INTERNAL_TRACEMALLOC_H
#define Py_INTERNAL_TRACEMALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


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
};

#define _tracemalloc_runtime_state_INIT \
    { \
        .config = { \
            .initialized = TRACEMALLOC_NOT_INITIALIZED, \
            .tracing = 0, \
            .max_nframe = 1, \
        }, \
    }


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_TRACEMALLOC_H
