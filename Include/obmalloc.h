#ifdef WITH_PYMALLOC
#ifndef Py_OBMALLOC_H
#define Py_OBMALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pyport.h"

/*==========================================================================*/

/*
 * -- Main tunable settings section --
 */

/*
 * Alignment of addresses returned to the user. 8-bytes alignment works
 * on most current architectures (with 32-bit or 64-bit address busses).
 * The alignment value is also used for grouping small requests in size
 * classes spaced ALIGNMENT bytes apart.
 *
 * You shouldn't change this unless you know what you are doing.
 */

#if SIZEOF_VOID_P > 4
#define PYMALLOC_ALIGNMENT              16               /* must be 2^N */
#define PYMALLOC_ALIGNMENT_SHIFT         4
#else
#define PYMALLOC_ALIGNMENT               8               /* must be 2^N */
#define PYMALLOC_ALIGNMENT_SHIFT         3
#endif

/* Return the number of bytes in size class I, as a uint. */
#define PYMALLOC_INDEX2SIZE(I) (((uint)(I) + 1) << PYMALLOC_ALIGNMENT_SHIFT)

/*
 * Max size threshold below which malloc requests are considered to be
 * small enough in order to use preallocated memory pools. You can tune
 * this value according to your application behaviour and memory needs.
 *
 * Note: a size threshold of 512 guarantees that newly created dictionaries
 * will be allocated from preallocated memory pools on 64-bit.
 *
 * The following invariants must hold:
 *      1) ALIGNMENT <= SMALL_REQUEST_THRESHOLD <= 512
 *      2) SMALL_REQUEST_THRESHOLD is evenly divisible by ALIGNMENT
 *
 * Although not required, for better performance and space efficiency,
 * it is recommended that SMALL_REQUEST_THRESHOLD is set to a power of 2.
 */
#define PYMALLOC_SMALL_REQUEST_THRESHOLD 512
#define PYMALLOC_NB_SMALL_SIZE_CLASSES   (PYMALLOC_SMALL_REQUEST_THRESHOLD / PYMALLOC_ALIGNMENT)

/*
 * The system's VMM page size can be obtained on most unices with a
 * getpagesize() call or deduced from various header files. To make
 * things simpler, we assume that it is 4K, which is OK for most systems.
 * It is probably better if this is the native page size, but it doesn't
 * have to be.  In theory, if SYSTEM_PAGE_SIZE is larger than the native page
 * size, then `POOL_ADDR(p)->arenaindex' could rarely cause a segmentation
 * violation fault.  4K is apparently OK for all the platforms that python
 * currently targets.
 */
#define PYMALLOC_SYSTEM_PAGE_SIZE        (4 * 1024)
#define PYMALLOC_SYSTEM_PAGE_SIZE_MASK   (PYMALLOC_SYSTEM_PAGE_SIZE - 1)

/*
 * Maximum amount of memory managed by the allocator for small requests.
 */
#ifdef WITH_MEMORY_LIMITS
#ifndef PYMALLOC_SMALL_MEMORY_LIMIT
#define PYMALLOC_SMALL_MEMORY_LIMIT      (64 * 1024 * 1024)      /* 64 MB -- more? */
#endif
#endif

/*
 * The allocator sub-allocates <Big> blocks of memory (called arenas) aligned
 * on a page boundary. This is a reserved virtual address space for the
 * current process (obtained through a malloc()/mmap() call). In no way this
 * means that the memory arenas will be used entirely. A malloc(<Big>) is
 * usually an address range reservation for <Big> bytes, unless all pages within
 * this space are referenced subsequently. So malloc'ing big blocks and not
 * using them does not mean "wasting memory". It's an addressable range
 * wastage...
 *
 * Arenas are allocated with mmap() on systems supporting anonymous memory
 * mappings to reduce heap fragmentation.
 */
#define PYMALLOC_ARENA_SIZE              (256 << 10)     /* 256KB */

#ifdef WITH_MEMORY_LIMITS
#define PYMALLOC_MAX_ARENAS              (PYMALLOC_SMALL_MEMORY_LIMIT / PYMALLOC_ARENA_SIZE)
#endif

/*
 * Size of the pools used for small blocks. Should be a power of 2,
 * between 1K and SYSTEM_PAGE_SIZE, that is: 1k, 2k, 4k.
 */
#define PYMALLOC_POOL_SIZE               PYMALLOC_SYSTEM_PAGE_SIZE        /* must be 2^N */
#define PYMALLOC_POOL_SIZE_MASK          PYMALLOC_SYSTEM_PAGE_SIZE_MASK

#define PYMALLOC_MAX_POOLS_IN_ARENA  (PYMALLOC_ARENA_SIZE / PYMALLOC_POOL_SIZE)
#if PYMALLOC_MAX_POOLS_IN_ARENA * PYMALLOC_POOL_SIZE != PYMALLOC_ARENA_SIZE
#   error "arena size not an exact multiple of pool size"
#endif

/*
 * -- End of tunable settings section --
 */

/*==========================================================================*/


typedef unsigned int _obmalloc_uint;

/* When you say memory, my mind reasons in terms of (pointers to) blocks */
typedef uint8_t _obmalloc_memory_block;

/* Pool for small blocks. */
struct _obmalloc_pool_header {
    union { _obmalloc_memory_block *_padding;
        _obmalloc_uint count; } ref;          /* number of allocated blocks    */
    _obmalloc_memory_block *freeblock;                   /* pool's free list head         */
    struct _obmalloc_pool_header *nextpool;       /* next pool of this size class  */
    struct _obmalloc_pool_header *prevpool;       /* previous pool       ""        */
    _obmalloc_uint arenaindex;                    /* index into arenas of base adr */
    _obmalloc_uint szidx;                         /* block size class index        */
    _obmalloc_uint nextoffset;                    /* bytes to virgin block         */
    _obmalloc_uint maxnextoffset;                 /* largest valid nextoffset      */
};


/* Record keeping for arenas. */
struct _obmalloc_arena_object {
    /* The address of the arena, as returned by malloc.  Note that 0
     * will never be returned by a successful malloc, and is used
     * here to mark an _obmalloc_arena_object that doesn't correspond to an
     * allocated arena.
     */
    uintptr_t address;

    /* Pool-aligned pointer to the next pool to be carved off. */
    _obmalloc_memory_block* pool_address;

    /* The number of available pools in the arena:  free pools + never-
     * allocated pools.
     */
    _obmalloc_uint nfreepools;

    /* The total number of pools in the arena, whether or not available. */
    _obmalloc_uint ntotalpools;

    /* Singly-linked list of available pools. */
    struct _obmalloc_pool_header* freepools;

    /* Whenever this _obmalloc_arena_object is not associated with an allocated
     * arena, the nextarena member is used to link all unassociated
     * _obmalloc_arena_objects in the singly-linked `unused__obmalloc_arena_objects` list.
     * The prevarena member is unused in this case.
     *
     * When this _obmalloc_arena_object is associated with an allocated arena
     * with at least one available pool, both members are used in the
     * doubly-linked `usable_arenas` list, which is maintained in
     * increasing order of `nfreepools` values.
     *
     * Else this _obmalloc_arena_object is associated with an allocated arena
     * all of whose pools are in use.  `nextarena` and `prevarena`
     * are both meaningless in this case.
     */
    struct _obmalloc_arena_object* nextarena;
    struct _obmalloc_arena_object* prevarena;
};


#ifdef __cplusplus
}
#endif

#endif /* !Py_OBMALLOC_H */
#endif /* WITH_PYMALLOC */
