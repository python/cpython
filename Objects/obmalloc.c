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
 * have to be.
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

/* Return total number of blocks in poolp P, as a uint. */
#define NUMBLOCKS(P)	\
	((uint)(POOL_SIZE - POOL_OVERHEAD) / INDEX2SIZE((P)->szidx))

/*==========================================================================*/

/*
 * This malloc lock
 */
SIMPLELOCK_DECL(_malloc_lock);
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
available for allocating.  If pool->freeblock is NULL then, that means we
simply haven't yet gotten to one of the higher-address blocks.  The offset
from the pool_header to the start of "the next" virgin block is stored in
the pool_header nextoffset member, and the largest value of nextoffset that
makes sense is stored in the maxnextoffset member when a pool is initialized.
All the blocks in a pool have been passed out at least when and only when
nextoffset > maxnextoffset.


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

#if 0
static ulong wasmine = 0;
static ulong wasntmine = 0;

static void
dumpem(void *ptr)
{
	if (ptr)
		printf("inserted new arena at %08x\n", ptr);
	printf("# arenas %u\n", narenas);
	printf("was mine %lu wasn't mine %lu\n", wasmine, wasntmine);
}
#define INCMINE ++wasmine
#define INCTHEIRS ++wasntmine

#else
#define dumpem(ptr)
#define INCMINE
#define INCTHEIRS
#endif

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

	/* arenabase <- first pool-aligned address in the arena
	   nfreepools <- number of whole pools that fit after alignment */
	arenabase = bp;
	nfreepools = ARENA_SIZE / POOL_SIZE;
	assert(POOL_SIZE * nfreepools == ARENA_SIZE);
	excess = (uint)bp & POOL_SIZE_MASK;
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
		/* Grow arenas.  Don't use realloc:  if this fails, we
		 * don't want to lose the base addresses we already have.
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
	dumpem(bp);
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
#define ADDRESS_IN_RANGE(P, I) \
	((I) < narenas && (uptr)(P) - arenas[I] < (uptr)ARENA_SIZE)

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

void *
_PyMalloc_Malloc(size_t nbytes)
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
	return (void *)malloc(nbytes ? nbytes : 1);
}

/* free */

void
_PyMalloc_Free(void *p)
{
	poolp pool;
	block *lastfree;
	poolp next, prev;
	uint size;

	if (p == NULL)	/* free(NULL) has no effect */
		return;

	pool = POOL_ADDR(p);
	if (ADDRESS_IN_RANGE(p, pool->arenaindex)) {
		/* We allocated this address. */
		LOCK();
		INCMINE;
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
	INCTHEIRS;
	free(p);
}

/* realloc.  If p is NULL, this acts like malloc(nbytes).  Else if nbytes==0,
 * then as the Python docs promise, we do not treat this like free(p), and
 * return a non-NULL result.
 */

void *
_PyMalloc_Realloc(void *p, size_t nbytes)
{
	void *bp;
	poolp pool;
	uint size;

	if (p == NULL)
		return _PyMalloc_Malloc(nbytes);

	pool = POOL_ADDR(p);
	if (ADDRESS_IN_RANGE(p, pool->arenaindex)) {
		/* We're in charge of this block */
		INCMINE;
		size = INDEX2SIZE(pool->szidx);
		if (size >= nbytes)
			/* Don't bother if a smaller size was requested. */
			return p;
		/* We need more memory. */
		assert(nbytes != 0);
		bp = _PyMalloc_Malloc(nbytes);
		if (bp != NULL) {
			memcpy(bp, p, size);
			_PyMalloc_Free(p);
		}
		return bp;
	}
	/* We're not managing this block. */
	INCTHEIRS;
	if (nbytes <= SMALL_REQUEST_THRESHOLD) {
		/* Take over this block. */
		bp = _PyMalloc_Malloc(nbytes ? nbytes : 1);
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
/* pymalloc not enabled:  Redirect the entry points to the PyMem family. */

void *
_PyMalloc_Malloc(size_t n)
{
	return PyMem_MALLOC(n);
}

void *
_PyMalloc_Realloc(void *p, size_t n)
{
	return PyMem_REALLOC(p, n);
}

void
_PyMalloc_Free(void *p)
{
	PyMem_FREE(p);
}
#endif /* WITH_PYMALLOC */

/*==========================================================================*/
/* Regardless of whether pymalloc is enabled, export entry points for
 * the object-oriented pymalloc functions.
 */

PyObject *
_PyMalloc_New(PyTypeObject *tp)
{
	PyObject *op;
	op = (PyObject *) _PyMalloc_MALLOC(_PyObject_SIZE(tp));
	if (op == NULL)
		return PyErr_NoMemory();
	return PyObject_INIT(op, tp);
}

PyVarObject *
_PyMalloc_NewVar(PyTypeObject *tp, int nitems)
{
	PyVarObject *op;
	const size_t size = _PyObject_VAR_SIZE(tp, nitems);
	op = (PyVarObject *) _PyMalloc_MALLOC(size);
	if (op == NULL)
		return (PyVarObject *)PyErr_NoMemory();
	return PyObject_INIT_VAR(op, tp, nitems);
}

void
_PyMalloc_Del(PyObject *op)
{
	_PyMalloc_FREE(op);
}

#ifdef PYMALLOC_DEBUG
/*==========================================================================*/
/* A x-platform debugging allocator.  This doesn't manage memory directly,
 * it wraps a real allocator, adding extra debugging info to the memory blocks.
 */

#define PYMALLOC_CLEANBYTE      0xCB    /* uninitialized memory */
#define PYMALLOC_DEADBYTE       0xDB    /* free()ed memory */
#define PYMALLOC_FORBIDDENBYTE  0xFB    /* unusable memory */

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

/* The debug malloc asks for 16 extra bytes and fills them with useful stuff,
   here calling the underlying malloc's result p:

p[0:4]
    Number of bytes originally asked for.  4-byte unsigned integer,
    big-endian (easier to read in a memory dump).
p[4:8]
    Copies of PYMALLOC_FORBIDDENBYTE.  Used to catch under- writes
    and reads.
p[8:8+n]
    The requested memory, filled with copies of PYMALLOC_CLEANBYTE.
    Used to catch reference to uninitialized memory.
    &p[8] is returned.  Note that this is 8-byte aligned if PyMalloc
    handled the request itself.
p[8+n:8+n+4]
    Copies of PYMALLOC_FORBIDDENBYTE.  Used to catch over- writes
    and reads.
p[8+n+4:8+n+8]
    A serial number, incremented by 1 on each call to _PyMalloc_DebugMalloc
    and _PyMalloc_DebugRealloc.
    4-byte unsigned integer, big-endian.
    If "bad memory" is detected later, the serial number gives an
    excellent way to set a breakpoint on the next run, to capture the
    instant at which this block was passed out.
*/

void *
_PyMalloc_DebugMalloc(size_t nbytes)
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

	p = _PyMalloc_Malloc(total);
	if (p == NULL)
		return NULL;

	write4(p, nbytes);
	p[4] = p[5] = p[6] = p[7] = PYMALLOC_FORBIDDENBYTE;

	if (nbytes > 0)
		memset(p+8, PYMALLOC_CLEANBYTE, nbytes);

	tail = p + 8 + nbytes;
	tail[0] = tail[1] = tail[2] = tail[3] = PYMALLOC_FORBIDDENBYTE;
	write4(tail + 4, serialno);

	return p+8;
}

/* The debug free first checks the 8 bytes on each end for sanity (in
   particular, that the PYMALLOC_FORBIDDENBYTEs are still intact).
   Then fills the original bytes with PYMALLOC_DEADBYTE.
   Then calls the underlying free.
*/
void
_PyMalloc_DebugFree(void *p)
{
	uchar *q = (uchar *)p;
	size_t nbytes;

	if (p == NULL)
		return;
	_PyMalloc_DebugCheckAddress(p);
	nbytes = read4(q-8);
	if (nbytes > 0)
		memset(q, PYMALLOC_DEADBYTE, nbytes);
	_PyMalloc_Free(q-8);
}

void *
_PyMalloc_DebugRealloc(void *p, size_t nbytes)
{
	uchar *q = (uchar *)p;
	size_t original_nbytes;
	void *fresh;	/* new memory block, if needed */

	if (p == NULL)
		return _PyMalloc_DebugMalloc(nbytes);

	_PyMalloc_DebugCheckAddress(p);
	original_nbytes = read4(q-8);
	if (nbytes == original_nbytes) {
		/* note that this case is likely to be common due to the
		   way Python appends to lists */
		bumpserialno();
		write4(q + nbytes + 4, serialno);
		return p;
	}

	if (nbytes < original_nbytes) {
		/* shrinking -- leave the guts alone, except to
		   fill the excess with DEADBYTE */
		const size_t excess = original_nbytes - nbytes;
		bumpserialno();
		write4(q-8, nbytes);
		/* kill the excess bytes plus the trailing 8 pad bytes */
		q += nbytes;
		q[0] = q[1] = q[2] = q[3] = PYMALLOC_FORBIDDENBYTE;
		write4(q+4, serialno);
		memset(q+8, PYMALLOC_DEADBYTE, excess);
		return p;
	}

	/* More memory is needed:  get it, copy over the first original_nbytes
	   of the original data, and free the original memory. */
	fresh = _PyMalloc_DebugMalloc(nbytes);
	if (fresh != NULL && original_nbytes > 0)
		memcpy(fresh, p, original_nbytes);
	_PyMalloc_DebugFree(p);
	return fresh;
}

/* Check the forbidden bytes on both ends of the memory allocated for p.
 * If anything is wrong, print info to stderr via _PyMalloc_DebugDumpAddress,
 * and call Py_FatalError to kill the program.
 */
 void
_PyMalloc_DebugCheckAddress(const void *p)
{
	const uchar *q = (const uchar *)p;
	char *msg;
	int i;

	if (p == NULL) {
		msg = "didn't expect a NULL pointer";
		goto error;
	}

	for (i = 4; i >= 1; --i) {
		if (*(q-i) != PYMALLOC_FORBIDDENBYTE) {
			msg = "bad leading pad byte";
			goto error;
		}
	}

	{
		const ulong nbytes = read4(q-8);
		const uchar *tail = q + nbytes;
		for (i = 0; i < 4; ++i) {
			if (tail[i] != PYMALLOC_FORBIDDENBYTE) {
				msg = "bad trailing pad byte";
				goto error;
			}
		}
	}

	return;

error:
	_PyMalloc_DebugDumpAddress(p);
	Py_FatalError(msg);
}

/* Display info to stderr about the memory block at p. */
void
_PyMalloc_DebugDumpAddress(const void *p)
{
	const uchar *q = (const uchar *)p;
	const uchar *tail;
	ulong nbytes, serial;
	int i;

	fprintf(stderr, "Debug memory block at address p=%p:\n", p);
	if (p == NULL)
		return;

	nbytes = read4(q-8);
	fprintf(stderr, "    %lu bytes originally allocated\n", nbytes);

	/* In case this is nuts, check the pad bytes before trying to read up
	   the serial number (the address deref could blow up). */

	fputs("    the 4 pad bytes at p-4 are ", stderr);
	if (*(q-4) == PYMALLOC_FORBIDDENBYTE &&
	    *(q-3) == PYMALLOC_FORBIDDENBYTE &&
	    *(q-2) == PYMALLOC_FORBIDDENBYTE &&
	    *(q-1) == PYMALLOC_FORBIDDENBYTE) {
		fputs("PYMALLOC_FORBIDDENBYTE, as expected\n", stderr);
	}
	else {
		fprintf(stderr, "not all PYMALLOC_FORBIDDENBYTE (0x%02x):\n",
			PYMALLOC_FORBIDDENBYTE);
		for (i = 4; i >= 1; --i) {
			const uchar byte = *(q-i);
			fprintf(stderr, "        at p-%d: 0x%02x", i, byte);
			if (byte != PYMALLOC_FORBIDDENBYTE)
				fputs(" *** OUCH", stderr);
			fputc('\n', stderr);
		}
	}

	tail = q + nbytes;
	fprintf(stderr, "    the 4 pad bytes at tail=%p are ", tail);
	if (tail[0] == PYMALLOC_FORBIDDENBYTE &&
	    tail[1] == PYMALLOC_FORBIDDENBYTE &&
	    tail[2] == PYMALLOC_FORBIDDENBYTE &&
	    tail[3] == PYMALLOC_FORBIDDENBYTE) {
		fputs("PYMALLOC_FORBIDDENBYTE, as expected\n", stderr);
	}
	else {
		fprintf(stderr, "not all PYMALLOC_FORBIDDENBYTE (0x%02x):\n",
			PYMALLOC_FORBIDDENBYTE);
		for (i = 0; i < 4; ++i) {
			const uchar byte = tail[i];
			fprintf(stderr, "        at tail+%d: 0x%02x",
				i, byte);
			if (byte != PYMALLOC_FORBIDDENBYTE)
				fputs(" *** OUCH", stderr);
			fputc('\n', stderr);
		}
	}

	serial = read4(tail+4);
	fprintf(stderr, "    the block was made by call #%lu to "
	                "debug malloc/realloc\n", serial);

	if (nbytes > 0) {
		int i = 0;
		fputs("    data at p:", stderr);
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

/* Print summary info to stderr about the state of pymalloc's structures. */
void
_PyMalloc_DebugDumpStats(void)
{
	uint i;
	const uint numclasses = SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT;
	uint numfreepools = 0;
	/* # of pools per class index */
	ulong numpools[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	/* # of allocated blocks per class index */
	ulong numblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	/* # of free blocks per class index */
	ulong numfreeblocks[SMALL_REQUEST_THRESHOLD >> ALIGNMENT_SHIFT];
	ulong grandtotal;	/* total # of allocated bytes */
	ulong freegrandtotal;	/* total # of available bytes in used pools */

	fprintf(stderr, "%u arenas * %d bytes/arena = %lu total bytes.\n",
		narenas, ARENA_SIZE, narenas * (ulong)ARENA_SIZE);
	fprintf(stderr, "Small block threshold = %d, in %u size classes.\n",
		SMALL_REQUEST_THRESHOLD, numclasses);
	fprintf(stderr, "pymalloc malloc+realloc called %lu times.\n",
		serialno);

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
			if (p->ref.count == 0) {
				/* currently unused */
				++numfreepools;
				continue;
			}
			++numpools[p->szidx];
			numblocks[p->szidx] += p->ref.count;
			numfreeblocks[p->szidx] += NUMBLOCKS(p) - p->ref.count;
		}
	}

	fputc('\n', stderr);
	fprintf(stderr, "Number of unused pools: %u\n", numfreepools);
	fputc('\n', stderr);
	fputs("class   num bytes   num pools   blocks in use  avail blocks\n"
	      "-----   ---------   ---------   -------------  ------------\n",
		stderr);

	grandtotal = freegrandtotal = 0;
	for (i = 0; i < numclasses; ++i) {
		ulong p = numpools[i];
		ulong b = numblocks[i];
		ulong f = numfreeblocks[i];
		uint size = INDEX2SIZE(i);
		if (p == 0) {
			assert(b == 0 && f == 0);
			continue;
		}
		fprintf(stderr, "%5u %11u %11lu %15lu %13lu\n",
			i, size, p, b, f);
		grandtotal += b * size;
		freegrandtotal += f * size;
	}
	fputc('\n', stderr);
	fprintf(stderr, "Total bytes in allocated blocks: %lu\n",
		grandtotal);
	fprintf(stderr, "Total free bytes in used pools:  %lu\n",
		freegrandtotal);
}

#endif	/* PYMALLOC_DEBUG */
