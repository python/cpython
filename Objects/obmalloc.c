#include "Python.h"

#ifdef WITH_PYMALLOC

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

/* #undef WITH_MEMORY_LIMITS */		/* disable mem limit checks  */

/*==========================================================================*/

/*
 * Allocation strategy abstract:
 *
 * For small requests, the allocator sub-allocates <Big> blocks of memory.
 * Requests greater than 256 bytes are routed to the system's allocator.
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
 * Request in bytes	Size of allocated block      Size class idx
 * ----------------------------------------------------------------
 *        1-8                     8                       0
 *	  9-16                   16                       1
 *	 17-24                   24                       2
 *	 25-32                   32                       3
 *	 33-40                   40                       4
 *	 41-48                   48                       5
 *	 49-56                   56                       6
 *	 57-64                   64                       7
 *	 65-72                   72                       8
 *	  ...                   ...                     ...
 *	241-248                 248                      30
 *	249-256                 256                      31
 *
 *	0, 257 and up: routed to the underlying allocator.
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
#define ALIGNMENT		8		/* must be 2^N */
#define ALIGNMENT_SHIFT		3
#define ALIGNMENT_MASK		(ALIGNMENT - 1)

/* Return the number of bytes in size class I, as a uint. */
#define INDEX2SIZE(I) (((uint)(I) + 1) << ALIGNMENT_SHIFT)

/*
 * Max size threshold below which malloc requests are considered to be
 * small enough in order to use preallocated memory pools. You can tune
 * this value according to your application behaviour and memory needs.
 *
 * The following invariants must hold:
 *	1) ALIGNMENT <= SMALL_REQUEST_THRESHOLD <= 256
 *	2) SMALL_REQUEST_THRESHOLD is evenly divisible by ALIGNMENT
 *
 * Although not required, for better performance and space efficiency,
 * it is recommended that SMALL_REQUEST_THRESHOLD is set to a power of 2.
 */
#define SMALL_REQUEST_THRESHOLD	256
#define NB_SMALL_SIZE_CLASSES	(SMALL_REQUEST_THRESHOLD / ALIGNMENT)

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
#define SYSTEM_PAGE_SIZE	(4 * 1024)
#define SYSTEM_PAGE_SIZE_MASK	(SYSTEM_PAGE_SIZE - 1)

/*
 * Maximum amount of memory managed by the allocator for small requests.
 */
#ifdef WITH_MEMORY_LIMITS
#ifndef SMALL_MEMORY_LIMIT
#define SMALL_MEMORY_LIMIT	(64 * 1024 * 1024)	/* 64 MB -- more? */
#endif
#endif

/*
 * The allocator sub-allocates <Big> blocks of memory (called arenas) aligned
 * on a page boundary. This is a reserved virtual address space for the
 * current process (obtained through a malloc call). In no way this means
 * that the memory arenas will be used entirely. A malloc(<Big>) is usually
 * an address range reservation for <Big> bytes, unless all pages within this
 * space are referenced subsequently. So malloc'ing big blocks and not using
 * them does not mean "wasting memory". It's an addressable range wastage...
 *
 * Therefore, allocating arenas with malloc is not optimal, because there is
 * some address space wastage, but this is the most portable way to request
 * memory from the system across various platforms.
 */
#define ARENA_SIZE		(256 << 10)	/* 256KB */

#ifdef WITH_MEMORY_LIMITS
#define MAX_ARENAS		(SMALL_MEMORY_LIMIT / ARENA_SIZE)
#endif

/*
 * Size of the pools used for small blocks. Should be a power of 2,
 * between 1K and SYSTEM_PAGE_SIZE, that is: 1k, 2k, 4k.
 */
#define POOL_SIZE		SYSTEM_PAGE_SIZE	/* must be 2^N */
#define POOL_SIZE_MASK		SYSTEM_PAGE_SIZE_MASK

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
#define SIMPLELOCK_DECL(lock)	/* simple lock declaration		*/
#define SIMPLELOCK_INIT(lock)	/* allocate (if needed) and initialize	*/
#define SIMPLELOCK_FINI(lock)	/* free/destroy an existing lock 	*/
#define SIMPLELOCK_LOCK(lock)	/* acquire released lock */
#define SIMPLELOCK_UNLOCK(lock)	/* release acquired lock */

/*
 * Basic types
 * I don't care if these are defined in <sys/types.h> or elsewhere. Axiom.
 */
#undef  uchar
#define uchar			unsigned char	/* assuming == 8 bits  */

#undef  uint
#define uint			unsigned int	/* assuming >= 16 bits */

#undef  ulong
#define ulong			unsigned long	/* assuming >= 32 bits */

#undef uptr
#define uptr			Py_uintptr_t

/* When you say memory, my mind reasons in terms of (pointers to) blocks */
typedef uchar block;

/* Pool for small blocks. */
struct pool_header {
	union { block *_padding;
		uint count; } ref;	/* number of allocated blocks    */
	block *freeblock;		/* pool's free list head         */
	struct pool_header *nextpool;	/* next pool of this size class  */
	struct pool_header *prevpool;	/* previous pool       ""        */
	uint arenaindex;		/* index into arenas of base adr */
	uint szidx;			/* block size class index	 */
	uint nextoffset;		/* bytes to virgin block	 */
	uint maxnextoffset;		/* largest valid nextoffset	 */
};

typedef struct pool_header *poolp;

#undef  ROUNDUP
#define ROUNDUP(x)		(((x) + ALIGNMENT_MASK) & ~ALIGNMENT_MASK)
#define POOL_OVERHEAD		ROUNDUP(sizeof(struct pool_header))

#define DUMMY_SIZE_IDX		0xffff	/* size class of newly cached pools */

/* Round pointer P down to the closest pool-aligned address <= P, as a poolp */
#define POOL_ADDR(P) ((poolp)((uptr)(P) & ~(uptr)POOL_SIZE_MASK))

/* Return total number of blocks in pool of size index I, as a uint. */
#define NUMBLOCKS(I) ((uint)(POOL_SIZE - POOL_OVERHEAD) / INDEX2SIZE(I))

/*==========================================================================*/

/*
 * This malloc lock
 */
SIMPLELOCK_DECL(_malloc_lock)
#define LOCK()		SIMPLELOCK_LOCK(_malloc_lock)
#define UNLOCK()	SIMPLELOCK_UNLOCK(_malloc_lock)
#define LOCK_INIT()	SIMPLELOCK_INIT(_malloc_lock)
#define LOCK_FINI()	SIMPLELOCK_FINI(_malloc_lock)

/*
 * Pool table -- headed, circular, doubly-linked lists of partially used pools.

This is involved.  For an index i, usedpools[i+i] is the header for a list of
all partially used pools holding small blocks with "size class idx" i. So
usedpools[0] corresponds to blocks of size 8, usedpools[2] to blocks of size
16, and so on:  index 2*i <-> blocks of size (i+1)<<ALIGNMENT_SHIFT.

Pools are carved off the current arena highwater mark (file static arenabase)
as needed.  Once carved off, a pool is in one of three states forever after:

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
    and linked to the front of the (file static) singly-linked freepools list,
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

#define PTA(x)	((poolp )((uchar *)&(usedpools[2*(x)]) - 2*sizeof(block *)))
#define PT(x)	PTA(x), PTA(x)

static poolp usedpools[2 * ((NB_SMALL_SIZE_CLASSES + 7) / 8) * 8] = {
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
#endif /* NB_SMALL_SIZE_CLASSES > 56 */
#endif /* NB_SMALL_SIZE_CLASSES > 48 */
#endif /* NB_SMALL_SIZE_CLASSES > 40 */
#endif /* NB_SMALL_SIZE_CLASSES > 32 */
#endif /* NB_SMALL_SIZE_CLASSES > 24 */
#endif /* NB_SMALL_SIZE_CLASSES > 16 */
#endif /* NB_SMALL_SIZE_CLASSES >  8 */
};

/*
 * Free (cached) pools
 */
static poolp freepools = NULL;		/* free list for cached pools */

/*==========================================================================*/
/* Arena management. */

/* arenas is a vector of arena base addresses, in order of allocation time.
 * arenas currently contains narenas entries, and has space allocated
 * for at most maxarenas entries.
 *
 * CAUTION:  See the long comment block about thread safety in new_arena():
 * the code currently relies in deep ways on that this vector only grows,
 * and only grows by appending at the end.  For now we never return an arena
 * to the OS.
 */
static uptr *volatile arenas = NULL;	/* the pointer itself is volatile */
static volatile uint narenas = 0;
static uint maxarenas = 0;

/* Number of pools still available to be allocated in the current arena. */
static uint nfreepools = 0;

/* Free space start address in current arena.  This is pool-aligned. */
static block *arenabase = NULL;

/* Allocate a new arena and return its base address.  If we run out of
 * memory, return NULL.
 */
static block *
new_arena(void)
{
	uint excess;	/* number of bytes above pool alignment */
	block *bp = (block *)malloc(ARENA_SIZE);
	if (bp == NULL)
		return NULL;

#ifdef PYMALLOC_DEBUG
	if (Py_GETENV("PYTHONMALLOCSTATS"))
		_PyObject_DebugMallocStats();
#endif

	/* arenabase <- first pool-aligned address in the arena
	   nfreepools <- number of whole pools that fit after alignment */
	arenabase = bp;
	nfreepools = ARENA_SIZE / POOL_SIZE;
	assert(POOL_SIZE * nfreepools == ARENA_SIZE);
	excess = (uint) ((Py_uintptr_t)bp & POOL_SIZE_MASK);
	if (excess != 0) {
		--nfreepools;
		arenabase += POOL_SIZE - excess;
	}

	/* Make room for a new entry in the arenas vector. */
	if (arenas == NULL) {
		assert(narenas == 0 && maxarenas == 0);
		arenas = (uptr *)malloc(16 * sizeof(*arenas));
		if (arenas == NULL)
			goto error;
		maxarenas = 16;
	}
	else if (narenas == maxarenas) {
		/* Grow arenas.
		 *
		 * Exceedingly subtle:  Someone may be calling the pymalloc
		 * free via PyMem_{DEL, Del, FREE, Free} without holding the
		 *.GIL.  Someone else may simultaneously be calling the
		 * pymalloc malloc while holding the GIL via, e.g.,
		 * PyObject_New.  Now the pymalloc free may index into arenas
		 * for an address check, while the pymalloc malloc calls
		 * new_arena and we end up here to grow a new arena *and*
		 * grow the arenas vector.  If the value for arenas pymalloc
		 * free picks up "vanishes" during this resize, anything may
		 * happen, and it would be an incredibly rare bug.  Therefore
		 * the code here takes great pains to make sure that, at every
		 * moment, arenas always points to an intact vector of
		 * addresses.  It doesn't matter whether arenas points to a
		 * wholly up-to-date vector when pymalloc free checks it in
		 * this case, because the only legal (and that even this is
		 * legal is debatable) way to call PyMem_{Del, etc} while not
		 * holding the GIL is if the memory being released is not
		 * object memory, i.e. if the address check in pymalloc free
		 * is supposed to fail.  Having an incomplete vector can't
		 * make a supposed-to-fail case succeed by mistake (it could
		 * only make a supposed-to-succeed case fail by mistake).
		 *
		 * In addition, without a lock we can't know for sure when
		 * an old vector is no longer referenced, so we simply let
		 * old vectors leak.
		 *
		 * And on top of that, since narenas and arenas can't be
		 * changed as-a-pair atomically without a lock, we're also
		 * careful to declare them volatile and ensure that we change
		 * arenas first.  This prevents another thread from picking
		 * up an narenas value too large for the arenas value it
		 * reads up (arenas never shrinks).
		 *
		 * Read the above 50 times before changing anything in this
		 * block.
		 */
		uptr *p;
		uint newmax = maxarenas << 1;
		if (newmax <= maxarenas)	/* overflow */
			goto error;
		p = (uptr *)malloc(newmax * sizeof(*arenas));
		if (p == NULL)
			goto error;
		memcpy(p, arenas, narenas * sizeof(*arenas));
		arenas = p;	/* old arenas deliberately leaked */
		maxarenas = newmax;
	}

	/* Append the new arena address to arenas. */
	assert(narenas < maxarenas);
	arenas[narenas] = (uptr)bp;
	++narenas;	/* can't overflow, since narenas < maxarenas before */
	return bp;

error:
	free(bp);
	nfreepools = 0;
	return NULL;
}

/* Return true if and only if P is an address that was allocated by
 * pymalloc.  I must be the index into arenas that the address claims
 * to come from.
 *
 * Tricky:  Letting B be the arena base address in arenas[I], P belongs to the
 * arena if and only if
 *	B <= P < B + ARENA_SIZE
 * Subtracting B throughout, this is true iff
 *	0 <= P-B < ARENA_SIZE
 * By using unsigned arithmetic, the "0 <=" half of the test can be skipped.
 *
 * Obscure:  A PyMem "free memory" function can call the pymalloc free or
 * realloc before the first arena has been allocated.  arenas is still
 * NULL in that case.  We're relying on that narenas is also 0 in that case,
 * so the (I) < narenas must be false, saving us from trying to index into
 * a NULL arenas.
 */
#define Py_ADDRESS_IN_RANGE(P, POOL)	\
	((POOL)->arenaindex < narenas &&		\
	 (uptr)(P) - arenas[(POOL)->arenaindex] < (uptr)ARENA_SIZE)

/* This is only useful when running memory debuggers such as
 * Purify or Valgrind.  Uncomment to use.
 *
#define Py_USING_MEMORY_DEBUGGER
 */

#ifdef Py_USING_MEMORY_DEBUGGER

/* Py_ADDRESS_IN_RANGE may access uninitialized memory by design
 * This leads to thousands of spurious warnings when using
 * Purify or Valgrind.  By making a function, we can easily
 * suppress the uninitialized memory reads in this one function.
 * So we won't ignore real errors elsewhere.
 *
 * Disable the macro and use a function.
 */

#undef Py_ADDRESS_IN_RANGE

/* Don't make static, to ensure this isn't inlined. */
int Py_ADDRESS_IN_RANGE(void *P, poolp pool);
#endif

/*==========================================================================*/

/* malloc.  Note that nbytes==0 tries to return a non-NULL pointer, distinct
 * from all other currently live pointers.  This may not be possible.
 */

/*
 * The basic blocks are ordered by decreasing execution frequency,
 * which minimizes the number of jumps in the most common cases,
 * improves branching prediction and instruction scheduling (small
 * block allocations typically result in a couple of instructions).
 * Unless the optimizer reorders everything, being too smart...
 */

#undef PyObject_Malloc
void *
PyObject_Malloc(size_t nbytes)
{
	block *bp;
	poolp pool;
	poolp next;
	uint size;

	/*
	 * This implicitly redirects malloc(0).
	 */
	if ((nbytes - 1) < SMALL_REQUEST_THRESHOLD) {
		LOCK();
		/*
		 * Most frequent paths first
		 */
		size = (uint )(nbytes - 1) >> ALIGNMENT_SHIFT;
		pool = usedpools[size + size];
		if (pool != pool->nextpool) {
			/*
			 * There is a used pool for this size class.
			 * Pick up the head block of its free list.
			 */
			++pool->ref.count;
			bp = pool->freeblock;
			assert(bp != NULL);
			if ((pool->freeblock = *(block **)bp) != NULL) {
				UNLOCK();
				return (void *)bp;
			}
			/*
			 * Reached the end of the free list, try to extend it
			 */
			if (pool->nextoffset <= pool->maxnextoffset) {
				/*
				 * There is room for another block
				 */
				pool->freeblock = (block *)pool +
						  pool->nextoffset;
				pool->nextoffset += INDEX2SIZE(size);
				*(block **)(pool->freeblock) = NULL;
				UNLOCK();
				return (void *)bp;
			}
			/*
			 * Pool is full, unlink from used pools
			 */
			next = pool->nextpool;
			pool = pool->prevpool;
			next->prevpool = pool;
			pool->nextpool = next;
			UNLOCK();
			return (void *)bp;
		}
		/*
		 * Try to get a cached free pool
		 */
		pool = freepools;
		if (pool != NULL) {
			/*
			 * Unlink from cached pools
			 */
			freepools = pool->nextpool;
		init_pool:
			/*
			 * Frontlink to used pools
			 */
			next = usedpools[size + size]; /* == prev */
			pool->nextpool = next;
			pool->prevpool = next;
			next->nextpool = pool;
			next->prevpool = pool;
			pool->ref.count = 1;
			if (pool->szidx == size) {
				/*
				 * Luckily, this pool last contained blocks
				 * of the same size class, so its header
				 * and free list are already initialized.
				 */
				bp = pool->freeblock;
				pool->freeblock = *(block **)bp;
				UNLOCK();
				return (void *)bp;
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
			UNLOCK();
			return (void *)bp;
		}
		/*
		 * Allocate new pool
		 */
		if (nfreepools) {
		commit_pool:
			--nfreepools;
			pool = (poolp)arenabase;
			arenabase += POOL_SIZE;
			pool->arenaindex = narenas - 1;
			pool->szidx = DUMMY_SIZE_IDX;
			goto init_pool;
		}
		/*
		 * Allocate new arena
		 */
#ifdef WITH_MEMORY_LIMITS
		if (!(narenas < MAX_ARENAS)) {
			UNLOCK();
			goto redirect;
		}
#endif
		bp = new_arena();
		if (bp != NULL)
			goto commit_pool;
		UNLOCK();
		goto redirect;
	}

        /* The small block allocator ends here. */

redirect:
	/*
	 * Redirect the original request to the underlying (libc) allocator.
	 * We jump here on bigger requests, on error in the code above (as a
	 * last chance to serve the request) or when the max memory limit
	 * has been reached.
	 */
	if (nbytes == 0)
		nbytes = 1;
	return (void *)malloc(nbytes);
}

/* free */

#undef PyObject_Free
void
PyObject_Free(void *p)
{
	poolp pool;
	block *lastfree;
	poolp next, prev;
	uint size;

	if (p == NULL)	/* free(NULL) has no effect */
		return;

	pool = POOL_ADDR(p);
	if (Py_ADDRESS_IN_RANGE(p, pool)) {
		/* We allocated this address. */
		LOCK();
		/*
		 * Link p to the start of the pool's freeblock list.  Since
		 * the pool had at least the p block outstanding, the pool
		 * wasn't empty (so it's already in a usedpools[] list, or
		 * was full and is in no list -- it's not in the freeblocks
		 * list in any case).
		 */
		assert(pool->ref.count > 0);	/* else it was empty */
		*(block **)p = lastfree = pool->freeblock;
		pool->freeblock = (block *)p;
		if (lastfree) {
			/*
			 * freeblock wasn't NULL, so the pool wasn't full,
			 * and the pool is in a usedpools[] list.
			 */
			if (--pool->ref.count != 0) {
				/* pool isn't empty:  leave it in usedpools */
				UNLOCK();
				return;
			}
			/*
			 * Pool is now empty:  unlink from usedpools, and
			 * link to the front of freepools.  This ensures that
			 * previously freed pools will be allocated later
			 * (being not referenced, they are perhaps paged out).
			 */
			next = pool->nextpool;
			prev = pool->prevpool;
			next->prevpool = prev;
			prev->nextpool = next;
			/* Link to freepools.  This is a singly-linked list,
			 * and pool->prevpool isn't used there.
			 */
			pool->nextpool = freepools;
			freepools = pool;
			UNLOCK();
			return;
		}
		/*
		 * Pool was full, so doesn't currently live in any list:
		 * link it to the front of the appropriate usedpools[] list.
		 * This mimics LRU pool usage for new allocations and
		 * targets optimal filling when several pools contain
		 * blocks of the same size class.
		 */
		--pool->ref.count;
		assert(pool->ref.count > 0);	/* else the pool is empty */
		size = pool->szidx;
		next = usedpools[size + size];
		prev = next->prevpool;
		/* insert pool before next:   prev <-> pool <-> next */
		pool->nextpool = next;
		pool->prevpool = prev;
		next->prevpool = pool;
		prev->nextpool = pool;
		UNLOCK();
		return;
	}

	/* We didn't allocate this address. */
	free(p);
}

/* realloc.  If p is NULL, this acts like malloc(nbytes).  Else if nbytes==0,
 * then as the Python docs promise, we do not treat this like free(p), and
 * return a non-NULL result.
 */

#undef PyObject_Realloc
void *
PyObject_Realloc(void *p, size_t nbytes)
{
	void *bp;
	poolp pool;
	uint size;

	if (p == NULL)
		return PyObject_Malloc(nbytes);

	pool = POOL_ADDR(p);
	if (Py_ADDRESS_IN_RANGE(p, pool)) {
		/* We're in charge of this block */
		size = INDEX2SIZE(pool->szidx);
		if (nbytes <= size) {
			/* The block is staying the same or shrinking.  If
			 * it's shrinking, there's a tradeoff:  it costs
			 * cycles to copy the block to a smaller size class,
			 * but it wastes memory not to copy it.  The
			 * compromise here is to copy on shrink only if at
			 * least 25% of size can be shaved off.
			 */
			if (4 * nbytes > 3 * size) {
				/* It's the same,
				 * or shrinking and new/old > 3/4.
				 */
				return p;
			}
			size = nbytes;
		}
		bp = PyObject_Malloc(nbytes);
		if (bp != NULL) {
			memcpy(bp, p, size);
			PyObject_Free(p);
		}
		return bp;
	}
	/* We're not managing this block. */
	if (nbytes <= SMALL_REQUEST_THRESHOLD) {
		/* Take over this block -- ask for at least one byte so
		 * we really do take it over (PyObject_Malloc(0) goes to
		 * the system malloc).
		 */
		bp = PyObject_Malloc(nbytes ? nbytes : 1);
		if (bp != NULL) {
			memcpy(bp, p, nbytes);
			free(p);
		}
		else if (nbytes == 0) {
			/* Meet the doc's promise that nbytes==0 will
			 * never return a NULL pointer when p isn't NULL.
			 */
			bp = p;
		}

	}
	else {
		assert(nbytes != 0);
		bp = realloc(p, nbytes);
	}
	return bp;
}

#else	/* ! WITH_PYMALLOC */

/*==========================================================================*/
/* pymalloc not enabled:  Redirect the entry points to malloc.  These will
 * only be used by extensions that are compiled with pymalloc enabled. */

void *
PyObject_Malloc(size_t n)
{
	return PyMem_MALLOC(n);
}

void *
PyObject_Realloc(void *p, size_t n)
{
	return PyMem_REALLOC(p, n);
}

void
PyObject_Free(void *p)
{
	PyMem_FREE(p);
}
#endif /* WITH_PYMALLOC */

#ifdef PYMALLOC_DEBUG
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

static ulong serialno = 0;	/* incremented on each debug {m,re}alloc */

/* serialno is always incremented via calling this routine.  The point is
   to supply a single place to set a breakpoint.
*/
static void
bumpserialno(void)
{
	++serialno;
}


/* Read 4 bytes at p as a big-endian ulong. */
static ulong
read4(const void *p)
{
	const uchar *q = (const uchar *)p;
	return ((ulong)q[0] << 24) |
	       ((ulong)q[1] << 16) |
	       ((ulong)q[2] <<  8) |
	        (ulong)q[3];
}

/* Write the 4 least-significant bytes of n as a big-endian unsigned int,
   MSB at address p, LSB at p+3. */
static void
write4(void *p, ulong n)
{
	uchar *q = (uchar *)p;
	q[0] = (uchar)((n >> 24) & 0xff);
	q[1] = (uchar)((n >> 16) & 0xff);
	q[2] = (uchar)((n >>  8) & 0xff);
	q[3] = (uchar)( n        & 0xff);
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

#else
#define pool_is_in_list(X, Y) 1

#endif	/* Py_DEBUG */

/* The debug malloc asks for 16 extra bytes and fills them with useful stuff,
   here calling the underlying malloc's result p:

p[0:4]
    Number of bytes originally asked for.  4-byte unsigned integer,
    big-endian (easier to read in a memory dump).
p[4:8]
    Copies of FORBIDDENBYTE.  Used to catch under- writes and reads.
p[8:8+n]
    The requested memory, filled with copies of CLEANBYTE.
    Used to catch reference to uninitialized memory.
    &p[8] is returned.  Note that this is 8-byte aligned if pymalloc
    handled the request itself.
p[8+n:8+n+4]
    Copies of FORBIDDENBYTE.  Used to catch over- writes and reads.
p[8+n+4:8+n+8]
    A serial number, incremented by 1 on each call to _PyObject_DebugMalloc
    and _PyObject_DebugRealloc.
    4-byte unsigned integer, big-endian.
    If "bad memory" is detected later, the serial number gives an
    excellent way to set a breakpoint on the next run, to capture the
    instant at which this block was passed out.
*/

void *
_PyObject_DebugMalloc(size_t nbytes)
{
	uchar *p;	/* base address of malloc'ed block */
	uchar *tail;	/* p + 8 + nbytes == pointer to tail pad bytes */
	size_t total;	/* nbytes + 16 */

	bumpserialno();
	total = nbytes + 16;
	if (total < nbytes || (total >> 31) > 1) {
		/* overflow, or we can't represent it in 4 bytes */
		/* Obscure:  can't do (total >> 32) != 0 instead, because
		   C doesn't define what happens for a right-shift of 32
		   when size_t is a 32-bit type.  At least C guarantees
		   size_t is an unsigned type. */
		return NULL;
	}

	p = (uchar *)PyObject_Malloc(total);
	if (p == NULL)
		return NULL;

	write4(p, nbytes);
	p[4] = p[5] = p[6] = p[7] = FORBIDDENBYTE;

	if (nbytes > 0)
		memset(p+8, CLEANBYTE, nbytes);

	tail = p + 8 + nbytes;
	tail[0] = tail[1] = tail[2] = tail[3] = FORBIDDENBYTE;
	write4(tail + 4, serialno);

	return p+8;
}

/* The debug free first checks the 8 bytes on each end for sanity (in
   particular, that the FORBIDDENBYTEs are still intact).
   Then fills the original bytes with DEADBYTE.
   Then calls the underlying free.
*/
void
_PyObject_DebugFree(void *p)
{
	uchar *q = (uchar *)p;
	size_t nbytes;

	if (p == NULL)
		return;
	_PyObject_DebugCheckAddress(p);
	nbytes = read4(q-8);
	if (nbytes > 0)
		memset(q, DEADBYTE, nbytes);
	PyObject_Free(q-8);
}

void *
_PyObject_DebugRealloc(void *p, size_t nbytes)
{
	uchar *q = (uchar *)p;
	uchar *tail;
	size_t total;	/* nbytes + 16 */
	size_t original_nbytes;

	if (p == NULL)
		return _PyObject_DebugMalloc(nbytes);

	_PyObject_DebugCheckAddress(p);
	bumpserialno();
	original_nbytes = read4(q-8);
	total = nbytes + 16;
	if (total < nbytes || (total >> 31) > 1) {
		/* overflow, or we can't represent it in 4 bytes */
		return NULL;
	}

	if (nbytes < original_nbytes) {
		/* shrinking:  mark old extra memory dead */
		memset(q + nbytes, DEADBYTE, original_nbytes - nbytes);
	}

	/* Resize and add decorations. */
	q = (uchar *)PyObject_Realloc(q-8, total);
	if (q == NULL)
		return NULL;

	write4(q, nbytes);
	assert(q[4] == FORBIDDENBYTE &&
	       q[5] == FORBIDDENBYTE &&
	       q[6] == FORBIDDENBYTE &&
	       q[7] == FORBIDDENBYTE);
	q += 8;
	tail = q + nbytes;
	tail[0] = tail[1] = tail[2] = tail[3] = FORBIDDENBYTE;
	write4(tail + 4, serialno);

	if (nbytes > original_nbytes) {
		/* growing:  mark new extra memory clean */
		memset(q + original_nbytes, CLEANBYTE,
			nbytes - original_nbytes);
	}

	return q;
}

/* Check the forbidden bytes on both ends of the memory allocated for p.
 * If anything is wrong, print info to stderr via _PyObject_DebugDumpAddress,
 * and call Py_FatalError to kill the program.
 */
 void
_PyObject_DebugCheckAddress(const void *p)
{
	const uchar *q = (const uchar *)p;
	char *msg;
	ulong nbytes;
	const uchar *tail;
	int i;

	if (p == NULL) {
		msg = "didn't expect a NULL pointer";
		goto error;
	}

	/* Check the stuff at the start of p first:  if there's underwrite
	 * corruption, the number-of-bytes field may be nuts, and checking
	 * the tail could lead to a segfault then.
	 */
	for (i = 4; i >= 1; --i) {
		if (*(q-i) != FORBIDDENBYTE) {
			msg = "bad leading pad byte";
			goto error;
		}
	}

	nbytes = read4(q-8);
	tail = q + nbytes;
	for (i = 0; i < 4; ++i) {
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
void
_PyObject_DebugDumpAddress(const void *p)
{
	const uchar *q = (const uchar *)p;
	const uchar *tail;
	ulong nbytes, serial;
	int i;

	fprintf(stderr, "Debug memory block at address p=%p:\n", p);
	if (p == NULL)
		return;

	nbytes = read4(q-8);
	fprintf(stderr, "    %lu bytes originally requested\n", nbytes);

	/* In case this is nuts, check the leading pad bytes first. */
	fputs("    The 4 pad bytes at p-4 are ", stderr);
	if (*(q-4) == FORBIDDENBYTE &&
	    *(q-3) == FORBIDDENBYTE &&
	    *(q-2) == FORBIDDENBYTE &&
	    *(q-1) == FORBIDDENBYTE) {
		fputs("FORBIDDENBYTE, as expected.\n", stderr);
	}
	else {
		fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
			FORBIDDENBYTE);
		for (i = 4; i >= 1; --i) {
			const uchar byte = *(q-i);
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
	fprintf(stderr, "    The 4 pad bytes at tail=%p are ", tail);
	if (tail[0] == FORBIDDENBYTE &&
	    tail[1] == FORBIDDENBYTE &&
	    tail[2] == FORBIDDENBYTE &&
	    tail[3] == FORBIDDENBYTE) {
		fputs("FORBIDDENBYTE, as expected.\n", stderr);
	}
	else {
		fprintf(stderr, "not all FORBIDDENBYTE (0x%02x):\n",
			FORBIDDENBYTE);
		for (i = 0; i < 4; ++i) {
			const uchar byte = tail[i];
			fprintf(stderr, "        at tail+%d: 0x%02x",
				i, byte);
			if (byte != FORBIDDENBYTE)
				fputs(" *** OUCH", stderr);
			fputc('\n', stderr);
		}
	}

	serial = read4(tail+4);
	fprintf(stderr, "    The block was made by call #%lu to "
	                "debug malloc/realloc.\n", serial);

	if (nbytes > 0) {
		int i = 0;
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
}

static ulong
printone(const char* msg, ulong value)
{
	int i, k;
	char buf[100];
	ulong origvalue = value;

	fputs(msg, stderr);
	for (i = (int)strlen(msg); i < 35; ++i)
		fputc(' ', stderr);
	fputc('=', stderr);

	/* Write the value with commas. */
	i = 22;
	buf[i--] = '\0';
	buf[i--] = '\n';
	k = 3;
	do {
		ulong nextvalue = value / 10UL;
		uint digit = value - nextvalue * 10UL;
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
	fputs(buf, stderr);

	return origvalue;
}

/* Print summary info to stderr about the state of pymalloc's structures.
 * In Py_DEBUG mode, also perform some expensive internal consistency
 * checks.
 */
void
_PyObject_DebugMallocStats(void)
{
	uint i;
	const uint numclasses = SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT;
	/* # of pools, allocated blocks, and free blocks per class index */
	ulong numpools[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	ulong numblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	ulong numfreeblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	/* total # of allocated bytes in used and full pools */
	ulong allocated_bytes = 0;
	/* total # of available bytes in used pools */
	ulong available_bytes = 0;
	/* # of free pools + pools not yet carved out of current arena */
	uint numfreepools = 0;
	/* # of bytes for arena alignment padding */
	ulong arena_alignment = 0;
	/* # of bytes in used and full pools used for pool_headers */
	ulong pool_header_bytes = 0;
	/* # of bytes in used and full pools wasted due to quantization,
	 * i.e. the necessarily leftover space at the ends of used and
	 * full pools.
	 */
	ulong quantization = 0;
	/* running total -- should equal narenas * ARENA_SIZE */
	ulong total;
	char buf[128];

	fprintf(stderr, "Small block threshold = %d, in %u size classes.\n",
		SMALL_REQUEST_THRESHOLD, numclasses);

	for (i = 0; i < numclasses; ++i)
		numpools[i] = numblocks[i] = numfreeblocks[i] = 0;

	/* Because full pools aren't linked to from anything, it's easiest
	 * to march over all the arenas.  If we're lucky, most of the memory
	 * will be living in full pools -- would be a shame to miss them.
	 */
	for (i = 0; i < narenas; ++i) {
		uint poolsinarena;
		uint j;
		uptr base = arenas[i];

		/* round up to pool alignment */
		poolsinarena = ARENA_SIZE / POOL_SIZE;
		if (base & (uptr)POOL_SIZE_MASK) {
			--poolsinarena;
			arena_alignment += POOL_SIZE;
			base &= ~(uptr)POOL_SIZE_MASK;
			base += POOL_SIZE;
		}

		if (i == narenas - 1) {
			/* current arena may have raw memory at the end */
			numfreepools += nfreepools;
			poolsinarena -= nfreepools;
		}

		/* visit every pool in the arena */
		for (j = 0; j < poolsinarena; ++j, base += POOL_SIZE) {
			poolp p = (poolp)base;
			const uint sz = p->szidx;
			uint freeblocks;

			if (p->ref.count == 0) {
				/* currently unused */
				++numfreepools;
				assert(pool_is_in_list(p, freepools));
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

	fputc('\n', stderr);
	fputs("class   size   num pools   blocks in use  avail blocks\n"
	      "-----   ----   ---------   -------------  ------------\n",
		stderr);

	for (i = 0; i < numclasses; ++i) {
		ulong p = numpools[i];
		ulong b = numblocks[i];
		ulong f = numfreeblocks[i];
		uint size = INDEX2SIZE(i);
		if (p == 0) {
			assert(b == 0 && f == 0);
			continue;
		}
		fprintf(stderr, "%5u %6u %11lu %15lu %13lu\n",
			i, size, p, b, f);
		allocated_bytes += b * size;
		available_bytes += f * size;
		pool_header_bytes += p * POOL_OVERHEAD;
		quantization += p * ((POOL_SIZE - POOL_OVERHEAD) % size);
	}
	fputc('\n', stderr);
	(void)printone("# times object malloc called", serialno);

	PyOS_snprintf(buf, sizeof(buf),
		"%u arenas * %d bytes/arena", narenas, ARENA_SIZE);
	(void)printone(buf, (ulong)narenas * ARENA_SIZE);

	fputc('\n', stderr);

	total = printone("# bytes in allocated blocks", allocated_bytes);
	total += printone("# bytes in available blocks", available_bytes);

	PyOS_snprintf(buf, sizeof(buf),
		"%u unused pools * %d bytes", numfreepools, POOL_SIZE);
	total += printone(buf, (ulong)numfreepools * POOL_SIZE);

	total += printone("# bytes lost to pool headers", pool_header_bytes);
	total += printone("# bytes lost to quantization", quantization);
	total += printone("# bytes lost to arena alignment", arena_alignment);
	(void)printone("Total", total);
}

#endif	/* PYMALLOC_DEBUG */

#ifdef Py_USING_MEMORY_DEBUGGER
/* Make this function last so gcc won't inline it
   since the definition is after the reference. */
int
Py_ADDRESS_IN_RANGE(void *P, poolp pool)
{
	return ((pool->arenaindex) < narenas &&
		(uptr)(P) - arenas[pool->arenaindex] < (uptr)ARENA_SIZE);
}
#endif
