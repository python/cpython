#ifndef Py_TRACEMALLOC_H
#define Py_TRACEMALLOC_H
#ifndef Py_LIMITED_API
#ifdef __cplusplus
extern "C" {
#endif

/* Track an allocated memory block in the tracemalloc module.
   Return 0 on success, return -1 on error (failed to allocate memory to store
   the trace).

   Return -2 if tracemalloc is disabled.

   If memory block is already tracked, update the existing trace. */
PyAPI_FUNC(int) PyTraceMalloc_Track(
    unsigned int domain,
    uintptr_t ptr,
    size_t size);

/* Untrack an allocated memory block in the tracemalloc module.
   Do nothing if the block was not tracked.

   Return -2 if tracemalloc is disabled, otherwise return 0. */
PyAPI_FUNC(int) PyTraceMalloc_Untrack(
    unsigned int domain,
    uintptr_t ptr);

/* Get the traceback where a memory block was allocated.

   Return a tuple of (filename: str, lineno: int) tuples.

   Return None if the tracemalloc module is disabled or if the memory block
   is not tracked by tracemalloc.

   Raise an exception and return NULL on error. */
PyAPI_FUNC(PyObject*) _PyTraceMalloc_GetTraceback(
    unsigned int domain,
    uintptr_t ptr);

/* Return non-zero if tracemalloc is tracing */
PyAPI_FUNC(int) _PyTraceMalloc_IsTracing(void);

/* Clear the tracemalloc traces */
PyAPI_FUNC(void) _PyTraceMalloc_ClearTraces(void);

/* Clear the tracemalloc traces */
PyAPI_FUNC(PyObject *) _PyTraceMalloc_GetTraces(void);

/* Clear tracemalloc traceback for an object */
PyAPI_FUNC(PyObject *) _PyTraceMalloc_GetObjectTraceback(PyObject *obj);

/* Initialize tracemalloc */
PyAPI_FUNC(PyStatus) _PyTraceMalloc_Init(void);

/* Start tracemalloc */
PyAPI_FUNC(int) _PyTraceMalloc_Start(int max_nframe);

/* Stop tracemalloc */
PyAPI_FUNC(void) _PyTraceMalloc_Stop(void);

/* Get the tracemalloc traceback limit */
PyAPI_FUNC(int) _PyTraceMalloc_GetTracebackLimit(void);

/* Get the memory usage of tracemalloc in bytes */
PyAPI_FUNC(size_t) _PyTraceMalloc_GetMemory(void);

/* Get the current size and peak size of traced memory blocks as a 2-tuple */
PyAPI_FUNC(PyObject *) _PyTraceMalloc_GetTracedMemory(void);

/* Set the peak size of traced memory blocks to the current size */
PyAPI_FUNC(void) _PyTraceMalloc_ResetPeak(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LIMITED_API */
#endif /* !Py_TRACEMALLOC_H */
