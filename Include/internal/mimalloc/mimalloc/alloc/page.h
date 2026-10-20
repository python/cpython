/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_PAGE_H
#define MI_INTERNAL_PAGE_H

#include "../types.h"
#include "../prim/os.h"
#include "../heap/heap.h"
#include "../heap/page-map.h"
#include "../track.h"

mi_decl_restrict void* _mi_malloc_generic(mi_theap_t* theap, size_t size, size_t zero_huge_alignment, mi_page_t** ppage) mi_attr_noexcept mi_attr_malloc;
mi_decl_restrict void* _mi_malloc_generic_no_sample(mi_theap_t* theap, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept mi_attr_malloc;

void          _mi_page_retire(mi_page_t* page) mi_attr_noexcept;       // free the page if there are no other pages with many free blocks
void          _mi_page_unfull(mi_page_t* page);
bool          _mi_page_free(mi_page_t* page, mi_page_queue_t* pq);     // false if page release is delayed
void          _mi_page_abandon(mi_page_t* page, mi_page_queue_t* pq);  // abandon the page, to be picked up by another thread...
void          _mi_deferred_free(mi_theap_t* theap, bool force);
bool          _mi_page_free_collect(mi_page_t* page, bool force);  // returns `true` if cross-thread free'd blocks were collected
mi_block_t*   _mi_page_free_collect_partly(mi_page_t* page, mi_block_t* head);
mi_decl_nodiscard bool _mi_page_init(mi_theap_t* theap, mi_page_t* page);
void          _mi_page_update_stats(mi_page_t* page);

void          _mi_theap_collect_retired(mi_theap_t* theap, bool force);
bool          _mi_theap_page_reclaim(mi_theap_t* theap, mi_page_t* page);  // true if an empty page was freed
#if MI_DEBUG>1
bool          _mi_page_is_valid(mi_page_t* page);
#endif

// in page-queue.c
bool          _mi_page_queue_is_valid(mi_theap_t* theap, const mi_page_queue_t* pq);
size_t        _mi_page_stats_bin(const mi_page_t* page); // for stats
size_t        _mi_bin_size(size_t bin);                  // for stats
size_t        _mi_bin(size_t size);                      // for stats


static inline mi_page_queue_t* mi_page_queue(const mi_theap_t* theap, size_t size) {
  mi_page_queue_t* const pq = &((mi_theap_t*)theap)->pages[_mi_bin(size)];
  if (size <= MI_LARGE_MAX_OBJ_SIZE) { mi_assert_internal(pq->block_size <= MI_LARGE_MAX_OBJ_SIZE); }
  return pq;
}

// Get the block size of a page
static inline size_t mi_page_block_size(const mi_page_t* page) {
  mi_assert_internal(page->block_size > 0);
  return page->block_size;
}

// Page start
static inline uint8_t* mi_page_start(const mi_page_t* page) {
  // multiplication must be done in `size_t`
  return (uint8_t*)page + page->page_offset;
}

static inline size_t mi_page_size(const mi_page_t* page) {
  return mi_page_block_size(page) * page->reserved;
}

static inline uint8_t* mi_page_area(const mi_page_t* page, size_t* size) {
  if (size) { *size = mi_page_size(page); }
  return mi_page_start(page);
}

static inline size_t mi_page_info_size(void) {
  return mi_align_up(sizeof(mi_page_t), MI_MAX_ALIGN_SIZE);
}

// Can the first block in a page with blocks of size `bsize` have a cache offset? (see `MI_PAGE_CACHE_OFFSET`)
static inline bool mi_block_size_has_cache_offset(size_t bsize) {
  return (MI_PAGE_CACHE_OFFSET != 0 && bsize > MI_SMALL_MAX_OBJ_SIZE);
}

// Is a (non-small) block of size `bsize` allocated in a medium page?
// Blocks of 1/8th of a medium page (64 KiB) leave no space for a cache offset and use a large page instead.
static inline bool mi_block_size_is_medium(size_t bsize) {
  #if MI_ENABLE_LARGE_PAGES
  if (mi_block_size_has_cache_offset(bsize) && bsize == MI_MEDIUM_PAGE_SIZE/8) return false;
  #endif
  return (bsize <= MI_MEDIUM_MAX_OBJ_SIZE);
}

// The alignment that every block of size `bsize` is guaranteed to have in any page (or 0 if `bsize==0`).
// This is guaranteed by the block start (see `arena/arena-page.c:mi_page_meta_in_front_block_start` and `mi_page_meta_select`),
// and used by `mi_malloc_aligned` to avoid over-allocation for naturally aligned blocks.
static inline size_t mi_natural_alignment_of_size(size_t bsize) {
  const size_t lowbit = bsize & (~bsize + 1);  // largest power of 2 that divides `bsize`
  if (mi_is_power_of_two(bsize) && bsize <= MI_PAGE_MAX_START_BLOCK_ALIGN2) return bsize;   // power-of-two blocks up to 4KiB
  if (bsize <= MI_SMALL_SIZE_MAX) return lowbit;                                            // small blocks start at a multiple of their size
  if ((bsize % MI_PAGE_OSPAGE_BLOCK_ALIGN2) == 0) {                                         // multiples of 4KiB are 4KiB aligned (or cache offset unit aligned)
    return (mi_block_size_has_cache_offset(bsize) ? MI_PAGE_CACHE_OFFSET_UNIT : MI_PAGE_OSPAGE_BLOCK_ALIGN2);
  }
  return (lowbit < MI_MAX_ALIGN_SIZE ? lowbit : MI_MAX_ALIGN_SIZE);                         // otherwise blocks start at least `MI_MAX_ALIGN_SIZE` aligned
}

static inline bool mi_page_contains_address(const mi_page_t* page, const void* p) {
  size_t psize;
  uint8_t* start = mi_page_area(page, &psize);
  return (start <= (uint8_t*)p && (uint8_t*)p < start + psize);
}

static inline bool mi_page_is_in_arena(const mi_page_t* page) {
  return (page->memid.memkind == MI_MEM_ARENA);
}

static inline bool mi_page_is_singleton(const mi_page_t* page) {
  return (page->reserved == 1);
}

// Get the usable block size of a page without fixed padding.
// This may still include internal padding due to alignment and rounding up size classes.
static inline size_t mi_page_usable_block_size(const mi_page_t* page) {
  return mi_page_block_size(page) - MI_PADDING_SIZE;
}

static inline bool mi_page_meta_is_separated(const mi_page_t* page) {
  #if MI_PAGE_META_IS_ALIGNED
    #if MI_PAGE_META_SMALL_IS_ALIGNED
    return (page != mi_align_down_ptr(mi_page_start(page), MI_ARENA_SLICE_ALIGN));
    #else
    MI_UNUSED_RELEASE(page);
    mi_assert_internal(page != mi_align_down_ptr(mi_page_start(page), MI_ARENA_SLICE_ALIGN));
    return true;
    #endif
  #elif MI_PAGE_META_IS_SEPARATED
  // usually separated but can still be in front for direct OS allocations (due to size or alignment) or due to MI_PAGE_META_SMALL_IS_ALIGNED
  return (page->memid.memkind == MI_MEM_ARENA && page != mi_align_down_ptr(mi_page_start(page), MI_ARENA_SLICE_ALIGN));
  #else
  MI_UNUSED(page);
  return false;
  #endif
}

static inline uint8_t* mi_page_slice_start(const mi_page_t* page) {
  if (mi_page_meta_is_separated(page)) {
    // page meta info is at a separate location (at `arena->pages`)
    return (uint8_t*)mi_align_down_ptr(mi_page_start(page), MI_ARENA_SLICE_ALIGN);
  }
  else {
    // page meta info is at the start of the page slices
    return (uint8_t*)page;
  }
}

// This gives the offset relative to the start slice of a page.
static inline size_t mi_page_slice_offset_of(const mi_page_t* page, size_t offset_relative_to_page_start) {
  return (mi_page_start(page) - mi_page_slice_start(page)) + offset_relative_to_page_start;
}

// How much of the page is committed relative to the slice start? (or 0 if fully committed already)
static inline size_t mi_page_slice_committed(const mi_page_t* page) {
  return ((size_t)page->slice_pcommitted * _mi_os_page_size());
}

// Currently committed part of a page
static inline size_t mi_page_committed(const mi_page_t* page) {
  const size_t slice_committed = mi_page_slice_committed(page);
  return (slice_committed == 0 ? mi_page_size(page) : slice_committed - mi_page_slice_offset_of(page,0));
}

static inline size_t mi_page_used(const mi_page_t* page) {
  mi_assert_internal(page != NULL);
  return mi_xused_used_count(page->xused);
}

static inline void mi_page_used_reset(mi_page_t* page) {
  page->xused = mi_xused_used_reset(page->xused);
}

static inline size_t mi_page_alloc_count(const mi_page_t* page) {
  return mi_xused_alloc_count(page->xused);
}

static inline size_t mi_page_last_used(const mi_page_t* page) {
  #if MI_SIZE_SIZE >= 8
  return (page->xused.used_alloc >> 32) & 0xFFFF;
  #else
  return page->xlast_used;
  #endif
}

static inline size_t mi_page_last_alloc(const mi_page_t* page) {
  #if MI_SIZE_SIZE >= 8
  return (page->xused.used_alloc >> 48) & 0xFFFF;
  #else
  return page->xlast_alloc;
  #endif
}


// are all blocks in a page freed?
// note: needs up-to-date used count, (as the `xthread_free` list may not be empty). see `_mi_page_collect_free`.
static inline bool mi_page_all_free(const mi_page_t* page) {
  mi_assert_internal(page != NULL);
  return (mi_page_used(page)==0);
}

// are there immediately available blocks, i.e. blocks available on the free list.
static inline bool mi_page_immediate_available(const mi_page_t* page) {
  mi_assert_internal(page != NULL);
  return (page->free != NULL);
}


// is the page not yet used up to its reserved space?
static inline bool mi_page_is_expandable(const mi_page_t* page) {
  mi_assert_internal(page != NULL);
  mi_assert_internal(page->capacity <= page->reserved);
  return (page->capacity < page->reserved);
}


static inline bool mi_page_is_full(const mi_page_t* page) {
  const bool full = (page->reserved == mi_page_used(page));
  mi_assert_internal(!full || page->free == NULL);
  return full;
}

// is more than (1 - 1/n)'th of a page in use?
static inline bool mi_page_is_used_at_frac(const mi_page_t* page, size_t n) {
  if (page==NULL) return true;
  const size_t frac = page->reserved / n;
  return (page->reserved - mi_page_used(page) <= frac);
}

// is more than 7/8th of a page in use?
static inline bool mi_page_is_mostly_used(const mi_page_t* page) {
  return mi_page_is_used_at_frac(page, 8);
}

static inline bool mi_page_is_small(const mi_page_t* page) {
  return (page->block_size <= MI_SMALL_MAX_OBJ_SIZE && !mi_page_is_singleton(page));
}

static inline bool mi_page_is_huge(const mi_page_t* page) {
  return (mi_page_is_singleton(page) &&
          (page->block_size > MI_LARGE_MAX_OBJ_SIZE ||
           (mi_memkind_is_os(page->memid.memkind) && page->memid.mem.os.base < (void*)page)));
}

static inline size_t mi_page_min_commit_size(void) {
  const size_t psize = _mi_os_page_size();
  return (MI_PAGE_MIN_COMMIT_SIZE >= psize ? MI_PAGE_MIN_COMMIT_SIZE : psize);
}

//-----------------------------------------------------------
// Page thread id and flags
//-----------------------------------------------------------

// Thread id of thread that owns this page (with flags in the bottom 2 bits)
static inline mi_threadid_t mi_page_xthread_id(const mi_page_t* page) {
  return mi_atomic_load_relaxed(&((mi_page_t*)page)->xthread_id);
}

// Plain thread id of the thread that owns this page
static inline mi_threadid_t mi_page_thread_id(const mi_page_t* page) {
  return (mi_page_xthread_id(page) & ~MI_PAGE_FLAG_MASK);
}

static inline mi_page_flags_t mi_page_flags(const mi_page_t* page) {
  return (mi_page_xthread_id(page) & MI_PAGE_FLAG_MASK);
}

static inline bool mi_page_flags_set(mi_page_t* page, bool set, mi_page_flags_t newflag) {
  mi_page_flags_t old;
  if (set) { old = mi_atomic_or_relaxed(&page->xthread_id, newflag); }
      else { old = mi_atomic_and_relaxed(&page->xthread_id, ~newflag); }
  return ((old & newflag) == newflag);
}

static inline bool mi_page_is_in_full(const mi_page_t* page) {
  return ((mi_page_flags(page) & MI_PAGE_IN_FULL_QUEUE) != 0);
}

static inline void mi_page_set_in_full(mi_page_t* page, bool in_full) {
  const bool was_in_full = mi_page_flags_set(page, in_full, MI_PAGE_IN_FULL_QUEUE);
  if (was_in_full != in_full) {
    // optimize: maintain pages_full_size to avoid visiting the full queue (issue #1220)
    mi_theap_t* const theap = page->theap;
    mi_assert_internal(theap!=NULL);
    if (theap != NULL) {
      mi_assert_internal(page->capacity==page->reserved);
      const size_t size = page->reserved * mi_page_block_size(page);
      if (in_full) { theap->pages_full_size += size; }
              else { mi_assert_internal(size <= theap->pages_full_size); theap->pages_full_size -= size; }
    }
  }
}

static inline bool mi_page_has_interior_pointers(const mi_page_t* page) {
  return ((mi_page_flags(page) & MI_PAGE_HAS_INTERIOR_POINTERS) != 0);
}

static inline void mi_page_set_has_interior_pointers(mi_page_t* page, bool has_aligned) {
  mi_page_flags_set(page, has_aligned, MI_PAGE_HAS_INTERIOR_POINTERS);
}

static inline void mi_page_set_theap(mi_page_t* page, mi_theap_t* theap) {
  // mi_assert_internal(!mi_page_is_in_full(page));  // can happen when destroying pages on theap_destroy
  page->theap = theap;
  const mi_threadid_t tid = (theap == NULL ? MI_THREADID_ABANDONED : theap->tld->thread_id);
  mi_assert_internal((tid & MI_PAGE_FLAG_MASK) == 0);

  // we need to use an atomic cas since a concurrent thread may still set the MI_PAGE_HAS_INTERIOR_POINTERS flag (see `alloc_aligned.c`).
  mi_threadid_t xtid_old = mi_page_xthread_id(page);
  mi_threadid_t xtid;
  do {
    xtid = tid | (xtid_old & MI_PAGE_FLAG_MASK);
  } while (!mi_atomic_cas_weak_release(&page->xthread_id, &xtid_old, xtid));
}

static inline bool mi_page_is_abandoned(const mi_page_t* page) {
  // note: the xtheap field of an abandoned theap is set to the subproc (for fast reclaim-on-free)
  return (mi_page_thread_id(page) <= MI_THREADID_ABANDONED_MAPPED);
}

static inline bool mi_page_is_abandoned_mapped(const mi_page_t* page) {
  return (mi_page_thread_id(page) == MI_THREADID_ABANDONED_MAPPED);
}

static inline void mi_page_set_abandoned_mapped(mi_page_t* page) {
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_atomic_or_relaxed(&page->xthread_id, (mi_threadid_t)MI_THREADID_ABANDONED_MAPPED);
}

static inline void mi_page_clear_abandoned_mapped(mi_page_t* page) {
  mi_assert_internal(mi_page_is_abandoned_mapped(page));
  mi_atomic_and_relaxed(&page->xthread_id, (mi_threadid_t)MI_PAGE_FLAG_MASK);
}


static inline mi_theap_t* mi_page_theap(const mi_page_t* page) {
  mi_assert_internal(!mi_page_is_abandoned(page));
  mi_assert_internal(page->theap != NULL && !_mi_is_empty_theap(page->theap));
  return page->theap;
}

static inline mi_tld_t* mi_page_tld(const mi_page_t* page) {
  mi_assert_internal(!mi_page_is_abandoned(page));
  mi_assert_internal(page->theap != NULL);
  return page->theap->tld;
}


static inline mi_heap_t* mi_page_heap(const mi_page_t* page) {
  mi_heap_t* heap = page->heap;
  mi_assert_internal(heap != NULL);
  return heap;
}

static inline mi_subproc_t* mi_page_subproc(const mi_page_t* page) {
  mi_heap_t* const heap = mi_page_heap(page);
  return heap->subproc;
}

//-----------------------------------------------------------
// Thread free list and ownership
//-----------------------------------------------------------

// Thread free flag helpers
static inline mi_block_t* mi_tf_block(mi_thread_free_t tf) {
  return (mi_block_t*)(tf & ~1);
}

static inline bool mi_tf_is_owned(mi_thread_free_t tf) {
  return ((tf & 1) == 1);
}

static inline mi_thread_free_t mi_tf_create(mi_block_t* block, bool owned) {
  const uintptr_t base = (uintptr_t)block | (owned ? 1 : 0);
  return (mi_thread_free_t)base;
}

static inline mi_thread_free_t mi_tf_set_owned(mi_thread_free_t tf, bool owned) {
  return mi_tf_create(mi_tf_block(tf), owned);
}

// Thread free access
static inline mi_block_t* mi_page_thread_free(const mi_page_t* page) {
  return mi_tf_block(mi_atomic_load_relaxed(&((mi_page_t*)page)->xthread_free));
}

// are there any available blocks?
static inline bool mi_page_has_any_available(const mi_page_t* page) {
  mi_assert_internal(page != NULL && page->reserved > 0);
  return (mi_page_used(page) < page->reserved || (mi_page_thread_free(page) != NULL));
}

// Owned?
static inline bool mi_page_is_owned(const mi_page_t* page) {
  return mi_tf_is_owned(mi_atomic_load_relaxed(&((mi_page_t*)page)->xthread_free));
}

// get ownership; returns true if the page was not owned before.
static inline bool mi_page_claim_ownership(mi_page_t* page) {
  const uintptr_t old = mi_atomic_or_acq_rel(&page->xthread_free, (uintptr_t)1);
  return ((old&1)==0);
}

/* -----------------------------------------------------------
  Delayed page release
----------------------------------------------------------- */

static inline void mi_page_qsbr_unlink(mi_page_t* page) {
#if MI_QSBR
  if (!page->qsbr_in_queue) {
    mi_assert_internal(page->qsbr_next == NULL && page->qsbr_prev == NULL);
    return;
  }
  mi_theap_t* theap = mi_page_theap(page);
  if (page->qsbr_prev != NULL) { page->qsbr_prev->qsbr_next = page->qsbr_next; }
  else {
    mi_assert_internal(theap->qsbr_first == page);
    theap->qsbr_first = page->qsbr_next;
  }
  if (page->qsbr_next != NULL) { page->qsbr_next->qsbr_prev = page->qsbr_prev; }
  else {
    mi_assert_internal(theap->qsbr_last == page);
    theap->qsbr_last = page->qsbr_prev;
  }
  page->qsbr_next = page->qsbr_prev = NULL;
  page->qsbr_in_queue = false;
#else
  MI_UNUSED(page);
#endif
}

static inline void mi_page_qsbr_enqueue(mi_page_t* page, mi_theap_t* theap) {
#if MI_QSBR
  mi_assert_internal(mi_page_theap(page) == theap);
  mi_assert_internal(mi_page_all_free(page));
  mi_assert_internal(mi_heap_qsbr_is_enabled(mi_page_heap(page)));
  mi_assert_internal(!mi_page_qsbr_is_clear(&page->qsbr));
  if (page->qsbr_in_queue) return;
  mi_assert_internal(page->qsbr_next == NULL && page->qsbr_prev == NULL);
  page->qsbr_prev = theap->qsbr_last;
  if (theap->qsbr_last != NULL) { theap->qsbr_last->qsbr_next = page; }
  else { theap->qsbr_first = page; }
  theap->qsbr_last = page;
  page->qsbr_in_queue = true;
#else
  MI_UNUSED(page);
  MI_UNUSED(theap);
#endif
}

static inline void _mi_page_qsbr_clear(mi_page_t* page) {
#if MI_QSBR
  mi_page_qsbr_unlink(page);
  mi_page_qsbr_clear(&page->qsbr);
#else
  MI_UNUSED(page);
#endif
}

static inline bool _mi_page_qsbr_is_clear(const mi_page_t* page) {
#if MI_QSBR
  return (!page->qsbr_in_queue && page->qsbr_next == NULL && page->qsbr_prev == NULL &&
          mi_page_qsbr_is_clear(&page->qsbr));
#else
  MI_UNUSED(page);
  return true;
#endif
}

static inline bool _mi_page_qsbr_should_delay(mi_page_t* page) {
#if MI_QSBR
  mi_assert_internal(mi_page_is_owned(page) && mi_page_all_free(page));
  mi_heap_t* heap = mi_page_heap(page);
  if (!mi_heap_qsbr_is_enabled(heap)) return false;
  const bool delayed = mi_page_qsbr_should_delay(&page->qsbr, &heap->qsbr, page->capacity * mi_page_block_size(page));
  mi_assert_internal(!delayed || !mi_page_qsbr_is_clear(&page->qsbr));
  return delayed;
#else
  MI_UNUSED(page);
  return false;
#endif
}

/* -------------------------------------------------------------------
Encoding/Decoding the free list next pointers

This is to protect against buffer overflow exploits where the
free list is mutated. Many hardened allocators xor the next pointer `p`
with a secret key `k1`, as `p^k1`. This prevents overwriting with known
values but might be still too weak: if the attacker can guess
the pointer `p` this  can reveal `k1` (since `p^k1^p == k1`).
Moreover, if multiple blocks can be read as well, the attacker can
xor both as `(p1^k1) ^ (p2^k1) == p1^p2` which may reveal a lot
about the pointers (and subsequently `k1`).

Instead mimalloc uses an extra key `k2` and encodes as `((p^k2)<<<k1)+k1`.
Since these operations are not associative, the above approaches do not
work so well any more even if the `p` can be guesstimated. For example,
for the read case we can subtract two entries to discard the `+k1` term,
but that leads to `((p1^k2)<<<k1) - ((p2^k2)<<<k1)` at best.
We include the left-rotation since xor and addition are otherwise linear
in the lowest bit. Finally, both keys are unique per page which reduces
the re-use of keys by a large factor.

We also pass a separate `null` value to be used as `NULL` or otherwise
`(k2<<<k1)+k1` would appear (too) often as a sentinel value.
------------------------------------------------------------------- */

static inline bool mi_is_in_same_page(const void* p, const void* q) {
  mi_page_t* page = mi_ptr_page(p);
  return mi_page_contains_address(page,q);
  // return (mi_ptr_page(p) == mi_ptr_page(q));
}

static inline void* mi_ptr_decode(const void* null, const mi_encoded_t x, const uintptr_t* keys) {
  const uintptr_t k1 = keys[0];
  #if MI_PAGE_KEY_COUNT==2
  const uintptr_t k2 = keys[1];
  #else
  const uintptr_t k2 = mi_rotr(k1,13);
  #endif
  void* p = (void*)(mi_rotr(x - k1, k1) ^ k2);
  return (p==null ? NULL : p);
}

static inline mi_encoded_t mi_ptr_encode(const void* null, const void* p, const uintptr_t* keys) {
  const uintptr_t k1 = keys[0];
  #if MI_PAGE_KEY_COUNT==2
  const uintptr_t k2 = keys[1];
  #else
  const uintptr_t k2 = mi_rotr(k1,13);
  #endif
  const uintptr_t x = (uintptr_t)(p==NULL ? null : p);
  return mi_rotl(x ^ k2, k1) + k1;
}

static inline uint32_t mi_ptr_encode_canary(const void* null, const void* p, const uintptr_t* keys) {
  const uint32_t x = (uint32_t)(mi_ptr_encode(null,p,keys));
  // make the lowest byte 0 to prevent spurious read overflows which could be a security issue (issue #951)
  // also clear bit 9 which we set only when a block is freed.
  #if MI_BIG_ENDIAN
  return (x & 0x00FFFEFF);
  #else
  return (x & 0xFFFFFE00);
  #endif
}

static inline uint32_t mi_ptr_encode_canary_freed(void) {
  return (0x00DEAD00);  // set bit 9 so it is different from any valid canary
}

static inline bool mi_ptr_decode_canary_is_freed(uint32_t canary) {
  return (canary == mi_ptr_encode_canary_freed());
}

static inline mi_block_t* mi_block_nextx( const void* null, const mi_block_t* block, const uintptr_t* keys ) {
  mi_track_mem_defined(block,sizeof(mi_block_t));
  mi_block_t* next;
  #if MI_ENCODE_FREELIST
  next = (mi_block_t*)mi_ptr_decode(null, block->next, keys);
  #else
  MI_UNUSED(keys); MI_UNUSED(null);
  next = (mi_block_t*)block->next;
  #endif
  mi_track_mem_noaccess(block,sizeof(mi_block_t));
  return next;
}

static inline void mi_block_set_nextx(const void* null, mi_block_t* block, const mi_block_t* next, const uintptr_t* keys) {
  mi_track_mem_undefined(block,sizeof(mi_block_t));
  #if MI_ENCODE_FREELIST
  block->next = mi_ptr_encode(null, next, keys);
  #else
  MI_UNUSED(keys); MI_UNUSED(null);
  block->next = (mi_encoded_t)next;
  #endif
  mi_track_mem_noaccess(block,sizeof(mi_block_t));
}

static inline mi_block_t* mi_block_next(const mi_page_t* page, const mi_block_t* block) {
  #if MI_ENCODE_FREELIST
  mi_block_t* next = mi_block_nextx(page,block,page->keys);
  // check for free list corruption: is `next` at least in the same page?
  // todo: check if `next` is `page->block_size` aligned?
  if mi_unlikely(next!=NULL && !mi_page_contains_address(page,next)) {
    return _mi_block_next_is_corrupted(page,block,next); // returns NULL
  }
  return next;
  #else
  MI_UNUSED(page);
  return mi_block_nextx(page,block,NULL);
  #endif
}

static inline void mi_block_set_next(const mi_page_t* page, mi_block_t* block, const mi_block_t* next) {
  #if MI_ENCODE_FREELIST
  mi_block_set_nextx(page,block,next, page->keys);
  #else
  MI_UNUSED(page);
  mi_block_set_nextx(page,block,next,NULL);
  #endif
}

#endif
