/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc/prim.h"  // mi_prim_out_stderr
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/util/options.h"

#include <stdio.h>      // stdin/stdout
#include <stdlib.h>     // abort

static long mi_max_error_count   = 16; // stop outputting errors after this (use < 0 for no limit)
static long mi_max_warning_count = 16; // stop outputting warnings after this (use < 0 for no limit)

static void mi_add_stderr_output(void);

int mi_version(void) {
  return MI_MALLOC_VERSION;
}


// --------------------------------------------------------
// Options
// These can be accessed by multiple threads and may be
// concurrently initialized, but an initializing data race
// is ok since they resolve to the same value.
// --------------------------------------------------------


#define MI_OPTION(opt)                  mi_option_##opt, #opt, NULL
#define MI_OPTION_LEGACY(opt,legacy)    mi_option_##opt, #opt, #legacy

// Some options can be set at build time for statically linked libraries
// (use `-DMI_EXTRA_CPPDEFS="opt1=val1;opt2=val2"`)
//
// This is useful if we cannot pass them as environment variables
// (and setting them programmatically would be too late)

#ifndef MI_DEFAULT_VERBOSE
#define MI_DEFAULT_VERBOSE 0
#endif

#ifndef MI_DEFAULT_ARENA_EAGER_COMMIT
#define MI_DEFAULT_ARENA_EAGER_COMMIT 2
#endif

// in KiB
#ifndef MI_DEFAULT_ARENA_RESERVE
 #if (MI_SIZE_SIZE>4)
  #define MI_DEFAULT_ARENA_RESERVE 1024L*1024L
 #else
  #define MI_DEFAULT_ARENA_RESERVE 128L*1024L
 #endif
#endif

#ifndef MI_DEFAULT_ARENA_MAX_OBJECT_SIZE
#define MI_DEFAULT_ARENA_MAX_OBJECT_SIZE   ((MI_SIZE_BITS * MI_ARENA_MAX_CHUNK_OBJ_SIZE)/MI_KiB)  /* 2 GiB (or 256 MiB on 32-bit), larger than this is alloc'd by the OS */
#endif

#ifndef MI_DEFAULT_DISALLOW_ARENA_ALLOC
#define MI_DEFAULT_DISALLOW_ARENA_ALLOC 0
#endif

#ifndef MI_DEFAULT_ALLOW_LARGE_OS_PAGES
#define MI_DEFAULT_ALLOW_LARGE_OS_PAGES 0
#endif

#ifndef MI_DEFAULT_RESERVE_HUGE_OS_PAGES
#define MI_DEFAULT_RESERVE_HUGE_OS_PAGES 0
#endif

#ifndef MI_DEFAULT_RESERVE_OS_MEMORY
#define MI_DEFAULT_RESERVE_OS_MEMORY 0
#endif

#ifndef MI_DEFAULT_GUARDED_SAMPLE_RATE
#if MI_GUARDED && MI_DEBUG
#define MI_DEFAULT_GUARDED_SAMPLE_RATE  0  /* MI_MiB */
#else
#define MI_DEFAULT_GUARDED_SAMPLE_RATE  0
#endif
#endif

#ifndef MI_DEFAULT_PAGEMAP_COMMIT
#if defined(__APPLE__) && MI_PAGE_MAP_FLAT  // when overloading malloc, we still get mixed pointers sometimes on macOS; this avoids a bad access
#define MI_DEFAULT_PAGEMAP_COMMIT 1
#else
#define MI_DEFAULT_PAGEMAP_COMMIT 0
#endif
#endif

#ifndef MI_DEFAULT_PAGE_MAX_RECLAIM
#define MI_DEFAULT_PAGE_MAX_RECLAIM  (-1)               // unlimited
#endif

#ifndef MI_DEFAULT_PAGE_FULL_RETAIN
#define MI_DEFAULT_PAGE_FULL_RETAIN  2
#endif

#ifndef MI_DEFAULT_PAGE_CROSS_THREAD_MAX_RECLAIM
#define MI_DEFAULT_PAGE_CROSS_THREAD_MAX_RECLAIM  32
#endif

#ifndef MI_DEFAULT_ALLOW_THP
#if defined(__ANDROID__)
#define MI_DEFAULT_ALLOW_THP  0
#else
// #define MI_DEFAULT_ALLOW_THP  1    // allow THP but purging may split up THP pages
#define MI_DEFAULT_ALLOW_THP  2       // allow THP and set the minimal purge size to 2MiB to avoid breaking them up
#endif
#endif

#ifndef MI_DEFAULT_STATS_MERGE_ON_COLLECT
#define MI_DEFAULT_STATS_MERGE_ON_COLLECT  1
#endif

#ifndef MI_DEFAULT_STATS_MERGE_THRESHOLD
#define MI_DEFAULT_STATS_MERGE_THRESHOLD   (512*1024)  // 512 KiB
#endif

#ifndef MI_DEFAULT_ARENA_PURGE_IMMEDIATE_SIZE
#define MI_DEFAULT_ARENA_PURGE_IMMEDIATE_SIZE  ((MI_SIZE_SIZE * MI_LARGE_PAGE_SIZE) / MI_KiB)  // 32 MiB (in KiB)
#endif

// Static options
static mi_option_desc_t mi_options[_mi_option_last] =
{
  // stable options
#if MI_DEBUG || defined(MI_SHOW_ERRORS)
  { 1, MI_OPTION_UNINIT, MI_OPTION(show_errors) },
#else
  { 0, MI_OPTION_UNINIT, MI_OPTION(show_errors) },
#endif
  { 0, MI_OPTION_UNINIT, MI_OPTION(show_stats) },
  { MI_DEFAULT_VERBOSE, MI_OPTION_UNINIT, MI_OPTION(verbose) },

  // some of the following options are experimental and not all combinations are allowed.
  { 1, MI_OPTION_UNINIT, MI_OPTION(deprecated_eager_commit) },
  { MI_DEFAULT_ARENA_EAGER_COMMIT,
       MI_OPTION_UNINIT, MI_OPTION_LEGACY(arena_eager_commit,eager_region_commit) }, // eager commit arena's? 2 is used to enable this only on an OS that has overcommit (i.e. linux)
  { 1, MI_OPTION_UNINIT, MI_OPTION_LEGACY(purge_decommits,reset_decommits) },        // purge decommits memory (instead of reset) (note: on linux this uses MADV_DONTNEED for decommit)
  { MI_DEFAULT_ALLOW_LARGE_OS_PAGES,
       MI_OPTION_UNINIT, MI_OPTION_LEGACY(allow_large_os_pages,large_os_pages) },    // use large OS pages, use only with eager commit to prevent fragmentation of VMA's
  { MI_DEFAULT_RESERVE_HUGE_OS_PAGES,
       MI_OPTION_UNINIT, MI_OPTION(reserve_huge_os_pages) },      // per 1GiB huge pages
  {-1, MI_OPTION_UNINIT, MI_OPTION(reserve_huge_os_pages_at) },   // reserve huge pages at node N
  { MI_DEFAULT_RESERVE_OS_MEMORY,
       MI_OPTION_UNINIT, MI_OPTION(reserve_os_memory)     },      // reserve N KiB OS memory in advance (use `option_get_size`)
  { 0, MI_OPTION_UNINIT, MI_OPTION(deprecated_segment_cache) },   // cache N segments per thread
  { 0, MI_OPTION_UNINIT, MI_OPTION(deprecated_page_reset) },      // reset page memory on free
  { 0, MI_OPTION_UNINIT, MI_OPTION(deprecated_abandoned_page_purge) },
  { 0, MI_OPTION_UNINIT, MI_OPTION(deprecated_segment_reset) },   // reset segment memory on free (needs eager commit)
  { 1, MI_OPTION_UNINIT, MI_OPTION(deprecated_eager_commit_delay) },
  { 1000,MI_OPTION_UNINIT, MI_OPTION_LEGACY(purge_delay,reset_delay) },  // purge delay in milli-seconds
  { 0,   MI_OPTION_UNINIT, MI_OPTION(use_numa_nodes) },           // 0 = use available numa nodes, otherwise use at most N nodes.
  { 0,   MI_OPTION_UNINIT, MI_OPTION_LEGACY(disallow_os_alloc,limit_os_alloc) },           // 1 = do not use OS memory for allocation (but only reserved arenas)
  { 100, MI_OPTION_UNINIT, MI_OPTION(os_tag) },                   // only apple specific for now but might serve more or less related purpose
  { 32,  MI_OPTION_UNINIT, MI_OPTION(max_errors) },               // maximum errors that are output
  { 32,  MI_OPTION_UNINIT, MI_OPTION(max_warnings) },             // maximum warnings that are output
  { 10,  MI_OPTION_UNINIT, MI_OPTION(deprecated_max_segment_reclaim)},       // max. percentage of the abandoned segments to be reclaimed per try.
  { 0,   MI_OPTION_UNINIT, MI_OPTION(destroy_on_exit)},           // release all OS memory on process exit; careful with dangling pointer or after-exit frees!
  { MI_DEFAULT_ARENA_RESERVE, MI_OPTION_UNINIT, MI_OPTION(arena_reserve) }, // reserve memory N KiB at a time (=1GiB) (use `option_get_size`)
  { 4,   MI_OPTION_UNINIT, MI_OPTION(arena_purge_mult) },         // purge delay multiplier for arena's
  { 1,   MI_OPTION_UNINIT, MI_OPTION_LEGACY(deprecated_purge_extend_delay, decommit_extend_delay) },
  { MI_DEFAULT_DISALLOW_ARENA_ALLOC,   MI_OPTION_UNINIT, MI_OPTION(disallow_arena_alloc) }, // 1 = do not use arena's for allocation (except if using specific arena id's)
  { 400, MI_OPTION_UNINIT, MI_OPTION(retry_on_oom) },             // windows only: retry on out-of-memory for N milli seconds (=400), set to 0 to disable retries.
  { 1,   MI_OPTION_UNINIT, MI_OPTION(deprecated_visit_abandoned) },
  { 0,   MI_OPTION_UNINIT, MI_OPTION(guarded_min) },              // only used when building with MI_GUARDED: minimal rounded object size for guarded objects
  { MI_GiB, MI_OPTION_UNINIT, MI_OPTION(guarded_max) },           // only used when building with MI_GUARDED: maximal rounded object size for guarded objects
  { 0,   MI_OPTION_UNINIT, MI_OPTION(unsafe_guarded_precise) },          // disregard minimal alignment requirement to always place guarded blocks exactly in front of a guard page (=0)
  { MI_DEFAULT_GUARDED_SAMPLE_RATE,
         MI_OPTION_UNINIT, MI_OPTION(guarded_sample_rate)},       // guard an object in the min/max range about every N requested bytes (=0)
  { 0,   MI_OPTION_UNINIT, MI_OPTION(guarded_sample_seed)},
  { 10000, MI_OPTION_UNINIT, MI_OPTION(generic_collect) },        // collect theaps every N (=10000) generic allocation calls
  { 1,   MI_OPTION_UNINIT, MI_OPTION_LEGACY(page_reclaim_on_free, abandoned_reclaim_on_free) },// reclaim abandoned (small) pages on a free: -1 = disable completely, 0 = only reclaim into the originating theap, 1 = reclaim on free across theaps
  { MI_DEFAULT_PAGE_FULL_RETAIN,
        MI_OPTION_UNINIT, MI_OPTION(page_full_retain) },         // number of (small) pages to retain in the free page queues
  { 4,   MI_OPTION_UNINIT, MI_OPTION(page_max_candidates) },      // max search to find a best page candidate
  { 0,   MI_OPTION_UNINIT, MI_OPTION(max_vabits) },               // max virtual address space bits
  { MI_DEFAULT_PAGEMAP_COMMIT,
         MI_OPTION_UNINIT, MI_OPTION(pagemap_commit) },           // commit the full pagemap upfront?
  { 0,   MI_OPTION_UNINIT, MI_OPTION(page_commit_on_demand) },    // commit pages on-demand (2 disables this only on overcommit systems (like Linux))
  { MI_DEFAULT_PAGE_MAX_RECLAIM,
         MI_OPTION_UNINIT, MI_OPTION(page_max_reclaim) },         // don't reclaim (small) pages of the same originating theap if we already own N pages in that size class
  { MI_DEFAULT_PAGE_CROSS_THREAD_MAX_RECLAIM,
         MI_OPTION_UNINIT, MI_OPTION(page_cross_thread_max_reclaim) }, // don't reclaim (small) pages across threads if we already own N pages in that size class
  { MI_DEFAULT_ALLOW_THP,
         MI_OPTION_UNINIT, MI_OPTION(allow_thp) },                // allow transparent huge pages? (=1) (on Android =0 by default). Set to 0 to disable THP for the process.
  { 0,   MI_OPTION_UNINIT, MI_OPTION_LEGACY(arena_purge_min_size, minimal_purge_size) },       // set minimal purge size (in KiB) (=0). Using 0 resolves to either 64 (or 2048 if THP is enabled).
  { MI_DEFAULT_ARENA_MAX_OBJECT_SIZE,
         MI_OPTION_UNINIT, MI_OPTION(arena_max_object_size) },    // set maximal object size that can be allocated in an arena (in KiB) (=2GiB on 64-bit).
  { 0,   MI_OPTION_UNINIT, MI_OPTION(arena_is_numa_local) },      // associate local numa node with an initial arena allocation
  { MI_DEFAULT_STATS_MERGE_ON_COLLECT,
         MI_OPTION_UNINIT, MI_OPTION(stats_merge_on_collect) },   // on each theap collect, stats are merged with the parent heap
  { MI_GiB, MI_OPTION_UNINIT, MI_OPTION(profile_alloc_interval) },// N bytes (=1 GiB default) between automatic dumps of the `MIMALLOC_PROFILE` heap profiler (use `option_get_size`); 0 disables automatic dumping
  { 0,   MI_OPTION_UNINIT, MI_OPTION(profile_inuse_interval) },   // N bytes between automatic dumps of the `MIMALLOC_PROFILE` heap profiler whenever in-use bytes grow by that amount (use `option_get_size`); 0 disables (default)
  { 0,   MI_OPTION_UNINIT, MI_OPTION(profile_time_interval) },    // N seconds between automatic dumps of the `MIMALLOC_PROFILE` heap profiler; 0 disables (default)
  { 512*MI_KiB, MI_OPTION_UNINIT, MI_OPTION(profile_sample_rate) }, // sample rate in bytes for the `MIMALLOC_PROFILE` heap profiler (use `option_get_size`) (=512 KiB)
  { MI_DEFAULT_STATS_MERGE_THRESHOLD,
         MI_OPTION_UNINIT, MI_OPTION(stats_merge_threshold) },    // on a page stats update, merge theap stats into the heap if N bytes were freed/allocated; 0 disables (=512 KiB)
  { MI_DEFAULT_ARENA_PURGE_IMMEDIATE_SIZE,
         MI_OPTION_UNINIT, MI_OPTION(arena_purge_immediate_size) }, // ranges above this size are purged immediately

};

static void mi_option_init(mi_option_desc_t* desc);

// option value is stored in KiB so it can fit a `long`
// todo: use mi_ssize_t for options?
static bool mi_option_has_size_in_kib(mi_option_t option) {
  return (option == mi_option_reserve_os_memory || option == mi_option_arena_reserve ||
          option == mi_option_arena_purge_min_size || option == mi_option_arena_purge_immediate_size ||
          option == mi_option_arena_max_object_size
          // || option == mi_option_profile_alloc_interval || option == mi_option_profile_inuse_interval || option == mi_option_profile_sample_rate
         );
}

// option value is stored in bytes as a `long` is enough.
static bool mi_option_has_size_in_bytes(mi_option_t option) {
  return (option == mi_option_stats_merge_threshold ||
          option == mi_option_profile_alloc_interval || option == mi_option_profile_inuse_interval || option == mi_option_profile_sample_rate);
}

void _mi_options_init(void) {
  // called on process load
  for(int i = 0; i < _mi_option_last; i++ ) {
    mi_option_t option = (mi_option_t)i;
    long l = mi_option_get(option); MI_UNUSED(l); // initialize
  }
  mi_max_error_count = mi_option_get(mi_option_max_errors);
  mi_max_warning_count = mi_option_get(mi_option_max_warnings);
  #if MI_GUARDED
  if (mi_option_get(mi_option_guarded_sample_rate) > 0) {
    if (mi_option_is_enabled(mi_option_allow_large_os_pages)) {
      mi_option_disable(mi_option_allow_large_os_pages);
      _mi_warning_message("option 'allow_large_os_pages' is disabled to allow for guarded objects\n");
    }
  }
  #endif
}

// called at actual process load, it should be safe to print now
void _mi_options_post_init(void) {
  mi_add_stderr_output(); // now it safe to use stderr for output
  if (mi_option_is_enabled(mi_option_verbose)) { mi_options_print(); }
}

#define mi_stringifyx(str)  #str                // and stringify
#define mi_stringify(str)   mi_stringifyx(str)  // expand

mi_decl_export void mi_options_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept
{
  // show version
  const int vermajor = MI_MALLOC_VERSION/10000;
  const int verminor = (MI_MALLOC_VERSION%10000)/100;
  const int verpatch = (MI_MALLOC_VERSION%100);
  _mi_fprintf(out, arg, "v%i.%i.%i%s%s\n", vermajor, verminor, verpatch,
      #if defined(MI_CMAKE_BUILD_TYPE)
      ", " mi_stringify(MI_CMAKE_BUILD_TYPE)
      #else
      ""
      #endif
      ,
      #if defined(MI_GIT_DESCRIBE)
      ", git " mi_stringify(MI_GIT_DESCRIBE)
      #else
      ""
      #endif
      );

  // show options
  for (int i = 0; i < _mi_option_last; i++) {
    mi_option_t option = (mi_option_t)i;
    long l = mi_option_get(option); MI_UNUSED(l); // possibly initialize
    mi_option_desc_t* desc = &mi_options[option];
    _mi_fprintf(out, arg, "option '%s': %ld %s\n", desc->name, desc->value, (mi_option_has_size_in_kib(option) ? "KiB" : (mi_option_has_size_in_bytes(option) ? "B" : "")));
  }

  // show build configuration
  #if MI_PAGE_META_IS_ALIGNED && MI_PAGE_META_SMALL_IS_ALIGNED
  _mi_fprintf(out, arg, "free mode   : (small) aligned, page size: %zu\n", sizeof(mi_page_t));
  #elif MI_PAGE_META_IS_ALIGNED
  _mi_fprintf(out, arg, "free mode   : aligned, page size: %zu\n", sizeof(mi_page_t));
  #elif MI_PAGE_META_SMALL_IS_ALIGNED
  _mi_fprintf(out, arg, "free mode   : small aligned + pagemap, page size: %zu\n", sizeof(mi_page_t));
  #elif MI_FREE_IS_CHECKED
  _mi_fprintf(out, arg, "free mode   : checked, page size: %zu\n", sizeof(mi_page_t));
  #else
  _mi_fprintf(out, arg, "free mode   : pagemap, page size: %zu\n", sizeof(mi_page_t));
  #endif
  #if MI_ENCODE_FREELIST
  _mi_fprintf(out, arg, "free lists  : encoded with %d key(s)\n", MI_PAGE_KEY_COUNT);
  #endif
  #if MI_GUARDED
  _mi_fprintf(out, arg, "guarded mode: %s, rate=%ld\n",  MI_SAMPLE==2 ? "fine-grained" : "enabled", mi_option_get(mi_option_guarded_sample_rate));
  #endif
  #if MI_PROFILE
  _mi_fprintf(out, arg, "profiling   : %s\n", MI_SAMPLE==2 ? "fine-grained" : "enabled");
  #endif
  #if MI_TSAN
  _mi_fprintf(out, arg, "thread santizer enabled\n");
  #endif
  _mi_fprintf(out, arg, "mem tracking: %s\n", MI_TRACK_TOOL);
  _mi_fprintf(out, arg, "debug level : %d\n", MI_DEBUG );
  _mi_fprintf(out, arg, "secure level: %d\n", MI_SECURE );
  _mi_fprintf(out, arg, "padding     : %s\n", (MI_PADDING==0 ? "none" : (MI_PADDING==1 ? "enabled" : "byte precise")));
}

mi_decl_export void mi_options_print(void) mi_attr_noexcept {
  mi_options_print_out(NULL, NULL);
}

long _mi_option_get_fast(mi_option_t option) {
  mi_assert(option >= 0 && option < _mi_option_last);
  mi_option_desc_t* desc = &mi_options[option];
  mi_assert(desc->option == option);  // index should match the option
  //mi_assert(desc->init != MI_OPTION_UNINIT);
  return desc->value;
}


mi_decl_nodiscard long mi_option_get(mi_option_t option) {
  mi_assert(option >= 0 && option < _mi_option_last);
  if (option < 0 || option >= _mi_option_last) return 0;
  mi_option_desc_t* desc = &mi_options[option];
  mi_assert(desc->option == option);  // index should match the option
  if mi_unlikely(desc->init == MI_OPTION_UNINIT) {
    mi_option_init(desc);
  }
  return desc->value;
}

mi_decl_nodiscard long mi_option_get_clamp(mi_option_t option, long min, long max) {
  long x = mi_option_get(option);
  return (x < min ? min : (x > max ? max : x));
}

mi_decl_nodiscard size_t mi_option_get_size(mi_option_t option) {
  const long x = mi_option_get(option);
  size_t size = (x < 0 ? 0 : (size_t)x);
  if (mi_option_has_size_in_kib(option)) {
    if (mi_mul_overflow(size, MI_KiB, &size)) {
      size = MI_MAX_ALLOC_SIZE;
    }
  }
  return size;
}

void mi_option_set(mi_option_t option, long value) {
  mi_assert(option >= 0 && option < _mi_option_last);
  if (option < 0 || option >= _mi_option_last) return;
  mi_option_desc_t* desc = &mi_options[option];
  mi_assert(desc->option == option);  // index should match the option
  desc->value = value;
  desc->init = MI_OPTION_INITIALIZED;
  // ensure min/max range; be careful to not recurse.
  if (desc->option == mi_option_guarded_min && _mi_option_get_fast(mi_option_guarded_max) < value) {
    mi_option_set(mi_option_guarded_max, value);
  }
  else if (desc->option == mi_option_guarded_max && _mi_option_get_fast(mi_option_guarded_min) > value) {
    mi_option_set(mi_option_guarded_min, value);
  }
}

void mi_option_set_default(mi_option_t option, long value) {
  mi_assert(option >= 0 && option < _mi_option_last);
  if (option < 0 || option >= _mi_option_last) return;
  mi_option_desc_t* desc = &mi_options[option];
  if (desc->init != MI_OPTION_INITIALIZED) {
    desc->value = value;
  }
}

mi_decl_nodiscard bool mi_option_is_enabled(mi_option_t option) {
  return (mi_option_get(option) != 0);
}

void mi_option_set_enabled(mi_option_t option, bool enable) {
  mi_option_set(option, (enable ? 1 : 0));
}

void mi_option_set_enabled_default(mi_option_t option, bool enable) {
  mi_option_set_default(option, (enable ? 1 : 0));
}

void mi_option_enable(mi_option_t option) {
  mi_option_set_enabled(option,true);
}

void mi_option_disable(mi_option_t option) {
  mi_option_set_enabled(option,false);
}

static void mi_cdecl mi_out_stderr(const char* msg, void* arg) {
  MI_UNUSED(arg);
  if (msg != NULL && msg[0] != 0) {
    _mi_prim_out_stderr(msg);
  }
}

// Since an output function can be registered earliest in the `main`
// function we also buffer output that happens earlier. When
// an output function is registered it is called immediately with
// the output up to that point.
#ifndef MI_MAX_DELAY_OUTPUT
#define MI_MAX_DELAY_OUTPUT ((size_t)(16*1024))
#endif
static char out_buf[MI_MAX_DELAY_OUTPUT+1];
static _Atomic(size_t) out_len;
static mi_lock_t out_buf_lock = MI_LOCK_INITIALIZER;

static void mi_cdecl mi_out_buf(const char* msg, void* arg) {
  MI_UNUSED(arg);
  if (msg==NULL) return;
  if (mi_atomic_load_acquire(&out_len)>=MI_MAX_DELAY_OUTPUT) return;
  size_t n = _mi_strlen(msg);
  if (n==0 || n >= MI_MAX_DELAY_OUTPUT) return;
  // copy msg into the buffer
  mi_lock(&out_buf_lock) {
    const size_t start = mi_atomic_add_acq_rel(&out_len, n);
    if (start < MI_MAX_DELAY_OUTPUT) {
      // check bound
      if (start+n >= MI_MAX_DELAY_OUTPUT) {
        n = MI_MAX_DELAY_OUTPUT-start-1;
      }
      mi_memcpy(&out_buf[start], msg, n);
    }
  }
}

static void mi_out_buf_flush(mi_output_fun* out, bool no_more_buf, void* arg) {
  if (out==NULL) return;
  // claim (if `no_more_buf == true`, no more output will be added after this point)
  mi_lock(&out_buf_lock) {
    size_t count = mi_atomic_add_acq_rel(&out_len, (no_more_buf ? MI_MAX_DELAY_OUTPUT : 1));
    // and output the current contents
    if (count>MI_MAX_DELAY_OUTPUT) count = MI_MAX_DELAY_OUTPUT;
    out_buf[count] = 0;
    out(out_buf,arg);
    if (!no_more_buf) {
      out_buf[count] = '\n'; // if continue with the buffer, insert a newline
    }
  }
}


// Once this module is loaded, switch to this routine
// which outputs to stderr and the delayed output buffer.
static void mi_cdecl mi_out_buf_stderr(const char* msg, void* arg) {
  mi_out_stderr(msg,arg);
  mi_out_buf(msg,arg);
}



// --------------------------------------------------------
// Default output handler
// --------------------------------------------------------

// The program should only install a single output handler from a single thread
// since otherwise the argument and output function may not match.
static _Atomic(void*) mi_out_default; // = // is `mi_output_fun*` (but some platforms don't support atomic function pointers)
static _Atomic(void*) mi_out_arg; // = NULL

static mi_output_fun* mi_out_get_default(void** parg) {
  mi_output_fun* const out = (mi_output_fun*)mi_atomic_load_ptr_acquire(void,&mi_out_default);
  if (parg != NULL) { *parg = mi_atomic_load_ptr_acquire(void,&mi_out_arg); }
  return (out == NULL ? &mi_out_buf : out);
}

void mi_register_output(mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_atomic_store_ptr_release(void,&mi_out_default, (void*)(out == NULL ? &mi_out_stderr : out)); // stop using the delayed output buffer
  mi_atomic_store_ptr_release(void,&mi_out_arg, arg);
  if (out!=NULL) { mi_out_buf_flush(out,true,arg); }        // output all the delayed output now
}

// add stderr to the delayed output after the module is loaded
static void mi_add_stderr_output(void) {
  mi_assert_internal(mi_out_default == NULL);
  mi_out_buf_flush(&mi_out_stderr, false, NULL); // flush current contents to stderr
  mi_atomic_store_ptr_release(void,&mi_out_default,(void*)&mi_out_buf_stderr);  // and add stderr to the delayed output
  mi_atomic_store_ptr_release(void,&mi_out_arg,NULL);
}

// --------------------------------------------------------
// Messages, all end up calling `_mi_fputs`.
// --------------------------------------------------------
static _Atomic(size_t) error_count;   // = 0;  // when >= max_error_count stop emitting errors
static _Atomic(size_t) warning_count; // = 0;  // when >= max_warning_count stop emitting warnings

static mi_decl_noinline bool mi_recurse_enter_prim(void) {
  return true;
}

static mi_decl_noinline void mi_recurse_exit_prim(void) {
  /* nothing */
}

static bool mi_recurse_enter(void) {
  #if defined(__APPLE__) || defined(__ANDROID__) || defined(MI_TLS_RECURSE_GUARD)
  if (_mi_preloading()) return false;
  #endif
  return mi_recurse_enter_prim();
}

static void mi_recurse_exit(void) {
  #if defined(__APPLE__) || defined(__ANDROID__) || defined(MI_TLS_RECURSE_GUARD)
  if (_mi_preloading()) return;
  #endif
  mi_recurse_exit_prim();
}

void _mi_fputs(mi_output_fun* out, void* arg, const char* prefix, const char* message) {
  if (out==NULL || (void*)out==(void*)stdout || (void*)out==(void*)stderr) { // todo: use mi_out_stderr for stderr?
    if (!mi_recurse_enter()) return;
    out = mi_out_get_default(&arg);
    if (prefix != NULL) out(prefix, arg);
    out(message, arg);
    mi_recurse_exit();
  }
  else {
    if (prefix != NULL) out(prefix, arg);
    out(message, arg);
  }
}

// Define our own limited `fprintf` that avoids memory allocation.
// We do this using `_mi_vsnprintf` with a limited buffer.
static void mi_vfprintf( mi_output_fun* out, void* arg, const char* prefix, const char* fmt, va_list args ) {
  char buf[992];
  if (fmt==NULL) return;
  if (!mi_recurse_enter()) return;
  _mi_vsnprintf(buf, sizeof(buf)-1, fmt, args);
  mi_recurse_exit();
  _mi_fputs(out,arg,prefix,buf);
}

void _mi_fprintf( mi_output_fun* out, void* arg, const char* fmt, ... ) {
  va_list args;
  va_start(args,fmt);
  mi_vfprintf(out,arg,NULL,fmt,args);
  va_end(args);
}

static void mi_vfprintf_thread(mi_output_fun* out, void* arg, const char* prefix, const char* fmt, va_list args) {
  if (prefix != NULL && _mi_strnlen(prefix,33) <= 32) { // && !_mi_is_main_thread()) {
    char tprefix[64];
    _mi_snprintf(tprefix, sizeof(tprefix), "%sthread 0x%tx: ", prefix, (uintptr_t)_mi_thread_id());
    mi_vfprintf(out, arg, tprefix, fmt, args);
  }
  else {
    mi_vfprintf(out, arg, prefix, fmt, args);
  }
}

void _mi_raw_message(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  mi_vfprintf(NULL, NULL, NULL, fmt, args);
  va_end(args);
}

void _mi_trace_message(const char* fmt, ...) {
  if (mi_option_get(mi_option_verbose) <= 1) return;  // only with verbose level 2 or higher
  va_list args;
  va_start(args, fmt);
  mi_vfprintf_thread(NULL, NULL, "mimalloc: ", fmt, args);
  va_end(args);
}

void _mi_verbose_message(const char* fmt, ...) {
  if (!mi_option_is_enabled(mi_option_verbose)) return;
  va_list args;
  va_start(args,fmt);
  mi_vfprintf(NULL, NULL, "mimalloc: ", fmt, args);
  va_end(args);
}

static void mi_show_error_message(const char* fmt, va_list args) {
  if (!mi_option_is_enabled(mi_option_verbose)) {
    if (!mi_option_is_enabled(mi_option_show_errors)) return;
    if (mi_max_error_count >= 0 && (long)mi_atomic_increment_acq_rel(&error_count) > mi_max_error_count) return;
  }
  mi_vfprintf_thread(NULL, NULL, "mimalloc: error: ", fmt, args);
}

void _mi_warning_message(const char* fmt, ...) {
  if (!mi_option_is_enabled(mi_option_verbose)) {
    if (!mi_option_is_enabled(mi_option_show_errors)) return;
    if (mi_max_warning_count >= 0 && (long)mi_atomic_increment_acq_rel(&warning_count) > mi_max_warning_count) return;
  }
  va_list args;
  va_start(args,fmt);
  mi_vfprintf_thread(NULL, NULL, "mimalloc: warning: ", fmt, args);
  va_end(args);
}


#if MI_DEBUG
mi_decl_noreturn mi_decl_cold void _mi_assert_fail(const char* assertion, const char* fname, unsigned line, const char* func ) mi_attr_noexcept {
  _mi_fprintf(NULL, NULL, "mimalloc: assertion failed: at \"%s\":%u, %s\n  assertion: \"%s\"\n", fname, line, (func==NULL?"":func), assertion);
  abort();
}
#endif

// --------------------------------------------------------
// Errors
// --------------------------------------------------------

static mi_error_fun* volatile  mi_error_handler; // = NULL
static _Atomic(void*) mi_error_arg;     // = NULL

static void mi_error_default(int err) {
  MI_UNUSED(err);
  #if (MI_DEBUG>0)
    if (err==EFAULT) {
      #ifdef _MSC_VER
      __debugbreak();
      #endif
      abort();
    }
  #endif
  #if (MI_SECURE>0)
    if (err==EFAULT) {  // abort on serious errors in secure mode (corrupted meta-data)
      abort();
    }
  #endif
  #if defined(MI_XMALLOC)
    if (err==ENOMEM || err==EOVERFLOW || err==EINVAL) { // abort on memory allocation fails in xmalloc mode
      abort();
    }
  #endif
  if (errno==0) {
    errno = (err==EINVAL ? EINVAL : ENOMEM /* compatibility */ );
  }
}

void mi_register_error(mi_error_fun* fun, void* arg) {
  mi_error_handler = fun;  // can be NULL
  mi_atomic_store_ptr_release(void,&mi_error_arg, arg);
}

void _mi_error_message(int err, const char* fmt, ...) {
  // show detailed error message
  va_list args;
  va_start(args, fmt);
  mi_show_error_message(fmt, args);
  va_end(args);
  // and call the error handler which may abort (or return normally, potentially setting errno)
  if (mi_error_handler != NULL) {
    mi_error_handler(err, mi_atomic_load_ptr_acquire(void,&mi_error_arg));
  }
  else {
    mi_error_default(err);
  }
}

mi_decl_noinline mi_block_t* _mi_block_next_is_corrupted(const mi_page_t* page, const mi_block_t* block, const mi_block_t* next) {
  _mi_error_message(EFAULT, "corrupted free list entry of size %zub at %p: value 0x%zx\n", mi_page_block_size(page), block, (uintptr_t)next);
  return NULL;
}

// --------------------------------------------------------
// Initialize options by checking the environment
// --------------------------------------------------------

static void mi_option_init(mi_option_desc_t* desc) {
  // Read option value from the environment
  char s[64 + 1];
  char buf[64+1];
  _mi_strlcpy(buf, "mimalloc_", sizeof(buf));
  _mi_strlcat(buf, desc->name, sizeof(buf));
  int err = _mi_getenv(buf, s, sizeof(s));
  if (err==ENOENT && desc->legacy_name != NULL) {
    _mi_strlcpy(buf, "mimalloc_", sizeof(buf));
    _mi_strlcat(buf, desc->legacy_name, sizeof(buf));
    err = _mi_getenv(buf, s, sizeof(s));
    if (err==0) {
      _mi_warning_message("environment option \"mimalloc_%s\" is deprecated -- use \"mimalloc_%s\" instead.\n", desc->legacy_name, desc->name);
    }
  }

  if (err==0) {
    size_t len = _mi_strnlen(s, sizeof(buf) - 1);
    for (size_t i = 0; i < len; i++) {
      buf[i] = _mi_toupper(s[i]);
    }
    buf[len] = 0;
    if (buf[0] == 0 || _mi_streq(buf,"1") || _mi_streq(buf,"TRUE") || _mi_streq(buf,"YES") || _mi_streq(buf,"ON")) {
      desc->value = 1;
      desc->init = MI_OPTION_INITIALIZED;
    }
    else if (_mi_streq(buf,"0") || _mi_streq(buf,"FALSE") || _mi_streq(buf,"NO") || _mi_streq(buf,"OFF")) {
      desc->value = 0;
      desc->init = MI_OPTION_INITIALIZED;
    }
    else {
      const char* end = buf;
      long value = 0;
      const bool ok = _mi_strntol(buf, sizeof(buf), &end, &value);
      if (ok && (mi_option_has_size_in_kib(desc->option) || mi_option_has_size_in_bytes(desc->option))) {
        // the size is interpreted in bytes.
        size_t size = (value < 0 ? 0 : (size_t)value);
        bool overflow = false;
        if (*end == 'K') { overflow = mi_mul_overflow(size,MI_KiB,&size); end++; }
        #if MI_SIZE_SIZE >= 4
        else if (*end == 'M') { overflow = mi_mul_overflow(size,MI_MiB,&size); end++; }
        else if (*end == 'G') { overflow = mi_mul_overflow(size,MI_GiB,&size); end++; }
        #endif
        #if MI_SIZE_SIZE >= 8
        else if (*end == 'T') { overflow = mi_mul_overflow(size,MI_GiB * MI_KiB,&size); end++; }
        #endif
        if (end[0] == 'I' && end[1] == 'B') { end += 2; } // KiB, MiB, GiB, TiB
        else if (*end == 'B') { end++; }                  // Kb, Mb, Gb, Tb
        if (overflow || size > MI_MAX_ALLOC_SIZE) { size = MI_MAX_ALLOC_SIZE; }
        if (mi_option_has_size_in_kib(desc->option)) {
          // This option is interpreted in KiB to prevent overflow of `long` for large allocations
          // (long is 32-bit on 64-bit windows, which allows for 4TiB max.)
          size = (size + MI_KiB - 1) / MI_KiB;  // no overflow due to check before this
        }
        value = (size > LONG_MAX ? LONG_MAX : (long)size);
      }
      if (ok && *end == 0) {
        mi_option_set(desc->option, value);
      }
      else {
        // set `init` first to avoid recursion through _mi_warning_message on mimalloc_verbose.
        desc->init = MI_OPTION_DEFAULTED;
        if (desc->option == mi_option_verbose && desc->value == 0) {
          // if the 'mimalloc_verbose' env var has a bogus value we'd never know
          // (since the value defaults to 'off') so in that case briefly enable verbose
          desc->value = 1;
          _mi_warning_message("environment option mimalloc_%s has an invalid value.\n", desc->name);
          desc->value = 0;
        }
        else {
          _mi_warning_message("environment option mimalloc_%s has an invalid value.\n", desc->name);
        }
      }
    }
    mi_assert_internal(desc->init != MI_OPTION_UNINIT);
  }
  else if (err==ENOENT) {
    desc->init = MI_OPTION_DEFAULTED;
  }
  // and on another error, keep unitialized to try again (can happen during preloading if getenv is not available)
}
