#ifndef Py_LIMITED_API
#ifndef Py_TRACEMALLOC_H
#define Py_TRACEMALLOC_H

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

#endif  // !Py_TRACEMALLOC_H
#endif  // !Py_LIMITED_API
