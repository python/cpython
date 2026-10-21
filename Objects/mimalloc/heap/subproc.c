/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/arena/arena-alloc.h"
#include "mimalloc/arena/arena.h"
#include "mimalloc/heap/page-map.h"
#include "mimalloc/profile/sample-profile.h"
#include "mimalloc/heap/subproc.h"
#include "mimalloc/util/threadlocal.h"

// pre-allocate the main subprocess structure.
static mi_decl_cache_align mi_subproc_t mi_process_subproc_main = mi_init_struct_zero;
static mi_subproc_t* mi_subprocs = NULL;
static mi_lock_t     mi_subprocs_lock = MI_LOCK_INITIALIZER;


/* -----------------------------------------------------------
  Meta-data allocation
  We allocate thread local data and theaps through a dedicated
  theap `subproc.theap_meta` which uses a detached tld with
  a detached thread id. The initial theap_meta is statically
  allocated and can thus be used to allocate on an as yet
  uninitialized thread or process.
  We need to take a lock though to allocate safely on the
  detached `theap_meta`.
----------------------------------------------------------- */

void* _mi_meta_zalloc( mi_subproc_t* subproc, size_t size, mi_memid_t* memid ) {
  mi_assert_internal(subproc->theap_meta != NULL);
  void* p = NULL;
  mi_lock(&subproc->theap_meta_lock) {
    p = mi_theap_zalloc(subproc->theap_meta, size);
    if (memid != NULL) { *memid = (p==NULL ? _mi_memid_none() : _mi_memid_create_malloc(p,size,true) ); }
  }
  return p;
}

void* _mi_meta_zalloc_aligned( mi_subproc_t* subproc, size_t size, size_t aligned, mi_memid_t* memid ) {
  mi_assert_internal(subproc->theap_meta != NULL);
  void* p = NULL;
  mi_lock(&subproc->theap_meta_lock) {
    p = mi_theap_zalloc_aligned(subproc->theap_meta, size, aligned);
    if (memid != NULL) { *memid = (p==NULL ? _mi_memid_none() : _mi_memid_create_malloc(p,size,true) ); }
  }
  return p;
}

void* _mi_meta_rezalloc( mi_subproc_t* subproc, void* oldp, size_t newsize, mi_memid_t* memid ) {
  mi_assert_internal(subproc->theap_meta != NULL);
  // note: since we take a meta lock we cannot use `mi_theap_rezalloc` as that could call `mi_free` which
  // can call `mi_stat_free` which would try to take the meta lock again. See issue #1358.
  void* p = NULL;
  mi_lock(&subproc->theap_meta_lock) {
    p = mi_theap_zalloc(subproc->theap_meta, newsize);
  }
  if (p!=NULL) {
    if (oldp!=NULL) {
      const size_t oldsize  = mi_usable_size(oldp);
      const size_t copysize = (newsize < oldsize ? newsize : oldsize);
      mi_memcpy(p,oldp,copysize);
      if (memid!=NULL) { _mi_meta_free(subproc,oldp,*memid); }
                  else { mi_free(oldp); }
    }
    if (memid!=NULL) { *memid = _mi_memid_create_malloc(p,newsize,true); }
  }
  else {
    if (memid!=NULL) { *memid = _mi_memid_none(); }
  }
  return p;
}

void _mi_meta_free(mi_subproc_t* subproc, void* p, mi_memid_t memid) {
  if (p==NULL || mi_memid_needs_no_free(memid)) return;
  if (memid.memkind == MI_MEM_MALLOC) {
    mi_free(p);
  }
  else {
    mi_assert_internal(subproc!=NULL);
    _mi_arenas_free(subproc, p, _mi_memid_size(memid), memid);
  }
}

bool _mi_meta_is_meta_page(const mi_subproc_t* subproc, const mi_page_t* page) {
  if (page==NULL) return false;
  mi_theap_t* theap = page->theap;
  return (theap != NULL && theap == subproc->theap_meta);
}


/* -----------------------------------------------------------
  Sub process helpers
----------------------------------------------------------- */

mi_subproc_t* _mi_subproc_main(void) {
  return &mi_process_subproc_main;
}

bool _mi_subproc_is_main(mi_subproc_t* subproc) {
  return (subproc == &mi_process_subproc_main);
}

mi_subproc_t* _mi_subproc(void) {
  mi_theap_t* theap = _mi_theap_default();
  if (theap == NULL || theap->tld == NULL || theap->tld->subproc == NULL) {  // see issue #1289 and #1391
    return _mi_subproc_main();
  }
  else {
    return theap->tld->subproc;
  }
}

mi_heap_t* mi_heap_main(void) {
  return _mi_subproc_heap_main(_mi_subproc()); // don't use mi_theap_main_init_get() so this call works during process_init
}


mi_subproc_t* _mi_subproc_from_id(mi_subproc_id_t subproc_id) {
  return (mi_subproc_t*)(subproc_id._mi_subproc_id);
}

mi_subproc_id_t _mi_subproc_to_id(mi_subproc_t* subproc) {
  mi_subproc_id_t id = { subproc };
  return id;
}

mi_subproc_id_t mi_subproc_main(void) {
  return _mi_subproc_to_id(_mi_subproc_main());
}

mi_subproc_id_t mi_subproc_current(void) {
  return _mi_subproc_to_id(_mi_subproc());
}


/* -----------------------------------------------------------
  Sub process creation
----------------------------------------------------------- */


static mi_subproc_t* mi_subproc_init(mi_subproc_t* subproc, mi_subproc_t* parent) {
  static _Atomic(size_t) subproc_total_count;
  subproc->parent = parent;
  subproc->subproc_seq = mi_atomic_increment_relaxed(&subproc_total_count);
  mi_stats_header_init(&subproc->stats);
  mi_lock_init(&subproc->arena_reserve_lock);
  mi_lock_init(&subproc->heaps_lock);
  mi_lock_init(&subproc->theap_meta_lock);
  mi_lock(&mi_subprocs_lock) {
    // push on subproc list
    subproc->next = mi_subprocs;
    if (mi_subprocs!=NULL) { mi_subprocs->prev = subproc; }
    mi_subprocs = subproc;
  }
  return subproc;
}

mi_subproc_id_t mi_subproc_new(void) {
  mi_thread_init();
  mi_subproc_t* const parent = _mi_subproc();
  mi_memid_t memid;
  mi_subproc_t* const subproc = (mi_subproc_t*)_mi_meta_zalloc(parent, sizeof(mi_subproc_t), &memid);
  if (subproc == NULL) { return _mi_subproc_to_id(NULL); }
  subproc->memid  = memid;

  mi_memid_t theap_memid;
  mi_theap_t* const theap_meta = (mi_theap_t*)_mi_meta_zalloc(parent, sizeof(mi_theap_t), &theap_memid);
  if (theap_meta==NULL) {
    _mi_meta_free(parent, subproc, memid);
    return _mi_subproc_to_id(NULL);
  }
  theap_meta->memid = memid;

  // init subproc
  mi_subproc_init(subproc,parent);

  // init main heap
  mi_heap_t* heap_main = _mi_heap_new_for_subproc(subproc,0,true,false);
  if (heap_main==NULL) {
    _mi_meta_free(parent, theap_meta, theap_meta->memid);
    mi_subproc_destroy(_mi_subproc_to_id(subproc));
    return _mi_subproc_to_id(NULL);
  }
  mi_assert_internal(subproc->heap_main == heap_main);

  // init meta theap
  mi_assert_internal(parent->theap_meta!=NULL);
  mi_assert_internal(parent->theap_meta->tld!=NULL);
  mi_assert_internal(parent->theap_meta->tld->thread_id == MI_THREADID_DETACHED);
  _mi_theap_init(theap_meta,heap_main,parent->theap_meta->tld /* detached tld */);
  mi_theap_profile_never(theap_meta);  // permanently exclude from profiling, see `mi_theap_profile_never`
  subproc->theap_meta = theap_meta;

  return _mi_subproc_to_id(subproc);
}


/* -----------------------------------------------------------
  Sub process destruction
----------------------------------------------------------- */

// destroy all subproc resources including arena's, heap's etc.
static void mi_subproc_unsafe_destroy(mi_subproc_t* subproc, bool acquire_subprocs_lock)
{
  if (subproc==NULL) return;

  // remove from the subproc list
  mi_lock_maybe(&mi_subprocs_lock, acquire_subprocs_lock) {
    if (subproc->next!=NULL) { subproc->next->prev = subproc->prev;  }
    if (subproc->prev!=NULL) { subproc->prev->next = subproc->next;  }
                        else { mi_assert_internal(mi_subprocs==subproc);  mi_subprocs = subproc->next; }
  }

  // destroy all subproc heaps
  mi_lock(&subproc->heaps_lock) {
    mi_heap_t* heap = subproc->heaps;
    while (heap != NULL) {
      mi_heap_t* next = heap->next;
      if (heap!=subproc->heap_main) { _mi_heap_force_destroy(heap, false /* don't re-acquire the heaps_lock */); }
      heap = next;
    }
    mi_assert_internal(subproc->heap_main==NULL || subproc->heaps == subproc->heap_main);
    if (subproc->heap_main!=NULL) {
      _mi_thread_locals_thread_done(); // release thread locals that may have been allocated (safe as the main heap uses the fast key)
      if (_mi_subproc_is_main(subproc)) {
        _mi_thread_locals_done();
      }
      _mi_heap_force_destroy(subproc->heap_main, false /* don't re-acquire the heaps_lock */);  // no warning if destroying the main heap
    }
  }

  subproc->theap_meta = NULL;     // theap meta stats are merged during heap_destroy of the main heap

  if (!_mi_subproc_is_main(subproc)) {
    // merge stats back into the main subproc
    _mi_stats_merge_into(&mi_process_subproc_main.stats, &subproc->stats);
  }

  // remove associated arenas
  _mi_arenas_unsafe_destroy_all(subproc);

  // show stats of the main process (at process end) before releasing the heaps lock
  if (_mi_subproc_is_main(subproc)) {
    if (mi_option_is_enabled(mi_option_show_stats) || mi_option_is_enabled(mi_option_verbose)) {
      mi_subproc_stats_print_out(mi_subproc_main(), NULL, NULL);
    }
  }

  // todo: should we refcount subprocesses?
  mi_lock_done(&subproc->arena_reserve_lock);
  mi_lock_done(&subproc->heaps_lock);
  mi_lock_done(&subproc->theap_meta_lock);
  _mi_meta_free( subproc->parent, subproc, subproc->memid);
  if (_mi_subproc_is_main(subproc)) {
    // for the main subproc, also release the global page map
    _mi_page_map_unsafe_destroy();
  }
}

void mi_subproc_destroy(mi_subproc_id_t subproc_id) {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL || subproc==&mi_process_subproc_main) return;
  mi_subproc_unsafe_destroy(subproc, true /* take lock */);
}

void _mi_subprocs_unsafe_destroy_all(void) {
  mi_lock(&mi_subprocs_lock) {
    mi_subproc_t* subproc = mi_subprocs;
    while (subproc!=NULL) {
      mi_subproc_t* next = subproc->next;
      if (subproc!=&mi_process_subproc_main) {
        mi_subproc_unsafe_destroy(subproc, false /* take mi_subprocs lock */);
      }
      subproc = next;
    }
  }
  mi_subproc_unsafe_destroy(&mi_process_subproc_main, true /* take mi_subprocs lock */);
}


/* -----------------------------------------------------------
  Sub process various
----------------------------------------------------------- */

void mi_subproc_add_current_thread(mi_subproc_id_t subproc_id) {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  mi_assert_internal(subproc!=NULL);
  if (subproc==NULL) return;
  mi_assert_internal(subproc->heap_main!=NULL);
  if (subproc->heap_main==NULL) return;
  mi_theap_t* theap = _mi_theap_default();
  if (mi_theap_is_initialized(theap)) {
    if (theap->tld!=NULL && theap->tld->subproc != subproc) {
      _mi_warning_message("unable to add thread to the subprocess as it was already in another subprocess (at %p)\n", theap->tld->subproc);
    }
    return;
  }

  // initialize this thread tld & theap
  _mi_thread_init_with_heap(subproc->heap_main);
}


bool mi_subproc_visit_heaps(mi_subproc_id_t subproc_id, mi_heap_visit_fun* visitor, void* arg) {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL) return false;
  bool ok = true;
  mi_lock(&subproc->heaps_lock) {
    for (mi_heap_t* heap = subproc->heaps; heap!=NULL && ok; heap = heap->next) {
      ok = (*visitor)(heap, arg);
    }
  }
  return ok;
}


mi_subproc_t* _mi_subproc_main_init(void) {
  mi_lock_init(&mi_subprocs_lock);
  mi_memid_t memid = _mi_memid_create_static(&mi_process_subproc_main,sizeof(mi_subproc_t));
  mi_process_subproc_main.memid = memid;
  mi_subproc_init(&mi_process_subproc_main,NULL);
  return &mi_process_subproc_main;
}

void _mi_subproc_main_done(void) {
  mi_lock_done(&mi_subprocs_lock);
}


/* -----------------------------------------------------------
  Fork support

  `fork()` is only safe if no other thread is executing inside mimalloc
  at the time of the fork; all other threads must be quiesced at a safe
  point outside mimalloc (as is done for example by CPython's stop-the-world).
  The child then inherits consistent data structures with no locks held
  (as no thread was inside mimalloc) so we only need to abandon the pages
  of the threads that do not exist in the child.
----------------------------------------------------------- */

// Abandon the pages of theaps whose threads do not exist in the child.
// The theap and tld memory itself is not freed as the (dead) thread lists may be inconsistent.
// We need to take no locks as at this point only the child process runs in a single thread.
static void mi_fork_child_abandon_theaps(void) {
  mi_theap_t* const theap_default = _mi_theap_default();
  mi_tld_t* const tld_current = (mi_theap_is_initialized(theap_default) ? theap_default->tld : NULL);
  mi_theap_t* dead = NULL;
  for (mi_subproc_t* subproc = mi_subprocs; subproc != NULL; subproc = subproc->next) {
    for (mi_heap_t* heap = subproc->heaps; heap != NULL; heap = heap->next) {
      mi_theap_t* theap = heap->theaps;
      while (theap != NULL) {
        mi_theap_t* const next = theap->hnext;
        mi_tld_t* const tld = theap->tld;
        if (tld != NULL && tld != tld_current && tld->thread_id != MI_THREADID_DETACHED) {
          // remove from the heap list and push on the dead list
          if (theap->hnext != NULL) { theap->hnext->hprev = theap->hprev; }
          if (theap->hprev != NULL) { theap->hprev->hnext = theap->hnext; }
                               else { heap->theaps = theap->hnext; }
          theap->hprev = NULL;
          theap->hnext = dead;
          dead = theap;
        }
        theap = next;
      }
    }
  }
  while (dead != NULL) {
    mi_theap_t* const next = dead->hnext;
    mi_heap_t* const heap = mi_theap_heap(dead);
    dead->hnext = NULL;
    dead->tld = NULL;
    _mi_theap_abandon_owned_pages(dead);
    _mi_stats_merge_into(&heap->stats, &dead->stats);
    mi_atomic_store_ptr_release(mi_heap_t, &dead->heap, NULL);
    dead = next;
  }
}

void _mi_fork_child(void) {
  mi_fork_child_abandon_theaps();
}

// Called at process initialization.
void _mi_fork_init(void) {
  #if defined(MI_USE_PTHREADS) && !defined(__wasi__) && !defined(__EMSCRIPTEN__) && !MI_NO_PTHREAD_ATFORK
  if (pthread_atfork(NULL, NULL, &_mi_fork_child) != 0) {
    _mi_warning_message("unable to register the pthread fork handler\n");
  }
  #endif
}