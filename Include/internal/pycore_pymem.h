#ifndef Py_INTERNAL_PYMEM_H
#define Py_INTERNAL_PYMEM_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pymem.h"      // PyMemAllocatorName


typedef struct {
    /* We tag each block with an API ID in order to tag API violations */
    char api_id;
    PyMemAllocatorEx alloc;
} debug_alloc_api_t;

struct _pymem_allocators {
    struct {
        PyMemAllocatorEx raw;
        PyMemAllocatorEx mem;
        PyMemAllocatorEx obj;
    } standard;
    struct {
        debug_alloc_api_t raw;
        debug_alloc_api_t mem;
        debug_alloc_api_t obj;
    } debug;
    PyObjectArenaAllocator obj_arena;
};


/* Set the memory allocator of the specified domain to the default.
   Save the old allocator into *old_alloc if it's non-NULL.
   Return on success, or return -1 if the domain is unknown. */
PyAPI_FUNC(int) _PyMem_SetDefaultAllocator(
    PyMemAllocatorDomain domain,
    PyMemAllocatorEx *old_alloc);

/* Special bytes broadcast into debug memory blocks at appropriate times.
   Strings of these are unlikely to be valid addresses, floats, ints or
   7-bit ASCII.

   - PYMEM_CLEANBYTE: clean (newly allocated) memory
   - PYMEM_DEADBYTE dead (newly freed) memory
   - PYMEM_FORBIDDENBYTE: untouchable bytes at each end of a block

   Byte patterns 0xCB, 0xDB and 0xFB have been replaced with 0xCD, 0xDD and
   0xFD to use the same values than Windows CRT debug malloc() and free().
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

PyAPI_FUNC(int) _PyMem_GetAllocatorName(
    const char *name,
    PyMemAllocatorName *allocator);

/* Configure the Python memory allocators.
   Pass PYMEM_ALLOCATOR_DEFAULT to use default allocators.
   PYMEM_ALLOCATOR_NOT_SET does nothing. */
PyAPI_FUNC(int) _PyMem_SetupAllocators(PyMemAllocatorName allocator);


#if WITH_FREELISTS
/* Free lists.
 *
 * Free lists have a pointer to their first entry and
 * the amount of space available allowing fast checks
 * for emptiness and fullness.
 * When empty they are half filled and when full they are
 * completely emptied. This helps the underlying allocator
 * avoid fragmentation and helps performance.
 */

typedef struct _freelist {
    void *ptr;
    uint32_t space;
    uint16_t size;
    uint16_t capacity;
} _PyFreeList;

extern void *_PyFreeList_HalfFillAndAllocate(_PyFreeList *list);
extern void _PyFreeList_FreeToFull(_PyFreeList *list, void *ptr);


static inline void *
_PyFreeList_Alloc(_PyFreeList *list) {
    if (list->ptr != NULL) {
#ifdef Py_STATS
        if (_py_stats) _py_stats->object_stats.from_freelist++;
#endif
        void *result = list->ptr;
        list->ptr = *((void **)result);
        list->space++;
        return result;
    }
    return _PyFreeList_HalfFillAndAllocate(list);
}

static inline void
_PyFreeList_Free(_PyFreeList *list, void *ptr) {
    if (list->space) {
#ifdef Py_STATS
        if (_py_stats) _py_stats->object_stats.to_freelist++;
#endif
        *((void **)ptr) = list->ptr;
        list->ptr = ptr;
        list->space--;
        return;
    }
    _PyFreeList_FreeToFull(list, ptr);
}

static inline void
_PyFreeList_Init(_PyFreeList *list, int size, int capacity)
{
    list->ptr = NULL;
    list->space = list->capacity = capacity;
    list->size = size;
}

extern void _PyFreeList_Clear(_PyFreeList *list);
#endif  /* WITH_FREELISTS */

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYMEM_H
