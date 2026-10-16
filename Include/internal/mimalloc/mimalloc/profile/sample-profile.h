/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_SAMPLE_PROFILE_H
#define MI_INTERNAL_SAMPLE_PROFILE_H

#include "../types.h"

mi_decl_restrict void* _mi_theap_malloc_sampled(mi_theap_t* theap, size_t req_size, bool zero, mi_page_t** ppage) mi_attr_noexcept;
size_t        _mi_theap_update_sample_rate(mi_theap_t* theap);

mi_decl_restrict void* _mi_theap_malloc_profiled(mi_theap_t* theap, size_t size, uint64_t requested_since_last_sample, bool zero, mi_page_t** ppage) mi_attr_noexcept;
void          _mi_page_profile_on_free(mi_page_t* page, mi_block_t* block, void* p);
size_t        _mi_theap_set_profile_sample_rate(mi_theap_t* theap, size_t sample_rate);

// permanently exclude a theap from profiling (mirrors `mi_heap_profile_disable` but at the theap level);
// used for detached/meta theaps used to bootstrap thread/theap metadata, since sampling those can call
// back into the profiler while allocating on a not yet (re-)initialized thread, causing deadlock.
static inline void mi_theap_profile_disable(mi_theap_t* theap) {
  theap->profile_disabled = true;
  theap->profile_sample_rate = 0;
  theap->profile_sample_countdown = 0;
}

/* -------------------------------------------------------------------
  Guarded and profiled objects
------------------------------------------------------------------- */

#define MI_SAMPLE_RATE_MAX        (SIZE_MAX/4)
#define MI_SAMPLE_COUNTDOWN_MAX   (MI_MAX_ALLOC_SIZE)

static inline bool mi_theap_should_sample(mi_theap_t* theap, size_t req_size) {
  // note: this should return `true` on an empty theap so we initialize it's countdown to `-1`.
  mi_assert_internal(req_size <= SIZE_MAX/2);
  // const size_t sample_countdown = theap->sample_countdown - req_size;
  // return ((mi_ssize_t)sample_countdown < 0);
  return ((mi_ssize_t)theap->sample_countdown < (mi_ssize_t)req_size);
}

// we always align guarded pointers in a block at an offset
// the block `next` field is then used as a tag to distinguish regular offset aligned blocks from guarded ones
#define MI_BLOCK_TAG_ALIGNED   ((mi_encoded_t)(0))
#define MI_BLOCK_TAG_PROFILED  ((mi_encoded_t)(1))


static inline bool mi_block_ptr_is_sampled(const mi_block_t* block, const void* p) {
#if MI_GUARDED || MI_PROFILE
  const ptrdiff_t offset = (uint8_t*)p - (uint8_t*)block;
  return (offset >= (ptrdiff_t)(sizeof(mi_block_t)) && block->next != MI_BLOCK_TAG_ALIGNED);
#else
  MI_UNUSED(block); MI_UNUSED(p);
  return false;
#endif
}

static inline bool mi_profiler_is_enabled(const mi_profiler_t* prof) {
  _Atomic(size_t)* penabled = (_Atomic(size_t)*)&prof->reserved;
  return (mi_atomic_load_acquire(penabled) != 0);
}

static inline bool mi_profiler_set_enabled(mi_profiler_t* prof, bool enable) {
  _Atomic(size_t)* penabled = (_Atomic(size_t)*)&prof->reserved;
  return (mi_atomic_exchange_release(penabled, (enable ? 1 : 0)) != 0);
}

#endif
