/*----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/prim.h"           // _mi_prim_thread_yield
#include "mimalloc/prim/prim-tls.h"  // _mi_theap_default
#include "mimalloc/util/random.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/arena/arena-alloc.h"
#include "mimalloc/profile/sample-guarded.h"
#include "mimalloc/heap/heap.h"
#include "mimalloc/heap/theap.h"

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#pragma warning(disable:4204)  // non-constant aggregate initializer
#endif

/* -----------------------------------------------------------
  Helpers
----------------------------------------------------------- */

// return `true` if ok, `false` to break
typedef bool (theap_page_visitor_fun)(mi_theap_t* theap, mi_page_queue_t* pq, mi_page_t* page, void* arg1, void* arg2);

// Visit all pages in a theap; returns `false` if break was called.
static bool mi_theap_visit_pages(mi_theap_t* theap, theap_page_visitor_fun* fn, bool include_full, void* arg1, void* arg2)
{
  if (theap==NULL || theap->page_count==0) return true;

  // visit all pages
  #if MI_DEBUG>1
  size_t total = theap->page_count;
  size_t count = 0;
  #endif

  const size_t max_bin = (include_full ? MI_BIN_FULL : MI_BIN_FULL - 1);
  for (size_t i = 0; i <= max_bin; i++) {
    mi_page_queue_t* pq = &theap->pages[i];
    mi_page_t* page = pq->first;
    while(page != NULL) {
      mi_page_t* next = page->next; // save next in case the page gets removed from the queue
      mi_assert_internal(mi_page_theap(page) == theap);
      #if MI_DEBUG>1
      count++;
      #endif
      if (!fn(theap, pq, page, arg1, arg2)) return false;
      page = next; // and continue
    }
  }
  mi_assert_internal(!include_full || count == total);
  return true;
}


#if MI_DEBUG>=3
static bool mi_theap_page_is_valid(mi_theap_t* theap, mi_page_queue_t* pq, mi_page_t* page, void* arg1, void* arg2) {
  MI_UNUSED(arg1);
  MI_UNUSED(arg2);
  MI_UNUSED(pq);
  mi_assert_internal(mi_page_theap(page) == theap);
  mi_theap_t* const page_theap = _mi_heap_theap_peek(page->heap);
  mi_assert_internal(page_theap == NULL || theap == page_theap || mi_theap_is_detached(theap));
  mi_assert_expensive(_mi_page_is_valid(page));
  return true;
}

static bool mi_theap_is_valid(mi_theap_t* theap) {
  mi_assert_internal(theap!=NULL);
  mi_heap_t* const heap = mi_theap_heap_peek(theap);
  mi_assert_internal(heap != NULL);
  mi_theap_t* const heap_theap = _mi_heap_theap_peek(heap);  // don't use mi_heap_theap as that may re-initialize the thread
  // Detached metadata theaps are shared and need not match the calling thread's TLS.
  mi_assert_internal(heap_theap==NULL || heap_theap == theap || mi_theap_is_detached(theap));
  mi_theap_visit_pages(theap, &mi_theap_page_is_valid, true, NULL, NULL);
  for (size_t bin = 0; bin < MI_BIN_COUNT; bin++) {
    mi_assert_internal(_mi_page_queue_is_valid(theap, &theap->pages[bin]));
  }
  return true;
}
#endif




/* -----------------------------------------------------------
  "Collect" pages by migrating `local_free` and `thread_free`
  lists and freeing empty pages. This is done when a thread
  stops (and in that case abandons pages if there are still
  blocks alive)
----------------------------------------------------------- */

typedef enum mi_collect_e {
  MI_NORMAL,
  MI_FORCE,
  MI_ABANDON
} mi_collect_t;


static bool mi_theap_page_collect(mi_theap_t* theap, mi_page_queue_t* pq, mi_page_t* page, void* arg_collect, void* arg2 ) {
  MI_UNUSED(arg2);
  MI_UNUSED(theap);
  mi_assert_expensive(mi_theap_page_is_valid(theap, pq, page, NULL, NULL));
  mi_collect_t collect = *((mi_collect_t*)arg_collect);
  _mi_page_free_collect(page, collect >= MI_FORCE);  // update used count
  _mi_page_update_stats(page);
  if (mi_page_all_free(page)) {
    // no more used blocks, possibly free the page.
    if (collect >= MI_FORCE || page->retire_expire == 0) {  // either forced/abandon, or not already retired
      // note: this will potentially free retired pages as well.
      if (_mi_page_free(page, pq)) return true;
    }
  }
  if (collect == MI_ABANDON) {
    _mi_page_abandon(page, pq);
  }
  return true; // don't break
}

void _mi_theap_merge_stats(mi_theap_t* theap) {
  mi_assert_internal(mi_theap_is_initialized(theap));
  mi_heap_t* const heap = mi_theap_heap(theap);
  _mi_stats_merge_into(&heap->stats, &theap->stats);
}

static void mi_theap_collect_ex(mi_theap_t* theap, mi_collect_t collect)
{
  if (theap==NULL || !mi_theap_is_initialized(theap)) return;
  mi_assert_expensive(mi_theap_is_valid(theap));

  const bool force = (collect >= MI_FORCE);
  if (collect != MI_ABANDON) {     // skip on thread shutdown to avoid running user-code after process exit (issue #1377)
    _mi_deferred_free(theap, force);
  }

  // python/cpython#112532: we may be called from a thread that is not the owner of the theap
  // const bool is_main_thread = (_mi_is_main_thread() && theap->thread_id == _mi_thread_id());

  // collect retired pages (and full pages if theap->allow_page_abandon is false)
  _mi_theap_collect_retired(theap, force);

  // collect all pages owned by this thread
  mi_theap_visit_pages(theap, &mi_theap_page_collect, (collect!=MI_NORMAL), &collect, NULL);  // dont normally visit full pages, see issue #1220

  // collect arenas (this is program wide so don't force purges on abandonment of threads)
  //mi_atomic_storei64_release(&theap->tld->subproc->purge_expire, 1);
  if (collect != MI_ABANDON) {
    _mi_arenas_collect(collect == MI_FORCE /* force purge? */, collect >= MI_FORCE /* visit all? */, theap->tld);
  }

  // merge statistics
  if (mi_option_is_enabled(mi_option_stats_merge_on_collect)) {
    _mi_theap_merge_stats(theap);
  }
}

void _mi_theap_collect_abandon(mi_theap_t* theap) {
  mi_theap_collect_ex(theap, MI_ABANDON);
}

// Abandon all owned pages of a valid but detached theap (`tld==NULL`) into the heap-wide registries.
void _mi_theap_abandon_owned_pages(mi_theap_t* theap) {
  mi_assert_internal(theap!=NULL);
  mi_assert_internal(theap->tld == NULL);  // should be detached as we may access from another thread
  if (theap==NULL) return;
  for (size_t bin = 0; bin <= MI_BIN_FULL; bin++) {
    mi_page_queue_t* const pq = &theap->pages[bin];
    mi_page_t* page = pq->first;
    while (page != NULL) {
      mi_page_t* const next = page->next;  // save as the page is removed from the queue
      _mi_page_free_collect(page, true);   // collect cross-thread frees so `used` is precise
      if (!mi_page_all_free(page) || !_mi_page_free(page, pq)) {
        _mi_page_abandon(page, pq);
      }
      page = next;
    }
  }
}

void mi_theap_collect(mi_theap_t* theap, bool force) mi_attr_noexcept {
  mi_theap_collect_ex(theap, (force ? MI_FORCE : MI_NORMAL));
}

void mi_collect(bool force) mi_attr_noexcept {
  // cannot really collect process wide, just a theap..
  mi_theap_collect(_mi_theap_default(), force);
}

void mi_heap_collect(mi_heap_t* heap, bool force) {
  // cannot really collect a heap, just a theap..
  mi_theap_collect(mi_heap_theap(heap), force);
}

/* -----------------------------------------------------------
  Heap new
----------------------------------------------------------- */

mi_theap_t* mi_theap_get_default(void) {
  mi_theap_t* theap = _mi_theap_default();
  if mi_unlikely(!mi_theap_is_initialized(theap)) {
    mi_thread_init();
    theap = _mi_theap_default();
    mi_assert_internal(mi_theap_is_initialized(theap));
  }
  return theap;
}

mi_theap_t* mi_theap_set_default(mi_theap_t* theap) {
  mi_theap_t* const previous = mi_theap_get_default();
  if (mi_theap_is_initialized(theap)) {
    _mi_theap_default_set(theap);
  }
  return previous;
}


static void mi_theap_options_init(mi_theap_t* theap, mi_heap_t* heap) {
  theap->allow_page_reclaim = (mi_heap_reclaim_on_free(heap) >= 0);
  theap->allow_page_abandon = (mi_option_get(mi_option_page_full_retain) >= 0); // nowadays qsbr pages can be abandoned: && !mi_heap_qsbr_is_enabled(heap));
  theap->page_full_retain = mi_option_get_clamp(mi_option_page_full_retain, -1, 32);
  theap->is_detached = (theap->tld->thread_id == MI_THREADID_DETACHED);
}

// todo: make order of parameters consistent (but would that break compat with CPython?)
void _mi_theap_init(mi_theap_t* theap, mi_heap_t* heap, mi_tld_t* tld)
{
  mi_assert_internal(theap!=NULL);
  mi_assert_internal(heap!=NULL);
  mi_assert_internal(tld!=NULL);
  mi_memid_t memid = theap->memid;
  mi_memcpy_aligned(theap, &_mi_theap_empty, sizeof(mi_theap_t));
  theap->memid = memid;
  theap->tld   = tld;  // avoid reading the thread-local tld during initialization
  theap->profile_never = false;  // only set for the empty and meta theaps (see `mi_theap_profile_never`)
  mi_atomic_store_release(&theap->refcount,1);
  mi_atomic_store_ptr_release(mi_subproc_t,&theap->subproc,heap->subproc);
  mi_assert_internal(theap->stats.size == sizeof(mi_stats_t));
  mi_theap_options_init(theap, heap);

  if (theap->tld->is_in_threadpool) {
    // if we run as part of a thread pool it is better to not arbitrarily reclaim abandoned pages into our theap.
    // this is checked in `alloc/free.c:mi_free_try_collect_mt`
    // .. but abandoning is good in this case: quarter the full page retain (possibly to 0)
    // (so blocked threads do not hold on to too much memory)
    if (theap->page_full_retain > 0) {
      theap->page_full_retain = theap->page_full_retain / 4;
    }
  }

  // push on the thread local theaps list
  mi_theap_t* head = NULL;
  mi_random_ctx_t head_random;
  mi_lock(&theap->tld->theaps_lock) {
    head = theap->tld->theaps;
    theap->tprev = NULL;
    theap->tnext = head;
    theap->tld->theaps = theap;
    if (head!=NULL) {
      head->tprev = theap;
      head_random = head->random;
    }
  }

  // initialize random if heap==NULL
  if (head==NULL) {  // first theap of the first thread?
    // will be initialized on the first random_next/split call. (see #1377, W20)

    // #if defined(_WIN32) && !defined(MI_SHARED_LIB)
    // if (tld->thread_seq==0) {
    //   _mi_random_init_weak(&theap->random);    // prevent allocation failure during bcrypt dll initialization with static linking (issue #1185)
    // }
    // else
    // #endif
    // {
    //   _mi_random_init(&theap->random);
    // }

  }
  else {
    _mi_random_split(&head_random, &theap->random); // &theap->random is used as nonce so it is ok if threads capture the same head->random
  }
  // theap->cookie = _mi_theap_random_next(theap) | 1;
  _mi_theap_guarded_init(theap); // needs theap->random
  if (!theap->is_detached) {
    mi_subproc_stat_increase(mi_theap_subproc(theap),theaps,1);  // on subproc to match theap_free_mem
  }

  // only now set the heap member as it is used to determine if a theap is initialized
  mi_atomic_store_ptr_release(mi_heap_t,&theap->heap,heap);

  // push on the heap's theap list
  mi_lock(&heap->theaps_lock) {
    head = heap->theaps;
    theap->hprev = NULL;
    theap->hnext = head;
    if (head!=NULL) { head->hprev = theap; }
    heap->theaps = theap;
  }
}

mi_theap_t* _mi_theap_alloc(mi_heap_t* heap, mi_tld_t* tld) {
  mi_assert_internal(tld!=NULL);
  mi_assert_internal(heap!=NULL);
  mi_assert_internal(tld->thread_id == MI_THREADID_DETACHED || _mi_thread_id() == tld->thread_id);
  // mi_assert_internal(_mi_heap_theap_peek(heap)==NULL);  // don't access thread locals as this is called on thread init

  // allocate and initialize a theap
  mi_memid_t memid;
  mi_theap_t* theap;

  if (heap->exclusive_arena == NULL) {
    theap = (mi_theap_t*)_mi_meta_zalloc(heap->subproc, sizeof(mi_theap_t), &memid);
  }
  else {
    // theaps associated with a specific arena are allocated in that arena
    // note: takes up at least one slice which is quite wasteful...
    const size_t size = mi_align_up(sizeof(mi_theap_t),MI_ARENA_MIN_OBJ_SIZE);
    theap = (mi_theap_t*)_mi_arenas_alloc(heap, size, true, true, heap->exclusive_arena, tld->thread_seq, tld->numa_node, &memid);
  }
  if (theap==NULL) {
    _mi_error_message(ENOMEM, "unable to allocate theap meta-data\n");
    return NULL;
  }

  theap->memid = memid;
  return theap;
}

mi_theap_t* _mi_theap_create(mi_heap_t* heap, mi_tld_t* tld) {
  mi_theap_t* theap = _mi_theap_alloc(heap,tld);
  if (theap == NULL) return NULL;
  _mi_theap_init(theap, heap, tld);
  return theap;
}

size_t _mi_theap_random_next(mi_theap_t* theap) {
  return _mi_random_next(&theap->random);
}

static void mi_theap_free_mem(mi_theap_t* theap) {
  if (theap!=NULL) {
    #if MI_QSBR
    mi_assert_internal(theap->qsbr_first == NULL && theap->qsbr_last == NULL);
    #endif
    mi_subproc_t* const subproc = mi_atomic_load_ptr_relaxed(mi_subproc_t,&theap->subproc);
    if (!theap->is_detached) {
      mi_subproc_stat_decrease(subproc,theaps,1);
    }
    _mi_meta_free(subproc, theap, theap->memid);
  }
}

// we need to reference count theaps due to the _mi_theap_cached thread locals
void _mi_theap_incref(mi_theap_t* theap) {
  if (theap!=NULL && !mi_memid_needs_no_free(theap->memid)) {
    mi_atomic_increment_acq_rel(&theap->refcount);
  }
}

void _mi_theap_decref(mi_theap_t* theap) {
  if (theap!=NULL && !mi_memid_needs_no_free(theap->memid)) {
    if (mi_atomic_decrement_acq_rel(&theap->refcount) == 1) {
      mi_theap_free_mem(theap);
    }
  }
}

// Thread termination and heap delete/destroy might run concurrently
// and we need to ensure we free the memory correctly. A heap or tld
// will first "detach" its theaps so it has a list with theaps that are
// no longer shared, and only then free's the theaps in that list.
// To detach we need to hold both the `heap->theaps_lock` and the `tld->theaps_lock`.
// Due to lock-inversion we need to use `mi_lock_try_acquire` and if that fails
// we back-off, release the outer lock, and try again until we succeed.

// Both the heap and tld list locks must be held.
static void mi_theap_detach_from_tld(mi_theap_t* theap) {
  mi_tld_t* const tld = theap->tld;
  if (theap->tnext != NULL) { theap->tnext->tprev = theap->tprev; }
  if (theap->tprev != NULL) { theap->tprev->tnext = theap->tnext; }
                      else { mi_assert_internal(tld->theaps == theap); tld->theaps = theap->tnext; }
  theap->tnext = theap->tprev = NULL;
  theap->tld = NULL;
}

static void mi_theap_detach_from_heap(mi_theap_t* theap) {
  mi_heap_t* const heap = mi_theap_heap(theap);
  if (theap->hnext != NULL) { theap->hnext->hprev = theap->hprev; }
  if (theap->hprev != NULL) { theap->hprev->hnext = theap->hnext; }
                      else { mi_assert_internal(heap->theaps == theap); heap->theaps = theap->hnext; }
  theap->hnext = theap->hprev = NULL;
  mi_atomic_store_ptr_release(mi_heap_t, &theap->heap, NULL);
}

// Remove the theaps in this heap from any thread local tld lists.
void _mi_heap_detach_theaps( mi_heap_t* heap ) {
  bool all_detached;
  do {
    all_detached = true;
    mi_lock(&heap->theaps_lock) {
      mi_theap_t* theap = heap->theaps;
      while (theap != NULL) {
        mi_theap_t* next = theap->hnext;
        mi_tld_t* tld = theap->tld;
        if (tld != NULL) {
          if (mi_lock_try_acquire(&tld->theaps_lock)) {
            mi_theap_detach_from_tld(theap);
            mi_lock_release(&tld->theaps_lock);
          }
          else {
            all_detached = false;
          }
        }
        theap = next;
      }
    }
    if (!all_detached) {
      mi_subproc_stat_counter_increase(heap->subproc,heaps_delete_wait,1);
      _mi_prim_thread_yield();
    }
  } while (!all_detached);
}

// Remove the theaps in this thread from the heaps that own them.
void _mi_tld_detach_theaps( mi_tld_t* tld ) {
  bool all_detached;
  do {
    all_detached = true;
    mi_lock(&tld->theaps_lock) {
      mi_theap_t* theap = tld->theaps;
      while (theap != NULL) {
        mi_theap_t* next = theap->tnext;
        mi_assert_internal(theap->page_count==0);
        mi_heap_t* heap = mi_theap_heap_peek(theap); // now the heap might be NULL from an earlier iteration
        if (heap != NULL) {
          if (mi_lock_try_acquire(&heap->theaps_lock)) {
            // merge stats into the owning heap stats
            _mi_stats_merge_into(&heap->stats, &theap->stats);
            mi_theap_detach_from_heap(theap);
            mi_lock_release(&heap->theaps_lock);
          }
          else {
            all_detached = false;
          }
        }
        theap = next;
      }
    }
    if (!all_detached) {
      mi_subproc_stat_counter_increase(tld->subproc,heaps_delete_wait,1);
      _mi_prim_thread_yield();
    }
  } while (!all_detached);
}



/* -----------------------------------------------------------
  Safe theap delete
----------------------------------------------------------- */

void mi_theap_delete(mi_theap_t* theap) mi_attr_noexcept {
  if (theap==NULL) return;
  // The main theap stays alive even if a concurrent heap delete invalidates our default.
  mi_theap_t* const main = _mi_theap_main();
  if (main==NULL) {
    _mi_warning_message("cannot delete a theap on an uninitialized thread\n");
    return;
  }
  mi_tld_t* const tld = main->tld;
  if (_mi_theap_default() == theap) {
    _mi_theap_default_set(main);
  }

  bool found = false;
  bool release = false;
  mi_lock(&tld->theaps_lock) {
    // Do not dereference the argument until list membership protects it from heap deletion.
    mi_theap_t* owned = tld->theaps;
    while (owned!=NULL && owned!=theap) { owned = owned->tnext; }
    if (owned!=NULL) {
      found = true;
      mi_heap_t* const heap = mi_theap_heap(owned);
      mi_assert_internal(heap!=NULL);
      mi_lock(&heap->theaps_lock) {
        if (owned == main) {
          _mi_theap_collect_abandon(owned);  // retain the backing theap and tld for the living thread
          _mi_theap_merge_stats(owned);
        }
        else {
          const bool cleared = _mi_thread_local_set(heap->theap, NULL);
          mi_assert_internal(cleared); MI_UNUSED_RELEASE(cleared);
          mi_theap_detach_from_tld(owned);
          _mi_theap_abandon_owned_pages(owned);
          _mi_theap_merge_stats(owned);
          mi_theap_detach_from_heap(owned);
          release = true;
        }
      }
    }
  }
  if (_mi_theap_cached() == theap) {
    _mi_theap_cached_set(_mi_theap_empty_get());
  }
  if (release) { _mi_theap_decref(theap); }
  if (!found) { _mi_verbose_message("theap %p is no longer owned by this thread\n", theap); }
}


void mi_theap_quiescent_abandon(mi_theap_t* theap) mi_attr_noexcept {
  if (theap==NULL || !mi_theap_is_initialized(theap)) return;
  mi_tld_t* const tld = theap->tld;
  mi_heap_t* const heap = mi_theap_heap(theap);
  mi_lock_maybe(&tld->theaps_lock, tld!=NULL) {
    mi_lock(&heap->theaps_lock) {
      // Temporarily detach for foreign-thread page cleanup, without changing any TLS references.
      theap->tld = NULL;
      _mi_theap_abandon_owned_pages(theap);
      theap->tld = tld;
      _mi_theap_merge_stats(theap);
    }
  }
}

/* -----------------------------------------------------------
  Load/unload theaps
----------------------------------------------------------- */
/*
void mi_theap_unload(mi_theap_t* theap) {
  mi_assert(mi_theap_is_initialized(theap));
  mi_assert_expensive(mi_theap_is_valid(theap));
  if (theap==NULL || !mi_theap_is_initialized(theap)) return;
  if (mi_theap_heap(theap)->exclusive_arena == NULL) {
    _mi_warning_message("cannot unload theaps that are not associated with an exclusive arena\n");
    return;
  }

  // abandon all pages so all thread'id in the pages are cleared
  _mi_theap_collect_abandon(theap);
  mi_assert_internal(theap->page_count==0);

  // remove from theap list
  mi_theap_free(theap, false); // but don't actually free the memory

  // disassociate from the current thread-local and static state
  theap->tld = NULL;
  return;
}

bool mi_theap_reload(mi_theap_t* theap, mi_arena_id_t arena_id) {
  mi_assert(mi_theap_is_initialized(theap));
  if (theap==NULL || !mi_theap_is_initialized(theap)) return false;
  if (mi_theap_heap(theap)->exclusive_arena == NULL) {
    _mi_warning_message("cannot reload theaps that were not associated with an exclusive arena\n");
    return false;
  }
  if (theap->tld != NULL) {
    _mi_warning_message("cannot reload theaps that were not unloaded first\n");
    return false;
  }
  mi_arena_t* arena = _mi_arena_from_id(arena_id);
  if (mi_theap_heap(theap)->exclusive_arena != arena) {
    _mi_warning_message("trying to reload a theap at a different arena address: %p vs %p\n", mi_theap_heap(theap)->exclusive_arena, arena);
    return false;
  }

  mi_assert_internal(theap->page_count==0);

  // re-associate with the current thread-local and static state
  theap->tld = mi_theap_get_default()->tld;

  // reinit direct pages (as we may be in a different process)
  mi_assert_internal(theap->page_count == 0);
  for (size_t i = 0; i < MI_PAGES_DIRECT; i++) {
    theap->pages_free_direct[i] = _mi_page_empty_get();
  }

  // push on the thread local theaps list
  theap->tnext = theap->tld->theaps;
  theap->tld->theaps = theap;
  return true;
}
*/


/* -----------------------------------------------------------
  Visit all theap blocks and areas
  Todo: enable visiting abandoned pages, and
        enable visiting all blocks of all theaps across threads
----------------------------------------------------------- */

void _mi_heap_area_init(mi_heap_area_t* area, mi_page_t* page) {
  const size_t bsize = mi_page_block_size(page);
  const size_t ubsize = mi_page_usable_block_size(page);
  area->reserved = page->reserved * bsize;
  area->committed = page->capacity * bsize;
  area->blocks = mi_page_start(page);
  area->used = mi_page_used(page);   // number of blocks in use (#553)
  area->block_size = ubsize;
  area->full_block_size = bsize;
  area->reserved1 = page;
}

static void mi_get_fast_divisor(size_t divisor, uint64_t* magic, size_t* shift) {
  mi_assert_internal(divisor > 0 && divisor <= UINT32_MAX);
  *shift = MI_SIZE_BITS - mi_clz(divisor - 1);
  *magic = ((((uint64_t)1 << 32) * (((uint64_t)1 << *shift) - divisor)) / divisor + 1);
}

static size_t mi_fast_divide(size_t n, uint64_t magic, size_t shift) {
  mi_assert_internal(n <= UINT32_MAX);
  const uint64_t hi = ((uint64_t)n * magic) >> 32;
  return (size_t)((hi + n) >> shift);
}

bool _mi_theap_area_visit_blocks(const mi_heap_area_t* area, mi_page_t* page, mi_block_visit_fun* visitor, void* arg) {
  mi_assert(area != NULL);
  if (area==NULL) return true;
  mi_assert(page != NULL);
  if (page == NULL) return true;

  _mi_page_free_collect(page,true);              // collect both thread_delayed and local_free
  mi_assert_internal(page->local_free == NULL);
  if (mi_page_used(page) == 0) return true;

  size_t psize;
  uint8_t* const pstart = mi_page_area(page, &psize);
  mi_heap_t* const heap = mi_page_heap(page);
  const size_t bsize    = mi_page_block_size(page);
  const size_t ubsize   = mi_page_usable_block_size(page); // without padding

  // optimize page with one block
  if (page->capacity == 1) {
    mi_assert_internal(mi_page_used(page) == 1 && page->free == NULL);
    return visitor(heap, area, pstart, ubsize, arg);
  }
  mi_assert(bsize <= UINT32_MAX);

  // optimize full pages
  if (mi_page_used(page) == page->capacity) {
    uint8_t* block = pstart;
    for (size_t i = 0; i < page->capacity; i++) {
      if (!visitor(heap, area, block, ubsize, arg)) return false;
      block += bsize;
    }
    return true;
  }

  // create a bitmap of free blocks.
  #define MI_MAX_BLOCKS   (MI_SMALL_PAGE_SIZE / sizeof(void*))
  uintptr_t free_map[MI_MAX_BLOCKS / MI_INTPTR_BITS];
  const uintptr_t bmapsize = mi_divide_up(page->capacity, MI_INTPTR_BITS);
  memset(free_map, 0, bmapsize * sizeof(intptr_t));
  if (page->capacity % MI_INTPTR_BITS != 0) {
    // mark left-over bits at the end as free
    size_t shift   = (page->capacity % MI_INTPTR_BITS);
    uintptr_t mask = (UINTPTR_MAX << shift);
    free_map[bmapsize - 1] = mask;
  }

  // fast repeated division by the block size
  uint64_t magic;
  size_t   shift;
  mi_get_fast_divisor(bsize, &magic, &shift);

  #if MI_DEBUG>1
  size_t free_count = 0;
  #endif
  for (mi_block_t* block = page->free; block != NULL; block = mi_block_next(page, block)) {
    #if MI_DEBUG>1
    free_count++;
    #endif
    mi_assert_internal((uint8_t*)block >= pstart && (uint8_t*)block < (pstart + psize));
    size_t offset = (uint8_t*)block - pstart;
    mi_assert_internal(offset % bsize == 0);
    mi_assert_internal(offset <= UINT32_MAX);
    size_t blockidx = mi_fast_divide(offset, magic, shift);
    mi_assert_internal(blockidx == offset / bsize);
    mi_assert_internal(blockidx < MI_MAX_BLOCKS);
    size_t bitidx = (blockidx / MI_INTPTR_BITS);
    size_t bit = blockidx - (bitidx * MI_INTPTR_BITS);
    free_map[bitidx] |= ((uintptr_t)1 << bit);
  }
  mi_assert_internal(page->capacity == (free_count + mi_page_used(page)));

  // walk through all blocks skipping the free ones
  #if MI_DEBUG>1
  size_t used_count = 0;
  #endif
  uint8_t* block = pstart;
  for (size_t i = 0; i < bmapsize; i++) {
    if (free_map[i] == 0) {
      // every block is in use
      for (size_t j = 0; j < MI_INTPTR_BITS; j++) {
        #if MI_DEBUG>1
        used_count++;
        #endif
        if (!visitor(heap, area, block, ubsize, arg)) return false;
        block += bsize;
      }
    }
    else {
      // visit the used blocks in the mask
      uintptr_t m = ~free_map[i];
      while (m != 0) {
        #if MI_DEBUG>1
        used_count++;
        #endif
        size_t bitidx = mi_ctz(m);
        if (!visitor(heap, area, block + (bitidx * bsize), ubsize, arg)) return false;
        m &= m - 1;  // clear least significant bit
      }
      block += bsize * MI_INTPTR_BITS;
    }
  }
  mi_assert_internal(mi_page_used(page) == used_count);
  return true;
}

// bool _mi_page_visit_blocks( mi_page_t* page, mi_block_visit_fun* visitor, void* arg ) {
//   mi_heap_area_t area;
//   _mi_heap_area_init(&area, page);
//   return _mi_theap_area_visit_blocks(&area, page, visitor, arg);
// }


// Separate struct to keep `mi_page_t` out of the public interface
typedef struct mi_theap_area_ex_s {
  mi_heap_area_t area;
  mi_page_t* page;
} mi_theap_area_ex_t;

typedef bool (mi_theap_area_visit_fun)(const mi_theap_t* theap, const mi_theap_area_ex_t* area, void* arg);

static bool mi_theap_visit_areas_page(mi_theap_t* theap, mi_page_queue_t* pq, mi_page_t* page, void* vfun, void* arg) {
  MI_UNUSED(theap);
  MI_UNUSED(pq);
  mi_theap_area_visit_fun* fun = (mi_theap_area_visit_fun*)vfun;
  mi_theap_area_ex_t xarea;
  xarea.page = page;
  _mi_page_free_collect(page, true);  // collect cross-thread free'd blocks so `area.used` is precise
  _mi_heap_area_init(&xarea.area, page);
  return fun(theap, &xarea, arg);
}

// Visit all theap pages as areas
static bool mi_theap_visit_areas(const mi_theap_t* theap, mi_theap_area_visit_fun* visitor, void* arg) {
  if (visitor == NULL) return false;
  return mi_theap_visit_pages((mi_theap_t*)theap, &mi_theap_visit_areas_page, true, (void*)(visitor), arg); // note: function pointer to void* :-{
}

// Just to pass arguments
typedef struct mi_visit_blocks_args_s {
  bool  visit_blocks;
  mi_block_visit_fun* visitor;
  void* arg;
} mi_visit_blocks_args_t;

static bool mi_theap_area_visitor(const mi_theap_t* theap, const mi_theap_area_ex_t* xarea, void* arg) {
  mi_visit_blocks_args_t* args = (mi_visit_blocks_args_t*)arg;
  if (!args->visitor(mi_theap_heap(theap), &xarea->area, NULL, xarea->area.block_size, args->arg)) return false;
  if (args->visit_blocks) {
    return _mi_theap_area_visit_blocks(&xarea->area, xarea->page, args->visitor, args->arg);
  }
  else {
    return true;
  }
}

// Visit all blocks in a theap
bool mi_theap_visit_blocks(const mi_theap_t* theap, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  mi_visit_blocks_args_t args = { visit_blocks, visitor, arg };
  return mi_theap_visit_areas(theap, &mi_theap_area_visitor, &args);
}
