/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_PROCESS_H
#define MI_INTERNAL_PROCESS_H

#include "../types.h"
#include "../prim.h"          // mi_os_mem_config_t
#include "../util/options.h"

mi_heap_t*    _mi_subproc_heap_main(mi_subproc_t* subproc);
mi_theap_t*   _mi_theap_main(void);  // current thread's backing theap, without initialization
mi_page_t*    _mi_page_empty_get(void);
void          _mi_auto_process_init(void);
void mi_cdecl _mi_auto_process_done(void) mi_attr_noexcept;
bool          _mi_preloading(void);           // true while the C runtime is not initialized yet
void          _mi_thread_done(mi_theap_t* theap);
mi_theap_t*   _mi_thread_init(void);
mi_theap_t*   _mi_thread_init_with_heap(mi_heap_t* heap);
bool          _mi_is_empty_theap(const mi_theap_t* theap);

extern mi_decl_hidden const mi_theap_t _mi_theap_empty; // read-only empty theap, initial value of the thread local default theap (in the MI_TLS_MODEL_LOCAL)

// Fork state
typedef struct mi_fork_state_s {
  size_t           epoch;            // incremented in the child at every fork
  _Atomic(size_t)  pending;          // set in the child until the dead theaps are abandoned
  bool             torn;             // set in a child that may have a torn snapshot of memory (on macOS)
  mi_heap_t*       suspended_heap;   // heap with suspended abandoned reclaim (on macOS)
  size_t           suspended_count[MI_BIN_COUNT];  // and its abandoned counts
} mi_fork_state_t;


// In secure mode, guard static process data such that guard pages fit in secure mode (twice the maximal OS page size)
#ifndef MI_SECURE_STATIC_GUARD
  #if !MI_SECURE || !MI_OS_HAS_GUARD_PAGES
  #define MI_SECURE_STATIC_GUARD    (MI_CACHE_LINE_SIZE)          // no guard pages; just a cache line
  #else
  #define MI_SECURE_STATIC_GUARD    (2*MI_OS_PROBABLE_PAGE_SIZE)  // checked at runtime
  #endif
#endif

// Output before an output function is registered (or the C runtime is initialized) is buffered (see `util/options.c`)
#ifndef MI_MAX_DELAY_OUTPUT
#define MI_MAX_DELAY_OUTPUT         (16*MI_KiB)
#endif

// Statically allocated process data. This is zero initialized and padded such that
// in secure mode it is surrounded by guard pages (see `heap/process.c:mi_process_guard_init`)
typedef struct mi_process_s {
  uint8_t                           guard_front[MI_SECURE_STATIC_GUARD];

  _Atomic(void*)                    deferred_free;          // is `mi_deferred_free_fun*` (but some platforms don't support atomic function pointers)
  _Atomic(void*)                    deferred_arg;
  _Atomic(void*)                    out_default;            // is `mi_output_fun*`
  _Atomic(void*)                    out_arg;
  _Atomic(void*)                    error_handler;          // is `mi_error_fun*`
  _Atomic(void*)                    error_arg;

  _Atomic(struct mi_page_map_s*)    page_map;               // the page map (`NULL` before initialization) (see `heap/page-map.h`)
  _Atomic(size_t)                   page_map_committed;     // committed entries in the page map

  bool                              process_is_initialized; // set to `true` in `mi_process_init`
  bool                              libc_is_initialized;    // `true` once the C runtime is initialized (and `false` again after process done)
  bool                              malloc_is_redirected;   // `true` if malloc is redirected to mimalloc (on Windows with `mimalloc-redirect.dll`)
  _Atomic(size_t)                   process_done_called;    // set once `mi_process_done` is called
  _Atomic(size_t)                   subprocs_created;       // total number of sub-processes created (used for `subproc_seq`)
  mi_atomic_guard_t                 arena_purge_guard;      // allow only one thread at a time to purge arenas

  mi_msecs_t                        clock_process_start;    // clock at process initialization (see `util/stats.c`)
  mi_msecs_t                        clock_overhead;         // measured overhead of reading the clock (subtracted in `_mi_clock_end`)
  int64_t                           clock_ticks_per_msec;   // performance counter ticks per millisecond (on Windows) (0 until first use)

  _Atomic(size_t)                   output_buf_len;         // used length of `output_buf`
  _Atomic(size_t)                   error_count;            // stop emitting errors when >= `mi_option_max_errors`
  _Atomic(size_t)                   warning_count;          // stop emitting warnings when >= `mi_option_max_warnings`

  mi_os_mem_config_t                os_mem_config;          // OS memory configuration
  _Atomic(size_t)                   os_numa_node_count;     // cached NUMA node count (0 until first use)
  _Atomic(mi_addr_t)                os_aligned_base;        // next aligned address hint (see `_mi_os_get_aligned_hint`)
  _Atomic(mi_addr_t)                os_huge_start;          // next address for huge OS pages (see `mi_os_claim_huge_pages`)

  mi_lock_t                         thread_locals_lock;     // initialized in `_mi_thread_locals_init` (see `util/threadlocal.c`)
  mi_bitmap_t*                      thread_locals_free;     // bitmap of free thread local slots (1=free, 0=in-use)
  mi_memid_t                        thread_locals_memid;    // provenance of `thread_locals_free`
  size_t                            thread_locals_version;  // version to be able to reuse slots safely

  mi_subproc_t*                     subprocs;               // list of all sub-processes
  mi_lock_t                         subprocs_lock;          // initialized in `_mi_subproc_main_init` (as a zero lock is invalid on some platforms)
  mi_fork_state_t                   fork;                   // fork state (see `heap/subproc.c`)

  mi_profiler_t*                    profile_env_profiler;   // the pprof profiler if `MIMALLOC_PROFILE` was set (see `profile/pprof.c`)
  mi_atomic_guard_t                 profile_snapshot_guard; // allow only one thread at a time to take a profile snapshot

  mi_decl_cache_align mi_subproc_t  subproc_main;           // initial subprocess
  mi_decl_cache_align mi_heap_t     heap_main;              // initial heap for the process
  mi_decl_cache_align mi_theap_t    theap_main;             // initial theap for the main thread
  mi_decl_cache_align mi_tld_t      tld_main;               // initial tld for the main thread
  mi_decl_cache_align mi_theap_t    theap_meta;             // detached theap for meta-data allocation
  mi_decl_cache_align mi_tld_t      tld_meta;               // detached tld for `theap_meta`
  mi_decl_cache_align mi_theap_t    theap_empty_wrong;      // read-only empty theap used to signal that a theap for a heap could not be allocated

  // arrays last
  mi_option_state_t                 options[_mi_option_last]; // current option values (see `util/options.c`)
  char                              output_buf[MI_MAX_DELAY_OUTPUT+1]; // delayed output buffer (see `util/options.c`)

  // end guard
  mi_decl_cache_align uint8_t       guard_end[MI_SECURE_STATIC_GUARD];
} mi_process_t;

extern mi_decl_hidden mi_process_t _mi_process;

#endif
