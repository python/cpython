/* ----------------------------------------------------------------------------
Copyright (c) 2024-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MIMALLOC_PROFILE_H
#define MIMALLOC_PROFILE_H

#include "mimalloc.h"
#include <stdbool.h>  // bool
#include <stdint.h>   // uint64_t

//---------------------------------------------------------------------------
// Initial support for profiling hooks; still experimental
//---------------------------------------------------------------------------

#define MI_PROFILE_SAMPLE_DATA_MAX_SIZE  (1024)

// User defined profiler; see `test_profile.c` for an example.
typedef struct mi_profiler_s mi_profiler_t;

// Profiler data is stored together with each sampled allocation (unless the `on_free` field in the profiler is NULL.)
typedef struct mi_profiler_sample_data_s {
  size_t user_data_size;      // size of the user_data (should be the `mi_profiler_t.sample_data_size`)
  void*  user_data[1];        // default, but can be less or more (up to 1KiB), depending on `sample_data_size`
} mi_profiler_sample_data_t;

// Profiling callback invoked on each sampled allocation.
// If `profiler_data!=NULL` (i.e. when `on_free` is not NULL), then `profiler_data->requested_size == requested_size`.
typedef size_t (mi_cdecl mi_profiler_on_alloc_fun)(mi_profiler_t* profiler, mi_profiler_sample_data_t* profiler_data, void* ptr, size_t requested_size, size_t bytes_sample_rate, uint64_t bytes_since_last_sample, mi_heap_t* heap);

// Profiling callback invoked on each sampled in-place re-allocation.
typedef size_t (mi_cdecl mi_profiler_on_realloc_inplace_fun)(mi_profiler_t* profiler, mi_profiler_sample_data_t* profiler_data, void* ptr, size_t old_size, mi_heap_t* heap);

// Profiling callback invoked on a previously sampled allocation.
typedef void   (mi_cdecl mi_profiler_on_free_fun)(mi_profiler_t* profiler, mi_profiler_sample_data_t* profiler_data, void* ptr, mi_heap_t* heap);

// Profiling callback invoked on mi_profiler_snapshot()
typedef void   (mi_cdecl mi_profiler_on_snapshot_fun)(mi_profiler_t* profiler);

// A profiler
// All fields are considered immutable such that they can be copied and accessed concurrently. All fields can be NULL/0.
struct mi_profiler_s {
  void*                         reserved;           // opaque; reserved by mimalloc
  size_t                        sample_data_size;   // size of required profiler data for each sampled allocation (or zero for no data)
  size_t                        initial_sample_rate;// initial sample rate in bytes (set to at least 1 or higher) (can be adjusted by `on_alloc`)
  mi_profiler_on_alloc_fun*     on_alloc;           // called on a sampled allocation (may be called concurrently)
  mi_profiler_on_free_fun*      on_free;            // called on when previous sampled allocation is freed (may be called concurrently)
  mi_profiler_on_realloc_inplace_fun* on_realloc_inplace;  // (as yet unused) called on in-place reallocation of a previous sampled allocation (may be called concurrently)
  mi_profiler_on_snapshot_fun*  on_snapshot;        // called on mi_profiler_snapshot() (only one thread at a time)
  // ... more user fields allowed
};

// Exported definitions
#ifdef __cplusplus
extern "C" {
#endif

// attach a profiler to a particular heap only.
mi_decl_export bool mi_heap_profile(mi_heap_t* heap, mi_profiler_t* profiler);

// disable profiling for a particular heap; useful for a heap that the profiler uses itself for metadata.
mi_decl_export void mi_heap_profile_disable(mi_heap_t* heap);

// attach a profiler to any (current and future) heaps in a sub-process (unless those heaps disabled profiling)
mi_decl_export bool mi_subproc_profile(mi_subproc_id_t subproc_id, mi_profiler_t* profiler);

// attach a profiler to any heaps in the main sub-process (use NULL to detach)
mi_decl_export bool mi_profile(mi_profiler_t* profiler);

// start sampling
mi_decl_export bool mi_profiler_start(mi_profiler_t* profiler);

// end sampling
mi_decl_export bool mi_profiler_stop(mi_profiler_t* profiler);

// snapshot the current profile (may be called concurrently but only one thread will actually call the snapshot callback)
mi_decl_export void mi_profiler_snapshot(mi_profiler_t* profiler);

// builtin pprof compatible profiler
// the builtin profiler can also be activated using environment variables (see [`pprof.c`](/src/profile/pprof.c))
// `base_file_name` selects the text snapshot format if it ends in `.heap` or `.text` (the extension is
// stripped); any other extension, or none, uses the default (protobuf) snapshot format.
// `alloc_interval_size`, if >0, automatically snapshots every that many allocated bytes.
// `inuse_interval_size`, if >0, automatically snapshots every time the in-use bytes grow by that many bytes.
// `time_interval_secs`, if >0, automatically snapshots every that many seconds (checked from allocation events).
mi_decl_export mi_profiler_t* mi_pprof_profiler_new(size_t initial_threshold, const char* base_file_name, size_t alloc_interval_size, size_t inuse_interval_size, size_t time_interval_secs);
mi_decl_export void mi_pprof_profiler_delete(mi_profiler_t* profiler);


#ifdef __cplusplus
}
#endif

#endif // MIMALLOC_PROFILE_H
