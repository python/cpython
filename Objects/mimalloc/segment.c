/* ----------------------------------------------------------------------------
Copyright (c) 2018-2020, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc-internal.h"
#include "mimalloc-atomic.h"

#include <string.h>  // memset
#include <stdio.h>

#define MI_PAGE_HUGE_ALIGN  (256*1024)

static uint8_t* mi_segment_raw_page_start(const mi_segment_t* segment, const mi_page_t* page, size_t* page_size);

/* --------------------------------------------------------------------------------
  Segment allocation
  We allocate pages inside bigger "segments" (4mb on 64-bit). This is to avoid
  splitting VMA's on Linux and reduce fragmentation on other OS's.
  Each thread owns its own segments.

  Currently we have:
  - small pages (64kb), 64 in one segment
  - medium pages (512kb), 8 in one segment
  - large pages (4mb), 1 in one segment
  - huge blocks > MI_LARGE_OBJ_SIZE_MAX become large segment with 1 page

  In any case the memory for a segment is virtual and usually committed on demand.
  (i.e. we are careful to not touch the memory until we actually allocate a block there)

  If a  thread ends, it "abandons" pages with used blocks
  and there is an abandoned segment list whose segments can
  be reclaimed by still running threads, much like work-stealing.
-------------------------------------------------------------------------------- */


/* -----------------------------------------------------------
  Queue of segments containing free pages
----------------------------------------------------------- */

#if (MI_DEBUG>=3)
static bool mi_segment_queue_contains(const mi_segment_queue_t* queue, const mi_segment_t* segment) {
  mi_assert_internal(segment != NULL);
  mi_segment_t* list = queue->first;
  while (list != NULL) {
    if (list == segment) break;
    mi_assert_internal(list->next==NULL || list->next->prev == list);
    mi_assert_internal(list->prev==NULL || list->prev->next == list);
    list = list->next;
  }
  return (list == segment);
}
#endif

static bool mi_segment_queue_is_empty(const mi_segment_queue_t* queue) {
  return (queue->first == NULL);
}

static void mi_segment_queue_remove(mi_segment_queue_t* queue, mi_segment_t* segment) {
  mi_assert_expensive(mi_segment_queue_contains(queue, segment));
  if (segment->prev != NULL) segment->prev->next = segment->next;
  if (segment->next != NULL) segment->next->prev = segment->prev;
  if (segment == queue->first) queue->first = segment->next;
  if (segment == queue->last)  queue->last = segment->prev;
  segment->next = NULL;
  segment->prev = NULL;
}

static void mi_segment_enqueue(mi_segment_queue_t* queue, mi_segment_t* segment) {
  mi_assert_expensive(!mi_segment_queue_contains(queue, segment));
  segment->next = NULL;
  segment->prev = queue->last;
  if (queue->last != NULL) {
    mi_assert_internal(queue->last->next == NULL);
    queue->last->next = segment;
    queue->last = segment;
  }
  else {
    queue->last = queue->first = segment;
  }
}

static mi_segment_queue_t* mi_segment_free_queue_of_kind(mi_page_kind_t kind, mi_segments_tld_t* tld) {
  if (kind == MI_PAGE_SMALL) return &tld->small_free;
  else if (kind == MI_PAGE_MEDIUM) return &tld->medium_free;
  else return NULL;
}

static mi_segment_queue_t* mi_segment_free_queue(const mi_segment_t* segment, mi_segments_tld_t* tld) {
  return mi_segment_free_queue_of_kind(segment->page_kind, tld);
}

// remove from free queue if it is in one
static void mi_segment_remove_from_free_queue(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_segment_queue_t* queue = mi_segment_free_queue(segment, tld); // may be NULL
  bool in_queue = (queue!=NULL && (segment->next != NULL || segment->prev != NULL || queue->first == segment));
  if (in_queue) {
    mi_segment_queue_remove(queue, segment);
  }
}

static void mi_segment_insert_in_free_queue(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_segment_enqueue(mi_segment_free_queue(segment, tld), segment);
}


/* -----------------------------------------------------------
 Invariant checking
----------------------------------------------------------- */

#if (MI_DEBUG>=2)
static bool mi_segment_is_in_free_queue(const mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_segment_queue_t* queue = mi_segment_free_queue(segment, tld);
  bool in_queue = (queue!=NULL && (segment->next != NULL || segment->prev != NULL || queue->first == segment));
  if (in_queue) {
    mi_assert_expensive(mi_segment_queue_contains(queue, segment));
  }
  return in_queue;
}
#endif

static size_t mi_segment_page_size(const mi_segment_t* segment) {
  if (segment->capacity > 1) {
    mi_assert_internal(segment->page_kind <= MI_PAGE_MEDIUM);
    return ((size_t)1 << segment->page_shift);
  }
  else {
    mi_assert_internal(segment->page_kind >= MI_PAGE_LARGE);
    return segment->segment_size;
  }
}


#if (MI_DEBUG>=2)
static bool mi_pages_reset_contains(const mi_page_t* page, mi_segments_tld_t* tld) {
  mi_page_t* p = tld->pages_reset.first;
  while (p != NULL) {
    if (p == page) return true;
    p = p->next;
  }
  return false;
}
#endif

#if (MI_DEBUG>=3)
static bool mi_segment_is_valid(const mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(segment != NULL);
  mi_assert_internal(_mi_ptr_cookie(segment) == segment->cookie);
  mi_assert_internal(segment->used <= segment->capacity);
  mi_assert_internal(segment->abandoned <= segment->used);
  size_t nfree = 0;
  for (size_t i = 0; i < segment->capacity; i++) {
    const mi_page_t* const page = &segment->pages[i];
    if (!page->segment_in_use) {
      nfree++;
    }
    if (page->segment_in_use || page->is_reset) {
      mi_assert_expensive(!mi_pages_reset_contains(page, tld));
    }
  }
  mi_assert_internal(nfree + segment->used == segment->capacity);
  // mi_assert_internal(segment->thread_id == _mi_thread_id() || (segment->thread_id==0)); // or 0
  mi_assert_internal(segment->page_kind == MI_PAGE_HUGE ||
                     (mi_segment_page_size(segment) * segment->capacity == segment->segment_size));
  return true;
}
#endif

static bool mi_page_not_in_queue(const mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert_internal(page != NULL);
  if (page->next != NULL || page->prev != NULL) {
    mi_assert_internal(mi_pages_reset_contains(page, tld));
    return false;
  }
  else {
    // both next and prev are NULL, check for singleton list
    return (tld->pages_reset.first != page && tld->pages_reset.last != page);
  }
}


/* -----------------------------------------------------------
  Guard pages
----------------------------------------------------------- */

static void mi_segment_protect_range(void* p, size_t size, bool protect) {
  if (protect) {
    _mi_mem_protect(p, size);
  }
  else {
    _mi_mem_unprotect(p, size);
  }
}

static void mi_segment_protect(mi_segment_t* segment, bool protect, mi_os_tld_t* tld) {
  // add/remove guard pages
  if (MI_SECURE != 0) {
    // in secure mode, we set up a protected page in between the segment info and the page data
    const size_t os_psize = _mi_os_page_size();
    mi_assert_internal((segment->segment_info_size - os_psize) >= (sizeof(mi_segment_t) + ((segment->capacity - 1) * sizeof(mi_page_t))));
    mi_assert_internal(((uintptr_t)segment + segment->segment_info_size) % os_psize == 0);
    mi_segment_protect_range((uint8_t*)segment + segment->segment_info_size - os_psize, os_psize, protect);
    if (MI_SECURE <= 1 || segment->capacity == 1) {
      // and protect the last (or only) page too
      mi_assert_internal(MI_SECURE <= 1 || segment->page_kind >= MI_PAGE_LARGE);
      uint8_t* start = (uint8_t*)segment + segment->segment_size - os_psize;
      if (protect && !segment->mem_is_committed) {
        if (protect) {
          // ensure secure page is committed
          if (_mi_mem_commit(start, os_psize, NULL, tld)) {  // if this fails that is ok (as it is an unaccessible page)
            mi_segment_protect_range(start, os_psize, protect);
          }
        }
      }
      else {
        mi_segment_protect_range(start, os_psize, protect);
      }
    }
    else {
      // or protect every page
      const size_t page_size = mi_segment_page_size(segment);
      for (size_t i = 0; i < segment->capacity; i++) {
        if (segment->pages[i].is_committed) {
          mi_segment_protect_range((uint8_t*)segment + (i+1)*page_size - os_psize, os_psize, protect);
        }
      }
    }
  }
}

/* -----------------------------------------------------------
  Page reset
----------------------------------------------------------- */

static void mi_page_reset(mi_segment_t* segment, mi_page_t* page, size_t size, mi_segments_tld_t* tld) {
  mi_assert_internal(page->is_committed);
  if (!mi_option_is_enabled(mi_option_page_reset)) return;
  if (segment->mem_is_pinned || page->segment_in_use || !page->is_committed || page->is_reset) return;
  size_t psize;
  void* start = mi_segment_raw_page_start(segment, page, &psize);
  page->is_reset = true;
  mi_assert_internal(size <= psize);
  size_t reset_size = ((size == 0 || size > psize) ? psize : size);
  if (reset_size > 0) _mi_mem_reset(start, reset_size, tld->os);
}

static bool mi_page_unreset(mi_segment_t* segment, mi_page_t* page, size_t size, mi_segments_tld_t* tld)
{
  mi_assert_internal(page->is_reset);
  mi_assert_internal(page->is_committed);
  mi_assert_internal(!segment->mem_is_pinned);
  if (segment->mem_is_pinned || !page->is_committed || !page->is_reset) return true;
  page->is_reset = false;
  size_t psize;
  uint8_t* start = mi_segment_raw_page_start(segment, page, &psize);
  size_t unreset_size = (size == 0 || size > psize ? psize : size);
  bool is_zero = false;
  bool ok = true;
  if (unreset_size > 0) {
    ok = _mi_mem_unreset(start, unreset_size, &is_zero, tld->os);
  }
  if (is_zero) page->is_zero_init = true;
  return ok;
}


/* -----------------------------------------------------------
  The free page queue
----------------------------------------------------------- */

// we re-use the `used` field for the expiration counter. Since this is a
// a 32-bit field while the clock is always 64-bit we need to guard
// against overflow, we use substraction to check for expiry which work
// as long as the reset delay is under (2^30 - 1) milliseconds (~12 days)
static void mi_page_reset_set_expire(mi_page_t* page) {
  uint32_t expire = (uint32_t)_mi_clock_now() + mi_option_get(mi_option_reset_delay);
  page->used = expire;
}

static bool mi_page_reset_is_expired(mi_page_t* page, mi_msecs_t now) {
  int32_t expire = (int32_t)(page->used);
  return (((int32_t)now - expire) >= 0);
}

static void mi_pages_reset_add(mi_segment_t* segment, mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert_internal(!page->segment_in_use || !page->is_committed);
  mi_assert_internal(mi_page_not_in_queue(page,tld));
  mi_assert_expensive(!mi_pages_reset_contains(page, tld));
  mi_assert_internal(_mi_page_segment(page)==segment);
  if (!mi_option_is_enabled(mi_option_page_reset)) return;
  if (segment->mem_is_pinned || page->segment_in_use || !page->is_committed || page->is_reset) return;

  if (mi_option_get(mi_option_reset_delay) == 0) {
    // reset immediately?
    mi_page_reset(segment, page, 0, tld);
  }
  else {
    // otherwise push on the delayed page reset queue
    mi_page_queue_t* pq = &tld->pages_reset;
    // push on top
    mi_page_reset_set_expire(page);
    page->next = pq->first;
    page->prev = NULL;
    if (pq->first == NULL) {
      mi_assert_internal(pq->last == NULL);
      pq->first = pq->last = page;
    }
    else {
      pq->first->prev = page;
      pq->first = page;
    }
  }
}

static void mi_pages_reset_remove(mi_page_t* page, mi_segments_tld_t* tld) {
  if (mi_page_not_in_queue(page,tld)) return;

  mi_page_queue_t* pq = &tld->pages_reset;
  mi_assert_internal(pq!=NULL);
  mi_assert_internal(!page->segment_in_use);
  mi_assert_internal(mi_pages_reset_contains(page, tld));
  if (page->prev != NULL) page->prev->next = page->next;
  if (page->next != NULL) page->next->prev = page->prev;
  if (page == pq->last)  pq->last = page->prev;
  if (page == pq->first) pq->first = page->next;
  page->next = page->prev = NULL;
  page->used = 0;
}

static void mi_pages_reset_remove_all_in_segment(mi_segment_t* segment, bool force_reset, mi_segments_tld_t* tld) {
  if (segment->mem_is_pinned) return; // never reset in huge OS pages
  for (size_t i = 0; i < segment->capacity; i++) {
    mi_page_t* page = &segment->pages[i];
    if (!page->segment_in_use && page->is_committed && !page->is_reset) {
      mi_pages_reset_remove(page, tld);
      if (force_reset) {
        mi_page_reset(segment, page, 0, tld);
      }
    }
    else {
      mi_assert_internal(mi_page_not_in_queue(page,tld));
    }
  }
}

static void mi_reset_delayed(mi_segments_tld_t* tld) {
  if (!mi_option_is_enabled(mi_option_page_reset)) return;
  mi_msecs_t now = _mi_clock_now();
  mi_page_queue_t* pq = &tld->pages_reset;
  // from oldest up to the first that has not expired yet
  mi_page_t* page = pq->last;
  while (page != NULL && mi_page_reset_is_expired(page,now)) {
    mi_page_t* const prev = page->prev; // save previous field
    mi_page_reset(_mi_page_segment(page), page, 0, tld);
    page->used = 0;
    page->prev = page->next = NULL;
    page = prev;
  }
  // discard the reset pages from the queue
  pq->last = page;
  if (page != NULL){
    page->next = NULL;
  }
  else {
    pq->first = NULL;
  }
}


/* -----------------------------------------------------------
 Segment size calculations
----------------------------------------------------------- */

// Raw start of the page available memory; can be used on uninitialized pages (only `segment_idx` must be set)
// The raw start is not taking aligned block allocation into consideration.
static uint8_t* mi_segment_raw_page_start(const mi_segment_t* segment, const mi_page_t* page, size_t* page_size) {
  size_t   psize = (segment->page_kind == MI_PAGE_HUGE ? segment->segment_size : (size_t)1 << segment->page_shift);
  uint8_t* p = (uint8_t*)segment + page->segment_idx * psize;

  if (page->segment_idx == 0) {
    // the first page starts after the segment info (and possible guard page)
    p += segment->segment_info_size;
    psize -= segment->segment_info_size;
  }

#if (MI_SECURE > 1)  // every page has an os guard page
  psize -= _mi_os_page_size();
#elif (MI_SECURE==1) // the last page has an os guard page at the end
  if (page->segment_idx == segment->capacity - 1) {
    psize -= _mi_os_page_size();
  }
#endif

  if (page_size != NULL) *page_size = psize;
  mi_assert_internal(page->xblock_size == 0 || _mi_ptr_page(p) == page);
  mi_assert_internal(_mi_ptr_segment(p) == segment);
  return p;
}

// Start of the page available memory; can be used on uninitialized pages (only `segment_idx` must be set)
uint8_t* _mi_segment_page_start(const mi_segment_t* segment, const mi_page_t* page, size_t block_size, size_t* page_size, size_t* pre_size)
{
  size_t   psize;
  uint8_t* p = mi_segment_raw_page_start(segment, page, &psize);
  if (pre_size != NULL) *pre_size = 0;
  if (page->segment_idx == 0 && block_size > 0 && segment->page_kind <= MI_PAGE_MEDIUM) {
    // for small and medium objects, ensure the page start is aligned with the block size (PR#66 by kickunderscore)
    size_t adjust = block_size - ((uintptr_t)p % block_size);
    if (adjust < block_size) {
      p += adjust;
      psize -= adjust;
      if (pre_size != NULL) *pre_size = adjust;
    }
    mi_assert_internal((uintptr_t)p % block_size == 0);
  }

  if (page_size != NULL) *page_size = psize;
  mi_assert_internal(page->xblock_size==0 || _mi_ptr_page(p) == page);
  mi_assert_internal(_mi_ptr_segment(p) == segment);
  return p;
}

static size_t mi_segment_size(size_t capacity, size_t required, size_t* pre_size, size_t* info_size)
{
  const size_t minsize   = sizeof(mi_segment_t) + ((capacity - 1) * sizeof(mi_page_t)) + 16 /* padding */;
  size_t guardsize = 0;
  size_t isize     = 0;

  if (MI_SECURE == 0) {
    // normally no guard pages
    isize = _mi_align_up(minsize, 16 * MI_MAX_ALIGN_SIZE);
  }
  else {
    // in secure mode, we set up a protected page in between the segment info
    // and the page data (and one at the end of the segment)
    const size_t page_size = _mi_os_page_size();
    isize = _mi_align_up(minsize, page_size);
    guardsize = page_size;
    required = _mi_align_up(required, page_size);
  }

  if (info_size != NULL) *info_size = isize;
  if (pre_size != NULL)  *pre_size  = isize + guardsize;
  return (required==0 ? MI_SEGMENT_SIZE : _mi_align_up( required + isize + 2*guardsize, MI_PAGE_HUGE_ALIGN) );
}


/* ----------------------------------------------------------------------------
Segment caches
We keep a small segment cache per thread to increase local
reuse and avoid setting/clearing guard pages in secure mode.
------------------------------------------------------------------------------- */

static void mi_segments_track_size(long segment_size, mi_segments_tld_t* tld) {
  if (segment_size>=0) _mi_stat_increase(&tld->stats->segments,1);
                  else _mi_stat_decrease(&tld->stats->segments,1);
  tld->count += (segment_size >= 0 ? 1 : -1);
  if (tld->count > tld->peak_count) tld->peak_count = tld->count;
  tld->current_size += segment_size;
  if (tld->current_size > tld->peak_size) tld->peak_size = tld->current_size;
}

static void mi_segment_os_free(mi_segment_t* segment, size_t segment_size, mi_segments_tld_t* tld) {
  segment->thread_id = 0;
  mi_segments_track_size(-((long)segment_size),tld);
  if (MI_SECURE != 0) {
    mi_assert_internal(!segment->mem_is_pinned);
    mi_segment_protect(segment, false, tld->os); // ensure no more guard pages are set
  }

  bool any_reset = false;
  bool fully_committed = true;
  for (size_t i = 0; i < segment->capacity; i++) {
    mi_page_t* page = &segment->pages[i];
    if (!page->is_committed) { fully_committed = false; }
    if (page->is_reset)      { any_reset = true; }
  }
  if (any_reset && mi_option_is_enabled(mi_option_reset_decommits)) {
    fully_committed = false;
  }
  _mi_mem_free(segment, segment_size, segment->memid, fully_committed, any_reset, tld->os);
}


// The thread local segment cache is limited to be at most 1/8 of the peak size of segments in use,
#define MI_SEGMENT_CACHE_FRACTION (8)

// note: returned segment may be partially reset
static mi_segment_t* mi_segment_cache_pop(size_t segment_size, mi_segments_tld_t* tld) {
  if (segment_size != 0 && segment_size != MI_SEGMENT_SIZE) return NULL;
  mi_segment_t* segment = tld->cache;
  if (segment == NULL) return NULL;
  tld->cache_count--;
  tld->cache = segment->next;
  segment->next = NULL;
  mi_assert_internal(segment->segment_size == MI_SEGMENT_SIZE);
  _mi_stat_decrease(&tld->stats->segments_cache, 1);
  return segment;
}

static bool mi_segment_cache_full(mi_segments_tld_t* tld)
{
  // if (tld->count == 1 && tld->cache_count==0) return false; // always cache at least the final segment of a thread
  size_t max_cache = mi_option_get(mi_option_segment_cache);
  if (tld->cache_count < max_cache
       && tld->cache_count < (1 + (tld->peak_count / MI_SEGMENT_CACHE_FRACTION)) // at least allow a 1 element cache
     ) {
    return false;
  }
  // take the opportunity to reduce the segment cache if it is too large (now)
  // TODO: this never happens as we check against peak usage, should we use current usage instead?
  while (tld->cache_count > max_cache) { //(1 + (tld->peak_count / MI_SEGMENT_CACHE_FRACTION))) {
    mi_segment_t* segment = mi_segment_cache_pop(0,tld);
    mi_assert_internal(segment != NULL);
    if (segment != NULL) mi_segment_os_free(segment, segment->segment_size, tld);
  }
  return true;
}

static bool mi_segment_cache_push(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(!mi_segment_is_in_free_queue(segment, tld));
  mi_assert_internal(segment->next == NULL);
  if (segment->segment_size != MI_SEGMENT_SIZE || mi_segment_cache_full(tld)) {
    return false;
  }
  mi_assert_internal(segment->segment_size == MI_SEGMENT_SIZE);
  segment->next = tld->cache;
  tld->cache = segment;
  tld->cache_count++;
  _mi_stat_increase(&tld->stats->segments_cache,1);
  return true;
}

// called by threads that are terminating to free cached segments
void _mi_segment_thread_collect(mi_segments_tld_t* tld) {
  mi_segment_t* segment;
  while ((segment = mi_segment_cache_pop(0,tld)) != NULL) {
    mi_segment_os_free(segment, segment->segment_size, tld);
  }
  mi_assert_internal(tld->cache_count == 0);
  mi_assert_internal(tld->cache == NULL);
#if MI_DEBUG>=2
  if (!_mi_is_main_thread()) {
    mi_assert_internal(tld->pages_reset.first == NULL);
    mi_assert_internal(tld->pages_reset.last == NULL);
  }
#endif
}


/* -----------------------------------------------------------
   Segment allocation
----------------------------------------------------------- */

// Allocate a segment from the OS aligned to `MI_SEGMENT_SIZE` .
static mi_segment_t* mi_segment_init(mi_segment_t* segment, size_t required, mi_page_kind_t page_kind, size_t page_shift, mi_segments_tld_t* tld, mi_os_tld_t* os_tld)
{
  // the segment parameter is non-null if it came from our cache
  mi_assert_internal(segment==NULL || (required==0 && page_kind <= MI_PAGE_LARGE));

  // calculate needed sizes first
  size_t capacity;
  if (page_kind == MI_PAGE_HUGE) {
    mi_assert_internal(page_shift == MI_SEGMENT_SHIFT && required > 0);
    capacity = 1;
  }
  else {
    mi_assert_internal(required == 0);
    size_t page_size = (size_t)1 << page_shift;
    capacity = MI_SEGMENT_SIZE / page_size;
    mi_assert_internal(MI_SEGMENT_SIZE % page_size == 0);
    mi_assert_internal(capacity >= 1 && capacity <= MI_SMALL_PAGES_PER_SEGMENT);
  }
  size_t info_size;
  size_t pre_size;
  size_t segment_size = mi_segment_size(capacity, required, &pre_size, &info_size);
  mi_assert_internal(segment_size >= required);

  // Initialize parameters
  const bool eager_delayed = (page_kind <= MI_PAGE_MEDIUM && tld->count < (size_t)mi_option_get(mi_option_eager_commit_delay));
  const bool eager  = !eager_delayed && mi_option_is_enabled(mi_option_eager_commit);
  bool commit = eager; // || (page_kind >= MI_PAGE_LARGE);
  bool pages_still_good = false;
  bool is_zero = false;

  // Try to get it from our thread local cache first
  if (segment != NULL) {
    // came from cache
    mi_assert_internal(segment->segment_size == segment_size);
    if (page_kind <= MI_PAGE_MEDIUM && segment->page_kind == page_kind && segment->segment_size == segment_size) {
      pages_still_good = true;
    }
    else
    {
      if (MI_SECURE!=0) {
        mi_assert_internal(!segment->mem_is_pinned);
        mi_segment_protect(segment, false, tld->os); // reset protection if the page kind differs
      }
      // different page kinds; unreset any reset pages, and unprotect
      // TODO: optimize cache pop to return fitting pages if possible?
      for (size_t i = 0; i < segment->capacity; i++) {
        mi_page_t* page = &segment->pages[i];
        if (page->is_reset) {
          if (!commit && mi_option_is_enabled(mi_option_reset_decommits)) {
            page->is_reset = false;
          }
          else {
            mi_page_unreset(segment, page, 0, tld);  // todo: only unreset the part that was reset? (instead of the full page)
          }
        }
      }
      // ensure the initial info is committed
      if (segment->capacity < capacity) {
        bool commit_zero = false;
        bool ok = _mi_mem_commit(segment, pre_size, &commit_zero, tld->os);
        if (commit_zero) is_zero = true;
        if (!ok) {
          return NULL;
        }
      }
    }
  }
  else {
    // Allocate the segment from the OS
    size_t memid;
    bool   mem_large = (!eager_delayed && (MI_SECURE==0)); // only allow large OS pages once we are no longer lazy
    bool   is_pinned = false;
    segment = (mi_segment_t*)_mi_mem_alloc_aligned(segment_size, MI_SEGMENT_SIZE, &commit, &mem_large, &is_pinned, &is_zero, &memid, os_tld);
    if (segment == NULL) return NULL;  // failed to allocate
    if (!commit) {
      // ensure the initial info is committed
      mi_assert_internal(!mem_large && !is_pinned);
      bool commit_zero = false;
      bool ok = _mi_mem_commit(segment, pre_size, &commit_zero, tld->os);
      if (commit_zero) is_zero = true;
      if (!ok) {
        // commit failed; we cannot touch the memory: free the segment directly and return `NULL`
        _mi_mem_free(segment, MI_SEGMENT_SIZE, memid, false, false, os_tld);
        return NULL;  
      }
    }
    segment->memid = memid;
    segment->mem_is_pinned = (mem_large || is_pinned);
    segment->mem_is_committed = commit;    
    mi_segments_track_size((long)segment_size, tld);
  }
  mi_assert_internal(segment != NULL && (uintptr_t)segment % MI_SEGMENT_SIZE == 0);
  mi_assert_internal(segment->mem_is_pinned ? segment->mem_is_committed : true);  
  mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);  // tsan
  if (!pages_still_good) {
    // zero the segment info (but not the `mem` fields)
    ptrdiff_t ofs = offsetof(mi_segment_t, next);
    memset((uint8_t*)segment + ofs, 0, info_size - ofs);

    // initialize pages info
    for (uint8_t i = 0; i < capacity; i++) {
      segment->pages[i].segment_idx = i;
      segment->pages[i].is_reset = false;
      segment->pages[i].is_committed = commit;
      segment->pages[i].is_zero_init = is_zero;
    }
  }
  else {
    // zero the segment info but not the pages info (and mem fields)
    ptrdiff_t ofs = offsetof(mi_segment_t, next);
    memset((uint8_t*)segment + ofs, 0, offsetof(mi_segment_t,pages) - ofs);
  }

  // initialize
  segment->page_kind  = page_kind;
  segment->capacity   = capacity;
  segment->page_shift = page_shift;
  segment->segment_size = segment_size;
  segment->segment_info_size = pre_size;
  segment->thread_id  = _mi_thread_id();
  segment->cookie = _mi_ptr_cookie(segment);
  // _mi_stat_increase(&tld->stats->page_committed, segment->segment_info_size);

  // set protection
  mi_segment_protect(segment, true, tld->os);

  // insert in free lists for small and medium pages
  if (page_kind <= MI_PAGE_MEDIUM) {
    mi_segment_insert_in_free_queue(segment, tld);
  }

  //fprintf(stderr,"mimalloc: alloc segment at %p\n", (void*)segment);
  return segment;
}

static mi_segment_t* mi_segment_alloc(size_t required, mi_page_kind_t page_kind, size_t page_shift, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  return mi_segment_init(NULL, required, page_kind, page_shift, tld, os_tld);
}

static void mi_segment_free(mi_segment_t* segment, bool force, mi_segments_tld_t* tld) {
  UNUSED(force);
  mi_assert(segment != NULL);
  // note: don't reset pages even on abandon as the whole segment is freed? (and ready for reuse)
  bool force_reset = (force && mi_option_is_enabled(mi_option_abandoned_page_reset));
  mi_pages_reset_remove_all_in_segment(segment, force_reset, tld);
  mi_segment_remove_from_free_queue(segment,tld);

  mi_assert_expensive(!mi_segment_queue_contains(&tld->small_free, segment));
  mi_assert_expensive(!mi_segment_queue_contains(&tld->medium_free, segment));
  mi_assert(segment->next == NULL);
  mi_assert(segment->prev == NULL);
  _mi_stat_decrease(&tld->stats->page_committed, segment->segment_info_size);

  if (!force && mi_segment_cache_push(segment, tld)) {
    // it is put in our cache
  }
  else {
    // otherwise return it to the OS
    mi_segment_os_free(segment, segment->segment_size, tld);
  }
}

/* -----------------------------------------------------------
  Free page management inside a segment
----------------------------------------------------------- */


static bool mi_segment_has_free(const mi_segment_t* segment) {
  return (segment->used < segment->capacity);
}

static bool mi_segment_page_claim(mi_segment_t* segment, mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert_internal(_mi_page_segment(page) == segment);
  mi_assert_internal(!page->segment_in_use);
  mi_pages_reset_remove(page, tld);
  // check commit
  if (!page->is_committed) {
    mi_assert_internal(!segment->mem_is_pinned);
    mi_assert_internal(!page->is_reset);    
    size_t psize;
    uint8_t* start = mi_segment_raw_page_start(segment, page, &psize);
    bool is_zero = false;
    const size_t gsize = (MI_SECURE >= 2 ? _mi_os_page_size() : 0);
    bool ok = _mi_mem_commit(start, psize + gsize, &is_zero, tld->os);
    if (!ok) return false; // failed to commit!
    if (gsize > 0) { mi_segment_protect_range(start + psize, gsize, true); }
    if (is_zero) { page->is_zero_init = true; }
    page->is_committed = true;
  }
  // set in-use before doing unreset to prevent delayed reset
  page->segment_in_use = true;
  segment->used++;
  // check reset
  if (page->is_reset) {
    mi_assert_internal(!segment->mem_is_pinned);
    bool ok = mi_page_unreset(segment, page, 0, tld); 
    if (!ok) {
      page->segment_in_use = false;
      segment->used--;
      return false;
    }
  }
  mi_assert_internal(page->segment_in_use);
  mi_assert_internal(segment->used <= segment->capacity);
  if (segment->used == segment->capacity && segment->page_kind <= MI_PAGE_MEDIUM) {
    // if no more free pages, remove from the queue
    mi_assert_internal(!mi_segment_has_free(segment));
    mi_segment_remove_from_free_queue(segment, tld);
  }
  return true;
}


/* -----------------------------------------------------------
   Free
----------------------------------------------------------- */

static void mi_segment_abandon(mi_segment_t* segment, mi_segments_tld_t* tld);

// clear page data; can be called on abandoned segments
static void mi_segment_page_clear(mi_segment_t* segment, mi_page_t* page, bool allow_reset, mi_segments_tld_t* tld)
{
  mi_assert_internal(page->segment_in_use);
  mi_assert_internal(mi_page_all_free(page));
  mi_assert_internal(page->is_committed);
  mi_assert_internal(mi_page_not_in_queue(page, tld));

  size_t inuse = page->capacity * mi_page_block_size(page);
  _mi_stat_decrease(&tld->stats->page_committed, inuse);
  _mi_stat_decrease(&tld->stats->pages, 1);

  // calculate the used size from the raw (non-aligned) start of the page
  //size_t pre_size;
  //_mi_segment_page_start(segment, page, page->block_size, NULL, &pre_size);
  //size_t used_size = pre_size + (page->capacity * page->block_size);

  page->is_zero_init = false;
  page->segment_in_use = false;

  // reset the page memory to reduce memory pressure?
  // note: must come after setting `segment_in_use` to false but before block_size becomes 0
  //mi_page_reset(segment, page, 0 /*used_size*/, tld);

  // zero the page data, but not the segment fields and capacity, and block_size (for page size calculations)
  uint32_t block_size = page->xblock_size;
  uint16_t capacity = page->capacity;
  uint16_t reserved = page->reserved;
  ptrdiff_t ofs = offsetof(mi_page_t,capacity);
  memset((uint8_t*)page + ofs, 0, sizeof(*page) - ofs);
  page->capacity = capacity;
  page->reserved = reserved;
  page->xblock_size = block_size;
  segment->used--;

  // add to the free page list for reuse/reset
  if (allow_reset) {
    mi_pages_reset_add(segment, page, tld);
  }

  page->capacity = 0;  // after reset these can be zero'd now
  page->reserved = 0;
}

void _mi_segment_page_free(mi_page_t* page, bool force, mi_segments_tld_t* tld)
{
  mi_assert(page != NULL);
  mi_segment_t* segment = _mi_page_segment(page);
  mi_assert_expensive(mi_segment_is_valid(segment,tld));
  mi_reset_delayed(tld);

  // mark it as free now
  mi_segment_page_clear(segment, page, true, tld);

  if (segment->used == 0) {
    // no more used pages; remove from the free list and free the segment
    mi_segment_free(segment, force, tld);
  }
  else {
    if (segment->used == segment->abandoned) {
      // only abandoned pages; remove from free list and abandon
      mi_segment_abandon(segment,tld);
    }
    else if (segment->used + 1 == segment->capacity) {
      mi_assert_internal(segment->page_kind <= MI_PAGE_MEDIUM); // for now we only support small and medium pages
      // move back to segments  free list
      mi_segment_insert_in_free_queue(segment,tld);
    }
  }
}


/* -----------------------------------------------------------
Abandonment

When threads terminate, they can leave segments with
live blocks (reached through other threads). Such segments
are "abandoned" and will be reclaimed by other threads to
reuse their pages and/or free them eventually

We maintain a global list of abandoned segments that are
reclaimed on demand. Since this is shared among threads
the implementation needs to avoid the A-B-A problem on
popping abandoned segments: <https://en.wikipedia.org/wiki/ABA_problem>
We use tagged pointers to avoid accidentially identifying
reused segments, much like stamped references in Java.
Secondly, we maintain a reader counter to avoid resetting
or decommitting segments that have a pending read operation.

Note: the current implementation is one possible design;
another way might be to keep track of abandoned segments
in the regions. This would have the advantage of keeping
all concurrent code in one place and not needing to deal
with ABA issues. The drawback is that it is unclear how to
scan abandoned segments efficiently in that case as they
would be spread among all other segments in the regions.
----------------------------------------------------------- */

// Use the bottom 20-bits (on 64-bit) of the aligned segment pointers
// to put in a tag that increments on update to avoid the A-B-A problem.
#define MI_TAGGED_MASK   MI_SEGMENT_MASK
typedef uintptr_t        mi_tagged_segment_t;

static mi_segment_t* mi_tagged_segment_ptr(mi_tagged_segment_t ts) {
  return (mi_segment_t*)(ts & ~MI_TAGGED_MASK);
}

static mi_tagged_segment_t mi_tagged_segment(mi_segment_t* segment, mi_tagged_segment_t ts) {
  mi_assert_internal(((uintptr_t)segment & MI_TAGGED_MASK) == 0);
  uintptr_t tag = ((ts & MI_TAGGED_MASK) + 1) & MI_TAGGED_MASK;
  return ((uintptr_t)segment | tag);
}

// This is a list of visited abandoned pages that were full at the time.
// this list migrates to `abandoned` when that becomes NULL. The use of
// this list reduces contention and the rate at which segments are visited.
static mi_decl_cache_align _Atomic(mi_segment_t*)       abandoned_visited; // = NULL

// The abandoned page list (tagged as it supports pop)
static mi_decl_cache_align _Atomic(mi_tagged_segment_t) abandoned;         // = NULL

// Maintain these for debug purposes (these counts may be a bit off)
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_count; 
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_visited_count;

// We also maintain a count of current readers of the abandoned list
// in order to prevent resetting/decommitting segment memory if it might
// still be read.
static mi_decl_cache_align _Atomic(uintptr_t)           abandoned_readers; // = 0

// Push on the visited list
static void mi_abandoned_visited_push(mi_segment_t* segment) {
  mi_assert_internal(segment->thread_id == 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t,&segment->abandoned_next) == NULL);
  mi_assert_internal(segment->next == NULL && segment->prev == NULL);
  mi_assert_internal(segment->used > 0);
  mi_segment_t* anext = mi_atomic_load_ptr_relaxed(mi_segment_t, &abandoned_visited);
  do {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, anext);
  } while (!mi_atomic_cas_ptr_weak_release(mi_segment_t, &abandoned_visited, &anext, segment));
  mi_atomic_increment_relaxed(&abandoned_visited_count);
}

// Move the visited list to the abandoned list.
static bool mi_abandoned_visited_revisit(void)
{
  // quick check if the visited list is empty
  if (mi_atomic_load_ptr_relaxed(mi_segment_t, &abandoned_visited) == NULL) return false;

  // grab the whole visited list
  mi_segment_t* first = mi_atomic_exchange_ptr_acq_rel(mi_segment_t, &abandoned_visited, NULL);
  if (first == NULL) return false;

  // first try to swap directly if the abandoned list happens to be NULL
  mi_tagged_segment_t afirst;
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  if (mi_tagged_segment_ptr(ts)==NULL) {
    uintptr_t count = mi_atomic_load_relaxed(&abandoned_visited_count);
    afirst = mi_tagged_segment(first, ts);
    if (mi_atomic_cas_strong_acq_rel(&abandoned, &ts, afirst)) {
      mi_atomic_add_relaxed(&abandoned_count, count);
      mi_atomic_sub_relaxed(&abandoned_visited_count, count);
      return true;
    }
  }

  // find the last element of the visited list: O(n)
  mi_segment_t* last = first;
  mi_segment_t* next;
  while ((next = mi_atomic_load_ptr_relaxed(mi_segment_t, &last->abandoned_next)) != NULL) {
    last = next;
  }

  // and atomically prepend to the abandoned list
  // (no need to increase the readers as we don't access the abandoned segments)
  mi_tagged_segment_t anext = mi_atomic_load_relaxed(&abandoned);
  uintptr_t count;
  do {
    count = mi_atomic_load_relaxed(&abandoned_visited_count);
    mi_atomic_store_ptr_release(mi_segment_t, &last->abandoned_next, mi_tagged_segment_ptr(anext));
    afirst = mi_tagged_segment(first, anext);
  } while (!mi_atomic_cas_weak_release(&abandoned, &anext, afirst));
  mi_atomic_add_relaxed(&abandoned_count, count);
  mi_atomic_sub_relaxed(&abandoned_visited_count, count);
  return true;
}

// Push on the abandoned list.
static void mi_abandoned_push(mi_segment_t* segment) {
  mi_assert_internal(segment->thread_id == 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  mi_assert_internal(segment->next == NULL && segment->prev == NULL);
  mi_assert_internal(segment->used > 0);
  mi_tagged_segment_t next;
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  do {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, mi_tagged_segment_ptr(ts));
    next = mi_tagged_segment(segment, ts);
  } while (!mi_atomic_cas_weak_release(&abandoned, &ts, next));
  mi_atomic_increment_relaxed(&abandoned_count);
}

// Wait until there are no more pending reads on segments that used to be in the abandoned list
void _mi_abandoned_await_readers(void) {
  uintptr_t n;
  do {
    n = mi_atomic_load_acquire(&abandoned_readers);
    if (n != 0) mi_atomic_yield();
  } while (n != 0);
}

// Pop from the abandoned list
static mi_segment_t* mi_abandoned_pop(void) {
  mi_segment_t* segment;
  // Check efficiently if it is empty (or if the visited list needs to be moved)
  mi_tagged_segment_t ts = mi_atomic_load_relaxed(&abandoned);
  segment = mi_tagged_segment_ptr(ts);
  if (mi_likely(segment == NULL)) {
    if (mi_likely(!mi_abandoned_visited_revisit())) { // try to swap in the visited list on NULL
      return NULL;
    }
  }

  // Do a pop. We use a reader count to prevent
  // a segment to be decommitted while a read is still pending,
  // and a tagged pointer to prevent A-B-A link corruption.
  // (this is called from `region.c:_mi_mem_free` for example)
  mi_atomic_increment_relaxed(&abandoned_readers);  // ensure no segment gets decommitted
  mi_tagged_segment_t next = 0;
  ts = mi_atomic_load_acquire(&abandoned);
  do {
    segment = mi_tagged_segment_ptr(ts);
    if (segment != NULL) {
      mi_segment_t* anext = mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next);
      next = mi_tagged_segment(anext, ts); // note: reads the segment's `abandoned_next` field so should not be decommitted
    }
  } while (segment != NULL && !mi_atomic_cas_weak_acq_rel(&abandoned, &ts, next));
  mi_atomic_decrement_relaxed(&abandoned_readers);  // release reader lock
  if (segment != NULL) {
    mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);
    mi_atomic_decrement_relaxed(&abandoned_count);
  }
  return segment;
}

/* -----------------------------------------------------------
   Abandon segment/page
----------------------------------------------------------- */

static void mi_segment_abandon(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(segment->used == segment->abandoned);
  mi_assert_internal(segment->used > 0);
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  mi_assert_expensive(mi_segment_is_valid(segment, tld));

  // remove the segment from the free page queue if needed
  mi_reset_delayed(tld);
  mi_pages_reset_remove_all_in_segment(segment, mi_option_is_enabled(mi_option_abandoned_page_reset), tld);
  mi_segment_remove_from_free_queue(segment, tld);
  mi_assert_internal(segment->next == NULL && segment->prev == NULL);

  // all pages in the segment are abandoned; add it to the abandoned list
  _mi_stat_increase(&tld->stats->segments_abandoned, 1);
  mi_segments_track_size(-((long)segment->segment_size), tld);
  segment->thread_id = 0;
  segment->abandoned_visits = 0;
  mi_atomic_store_ptr_release(mi_segment_t, &segment->abandoned_next, NULL);
  mi_abandoned_push(segment);
}

void _mi_segment_page_abandon(mi_page_t* page, mi_segments_tld_t* tld) {
  mi_assert(page != NULL);
  mi_assert_internal(mi_page_thread_free_flag(page)==MI_NEVER_DELAYED_FREE);
  mi_assert_internal(mi_page_heap(page) == NULL);
  mi_segment_t* segment = _mi_page_segment(page);
  mi_assert_expensive(!mi_pages_reset_contains(page, tld));
  mi_assert_expensive(mi_segment_is_valid(segment, tld));
  segment->abandoned++;
  _mi_stat_increase(&tld->stats->pages_abandoned, 1);
  mi_assert_internal(segment->abandoned <= segment->used);
  if (segment->used == segment->abandoned) {
    // all pages are abandoned, abandon the entire segment
    mi_segment_abandon(segment, tld);
  }
}

/* -----------------------------------------------------------
  Reclaim abandoned pages
----------------------------------------------------------- */

// Possibly clear pages and check if free space is available
static bool mi_segment_check_free(mi_segment_t* segment, size_t block_size, bool* all_pages_free)
{
  mi_assert_internal(block_size < MI_HUGE_BLOCK_SIZE);
  bool has_page = false;
  size_t pages_used = 0;
  size_t pages_used_empty = 0;
  for (size_t i = 0; i < segment->capacity; i++) {
    mi_page_t* page = &segment->pages[i];
    if (page->segment_in_use) {
      pages_used++;
      // ensure used count is up to date and collect potential concurrent frees
      _mi_page_free_collect(page, false);
      if (mi_page_all_free(page)) {
        // if everything free already, page can be reused for some block size
        // note: don't clear the page yet as we can only OS reset it once it is reclaimed
        pages_used_empty++;
        has_page = true;
      }
      else if (page->xblock_size == block_size && mi_page_has_any_available(page)) {
        // a page has available free blocks of the right size
        has_page = true;
      }
    }
    else {
      // whole empty page
      has_page = true;
    }
  }
  mi_assert_internal(pages_used == segment->used && pages_used >= pages_used_empty);
  if (all_pages_free != NULL) {
    *all_pages_free = ((pages_used - pages_used_empty) == 0);
  }
  return has_page;
}


// Reclaim a segment; returns NULL if the segment was freed
// set `right_page_reclaimed` to `true` if it reclaimed a page of the right `block_size` that was not full.
static mi_segment_t* mi_segment_reclaim(mi_segment_t* segment, mi_heap_t* heap, size_t requested_block_size, bool* right_page_reclaimed, mi_segments_tld_t* tld) {
  mi_assert_internal(mi_atomic_load_ptr_relaxed(mi_segment_t, &segment->abandoned_next) == NULL);
  if (right_page_reclaimed != NULL) { *right_page_reclaimed = false; }

  segment->thread_id = _mi_thread_id();
  segment->abandoned_visits = 0;
  mi_segments_track_size((long)segment->segment_size, tld);
  mi_assert_internal(segment->next == NULL && segment->prev == NULL);
  mi_assert_expensive(mi_segment_is_valid(segment, tld));
  _mi_stat_decrease(&tld->stats->segments_abandoned, 1);

  for (size_t i = 0; i < segment->capacity; i++) {
    mi_page_t* page = &segment->pages[i];
    if (page->segment_in_use) {
      mi_assert_internal(!page->is_reset);
      mi_assert_internal(page->is_committed);
      mi_assert_internal(mi_page_not_in_queue(page, tld));
      mi_assert_internal(mi_page_thread_free_flag(page)==MI_NEVER_DELAYED_FREE);
      mi_assert_internal(mi_page_heap(page) == NULL);
      segment->abandoned--;
      mi_assert(page->next == NULL);
      _mi_stat_decrease(&tld->stats->pages_abandoned, 1);
      // set the heap again and allow heap thread delayed free again.
      mi_page_set_heap(page, heap);
      _mi_page_use_delayed_free(page, MI_USE_DELAYED_FREE, true); // override never (after heap is set)
      // TODO: should we not collect again given that we just collected in `check_free`?
      _mi_page_free_collect(page, false); // ensure used count is up to date
      if (mi_page_all_free(page)) {
        // if everything free already, clear the page directly
        mi_segment_page_clear(segment, page, true, tld);  // reset is ok now
      }
      else {
        // otherwise reclaim it into the heap
        _mi_page_reclaim(heap, page);
        if (requested_block_size == page->xblock_size && mi_page_has_any_available(page)) {
          if (right_page_reclaimed != NULL) { *right_page_reclaimed = true; }
        }
      }
    }
    else if (page->is_committed && !page->is_reset) {  // not in-use, and not reset yet
      // note: do not reset as this includes pages that were not touched before
      // mi_pages_reset_add(segment, page, tld);
    }
  }
  mi_assert_internal(segment->abandoned == 0);
  if (segment->used == 0) {
    mi_assert_internal(right_page_reclaimed == NULL || !(*right_page_reclaimed));
    mi_segment_free(segment, false, tld);
    return NULL;
  }
  else {
    if (segment->page_kind <= MI_PAGE_MEDIUM && mi_segment_has_free(segment)) {
      mi_segment_insert_in_free_queue(segment, tld);
    }
    return segment;
  }
}


void _mi_abandoned_reclaim_all(mi_heap_t* heap, mi_segments_tld_t* tld) {
  mi_segment_t* segment;
  while ((segment = mi_abandoned_pop()) != NULL) {
    mi_segment_reclaim(segment, heap, 0, NULL, tld);
  }
}

static mi_segment_t* mi_segment_try_reclaim(mi_heap_t* heap, size_t block_size, mi_page_kind_t page_kind, bool* reclaimed, mi_segments_tld_t* tld)
{
  *reclaimed = false;
  mi_segment_t* segment;
  int max_tries = 8;     // limit the work to bound allocation times
  while ((max_tries-- > 0) && ((segment = mi_abandoned_pop()) != NULL)) {
    segment->abandoned_visits++;
    bool all_pages_free;
    bool has_page = mi_segment_check_free(segment,block_size,&all_pages_free); // try to free up pages (due to concurrent frees)
    if (all_pages_free) {
      // free the segment (by forced reclaim) to make it available to other threads.
      // note1: we prefer to free a segment as that might lead to reclaiming another
      // segment that is still partially used.
      // note2: we could in principle optimize this by skipping reclaim and directly
      // freeing but that would violate some invariants temporarily)
      mi_segment_reclaim(segment, heap, 0, NULL, tld);
    }
    else if (has_page && segment->page_kind == page_kind) {
      // found a free page of the right kind, or page of the right block_size with free space
      // we return the result of reclaim (which is usually `segment`) as it might free
      // the segment due to concurrent frees (in which case `NULL` is returned).
      return mi_segment_reclaim(segment, heap, block_size, reclaimed, tld);
    }
    else if (segment->abandoned_visits >= 3) {
      // always reclaim on 3rd visit to limit the list length.
      mi_segment_reclaim(segment, heap, 0, NULL, tld);
    }
    else {
      // otherwise, push on the visited list so it gets not looked at too quickly again
      mi_abandoned_visited_push(segment);
    }
  }
  return NULL;
}


/* -----------------------------------------------------------
   Reclaim or allocate
----------------------------------------------------------- */

static mi_segment_t* mi_segment_reclaim_or_alloc(mi_heap_t* heap, size_t block_size, mi_page_kind_t page_kind, size_t page_shift, mi_segments_tld_t* tld, mi_os_tld_t* os_tld)
{
  mi_assert_internal(page_kind <= MI_PAGE_LARGE);
  mi_assert_internal(block_size < MI_HUGE_BLOCK_SIZE);
  // 1. try to get a segment from our cache
  mi_segment_t* segment = mi_segment_cache_pop(MI_SEGMENT_SIZE, tld);
  if (segment != NULL) {
    mi_segment_init(segment, 0, page_kind, page_shift, tld, os_tld);
    return segment;
  }
  // 2. try to reclaim an abandoned segment
  bool reclaimed;
  segment = mi_segment_try_reclaim(heap, block_size, page_kind, &reclaimed, tld);
  if (reclaimed) {
    // reclaimed the right page right into the heap
    mi_assert_internal(segment != NULL && segment->page_kind == page_kind && page_kind <= MI_PAGE_LARGE);
    return NULL; // pretend out-of-memory as the page will be in the page queue of the heap with available blocks
  }
  else if (segment != NULL) {
    // reclaimed a segment with empty pages (of `page_kind`) in it
    return segment;
  }
  // 3. otherwise allocate a fresh segment
  return mi_segment_alloc(0, page_kind, page_shift, tld, os_tld);
}


/* -----------------------------------------------------------
   Small page allocation
----------------------------------------------------------- */

static mi_page_t* mi_segment_find_free(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(mi_segment_has_free(segment));
  mi_assert_expensive(mi_segment_is_valid(segment, tld));
  for (size_t i = 0; i < segment->capacity; i++) {  // TODO: use a bitmap instead of search?
    mi_page_t* page = &segment->pages[i];
    if (!page->segment_in_use) {
      bool ok = mi_segment_page_claim(segment, page, tld);
      if (ok) return page;
    }
  }
  mi_assert(false);
  return NULL;
}

// Allocate a page inside a segment. Requires that the page has free pages
static mi_page_t* mi_segment_page_alloc_in(mi_segment_t* segment, mi_segments_tld_t* tld) {
  mi_assert_internal(mi_segment_has_free(segment));
  return mi_segment_find_free(segment, tld);
}

static mi_page_t* mi_segment_page_alloc(mi_heap_t* heap, size_t block_size, mi_page_kind_t kind, size_t page_shift, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  // find an available segment the segment free queue
  mi_segment_queue_t* const free_queue = mi_segment_free_queue_of_kind(kind, tld);
  if (mi_segment_queue_is_empty(free_queue)) {
    // possibly allocate or reclaim a fresh segment
    mi_segment_t* const segment = mi_segment_reclaim_or_alloc(heap, block_size, kind, page_shift, tld, os_tld);
    if (segment == NULL) return NULL;  // return NULL if out-of-memory (or reclaimed)
    mi_assert_internal(free_queue->first == segment);
    mi_assert_internal(segment->page_kind==kind);
    mi_assert_internal(segment->used < segment->capacity);
  }
  mi_assert_internal(free_queue->first != NULL);
  mi_page_t* const page = mi_segment_page_alloc_in(free_queue->first, tld);
  mi_assert_internal(page != NULL);
#if MI_DEBUG>=2
  // verify it is committed
  _mi_segment_page_start(_mi_page_segment(page), page, sizeof(void*), NULL, NULL)[0] = 0;
#endif
  return page;
}

static mi_page_t* mi_segment_small_page_alloc(mi_heap_t* heap, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  return mi_segment_page_alloc(heap, block_size, MI_PAGE_SMALL,MI_SMALL_PAGE_SHIFT,tld,os_tld);
}

static mi_page_t* mi_segment_medium_page_alloc(mi_heap_t* heap, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  return mi_segment_page_alloc(heap, block_size, MI_PAGE_MEDIUM, MI_MEDIUM_PAGE_SHIFT, tld, os_tld);
}

/* -----------------------------------------------------------
   large page allocation
----------------------------------------------------------- */

static mi_page_t* mi_segment_large_page_alloc(mi_heap_t* heap, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  mi_segment_t* segment = mi_segment_reclaim_or_alloc(heap,block_size,MI_PAGE_LARGE,MI_LARGE_PAGE_SHIFT,tld,os_tld);
  if (segment == NULL) return NULL;
  mi_page_t* page = mi_segment_find_free(segment, tld);
  mi_assert_internal(page != NULL);
#if MI_DEBUG>=2
  _mi_segment_page_start(segment, page, sizeof(void*), NULL, NULL)[0] = 0;
#endif
  return page;
}

static mi_page_t* mi_segment_huge_page_alloc(size_t size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld)
{
  mi_segment_t* segment = mi_segment_alloc(size, MI_PAGE_HUGE, MI_SEGMENT_SHIFT,tld,os_tld);
  if (segment == NULL) return NULL;
  mi_assert_internal(mi_segment_page_size(segment) - segment->segment_info_size - (2*(MI_SECURE == 0 ? 0 : _mi_os_page_size())) >= size);
  segment->thread_id = 0; // huge pages are immediately abandoned
  mi_segments_track_size(-(long)segment->segment_size, tld);
  mi_page_t* page = mi_segment_find_free(segment, tld);
  mi_assert_internal(page != NULL);
  return page;
}

// free huge block from another thread
void _mi_segment_huge_page_free(mi_segment_t* segment, mi_page_t* page, mi_block_t* block) {
  // huge page segments are always abandoned and can be freed immediately by any thread
  mi_assert_internal(segment->page_kind==MI_PAGE_HUGE);
  mi_assert_internal(segment == _mi_page_segment(page));
  mi_assert_internal(mi_atomic_load_relaxed(&segment->thread_id)==0);

  // claim it and free
  mi_heap_t* heap = mi_heap_get_default(); // issue #221; don't use the internal get_default_heap as we need to ensure the thread is initialized.
  // paranoia: if this it the last reference, the cas should always succeed
  uintptr_t expected_tid = 0;
  if (mi_atomic_cas_strong_acq_rel(&segment->thread_id, &expected_tid, heap->thread_id)) {
    mi_block_set_next(page, block, page->free);
    page->free = block;
    page->used--;
    page->is_zero = false;
    mi_assert(page->used == 0);
    mi_tld_t* tld = heap->tld;
    mi_segments_track_size((long)segment->segment_size, &tld->segments);
    _mi_segment_page_free(page, true, &tld->segments);
  }
#if (MI_DEBUG!=0)
  else {
    mi_assert_internal(false);
  }
#endif
}

/* -----------------------------------------------------------
   Page allocation
----------------------------------------------------------- */

mi_page_t* _mi_segment_page_alloc(mi_heap_t* heap, size_t block_size, mi_segments_tld_t* tld, mi_os_tld_t* os_tld) {
  mi_page_t* page;
  if (block_size <= MI_SMALL_OBJ_SIZE_MAX) {
    page = mi_segment_small_page_alloc(heap, block_size, tld, os_tld);
  }
  else if (block_size <= MI_MEDIUM_OBJ_SIZE_MAX) {
    page = mi_segment_medium_page_alloc(heap, block_size, tld, os_tld);
  }
  else if (block_size <= MI_LARGE_OBJ_SIZE_MAX) {
    page = mi_segment_large_page_alloc(heap, block_size, tld, os_tld);
  }
  else {
    page = mi_segment_huge_page_alloc(block_size,tld,os_tld);
  }
  mi_assert_expensive(page == NULL || mi_segment_is_valid(_mi_page_segment(page),tld));
  mi_assert_internal(page == NULL || (mi_segment_page_size(_mi_page_segment(page)) - (MI_SECURE == 0 ? 0 : _mi_os_page_size())) >= block_size);
  mi_reset_delayed(tld);
  mi_assert_internal(page == NULL || mi_page_not_in_queue(page, tld));
  return page;
}
