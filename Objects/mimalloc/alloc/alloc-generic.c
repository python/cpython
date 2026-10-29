/*----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* -----------------------------------------------------------
  The "slow" allocation path (`mi_malloc_generic`), including
  finding suitable pages with free blocks.
----------------------------------------------------------- */

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/alloc.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/profile/sample-profile.h"
#include "page-queue.h"
#include "page.h"

// returns `true` if after collection `mi_page_immediate_available` is true.
static inline bool mi_page_free_quick_collect(mi_page_t* page) {
  if mi_likely(page->free != NULL) return true;
  if (page->local_free == NULL) return false;
  // move local_free to free
  page->free = page->local_free;
  page->local_free = NULL;
  page->free_is_zero = false;
  _mi_page_update_sample_countdown(page);
  return true;
}


/* -----------------------------------------------------------
  Find pages with free blocks
-------------------------------------------------------------*/

#define MI_XCOLLECT_SCORE_MAX   (256)  // saturation limit for `xcollect_score`
#define MI_XCOLLECT_SCORE_SHIFT   (5)  // retain bonus = score >> shift  (max 8)
#define MI_XCOLLECT_SCORE_INC     (8)  // a revived page saves a full abandon/reclaim round-trip


// Find a page with free blocks of `page->block_size`.
static mi_decl_noinline mi_page_t* mi_page_queue_find_free_ex(mi_theap_t* theap, mi_page_queue_t* pq, bool first_try)
{
  // search through the pages in "next fit" order
  size_t count = 0;
  long candidate_limit = 0;          // we reset this on the first candidate to limit the search
  long page_full_retain = (pq->block_size > MI_SMALL_MAX_OBJ_SIZE ? 0 : theap->page_full_retain); // only retain small pages
  if (page_full_retain >= 0) {
    // adaptively retain more full pages when cross-thread frees keep reviving them (producer/consumer patterns)
    page_full_retain += (long)(pq->xcollect_score >> MI_XCOLLECT_SCORE_SHIFT);
  }
  mi_page_t* page_candidate = NULL;  // a page with free space
  mi_page_t* page = pq->first;
  mi_page_t* const last = pq->last;

  while (page!=NULL)
  {
    mi_page_t* next = page->next; // remember next (as this page can move to another queue)
    count++;
    candidate_limit--;

    // search up to N pages for a best candidate

    // is the local free list non-empty?
    bool immediate_available = mi_page_immediate_available(page);
    if (!immediate_available) {
      // collect freed blocks by us and other threads to we get a proper use count
      const bool collected_xfree = _mi_page_free_collect(page, false);
      immediate_available = mi_page_immediate_available(page);
      if (collected_xfree && immediate_available) {
        // a cross-thread free revived this page: retaining such pages avoids an abandon/reclaim round-trip
        pq->xcollect_score = mi_min(pq->xcollect_score + MI_XCOLLECT_SCORE_INC, MI_XCOLLECT_SCORE_MAX);
      }
      else if (pq->xcollect_score > 0) {
        pq->xcollect_score--;
      }
    }

    // if the page is completely full, move it to the `mi_pages_full`
    // queue so we don't visit long-lived pages too often.
    if (!immediate_available && !mi_page_is_expandable(page)) {
      page_full_retain--;
      if (page_full_retain < 0) {
        mi_assert_internal(!mi_page_is_in_full(page) && !mi_page_immediate_available(page));
        _mi_page_to_full(page, pq);
      }
      else if (page!=last && pq->last!=page) {
        // avoid revisiting this page for a while
        mi_page_queue_move_to_back(theap, pq, page);
      }
    }
    else {
      // the page has free space, make it a candidate
      // we prefer non-expandable pages with high usage as candidates (to reduce commit, and increase chances of free-ing up pages)
      if (page_candidate == NULL) {
        page_candidate = page;
        candidate_limit = _mi_option_get_fast(mi_option_page_max_candidates);
      }
      else if (mi_page_all_free(page_candidate)) {
        _mi_page_free(page_candidate, pq);
        page_candidate = page;
      }
      // prefer to reuse fuller pages (in the hope the less used page gets freed)
      else if (mi_page_used(page) >= mi_page_used(page_candidate) && !mi_page_is_mostly_used(page)) { // && !mi_page_is_expandable(page)) {
        page_candidate = page;
      }
      // if we find a non-expandable candidate, or searched for N pages, return with the best candidate
      if (immediate_available || candidate_limit <= 0) {
        mi_assert_internal(page_candidate!=NULL);
        break;
      }
    }

  #if 0
    // first-fit algorithm without candidates
    // If the page contains free blocks, we are done
    if (mi_page_immediate_available(page) || mi_page_is_expandable(page)) {
      break;  // pick this one
    }

    // If the page is completely full, move it to the `mi_pages_full`
    // queue so we don't visit long-lived pages too often.
    mi_assert_internal(!mi_page_is_in_full(page) && !mi_page_immediate_available(page));
    _mi_page_to_full(page, pq);
  #endif
    if (page==last) { page=NULL; }  // don't revisit earlier pages that moved to the back
              else  { page = next; }
  } // for each page

  mi_theap_stat_counter_increase(theap, page_searches, count);
  mi_theap_stat_counter_increase(theap, page_searches_count, 1);

  // set the page to the best candidate
  if (page_candidate != NULL) {
    page = page_candidate;
  }
  if (page != NULL) {
    if (!mi_page_immediate_available(page)) {
      mi_assert_internal(mi_page_is_expandable(page));
      if (!_mi_page_extend_free(theap, page)) {
        page = NULL; // failed to extend
      }
    }
    mi_assert_internal(page == NULL || mi_page_immediate_available(page));
  }

  if (page == NULL) {
    _mi_theap_collect_retired(theap, false); // perhaps make a page available
    page = _mi_page_fresh(theap, pq);
    mi_assert_internal(page == NULL || mi_page_immediate_available(page));
    if (page == NULL && first_try) {
      // out-of-memory _or_ an abandoned page with free blocks was reclaimed, try once again
      page = mi_page_queue_find_free_ex(theap, pq, false);
      mi_assert_internal(page == NULL || mi_page_immediate_available(page));
    }
  }
  else {
    mi_assert_internal(page == NULL || mi_page_immediate_available(page));
    // move the page to the front of the queue
    mi_page_queue_move_to_front(theap, pq, page);
    page->retire_expire = 0;
    _mi_page_qsbr_clear(page);
    // _mi_theap_collect_retired(theap, false); // update retire counts; note: increases rss on MemoryLoad bench so don't do this
  }
  mi_assert_internal(page == NULL || mi_page_immediate_available(page));


  return page;
}

// Look for a page with free blocks of `size` but don't try to search or allocate
static inline mi_page_t* mi_page_queue_lookup_free_first(mi_theap_t* theap, mi_page_queue_t* pq) {
  mi_assert_internal(!mi_page_queue_is_huge(pq));
  mi_page_t* page = pq->first;
  if mi_likely(page!=NULL && mi_page_free_quick_collect(page)) {
    // fast path
    #if (MI_SECURE>=2) // in secure mode, we extend half the time to increase randomness
    if (page->capacity < page->reserved && ((_mi_theap_random_next(theap) & 1) == 1)) {
      (void)_mi_page_extend_free(theap, page);  // ok if this fails
      mi_assert_internal(mi_page_immediate_available(page));
    }
    #else
    MI_UNUSED(theap);
    #endif
    page->retire_expire = 0;
    _mi_page_qsbr_clear(page);
    mi_assert_internal(mi_page_immediate_available(page));
    mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
    mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
    return page;
  }
  else {
    return NULL;
  }
}

// Find a page with free blocks of `size`.
static inline mi_page_t* mi_page_queue_find_free(mi_theap_t* theap, mi_page_queue_t* pq) {
  // mi_page_queue_t* pq = mi_page_queue(theap, size);
  mi_assert_internal(!mi_page_queue_is_huge(pq));

  // check the first page: we even do this with candidate search or otherwise we re-search every time
  mi_page_t* page = mi_page_queue_lookup_free_first(theap,pq);
  if (page==NULL) {
    page = mi_page_queue_find_free_ex(theap, pq, true);
    if (page==NULL) return NULL;
  }
  mi_assert_internal(mi_page_immediate_available(page));
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  return page;
}

// Huge pages contain just one block, and the segment contains just that page.
// Huge pages are also use if the requested alignment is very large (> MI_BLOCK_ALIGNMENT_MAX)
// so their size is not always `> MI_LARGE_OBJ_SIZE_MAX`.
static mi_page_t* mi_huge_page_alloc(mi_theap_t* theap, size_t size, size_t page_alignment, mi_page_queue_t* pq) {
  _mi_theap_qsbr_collect_delayed(theap);
  const size_t block_size = _mi_os_good_alloc_size(size);
  // mi_assert_internal(mi_bin(block_size) == MI_BIN_HUGE || page_alignment > 0);
  #if MI_HUGE_PAGE_ABANDON
  #error todo.
  #else
  // mi_page_queue_t* pq = mi_page_queue(theap, MI_LARGE_MAX_OBJ_SIZE+1);  // always in the huge queue regardless of the block size
  mi_assert_internal(mi_page_queue_is_huge(pq));
  #endif
  mi_page_t* page = _mi_page_fresh_alloc(theap, pq, block_size, page_alignment);
  if (page != NULL) {
    mi_assert_internal(mi_page_block_size(page) >= size);
    mi_assert_internal(mi_page_immediate_available(page));
    mi_assert_internal(mi_page_is_huge(page));
    mi_assert_internal(mi_page_is_singleton(page));
    #if MI_HUGE_PAGE_ABANDON
    mi_assert_internal(mi_page_is_abandoned(page));
    mi_page_set_theap(page, NULL);
    #endif
    // mi_theap_stat_increase(theap, malloc_huge, mi_page_block_size(page));
    // mi_theap_stat_counter_increase(theap, malloc_huge_count, 1);
  }
  return page;
}

// Allocate a page
// Note: in debug mode the size includes MI_PADDING_SIZE and might have overflowed.
static mi_page_t* mi_find_page(mi_theap_t* theap, size_t size, size_t huge_alignment) mi_attr_noexcept {
  const size_t req_size = size - MI_PADDING_SIZE;  // correct for padding_size in case of an overflow on `size`
  if mi_unlikely(req_size > MI_MAX_ALLOC_SIZE) {   // TODO: remove as we check in generic_fallback ?
    _mi_error_message(EOVERFLOW, "allocation request is too large (%zu bytes)\n", req_size);
    return NULL;
  }
  mi_page_queue_t* pq = mi_page_queue(theap, (huge_alignment > 0 ? MI_LARGE_MAX_OBJ_SIZE+1 : size));
  mi_page_t* page;
  // huge allocation?
  if mi_unlikely(mi_page_queue_is_huge(pq) || req_size > MI_MAX_ALLOC_SIZE) {
    page = mi_huge_page_alloc(theap,size,huge_alignment,pq);
  }
  else {
    // otherwise find a page with free blocks in our size segregated queues
    #if MI_PADDING
    mi_assert_internal(size >= MI_PADDING_SIZE);
    #endif
    page = mi_page_queue_find_free(theap,pq);
  }
  if (page==NULL) return NULL;
  mi_assert_internal(mi_page_block_size(page) >= size);
  mi_assert_internal(mi_page_immediate_available(page));
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  return page;
}



/* -----------------------------------------------------------
  Users can register a deferred free function called
  when the `free` list is empty. Since the `local_free`
  is separate this is deterministically called after
  a certain number of allocations.
----------------------------------------------------------- */

// The program should only install a single deferred free handler before doing allocation.
static _Atomic(void*) deferred_free; // is `mi_deferred_free_fun*` (but some platforms don't support atomic function pointers)
static _Atomic(void*) deferred_arg;

void _mi_deferred_free(mi_theap_t* theap, bool force) {
  theap->heartbeat++;
  mi_deferred_free_fun* const fun = (mi_deferred_free_fun*)mi_atomic_load_ptr_acquire(void,&deferred_free);
  if (fun != NULL && !theap->tld->recurse) {
    theap->tld->recurse = true;
    void* const arg = mi_atomic_load_ptr_acquire(void,&deferred_arg);
    fun(force, theap->heartbeat, arg);
    theap->tld->recurse = false;
  }
}

void mi_register_deferred_free(mi_deferred_free_fun* fn, void* arg) mi_attr_noexcept {
  mi_atomic_store_ptr_release(void,&deferred_arg, arg);
  mi_atomic_store_ptr_release(void,&deferred_free, (void*)fn);
}


/* -----------------------------------------------------------
  Admin
----------------------------------------------------------- */
static mi_theap_t* mi_theap_init(mi_theap_t* theap) {
  if mi_likely(mi_theap_is_initialized(theap)) return theap;
  if (theap==&_mi_theap_empty_wrong) {
    // we were unable to allocate a theap for a first-class heap
    return NULL;
  }
  // otherwise we initialize the thread and its default theap
  theap = _mi_thread_init();
  if mi_unlikely(!mi_theap_is_initialized(theap)) { return NULL; }
  mi_assert_internal(mi_theap_is_initialized(theap));
  return theap;
}

static mi_theap_t* mi_malloc_generic_admin(mi_theap_t* theap)
{
  theap = mi_theap_init(theap);
  if (theap==NULL) return NULL;
  mi_assert_internal(mi_theap_is_initialized(theap));

  // do administrative tasks every N generic mallocs
  if mi_unlikely(theap->generic_count >= 1000) {
    theap->generic_collect_count += theap->generic_count;
    theap->generic_count = 0;

    // check if the profiler is enabled (unless this theap is permanently excluded, see `mi_theap_profile_never`)
    if (!theap->profile_never) {
      mi_heap_t* const heap = mi_theap_heap(theap);
      mi_profiler_t* prof = mi_atomic_load_ptr_relaxed(mi_profiler_t, &heap->profiler);
      const bool prof_enabled = (prof!=NULL && mi_profiler_is_enabled(prof));
      if (prof_enabled && theap->profile_sample_rate==0) {
        _mi_theap_set_profile_sample_rate(theap,mi_max(1,prof->initial_sample_rate)); // start profiling
      }
      else if (!prof_enabled && theap->profile_sample_rate!=0) {
        _mi_theap_set_profile_sample_rate(theap,0); // stop profiling
      }
    }

    // do a full theap collect every once in a while (10000 by default)
    const long generic_collect = mi_option_get_clamp(mi_option_generic_collect, 1, 1000000L);
    if (theap->generic_collect_count >= generic_collect) {
      theap->generic_collect_count = 0;
      mi_theap_collect(theap, false /* force? */);
    }
    // else if (theap->sample_rate != 0) {             // update stats more aggressively if we are sampling
    //   mi_theap_collect(theap, false /* force? */);  // TODO: make specialized mi_theap_collect_update_stats ?
    // }
    else {
      // otherwise we do a mini-collect
      _mi_deferred_free(theap, false);         // call potential deferred free routines
      _mi_theap_collect_retired(theap, false); // free retired pages
    }
  }
  return theap;
}

/* -----------------------------------------------------------
  Generic allocation
----------------------------------------------------------- */

static bool mi_malloc_generic_check_size(size_t size) {
  const size_t req_size = size - MI_PADDING_SIZE;
  if mi_unlikely(req_size > MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE) {
    _mi_error_message(EOVERFLOW, "allocation request is too large (%zu bytes)\n", req_size);
    return false;
  }
  return true;
}

// Allocate from a page of the right size (without sampling)
static void* mi_malloc_generic_from_page(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage, size_t sample_countdown_adjust)
{
  // find (or allocate) a page of the right size
  mi_page_t* page = mi_find_page(theap, size, huge_alignment);
  if mi_unlikely(page == NULL) { // first time out of memory, try to collect and retry the allocation once more
    mi_theap_collect(theap, true /* force? */);
    page = mi_find_page(theap, size, huge_alignment);
  }

  if mi_unlikely(page == NULL) { // out of memory
    theap->sample_countdown -= sample_countdown_adjust;  // unadjust again if we failed to find a page
    _mi_error_message(ENOMEM, "unable to allocate memory (%zu bytes)\n", size - MI_PADDING_SIZE);
    return NULL;
  }

  mi_assert_internal(mi_page_immediate_available(page));
  mi_assert_internal(mi_page_block_size(page) >= size);
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);

  // and try again, this time succeeding! (i.e. this should never recurse through _mi_page_malloc_zero)
  if (ppage!=NULL) { *ppage = page; }
  void* const p = _mi_page_malloc_zero(theap,page,size,zero);
  mi_assert_internal(p != NULL);
  _mi_page_update_sample_countdown(page);

  // move full pages to the full queue
  // this will also call _mi_page_update_stats for huge pages
  if (mi_page_block_size(page) > MI_SMALL_MAX_OBJ_SIZE) {
    if (mi_page_is_full(page)) {
      _mi_page_to_full(page, mi_page_queue_of(page));
    }
  }
  return p;
}

// Called when the sample countdown is reached: take a sample if possible, or otherwise allocate from a page
static mi_decl_noinline void* mi_malloc_generic_sample(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage)
{
  const size_t req_size = size - MI_PADDING_SIZE;
  if (huge_alignment==0 && theap->sample_rate!=0) {
    return _mi_theap_malloc_sampled(theap,req_size,zero,ppage);
  }
  mi_assert_internal(!_mi_is_empty_theap(theap));       // cannot write to the empty theap
  mi_assert_internal(req_size <= MI_SAMPLE_COUNTDOWN_MAX);
  size_t sample_countdown_adjust = 0;
  if (theap->sample_rate==0) {
    theap->sample_countdown = MI_SAMPLE_COUNTDOWN_MAX;  // reset the countdown counter
  }
  #if MI_SAMPLE==2
  else {
    // huge_alignment!=0 so we need to adjust the countdown
    mi_assert_internal(huge_alignment!=0);
    mi_assert_internal(theap->sample_countdown <= SIZE_MAX - req_size);
    sample_countdown_adjust = req_size;
    theap->sample_countdown += req_size;                // adjust such that after `_mi_page_malloc_zero` the countdown is correct again
  }
  #endif
  return mi_malloc_generic_from_page(theap, size, zero, huge_alignment, ppage, sample_countdown_adjust);
}

static mi_decl_noinline void* mi_malloc_generic_fallback(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage)
{
  // initialize if necessary
  theap = mi_malloc_generic_admin(theap);
  if (theap==NULL) return NULL;

  // check allocation size
  if (!mi_malloc_generic_check_size(size)) return NULL;

  if mi_unlikely(mi_theap_should_sample(theap, size - MI_PADDING_SIZE)) {
    // sampled allocation
    return mi_malloc_generic_sample(theap, size, zero, huge_alignment, ppage);
  }
  else {
    // direct allocation
    return mi_malloc_generic_from_page(theap, size, zero, huge_alignment, ppage, 0);
  }
}


// Generic allocation routine if the fast path (`alloc/alloc.c:mi_page_malloc`) does not succeed.
// Note: in debug mode the size includes MI_PADDING_SIZE and might have overflowed.
// The `huge_alignment` is normally 0 but is set to a multiple of MI_SLICE_SIZE for
// very large requested alignments in which case we use a huge singleton page.
// Note: we put `bool zero, size_t huge_alignment` into one parameter (with zero in bit 0)
// to use 4 parameters which compiles better on msvc for the malloc fast path.
mi_decl_restrict void* _mi_malloc_generic(mi_theap_t* theap, size_t size, size_t zero_huge_alignment, mi_page_t** ppage) mi_attr_noexcept
{
  #if !MI_THEAP_INITASNULL
  mi_assert_internal(theap != NULL);
  #endif
  const bool zero = ((zero_huge_alignment & 1) != 0);
  const size_t huge_alignment = (zero_huge_alignment & ~1);
  mi_assert_internal(huge_alignment==0 || huge_alignment > MI_PAGE_MAX_OVERALLOC_ALIGN);
  mi_page_t* page = NULL;

  // fast path objects that fit in a small page
  if mi_likely(mi_theap_is_initialized(theap) && ++theap->generic_count < 1000 && huge_alignment==0) {
    const size_t req_size = size - MI_PADDING_SIZE;  // correct for padding_size in case of an overflow on `size`
    if (req_size < MI_SMALL_MAX_OBJ_SIZE)
    {
      #if MI_SAMPLE
      if mi_likely(!mi_theap_should_sample(theap,req_size)) // ensure we don't need to take a sample
      #endif
      {
        mi_page_queue_t* pq = mi_page_queue(theap, size);
        mi_assert_internal(pq!=NULL && !mi_page_queue_is_huge(pq));
        page = mi_page_queue_find_free(theap,pq);
        // mi_assert_internal(mi_page_block_size(page) <= MI_SMALL_MAX_OBJ_SIZE);
        if (page!=NULL) {
          if (ppage!=NULL) { *ppage = page; }
          mi_assert_internal(mi_page_immediate_available(page)); // we should never recurse in _mi_page_malloc_zero
          void* p = _mi_page_malloc_zero(theap,page,size,zero);  // always succeeds
          mi_assert_internal(p != NULL);
          return p;
        }
      }
    }
  }
  // otherwise fallback
  return mi_malloc_generic_fallback(theap,size,zero,huge_alignment,ppage);
}

// Generic allocation without sampling
mi_decl_restrict void* _mi_malloc_generic_no_sample(mi_theap_t* theap, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  // do the admin first as it can update the sample rate; we never call it (or the sampling path) recursively here
  theap = mi_malloc_generic_admin(theap);
  if (theap==NULL) return NULL;
  if (!mi_malloc_generic_check_size(size)) return NULL;
  const size_t sample_countdown = theap->sample_countdown;
  theap->sample_countdown = MI_SAMPLE_COUNTDOWN_MAX;  // don't count this allocation
  void* p = mi_malloc_generic_from_page(theap, size, zero, 0, ppage, 0);
  // the sample rate may have changed (e.g. by a profiler `on_free` callback during a collect)
  theap->sample_countdown = (theap->sample_rate!=0 && sample_countdown > theap->sample_rate ? theap->sample_rate : sample_countdown);
  return p;
}
