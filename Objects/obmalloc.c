/* Python's malloc wrappers (see pymem.h) */

#include "Python.h"
#include "pycore_code.h"          // stats
#include "pycore_object.h"        // _PyDebugAllocatorStats() definition
#include "pycore_obmalloc.h"
#include "pycore_pyerrors.h"      // _Py_FatalErrorFormat()
#include "pycore_pymem.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET
#include "pycore_obmalloc_init.h"

#include <stdlib.h>               // malloc()
#include <stdbool.h>
#ifdef WITH_MIMALLOC
// Forward declarations of functions used in our mimalloc modifications
static void _PyMem_mi_page_clear_qsbr(mi_page_t *page);
static bool _PyMem_mi_page_is_safe_to_free(mi_page_t *page);
static bool _PyMem_mi_page_maybe_free(mi_page_t *page, mi_page_queue_t *pq, bool force);
static void _PyMem_mi_page_reclaimed(mi_page_t *page);
static void _PyMem_mi_heap_collect_qsbr(mi_heap_t *heap);
#  include "pycore_mimalloc.h"
#  include "mimalloc/static.c"
#  include "mimalloc/internal.h"  // for stats
#endif

#if defined(Py_GIL_DISABLED) && !defined(WITH_MIMALLOC)
#  error "Py_GIL_DISABLED requires WITH_MIMALLOC"
#endif

#undef  uint
#define uint pymem_uint


/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);

static void _PyObject_DebugDumpAddress(const void *p);
static void _PyMem_DebugCheckAddress(const char *func, char api_id, const void *p);


static void set_up_debug_hooks_domain_unlocked(PyMemAllocatorDomain domain);
static void set_up_debug_hooks_unlocked(void);
static void get_allocator_unlocked(PyMemAllocatorDomain, PyMemAllocatorEx *);
static void set_allocator_unlocked(PyMemAllocatorDomain, PyMemAllocatorEx *);


/***************************************/
/* low-level allocator implementations */
/***************************************/

/* the default raw allocator (wraps malloc) */

void *
_PyMem_RawMalloc(void *Py_UNUSED(ctx), size_t size)
{
    /* PyMem_RawMalloc(0) means malloc(1). Some systems would return NULL
       for malloc(0), which would be treated as an error. Some platforms would
       return a pointer with no memory behind it, which would break pymalloc.
       To solve these problems, allocate an extra byte. */
    if (size == 0)
        size = 1;
    return malloc(size);
}

void *
_PyMem_RawCalloc(void *Py_UNUSED(ctx), size_t nelem, size_t elsize)
{
    /* PyMem_RawCalloc(0, 0) means calloc(1, 1). Some systems would return NULL
       for calloc(0, 0), which would be treated as an error. Some platforms
       would return a pointer with no memory behind it, which would break
       pymalloc.  To solve these problems, allocate an extra byte. */
    if (nelem == 0 || elsize == 0) {
        nelem = 1;
        elsize = 1;
    }
    return calloc(nelem, elsize);
}

void *
_PyMem_RawRealloc(void *Py_UNUSED(ctx), void *ptr, size_t size)
{
    if (size == 0)
        size = 1;
    return realloc(ptr, size);
}

void
_PyMem_RawFree(void *Py_UNUSED(ctx), void *ptr)
{
    free(ptr);
}

#ifdef WITH_MIMALLOC

static void
_PyMem_mi_page_clear_qsbr(mi_page_t *page)
{
#ifdef Py_GIL_DISABLED
    // Clear the QSBR goal and remove the page from the QSBR linked list.
    page->qsbr_goal = 0;
    if (page->qsbr_node.next != NULL) {
        llist_remove(&page->qsbr_node);
    }
#endif
}

// Check if an empty, newly reclaimed page is safe to free now.
static bool
_PyMem_mi_page_is_safe_to_free(mi_page_t *page)
{
    assert(mi_page_all_free(page));
#ifdef Py_GIL_DISABLED
    assert(page->qsbr_node.next == NULL);
    if (page->use_qsbr && page->qsbr_goal != 0) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
        if (tstate == NULL) {
            return false;
        }
        return _Py_qbsr_goal_reached(tstate->qsbr, page->qsbr_goal);
    }
#endif
    return true;

}

static bool
_PyMem_mi_page_maybe_free(mi_page_t *page, mi_page_queue_t *pq, bool force)
{
#ifdef Py_GIL_DISABLED
    assert(mi_page_all_free(page));
    if (page->use_qsbr) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)PyThreadState_GET();
        if (page->qsbr_goal != 0 && _Py_qbsr_goal_reached(tstate->qsbr, page->qsbr_goal)) {
            _PyMem_mi_page_clear_qsbr(page);
            _mi_page_free(page, pq, force);
            return true;
        }

        _PyMem_mi_page_clear_qsbr(page);
        page->retire_expire = 0;
        page->qsbr_goal = _Py_qsbr_deferred_advance(tstate->qsbr);
        llist_insert_tail(&tstate->mimalloc.page_list, &page->qsbr_node);
        return false;
    }
#endif
    _mi_page_free(page, pq, force);
    return true;
}

static void
_PyMem_mi_page_reclaimed(mi_page_t *page)
{
#ifdef Py_GIL_DISABLED
    assert(page->qsbr_node.next == NULL);
    if (page->qsbr_goal != 0) {
        if (mi_page_all_free(page)) {
            assert(page->qsbr_node.next == NULL);
            _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)PyThreadState_GET();
            page->retire_expire = 0;
            llist_insert_tail(&tstate->mimalloc.page_list, &page->qsbr_node);
        }
        else {
            page->qsbr_goal = 0;
        }
    }
#endif
}

static void
_PyMem_mi_heap_collect_qsbr(mi_heap_t *heap)
{
#ifdef Py_GIL_DISABLED
    if (!heap->page_use_qsbr) {
        return;
    }

    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct llist_node *head = &tstate->mimalloc.page_list;
    if (llist_empty(head)) {
        return;
    }

    struct llist_node *node;
    llist_for_each_safe(node, head) {
        mi_page_t *page = llist_data(node, mi_page_t, qsbr_node);
        if (!mi_page_all_free(page)) {
            // We allocated from this page some point after the delayed free
            _PyMem_mi_page_clear_qsbr(page);
            continue;
        }

        if (!_Py_qsbr_poll(tstate->qsbr, page->qsbr_goal)) {
            return;
        }

        _PyMem_mi_page_clear_qsbr(page);
        _mi_page_free(page, mi_page_queue_of(page), false);
    }
#endif
}

void *
_PyMem_MiMalloc(void *ctx, size_t size)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = &tstate->mimalloc.heaps[_Py_MIMALLOC_HEAP_MEM];
    return mi_heap_malloc(heap, size);
#else
    return mi_malloc(size);
#endif
}

void *
_PyMem_MiCalloc(void *ctx, size_t nelem, size_t elsize)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = &tstate->mimalloc.heaps[_Py_MIMALLOC_HEAP_MEM];
    return mi_heap_calloc(heap, nelem, elsize);
#else
    return mi_calloc(nelem, elsize);
#endif
}

void *
_PyMem_MiRealloc(void *ctx, void *ptr, size_t size)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = &tstate->mimalloc.heaps[_Py_MIMALLOC_HEAP_MEM];
    return mi_heap_realloc(heap, ptr, size);
#else
    return mi_realloc(ptr, size);
#endif
}

void
_PyMem_MiFree(void *ctx, void *ptr)
{
    mi_free(ptr);
}

void *
_PyObject_MiMalloc(void *ctx, size_t nbytes)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = tstate->mimalloc.current_object_heap;
    return mi_heap_malloc(heap, nbytes);
#else
    return mi_malloc(nbytes);
#endif
}

void *
_PyObject_MiCalloc(void *ctx, size_t nelem, size_t elsize)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = tstate->mimalloc.current_object_heap;
    return mi_heap_calloc(heap, nelem, elsize);
#else
    return mi_calloc(nelem, elsize);
#endif
}


void *
_PyObject_MiRealloc(void *ctx, void *ptr, size_t nbytes)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    mi_heap_t *heap = tstate->mimalloc.current_object_heap;
    return mi_heap_realloc(heap, ptr, nbytes);
#else
    return mi_realloc(ptr, nbytes);
#endif
}

void
_PyObject_MiFree(void *ctx, void *ptr)
{
    mi_free(ptr);
}

#endif // WITH_MIMALLOC


#define MALLOC_ALLOC {NULL, _PyMem_RawMalloc, _PyMem_RawCalloc, _PyMem_RawRealloc, _PyMem_RawFree}


#ifdef WITH_MIMALLOC
#  define MIMALLOC_ALLOC {NULL, _PyMem_MiMalloc, _PyMem_MiCalloc, _PyMem_MiRealloc, _PyMem_MiFree}
#  define MIMALLOC_OBJALLOC {NULL, _PyObject_MiMalloc, _PyObject_MiCalloc, _PyObject_MiRealloc, _PyObject_MiFree}
#endif

/* the pymalloc allocator */

// The actual implementation is further down.

#if defined(WITH_PYMALLOC)
void* _PyObject_Malloc(void *ctx, size_t size);
void* _PyObject_Calloc(void *ctx, size_t nelem, size_t elsize);
void _PyObject_Free(void *ctx, void *p);
void* _PyObject_Realloc(void *ctx, void *ptr, size_t size);
#  define PYMALLOC_ALLOC {NULL, _PyObject_Malloc, _PyObject_Calloc, _PyObject_Realloc, _PyObject_Free}
#endif  // WITH_PYMALLOC

#if defined(Py_GIL_DISABLED)
// Py_GIL_DISABLED requires using mimalloc for "mem" and "obj" domains.
#  define PYRAW_ALLOC MALLOC_ALLOC
#  define PYMEM_ALLOC MIMALLOC_ALLOC
#  define PYOBJ_ALLOC MIMALLOC_OBJALLOC
#elif defined(WITH_PYMALLOC)
#  define PYRAW_ALLOC MALLOC_ALLOC
#  define PYMEM_ALLOC PYMALLOC_ALLOC
#  define PYOBJ_ALLOC PYMALLOC_ALLOC
#else
#  define PYRAW_ALLOC MALLOC_ALLOC
#  define PYMEM_ALLOC MALLOC_ALLOC
#  define PYOBJ_ALLOC MALLOC_ALLOC
#endif


/* the default debug allocators */

// The actual implementation is further down.

void* _PyMem_DebugRawMalloc(void *ctx, size_t size);
void* _PyMem_DebugRawCalloc(void *ctx, size_t nelem, size_t elsize);
void* _PyMem_DebugRawRealloc(void *ctx, void *ptr, size_t size);
void _PyMem_DebugRawFree(void *ctx, void *ptr);

void* _PyMem_DebugMalloc(void *ctx, size_t size);
void* _PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize);
void* _PyMem_DebugRealloc(void *ctx, void *ptr, size_t size);
void _PyMem_DebugFree(void *ctx, void *p);

#define PYDBGRAW_ALLOC \
    {&_PyRuntime.allocators.debug.raw, _PyMem_DebugRawMalloc, _PyMem_DebugRawCalloc, _PyMem_DebugRawRealloc, _PyMem_DebugRawFree}
#define PYDBGMEM_ALLOC \
    {&_PyRuntime.allocators.debug.mem, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}
#define PYDBGOBJ_ALLOC \
    {&_PyRuntime.allocators.debug.obj, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}

/* default raw allocator (not swappable) */

void *
_PyMem_DefaultRawMalloc(size_t size)
{
#ifdef Py_DEBUG
    return _PyMem_DebugRawMalloc(&_PyRuntime.allocators.debug.raw, size);
#else
    return _PyMem_RawMalloc(NULL, size);
#endif
}

void *
_PyMem_DefaultRawCalloc(size_t nelem, size_t elsize)
{
#ifdef Py_DEBUG
    return _PyMem_DebugRawCalloc(&_PyRuntime.allocators.debug.raw, nelem, elsize);
#else
    return _PyMem_RawCalloc(NULL, nelem, elsize);
#endif
}

void *
_PyMem_DefaultRawRealloc(void *ptr, size_t size)
{
#ifdef Py_DEBUG
    return _PyMem_DebugRawRealloc(&_PyRuntime.allocators.debug.raw, ptr, size);
#else
    return _PyMem_RawRealloc(NULL, ptr, size);
#endif
}

void
_PyMem_DefaultRawFree(void *ptr)
{
#ifdef Py_DEBUG
    _PyMem_DebugRawFree(&_PyRuntime.allocators.debug.raw, ptr);
#else
    _PyMem_RawFree(NULL, ptr);
#endif
}

wchar_t*
_PyMem_DefaultRawWcsdup(const wchar_t *str)
{
    assert(str != NULL);

    size_t len = wcslen(str);
    if (len > (size_t)PY_SSIZE_T_MAX / sizeof(wchar_t) - 1) {
        return NULL;
    }

    size_t size = (len + 1) * sizeof(wchar_t);
    wchar_t *str2 = _PyMem_DefaultRawMalloc(size);
    if (str2 == NULL) {
        return NULL;
    }

    memcpy(str2, str, size);
    return str2;
}

/* the low-level virtual memory allocator */

#ifdef WITH_PYMALLOC
#  ifdef MS_WINDOWS
#    include <windows.h>
#  elif defined(HAVE_MMAP)
#    include <sys/mman.h>
#    ifdef MAP_ANONYMOUS
#      define ARENAS_USE_MMAP
#    endif
#  endif
#endif

void *
_PyMem_ArenaAlloc(void *Py_UNUSED(ctx), size_t size)
{
#ifdef MS_WINDOWS
    return VirtualAlloc(NULL, size,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(ARENAS_USE_MMAP)
    void *ptr;
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    assert(ptr != NULL);
    return ptr;
#else
    return malloc(size);
#endif
}

void
_PyMem_ArenaFree(void *Py_UNUSED(ctx), void *ptr,
#if defined(ARENAS_USE_MMAP)
    size_t size
#else
    size_t Py_UNUSED(size)
#endif
)
{
#ifdef MS_WINDOWS
    /* Unlike free(), VirtualFree() does not special-case NULL to noop. */
    if (ptr == NULL) {
        return;
    }
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(ARENAS_USE_MMAP)
    /* Unlike free(), munmap() does not special-case NULL to noop. */
    if (ptr == NULL) {
        return;
    }
    munmap(ptr, size);
#else
    free(ptr);
#endif
}

/*******************************************/
/* end low-level allocator implementations */
/*******************************************/


#if defined(__has_feature)  /* Clang */
#  if __has_feature(address_sanitizer) /* is ASAN enabled? */
#    define _Py_NO_SANITIZE_ADDRESS \
        __attribute__((no_sanitize("address")))
#  endif
#  if __has_feature(thread_sanitizer)  /* is TSAN enabled? */
#    define _Py_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#  endif
#  if __has_feature(memory_sanitizer)  /* is MSAN enabled? */
#    define _Py_NO_SANITIZE_MEMORY __attribute__((no_sanitize_memory))
#  endif
#elif defined(__GNUC__)
#  if defined(__SANITIZE_ADDRESS__)    /* GCC 4.8+, is ASAN enabled? */
#    define _Py_NO_SANITIZE_ADDRESS \
        __attribute__((no_sanitize_address))
#  endif
   // TSAN is supported since GCC 5.1, but __SANITIZE_THREAD__ macro
   // is provided only since GCC 7.
#  if __GNUC__ > 5 || (__GNUC__ == 5 && __GNUC_MINOR__ >= 1)
#    define _Py_NO_SANITIZE_THREAD __attribute__((no_sanitize_thread))
#  endif
#endif

#ifndef _Py_NO_SANITIZE_ADDRESS
#  define _Py_NO_SANITIZE_ADDRESS
#endif
#ifndef _Py_NO_SANITIZE_THREAD
#  define _Py_NO_SANITIZE_THREAD
#endif
#ifndef _Py_NO_SANITIZE_MEMORY
#  define _Py_NO_SANITIZE_MEMORY
#endif


#define ALLOCATORS_MUTEX (_PyRuntime.allocators.mutex)
#define _PyMem_Raw (_PyRuntime.allocators.standard.raw)
#define _PyMem (_PyRuntime.allocators.standard.mem)
#define _PyObject (_PyRuntime.allocators.standard.obj)
#define _PyMem_Debug (_PyRuntime.allocators.debug)
#define _PyObject_Arena (_PyRuntime.allocators.obj_arena)


/***************************/
/* managing the allocators */
/***************************/

static int
set_default_allocator_unlocked(PyMemAllocatorDomain domain, int debug,
                               PyMemAllocatorEx *old_alloc)
{
    if (old_alloc != NULL) {
        get_allocator_unlocked(domain, old_alloc);
    }


    PyMemAllocatorEx new_alloc;
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW:
        new_alloc = (PyMemAllocatorEx)PYRAW_ALLOC;
        break;
    case PYMEM_DOMAIN_MEM:
        new_alloc = (PyMemAllocatorEx)PYMEM_ALLOC;
        break;
    case PYMEM_DOMAIN_OBJ:
        new_alloc = (PyMemAllocatorEx)PYOBJ_ALLOC;
        break;
    default:
        /* unknown domain */
        return -1;
    }
    set_allocator_unlocked(domain, &new_alloc);
    if (debug) {
        set_up_debug_hooks_domain_unlocked(domain);
    }
    return 0;
}


#ifdef Py_DEBUG
static const int pydebug = 1;
#else
static const int pydebug = 0;
#endif

int
_PyMem_GetAllocatorName(const char *name, PyMemAllocatorName *allocator)
{
    if (name == NULL || *name == '\0') {
        /* PYTHONMALLOC is empty or is not set or ignored (-E/-I command line
           nameions): use default memory allocators */
        *allocator = PYMEM_ALLOCATOR_DEFAULT;
    }
    else if (strcmp(name, "default") == 0) {
        *allocator = PYMEM_ALLOCATOR_DEFAULT;
    }
    else if (strcmp(name, "debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_DEBUG;
    }
#if defined(WITH_PYMALLOC) && !defined(Py_GIL_DISABLED)
    else if (strcmp(name, "pymalloc") == 0) {
        *allocator = PYMEM_ALLOCATOR_PYMALLOC;
    }
    else if (strcmp(name, "pymalloc_debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_PYMALLOC_DEBUG;
    }
#endif
#ifdef WITH_MIMALLOC
    else if (strcmp(name, "mimalloc") == 0) {
        *allocator = PYMEM_ALLOCATOR_MIMALLOC;
    }
    else if (strcmp(name, "mimalloc_debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_MIMALLOC_DEBUG;
    }
#endif
#ifndef Py_GIL_DISABLED
    else if (strcmp(name, "malloc") == 0) {
        *allocator = PYMEM_ALLOCATOR_MALLOC;
    }
    else if (strcmp(name, "malloc_debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_MALLOC_DEBUG;
    }
#endif
    else {
        /* unknown allocator */
        return -1;
    }
    return 0;
}


static int
set_up_allocators_unlocked(PyMemAllocatorName allocator)
{
    switch (allocator) {
    case PYMEM_ALLOCATOR_NOT_SET:
        /* do nothing */
        break;

    case PYMEM_ALLOCATOR_DEFAULT:
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_RAW, pydebug, NULL);
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_MEM, pydebug, NULL);
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_OBJ, pydebug, NULL);
        _PyRuntime.allocators.is_debug_enabled = pydebug;
        break;

    case PYMEM_ALLOCATOR_DEBUG:
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_RAW, 1, NULL);
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_MEM, 1, NULL);
        (void)set_default_allocator_unlocked(PYMEM_DOMAIN_OBJ, 1, NULL);
        _PyRuntime.allocators.is_debug_enabled = 1;
        break;

#ifdef WITH_PYMALLOC
    case PYMEM_ALLOCATOR_PYMALLOC:
    case PYMEM_ALLOCATOR_PYMALLOC_DEBUG:
    {
        PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_RAW, &malloc_alloc);

        PyMemAllocatorEx pymalloc = PYMALLOC_ALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_MEM, &pymalloc);
        set_allocator_unlocked(PYMEM_DOMAIN_OBJ, &pymalloc);

        int is_debug = (allocator == PYMEM_ALLOCATOR_PYMALLOC_DEBUG);
        _PyRuntime.allocators.is_debug_enabled = is_debug;
        if (is_debug) {
            set_up_debug_hooks_unlocked();
        }
        break;
    }
#endif
#ifdef WITH_MIMALLOC
    case PYMEM_ALLOCATOR_MIMALLOC:
    case PYMEM_ALLOCATOR_MIMALLOC_DEBUG:
    {
        PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_RAW, &malloc_alloc);

        PyMemAllocatorEx pymalloc = MIMALLOC_ALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_MEM, &pymalloc);

        PyMemAllocatorEx objmalloc = MIMALLOC_OBJALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_OBJ, &objmalloc);

        int is_debug = (allocator == PYMEM_ALLOCATOR_MIMALLOC_DEBUG);
        _PyRuntime.allocators.is_debug_enabled = is_debug;
        if (is_debug) {
            set_up_debug_hooks_unlocked();
        }

        break;
    }
#endif

    case PYMEM_ALLOCATOR_MALLOC:
    case PYMEM_ALLOCATOR_MALLOC_DEBUG:
    {
        PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
        set_allocator_unlocked(PYMEM_DOMAIN_RAW, &malloc_alloc);
        set_allocator_unlocked(PYMEM_DOMAIN_MEM, &malloc_alloc);
        set_allocator_unlocked(PYMEM_DOMAIN_OBJ, &malloc_alloc);

        int is_debug = (allocator == PYMEM_ALLOCATOR_MALLOC_DEBUG);
        _PyRuntime.allocators.is_debug_enabled = is_debug;
        if (is_debug) {
            set_up_debug_hooks_unlocked();
        }
        break;
    }

    default:
        /* unknown allocator */
        return -1;
    }

    return 0;
}

int
_PyMem_SetupAllocators(PyMemAllocatorName allocator)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    int res = set_up_allocators_unlocked(allocator);
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
    return res;
}


static int
pymemallocator_eq(PyMemAllocatorEx *a, PyMemAllocatorEx *b)
{
    return (memcmp(a, b, sizeof(PyMemAllocatorEx)) == 0);
}


static const char*
get_current_allocator_name_unlocked(void)
{
    PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
#ifdef WITH_PYMALLOC
    PyMemAllocatorEx pymalloc = PYMALLOC_ALLOC;
#endif
#ifdef WITH_MIMALLOC
    PyMemAllocatorEx mimalloc = MIMALLOC_ALLOC;
    PyMemAllocatorEx mimalloc_obj = MIMALLOC_OBJALLOC;
#endif

    if (pymemallocator_eq(&_PyMem_Raw, &malloc_alloc) &&
        pymemallocator_eq(&_PyMem, &malloc_alloc) &&
        pymemallocator_eq(&_PyObject, &malloc_alloc))
    {
        return "malloc";
    }
#ifdef WITH_PYMALLOC
    if (pymemallocator_eq(&_PyMem_Raw, &malloc_alloc) &&
        pymemallocator_eq(&_PyMem, &pymalloc) &&
        pymemallocator_eq(&_PyObject, &pymalloc))
    {
        return "pymalloc";
    }
#endif
#ifdef WITH_MIMALLOC
    if (pymemallocator_eq(&_PyMem_Raw, &malloc_alloc) &&
        pymemallocator_eq(&_PyMem, &mimalloc) &&
        pymemallocator_eq(&_PyObject, &mimalloc_obj))
    {
        return "mimalloc";
    }
#endif

    PyMemAllocatorEx dbg_raw = PYDBGRAW_ALLOC;
    PyMemAllocatorEx dbg_mem = PYDBGMEM_ALLOC;
    PyMemAllocatorEx dbg_obj = PYDBGOBJ_ALLOC;

    if (pymemallocator_eq(&_PyMem_Raw, &dbg_raw) &&
        pymemallocator_eq(&_PyMem, &dbg_mem) &&
        pymemallocator_eq(&_PyObject, &dbg_obj))
    {
        /* Debug hooks installed */
        if (pymemallocator_eq(&_PyMem_Debug.raw.alloc, &malloc_alloc) &&
            pymemallocator_eq(&_PyMem_Debug.mem.alloc, &malloc_alloc) &&
            pymemallocator_eq(&_PyMem_Debug.obj.alloc, &malloc_alloc))
        {
            return "malloc_debug";
        }
#ifdef WITH_PYMALLOC
        if (pymemallocator_eq(&_PyMem_Debug.raw.alloc, &malloc_alloc) &&
            pymemallocator_eq(&_PyMem_Debug.mem.alloc, &pymalloc) &&
            pymemallocator_eq(&_PyMem_Debug.obj.alloc, &pymalloc))
        {
            return "pymalloc_debug";
        }
#endif
#ifdef WITH_MIMALLOC
        if (pymemallocator_eq(&_PyMem_Debug.raw.alloc, &malloc_alloc) &&
            pymemallocator_eq(&_PyMem_Debug.mem.alloc, &mimalloc) &&
            pymemallocator_eq(&_PyMem_Debug.obj.alloc, &mimalloc_obj))
        {
            return "mimalloc_debug";
        }
#endif
    }
    return NULL;
}

const char*
_PyMem_GetCurrentAllocatorName(void)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    const char *name = get_current_allocator_name_unlocked();
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
    return name;
}


int
_PyMem_DebugEnabled(void)
{
    return _PyRuntime.allocators.is_debug_enabled;
}

#ifdef WITH_PYMALLOC
static int
_PyMem_PymallocEnabled(void)
{
    if (_PyMem_DebugEnabled()) {
        return (_PyMem_Debug.obj.alloc.malloc == _PyObject_Malloc);
    }
    else {
        return (_PyObject.malloc == _PyObject_Malloc);
    }
}

#ifdef WITH_MIMALLOC
static int
_PyMem_MimallocEnabled(void)
{
#ifdef Py_GIL_DISABLED
    return 1;
#else
    if (_PyMem_DebugEnabled()) {
        return (_PyMem_Debug.obj.alloc.malloc == _PyObject_MiMalloc);
    }
    else {
        return (_PyObject.malloc == _PyObject_MiMalloc);
    }
#endif
}
#endif  // WITH_MIMALLOC

#endif  // WITH_PYMALLOC


static void
set_up_debug_hooks_domain_unlocked(PyMemAllocatorDomain domain)
{
    PyMemAllocatorEx alloc;

    if (domain == PYMEM_DOMAIN_RAW) {
        if (_PyMem_Raw.malloc == _PyMem_DebugRawMalloc) {
            return;
        }

        get_allocator_unlocked(domain, &_PyMem_Debug.raw.alloc);
        alloc.ctx = &_PyMem_Debug.raw;
        alloc.malloc = _PyMem_DebugRawMalloc;
        alloc.calloc = _PyMem_DebugRawCalloc;
        alloc.realloc = _PyMem_DebugRawRealloc;
        alloc.free = _PyMem_DebugRawFree;
        set_allocator_unlocked(domain, &alloc);
    }
    else if (domain == PYMEM_DOMAIN_MEM) {
        if (_PyMem.malloc == _PyMem_DebugMalloc) {
            return;
        }

        get_allocator_unlocked(domain, &_PyMem_Debug.mem.alloc);
        alloc.ctx = &_PyMem_Debug.mem;
        alloc.malloc = _PyMem_DebugMalloc;
        alloc.calloc = _PyMem_DebugCalloc;
        alloc.realloc = _PyMem_DebugRealloc;
        alloc.free = _PyMem_DebugFree;
        set_allocator_unlocked(domain, &alloc);
    }
    else if (domain == PYMEM_DOMAIN_OBJ)  {
        if (_PyObject.malloc == _PyMem_DebugMalloc) {
            return;
        }

        get_allocator_unlocked(domain, &_PyMem_Debug.obj.alloc);
        alloc.ctx = &_PyMem_Debug.obj;
        alloc.malloc = _PyMem_DebugMalloc;
        alloc.calloc = _PyMem_DebugCalloc;
        alloc.realloc = _PyMem_DebugRealloc;
        alloc.free = _PyMem_DebugFree;
        set_allocator_unlocked(domain, &alloc);
    }
}


static void
set_up_debug_hooks_unlocked(void)
{
    set_up_debug_hooks_domain_unlocked(PYMEM_DOMAIN_RAW);
    set_up_debug_hooks_domain_unlocked(PYMEM_DOMAIN_MEM);
    set_up_debug_hooks_domain_unlocked(PYMEM_DOMAIN_OBJ);
    _PyRuntime.allocators.is_debug_enabled = 1;
}

void
PyMem_SetupDebugHooks(void)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    set_up_debug_hooks_unlocked();
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
}

static void
get_allocator_unlocked(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
{
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: *allocator = _PyMem_Raw; break;
    case PYMEM_DOMAIN_MEM: *allocator = _PyMem; break;
    case PYMEM_DOMAIN_OBJ: *allocator = _PyObject; break;
    default:
        /* unknown domain: set all attributes to NULL */
        allocator->ctx = NULL;
        allocator->malloc = NULL;
        allocator->calloc = NULL;
        allocator->realloc = NULL;
        allocator->free = NULL;
    }
}

static void
set_allocator_unlocked(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
{
    switch(domain)
    {
    case PYMEM_DOMAIN_RAW: _PyMem_Raw = *allocator; break;
    case PYMEM_DOMAIN_MEM: _PyMem = *allocator; break;
    case PYMEM_DOMAIN_OBJ: _PyObject = *allocator; break;
    /* ignore unknown domain */
    }
}

void
PyMem_GetAllocator(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    get_allocator_unlocked(domain, allocator);
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
}

void
PyMem_SetAllocator(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    set_allocator_unlocked(domain, allocator);
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
}

void
PyObject_GetArenaAllocator(PyObjectArenaAllocator *allocator)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    *allocator = _PyObject_Arena;
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
}

void
PyObject_SetArenaAllocator(PyObjectArenaAllocator *allocator)
{
    PyMutex_Lock(&ALLOCATORS_MUTEX);
    _PyObject_Arena = *allocator;
    PyMutex_Unlock(&ALLOCATORS_MUTEX);
}


/* Note that there is a possible, but very unlikely, race in any place
 * below where we call one of the allocator functions.  We access two
 * fields in each case:  "malloc", etc. and "ctx".
 *
 * It is unlikely that the allocator will be changed while one of those
 * calls is happening, much less in that very narrow window.
 * Furthermore, the likelihood of a race is drastically reduced by the
 * fact that the allocator may not be changed after runtime init
 * (except with a wrapper).
 *
 * With the above in mind, we currently don't worry about locking
 * around these uses of the runtime-global allocators state. */


/*************************/
/* the "arena" allocator */
/*************************/

void *
_PyObject_VirtualAlloc(size_t size)
{
    return _PyObject_Arena.alloc(_PyObject_Arena.ctx, size);
}

void
_PyObject_VirtualFree(void *obj, size_t size)
{
    _PyObject_Arena.free(_PyObject_Arena.ctx, obj, size);
}


/***********************/
/* the "raw" allocator */
/***********************/

void *
PyMem_RawMalloc(size_t size)
{
    /*
     * Limit ourselves to PY_SSIZE_T_MAX bytes to prevent security holes.
     * Most python internals blindly use a signed Py_ssize_t to track
     * things without checking for overflows or negatives.
     * As size_t is unsigned, checking for size < 0 is not required.
     */
    if (size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyMem_Raw.malloc(_PyMem_Raw.ctx, size);
}

void *
PyMem_RawCalloc(size_t nelem, size_t elsize)
{
    /* see PyMem_RawMalloc() */
    if (elsize != 0 && nelem > (size_t)PY_SSIZE_T_MAX / elsize)
        return NULL;
    return _PyMem_Raw.calloc(_PyMem_Raw.ctx, nelem, elsize);
}

void*
PyMem_RawRealloc(void *ptr, size_t new_size)
{
    /* see PyMem_RawMalloc() */
    if (new_size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyMem_Raw.realloc(_PyMem_Raw.ctx, ptr, new_size);
}

void PyMem_RawFree(void *ptr)
{
    _PyMem_Raw.free(_PyMem_Raw.ctx, ptr);
}


/***********************/
/* the "mem" allocator */
/***********************/

void *
PyMem_Malloc(size_t size)
{
    /* see PyMem_RawMalloc() */
    if (size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    OBJECT_STAT_INC_COND(allocations512, size < 512);
    OBJECT_STAT_INC_COND(allocations4k, size >= 512 && size < 4094);
    OBJECT_STAT_INC_COND(allocations_big, size >= 4094);
    OBJECT_STAT_INC(allocations);
    return _PyMem.malloc(_PyMem.ctx, size);
}

void *
PyMem_Calloc(size_t nelem, size_t elsize)
{
    /* see PyMem_RawMalloc() */
    if (elsize != 0 && nelem > (size_t)PY_SSIZE_T_MAX / elsize)
        return NULL;
    OBJECT_STAT_INC_COND(allocations512, elsize < 512);
    OBJECT_STAT_INC_COND(allocations4k, elsize >= 512 && elsize < 4094);
    OBJECT_STAT_INC_COND(allocations_big, elsize >= 4094);
    OBJECT_STAT_INC(allocations);
    return _PyMem.calloc(_PyMem.ctx, nelem, elsize);
}

void *
PyMem_Realloc(void *ptr, size_t new_size)
{
    /* see PyMem_RawMalloc() */
    if (new_size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyMem.realloc(_PyMem.ctx, ptr, new_size);
}

void
PyMem_Free(void *ptr)
{
    OBJECT_STAT_INC(frees);
    _PyMem.free(_PyMem.ctx, ptr);
}


/***************************/
/* pymem utility functions */
/***************************/

wchar_t*
_PyMem_RawWcsdup(const wchar_t *str)
{
    assert(str != NULL);

    size_t len = wcslen(str);
    if (len > (size_t)PY_SSIZE_T_MAX / sizeof(wchar_t) - 1) {
        return NULL;
    }

    size_t size = (len + 1) * sizeof(wchar_t);
    wchar_t *str2 = PyMem_RawMalloc(size);
    if (str2 == NULL) {
        return NULL;
    }

    memcpy(str2, str, size);
    return str2;
}

char *
_PyMem_RawStrdup(const char *str)
{
    assert(str != NULL);
    size_t size = strlen(str) + 1;
    char *copy = PyMem_RawMalloc(size);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, str, size);
    return copy;
}

char *
_PyMem_Strdup(const char *str)
{
    assert(str != NULL);
    size_t size = strlen(str) + 1;
    char *copy = PyMem_Malloc(size);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, str, size);
    return copy;
}

/***********************************************/
/* Delayed freeing support for Py_GIL_DISABLED */
/***********************************************/

// So that sizeof(struct _mem_work_chunk) is 4096 bytes on 64-bit platforms.
#define WORK_ITEMS_PER_CHUNK 254

// A pointer to be freed once the QSBR read sequence reaches qsbr_goal.
struct _mem_work_item {
    uintptr_t ptr; // lowest bit tagged 1 for objects freed with PyObject_Free
    uint64_t qsbr_goal;
};

// A fixed-size buffer of pointers to be freed
struct _mem_work_chunk {
    // Linked list node of chunks in queue
    struct llist_node node;

    Py_ssize_t rd_idx;  // index of next item to read
    Py_ssize_t wr_idx;  // index of next item to write
    struct _mem_work_item array[WORK_ITEMS_PER_CHUNK];
};

static int
work_item_should_decref(uintptr_t ptr)
{
    return ptr & 0x01;
}

static void
free_work_item(uintptr_t ptr, delayed_dealloc_cb cb, void *state)
{
    if (work_item_should_decref(ptr)) {
        PyObject *obj = (PyObject *)(ptr - 1);
#ifdef Py_GIL_DISABLED
        if (cb == NULL) {
            assert(!_PyInterpreterState_GET()->stoptheworld.world_stopped);
            Py_DECREF(obj);
            return;
        }
        assert(_PyInterpreterState_GET()->stoptheworld.world_stopped);
        Py_ssize_t refcount = _Py_ExplicitMergeRefcount(obj, -1);
        if (refcount == 0) {
            cb(obj, state);
        }
#else
        Py_DECREF(obj);
#endif
    }
    else {
        PyMem_Free((void *)ptr);
    }
}

static void
free_delayed(uintptr_t ptr)
{
#ifndef Py_GIL_DISABLED
    free_work_item(ptr, NULL, NULL);
#else
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (_PyInterpreterState_GetFinalizing(interp) != NULL ||
        interp->stoptheworld.world_stopped)
    {
        // Free immediately during interpreter shutdown or if the world is
        // stopped.
        assert(!interp->stoptheworld.world_stopped || !work_item_should_decref(ptr));
        free_work_item(ptr, NULL, NULL);
        return;
    }

    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct llist_node *head = &tstate->mem_free_queue;

    struct _mem_work_chunk *buf = NULL;
    if (!llist_empty(head)) {
        // Try to re-use the last buffer
        buf = llist_data(head->prev, struct _mem_work_chunk, node);
        if (buf->wr_idx == WORK_ITEMS_PER_CHUNK) {
            // already full
            buf = NULL;
        }
    }

    if (buf == NULL) {
        buf = PyMem_Calloc(1, sizeof(*buf));
        if (buf != NULL) {
            llist_insert_tail(head, &buf->node);
        }
    }

    if (buf == NULL) {
        // failed to allocate a buffer, free immediately
        PyObject *to_dealloc = NULL;
        _PyEval_StopTheWorld(tstate->base.interp);
        if (work_item_should_decref(ptr)) {
            PyObject *obj = (PyObject *)(ptr - 1);
            Py_ssize_t refcount = _Py_ExplicitMergeRefcount(obj, -1);
            if (refcount == 0) {
                to_dealloc = obj;
            }
        }
        else {
            PyMem_Free((void *)ptr);
        }
        _PyEval_StartTheWorld(tstate->base.interp);
        if (to_dealloc != NULL) {
            _Py_Dealloc(to_dealloc);
        }
        return;
    }

    assert(buf != NULL && buf->wr_idx < WORK_ITEMS_PER_CHUNK);
    uint64_t seq = _Py_qsbr_deferred_advance(tstate->qsbr);
    buf->array[buf->wr_idx].ptr = ptr;
    buf->array[buf->wr_idx].qsbr_goal = seq;
    buf->wr_idx++;

    if (buf->wr_idx == WORK_ITEMS_PER_CHUNK) {
        _PyMem_ProcessDelayed((PyThreadState *)tstate);
    }
#endif
}

void
_PyMem_FreeDelayed(void *ptr)
{
    assert(!((uintptr_t)ptr & 0x01));
    free_delayed((uintptr_t)ptr);
}

#ifdef Py_GIL_DISABLED
void
_PyObject_XDecRefDelayed(PyObject *ptr)
{
    assert(!((uintptr_t)ptr & 0x01));
    if (ptr != NULL) {
        free_delayed(((uintptr_t)ptr)|0x01);
    }
}
#endif

static struct _mem_work_chunk *
work_queue_first(struct llist_node *head)
{
    return llist_data(head->next, struct _mem_work_chunk, node);
}

static void
process_queue(struct llist_node *head, struct _qsbr_thread_state *qsbr,
              bool keep_empty, delayed_dealloc_cb cb, void *state)
{
    while (!llist_empty(head)) {
        struct _mem_work_chunk *buf = work_queue_first(head);

        if (buf->rd_idx < buf->wr_idx) {
            struct _mem_work_item *item = &buf->array[buf->rd_idx];
            if (!_Py_qsbr_poll(qsbr, item->qsbr_goal)) {
                return;
            }

            buf->rd_idx++;
            // NB: free_work_item may re-enter or execute arbitrary code
            free_work_item(item->ptr, cb, state);
            continue;
        }

        assert(buf->rd_idx == buf->wr_idx);
        if (keep_empty && buf->node.next == head) {
            // Keep the last buffer in the queue to reduce re-allocations
            buf->rd_idx = buf->wr_idx = 0;
            return;
        }

        llist_remove(&buf->node);
        PyMem_Free(buf);
    }
}

static void
process_interp_queue(struct _Py_mem_interp_free_queue *queue,
                     struct _qsbr_thread_state *qsbr, delayed_dealloc_cb cb,
                     void *state)
{
    if (!_Py_atomic_load_int_relaxed(&queue->has_work)) {
        return;
    }

    // Try to acquire the lock, but don't block if it's already held.
    if (_PyMutex_LockTimed(&queue->mutex, 0, 0) == PY_LOCK_ACQUIRED) {
        process_queue(&queue->head, qsbr, false, cb, state);

        int more_work = !llist_empty(&queue->head);
        _Py_atomic_store_int_relaxed(&queue->has_work, more_work);

        PyMutex_Unlock(&queue->mutex);
    }
}

void
_PyMem_ProcessDelayed(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;

    // Process thread-local work
    process_queue(&tstate_impl->mem_free_queue, tstate_impl->qsbr, true, NULL, NULL);

    // Process shared interpreter work
    process_interp_queue(&interp->mem_free_queue, tstate_impl->qsbr, NULL, NULL);
}

void
_PyMem_ProcessDelayedNoDealloc(PyThreadState *tstate, delayed_dealloc_cb cb, void *state)
{
    PyInterpreterState *interp = tstate->interp;
    _PyThreadStateImpl *tstate_impl = (_PyThreadStateImpl *)tstate;

    // Process thread-local work
    process_queue(&tstate_impl->mem_free_queue, tstate_impl->qsbr, true, cb, state);

    // Process shared interpreter work
    process_interp_queue(&interp->mem_free_queue, tstate_impl->qsbr, cb, state);
}

void
_PyMem_AbandonDelayed(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;
    struct llist_node *queue = &((_PyThreadStateImpl *)tstate)->mem_free_queue;

    if (llist_empty(queue)) {
        return;
    }

    // Check if the queue contains one empty buffer
    struct _mem_work_chunk *buf = work_queue_first(queue);
    if (buf->rd_idx == buf->wr_idx) {
        llist_remove(&buf->node);
        PyMem_Free(buf);
        assert(llist_empty(queue));
        return;
    }

    // Merge the thread's work queue into the interpreter's work queue.
    PyMutex_Lock(&interp->mem_free_queue.mutex);
    llist_concat(&interp->mem_free_queue.head, queue);
    _Py_atomic_store_int_relaxed(&interp->mem_free_queue.has_work, 1);
    PyMutex_Unlock(&interp->mem_free_queue.mutex);

    assert(llist_empty(queue));  // the thread's queue is now empty
}

void
_PyMem_FiniDelayed(PyInterpreterState *interp)
{
    struct llist_node *head = &interp->mem_free_queue.head;
    while (!llist_empty(head)) {
        struct _mem_work_chunk *buf = work_queue_first(head);

        if (buf->rd_idx < buf->wr_idx) {
            // Free the remaining items immediately. There should be no other
            // threads accessing the memory at this point during shutdown.
            struct _mem_work_item *item = &buf->array[buf->rd_idx];
            buf->rd_idx++;
            // NB: free_work_item may re-enter or execute arbitrary code
            free_work_item(item->ptr, NULL, NULL);
            continue;
        }

        llist_remove(&buf->node);
        PyMem_Free(buf);
    }
}

/**************************/
/* the "object" allocator */
/**************************/

void *
PyObject_Malloc(size_t size)
{
    /* see PyMem_RawMalloc() */
    if (size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    OBJECT_STAT_INC_COND(allocations512, size < 512);
    OBJECT_STAT_INC_COND(allocations4k, size >= 512 && size < 4094);
    OBJECT_STAT_INC_COND(allocations_big, size >= 4094);
    OBJECT_STAT_INC(allocations);
    return _PyObject.malloc(_PyObject.ctx, size);
}

void *
PyObject_Calloc(size_t nelem, size_t elsize)
{
    /* see PyMem_RawMalloc() */
    if (elsize != 0 && nelem > (size_t)PY_SSIZE_T_MAX / elsize)
        return NULL;
    OBJECT_STAT_INC_COND(allocations512, elsize < 512);
    OBJECT_STAT_INC_COND(allocations4k, elsize >= 512 && elsize < 4094);
    OBJECT_STAT_INC_COND(allocations_big, elsize >= 4094);
    OBJECT_STAT_INC(allocations);
    return _PyObject.calloc(_PyObject.ctx, nelem, elsize);
}

void *
PyObject_Realloc(void *ptr, size_t new_size)
{
    /* see PyMem_RawMalloc() */
    if (new_size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyObject.realloc(_PyObject.ctx, ptr, new_size);
}

void
PyObject_Free(void *ptr)
{
    OBJECT_STAT_INC(frees);
    _PyObject.free(_PyObject.ctx, ptr);
}


/* If we're using GCC, use __builtin_expect() to reduce overhead of
   the valgrind checks */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#  define LIKELY(value) __builtin_expect((value), 1)
#else
#  define UNLIKELY(value) (value)
#  define LIKELY(value) (value)
#endif

#ifdef WITH_PYMALLOC

#ifdef WITH_VALGRIND
#include <valgrind/valgrind.h>

/* -1 indicates that we haven't checked that we're running on valgrind yet. */
static int running_on_valgrind = -1;
#endif

typedef struct _obmalloc_state OMState;

/* obmalloc state for main interpreter and shared by all interpreters without
 * their own obmalloc state.  By not explicitly initializing this structure, it
 * will be allocated in the BSS which is a small performance win.  The radix
 * tree arrays are fairly large but are sparsely used.  */
static struct _obmalloc_state obmalloc_state_main;
static bool obmalloc_state_initialized;

static inline int
has_own_state(PyInterpreterState *interp)
{
    return (_Py_IsMainInterpreter(interp) ||
            !(interp->feature_flags & Py_RTFLAGS_USE_MAIN_OBMALLOC) ||
            _Py_IsMainInterpreterFinalizing(interp));
}

static inline OMState *
get_state(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->obmalloc != NULL); // otherwise not initialized or freed
    return interp->obmalloc;
}

// These macros all rely on a local "state" variable.
#define usedpools (state->pools.used)
#define allarenas (state->mgmt.arenas)
#define maxarenas (state->mgmt.maxarenas)
#define unused_arena_objects (state->mgmt.unused_arena_objects)
#define usable_arenas (state->mgmt.usable_arenas)
#define nfp2lasta (state->mgmt.nfp2lasta)
#define narenas_currently_allocated (state->mgmt.narenas_currently_allocated)
#define ntimes_arena_allocated (state->mgmt.ntimes_arena_allocated)
#define narenas_highwater (state->mgmt.narenas_highwater)
#define raw_allocated_blocks (state->mgmt.raw_allocated_blocks)

#ifdef WITH_MIMALLOC
static bool count_blocks(
    const mi_heap_t* heap, const mi_heap_area_t* area,
    void* block, size_t block_size, void* allocated_blocks)
{
    *(size_t *)allocated_blocks += area->used;
    return 1;
}

static Py_ssize_t
get_mimalloc_allocated_blocks(PyInterpreterState *interp)
{
    size_t allocated_blocks = 0;
#ifdef Py_GIL_DISABLED
    _Py_FOR_EACH_TSTATE_UNLOCKED(interp, t) {
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)t;
        for (int i = 0; i < _Py_MIMALLOC_HEAP_COUNT; i++) {
            mi_heap_t *heap = &tstate->mimalloc.heaps[i];
            mi_heap_visit_blocks(heap, false, &count_blocks, &allocated_blocks);
        }
    }

    mi_abandoned_pool_t *pool = &interp->mimalloc.abandoned_pool;
    for (uint8_t tag = 0; tag < _Py_MIMALLOC_HEAP_COUNT; tag++) {
        _mi_abandoned_pool_visit_blocks(pool, tag, false, &count_blocks,
                                        &allocated_blocks);
    }
#else
    // TODO(sgross): this only counts the current thread's blocks.
    mi_heap_t *heap = mi_heap_get_default();
    mi_heap_visit_blocks(heap, false, &count_blocks, &allocated_blocks);
#endif
    return allocated_blocks;
}
#endif

Py_ssize_t
_PyInterpreterState_GetAllocatedBlocks(PyInterpreterState *interp)
{
#ifdef WITH_MIMALLOC
    if (_PyMem_MimallocEnabled()) {
        return get_mimalloc_allocated_blocks(interp);
    }
#endif

#ifdef Py_DEBUG
    assert(has_own_state(interp));
#else
    if (!has_own_state(interp)) {
        _Py_FatalErrorFunc(__func__,
                           "the interpreter doesn't have its own allocator");
    }
#endif
    OMState *state = interp->obmalloc;

    if (state == NULL) {
        return 0;
    }

    Py_ssize_t n = raw_allocated_blocks;
    /* add up allocated blocks for used pools */
    for (uint i = 0; i < maxarenas; ++i) {
        /* Skip arenas which are not allocated. */
        if (allarenas[i].address == 0) {
            continue;
        }

        uintptr_t base = (uintptr_t)_Py_ALIGN_UP(allarenas[i].address, POOL_SIZE);

        /* visit every pool in the arena */
        assert(base <= (uintptr_t) allarenas[i].pool_address);
        for (; base < (uintptr_t) allarenas[i].pool_address; base += POOL_SIZE) {
            poolp p = (poolp)base;
            n += p->ref.count;
        }
    }
    return n;
}

static void free_obmalloc_arenas(PyInterpreterState *interp);

void
_PyInterpreterState_FinalizeAllocatedBlocks(PyInterpreterState *interp)
{
#ifdef WITH_MIMALLOC
    if (_PyMem_MimallocEnabled()) {
        Py_ssize_t leaked = _PyInterpreterState_GetAllocatedBlocks(interp);
        interp->runtime->obmalloc.interpreter_leaks += leaked;
        return;
    }
#endif
    if (has_own_state(interp) && interp->obmalloc != NULL) {
        Py_ssize_t leaked = _PyInterpreterState_GetAllocatedBlocks(interp);
        assert(has_own_state(interp) || leaked == 0);
        interp->runtime->obmalloc.interpreter_leaks += leaked;
        if (_PyMem_obmalloc_state_on_heap(interp) && leaked == 0) {
            // free the obmalloc arenas and radix tree nodes.  If leaked > 0
            // then some of the memory allocated by obmalloc has not been
            // freed.  It might be safe to free the arenas in that case but
            // it's possible that extension modules are still using that
            // memory.  So, it is safer to not free and to leak.  Perhaps there
            // should be warning when this happens.  It should be possible to
            // use a tool like "-fsanitize=address" to track down these leaks.
            free_obmalloc_arenas(interp);
        }
    }
}

static Py_ssize_t get_num_global_allocated_blocks(_PyRuntimeState *);

/* We preserve the number of blocks leaked during runtime finalization,
   so they can be reported if the runtime is initialized again. */
// XXX We don't lose any information by dropping this,
// so we should consider doing so.
static Py_ssize_t last_final_leaks = 0;

void
_Py_FinalizeAllocatedBlocks(_PyRuntimeState *runtime)
{
    last_final_leaks = get_num_global_allocated_blocks(runtime);
    runtime->obmalloc.interpreter_leaks = 0;
}

static Py_ssize_t
get_num_global_allocated_blocks(_PyRuntimeState *runtime)
{
    Py_ssize_t total = 0;
    if (_PyRuntimeState_GetFinalizing(runtime) != NULL) {
        PyInterpreterState *interp = _PyInterpreterState_Main();
        if (interp == NULL) {
            /* We are at the very end of runtime finalization.
               We can't rely on finalizing->interp since that thread
               state is probably already freed, so we don't worry
               about it. */
            assert(PyInterpreterState_Head() == NULL);
        }
        else {
            assert(interp != NULL);
            /* It is probably the last interpreter but not necessarily. */
            assert(PyInterpreterState_Next(interp) == NULL);
            total += _PyInterpreterState_GetAllocatedBlocks(interp);
        }
    }
    else {
        _PyEval_StopTheWorldAll(&_PyRuntime);
        HEAD_LOCK(runtime);
        PyInterpreterState *interp = PyInterpreterState_Head();
        assert(interp != NULL);
#ifdef Py_DEBUG
        int got_main = 0;
#endif
        for (; interp != NULL; interp = PyInterpreterState_Next(interp)) {
#ifdef Py_DEBUG
            if (_Py_IsMainInterpreter(interp)) {
                assert(!got_main);
                got_main = 1;
                assert(has_own_state(interp));
            }
#endif
            if (has_own_state(interp)) {
                total += _PyInterpreterState_GetAllocatedBlocks(interp);
            }
        }
        HEAD_UNLOCK(runtime);
        _PyEval_StartTheWorldAll(&_PyRuntime);
#ifdef Py_DEBUG
        assert(got_main);
#endif
    }
    total += runtime->obmalloc.interpreter_leaks;
    total += last_final_leaks;
    return total;
}

Py_ssize_t
_Py_GetGlobalAllocatedBlocks(void)
{
    return get_num_global_allocated_blocks(&_PyRuntime);
}

#if WITH_PYMALLOC_RADIX_TREE
/*==========================================================================*/
/* radix tree for tracking arena usage. */

#define arena_map_root (state->usage.arena_map_root)
#ifdef USE_INTERIOR_NODES
#define arena_map_mid_count (state->usage.arena_map_mid_count)
#define arena_map_bot_count (state->usage.arena_map_bot_count)
#endif

/* Return a pointer to a bottom tree node, return NULL if it doesn't exist or
 * it cannot be created */
static inline Py_ALWAYS_INLINE arena_map_bot_t *
arena_map_get(OMState *state, pymem_block *p, int create)
{
#ifdef USE_INTERIOR_NODES
    /* sanity check that IGNORE_BITS is correct */
    assert(HIGH_BITS(p) == HIGH_BITS(&arena_map_root));
    int i1 = MAP_TOP_INDEX(p);
    if (arena_map_root.ptrs[i1] == NULL) {
        if (!create) {
            return NULL;
        }
        arena_map_mid_t *n = PyMem_RawCalloc(1, sizeof(arena_map_mid_t));
        if (n == NULL) {
            return NULL;
        }
        arena_map_root.ptrs[i1] = n;
        arena_map_mid_count++;
    }
    int i2 = MAP_MID_INDEX(p);
    if (arena_map_root.ptrs[i1]->ptrs[i2] == NULL) {
        if (!create) {
            return NULL;
        }
        arena_map_bot_t *n = PyMem_RawCalloc(1, sizeof(arena_map_bot_t));
        if (n == NULL) {
            return NULL;
        }
        arena_map_root.ptrs[i1]->ptrs[i2] = n;
        arena_map_bot_count++;
    }
    return arena_map_root.ptrs[i1]->ptrs[i2];
#else
    return &arena_map_root;
#endif
}


/* The radix tree only tracks arenas.  So, for 16 MiB arenas, we throw
 * away 24 bits of the address.  That reduces the space requirement of
 * the tree compared to similar radix tree page-map schemes.  In
 * exchange for slashing the space requirement, it needs more
 * computation to check an address.
 *
 * Tracking coverage is done by "ideal" arena address.  It is easier to
 * explain in decimal so let's say that the arena size is 100 bytes.
 * Then, ideal addresses are 100, 200, 300, etc.  For checking if a
 * pointer address is inside an actual arena, we have to check two ideal
 * arena addresses.  E.g. if pointer is 357, we need to check 200 and
 * 300.  In the rare case that an arena is aligned in the ideal way
 * (e.g. base address of arena is 200) then we only have to check one
 * ideal address.
 *
 * The tree nodes for 200 and 300 both store the address of arena.
 * There are two cases: the arena starts at a lower ideal arena and
 * extends to this one, or the arena starts in this arena and extends to
 * the next ideal arena.  The tail_lo and tail_hi members correspond to
 * these two cases.
 */


/* mark or unmark addresses covered by arena */
static int
arena_map_mark_used(OMState *state, uintptr_t arena_base, int is_used)
{
    /* sanity check that IGNORE_BITS is correct */
    assert(HIGH_BITS(arena_base) == HIGH_BITS(&arena_map_root));
    arena_map_bot_t *n_hi = arena_map_get(
            state, (pymem_block *)arena_base, is_used);
    if (n_hi == NULL) {
        assert(is_used); /* otherwise node should already exist */
        return 0; /* failed to allocate space for node */
    }
    int i3 = MAP_BOT_INDEX((pymem_block *)arena_base);
    int32_t tail = (int32_t)(arena_base & ARENA_SIZE_MASK);
    if (tail == 0) {
        /* is ideal arena address */
        n_hi->arenas[i3].tail_hi = is_used ? -1 : 0;
    }
    else {
        /* arena_base address is not ideal (aligned to arena size) and
         * so it potentially covers two MAP_BOT nodes.  Get the MAP_BOT node
         * for the next arena.  Note that it might be in different MAP_TOP
         * and MAP_MID nodes as well so we need to call arena_map_get()
         * again (do the full tree traversal).
         */
        n_hi->arenas[i3].tail_hi = is_used ? tail : 0;
        uintptr_t arena_base_next = arena_base + ARENA_SIZE;
        /* If arena_base is a legit arena address, so is arena_base_next - 1
         * (last address in arena).  If arena_base_next overflows then it
         * must overflow to 0.  However, that would mean arena_base was
         * "ideal" and we should not be in this case. */
        assert(arena_base < arena_base_next);
        arena_map_bot_t *n_lo = arena_map_get(
                state, (pymem_block *)arena_base_next, is_used);
        if (n_lo == NULL) {
            assert(is_used); /* otherwise should already exist */
            n_hi->arenas[i3].tail_hi = 0;
            return 0; /* failed to allocate space for node */
        }
        int i3_next = MAP_BOT_INDEX(arena_base_next);
        n_lo->arenas[i3_next].tail_lo = is_used ? tail : 0;
    }
    return 1;
}

/* Return true if 'p' is a pointer inside an obmalloc arena.
 * _PyObject_Free() calls this so it needs to be very fast. */
static int
arena_map_is_used(OMState *state, pymem_block *p)
{
    arena_map_bot_t *n = arena_map_get(state, p, 0);
    if (n == NULL) {
        return 0;
    }
    int i3 = MAP_BOT_INDEX(p);
    /* ARENA_BITS must be < 32 so that the tail is a non-negative int32_t. */
    int32_t hi = n->arenas[i3].tail_hi;
    int32_t lo = n->arenas[i3].tail_lo;
    int32_t tail = (int32_t)(AS_UINT(p) & ARENA_SIZE_MASK);
    return (tail < lo) || (tail >= hi && hi != 0);
}

/* end of radix tree logic */
/*==========================================================================*/
#endif /* WITH_PYMALLOC_RADIX_TREE */


/* Allocate a new arena.  If we run out of memory, return NULL.  Else
 * allocate a new arena, and return the address of an arena_object
 * describing the new arena.  It's expected that the caller will set
 * `usable_arenas` to the return value.
 */
static struct arena_object*
new_arena(OMState *state)
{
    struct arena_object* arenaobj;
    uint excess;        /* number of bytes above pool alignment */
    void *address;

    int debug_stats = _PyRuntime.obmalloc.dump_debug_stats;
    if (debug_stats == -1) {
        const char *opt = Py_GETENV("PYTHONMALLOCSTATS");
        debug_stats = (opt != NULL && *opt != '\0');
        _PyRuntime.obmalloc.dump_debug_stats = debug_stats;
    }
    if (debug_stats) {
        _PyObject_DebugMallocStats(stderr);
    }

    if (unused_arena_objects == NULL) {
        uint i;
        uint numarenas;
        size_t nbytes;

        /* Double the number of arena objects on each allocation.
         * Note that it's possible for `numarenas` to overflow.
         */
        numarenas = maxarenas ? maxarenas << 1 : INITIAL_ARENA_OBJECTS;
        if (numarenas <= maxarenas)
            return NULL;                /* overflow */
#if SIZEOF_SIZE_T <= SIZEOF_INT
        if (numarenas > SIZE_MAX / sizeof(*allarenas))
            return NULL;                /* overflow */
#endif
        nbytes = numarenas * sizeof(*allarenas);
        arenaobj = (struct arena_object *)PyMem_RawRealloc(allarenas, nbytes);
        if (arenaobj == NULL)
            return NULL;
        allarenas = arenaobj;

        /* We might need to fix pointers that were copied.  However,
         * new_arena only gets called when all the pages in the
         * previous arenas are full.  Thus, there are *no* pointers
         * into the old array. Thus, we don't have to worry about
         * invalid pointers.  Just to be sure, some asserts:
         */
        assert(usable_arenas == NULL);
        assert(unused_arena_objects == NULL);

        /* Put the new arenas on the unused_arena_objects list. */
        for (i = maxarenas; i < numarenas; ++i) {
            allarenas[i].address = 0;              /* mark as unassociated */
            allarenas[i].nextarena = i < numarenas - 1 ?
                                        &allarenas[i+1] : NULL;
        }

        /* Update globals. */
        unused_arena_objects = &allarenas[maxarenas];
        maxarenas = numarenas;
    }

    /* Take the next available arena object off the head of the list. */
    assert(unused_arena_objects != NULL);
    arenaobj = unused_arena_objects;
    unused_arena_objects = arenaobj->nextarena;
    assert(arenaobj->address == 0);
    address = _PyObject_Arena.alloc(_PyObject_Arena.ctx, ARENA_SIZE);
#if WITH_PYMALLOC_RADIX_TREE
    if (address != NULL) {
        if (!arena_map_mark_used(state, (uintptr_t)address, 1)) {
            /* marking arena in radix tree failed, abort */
            _PyObject_Arena.free(_PyObject_Arena.ctx, address, ARENA_SIZE);
            address = NULL;
        }
    }
#endif
    if (address == NULL) {
        /* The allocation failed: return NULL after putting the
         * arenaobj back.
         */
        arenaobj->nextarena = unused_arena_objects;
        unused_arena_objects = arenaobj;
        return NULL;
    }
    arenaobj->address = (uintptr_t)address;

    ++narenas_currently_allocated;
    ++ntimes_arena_allocated;
    if (narenas_currently_allocated > narenas_highwater)
        narenas_highwater = narenas_currently_allocated;
    arenaobj->freepools = NULL;
    /* pool_address <- first pool-aligned address in the arena
       nfreepools <- number of whole pools that fit after alignment */
    arenaobj->pool_address = (pymem_block*)arenaobj->address;
    arenaobj->nfreepools = MAX_POOLS_IN_ARENA;
    excess = (uint)(arenaobj->address & POOL_SIZE_MASK);
    if (excess != 0) {
        --arenaobj->nfreepools;
        arenaobj->pool_address += POOL_SIZE - excess;
    }
    arenaobj->ntotalpools = arenaobj->nfreepools;

    return arenaobj;
}



#if WITH_PYMALLOC_RADIX_TREE
/* Return true if and only if P is an address that was allocated by
   pymalloc.  When the radix tree is used, 'poolp' is unused.
 */
static bool
address_in_range(OMState *state, void *p, poolp Py_UNUSED(pool))
{
    return arena_map_is_used(state, p);
}
#else
/*
address_in_range(P, POOL)

Return true if and only if P is an address that was allocated by pymalloc.
POOL must be the pool address associated with P, i.e., POOL = POOL_ADDR(P)
(the caller is asked to compute this because the macro expands POOL more than
once, and for efficiency it's best for the caller to assign POOL_ADDR(P) to a
variable and pass the latter to the macro; because address_in_range is
called on every alloc/realloc/free, micro-efficiency is important here).

Tricky:  Let B be the arena base address associated with the pool, B =
arenas[(POOL)->arenaindex].address.  Then P belongs to the arena if and only if

    B <= P < B + ARENA_SIZE

Subtracting B throughout, this is true iff

    0 <= P-B < ARENA_SIZE

By using unsigned arithmetic, the "0 <=" half of the test can be skipped.

Obscure:  A PyMem "free memory" function can call the pymalloc free or realloc
before the first arena has been allocated.  `arenas` is still NULL in that
case.  We're relying on that maxarenas is also 0 in that case, so that
(POOL)->arenaindex < maxarenas  must be false, saving us from trying to index
into a NULL arenas.

Details:  given P and POOL, the arena_object corresponding to P is AO =
arenas[(POOL)->arenaindex].  Suppose obmalloc controls P.  Then (barring wild
stores, etc), POOL is the correct address of P's pool, AO.address is the
correct base address of the pool's arena, and P must be within ARENA_SIZE of
AO.address.  In addition, AO.address is not 0 (no arena can start at address 0
(NULL)).  Therefore address_in_range correctly reports that obmalloc
controls P.

Now suppose obmalloc does not control P (e.g., P was obtained via a direct
call to the system malloc() or realloc()).  (POOL)->arenaindex may be anything
in this case -- it may even be uninitialized trash.  If the trash arenaindex
is >= maxarenas, the macro correctly concludes at once that obmalloc doesn't
control P.

Else arenaindex is < maxarena, and AO is read up.  If AO corresponds to an
allocated arena, obmalloc controls all the memory in slice AO.address :
AO.address+ARENA_SIZE.  By case assumption, P is not controlled by obmalloc,
so P doesn't lie in that slice, so the macro correctly reports that P is not
controlled by obmalloc.

Finally, if P is not controlled by obmalloc and AO corresponds to an unused
arena_object (one not currently associated with an allocated arena),
AO.address is 0, and the second test in the macro reduces to:

    P < ARENA_SIZE

If P >= ARENA_SIZE (extremely likely), the macro again correctly concludes
that P is not controlled by obmalloc.  However, if P < ARENA_SIZE, this part
of the test still passes, and the third clause (AO.address != 0) is necessary
to get the correct result:  AO.address is 0 in this case, so the macro
correctly reports that P is not controlled by obmalloc (despite that P lies in
slice AO.address : AO.address + ARENA_SIZE).

Note:  The third (AO.address != 0) clause was added in Python 2.5.  Before
2.5, arenas were never free()'ed, and an arenaindex < maxarena always
corresponded to a currently-allocated arena, so the "P is not controlled by
obmalloc, AO corresponds to an unused arena_object, and P < ARENA_SIZE" case
was impossible.

Note that the logic is excruciating, and reading up possibly uninitialized
memory when P is not controlled by obmalloc (to get at (POOL)->arenaindex)
creates problems for some memory debuggers.  The overwhelming advantage is
that this test determines whether an arbitrary address is controlled by
obmalloc in a small constant time, independent of the number of arenas
obmalloc controls.  Since this test is needed at every entry point, it's
extremely desirable that it be this fast.
*/

static bool _Py_NO_SANITIZE_ADDRESS
            _Py_NO_SANITIZE_THREAD
            _Py_NO_SANITIZE_MEMORY
address_in_range(OMState *state, void *p, poolp pool)
{
    // Since address_in_range may be reading from memory which was not allocated
    // by Python, it is important that pool->arenaindex is read only once, as
    // another thread may be concurrently modifying the value without holding
    // the GIL. The following dance forces the compiler to read pool->arenaindex
    // only once.
    uint arenaindex = *((volatile uint *)&pool->arenaindex);
    return arenaindex < maxarenas &&
        (uintptr_t)p - allarenas[arenaindex].address < ARENA_SIZE &&
        allarenas[arenaindex].address != 0;
}

#endif /* !WITH_PYMALLOC_RADIX_TREE */

/*==========================================================================*/

// Called when freelist is exhausted.  Extend the freelist if there is
// space for a block.  Otherwise, remove this pool from usedpools.
static void
pymalloc_pool_extend(poolp pool, uint size)
{
    if (UNLIKELY(pool->nextoffset <= pool->maxnextoffset)) {
        /* There is room for another block. */
        pool->freeblock = (pymem_block*)pool + pool->nextoffset;
        pool->nextoffset += INDEX2SIZE(size);
        *(pymem_block **)(pool->freeblock) = NULL;
        return;
    }

    /* Pool is full, unlink from used pools. */
    poolp next;
    next = pool->nextpool;
    pool = pool->prevpool;
    next->prevpool = pool;
    pool->nextpool = next;
}

/* called when pymalloc_alloc can not allocate a block from usedpool.
 * This function takes new pool and allocate a block from it.
 */
static void*
allocate_from_new_pool(OMState *state, uint size)
{
    /* There isn't a pool of the right size class immediately
     * available:  use a free pool.
     */
    if (UNLIKELY(usable_arenas == NULL)) {
        /* No arena has a free pool:  allocate a new arena. */
#ifdef WITH_MEMORY_LIMITS
        if (narenas_currently_allocated >= MAX_ARENAS) {
            return NULL;
        }
#endif
        usable_arenas = new_arena(state);
        if (usable_arenas == NULL) {
            return NULL;
        }
        usable_arenas->nextarena = usable_arenas->prevarena = NULL;
        assert(nfp2lasta[usable_arenas->nfreepools] == NULL);
        nfp2lasta[usable_arenas->nfreepools] = usable_arenas;
    }
    assert(usable_arenas->address != 0);

    /* This arena already had the smallest nfreepools value, so decreasing
     * nfreepools doesn't change that, and we don't need to rearrange the
     * usable_arenas list.  However, if the arena becomes wholly allocated,
     * we need to remove its arena_object from usable_arenas.
     */
    assert(usable_arenas->nfreepools > 0);
    if (nfp2lasta[usable_arenas->nfreepools] == usable_arenas) {
        /* It's the last of this size, so there won't be any. */
        nfp2lasta[usable_arenas->nfreepools] = NULL;
    }
    /* If any free pools will remain, it will be the new smallest. */
    if (usable_arenas->nfreepools > 1) {
        assert(nfp2lasta[usable_arenas->nfreepools - 1] == NULL);
        nfp2lasta[usable_arenas->nfreepools - 1] = usable_arenas;
    }

    /* Try to get a cached free pool. */
    poolp pool = usable_arenas->freepools;
    if (LIKELY(pool != NULL)) {
        /* Unlink from cached pools. */
        usable_arenas->freepools = pool->nextpool;
        usable_arenas->nfreepools--;
        if (UNLIKELY(usable_arenas->nfreepools == 0)) {
            /* Wholly allocated:  remove. */
            assert(usable_arenas->freepools == NULL);
            assert(usable_arenas->nextarena == NULL ||
                   usable_arenas->nextarena->prevarena ==
                   usable_arenas);
            usable_arenas = usable_arenas->nextarena;
            if (usable_arenas != NULL) {
                usable_arenas->prevarena = NULL;
                assert(usable_arenas->address != 0);
            }
        }
        else {
            /* nfreepools > 0:  it must be that freepools
             * isn't NULL, or that we haven't yet carved
             * off all the arena's pools for the first
             * time.
             */
            assert(usable_arenas->freepools != NULL ||
                   usable_arenas->pool_address <=
                   (pymem_block*)usable_arenas->address +
                       ARENA_SIZE - POOL_SIZE);
        }
    }
    else {
        /* Carve off a new pool. */
        assert(usable_arenas->nfreepools > 0);
        assert(usable_arenas->freepools == NULL);
        pool = (poolp)usable_arenas->pool_address;
        assert((pymem_block*)pool <= (pymem_block*)usable_arenas->address +
                                 ARENA_SIZE - POOL_SIZE);
        pool->arenaindex = (uint)(usable_arenas - allarenas);
        assert(&allarenas[pool->arenaindex] == usable_arenas);
        pool->szidx = DUMMY_SIZE_IDX;
        usable_arenas->pool_address += POOL_SIZE;
        --usable_arenas->nfreepools;

        if (usable_arenas->nfreepools == 0) {
            assert(usable_arenas->nextarena == NULL ||
                   usable_arenas->nextarena->prevarena ==
                   usable_arenas);
            /* Unlink the arena:  it is completely allocated. */
            usable_arenas = usable_arenas->nextarena;
            if (usable_arenas != NULL) {
                usable_arenas->prevarena = NULL;
                assert(usable_arenas->address != 0);
            }
        }
    }

    /* Frontlink to used pools. */
    pymem_block *bp;
    poolp next = usedpools[size + size]; /* == prev */
    pool->nextpool = next;
    pool->prevpool = next;
    next->nextpool = pool;
    next->prevpool = pool;
    pool->ref.count = 1;
    if (pool->szidx == size) {
        /* Luckily, this pool last contained blocks
         * of the same size class, so its header
         * and free list are already initialized.
         */
        bp = pool->freeblock;
        assert(bp != NULL);
        pool->freeblock = *(pymem_block **)bp;
        return bp;
    }
    /*
     * Initialize the pool header, set up the free list to
     * contain just the second block, and return the first
     * block.
     */
    pool->szidx = size;
    size = INDEX2SIZE(size);
    bp = (pymem_block *)pool + POOL_OVERHEAD;
    pool->nextoffset = POOL_OVERHEAD + (size << 1);
    pool->maxnextoffset = POOL_SIZE - size;
    pool->freeblock = bp + size;
    *(pymem_block **)(pool->freeblock) = NULL;
    return bp;
}

/* pymalloc allocator

   Return a pointer to newly allocated memory if pymalloc allocated memory.

   Return NULL if pymalloc failed to allocate the memory block: on bigger
   requests, on error in the code below (as a last chance to serve the request)
   or when the max memory limit has been reached.
*/
static inline void*
pymalloc_alloc(OMState *state, void *Py_UNUSED(ctx), size_t nbytes)
{
#ifdef WITH_VALGRIND
    if (UNLIKELY(running_on_valgrind == -1)) {
        running_on_valgrind = RUNNING_ON_VALGRIND;
    }
    if (UNLIKELY(running_on_valgrind)) {
        return NULL;
    }
#endif

    if (UNLIKELY(nbytes == 0)) {
        return NULL;
    }
    if (UNLIKELY(nbytes > SMALL_REQUEST_THRESHOLD)) {
        return NULL;
    }

    uint size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT;
    poolp pool = usedpools[size + size];
    pymem_block *bp;

    if (LIKELY(pool != pool->nextpool)) {
        /*
         * There is a used pool for this size class.
         * Pick up the head block of its free list.
         */
        ++pool->ref.count;
        bp = pool->freeblock;
        assert(bp != NULL);

        if (UNLIKELY((pool->freeblock = *(pymem_block **)bp) == NULL)) {
            // Reached the end of the free list, try to extend it.
            pymalloc_pool_extend(pool, size);
        }
    }
    else {
        /* There isn't a pool of the right size class immediately
         * available:  use a free pool.
         */
        bp = allocate_from_new_pool(state, size);
    }

    return (void *)bp;
}


void *
_PyObject_Malloc(void *ctx, size_t nbytes)
{
    OMState *state = get_state();
    void* ptr = pymalloc_alloc(state, ctx, nbytes);
    if (LIKELY(ptr != NULL)) {
        return ptr;
    }

    ptr = PyMem_RawMalloc(nbytes);
    if (ptr != NULL) {
        raw_allocated_blocks++;
    }
    return ptr;
}


void *
_PyObject_Calloc(void *ctx, size_t nelem, size_t elsize)
{
    assert(elsize == 0 || nelem <= (size_t)PY_SSIZE_T_MAX / elsize);
    size_t nbytes = nelem * elsize;

    OMState *state = get_state();
    void* ptr = pymalloc_alloc(state, ctx, nbytes);
    if (LIKELY(ptr != NULL)) {
        memset(ptr, 0, nbytes);
        return ptr;
    }

    ptr = PyMem_RawCalloc(nelem, elsize);
    if (ptr != NULL) {
        raw_allocated_blocks++;
    }
    return ptr;
}


static void
insert_to_usedpool(OMState *state, poolp pool)
{
    assert(pool->ref.count > 0);            /* else the pool is empty */

    uint size = pool->szidx;
    poolp next = usedpools[size + size];
    poolp prev = next->prevpool;

    /* insert pool before next:   prev <-> pool <-> next */
    pool->nextpool = next;
    pool->prevpool = prev;
    next->prevpool = pool;
    prev->nextpool = pool;
}

static void
insert_to_freepool(OMState *state, poolp pool)
{
    poolp next = pool->nextpool;
    poolp prev = pool->prevpool;
    next->prevpool = prev;
    prev->nextpool = next;

    /* Link the pool to freepools.  This is a singly-linked
     * list, and pool->prevpool isn't used there.
     */
    struct arena_object *ao = &allarenas[pool->arenaindex];
    pool->nextpool = ao->freepools;
    ao->freepools = pool;
    uint nf = ao->nfreepools;
    /* If this is the rightmost arena with this number of free pools,
     * nfp2lasta[nf] needs to change.  Caution:  if nf is 0, there
     * are no arenas in usable_arenas with that value.
     */
    struct arena_object* lastnf = nfp2lasta[nf];
    assert((nf == 0 && lastnf == NULL) ||
           (nf > 0 &&
            lastnf != NULL &&
            lastnf->nfreepools == nf &&
            (lastnf->nextarena == NULL ||
             nf < lastnf->nextarena->nfreepools)));
    if (lastnf == ao) {  /* it is the rightmost */
        struct arena_object* p = ao->prevarena;
        nfp2lasta[nf] = (p != NULL && p->nfreepools == nf) ? p : NULL;
    }
    ao->nfreepools = ++nf;

    /* All the rest is arena management.  We just freed
     * a pool, and there are 4 cases for arena mgmt:
     * 1. If all the pools are free, return the arena to
     *    the system free().  Except if this is the last
     *    arena in the list, keep it to avoid thrashing:
     *    keeping one wholly free arena in the list avoids
     *    pathological cases where a simple loop would
     *    otherwise provoke needing to allocate and free an
     *    arena on every iteration.  See bpo-37257.
     * 2. If this is the only free pool in the arena,
     *    add the arena back to the `usable_arenas` list.
     * 3. If the "next" arena has a smaller count of free
     *    pools, we have to "slide this arena right" to
     *    restore that usable_arenas is sorted in order of
     *    nfreepools.
     * 4. Else there's nothing more to do.
     */
    if (nf == ao->ntotalpools && ao->nextarena != NULL) {
        /* Case 1.  First unlink ao from usable_arenas.
         */
        assert(ao->prevarena == NULL ||
               ao->prevarena->address != 0);
        assert(ao ->nextarena == NULL ||
               ao->nextarena->address != 0);

        /* Fix the pointer in the prevarena, or the
         * usable_arenas pointer.
         */
        if (ao->prevarena == NULL) {
            usable_arenas = ao->nextarena;
            assert(usable_arenas == NULL ||
                   usable_arenas->address != 0);
        }
        else {
            assert(ao->prevarena->nextarena == ao);
            ao->prevarena->nextarena =
                ao->nextarena;
        }
        /* Fix the pointer in the nextarena. */
        if (ao->nextarena != NULL) {
            assert(ao->nextarena->prevarena == ao);
            ao->nextarena->prevarena =
                ao->prevarena;
        }
        /* Record that this arena_object slot is
         * available to be reused.
         */
        ao->nextarena = unused_arena_objects;
        unused_arena_objects = ao;

#if WITH_PYMALLOC_RADIX_TREE
        /* mark arena region as not under control of obmalloc */
        arena_map_mark_used(state, ao->address, 0);
#endif

        /* Free the entire arena. */
        _PyObject_Arena.free(_PyObject_Arena.ctx,
                             (void *)ao->address, ARENA_SIZE);
        ao->address = 0;                        /* mark unassociated */
        --narenas_currently_allocated;

        return;
    }

    if (nf == 1) {
        /* Case 2.  Put ao at the head of
         * usable_arenas.  Note that because
         * ao->nfreepools was 0 before, ao isn't
         * currently on the usable_arenas list.
         */
        ao->nextarena = usable_arenas;
        ao->prevarena = NULL;
        if (usable_arenas)
            usable_arenas->prevarena = ao;
        usable_arenas = ao;
        assert(usable_arenas->address != 0);
        if (nfp2lasta[1] == NULL) {
            nfp2lasta[1] = ao;
        }

        return;
    }

    /* If this arena is now out of order, we need to keep
     * the list sorted.  The list is kept sorted so that
     * the "most full" arenas are used first, which allows
     * the nearly empty arenas to be completely freed.  In
     * a few un-scientific tests, it seems like this
     * approach allowed a lot more memory to be freed.
     */
    /* If this is the only arena with nf, record that. */
    if (nfp2lasta[nf] == NULL) {
        nfp2lasta[nf] = ao;
    } /* else the rightmost with nf doesn't change */
    /* If this was the rightmost of the old size, it remains in place. */
    if (ao == lastnf) {
        /* Case 4.  Nothing to do. */
        return;
    }
    /* If ao were the only arena in the list, the last block would have
     * gotten us out.
     */
    assert(ao->nextarena != NULL);

    /* Case 3:  We have to move the arena towards the end of the list,
     * because it has more free pools than the arena to its right.  It needs
     * to move to follow lastnf.
     * First unlink ao from usable_arenas.
     */
    if (ao->prevarena != NULL) {
        /* ao isn't at the head of the list */
        assert(ao->prevarena->nextarena == ao);
        ao->prevarena->nextarena = ao->nextarena;
    }
    else {
        /* ao is at the head of the list */
        assert(usable_arenas == ao);
        usable_arenas = ao->nextarena;
    }
    ao->nextarena->prevarena = ao->prevarena;
    /* And insert after lastnf. */
    ao->prevarena = lastnf;
    ao->nextarena = lastnf->nextarena;
    if (ao->nextarena != NULL) {
        ao->nextarena->prevarena = ao;
    }
    lastnf->nextarena = ao;
    /* Verify that the swaps worked. */
    assert(ao->nextarena == NULL || nf <= ao->nextarena->nfreepools);
    assert(ao->prevarena == NULL || nf > ao->prevarena->nfreepools);
    assert(ao->nextarena == NULL || ao->nextarena->prevarena == ao);
    assert((usable_arenas == ao && ao->prevarena == NULL)
           || ao->prevarena->nextarena == ao);
}

/* Free a memory block allocated by pymalloc_alloc().
   Return 1 if it was freed.
   Return 0 if the block was not allocated by pymalloc_alloc(). */
static inline int
pymalloc_free(OMState *state, void *Py_UNUSED(ctx), void *p)
{
    assert(p != NULL);

#ifdef WITH_VALGRIND
    if (UNLIKELY(running_on_valgrind > 0)) {
        return 0;
    }
#endif

    poolp pool = POOL_ADDR(p);
    if (UNLIKELY(!address_in_range(state, p, pool))) {
        return 0;
    }
    /* We allocated this address. */

    /* Link p to the start of the pool's freeblock list.  Since
     * the pool had at least the p block outstanding, the pool
     * wasn't empty (so it's already in a usedpools[] list, or
     * was full and is in no list -- it's not in the freeblocks
     * list in any case).
     */
    assert(pool->ref.count > 0);            /* else it was empty */
    pymem_block *lastfree = pool->freeblock;
    *(pymem_block **)p = lastfree;
    pool->freeblock = (pymem_block *)p;
    pool->ref.count--;

    if (UNLIKELY(lastfree == NULL)) {
        /* Pool was full, so doesn't currently live in any list:
         * link it to the front of the appropriate usedpools[] list.
         * This mimics LRU pool usage for new allocations and
         * targets optimal filling when several pools contain
         * blocks of the same size class.
         */
        insert_to_usedpool(state, pool);
        return 1;
    }

    /* freeblock wasn't NULL, so the pool wasn't full,
     * and the pool is in a usedpools[] list.
     */
    if (LIKELY(pool->ref.count != 0)) {
        /* pool isn't empty:  leave it in usedpools */
        return 1;
    }

    /* Pool is now empty:  unlink from usedpools, and
     * link to the front of freepools.  This ensures that
     * previously freed pools will be allocated later
     * (being not referenced, they are perhaps paged out).
     */
    insert_to_freepool(state, pool);
    return 1;
}


void
_PyObject_Free(void *ctx, void *p)
{
    /* PyObject_Free(NULL) has no effect */
    if (p == NULL) {
        return;
    }

    OMState *state = get_state();
    if (UNLIKELY(!pymalloc_free(state, ctx, p))) {
        /* pymalloc didn't allocate this address */
        PyMem_RawFree(p);
        raw_allocated_blocks--;
    }
}


/* pymalloc realloc.

   If nbytes==0, then as the Python docs promise, we do not treat this like
   free(p), and return a non-NULL result.

   Return 1 if pymalloc reallocated memory and wrote the new pointer into
   newptr_p.

   Return 0 if pymalloc didn't allocated p. */
static int
pymalloc_realloc(OMState *state, void *ctx,
                 void **newptr_p, void *p, size_t nbytes)
{
    void *bp;
    poolp pool;
    size_t size;

    assert(p != NULL);

#ifdef WITH_VALGRIND
    /* Treat running_on_valgrind == -1 the same as 0 */
    if (UNLIKELY(running_on_valgrind > 0)) {
        return 0;
    }
#endif

    pool = POOL_ADDR(p);
    if (!address_in_range(state, p, pool)) {
        /* pymalloc is not managing this block.

           If nbytes <= SMALL_REQUEST_THRESHOLD, it's tempting to try to take
           over this block.  However, if we do, we need to copy the valid data
           from the C-managed block to one of our blocks, and there's no
           portable way to know how much of the memory space starting at p is
           valid.

           As bug 1185883 pointed out the hard way, it's possible that the
           C-managed block is "at the end" of allocated VM space, so that a
           memory fault can occur if we try to copy nbytes bytes starting at p.
           Instead we punt: let C continue to manage this block. */
        return 0;
    }

    /* pymalloc is in charge of this block */
    size = INDEX2SIZE(pool->szidx);
    if (nbytes <= size) {
        /* The block is staying the same or shrinking.

           If it's shrinking, there's a tradeoff: it costs cycles to copy the
           block to a smaller size class, but it wastes memory not to copy it.

           The compromise here is to copy on shrink only if at least 25% of
           size can be shaved off. */
        if (4 * nbytes > 3 * size) {
            /* It's the same, or shrinking and new/old > 3/4. */
            *newptr_p = p;
            return 1;
        }
        size = nbytes;
    }

    bp = _PyObject_Malloc(ctx, nbytes);
    if (bp != NULL) {
        memcpy(bp, p, size);
        _PyObject_Free(ctx, p);
    }
    *newptr_p = bp;
    return 1;
}


void *
_PyObject_Realloc(void *ctx, void *ptr, size_t nbytes)
{
    void *ptr2;

    if (ptr == NULL) {
        return _PyObject_Malloc(ctx, nbytes);
    }

    OMState *state = get_state();
    if (pymalloc_realloc(state, ctx, &ptr2, ptr, nbytes)) {
        return ptr2;
    }

    return PyMem_RawRealloc(ptr, nbytes);
}

#else   /* ! WITH_PYMALLOC */

/*==========================================================================*/
/* pymalloc not enabled:  Redirect the entry points to malloc.  These will
 * only be used by extensions that are compiled with pymalloc enabled. */

Py_ssize_t
_PyInterpreterState_GetAllocatedBlocks(PyInterpreterState *Py_UNUSED(interp))
{
    return 0;
}

Py_ssize_t
_Py_GetGlobalAllocatedBlocks(void)
{
    return 0;
}

void
_PyInterpreterState_FinalizeAllocatedBlocks(PyInterpreterState *Py_UNUSED(interp))
{
    return;
}

void
_Py_FinalizeAllocatedBlocks(_PyRuntimeState *Py_UNUSED(runtime))
{
    return;
}

#endif /* WITH_PYMALLOC */


/*==========================================================================*/
/* A x-platform debugging allocator.  This doesn't manage memory directly,
 * it wraps a real allocator, adding extra debugging info to the memory blocks.
 */

/* Uncomment this define to add the "serialno" field */
/* #define PYMEM_DEBUG_SERIALNO */

#ifdef PYMEM_DEBUG_SERIALNO
static size_t serialno = 0;     /* incremented on each debug {m,re}alloc */

/* serialno is always incremented via calling this routine.  The point is
 * to supply a single place to set a breakpoint.
 */
static void
bumpserialno(void)
{
    ++serialno;
}
#endif

#define SST SIZEOF_SIZE_T

#ifdef PYMEM_DEBUG_SERIALNO
#  define PYMEM_DEBUG_EXTRA_BYTES 4 * SST
#else
#  define PYMEM_DEBUG_EXTRA_BYTES 3 * SST
#endif

/* Read sizeof(size_t) bytes at p as a big-endian size_t. */
static size_t
read_size_t(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    size_t result = *q++;
    int i;

    for (i = SST; --i > 0; ++q)
        result = (result << 8) | *q;
    return result;
}

/* Write n as a big-endian size_t, MSB at address p, LSB at
 * p + sizeof(size_t) - 1.
 */
static void
write_size_t(void *p, size_t n)
{
    uint8_t *q = (uint8_t *)p + SST - 1;
    int i;

    for (i = SST; --i >= 0; --q) {
        *q = (uint8_t)(n & 0xff);
        n >>= 8;
    }
}

static void
fill_mem_debug(debug_alloc_api_t *api, void *data, int c, size_t nbytes,
               bool is_alloc)
{
#ifdef Py_GIL_DISABLED
    if (api->api_id == 'o') {
        // Don't overwrite the first few bytes of a PyObject allocation in the
        // free-threaded build
        _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
        size_t debug_offset;
        if (is_alloc) {
            debug_offset = tstate->mimalloc.current_object_heap->debug_offset;
        }
        else {
            char *alloc = (char *)data - 2*SST;  // start of the allocation
            debug_offset = _mi_ptr_page(alloc)->debug_offset;
        }
        debug_offset -= 2*SST;  // account for pymalloc extra bytes
        if (debug_offset < nbytes) {
            memset((char *)data + debug_offset, c, nbytes - debug_offset);
        }
        return;
    }
#endif
    memset(data, c, nbytes);
}

/* Let S = sizeof(size_t).  The debug malloc asks for 4 * S extra bytes and
   fills them with useful stuff, here calling the underlying malloc's result p:

p[0: S]
    Number of bytes originally asked for.  This is a size_t, big-endian (easier
    to read in a memory dump).
p[S]
    API ID.  See PEP 445.  This is a character, but seems undocumented.
p[S+1: 2*S]
    Copies of PYMEM_FORBIDDENBYTE.  Used to catch under- writes and reads.
p[2*S: 2*S+n]
    The requested memory, filled with copies of PYMEM_CLEANBYTE.
    Used to catch reference to uninitialized memory.
    &p[2*S] is returned.  Note that this is 8-byte aligned if pymalloc
    handled the request itself.
p[2*S+n: 2*S+n+S]
    Copies of PYMEM_FORBIDDENBYTE.  Used to catch over- writes and reads.
p[2*S+n+S: 2*S+n+2*S]
    A serial number, incremented by 1 on each call to _PyMem_DebugMalloc
    and _PyMem_DebugRealloc.
    This is a big-endian size_t.
    If "bad memory" is detected later, the serial number gives an
    excellent way to set a breakpoint on the next run, to capture the
    instant at which this block was passed out.

If PYMEM_DEBUG_SERIALNO is not defined (default), the debug malloc only asks
for 3 * S extra bytes, and omits the last serialno field.
*/

static void *
_PyMem_DebugRawAlloc(int use_calloc, void *ctx, size_t nbytes)
{
    debug_alloc_api_t *api = (debug_alloc_api_t *)ctx;
    uint8_t *p;           /* base address of malloc'ed pad block */
    uint8_t *data;        /* p + 2*SST == pointer to data bytes */
    uint8_t *tail;        /* data + nbytes == pointer to tail pad bytes */
    size_t total;         /* nbytes + PYMEM_DEBUG_EXTRA_BYTES */

    if (nbytes > (size_t)PY_SSIZE_T_MAX - PYMEM_DEBUG_EXTRA_BYTES) {
        /* integer overflow: can't represent total as a Py_ssize_t */
        return NULL;
    }
    total = nbytes + PYMEM_DEBUG_EXTRA_BYTES;

    /* Layout: [SSSS IFFF CCCC...CCCC FFFF NNNN]
                ^--- p    ^--- data   ^--- tail
       S: nbytes stored as size_t
       I: API identifier (1 byte)
       F: Forbidden bytes (size_t - 1 bytes before, size_t bytes after)
       C: Clean bytes used later to store actual data
       N: Serial number stored as size_t

       If PYMEM_DEBUG_SERIALNO is not defined (default), the last NNNN field
       is omitted. */

    if (use_calloc) {
        p = (uint8_t *)api->alloc.calloc(api->alloc.ctx, 1, total);
    }
    else {
        p = (uint8_t *)api->alloc.malloc(api->alloc.ctx, total);
    }
    if (p == NULL) {
        return NULL;
    }
    data = p + 2*SST;

#ifdef PYMEM_DEBUG_SERIALNO
    bumpserialno();
#endif

    /* at p, write size (SST bytes), id (1 byte), pad (SST-1 bytes) */
    write_size_t(p, nbytes);
    p[SST] = (uint8_t)api->api_id;
    memset(p + SST + 1, PYMEM_FORBIDDENBYTE, SST-1);

    if (nbytes > 0 && !use_calloc) {
        fill_mem_debug(api, data, PYMEM_CLEANBYTE, nbytes, true);
    }

    /* at tail, write pad (SST bytes) and serialno (SST bytes) */
    tail = data + nbytes;
    memset(tail, PYMEM_FORBIDDENBYTE, SST);
#ifdef PYMEM_DEBUG_SERIALNO
    write_size_t(tail + SST, serialno);
#endif

    return data;
}

void *
_PyMem_DebugRawMalloc(void *ctx, size_t nbytes)
{
    return _PyMem_DebugRawAlloc(0, ctx, nbytes);
}

void *
_PyMem_DebugRawCalloc(void *ctx, size_t nelem, size_t elsize)
{
    size_t nbytes;
    assert(elsize == 0 || nelem <= (size_t)PY_SSIZE_T_MAX / elsize);
    nbytes = nelem * elsize;
    return _PyMem_DebugRawAlloc(1, ctx, nbytes);
}


/* The debug free first checks the 2*SST bytes on each end for sanity (in
   particular, that the FORBIDDENBYTEs with the api ID are still intact).
   Then fills the original bytes with PYMEM_DEADBYTE.
   Then calls the underlying free.
*/
void
_PyMem_DebugRawFree(void *ctx, void *p)
{
    /* PyMem_Free(NULL) has no effect */
    if (p == NULL) {
        return;
    }

    debug_alloc_api_t *api = (debug_alloc_api_t *)ctx;
    uint8_t *q = (uint8_t *)p - 2*SST;  /* address returned from malloc */
    size_t nbytes;

    _PyMem_DebugCheckAddress(__func__, api->api_id, p);
    nbytes = read_size_t(q);
    nbytes += PYMEM_DEBUG_EXTRA_BYTES - 2*SST;
    memset(q, PYMEM_DEADBYTE, 2*SST);
    fill_mem_debug(api, p, PYMEM_DEADBYTE, nbytes, false);
    api->alloc.free(api->alloc.ctx, q);
}


void *
_PyMem_DebugRawRealloc(void *ctx, void *p, size_t nbytes)
{
    if (p == NULL) {
        return _PyMem_DebugRawAlloc(0, ctx, nbytes);
    }

    debug_alloc_api_t *api = (debug_alloc_api_t *)ctx;
    uint8_t *head;        /* base address of malloc'ed pad block */
    uint8_t *data;        /* pointer to data bytes */
    uint8_t *r;
    uint8_t *tail;        /* data + nbytes == pointer to tail pad bytes */
    size_t total;         /* 2 * SST + nbytes + 2 * SST */
    size_t original_nbytes;
#define ERASED_SIZE 64

    _PyMem_DebugCheckAddress(__func__, api->api_id, p);

    data = (uint8_t *)p;
    head = data - 2*SST;
    original_nbytes = read_size_t(head);
    if (nbytes > (size_t)PY_SSIZE_T_MAX - PYMEM_DEBUG_EXTRA_BYTES) {
        /* integer overflow: can't represent total as a Py_ssize_t */
        return NULL;
    }
    total = nbytes + PYMEM_DEBUG_EXTRA_BYTES;

    tail = data + original_nbytes;
#ifdef PYMEM_DEBUG_SERIALNO
    size_t block_serialno = read_size_t(tail + SST);
#endif
#ifndef Py_GIL_DISABLED
    /* Mark the header, the trailer, ERASED_SIZE bytes at the begin and
       ERASED_SIZE bytes at the end as dead and save the copy of erased bytes.
     */
    uint8_t save[2*ERASED_SIZE];  /* A copy of erased bytes. */
    if (original_nbytes <= sizeof(save)) {
        memcpy(save, data, original_nbytes);
        memset(data - 2 * SST, PYMEM_DEADBYTE,
               original_nbytes + PYMEM_DEBUG_EXTRA_BYTES);
    }
    else {
        memcpy(save, data, ERASED_SIZE);
        memset(head, PYMEM_DEADBYTE, ERASED_SIZE + 2 * SST);
        memcpy(&save[ERASED_SIZE], tail - ERASED_SIZE, ERASED_SIZE);
        memset(tail - ERASED_SIZE, PYMEM_DEADBYTE,
               ERASED_SIZE + PYMEM_DEBUG_EXTRA_BYTES - 2 * SST);
    }
#endif

    /* Resize and add decorations. */
    r = (uint8_t *)api->alloc.realloc(api->alloc.ctx, head, total);
    if (r == NULL) {
        /* if realloc() failed: rewrite header and footer which have
           just been erased */
        nbytes = original_nbytes;
    }
    else {
        head = r;
#ifdef PYMEM_DEBUG_SERIALNO
        bumpserialno();
        block_serialno = serialno;
#endif
    }
    data = head + 2*SST;

    write_size_t(head, nbytes);
    head[SST] = (uint8_t)api->api_id;
    memset(head + SST + 1, PYMEM_FORBIDDENBYTE, SST-1);

    tail = data + nbytes;
    memset(tail, PYMEM_FORBIDDENBYTE, SST);
#ifdef PYMEM_DEBUG_SERIALNO
    write_size_t(tail + SST, block_serialno);
#endif

#ifndef Py_GIL_DISABLED
    /* Restore saved bytes. */
    if (original_nbytes <= sizeof(save)) {
        memcpy(data, save, Py_MIN(nbytes, original_nbytes));
    }
    else {
        size_t i = original_nbytes - ERASED_SIZE;
        memcpy(data, save, Py_MIN(nbytes, ERASED_SIZE));
        if (nbytes > i) {
            memcpy(data + i, &save[ERASED_SIZE],
                   Py_MIN(nbytes - i, ERASED_SIZE));
        }
    }
#endif

    if (r == NULL) {
        return NULL;
    }

    if (nbytes > original_nbytes) {
        /* growing: mark new extra memory clean */
        memset(data + original_nbytes, PYMEM_CLEANBYTE,
               nbytes - original_nbytes);
    }

    return data;
}

static inline void
_PyMem_DebugCheckGIL(const char *func)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (tstate == NULL) {
#ifndef Py_GIL_DISABLED
        _Py_FatalErrorFunc(func,
                           "Python memory allocator called "
                           "without holding the GIL");
#else
        _Py_FatalErrorFunc(func,
                           "Python memory allocator called "
                           "without an active thread state. "
                           "Are you trying to call it inside of a Py_BEGIN_ALLOW_THREADS block?");
#endif
    }
}

void *
_PyMem_DebugMalloc(void *ctx, size_t nbytes)
{
    _PyMem_DebugCheckGIL(__func__);
    return _PyMem_DebugRawMalloc(ctx, nbytes);
}

void *
_PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize)
{
    _PyMem_DebugCheckGIL(__func__);
    return _PyMem_DebugRawCalloc(ctx, nelem, elsize);
}


void
_PyMem_DebugFree(void *ctx, void *ptr)
{
    _PyMem_DebugCheckGIL(__func__);
    _PyMem_DebugRawFree(ctx, ptr);
}


void *
_PyMem_DebugRealloc(void *ctx, void *ptr, size_t nbytes)
{
    _PyMem_DebugCheckGIL(__func__);
    return _PyMem_DebugRawRealloc(ctx, ptr, nbytes);
}

/* Check the forbidden bytes on both ends of the memory allocated for p.
 * If anything is wrong, print info to stderr via _PyObject_DebugDumpAddress,
 * and call Py_FatalError to kill the program.
 * The API id, is also checked.
 */
static void
_PyMem_DebugCheckAddress(const char *func, char api, const void *p)
{
    assert(p != NULL);

    const uint8_t *q = (const uint8_t *)p;
    size_t nbytes;
    const uint8_t *tail;
    int i;
    char id;

    /* Check the API id */
    id = (char)q[-SST];
    if (id != api) {
        _PyObject_DebugDumpAddress(p);
        _Py_FatalErrorFormat(func,
                             "bad ID: Allocated using API '%c', "
                             "verified using API '%c'",
                             id, api);
    }

    /* Check the stuff at the start of p first:  if there's underwrite
     * corruption, the number-of-bytes field may be nuts, and checking
     * the tail could lead to a segfault then.
     */
    for (i = SST-1; i >= 1; --i) {
        if (*(q-i) != PYMEM_FORBIDDENBYTE) {
            _PyObject_DebugDumpAddress(p);
            _Py_FatalErrorFunc(func, "bad leading pad byte");
        }
    }

    nbytes = read_size_t(q - 2*SST);
    tail = q + nbytes;
    for (i = 0; i < SST; ++i) {
        if (tail[i] != PYMEM_FORBIDDENBYTE) {
            _PyObject_DebugDumpAddress(p);
            _Py_FatalErrorFunc(func, "bad trailing pad byte");
        }
    }
}

/* Display info to stderr about the memory block at p. */
static void
_PyObject_DebugDumpAddress(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    const uint8_t *tail;
    size_t nbytes;
    int i;
    int ok;
    char id;

    fprintf(stderr, "Debug memory block at address p=%p:", p);
    if (p == NULL) {
        fprintf(stderr, "\n");
        return;
    }
    id = (char)q[-SST];
    fprintf(stderr, " API '%c'\n", id);

    nbytes = read_size_t(q - 2*SST);
    fprintf(stderr, "    %zu bytes originally requested\n", nbytes);

    /* In case this is nuts, check the leading pad bytes first. */
    fprintf(stderr, "    The %d pad bytes at p-%d are ", SST-1, SST-1);
    ok = 1;
    for (i = 1; i <= SST-1; ++i) {
        if (*(q-i) != PYMEM_FORBIDDENBYTE) {
            ok = 0;
            break;
        }
    }
    if (ok)
        fputs("FORBIDDENBYTE, as expected.\n", stderr);
    else {
        fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
            PYMEM_FORBIDDENBYTE);
        for (i = SST-1; i >= 1; --i) {
            const uint8_t byte = *(q-i);
            fprintf(stderr, "        at p-%d: 0x%02x", i, byte);
            if (byte != PYMEM_FORBIDDENBYTE)
                fputs(" *** OUCH", stderr);
            fputc('\n', stderr);
        }

        fputs("    Because memory is corrupted at the start, the "
              "count of bytes requested\n"
              "       may be bogus, and checking the trailing pad "
              "bytes may segfault.\n", stderr);
    }

    tail = q + nbytes;
    fprintf(stderr, "    The %d pad bytes at tail=%p are ", SST, (void *)tail);
    ok = 1;
    for (i = 0; i < SST; ++i) {
        if (tail[i] != PYMEM_FORBIDDENBYTE) {
            ok = 0;
            break;
        }
    }
    if (ok)
        fputs("FORBIDDENBYTE, as expected.\n", stderr);
    else {
        fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
                PYMEM_FORBIDDENBYTE);
        for (i = 0; i < SST; ++i) {
            const uint8_t byte = tail[i];
            fprintf(stderr, "        at tail+%d: 0x%02x",
                    i, byte);
            if (byte != PYMEM_FORBIDDENBYTE)
                fputs(" *** OUCH", stderr);
            fputc('\n', stderr);
        }
    }

#ifdef PYMEM_DEBUG_SERIALNO
    size_t serial = read_size_t(tail + SST);
    fprintf(stderr,
            "    The block was made by call #%zu to debug malloc/realloc.\n",
            serial);
#endif

    if (nbytes > 0) {
        i = 0;
        fputs("    Data at p:", stderr);
        /* print up to 8 bytes at the start */
        while (q < tail && i < 8) {
            fprintf(stderr, " %02x", *q);
            ++i;
            ++q;
        }
        /* and up to 8 at the end */
        if (q < tail) {
            if (tail - q > 8) {
                fputs(" ...", stderr);
                q = tail - 8;
            }
            while (q < tail) {
                fprintf(stderr, " %02x", *q);
                ++q;
            }
        }
        fputc('\n', stderr);
    }
    fputc('\n', stderr);

    fflush(stderr);
    _PyMem_DumpTraceback(fileno(stderr), p);
}


static size_t
printone(FILE *out, const char* msg, size_t value)
{
    int i, k;
    char buf[100];
    size_t origvalue = value;

    fputs(msg, out);
    for (i = (int)strlen(msg); i < 35; ++i)
        fputc(' ', out);
    fputc('=', out);

    /* Write the value with commas. */
    i = 22;
    buf[i--] = '\0';
    buf[i--] = '\n';
    k = 3;
    do {
        size_t nextvalue = value / 10;
        unsigned int digit = (unsigned int)(value - nextvalue * 10);
        value = nextvalue;
        buf[i--] = (char)(digit + '0');
        --k;
        if (k == 0 && value && i >= 0) {
            k = 3;
            buf[i--] = ',';
        }
    } while (value && i >= 0);

    while (i >= 0)
        buf[i--] = ' ';
    fputs(buf, out);

    return origvalue;
}

void
_PyDebugAllocatorStats(FILE *out,
                       const char *block_name, int num_blocks, size_t sizeof_block)
{
    char buf1[128];
    char buf2[128];
    PyOS_snprintf(buf1, sizeof(buf1),
                  "%d %ss * %zd bytes each",
                  num_blocks, block_name, sizeof_block);
    PyOS_snprintf(buf2, sizeof(buf2),
                  "%48s ", buf1);
    (void)printone(out, buf2, num_blocks * sizeof_block);
}

// Return true if the obmalloc state structure is heap allocated,
// by PyMem_RawCalloc().  For the main interpreter, this structure
// allocated in the BSS.  Allocating that way gives some memory savings
// and a small performance win (at least on a demand paged OS).  On
// 64-bit platforms, the obmalloc structure is 256 kB. Most of that
// memory is for the arena_map_top array.  Since normally only one entry
// of that array is used, only one page of resident memory is actually
// used, rather than the full 256 kB.
bool _PyMem_obmalloc_state_on_heap(PyInterpreterState *interp)
{
#if WITH_PYMALLOC
    return interp->obmalloc && interp->obmalloc != &obmalloc_state_main;
#else
    return false;
#endif
}

#ifdef WITH_PYMALLOC
static void
init_obmalloc_pools(PyInterpreterState *interp)
{
    // initialize the obmalloc->pools structure.  This must be done
    // before the obmalloc alloc/free functions can be called.
    poolp temp[OBMALLOC_USED_POOLS_SIZE] =
        _obmalloc_pools_INIT(interp->obmalloc->pools);
    memcpy(&interp->obmalloc->pools.used, temp, sizeof(temp));
}
#endif /* WITH_PYMALLOC */

int _PyMem_init_obmalloc(PyInterpreterState *interp)
{
#ifdef WITH_PYMALLOC
    /* Initialize obmalloc, but only for subinterpreters,
       since the main interpreter is initialized statically. */
    if (_Py_IsMainInterpreter(interp)
            || _PyInterpreterState_HasFeature(interp,
                                              Py_RTFLAGS_USE_MAIN_OBMALLOC)) {
        interp->obmalloc = &obmalloc_state_main;
        if (!obmalloc_state_initialized) {
            init_obmalloc_pools(interp);
            obmalloc_state_initialized = true;
        }
    } else {
        interp->obmalloc = PyMem_RawCalloc(1, sizeof(struct _obmalloc_state));
        if (interp->obmalloc == NULL) {
            return -1;
        }
        init_obmalloc_pools(interp);
    }
#endif /* WITH_PYMALLOC */
    return 0; // success
}


#ifdef WITH_PYMALLOC

static void
free_obmalloc_arenas(PyInterpreterState *interp)
{
    OMState *state = interp->obmalloc;
    for (uint i = 0; i < maxarenas; ++i) {
        // free each obmalloc memory arena
        struct arena_object *ao = &allarenas[i];
        _PyObject_Arena.free(_PyObject_Arena.ctx,
                             (void *)ao->address, ARENA_SIZE);
    }
    // free the array containing pointers to all arenas
    PyMem_RawFree(allarenas);
#if WITH_PYMALLOC_RADIX_TREE
#ifdef USE_INTERIOR_NODES
    // Free the middle and bottom nodes of the radix tree.  These are allocated
    // by arena_map_mark_used() but not freed when arenas are freed.
    for (int i1 = 0; i1 < MAP_TOP_LENGTH; i1++) {
         arena_map_mid_t *mid = arena_map_root.ptrs[i1];
         if (mid == NULL) {
             continue;
         }
         for (int i2 = 0; i2 < MAP_MID_LENGTH; i2++) {
            arena_map_bot_t *bot = arena_map_root.ptrs[i1]->ptrs[i2];
            if (bot == NULL) {
                continue;
            }
            PyMem_RawFree(bot);
         }
         PyMem_RawFree(mid);
    }
#endif
#endif
}

#ifdef Py_DEBUG
/* Is target in the list?  The list is traversed via the nextpool pointers.
 * The list may be NULL-terminated, or circular.  Return 1 if target is in
 * list, else 0.
 */
static int
pool_is_in_list(const poolp target, poolp list)
{
    poolp origlist = list;
    assert(target != NULL);
    if (list == NULL)
        return 0;
    do {
        if (target == list)
            return 1;
        list = list->nextpool;
    } while (list != NULL && list != origlist);
    return 0;
}
#endif

#ifdef WITH_MIMALLOC
struct _alloc_stats {
    size_t allocated_blocks;
    size_t allocated_bytes;
    size_t allocated_with_overhead;
    size_t bytes_reserved;
    size_t bytes_committed;
};

static bool _collect_alloc_stats(
    const mi_heap_t* heap, const mi_heap_area_t* area,
    void* block, size_t block_size, void* arg)
{
    struct _alloc_stats *stats = (struct _alloc_stats *)arg;
    stats->allocated_blocks += area->used;
    stats->allocated_bytes += area->used * area->block_size;
    stats->allocated_with_overhead += area->used * area->full_block_size;
    stats->bytes_reserved += area->reserved;
    stats->bytes_committed += area->committed;
    return 1;
}

static void
py_mimalloc_print_stats(FILE *out)
{
    fprintf(out, "Small block threshold = %zu, in %u size classes.\n",
        (size_t)MI_SMALL_OBJ_SIZE_MAX, MI_BIN_HUGE);
    fprintf(out, "Medium block threshold = %zu\n",
            (size_t)MI_MEDIUM_OBJ_SIZE_MAX);
    fprintf(out, "Large object max size = %zu\n",
            (size_t)MI_LARGE_OBJ_SIZE_MAX);

    mi_heap_t *heap = mi_heap_get_default();
    struct _alloc_stats stats;
    memset(&stats, 0, sizeof(stats));
    mi_heap_visit_blocks(heap, false, &_collect_alloc_stats, &stats);

    fprintf(out, "    Allocated Blocks: %zd\n", stats.allocated_blocks);
    fprintf(out, "    Allocated Bytes: %zd\n", stats.allocated_bytes);
    fprintf(out, "    Allocated Bytes w/ Overhead: %zd\n", stats.allocated_with_overhead);
    fprintf(out, "    Bytes Reserved: %zd\n", stats.bytes_reserved);
    fprintf(out, "    Bytes Committed: %zd\n", stats.bytes_committed);
}
#endif


static void
pymalloc_print_stats(FILE *out)
{
    OMState *state = get_state();

    uint i;
    const uint numclasses = SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT;
    /* # of pools, allocated blocks, and free blocks per class index */
    size_t numpools[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
    size_t numblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
    size_t numfreeblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
    /* total # of allocated bytes in used and full pools */
    size_t allocated_bytes = 0;
    /* total # of available bytes in used pools */
    size_t available_bytes = 0;
    /* # of free pools + pools not yet carved out of current arena */
    uint numfreepools = 0;
    /* # of bytes for arena alignment padding */
    size_t arena_alignment = 0;
    /* # of bytes in used and full pools used for pool_headers */
    size_t pool_header_bytes = 0;
    /* # of bytes in used and full pools wasted due to quantization,
     * i.e. the necessarily leftover space at the ends of used and
     * full pools.
     */
    size_t quantization = 0;
    /* # of arenas actually allocated. */
    size_t narenas = 0;
    /* running total -- should equal narenas * ARENA_SIZE */
    size_t total;
    char buf[128];

    fprintf(out, "Small block threshold = %d, in %u size classes.\n",
            SMALL_REQUEST_THRESHOLD, numclasses);

    for (i = 0; i < numclasses; ++i)
        numpools[i] = numblocks[i] = numfreeblocks[i] = 0;

    /* Because full pools aren't linked to from anything, it's easiest
     * to march over all the arenas.  If we're lucky, most of the memory
     * will be living in full pools -- would be a shame to miss them.
     */
    for (i = 0; i < maxarenas; ++i) {
        uintptr_t base = allarenas[i].address;

        /* Skip arenas which are not allocated. */
        if (allarenas[i].address == (uintptr_t)NULL)
            continue;
        narenas += 1;

        numfreepools += allarenas[i].nfreepools;

        /* round up to pool alignment */
        if (base & (uintptr_t)POOL_SIZE_MASK) {
            arena_alignment += POOL_SIZE;
            base &= ~(uintptr_t)POOL_SIZE_MASK;
            base += POOL_SIZE;
        }

        /* visit every pool in the arena */
        assert(base <= (uintptr_t) allarenas[i].pool_address);
        for (; base < (uintptr_t) allarenas[i].pool_address; base += POOL_SIZE) {
            poolp p = (poolp)base;
            const uint sz = p->szidx;
            uint freeblocks;

            if (p->ref.count == 0) {
                /* currently unused */
#ifdef Py_DEBUG
                assert(pool_is_in_list(p, allarenas[i].freepools));
#endif
                continue;
            }
            ++numpools[sz];
            numblocks[sz] += p->ref.count;
            freeblocks = NUMBLOCKS(sz) - p->ref.count;
            numfreeblocks[sz] += freeblocks;
#ifdef Py_DEBUG
            if (freeblocks > 0)
                assert(pool_is_in_list(p, usedpools[sz + sz]));
#endif
        }
    }
    assert(narenas == narenas_currently_allocated);

    fputc('\n', out);
    fputs("class   size   num pools   blocks in use  avail blocks\n"
          "-----   ----   ---------   -------------  ------------\n",
          out);

    for (i = 0; i < numclasses; ++i) {
        size_t p = numpools[i];
        size_t b = numblocks[i];
        size_t f = numfreeblocks[i];
        uint size = INDEX2SIZE(i);
        if (p == 0) {
            assert(b == 0 && f == 0);
            continue;
        }
        fprintf(out, "%5u %6u %11zu %15zu %13zu\n",
                i, size, p, b, f);
        allocated_bytes += b * size;
        available_bytes += f * size;
        pool_header_bytes += p * POOL_OVERHEAD;
        quantization += p * ((POOL_SIZE - POOL_OVERHEAD) % size);
    }
    fputc('\n', out);
#ifdef PYMEM_DEBUG_SERIALNO
    if (_PyMem_DebugEnabled()) {
        (void)printone(out, "# times object malloc called", serialno);
    }
#endif
    (void)printone(out, "# arenas allocated total", ntimes_arena_allocated);
    (void)printone(out, "# arenas reclaimed", ntimes_arena_allocated - narenas);
    (void)printone(out, "# arenas highwater mark", narenas_highwater);
    (void)printone(out, "# arenas allocated current", narenas);

    PyOS_snprintf(buf, sizeof(buf),
                  "%zu arenas * %d bytes/arena",
                  narenas, ARENA_SIZE);
    (void)printone(out, buf, narenas * ARENA_SIZE);

    fputc('\n', out);

    /* Account for what all of those arena bytes are being used for. */
    total = printone(out, "# bytes in allocated blocks", allocated_bytes);
    total += printone(out, "# bytes in available blocks", available_bytes);

    PyOS_snprintf(buf, sizeof(buf),
        "%u unused pools * %d bytes", numfreepools, POOL_SIZE);
    total += printone(out, buf, (size_t)numfreepools * POOL_SIZE);

    total += printone(out, "# bytes lost to pool headers", pool_header_bytes);
    total += printone(out, "# bytes lost to quantization", quantization);
    total += printone(out, "# bytes lost to arena alignment", arena_alignment);
    (void)printone(out, "Total", total);
    assert(narenas * ARENA_SIZE == total);

#if WITH_PYMALLOC_RADIX_TREE
    fputs("\narena map counts\n", out);
#ifdef USE_INTERIOR_NODES
    (void)printone(out, "# arena map mid nodes", arena_map_mid_count);
    (void)printone(out, "# arena map bot nodes", arena_map_bot_count);
    fputc('\n', out);
#endif
    total = printone(out, "# bytes lost to arena map root", sizeof(arena_map_root));
#ifdef USE_INTERIOR_NODES
    total += printone(out, "# bytes lost to arena map mid",
                      sizeof(arena_map_mid_t) * arena_map_mid_count);
    total += printone(out, "# bytes lost to arena map bot",
                      sizeof(arena_map_bot_t) * arena_map_bot_count);
    (void)printone(out, "Total", total);
#endif
#endif

}

/* Print summary info to "out" about the state of pymalloc's structures.
 * In Py_DEBUG mode, also perform some expensive internal consistency
 * checks.
 *
 * Return 0 if the memory debug hooks are not installed or no statistics was
 * written into out, return 1 otherwise.
 */
int
_PyObject_DebugMallocStats(FILE *out)
{
#ifdef WITH_MIMALLOC
    if (_PyMem_MimallocEnabled()) {
        py_mimalloc_print_stats(out);
        return 1;
    }
    else
#endif
    if (_PyMem_PymallocEnabled()) {
        pymalloc_print_stats(out);
        return 1;
    }
    else {
        return 0;
    }
}

#endif /* #ifdef WITH_PYMALLOC */
