#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include <Python.h>
#include <ffi.h>
#ifdef MS_WIN32
#  include <windows.h>
#else
#  include <sys/mman.h>
#  include <unistd.h>             // sysconf()
#  if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#    define MAP_ANONYMOUS MAP_ANON
#  endif
#endif
#include "ctypes.h"

/* BLOCKSIZE can be adjusted.  Larger blocksize will take a larger memory
   overhead, but allocate less blocks from the system.  It may be that some
   systems have a limit of how many mmap'd blocks can be open.
*/

#define BLOCKSIZE _pagesize

/* #define MALLOC_CLOSURE_DEBUG */ /* enable for some debugging output */


/******************************************************************/

typedef union _tagITEM {
    ffi_closure closure;
    union _tagITEM *next;
} ITEM;

typedef struct _arena {
    struct _arena *prev_arena;
    ITEM items[1];
} ARENA;

typedef struct {
    int pagesize;
    ITEM *free_list;
    ARENA *last_arena;
    Py_ssize_t arena_size;
    Py_ssize_t narenas;
} thunk_type_state;

static inline thunk_type_state *
get_type_state(PyTypeObject *type)
{
    void *state = PyObject_GetTypeData((PyObject *)type, Py_TYPE(type));
    assert(state != NULL);
    return (thunk_type_state *)state;
}

static void
more_core(thunk_type_state *st)
{
    ITEM *item;
    int count, i;
    int _pagesize = st->pagesize;

/* determine the pagesize */
#ifdef MS_WIN32
    if (!_pagesize) {
        SYSTEM_INFO systeminfo;
        GetSystemInfo(&systeminfo);
        _pagesize = systeminfo.dwPageSize;
    }
#else
    if (!_pagesize) {
#ifdef _SC_PAGESIZE
        _pagesize = sysconf(_SC_PAGESIZE);
#else
        _pagesize = getpagesize();
#endif
    }
#endif
    st->pagesize = _pagesize;

    /* calculate the number of nodes to allocate */
    count = (BLOCKSIZE - sizeof(ARENA *)) / sizeof(ITEM);
    if (count <= 0) {
        return;
    }
    st->arena_size = sizeof(ARENA *) + count * sizeof(ITEM);

    /* allocate a memory block */
#ifdef MS_WIN32
    ARENA *arena = (ARENA *)VirtualAlloc(NULL,
                                           st->arena_size,
                                           MEM_COMMIT,
                                           PAGE_EXECUTE_READWRITE);
    if (arena == NULL)
        return;
#else
    ARENA *arena = (ARENA *)mmap(NULL,
                        st->arena_size,
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0);
    if (arena == (void *)MAP_FAILED)
        return;
#endif

    arena->prev_arena = st->last_arena;
    st->last_arena = arena;
    ++st->narenas;
    item = arena->items;

#ifdef MALLOC_CLOSURE_DEBUG
    printf("block at %p allocated (%d bytes), %d ITEMs\n",
           item, count * (int)sizeof(ITEM), count);
#endif
    /* put them into the free list */
    for (i = 0; i < count; ++i) {
        item->next = st->free_list;
        st->free_list = item;
        ++item;
    }
}

static inline void
clear_free_list(thunk_type_state *st)
{
    while (st->narenas > 0) {
        ARENA *arena = st->last_arena;
        assert(arena != NULL);
        st->last_arena = arena->prev_arena;
#ifdef MS_WIN32
        VirtualFree(arena, 0, MEM_RELEASE);
#else
        munmap(arena, st->arena_size);
#endif
        st->narenas--;
    }
    assert(st->last_arena == NULL);
}

/******************************************************************/

/* put the item back into the free list */
void
Py_ffi_closure_free(PyTypeObject *thunk_tp, void *p)
{
#ifdef HAVE_FFI_CLOSURE_ALLOC
#ifdef USING_APPLE_OS_LIBFFI
# ifdef HAVE_BUILTIN_AVAILABLE
    if (__builtin_available(macos 10.15, ios 13, watchos 6, tvos 13, *)) {
#  else
    if (ffi_closure_free != NULL) {
#  endif
#endif
        ffi_closure_free(p);
        return;
#ifdef USING_APPLE_OS_LIBFFI
    }
#endif
#endif
    thunk_type_state *st = get_type_state(thunk_tp);

    ITEM *item = (ITEM *)p;
    item->next = st->free_list;
    st->free_list = item;
}

/* return one item from the free list, allocating more if needed */
void *
Py_ffi_closure_alloc(PyTypeObject *thunk_tp, size_t size, void** codeloc)
{
#ifdef HAVE_FFI_CLOSURE_ALLOC
#ifdef USING_APPLE_OS_LIBFFI
# ifdef HAVE_BUILTIN_AVAILABLE
    if (__builtin_available(macos 10.15, ios 13, watchos 6, tvos 13, *)) {
# else
    if (ffi_closure_alloc != NULL) {
#  endif
#endif
        return ffi_closure_alloc(size, codeloc);
#ifdef USING_APPLE_OS_LIBFFI
    }
#endif
#endif
    ITEM *item;
    thunk_type_state *st = get_type_state(thunk_tp);
    if (!st->free_list) {
        more_core(st);
    }
    if (!st->free_list) {
        return NULL;
    }
    item = st->free_list;
    st->free_list = item->next;
#ifdef _M_ARM
    // set Thumb bit so that blx is called correctly
    *codeloc = (ITEM*)((uintptr_t)item | 1);
#else
    *codeloc = (void *)item;
#endif
    return (void *)item;
}


/******************************************************************
 * PyCThunkType_Type - a metaclass of PyCThunk_Type.
 */

static int
CThunkType_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return PyType_Type.tp_traverse(self, visit, arg);
}

static int
CThunkType_clear(PyObject *self)
{
    return PyType_Type.tp_clear(self);
}

static void
CThunkType_dealloc(PyObject *self)
{
    PyTypeObject *type = Py_TYPE(self);
    thunk_type_state *st = get_type_state((PyTypeObject *)self);
    clear_free_list(st);
    memset(st, 0, sizeof(thunk_type_state));
    PyType_Type.tp_dealloc(self);  // delegate GC-untrack and free
    Py_DECREF(type);
}

static PyObject *
CThunkType_get_narenas(PyTypeObject *self, void *Py_UNUSED(ignored))
{
    thunk_type_state *st = get_type_state(self);
    return PyLong_FromSsize_t(st->narenas);
}

static PyGetSetDef CThunkType_getsets[] = {
    {"ffi_closure_containers_count", (getter)CThunkType_get_narenas,
        NULL, NULL, NULL},
    {0}
};

static PyType_Slot cthunk_type_slots[] = {
    {Py_tp_getset, CThunkType_getsets},
    {Py_tp_traverse, CThunkType_traverse},
    {Py_tp_clear, CThunkType_clear},
    {Py_tp_dealloc, CThunkType_dealloc},
    {0, NULL},
};

PyType_Spec cthunk_type_spec = {
    .name = "_ctypes.CThunkType",
    .basicsize = -(Py_ssize_t)sizeof(thunk_type_state),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_DISALLOW_INSTANTIATION |
              Py_TPFLAGS_BASETYPE),
    .slots = cthunk_type_slots,
};
