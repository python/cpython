/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc/prim.h"
#include "mimalloc/track.h"
#include "mimalloc/prim/os.h"
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/util/random.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/heap/page-map.h"
#include "mimalloc/profile/pprof.h"
#include "mimalloc/profile/sample-profile.h"

#include <stdlib.h>  // atexit
#include "mimalloc/heap/init.h"

// Empty page used to initialize the small free pages array
static const mi_page_t mi_page_empty = {
  #if MI_PAGE_META_IS_ALIGNED
  MI_ATOMIC_VAR_INIT(NULL),  // self
  #endif
  MI_ATOMIC_VAR_INIT(0),  // xthread_id
  NULL,                   // free
  NULL,                   // local_free
  0,                      // block_size
  {0},                    // xused
  #if MI_SIZE_SIZE < 8
  0,                      // xlast_used
  0,                      // xlast_alloc
  #endif
  0,                      // page_offset
  0,                      // capacity
  0,                      // reserved capacity
  0,                      // slice_pcommitted
  0,                      // retire_expire
  false,                  // is_zero
  MI_ATOMIC_VAR_INIT(0),  // xthread_free
  NULL,                   // theap
  NULL,                   // heap
  NULL, NULL,             // next, prev
  MI_MEMID_STATIC,        // memid
  #if (MI_PADDING || MI_ENCODE_FREELIST)
  #if MI_PAGE_KEY_COUNT==2
  { 0, 0 },               // keys
  #else
  { 0 },                  // key
  #endif
  #endif
  #if MI_QSBR
  NULL, NULL,            // qsbr_next, qsbr_prev
  false,                 // qsbr_in_queue
  mi_init_struct_zero,    // qsbr
  #endif
};

#define MI_PAGE_EMPTY() ((mi_page_t*)&mi_page_empty)

#if MI_SMALL_WSIZE_MAX == 128
#define MI_INIT_PAGES_DIRECT(p)  MI_INIT128(p)
#elif MI_SMALL_WSIZE_MAX == 256
#define MI_INIT_PAGES_DIRECT(p)  MI_INIT128(p), MI_INIT128(p)
#else
#error define initializer for direct pages
#endif

#if (MI_PADDING>0) && (MI_SIZE_SIZE >= 8)
#define MI_SMALL_PAGES_EMPTY  { MI_INIT_PAGES_DIRECT(MI_PAGE_EMPTY), MI_PAGE_EMPTY(), MI_PAGE_EMPTY() }
#elif (MI_PADDING>0)
#define MI_SMALL_PAGES_EMPTY  { MI_INIT_PAGES_DIRECT(MI_PAGE_EMPTY), MI_PAGE_EMPTY(), MI_PAGE_EMPTY(), MI_PAGE_EMPTY() }
#else
#define MI_SMALL_PAGES_EMPTY  { MI_INIT_PAGES_DIRECT(MI_PAGE_EMPTY), MI_PAGE_EMPTY() }
#endif


// Empty page queues for every bin
#define QNULL(sz)  { NULL, NULL, 0, (sz)*sizeof(uintptr_t), 0 }
#define MI_PAGE_QUEUES_EMPTY \
  { QNULL(1), \
    QNULL(     1), QNULL(     2), QNULL(     3), QNULL(     4), QNULL(     5), QNULL(     6), QNULL(     7), QNULL(     8), /* 8 */ \
    QNULL(    10), QNULL(    12), QNULL(    14), QNULL(    16), QNULL(    20), QNULL(    24), QNULL(    28), QNULL(    32), /* 16 */ \
    QNULL(    40), QNULL(    48), QNULL(    56), QNULL(    64), QNULL(    80), QNULL(    96), QNULL(   112), QNULL(   128), /* 24 */ \
    QNULL(   160), QNULL(   192), QNULL(   224), QNULL(   256), QNULL(   320), QNULL(   384), QNULL(   448), QNULL(   512), /* 32 */ \
    QNULL(   640), QNULL(   768), QNULL(   896), QNULL(  1024), QNULL(  1280), QNULL(  1536), QNULL(  1792), QNULL(  2048), /* 40 */ \
    QNULL(  2560), QNULL(  3072), QNULL(  3584), QNULL(  4096), QNULL(  5120), QNULL(  6144), QNULL(  7168), QNULL(  8192), /* 48 */ \
    QNULL( 10240), QNULL( 12288), QNULL( 14336), QNULL( 16384), QNULL( 20480), QNULL( 24576), QNULL( 28672), QNULL( 32768), /* 56 */ \
    QNULL( 40960), QNULL( 49152), QNULL( 57344), QNULL( 65536), QNULL( 81920), QNULL( 98304), QNULL(114688), QNULL(131072), /* 64 */ \
    QNULL(163840), QNULL(196608), QNULL(229376), QNULL(262144), QNULL(327680), QNULL(393216), QNULL(458752), QNULL(524288), /* 72 */ \
    QNULL(MI_LARGE_MAX_OBJ_WSIZE + 1  /* 655360, Huge queue */), \
    QNULL(MI_LARGE_MAX_OBJ_WSIZE + 2) /* Full queue */ }

#define MI_STAT_COUNT_NULL()  {0,0,0}

// Empty statistics
#define MI_STAT_COUNT(stat)     {0,0,0},
#define MI_STAT_COUNTER(stat)   {0},

#define MI_STATS_FIELDS_NULL  \
  MI_STAT_FIELDS()                    /* regular stat fields */ \
  { MI_INIT4(MI_STAT_COUNT_NULL) },   /* stat reserved */ \
  { { 0 }, { 0 }, { 0 }, { 0 } },     /* stat counter reserved */ \
  { MI_INIT74(MI_STAT_COUNT_NULL) },  /* malloc_bins */ \
  { MI_INIT74(MI_STAT_COUNT_NULL) },  /* page bins   */ \
  { MI_INIT5(MI_STAT_COUNT_NULL) }    /* chunk bins  */

#define MI_STATS_NULL \
  { sizeof(mi_stats_t), MI_STAT_VERSION, MI_STATS_FIELDS_NULL }

// --------------------------------------------------------
// Statically allocate an empty theap as the initial
// thread local value for the default theap,
// and statically allocate the backing theap for the main
// thread so it can function without doing any allocation
// itself (as accessing a thread local for the first time
// may lead to allocation itself on some platforms)
// --------------------------------------------------------

static mi_decl_cache_align mi_tld_t mi_tld_detached = {
  MI_THREADID_DETACHED,   // thread_id
  0,                      // thread_seq
  0,                      // default numa node
  NULL,                   // subproc
  NULL,                   // theaps list
  MI_LOCK_INITIALIZER,    // theaps lock
  false,                  // recurse
  false,                  // in_profiler
  false,                  // is_in_threadpool
  MI_MEMID_STATIC         // memid
};

#ifdef __cplusplus
extern // fix warning in C++ for const variables with internal linkage
#endif
mi_decl_hidden mi_decl_cache_align const mi_theap_t _mi_theap_empty = {
  MI_SMALL_PAGES_EMPTY,   // direct small pages
  &mi_tld_detached,       // tld
  MI_ATOMIC_VAR_INIT(NULL), // heap
  MI_ATOMIC_VAR_INIT(NULL), // subproc
  MI_ATOMIC_VAR_INIT(1),  // refcount
  0,                      // full page retain
  false,                  // allow reclaim
  true,                   // allow abandon
  true,                   // is_detached
  true,                   // profile_never: this static empty theap must never be sampled
  ~MI_ZU(0),              // sample countdown: "-1" (with a sample rate of 0, so we won't write to the empty theap with MI_SAMPLE==2)
  0, 0,                   // sample rate, requested
  0, 0,                   // profile rate, countdown
  0, 0, 0, 0,             // guarded rate, countdown, min, max
  0, 0,                   // profiler_reserved1, profiler_reserved2
  0,                      // heartbeat
  { {0}, {0}, 0, true },  // random
  0, 0,                   // page count, OS page count
  MI_BIN_FULL, 0,         // page retired min/max
  0,                      // pages_full_size
  0, 0,                   // generic count
  NULL, NULL,             // tnext, tprev
  NULL, NULL,             // hnext, hprev
  MI_PAGE_QUEUES_EMPTY,
  MI_MEMID_STATIC,
  MI_STATS_NULL,          // stats
  #if MI_QSBR
  NULL, NULL,            // qsbr_first, qsbr_last
  #endif
};

#undef MI_STAT_COUNT
#undef MI_STAT_COUNTER


// pre-allocate the process heap, and meta-data theap
static mi_decl_cache_align mi_heap_t    mi_process_heap_main  = mi_init_struct_zero;
static mi_decl_cache_align mi_theap_t   mi_process_theap_meta = mi_init_struct_zero;

// pre-allocate the initial tld and theap for the main thread (this is not strictly needed but nice for stats)
static mi_decl_cache_align mi_tld_t     mi_process_tld_main   = mi_init_struct_zero;
static mi_decl_cache_align mi_theap_t   mi_process_theap_main = mi_init_struct_zero;

mi_decl_hidden mi_decl_cache_align mi_theap_t _mi_theap_empty_wrong = mi_init_struct_zero;  // used for error paths
mi_decl_hidden bool _mi_process_is_initialized = false;  // set to `true` in `mi_process_init`.


mi_page_t* _mi_page_empty_get(void) {
  return (mi_page_t*)&mi_page_empty;
}

mi_decl_cold mi_decl_noinline mi_theap_t* _mi_theap_empty_get(void) {
  return (mi_theap_t*)&_mi_theap_empty;
}

bool _mi_is_empty_theap(const mi_theap_t* theap) {
  return (theap == &_mi_theap_empty);
}

/* -----------------------------------------------------------
  Initialization
  Note: on some platforms lock_init or just a thread local access
  can cause allocation and induce recursion during initialization.
----------------------------------------------------------- */

static mi_tld_t* mi_tld_init(mi_tld_t* tld, size_t tseq, mi_subproc_t* subproc);

// Initialize main heap
static void mi_heap_main_init_once(void) {
  mi_memid_t memid_static = _mi_memid_create(MI_MEM_STATIC);
  mi_memcpy(&_mi_theap_empty_wrong,&_mi_theap_empty,sizeof(_mi_theap_empty_wrong));

  // initialize the main subprocess
  mi_subproc_t* subproc_main = _mi_subproc_main_init();

  // detached tld for mi_theap_empty (and theap_meta)
  mi_tld_detached.memid = memid_static;
  mi_tld_init(&mi_tld_detached, 0, subproc_main);

  // main process heap
  mi_process_heap_main.memid = memid_static;
  mi_atomic_store_ptr_release(mi_heap_t,&subproc_main->heap_main,&mi_process_heap_main);
  _mi_heap_init(&mi_process_heap_main,mi_thread_local_key_fast,subproc_main,0,false);

  // detached theap for allocating meta-data (we can allocate on this without having an initialized thread)
  mi_process_theap_meta.memid = memid_static;
  _mi_theap_init(&mi_process_theap_meta,&mi_process_heap_main,&mi_tld_detached);
  mi_process_theap_meta.allow_page_abandon = false;  // for security, don't share with other threads
  mi_process_theap_meta.page_full_retain = 2;
  mi_process_theap_meta.sample_rate = 0; // no sampling for meta data
  mi_process_theap_meta.sample_countdown = 0;
  mi_theap_profile_never(&mi_process_theap_meta);  // permanently exclude from profiling, see `mi_theap_profile_never`
  subproc_main->theap_meta = &mi_process_theap_meta;

  // mi_heap_theap_set(&mi_process_heap_main,&mi_process_theap_main); // set in `mi_thread_init(_theap_default)`
}

static void mi_heap_main_init(void) {
  mi_atomic_do_once {
    mi_heap_main_init_once();
  }
}

mi_heap_t* _mi_subproc_heap_main(mi_subproc_t* subproc) {
  mi_heap_t* heap = mi_atomic_load_ptr_acquire(mi_heap_t,&subproc->heap_main);
  if mi_likely(heap!=NULL) {
    return heap;
  }
  else if (_mi_subproc_is_main(subproc)) {
    mi_heap_main_init();
    mi_assert_internal(mi_atomic_load_ptr_acquire(mi_heap_t,&subproc->heap_main) != NULL);
    return mi_atomic_load_ptr_acquire(mi_heap_t,&subproc->heap_main);
  }
  else {
    mi_assert_internal(false);
    return &mi_process_heap_main;
  }
}

/* -----------------------------------------------------------
  Thread local data
----------------------------------------------------------- */

mi_theap_t* _mi_theap_main(void) {
  // All subprocess main heaps share this slot; do not follow a possibly deleted default theap.
  const mi_thread_local_t key = mi_process_heap_main.theap;
  return (key==0 ? NULL : (mi_theap_t*)_mi_thread_local_get(key));
}

static mi_tld_t* mi_tld_init(mi_tld_t* tld, size_t tseq, mi_subproc_t* subproc) {
  tld->subproc = subproc;
  tld->theaps = NULL;
  mi_lock_init(&tld->theaps_lock);
  if (tld->thread_id == MI_THREADID_DETACHED) {
    tld->numa_node = -1;
  }
  else {
    tld->numa_node = _mi_os_numa_node();
    tld->thread_id = _mi_prim_thread_id();
    tld->is_in_threadpool = _mi_prim_thread_is_in_threadpool();
    tld->thread_seq = tseq;
    mi_atomic_increment_relaxed(&tld->subproc->thread_count);
  }
  return tld;
}

// Allocate fresh tld
static mi_tld_t* mi_tld_create(mi_subproc_t* subproc) {
  mi_assert_internal(subproc->theap_meta != NULL); // should be initialized on the main thread before other threads allocate
  const size_t tseq = mi_atomic_increment_relaxed(&subproc->thread_total_count);

  mi_memid_t memid;
  mi_tld_t* tld;
  if (_mi_subproc_is_main(subproc) && tseq==0 /* first tld */) {
    tld = &mi_process_tld_main;
    memid = _mi_memid_create_static(tld,sizeof(*tld));
  }
  else {
    tld  = (mi_tld_t*)_mi_meta_zalloc(subproc, sizeof(mi_tld_t), &memid);
  }
  if (tld==NULL) {
    _mi_error_message(ENOMEM, "unable to allocate memory for thread local data\n");
    return NULL;
  }
  tld->memid = memid;
  return mi_tld_init(tld,tseq,subproc);
}

mi_decl_noinline static void mi_tld_free(mi_tld_t* tld) {
  if (tld==NULL) return;
  mi_atomic_decrement_relaxed(&tld->subproc->thread_count);
  tld->thread_id = (mi_threadid_t)(~0);          // it is best to set an invalid tid for tld_main as sometimes the same thread-id
                                                 // is reused by the OS after a thread has terminated. (see issue #1287)
  mi_lock_done(&tld->theaps_lock);
  _mi_meta_free(tld->subproc, tld, tld->memid);  // note: safe for static tld
}


/* -----------------------------------------------------------
  Thread Init
----------------------------------------------------------- */

#if MI_DEBUG || defined(MI_TLS_RECURSE_GUARD)
static mi_theap_t* mi_heap_check_for_existing_theap(mi_heap_t* heap) {
  const mi_threadid_t tid = _mi_thread_id();
  mi_theap_t* thread_theap = NULL;
  mi_lock(&heap->theaps_lock) {
    for(mi_theap_t* theap = heap->theaps; theap != NULL; theap = theap->hnext ) {
      if (theap->tld->thread_id == tid) {
        thread_theap = theap;
        break;
      }
    }
  }
  return thread_theap;
}
#endif

// Initialize thread
mi_theap_t* _mi_thread_init_with_heap(mi_heap_t* heap_main)
{
  // ensure our process has started already
  mi_process_init();

  // if the theap_default is already set we have already initialized
  mi_theap_t* theap = _mi_theap_default();
  if (mi_theap_is_initialized(theap)) return theap;

  // initialize the default theap
  // note: we cannot access thread-locals yet as that can cause (recursive) allocation
  // (on macOS <= 14 for example where the loader allocates thread-local data on demand).
  if (heap_main==NULL) {
    heap_main = mi_heap_main();
    mi_assert_internal(heap_main == &mi_process_heap_main);
  }
  mi_assert_internal(heap_main!=NULL);

  #if MI_DEBUG || defined(MI_TLS_RECURSE_GUARD)
  theap = mi_heap_check_for_existing_theap(heap_main);  // recursion check
  #if !defined(MI_TLS_RECURSE_GUARD)
  mi_assert_internal(theap==NULL);
  #endif
  #else
  theap = NULL;
  #endif

  if (theap==NULL) {
    // allocated the tld
    mi_tld_t* tld = mi_tld_create(heap_main->subproc);
    if (tld==NULL) return NULL;    // out-of-memory on tld allocation
    // allocate and initialize the theap for the main heap
    if (tld==&mi_process_tld_main) {
      theap = &mi_process_theap_main;          // initial theap is pre-allocated
      theap->memid = _mi_memid_create_static(theap,sizeof(*theap));
    }
    else {
      theap = _mi_theap_alloc(heap_main,tld);  // otherwise meta allocate
      if (theap==NULL) { mi_tld_free(tld); return NULL; } // out-of-memory on theap allocation
    }
    _mi_theap_init(theap,heap_main,tld);
  }

  // now initialize the thread
  _mi_theap_default_set(theap);
  // and only then set the heap_theap field as that accesses thread locals
  _mi_heap_theap_set(heap_main, theap);  // todo: can fail!

  mi_assert_internal(mi_theap_is_initialized(theap));
  mi_theap_t* const heap_theap = (heap_main==NULL ? NULL : (mi_theap_t*)_mi_thread_local_get(heap_main->theap));
  mi_assert_internal(heap_main==NULL || heap_theap == theap); MI_UNUSED_RELEASE(heap_theap);

  mi_subproc_stat_increase(mi_theap_subproc(theap), threads, 1);  // or theap stats and wait for merge?
  // _mi_verbose_message("thread init: 0x%zx\n", _mi_thread_id());
  return theap;
}

mi_theap_t* _mi_thread_init(void) {
  return _mi_thread_init_with_heap(NULL);
}

void mi_decl_noinline mi_thread_init(void) mi_attr_noexcept {
  _mi_thread_init();
}



/* -----------------------------------------------------------
  Theaps done
----------------------------------------------------------- */

// Free the thread local theaps
static void mi_thread_theaps_done(mi_tld_t* tld)
{
  // abandon the pages of all theaps in this thread
  mi_lock(&tld->theaps_lock) {
    mi_theap_t* theap = tld->theaps;
    while (theap != NULL) {
      mi_theap_t* next = theap->tnext;
      // never destroy theaps; if a dll is linked statically with mimalloc,
      // there may still be delete/free calls after the mi_fls_done is called. Issue #207
      mi_heap_t* const heap = mi_theap_heap(theap);
      mi_lock(&heap->theaps_lock) { // keep page queues stable during a heap walk
        _mi_theap_collect_abandon(theap);
      }
      mi_assert_internal(theap->page_count==0);
      theap = next;
    }
  }

  // reset the thread local theaps
  // note: do this after abandon as page->heap may be NULL and mi_heap_main should return the heap
  // belonging to the right subprocess
  _mi_theap_default_set((mi_theap_t*)&_mi_theap_empty);
  _mi_theap_cached_set((mi_theap_t*)&_mi_theap_empty);

  // We might run concurrently with a `mi_heap_free_theaps` and we need to ensure we free theaps atomically.
  // we first detach our theaps list from any heaps
  _mi_tld_detach_theaps(tld);

  // no heaps point to our theaps anymore, free them
  mi_lock(&tld->theaps_lock) { // paranoia
    mi_theap_t* theap = tld->theaps;
    tld->theaps = NULL;
    while (theap != NULL) {
      mi_theap_t* next = theap->tnext;
      mi_assert_internal(theap->page_count==0);
      mi_assert_internal(mi_theap_heap_peek(theap)==NULL);
      theap->tld = NULL;
      theap->tnext = NULL;
      theap->tprev = NULL;
      mi_assert_internal(mi_atomic_load_relaxed(&theap->refcount) == 1); // as the cached entry is set to empty
      _mi_theap_decref(theap);
      theap = next;
    }
  }

  mi_assert(_mi_theap_default()==(mi_theap_t*)&_mi_theap_empty); // careful to not re-initialize the default theap during theap_delete
  mi_assert(!mi_theap_is_initialized(_mi_theap_default()));
}


// --------------------------------------------------------
// Try to run `mi_thread_done()` automatically so any memory
// owned by the thread but not yet released can be abandoned
// and re-owned by another thread.
//
// 1. windows dynamic library:
//     call from DllMain on DLL_THREAD_DETACH
// 2. windows static library:
//     use special linker section to call a destructor when the thread is done
// 3. unix, pthreads:
//     use a pthread key to call a destructor when a pthread is done
//
// In the last two cases we also need to call `mi_process_init`
// to set up the thread local keys.
// --------------------------------------------------------

// Set up hooks so `mi_thread_done` is called automatically
static void mi_process_setup_auto_thread_done(void) {
  mi_atomic_do_once {
    _mi_prim_thread_init_auto_done();
    mi_theap_t* theap = _mi_theap_default();
    mi_assert_internal(mi_theap_is_initialized(theap));
    if (mi_theap_is_initialized(theap)) {
      _mi_theap_default_set(theap);
    }
  }
}

void mi_thread_done(void) mi_attr_noexcept {
  _mi_thread_done(NULL);
}

void _mi_thread_done(mi_theap_t* _theap_main)
{
  // NULL can be passed on some platforms
  if (_theap_main==NULL) {
    _theap_main = _mi_theap_default();
  }

  // prevent re-entrancy through theap_done/theap_set_default_direct (issue #699)
  if (!mi_theap_is_initialized(_theap_main)) {
    return;
  }

  // get the current tld
  mi_tld_t* const tld = _theap_main->tld;

  // release dynamic thread_local's
  _mi_thread_locals_thread_done();

  // adjust stats
  mi_subproc_stat_decrease(tld->subproc, threads, 1);  // todo: or `_theap_main->heap`?

  // check thread-id as on Windows shutdown with FLS the main (exit) thread may call this on thread-local theaps...
  if (tld->thread_id != _mi_prim_thread_id()) return;

  // delete the thread local theaps
  mi_thread_theaps_done(tld);

  // free thread local data
  mi_tld_free(tld);
}

void mi_thread_set_in_threadpool(void) mi_attr_noexcept {
  mi_theap_t* theap = mi_theap_get_default();
  theap->tld->is_in_threadpool = true;
}


// --------------------------------------------------------
// Process init and done
// --------------------------------------------------------

static bool os_preloading = true;    // true until this module is initialized

// Returns true if this module has not been initialized; Don't use C runtime routines until it returns false.
bool mi_decl_noinline _mi_preloading(void) {
  return os_preloading;
}

// Returns true if mimalloc was redirected
mi_decl_nodiscard bool mi_is_redirected(void) mi_attr_noexcept {
  return _mi_is_redirected();
}

// Called once by the process loader from `src/prim/prim.c` before `main` is called.
void _mi_auto_process_init(void) {
  os_preloading = false;

  mi_process_init();

  _mi_options_post_init();  // now we can print to stderr
  if (_mi_is_redirected()) _mi_verbose_message("malloc is redirected.\n");

  // show message from the redirector (if present)
  const char* msg = NULL;
  _mi_allocator_init(&msg);
  if (msg != NULL && (mi_option_is_enabled(mi_option_verbose) || mi_option_is_enabled(mi_option_show_errors))) {
    _mi_fputs(NULL,NULL,NULL,msg);
  }

  // reseed random
  mi_theap_t* theap = _mi_theap_default();
  if (theap != NULL) {
    _mi_random_reinit_if_weak(&theap->random);
    mi_subproc_t* subproc = mi_theap_subproc(theap);
    if (subproc->theap_meta != NULL) {
      mi_lock(&subproc->theap_meta_lock) {
        _mi_random_reinit_if_weak(&subproc->theap_meta->random);
      }
    }
  }

  // check if we should start the pprof profiler
  _mi_pprof_profiler_init();
}


// Initialize the process; called by thread_init, the process loader, or an initial allocation (perhaps by the loader or a system library)
static void mi_process_init_once(void) {
  #if defined(__CYGWIN__)   // we need to kickstart the cygwin runtime
  __mi_thread_id_helper = NULL;
  #endif
  _mi_verbose_message("process init: 0x%zx\n", _mi_thread_id());

  _mi_detect_cpu_features();
  _mi_options_init();        // read environment (if possible)
  _mi_stats_init();          // start timer
  _mi_os_init();             // primitive dependent

  mi_heap_main_init();       // before page_map_init so stats are working
  _mi_page_map_init();       // todo: this could fail.. should we abort in that case?
  mi_thread_init();

  // the following can potentially allocate (on freeBSD for pthread keys)
  _mi_tls_slots_init();      // pthread key create
  _mi_thread_locals_init();  // pthread key create
  _mi_process_is_initialized = true;    // before `mi_process_setup_auto_thread_done` so `_mi_theap_default` returns the current theap with `MI_TLS_RECURSE_GUARD`
  mi_process_setup_auto_thread_done();  // after the above mi_thread_init so it can add the current theap
  _mi_fork_init();                      // register fork handlers (can allocate)

  #if defined(_WIN32) && defined(MI_WIN_INIT_USE_FLS)
  // On windows, when building as a static lib the FLS cleanup happens to early for the main thread.
  // To avoid this, set the FLS value for the main thread to NULL so the fls cleanup
  // will not call _mi_thread_done on the (still executing) main thread. See issue #508.
  _mi_prim_thread_associate_default_theap(NULL);
  #endif

  // mi_stats_reset();  // only call stat reset *after* thread init (or the theap tld == NULL)
  mi_track_init();
  if (mi_option_is_enabled(mi_option_reserve_huge_os_pages)) {
    size_t pages = mi_option_get_clamp(mi_option_reserve_huge_os_pages, 0, 128*1024);
    int reserve_at  = (int)mi_option_get_clamp(mi_option_reserve_huge_os_pages_at, -1, INT_MAX);
    if (reserve_at != -1) {
      mi_reserve_huge_os_pages_at(pages, reserve_at, pages*500);
    } else {
      mi_reserve_huge_os_pages_interleave(pages, 0, pages*500);
    }
  }
  if (mi_option_is_enabled(mi_option_reserve_os_memory)) {
    long ksize = mi_option_get(mi_option_reserve_os_memory);
    if (ksize > 0) {
      mi_reserve_os_memory((size_t)ksize*MI_KiB, true, true);
    }
  }
}

// Initialize the process; called by thread_init or the process loader
void mi_process_init(void) mi_attr_noexcept {
  mi_atomic_do_once {
    mi_process_init_once();
  }
}

// Called once on process exit (when the process/dll is exiting gracefully)
static void mi_process_unwind_once(void)
{
  // stop the profiler
  _mi_pprof_profiler_done();

  // decref any cached theap
  _mi_theap_cached_set(_mi_theap_empty_get());

  // release any thread specific resources and ensure _mi_thread_done is called on all but the main thread
  _mi_prim_thread_done_auto_done();

#ifndef MI_SKIP_COLLECT_ON_EXIT
#if (MI_DEBUG || !defined(MI_SHARED_LIB))
  // free all memory if possible on process exit. This is not needed for a stand-alone process
  // but should be done if mimalloc is statically linked into another shared library which
  // is repeatedly loaded/unloaded, see issue #281.
  mi_theap_collect(_mi_theap_default(), true /* force */);
#endif
#endif

  // done with tracking tools
  mi_track_done();

  // Forcefully release all retained memory; this can be dangerous in general if overriding regular malloc/free
  // since after process_done there might still be other code running that calls `free` (like at_exit routines,
  // or C-runtime termination code.
  mi_subproc_t* subproc_main = _mi_subproc_main();
  if (mi_option_is_enabled(mi_option_destroy_on_exit)) {
    _mi_subprocs_unsafe_destroy_all(); // destroys all mi_subprocs, arenas, thread locals, and the page_map!
  }
  else {
    // free dynamic thread locals (if used at all)
    _mi_thread_locals_thread_done();
    _mi_thread_locals_done();
    if (subproc_main->heap_main != NULL) {
      if (mi_option_is_enabled(mi_option_show_stats) || mi_option_is_enabled(mi_option_verbose)) {
        mi_theap_collect(subproc_main->theap_meta, false /* force */); // update stats of all pages in theap_meta
        _mi_theap_merge_stats(subproc_main->theap_meta);
        _mi_theap_merge_stats(_mi_theap_default());  // _mi_thread_locals_done can free
        mi_heap_stats_merge_to_subproc(subproc_main->heap_main);
        mi_subproc_stats_print_out(mi_subproc_main(), NULL, NULL); // note: can try to access (the now freed) thread_locals in mi_heap_theap_peek
      }
    }
  }

  _mi_tls_slots_done();
}

// Called when the process is done
static void mi_process_done_once(void) {
  // only shutdown if we were initialized
  if (!_mi_process_is_initialized) return;
  // ensure we are called once
  static bool process_done = false;
  if (process_done) return;
  process_done = true;

  const size_t thread_count = mi_atomic_load_acquire(&_mi_subproc_main()->thread_count);
  if (thread_count == 1 || !_mi_prim_process_is_killed()) {  // can we safely take locks?
    mi_process_unwind_once();
    _mi_verbose_message("process done: 0x%zx\n", mi_process_tld_main.thread_id);
  }

  _mi_subproc_main_done();
  _mi_allocator_done();
  os_preloading = true; // don't call the C runtime anymore
}


// Call when the process is done (cdecl as it is used with `at_exit` on some platforms)
void mi_cdecl mi_process_done(void) mi_attr_noexcept {
  // we don't use `mi_atomic_do_once` here to avoid deadlocks if threads are killed due to a forced exit (issue #1377)
  static _Atomic(size_t) mi_process_is_done;
  size_t expected = 0;
  if (mi_atomic_cas_strong_acq_rel(&mi_process_is_done, &expected, 1)) {
    mi_process_done_once();
  }
}

// Called automatically when the process is done (cdecl as it is used with `at_exit` on some platforms)
void mi_cdecl _mi_auto_process_done(void) mi_attr_noexcept {
  if (_mi_option_get_fast(mi_option_destroy_on_exit)>=2) return;  // allow disabling auto process done
  mi_process_done();
}
