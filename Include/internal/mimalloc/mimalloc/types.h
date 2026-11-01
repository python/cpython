/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_TYPES_H
#define MI_TYPES_H

// --------------------------------------------------------------------------
// This file contains the main type definitions for mimalloc:
// mi_heap_t      : all data for a heap; usually there is just one main default heap.
// mi_theap_t     : a thread local heap belonging to a specific heap:
//                  maintains lists of thread-local heap pages that have free space.
// mi_page_t      : a mimalloc page (usually 64KiB or 512KiB) from
//                  where objects of a single size are allocated.
//                  Note: we write "OS page" for OS memory pages while
//                  using plain "page" for mimalloc pages (`mi_page_t`).
// mi_arena_t     : a large memory area where pages are allocated (process shared)
// mi_tld_t       : thread local data
// mi_subproc_t   : all heaps belong to a sub-process (usually just the main one)
// --------------------------------------------------------------------------

#include "../mimalloc.h"
#include "../mimalloc-stats.h"
#include "../mimalloc-profile.h"
#include <stddef.h>   // ptrdiff_t
#include <stdint.h>   // uintptr_t, uint16_t, etc
#include <stdbool.h>  // bool
#include <limits.h>   // SIZE_MAX etc.
#include <errno.h>    // error codes

#include "decl.h"     // compiler dependent definitions
#include "bits.h"     // size defines (MI_SIZE_SIZE etc), bit operations
#include "atomic.h"   // _Atomic primitives
#include "qsbr.h"     // quiescent state-based reclamation (qsbr) primitives (for runtime systems like CPython)

// Minimal alignment necessary. On most platforms 16 bytes are needed
// due to SSE registers for example. This must be at least `sizeof(void*)`
#ifndef MI_MAX_ALIGN_SIZE
#define MI_MAX_ALIGN_SIZE  16   // sizeof(max_align_t)
#endif

#if MI_MAX_ALIGN_SIZE < MI_INTPTR_SIZE
#error MI_MAX_ALIGN_SIZE must be at least MI_INTPTR_SIZE
#endif

// ------------------------------------------------------
// Variants
// ------------------------------------------------------

// Define NDEBUG in the release version to disable assertions.
// #define NDEBUG

// Define MI_TRACK_<tool> to enable tracking support
// #define MI_TRACK_VALGRIND 1
// #define MI_TRACK_ASAN     1
// #define MI_TRACK_ETW      1
// #define MI_TRACK_CUSTOM   1

// Define MI_STATS as 1 to maintain statistics; set it to 2 to have detailed statistics (but costs some performance).
// #define MI_STATS 1

// Define MI_SECURE to enable security mitigations
// #define MI_SECURE 1  // check invalid pointer free, guard pages around meta data, randomize arena allocation addresses (like ASLR), abort on detected meta data corruption
// #define MI_SECURE 2  // randomize relative allocation addresses within mimalloc pages
// #define MI_SECURE 3  // check buffer overflow, check double free, encode free lists (and detect corrupted free lists)
// #define MI_SECURE 4  // same as level 3 for now (`-DMI_SECURE=ON`)
// #define MI_SECURE 5  // guard page at the end of each mimalloc page (expensive!) (`-DMI_SECURE_FULL=ON`), and byte-precise buffer overflow checks.

#if !defined(MI_SECURE)
#define MI_SECURE 0
#endif

// Define MI_DEBUG for assertion and invariant checking
// #define MI_DEBUG 1  // basic assertion checks and statistics, check double free, corrupted free list, and invalid pointer free. (cmake -DMI_DEBUG=ON)
// #define MI_DEBUG 2  // + internal assertion checks (cmake -DMI_DEBUG_INTERNAL=ON)
// #define MI_DEBUG 3  // + extensive internal invariant checking (cmake -DMI_DEBUG_FULL=ON)
#if !defined(MI_DEBUG)
#if defined(MI_BUILD_RELEASE) || defined(NDEBUG)
#define MI_DEBUG 0
#else
#define MI_DEBUG 2
#endif
#endif

// Statistics (0=only essential, 1=detailed (fast, can be enabled always), 2=slightly more expensive tracking of precise requested bytes)
#ifndef MI_STATS
#if (MI_DEBUG>0)
#define MI_STATS 2
#else
#define MI_STATS 1
#endif
#endif

// Enable profiling support (0=off, 1=fast, can be enabled always, 2=allow fine-grained sample rates(<64 KiB), a tad more expensive)
#ifndef MI_PROFILE
#define MI_PROFILE  1
#endif

// Enable guard pages behind objects of a certain size (set by the MIMALLOC_GUARDED_MIN/MAX/SAMPLE_RATE options)
#if !defined(MI_GUARDED) && MI_DEBUG && !defined(NDEBUG) && !MI_OPT_FREE_SMALL
#define MI_GUARDED  1
#endif

// For profiling or guarded pages we need to sample every once in a while. (0=no sampling, 1=fast, 2=allow fine grained sample rates(<64 KiB), a tad more expensive)
#ifndef MI_SAMPLE
#if MI_PROFILE>1 || MI_GUARDED>1
  #define MI_SAMPLE 2
#elif MI_PROFILE || MI_GUARDED
  #define MI_SAMPLE 1
#else
  #define MI_SAMPLE 0
#endif
#endif

// Reserve extra padding at the end of each block to be more resilient against theap block overflows.
// The padding can detect heap-block overflow on free, and provides byte-precise `mi_usable_size`.
#if !defined(MI_PADDING)
#if (MI_SECURE>=3 || MI_DEBUG>=1 || (MI_TRACK_VALGRIND || MI_TRACK_ASAN || MI_TRACK_ETW))
#define MI_PADDING  1
#else
#define MI_PADDING  0
#endif
#endif

// Check for byte-precise buffer overflow?
#if !defined(MI_PADDING_CHECK_BYTES) && MI_PADDING && (MI_SECURE>=5 || MI_DEBUG>=1)
#define MI_PADDING_CHECK_BYTES 1
#endif


// Encoded free lists allow detection of corrupted free lists
// and can detect buffer overflows, modify after free, and double `free`s.
#if (MI_SECURE>=3 || MI_DEBUG>=1)
#define MI_ENCODE_FREELIST  1
#endif

// Deprecated (double-free is now checked with padding)
// #if (MI_ENCODE_FREELIST && (MI_SECURE>=4 || MI_DEBUG!=0))
// #define MI_CHECK_DOUBLE_FREE  1
// #endif

#if MI_SECURE>=4 || MI_PADDING || (MI_FREE_IS_CHECKED && MI_FREE_USE_PAGEMAP)
#define MI_PAGE_KEY_COUNT 2
#else
#define MI_PAGE_KEY_COUNT 1
#endif

// Enable large pages for objects between 64KiB and 512KiB.
// This should perhaps be disabled by default as for many workloads the block sizes above 64 KiB
// are quite random which can lead to too many partially used large pages (but see issue #1104).
#ifndef MI_ENABLE_LARGE_PAGES
#define MI_ENABLE_LARGE_PAGES  1
#endif

// Place page meta info at the start of the page area or keep it separate?
// Separate keeps the page info at the arena start (default) which is more secure
// and reduces wasted space due to alignment and block sizes.
#if !defined(MI_PAGE_META_IS_SEPARATED)
#if MI_PAGE_MAP_FLAT
#define MI_PAGE_META_IS_SEPARATED    0
#else
#define MI_PAGE_META_IS_SEPARATED    1
#endif
#endif

// We can choose to page meta info aligned at the start of every MI_PAGE_META_ALIGNED_CHUNKS
// This can be used to have a faster `mi_free(_small)` as we can avoid a page_map lookup.
// This only works if valid pointers are passed to `mi_free` though. However, checked
// free `mi_cfree` can still use the page map to validate pointers.
#if !MI_FREE_IS_CHECKED && !MI_FREE_USE_PAGEMAP
#if MI_PAGE_META_IS_SEPARATED
#define MI_PAGE_META_IS_ALIGNED         1
#define MI_PAGE_META_ALIGNED_CHUNKS     MI_SIZE_SIZE
#else
#warning "cannot optimize free with alignment since the page meta data is not separated (due to MI_PAGE_MAP_FLAT?)"
#endif
#endif

// We can choose to only put page info of small pages at the start of the page area.
// This can be used to have a slightly faster `mi_free_small` function for specialized
// cases (like language runtime systems).
#if !defined(MI_PAGE_META_SMALL_IS_ALIGNED)
#if defined(MI_OPT_FREE_SMALL) && MI_OPT_FREE_SMALL==0
#define MI_PAGE_META_SMALL_IS_ALIGNED   0
#elif (MI_OPT_FREE_SMALL || MI_PAGE_META_IS_ALIGNED) && !MI_SECURE && !MI_GUARDED // guarded can put small allocations into big blocks in a medium page (which would make `mi_free_small` fail)
#define MI_PAGE_META_SMALL_IS_ALIGNED   1
#else
#define MI_PAGE_META_SMALL_IS_ALIGNED   0
#endif
#endif

// Configuration checks
#if !MI_PAGE_META_IS_SEPARATED && MI_SECURE
#error "secure mode should use separated page infos"
#endif
#if MI_PAGE_META_SMALL_IS_ALIGNED && MI_SECURE
#error "secure mode cannot use MI_OPT_FREE_SMALL (MI_PAGE_META_SMALL_IS_ALIGNED)"
#endif
#if MI_PAGE_META_IS_SEPARATED && MI_PAGE_MAP_FLAT
#error "cannot have a flat page map with separated page infos"
#endif


// --------------------------------------------------------------
// Sizes of internal data-structures
// (comments specify sizes on 64-bit, usually 32-bit is halved)
// --------------------------------------------------------------

// Main size parameter; determines max arena sizes and max arena object sizes etc.
#ifndef MI_ARENA_SLICE_SHIFT
  #ifdef  MI_SMALL_PAGE_SHIFT   // backward compatibility
  #define MI_ARENA_SLICE_SHIFT              MI_SMALL_PAGE_SHIFT
  #elif MI_SECURE>=5 && __APPLE__ && MI_ARCH_ARM64
  #define MI_ARENA_SLICE_SHIFT              (17)                        // 128 KiB to not waste too much due to 16 KiB guard pages
  #elif MI_SECURE>=5 && defined(PAGE_SIZE) && !defined(__wasi__)        // see issue #1440
    #if PAGE_SIZE >= 16*MI_KiB
    #define MI_ARENA_SLICE_SHIFT            (17)                        // 128 KiB to not waste too much due to 16 KiB guard pages
    #endif
  #endif
  #ifndef MI_ARENA_SLICE_SHIFT
  #define MI_ARENA_SLICE_SHIFT              (13 + MI_SIZE_SHIFT)        // 64 KiB (32 KiB on 32-bit)
  #endif
#endif
#if MI_ARENA_SLICE_SHIFT < 13
#error Arena slices should be at least 8KiB
#endif

#ifndef MI_BCHUNK_BITS_SHIFT
  #if MI_ARENA_SLICE_SHIFT <= 13    // <= 8KiB
  #define MI_BCHUNK_BITS_SHIFT              (7)   // 128 bits
  #elif MI_ARENA_SLICE_SHIFT < 16   // <= 32KiB
  #define MI_BCHUNK_BITS_SHIFT              (8)   // 256 bits
  #else
  #define MI_BCHUNK_BITS_SHIFT              (6 + MI_SIZE_SHIFT)       // 512 bits (or 256 on 32-bit)
  #endif
#endif

#define MI_BCHUNK_BITS                    (1 << MI_BCHUNK_BITS_SHIFT)         // sub-bitmaps in arena's are "bchunks" of 512 bits
#define MI_ARENA_SLICE_SIZE               (MI_ZU(1) << MI_ARENA_SLICE_SHIFT)  // arena's allocate in slices of 64 KiB
#define MI_ARENA_SLICE_ALIGN              (MI_ARENA_SLICE_SIZE)
#define MI_ARENA_CHUNK_SIZE               (MI_BCHUNK_BITS * MI_ARENA_SLICE_SIZE)

#define MI_ARENA_MIN_OBJ_SLICES           (1)
#define MI_ARENA_MAX_CHUNK_OBJ_SLICES     (MI_BCHUNK_BITS)                    // 32 MiB (or 8 MiB on 32-bit)

#define MI_ARENA_MIN_OBJ_SIZE             (MI_ARENA_MIN_OBJ_SLICES * MI_ARENA_SLICE_SIZE)
#define MI_ARENA_MAX_CHUNK_OBJ_SIZE       (MI_ARENA_MAX_CHUNK_OBJ_SLICES * MI_ARENA_SLICE_SIZE)

#if MI_ARENA_MAX_CHUNK_OBJ_SIZE < MI_SIZE_SIZE*1024
#error maximum object size may be too small to hold local thread data
#endif

#define MI_SMALL_PAGE_SIZE                MI_ARENA_MIN_OBJ_SIZE                    // 64 KiB
#define MI_MEDIUM_PAGE_SIZE               (8*MI_SMALL_PAGE_SIZE)                   // 512 KiB  (=byte in the bchunk bitmap)
#define MI_LARGE_PAGE_SIZE                (MI_SIZE_SIZE*MI_MEDIUM_PAGE_SIZE)       // 4 MiB    (=word in the bchunk bitmap)


// Maximum number of size classes. (spaced exponentially in 12.5% increments)
#if MI_BIN_HUGE != 73U
#error "mimalloc internal: expecting 73 bins"
#endif
#define MI_BIN_FULL  (MI_BIN_HUGE+1)
#define MI_BIN_COUNT (MI_BIN_FULL+1)

// We never allocate more than PTRDIFF_MAX (see also <https://sourceware.org/ml/libc-announce/2019/msg00001.html>)
#define MI_MAX_ALLOC_SIZE         PTRDIFF_MAX

// Minimal commit for a page on-demand commit
#define MI_PAGE_MIN_COMMIT_SIZE   (16*MI_KiB) /* MI_ARENA_SLICE_SIZE */

#if MI_PAGE_META_IS_ALIGNED
#define MI_PAGE_META_ALIGNED_COUNT    (MI_PAGE_META_ALIGNED_CHUNKS * MI_BCHUNK_BITS)
#define MI_PAGE_META_ALIGNMENT        (MI_PAGE_META_ALIGNED_COUNT * MI_ARENA_SLICE_SIZE)  // 256 MiB (32 MiB on 32-bit)
#define MI_ARENA_ALIGNMENT            MI_PAGE_META_ALIGNMENT
#else
#define MI_ARENA_ALIGNMENT            MI_ARENA_SLICE_ALIGN
#endif

// ------------------------------------------------------
// Arena's are large reserved areas of memory allocated from
// the OS that are managed by mimalloc to efficiently
// allocate MI_ARENA_SLICE_SIZE slices of memory for the
// mimalloc pages.
// ------------------------------------------------------

// A large memory arena where pages are allocated in.
typedef struct mi_arena_s mi_arena_t;     // defined below


// ------------------------------------------------------
// Heaps contain allocated blocks. Heaps are self-contained
// but share the (sub-process) memory in the arena's.
// ------------------------------------------------------

// A first-class heap.
typedef struct mi_heap_s mi_heap_t;       // heaps

// ------------------------------------------------------
// We can have sub-processes that are fully separated
// from each other (for running multiple Python interpreters
// for example). A sub-process holds the memory arenas and heaps.
// ------------------------------------------------------

// A sub-process
typedef struct mi_subproc_s mi_subproc_t;


// ---------------------------------------------------------------
// a memory id tracks the provenance of arena/OS allocated memory
// ---------------------------------------------------------------

// Memory can reside in arena's, direct OS allocated, meta-data pages, or statically allocated.
// The memid keeps track of this.
typedef enum mi_memkind_e {
  MI_MEM_NONE,      // not allocated (or static)
  MI_MEM_EXTERNAL,  // not owned by mimalloc but provided externally (via `mi_manage_os_memory` for example)
  MI_MEM_STATIC,    // allocated in a static area and should not be freed (the initial main theap data for example (`heap/init.c`))
  MI_MEM_OS,        // allocated from the OS
  MI_MEM_OS_HUGE,   // allocated as huge OS pages (usually 1GiB, pinned to physical memory)
  MI_MEM_OS_REMAP,  // allocated in a remapable area (i.e. using `mremap`)
  MI_MEM_ARENA,     // allocated from an arena (the usual case) (`arena/arena-alloc.c`)
  MI_MEM_MALLOC     // allocated with mi_malloc
} mi_memkind_t;

static inline bool mi_memkind_is_os(mi_memkind_t memkind) {
  return (memkind >= MI_MEM_OS && memkind <= MI_MEM_OS_REMAP);
}

static inline bool mi_memkind_needs_no_free(mi_memkind_t memkind) {
  return (memkind <= MI_MEM_STATIC);
}

typedef struct mi_meta_page_s mi_meta_page_t;

typedef struct mi_memid_os_info {
  void*         base;               // actual base address of the block (used for offset aligned allocations)
  size_t        size;               // allocated full size
  // size_t        alignment;       // alignment at allocation
} mi_memid_os_info_t;

typedef struct mi_memid_arena_info {
  mi_arena_t*   arena;              // arena that contains this memory
  uint32_t      slice_index;        // slice index in the arena
  uint32_t      slice_count;        // allocated slices
} mi_memid_arena_info_t;

typedef struct mi_memid_malloc_info {
  void*         base;               // returned pointer
  size_t        size;               // allocated size
} mi_memid_malloc_info_t;

typedef struct mi_memid_s {
  union {
    mi_memid_os_info_t     os;       // only used for MI_MEM_OS(_HUGE/_REMAP)
    mi_memid_arena_info_t  arena;    // only used for MI_MEM_ARENA
    mi_memid_malloc_info_t malloc;   // only used for MI_MEM_MALLOC
  } mem;
  mi_memkind_t  memkind;
  bool          is_pinned;          // `true` if we cannot decommit/reset/protect in this memory (e.g. when allocated using large (2Mib) or huge (1GiB) OS pages)
  bool          initially_committed;// `true` if the memory was originally allocated as committed
  bool          initially_zero;     // `true` if the memory was originally zero initialized
} mi_memid_t;

#define MI_MEMID_INIT(kind)   {{{NULL,0}}, kind, true /* pinned */, true /* committed */, false /* zero */ }
#define MI_MEMID_STATIC       MI_MEMID_INIT(MI_MEM_STATIC)

static inline bool mi_memid_is_os(mi_memid_t memid) {
  return mi_memkind_is_os(memid.memkind);
}

static inline bool mi_memid_needs_no_free(mi_memid_t memid) {
  return mi_memkind_needs_no_free(memid.memkind);
}

static inline mi_arena_t* mi_memid_arena(mi_memid_t memid) {
  return (memid.memkind == MI_MEM_ARENA ? memid.mem.arena.arena : NULL);
}

// defined in arena.c
mi_memid_t    _mi_memid_create(mi_memkind_t memkind);
mi_memid_t    _mi_memid_none(void);
mi_memid_t    _mi_memid_create_os(void* base, size_t size, bool committed, bool is_zero, bool is_large);
mi_memid_t    _mi_memid_create_static(void* p, size_t size);
mi_memid_t    _mi_memid_create_malloc(void* p, size_t size, bool iszero);
size_t        _mi_memid_size(mi_memid_t memid);


// ------------------------------------------------------
// Mimalloc pages contain allocated blocks
// ------------------------------------------------------

// The free lists use encoded next fields
// (Only actually encodes when MI_ENCODED_FREELIST is defined.)
typedef uintptr_t  mi_encoded_t;

// thread id's
typedef size_t     mi_threadid_t;

// free lists contain blocks
typedef struct mi_block_s {
  mi_encoded_t next;
} mi_block_t;


// The page flags are put in the bottom 2 bits of the thread_id (for a fast test in `mi_free`)
// If `has_interior_pointers` is true if the page has pointers at an offset in a block (so we have to unalign to the block start before free-ing)
// `in_full_queue` is true if the page is full and resides in the full queue (so we move it to a regular queue on free-ing)
#define MI_PAGE_IN_FULL_QUEUE           MI_ZU(0x01)
#define MI_PAGE_HAS_INTERIOR_POINTERS   MI_ZU(0x02)
#define MI_PAGE_FLAG_MASK               MI_ZU(0x03)
#define MI_PAGE_FLAG_BITS               (2)
typedef size_t mi_page_flags_t;

// There are two special threadid's: 0 for pages that are abandoned (and not in a theap queue),
// and 4 for abandoned & mapped threads -- abandoned-mapped pages are abandoned but also mapped
// in an arena (in `mi_heap_t.arena_pages.pages_abandoned`) so these can be quickly found for reuse.
// Abandoning partially used pages allows for sharing of this memory between threads (in particular if threads are blocked)
#define MI_THREADID_ABANDONED           MI_ZU(0)
#define MI_THREADID_ABANDONED_MAPPED    (MI_ZU(1) << MI_PAGE_FLAG_BITS)
#define MI_THREADID_DETACHED            (MI_ZU(2) << MI_PAGE_FLAG_BITS)

// Thread free list.
// Points to a list of blocks that are freed by other threads.
// The least-bit is set if the page is owned by the current thread. (`mi_page_is_owned`).
// Ownership is required before we can read any non-atomic fields in the page.
// This way we can push a block on the thread free list and try to claim ownership atomically in `alloc/free.c:mi_free_block_mt`.
typedef uintptr_t mi_thread_free_t;

// We store the currently used block count together with the total malloc call count as 16-bit numbers.
// This is done for better codegen `mi_malloc/mi_free` (where we can increment both at once as `used_alloc += 0x10001` for example).
// The `last_used` is the `used` count since the statistics are last updated; on 32-bit platforms this
// is a separate field in `mi_page_t` but on 64-bit we use the upper 32-bits to store it.
// We need the `alloc_count` and `last_used` to efficiently calculate allocation and free statistics even
// in a release build; this way we can update the stats in the slow path (`_mi_page_update_stats`).
typedef union mi_used_s {
  size_t      used_alloc;         // used + alloc_count
  // the following struct is unused but nice for debugging
  struct {
    uint16_t used_count;
    uint16_t alloc_count;
    #if MI_SIZE_SIZE >= 8
    uint16_t last_used;
    uint16_t last_alloc;
    #endif
  } le;  // little-endian layout
} mi_used_t;

static inline size_t mi_xused_used_count(mi_used_t xused)    { return (xused.used_alloc & 0xFFFF); }
static inline size_t mi_xused_alloc_count(mi_used_t xused)   { return ((xused.used_alloc>>16) & 0xFFFF); }
static inline mi_used_t mi_xused_used_reset(mi_used_t xused) { xused.used_alloc =  xused.used_alloc & ~0xFFFF; return xused; }

// A page contains blocks of one specific size (`block_size`).
// Each page has three list of free blocks:
// `free` for blocks that can be allocated,
// `local_free` for freed blocks that are not yet available to `mi_malloc`
// `thread_free` for freed blocks by other threads
// The `local_free` and `thread_free` lists are migrated to the `free` list
// when it is exhausted. The separate `local_free` list is necessary to
// implement a monotonic heartbeat. The `thread_free` list is needed for
// avoiding atomic operations when allocating from the owning thread.
//
// `used - |thread_free|` == actual blocks that are in use (alive)
// `used - |thread_free| + |free| + |local_free| == capacity`
//
// We don't count "freed" (as |free|) but use only the `used` field to reduce
// the number of memory accesses in the `mi_page_all_free` function(s).
// Use `_mi_page_free_collect` to collect the thread_free list and update the `used` count.
//
// Notes:
// - Non-atomic fields can only be accessed if having _ownership_ (low bit of `xthread_free` is 1).
//   Combining the `thread_free` list with an ownership bit allows a concurrent `free` to atomically
//   free an object and (re)claim ownership if the page was abandoned.
// - If a page is not part of a theap it is called "abandoned"  (`theap==NULL`) -- in
//   that case the `xthreadid` is 0 or 4 (4 is for abandoned pages that
//   are in the `pages_abandoned` lists of an arena, these are called "mapped" abandoned pages).
// - page flags are in the bottom 3 bits of `xthread_id` for the fast path in `mi_free`.
// - The layout below is optimized for `alloc/free.c:mi_free` and `alloc/alloc.c:mi_page_alloc` -- do not change it without careful consideration.

typedef struct mi_page_s {
  #if (MI_PAGE_META_IS_ALIGNED)
  _Atomic(struct mi_page_s*) self;             // points to the actual page info (for pages that span multiple slices)
  #endif
  _Atomic(mi_threadid_t)    xthread_id;        // thread this page belongs to. (= `theap->thread_id (or 0 or 4 if abandoned) | page_flags`)
  mi_block_t*               free;              // list of available free blocks (`malloc` allocates from this list)
  mi_block_t*               local_free;        // list of deferred free blocks by this thread (migrates to `free`)
  size_t                    block_size;        // const: size available in each block (always `>0`)
  mi_used_t                 xused;             // number of blocks in use (including blocks in `thread_free`) (and the allocated count for statistics)
  #if MI_SIZE_SIZE < 8
  uint16_t                  xlast_used;        // for statistics; on 64-bit platforms it is in bits 32..47 of xused.
  uint16_t                  xlast_alloc;       // for sampling; on 64-bit platforms it is in bits 48..63 of xused.
  #endif

  size_t                    page_offset;       // const: relative offset from the page to the start of the blocks
  uint16_t                  capacity;          // number of blocks committed
  uint16_t                  reserved;          // number of blocks reserved in memory
  uint16_t                  slice_pcommitted;  // committed size in OS page sizes relative to the first arena slice of the page data (or 0 if the page is fully committed already)
  uint8_t                   retire_expire;     // expiration count for retired blocks
  bool                      free_is_zero;      // `true` if the blocks in the free list are zero initialized

  // next cache line
  _Atomic(mi_thread_free_t) xthread_free;      // list of deferred free blocks freed by other threads (= `mi_block_t* | (1 if owned)`)
  mi_theap_t*               theap;             // the theap owning this page (may not be valid or NULL for abandoned pages)
  mi_heap_t*                heap;              // const: the heap owning this page

  struct mi_page_s*         next;              // next page owned by the theap with the same `block_size`
  struct mi_page_s*         prev;              // previous page owned by the theap with the same `block_size`
  mi_memid_t                memid;             // const: provenance of the page memory

  #if (MI_ENCODE_FREELIST || MI_PADDING)
  uintptr_t                 keys[MI_PAGE_KEY_COUNT]; // const: one or two random keys to encode the free lists (see `_mi_block_next`) or padding canary
  #endif
  #if MI_QSBR
  struct mi_page_s*         qsbr_next;
  struct mi_page_s*         qsbr_prev;
  bool                     qsbr_in_queue;
  mi_page_qsbr_t            qsbr;
  #endif
} mi_page_t;

// ------------------------------------------------------
// Object sizes
// ------------------------------------------------------

#define MI_PAGE_ALIGN                     MI_ARENA_SLICE_ALIGN      // page area's must be aligned on this for the page map.
#define MI_PAGE_MIN_START_BLOCK_ALIGN     MI_MAX_ALIGN_SIZE         // minimal block alignment for the first block in a page (16b)
#define MI_PAGE_MAX_START_BLOCK_ALIGN2    (4*MI_KiB)                // maximal block alignment for "power of 2"-sized blocks (such that we guarantee natural alignment)
#define MI_PAGE_OSPAGE_BLOCK_ALIGN2       (4*MI_KiB)                // also aligns any multiple of this size to avoid TLB misses.
#define MI_PAGE_MAX_OVERALLOC_ALIGN       MI_ARENA_SLICE_SIZE       // (64 KiB) limit for which we overallocate in arena pages, beyond this use OS allocation

// Offset the first block in pages with larger blocks (using the unused space at the end of the page) spread across cache sets (issue #1121).
#if !defined(MI_PAGE_CACHE_OFFSET)
#define MI_PAGE_CACHE_OFFSET              1
#endif
#define MI_PAGE_CACHE_OFFSET_UNIT         (128)                                                     // offset granularity (power of 2 between 16 and 4KiB)
#define MI_PAGE_CACHE_OFFSET_MAX          (MI_PAGE_OSPAGE_BLOCK_ALIGN2 - MI_PAGE_CACHE_OFFSET_UNIT) // maximal offset (32 possible offsets)
// Offset the first block in pages with small blocks (< 512)
#define MI_PAGE_SMALL_CACHE_OFFSET_UNIT   (64)
#define MI_PAGE_SMALL_CACHE_OFFSET_MAX    (7*MI_PAGE_SMALL_CACHE_OFFSET_UNIT)

// The max object sizes are intended to not waste more than ~ 12.5% internally over the page sizes.
#define MI_SMALL_MAX_OBJ_SIZE             ((MI_SMALL_PAGE_SIZE-MI_PAGE_OSPAGE_BLOCK_ALIGN2)/6)   // = 10 KiB
#if MI_ENABLE_LARGE_PAGES
#define MI_MEDIUM_MAX_OBJ_SIZE            ((MI_MEDIUM_PAGE_SIZE-MI_PAGE_OSPAGE_BLOCK_ALIGN2)/6)  // ~ 84 KiB (but see `alloc/page.h:mi_block_size_is_medium`)
#define MI_LARGE_MAX_OBJ_SIZE             (MI_LARGE_PAGE_SIZE/8)    // <= 512 KiB. note: this must be a nice power of 2 or we get rounding issues with `_mi_bin`
#else
#define MI_MEDIUM_MAX_OBJ_SIZE            (MI_MEDIUM_PAGE_SIZE/8)   // <= 64 KiB
#define MI_LARGE_MAX_OBJ_SIZE             MI_MEDIUM_MAX_OBJ_SIZE    // note: this must be a nice power of 2 or we get rounding issues with `_mi_bin`
#endif
#define MI_LARGE_MAX_OBJ_WSIZE            (MI_LARGE_MAX_OBJ_SIZE/MI_SIZE_SIZE)
#define MI_MAX_RECLAIM_OBJ_SIZE           (MI_LARGE_MAX_OBJ_SIZE/4) // only reclaim abandoned pages on free up to this block size

#if (MI_LARGE_MAX_OBJ_WSIZE >= 655360)
#error "mimalloc internal: define more bins"
#endif

// static invariant: MI_MAX_SINGLETON_BIN >= _mi_bin(MI_LARGE_MAX_OBJ_SIZE) (See heap/init.c for the size bins)
#if (MI_LARGE_MAX_OBJ_WSIZE <= 8192)     // 64 KiB
#define MI_MAX_SINGLETON_BIN   (48)
#elif (MI_LARGE_MAX_OBJ_WSIZE <= 32768)  // 256KiB
#define MI_MAX_SINGLETON_BIN   (56)
#elif (MI_LARGE_MAX_OBJ_WSIZE <= 65536)  // 512KiB
#define MI_MAX_SINGLETON_BIN   (60)
#else
#define MI_MAX_SINGLETON_BIN   MI_BIN_HUGE
#endif

// The small object max size must be larger than twice the small size max minus one,
// such that any aligned allocation with `alignment <= size <= MI_SMALL_SIZE_MAX` will
// be allocated in a small page (so `mi_free_csize` can delegate correctly to `mi_free_small`.
#if MI_SMALL_MAX_OBJ_SIZE <= (2*(MI_SMALL_WSIZE_MAX * MI_SIZE_SIZE)-1)
#error "mimalloc internal: the small object max size is too small"
#endif

// ------------------------------------------------------
// Page kinds
// ------------------------------------------------------

typedef enum mi_page_kind_e {
  MI_PAGE_SMALL,      // small blocks go into 64KiB pages
  MI_PAGE_MEDIUM,     // medium blocks go into 512KiB pages
  MI_PAGE_LARGE,      // larger blocks go into 4MiB pages (if `MI_ENABLE_LARGE_PAGES==1`)
  MI_PAGE_SINGLETON   // page containing a single block.
                      // used for blocks `> MI_LARGE_MAX_OBJ_SIZE` or an alignment `> MI_PAGE_MAX_OVERALLOC_ALIGN`.
} mi_page_kind_t;



// ------------------------------------------------------
// A "theap" is a thread local heap which owns pages.
// (making them thread-local avoids atomic operations)
//
// All theaps belong to a (non-thread-local) heap.
// A theap just owns a set of pages for allocation and
// can only be allocate/reallocate from the thread that created it.
// Freeing blocks can be done from any thread though.
//
// Per thread, there is always a default theap that belongs
// to the default heap. It is initialized to statically
// point initially to an empty theap to avoid initialization
// checks in the fast path.
// ------------------------------------------------------

// Thread local data
typedef struct mi_tld_s mi_tld_t;   // defined below

// Pages of a certain block size are held in a queue.
typedef struct mi_page_queue_s {
  mi_page_t* first;
  mi_page_t* last;
  size_t     count;
  size_t     block_size;
  size_t     xcollect_score;          // how often full pages here revived by cross-thread frees
} mi_page_queue_t;

// Random context
typedef struct mi_random_cxt_s {
  uint32_t input[16];
  uint32_t output[16];
  int      output_available;
  bool     weak;
} mi_random_ctx_t;


// In debug mode there is a padding structure at the end of the blocks to check for buffer overflows
#if MI_PADDING
typedef struct mi_padding_s {
  uint32_t canary; // encoded block value to check validity of the padding (in case of overflow)
  uint32_t delta;  // padding bytes before the block. (mi_full_usable_size(p) - delta == exact allocated bytes)
} mi_padding_t;
#define MI_PADDING_SIZE   (sizeof(mi_padding_t))
#define MI_PADDING_WSIZE  ((MI_PADDING_SIZE + MI_SIZE_SIZE - 1) / MI_SIZE_SIZE)
#else
#define MI_PADDING_SIZE   0
#define MI_PADDING_WSIZE  0
#endif

#define MI_PAGES_DIRECT   (MI_SMALL_WSIZE_MAX + MI_PADDING_WSIZE + 1)


// A thread-local heap ("theap") owns a set of thread-local pages.
struct mi_theap_s {
  // put in front for fast small allocations
  mi_page_t*            pages_free_direct[MI_PAGES_DIRECT];  // optimize: array where every entry points a page with possibly free blocks in the corresponding queue for that size.

  // less frequently accessed fields
  mi_tld_t*             tld;                                 // thread-local data
  _Atomic(mi_heap_t*)   heap;                                // the heap this theap belongs to.
  _Atomic(mi_subproc_t*)subproc;                             // subproc this belongs too (always `subproc == heap->subproc` but needed for safe destruction)
  _Atomic(size_t)       refcount;                            // reference count (needed for safe heap destroy)

  // config
  long                  page_full_retain;                    // how many full pages can be retained per queue (before abandoning them)
  bool                  allow_page_reclaim;                  // `true` if this theap can reclaim abandoned pages
  bool                  allow_page_abandon;                  // `true` if this theap can abandon pages to reduce memory footprint
  bool                  is_detached;                         // `true` if `tld->thread_id == MI_THREADID_DETACHED`
  bool                  profile_never;                       // if `true`, this theap is never profiled (only the empty and meta theaps)

  // sampling
  size_t                sample_countdown;                    // sample countdown in requested bytes (don't change the field order; see `internal.h:mi_theap_get_free_small_page`)
  size_t                sample_rate;                         // current sampling rate in requested bytes (or 0 to disable) (for profiling and guarded mode)
  uint64_t              sample_requested;                    // total allocated/requested bytes since the last sample
  size_t                profile_sample_rate;                 // sampling rate in requested bytes for profiling
  size_t                profile_sample_countdown;            // countdown in requested bytes for profiling
  size_t                guarded_sample_rate;                 // sampling rate in requested bytes for guarded objects
  size_t                guarded_sample_countdown;            // countdown in requested bytes for guarded objects
  size_t                guarded_size_min;                    // minimal size for guarded objects
  size_t                guarded_size_max;                    // maximal size for guarded objects
  uintptr_t             profiler_reserved1;                  // two fields available for a profiler (to avoid thread locals)
  uintptr_t             profiler_reserved2;

  // stats
  unsigned long long    heartbeat;                           // monotonic heartbeat count
  mi_random_ctx_t       random;                              // random number context used for secure allocation
  size_t                page_count;                          // total number of pages in the `pages` queues.
  size_t                page_count_os;                       // number of non-arena pages in the `pages` queues
  size_t                page_retired_min;                    // smallest retired index (retired pages are fully free, but still in the page queues)
  size_t                page_retired_max;                    // largest retired index into the `pages` array.
  size_t                pages_full_size;                     // optimization: total size of blocks in the pages of the full queue (issue #1220)
  long                  generic_count;                       // how often is `_mi_malloc_generic` called?
  long                  generic_collect_count;               // how often is `_mi_malloc_generic` called without collecting?

  // theaps belong to heaps and threads
  mi_theap_t*           tnext;                               // list of theaps in this thread
  mi_theap_t*           tprev;
  mi_theap_t*           hnext;                               // list of theaps of the owning `heap`
  mi_theap_t*           hprev;

  // page queues
  mi_page_queue_t       pages[MI_BIN_COUNT];                 // queue of pages for each size class (or "bin")
  mi_memid_t            memid;                               // provenance of the theap struct itself (meta or os)
  mi_stats_t            stats;                               // thread-local statistics
  #if MI_QSBR
  mi_page_t*           qsbr_first;
  mi_page_t*           qsbr_last;
  #endif
};




// ------------------------------------------------------
// Heaps contain allocated blocks. Heaps are self-contained
// but share the (sub-process) memory in the arena's.
// ------------------------------------------------------

// Keep track of all owned and abandoned pages in the arena's
struct mi_arena_pages_s;
typedef struct mi_arena_pages_s mi_arena_pages_t;

#define MI_MAX_ARENAS   (160)   // Limited for now (and takes up .bss).. but arena's scale up exponentially (see `_mi_arena_reserve`)
                                // 160 arenas is enough for ~2 TiB memory

// A dynamic thread-local variable; 0 for an invalid thread-local
typedef size_t mi_thread_local_t;

typedef struct mi_heap_s {
  mi_subproc_t*         subproc;                        // a heap belongs to a subprocess
  size_t                heap_seq;                       // unique sequence number for heaps in this subprocess
  mi_heap_t*            next;                           // list of heaps in this subprocess
  mi_heap_t*            prev;
  mi_thread_local_t     theap;                          // dynamic thread local for the thread-local theaps of this heap

  mi_arena_t*           exclusive_arena;                // if the heap should only allocate from a specific arena (or NULL)
  long                  reclaim_on_free;                // reclaim policy captured at heap initialization
  int                   numa_node;                      // if >=0, prefer this numa node for allocations
  #if MI_QSBR
  bool                  qsbr_enabled;                   // immutable after heap initialization
  #endif
  _Atomic(size_t)       debug_fill_preserve;            // in debug mode, the first N bytes of a block are not filled with MI_DEBUG_UNINIT/FREED

  mi_theap_t*           theaps;                         // list of all thread-local theaps belonging to this heap (using the `hnext`/`hprev` fields)
  mi_lock_t             theaps_lock;                    // protects the theaps list and serializes heap walks with thread-exit abandonment

  _Atomic(mi_profiler_t*) profiler;

  _Atomic(size_t)       abandoned_count[MI_BIN_COUNT];  // total count of abandoned pages in this heap
  mi_page_t*            os_abandoned_pages;             // list of pages that are OS allocated and not in an arena
  mi_lock_t             os_abandoned_pages_lock;        // lock for the os abandoned pages list (this lock protects list operations)

  _Atomic(mi_arena_pages_t*) arena_pages[MI_MAX_ARENAS]; // track owned and abandoned pages in the arenas (entries can be NULL)
  mi_lock_t             arena_pages_lock;                // lock to update the arena_pages array
  mi_memid_t            memid;                           // provenance of the heap memory
  mi_stats_t            stats;                           // statistics for this heap; periodically updated by merging from each theap
  #if MI_QSBR
  mi_heap_qsbr_t        qsbr;
  #endif
} mi_heap_t;


// ------------------------------------------------------
// Sub processes do not reclaim or visit pages from other sub processes.
// These are essentially the static variables of a process, and
// usually there is only one subprocess. This can be used for example
// by CPython to have separate interpreters within one process.
// Each thread can only belong to one subprocess
// (and needs to call `mi_subproc_add_current_thread` before any allocations).
// ------------------------------------------------------

struct mi_subproc_s {
  size_t                subproc_seq;                    // unique id for sub-processes
  mi_subproc_t*         next;                           // list of all sub-processes
  mi_subproc_t*         prev;
  _Atomic(mi_meta_page_t*) meta_pages;                  // meta data pages

  _Atomic(size_t)       arena_count;                    // current count of arena's
  _Atomic(mi_arena_t*)  arenas[MI_MAX_ARENAS];          // arena's of this sub-process
  mi_lock_t             arena_reserve_lock;             // lock to ensure arena's get reserved one at a time
  mi_decl_align(8)                                      // needed on some 32-bit platforms
  _Atomic(int64_t)      purge_expire;                   // expiration is set if any arenas can be purged

  _Atomic(mi_heap_t*)   heap_main;                      // main heap for this sub process
  mi_heap_t*            heaps;                          // heaps belonging to this sub-process
  mi_lock_t             heaps_lock;

  mi_theap_t*           theap_meta;                     // detached theap for allocating meta-data
  mi_lock_t             theap_meta_lock;                // all allocations in theap_meta need a lock

  _Atomic(size_t)       thread_count;                   // current threads associated with this sub-process
  _Atomic(size_t)       thread_total_count;             // total created threads associated with this sub-process
  _Atomic(size_t)       heap_count;                     // current heaps in this sub-process (== |heaps|)
  _Atomic(size_t)       heap_total_count;               // total created heaps in this sub-process

  _Atomic(mi_profiler_t*) profiler;

  mi_memid_t            memid;                          // provenance of this memory block (meta or static)
  mi_subproc_t*         parent;                         // subproc in which this one was allocated
  mi_decl_align(8)                                      // needed on some 32-bit platforms
  mi_stats_t            stats;                          // subprocess statistics; updated for arena/OS stats like committed,
                                                        // and otherwise merged with heap stats when those are deleted
};


// ------------------------------------------------------
// Thread Local data
// ------------------------------------------------------

// Milliseconds as in `int64_t` to avoid overflows
typedef int64_t  mi_msecs_t;

// Thread local data
struct mi_tld_s {
  mi_threadid_t         thread_id;            // thread id of this thread
  size_t                thread_seq;           // thread sequence id (linear count of created threads)
  int                   numa_node;            // thread preferred numa node
  mi_subproc_t*         subproc;              // sub-process this thread belongs to.
  mi_theap_t*           theaps;               // list of theaps in this thread (so we can abandon all when the thread terminates)
  mi_lock_t             theaps_lock;          // lock as the theaps list is sometimes accessed from another thread (on `mi_heap_free`)
  bool                  recurse;              // true if deferred was called; used to prevent infinite recursion.
  bool                  in_profiler;          // true while in a profiler `on_alloc` callback; nested allocations are not sampled
  bool                  is_in_threadpool;     // true if this thread is part of a threadpool (and can run arbitrary tasks)
  mi_memid_t            memid;                // provenance of the tld memory itself (meta or OS)
};


/* ----------------------------------------------------------------------------
  Arenas are fixed area's of OS memory from which we can allocate
  large blocks (>= MI_ARENA_MIN_BLOCK_SIZE).
  In contrast to the rest of mimalloc, the arenas are shared between
  threads and need to be accessed using atomic operations (using atomic `mi_bitmap_t`'s).

  Arenas are also used to for huge OS page (1GiB) reservations or for reserving
  OS memory upfront which can be improve performance or is sometimes needed
  on embedded devices. We can also employ this with WASI or `sbrk` systems
  to reserve large arenas upfront and be able to reuse the memory more effectively.
-----------------------------------------------------------------------------*/

#define MI_ARENA_BIN_COUNT      (MI_MAX_SINGLETON_BIN+1)
#define MI_ARENA_MIN_SIZE       (MI_BCHUNK_BITS * MI_ARENA_SLICE_SIZE)           // 32 MiB (or 8 MiB on 32-bit)
#define MI_ARENA_MAX_SIZE       (MI_BITMAP_MAX_BIT_COUNT * MI_ARENA_SLICE_SIZE)  // 16 GiB

typedef struct mi_bitmap_s  mi_bitmap_t;    // atomic bitmap  (defined in `src/arena/bitmap.h`)
typedef struct mi_bbitmap_s mi_bbitmap_t;   // atomic binned bitmap (defined in `src/arena/bitmap.h`)

struct mi_arena_pages_s {
  mi_bitmap_t* pages;                // all registered pages (abandoned and owned)
  mi_bitmap_t* pages_abandoned[MI_ARENA_BIN_COUNT];  // abandoned pages per size bin (a set bit means the start of the page)
  // followed by the bitmaps (whose siz`es depend on the arena size)
};


// A memory arena
typedef struct mi_arena_s {
  mi_memid_t          memid;                // provenance of the memory area
  mi_subproc_t*       subproc;              // subprocess this arena belongs to (`this 'element-of' this->subproc->arenas`)
  size_t              arena_idx;            // index in the arenas array
  void*               start;                // actual start of the arena area (the arena_s info may come later due to guard pages etc.)

  size_t              slice_count;          // total size of the area in arena slices (of `MI_ARENA_SLICE_SIZE`)
  size_t              info_slices;          // initial slices reserved for the arena bitmaps
  int                 numa_node;            // associated NUMA node
  bool                is_exclusive;         // only allow allocations if specifically for this arena
  mi_decl_align(8)                          // needed on some 32-bit platforms
  _Atomic(mi_msecs_t) purge_expire;         // expiration time when slices can be purged from `slices_purge`.
  mi_commit_fun_t*    commit_fun;           // custom commit/decommit memory
  void*               commit_fun_arg;       // user argument for a custom commit function

  size_t              total_size;           // for (user given) memory more than MI_ARENA_MAX_SIZE, we use N arena's to cover it. The first (parent) has the total size (and the other sub-arena's 0).
  mi_arena_t*         parent;               // if this is a sub arena, this points to the first one in the memory area.

  mi_bbitmap_t*       slices_free;          // is the slice free? (a binned bitmap with size classes)
  mi_bitmap_t*        slices_committed;     // is the slice committed? (i.e. accessible)
  mi_bitmap_t*        slices_dirty;         // is the slice potentially non-zero?
  mi_bitmap_t*        slices_purge;         // slices that can be purged
  mi_page_t*          pages_meta;           // pre-allocated `slice_count` page meta info -- only used if `MI_PAGE_META_IS_SEPARATED!=0`
  mi_arena_pages_t    pages_main;           // arena page bitmaps for the main heap are allocated up front as well

  // followed by the bitmaps (whose sizes depend on the arena size)
  // note: when adding bitmaps revise `mi_arena_info_slices_needed`
} mi_arena_t;



/* -----------------------------------------------------------
  Error codes passed to `_mi_fatal_error`
  All are recoverable but EFAULT is a serious error and aborts by default in secure mode.
  For portability define undefined error codes using common Unix codes:
  <https://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html>
----------------------------------------------------------- */

#ifndef EAGAIN         // double free
#define EAGAIN (11)
#endif
#ifndef ENOMEM         // out of memory
#define ENOMEM (12)
#endif
#ifndef EFAULT         // corrupted free-list or meta-data
#define EFAULT (14)
#endif
#ifndef EINVAL         // trying to free an invalid pointer
#define EINVAL (22)
#endif
#ifndef EOVERFLOW      // count*size overflow
#define EOVERFLOW (75)
#endif
#ifndef ENOENT         // environment variable not found
#define ENOENT (2)
#endif


/* -----------------------------------------------------------
  Debug constants
----------------------------------------------------------- */

#if !defined(MI_DEBUG_UNINIT)
#define MI_DEBUG_UNINIT     (0xD0)
#endif
#if !defined(MI_DEBUG_FREED)
#define MI_DEBUG_FREED      (0xDF)
#endif
#if !defined(MI_DEBUG_PADDING)
#define MI_DEBUG_PADDING    (0xDE)
#endif


#endif // MI_TYPES_H
