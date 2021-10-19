/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MIMALLOC_TYPES_H
#define MIMALLOC_TYPES_H

#include <stddef.h>   // ptrdiff_t
#include <stdint.h>   // uintptr_t, uint16_t, etc
#include "mimalloc-atomic.h"  // _Atomic

#ifdef _MSC_VER
#pragma warning(disable:4214) // bitfield is not int
#endif 

// Minimal alignment necessary. On most platforms 16 bytes are needed
// due to SSE registers for example. This must be at least `MI_INTPTR_SIZE`
#ifndef MI_MAX_ALIGN_SIZE
#define MI_MAX_ALIGN_SIZE  16   // sizeof(max_align_t)
#endif

// ------------------------------------------------------
// Variants
// ------------------------------------------------------

// Define NDEBUG in the release version to disable assertions.
// #define NDEBUG

// Define MI_STAT as 1 to maintain statistics; set it to 2 to have detailed statistics (but costs some performance).
// #define MI_STAT 1

// Define MI_SECURE to enable security mitigations
// #define MI_SECURE 1  // guard page around metadata
// #define MI_SECURE 2  // guard page around each mimalloc page
// #define MI_SECURE 3  // encode free lists (detect corrupted free list (buffer overflow), and invalid pointer free)
// #define MI_SECURE 4  // checks for double free. (may be more expensive)

#if !defined(MI_SECURE)
#define MI_SECURE 0
#endif

// Define MI_DEBUG for debug mode
// #define MI_DEBUG 1  // basic assertion checks and statistics, check double free, corrupted free list, and invalid pointer free.
// #define MI_DEBUG 2  // + internal assertion checks
// #define MI_DEBUG 3  // + extensive internal invariant checking (cmake -DMI_DEBUG_FULL=ON)
#if !defined(MI_DEBUG)
#if !defined(NDEBUG) || defined(_DEBUG)
#define MI_DEBUG 2
#else
#define MI_DEBUG 0
#endif
#endif

// Reserve extra padding at the end of each block to be more resilient against heap block overflows.
// The padding can detect byte-precise buffer overflow on free.
#if !defined(MI_PADDING) && (MI_DEBUG>=1)
#define MI_PADDING  1
#endif


// Encoded free lists allow detection of corrupted free lists
// and can detect buffer overflows, modify after free, and double `free`s.
#if (MI_SECURE>=3 || MI_DEBUG>=1 || MI_PADDING > 0)
#define MI_ENCODE_FREELIST  1
#endif

// ------------------------------------------------------
// Platform specific values
// ------------------------------------------------------

// ------------------------------------------------------
// Size of a pointer.
// We assume that `sizeof(void*)==sizeof(intptr_t)`
// and it holds for all platforms we know of.
//
// However, the C standard only requires that:
//  p == (void*)((intptr_t)p))
// but we also need:
//  i == (intptr_t)((void*)i)
// or otherwise one might define an intptr_t type that is larger than a pointer...
// ------------------------------------------------------

#if INTPTR_MAX == 9223372036854775807LL
# define MI_INTPTR_SHIFT (3)
#elif INTPTR_MAX == 2147483647LL
# define MI_INTPTR_SHIFT (2)
#else
#error platform must be 32 or 64 bits
#endif

#define MI_INTPTR_SIZE  (1<<MI_INTPTR_SHIFT)
#define MI_INTPTR_BITS  (MI_INTPTR_SIZE*8)

#define KiB     ((size_t)1024)
#define MiB     (KiB*KiB)
#define GiB     (MiB*KiB)


// ------------------------------------------------------
// Main internal data-structures
// ------------------------------------------------------

// Main tuning parameters for segment and page sizes
// Sizes for 64-bit, divide by two for 32-bit
#define MI_SMALL_PAGE_SHIFT               (13 + MI_INTPTR_SHIFT)      // 64kb
#define MI_MEDIUM_PAGE_SHIFT              ( 3 + MI_SMALL_PAGE_SHIFT)  // 512kb
#define MI_LARGE_PAGE_SHIFT               ( 3 + MI_MEDIUM_PAGE_SHIFT) // 4mb
#define MI_SEGMENT_SHIFT                  ( MI_LARGE_PAGE_SHIFT)      // 4mb

// Derived constants
#define MI_SEGMENT_SIZE                   (1UL<<MI_SEGMENT_SHIFT)
#define MI_SEGMENT_MASK                   ((uintptr_t)MI_SEGMENT_SIZE - 1)

#define MI_SMALL_PAGE_SIZE                (1UL<<MI_SMALL_PAGE_SHIFT)
#define MI_MEDIUM_PAGE_SIZE               (1UL<<MI_MEDIUM_PAGE_SHIFT)
#define MI_LARGE_PAGE_SIZE                (1UL<<MI_LARGE_PAGE_SHIFT)

#define MI_SMALL_PAGES_PER_SEGMENT        (MI_SEGMENT_SIZE/MI_SMALL_PAGE_SIZE)
#define MI_MEDIUM_PAGES_PER_SEGMENT       (MI_SEGMENT_SIZE/MI_MEDIUM_PAGE_SIZE)
#define MI_LARGE_PAGES_PER_SEGMENT        (MI_SEGMENT_SIZE/MI_LARGE_PAGE_SIZE)

// The max object size are checked to not waste more than 12.5% internally over the page sizes.
// (Except for large pages since huge objects are allocated in 4MiB chunks)
#define MI_SMALL_OBJ_SIZE_MAX             (MI_SMALL_PAGE_SIZE/4)   // 16kb
#define MI_MEDIUM_OBJ_SIZE_MAX            (MI_MEDIUM_PAGE_SIZE/4)  // 128kb
#define MI_LARGE_OBJ_SIZE_MAX             (MI_LARGE_PAGE_SIZE/2)   // 2mb
#define MI_LARGE_OBJ_WSIZE_MAX            (MI_LARGE_OBJ_SIZE_MAX/MI_INTPTR_SIZE)
#define MI_HUGE_OBJ_SIZE_MAX              (2*MI_INTPTR_SIZE*MI_SEGMENT_SIZE)        // (must match MI_REGION_MAX_ALLOC_SIZE in memory.c)

// Maximum number of size classes. (spaced exponentially in 12.5% increments)
#define MI_BIN_HUGE  (73U)

#if (MI_LARGE_OBJ_WSIZE_MAX >= 655360)
#error "define more bins"
#endif

// Used as a special value to encode block sizes in 32 bits.
#define MI_HUGE_BLOCK_SIZE   ((uint32_t)MI_HUGE_OBJ_SIZE_MAX)

// The free lists use encoded next fields
// (Only actually encodes when MI_ENCODED_FREELIST is defined.)
typedef uintptr_t mi_encoded_t;

// free lists contain blocks
typedef struct mi_block_s {
  mi_encoded_t next;
} mi_block_t;


// The delayed flags are used for efficient multi-threaded free-ing
typedef enum mi_delayed_e {
  MI_USE_DELAYED_FREE   = 0, // push on the owning heap thread delayed list
  MI_DELAYED_FREEING    = 1, // temporary: another thread is accessing the owning heap
  MI_NO_DELAYED_FREE    = 2, // optimize: push on page local thread free queue if another block is already in the heap thread delayed free list
  MI_NEVER_DELAYED_FREE = 3  // sticky, only resets on page reclaim
} mi_delayed_t;


// The `in_full` and `has_aligned` page flags are put in a union to efficiently
// test if both are false (`full_aligned == 0`) in the `mi_free` routine.
#if !MI_TSAN
typedef union mi_page_flags_s {
  uint8_t full_aligned;
  struct {
    uint8_t in_full : 1;
    uint8_t has_aligned : 1;
  } x;
} mi_page_flags_t;
#else
// under thread sanitizer, use a byte for each flag to suppress warning, issue #130
typedef union mi_page_flags_s {
  uint16_t full_aligned;
  struct {
    uint8_t in_full;
    uint8_t has_aligned;
  } x;
} mi_page_flags_t;
#endif

// Thread free list.
// We use the bottom 2 bits of the pointer for mi_delayed_t flags
typedef uintptr_t mi_thread_free_t;

// A page contains blocks of one specific size (`block_size`).
// Each page has three list of free blocks:
// `free` for blocks that can be allocated,
// `local_free` for freed blocks that are not yet available to `mi_malloc`
// `thread_free` for freed blocks by other threads
// The `local_free` and `thread_free` lists are migrated to the `free` list
// when it is exhausted. The separate `local_free` list is necessary to
// implement a monotonic heartbeat. The `thread_free` list is needed for
// avoiding atomic operations in the common case.
//
//
// `used - |thread_free|` == actual blocks that are in use (alive)
// `used - |thread_free| + |free| + |local_free| == capacity`
//
// We don't count `freed` (as |free|) but use `used` to reduce
// the number of memory accesses in the `mi_page_all_free` function(s).
//
// Notes: 
// - Access is optimized for `mi_free` and `mi_page_alloc` (in `alloc.c`)
// - Using `uint16_t` does not seem to slow things down
// - The size is 8 words on 64-bit which helps the page index calculations
//   (and 10 words on 32-bit, and encoded free lists add 2 words. Sizes 10 
//    and 12 are still good for address calculation)
// - To limit the structure size, the `xblock_size` is 32-bits only; for 
//   blocks > MI_HUGE_BLOCK_SIZE the size is determined from the segment page size
// - `thread_free` uses the bottom bits as a delayed-free flags to optimize
//   concurrent frees where only the first concurrent free adds to the owning
//   heap `thread_delayed_free` list (see `alloc.c:mi_free_block_mt`).
//   The invariant is that no-delayed-free is only set if there is
//   at least one block that will be added, or as already been added, to 
//   the owning heap `thread_delayed_free` list. This guarantees that pages
//   will be freed correctly even if only other threads free blocks.
typedef struct mi_page_s {
  // "owned" by the segment
  uint8_t               segment_idx;       // index in the segment `pages` array, `page == &segment->pages[page->segment_idx]`
  uint8_t               segment_in_use:1;  // `true` if the segment allocated this page
  uint8_t               is_reset:1;        // `true` if the page memory was reset
  uint8_t               is_committed:1;    // `true` if the page virtual memory is committed
  uint8_t               is_zero_init:1;    // `true` if the page was zero initialized

  // layout like this to optimize access in `mi_malloc` and `mi_free`
  uint16_t              capacity;          // number of blocks committed, must be the first field, see `segment.c:page_clear`
  uint16_t              reserved;          // number of blocks reserved in memory
  mi_page_flags_t       flags;             // `in_full` and `has_aligned` flags (8 bits)
  uint8_t               is_zero:1;         // `true` if the blocks in the free list are zero initialized
  uint8_t               retire_expire:7;   // expiration count for retired blocks

  mi_block_t*           free;              // list of available free blocks (`malloc` allocates from this list)
  #ifdef MI_ENCODE_FREELIST
  uintptr_t             keys[2];           // two random keys to encode the free lists (see `_mi_block_next`)
  #endif
  uint32_t              used;              // number of blocks in use (including blocks in `local_free` and `thread_free`)
  uint32_t              xblock_size;       // size available in each block (always `>0`) 

  mi_block_t*           local_free;        // list of deferred free blocks by this thread (migrates to `free`)
  _Atomic(mi_thread_free_t) xthread_free;  // list of deferred free blocks freed by other threads
  _Atomic(uintptr_t)        xheap;
  
  struct mi_page_s*     next;              // next page owned by this thread with the same `block_size`
  struct mi_page_s*     prev;              // previous page owned by this thread with the same `block_size`
} mi_page_t;



typedef enum mi_page_kind_e {
  MI_PAGE_SMALL,    // small blocks go into 64kb pages inside a segment
  MI_PAGE_MEDIUM,   // medium blocks go into 512kb pages inside a segment
  MI_PAGE_LARGE,    // larger blocks go into a single page spanning a whole segment
  MI_PAGE_HUGE      // huge blocks (>512kb) are put into a single page in a segment of the exact size (but still 2mb aligned)
} mi_page_kind_t;

// Segments are large allocated memory blocks (2mb on 64 bit) from
// the OS. Inside segments we allocated fixed size _pages_ that
// contain blocks.
typedef struct mi_segment_s {
  // memory fields
  size_t               memid;            // id for the os-level memory manager
  bool                 mem_is_pinned;    // `true` if we cannot decommit/reset/protect in this memory (i.e. when allocated using large OS pages)
  bool                 mem_is_committed; // `true` if the whole segment is eagerly committed  

  // segment fields
  _Atomic(struct mi_segment_s*) abandoned_next;
  struct mi_segment_s* next;             // must be the first segment field after abandoned_next -- see `segment.c:segment_init`
  struct mi_segment_s* prev;

  size_t               abandoned;        // abandoned pages (i.e. the original owning thread stopped) (`abandoned <= used`)
  size_t               abandoned_visits; // count how often this segment is visited in the abandoned list (to force reclaim if it is too long)

  size_t               used;             // count of pages in use (`used <= capacity`)
  size_t               capacity;         // count of available pages (`#free + used`)
  size_t               segment_size;     // for huge pages this may be different from `MI_SEGMENT_SIZE`
  size_t               segment_info_size;// space we are using from the first page for segment meta-data and possible guard pages.
  uintptr_t            cookie;           // verify addresses in secure mode: `_mi_ptr_cookie(segment) == segment->cookie`

  // layout like this to optimize access in `mi_free`
  size_t               page_shift;       // `1 << page_shift` == the page sizes == `page->block_size * page->reserved` (unless the first page, then `-segment_info_size`).
  _Atomic(uintptr_t)   thread_id;        // unique id of the thread owning this segment
  mi_page_kind_t       page_kind;        // kind of pages: small, large, or huge
  mi_page_t            pages[1];         // up to `MI_SMALL_PAGES_PER_SEGMENT` pages
} mi_segment_t;


// ------------------------------------------------------
// Heaps
// Provide first-class heaps to allocate from.
// A heap just owns a set of pages for allocation and
// can only be allocate/reallocate from the thread that created it.
// Freeing blocks can be done from any thread though.
// Per thread, the segments are shared among its heaps.
// Per thread, there is always a default heap that is
// used for allocation; it is initialized to statically
// point to an empty heap to avoid initialization checks
// in the fast path.
// ------------------------------------------------------

// Thread local data
typedef struct mi_tld_s mi_tld_t;

// Pages of a certain block size are held in a queue.
typedef struct mi_page_queue_s {
  mi_page_t* first;
  mi_page_t* last;
  size_t     block_size;
} mi_page_queue_t;

#define MI_BIN_FULL  (MI_BIN_HUGE+1)

// Random context
typedef struct mi_random_cxt_s {
  uint32_t input[16];
  uint32_t output[16];
  int      output_available;
} mi_random_ctx_t;


// In debug mode there is a padding stucture at the end of the blocks to check for buffer overflows
#if (MI_PADDING)
typedef struct mi_padding_s {
  uint32_t canary; // encoded block value to check validity of the padding (in case of overflow)
  uint32_t delta;  // padding bytes before the block. (mi_usable_size(p) - delta == exact allocated bytes)
} mi_padding_t;
#define MI_PADDING_SIZE   (sizeof(mi_padding_t))
#define MI_PADDING_WSIZE  ((MI_PADDING_SIZE + MI_INTPTR_SIZE - 1) / MI_INTPTR_SIZE)
#else
#define MI_PADDING_SIZE   0
#define MI_PADDING_WSIZE  0
#endif

#define MI_PAGES_DIRECT   (MI_SMALL_WSIZE_MAX + MI_PADDING_WSIZE + 1)


// A heap owns a set of pages.
struct mi_heap_s {
  mi_tld_t*             tld;
  mi_page_t*            pages_free_direct[MI_PAGES_DIRECT];  // optimize: array where every entry points a page with possibly free blocks in the corresponding queue for that size.
  mi_page_queue_t       pages[MI_BIN_FULL + 1];              // queue of pages for each size class (or "bin")
  _Atomic(mi_block_t*)  thread_delayed_free;
  uintptr_t             thread_id;                           // thread this heap belongs too
  uintptr_t             cookie;                              // random cookie to verify pointers (see `_mi_ptr_cookie`)
  uintptr_t             keys[2];                             // two random keys used to encode the `thread_delayed_free` list
  mi_random_ctx_t       random;                              // random number context used for secure allocation
  size_t                page_count;                          // total number of pages in the `pages` queues.
  size_t                page_retired_min;                    // smallest retired index (retired pages are fully free, but still in the page queues)
  size_t                page_retired_max;                    // largest retired index into the `pages` array.
  mi_heap_t*            next;                                // list of heaps per thread
  bool                  no_reclaim;                          // `true` if this heap should not reclaim abandoned pages
};



// ------------------------------------------------------
// Debug
// ------------------------------------------------------

#define MI_DEBUG_UNINIT     (0xD0)
#define MI_DEBUG_FREED      (0xDF)
#define MI_DEBUG_PADDING    (0xDE)

#if (MI_DEBUG)
// use our own assertion to print without memory allocation
void _mi_assert_fail(const char* assertion, const char* fname, unsigned int line, const char* func );
#define mi_assert(expr)     ((expr) ? (void)0 : _mi_assert_fail(#expr,__FILE__,__LINE__,__func__))
#else
#define mi_assert(x)
#endif

#if (MI_DEBUG>1)
#define mi_assert_internal    mi_assert
#else
#define mi_assert_internal(x)
#endif

#if (MI_DEBUG>2)
#define mi_assert_expensive   mi_assert
#else
#define mi_assert_expensive(x)
#endif

// ------------------------------------------------------
// Statistics
// ------------------------------------------------------

#ifndef MI_STAT
#if (MI_DEBUG>0)
#define MI_STAT 2
#else
#define MI_STAT 0
#endif
#endif

typedef struct mi_stat_count_s {
  int64_t allocated;
  int64_t freed;
  int64_t peak;
  int64_t current;
} mi_stat_count_t;

typedef struct mi_stat_counter_s {
  int64_t total;
  int64_t count;
} mi_stat_counter_t;

typedef struct mi_stats_s {
  mi_stat_count_t segments;
  mi_stat_count_t pages;
  mi_stat_count_t reserved;
  mi_stat_count_t committed;
  mi_stat_count_t reset;
  mi_stat_count_t page_committed;
  mi_stat_count_t segments_abandoned;
  mi_stat_count_t pages_abandoned;
  mi_stat_count_t threads;
  mi_stat_count_t normal;
  mi_stat_count_t huge;
  mi_stat_count_t giant;
  mi_stat_count_t malloc;
  mi_stat_count_t segments_cache;
  mi_stat_counter_t pages_extended;
  mi_stat_counter_t mmap_calls;
  mi_stat_counter_t commit_calls;
  mi_stat_counter_t page_no_retire;
  mi_stat_counter_t searches;
  mi_stat_counter_t normal_count;
  mi_stat_counter_t huge_count;
  mi_stat_counter_t giant_count;
#if MI_STAT>1
  mi_stat_count_t normal_bins[MI_BIN_HUGE+1];
#endif
} mi_stats_t;


void _mi_stat_increase(mi_stat_count_t* stat, size_t amount);
void _mi_stat_decrease(mi_stat_count_t* stat, size_t amount);
void _mi_stat_counter_increase(mi_stat_counter_t* stat, size_t amount);

#if (MI_STAT)
#define mi_stat_increase(stat,amount)         _mi_stat_increase( &(stat), amount)
#define mi_stat_decrease(stat,amount)         _mi_stat_decrease( &(stat), amount)
#define mi_stat_counter_increase(stat,amount) _mi_stat_counter_increase( &(stat), amount)
#else
#define mi_stat_increase(stat,amount)         (void)0
#define mi_stat_decrease(stat,amount)         (void)0
#define mi_stat_counter_increase(stat,amount) (void)0
#endif

#define mi_heap_stat_counter_increase(heap,stat,amount)  mi_stat_counter_increase( (heap)->tld->stats.stat, amount)
#define mi_heap_stat_increase(heap,stat,amount)  mi_stat_increase( (heap)->tld->stats.stat, amount)
#define mi_heap_stat_decrease(heap,stat,amount)  mi_stat_decrease( (heap)->tld->stats.stat, amount)

// ------------------------------------------------------
// Thread Local data
// ------------------------------------------------------

typedef int64_t  mi_msecs_t;

// Queue of segments
typedef struct mi_segment_queue_s {
  mi_segment_t* first;
  mi_segment_t* last;
} mi_segment_queue_t;

// OS thread local data
typedef struct mi_os_tld_s {
  size_t                region_idx;   // start point for next allocation
  mi_stats_t*           stats;        // points to tld stats
} mi_os_tld_t;

// Segments thread local data
typedef struct mi_segments_tld_s {
  mi_segment_queue_t  small_free;   // queue of segments with free small pages
  mi_segment_queue_t  medium_free;  // queue of segments with free medium pages
  mi_page_queue_t     pages_reset;  // queue of freed pages that can be reset
  size_t              count;        // current number of segments;
  size_t              peak_count;   // peak number of segments
  size_t              current_size; // current size of all segments
  size_t              peak_size;    // peak size of all segments
  size_t              cache_count;  // number of segments in the cache
  size_t              cache_size;   // total size of all segments in the cache
  mi_segment_t*       cache;        // (small) cache of segments
  mi_stats_t*         stats;        // points to tld stats
  mi_os_tld_t*        os;           // points to os stats
} mi_segments_tld_t;

// Thread local data
struct mi_tld_s {
  unsigned long long  heartbeat;     // monotonic heartbeat count
  bool                recurse;       // true if deferred was called; used to prevent infinite recursion.
  mi_heap_t*          heap_backing;  // backing heap of this thread (cannot be deleted)
  mi_heap_t*          heaps;         // list of heaps in this thread (so we can abandon all when the thread terminates)
  mi_segments_tld_t   segments;      // segment tld
  mi_os_tld_t         os;            // os tld
  mi_stats_t          stats;         // statistics
};

#endif
