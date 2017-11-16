#include "Python.h"
#include "internal/mem.h"
#include "internal/pystate.h"

#include <stdbool.h>


/* Defined in tracemalloc.c */
extern void _PyMem_DumpTraceback(int fd, const void *ptr);


/* Python's malloc wrappers (see pymem.h) */

#undef  uint
#define uint    unsigned int    /* assuming >= 16 bits */

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
static void _PyMem_DebugCheckAddress(char api_id, const void *p);

#if defined(__has_feature)  /* Clang */
 #if __has_feature(address_sanitizer)  /* is ASAN enabled? */
  #define ATTRIBUTE_NO_ADDRESS_SAFETY_ANALYSIS \
        __attribute__((no_address_safety_analysis))
 #else
  #define ATTRIBUTE_NO_ADDRESS_SAFETY_ANALYSIS
 #endif
#else
 #if defined(__SANITIZE_ADDRESS__)  /* GCC 4.8.x, is ASAN enabled? */
  #define ATTRIBUTE_NO_ADDRESS_SAFETY_ANALYSIS \
        __attribute__((no_address_safety_analysis))
 #else
  #define ATTRIBUTE_NO_ADDRESS_SAFETY_ANALYSIS
 #endif
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


#define PYRAW_FUNCS _PyMem_RawMalloc, _PyMem_RawCalloc, _PyMem_RawRealloc, _PyMem_RawFree
#ifdef WITH_PYMALLOC
#  define PYOBJ_FUNCS _PyObject_Malloc, _PyObject_Calloc, _PyObject_Realloc, _PyObject_Free
#else
#  define PYOBJ_FUNCS PYRAW_FUNCS
#endif
#define PYMEM_FUNCS PYOBJ_FUNCS

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
    {'r', {NULL, PYRAW_FUNCS}},
    {'m', {NULL, PYMEM_FUNCS}},
    {'o', {NULL, PYOBJ_FUNCS}}
    };

#define PYRAWDBG_FUNCS \
    _PyMem_DebugRawMalloc, _PyMem_DebugRawCalloc, _PyMem_DebugRawRealloc, _PyMem_DebugRawFree
#define PYDBG_FUNCS \
    _PyMem_DebugMalloc, _PyMem_DebugCalloc, _PyMem_DebugRealloc, _PyMem_DebugFree


#define _PyMem_Raw _PyRuntime.mem.allocators.raw

#define _PyMem _PyRuntime.mem.allocators.mem

#define _PyObject _PyRuntime.mem.allocators.obj

void
_PyMem_GetDefaultRawAllocator(PyMemAllocatorEx *alloc_p)
{
    PyMemAllocatorEx pymem_raw = {
#ifdef Py_DEBUG
        &_PyMem_Debug.raw, PYRAWDBG_FUNCS
#else
        NULL, PYRAW_FUNCS
#endif
    };
    *alloc_p = pymem_raw;
}

int
_PyMem_SetupAllocators(const char *opt)
{
    if (opt == NULL || *opt == '\0') {
        /* PYTHONMALLOC is empty or is not set or ignored (-E/-I command line
           options): use default allocators */
#ifdef Py_DEBUG
#  ifdef WITH_PYMALLOC
        opt = "pymalloc_debug";
#  else
        opt = "malloc_debug";
#  endif
#else
   /* !Py_DEBUG */
#  ifdef WITH_PYMALLOC
        opt = "pymalloc";
#  else
        opt = "malloc";
#  endif
#endif
    }

    if (strcmp(opt, "debug") == 0) {
        PyMem_SetupDebugHooks();
    }
    else if (strcmp(opt, "malloc") == 0 || strcmp(opt, "malloc_debug") == 0)
    {
        PyMemAllocatorEx alloc = {NULL, PYRAW_FUNCS};

        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);

        if (strcmp(opt, "malloc_debug") == 0)
            PyMem_SetupDebugHooks();
    }
#ifdef WITH_PYMALLOC
    else if (strcmp(opt, "pymalloc") == 0
             || strcmp(opt, "pymalloc_debug") == 0)
    {
        PyMemAllocatorEx raw_alloc = {NULL, PYRAW_FUNCS};
        PyMemAllocatorEx mem_alloc = {NULL, PYMEM_FUNCS};
        PyMemAllocatorEx obj_alloc = {NULL, PYOBJ_FUNCS};

        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &raw_alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &mem_alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &obj_alloc);

        if (strcmp(opt, "pymalloc_debug") == 0)
            PyMem_SetupDebugHooks();
    }
#endif
    else {
        /* unknown allocator */
        return -1;
    }
    return 0;
}


void
_PyObject_Initialize(struct _pyobj_runtime_state *state)
{
    PyObjectArenaAllocator _PyObject_Arena = {NULL,
#ifdef MS_WINDOWS
        _PyObject_ArenaVirtualAlloc, _PyObject_ArenaVirtualFree
#elif defined(ARENAS_USE_MMAP)
        _PyObject_ArenaMmap, _PyObject_ArenaMunmap
#else
        _PyObject_ArenaMalloc, _PyObject_ArenaFree
#endif
    };

    state->allocator_arenas = _PyObject_Arena;
}


void
_PyMem_Initialize(struct _pymem_runtime_state *state)
{
    PyMemAllocatorEx pymem = {
#ifdef Py_DEBUG
        &_PyMem_Debug.mem, PYDBG_FUNCS
#else
        NULL, PYMEM_FUNCS
#endif
    };
    PyMemAllocatorEx pyobject = {
#ifdef Py_DEBUG
        &_PyMem_Debug.obj, PYDBG_FUNCS
#else
        NULL, PYOBJ_FUNCS
#endif
    };

    _PyMem_GetDefaultRawAllocator(&state->allocators.raw);
    state->allocators.mem = pymem;
    state->allocators.obj = pyobject;

#ifdef WITH_PYMALLOC
    Py_BUILD_ASSERT(NB_SMALL_SIZE_CLASSES == 64);

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            int x = i * 8 + j;
            poolp *addr = &(state->usedpools[2*(x)]);
            poolp val = (poolp)((uint8_t *)addr - 2*sizeof(pyblock *));
            state->usedpools[x * 2] = val;
            state->usedpools[x * 2 + 1] = val;
        };
    };
#endif /* WITH_PYMALLOC */
}


#ifdef WITH_PYMALLOC
static int
_PyMem_DebugEnabled(void)
{
    return (_PyObject.malloc == _PyMem_DebugMalloc);
}

int
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

void
PyMem_SetupDebugHooks(void)
{
    PyMemAllocatorEx alloc;

    alloc.malloc = _PyMem_DebugRawMalloc;
    alloc.calloc = _PyMem_DebugRawCalloc;
    alloc.realloc = _PyMem_DebugRawRealloc;
    alloc.free = _PyMem_DebugRawFree;

    if (_PyMem_Raw.malloc != _PyMem_DebugRawMalloc) {
        alloc.ctx = &_PyMem_Debug.raw;
        PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &_PyMem_Debug.raw.alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &alloc);
    }

    alloc.malloc = _PyMem_DebugMalloc;
    alloc.calloc = _PyMem_DebugCalloc;
    alloc.realloc = _PyMem_DebugRealloc;
    alloc.free = _PyMem_DebugFree;

    if (_PyMem.malloc != _PyMem_DebugMalloc) {
        alloc.ctx = &_PyMem_Debug.mem;
        PyMem_GetAllocator(PYMEM_DOMAIN_MEM, &_PyMem_Debug.mem.alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_MEM, &alloc);
    }

    if (_PyObject.malloc != _PyMem_DebugMalloc) {
        alloc.ctx = &_PyMem_Debug.obj;
        PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, &_PyMem_Debug.obj.alloc);
        PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &alloc);
    }
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
    *allocator = _PyRuntime.obj.allocator_arenas;
}

void
PyObject_SetArenaAllocator(PyObjectArenaAllocator *allocator)
{
    _PyRuntime.obj.allocator_arenas = *allocator;
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

void
PyMem_RawFree(void *ptr)
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


char *
_PyMem_RawStrdup(const char *str)
{
    size_t size;
    char *copy;

    size = strlen(str) + 1;
    copy = PyMem_RawMalloc(size);
    if (copy == NULL)
        return NULL;
    memcpy(copy, str, size);
    return copy;
}

char *
_PyMem_Strdup(const char *str)
{
    size_t size;
    char *copy;

    size = strlen(str) + 1;
    copy = PyMem_Malloc(size);
    if (copy == NULL)
        return NULL;
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


#ifdef WITH_PYMALLOC

#ifdef WITH_VALGRIND
#include <valgrind/valgrind.h>

/* If we're using GCC, use __builtin_expect() to reduce overhead of
   the valgrind checks */
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#  define UNLIKELY(value) __builtin_expect((value), 0)
#else
#  define UNLIKELY(value) (value)
#endif

/* -1 indicates that we haven't checked that we're running on valgrind yet. */
static int running_on_valgrind = -1;
#endif


Py_ssize_t
_Py_GetAllocatedBlocks(void)
{
    return _PyRuntime.mem.num_allocated_blocks;
}


/* Allocate a new arena.  If we run out of memory, return NULL.  Else
 * allocate a new arena, and return the address of an arena_object
 * describing the new arena.  It's expected that the caller will set
 * `usable_arenas` to the return value.
 */
static struct arena_object*
new_arena(void)
{
    struct arena_object* arenaobj;
    uint excess;        /* number of bytes above pool alignment */
    void *address;
    static int debug_stats = -1;

    if (debug_stats == -1) {
        char *opt = Py_GETENV("PYTHONMALLOCSTATS");
        debug_stats = (opt != NULL && *opt != '\0');
    }
    if (debug_stats)
        _PyObject_DebugMallocStats(stderr);

    if (_PyRuntime.mem.unused_arena_objects == NULL) {
        uint i;
        uint numarenas;
        size_t nbytes;

        /* Double the number of arena objects on each allocation.
         * Note that it's possible for `numarenas` to overflow.
         */
        numarenas = _PyRuntime.mem.maxarenas ? _PyRuntime.mem.maxarenas << 1 : INITIAL_ARENA_OBJECTS;
        if (numarenas <= _PyRuntime.mem.maxarenas)
            return NULL;                /* overflow */
#if SIZEOF_SIZE_T <= SIZEOF_INT
        if (numarenas > SIZE_MAX / sizeof(*_PyRuntime.mem.arenas))
            return NULL;                /* overflow */
#endif
        nbytes = numarenas * sizeof(*_PyRuntime.mem.arenas);
        arenaobj = (struct arena_object *)PyMem_RawRealloc(_PyRuntime.mem.arenas, nbytes);
        if (arenaobj == NULL)
            return NULL;
        _PyRuntime.mem.arenas = arenaobj;

        /* We might need to fix pointers that were copied.  However,
         * new_arena only gets called when all the pages in the
         * previous arenas are full.  Thus, there are *no* pointers
         * into the old array. Thus, we don't have to worry about
         * invalid pointers.  Just to be sure, some asserts:
         */
        assert(_PyRuntime.mem.usable_arenas == NULL);
        assert(_PyRuntime.mem.unused_arena_objects == NULL);

        /* Put the new arenas on the unused_arena_objects list. */
        for (i = _PyRuntime.mem.maxarenas; i < numarenas; ++i) {
            _PyRuntime.mem.arenas[i].address = 0;              /* mark as unassociated */
            _PyRuntime.mem.arenas[i].nextarena = i < numarenas - 1 ?
                                   &_PyRuntime.mem.arenas[i+1] : NULL;
        }

        /* Update globals. */
        _PyRuntime.mem.unused_arena_objects = &_PyRuntime.mem.arenas[_PyRuntime.mem.maxarenas];
        _PyRuntime.mem.maxarenas = numarenas;
    }

    /* Take the next available arena object off the head of the list. */
    assert(_PyRuntime.mem.unused_arena_objects != NULL);
    arenaobj = _PyRuntime.mem.unused_arena_objects;
    _PyRuntime.mem.unused_arena_objects = arenaobj->nextarena;
    assert(arenaobj->address == 0);
    address = _PyRuntime.obj.allocator_arenas.alloc(_PyRuntime.obj.allocator_arenas.ctx, ARENA_SIZE);
    if (address == NULL) {
        /* The allocation failed: return NULL after putting the
         * arenaobj back.
         */
        arenaobj->nextarena = _PyRuntime.mem.unused_arena_objects;
        _PyRuntime.mem.unused_arena_objects = arenaobj;
        return NULL;
    }
    arenaobj->address = (uintptr_t)address;

    ++_PyRuntime.mem.narenas_currently_allocated;
    ++_PyRuntime.mem.ntimes_arena_allocated;
    if (_PyRuntime.mem.narenas_currently_allocated > _PyRuntime.mem.narenas_highwater)
        _PyRuntime.mem.narenas_highwater = _PyRuntime.mem.narenas_currently_allocated;
    arenaobj->freepools = NULL;
    /* pool_address <- first pool-aligned address in the arena
       nfreepools <- number of whole pools that fit after alignment */
    arenaobj->pool_address = (pyblock*)arenaobj->address;
    arenaobj->nfreepools = ARENA_SIZE / POOL_SIZE;
    assert(POOL_SIZE * arenaobj->nfreepools == ARENA_SIZE);
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

static bool ATTRIBUTE_NO_ADDRESS_SAFETY_ANALYSIS
address_in_range(void *p, poolp pool)
{
    // Since address_in_range may be reading from memory which was not allocated
    // by Python, it is important that pool->arenaindex is read only once, as
    // another thread may be concurrently modifying the value without holding
    // the GIL. The following dance forces the compiler to read pool->arenaindex
    // only once.
    uint arenaindex = *((volatile uint *)&pool->arenaindex);
    return arenaindex < _PyRuntime.mem.maxarenas &&
        (uintptr_t)p - _PyRuntime.mem.arenas[arenaindex].address < ARENA_SIZE &&
        _PyRuntime.mem.arenas[arenaindex].address != 0;
}


/*==========================================================================*/

/* pymalloc allocator

   The basic blocks are ordered by decreasing execution frequency,
   which minimizes the number of jumps in the most common cases,
   improves branching prediction and instruction scheduling (small
   block allocations typically result in a couple of instructions).
   Unless the optimizer reorders everything, being too smart...

   Return 1 if pymalloc allocated memory and wrote the pointer into *ptr_p.

   Return 0 if pymalloc failed to allocate the memory block: on bigger
   requests, on error in the code below (as a last chance to serve the request)
   or when the max memory limit has been reached. */
static int
pymalloc_alloc(void *ctx, void **ptr_p, size_t nbytes)
{
    pyblock *bp;
    poolp pool;
    poolp next;
    uint size;

#ifdef WITH_VALGRIND
    if (UNLIKELY(running_on_valgrind == -1)) {
        running_on_valgrind = RUNNING_ON_VALGRIND;
    }
    if (UNLIKELY(running_on_valgrind)) {
        return 0;
    }
#endif

    if (nbytes == 0) {
        return 0;
    }
    if (nbytes > SMALL_REQUEST_THRESHOLD) {
        return 0;
    }

    LOCK();
    /*
     * Most frequent paths first
     */
    size = (uint)(nbytes - 1) >> ALIGNMENT_SHIFT;
    pool = _PyRuntime.mem.usedpools[size + size];
    if (pool != pool->nextpool) {
        /*
         * There is a used pool for this size class.
         * Pick up the head block of its free list.
         */
        ++pool->ref.count;
        bp = pool->freeblock;
        assert(bp != NULL);
        if ((pool->freeblock = *(pyblock **)bp) != NULL) {
            goto success;
        }

        /*
         * Reached the end of the free list, try to extend it.
         */
        if (pool->nextoffset <= pool->maxnextoffset) {
            /* There is room for another block. */
            pool->freeblock = (pyblock*)pool +
                              pool->nextoffset;
            pool->nextoffset += INDEX2SIZE(size);
            *(pyblock **)(pool->freeblock) = NULL;
            goto success;
        }

        /* Pool is full, unlink from used pools. */
        next = pool->nextpool;
        pool = pool->prevpool;
        next->prevpool = pool;
        pool->nextpool = next;
        goto success;
    }

    /* There isn't a pool of the right size class immediately
     * available:  use a free pool.
     */
    if (_PyRuntime.mem.usable_arenas == NULL) {
        /* No arena has a free pool:  allocate a new arena. */
#ifdef WITH_MEMORY_LIMITS
        if (_PyRuntime.mem.narenas_currently_allocated >= MAX_ARENAS) {
            goto failed;
        }
#endif
        _PyRuntime.mem.usable_arenas = new_arena();
        if (_PyRuntime.mem.usable_arenas == NULL) {
            goto failed;
        }
        _PyRuntime.mem.usable_arenas->nextarena =
            _PyRuntime.mem.usable_arenas->prevarena = NULL;
    }
    assert(_PyRuntime.mem.usable_arenas->address != 0);

    /* Try to get a cached free pool. */
    pool = _PyRuntime.mem.usable_arenas->freepools;
    if (pool != NULL) {
        /* Unlink from cached pools. */
        _PyRuntime.mem.usable_arenas->freepools = pool->nextpool;

        /* This arena already had the smallest nfreepools
         * value, so decreasing nfreepools doesn't change
         * that, and we don't need to rearrange the
         * usable_arenas list.  However, if the arena has
         * become wholly allocated, we need to remove its
         * arena_object from usable_arenas.
         */
        --_PyRuntime.mem.usable_arenas->nfreepools;
        if (_PyRuntime.mem.usable_arenas->nfreepools == 0) {
            /* Wholly allocated:  remove. */
            assert(_PyRuntime.mem.usable_arenas->freepools == NULL);
            assert(_PyRuntime.mem.usable_arenas->nextarena == NULL ||
                   _PyRuntime.mem.usable_arenas->nextarena->prevarena ==
                   _PyRuntime.mem.usable_arenas);

            _PyRuntime.mem.usable_arenas = _PyRuntime.mem.usable_arenas->nextarena;
            if (_PyRuntime.mem.usable_arenas != NULL) {
                _PyRuntime.mem.usable_arenas->prevarena = NULL;
                assert(_PyRuntime.mem.usable_arenas->address != 0);
            }
        }
        else {
            /* nfreepools > 0:  it must be that freepools
             * isn't NULL, or that we haven't yet carved
             * off all the arena's pools for the first
             * time.
             */
            assert(_PyRuntime.mem.usable_arenas->freepools != NULL ||
                   _PyRuntime.mem.usable_arenas->pool_address <=
                   (pyblock*)_PyRuntime.mem.usable_arenas->address +
                       ARENA_SIZE - POOL_SIZE);
        }

    init_pool:
        /* Frontlink to used pools. */
        next = _PyRuntime.mem.usedpools[size + size]; /* == prev */
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
            pool->freeblock = *(pyblock **)bp;
            goto success;
        }
        /*
         * Initialize the pool header, set up the free list to
         * contain just the second block, and return the first
         * block.
         */
        pool->szidx = size;
        size = INDEX2SIZE(size);
        bp = (pyblock *)pool + POOL_OVERHEAD;
        pool->nextoffset = POOL_OVERHEAD + (size << 1);
        pool->maxnextoffset = POOL_SIZE - size;
        pool->freeblock = bp + size;
        *(pyblock **)(pool->freeblock) = NULL;
        goto success;
    }

    /* Carve off a new pool. */
    assert(_PyRuntime.mem.usable_arenas->nfreepools > 0);
    assert(_PyRuntime.mem.usable_arenas->freepools == NULL);
    pool = (poolp)_PyRuntime.mem.usable_arenas->pool_address;
    assert((pyblock*)pool <= (pyblock*)_PyRuntime.mem.usable_arenas->address +
                             ARENA_SIZE - POOL_SIZE);
    pool->arenaindex = (uint)(_PyRuntime.mem.usable_arenas - _PyRuntime.mem.arenas);
    assert(&_PyRuntime.mem.arenas[pool->arenaindex] == _PyRuntime.mem.usable_arenas);
    pool->szidx = DUMMY_SIZE_IDX;
    _PyRuntime.mem.usable_arenas->pool_address += POOL_SIZE;
    --_PyRuntime.mem.usable_arenas->nfreepools;

    if (_PyRuntime.mem.usable_arenas->nfreepools == 0) {
        assert(_PyRuntime.mem.usable_arenas->nextarena == NULL ||
               _PyRuntime.mem.usable_arenas->nextarena->prevarena ==
               _PyRuntime.mem.usable_arenas);
        /* Unlink the arena:  it is completely allocated. */
        _PyRuntime.mem.usable_arenas = _PyRuntime.mem.usable_arenas->nextarena;
        if (_PyRuntime.mem.usable_arenas != NULL) {
            _PyRuntime.mem.usable_arenas->prevarena = NULL;
            assert(_PyRuntime.mem.usable_arenas->address != 0);
        }
    }

    goto init_pool;

success:
    UNLOCK();
    assert(bp != NULL);
    *ptr_p = (void *)bp;
    return 1;

failed:
    UNLOCK();
    return 0;
}


static void *
_PyObject_Malloc(void *ctx, size_t nbytes)
{
    void* ptr;
    if (pymalloc_alloc(ctx, &ptr, nbytes)) {
        _PyRuntime.mem.num_allocated_blocks++;
        return ptr;
    }

    ptr = PyMem_RawMalloc(nbytes);
    if (ptr != NULL) {
        _PyRuntime.mem.num_allocated_blocks++;
    }
    return ptr;
}


static void *
_PyObject_Calloc(void *ctx, size_t nelem, size_t elsize)
{
    void* ptr;

    assert(elsize == 0 || nelem <= (size_t)PY_SSIZE_T_MAX / elsize);
    size_t nbytes = nelem * elsize;

    if (pymalloc_alloc(ctx, &ptr, nbytes)) {
        memset(ptr, 0, nbytes);
        _PyRuntime.mem.num_allocated_blocks++;
        return ptr;
    }

    ptr = PyMem_RawCalloc(nelem, elsize);
    if (ptr != NULL) {
        _PyRuntime.mem.num_allocated_blocks++;
    }
    return ptr;
}


/* Free a memory block allocated by pymalloc_alloc().
   Return 1 if it was freed.
   Return 0 if the block was not allocated by pymalloc_alloc(). */
static int
pymalloc_free(void *ctx, void *p)
{
    poolp pool;
    pyblock *lastfree;
    poolp next, prev;
    uint size;

    assert(p != NULL);

#ifdef WITH_VALGRIND
    if (UNLIKELY(running_on_valgrind > 0)) {
        return 0;
    }
#endif

    pool = POOL_ADDR(p);
    if (!address_in_range(p, pool)) {
        return 0;
    }
    /* We allocated this address. */

    LOCK();

    /* Link p to the start of the pool's freeblock list.  Since
     * the pool had at least the p block outstanding, the pool
     * wasn't empty (so it's already in a usedpools[] list, or
     * was full and is in no list -- it's not in the freeblocks
     * list in any case).
     */
    assert(pool->ref.count > 0);            /* else it was empty */
    *(pyblock **)p = lastfree = pool->freeblock;
    pool->freeblock = (pyblock *)p;
    if (!lastfree) {
        /* Pool was full, so doesn't currently live in any list:
         * link it to the front of the appropriate usedpools[] list.
         * This mimics LRU pool usage for new allocations and
         * targets optimal filling when several pools contain
         * blocks of the same size class.
         */
        --pool->ref.count;
        assert(pool->ref.count > 0);            /* else the pool is empty */
        size = pool->szidx;
        next = _PyRuntime.mem.usedpools[size + size];
        prev = next->prevpool;

        /* insert pool before next:   prev <-> pool <-> next */
        pool->nextpool = next;
        pool->prevpool = prev;
        next->prevpool = pool;
        prev->nextpool = pool;
        goto success;
    }

    struct arena_object* ao;
    uint nf;  /* ao->nfreepools */

    /* freeblock wasn't NULL, so the pool wasn't full,
     * and the pool is in a usedpools[] list.
     */
    if (--pool->ref.count != 0) {
        /* pool isn't empty:  leave it in usedpools */
        goto success;
    }
    /* Pool is now empty:  unlink from usedpools, and
     * link to the front of freepools.  This ensures that
     * previously freed pools will be allocated later
     * (being not referenced, they are perhaps paged out).
     */
    next = pool->nextpool;
    prev = pool->prevpool;
    next->prevpool = prev;
    prev->nextpool = next;

    /* Link the pool to freepools.  This is a singly-linked
     * list, and pool->prevpool isn't used there.
     */
    ao = &_PyRuntime.mem.arenas[pool->arenaindex];
    pool->nextpool = ao->freepools;
    ao->freepools = pool;
    nf = ++ao->nfreepools;

    /* All the rest is arena management.  We just freed
     * a pool, and there are 4 cases for arena mgmt:
     * 1. If all the pools are free, return the arena to
     *    the system free().
     * 2. If this is the only free pool in the arena,
     *    add the arena back to the `usable_arenas` list.
     * 3. If the "next" arena has a smaller count of free
     *    pools, we have to "slide this arena right" to
     *    restore that usable_arenas is sorted in order of
     *    nfreepools.
     * 4. Else there's nothing more to do.
     */
    if (nf == ao->ntotalpools) {
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
            _PyRuntime.mem.usable_arenas = ao->nextarena;
            assert(_PyRuntime.mem.usable_arenas == NULL ||
                   _PyRuntime.mem.usable_arenas->address != 0);
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
        ao->nextarena = _PyRuntime.mem.unused_arena_objects;
        _PyRuntime.mem.unused_arena_objects = ao;

        /* Free the entire arena. */
        _PyRuntime.obj.allocator_arenas.free(_PyRuntime.obj.allocator_arenas.ctx,
                             (void *)ao->address, ARENA_SIZE);
        ao->address = 0;                        /* mark unassociated */
        --_PyRuntime.mem.narenas_currently_allocated;

        goto success;
    }

    if (nf == 1) {
        /* Case 2.  Put ao at the head of
         * usable_arenas.  Note that because
         * ao->nfreepools was 0 before, ao isn't
         * currently on the usable_arenas list.
         */
        ao->nextarena = _PyRuntime.mem.usable_arenas;
        ao->prevarena = NULL;
        if (_PyRuntime.mem.usable_arenas)
            _PyRuntime.mem.usable_arenas->prevarena = ao;
        _PyRuntime.mem.usable_arenas = ao;
        assert(_PyRuntime.mem.usable_arenas->address != 0);

        goto success;
    }

    /* If this arena is now out of order, we need to keep
     * the list sorted.  The list is kept sorted so that
     * the "most full" arenas are used first, which allows
     * the nearly empty arenas to be completely freed.  In
     * a few un-scientific tests, it seems like this
     * approach allowed a lot more memory to be freed.
     */
    if (ao->nextarena == NULL ||
                 nf <= ao->nextarena->nfreepools) {
        /* Case 4.  Nothing to do. */
        goto success;
    }
    /* Case 3:  We have to move the arena towards the end
     * of the list, because it has more free pools than
     * the arena to its right.
     * First unlink ao from usable_arenas.
     */
    if (ao->prevarena != NULL) {
        /* ao isn't at the head of the list */
        assert(ao->prevarena->nextarena == ao);
        ao->prevarena->nextarena = ao->nextarena;
    }
    else {
        /* ao is at the head of the list */
        assert(_PyRuntime.mem.usable_arenas == ao);
        _PyRuntime.mem.usable_arenas = ao->nextarena;
    }
    ao->nextarena->prevarena = ao->prevarena;

    /* Locate the new insertion point by iterating over
     * the list, using our nextarena pointer.
     */
    while (ao->nextarena != NULL && nf > ao->nextarena->nfreepools) {
        ao->prevarena = ao->nextarena;
        ao->nextarena = ao->nextarena->nextarena;
    }

    /* Insert ao at this point. */
    assert(ao->nextarena == NULL || ao->prevarena == ao->nextarena->prevarena);
    assert(ao->prevarena->nextarena == ao->nextarena);

    ao->prevarena->nextarena = ao;
    if (ao->nextarena != NULL) {
        ao->nextarena->prevarena = ao;
    }

    /* Verify that the swaps worked. */
    assert(ao->nextarena == NULL || nf <= ao->nextarena->nfreepools);
    assert(ao->prevarena == NULL || nf > ao->prevarena->nfreepools);
    assert(ao->nextarena == NULL || ao->nextarena->prevarena == ao);
    assert((_PyRuntime.mem.usable_arenas == ao && ao->prevarena == NULL)
           || ao->prevarena->nextarena == ao);

    goto success;

success:
    UNLOCK();
    return 1;
}


static void
_PyObject_Free(void *ctx, void *p)
{
    /* PyObject_Free(NULL) has no effect */
    if (p == NULL) {
        return;
    }

    _PyRuntime.mem.num_allocated_blocks--;
    if (!pymalloc_free(ctx, p)) {
        /* pymalloc didn't allocate this address */
        PyMem_RawFree(p);
    }
}


/* pymalloc realloc.

   If nbytes==0, then as the Python docs promise, we do not treat this like
   free(p), and return a non-NULL result.

   Return 1 if pymalloc reallocated memory and wrote the new pointer into
   newptr_p.

   Return 0 if pymalloc didn't allocated p. */
static int
pymalloc_realloc(void *ctx, void **newptr_p, void *p, size_t nbytes)
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
    if (!address_in_range(p, pool)) {
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

    if (pymalloc_realloc(ctx, &ptr2, ptr, nbytes)) {
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

/* Special bytes broadcast into debug memory blocks at appropriate times.
 * Strings of these are unlikely to be valid addresses, floats, ints or
 * 7-bit ASCII.
 */
#undef CLEANBYTE
#undef DEADBYTE
#undef FORBIDDENBYTE
#define CLEANBYTE      0xCB    /* clean (newly allocated) memory */
#define DEADBYTE       0xDB    /* dead (newly freed) memory */
#define FORBIDDENBYTE  0xFB    /* untouchable bytes at each end of a block */

/* serialno is always incremented via calling this routine.  The point is
 * to supply a single place to set a breakpoint.
 */
static void
bumpserialno(void)
{
    ++_PyRuntime.mem.serialno;
}

#define SST SIZEOF_SIZE_T

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

/* Let S = sizeof(size_t).  The debug malloc asks for 4*S extra bytes and
   fills them with useful stuff, here calling the underlying malloc's result p:

p[0: S]
    Number of bytes originally asked for.  This is a size_t, big-endian (easier
    to read in a memory dump).
p[S]
    API ID.  See PEP 445.  This is a character, but seems undocumented.
p[S+1: 2*S]
    Copies of FORBIDDENBYTE.  Used to catch under- writes and reads.
p[2*S: 2*S+n]
    The requested memory, filled with copies of CLEANBYTE.
    Used to catch reference to uninitialized memory.
    &p[2*S] is returned.  Note that this is 8-byte aligned if pymalloc
    handled the request itself.
p[2*S+n: 2*S+n+S]
    Copies of FORBIDDENBYTE.  Used to catch over- writes and reads.
p[2*S+n+S: 2*S+n+2*S]
    A serial number, incremented by 1 on each call to _PyMem_DebugMalloc
    and _PyMem_DebugRealloc.
    This is a big-endian size_t.
    If "bad memory" is detected later, the serial number gives an
    excellent way to set a breakpoint on the next run, to capture the
    instant at which this block was passed out.
*/

static void *
_PyMem_DebugRawAlloc(int use_calloc, void *ctx, size_t nbytes)
{
    debug_alloc_api_t *api = (debug_alloc_api_t *)ctx;
    uint8_t *p;           /* base address of malloc'ed pad block */
    uint8_t *data;        /* p + 2*SST == pointer to data bytes */
    uint8_t *tail;        /* data + nbytes == pointer to tail pad bytes */
    size_t total;         /* 2 * SST + nbytes + 2 * SST */

    if (nbytes > (size_t)PY_SSIZE_T_MAX - 4 * SST) {
        /* integer overflow: can't represent total as a Py_ssize_t */
        return NULL;
    }
    total = nbytes + 4 * SST;

    /* Layout: [SSSS IFFF CCCC...CCCC FFFF NNNN]
     *          ^--- p    ^--- data   ^--- tail
       S: nbytes stored as size_t
       I: API identifier (1 byte)
       F: Forbidden bytes (size_t - 1 bytes before, size_t bytes after)
       C: Clean bytes used later to store actual data
       N: Serial number stored as size_t */

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

    bumpserialno();

    /* at p, write size (SST bytes), id (1 byte), pad (SST-1 bytes) */
    write_size_t(p, nbytes);
    p[SST] = (uint8_t)api->api_id;
    memset(p + SST + 1, FORBIDDENBYTE, SST-1);

    if (nbytes > 0 && !use_calloc) {
        memset(data, CLEANBYTE, nbytes);
    }

    /* at tail, write pad (SST bytes) and serialno (SST bytes) */
    tail = data + nbytes;
    memset(tail, FORBIDDENBYTE, SST);
    write_size_t(tail + SST, _PyRuntime.mem.serialno);

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
   Then fills the original bytes with DEADBYTE.
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

    _PyMem_DebugCheckAddress(api->api_id, p);
    nbytes = read_size_t(q);
    nbytes += 4 * SST;
    memset(q, DEADBYTE, nbytes);
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
    size_t serialno;
#define ERASED_SIZE 64
    uint8_t save[2*ERASED_SIZE];  /* A copy of erased bytes. */

    _PyMem_DebugCheckAddress(api->api_id, p);

    data = (uint8_t *)p;
    head = data - 2*SST;
    original_nbytes = read_size_t(head);
    if (nbytes > (size_t)PY_SSIZE_T_MAX - 4*SST) {
        /* integer overflow: can't represent total as a Py_ssize_t */
        return NULL;
    }
    total = nbytes + 4*SST;

    tail = data + original_nbytes;
    serialno = read_size_t(tail + SST);
    /* Mark the header, the trailer, ERASED_SIZE bytes at the begin and
       ERASED_SIZE bytes at the end as dead and save the copy of erased bytes.
     */
    if (original_nbytes <= sizeof(save)) {
        memcpy(save, data, original_nbytes);
        memset(data - 2*SST, DEADBYTE, original_nbytes + 4*SST);
    }
    else {
        memcpy(save, data, ERASED_SIZE);
        memset(head, DEADBYTE, ERASED_SIZE + 2*SST);
        memcpy(&save[ERASED_SIZE], tail - ERASED_SIZE, ERASED_SIZE);
        memset(tail - ERASED_SIZE, DEADBYTE, ERASED_SIZE + 2*SST);
    }

    /* Resize and add decorations. */
    r = (uint8_t *)api->alloc.realloc(api->alloc.ctx, head, total);
    if (r == NULL) {
        nbytes = original_nbytes;
    }
    else {
        head = r;
        bumpserialno();
        serialno = _PyRuntime.mem.serialno;
    }

    write_size_t(head, nbytes);
    head[SST] = (uint8_t)api->api_id;
    memset(head + SST + 1, FORBIDDENBYTE, SST-1);
    data = head + 2*SST;

    tail = data + nbytes;
    memset(tail, FORBIDDENBYTE, SST);
    write_size_t(tail + SST, serialno);

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
        /* growing:  mark new extra memory clean */
        memset(data + original_nbytes, CLEANBYTE, nbytes - original_nbytes);
    }

    return data;
}

static void
_PyMem_DebugCheckGIL(void)
{
    if (!PyGILState_Check())
        Py_FatalError("Python memory allocator called "
                      "without holding the GIL");
}

static void *
_PyMem_DebugMalloc(void *ctx, size_t nbytes)
{
    _PyMem_DebugCheckGIL();
    return _PyMem_DebugRawMalloc(ctx, nbytes);
}

static void *
_PyMem_DebugCalloc(void *ctx, size_t nelem, size_t elsize)
{
    _PyMem_DebugCheckGIL();
    return _PyMem_DebugRawCalloc(ctx, nelem, elsize);
}


static void
_PyMem_DebugFree(void *ctx, void *ptr)
{
    _PyMem_DebugCheckGIL();
    _PyMem_DebugRawFree(ctx, ptr);
}


static void *
_PyMem_DebugRealloc(void *ctx, void *ptr, size_t nbytes)
{
    _PyMem_DebugCheckGIL();
    return _PyMem_DebugRawRealloc(ctx, ptr, nbytes);
}

/* Check the forbidden bytes on both ends of the memory allocated for p.
 * If anything is wrong, print info to stderr via _PyObject_DebugDumpAddress,
 * and call Py_FatalError to kill the program.
 * The API id, is also checked.
 */
static void
_PyMem_DebugCheckAddress(char api, const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    char msgbuf[64];
    const char *msg;
    size_t nbytes;
    const uint8_t *tail;
    int i;
    char id;

    if (p == NULL) {
        msg = "didn't expect a NULL pointer";
        goto error;
    }

    /* Check the API id */
    id = (char)q[-SST];
    if (id != api) {
        msg = msgbuf;
        snprintf(msgbuf, sizeof(msgbuf), "bad ID: Allocated using API '%c', verified using API '%c'", id, api);
        msgbuf[sizeof(msgbuf)-1] = 0;
        goto error;
    }

    /* Check the stuff at the start of p first:  if there's underwrite
     * corruption, the number-of-bytes field may be nuts, and checking
     * the tail could lead to a segfault then.
     */
    for (i = SST-1; i >= 1; --i) {
        if (*(q-i) != FORBIDDENBYTE) {
            msg = "bad leading pad byte";
            goto error;
        }
    }

    nbytes = read_size_t(q - 2*SST);
    tail = q + nbytes;
    for (i = 0; i < SST; ++i) {
        if (tail[i] != FORBIDDENBYTE) {
            msg = "bad trailing pad byte";
            goto error;
        }
    }

    return;

error:
    _PyObject_DebugDumpAddress(p);
    Py_FatalError(msg);
}

/* Display info to stderr about the memory block at p. */
static void
_PyObject_DebugDumpAddress(const void *p)
{
    const uint8_t *q = (const uint8_t *)p;
    const uint8_t *tail;
    size_t nbytes, serial;
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
    fprintf(stderr, "    %" PY_FORMAT_SIZE_T "u bytes originally "
                    "requested\n", nbytes);

    /* In case this is nuts, check the leading pad bytes first. */
    fprintf(stderr, "    The %d pad bytes at p-%d are ", SST-1, SST-1);
    ok = 1;
    for (i = 1; i <= SST-1; ++i) {
        if (*(q-i) != FORBIDDENBYTE) {
            ok = 0;
            break;
        }
    }
    if (ok)
        fputs("FORBIDDENBYTE, as expected.\n", stderr);
    else {
        fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
            FORBIDDENBYTE);
        for (i = SST-1; i >= 1; --i) {
            const uint8_t byte = *(q-i);
            fprintf(stderr, "        at p-%d: 0x%02x", i, byte);
            if (byte != FORBIDDENBYTE)
                fputs(" *** OUCH", stderr);
            fputc('\n', stderr);
        }

        fputs("    Because memory is corrupted at the start, the "
              "count of bytes requested\n"
              "       may be bogus, and checking the trailing pad "
              "bytes may segfault.\n", stderr);
    }

    tail = q + nbytes;
    fprintf(stderr, "    The %d pad bytes at tail=%p are ", SST, tail);
    ok = 1;
    for (i = 0; i < SST; ++i) {
        if (tail[i] != FORBIDDENBYTE) {
            ok = 0;
            break;
        }
    }
    if (ok)
        fputs("FORBIDDENBYTE, as expected.\n", stderr);
    else {
        fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
                FORBIDDENBYTE);
        for (i = 0; i < SST; ++i) {
            const uint8_t byte = tail[i];
            fprintf(stderr, "        at tail+%d: 0x%02x",
                    i, byte);
            if (byte != FORBIDDENBYTE)
                fputs(" *** OUCH", stderr);
            fputc('\n', stderr);
        }
    }

    serial = read_size_t(tail + SST);
    fprintf(stderr, "    The block was made by call #%" PY_FORMAT_SIZE_T
                    "u to debug malloc/realloc.\n", serial);

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
                  "%d %ss * %" PY_FORMAT_SIZE_T "d bytes each",
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
 */
void
_PyObject_DebugMallocStats(FILE *out)
{
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
    for (i = 0; i < _PyRuntime.mem.maxarenas; ++i) {
        uint j;
        uintptr_t base = _PyRuntime.mem.arenas[i].address;

        /* Skip arenas which are not allocated. */
        if (_PyRuntime.mem.arenas[i].address == (uintptr_t)NULL)
            continue;
        narenas += 1;

        numfreepools += _PyRuntime.mem.arenas[i].nfreepools;

        /* round up to pool alignment */
        if (base & (uintptr_t)POOL_SIZE_MASK) {
            arena_alignment += POOL_SIZE;
            base &= ~(uintptr_t)POOL_SIZE_MASK;
            base += POOL_SIZE;
        }

        /* visit every pool in the arena */
        assert(base <= (uintptr_t) _PyRuntime.mem.arenas[i].pool_address);
        for (j = 0; base < (uintptr_t) _PyRuntime.mem.arenas[i].pool_address;
             ++j, base += POOL_SIZE) {
            poolp p = (poolp)base;
            const uint sz = p->szidx;
            uint freeblocks;

            if (p->ref.count == 0) {
                /* currently unused */
#ifdef Py_DEBUG
                assert(pool_is_in_list(p, _PyRuntime.mem.arenas[i].freepools));
#endif
                continue;
            }
            ++numpools[sz];
            numblocks[sz] += p->ref.count;
            freeblocks = NUMBLOCKS(sz) - p->ref.count;
            numfreeblocks[sz] += freeblocks;
#ifdef Py_DEBUG
            if (freeblocks > 0)
                assert(pool_is_in_list(p, _PyRuntime.mem.usedpools[sz + sz]));
#endif
        }
    }
    assert(narenas == _PyRuntime.mem.narenas_currently_allocated);

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
        fprintf(out, "%5u %6u "
                        "%11" PY_FORMAT_SIZE_T "u "
                        "%15" PY_FORMAT_SIZE_T "u "
                        "%13" PY_FORMAT_SIZE_T "u\n",
                i, size, p, b, f);
        allocated_bytes += b * size;
        available_bytes += f * size;
        pool_header_bytes += p * POOL_OVERHEAD;
        quantization += p * ((POOL_SIZE - POOL_OVERHEAD) % size);
    }
    fputc('\n', out);
    if (_PyMem_DebugEnabled())
        (void)printone(out, "# times object malloc called", _PyRuntime.mem.serialno);
    (void)printone(out, "# arenas allocated total", _PyRuntime.mem.ntimes_arena_allocated);
    (void)printone(out, "# arenas reclaimed", _PyRuntime.mem.ntimes_arena_allocated - narenas);
    (void)printone(out, "# arenas highwater mark", _PyRuntime.mem.narenas_highwater);
    (void)printone(out, "# arenas allocated current", narenas);

    PyOS_snprintf(buf, sizeof(buf),
        "%" PY_FORMAT_SIZE_T "u arenas * %d bytes/arena",
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
}

#endif /* #ifdef WITH_PYMALLOC */
