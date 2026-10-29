/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#if !defined(MI_IN_ALLOC_C)
#error "this file should be included from 'alloc.c' (so aliases can work from alloc-override)"
#endif

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/prim/prim-tls.h"             // _mi_prim_thread_id()
#include "mimalloc/util/stats.h"
#include "mimalloc/arena/arena-page.h"
#include "mimalloc/arena/arena.h"               // _mi_arena_memid_is_suitable
#include "mimalloc/profile/sample-guarded.h"    // unguard
#include "mimalloc/alloc/page.h"
#include "mimalloc/alloc/free.h"
#include "mimalloc/heap/heap.h"


// forward declarations
mi_decl_nodiscard static bool mi_check_padding_on_free(const mi_page_t* page, const mi_block_t* block, bool is_guarded, size_t* usable_size);
mi_decl_nodiscard static bool mi_check_double_free(const mi_page_t* page, const mi_block_t* block);
static size_t mi_page_usable_size_of(const mi_page_t* page, const mi_block_t* block, bool was_guarded);

#if (MI_DEBUG>0) && !MI_TRACK_ENABLED && !MI_TSAN
// Fill a block with a debug pattern, preserving the first `heap->debug_fill_preserve` bytes
// (used by CPython to keep object headers intact that may be read by concurrent lock-free readers)
static inline void mi_debug_fill(const mi_page_t* page, void* block, int c, size_t size) {
  const size_t preserve = mi_atomic_load_relaxed(&mi_page_heap(page)->debug_fill_preserve);
  if (preserve < size) {
    mi_memset((uint8_t*)block + preserve, c, size - preserve);
  }
}
#endif

// ------------------------------------------------------
// Free
// ------------------------------------------------------

// regular free of a (thread local) block pointer
// fast path written carefully to prevent spilling on the stack
static inline void mi_free_block_local(mi_page_t* page, mi_block_t* block, bool was_guarded, bool check_full)
{
  // checks
  size_t usable_size;
  if mi_unlikely(!mi_check_padding_on_free(page, block, was_guarded, &usable_size)) return;
  if mi_unlikely(!mi_check_double_free(page,block)) return;  // usually checked with padding

  // mi_stat_free(page, block);
  mi_track_free_size(block, usable_size);

  #if (MI_DEBUG>0) && !MI_TRACK_ENABLED  && !MI_TSAN
  const size_t dbgsize = (usable_size > MI_MiB ? MI_MiB : usable_size);
  mi_debug_fill(page, block, MI_DEBUG_FREED, dbgsize);
  #endif

  // actual free: push on the local free list fast-path
  mi_used_t xused = page->xused;
  mi_block_t* lfree = page->local_free;
  xused.used_alloc--;
  mi_block_set_next(page, block, lfree);
  page->xused = xused;
  page->local_free = block;
  const bool is_empty = (mi_xused_used_count(xused) == 0);
  mi_assert_internal(mi_page_alloc_count(page) + mi_page_last_used(page) >= mi_page_used(page));
  if mi_unlikely(is_empty) {      // is used count zero ?
    _mi_page_qsbr_clear(page);    // a new empty interval must not reuse an earlier grace period
    if (page->retire_expire==0) { // no need to re-retire retired pages (happens when we alloc/free one block repeatedly in an empty page)
      _mi_page_retire(page);
    }
  }
  else if mi_unlikely(check_full && mi_page_is_in_full(page)) {
    _mi_page_unfull(page);
  }
}

// Forward declaration for multi-threaded collect
static void mi_decl_noinline mi_free_try_collect_mt(mi_page_t* page, mi_block_t* mt_free, bool allow_reclaim) mi_attr_noexcept;

// Free a block multi-threaded
#if defined(_MSC_VER)
static mi_decl_noinline  /* ensures mi_free has no stack frame */
#else
static inline
#endif
void mi_free_block_mt(mi_page_t* page, mi_block_t* block, bool was_guarded, bool allow_reclaim) mi_attr_noexcept
{
  size_t usable_size;
  if mi_unlikely(!mi_check_padding_on_free(page, block, was_guarded, &usable_size)) return;    // checking padding is safe for mt
  mi_track_free_size(block, usable_size);

  // _mi_padding_shrink(page, block, sizeof(mi_block_t));
  #if (MI_DEBUG>0) && !MI_TRACK_ENABLED  && !MI_TSAN       // note: when tracking, cannot use mi_usable_size with multi-threading
  if (!was_guarded) {
    const size_t dbgsize = (usable_size > MI_MiB ? MI_MiB : usable_size);
    mi_debug_fill(page, block, MI_DEBUG_FREED, dbgsize);
  }
  #endif

  // push atomically on the page thread free list
  // mi_theap_t* theap = _mi_page_associated_theap_peek(page);
  mi_thread_free_t tf_new;
  mi_thread_free_t tf_old = mi_atomic_load_relaxed(&page->xthread_free);
  do {
    mi_block_set_next(page, block, mi_tf_block(tf_old));
    tf_new = mi_tf_create(block, true);
  } while(!mi_atomic_cas_weak_acq_rel(&page->xthread_free, &tf_old, tf_new));

  // and atomically try to collect the page if it was abandoned
  mi_assert_internal(mi_tf_is_owned(tf_new));
  const bool is_newly_owned = !mi_tf_is_owned(tf_old); // && mi_tf_is_owned(tf_new)
  if (is_newly_owned) {
    mi_assert_internal(mi_page_is_abandoned(page));
    // mi_assert_internal(!mi_page_is_abandoned_mapped(page));
    mi_free_try_collect_mt(page, block, allow_reclaim);
  }
}


// Adjust a block that was allocated aligned, to the actual start of the block in the page.
// note: this can be called from `mi_free_generic_mt` where a non-owning thread accesses the
// `page_woffset` and `block_size` fields; however these are constant and the page won't be
// deallocated (as the block we are freeing keeps it alive) and thus safe to read concurrently.
static inline mi_block_t* mi_page_ptr_unalign_ex(const mi_page_t* page, const void* p, size_t* poffset) {
  mi_assert_internal(page!=NULL && p!=NULL);

  const size_t diff = (uint8_t*)p - mi_page_start(page);
  const size_t block_size = mi_page_block_size(page);
  size_t adjust = diff & (block_size - 1);
  if mi_unlikely(!mi_is_power_of_two(block_size)) {
    adjust = diff % block_size;
  }
  if (poffset!=NULL) { *poffset = adjust; }
  return (mi_block_t*)((uintptr_t)p - adjust);
}

mi_block_t* _mi_page_ptr_unalign(const mi_page_t* page, const void* p) {
  return mi_page_ptr_unalign_ex(page,p,NULL);
}

static inline mi_block_t* mi_validate_block_from_ptr( const mi_page_t* page, const void* p ) {
  mi_assert(_mi_page_ptr_unalign(page,p) == (mi_block_t*)p); // should never be an interior pointer
  #if MI_SECURE > 0
  // in secure mode we always unalign to guard against free-ing interior pointers
  return _mi_page_ptr_unalign(page,p);
  #else
  MI_UNUSED(page);
  return (mi_block_t*)p;
  #endif
}

static inline mi_block_t* mi_page_ptr_block_check(mi_page_t* page, void* p, bool* was_guarded) mi_attr_noexcept {
  MI_UNUSED(was_guarded);
  if mi_likely(!mi_page_has_interior_pointers(page)) {
    return mi_validate_block_from_ptr(page,p);
  }
  else {
    size_t offset;
    mi_block_t* const block = mi_page_ptr_unalign_ex(page,p,&offset);
    #if MI_GUARDED || MI_PROFILE
    if (offset >= sizeof(mi_block_t)) {
      #if MI_PROFILE
      if (block->next == MI_BLOCK_TAG_PROFILED) {
        _mi_page_profile_on_free(page,block,p);
      }
      else
      #endif
      #if MI_GUARDED
      if (block->next == MI_BLOCK_TAG_GUARDED) {
        _mi_page_block_unguard(page, block, p);
        *was_guarded = true;
      }
      else
      #endif
      { /* nothing */ }
    }
    #endif
    return block;
  }
}

// free a local pointer  (page parameter comes first for better codegen)
static void mi_decl_noinline mi_free_generic_local(mi_page_t* page, void* p) mi_attr_noexcept {
  mi_assert_internal(p!=NULL && page != NULL);
  bool was_guarded = false;
  mi_block_t* block = mi_page_ptr_block_check(page,p,&was_guarded);
  // mi_block_t* const block = (mi_page_has_interior_pointers(page) ? _mi_page_ptr_unalign(page, p) : mi_validate_block_from_ptr(page,p));
  // mi_block_check_profiled(page,block,p);
  // const bool was_guarded = mi_block_check_unguard(page, block, p);
  mi_free_block_local(page, block, was_guarded, true /* check for a full page */);
}

// free a pointer owned by another thread (page parameter comes first for better codegen)
static void mi_decl_noinline mi_free_generic_mt(mi_page_t* page, void* p, bool allow_reclaim) mi_attr_noexcept {
  mi_assert_internal(p!=NULL && page != NULL);
  bool was_guarded = false;
  mi_block_t* block = mi_page_ptr_block_check(page,p,&was_guarded);
  // mi_block_t* const block = (mi_page_has_interior_pointers(page) ? _mi_page_ptr_unalign(page, p) : mi_validate_block_from_ptr(page,p));
  // mi_block_check_profiled(page,block,p);
  // const bool was_guarded = mi_block_check_unguard(page, block, p);
  mi_free_block_mt(page, block, was_guarded, allow_reclaim);
}

// generic free (for runtime integration)
void mi_decl_noinline _mi_free_generic(mi_page_t* page, bool is_local, void* p) mi_attr_noexcept {
  if (is_local) mi_free_generic_local(page,p);
           else mi_free_generic_mt(page,p,true);
}


// Get the page belonging to a pointer
// Does further checks in debug mode to see if this was a valid pointer.
static mi_decl_forceinline bool mi_ptr_page_is_valid_ex(const void* p, const char* msg, bool free_small, bool check_p_for_null, mi_page_t** ppage)
{
  MI_UNUSED_RELEASE(msg); MI_UNUSED(free_small);
  #if MI_DEBUG
  if mi_unlikely(!mi_is_aligned(p,MI_SIZE_SIZE) && !mi_option_is_enabled(mi_option_unsafe_guarded_precise)) {
    _mi_error_message(EINVAL, "%s: invalid (unaligned) pointer: %p\n", msg, p);
    return false;
  }
  #endif

  mi_page_t* page;
  #if MI_PAGE_META_SMALL_IS_ALIGNED
    if (free_small) {
      const uintptr_t up = mi_align_down((uintptr_t)p,MI_SMALL_PAGE_SIZE);  // like this for codegen on gcc
      if mi_unlikely(up==0 && check_p_for_null) { return false; }
      page = (mi_page_t*)up;
    }
    else
  #endif
    {
      #if MI_PAGE_META_IS_ALIGNED
        { page = mi_aligned_ptr_page0(p); }
      #else
        { page = mi_ptr_page(p); }
      #endif
      if mi_unlikely(page==NULL && check_p_for_null) {
        #if MI_DEBUG
        if (p!=NULL) { _mi_error_message(EINVAL, "%s: invalid pointer: %p\n", msg, p); }
        #endif
        return false;
      }
    }
  #if MI_DEBUG
  mi_page_t* const cpage = mi_checked_ptr_page(p);
  if mi_unlikely(cpage==NULL) { _mi_error_message(EINVAL, "%s: invalid pointer: %p\n", msg, p); }
  #endif

  #if MI_PAGE_META_IS_ALIGNED
    #if MI_PAGE_META_SMALL_IS_ALIGNED
    if (free_small) { mi_assert_internal(NULL == mi_atomic_load_ptr_acquire(mi_page_t,&page->self)); }
    else
    #elif MI_SMALL_PAGE_SIZE == MI_ARENA_SLICE_SIZE && !MI_GUARDED
    // for mi_free_small we can avoid a load-acquire (but not when guarded as that may still allocate large blocks)
    if (free_small) { mi_assert_internal(page == mi_atomic_load_ptr_acquire(mi_page_t,&page->self)); }
    else
    #endif
    {
      // The `self` load can be relaxed here as we free a known pointer and thus it has been synchronized.
      // However, the ThreadSanitizer seems to require an acquire load to properly track memory dependencies.
      #if MI_TSAN
      page = mi_atomic_load_ptr_acquire(mi_page_t,&page->self);
      #else
      page = mi_atomic_load_ptr_relaxed(mi_page_t,&page->self);
      #endif
    }
  #endif

  mi_assert_internal(page!=NULL);
  mi_assert(cpage==page /* page_map lookup should be the same as aligned lookup */ );
  #if !MI_GUARDED
  if (free_small) { mi_assert_internal(page->block_size <= MI_SMALL_MAX_OBJ_SIZE); /* free small should only be called on small pages */ }
  #endif
  *ppage = page;
  return true;
}

static mi_decl_forceinline bool mi_ptr_page_is_valid(const void* p, const char* msg, mi_page_t** ppage) {
  return mi_ptr_page_is_valid_ex(p,msg,false /* free_small? */, true /* check_p_for_null */, ppage );
}

static mi_decl_forceinline mi_page_t* mi_ptr_page_validate(const void* p, const char* msg) {
  mi_page_t* page;
  return (mi_ptr_page_is_valid(p,msg,&page) ? page : NULL);
}

// Free a block
// Fast path written carefully to prevent register spilling on the stack
static mi_decl_forceinline void mi_free_nonnull(void* p, mi_page_t* page, size_t* pblock_size, bool allow_reclaim)
{
  mi_assert_internal(p!=NULL && page!=NULL);
  if (pblock_size!=NULL) { *pblock_size = mi_page_block_size(page); }

  const mi_threadid_t ptid = mi_page_xthread_id(page);
  const mi_threadid_t xtid = (_mi_prim_thread_id() ^ ptid);
  if mi_likely(xtid == 0) {                        // `tid == mi_page_thread_id(page) && mi_page_flags(page) == 0`
    // thread-local, aligned, and not a full page
    mi_block_t* const block = mi_validate_block_from_ptr(page,p);
    mi_free_block_local(page, block, false /* was guarded */, false /* no need to check if the page is full */);
  }
  else if (xtid <= MI_PAGE_FLAG_MASK) {            // `tid == mi_page_thread_id(page) && mi_page_flags(page) != 0`
    // page is local, but is full or contains (inner) aligned blocks; use generic path
    mi_free_generic_local(page, p);
  }
  // free-ing in a page owned by a theap in another thread, or an abandoned page (not belonging to a theap)
  else if ((xtid & MI_PAGE_FLAG_MASK) == 0) {      // `tid != mi_page_thread_id(page) && mi_page_flags(page) == 0`
    // blocks are aligned (and not a full page); push on the thread_free list
    mi_block_t* const block = mi_validate_block_from_ptr(page,p);
    mi_free_block_mt(page,block,false /* was_guarded */, allow_reclaim);
  }
  else {
    // page is full or contains (inner) aligned blocks; use generic multi-thread path
    mi_free_generic_mt(page, p, allow_reclaim);
  }
}

void mi_free(void* p) mi_attr_noexcept {
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid(p,"mi_free",&page)) {
    mi_free_nonnull(p, page, NULL, true /* allow reclaim? */);
  }
}

void mi_ufree(void* p, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid(p,"mi_ufree",&page)) {
    mi_free_nonnull(p, page, pblock_size, true /* allow reclaim? */);
  }
  else {
    if (pblock_size!=NULL) { *pblock_size = 0; }
  }
}

// Free a pointer that is potentially allocated in a different sub-process
void _mi_free_subproc_safe(void* p) mi_attr_noexcept {
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid(p,"_mi_free_subproc_safe",&page)) {
    // const bool allow_reclaim = (_mi_subproc() == mi_page_subproc(page));
    mi_free_nonnull(p, page, NULL, false /* allow reclaim */);
  }
}

void mi_free_small(void* p) mi_attr_noexcept {
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid_ex(p,"mi_free_small",true /* is_small? */,true /*check p for null*/, &page)) {
    mi_free_nonnull(p, page, NULL, true /* allow reclaim? */);
  }
}

void mi_free_small_nonnull(void* p) mi_attr_noexcept {
  mi_assert(p!=NULL);
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid_ex(p,"mi_free_small_nonnull",true /* is_small? */,false /*check p for null*/, &page)) {
    mi_free_nonnull(p, page, NULL, true /* allow reclaim? */);
  }
}

// For runtime systems: Free a pointer that is guaranteed to be small, and in a page owned by the current thread.
static mi_decl_forceinline void mi_free_small_local_ex(void* p, bool check_p_for_null) mi_attr_noexcept {
  mi_assert(p!=NULL);
  mi_page_t* page = NULL;
  if mi_likely(mi_ptr_page_is_valid_ex(p,"mi_free_small_local_ex", true /* is_small? */, check_p_for_null /*check p for null*/, &page)) {
    mi_assert_internal(mi_page_thread_id(page) == _mi_thread_id());
    if mi_likely(mi_page_flags(page) == 0) {
      // thread-local, aligned, and not a full page
      mi_block_t* const block = mi_validate_block_from_ptr(page,p);
      mi_free_block_local(page, block, false /* was guarded */, false /* no need to check if the page is full */);
      return;
    }
    else {
      // page is local, but is full or contains (inner) aligned blocks; use generic path
      mi_free_generic_local(page, p);
    }
  }
}

void mi_free_small_local(void* p) mi_attr_noexcept {
  mi_free_small_local_ex(p, true /* check null */);
}

void mi_free_small_local_nonnull(void* p) mi_attr_noexcept {
  mi_free_small_local_ex(p, false /* check null */);
}



// ------------------------------------------------------
// Free variants
// ------------------------------------------------------

void mi_free_size(void* p, size_t size) mi_attr_noexcept {
  MI_UNUSED_RELEASE(size);
  #if MI_DEBUG
    const mi_page_t* const page = mi_ptr_page_validate(p,"mi_free_size");
    if (page==NULL) return;
    mi_assert(p!=NULL);
    const mi_block_t* block = _mi_page_ptr_unalign(page, p);
    const size_t usable = _mi_page_usable_size(page,p);
    const bool is_guarded = mi_block_ptr_is_guarded(block,p);  // guarded blocks are always in a non-small page
    if mi_unlikely(size > usable) {
      if (!is_guarded) {
        _mi_error_message(EINVAL, "pointer %p is freed with 'mi_free_size' but the size %zu is greater than the usable size %zu\n", p, size, usable);
        mi_free(p);
        return;
      }
    }
    else if mi_unlikely(size <= MI_SMALL_SIZE_MAX && !mi_page_is_small(page) && !is_guarded) {
      _mi_error_message(EINVAL, "pointer %p is freed with 'mi_free_size' with size %zu (<= MI_SMALL_SIZE_MAX), but it was allocated in a non-small page with block size %zu -- maybe a mismatched delete operator?\n", p, size, mi_page_block_size(page));
      mi_free(p);
      return;
    }
  #endif

  // note: only use `mi_free_small` if small allocations are aligned.
  // this allows building with `-DMI_OPT_FREE_SMALL=OFF` in case wrong sizes are passed to `mi_free_size`, see issue #1423
  #if MI_PAGE_META_SMALL_IS_ALIGNED
  if mi_likely(size <= MI_SMALL_SIZE_MAX) {
    mi_free_small(p);
  }
  else
  #endif
  {
    mi_free(p);
  }
}

void mi_free_size_aligned(void* p, size_t size, size_t alignment) mi_attr_noexcept {
  mi_assert(((uintptr_t)p % alignment) == 0);
  // If the alignment is smaller than the `size`, then for `size <= MI_SMALL_SIZE_MAX`
  // the block must be allocated within a small page and can thus be handled safely by `mi_free_size`.
  // (since even with over-allocation the block size will be less than 2*MI_SMALL_SIZE_MAX <= MI_SMALL_OBJ_SIZE_MAX)
  if mi_likely(alignment <= size) {
    mi_free_size(p,size);
  }
  else {
    mi_free(p);
  }
}

void mi_free_aligned(void* p, size_t alignment) mi_attr_noexcept {
  MI_UNUSED_RELEASE(alignment);
  mi_assert(((uintptr_t)p % alignment) == 0);
  mi_free(p);
}

// checked free
bool mi_cfree(void* p) mi_attr_noexcept {
  mi_page_t* const page = mi_checked_ptr_page(p);
  if mi_likely(page!=NULL) {
    mi_free_nonnull(p, page, NULL, true /* allow reclaim? */);
    return true;
  }
  else {
    return false;
  }
}


// --------------------------------------------------------------------------------------------
// `mi_free_try_collect_mt`: Potentially collect a page in a free in an abandoned page.
// 1. if the page becomes empty, free it
// 2. if it can be reclaimed, reclaim it in our theap
// 3. if it went to < 7/8th used, re-abandon to be mapped (so it can be found by theaps looking for free pages)
// --------------------------------------------------------------------------------------------

// Consume an empty page, reclaiming delayed pages if this thread has a theap for their heap.
static bool mi_abandoned_page_collect_empty(mi_page_t* page)
{
  if (!mi_page_all_free(page)) return false;
  const bool should_delay = _mi_page_qsbr_should_delay(page);
  // first remove it from the abandoned pages in the arena (if mapped, this might wait for any readers to finish)
  _mi_arenas_page_unabandon(page,NULL);
  if mi_likely(!should_delay) {
    _mi_page_qsbr_clear(page);
    _mi_arenas_page_free(page,NULL);
  }
  else {
    mi_heap_t* const heap = mi_page_heap(page);
    mi_theap_t* const theap = mi_heap_theap(heap);
    if (theap != NULL) {
      // reclaim such that it gets part of the theap's qsbr list.
      _mi_theap_page_reclaim(theap, page);
    }
    else {
      mi_assert_internal(false);
      // abandon as empty; it can only be collected by _mi_heap_collect_abandoned at this point
      _mi_arenas_page_abandon(page,NULL);
    }
  }
  return true;
}

// Helper for mi_free_try_collect_mt: try if we can reabandon a previously abandoned mostly full page to be mapped
static bool mi_abandoned_page_try_reabandon_to_mapped(mi_page_t* page)
{
  // if the page is unmapped, try to reabandon so it can possibly be mapped and found for allocations
  // We only reabandon if a full page starts to have enough blocks available to prevent immediate re-abandon of a full page
  if (mi_page_is_mostly_used(page)) return false;   // not too full (1/8th should be free)
  if (page->memid.memkind != MI_MEM_ARENA || mi_page_is_abandoned_mapped(page)) return false;  // and not already mapped (or unmappable)

  mi_assert(!mi_page_is_full(page));
  return _mi_arenas_page_try_reabandon_to_mapped(page);
}

// Release ownership of a page. This may free or reabandoned the page if other blocks are concurrently
// freed in the meantime. Returns `true` if the page was freed.
// By passing the captured `expected_thread_free`, we can often avoid calling `mi_page_free_collect`.
static void mi_abandoned_page_unown_from_free(mi_page_t* page, mi_block_t* page_thread_free) {
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(!mi_page_all_free(page));
  // try to cas atomically the original free list (`mt_free`) back with the ownership cleared.
  mi_thread_free_t tf_expect = mi_tf_create(page_thread_free,true);
  mi_thread_free_t tf_new    = mi_tf_set_owned(tf_expect,false);
  while mi_unlikely(!mi_atomic_cas_strong_acq_rel(&page->xthread_free, &tf_expect, tf_new)) {
    mi_assert_internal(mi_tf_is_owned(tf_expect));
    // while the xthread_free list is not empty..
    while (mi_tf_block(tf_expect) != NULL) {
      // if there were concurrent updates to the thread-free list, we retry to free or reabandon to mapped (if it became !mosty_used).
      _mi_page_free_collect(page,false);  // update used count
      if (mi_abandoned_page_collect_empty(page)) return;
      if (mi_abandoned_page_try_reabandon_to_mapped(page)) return;
      // otherwise continue un-owning
      tf_expect = mi_atomic_load_acquire(&page->xthread_free);
    }
    // and try again to release ownership
    mi_subproc_stat_counter_increase(mi_page_subproc(page), pages_unown_cas_retry, 1);
    mi_assert_internal(mi_tf_block(tf_expect)==NULL);
    tf_new = mi_tf_create(NULL, false);
  }
}

static inline bool mi_page_queue_len_is_atmost( mi_theap_t* theap, size_t block_size, long atmost) {
  if (atmost < 0) return false;
  mi_page_queue_t* const pq = mi_page_queue(theap,block_size);
  mi_assert_internal(pq!=NULL);
  return (pq->count <= (size_t)atmost);
}

// Helper for mi_free_try_collect_mt:  try to reclaim the page for ourselves
static mi_decl_noinline bool mi_abandoned_page_try_reclaim(mi_page_t* page, long reclaim_on_free) mi_attr_noexcept
{
  // note: reclaiming can improve benchmarks like `larson` or `rbtree-ck` a lot even in the single-threaded case,
  // since free-ing from an owned page avoids atomic operations. However, if we reclaim too eagerly in
  // a multi-threaded scenario we may start to hold on to too much memory and reduce reuse among threads.
  // If the current theap is where the page originally came from, we reclaim much more eagerly while
  // 'cross-thread' reclaiming on free is by default off (and we only 'reclaim' these by finding the abandoned
  // pages when we allocate a fresh page).
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(!mi_page_all_free(page));
  mi_assert_internal(page->block_size <= MI_MAX_RECLAIM_OBJ_SIZE);
  mi_assert_internal(reclaim_on_free >= 0);

  // dont reclaim if we just have terminated this thread and we should
  // not reinitialize the theap for this thread. (can happen due to thread-local destructors for example -- issue #944)
  if (!_mi_thread_is_initialized()) return false;

  // get our theap
  mi_theap_t* const theap = _mi_page_associated_theap_peek(page);
  if (theap==NULL || theap->tld==NULL || !theap->allow_page_reclaim) return false;  // see issue #1289

  // todo: cache `is_in_threadpool` and `exclusive_arena` directly in the theap for performance?
  // set max_reclaim limit
  long max_reclaim = 0;
  if mi_likely(theap == page->theap) {  // did this page originate from the current theap? (and thus allocated from this thread)
    // originating theap
    max_reclaim = _mi_option_get_fast(theap->tld->is_in_threadpool ? mi_option_page_cross_thread_max_reclaim : mi_option_page_max_reclaim);
  }
  else if (reclaim_on_free == 1 &&               // if cross-thread is allowed
            !theap->tld->is_in_threadpool &&     // and we are not part of a threadpool
            !mi_page_is_used_at_frac(page,4) &&  // and at least 1/4th of the page is free
            _mi_arena_memid_is_suitable(page->memid, mi_theap_heap(theap)->exclusive_arena)) {   // and it fits our memory
    // across threads
    max_reclaim = _mi_option_get_fast(mi_option_page_cross_thread_max_reclaim);
  }
  else {
    return false;
  }

  // are we within the reclaim limit?
  if (max_reclaim >= 0 && !mi_page_queue_len_is_atmost(theap, page->block_size, max_reclaim)) {
    return false;
  }

  // reclaim the page into this theap
  // first remove it from the abandoned pages in the arena -- this might wait for any readers to finish
  _mi_arenas_page_unabandon(page, theap);
  _mi_theap_page_reclaim(theap, page);
  mi_theap_stat_counter_increase(theap, pages_reclaim_on_free, 1);
  return true;
}


// We freed a block in an abandoned page (that was not owned). Try to collect
static void mi_decl_noinline mi_free_try_collect_mt(mi_page_t* page, mi_block_t* mt_free, bool allow_reclaim) mi_attr_noexcept
{
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(mt_free != NULL);
  // mi_assert_internal(!allow_reclaim || _mi_subproc() == mi_page_subproc(page));  // never reclaim across subprocesses

  // we own the page now, and it is safe to collect the thread atomic free list
  if (page->block_size <= MI_SMALL_SIZE_MAX) {
    // use the `_partly` version to avoid atomic operations since we already have the `mt_free` pointing into the thread free list
    // (after this the `used` count might be too high (as some blocks may have been concurrently added to the thread free list and are yet uncounted).
    //  however, if the page became completely free, the used count is guaranteed to be 0.)
    mi_assert_internal(page->reserved>=16); // below this even one freed block goes from full to no longer mostly used.
    mt_free = _mi_page_free_collect_partly(page, mt_free);
  }
  else {
    // for larger blocks we use the regular collect
    _mi_page_free_collect(page,false /* no force */);
    mt_free = NULL; // expected page->xthread_free value after collection
  }
  const long reclaim_on_free = mi_heap_reclaim_on_free(mi_page_heap(page));
  #if MI_DEBUG > 1
  if (mi_page_is_singleton(page)) { mi_assert_internal(mi_page_all_free(page)); }
  if (mi_page_is_full(page))      { mi_assert(mi_page_is_mostly_used(page)); }
  #endif

  // try to: 1. free it, 2. reclaim it, or 3. reabandon it to be mapped
  if (mi_abandoned_page_collect_empty(page)) return;
  if (allow_reclaim && page->block_size <= MI_MAX_RECLAIM_OBJ_SIZE && reclaim_on_free >= 0) {  // early test for better codegen
    if (mi_abandoned_page_try_reclaim(page, reclaim_on_free)) return;
  }
  if (mi_abandoned_page_try_reabandon_to_mapped(page)) return;

  // otherwise unown the page again
  mi_abandoned_page_unown_from_free(page, mt_free);
}


// ------------------------------------------------------
// Usable size
// ------------------------------------------------------

// Bytes available in a block
static size_t mi_decl_noinline mi_page_usable_aligned_size_of(const mi_page_t* page, const void* p, const mi_block_t* block) mi_attr_noexcept {
  // const mi_block_t* block = _mi_page_ptr_unalign(page, p);
  const bool is_guarded = mi_block_ptr_is_guarded(block,p);
  const size_t size = mi_page_usable_size_of(page, block, is_guarded);
  mi_assert_internal((void*)p >= (void*)block);
  const size_t adjust = (uint8_t*)p - (uint8_t*)block;
  mi_assert_internal(adjust <= size);
  const size_t aligned_size = (adjust <= size ? size - adjust : 0);  // size can be zero if the padding is corrupted
  return aligned_size;
}

size_t _mi_page_usable_size(const mi_page_t* page, const void* p) mi_attr_noexcept {
  if mi_unlikely(page==NULL) return 0;
  mi_assert_internal(mi_ptr_page_validate(p,"_mi_page_usable_size") == page);
  const mi_block_t* block = _mi_page_ptr_unalign(page, p); // for safety, always unalign
  if mi_likely((const void*)block==page) {
    return mi_page_usable_size_of(page, block, false /* is guarded */);
  }
  else {
    // split out to separate routine for improved code generation
    return mi_page_usable_aligned_size_of(page, p, block);
  }
}

mi_decl_nodiscard size_t mi_usable_size(const void* p) mi_attr_noexcept {
  const mi_page_t* const page = mi_ptr_page_validate(p,"mi_usable_size");
  return _mi_page_usable_size(page,p);
}

mi_decl_nodiscard size_t mi_thread_id_of(const void* p) mi_attr_noexcept {
  if (p == NULL) return 0;
  const mi_page_t* const page = mi_ptr_page(p);
  const mi_threadid_t tid = mi_page_thread_id(page);
  return (tid <= MI_THREADID_DETACHED ? 0 : tid);
}


// ------------------------------------------------------
// Deprecated: double free is usually checked with padding now
// as that is faster and works better for cross-thread free'ing.
// Check for double free in secure and debug mode
// This is somewhat expensive so only enabled for secure mode 4
// ------------------------------------------------------

#if MI_SECURE>=3 && !MI_PADDING
// linear check if the free list contains a specific element
static bool mi_list_contains(const mi_page_t* page, const mi_block_t* list, const mi_block_t* elem, const char* list_kind) {
  const size_t max_count = page->capacity;      // can never hold more blocks than the capacity
  size_t count = 0;
  while (list != NULL && count <= max_count) {  // double-free can create cycles so we limit the number of iterations
    if (elem==list) return true;
    list = mi_block_next(page, list);
    count++;
  }
  if mi_unlikely(count > max_count) {
    _mi_error_message(EFAULT, "corrupted %s list (possibly due to a double free)\n", list_kind);
  }
  return false;
}

static mi_decl_noinline bool mi_check_double_freex(const mi_page_t* page, const mi_block_t* block) {
  // The decoded value is in the same page (or NULL).
  // Walk the free lists to verify positively if it is already freed
  if (mi_list_contains(page, page->free, block, "free") ||
      mi_list_contains(page, page->local_free, block, "local free") ||
      mi_list_contains(page, mi_page_thread_free(page), block, "thread free"))
  {
    _mi_error_message(EAGAIN, "double free detected of block %p with size %zu\n", block, mi_page_block_size(page));
    return false;
  }
  return true;
}

// Used for double free checking to avoid checking free lists too frequently
static inline bool mi_block_could_be_double_free(const mi_page_t* page, const mi_block_t* block) {
  mi_block_t* n = mi_block_nextx(page,block,page->keys);
  return (mi_is_aligned(block,MI_SIZE_SIZE) &&           // quick check: aligned pointer?
          (n==NULL || mi_page_contains_address(page,n))); // quick check: in the same page or NULL?
}

// check if `block` was free'd before
static inline bool mi_check_double_free(const mi_page_t* page, const mi_block_t* block) {
  if mi_unlikely(mi_block_could_be_double_free(page,block))  // quick check: next field is aligned in the same page or NULL?
  {
    // Suspicious: decoded value a in block is in the same page (or NULL) -- maybe a double free?
    // (continue in separate function to improve code generation)
    return mi_check_double_freex(page, block);
  }
  else return true;
}
#else
static inline bool mi_check_double_free(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(page);
  MI_UNUSED(block);
  return true;
}
#endif


// ---------------------------------------------------------------------------
// Check for theap block overflow by setting up padding at the end of the block
// ---------------------------------------------------------------------------

#if MI_PADDING // && !MI_TRACK_ENABLED
static inline bool mi_page_decode_padding(const mi_page_t* page, const mi_block_t* block, size_t* delta, size_t* bsize, bool* double_free) {
  *bsize = mi_page_usable_block_size(page);
  mi_padding_t* const padding = (mi_padding_t*)((uint8_t*)block + *bsize);
  mi_track_mem_defined(padding,sizeof(mi_padding_t));
  *delta = padding->delta;
  const uint32_t canary = padding->canary;
  const bool ok = (mi_ptr_encode_canary(page,block,page->keys) == canary && *delta <= *bsize);
  if (double_free!=NULL) {
    if mi_unlikely(!ok) { *double_free = mi_ptr_decode_canary_is_freed(canary); }   // double free?
                   else { padding->canary = mi_ptr_encode_canary_freed(); }         // mark as freed
  }
  mi_track_mem_noaccess(padding,sizeof(mi_padding_t));
  return ok;
}

// Return the exact usable size of a block.
static size_t mi_page_usable_size_of(const mi_page_t* page, const mi_block_t* block, bool is_guarded) {
  if mi_unlikely(is_guarded) {
    const size_t bsize = mi_page_block_size(page);
    return (bsize - _mi_os_page_size());
  }
  else {
    size_t bsize;
    size_t delta;
    bool ok = mi_page_decode_padding(page, block, &delta, &bsize, NULL);
    mi_assert_internal(ok); mi_assert_internal(delta <= bsize);
    return (ok ? bsize - delta : 0);
  }
}

// When a non-thread-local block is freed, it becomes part of the thread delayed free
// list that is freed later by the owning theap. If the exact usable size is too small to
// contain the pointer for the delayed list, then shrink the padding (by decreasing delta)
// so it will later not trigger an overflow error in `mi_free_block`.
void _mi_padding_shrink(const mi_page_t* page, const mi_block_t* block, const size_t min_size) {
  if (mi_block_ptr_is_guarded(block,block + 1)) return;
  size_t bsize;
  size_t delta;
  bool ok = mi_page_decode_padding(page, block, &delta, &bsize, NULL);
  mi_assert_internal(ok);
  if (!ok || (bsize - delta) >= min_size) return;  // usually already enough space
  mi_assert_internal(bsize >= min_size);
  if (bsize < min_size) return;  // should never happen
  size_t new_delta = (bsize - min_size);
  mi_assert_internal(new_delta < bsize);
  mi_padding_t* padding = (mi_padding_t*)((uint8_t*)block + bsize);
  mi_track_mem_defined(padding,sizeof(mi_padding_t));
  padding->delta = (uint32_t)new_delta;
  mi_track_mem_noaccess(padding,sizeof(mi_padding_t));
}
#else
static inline size_t mi_page_usable_size_of(const mi_page_t* page, const mi_block_t* block, bool is_guarded) {
  MI_UNUSED(block);
  if mi_unlikely(is_guarded) {
    const size_t bsize = mi_page_block_size(page);
    return (bsize - _mi_os_page_size());
  }
  else {
    return mi_page_usable_block_size(page);
  }
}

void _mi_padding_shrink(const mi_page_t* page, const mi_block_t* block, const size_t min_size) {
  MI_UNUSED(page); MI_UNUSED(block); MI_UNUSED(min_size);
}
#endif

#if MI_PADDING

static bool mi_verify_padding(const mi_page_t* page, const mi_block_t* block, size_t* size, size_t* wrong, bool* is_double_free) {
  size_t bsize;
  size_t delta;
  bool ok = mi_page_decode_padding(page, block, &delta, &bsize, is_double_free );
  *size = *wrong = bsize;
  if (!ok) return false;
  mi_assert_internal(bsize >= delta);
  *size = bsize - delta;
  #if MI_PADDING_CHECK_BYTES
  if (!mi_page_is_huge(page)) {
    uint8_t* fill = (uint8_t*)block + bsize - delta;
    const size_t maxpad = (delta > MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : delta); // check at most the first N padding bytes
    mi_track_mem_defined(fill, maxpad);
    for (size_t i = 0; i < maxpad; i++) {
      if (fill[i] != MI_DEBUG_PADDING) {
        *wrong = bsize - delta + i;
        ok = false;
        break;
      }
    }
    mi_track_mem_noaccess(fill, maxpad);
  }
  #endif
  return ok;
}

mi_decl_nodiscard static bool mi_check_padding_on_free(const mi_page_t* page, const mi_block_t* block, bool is_guarded, size_t* usable_size) {
  if mi_unlikely(is_guarded) {
    const size_t bsize = mi_page_block_size(page);
    *usable_size = (bsize - _mi_os_page_size());
    return true;
  }
  else {
    size_t wrong;
    bool is_double_free;
    if mi_unlikely(!mi_verify_padding(page,block,usable_size,&wrong,&is_double_free)) {
      if (is_double_free) {
        _mi_error_message(EAGAIN, "double free detected of heap block %p with size %zu\n", block, *usable_size);
      }
      else {
        _mi_error_message(EFAULT, "buffer overflow in heap block %p of size %zu: write after %zu bytes\n", block, *usable_size, wrong );
      }
      return false;
    }
    return true;
  }
}

#else

mi_decl_nodiscard static bool mi_check_padding_on_free(const mi_page_t* page, const mi_block_t* block, bool is_guarded, size_t* usable_size) {
  *usable_size = mi_page_usable_size_of(page,block,is_guarded);
  return true;
}

#endif

// only maintain stats for smaller objects if requested
#if 0 && (MI_STATS>0)
static void mi_stat_free(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(block);
  mi_theap_t* theap = _mi_theap_default();
  mi_lock_t* lock = NULL;
  mi_subproc_t* const subproc = mi_page_subproc(page);
  mi_theap_t* const theap_meta = subproc->theap_meta;
  if mi_unlikely(!mi_theap_is_initialized(theap) || // can happen if free'd after thread_done was called (usually a thread cleanup call by the OS)
                  // page->theap == subproc->theap_meta  .. but we cannot read `theap` if we don't own the page
                  (theap_meta != NULL && mi_page_thread_id(page) == theap_meta->tld->thread_id)) {
    theap = theap_meta;
    lock = &subproc->theap_meta_lock;
    mi_lock_acquire(lock);
  }

  const size_t bsize = mi_page_usable_block_size(page);
  // #if (MI_STATS>=2)
  // const size_t usize = mi_page_usable_size_of(page, block);
  // mi_theap_stat_decrease(theap, malloc_requested, usize);
  // #endif
  if (bsize <= MI_LARGE_MAX_OBJ_SIZE) {
    mi_theap_stat_decrease(theap, malloc_normal, bsize);
    #if (MI_STATS > 1)
    mi_theap_stat_decrease(theap, malloc_bins[_mi_bin(bsize)], 1);
    #endif
  }
  else {
    const size_t bpsize = mi_page_block_size(page);  // match stat in page.c:mi_huge_page_alloc
    mi_theap_stat_decrease(theap, malloc_huge, bpsize);
  }

  if mi_unlikely(lock!=NULL) {
    mi_lock_release(lock);
  }
}
#else
void mi_stat_free(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(page); MI_UNUSED(block);
}
#endif
