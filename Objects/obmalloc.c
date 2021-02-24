#include "Python.h"
#include "pycore_pymem.h"         // _PyTraceMalloc_Config
#include <stdbool.h>

#include "pycore_pystate.h"
#include "pycore_interp.h"
#include "obmalloc.h"

/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);

/* Forward declaration */
static void* _PyMem_DebugRawMalloc(void *ctx, size_t size);
static void* _PyMem_DebugRawCalloc(void *ctx, size_t nelem, size_t elsize);
static void* _PyMem_DebugRawRealloc(void *ctx, void *ptr, size_t size);
static void _PyMem_DebugRawFree(void *ctx, void *ptr);

static void* _PyMem_DebugMalloc(void *ctx, size_t size);
static void* _PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize);
static void* _PyMem_DebugRealloc(void *ctx, void *ptr, size_t size);
static void _PyMem_DebugFree(void *ctx, void *p);

static void _PyObject_DebugDumpAddress(const void *p);
static void _PyMem_DebugCheckAddress(const char *func, char api_id, const void *p);

static void _PyMem_SetupDebugHooksDomain(PyMemAllocatorDomain domain);

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

#ifdef WITH_PYMALLOC

#ifdef MS_WINDOWS
#  include <windows.h>
#elif defined(HAVE_MMAP)
#  include <sys/mman.h>
#  ifdef MAP_ANONYMOUS
#    define ARENAS_USE_MMAP
#  endif
#endif

/* Forward declaration */
static void* _PyObject_Malloc(void *ctx, size_t size);
static void* _PyObject_Calloc(void *ctx, size_t nelem, size_t elsize);
static void _PyObject_Free(void *ctx, void *p);
static void* _PyObject_Realloc(void *ctx, void *ptr, size_t size);
#endif


/* bpo-35053: Declare tracemalloc configuration here rather than
   Modules/_tracemalloc.c because _tracemalloc can be compiled as dynamic
   library, whereas _Py_NewReference() requires it. */
struct _PyTraceMalloc_Config _Py_tracemalloc_config = _PyTraceMalloc_Config_INIT;


static void *
_PyMem_RawMalloc(void *ctx, size_t size)
{
    /* PyMem_RawMalloc(0) means malloc(1). Some systems would return NULL
       for malloc(0), which would be treated as an error. Some platforms would
       return a pointer with no memory behind it, which would break pymalloc.
       To solve these problems, allocate an extra byte. */
    if (size == 0)
        size = 1;
    return malloc(size);
}

static void *
_PyMem_RawCalloc(void *ctx, size_t nelem, size_t elsize)
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

static void *
_PyMem_RawRealloc(void *ctx, void *ptr, size_t size)
{
    if (size == 0)
        size = 1;
    return realloc(ptr, size);
}

static void
_PyMem_RawFree(void *ctx, void *ptr)
{
    free(ptr);
}


#ifdef MS_WINDOWS
static void *
_PyObject_ArenaVirtualAlloc(void *ctx, size_t size)
{
    return VirtualAlloc(NULL, size,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

static void
_PyObject_ArenaVirtualFree(void *ctx, void *ptr, size_t size)
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

#elif defined(ARENAS_USE_MMAP)
static void *
_PyObject_ArenaMmap(void *ctx, size_t size)
{
    void *ptr;
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE,
               MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    assert(ptr != NULL);
    return ptr;
}

static void
_PyObject_ArenaMunmap(void *ctx, void *ptr, size_t size)
{
    munmap(ptr, size);
}

#else
static void *
_PyObject_ArenaMalloc(void *ctx, size_t size)
{
    return malloc(size);
}

static void
_PyObject_ArenaFree(void *ctx, void *ptr, size_t size)
{
    free(ptr);
}
#endif

#define MALLOC_ALLOC {NULL, _PyMem_RawMalloc, _PyMem_RawCalloc, _PyMem_RawRealloc, _PyMem_RawFree}
#ifdef WITH_PYMALLOC
#  define PYMALLOC_ALLOC {NULL, _PyObject_Malloc, _PyObject_Calloc, _PyObject_Realloc, _PyObject_Free}
#endif

#define PYRAW_ALLOC MALLOC_ALLOC
#ifdef WITH_PYMALLOC
#  define PYOBJ_ALLOC PYMALLOC_ALLOC
#else
#  define PYOBJ_ALLOC MALLOC_ALLOC
#endif
#define PYMEM_ALLOC PYOBJ_ALLOC

typedef struct {
    /* We tag each block with an API ID in order to tag API violations */
    char api_id;
    PyMemAllocatorEx alloc;
} debug_alloc_api_t;
static struct {
    debug_alloc_api_t raw;
    debug_alloc_api_t mem;
    debug_alloc_api_t obj;
} _PyMem_Debug = {
    {'r', PYRAW_ALLOC},
    {'m', PYMEM_ALLOC},
    {'o', PYOBJ_ALLOC}
    };

#define PYDBGRAW_ALLOC \
    {&_PyMem_Debug.raw, _PyMem_DebugRawMalloc, _PyMem_DebugRawCalloc, _PyMem_DebugRawRealloc, _PyMem_DebugRawFree}
#define PYDBGMEM_ALLOC \
    {&_PyMem_Debug.mem, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}
#define PYDBGOBJ_ALLOC \
    {&_PyMem_Debug.obj, _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree}

#ifdef Py_DEBUG
static PyMemAllocatorEx _PyMem_Raw = PYDBGRAW_ALLOC;
static PyMemAllocatorEx _PyMem = PYDBGMEM_ALLOC;
static PyMemAllocatorEx _PyObject = PYDBGOBJ_ALLOC;
#else
static PyMemAllocatorEx _PyMem_Raw = PYRAW_ALLOC;
static PyMemAllocatorEx _PyMem = PYMEM_ALLOC;
static PyMemAllocatorEx _PyObject = PYOBJ_ALLOC;
#endif


static int
pymem_set_default_allocator(PyMemAllocatorDomain domain, int debug,
                            PyMemAllocatorEx *old_alloc)
{
    if (old_alloc != NULL) {
        PyMem_GetAllocator(domain, old_alloc);
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
    PyMem_SetAllocator(domain, &new_alloc);
    if (debug) {
        _PyMem_SetupDebugHooksDomain(domain);
    }
    return 0;
}


int
_PyMem_SetDefaultAllocator(PyMemAllocatorDomain domain,
                           PyMemAllocatorEx *old_alloc)
{
#ifdef Py_DEBUG
    const int debug = 1;
#else
    const int debug = 0;
#endif
    return pymem_set_default_allocator(domain, debug, old_alloc);
}


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
#ifdef WITH_PYMALLOC
    else if (strcmp(name, "pymalloc") == 0) {
        *allocator = PYMEM_ALLOCATOR_PYMALLOC;
    }
    else if (strcmp(name, "pymalloc_debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_PYMALLOC_DEBUG;
    }
#endif
    else if (strcmp(name, "malloc") == 0) {
        *allocator = PYMEM_ALLOCATOR_MALLOC;
    }
    else if (strcmp(name, "malloc_debug") == 0) {
        *allocator = PYMEM_ALLOCATOR_MALLOC_DEBUG;
    }
    else {
        /* unknown allocator */
        return -1;
    }
    return 0;
}


int
_PyMem_SetupAllocators(PyMemAllocatorName allocator)
{
    switch (allocator) {
    case PYMEM_ALLOCATOR_NOT_SET:
        /* do nothing */
        break;

    case PYMEM_ALLOCATOR_DEFAULT:
        (void)_PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, NULL);
        (void)_PyMem_SetDefaultAllocator(PYMEM_DOMAIN_MEM, NULL);
        (void)_PyMem_SetDefaultAllocator(PYMEM_DOMAIN_OBJ, NULL);
        break;

    case PYMEM_ALLOCATOR_DEBUG:
        (void)pymem_set_default_allocator(PYMEM_DOMAIN_RAW, 1, NULL);
        (void)pymem_set_default_allocator(PYMEM_DOMAIN_MEM, 1, NULL);
        (void)pymem_set_default_allocator(PYMEM_DOMAIN_OBJ, 1, NULL);
        break;

#ifdef WITH_PYMALLOC
    case PYMEM_ALLOCATOR_PYMALLOC:
    case PYMEM_ALLOCATOR_PYMALLOC_DEBUG:
    {
        PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &malloc_alloc);

        PyMemAllocatorEx pymalloc = PYMALLOC_ALLOC;
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &pymalloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &pymalloc);

        if (allocator == PYMEM_ALLOCATOR_PYMALLOC_DEBUG) {
            PyMem_SetupDebugHooks();
        }
        break;
    }
#endif

    case PYMEM_ALLOCATOR_MALLOC:
    case PYMEM_ALLOCATOR_MALLOC_DEBUG:
    {
        PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &malloc_alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &malloc_alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &malloc_alloc);

        if (allocator == PYMEM_ALLOCATOR_MALLOC_DEBUG) {
            PyMem_SetupDebugHooks();
        }
        break;
    }

    default:
        /* unknown allocator */
        return -1;
    }
    return 0;
}


static int
pymemallocator_eq(PyMemAllocatorEx *a, PyMemAllocatorEx *b)
{
    return (memcmp(a, b, sizeof(PyMemAllocatorEx)) == 0);
}


const char*
_PyMem_GetCurrentAllocatorName(void)
{
    PyMemAllocatorEx malloc_alloc = MALLOC_ALLOC;
#ifdef WITH_PYMALLOC
    PyMemAllocatorEx pymalloc = PYMALLOC_ALLOC;
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
    }
    return NULL;
}


#undef MALLOC_ALLOC
#undef PYMALLOC_ALLOC
#undef PYRAW_ALLOC
#undef PYMEM_ALLOC
#undef PYOBJ_ALLOC
#undef PYDBGRAW_ALLOC
#undef PYDBGMEM_ALLOC
#undef PYDBGOBJ_ALLOC


static PyObjectArenaAllocator _PyObject_Arena = {NULL,
#ifdef MS_WINDOWS
    _PyObject_ArenaVirtualAlloc, _PyObject_ArenaVirtualFree
#elif defined(ARENAS_USE_MMAP)
    _PyObject_ArenaMmap, _PyObject_ArenaMunmap
#else
    _PyObject_ArenaMalloc, _PyObject_ArenaFree
#endif
    };

#ifdef WITH_PYMALLOC
static int
_PyMem_DebugEnabled(void)
{
    return (_PyObject.malloc == _PyMem_DebugMalloc);
}

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
#endif


static void
_PyMem_SetupDebugHooksDomain(PyMemAllocatorDomain domain)
{
    PyMemAllocatorEx alloc;

    if (domain == PYMEM_DOMAIN_RAW) {
        if (_PyMem_Raw.malloc == _PyMem_DebugRawMalloc) {
            return;
        }

        PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &_PyMem_Debug.raw.alloc);
        alloc.ctx = &_PyMem_Debug.raw;
        alloc.malloc = _PyMem_DebugRawMalloc;
        alloc.calloc = _PyMem_DebugRawCalloc;
        alloc.realloc = _PyMem_DebugRawRealloc;
        alloc.free = _PyMem_DebugRawFree;
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);
    }
    else if (domain == PYMEM_DOMAIN_MEM) {
        if (_PyMem.malloc == _PyMem_DebugMalloc) {
            return;
        }

        PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &_PyMem_Debug.mem.alloc);
        alloc.ctx = &_PyMem_Debug.mem;
        alloc.malloc = _PyMem_DebugMalloc;
        alloc.calloc = _PyMem_DebugCalloc;
        alloc.realloc = _PyMem_DebugRealloc;
        alloc.free = _PyMem_DebugFree;
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);
    }
    else if (domain == PYMEM_DOMAIN_OBJ)  {
        if (_PyObject.malloc == _PyMem_DebugMalloc) {
            return;
        }

        PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &_PyMem_Debug.obj.alloc);
        alloc.ctx = &_PyMem_Debug.obj;
        alloc.malloc = _PyMem_DebugMalloc;
        alloc.calloc = _PyMem_DebugCalloc;
        alloc.realloc = _PyMem_DebugRealloc;
        alloc.free = _PyMem_DebugFree;
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
    }
}


void
PyMem_SetupDebugHooks(void)
{
    _PyMem_SetupDebugHooksDomain(PYMEM_DOMAIN_RAW);
    _PyMem_SetupDebugHooksDomain(PYMEM_DOMAIN_MEM);
    _PyMem_SetupDebugHooksDomain(PYMEM_DOMAIN_OBJ);
}

void
PyMem_GetAllocator(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
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

void
PyMem_SetAllocator(PyMemAllocatorDomain domain, PyMemAllocatorEx *allocator)
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
PyObject_GetArenaAllocator(PyObjectArenaAllocator *allocator)
{
    *allocator = _PyObject_Arena;
}

void
PyObject_SetArenaAllocator(PyObjectArenaAllocator *allocator)
{
    _PyObject_Arena = *allocator;
}

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


void *
PyMem_Malloc(size_t size)
{
    /* see PyMem_RawMalloc() */
    if (size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyMem.malloc(_PyMem.ctx, size);
}

void *
PyMem_Calloc(size_t nelem, size_t elsize)
{
    /* see PyMem_RawMalloc() */
    if (elsize != 0 && nelem > (size_t)PY_SSIZE_T_MAX / elsize)
        return NULL;
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
    _PyMem.free(_PyMem.ctx, ptr);
}


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

void *
PyObject_Malloc(size_t size)
{
    /* see PyMem_RawMalloc() */
    if (size > (size_t)PY_SSIZE_T_MAX)
        return NULL;
    return _PyObject.malloc(_PyObject.ctx, size);
}

void *
PyObject_Calloc(size_t nelem, size_t elsize)
{
    /* see PyMem_RawMalloc() */
    if (elsize != 0 && nelem > (size_t)PY_SSIZE_T_MAX / elsize)
        return NULL;
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

typedef struct _obmalloc_pool_header *poolp;

#define block               _obmalloc_memory_block
#define uint                _obmalloc_uint
#define pool_header         _obmalloc_pool_header
#define arena_object        _obmalloc_arena_object

#define ALIGNMENT                   PYMALLOC_ALIGNMENT
#define ALIGNMENT_SHIFT             PYMALLOC_ALIGNMENT_SHIFT
#define INDEX2SIZE                  PYMALLOC_INDEX2SIZE
#define SMALL_REQUEST_THRESHOLD     PYMALLOC_SMALL_REQUEST_THRESHOLD
#define NB_SMALL_SIZE_CLASSES       PYMALLOC_NB_SMALL_SIZE_CLASSES
#define SYSTEM_PAGE_SIZE            PYMALLOC_SYSTEM_PAGE_SIZE
#define PAGE_SIZE_MASK              PYMALLOC_SYSTEM_PAGE_SIZE_MASK
#define SMALL_MEMORY_LIMIT          PYMALLOC_SMALL_MEMORY_LIMIT
#define ARENA_SIZE                  PYMALLOC_ARENA_SIZE
#define MAX_ARENAS                  PYMALLOC_MAX_ARENAS
#define POOL_SIZE                   PYMALLOC_POOL_SIZE
#define POOL_SIZE_MASK              PYMALLOC_POOL_SIZE_MASK
#define MAX_POOLS_IN_ARENA          PYMALLOC_MAX_POOLS_IN_ARENA

#ifdef WITH_VALGRIND
#include <valgrind/valgrind.h>

/* -1 indicates that we haven't checked that we're running on valgrind yet. */
static int running_on_valgrind = -1;
#endif


/* An object allocator for Python.

   Here is an introduction to the layers of the Python memory architecture,
   showing where the object allocator is actually used (layer +2), It is
   called for every object allocation and deallocation (PyObject_New/Del),
   unless the object-specific allocators implement a proprietary allocation
   scheme (ex.: ints use a simple free list). This is also the place where
   the cyclic garbage collector operates selectively on container objects.


    Object-specific allocators
    _____   ______   ______       ________
   [ int ] [ dict ] [ list ] ... [ string ]       Python core         |
+3 | <----- Object-specific memory -----> | <-- Non-object memory --> |
    _______________________________       |                           |
   [   Python's object allocator   ]      |                           |
+2 | ####### Object memory ####### | <------ Internal buffers ------> |
    ______________________________________________________________    |
   [          Python's raw memory allocator (PyMem_ API)          ]   |
+1 | <----- Python memory (under PyMem manager's control) ------> |   |
    __________________________________________________________________
   [    Underlying general-purpose allocator (ex: C library malloc)   ]
 0 | <------ Virtual memory allocated for the python process -------> |

   =========================================================================
    _______________________________________________________________________
   [                OS-specific Virtual Memory Manager (VMM)               ]
-1 | <--- Kernel dynamic storage allocation & management (page-based) ---> |
    __________________________________   __________________________________
   [                                  ] [                                  ]
-2 | <-- Physical memory: ROM/RAM --> | | <-- Secondary storage (swap) --> |

*/
/*==========================================================================*/

/* A fast, special-purpose memory allocator for small blocks, to be used
   on top of a general-purpose malloc -- heavily based on previous art. */

/* Vladimir Marangozov -- August 2000 */

/*
 * "Memory management is where the rubber meets the road -- if we do the wrong
 * thing at any level, the results will not be good. And if we don't make the
 * levels work well together, we are in serious trouble." (1)
 *
 * (1) Paul R. Wilson, Mark S. Johnstone, Michael Neely, and David Boles,
 *    "Dynamic Storage Allocation: A Survey and Critical Review",
 *    in Proc. 1995 Int'l. Workshop on Memory Management, September 1995.
 */

/* #undef WITH_MEMORY_LIMITS */         /* disable mem limit checks  */

/*==========================================================================*/

/*
 * Allocation strategy abstract:
 *
 * For small requests, the allocator sub-allocates <Big> blocks of memory.
 * Requests greater than SMALL_REQUEST_THRESHOLD bytes are routed to the
 * system's allocator.
 *
 * Small requests are grouped in size classes spaced 8 bytes apart, due
 * to the required valid alignment of the returned address. Requests of
 * a particular size are serviced from memory pools of 4K (one VMM page).
 * Pools are fragmented on demand and contain free lists of blocks of one
 * particular size class. In other words, there is a fixed-size allocator
 * for each size class. Free pools are shared by the different allocators
 * thus minimizing the space reserved for a particular size class.
 *
 * This allocation strategy is a variant of what is known as "simple
 * segregated storage based on array of free lists". The main drawback of
 * simple segregated storage is that we might end up with lot of reserved
 * memory for the different free lists, which degenerate in time. To avoid
 * this, we partition each free list in pools and we share dynamically the
 * reserved space between all free lists. This technique is quite efficient
 * for memory intensive programs which allocate mainly small-sized blocks.
 *
 * For small requests we have the following table:
 *
 * Request in bytes     Size of allocated block      Size class idx
 * ----------------------------------------------------------------
 *        1-8                     8                       0
 *        9-16                   16                       1
 *       17-24                   24                       2
 *       25-32                   32                       3
 *       33-40                   40                       4
 *       41-48                   48                       5
 *       49-56                   56                       6
 *       57-64                   64                       7
 *       65-72                   72                       8
 *        ...                   ...                     ...
 *      497-504                 504                      62
 *      505-512                 512                      63
 *
 *      0, SMALL_REQUEST_THRESHOLD + 1 and up: routed to the underlying
 *      allocator.
 */


#define POOL_OVERHEAD   _Py_SIZE_ROUND_UP(sizeof(struct pool_header), ALIGNMENT)

#define DUMMY_SIZE_IDX          0xffff  /* size class of newly cached pools */

/* Round pointer P down to the closest pool-aligned address <= P, as a poolp */
#define POOL_ADDR(P) ((poolp)_Py_ALIGN_DOWN((P), POOL_SIZE))

/* Return total number of blocks in pool of size index I, as a uint. */
#define NUMBLOCKS(I) ((uint)(POOL_SIZE - POOL_OVERHEAD) / INDEX2SIZE(I))

/*==========================================================================*/

/*
 * Pool table -- headed, circular, doubly-linked lists of partially used pools.

This is involved.  For an index i, usedpools[i+i] is the header for a list of
all partially used pools holding small blocks with "size class idx" i. So
usedpools[0] corresponds to blocks of size 8, usedpools[2] to blocks of size
16, and so on:  index 2*i <-> blocks of size (i+1)<<ALIGNMENT_SHIFT.

Pools are carved off an arena's highwater mark (an arena_object's pool_address
member) as needed.  Once carved off, a pool is in one of three states forever
after:

used == partially used, neither empty nor full
    At least one block in the pool is currently allocated, and at least one
    block in the pool is not currently allocated (note this implies a pool
    has room for at least two blocks).
    This is a pool's initial state, as a pool is created only when malloc
    needs space.
    The pool holds blocks of a fixed size, and is in the circular list headed
    at usedpools[i] (see above).  It's linked to the other used pools of the
    same size class via the pool_header's nextpool and prevpool members.
    If all but one block is currently allocated, a malloc can cause a
    transition to the full state.  If all but one block is not currently
    allocated, a free can cause a transition to the empty state.

full == all the pool's blocks are currently allocated
    On transition to full, a pool is unlinked from its usedpools[] list.
    It's not linked to from anything then anymore, and its nextpool and
    prevpool members are meaningless until it transitions back to used.
    A free of a block in a full pool puts the pool back in the used state.
    Then it's linked in at the front of the appropriate usedpools[] list, so
    that the next allocation for its size class will reuse the freed block.

empty == all the pool's blocks are currently available for allocation
    On transition to empty, a pool is unlinked from its usedpools[] list,
    and linked to the front of its arena_object's singly-linked freepools list,
    via its nextpool member.  The prevpool member has no meaning in this case.
    Empty pools have no inherent size class:  the next time a malloc finds
    an empty list in usedpools[], it takes the first pool off of freepools.
    If the size class needed happens to be the same as the size class the pool
    last had, some pool initialization can be skipped.


Block Management

Blocks within pools are again carved out as needed.  pool->freeblock points to
the start of a singly-linked list of free blocks within the pool.  When a
block is freed, it's inserted at the front of its pool's freeblock list.  Note
that the available blocks in a pool are *not* linked all together when a pool
is initialized.  Instead only "the first two" (lowest addresses) blocks are
set up, returning the first such block, and setting pool->freeblock to a
one-block list holding the second such block.  This is consistent with that
pymalloc strives at all levels (arena, pool, and block) never to touch a piece
of memory until it's actually needed.

So long as a pool is in the used state, we're certain there *is* a block
available for allocating, and pool->freeblock is not NULL.  If pool->freeblock
points to the end of the free list before we've carved the entire pool into
blocks, that means we simply haven't yet gotten to one of the higher-address
blocks.  The offset from the pool_header to the start of "the next" virgin
block is stored in the pool_header nextoffset member, and the largest value
of nextoffset that makes sense is stored in the maxnextoffset member when a
pool is initialized.  All the blocks in a pool have been passed out at least
once when and only when nextoffset > maxnextoffset.


Major obscurity:  While the usedpools vector is declared to have poolp
entries, it doesn't really.  It really contains two pointers per (conceptual)
poolp entry, the nextpool and prevpool members of a pool_header.  The
excruciating initialization code below fools C so that

    usedpool[i+i]

"acts like" a genuine poolp, but only so long as you only reference its
nextpool and prevpool members.  The "- 2*sizeof(block *)" gibberish is
compensating for that a pool_header's nextpool and prevpool members
immediately follow a pool_header's first two members:

    union { block *_padding;
            uint count; } ref;
    block *freeblock;

each of which consume sizeof(block *) bytes.  So what usedpools[i+i] really
contains is a fudged-up pointer p such that *if* C believes it's a poolp
pointer, then p->nextpool and p->prevpool are both p (meaning that the headed
circular list is empty).

It's unclear why the usedpools setup is so convoluted.  It could be to
minimize the amount of cache required to hold this heavily-referenced table
(which only *needs* the two interpool pointer members of a pool_header). OTOH,
referencing code has to remember to "double the index" and doing so isn't
free, usedpools[0] isn't a strictly legal pointer, and we're crucially relying
on that C doesn't insert any padding anywhere in a pool_header at or before
the prevpool member.
**************************************************************************** */


/*==========================================================================
Arena management.

`arenas` is a vector of arena_objects.  It contains maxarenas entries, some of
which may not be currently used (== they're arena_objects that aren't
currently associated with an allocated arena).  Note that arenas proper are
separately malloc'ed.

Prior to Python 2.5, arenas were never free()'ed.  Starting with Python 2.5,
we do try to free() arenas, and use some mild heuristic strategies to increase
the likelihood that arenas eventually can be freed.

unused_arena_objects

    This is a singly-linked list of the arena_objects that are currently not
    being used (no arena is associated with them).  Objects are taken off the
    head of the list in new_arena(), and are pushed on the head of the list in
    PyObject_Free() when the arena is empty.  Key invariant:  an arena_object
    is on this list if and only if its .address member is 0.

usable_arenas

    This is a doubly-linked list of the arena_objects associated with arenas
    that have pools available.  These pools are either waiting to be reused,
    or have not been used before.  The list is sorted to have the most-
    allocated arenas first (ascending order based on the nfreepools member).
    This means that the next allocation will come from a heavily used arena,
    which gives the nearly empty arenas a chance to be returned to the system.
    In my unscientific tests this dramatically improved the number of arenas
    that could be freed.

Note that an arena_object associated with an arena all of whose pools are
currently in use isn't on either list.

Changed in Python 3.8:  keeping usable_arenas sorted by number of free pools
used to be done by one-at-a-time linear search when an arena's number of
free pools changed.  That could, overall, consume time quadratic in the
number of arenas.  That didn't really matter when there were only a few
hundred arenas (typical!), but could be a timing disaster when there were
hundreds of thousands.  See bpo-37029.

Now we have a vector of "search fingers" to eliminate the need to search:
nfp2lasta[nfp] returns the last ("rightmost") arena in usable_arenas
with nfp free pools.  This is NULL if and only if there is no arena with
nfp free pools in usable_arenas.
*/



/* How many arena_objects do we initially allocate?
 * 16 = can allocate 16 arenas = 16 * ARENA_SIZE = 4MB before growing the
 * `arenas` vector.
 */
#define INITIAL_ARENA_OBJECTS 16



#define PTA(x)  ((poolp )((uint8_t *)&(usedpools[2*(x)]) - 2*sizeof(block *)))
#define PT(x)   PTA(x), PTA(x)


static void
initialize_pymalloc_pool(poolp *usedpools)
{
    poolp template_usedpools[2 * ((NB_SMALL_SIZE_CLASSES + 7) / 8) * 8] = {
        PT(0), PT(1), PT(2), PT(3), PT(4), PT(5), PT(6), PT(7)
    #if NB_SMALL_SIZE_CLASSES > 8
        , PT(8), PT(9), PT(10), PT(11), PT(12), PT(13), PT(14), PT(15)
    #if NB_SMALL_SIZE_CLASSES > 16
        , PT(16), PT(17), PT(18), PT(19), PT(20), PT(21), PT(22), PT(23)
    #if NB_SMALL_SIZE_CLASSES > 24
        , PT(24), PT(25), PT(26), PT(27), PT(28), PT(29), PT(30), PT(31)
    #if NB_SMALL_SIZE_CLASSES > 32
        , PT(32), PT(33), PT(34), PT(35), PT(36), PT(37), PT(38), PT(39)
    #if NB_SMALL_SIZE_CLASSES > 40
        , PT(40), PT(41), PT(42), PT(43), PT(44), PT(45), PT(46), PT(47)
    #if NB_SMALL_SIZE_CLASSES > 48
        , PT(48), PT(49), PT(50), PT(51), PT(52), PT(53), PT(54), PT(55)
    #if NB_SMALL_SIZE_CLASSES > 56
        , PT(56), PT(57), PT(58), PT(59), PT(60), PT(61), PT(62), PT(63)
    #if NB_SMALL_SIZE_CLASSES > 64
    #error "NB_SMALL_SIZE_CLASSES should be less than 64"
    #endif /* NB_SMALL_SIZE_CLASSES > 64 */
    #endif /* NB_SMALL_SIZE_CLASSES > 56 */
    #endif /* NB_SMALL_SIZE_CLASSES > 48 */
    #endif /* NB_SMALL_SIZE_CLASSES > 40 */
    #endif /* NB_SMALL_SIZE_CLASSES > 32 */
    #endif /* NB_SMALL_SIZE_CLASSES > 24 */
    #endif /* NB_SMALL_SIZE_CLASSES > 16 */
    #endif /* NB_SMALL_SIZE_CLASSES >  8 */
    };
    
    memcpy((void *)usedpools, (void *)template_usedpools, sizeof(template_usedpools));
}

static struct _Py_pymalloc_state*
get_pymalloc_state(void)
{
    PyInterpreterState* interp = _PyInterpreterState_Get();
    return &interp->pymalloc_state;
}

Py_ssize_t
_Py_GetAllocatedBlocks(void)
{
    struct _Py_pymalloc_state *state = get_pymalloc_state();
    
    Py_ssize_t n = state->raw_allocated_blocks;
    /* add up allocated blocks for used pools */
    for (uint i = 0; i < state->maxarenas; ++i) {
        /* Skip arenas which are not allocated. */
        if (state->arenas[i].address == 0) {
            continue;
        }

        uintptr_t base = (uintptr_t)_Py_ALIGN_UP(state->arenas[i].address, POOL_SIZE);

        /* visit every pool in the arena */
        assert(base <= (uintptr_t) state->arenas[i].pool_address);
        for (; base < (uintptr_t) state->arenas[i].pool_address; base += POOL_SIZE) {
            poolp p = (poolp)base;
            n += p->ref.count;
        }
    }
    return n;
}


/* Allocate a new arena.  If we run out of memory, return NULL.  Else
 * allocate a new arena, and return the address of an arena_object
 * describing the new arena.  It's expected that the caller will set
 * `usable_arenas` to the return value.
 */
static struct arena_object*
new_arena(struct _Py_pymalloc_state *state)
{
    struct arena_object* arenaobj;
    uint excess;        /* number of bytes above pool alignment */
    void *address;
    static int debug_stats = -1;

    if (debug_stats == -1) {
        const char *opt = Py_GETENV("PYTHONMALLOCSTATS");
        debug_stats = (opt != NULL && *opt != '\0');
    }
    if (debug_stats)
        _PyObject_DebugMallocStats(stderr);

    if (state->unused_arena_objects == NULL) {
        uint i;
        uint numarenas;
        size_t nbytes;

        /* Double the number of arena objects on each allocation.
         * Note that it's possible for `numarenas` to overflow.
         */
        numarenas = state->maxarenas ? state->maxarenas << 1 : INITIAL_ARENA_OBJECTS;
        if (numarenas <= state->maxarenas)
            return NULL;                /* overflow */
#if SIZEOF_SIZE_T <= SIZEOF_INT
        if (numarenas > SIZE_MAX / sizeof(*state->arenas))
            return NULL;                /* overflow */
#endif
        nbytes = numarenas * sizeof(*state->arenas);
        arenaobj = (struct arena_object *)PyMem_RawRealloc(state->arenas, nbytes);
        if (arenaobj == NULL)
            return NULL;
        state->arenas = arenaobj;

        /* We might need to fix pointers that were copied.  However,
         * new_arena only gets called when all the pages in the
         * previous arenas are full.  Thus, there are *no* pointers
         * into the old array. Thus, we don't have to worry about
         * invalid pointers.  Just to be sure, some asserts:
         */
        assert(state->usable_arenas == NULL);
        assert(state->unused_arena_objects == NULL);

        /* Put the new arenas on the unused_arena_objects list. */
        for (i = state->maxarenas; i < numarenas; ++i) {
            state->arenas[i].address = 0;              /* mark as unassociated */
            state->arenas[i].nextarena = i < numarenas - 1 ?
                                   &state->arenas[i+1] : NULL;
        }

        /* Update globals. */
        state->unused_arena_objects = &state->arenas[state->maxarenas];
        state->maxarenas = numarenas;
    }

    /* Take the next available arena object off the head of the list. */
    assert(state->unused_arena_objects != NULL);
    arenaobj = state->unused_arena_objects;
    state->unused_arena_objects = arenaobj->nextarena;
    assert(arenaobj->address == 0);
    address = _PyObject_Arena.alloc(_PyObject_Arena.ctx, ARENA_SIZE);
    if (address == NULL) {
        /* The allocation failed: return NULL after putting the
         * arenaobj back.
         */
        arenaobj->nextarena = state->unused_arena_objects;
        state->unused_arena_objects = arenaobj;
        return NULL;
    }
    arenaobj->address = (uintptr_t)address;

    ++state->narenas_currently_allocated;
    ++state->ntimes_arena_allocated;
    if (state->narenas_currently_allocated > state->narenas_highwater)
        state->narenas_highwater = state->narenas_currently_allocated;
    arenaobj->freepools = NULL;
    /* pool_address <- first pool-aligned address in the arena
       nfreepools <- number of whole pools that fit after alignment */
    arenaobj->pool_address = (block*)arenaobj->address;
    arenaobj->nfreepools = MAX_POOLS_IN_ARENA;
    excess = (uint)(arenaobj->address & POOL_SIZE_MASK);
    if (excess != 0) {
        --arenaobj->nfreepools;
        arenaobj->pool_address += POOL_SIZE - excess;
    }
    arenaobj->ntotalpools = arenaobj->nfreepools;

    return arenaobj;
}


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
address_in_range(void *p, poolp pool, struct _Py_pymalloc_state *state)
{
    // Since address_in_range may be reading from memory which was not allocated
    // by Python, it is important that pool->arenaindex is read only once, as
    // another thread may be concurrently modifying the value without holding
    // the GIL. The following dance forces the compiler to read pool->arenaindex
    // only once.
    uint arenaindex = *((volatile uint *)&pool->arenaindex);
    return arenaindex < state->maxarenas &&
        (uintptr_t)p - state->arenas[arenaindex].address < ARENA_SIZE &&
        state->arenas[arenaindex].address != 0;
}


/*==========================================================================*/

// Called when freelist is exhausted.  Extend the freelist if there is
// space for a block.  Otherwise, remove this pool from usedpools.
static void
pymalloc_pool_extend(poolp pool, uint size)
{
    if (UNLIKELY(pool->nextoffset <= pool->maxnextoffset)) {
        /* There is room for another block. */
        pool->freeblock = (block*)pool + pool->nextoffset;
        pool->nextoffset += INDEX2SIZE(size);
        *(block **)(pool->freeblock) = NULL;
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
allocate_from_new_pool(uint size, struct _Py_pymalloc_state *state)
{
    /* There isn't a pool of the right size class immediately
     * available:  use a free pool.
     */
    if (UNLIKELY(state->usable_arenas == NULL)) {
        /* No arena has a free pool:  allocate a new arena. */
#ifdef WITH_MEMORY_LIMITS
        if (state->narenas_currently_allocated >= MAX_ARENAS) {
            return NULL;
        }
#endif
        state->usable_arenas = new_arena(state);
        if (state->usable_arenas == NULL) {
            return NULL;
        }
        state->usable_arenas->nextarena = state->usable_arenas->prevarena = NULL;
        assert(state->nfp2lasta[state->usable_arenas->nfreepools] == NULL);
        state->nfp2lasta[state->usable_arenas->nfreepools] = state->usable_arenas;
    }
    assert(state->usable_arenas->address != 0);

    /* This arena already had the smallest nfreepools value, so decreasing
     * nfreepools doesn't change that, and we don't need to rearrange the
     * usable_arenas list.  However, if the arena becomes wholly allocated,
     * we need to remove its arena_object from usable_arenas.
     */
    assert(state->usable_arenas->nfreepools > 0);
    if (state->nfp2lasta[state->usable_arenas->nfreepools] == state->usable_arenas) {
        /* It's the last of this size, so there won't be any. */
        state->nfp2lasta[state->usable_arenas->nfreepools] = NULL;
    }
    /* If any free pools will remain, it will be the new smallest. */
    if (state->usable_arenas->nfreepools > 1) {
        assert(state->nfp2lasta[state->usable_arenas->nfreepools - 1] == NULL);
        state->nfp2lasta[state->usable_arenas->nfreepools - 1] = state->usable_arenas;
    }

    /* Try to get a cached free pool. */
    poolp pool = state->usable_arenas->freepools;
    if (LIKELY(pool != NULL)) {
        /* Unlink from cached pools. */
        state->usable_arenas->freepools = pool->nextpool;
        state->usable_arenas->nfreepools--;
        if (UNLIKELY(state->usable_arenas->nfreepools == 0)) {
            /* Wholly allocated:  remove. */
            assert(state->usable_arenas->freepools == NULL);
            assert(state->usable_arenas->nextarena == NULL ||
                   state->usable_arenas->nextarena->prevarena ==
                   state->usable_arenas);
            state->usable_arenas = state->usable_arenas->nextarena;
            if (state->usable_arenas != NULL) {
                state->usable_arenas->prevarena = NULL;
                assert(state->usable_arenas->address != 0);
            }
        }
        else {
            /* nfreepools > 0:  it must be that freepools
             * isn't NULL, or that we haven't yet carved
             * off all the arena's pools for the first
             * time.
             */
            assert(state->usable_arenas->freepools != NULL ||
                   state->usable_arenas->pool_address <=
                   (block*)state->usable_arenas->address +
                       ARENA_SIZE - POOL_SIZE);
        }
    }
    else {
        /* Carve off a new pool. */
        assert(state->usable_arenas->nfreepools > 0);
        assert(state->usable_arenas->freepools == NULL);
        pool = (poolp)state->usable_arenas->pool_address;
        assert((block*)pool <= (block*)state->usable_arenas->address +
                                 ARENA_SIZE - POOL_SIZE);
        pool->arenaindex = (uint)(state->usable_arenas - state->arenas);
        assert(&state->arenas[pool->arenaindex] == state->usable_arenas);
        pool->szidx = DUMMY_SIZE_IDX;
        state->usable_arenas->pool_address += POOL_SIZE;
        --state->usable_arenas->nfreepools;

        if (state->usable_arenas->nfreepools == 0) {
            assert(state->usable_arenas->nextarena == NULL ||
                   state->usable_arenas->nextarena->prevarena ==
                   state->usable_arenas);
            /* Unlink the arena:  it is completely allocated. */
            state->usable_arenas = state->usable_arenas->nextarena;
            if (state->usable_arenas != NULL) {
                state->usable_arenas->prevarena = NULL;
                assert(state->usable_arenas->address != 0);
            }
        }
    }

    /* Frontlink to used pools. */
    block *bp;
    poolp next = state->usedpools[size + size]; /* == prev */
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
        pool->freeblock = *(block **)bp;
        return bp;
    }
    /*
     * Initialize the pool header, set up the free list to
     * contain just the second block, and return the first
     * block.
     */
    pool->szidx = size;
    size = INDEX2SIZE(size);
    bp = (block *)pool + POOL_OVERHEAD;
    pool->nextoffset = POOL_OVERHEAD + (size << 1);
    pool->maxnextoffset = POOL_SIZE - size;
    pool->freeblock = bp + size;
    *(block **)(pool->freeblock) = NULL;
    return bp;
}

/* pymalloc allocator

   Return a pointer to newly allocated memory if pymalloc allocated memory.

   Return NULL if pymalloc failed to allocate the memory block: on bigger
   requests, on error in the code below (as a last chance to serve the request)
   or when the max memory limit has been reached.
*/
static inline void*
pymalloc_alloc(void *ctx, size_t nbytes, struct _Py_pymalloc_state *state)
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
    if (state->usedpools_initialized == 0) {
        initialize_pymalloc_pool(state->usedpools);
        state->usedpools_initialized = 1;
    }
    poolp pool = state->usedpools[size + size];
    
    block *bp;

    if (LIKELY(pool != pool->nextpool)) {
        /*
         * There is a used pool for this size class.
         * Pick up the head block of its free list.
         */
        ++pool->ref.count;
        bp = pool->freeblock;
        assert(bp != NULL);

        if (UNLIKELY((pool->freeblock = *(block **)bp) == NULL)) {
            // Reached the end of the free list, try to extend it.
            pymalloc_pool_extend(pool, size);
        }
    }
    else {
        /* There isn't a pool of the right size class immediately
         * available:  use a free pool.
         */
        bp = allocate_from_new_pool(size, state);
    }

    return (void *)bp;
}


static void *
_PyObject_Malloc(void *ctx, size_t nbytes)
{
    struct _Py_pymalloc_state *state = get_pymalloc_state();
    void* ptr = pymalloc_alloc(ctx, nbytes, state);
    if (LIKELY(ptr != NULL)) {
        return ptr;
    }

    ptr = PyMem_RawMalloc(nbytes);
    if (ptr != NULL) {
        state->raw_allocated_blocks++;
    }
    return ptr;
}


static void *
_PyObject_Calloc(void *ctx, size_t nelem, size_t elsize)
{
    assert(elsize == 0 || nelem <= (size_t)PY_SSIZE_T_MAX / elsize);
 
    struct _Py_pymalloc_state *state = get_pymalloc_state();

    size_t nbytes = nelem * elsize;

    void* ptr = pymalloc_alloc(ctx, nbytes, state);
    if (LIKELY(ptr != NULL)) {
        memset(ptr, 0, nbytes);
        return ptr;
    }

    ptr = PyMem_RawCalloc(nelem, elsize);
    if (ptr != NULL) {
        state->raw_allocated_blocks++;
    }
    return ptr;
}


static void
insert_to_usedpool(poolp pool, struct _Py_pymalloc_state *state)
{
    assert(pool->ref.count > 0);            /* else the pool is empty */

    uint size = pool->szidx;
    poolp next = state->usedpools[size + size];
    poolp prev = next->prevpool;

    /* insert pool before next:   prev <-> pool <-> next */
    pool->nextpool = next;
    pool->prevpool = prev;
    next->prevpool = pool;
    prev->nextpool = pool;
}

static void
insert_to_freepool(poolp pool, struct _Py_pymalloc_state *state)
{
    poolp next = pool->nextpool;
    poolp prev = pool->prevpool;
    next->prevpool = prev;
    prev->nextpool = next;

    /* Link the pool to freepools.  This is a singly-linked
     * list, and pool->prevpool isn't used there.
     */
    struct arena_object *ao = &state->arenas[pool->arenaindex];
    pool->nextpool = ao->freepools;
    ao->freepools = pool;
    uint nf = ao->nfreepools;
    /* If this is the rightmost arena with this number of free pools,
     * nfp2lasta[nf] needs to change.  Caution:  if nf is 0, there
     * are no arenas in usable_arenas with that value.
     */
    struct arena_object* lastnf = state->nfp2lasta[nf];
    assert((nf == 0 && lastnf == NULL) ||
           (nf > 0 &&
            lastnf != NULL &&
            lastnf->nfreepools == nf &&
            (lastnf->nextarena == NULL ||
             nf < lastnf->nextarena->nfreepools)));
    if (lastnf == ao) {  /* it is the rightmost */
        struct arena_object* p = ao->prevarena;
        state->nfp2lasta[nf] = (p != NULL && p->nfreepools == nf) ? p : NULL;
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
        /* Case 1.  First unlink ao from state->usable_arenas.
         */
        assert(ao->prevarena == NULL ||
               ao->prevarena->address != 0);
        assert(ao ->nextarena == NULL ||
               ao->nextarena->address != 0);

        /* Fix the pointer in the prevarena, or the
         * state->usable_arenas pointer.
         */
        if (ao->prevarena == NULL) {
            state->usable_arenas = ao->nextarena;
            assert(state->usable_arenas == NULL ||
                   state->usable_arenas->address != 0);
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
        ao->nextarena = state->unused_arena_objects;
        state->unused_arena_objects = ao;

        /* Free the entire arena. */
        _PyObject_Arena.free(_PyObject_Arena.ctx,
                             (void *)ao->address, ARENA_SIZE);
        ao->address = 0;                        /* mark unassociated */
        --state->narenas_currently_allocated;

        return;
    }

    if (nf == 1) {
        /* Case 2.  Put ao at the head of
         * usable_arenas.  Note that because
         * ao->nfreepools was 0 before, ao isn't
         * currently on the usable_arenas list.
         */
        ao->nextarena = state->usable_arenas;
        ao->prevarena = NULL;
        if (state->usable_arenas)
            state->usable_arenas->prevarena = ao;
        state->usable_arenas = ao;
        assert(state->usable_arenas->address != 0);
        if (state->nfp2lasta[1] == NULL) {
            state->nfp2lasta[1] = ao;
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
    if (state->nfp2lasta[nf] == NULL) {
        state->nfp2lasta[nf] = ao;
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
        assert(state->usable_arenas == ao);
        state->usable_arenas = ao->nextarena;
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
    assert((state->usable_arenas == ao && ao->prevarena == NULL)
           || ao->prevarena->nextarena == ao);
}

/* Free a memory block allocated by pymalloc_alloc().
   Return 1 if it was freed.
   Return 0 if the block was not allocated by pymalloc_alloc(). */
static inline int
pymalloc_free(void *ctx, void *p, struct _Py_pymalloc_state *state)
{
    assert(p != NULL);

#ifdef WITH_VALGRIND
    if (UNLIKELY(running_on_valgrind > 0)) {
        return 0;
    }
#endif

    poolp pool = POOL_ADDR(p);
    if (UNLIKELY(!address_in_range(p, pool, state))) {
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
    block *lastfree = pool->freeblock;
    *(block **)p = lastfree;
    pool->freeblock = (block *)p;
    pool->ref.count--;

    if (UNLIKELY(lastfree == NULL)) {
        /* Pool was full, so doesn't currently live in any list:
         * link it to the front of the appropriate usedpools[] list.
         * This mimics LRU pool usage for new allocations and
         * targets optimal filling when several pools contain
         * blocks of the same size class.
         */
        insert_to_usedpool(pool, state);
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
    insert_to_freepool(pool, state);
    return 1;
}


static void
_PyObject_Free(void *ctx, void *p)
{
    struct _Py_pymalloc_state *state = get_pymalloc_state();

    /* PyObject_Free(NULL) has no effect */
    if (p == NULL) {
        return;
    }

    if (UNLIKELY(!pymalloc_free(ctx, p, state))) {
        /* pymalloc didn't allocate this address */
        PyMem_RawFree(p);
        state->raw_allocated_blocks--;
    }
}


/* pymalloc realloc.

   If nbytes==0, then as the Python docs promise, we do not treat this like
   free(p), and return a non-NULL result.

   Return 1 if pymalloc reallocated memory and wrote the new pointer into
   newptr_p.

   Return 0 if pymalloc didn't allocated p. */
static int
pymalloc_realloc(void *ctx, void **newptr_p, void *p, size_t nbytes, struct _Py_pymalloc_state *state)
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
    if (!address_in_range(p, pool, state)) {
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


static void *
_PyObject_Realloc(void *ctx, void *ptr, size_t nbytes)
{
    void *ptr2;

    if (ptr == NULL) {
        return _PyObject_Malloc(ctx, nbytes);
    }
    struct _Py_pymalloc_state *state = get_pymalloc_state();

    if (pymalloc_realloc(ctx, &ptr2, ptr, nbytes, state)) {
        return ptr2;
    }

    return PyMem_RawRealloc(ptr, nbytes);
}

#else   /* ! WITH_PYMALLOC */

/*==========================================================================*/
/* pymalloc not enabled:  Redirect the entry points to malloc.  These will
 * only be used by extensions that are compiled with pymalloc enabled. */

Py_ssize_t
_Py_GetAllocatedBlocks(void)
{
    return 0;
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
        memset(data, PYMEM_CLEANBYTE, nbytes);
    }

    /* at tail, write pad (SST bytes) and serialno (SST bytes) */
    tail = data + nbytes;
    memset(tail, PYMEM_FORBIDDENBYTE, SST);
#ifdef PYMEM_DEBUG_SERIALNO
    write_size_t(tail + SST, serialno);
#endif

    return data;
}

static void *
_PyMem_DebugRawMalloc(void *ctx, size_t nbytes)
{
    return _PyMem_DebugRawAlloc(0, ctx, nbytes);
}

static void *
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
static void
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
    nbytes += PYMEM_DEBUG_EXTRA_BYTES;
    memset(q, PYMEM_DEADBYTE, nbytes);
    api->alloc.free(api->alloc.ctx, q);
}


static void *
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
    uint8_t save[2*ERASED_SIZE];  /* A copy of erased bytes. */

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
    /* Mark the header, the trailer, ERASED_SIZE bytes at the begin and
       ERASED_SIZE bytes at the end as dead and save the copy of erased bytes.
     */
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
    if (!PyGILState_Check()) {
        _Py_FatalErrorFunc(func,
                           "Python memory allocator called "
                           "without holding the GIL");
    }
}

static void *
_PyMem_DebugMalloc(void *ctx, size_t nbytes)
{
    _PyMem_DebugCheckGIL(__func__);
    return _PyMem_DebugRawMalloc(ctx, nbytes);
}

static void *
_PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize)
{
    _PyMem_DebugCheckGIL(__func__);
    return _PyMem_DebugRawCalloc(ctx, nelem, elsize);
}


static void
_PyMem_DebugFree(void *ctx, void *ptr)
{
    _PyMem_DebugCheckGIL(__func__);
    _PyMem_DebugRawFree(ctx, ptr);
}


static void *
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


#ifdef WITH_PYMALLOC

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
    if (!_PyMem_PymallocEnabled()) {
        return 0;
    }
    struct _Py_pymalloc_state *state = get_pymalloc_state();

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
    for (i = 0; i < state->maxarenas; ++i) {
        uint j;
        uintptr_t base = state->arenas[i].address;

        /* Skip arenas which are not allocated. */
        if (state->arenas[i].address == (uintptr_t)NULL)
            continue;
        narenas += 1;

        numfreepools += state->arenas[i].nfreepools;

        /* round up to pool alignment */
        if (base & (uintptr_t)POOL_SIZE_MASK) {
            arena_alignment += POOL_SIZE;
            base &= ~(uintptr_t)POOL_SIZE_MASK;
            base += POOL_SIZE;
        }

        /* visit every pool in the arena */
        assert(base <= (uintptr_t) state->arenas[i].pool_address);
        for (j = 0; base < (uintptr_t) state->arenas[i].pool_address;
             ++j, base += POOL_SIZE) {
            poolp p = (poolp)base;
            const uint sz = p->szidx;
            uint freeblocks;

            if (p->ref.count == 0) {
                /* currently unused */
#ifdef Py_DEBUG
                assert(pool_is_in_list(p, state->arenas[i].freepools));
#endif
                continue;
            }
            ++numpools[sz];
            numblocks[sz] += p->ref.count;
            freeblocks = NUMBLOCKS(sz) - p->ref.count;
            numfreeblocks[sz] += freeblocks;
#ifdef Py_DEBUG
            if (freeblocks > 0)
                assert(pool_is_in_list(p, state->usedpools[sz + sz]));
#endif
        }
    }
    assert(narenas == state->narenas_currently_allocated);

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
    (void)printone(out, "# arenas allocated total", state->ntimes_arena_allocated);
    (void)printone(out, "# arenas reclaimed", state->ntimes_arena_allocated - narenas);
    (void)printone(out, "# arenas highwater mark", state->narenas_highwater);
    (void)printone(out, "# arenas allocated current", narenas);

    PyOS_snprintf(buf, sizeof(buf),
                  "%zu arenas * %d bytes/arena",
                  narenas, ARENA_SIZE);
    (void)printone(out, buf, narenas * ARENA_SIZE);

    fputc('\n', out);

    total = printone(out, "# bytes in allocated blocks", allocated_bytes);
    total += printone(out, "# bytes in available blocks", available_bytes);

    PyOS_snprintf(buf, sizeof(buf),
        "%u unused pools * %d bytes", numfreepools, POOL_SIZE);
    total += printone(out, buf, (size_t)numfreepools * POOL_SIZE);

    total += printone(out, "# bytes lost to pool headers", pool_header_bytes);
    total += printone(out, "# bytes lost to quantization", quantization);
    total += printone(out, "# bytes lost to arena alignment", arena_alignment);
    (void)printone(out, "Total", total);
    return 1;
}

#endif /* #ifdef WITH_PYMALLOC */
