#include <Python.h>
#include <ffi.h>
#ifdef MS_WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
# if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif
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

static ITEM *free_list;
static int _pagesize;

static void more_core(void)
{
    ITEM *item;
    int count, i;

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

    /* calculate the number of nodes to allocate */
    count = BLOCKSIZE / sizeof(ITEM);

    /* allocate a memory block */
#ifdef MS_WIN32
    item = (ITEM *)VirtualAlloc(NULL,
                                           count * sizeof(ITEM),
                                           MEM_COMMIT,
                                           PAGE_EXECUTE_READWRITE);
    if (item == NULL)
        return;
#else
    item = (ITEM *)mmap(NULL,
                        count * sizeof(ITEM),
                        PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANONYMOUS,
                        -1,
                        0);
    if (item == (void *)MAP_FAILED)
        return;
#endif

#ifdef MALLOC_CLOSURE_DEBUG
    printf("block at %p allocated (%d bytes), %d ITEMs\n",
           item, count * sizeof(ITEM), count);
#endif
    /* put them into the free list */
    for (i = 0; i < count; ++i) {
        item->next = free_list;
        free_list = item;
        ++item;
    }
}

/******************************************************************/

/* put the item back into the free list */
void ffi_closure_free(void *p)
{
    ITEM *item = (ITEM *)p;
    item->next = free_list;
    free_list = item;
}

/* return one item from the free list, allocating more if needed */
void *ffi_closure_alloc(size_t ignored, void** codeloc)
{
    ITEM *item;
    if (!free_list)
        more_core();
    if (!free_list)
        return NULL;
    item = free_list;
    free_list = item->next;
    *codeloc = (void *)item;
    return (void *)item;
}
