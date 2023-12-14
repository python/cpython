#ifndef Py_INTERNAL_OBMALLOC_H
#define Py_INTERNAL_OBMALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


typedef unsigned int pymem_uint;  /* assuming >= 16 bits */

#undef  uint
#define uint pymem_uint


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

/*==========================================================================*/

/*
 * -- Main tunable settings section --
 */

/*
 * Alignment of addresses returned to the user. 8-bytes alignment works
 * on most current architectures (with 32-bit or 64-bit address buses).
 * The alignment value is also used for grouping small requests in size
 * classes spaced ALIGNMENT bytes apart.
 *
 * You shouldn't change this unless you know what you are doing.
 */

#if SIZEOF_VOID_P > 4
#define ALIGNMENT              16               /* must be 2^N */
#define ALIGNMENT_SHIFT         4
#else
#define ALIGNMENT               8               /* must be 2^N */
#define ALIGNMENT_SHIFT         3
#endif

/* Return the number of bytes in size class I, as a uint. */
#define INDEX2SIZE(I) (((pymem_uint)(I) + 1) << ALIGNMENT_SHIFT)

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

/*
 * Maximum amount of memory managed by the allocator for small requests.
 */
#ifdef WITH_MEMORY_LIMITS
#ifndef SMALL_MEMORY_LIMIT
#define SMALL_MEMORY_LIMIT      (64 * 1024 * 1024)      /* 64 MB -- more? */
#endif
#endif

#if !defined(WITH_PYMALLOC_RADIX_TREE)
/* Use radix-tree to track arena memory regions, for address_in_range().
 * Enable by default since it allows larger pool sizes.  Can be disabled
 * using -DWITH_PYMALLOC_RADIX_TREE=0 */
#define WITH_PYMALLOC_RADIX_TREE 1
#endif

#if SIZEOF_VOID_P > 4
/* on 64-bit platforms use larger pools and arenas if we can */
#define USE_LARGE_ARENAS
#if WITH_PYMALLOC_RADIX_TREE
/* large pools only supported if radix-tree is enabled */
#define USE_LARGE_POOLS
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
#ifdef USE_LARGE_ARENAS
#define ARENA_BITS              20                    /* 1 MiB */
#else
#define ARENA_BITS              18                    /* 256 KiB */
#endif
#define ARENA_SIZE              (1 << ARENA_BITS)
#define ARENA_SIZE_MASK         (ARENA_SIZE - 1)

#ifdef WITH_MEMORY_LIMITS
#define MAX_ARENAS              (SMALL_MEMORY_LIMIT / ARENA_SIZE)
#endif

/*
 * Size of the pools used for small blocks.  Must be a power of 2.
 */
#ifdef USE_LARGE_POOLS
#define POOL_BITS               14                  /* 16 KiB */
#else
#define POOL_BITS               12                  /* 4 KiB */
#endif
#define POOL_SIZE               (1 << POOL_BITS)
#define POOL_SIZE_MASK          (POOL_SIZE - 1)

#if !WITH_PYMALLOC_RADIX_TREE
#if POOL_SIZE != SYSTEM_PAGE_SIZE
#   error "pool size must be equal to system page size"
#endif
#endif

#define MAX_POOLS_IN_ARENA  (ARENA_SIZE / POOL_SIZE)
#if MAX_POOLS_IN_ARENA * POOL_SIZE != ARENA_SIZE
#   error "arena size not an exact multiple of pool size"
#endif

/*
 * -- End of tunable settings section --
 */

/*==========================================================================*/

/* When you say memory, my mind reasons in terms of (pointers to) blocks */
typedef uint8_t pymem_block;

/* Pool for small blocks. */
struct pool_header {
    union { pymem_block *_padding;
            uint count; } ref;          /* number of allocated blocks    */
    pymem_block *freeblock;             /* pool's free list head         */
    struct pool_header *nextpool;       /* next pool of this size class  */
    struct pool_header *prevpool;       /* previous pool       ""        */
    uint arenaindex;                    /* index into arenas of base adr */
    uint szidx;                         /* block size class index        */
    uint nextoffset;                    /* bytes to virgin block         */
    uint maxnextoffset;                 /* largest valid nextoffset      */
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
    pymem_block* pool_address;

    /* The number of available pools in the arena:  free pools + never-
     * allocated pools.
     */
    uint nfreepools;

    /* The total number of pools in the arena, whether or not available. */
    uint ntotalpools;

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
#define NUMBLOCKS(I) ((pymem_uint)(POOL_SIZE - POOL_OVERHEAD) / INDEX2SIZE(I))

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
nextpool and prevpool members.  The "- 2*sizeof(pymem_block *)" gibberish is
compensating for that a pool_header's nextpool and prevpool members
immediately follow a pool_header's first two members:

    union { pymem_block *_padding;
            uint count; } ref;
    pymem_block *freeblock;

each of which consume sizeof(pymem_block *) bytes.  So what usedpools[i+i] really
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

#define OBMALLOC_USED_POOLS_SIZE (2 * ((NB_SMALL_SIZE_CLASSES + 7) / 8) * 8)

struct _obmalloc_pools {
    poolp used[OBMALLOC_USED_POOLS_SIZE];
};


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

struct _obmalloc_mgmt {
    /* Array of objects used to track chunks of memory (arenas). */
    struct arena_object* arenas;
    /* Number of slots currently allocated in the `arenas` vector. */
    uint maxarenas;

    /* The head of the singly-linked, NULL-terminated list of available
     * arena_objects.
     */
    struct arena_object* unused_arena_objects;

    /* The head of the doubly-linked, NULL-terminated at each end, list of
     * arena_objects associated with arenas that have pools available.
     */
    struct arena_object* usable_arenas;

    /* nfp2lasta[nfp] is the last arena in usable_arenas with nfp free pools */
    struct arena_object* nfp2lasta[MAX_POOLS_IN_ARENA + 1];

    /* Number of arenas allocated that haven't been free()'d. */
    size_t narenas_currently_allocated;

    /* Total number of times malloc() called to allocate an arena. */
    size_t ntimes_arena_allocated;
    /* High water mark (max value ever seen) for narenas_currently_allocated. */
    size_t narenas_highwater;

    Py_ssize_t raw_allocated_blocks;
};


#if WITH_PYMALLOC_RADIX_TREE
/*==========================================================================*/
/* radix tree for tracking arena usage.  If enabled, used to implement
   address_in_range().

   memory address bit allocation for keys

   64-bit pointers, IGNORE_BITS=0 and 2^20 arena size:
     15 -> MAP_TOP_BITS
     15 -> MAP_MID_BITS
     14 -> MAP_BOT_BITS
     20 -> ideal aligned arena
   ----
     64

   64-bit pointers, IGNORE_BITS=16, and 2^20 arena size:
     16 -> IGNORE_BITS
     10 -> MAP_TOP_BITS
     10 -> MAP_MID_BITS
      8 -> MAP_BOT_BITS
     20 -> ideal aligned arena
   ----
     64

   32-bit pointers and 2^18 arena size:
     14 -> MAP_BOT_BITS
     18 -> ideal aligned arena
   ----
     32

*/

#if SIZEOF_VOID_P == 8

/* number of bits in a pointer */
#define POINTER_BITS 64

/* High bits of memory addresses that will be ignored when indexing into the
 * radix tree.  Setting this to zero is the safe default.  For most 64-bit
 * machines, setting this to 16 would be safe.  The kernel would not give
 * user-space virtual memory addresses that have significant information in
 * those high bits.  The main advantage to setting IGNORE_BITS > 0 is that less
 * virtual memory will be used for the top and middle radix tree arrays.  Those
 * arrays are allocated in the BSS segment and so will typically consume real
 * memory only if actually accessed.
 */
#define IGNORE_BITS 0

/* use the top and mid layers of the radix tree */
#define USE_INTERIOR_NODES

#elif SIZEOF_VOID_P == 4

#define POINTER_BITS 32
#define IGNORE_BITS 0

#else

 /* Currently this code works for 64-bit or 32-bit pointers only.  */
#error "obmalloc radix tree requires 64-bit or 32-bit pointers."

#endif /* SIZEOF_VOID_P */

/* arena_coverage_t members require this to be true  */
#if ARENA_BITS >= 32
#   error "arena size must be < 2^32"
#endif

/* the lower bits of the address that are not ignored */
#define ADDRESS_BITS (POINTER_BITS - IGNORE_BITS)

#ifdef USE_INTERIOR_NODES
/* number of bits used for MAP_TOP and MAP_MID nodes */
#define INTERIOR_BITS ((ADDRESS_BITS - ARENA_BITS + 2) / 3)
#else
#define INTERIOR_BITS 0
#endif

#define MAP_TOP_BITS INTERIOR_BITS
#define MAP_TOP_LENGTH (1 << MAP_TOP_BITS)
#define MAP_TOP_MASK (MAP_TOP_LENGTH - 1)

#define MAP_MID_BITS INTERIOR_BITS
#define MAP_MID_LENGTH (1 << MAP_MID_BITS)
#define MAP_MID_MASK (MAP_MID_LENGTH - 1)

#define MAP_BOT_BITS (ADDRESS_BITS - ARENA_BITS - 2*INTERIOR_BITS)
#define MAP_BOT_LENGTH (1 << MAP_BOT_BITS)
#define MAP_BOT_MASK (MAP_BOT_LENGTH - 1)

#define MAP_BOT_SHIFT ARENA_BITS
#define MAP_MID_SHIFT (MAP_BOT_BITS + MAP_BOT_SHIFT)
#define MAP_TOP_SHIFT (MAP_MID_BITS + MAP_MID_SHIFT)

#define AS_UINT(p) ((uintptr_t)(p))
#define MAP_BOT_INDEX(p) ((AS_UINT(p) >> MAP_BOT_SHIFT) & MAP_BOT_MASK)
#define MAP_MID_INDEX(p) ((AS_UINT(p) >> MAP_MID_SHIFT) & MAP_MID_MASK)
#define MAP_TOP_INDEX(p) ((AS_UINT(p) >> MAP_TOP_SHIFT) & MAP_TOP_MASK)

#if IGNORE_BITS > 0
/* Return the ignored part of the pointer address.  Those bits should be same
 * for all valid pointers if IGNORE_BITS is set correctly.
 */
#define HIGH_BITS(p) (AS_UINT(p) >> ADDRESS_BITS)
#else
#define HIGH_BITS(p) 0
#endif


/* This is the leaf of the radix tree.  See arena_map_mark_used() for the
 * meaning of these members. */
typedef struct {
    int32_t tail_hi;
    int32_t tail_lo;
} arena_coverage_t;

typedef struct arena_map_bot {
    /* The members tail_hi and tail_lo are accessed together.  So, it
     * better to have them as an array of structs, rather than two
     * arrays.
     */
    arena_coverage_t arenas[MAP_BOT_LENGTH];
} arena_map_bot_t;

#ifdef USE_INTERIOR_NODES
typedef struct arena_map_mid {
    struct arena_map_bot *ptrs[MAP_MID_LENGTH];
} arena_map_mid_t;

typedef struct arena_map_top {
    struct arena_map_mid *ptrs[MAP_TOP_LENGTH];
} arena_map_top_t;
#endif

struct _obmalloc_usage {
    /* The root of radix tree.  Note that by initializing like this, the memory
     * should be in the BSS.  The OS will only memory map pages as the MAP_MID
     * nodes get used (OS pages are demand loaded as needed).
     */
#ifdef USE_INTERIOR_NODES
    arena_map_top_t arena_map_root;
    /* accounting for number of used interior nodes */
    int arena_map_mid_count;
    int arena_map_bot_count;
#else
    arena_map_bot_t arena_map_root;
#endif
};

#endif /* WITH_PYMALLOC_RADIX_TREE */


struct _obmalloc_global_state {
    int dump_debug_stats;
    Py_ssize_t interpreter_leaks;
};

struct _obmalloc_state {
    struct _obmalloc_pools pools;
    struct _obmalloc_mgmt mgmt;
#if WITH_PYMALLOC_RADIX_TREE
    struct _obmalloc_usage usage;
#endif
};


#undef  uint


/* Allocate memory directly from the O/S virtual memory system,
 * where supported. Otherwise fallback on malloc */
void *_PyObject_VirtualAlloc(size_t size);
void _PyObject_VirtualFree(void *, size_t size);


/* This function returns the number of allocated memory blocks, regardless of size */
extern Py_ssize_t _Py_GetGlobalAllocatedBlocks(void);
#define _Py_GetAllocatedBlocks() \
    _Py_GetGlobalAllocatedBlocks()
extern Py_ssize_t _PyInterpreterState_GetAllocatedBlocks(PyInterpreterState *);
extern void _PyInterpreterState_FinalizeAllocatedBlocks(PyInterpreterState *);


#ifdef WITH_PYMALLOC
// Export the symbol for the 3rd party 'guppy3' project
PyAPI_FUNC(int) _PyObject_DebugMallocStats(FILE *out);
#endif


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBMALLOC_H
