/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.

Disclaimer: Unlike the rest of the considerate hand-crafted mimalloc sources, 
this file was written with AI assistance (for the process image and protobuf 
format generation).
-----------------------------------------------------------------------------*/

// ---------------------------------------------------------------------------
// This implements a basic memory profiler that outputs pprof-compatible heap 
// snapshots (text or protobuf). It is basic, but it does scale with a lock-free
// hash table for locations, and uses poisson distributed sampling.
// It has no impact on runtime performance when profiling is not active.
//
// It can be used programmatically (using `mi_pprof_profiler_new` etc.)
// or be invoked with environment variables. For example:
//   MIMALLOC_PROFILE=profile ./my_program
// and then analyze the generated profile using pprof tools:
//   pprof -http=:8080 ./myprogram  profile.*
// ---------------------------------------------------------------------------

#include "mimalloc.h"
#include "mimalloc/prim/prim-tls.h"   // _mi_theap_default
#include "mimalloc/util/libc.h"
#include "mimalloc/util/options.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/profile/pprof.h"

// ---------------------------------------------------------------------------
// Internal API
// ---------------------------------------------------------------------------

typedef struct mi_location_s  mi_location_t;
typedef struct mi_locations_s mi_locations_t;
typedef struct mi_callstack_s mi_callstack_t;

static bool    mi_locations_init(mi_heap_t* heap, mi_locations_t* locations);
static void    mi_locations_done(mi_heap_t* heap, mi_locations_t* locations);
static mi_location_t* mi_locations_find_or_insert(mi_heap_t* heap, mi_locations_t* locations, mi_threadid_t thread_id, const mi_callstack_t* callstack, size_t callstack_hash);

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
#define MI_MAX_BACKTRACE_DEPTH    (100)
#define MI_BACKTRACE_SKIP_FRAMES  (3)

// Threshold (in bytes) for the thread-local interval countdown batching in `on_alloc`
#ifndef MI_PROFILE_COUNTDOWN_THRESHOLD
#define MI_PROFILE_COUNTDOWN_THRESHOLD        (8*MI_KiB)
#endif
#define MI_PROFILE_COUNTDOWN_MIN_INTERVAL     (8*MI_PROFILE_COUNTDOWN_THRESHOLD)

// A callstack represents a sequence of function call frames captured at a specific point in time.
struct mi_callstack_s {
  size_t  count;
  void**  frames;
};

// A location is a unique combination of a thread and runtime callstack for 
// which we track allocation and deallocation statistics.
//
// Locations are only ever inserted, or mutated in-place after publication, and a location is only ever looked up using its own
// `hash`/`thread_id`/`callstack` which allows a lock-free lookup of locations in the hash table.
struct mi_location_s {
  _Atomic(struct mi_location_s*) next;  // next location in the same hash bucket (for chaining on collision);
  mi_threadid_t     thread_id;
  mi_callstack_t    callstack;
  size_t            hash;         // hash of the thread_id and callstack
  int64_t           alloc_count;  // only ever written by the single (allocating) owning thread: not atomic
  int64_t           alloc_bytes;
  _Atomic(size_t)   inuse_count;  // currently in-use (allocated but not yet freed) objects/bytes for this location;
  _Atomic(size_t)   inuse_bytes;  // incremented in `on_alloc` and decremented in `on_free`, both possibly on any thread
};

// A basic hash table of locations. The bucket array is fixed-size (allocated
// once in `mi_locations_init`) and each bucket is a lock-free singly-linked
// list that only ever grows by prepending (see `mi_locations_find_or_insert`).
struct mi_locations_s {
  size_t                      bucket_count;  // number of buckets (fixed size)
  _Atomic(mi_location_t*)*    buckets;       // array of `bucket_count` chain heads
  _Atomic(size_t)             count;         // number of locations currently stored (just a statistic)
};

// Our profiler.
typedef struct {
  mi_profiler_t        profiler;                  // mimalloc profiler info
  mi_heap_t*           profile_heap;              // heap used for all profiler allocations (so these don't interfere)
  mi_locations_t       locations;                 // hash table of call locations
  size_t               sample_threshold;          // current sample threshold
  size_t               snapshot_count;                // number of times `mi_profiler_snapshot` was called (used to number the snapshot files)
  char*                base_file_name;            // base file name for snapshot files, e.g. "<base_file_name>.<seq>.heap"
  bool                 format_text;               // text or protobuf snapshot format
  size_t               alloc_interval_size;       // if >0, automatically snapshot every `alloc_interval_size` allocated bytes (capped at `MI_SSIZE_MAX`)
  _Atomic(mi_ssize_t)  alloc_interval_countdown;  // bytes remaining until the next automatic snapshot (can go negative; decremented concurrently by `on_alloc` on any thread)
  size_t               inuse_interval_size;       // if >0, automatically snapshot every time (sampled) in-use bytes grow by `inuse_interval_size` bytes
  _Atomic(size_t)      inuse_bytes;               // running total of (sampled) in-use bytes across all locations (can never exceed the address space); incremented in `on_alloc`, decremented in `on_free`, both possibly on any thread
  _Atomic(size_t)      inuse_interval_threshold;  // next `inuse_bytes` total that triggers a snapshot; advanced (monotonically increasing) whenever reached in `on_alloc`
  size_t               time_interval_secs;        // if >0, automatically snapshot every `time_interval_secs` seconds (checked from `on_alloc`)
  _Atomic(mi_msecs_t)  time_interval_deadline;    // next `_mi_clock_now()` value that triggers a snapshot; advanced (monotonically increasing) whenever reached in `on_alloc`
} mi_pprof_profiler_t;

static inline mi_pprof_profiler_t* downcast( mi_profiler_t* prof ) { 
  return (mi_pprof_profiler_t*)prof; 
} 

static void    mi_pprof_profiler_snapshot(mi_pprof_profiler_t* prof);
static size_t  mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash);  // `buffer` must have room for `max_depth + MI_PPROF_SKIP_FRAMES + 1` entries (see definition)


// ---------------------------------------------------------------------------
// Automatic profiler driven by the `MIMALLOC_PROFILE` environment variable.
// This allows profiling an arbitrary program (with no code changes) by
// overriding its allocator with a mimalloc shared library build and setting
// `MIMALLOC_PROFILE=<base_file_name>` 
// (and optionally `MIMALLOC_PROFILE_ALLOC_INTERVAL=<KiB>` to periodically snapshot 
// while the program is running).
// ---------------------------------------------------------------------------
#ifndef MI_PROFILE_USE_BUILTIN
#define MI_PROFILE_USE_BUILTIN  (MI_PROFILE)
#endif

#if MI_PROFILE_USE_BUILTIN   // todo: also disable profiling in secure mode?

static bool mi_is_elevated_process(void);
static mi_profiler_t* mi_profile_env_profiler;  // NULL if `MIMALLOC_PROFILE` was not set (or initialization failed)

// Called once at process initialization (see `mi_process_init` in `heap/init.c`).
void _mi_pprof_profiler_init(void) {
  #if MI_PROFILE
  if (mi_is_elevated_process()) return;  // don't let an untrusted environment influence a privileged process
  char fname[1024];
  if (_mi_getenv("mimalloc_profile", fname, sizeof(fname)) != 0 || fname[0] == 0) return;  // `MIMALLOC_PROFILE` not set
  const size_t sample_rate   = mi_option_get_size(mi_option_profile_sample_rate);           // 16 KiB by default
  const size_t alloc_interval_size = mi_option_get_size(mi_option_profile_alloc_interval);        // 0 by default (no automatic interval snapshots)
  const size_t inuse_interval_size = mi_option_get_size(mi_option_profile_inuse_interval);        // 0 by default (no automatic interval snapshots)
  const size_t time_interval_secs = mi_option_get_clamp(mi_option_profile_time_interval,0,24*60*60L);  // 0 by default (no automatic interval snapshots)  
  mi_profiler_t* profiler = mi_pprof_profiler_new(sample_rate, fname, alloc_interval_size, inuse_interval_size, time_interval_secs);
  if (profiler == NULL) return;
  mi_profile_env_profiler = profiler;
  _mi_verbose_message("pprof profiler initialized with base file name: %s\n", fname);
  mi_profile(profiler);           // attach to the main sub-process (and any current/future heaps in it)
  mi_profiler_start(profiler);
  #endif
}

// Called once at process termination (see `mi_process_done` in `heap/init.c`).
void _mi_pprof_profiler_done(void) {
  #if MI_PROFILE
  mi_profiler_t* profiler = mi_profile_env_profiler;
  if (profiler == NULL) return;
  mi_profile_env_profiler = NULL;
  mi_profiler_stop(profiler);
  _mi_verbose_message("pprof profiler done\n");
  mi_profiler_snapshot(profiler);    // final snapshot so short-lived processes still get a complete profile
  mi_profile(NULL);                  // detach from the main sub-process
  mi_pprof_profiler_delete(profiler);
  #endif
}

#else

void _mi_pprof_profiler_init(void) {
}

void _mi_pprof_profiler_done(void) {
}

#endif

// ---------------------------------------------------------------------------
// Get a location
// ---------------------------------------------------------------------------

static mi_decl_noinline mi_location_t* mi_location_get(mi_pprof_profiler_t* prof) {
  void* frames[MI_MAX_BACKTRACE_DEPTH + 1 + MI_BACKTRACE_SKIP_FRAMES];  // extra room: `mi_prim_backtrace` captures+shifts out its own frame and `MI_BACKTRACE_SKIP_FRAMES` wrapper frames in-place
  size_t callstack_hash = 0;
  const size_t depth = mi_prim_backtrace(frames, MI_MAX_BACKTRACE_DEPTH, &callstack_hash);
  if (depth==0) return NULL;
  mi_callstack_t callstack = { depth, frames };
  const mi_threadid_t thread_id = _mi_thread_id();
  mi_location_t* loc = mi_locations_find_or_insert(prof->profile_heap, &prof->locations, thread_id, &callstack, callstack_hash);
  return loc;
}

// ---------------------------------------------------------------------------
// Profiler callbacks and initialization
// ---------------------------------------------------------------------------

// Draws an integer approximating Exp(scale) to give a poisson distribution,
// instead of a fixed period, which avoids sampling bias from allocation 
// patterns that happen to align with a constant period
static size_t mi_exp_sample(mi_theap_t* theap, size_t scale) {
  if (scale == 0) return 0;                                     // keep 0
  #if MI_PROFILE==2
  if (scale == 1) return 1;                                     // keep 1 as is (to allow sampling every allocation)
  #endif
  const size_t r  = _mi_theap_random_next(theap) | 1;   
  const size_t lz = mi_clz(r);                                  // coarse -log2(U): Geometric(1/2)
  const size_t frac_q8 = (r << (lz + 1)) >> (MI_SIZE_BITS - 8); // next 8 bits: fractional refinement, in [0,256)
  const size_t bits_q8 = (lz << 8) | frac_q8;                   // Q8 fixed-point estimate of -log2(U)
  const size_t ln_q8   = (bits_q8 * 177) >> 8;                  // -log2(U) -> -ln(U): *ln(2) (~177/256)
  size_t next = ((ln_q8 * scale) >> 8) + 1;                     // scale by the mean; +1 avoids a zero threshold
  const size_t cap = scale * 32;                                // clamp the (rare) long tail
  return (next > cap ? cap : next);
}

// Check if the sampled bytes accumulated so far (batched per-thread) should trigger a shared
// countdown decrement, and if so, whether that decrement crosses zero (triggering a snapshot and
// resetting the countdown to `alloc_interval_size`).
static bool mi_interval_update(uint64_t ubytes_since_last_sample, size_t alloc_interval_size, _Atomic(mi_ssize_t)* alloc_interval_countdown, mi_ssize_t* interval_pending) {
  mi_ssize_t sampled_bytes = 0;
  mi_ssize_t bytes_since_last_sample= (ubytes_since_last_sample > MI_SSIZE_MAX ? MI_SSIZE_MAX : (mi_ssize_t)ubytes_since_last_sample);
  if (alloc_interval_size < MI_PROFILE_COUNTDOWN_MIN_INTERVAL || MI_SSIZE_MAX - *interval_pending < bytes_since_last_sample /* overflow? */) {
    // interval too small relative to the batching; always adjust
    sampled_bytes = bytes_since_last_sample;
  }
  else {
    // batch locally per-thread and only touch the shared (contended) countdown
    // once the local pending amount reaches `MI_PROFILE_COUNTDOWN_THRESHOLD`.
    *interval_pending += bytes_since_last_sample;
    if (*interval_pending >= (mi_ssize_t)MI_PROFILE_COUNTDOWN_THRESHOLD) {
      sampled_bytes = *interval_pending;
      *interval_pending = 0;
    }
  }
  // update profile interval countdown if we have sampled bytes
  if (sampled_bytes > 0) {
    const mi_ssize_t new_countdown = mi_atomic_addi_relaxed(alloc_interval_countdown, -sampled_bytes) - sampled_bytes;
    if (new_countdown <= 0 && new_countdown + sampled_bytes > 0) {
      mi_atomic_storess_relaxed(alloc_interval_countdown, (mi_ssize_t)alloc_interval_size);
      return true;
    }
  }
  return false;
}

// Check if the (global) in-use byte total has reached `inuse_interval_threshold`
static bool mi_inuse_threshold_update(size_t new_total, size_t inuse_interval_size, _Atomic(size_t)* inuse_interval_threshold) {
  size_t threshold = mi_atomic_load_relaxed(inuse_interval_threshold);
  while (new_total >= threshold) {
    size_t next_threshold = threshold + inuse_interval_size;
    while (next_threshold <= new_total) { next_threshold += inuse_interval_size; }  // skip ahead if we grew by more than one interval
    if (mi_atomic_cas_weak_relaxed(inuse_interval_threshold, &threshold, next_threshold)) {
      return true;
    }
    // CAS failed: `threshold` now holds the current value; retry (another thread may have already advanced it far enough)
  }
  return false;
}

// Check if `now` has reached `time_interval_deadline`, and if so, atomically advance the
// deadline to the next multiple of `time_interval_secs` beyond `now`, returning `true` (to the
// single thread that won the race) so a snapshot is triggered exactly once.
static bool mi_time_threshold_update(mi_msecs_t now, size_t time_interval_secs, _Atomic(mi_msecs_t)* time_interval_deadline) {
  const mi_msecs_t interval_msecs = (mi_msecs_t)time_interval_secs * 1000;
  mi_msecs_t deadline = mi_atomic_loadi64_relaxed(time_interval_deadline);
  while (now >= deadline) {
    mi_msecs_t next_deadline = deadline + interval_msecs;
    while (next_deadline <= now) { next_deadline += interval_msecs; }  // skip ahead if more than one interval has passed
    if (mi_atomic_casi64_strong_acq_rel(time_interval_deadline, &deadline, next_deadline)) {
      return true;
    }
    // CAS failed: `deadline` now holds the current value; retry (another thread may have already advanced it far enough)
  }
  return false;
}

// Sample allocation event and update profiling data accordingly
static size_t mi_cdecl on_alloc(mi_profiler_t* profiler, mi_profiler_sample_data_t* data, void* ptr, size_t requested_size, size_t threshold, uint64_t bytes_since_last_sample, mi_heap_t* heap) 
{
  MI_UNUSED(threshold); MI_UNUSED(heap); MI_UNUSED(ptr);
  mi_pprof_profiler_t* prof = downcast(profiler);
  mi_location_t* loc = mi_location_get(prof);
  mi_theap_t* theap = _mi_heap_theap(heap);
  const size_t new_threshold = mi_exp_sample(theap, prof->sample_threshold);  // randomized (Poisson) next sample threshold
  const size_t alloc_size = requested_size;            // or mi_heap_usable_size(heap,ptr) ?

  if (data!=NULL) {
    // pass the location to the corresponding mi_free (maybe called from another thread)
    mi_assert(data->user_data_size >= 2*sizeof(void*));
    data->user_data[0] = loc;
    data->user_data[1] = (void*)((uintptr_t)alloc_size);
  }
  if (loc!=NULL) {
    // `on_alloc` for a given location only ever runs on its one owning (allocating)
    // thread, so plain increments are safe here (no concurrent writers possible).
    loc->alloc_bytes += (int64_t)alloc_size;
    loc->alloc_count += 1;
    // `inuse_count`/`inuse_bytes` can be decremented concurrently by `on_free` on any
    // thread, so these updates must be atomic.
    mi_atomic_add_relaxed(&loc->inuse_bytes, alloc_size);
    mi_atomic_increment_relaxed(&loc->inuse_count);
  }

  if (prof->alloc_interval_size > 0) { // interval-based profiling enabled
    mi_ssize_t* pinterval_pending = (mi_ssize_t*)&theap->profiler_reserved1;
    if (mi_interval_update(bytes_since_last_sample, prof->alloc_interval_size, &prof->alloc_interval_countdown, pinterval_pending)) {
      mi_profiler_snapshot(profiler);
    }    
  }
  if (prof->inuse_interval_size > 0) { // snapshot whenever (sampled) in-use memory has grown by `inuse_interval_size` bytes
    const size_t total_inuse_bytes = mi_atomic_add_relaxed(&prof->inuse_bytes, alloc_size) + alloc_size;
    if (mi_inuse_threshold_update(total_inuse_bytes, prof->inuse_interval_size, &prof->inuse_interval_threshold)) {
      mi_profiler_snapshot(profiler);
    }
  }
  if (prof->time_interval_secs > 0) { // snapshot whenever `time_interval_secs` seconds have passed
    if (mi_time_threshold_update(_mi_clock_now(), prof->time_interval_secs, &prof->time_interval_deadline)) {
      mi_profiler_snapshot(profiler);
    }
  }
  return new_threshold;
}

// Sample free event and update profiling data accordingly
static void mi_cdecl on_free(mi_profiler_t* profiler, mi_profiler_sample_data_t* data, void* ptr, mi_heap_t* heap) 
{
  MI_UNUSED(heap); MI_UNUSED(ptr);
  mi_pprof_profiler_t* prof = downcast(profiler);
  if (data!=NULL) {
    mi_assert(data->user_data_size >= 2*sizeof(void*));
    mi_location_t* loc = (mi_location_t*)data->user_data[0];
    const size_t freed_bytes = (size_t)((uintptr_t)data->user_data[1]);
    if (loc!=NULL) {
      // different threads can free allocations that share the same (allocating-thread,
      // callstack) location concurrently, so these updates must be atomic.
      mi_atomic_sub_relaxed(&loc->inuse_bytes, freed_bytes);
      mi_atomic_decrement_relaxed(&loc->inuse_count);
    }
    if (prof->inuse_interval_size > 0) {  // keep the (global) in-use total in sync while interval-based snapshotting is enabled
      mi_atomic_sub_relaxed(&prof->inuse_bytes, freed_bytes);
    }
  }
}

static void mi_cdecl on_snapshot(mi_profiler_t* profiler) {
  mi_pprof_profiler_t* prof = downcast(profiler);
  mi_pprof_profiler_snapshot(prof);
}

// Create a new profiler. If `base_file_name` ends with a recognized extension (`.heap` or `.text`),
// that extension selects the text snapshot format and is stripped from the stored base file name
// (the actual snapshot files always get their own `.<seq>.heap`/`.<seq>.pb` extension, see `mi_pprof_profiler_snapshot`).
// Any other extension (or none) keeps the default (protobuf) snapshot format and is left untouched.
mi_profiler_t* mi_pprof_profiler_new(size_t initial_threshold, const char* base_file_name, size_t alloc_interval_size, size_t inuse_interval_size, size_t time_interval_secs) {
  // heap just for the profiler itself
  mi_heap_t* heap = mi_heap_new();
  mi_heap_profile_disable(heap);  // don't sample allocations in this heap

  // allocate and initialize the profiler structure from this heap
  mi_pprof_profiler_t* prof = mi_heap_zalloc_tp(mi_pprof_profiler_t,heap);
  if (prof == NULL) return NULL;
  prof->sample_threshold = initial_threshold;
  prof->profile_heap = heap;
  prof->base_file_name = (base_file_name != NULL ? mi_heap_strndup(heap, base_file_name, 1024) : NULL);
  prof->format_text = false;  // default: protobuf format, unless overridden by a recognized extension below
  if (prof->base_file_name != NULL) {
    // only look for a '.' extension after the last path separator (if any)
    const char* start = prof->base_file_name;
    const char* sep = _mi_strrchr(start, '/');
    if (sep != NULL) start = sep + 1;
    sep = _mi_strrchr(start, '\\');
    if (sep != NULL) start = sep + 1;
    const char* dot = _mi_strrchr(start, '.');
    if (dot != NULL) {
      if (_mi_streq(dot + 1, "heap") || _mi_streq(dot + 1, "text")) {
        prof->format_text = true;
      }
      prof->base_file_name[dot - prof->base_file_name] = 0;  // always strip the extension, whether recognized or not
    }
  }
  prof->alloc_interval_size = (alloc_interval_size > (size_t)MI_SSIZE_MAX ? (size_t)MI_SSIZE_MAX : alloc_interval_size);  // cap so it always fits in a `mi_ssize_t`
  mi_atomic_storess_relaxed(&prof->alloc_interval_countdown, (mi_ssize_t)prof->alloc_interval_size);  // 0 if disabled: `on_alloc` never decrements/checks in that case
  prof->inuse_interval_size = inuse_interval_size;
  mi_atomic_store_relaxed(&prof->inuse_interval_threshold, prof->inuse_interval_size);  // 0 if disabled (`inuse_interval_size` defaults to 0): `on_alloc` never checks in that case
  prof->time_interval_secs = time_interval_secs;
  mi_atomic_storei64_relaxed(&prof->time_interval_deadline, _mi_clock_now() + (mi_msecs_t)prof->time_interval_secs * 1000);  // deadline stays unreachable (now, i.e. always due) if disabled, but `on_alloc` never checks in that case
  if (!mi_locations_init(heap, &prof->locations)) {
    mi_free(prof);
    return NULL;
  }
  prof->profiler.initial_sample_rate = initial_threshold;
  prof->profiler.sample_data_size = 2*sizeof(void*);  // we store the location and the allocation size in the sample data
  prof->profiler.on_alloc = &on_alloc;
  prof->profiler.on_free = &on_free;
  prof->profiler.on_snapshot = &on_snapshot;
  return &prof->profiler;
}

void mi_pprof_profiler_delete(mi_profiler_t* profiler) {
  if (profiler == NULL) return;
  mi_pprof_profiler_t* prof = downcast(profiler);
  mi_heap_t* heap = prof->profile_heap;
  mi_locations_done(heap, &prof->locations);
  mi_free(prof->base_file_name);
  mi_free(prof);
  mi_heap_delete(heap);
}

// ---------------------------------------------------------------------------
// A basic hash table of locations. All memory (the bucket array, the
// locations, and their callstack frames) is allocated from the profiler's
// own heap so it does not interfere with the profiled allocations.
// Collisions are resolved with simple chaining.
//
// `on_alloc` and `on_free` (and thus `mi_locations_find_or_insert`) can run
// concurrently on multiple threads and are made thread-safe without a lock:
// a bucket is a singly-linked list that only ever grows by prepending a new,
// fully initialized location; once published, a location's identifying
// fields (`hash`, `thread_id`, `callstack`) never change again. Moreover,
// since the hash (and thus the location itself) is derived from the calling
// thread's own id, a location is only ever looked up by the same thread
// that creates it, so there is never a concurrent insert race for the
// same location. 
// ---------------------------------------------------------------------------

// A reasonably large prime bucket count to keep collision chains short 
#define MI_LOCATIONS_BUCKET_COUNT  (16 * 1024 - 3)  /* 16381, prime */

// FNV-1a hash, mixed over the thread id and the (precomputed) callstack hash;
#if SIZE_MAX > UINT32_MAX
#define MI_FNV_OFFSET_BASIS  ((size_t)0xcbf29ce484222325ULL)  // FNV-1a 64-bit offset basis
#define MI_FNV_PRIME         ((size_t)0x100000001b3ULL)       // FNV-1a 64-bit prime
#else
#define MI_FNV_OFFSET_BASIS  ((size_t)0x811c9dc5UL)           // FNV-1a 32-bit offset basis
#define MI_FNV_PRIME         ((size_t)0x01000193UL)           // FNV-1a 32-bit prime
#endif

static size_t mi_location_hash(mi_threadid_t thread_id, size_t callstack_hash) {
  size_t hash = MI_FNV_OFFSET_BASIS;
  const size_t prime = MI_FNV_PRIME;
  hash = (hash ^ (size_t)thread_id) * prime;
  hash = (hash ^ callstack_hash) * prime;
  return hash;
}

// Initialize a location hash table
static bool mi_locations_init(mi_heap_t* heap, mi_locations_t* locations) {
  locations->bucket_count = MI_LOCATIONS_BUCKET_COUNT;
  locations->buckets = (_Atomic(mi_location_t*)*)mi_heap_zalloc(heap, locations->bucket_count * sizeof(*locations->buckets));
  mi_atomic_store_relaxed(&locations->count, (size_t)0);
  return (locations->buckets != NULL);
}

// Free all memory associated with the location hash table.
// Not thread-safe: assumes no concurrent `on_alloc`/`on_free` (see the note above).
static void mi_locations_done(mi_heap_t* heap, mi_locations_t* locations) {
  MI_UNUSED(heap);
  if (locations->buckets == NULL) return;
  for (size_t i = 0; i < locations->bucket_count; i++) {
    mi_location_t* loc = mi_atomic_load_ptr_relaxed(mi_location_t, &locations->buckets[i]);
    while (loc != NULL) {
      mi_location_t* next = mi_atomic_load_ptr_relaxed(mi_location_t, &loc->next);
      if (loc->callstack.frames != NULL) {
        mi_free(loc->callstack.frames);
      }
      mi_free(loc);
      loc = next;
    }
  }
  mi_free(locations->buckets);
  locations->buckets = NULL;
  mi_atomic_store_relaxed(&locations->count, (size_t)0);
}

// Match a location's identifying fields; these never change after publication.
static bool mi_location_matches(const mi_location_t* loc, size_t hash, mi_threadid_t thread_id, const mi_callstack_t* callstack) {
  return (loc->hash == hash && loc->thread_id == thread_id &&
          loc->callstack.count == callstack->count &&
          (callstack->count == 0 || mi_memcmp(loc->callstack.frames, callstack->frames, callstack->count * sizeof(void*)) == 0));
}

// Find an existing location matching `thread_id` and `callstack`, or insert
// a fresh, zero-initialized one (with `hash` and identifying fields filled in)
// if none exists yet. Returns NULL only on allocation failure.
// Thread-safe (see the note above): the common case (an existing location)
// is a lock-free, read-only traversal using relaxed loads; only inserting a
// genuinely new location uses a CAS (and even then, only ever races against
// other threads inserting *different* locations into the same bucket).
static mi_location_t* mi_locations_find_or_insert(mi_heap_t* heap, mi_locations_t* locations, mi_threadid_t thread_id, const mi_callstack_t* callstack, size_t callstack_hash) {
  if (locations->buckets == NULL) return NULL;
  const size_t hash = mi_location_hash(thread_id, callstack_hash);
  const size_t idx = hash % locations->bucket_count;
  _Atomic(mi_location_t*)* bucket = &locations->buckets[idx];

  // fast path: lock-free, read-only lookup in the (possibly concurrently growing) chain
  for (mi_location_t* loc = mi_atomic_load_ptr_relaxed(mi_location_t, bucket); loc != NULL;
       loc = mi_atomic_load_ptr_relaxed(mi_location_t, &loc->next))
  {
    if (mi_location_matches(loc, hash, thread_id, callstack)) return loc;
  }

  // not found: allocate and fully initialize a new location; it is not yet
  // visible to any other thread so plain (non-atomic) stores are fine here.
  mi_location_t* loc = mi_heap_zalloc_tp(mi_location_t,heap);
  if (loc == NULL) return NULL;
  if (callstack->count > 0) {
    loc->callstack.frames = (void**)mi_heap_mallocn(heap, callstack->count, sizeof(void*));
    if (loc->callstack.frames == NULL) return NULL;
    mi_memcpy(loc->callstack.frames, callstack->frames, callstack->count * sizeof(void*));
  }
  loc->callstack.count = callstack->count;
  loc->thread_id = thread_id;
  loc->hash = hash;

  // publish the new location at the head of the bucket with a CAS loop. Since
  // a location's key includes the calling thread's own id, no other thread
  // can ever race to insert this *same* key -- a CAS can only fail because
  // some other thread prepended an unrelated location to this bucket in the
  // meantime, so we simply retry with the updated head (no duplicate check
  // needed, unlike a typical lock-free insert).
  mi_location_t* head = mi_atomic_load_ptr_relaxed(mi_location_t, bucket);
  do {
    mi_atomic_store_ptr_relaxed(mi_location_t, &loc->next, head);  // `loc` not yet published: plain store is fine
  } while (!mi_atomic_cas_ptr_weak_release(mi_location_t, bucket, &head, loc));  // `head` is updated to the current value on failure
  mi_atomic_increment_relaxed(&locations->count);  // just a statistic
  return loc;
}



// ---------------------------------------------------------------------------
// Avoid OS specific code when MI_PROFILE_USE_BUILTIN is not enabled
// ---------------------------------------------------------------------------

#if !MI_PROFILE_USE_BUILTIN

static size_t mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash) {
  MI_UNUSED(buffer);
  MI_UNUSED(max_depth);
  *hash = 0;
  return 0; 
}

void mi_pprof_profiler_snapshot(mi_pprof_profiler_t* profiler) {
  MI_UNUSED(profiler);
}

#else

#include <stdio.h>      // FILE, fopen, fprintf, fclose
#if defined(_WIN32)
#include <fcntl.h>      // _O_CREAT, _O_TRUNC, _O_WRONLY, _O_BINARY, _O_TEXT
#include <io.h>         // _sopen_s, _fdopen, _close
#include <share.h>      // _SH_DENYNO
#include <sys/stat.h>   // _S_IREAD, _S_IWRITE
#elif MI_HAS_UNISTDH
#include <fcntl.h>      // open, O_CREAT, O_EXCL, O_NOFOLLOW
#include <unistd.h>     // close, getuid, geteuid, getgid, getegid, issetugid
#include <sys/stat.h>   // S_IRUSR, S_IWUSR
#endif

// forward declarations; implemented further below.
static void mi_pprof_write_mapped_libraries(FILE* f);
static void mi_pprof_snapshot_text(mi_pprof_profiler_t* prof, const char* fname);
static void mi_pprof_snapshot_proto(mi_pprof_profiler_t* prof, const char* fname);


// Detect if we are running with elevated privileges (e.g. a setuid/setgid executable or similar), 
// in which case we must not trust `MIMALLOC_PROFILE` environment.
#if defined(_WIN32)
static bool mi_is_elevated_process(void) {
  return false;  // no direct setuid/setgid equivalent; a UAC elevation does not inherit the parent's environment
}
#elif MI_HAS_UNISTDH && (defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__))
static bool mi_is_elevated_process(void) {
  return (issetugid() != 0);
}
#elif MI_HAS_UNISTDH
static bool mi_is_elevated_process(void) {
  return (getuid() != geteuid() || getgid() != getegid());
}
#else
static bool mi_is_elevated_process(void) {
  return false;  // no portable way to check without `unistd.h`; assume not elevated
}
#endif


// Open a snapshot file for writing, refusing to follow a symlink at `fname` (to avoid an
// attacker redirecting the write to an arbitrary target) and restricting the file's
// permissions to the owner only (snapshots can contain sensitive address/callstack data).
// On platforms without these POSIX/Win32 facilities (e.g. wasm/Emscripten), we fall
// back to a fully portable plain `fopen` (without the symlink/permission hardening).
static FILE* mi_pprof_fopen(const char* fname, bool binary) {
  #if defined(_WIN32)
    int oflag = _O_CREAT | _O_TRUNC | _O_WRONLY | (binary ? _O_BINARY : _O_TEXT);
    int fd = -1;
    if (_sopen_s(&fd, fname, oflag, _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0 || fd < 0) return NULL;
    FILE* f = _fdopen(fd, binary ? "wb" : "w");
    if (f == NULL) { _close(fd); }
    return f;
  #elif MI_HAS_UNISTDH
    int oflag = O_CREAT | O_TRUNC | O_WRONLY;
    #if defined(O_NOFOLLOW)
    oflag |= O_NOFOLLOW;   // fail (ELOOP) if `fname` is a symlink
    #endif
    const int fd = open(fname, oflag, S_IRUSR | S_IWUSR);  // restrict to owner read/write only
    if (fd < 0) return NULL;
    FILE* f = fdopen(fd, binary ? "wb" : "w");
    if (f == NULL) { close(fd); }
    return f;
  #else
    return fopen(fname, binary ? "wb" : "w");  // fully portable fallback: no symlink/permission hardening available
  #endif
}

// Write out the current profiler data to a new snapshot file named
// `<base_file_name>.<seq>.<ext>` with an incrementing sequence number
// (`.heap` for the text format, mimicking the naming used by the original
// (gperftools) pprof heap profiler; `.pb` for the (uncompressed) protobuf
// format). The format is chosen when the profiler is created (see
// `mi_pprof_profiler_new`). The base file name is also set at creation time;
// if it is NULL, no snapshot is written and this function does nothing.
static void mi_pprof_profiler_snapshot(mi_pprof_profiler_t* prof) {
  if (prof == NULL) return;
  bool was_running = mi_profiler_stop(&prof->profiler);    
  mi_locations_t* locations = &prof->locations;
  if (locations->buckets != NULL && prof->base_file_name != NULL) {
    const size_t seq = ++prof->snapshot_count;
    char fname[1024];
    if (!prof->format_text) {
      _mi_snprintf(fname, sizeof(fname), "%s.%04zu.pb", prof->base_file_name, seq);
      mi_pprof_snapshot_proto(prof, fname);
    }
    else {
      _mi_snprintf(fname, sizeof(fname), "%s.%04zu.heap", prof->base_file_name, seq);
      mi_pprof_snapshot_text(prof, fname);
    }
    _mi_verbose_message("pprof profiler snapshot: %s\n", fname);
  }
  if (was_running) { mi_profiler_start(&prof->profiler); }
}

// ---------------------------------------------------------------------------
// Basic backtraces
// ---------------------------------------------------------------------------

#if MI_HAS_EXECINFOH || MI_HAS_LIBUNWINDH
static size_t mi_backtrace_hash(void** frames, size_t depth) {
  size_t hash = MI_FNV_OFFSET_BASIS;
  for (size_t i = 0; i < depth; i++) {
    hash = (hash ^ (size_t)((uintptr_t)frames[i])) * MI_FNV_PRIME;
  }
  return hash;
}
#endif

#if _WIN32
#include <windows.h>    // CaptureStackBackTrace
static mi_decl_noinline size_t mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash) {
  if (max_depth > ULONG_MAX) { max_depth = ULONG_MAX; }
  // On Windows XP/Server 2003, `FramesToSkip + FramesToCapture` must be less than 63;
  // clamp defensively so we never exceed this, regardless of the actual OS version.
  const size_t skip = 1 + MI_BACKTRACE_SKIP_FRAMES;  // +1 for `mi_prim_backtrace`'s own frame
  const size_t win_limit = 62;                       // strictly less than 63
  if (skip + max_depth >= win_limit) { max_depth = (skip <= win_limit ? win_limit - skip : 0); }
  if (max_depth == 0) { *hash = 0; return 0; }
  DWORD win_hash = 0;
  const size_t depth = (size_t)CaptureStackBackTrace((ULONG)skip, (ULONG)max_depth, buffer, &win_hash);
  *hash = (size_t)win_hash;  // use the hash computed by `CaptureStackBackTrace` itself
  return depth;
}
#elif MI_HAS_EXECINFOH
#include <execinfo.h>   // backtrace
static mi_decl_noinline size_t mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash) {
  const size_t skip = 1 + MI_BACKTRACE_SKIP_FRAMES;  // +1 for `mi_prim_backtrace`'s own frame
  if (max_depth <= skip) { *hash = 0; return 0; }
  if (max_depth > INT_MAX) { max_depth = INT_MAX; }
  const size_t raw_depth = (size_t)backtrace(buffer, (int)(max_depth - skip));
  if (raw_depth <= skip) { *hash = 0; return 0; }
  const size_t depth = raw_depth - skip;
  memmove(buffer, buffer + skip, depth * sizeof(void*));
  *hash = mi_backtrace_hash(buffer, depth);  // no platform-provided hash, compute our own
  return depth;
}
#elif MI_HAS_LIBUNWINDH
#define UNW_LOCAL_ONLY
#include <libunwind.h>  // unw_backtrace
static mi_decl_noinline size_t mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash) {
  // used as a fallback on platforms without `execinfo.h` (e.g. musl-based Linux like Alpine)
  const size_t skip = 1 + MI_BACKTRACE_SKIP_FRAMES;  // +1 for `mi_prim_backtrace`'s own frame
  if (max_depth <= skip) { *hash = 0; return 0; }
  if (max_depth > INT_MAX) { max_depth = INT_MAX; }
  const size_t raw_depth = (size_t)unw_backtrace(buffer, (int)(max_depth - skip));
  if (raw_depth <= skip) { *hash = 0; return 0; }
  const size_t depth = raw_depth - skip;
  memmove(buffer, buffer + skip, depth * sizeof(void*));
  *hash = mi_backtrace_hash(buffer, depth);  // no platform-provided hash, compute our own
  return depth;
}
#else
static size_t mi_prim_backtrace(void** buffer, size_t max_depth, size_t* hash) {
  MI_UNUSED(buffer);
  MI_UNUSED(max_depth);
  *hash = 0;
  return 0;
}
#endif


// ---------------------------------------------------------------------------
// Write out mapped libraries
// ---------------------------------------------------------------------------

// A loaded module's address range and (if known) its file path; used both to
// print the `MAPPED_LIBRARIES` trailer of the text format and to build the
// `Mapping` entries of the protobuf format (see `mi_pprof_snapshot_proto`).
typedef struct {
  uintptr_t start;
  uintptr_t end;
  char*     path;  // heap-allocated (owned), or NULL if unknown
} mi_pprof_module_t;

typedef struct {
  mi_pprof_module_t* modules;
  size_t              count;
  size_t              capacity;
} mi_pprof_modules_t;

static void mi_pprof_modules_add(mi_heap_t* heap, mi_pprof_modules_t* mods, uintptr_t start, uintptr_t end, const char* path) {
  if (mods->count == mods->capacity) {
    const size_t new_capacity = (mods->capacity == 0 ? 16 : mods->capacity * 2);
    mods->modules = (mi_pprof_module_t*)mi_heap_realloc(heap, mods->modules, new_capacity * sizeof(*mods->modules));
    mods->capacity = new_capacity;
  }
  mi_pprof_module_t* m = &mods->modules[mods->count++];
  m->start = start;
  m->end = end;
  m->path = (path != NULL ? mi_heap_strndup(heap, path, 1024) : NULL);
}

static void mi_pprof_modules_done(mi_pprof_modules_t* mods) {
  for (size_t i = 0; i < mods->count; i++) { mi_free(mods->modules[i].path); }
  mi_free(mods->modules);
  mods->modules = NULL;
  mods->count = 0;
  mods->capacity = 0;
}

// On Linux, `pprof` can symbolize addresses using the `MAPPED_LIBRARIES` trailer
// which is just a copy of `/proc/self/maps`. On macOS there is no equivalent
// file so we reconstruct a similarly formatted line (`start-end perm ... path`)
// per loaded Mach-O image using the dyld APIs: the range is taken from the
// image's `__TEXT` segment, which is also the "load address" that tools like
// `atos` use to symbolize addresses (`atos -o <path> -l <start> <addr>`). On
// Windows we walk the process address space with `VirtualQuery` 
// On other platforms we leave this section empty (`pprof` can still work
// without it if not using PIE, or when combined with tools like `atos` /
// `addr2line` / a symbol server).

// A small wrapper that formats with our own minimal `_mi_vsnprintf` (so call
// sites can use its `%l8x`/`%l8d`/`%tx` extensions instead of <inttypes.h>'s
// `PRIx64`/`PRIu64`/`PRIxPTR` macros) and writes the result directly to a `FILE*`.
// (Distinct from the existing `_mi_fprintf(mi_output_fun*, void*, ...)`  in
// `util/options.c`, which targets mimalloc's redirectable output callback instead
// of an arbitrary `FILE*`.)
static void mi_fprintf(FILE* f, const char* fmt, ...) {
  char buf[512];
  va_list args;
  va_start(args, fmt);
  _mi_vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  fputs(buf, f);
}

#if defined(_WIN32)
#include <windows.h>

// Walk the process address space, calling `on_module(ctx, base, end, path)`
// once for each loaded module. Consecutive `MEM_IMAGE` regions that belong to
// the same module (same allocation base) are merged into a single range.
static void mi_walk_modules_win32(void (*on_module)(void* ctx, uintptr_t base, uintptr_t end, const char* path), void* ctx) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  uint8_t* addr = (uint8_t*)si.lpMinimumApplicationAddress;
  uint8_t* const addr_max = (uint8_t*)si.lpMaximumApplicationAddress;
  void* cur_base = NULL;
  uintptr_t cur_end = 0;
  while (addr < addr_max) {
    MEMORY_BASIC_INFORMATION mbi;
    const SIZE_T n = VirtualQuery(addr, &mbi, sizeof(mbi));
    if (n == 0) break;  // no more (queryable) regions
    if (mbi.Type == MEM_IMAGE && mbi.State != MEM_FREE) {
      if (mbi.AllocationBase == cur_base) {
        // extend the current module's range
        cur_end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
      }
      else {
        // a new module starts: flush the previous one first
        if (cur_base != NULL) {
          char path[MAX_PATH];
          const DWORD len = GetModuleFileNameA((HMODULE)cur_base, path, (DWORD)sizeof(path));
          on_module(ctx, (uintptr_t)cur_base, cur_end, (len > 0 ? path : ""));
        }
        cur_base = mbi.AllocationBase;
        cur_end  = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
      }
    }
    else if (cur_base != NULL) {
      // the module's regions ended: flush it
      char path[MAX_PATH];
      const DWORD len = GetModuleFileNameA((HMODULE)cur_base, path, (DWORD)sizeof(path));
      on_module(ctx, (uintptr_t)cur_base, cur_end, (len > 0 ? path : ""));
      cur_base = NULL;
    }
    const uint8_t* next = (const uint8_t*)mbi.BaseAddress + mbi.RegionSize;
    if (next <= addr) break;  // guard against a non-advancing region
    addr = (uint8_t*)next;
  }
  if (cur_base != NULL) {
    char path[MAX_PATH];
    const DWORD len = GetModuleFileNameA((HMODULE)cur_base, path, (DWORD)sizeof(path));
    on_module(ctx, (uintptr_t)cur_base, cur_end, (len > 0 ? path : ""));
  }
}

static void mi_write_mapped_libraries_on_module_win32(void* ctx, uintptr_t base, uintptr_t end, const char* path) {
  FILE* f = (FILE*)ctx;
  mi_fprintf(f, "%tx-%tx r-xp 00000000 00:00 0            %s\n", base, end, path);
}

static void mi_write_mapped_libraries_win32(FILE* f) {
  mi_walk_modules_win32(&mi_write_mapped_libraries_on_module_win32, f);
}

typedef struct { mi_heap_t* heap; mi_pprof_modules_t* mods; } mi_pprof_collect_modules_win32_ctx_t;

static void mi_pprof_collect_modules_win32_on_module(void* ctx, uintptr_t base, uintptr_t end, const char* path) {
  mi_pprof_collect_modules_win32_ctx_t* c = (mi_pprof_collect_modules_win32_ctx_t*)ctx;
  mi_pprof_modules_add(c->heap, c->mods, base, end, (path[0] != 0 ? path : NULL));
}

static void mi_pprof_collect_modules_win32(mi_heap_t* heap, mi_pprof_modules_t* mods) {
  mi_pprof_collect_modules_win32_ctx_t ctx = { heap, mods };
  mi_walk_modules_win32(&mi_pprof_collect_modules_win32_on_module, &ctx);
}

#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

// Find the (slid) virtual address range of the `__TEXT` segment of a loaded
// Mach-O image; this is the image's load address as used for symbolication.
static bool mi_macho_image_range(const struct mach_header* mh, intptr_t slide, uintptr_t* start, uintptr_t* end) {
  const bool is64 = (mh->magic == MH_MAGIC_64 || mh->magic == MH_CIGAM_64);
  const uint8_t* cmd_ptr = (const uint8_t*)mh + (is64 ? sizeof(struct mach_header_64) : sizeof(struct mach_header));
  for (uint32_t c = 0; c < mh->ncmds; c++) {
    const struct load_command* lc = (const struct load_command*)cmd_ptr;
    if (is64 && lc->cmd == LC_SEGMENT_64) {
      const struct segment_command_64* seg = (const struct segment_command_64*)lc;
      if (_mi_streq(seg->segname, SEG_TEXT)) {
        *start = (uintptr_t)seg->vmaddr + (uintptr_t)slide;
        *end   = *start + (uintptr_t)seg->vmsize;
        return true;
      }
    }
    else if (!is64 && lc->cmd == LC_SEGMENT) {
      const struct segment_command* seg = (const struct segment_command*)lc;
      if (_mi_streq(seg->segname, SEG_TEXT)) {
        *start = (uintptr_t)seg->vmaddr + (uintptr_t)slide;
        *end   = *start + (uintptr_t)seg->vmsize;
        return true;
      }
    }
    cmd_ptr += lc->cmdsize;
  }
  return false;  // no __TEXT segment found
}

static void mi_write_mapped_libraries_macos(FILE* f) {
  const uint32_t count = _dyld_image_count();
  for (uint32_t i = 0; i < count; i++) {
    const struct mach_header* mh = _dyld_get_image_header(i);
    const char* name = _dyld_get_image_name(i);
    if (mh == NULL || name == NULL) continue;
    const intptr_t slide = _dyld_get_image_vmaddr_slide(i);
    uintptr_t start = 0;
    uintptr_t end = 0;
    if (!mi_macho_image_range(mh, slide, &start, &end)) continue;
    mi_fprintf(f, "%tx-%tx r-xp 00000000 00:00 0            %s\n", start, end, name);
  }
}

static void mi_pprof_collect_modules_macos(mi_heap_t* heap, mi_pprof_modules_t* mods) {
  const uint32_t count = _dyld_image_count();
  for (uint32_t i = 0; i < count; i++) {
    const struct mach_header* mh = _dyld_get_image_header(i);
    const char* name = _dyld_get_image_name(i);
    if (mh == NULL || name == NULL) continue;
    const intptr_t slide = _dyld_get_image_vmaddr_slide(i);
    uintptr_t start = 0;
    uintptr_t end = 0;
    if (!mi_macho_image_range(mh, slide, &start, &end)) continue;
    mi_pprof_modules_add(heap, mods, start, end, name);
  }
}

#elif defined(__linux__)
#include <stdlib.h>   // strtoull

// Copy `/proc/self/maps` verbatim; this is exactly the format `pprof` expects.
static void mi_write_mapped_libraries_linux(FILE* f) {
  FILE* maps = fopen("/proc/self/maps", "r");
  if (maps != NULL) {
    char line[512];
    while (fgets(line, sizeof(line), maps) != NULL) {
      fputs(line, f);
    }
    fclose(maps);
  }
}

// Parse `/proc/self/maps` lines of the form
// `<start>-<end> <perms> <offset> <dev> <inode>  <path>` (path may be absent
// for anonymous mappings, which are skipped here since they are not useful
// for symbolization).
static void mi_pprof_collect_modules_linux(mi_heap_t* heap, mi_pprof_modules_t* mods) {
  FILE* maps = fopen("/proc/self/maps", "r");
  if (maps == NULL) return;
  char line[512];
  while (fgets(line, sizeof(line), maps) != NULL) {
    char* end_ptr = NULL;
    const uintptr_t start = (uintptr_t)strtoull(line, &end_ptr, 16);
    if (end_ptr == NULL || *end_ptr != '-') continue;
    const uintptr_t end = (uintptr_t)strtoull(end_ptr + 1, &end_ptr, 16);
    // the path (if present) is the last whitespace-separated field on the line
    const char* path = strrchr(line, ' ');
    if (path == NULL) continue;
    while (*path == ' ') path++;
    size_t len = strlen(path);
    while (len > 0 && (path[len-1] == '\n' || path[len-1] == '\r')) { len--; }
    if (len == 0 || path[0] == '[') continue;  // skip anonymous mappings like `[heap]`, `[stack]`
    char pathbuf[512];
    if (len >= sizeof(pathbuf)) { len = sizeof(pathbuf) - 1; }
    memcpy(pathbuf, path, len);
    pathbuf[len] = 0;
    mi_pprof_modules_add(heap, mods, start, end, pathbuf);
  }
  fclose(maps);
}

#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/user.h>   // struct kinfo_vmentry
#include <libutil.h>    // kinfo_getvmmap
#include <unistd.h>     // getpid
#include <stdlib.h>     // free

// FreeBSD (and DragonFly, which forked from FreeBSD) do not reliably provide Linux's
// `/proc/self/maps` (procfs is optional and rarely mounted by default); use the native
// `kinfo_getvmmap` API instead (declared in <libutil.h>, requires linking `-lutil`) to
// enumerate the process's virtual memory mappings.
static void mi_write_mapped_libraries_bsd(FILE* f) {
  int cnt = 0;
  struct kinfo_vmentry* vmmap = kinfo_getvmmap(getpid(), &cnt);
  if (vmmap == NULL) return;
  for (int i = 0; i < cnt; i++) {
    const struct kinfo_vmentry* kve = &vmmap[i];
    if (kve->kve_path[0] == 0) continue;  // skip anonymous mappings
    mi_fprintf(f, "%l8x-%l8x r-xp 00000000 00:00 0            %s\n", (uint64_t)kve->kve_start, (uint64_t)kve->kve_end, kve->kve_path);
  }
  free(vmmap);
}

static void mi_pprof_collect_modules_bsd(mi_heap_t* heap, mi_pprof_modules_t* mods) {
  int cnt = 0;
  struct kinfo_vmentry* vmmap = kinfo_getvmmap(getpid(), &cnt);
  if (vmmap == NULL) return;
  for (int i = 0; i < cnt; i++) {
    const struct kinfo_vmentry* kve = &vmmap[i];
    if (kve->kve_path[0] == 0) continue;  // skip anonymous mappings
    mi_pprof_modules_add(heap, mods, (uintptr_t)kve->kve_start, (uintptr_t)kve->kve_end, kve->kve_path);
  }
  free(vmmap);
}
#endif

static void mi_pprof_collect_modules(mi_heap_t* heap, mi_pprof_modules_t* mods) {
  mods->modules = NULL;
  mods->count = 0;
  mods->capacity = 0;
  #if defined(__linux__)
  mi_pprof_collect_modules_linux(heap, mods);
  #elif defined(_WIN32)
  mi_pprof_collect_modules_win32(heap, mods);
  #elif defined(__APPLE__)
  mi_pprof_collect_modules_macos(heap, mods);
  #elif defined(__FreeBSD__) || defined(__DragonFly__)
  mi_pprof_collect_modules_bsd(heap, mods);
  #else
  MI_UNUSED(heap);
  #endif
}

static void mi_pprof_write_mapped_libraries(FILE* f) {
  fprintf(f, "MAPPED_LIBRARIES:\n");
  #if defined(__linux__)
  mi_write_mapped_libraries_linux(f);
  #elif defined(_WIN32)
  mi_write_mapped_libraries_win32(f);
  #elif defined(__APPLE__)
  mi_write_mapped_libraries_macos(f);
  #elif defined(__FreeBSD__) || defined(__DragonFly__)
  mi_write_mapped_libraries_bsd(f);
  #endif
}


// ---------------------------------------------------------------------------
// Writing the profile in the (original, textual) pprof heap profile format
// ---------------------------------------------------------------------------

// Write out the current profiler data in the pprof heap profile text format:
// a header line with the totals, followed by one line per unique call
// location, and a trailer with the mapped libraries (used by `pprof` to
// symbolize the addresses).
static void mi_pprof_snapshot_text(mi_pprof_profiler_t* prof, const char* fname) {
  mi_locations_t* locations = &prof->locations;
  FILE* f = mi_pprof_fopen(fname, false);
  if (f == NULL) return;

  // first pass: compute the totals over all locations
  // (snapshot is assumed to run single-threaded while sampling is stopped, so
  // `alloc_count`/`alloc_bytes` can be read directly; `inuse_count`/`inuse_bytes`
  // are still `_Atomic` fields so we use a relaxed load for those)
  uint64_t total_inuse_objects = 0;
  uint64_t total_inuse_bytes   = 0;
  uint64_t total_alloc_objects = 0;
  uint64_t total_alloc_bytes   = 0;
  for (size_t i = 0; i < locations->bucket_count; i++) {
    for (mi_location_t* loc = mi_atomic_load_ptr_relaxed(mi_location_t, &locations->buckets[i]); loc != NULL; loc = mi_atomic_load_ptr_relaxed(mi_location_t, &loc->next)) {
      total_inuse_objects += (uint64_t)mi_atomic_load_relaxed(&loc->inuse_count);
      total_inuse_bytes   += (uint64_t)mi_atomic_load_relaxed(&loc->inuse_bytes);
      total_alloc_objects += (uint64_t)loc->alloc_count;
      total_alloc_bytes   += (uint64_t)loc->alloc_bytes;
    }
  }

  mi_fprintf(f, "heap profile: %6l8d: %8l8d [%6l8d: %8l8d] @ heapprofile\n",
             total_inuse_objects, total_inuse_bytes, total_alloc_objects, total_alloc_bytes);

  // second pass: write one line per unique call location
  for (size_t i = 0; i < locations->bucket_count; i++) {
    for (mi_location_t* loc = mi_atomic_load_ptr_relaxed(mi_location_t, &locations->buckets[i]); loc != NULL; loc = mi_atomic_load_ptr_relaxed(mi_location_t, &loc->next)) {
      const size_t inuse_count = mi_atomic_load_relaxed(&loc->inuse_count);
      const size_t inuse_bytes = mi_atomic_load_relaxed(&loc->inuse_bytes);
      mi_fprintf(f, "%6l8d: %8l8d [%6l8d: %8l8d] @",
                 (uint64_t)inuse_count, (uint64_t)inuse_bytes,
                 (uint64_t)loc->alloc_count, (uint64_t)loc->alloc_bytes);
      for (size_t j = 0; j < loc->callstack.count; j++) {
        mi_fprintf(f, " 0x%tx", (uintptr_t)loc->callstack.frames[j]);
      }
      fprintf(f, "\n");
    }
  }

  mi_pprof_write_mapped_libraries(f);
  fclose(f);
}


// ---------------------------------------------------------------------------
// Writing the profile in the modern `pprof` protobuf format
// (`perftools.profiles.Profile`, see
//  https://github.com/google/pprof/blob/main/proto/profile.proto)
//
// On-disk, `pprof` profiles are normally gzip-compressed, but `pprof` also
// accepts a *raw* (uncompressed) serialized `Profile` message: it only tries
// to gunzip the input if it starts with the gzip magic bytes, and otherwise
// parses it directly as a protobuf message. We take advantage of that so we
// don't need to implement a deflate/gzip writer.
//
// We still need the locations hash table for this format: samples are
// aggregated per call site (thread + callstack), exactly as for the text
// format, rather than emitting one `Sample` per individual allocation. What
// changes is the *representation* of a callstack: instead of a flat list of
// hex addresses embedded directly in a text line, each frame address becomes
// its own (deduplicated) `Location` entry, referenced by id from `Sample.
// location_id`. The originating thread id -- which the text format doesn't
// record explicitly (it is only implicit in which addresses got sampled) --
// is attached to each `Sample` via a numeric `Label` (pprof has no dedicated
// thread-id field, but labels are the generic mechanism for this).
// ---------------------------------------------------------------------------

// A simple growable byte buffer, allocated from the profiler's own heap.
typedef struct {
  mi_heap_t* heap;
  uint8_t*   data;
  size_t     size;
  size_t     capacity;
} mi_pb_buf_t;

static void mi_pb_init(mi_pb_buf_t* b, mi_heap_t* heap) {
  b->heap = heap;
  b->data = NULL;
  b->size = 0;
  b->capacity = 0;
}

static void mi_pb_done(mi_pb_buf_t* b) {
  mi_free(b->data);
  b->data = NULL;
  b->size = 0;
  b->capacity = 0;
}

static void mi_pb_reserve(mi_pb_buf_t* b, size_t extra) {
  if (b->size + extra <= b->capacity) return;
  size_t new_capacity = (b->capacity == 0 ? 4096 : b->capacity * 2);
  while (new_capacity < b->size + extra) { new_capacity *= 2; }
  b->data = (uint8_t*)mi_heap_realloc(b->heap, b->data, new_capacity);
  b->capacity = new_capacity;
}

static void mi_pb_bytes(mi_pb_buf_t* b, const void* p, size_t n) {
  mi_pb_reserve(b, n);
  memcpy(b->data + b->size, p, n);
  b->size += n;
}

static void mi_pb_byte(mi_pb_buf_t* b, uint8_t byte) {
  mi_pb_bytes(b, &byte, 1);
}

// Unsigned LEB128 (protobuf "varint") encoding.
static void mi_pb_varint(mi_pb_buf_t* b, uint64_t v) {
  do {
    uint8_t byte = (uint8_t)(v & 0x7F);
    v >>= 7;
    if (v != 0) { byte |= 0x80; }
    mi_pb_byte(b, byte);
  } while (v != 0);
}

static void mi_pb_tag(mi_pb_buf_t* b, uint32_t field, uint32_t wiretype) {
  mi_pb_varint(b, ((uint64_t)field << 3) | wiretype);
}

// Scalar varint fields; proto3 encoders may omit fields set to their default
// (0/false) value, which we do here to keep the output compact.
static void mi_pb_uint64_field(mi_pb_buf_t* b, uint32_t field, uint64_t v) {
  if (v == 0) return;
  mi_pb_tag(b, field, 0);
  mi_pb_varint(b, v);
}

static void mi_pb_int64_field(mi_pb_buf_t* b, uint32_t field, int64_t v) {
  if (v == 0) return;
  mi_pb_tag(b, field, 0);
  mi_pb_varint(b, (uint64_t)v);  // proto3 plain int64 is encoded as a varint of its 2's complement bit pattern
}

// Length-delimited fields (string/bytes/embedded message) are always emitted,
// even when empty: this matters for `string_table[0]` (which must be the
// empty string) and for repeated message fields (an empty message is still a
// valid, distinct list entry).
static void mi_pb_len_field(mi_pb_buf_t* b, uint32_t field, const void* data, size_t len) {
  mi_pb_tag(b, field, 2);
  mi_pb_varint(b, len);
  mi_pb_bytes(b, data, len);
}

static void mi_pb_message_field(mi_pb_buf_t* b, uint32_t field, const mi_pb_buf_t* msg) {
  mi_pb_len_field(b, field, msg->data, msg->size);
}

// A packed repeated scalar field (used for `Sample.location_id`/`Sample.value`):
// all elements are varint-encoded and concatenated into one length-delimited blob.
static void mi_pb_packed_uint64_field(mi_pb_buf_t* b, uint32_t field, const uint64_t* values, size_t count) {
  if (count == 0) return;
  mi_pb_buf_t tmp;
  mi_pb_init(&tmp, b->heap);
  for (size_t i = 0; i < count; i++) { mi_pb_varint(&tmp, values[i]); }
  mi_pb_message_field(b, field, &tmp);
  mi_pb_done(&tmp);
}

static void mi_pb_packed_int64_field(mi_pb_buf_t* b, uint32_t field, const int64_t* values, size_t count) {
  mi_pb_packed_uint64_field(b, field, (const uint64_t*)values, count);  // same varint bit pattern
}

// ---------------------------------------------------------------------------
// String table: dedups strings into `Profile.string_table` (field 6) and
// hands back the (1-based; index 0 is reserved for "") index of each string.
// A linear search is fine here: a profile typically only has a handful to a
// few hundred distinct strings (sample type names, mapped file paths).
// ---------------------------------------------------------------------------

typedef struct {
  char*   str;
  int64_t index;
} mi_pb_string_entry_t;

typedef struct {
  mi_heap_t*            heap;
  mi_pb_string_entry_t* entries;
  size_t                count;
  size_t                capacity;
  mi_pb_buf_t           table;  // the serialized `string_table` entries (Profile field 6)
} mi_pb_strings_t;

static void mi_pb_strings_init(mi_pb_strings_t* strings, mi_heap_t* heap) {
  strings->heap = heap;
  strings->entries = NULL;
  strings->count = 0;
  strings->capacity = 0;
  mi_pb_init(&strings->table, heap);
  mi_pb_len_field(&strings->table, 6, "", 0);  // index 0 must always be ""
}

static void mi_pb_strings_done(mi_pb_strings_t* strings) {
  for (size_t i = 0; i < strings->count; i++) { mi_free(strings->entries[i].str); }
  mi_free(strings->entries);
  mi_pb_done(&strings->table);
}

static int64_t mi_pb_strings_intern(mi_pb_strings_t* strings, const char* s) {
  if (s == NULL) { s = ""; }
  for (size_t i = 0; i < strings->count; i++) {
    if (_mi_streq(strings->entries[i].str, s)) { return strings->entries[i].index; }
  }
  if (strings->count == strings->capacity) {
    const size_t new_capacity = (strings->capacity == 0 ? 32 : strings->capacity * 2);
    strings->entries = (mi_pb_string_entry_t*)mi_heap_realloc(strings->heap, strings->entries, new_capacity * sizeof(*strings->entries));
    strings->capacity = new_capacity;
  }
  const int64_t index = (int64_t)(strings->count + 1);  // index 0 is reserved for ""
  strings->entries[strings->count].str = mi_heap_strndup(strings->heap, s, 1024);
  strings->entries[strings->count].index = index;
  strings->count++;
  mi_pb_len_field(&strings->table, 6, s, strlen(s));
  return index;
}

// ---------------------------------------------------------------------------
// A small single-threaded hash map from a raw frame address to the id of its
// (deduplicated) `Location` entry. Only ever built and consulted from within
// `mi_pprof_snapshot_proto`, so (unlike the locations hash table) it needs no
// atomics at all.
// ---------------------------------------------------------------------------

typedef struct mi_pb_addr_entry_s {
  struct mi_pb_addr_entry_s* next;
  uintptr_t                  addr;
  uint64_t                   id;
} mi_pb_addr_entry_t;

typedef struct {
  mi_heap_t*            heap;
  size_t                bucket_count;
  mi_pb_addr_entry_t**  buckets;
} mi_pb_addr_map_t;

static void mi_pb_addr_map_init(mi_pb_addr_map_t* map, mi_heap_t* heap, size_t bucket_count) {
  map->heap = heap;
  map->bucket_count = (bucket_count == 0 ? 1 : bucket_count);
  map->buckets = (mi_pb_addr_entry_t**)mi_heap_zalloc(heap, map->bucket_count * sizeof(*map->buckets));
}

static void mi_pb_addr_map_done(mi_pb_addr_map_t* map) {
  if (map->buckets == NULL) return;
  for (size_t i = 0; i < map->bucket_count; i++) {
    mi_pb_addr_entry_t* e = map->buckets[i];
    while (e != NULL) {
      mi_pb_addr_entry_t* const next = e->next;
      mi_free(e);
      e = next;
    }
  }
  mi_free(map->buckets);
  map->buckets = NULL;
}

// Returns the existing location id for `addr`, or assigns it `next_id` and
// returns that (setting `*is_new` to true) if this is the first time `addr`
// is seen.
static uint64_t mi_pb_addr_map_find_or_insert(mi_pb_addr_map_t* map, uintptr_t addr, uint64_t next_id, bool* is_new) {
  if (map->buckets == NULL) { *is_new = false; return 0; }
  const size_t idx = (size_t)(addr ^ (addr >> 16)) % map->bucket_count;
  for (mi_pb_addr_entry_t* e = map->buckets[idx]; e != NULL; e = e->next) {
    if (e->addr == addr) { *is_new = false; return e->id; }
  }
  mi_pb_addr_entry_t* e = mi_heap_malloc_tp(mi_pb_addr_entry_t, map->heap);
  e->addr = addr;
  e->id = next_id;
  e->next = map->buckets[idx];
  map->buckets[idx] = e;
  *is_new = true;
  return next_id;
}

// Find the (1-based) `Mapping` id of the module containing `addr`, or 0 if
// `addr` doesn't fall within any collected module range.
static uint64_t mi_pprof_find_mapping_id(const mi_pprof_modules_t* mods, uintptr_t addr) {
  for (size_t i = 0; i < mods->count; i++) {
    if (addr >= mods->modules[i].start && addr < mods->modules[i].end) { return (uint64_t)(i + 1); }
  }
  return 0;
}

// Write out the current profiler data as a raw (uncompressed) `perftools.
// profiles.Profile` protobuf message (see the file-level comment above).
static void mi_pprof_snapshot_proto(mi_pprof_profiler_t* prof, const char* fname) {
  mi_heap_t* heap = prof->profile_heap;
  mi_locations_t* locations = &prof->locations;

  mi_pb_buf_t profile;
  mi_pb_init(&profile, heap);
  mi_pb_strings_t strings;
  mi_pb_strings_init(&strings, heap);

  const int64_t str_count         = mi_pb_strings_intern(&strings, "count");
  const int64_t str_bytes         = mi_pb_strings_intern(&strings, "bytes");
  const int64_t str_alloc_objects = mi_pb_strings_intern(&strings, "alloc_objects");
  const int64_t str_alloc_bytes   = mi_pb_strings_intern(&strings, "alloc_bytes");
  const int64_t str_inuse_objects = mi_pb_strings_intern(&strings, "inuse_objects");
  const int64_t str_inuse_bytes   = mi_pb_strings_intern(&strings, "inuse_bytes");
  const int64_t str_space         = mi_pb_strings_intern(&strings, "space");
  const int64_t str_thread        = mi_pb_strings_intern(&strings, "thread");

  // Profile.sample_type = 1: [alloc_objects:count, alloc_bytes:bytes, inuse_objects:count, inuse_bytes:bytes]
  {
    const int64_t types[4] = { str_alloc_objects, str_alloc_bytes, str_inuse_objects, str_inuse_bytes };
    const int64_t units[4] = { str_count,         str_bytes,       str_count,         str_bytes };
    for (size_t i = 0; i < 4; i++) {
      mi_pb_buf_t vt;
      mi_pb_init(&vt, heap);
      mi_pb_int64_field(&vt, 1, types[i]);
      mi_pb_int64_field(&vt, 2, units[i]);
      mi_pb_message_field(&profile, 1, &vt);
      mi_pb_done(&vt);
    }
  }

  // Profile.mapping = 3: the modules loaded into this process, so `pprof` can
  // symbolize the (raw) addresses recorded in each `Location` below.
  mi_pprof_modules_t mods;
  mi_pprof_collect_modules(heap, &mods);
  for (size_t i = 0; i < mods.count; i++) {
    const mi_pprof_module_t* m = &mods.modules[i];
    mi_pb_buf_t mapping;
    mi_pb_init(&mapping, heap);
    mi_pb_uint64_field(&mapping, 1, (uint64_t)(i + 1));    // id
    mi_pb_uint64_field(&mapping, 2, (uint64_t)m->start);   // memory_start
    mi_pb_uint64_field(&mapping, 3, (uint64_t)m->end);     // memory_limit
    if (m->path != NULL) {
      mi_pb_int64_field(&mapping, 5, mi_pb_strings_intern(&strings, m->path));  // filename
    }
    mi_pb_message_field(&profile, 3, &mapping);
    mi_pb_done(&mapping);
  }

  // Profile.location = 4 and Profile.sample = 2: one (deduplicated) `Location`
  // per unique frame address, and one `Sample` per unique call site (i.e. one
  // per entry in our own locations hash table), exactly mirroring the text
  // format's aggregation.
  mi_pb_addr_map_t addr_map;
  mi_pb_addr_map_init(&addr_map, heap, locations->bucket_count);
  uint64_t next_location_id = 1;

  for (size_t i = 0; i < locations->bucket_count; i++) {
    for (mi_location_t* loc = mi_atomic_load_ptr_relaxed(mi_location_t, &locations->buckets[i]); loc != NULL; loc = mi_atomic_load_ptr_relaxed(mi_location_t, &loc->next)) {
      const size_t inuse_count = mi_atomic_load_relaxed(&loc->inuse_count);
      const size_t inuse_bytes = mi_atomic_load_relaxed(&loc->inuse_bytes);

      const size_t frame_count = (loc->callstack.count > MI_MAX_BACKTRACE_DEPTH ? MI_MAX_BACKTRACE_DEPTH : loc->callstack.count);
      uint64_t location_ids[MI_MAX_BACKTRACE_DEPTH];
      for (size_t j = 0; j < frame_count; j++) {
        const uintptr_t addr = (uintptr_t)loc->callstack.frames[j];
        bool is_new = false;
        const uint64_t id = mi_pb_addr_map_find_or_insert(&addr_map, addr, next_location_id, &is_new);
        location_ids[j] = id;
        if (is_new) {
          next_location_id++;
          mi_pb_buf_t location;
          mi_pb_init(&location, heap);
          mi_pb_uint64_field(&location, 1, id);                                    // id
          mi_pb_uint64_field(&location, 2, mi_pprof_find_mapping_id(&mods, addr));  // mapping_id
          mi_pb_uint64_field(&location, 3, (uint64_t)addr);                        // address
          mi_pb_message_field(&profile, 4, &location);
          mi_pb_done(&location);
        }
      }

      mi_pb_buf_t sample;
      mi_pb_init(&sample, heap);
      mi_pb_packed_uint64_field(&sample, 1, location_ids, frame_count);  // location_id (leaf-first, matching our capture order)
      const int64_t values[4] = {
        loc->alloc_count, loc->alloc_bytes,
        (int64_t)inuse_count, (int64_t)inuse_bytes
      };
      mi_pb_packed_int64_field(&sample, 2, values, 4);  // value
      {
        // attach the originating thread id as a numeric label: pprof has no
        // dedicated thread-id field, but `Sample.label` is the generic
        // mechanism for this kind of per-sample metadata.
        mi_pb_buf_t label;
        mi_pb_init(&label, heap);
        mi_pb_int64_field(&label, 1, str_thread);            // key
        mi_pb_int64_field(&label, 3, (int64_t)loc->thread_id); // num
        mi_pb_message_field(&sample, 3, &label);
        mi_pb_done(&label);
      }
      mi_pb_message_field(&profile, 2, &sample);
      mi_pb_done(&sample);
    }
  }

  mi_pb_addr_map_done(&addr_map);
  mi_pprof_modules_done(&mods);

  // Profile.period_type = 11 / Profile.period = 12: matches the convention
  // used by Go's heap profiles ("space"/"bytes", sampling period in bytes).
  {
    mi_pb_buf_t period_type;
    mi_pb_init(&period_type, heap);
    mi_pb_int64_field(&period_type, 1, str_space);
    mi_pb_int64_field(&period_type, 2, str_bytes);
    mi_pb_message_field(&profile, 11, &period_type);
    mi_pb_done(&period_type);
  }
  mi_pb_int64_field(&profile, 12, (int64_t)prof->sample_threshold);      // period
  mi_pb_int64_field(&profile, 9, (int64_t)_mi_clock_now() * 1000);       // clock_now is in milli-secs

  // the string table (Profile.string_table = 6) can only be finalized now,
  // since strings were interned into it while building the message above.
  mi_pb_bytes(&profile, strings.table.data, strings.table.size);
  mi_pb_strings_done(&strings);

  FILE* f = mi_pprof_fopen(fname, true);
  if (f != NULL) {
    fwrite(profile.data, 1, profile.size, f);
    fclose(f);
  }
  mi_pb_done(&profile);
}

#endif  // MI_PROFILE
