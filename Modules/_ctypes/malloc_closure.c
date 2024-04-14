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

typedef malloc_closure_item ITEM;
typedef malloc_closure_arena ARENA;

static void
more_core(malloc_closure_state *st)
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

void
clear_malloc_closure_free_list(ctypes_state *state)
{
    malloc_closure_state *st = &state->malloc_closure;
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
Py_ffi_closure_free(ctypes_state *state, void *p)
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
    malloc_closure_state *st = &state->malloc_closure;
    if (st->narenas <= 0) {
        return;
    }
    ITEM *item = (ITEM *)p;
    item->next = st->free_list;
    st->free_list = item;
}

/* return one item from the free list, allocating more if needed */
void *
Py_ffi_closure_alloc(ctypes_state *state, size_t size, void** codeloc)
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
    malloc_closure_state *st = &state->malloc_closure;
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
