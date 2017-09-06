
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

#ifndef Py_INTERNAL_PYMALLOC_H
#define Py_INTERNAL_PYMALLOC_H

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
#define ALIGNMENT               8               /* must be 2^N */
#define ALIGNMENT_SHIFT         3

/* Return the number of bytes in size class I, as a uint. */
#define INDEX2SIZE(I) (((unsigned int)(I) + 1) << ALIGNMENT_SHIFT)

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
#define SMALL_REQUEST_THRESHOLD 512
#define NB_SMALL_SIZE_CLASSES   (SMALL_REQUEST_THRESHOLD / ALIGNMENT)

#if NB_SMALL_SIZE_CLASSES > 64
#error "NB_SMALL_SIZE_CLASSES should be less than 64"
#endif /* NB_SMALL_SIZE_CLASSES > 64 */

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
#define SYSTEM_PAGE_SIZE        (4 * 1024)
#define SYSTEM_PAGE_SIZE_MASK   (SYSTEM_PAGE_SIZE - 1)

/*
 * Maximum amount of memory managed by the allocator for small requests.
 */
#ifdef WITH_MEMORY_LIMITS
#ifndef SMALL_MEMORY_LIMIT
#define SMALL_MEMORY_LIMIT      (64 * 1024 * 1024)      /* 64 MB -- more? */
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
#define ARENA_SIZE              (256 << 10)     /* 256KB */

#ifdef WITH_MEMORY_LIMITS
#define MAX_ARENAS              (SMALL_MEMORY_LIMIT / ARENA_SIZE)
#endif

/*
 * Size of the pools used for small blocks. Should be a power of 2,
 * between 1K and SYSTEM_PAGE_SIZE, that is: 1k, 2k, 4k.
 */
#define POOL_SIZE               SYSTEM_PAGE_SIZE        /* must be 2^N */
#define POOL_SIZE_MASK          SYSTEM_PAGE_SIZE_MASK

/*
 * -- End of tunable settings section --
 */

/*==========================================================================*/

/*
 * Locking
 *
 * To reduce lock contention, it would probably be better to refine the
 * crude function locking with per size class locking. I'm not positive
 * however, whether it's worth switching to such locking policy because
 * of the performance penalty it might introduce.
 *
 * The following macros describe the simplest (should also be the fastest)
 * lock object on a particular platform and the init/fini/lock/unlock
 * operations on it. The locks defined here are not expected to be recursive
 * because it is assumed that they will always be called in the order:
 * INIT, [LOCK, UNLOCK]*, FINI.
 */

/*
 * Python's threads are serialized, so object malloc locking is disabled.
 */
#define SIMPLELOCK_DECL(lock)   /* simple lock declaration              */
#define SIMPLELOCK_INIT(lock)   /* allocate (if needed) and initialize  */
#define SIMPLELOCK_FINI(lock)   /* free/destroy an existing lock        */
#define SIMPLELOCK_LOCK(lock)   /* acquire released lock */
#define SIMPLELOCK_UNLOCK(lock) /* release acquired lock */

/* When you say memory, my mind reasons in terms of (pointers to) blocks */
typedef uint8_t pyblock;

/* Pool for small blocks. */
struct pool_header {
    union { pyblock *_padding;
            unsigned int count; } ref;  /* number of allocated blocks    */
    pyblock *freeblock;                 /* pool's free list head         */
    struct pool_header *nextpool;       /* next pool of this size class  */
    struct pool_header *prevpool;       /* previous pool       ""        */
    unsigned int arenaindex;            /* index into arenas of base adr */
    unsigned int szidx;                 /* block size class index        */
    unsigned int nextoffset;            /* bytes to virgin block         */
    unsigned int maxnextoffset;         /* largest valid nextoffset      */
};

typedef struct pool_header *poolp;

/* Record keeping for arenas. */
struct arena_object {
    /* The address of the arena, as returned by malloc.  Note that 0
     * will never be returned by a successful malloc, and is used
     * here to mark an arena_object that doesn't correspond to an
     * allocated arena.
     */
    uintptr_t address;

    /* Pool-aligned pointer to the next pool to be carved off. */
    pyblock* pool_address;

    /* The number of available pools in the arena:  free pools + never-
     * allocated pools.
     */
    unsigned int nfreepools;

    /* The total number of pools in the arena, whether or not available. */
    unsigned int ntotalpools;

    /* Singly-linked list of available pools. */
    struct pool_header* freepools;

    /* Whenever this arena_object is not associated with an allocated
     * arena, the nextarena member is used to link all unassociated
     * arena_objects in the singly-linked `unused_arena_objects` list.
     * The prevarena member is unused in this case.
     *
     * When this arena_object is associated with an allocated arena
     * with at least one available pool, both members are used in the
     * doubly-linked `usable_arenas` list, which is maintained in
     * increasing order of `nfreepools` values.
     *
     * Else this arena_object is associated with an allocated arena
     * all of whose pools are in use.  `nextarena` and `prevarena`
     * are both meaningless in this case.
     */
    struct arena_object* nextarena;
    struct arena_object* prevarena;
};

#define POOL_OVERHEAD   _Py_SIZE_ROUND_UP(sizeof(struct pool_header), ALIGNMENT)

#define DUMMY_SIZE_IDX          0xffff  /* size class of newly cached pools */

/* Round pointer P down to the closest pool-aligned address <= P, as a poolp */
#define POOL_ADDR(P) ((poolp)_Py_ALIGN_DOWN((P), POOL_SIZE))

/* Return total number of blocks in pool of size index I, as a uint. */
#define NUMBLOCKS(I) \
    ((unsigned int)(POOL_SIZE - POOL_OVERHEAD) / INDEX2SIZE(I))

/*==========================================================================*/

/*
 * This malloc lock
 */
SIMPLELOCK_DECL(_malloc_lock)
#define LOCK()          SIMPLELOCK_LOCK(_malloc_lock)
#define UNLOCK()        SIMPLELOCK_UNLOCK(_malloc_lock)
#define LOCK_INIT()     SIMPLELOCK_INIT(_malloc_lock)
#define LOCK_FINI()     SIMPLELOCK_FINI(_malloc_lock)

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

#define MAX_POOLS  (2 * ((NB_SMALL_SIZE_CLASSES + 7) / 8) * 8)

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
*/

/* How many arena_objects do we initially allocate?
 * 16 = can allocate 16 arenas = 16 * ARENA_SIZE = 4MB before growing the
 * `arenas` vector.
 */
#define INITIAL_ARENA_OBJECTS 16

#endif /* Py_INTERNAL_PYMALLOC_H */
