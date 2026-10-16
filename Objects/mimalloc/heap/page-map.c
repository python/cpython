/*----------------------------------------------------------------------------
Copyright (c) 2023-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/arena/arena.h"
#include "mimalloc/heap/subproc.h"
#include "mimalloc/heap/page-map.h"
#include "../arena/bitmap.h"

static void mi_page_map_cannot_commit(void) {
  _mi_warning_message("unable to commit the allocation page-map on-demand\n" );
}

#if MI_PAGE_MAP_FLAT

// The page-map contains a byte for each 64kb slice in the address space.
// For an address `a` where `ofs = _mi_page_map[a >> 16]`:
// 0 = unused
// 1 = the slice at `a & ~0xFFFF` is a mimalloc page.
// 1 < ofs <= 127 = the slice is part of a page, starting at `(((a>>16) - ofs - 1) << 16)`.
//
// 1 byte per slice => 1 TiB address space needs a 2^14 * 2^16 = 16 MiB page map.
// A full 256 TiB address space (48 bit) needs a 4 GiB page map.
// A full 4 GiB address space (32 bit) needs only a 64 KiB page map.

// Use an initial empty page map so `free(NULL)` works even if mimalloc is not yet initialized (issue #1341)
static uint8_t mi_page_map_empty[1] = { 1 };      // mi_ptr_page(NULL) == NULL

mi_decl_hidden mi_decl_cache_align _Atomic(uint8_t*) _mi_page_map   = mi_page_map_empty;
mi_decl_hidden _Atomic(void*)  _mi_page_map_max_address = NULL;
static mi_memid_t   mi_page_map_memid;

#define MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT   MI_ARENA_SLICE_SIZE
static mi_bitmap_t* mi_page_map_commit; // one bit per committed 64 KiB entries

mi_decl_nodiscard static bool mi_page_map_ensure_committed(size_t idx, size_t slice_count);

bool _mi_page_map_init(void) {
  size_t vbits = (size_t)mi_option_get_clamp(mi_option_max_vabits, 0, MI_MAX_VABITS);
  if (vbits == 0) {
    vbits = _mi_os_virtual_address_bits();
    #if MI_ARCH_X64  // canonical address is limited to the first 128 TiB
    if (vbits >= 48) { vbits = 47; }
    #endif
  }
  if (vbits < MI_ARENA_SLICE_SHIFT) {
    vbits = MI_ARENA_SLICE_SHIFT;
  }
  if (vbits < MI_MIN_VABITS) {    // cover at least this much for a faster _mi_checked_ptr
    vbits = MI_MIN_VABITS;
  }
  if (vbits > MI_MAX_VABITS) {    // limit page map size even if more virtual addresses are available
    vbits = MI_MAX_VABITS;
  }

  // Allocate the page map and commit bits
  mi_atomic_store_ptr_release(void, &_mi_page_map_max_address, (void*)(vbits >= MI_SIZE_BITS ? (SIZE_MAX - MI_ARENA_SLICE_SIZE + 1) : (MI_PU(1) << vbits)));
  const size_t page_map_size = (MI_ZU(1) << (vbits - MI_ARENA_SLICE_SHIFT));
  const bool commit = (page_map_size <= 1*MI_MiB || mi_option_is_enabled(mi_option_pagemap_commit)); // _mi_os_has_overcommit(); // commit on-access on Linux systems?
  const size_t commit_bits = mi_divide_up(page_map_size, MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT);
  const size_t bitmap_size = (commit ? 0 : mi_bitmap_size(commit_bits, NULL));
  const size_t reserve_size = bitmap_size + page_map_size;
  mi_subproc_t* const subproc = _mi_subproc_main();
  uint8_t* const base = (uint8_t*)_mi_os_alloc_aligned(subproc, reserve_size, 1, commit, true /* allow large */, &mi_page_map_memid);
  if (base==NULL) {
    _mi_error_message(ENOMEM, "unable to reserve virtual memory for the page map (%zu KiB)\n", page_map_size / MI_KiB);
    return false;
  }
  if (mi_page_map_memid.initially_committed && !mi_page_map_memid.initially_zero) {
    _mi_warning_message("internal: the page map was committed but not zero initialized!\n");
    mi_memzero_aligned(base, reserve_size);
  }
  if (bitmap_size > 0) {
    mi_page_map_commit = (mi_bitmap_t*)base;
    if (!_mi_os_commit(subproc, mi_page_map_commit, bitmap_size, NULL)) {
      mi_page_map_cannot_commit();
      return false;
    }
    mi_bitmap_init(mi_page_map_commit, commit_bits, true);
  }
  mi_atomic_store_ptr_release(uint8_t,&_mi_page_map, base + bitmap_size);

  // commit the first part so NULL pointers get resolved without an access violation
  if (!commit) {
    if (!mi_page_map_ensure_committed(0, 1)) {
      mi_page_map_cannot_commit();
      return false;
    }
  }
  mi_atomic_load_ptr_relaxed(uint8_t, &_mi_page_map)[0] = 1; // so mi_ptr_page(NULL) == NULL
  mi_assert_internal(mi_ptr_page(NULL)==NULL);
  return true;
}

void _mi_page_map_unsafe_destroy(void) {
  mi_assert_internal(mi_atomic_load_ptr_relaxed(uint8_t, &_mi_page_map) != NULL);
  if (mi_atomic_load_ptr_relaxed(uint8_t, &_mi_page_map) == NULL) return;
  _mi_os_free_ex(_mi_subproc_main(), mi_page_map_memid.mem.os.base, mi_page_map_memid.mem.os.size, true, mi_page_map_memid);
  mi_atomic_store_ptr_release(uint8_t, &_mi_page_map, NULL);
  mi_page_map_commit = NULL;
  mi_atomic_store_ptr_release(void, &_mi_page_map_max_address, NULL);
  mi_page_map_memid = _mi_memid_none();
}


static bool mi_page_map_ensure_committed(size_t idx, size_t slice_count) {
  // is the page map area that contains the page address committed?
  // we always set the commit bits so we can track what ranges are in-use.
  // we only actually commit if the map wasn't committed fully already.
  uint8_t* const page_map = mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map);
  if (mi_page_map_commit != NULL) {
    const size_t commit_idx = idx / MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT;
    const size_t commit_idx_hi = (idx + slice_count - 1) / MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT;
    for (size_t i = commit_idx; i <= commit_idx_hi; i++) {  // per bit to avoid crossing over bitmap chunks
      if (mi_bitmap_is_clear(mi_page_map_commit, i)) {
        // this may race, in which case we do multiple commits (which is ok)
        bool is_zero;
        uint8_t* const start = page_map + (i * MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT);
        const size_t   size  = MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT;
        if (!_mi_os_commit(_mi_subproc_main(), start, size, &is_zero)) {
          mi_page_map_cannot_commit();
          return false;
        }
        if (!is_zero && !mi_page_map_memid.initially_zero) { mi_memzero(start, size); }
        mi_bitmap_set(mi_page_map_commit, i);
      }
    }
  }
  #if MI_DEBUG > 0
  page_map[idx] = 0;
  page_map[idx+slice_count-1] = 0;
  #endif
  return true;
}


static size_t mi_page_map_get_idx(mi_page_t* page, uint8_t** page_start, size_t* slice_count) {
  size_t page_size;
  *page_start = mi_page_area(page, &page_size);
  if (page_size > MI_LARGE_PAGE_SIZE) { page_size = MI_LARGE_PAGE_SIZE - MI_ARENA_SLICE_SIZE; }  // furthest interior pointer
  *slice_count = _mi_slice_count_of_size(page_size) + ((*page_start - mi_page_slice_start(page))/MI_ARENA_SLICE_SIZE); // add for large aligned blocks
  return mi_page_map_index(page);
}

bool _mi_page_map_register(mi_page_t* page) {
  mi_assert_internal(page != NULL);
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map) != NULL);  // should be initialized before multi-thread access!
  uint8_t* page_map = mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map);
  if mi_unlikely(mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map) == NULL) {
    if (!_mi_page_map_init()) return false;
    page_map = mi_atomic_load_ptr_acquire(uint8_t,&_mi_page_map);
  }
  mi_assert(page_map!=NULL);
  uint8_t* page_start;
  size_t   slice_count;
  const size_t idx = mi_page_map_get_idx(page, &page_start, &slice_count);

  if (!mi_page_map_ensure_committed(idx, slice_count)) {
    return false;
  }

  // set the offsets
  for (size_t i = 0; i < slice_count; i++) {
    mi_assert_internal(i < 128);
    page_map[idx + i] = (uint8_t)(i+1);
  }
  return true;
}

void _mi_page_map_unregister(mi_page_t* page) {
  uint8_t* const page_map = mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map);
  mi_assert_internal(page_map != NULL);
  if (page_map == NULL) return;
  // get index and count
  uint8_t* page_start;
  size_t   slice_count;
  const size_t idx = mi_page_map_get_idx(page, &page_start, &slice_count);
  // unset the offsets
  mi_memzero(page_map + idx, slice_count);
}

void _mi_page_map_unregister_range(void* start, size_t size) {
  uint8_t* const page_map = mi_atomic_load_ptr_relaxed(uint8_t,&_mi_page_map);
  mi_assert_internal(page_map!=NULL);
  if (page_map == NULL) return;
  const size_t slice_count = mi_divide_up(size, MI_ARENA_SLICE_SIZE);
  const uintptr_t index = mi_page_map_index(start);
  // todo: scan the commit bits and clear only those ranges?
  if (!mi_page_map_ensure_committed(index, slice_count)) { // we commit the range in total;
    return;
  }
  mi_memzero(&page_map[index], slice_count);
}


mi_page_t* _mi_safe_ptr_page(const void* p) {
  if mi_unlikely(p >= mi_atomic_load_ptr_relaxed(void, &_mi_page_map_max_address)) return NULL;
  const uintptr_t idx = mi_page_map_index(p);
  if mi_unlikely(mi_page_map_commit != NULL && !mi_bitmap_is_set(mi_page_map_commit, idx/MI_PAGE_MAP_ENTRIES_PER_COMMIT_BIT)) return NULL;
  const uintptr_t ofs = mi_page_map_at(idx);
  if mi_unlikely(ofs == 0) return NULL;
  return (mi_page_t*)((((uintptr_t)p >> MI_ARENA_SLICE_SHIFT) - ofs + 1) << MI_ARENA_SLICE_SHIFT);
}

mi_decl_nodiscard mi_decl_export bool mi_is_in_heap_region(const void* p) mi_attr_noexcept {
  return (_mi_safe_ptr_page(p) != NULL);
}

#else

// A 2-level page map
#define MI_PAGE_MAP_SUB_SIZE          (MI_PAGE_MAP_SUB_COUNT * sizeof(mi_page_t*))

// Use an initial empty page map so `free(NULL)` works even if mimalloc is not yet initialized (issue #1341)
static mi_page_map_t mi_page_map_empty = { 
  MI_ATOMIC_VAR_INIT(1),
  sizeof(mi_page_map_t),
  MI_MEMID_STATIC,
  MI_LOCK_INITIALIZER,
  { MI_ATOMIC_VAR_INIT(NULL) } 
};

mi_decl_hidden mi_decl_cache_align _Atomic(mi_page_map_t*) __mi_page_map  = MI_ATOMIC_VAR_INIT(&mi_page_map_empty);

static size_t mi_page_map_count_of_size(size_t size) {
  return (size < sizeof(mi_page_map_t) ? 0 : 1 + (size - sizeof(mi_page_map_t))/sizeof(mi_submap_t));
}

// static void* mi_page_map_addr_of_index(size_t idx) {
//   return (void*)((uintptr_t)idx * MI_PAGE_MAP_SUB_COUNT * MI_ARENA_SLICE_SIZE);
// }

mi_decl_nodiscard static mi_decl_noinline bool mi_page_map_commit_entries(mi_page_map_t* pmap, size_t required_idx) {
  const size_t reserved_count = mi_page_map_count_of_size(pmap->reserved_size);  
  if mi_unlikely(required_idx >= reserved_count) {
    mi_page_map_cannot_commit(); 
    return false;
  }
  size_t commit_size = mi_align_up( sizeof(mi_page_map_t) + (required_idx * sizeof(mi_submap_t)), MI_ARENA_SLICE_SIZE );
  if (pmap->reserved_size < commit_size) { commit_size = pmap->reserved_size; }
  const size_t commit_count = mi_page_map_count_of_size(commit_size);
  mi_assert_internal(commit_count > required_idx);
  mi_assert_internal(commit_count <= reserved_count);
  // note: we rely on uncommitted memory to be zero initialized on the first commit (and further concurrent commits leave the memory as is).
  bool is_zero;
  if mi_unlikely(!_mi_os_commit(_mi_subproc_main(), pmap, commit_size, &is_zero)) {
    mi_page_map_cannot_commit();
    return false;
  }
  mi_assert_internal(is_zero || pmap->memid.initially_zero);
  mi_atomic_store_release(&pmap->committed_count, commit_count); 
  // mi_atomic_store_release(&pmap->committed_addr, mi_page_map_addr_of_index(commit_count));
  return true;
}

mi_decl_nodiscard static bool mi_page_map_ensure_committed(mi_page_map_t* pmap, size_t idx, mi_submap_t* submap) {
  mi_assert_internal(submap!=NULL && *submap==NULL);
  if mi_unlikely(idx >= mi_atomic_load_relaxed(&pmap->committed_count)) {
    if (idx >= mi_atomic_load_acquire(&pmap->committed_count)) {
      if (!mi_page_map_commit_entries(pmap,idx)) return false;    
      mi_assert_internal(idx < mi_atomic_load_relaxed(&pmap->committed_count));
    }
  }
  *submap = mi_atomic_load_ptr_acquire(mi_page_t*, &pmap->submaps[idx]);
  return true;
}

// initialize the page map 
static bool mi_page_map_init_once(void) {
  size_t vbits = (size_t)mi_option_get_clamp(mi_option_max_vabits, 0, MI_MAX_VABITS);
  if (vbits == 0) {
    vbits = _mi_os_virtual_address_bits();
    #if MI_ARCH_X64  // canonical address is limited to the first 128 TiB
    if (vbits >= 48) { vbits = 47; }
    #endif
  }
  if (vbits < MI_PAGE_MAP_SUB_SHIFT + MI_ARENA_SLICE_SHIFT) {
    vbits = MI_PAGE_MAP_SUB_SHIFT + MI_ARENA_SLICE_SHIFT;
  }
  if (vbits < MI_MIN_VABITS) {    // cover at least this much for a faster _mi_checked_ptr
    vbits = MI_MIN_VABITS;
  }
  if (vbits > MI_MAX_VABITS) {    // limit page map size even if more virtual addresses are available
    vbits = MI_MAX_VABITS;
  }

  // Allocate the page map and commit bits
  mi_assert(MI_MAX_VABITS >= vbits);
  mi_assert(MI_MIN_VABITS <= vbits);
  const size_t reserve_count    = (MI_ZU(1) << (vbits - MI_PAGE_MAP_SUB_SHIFT - MI_ARENA_SLICE_SHIFT));
  const size_t os_page_size  = _mi_os_page_size();
  const size_t reserve_size  = mi_align_up( sizeof(mi_page_map_t) + ((reserve_count - 1) * sizeof(mi_submap_t)), os_page_size);
  const size_t submap_size   = MI_PAGE_MAP_SUB_SIZE;
  const size_t extra_reserve_size  = reserve_size + submap_size;
  const bool commit = (vbits == MI_MIN_VABITS) || (reserve_size <= 64*MI_KiB) || // 42 virtual address bits
                      mi_option_is_enabled(mi_option_pagemap_commit) || _mi_os_has_overcommit();
  mi_subproc_t* const subproc = _mi_subproc_main();
  mi_memid_t memid;
  mi_page_map_t* const pmap = (mi_page_map_t*)_mi_os_alloc_aligned(subproc, extra_reserve_size, 1, commit, true /* allow large */, &memid);
  if mi_unlikely(pmap==NULL) {
    _mi_error_message(ENOMEM, "unable to reserve virtual memory for the page map (%zu KiB)\n", extra_reserve_size / MI_KiB);
    return false;
  }
 
  // commit 
  size_t commit_count;
  if (memid.initially_committed) {
    if (!memid.initially_zero) {
      _mi_warning_message("internal: the page map was committed but not zero initialized!\n");
      mi_memzero_aligned(pmap, extra_reserve_size);
      memid.initially_zero = true;
    }
    commit_count = mi_page_map_count_of_size(reserve_size);
  }
  else {
    // commit first entries up to MI_MIN_VABITS entries
    const size_t min_commit_count = (MI_ZU(1) << (MI_MIN_VABITS - MI_PAGE_MAP_SUB_SHIFT - MI_ARENA_SLICE_SHIFT));  
    const size_t min_commit_size = mi_align_up( sizeof(mi_page_map_t) + ((min_commit_count-1) * sizeof(mi_submap_t)), os_page_size);
    mi_assert_internal(min_commit_size <= reserve_size);
    bool is_zero;
    if (!_mi_os_commit(subproc,pmap,min_commit_size,&is_zero)) {
      mi_page_map_cannot_commit();
      _mi_os_free(subproc,pmap,extra_reserve_size,memid);
      return false;
    };
    mi_assert_internal(is_zero || memid.initially_zero);
    commit_count = mi_page_map_count_of_size(min_commit_size);
    mi_assert_internal(commit_count >= min_commit_count);
  }
  
  // ensure there is a submap for the NULL address
  mi_page_t** const sub0 = (mi_submap_t)((uint8_t*)pmap + reserve_size);  // we reserved a submap part at the end already
  if (!memid.initially_committed) {
    if (!_mi_os_commit(subproc, sub0, submap_size, NULL)) {  // commit full submap (issue #1087)
      mi_page_map_cannot_commit();
      _mi_os_free(subproc,pmap,extra_reserve_size,memid);
      return false;
    }
  }
  if (!memid.initially_zero) {     // initialize low addresses with NULL
    mi_memzero_aligned(sub0, submap_size);
  }

  // initialize the fields
  pmap->memid = memid;
  pmap->reserved_size  = reserve_size;
  mi_lock_init(&pmap->lock);
  mi_atomic_store_release(&pmap->committed_count, commit_count);
  // mi_atomic_store_release(&pmap->committed_addr, mi_page_map_addr_of_index(commit_count));
  mi_atomic_store_ptr_release(mi_page_t*, &pmap->submaps[0], sub0);
  mi_atomic_store_ptr_release(mi_page_map_t, &__mi_page_map, pmap);
  mi_assert_internal(mi_ptr_page(NULL)==NULL);
  return true;
}

bool _mi_page_map_init(void) {
  bool ok = true;
  mi_atomic_do_once {
    ok = mi_page_map_init_once();
  }
  return ok;
}

void _mi_page_map_unsafe_destroy(void) {
  mi_page_map_t* const pmap = mi_page_map();
  mi_assert_internal(pmap != NULL);
  if (pmap == NULL || pmap == &mi_page_map_empty) return;
  mi_subproc_t* const subproc = _mi_subproc_main();
  mi_lock_done(&pmap->lock);  
  for (size_t idx = 1; idx < pmap->committed_count; idx++) {  // skip entry 0 (as we allocate that submap at the end of the page_map)
    // free all sub-maps   
    mi_submap_t sub = mi_page_map_at(pmap,idx);
    if (sub != NULL) {
      mi_memid_t memid = _mi_memid_create_os(sub, MI_PAGE_MAP_SUB_SIZE, true, false, false);
      _mi_os_free_ex(subproc, memid.mem.os.base, memid.mem.os.size, true, memid);
      mi_atomic_store_ptr_release(mi_page_t*, &pmap->submaps[idx], NULL);
    }
  }
  _mi_os_free_ex(subproc, pmap, pmap->reserved_size, true, pmap->memid);
  mi_atomic_store_ptr_release(mi_page_map_t, &__mi_page_map, &mi_page_map_empty);  
}

mi_decl_nodiscard static mi_decl_noinline mi_submap_t mi_page_map_alloc_submap_at(mi_page_map_t* pmap, size_t idx) {
  // sub map not yet allocated, alloc now
  mi_submap_t sub = NULL;
  mi_lock(&pmap->lock) 
  {
    sub = mi_atomic_load_ptr_acquire(mi_page_t*, &pmap->submaps[idx]); // reload
    if (sub==NULL) // not yet allocated by another thread?      
    {
      mi_subproc_t* const subproc = _mi_subproc_main();
      mi_memid_t memid;
      const size_t submap_size = MI_PAGE_MAP_SUB_SIZE;        
      sub = (mi_submap_t)_mi_os_zalloc(subproc, submap_size, &memid);        
      if (sub==NULL) {
        _mi_warning_message("internal error: unable to extend the page map\n");          
      }
      else {
        mi_submap_t expect = NULL;
        if (!mi_atomic_cas_ptr_strong_acq_rel(mi_page_t*, &pmap->submaps[idx], &expect, sub)) {
          // another thread already allocated it.. free and continue
          _mi_os_free(subproc, sub, submap_size, memid);
          sub = expect;
        }
      }
    }
  }
  return sub;
}

mi_decl_nodiscard static bool mi_page_map_ensure_submap_at(mi_page_map_t* pmap, size_t idx, mi_submap_t* submap) {
  mi_assert_internal(submap!=NULL && *submap==NULL);
  mi_submap_t sub = NULL;
  if (!mi_page_map_ensure_committed(pmap, idx, &sub)) {
    return false;
  }
  if mi_unlikely(sub==NULL) {
    sub = mi_page_map_alloc_submap_at(pmap, idx);
    if (sub==NULL) return false; // unable to allocate the submap..
  }
  mi_assert_internal(sub!=NULL);
  *submap = sub;
  return true;
}

static bool mi_page_map_set_range_prim(mi_page_map_t* pmap, mi_page_t* page, size_t idx, size_t sub_idx, size_t slice_count) {
  // is the page map area that contains the page address committed?
  while (slice_count > 0) {
    mi_submap_t sub = NULL;
    if (!mi_page_map_ensure_submap_at(pmap, idx, &sub)) {
      return false;
    };
    mi_assert_internal(sub!=NULL);
    // set the offsets for the page
    while (slice_count > 0 && sub_idx < MI_PAGE_MAP_SUB_COUNT) {
      sub[sub_idx] = page;
      slice_count--;
      sub_idx++;
    }
    idx++; // potentially wrap around to the next idx
    sub_idx = 0;
  }
  return true;
}

static bool mi_page_map_set_range(mi_page_map_t* pmap, mi_page_t* page, size_t idx, size_t sub_idx, size_t slice_count) {
  if mi_unlikely(!mi_page_map_set_range_prim(pmap, page,idx,sub_idx,slice_count)) {
    // failed to commit, call again to reset the page pointer if needed
    if (page!=NULL) {
      mi_page_map_set_range_prim(pmap,NULL,idx,sub_idx,slice_count);
    }
    return false;
  }
  return true;
}

static size_t mi_page_map_get_idx(mi_page_t* page, size_t* sub_idx, size_t* slice_count) {
  size_t page_size;
  uint8_t* page_start = mi_page_area(page, &page_size);
  if (page_size > MI_LARGE_PAGE_SIZE) { page_size = MI_LARGE_PAGE_SIZE - MI_ARENA_SLICE_SIZE; }  // furthest interior pointer
  *slice_count = _mi_slice_count_of_size(page_size) + ((page_start - mi_page_slice_start(page))/MI_ARENA_SLICE_SIZE); // add for large aligned blocks
  return mi_page_map_index(page_start, sub_idx);
}

bool _mi_page_map_register(mi_page_t* page) {
  mi_assert_internal(page != NULL);
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_page_map_t* pmap = mi_page_map();
  mi_assert_internal(pmap != NULL);  // should be initialized before multi-thread access!
  if mi_unlikely(pmap == NULL) {
    if (!_mi_page_map_init()) return false;
    pmap = mi_atomic_load_ptr_acquire(mi_page_map_t,&__mi_page_map);
  }
  mi_assert(pmap!=NULL);
  size_t   slice_count;
  size_t   sub_idx;
  const size_t idx = mi_page_map_get_idx(page, &sub_idx, &slice_count);
  return mi_page_map_set_range(pmap, page, idx, sub_idx, slice_count);
}

void _mi_page_map_unregister(mi_page_t* page) {
  mi_assert_internal(__mi_page_map != NULL);
  mi_assert_internal(page != NULL);
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_page_map_t* const pmap = mi_page_map();
  // note: should proceed even if the page was not registered yet (for failure paths in page allocation in `arena/arena-page.c`)
  if mi_unlikely(pmap == NULL) return;
  // get index and count
  size_t slice_count;
  size_t sub_idx;
  const size_t idx = mi_page_map_get_idx(page, &sub_idx, &slice_count);
  // unset the offsets
  mi_page_map_set_range(pmap, NULL, idx, sub_idx, slice_count);
}

void _mi_page_map_unregister_range(void* start, size_t size) {
  mi_page_map_t* const pmap = mi_page_map();
  if mi_unlikely(pmap == NULL) return;
  const size_t slice_count = mi_divide_up(size, MI_ARENA_SLICE_SIZE);
  size_t sub_idx;
  const uintptr_t idx = mi_page_map_index(start, &sub_idx);
  mi_page_map_set_range(pmap, NULL, idx, sub_idx, slice_count);  // todo: avoid committing if not already committed?
}

// Return NULL for invalid pointers
mi_page_t* _mi_safe_ptr_page(const void* p) {
  return mi_checked_ptr_page(p);
}

mi_decl_nodiscard mi_decl_export bool mi_is_in_heap_region(const void* p) mi_attr_noexcept {
  return (_mi_safe_ptr_page(p) != NULL);
}

#endif
