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
#define WITH_MALLOC_HOOKS		/* for profiling & debugging */

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

/*
 * Max size threshold below which malloc requests are considered to be
 * small enough in order to use preallocated memory pools. You can tune
 * this value according to your application behaviour and memory needs.
 *
 * The following invariants must hold:
 *	1) ALIGNMENT <= SMALL_REQUEST_THRESHOLD <= 256
 *	2) SMALL_REQUEST_THRESHOLD == N * ALIGNMENT
 *
 * Although not required, for better performance and space efficiency,
 * it is recommended that SMALL_REQUEST_THRESHOLD is set to a power of 2.
 */

/*
 * For Python compiled on systems with 32 bit pointers and integers,
 * a value of 64 (= 8 * 8) is a reasonable speed/space tradeoff for
 * the object allocator. To adjust automatically this threshold for
 * systems with 64 bit pointers, we make this setting depend on a
 * Python-specific slot size unit = sizeof(long) + sizeof(void *),
 * which is expected to be 8, 12 or 16 bytes.
 */

#define _PYOBJECT_THRESHOLD	((SIZEOF_LONG + SIZEOF_VOID_P) * ALIGNMENT)

#define SMALL_REQUEST_THRESHOLD	_PYOBJECT_THRESHOLD /* must be N * ALIGNMENT */

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
 * memory from the system accross various platforms.
 */

#define ARENA_SIZE		(256 * 1024 - SYSTEM_PAGE_SIZE)	/* 256k - 1p */

#ifdef WITH_MEMORY_LIMITS
#define MAX_ARENAS		(SMALL_MEMORY_LIMIT / ARENA_SIZE)
#endif

/*
 * Size of the pools used for small blocks. Should be a power of 2,
 * between 1K and SYSTEM_PAGE_SIZE, that is: 1k, 2k, 4k, eventually 8k.
 */

#define POOL_SIZE		SYSTEM_PAGE_SIZE	/* must be 2^N */
#define POOL_SIZE_MASK		SYSTEM_PAGE_SIZE_MASK
#define POOL_MAGIC		0x74D3A651		/* authentication id */

#define ARENA_NB_POOLS		(ARENA_SIZE / POOL_SIZE)
#define ARENA_NB_PAGES		(ARENA_SIZE / SYSTEM_PAGE_SIZE)

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

#undef  ushort
#define ushort			unsigned short	/* assuming >= 16 bits */

#undef  uint
#define uint			unsigned int	/* assuming >= 16 bits */

#undef  ulong
#define ulong			unsigned long	/* assuming >= 32 bits */

#undef  off_t
#define off_t 			uint	/* 16 bits <= off_t <= 64 bits */

/* When you say memory, my mind reasons in terms of (pointers to) blocks */
typedef uchar block;

/* Pool for small blocks */
struct pool_header {
	union { block *_padding;
		uint count; } ref;	/* number of allocated blocks    */
	block *freeblock;		/* pool's free list head         */
	struct pool_header *nextpool;	/* next pool of this size class  */
	struct pool_header *prevpool;	/* previous pool       ""        */
	struct pool_header *pooladdr;	/* pool address (always aligned) */
	uint magic;			/* pool magic number		 */
	uint szidx;			/* block size class index	 */
	uint capacity;			/* pool capacity in # of blocks  */
};

typedef struct pool_header *poolp;

#undef  ROUNDUP
#define ROUNDUP(x)		(((x) + ALIGNMENT_MASK) & ~ALIGNMENT_MASK)
#define POOL_OVERHEAD		ROUNDUP(sizeof(struct pool_header))

#define DUMMY_SIZE_IDX		0xffff	/* size class of newly cached pools */

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
 * Pool table -- doubly linked lists of partially used pools
 */
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

/*
 * Arenas
 */
static uint arenacnt = 0;		/* number of allocated arenas */
static uint watermark = ARENA_NB_POOLS;	/* number of pools allocated from
					   the current arena */
static block *arenalist = NULL;		/* list of allocated arenas */
static block *arenabase = NULL;		/* free space start address in
					   current arena */

/*
 * Hooks
 */
#ifdef WITH_MALLOC_HOOKS
static void *(*malloc_hook)(size_t) = NULL;
static void *(*calloc_hook)(size_t, size_t) = NULL;
static void *(*realloc_hook)(void *, size_t) = NULL;
static void (*free_hook)(void *) = NULL;
#endif /* !WITH_MALLOC_HOOKS */

/*==========================================================================*/

/* malloc */

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

#ifdef WITH_MALLOC_HOOKS	
	if (malloc_hook != NULL)
		return (*malloc_hook)(nbytes);
#endif

	/*
	 * This implicitly redirects malloc(0)
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
			if (pool->ref.count < pool->capacity) {
				/*
				 * There is room for another block
				 */
				size++;
				size <<= ALIGNMENT_SHIFT; /* block size */
				pool->freeblock = (block *)pool + \
						  POOL_OVERHEAD + \
						  pool->ref.count * size;
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
			 * Initialize the pool header and free list
			 * then return the first block.
			 */
			pool->szidx = size;
			size++;
			size <<= ALIGNMENT_SHIFT; /* block size */
			bp = (block *)pool + POOL_OVERHEAD;
			pool->freeblock = bp + size;
			*(block **)(pool->freeblock) = NULL;
			pool->capacity = (POOL_SIZE - POOL_OVERHEAD) / size;
			UNLOCK();
			return (void *)bp;
		}
                /*
                 * Allocate new pool
                 */
		if (watermark < ARENA_NB_POOLS) {
			/* commit malloc(POOL_SIZE) from the current arena */
		commit_pool:
			watermark++;
			pool = (poolp )arenabase;
			arenabase += POOL_SIZE;
			pool->pooladdr = pool;
			pool->magic = (uint )POOL_MAGIC;
			pool->szidx = DUMMY_SIZE_IDX;
			goto init_pool;
		}
                /*
                 * Allocate new arena
                 */
#ifdef WITH_MEMORY_LIMITS
		if (!(arenacnt < MAX_ARENAS)) {
			UNLOCK();
			goto redirect;
		}
#endif
		/*
		 * With malloc, we can't avoid loosing one page address space
		 * per arena due to the required alignment on page boundaries.
		 */
		bp = (block *)PyMem_MALLOC(ARENA_SIZE + SYSTEM_PAGE_SIZE);
		if (bp == NULL) {
			UNLOCK();
			goto redirect;
		}
		/* 
		 * Keep a reference in the list of allocated arenas. We might
		 * want to release (some of) them in the future. The first
		 * word is never used, no matter whether the returned address
		 * is page-aligned or not, so we safely store a pointer in it.
		 */
		*(block **)bp = arenalist;
		arenalist = bp;
		arenacnt++;
		watermark = 0;
		/* Page-round up */
		arenabase = bp + (SYSTEM_PAGE_SIZE -
				  ((off_t )bp & SYSTEM_PAGE_SIZE_MASK));
		goto commit_pool;
	}

        /* The small block allocator ends here. */

	redirect:
	
	/*
	 * Redirect the original request to the underlying (libc) allocator.
	 * We jump here on bigger requests, on error in the code above (as a
	 * last chance to serve the request) or when the max memory limit
	 * has been reached.
	 */
	return (void *)PyMem_MALLOC(nbytes);
}

/* free */

void
_PyMalloc_Free(void *p)
{
	poolp pool;
	poolp next, prev;
	uint size;
	off_t offset;

#ifdef WITH_MALLOC_HOOKS
	if (free_hook != NULL) {
		(*free_hook)(p);
		return;
	}
#endif

	if (p == NULL)	/* free(NULL) has no effect */
		return;

	offset = (off_t )p & POOL_SIZE_MASK;
	pool = (poolp )((block *)p - offset);
	if (pool->pooladdr != pool || pool->magic != (uint )POOL_MAGIC) {
		PyMem_FREE(p);
		return;
	}

	LOCK();
	/*
	 * At this point, the pool is not empty
	 */
	if ((*(block **)p = pool->freeblock) == NULL) {
		/*
		 * Pool was full
		 */
		pool->freeblock = (block *)p;
		--pool->ref.count;
		/*
		 * Frontlink to used pools
		 * This mimics LRU pool usage for new allocations and
		 * targets optimal filling when several pools contain
		 * blocks of the same size class.
		 */
		size = pool->szidx;
		next = usedpools[size + size];
		prev = next->prevpool;
		pool->nextpool = next;
		pool->prevpool = prev;
		next->prevpool = pool;
		prev->nextpool = pool;
		UNLOCK();
		return;
	}
	/*
	 * Pool was not full
	 */
	pool->freeblock = (block *)p;
	if (--pool->ref.count != 0) {
		UNLOCK();
		return;
	}
	/*
	 * Pool is now empty, unlink from used pools
	 */
	next = pool->nextpool;
	prev = pool->prevpool;
	next->prevpool = prev;
	prev->nextpool = next;
	/*
	 * Frontlink to free pools
	 * This ensures that previously freed pools will be allocated
	 * later (being not referenced, they are perhaps paged out).
	 */
	pool->nextpool = freepools;
	freepools = pool;
	UNLOCK();
	return;
}

/* realloc */

void *
_PyMalloc_Realloc(void *p, size_t nbytes)
{
	block *bp;
	poolp pool;
	uint size;

#ifdef WITH_MALLOC_HOOKS
	if (realloc_hook != NULL)
		return (*realloc_hook)(p, nbytes);
#endif

	if (p == NULL)
		return _PyMalloc_Malloc(nbytes);

	/* realloc(p, 0) on big blocks is redirected. */
	pool = (poolp )((block *)p - ((off_t )p & POOL_SIZE_MASK));
	if (pool->pooladdr != pool || pool->magic != (uint )POOL_MAGIC) {
		/* We haven't allocated this block */
		if (!(nbytes > SMALL_REQUEST_THRESHOLD) && nbytes) {
			/* small request */
			size = nbytes;
			goto malloc_copy_free;
		}
		bp = (block *)PyMem_REALLOC(p, nbytes);
	}
	else {
		/* We're in charge of this block */
		size = (pool->szidx + 1) << ALIGNMENT_SHIFT; /* block size */
		if (size >= nbytes) {
			/* Don't bother if a smaller size was requested
			   except for realloc(p, 0) == free(p), ret NULL */
			if (nbytes == 0) {
				_PyMalloc_Free(p);
				bp = NULL;
			}
			else
				bp = (block *)p;
		}
		else {

		malloc_copy_free:

			bp = (block *)_PyMalloc_Malloc(nbytes);
			if (bp != NULL) {
				memcpy(bp, p, size);
				_PyMalloc_Free(p);
			}
		}
	}
	return (void *)bp;
}

/* calloc */

/* -- unused --
void *
_PyMalloc_Calloc(size_t nbel, size_t elsz)
{
        void *p;
	size_t nbytes;

#ifdef WITH_MALLOC_HOOKS
	if (calloc_hook != NULL)
		return (*calloc_hook)(nbel, elsz);
#endif

	nbytes = nbel * elsz;
	p = _PyMalloc_Malloc(nbytes);
	if (p != NULL)
		memset(p, 0, nbytes);
	return p;
}
*/

/*==========================================================================*/

/*
 * Hooks
 */

#ifdef WITH_MALLOC_HOOKS

void
_PyMalloc_SetHooks( void *(*malloc_func)(size_t),
		    void *(*calloc_func)(size_t, size_t),
		    void *(*realloc_func)(void *, size_t),
		    void (*free_func)(void *) )
{
	LOCK();
	malloc_hook = malloc_func;
	calloc_hook = calloc_func;
	realloc_hook = realloc_func;
	free_hook = free_func;
	UNLOCK();
}

void
_PyMalloc_FetchHooks( void *(**malloc_funcp)(size_t),
		      void *(**calloc_funcp)(size_t, size_t),
		      void *(**realloc_funcp)(void *, size_t),
		      void (**free_funcp)(void *) )
{
	LOCK();
	*malloc_funcp = malloc_hook;
	*calloc_funcp = calloc_hook;
	*realloc_funcp = realloc_hook;
	*free_funcp = free_hook;
	UNLOCK();
}
#endif /* !WITH_MALLOC_HOOKS */
