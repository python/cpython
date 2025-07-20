#ifndef Py_INTERNAL_PYMEM_H
#define Py_INTERNAL_PYMEM_H

#include "pycore_llist.h"           // struct llist_node

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Try to get the allocators name set by _PyMem_SetupAllocators().
// Return NULL if unknown.
// Export for '_testinternalcapi' shared extension.
PyAPI_FUNC(const char*) _PyMem_GetCurrentAllocatorName(void);

// strdup() using PyMem_RawMalloc()
extern char* _PyMem_RawStrdup(const char *str);

// strdup() using PyMem_Malloc().
// Export for '_pickle ' shared extension.
PyAPI_FUNC(char*) _PyMem_Strdup(const char *str);

// wcsdup() using PyMem_RawMalloc()
extern wchar_t* _PyMem_RawWcsdup(const wchar_t *str);

/* Special bytes broadcast into debug memory blocks at appropriate times.
   Strings of these are unlikely to be valid addresses, floats, ints or
   7-bit ASCII.

   - PYMEM_CLEANBYTE: clean (newly allocated) memory
   - PYMEM_DEADBYTE dead (newly freed) memory
   - PYMEM_FORBIDDENBYTE: untouchable bytes at each end of a block

   Byte patterns 0xCB, 0xDB and 0xFB have been replaced with 0xCD, 0xDD and
   0xFD to use the same values as Windows CRT debug malloc() and free().
   If modified, _PyMem_IsPtrFreed() should be updated as well. */
#define PYMEM_CLEANBYTE      0xCD
#define PYMEM_DEADBYTE       0xDD
#define PYMEM_FORBIDDENBYTE  0xFD

/* Heuristic checking if a pointer value is newly allocated
   (uninitialized), newly freed or NULL (is equal to zero).

   The pointer is not dereferenced, only the pointer value is checked.

   The heuristic relies on the debug hooks on Python memory allocators which
   fills newly allocated memory with CLEANBYTE (0xCD) and newly freed memory
   with DEADBYTE (0xDD). Detect also "untouchable bytes" marked
   with FORBIDDENBYTE (0xFD). */
static inline int _PyMem_IsPtrFreed(const void *ptr)
{
    uintptr_t value = (uintptr_t)ptr;
#if SIZEOF_VOID_P == 8
    return (value == 0
            || value == (uintptr_t)0xCDCDCDCDCDCDCDCD
            || value == (uintptr_t)0xDDDDDDDDDDDDDDDD
            || value == (uintptr_t)0xFDFDFDFDFDFDFDFD);
#elif SIZEOF_VOID_P == 4
    return (value == 0
            || value == (uintptr_t)0xCDCDCDCD
            || value == (uintptr_t)0xDDDDDDDD
            || value == (uintptr_t)0xFDFDFDFD);
#else
#  error "unknown pointer size"
#endif
}

extern int _PyMem_GetAllocatorName(
    const char *name,
    PyMemAllocatorName *allocator);

/* Configure the Python memory allocators.
   Pass PYMEM_ALLOCATOR_DEFAULT to use default allocators.
   PYMEM_ALLOCATOR_NOT_SET does nothing. */
extern int _PyMem_SetupAllocators(PyMemAllocatorName allocator);

// Default raw memory allocator that is not affected by PyMem_SetAllocator()
extern void *_PyMem_DefaultRawMalloc(size_t);
extern void *_PyMem_DefaultRawCalloc(size_t, size_t);
extern void *_PyMem_DefaultRawRealloc(void *, size_t);
extern void _PyMem_DefaultRawFree(void *);
extern wchar_t *_PyMem_DefaultRawWcsdup(const wchar_t *str);

/* Is the debug allocator enabled? */
extern int _PyMem_DebugEnabled(void);

// Enqueue a pointer to be freed possibly after some delay.
extern void _PyMem_FreeDelayed(void *ptr, size_t size);

// Periodically process delayed free requests.
extern void _PyMem_ProcessDelayed(PyThreadState *tstate);

// Periodically process delayed free requests when the world is stopped.
// Notify of any objects which should be freed.
typedef void (*delayed_dealloc_cb)(PyObject *, void *);
extern void _PyMem_ProcessDelayedNoDealloc(PyThreadState *tstate,
                                           delayed_dealloc_cb cb, void *state);

// Abandon all thread-local delayed free requests and push them to the
// interpreter's queue.
extern void _PyMem_AbandonDelayed(PyThreadState *tstate);

// On interpreter shutdown, frees all delayed free requests.
extern void _PyMem_FiniDelayed(PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYMEM_H
