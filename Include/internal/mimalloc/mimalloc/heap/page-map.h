/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_PAGE_MAP_H
#define MI_INTERNAL_PAGE_MAP_H

#include "../types.h"
#include "../util/libc.h"
#include "../util/options.h"

bool          _mi_page_map_init(void);
mi_decl_nodiscard bool _mi_page_map_register(mi_page_t* page);
void          _mi_page_map_unregister(mi_page_t* page);
void          _mi_page_map_unregister_range(void* start, size_t size);
void          _mi_page_map_unsafe_destroy(void);
mi_page_t*    _mi_safe_ptr_page(const void* p);


/* -----------------------------------------------------------
  The page map maps addresses to `mi_page_t` pointers
----------------------------------------------------------- */

#if MI_PAGE_MAP_FLAT

// flat page-map committed on demand, using one byte per slice (64 KiB).
// single indirection and low commit, but large initial virtual reserve (4 GiB with 48 bit virtual addresses)
// used by default on <= 40 bit virtual address spaces.
extern mi_decl_hidden _Atomic(uint8_t*) _mi_page_map;
extern mi_decl_hidden _Atomic(void*)    _mi_page_map_max_address;

static inline size_t mi_page_map_index(const void* p) {
  return (size_t)((uintptr_t)p >> MI_ARENA_SLICE_SHIFT);
}

static inline uint8_t mi_page_map_at(size_t idx) {
  return mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map)[idx];
}

static inline mi_page_t* mi_ptr_page_ex(const void* p, bool* valid) {
  const size_t idx = mi_page_map_index(p);
  const size_t ofs = mi_page_map_at(idx);
  if (valid != NULL) { *valid = (ofs != 0); }
  return (mi_page_t*)((((uintptr_t)p >> MI_ARENA_SLICE_SHIFT) + 1 - ofs) << MI_ARENA_SLICE_SHIFT);
}

static inline mi_page_t* mi_checked_ptr_page(const void* p) {
  #if MI_MIN_VABITS < MI_INTPTR_BITS
  if mi_unlikely(((uintptr_t)p >> MI_MIN_VABITS) != 0) {
    if (p > mi_atomic_load_ptr_relaxed(void, &_mi_page_map_max_address)) return NULL;
  }
  #endif
  bool valid;
  mi_page_t* const page = mi_ptr_page_ex(p, &valid);
  return (valid ? page : NULL);
}

static inline mi_page_t* mi_unchecked_ptr_page(const void* p) {
  return mi_ptr_page_ex(p, NULL);
}

#else

// 2-level page map:
// double indirection, but low commit and low virtual reserve.
//
// the page-map is usually 4 MiB (for 48 bit virtual addresses) and points to sub maps of 64 KiB.
// the page-map is committed on-demand (in 64 KiB parts) (and sub-maps are committed on-demand as well)
// one sub page-map = 64 KiB => covers 2^(16-3) * 2^16 = 2^29 = 512 MiB address space
// the page-map needs 48-(16+13) = 19 bits => 2^19 sub map pointers = 2^22 bytes = 4 MiB reserved size.
#define MI_PAGE_MAP_SUB_SHIFT     (13)
#define MI_PAGE_MAP_SUB_COUNT     (MI_ZU(1) << MI_PAGE_MAP_SUB_SHIFT)
#define MI_PAGE_MAP_SHIFT         (MI_MAX_VABITS - MI_PAGE_MAP_SUB_SHIFT - MI_ARENA_SLICE_SHIFT)

typedef mi_page_t**   mi_submap_t;
typedef struct mi_page_map_s {
  _Atomic(size_t)      committed_count;  // currently committed entries
  size_t               reserved_size;    // full reserved size (mi_page_map_t + submaps)
  mi_memid_t           memid;            // provenance
  mi_lock_t            lock;             // used when allocating new submaps
  _Atomic(mi_submap_t) submaps[1];
} mi_page_map_t;

extern mi_decl_hidden _Atomic(mi_page_map_t*) __mi_page_map;

static inline size_t mi_page_map_index(const void* p, size_t* sub_idx) {
  const size_t u = (size_t)((uintptr_t)p / MI_ARENA_SLICE_SIZE);
  if (sub_idx != NULL) { *sub_idx = u % MI_PAGE_MAP_SUB_COUNT; }
  return (u / MI_PAGE_MAP_SUB_COUNT);
}

static inline mi_page_map_t* mi_page_map(void) {
  return mi_atomic_load_ptr_relaxed(mi_page_map_t,&__mi_page_map);
}

static inline mi_submap_t mi_page_map_at(const mi_page_map_t* pmap, size_t idx) {
  return mi_atomic_load_ptr_acquire(mi_page_t*, &pmap->submaps[idx]);
}

static inline mi_page_t* mi_unchecked_ptr_page(const void* p) {
  const mi_page_map_t* pmap = mi_page_map();
  size_t sub_idx;
  const size_t idx = mi_page_map_index(p, &sub_idx);
  return mi_page_map_at(pmap,idx)[sub_idx];  // NULL if p==NULL
}

static inline mi_page_t* mi_checked_ptr_page(const void* p) {
  const mi_page_map_t* pmap = mi_page_map();
  size_t sub_idx;
  const size_t idx = mi_page_map_index(p, &sub_idx);
  const size_t committed_count = mi_atomic_load_relaxed(&pmap->committed_count);
  if mi_unlikely(idx >= committed_count) return NULL;
  // #if MI_MIN_VABITS < MI_INTPTR_BITS   // is still invalid if free is called before the pagemap is initialized
  // if mi_unlikely(((uintptr_t)p >> MI_MIN_VABITS) != 0) {
  //   const size_t committed_count = mi_atomic_load_relaxed(&pmap->committed_count);
  //   if mi_unlikely(idx >= committed_count) return NULL;
  // }
  // #endif
  mi_submap_t const sub = mi_page_map_at(pmap,idx);
  if mi_unlikely(sub == NULL) return NULL;
  return sub[sub_idx];
}

#endif

#if MI_PAGE_META_IS_ALIGNED
// if the page meta data is aligned in front of pages we can find it efficiently
// without needing to go through the page map (for valid pointers).
static inline mi_page_t* mi_aligned_ptr_page0(const void* p) {
  mi_page_t* const page_metas = (mi_page_t*)mi_align_down_ptr(p,MI_PAGE_META_ALIGNMENT);
  // const ptrdiff_t page_idx = ((uint8_t*)p - (uint8_t*)page_metas)/MI_ARENA_SLICE_SIZE;
  const uintptr_t page_idx    = ((uintptr_t)p / MI_ARENA_SLICE_SIZE) % (MI_PAGE_META_ALIGNMENT / MI_ARENA_SLICE_SIZE);
  mi_assert_internal(page_idx <= MI_PAGE_META_ALIGNED_COUNT);
  #if MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_RISCV // better code on x64/x86/riscv64
  mi_page_t* const page = (mi_page_t*)((uintptr_t)page_metas | (page_idx * sizeof(mi_page_t)));
  #else
  mi_page_t* const page = &page_metas[page_idx];
  #endif
  return page;

}

static inline mi_page_t* mi_aligned_ptr_page(const void* p) {
  mi_page_t* const page = mi_aligned_ptr_page0(p);
  if mi_unlikely(page==NULL) return NULL;
  #if MI_DEBUG
    mi_page_t* const cpage = mi_checked_ptr_page(p);
    if mi_unlikely(cpage==NULL) {
      _mi_error_message(EINVAL, "mi_aligned_ptr_page: invalid pointer: %p\n", p);
      return NULL;
    }
  #endif
  return mi_atomic_load_ptr_acquire(mi_page_t, &page->self);
}
#endif

static inline mi_page_t* mi_ptr_page(const void* p) {
  mi_assert_internal(p==NULL || mi_is_in_heap_region(p));
  #if MI_SECURE || MI_FREE_IS_CHECKED
    return mi_checked_ptr_page(p);
  #elif MI_PAGE_META_IS_ALIGNED
    return mi_aligned_ptr_page(p);
  #elif MI_DEBUG
    return mi_checked_ptr_page(p);
  #else
    return mi_unchecked_ptr_page(p);
  #endif
}

#endif
