/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
"Arenas" are fixed area's of OS memory from which we can allocate
large blocks (>= MI_ARENA_MIN_BLOCK_SIZE, 64KiB).
In contrast to the rest of mimalloc, the arenas are shared between
threads and need to be accessed using atomic operations.

Arenas are also used to for huge OS page (1GiB) reservations or for reserving
OS memory upfront which can be improve performance or is sometimes needed
on embedded devices. We can also employ this with WASI or `sbrk` systems
to reserve large arenas upfront and be able to reuse the memory more effectively.

The arena allocation needs to be thread safe and we use an atomic bitmap to allocate.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/heap/subproc.h"
#include "mimalloc/arena/arena.h"
#include "arena.h"

mi_heap_t* _mi_arena_heap_main(const mi_arena_t* arena) {
  return _mi_subproc_heap_main(arena->subproc);
}

/* -----------------------------------------------------------
  arena blocks
----------------------------------------------------------- */

// Blocks needed for a given byte size
size_t _mi_slice_count_of_size(size_t size) {
  return mi_divide_up(size, MI_ARENA_SLICE_SIZE);
}

// Byte size of a number of blocks
size_t _mi_size_of_slices(size_t bcount) {
  return (bcount * MI_ARENA_SLICE_SIZE);
}

mi_memid_t _mi_memid_create(mi_memkind_t memkind) {
  mi_memid_t memid;
  _mi_memzero_var(memid);
  memid.memkind = memkind;
  return memid;
}

mi_memid_t _mi_memid_none(void) {
  return _mi_memid_create(MI_MEM_NONE);
}

mi_memid_t _mi_memid_create_os(void* base, size_t size, bool committed, bool is_zero, bool is_large) {
  mi_memid_t memid = _mi_memid_create(MI_MEM_OS);
  memid.mem.os.base = base;
  memid.mem.os.size = size;
  memid.initially_committed = committed;
  memid.initially_zero = is_zero;
  memid.is_pinned = is_large;
  return memid;
}

mi_memid_t _mi_memid_create_static(void* p, size_t size) {
  mi_memid_t memid = _mi_memid_create(MI_MEM_STATIC);
  memid.mem.malloc.base = p;
  memid.mem.malloc.size = size;
  memid.initially_committed = true;
  memid.is_pinned = true;
  return memid;
}

mi_memid_t _mi_memid_create_malloc(void* p, size_t size, bool iszero) {
  mi_memid_t memid = _mi_memid_create(MI_MEM_MALLOC);
  memid.mem.malloc.base = p;
  memid.mem.malloc.size = size;
  memid.initially_committed = true;
  memid.initially_zero = iszero;
  memid.is_pinned = true;
  return memid;
}

size_t _mi_memid_size(mi_memid_t memid) {
  if (mi_memid_is_os(memid)) {
    return memid.mem.os.size;
  }
  else if (memid.memkind == MI_MEM_ARENA) {
    return _mi_size_of_slices(memid.mem.arena.slice_count);
  }
  else if (memid.memkind == MI_MEM_MALLOC) {
    return memid.mem.malloc.size;
  }
  else {
    mi_assert_internal(mi_memid_needs_no_free(memid));
    return 0;
  }
}


/* -----------------------------------------------------------
  Arena id's
----------------------------------------------------------- */

mi_arena_id_t _mi_arena_id_none(void) {
  return NULL;
}

mi_arena_t* _mi_arena_from_id(mi_arena_id_t id) {
  mi_arena_t* const arena = (mi_arena_t*)id;
  mi_assert_internal(arena==NULL || arena->parent==NULL); // id's should never point to sub-arena's
  return arena;
}

mi_arena_id_t mi_arena_id_from_arena(mi_arena_t* arena) {
  mi_assert_internal(arena==NULL || arena->parent==NULL);
  return (arena==NULL ? _mi_arena_id_none() : (mi_arena_id_t)arena);
}


bool _mi_arena_memid_is_suitable(mi_memid_t memid, mi_arena_t* request_arena) {
  if (memid.memkind == MI_MEM_ARENA) {
    return mi_arena_is_suitable(memid.mem.arena.arena, request_arena);
  }
  else {
    return mi_arena_is_suitable(NULL, request_arena);
  }
}

size_t mi_arena_min_alignment(void) {
  return MI_ARENA_ALIGNMENT;
}

size_t mi_arena_min_size(void) {
  return MI_ARENA_MIN_SIZE;
}

// fixed limit for the maximum object size in an arena
static size_t mi_arena_max_fixed_object_size(void) {
  #if MI_PAGE_META_IS_ALIGNED
  return (MI_PAGE_META_ALIGNMENT - mi_align_up(MI_PAGE_META_ALIGNED_COUNT * sizeof(mi_page_t), MI_ARENA_SLICE_SIZE));
  #else
  return (MI_ARENA_MAX_SIZE - MI_ARENA_CHUNK_SIZE); // minus an initial chunk to accommodate meta info
  #endif
}

// Maximum object size allowed to be allocated in an arena
size_t mi_arena_max_object_size(void) {
  size_t max_size = mi_option_get_size(mi_option_arena_max_object_size);
  max_size = mi_align_up(max_size, MI_ARENA_SLICE_SIZE);
  if (max_size <= MI_ARENA_MIN_OBJ_SIZE) {
    return MI_ARENA_MIN_OBJ_SIZE;
  }
  else if (max_size >= mi_arena_max_fixed_object_size()) {
    return mi_arena_max_fixed_object_size();
  }
  else {
    return max_size;
  }
}

// Size of an arena
static size_t mi_arena_size(mi_arena_t* arena) {
  return _mi_size_of_slices(arena->slice_count);
}


// Arena area
void* mi_arena_area(mi_arena_id_t arena_id, size_t* size) {
  if (size != NULL) *size = 0;
  mi_arena_t* arena = _mi_arena_from_id(arena_id);
  if (arena == NULL) return NULL;
  if (size != NULL) {
    mi_assert_internal(_mi_size_of_slices(arena->slice_count) <= arena->total_size);
    *size = arena->total_size;
  }
  return mi_arena_start(arena);
}



/* -----------------------------------------------------------
  Arena iteration
----------------------------------------------------------- */

static bool mi_arena_is_suitable_ex(mi_arena_t* arena, mi_arena_t* req_arena, bool match_numa, int numa_node, bool allow_pinned) {
  if (!allow_pinned && arena->memid.is_pinned) return false;
  if (!mi_arena_is_suitable(arena, req_arena)) return false;
  if (req_arena == NULL) { // if not specific, check numa affinity
    const bool numa_suitable = (numa_node < 0 || arena->numa_node < 0 || arena->numa_node == numa_node);
    if (match_numa) { if (!numa_suitable) return false; }
               else { if (numa_suitable)  return false; }
  }
  return true;
}

// determine the start of search; important to keep heaps and threads
// into their own memory regions to reduce contention.
static inline size_t mi_arena_start_idx(mi_heap_t* heap, size_t tseq, size_t arena_cycle)
{
  const size_t hseq   = heap->heap_seq;
  const size_t hcount = mi_atomic_load_relaxed(&heap->subproc->heap_count);
  if (arena_cycle <= 1)     return 0;
  if (hseq==0 || hcount<=1 || arena_cycle > 0x8FF) return (tseq % arena_cycle); // common for single heap programs

  // spread heaps evenly among arena's, and then evenly for threads in their fraction
  size_t start;
  mi_assert_internal(arena_cycle <= 0x8FF);             // prevent overflow on 32-bit
  const size_t frac = (arena_cycle * 256) / hcount;     // fraction in the arena_cycle; at most: arena_cycle * 0x100
  if (frac==0) {
    // many heaps (> 256 per arena)
    start = (hseq % arena_cycle);
  }
  else {
    const size_t hspot = (hseq % hcount);
    start = (frac * hspot) / 256;           // (arena_cycle * (hseq % hcount)) / hcount
    if (frac >= 512) {                      // at least 2 arena's per heap?
      start = start + (tseq % (frac/256));
    }
  }
  mi_assert_internal(start < arena_cycle);
  return start;
}

mi_decl_forceinline bool mi_forall_arenas(mi_heap_t* heap, mi_arena_t* req_arena, size_t tseq, mi_forall_arena_fun_t* visit, const void* arg, void** result)
{
  if (result != NULL) *result = NULL;
  const size_t arena_count = mi_arenas_get_count(heap->subproc);
  const size_t arena_cycle = (arena_count == 0 ? 0 : arena_count - 1); /* first search the arenas below the last one */
  /* always start searching in the arena's below the max */
  const size_t start = (req_arena==NULL ? mi_arena_start_idx(heap,tseq,arena_cycle) : req_arena->arena_idx);
  mi_assert_internal(start <= arena_count);
  for (size_t i = 0; i < arena_count; i++) {
    size_t idx;
    if (i < arena_cycle) {
      idx = i + start;
      if (idx >= arena_cycle) { idx -= arena_cycle; } /* adjust so we rotate through the cycle */
    }
    else {
      idx = i; /* remaining arena's after the cycle */
    }
    mi_arena_t* arena = mi_arena_from_index(heap->subproc,idx);
    if (req_arena != NULL && arena != req_arena &&
        (arena == NULL || arena->parent != req_arena)) continue; /* only the requested arena or its children */

    // call the visitor
    if (arena != NULL) {
      if (!visit(arena, arg, result)) return false;
    }
  }
  return true;
}

typedef struct mi_arena_suitable_visit_info_s {
  mi_arena_t*            req_arena;
  bool                   match_numa;
  int                    numa_node;
  bool                   allow_large;
  mi_forall_arena_fun_t* visit;
  const void*            arg;
} mi_arena_suitable_visit_info_t;

static inline bool mi_arena_visit_suitable(mi_arena_t* arena, const void* arg, void** result)
{
  const mi_arena_suitable_visit_info_t* info = (const mi_arena_suitable_visit_info_t*)arg;
  if (!mi_arena_is_suitable_ex(arena, info->req_arena, info->match_numa, info->numa_node, info->allow_large)) {
    return true; // skip and continue
  }
  else {
    return info->visit(arena, info->arg, result); // visit
  }
}

mi_decl_forceinline bool mi_forall_suitable_arenas(mi_heap_t* heap, mi_arena_t* req_arena, size_t tseq, bool match_numa, int numa_node,
                                      bool allow_large, mi_forall_arena_fun_t* visit, const void* arg, void** result) {
  const mi_arena_suitable_visit_info_t info = { req_arena, match_numa, numa_node, allow_large, visit, arg };
  return mi_forall_arenas(heap, req_arena, tseq, &mi_arena_visit_suitable, &info, result);
}

#if defined(__cplusplus) || defined(_MSC_VER)
// since these are mi_decl_forceinline, ensure they are linked in the final binary by adding a reference.
void* _mi_arena_externs[] = {
  (void*)&mi_forall_arenas,
  (void*)&mi_forall_suitable_arenas
};
#endif


/* -----------------------------------------------------------
  Reserve a fresh arena
----------------------------------------------------------- */

static int mi_reserve_os_memory_ex2(mi_subproc_t* subproc, size_t size, bool commit, bool allow_large, bool exclusive, mi_arena_id_t* arena_id);

// try to reserve a fresh arena space
bool _mi_arena_reserve(mi_subproc_t* subproc, size_t req_size, bool allow_large, mi_arena_id_t* arena_id)
{
  const size_t arena_count = mi_arenas_get_count(subproc);
  if (arena_count > (MI_MAX_ARENAS - 4)) return false;

  // calc reserve
  size_t arena_reserve = mi_option_get_size(mi_option_arena_reserve);
  if (arena_reserve == 0) return false;

  if (!_mi_os_has_virtual_reserve()) {
    arena_reserve = arena_reserve/4;  // be conservative if virtual reserve is not supported (for WASM for example)
  }
  arena_reserve = mi_align_up(arena_reserve, MI_ARENA_SLICE_SIZE);

  if (arena_count >= 1 && arena_count <= 128) {
    // scale up the arena sizes exponentially every 8 entries
    const size_t multiplier = (size_t)1 << mi_clamp(arena_count/8, 0, 16);
    size_t reserve = 0;
    if (!mi_mul_overflow(multiplier, arena_reserve, &reserve)) {
      arena_reserve = reserve;
    }
  }

  // try to accommodate the requested size for huge allocations
  req_size = mi_align_up(req_size + MI_ARENA_MAX_CHUNK_OBJ_SIZE, MI_ARENA_MAX_CHUNK_OBJ_SIZE); // over-reserve for meta-info
  if (arena_reserve < req_size) {
    arena_reserve = req_size;
  }

  // check arena bounds
  const size_t min_reserve = MI_ARENA_MIN_SIZE;
  const size_t max_reserve = MI_ARENA_MAX_SIZE;   // 16 GiB
  if (arena_reserve < min_reserve) {
    arena_reserve = min_reserve;
  }
  else if (arena_reserve > max_reserve) {
    arena_reserve = max_reserve;
  }

  // should be able to at least handle the current allocation size
  if (arena_reserve < req_size) return false;

  // commit eagerly?
  bool arena_commit = false;
  const bool overcommit = _mi_os_has_overcommit();
  if (mi_option_get(mi_option_arena_eager_commit) == 2) { arena_commit = overcommit || mi_option_is_enabled(mi_option_allow_large_os_pages); }
  else if (mi_option_get(mi_option_arena_eager_commit) == 1) { arena_commit = true; }

  // on an OS with overcommit (Linux) we don't count the commit yet as it is on-demand. Once a slice
  // is actually allocated for the first time it will be counted.
  const bool adjust = (overcommit && arena_commit);
  if (adjust) { mi_subproc_stat_adjust_decrease( subproc, committed, arena_reserve); }
  // and try to reserve the arena
  int err = mi_reserve_os_memory_ex2(subproc, arena_reserve, arena_commit, allow_large, false /* exclusive? */, arena_id);
  if (err != 0) {
    if (adjust) { mi_subproc_stat_adjust_increase( subproc, committed, arena_reserve); } // roll back
    // failed to allocate: try a smaller size arena as fallback?
    const size_t small_arena_reserve = 4 * MI_ARENA_MIN_SIZE; // 128 MiB (or 32 MiB on 32-bit)
    if (arena_reserve > small_arena_reserve && small_arena_reserve > req_size) {
      // try again
      if (adjust) { mi_subproc_stat_adjust_decrease(subproc, committed, small_arena_reserve); }
      err = mi_reserve_os_memory_ex2(subproc, small_arena_reserve, arena_commit, allow_large, false /* exclusive? */, arena_id);
      if (err != 0 && adjust) { mi_subproc_stat_adjust_increase( subproc, committed, small_arena_reserve); } // roll back
    }
  }
  return (err==0);
}


// Is a pointer contained in the given arena area?
static bool mi_arena_strictly_contains(mi_arena_t* arena, const void* p) {
  return (arena != NULL &&
          mi_arena_start(arena) <= (const uint8_t*)p &&
          mi_arena_start(arena) + _mi_size_of_slices(arena->slice_count) >(const uint8_t*)p);
}

// Is a pointer inside any of our arenas?
static bool mi_arenas_contain_ex(const void* p, mi_arena_t* parent) {
  mi_subproc_t* subproc = _mi_subproc();
  const size_t max_arena = mi_arenas_get_count(subproc);
  for (size_t i = 0; i < max_arena; i++) {
    mi_arena_t* arena = mi_atomic_load_ptr_acquire(mi_arena_t, &subproc->arenas[i]);
    if (arena != NULL) {
      if (parent==NULL || arena==parent || arena->parent==parent) {
        if (mi_arena_strictly_contains(arena, p)) {
          return true;
        }
      }
    }
  }
  return false;
}

// // Is a pointer inside any of our arenas?
// bool _mi_arenas_contain(const void* p) {
//   return mi_arenas_contain_ex(p, NULL);
// }

// Is a pointer contained in the given arena area?
bool mi_arena_contains(mi_arena_id_t arena_id, const void* p) {
  mi_arena_t* arena = _mi_arena_from_id(arena_id);
  if (arena==NULL) return false;
  else if (mi_arena_strictly_contains(arena, p)) return true;
  else return mi_arenas_contain_ex(p, arena);  // maybe a subarena?
}


/* -----------------------------------------------------------
  Remove an arena.
----------------------------------------------------------- */

// destroy owned arenas; this is unsafe and should only be done using `mi_option_destroy_on_exit`
// for dynamic libraries that are unloaded and need to release all their allocated memory.
static void mi_arenas_unsafe_destroy(mi_subproc_t* subproc) {
  mi_assert_internal(subproc != NULL);
  const size_t arena_count = mi_arenas_get_count(subproc);
  for (size_t i = 0; i < arena_count; i++) {
    mi_arena_t* arena = mi_atomic_load_ptr_acquire(mi_arena_t, &subproc->arenas[i]);
    if (arena != NULL) {
      // mi_lock_done(&arena->abandoned_visit_lock);
      mi_atomic_store_ptr_release(mi_arena_t, &subproc->arenas[i], NULL);
      if (mi_memkind_is_os(arena->memid.memkind)) {
        _mi_os_free_ex(subproc, mi_arena_start(arena), mi_arena_size(arena), true, arena->memid);
      }
    }
  }
  // try to lower the max arena.
  size_t expected = arena_count;
  mi_atomic_cas_strong_acq_rel(&subproc->arena_count, &expected, (size_t)0);
}


// destroy owned arenas; this is unsafe and should only be done using `mi_option_destroy_on_exit`
// for dynamic libraries that are unloaded and need to release all their allocated memory.
void _mi_arenas_unsafe_destroy_all(mi_subproc_t* subproc) {
  mi_arenas_unsafe_destroy(subproc);
  // mi_arenas_try_purge(true /* force purge */, true /* visit all*/, subproc, 0 /* thread seq */);  // purge non-owned arenas
}


/* -----------------------------------------------------------
  Add an arena.
----------------------------------------------------------- */

static bool mi_arenas_add(mi_subproc_t* subproc, mi_arena_t* arena, mi_arena_id_t* arena_id)
{
  mi_assert_internal(arena != NULL);
  mi_assert_internal(arena->slice_count > 0);
  if (arena_id != NULL) { *arena_id = _mi_arena_id_none(); }

  // try to find a NULL entry
  mi_arena_t* expected;
  size_t count = mi_arenas_get_count(subproc);
  for( size_t i = 0; i < count; i++) {
    if (mi_arena_from_index(subproc,i) == NULL) {
      arena->arena_idx = i;
      expected = NULL;
      if (mi_atomic_cas_ptr_strong_release(mi_arena_t, &subproc->arenas[i], &expected, arena)) {
        // success
        if (arena_id != NULL) { *arena_id = mi_arena_id_from_arena(arena); }
        return true;
      }
    }
  }

  // otherwise, try to allocate a fresh slot
  while(count<MI_MAX_ARENAS) {
    if (mi_atomic_cas_strong_release(&subproc->arena_count, &count, count+1)) {
      arena->arena_idx = count;
      expected = NULL;
      if (mi_atomic_cas_ptr_strong_release(mi_arena_t, &subproc->arenas[count], &expected, arena)) {
        mi_subproc_stat_counter_increase(arena->subproc, arena_count, 1);
        if (arena_id != NULL) { *arena_id = mi_arena_id_from_arena(arena); }
        return true;
      }
    }
  }

  // failed
  arena->arena_idx = 0;
  arena->subproc = NULL;
  return false;
}


static mi_arena_t* mi_arena_info(void* area) {
  return (mi_arena_t*)((uint8_t*)area + _mi_size_of_slices(mi_arena_page_meta_aligned_slice_count()));
}

static size_t mi_arena_info_slices_needed(size_t slice_count, size_t* bitmap_base) {
  if (slice_count == 0) slice_count = MI_BCHUNK_BITS;
  mi_assert_internal((slice_count % MI_BCHUNK_BITS) == 0);
  const size_t base_size = _mi_size_of_slices(mi_arena_page_meta_aligned_slice_count()) + mi_align_up(sizeof(mi_arena_t), MI_BCHUNK_SIZE);
  const size_t bitmaps_count = 4 + MI_ARENA_BIN_COUNT; // commit, dirty, purge, pages, and abandoned
  const size_t bitmaps_size = bitmaps_count * mi_bitmap_size(slice_count, NULL) + mi_bbitmap_size(slice_count, NULL); // + free
  #if MI_PAGE_META_IS_SEPARATED && !MI_PAGE_META_IS_ALIGNED
  const size_t pages_size = slice_count * sizeof(mi_page_t);
  #else
  const size_t pages_size = 0;
  #endif
  const size_t size = base_size + bitmaps_size + pages_size;

  const size_t os_page_size = _mi_os_page_size();
  const size_t info_size = mi_align_up(size, os_page_size) + _mi_os_secure_guard_page_size();
  const size_t info_slices = _mi_slice_count_of_size(info_size);

  if (bitmap_base != NULL) *bitmap_base = base_size;
  return info_slices;
}

static mi_bbitmap_t* mi_arena_bbitmap_init(mi_subproc_t* subproc, size_t slice_count, uint8_t** base) {
  mi_bbitmap_t* bbitmap = (mi_bbitmap_t*)(*base);
  *base = (*base) + mi_bbitmap_init(subproc, bbitmap, slice_count, true /* already zero */);
  return bbitmap;
}


static mi_arena_t* mi_arena_initialize(mi_subproc_t* subproc, void* start,
                                        size_t slice_count, mi_arena_t* parent, size_t total_size,
                                        int numa_node, bool exclusive,
                                        mi_memid_t memid, mi_commit_fun_t* commit_fun, void* commit_fun_arg, mi_arena_id_t* arena_id)
{
  mi_assert_internal(mi_is_aligned(start,MI_ARENA_ALIGNMENT));
  mi_assert_internal(_mi_size_of_slices(slice_count)>=MI_ARENA_MIN_SIZE);

  if (slice_count > MI_BITMAP_MAX_BIT_COUNT) {  // 16 GiB for now
    // note: this should never happen if called from `mi_manage_os_memory` (as that allocates sub-arenas when needed)
    _mi_warning_message("cannot use OS memory since it is too large (size %zu MiB, maximum is %zu MiB)", _mi_size_of_slices(slice_count)/MI_MiB, _mi_size_of_slices(MI_BITMAP_MAX_BIT_COUNT)/MI_MiB);
    return NULL;
  }

  size_t bitmap_base;
  const size_t info_slices = mi_arena_info_slices_needed(slice_count, &bitmap_base);
  if (slice_count < info_slices+1) {
    _mi_warning_message("cannot use OS memory since it is not large enough (size %zu KiB, minimum required is %zu KiB)", _mi_size_of_slices(slice_count)/MI_KiB, _mi_size_of_slices(info_slices+1)/MI_KiB);
    return NULL;
  }
  // else if (info_slices >= MI_ARENA_MAX_CHUNK_OBJ_SLICES) {
  //   _mi_warning_message("cannot use OS memory since it is too large with respect to the maximum object size (size %zu MiB, meta-info slices %zu, maximum object slices are %zu)", _mi_size_of_slices(slice_count)/MI_MiB, info_slices, MI_ARENA_MAX_CHUNK_OBJ_SLICES);
  //   return NULL;
  // }

  // commit & zero if needed
  if (!memid.initially_committed) {
    size_t commit_size = _mi_size_of_slices(info_slices);
    // leave a guard OS page decommitted at the end?
    if (!memid.is_pinned) { commit_size -= _mi_os_secure_guard_page_size(); }
    bool ok = false;
    if (commit_fun != NULL) {
      ok = (*commit_fun)(true /* commit */, start, commit_size, NULL, commit_fun_arg);
    }
    else {
      ok = _mi_os_commit(subproc, start, commit_size, NULL);
    }
    if (!ok) {
      _mi_warning_message("unable to commit meta-data for OS memory");
      return NULL;
    }
  }
  else if (!memid.is_pinned) {
    // if MI_SECURE, set a guard page at the end of the arena info
    // todo: this does not respect the commit_fun as the memid is of external memory
    _mi_os_secure_guard_page_set_before(subproc, (uint8_t*)start + _mi_size_of_slices(info_slices), memid);
  }
  if (!memid.initially_zero) {
    mi_memzero(start, _mi_size_of_slices(info_slices) - _mi_os_secure_guard_page_size());
  }

  // init
  mi_arena_t* arena = mi_arena_info(start);
  arena->start = start;
  arena->subproc = subproc;
  arena->memid = memid;
  arena->is_exclusive = exclusive;
  arena->slice_count = slice_count;
  arena->info_slices = info_slices;
  if (numa_node<0 && mi_option_is_enabled(mi_option_arena_is_numa_local)) {
    arena->numa_node = _mi_os_numa_node();
  }
  else {
    arena->numa_node = numa_node;
  }
  arena->purge_expire = 0;
  arena->commit_fun = commit_fun;
  arena->commit_fun_arg = commit_fun_arg;
  arena->parent = parent;
  arena->total_size = total_size;

  // init bitmaps
  uint8_t* base = mi_arena_start(arena) + bitmap_base;
  arena->slices_free = mi_arena_bbitmap_init(subproc, slice_count, &base);
  arena->slices_committed = mi_arena_bitmap_init(slice_count, &base);
  arena->slices_dirty = mi_arena_bitmap_init(slice_count, &base);
  arena->slices_purge = mi_arena_bitmap_init(slice_count, &base);
  arena->pages_main.pages = mi_arena_bitmap_init(slice_count, &base);
  for (size_t i = 0; i < MI_ARENA_BIN_COUNT; i++) {
    arena->pages_main.pages_abandoned[i] = mi_arena_bitmap_init(slice_count, &base);
  }
  #if MI_PAGE_META_IS_SEPARATED && !MI_PAGE_META_IS_ALIGNED
  arena->pages_meta = (mi_page_t*)base;
  base += (slice_count * sizeof(mi_page_t));
  #else
  arena->pages_meta = NULL;
  #endif
  mi_assert_internal(_mi_size_of_slices(info_slices) >= (size_t)(base - mi_arena_start(arena)));

  // reserve our meta info (and reserve slices outside the memory area)
  #if MI_PAGE_META_IS_ALIGNED
  for(size_t i = 0; i < arena->slice_count; i += MI_PAGE_META_ALIGNED_COUNT) {
    // set all free slices (and skip the slices reserved for the page meta info)
    const size_t meta_slices = (i==0 ? info_slices : mi_arena_page_meta_aligned_slice_count());
    const size_t start_idx = (i + meta_slices);
    size_t count = MI_PAGE_META_ALIGNED_COUNT - meta_slices;
    if (start_idx < arena->slice_count) {
      if (count + start_idx > arena->slice_count) {
        count = arena->slice_count - start_idx;
        mi_assert_internal(count > 0);
      }
      mi_bbitmap_unsafe_setN(arena->slices_free, start_idx, count);
    }
  }
  #else
  mi_bbitmap_unsafe_setN(arena->slices_free, info_slices /* start */, arena->slice_count - info_slices);
  #endif
  if (memid.initially_committed) {
    mi_bitmap_unsafe_setN(arena->slices_committed, 0, arena->slice_count);
  }
  if (!memid.initially_zero) {
    mi_bitmap_unsafe_setN(arena->slices_dirty, 0, arena->slice_count);
  }

  if (!mi_arenas_add(subproc, arena, arena_id)) { return NULL;  }
  return arena;
}


/* -----------------------------------------------------------
  Reserve and manage OS memory (as an arena)
----------------------------------------------------------- */

static bool mi_manage_os_memory_ex2(mi_subproc_t* subproc, void* start, size_t size, int numa_node, bool exclusive,
  mi_memid_t memid, mi_commit_fun_t* commit_fun, void* commit_fun_arg, mi_arena_id_t* arena_id) mi_attr_noexcept
{
  // checks
  mi_assert(start!=NULL);
  if (arena_id != NULL) { *arena_id = _mi_arena_id_none(); }
  if (start==NULL) return false;
  if (!mi_is_aligned(start, MI_ARENA_ALIGNMENT)) {
    // we can align the start since the memid tracks the real base of the memory.
    void* const aligned_start = mi_align_up_ptr(start, MI_ARENA_ALIGNMENT);
    const size_t diff = (uint8_t*)aligned_start - (uint8_t*)start;
    if (diff >= size || (size - diff) < MI_ARENA_ALIGNMENT) {
      _mi_warning_message("after alignment, the size of the arena becomes too small (memory at %p with size %zu)\n", start, size);
      return false;
    }
    start = aligned_start;
    size = size - diff;
  }

  // allocate enough arena's to span the full memory area
  // the first arena is the owner, the rest are "sub-arena" (with `parent` pointing to the first one)
  size_t total_slice_count = mi_align_down(size / MI_ARENA_SLICE_SIZE, MI_BCHUNK_BITS);
  size_t total_size = _mi_size_of_slices(total_slice_count);
  if (total_size < MI_ARENA_MIN_SIZE) {
    _mi_warning_message("cannot use OS memory since it is not large enough (size %zu KiB, minimum required is %zu KiB)", size/MI_KiB, MI_ARENA_MIN_SIZE/MI_KiB);
    return false;
  }

  mi_arena_t* parent = NULL;
  do {
    // counting down on the total_slice_count
    size_t slice_count = total_slice_count;
    if (slice_count > MI_BITMAP_MAX_BIT_COUNT) {  // 16 GiB for now (with 64KiB slices)
      slice_count = MI_BITMAP_MAX_BIT_COUNT;
    }

    // initialize
    mi_arena_t* arena = mi_arena_initialize( subproc, start, slice_count, parent,
                                              (parent==NULL ? total_size : 0), numa_node, exclusive,
                                              memid, commit_fun, commit_fun_arg,
                                              (parent==NULL ? arena_id : NULL));
    if (arena==NULL) {
      // failed to initialize due to failing commit or too many arena's
      if (parent==NULL) {
        return false;
      }
      else {
        // partial success, but failed to use the full area..
        // todo: roll-back in this case? that requires a lock on the arena's array though
        mi_assert(_mi_size_of_slices(total_slice_count) <= parent->total_size);
        parent->total_size -= _mi_size_of_slices(total_slice_count);
        return true;
      }
    }

    // success
    if (parent==NULL) {
      parent = arena;
      memid.memkind = MI_MEM_NONE;
    }
    mi_assert(slice_count <= total_slice_count);
    total_slice_count -= slice_count;
    start = (uint8_t*)start + _mi_size_of_slices(slice_count);
  }
  while (total_slice_count > 0);

  return true;
}

bool mi_manage_os_memory_ex(void* start, size_t size, bool is_committed, bool is_pinned, bool is_zero, int numa_node, bool exclusive, mi_arena_id_t* arena_id) mi_attr_noexcept {
  mi_memid_t memid = _mi_memid_create(MI_MEM_EXTERNAL);
  memid.mem.os.base = start;
  memid.mem.os.size = size;
  memid.initially_committed = is_committed;
  memid.initially_zero = is_zero;
  memid.is_pinned = is_pinned;
  return mi_manage_os_memory_ex2(_mi_subproc(), start, size, numa_node, exclusive, memid, NULL, NULL, arena_id);
}

bool mi_manage_memory(void* start, size_t size, bool is_committed, bool is_pinned, bool is_zero, int numa_node, bool exclusive, mi_commit_fun_t* commit_fun, void* commit_fun_arg, mi_arena_id_t* arena_id) mi_attr_noexcept
{
  mi_memid_t memid = _mi_memid_create(MI_MEM_EXTERNAL);
  memid.mem.os.base = start;
  memid.mem.os.size = size;
  memid.initially_committed = is_committed;
  memid.initially_zero = is_zero;
  memid.is_pinned = is_pinned;
  return mi_manage_os_memory_ex2(_mi_subproc(), start, size, numa_node, exclusive, memid, commit_fun, commit_fun_arg, arena_id);
}


// Reserve a range of regular OS memory
static int mi_reserve_os_memory_ex2(mi_subproc_t* subproc, size_t size, bool commit, bool allow_large, bool exclusive, mi_arena_id_t* arena_id) {
  if (arena_id != NULL) *arena_id = _mi_arena_id_none();
  if (size <= MI_MAX_ALLOC_SIZE) {
    size = mi_align_up(size, MI_ARENA_SLICE_SIZE); // at least one slice
  }
  if (size > MI_MAX_ALLOC_SIZE) {
    _mi_error_message(EOVERFLOW, "memory reservation request is too large (size %zu)\n", size);
    return ENOMEM;
  }
  mi_memid_t memid;
  void* start = _mi_os_alloc_aligned(subproc, size, MI_ARENA_ALIGNMENT, commit, allow_large, &memid);
  if (start == NULL) return ENOMEM;
  if (!mi_manage_os_memory_ex2(subproc, start, size, -1 /* numa node */, exclusive, memid, NULL, NULL, arena_id)) {
    _mi_os_free_ex(subproc, start, size, commit, memid);
    _mi_verbose_message("failed to reserve %zu KiB memory\n", mi_divide_up(size, 1024));
    return ENOMEM;
  }
  _mi_verbose_message("reserved %zu KiB memory%s\n", mi_divide_up(size, 1024), memid.is_pinned ? " (in large os pages)" : "");
  // mi_debug_show_arenas(true, true, false);

  return 0;
}

// Reserve a range of regular OS memory
int mi_reserve_os_memory_ex(size_t size, bool commit, bool allow_large, bool exclusive, mi_arena_id_t* arena_id) mi_attr_noexcept {
  return mi_reserve_os_memory_ex2(_mi_subproc(), size, commit, allow_large, exclusive, arena_id);
}

// Manage a range of regular OS memory
bool mi_manage_os_memory(void* start, size_t size, bool is_committed, bool is_large, bool is_zero, int numa_node) mi_attr_noexcept {
  return mi_manage_os_memory_ex(start, size, is_committed, is_large, is_zero, numa_node, false /* exclusive? */, NULL);
}

// Reserve a range of regular OS memory
int mi_reserve_os_memory(size_t size, bool commit, bool allow_large) mi_attr_noexcept {
  return mi_reserve_os_memory_ex(size, commit, allow_large, false, NULL);
}


/* -----------------------------------------------------------
  Debugging
----------------------------------------------------------- */

// Return idx of the slice past the last used slice
static size_t mi_arena_used_slices(mi_arena_t* arena) {
  size_t idx;
  if (mi_bbitmap_bsr_inv(arena->slices_free, &idx)) {
    return (idx + 1);
  }
  else {
    return mi_arena_info_slices(arena);
  }
}

static size_t mi_debug_show_bfield(mi_bfield_t field, char* buf, size_t* k) {
  size_t bit_set_count = 0;
  for (int bit = 0; bit < MI_BFIELD_BITS; bit++) {
    bool is_set = ((((mi_bfield_t)1 << bit) & field) != 0);
    if (is_set) bit_set_count++;
    buf[*k] = (is_set ? 'x' : '.');
    *k = *k + 1;
  }
  return bit_set_count;
}

typedef enum mi_ansi_color_e {
  MI_BLACK = 30,
  MI_MAROON,
  MI_DARKGREEN,
  MI_ORANGE,
  MI_NAVY,
  MI_PURPLE,
  MI_TEAL,
  MI_GRAY,
  MI_DARKGRAY = 90,
  MI_RED,
  MI_GREEN,
  MI_YELLOW,
  MI_BLUE,
  MI_MAGENTA,
  MI_CYAN,
  MI_WHITE
} mi_ansi_color_t;

static void mi_debug_color(char* buf, size_t* k, mi_ansi_color_t color) {
  *k += _mi_snprintf(buf + *k, 32, "\x1B[%dm", (int)color);
}

static int mi_page_commit_usage(mi_page_t* page) {
  const size_t committed_size = mi_page_committed(page);
  const size_t used_size = mi_page_used(page) * mi_page_block_size(page);
  return (int)(used_size * 100 / committed_size);
}

static size_t mi_debug_show_page_bfield(char* buf, size_t* k, mi_arena_t* arena, size_t slice_index, long* pbit_of_page, mi_ansi_color_t* pcolor_of_page ) {
  size_t bit_set_count = 0;
  long bit_of_page = *pbit_of_page;
  mi_ansi_color_t color = *pcolor_of_page;
  mi_ansi_color_t prev_color = MI_GRAY;
  for (int bit = 0; bit < MI_BFIELD_BITS; bit++, bit_of_page--) {
    // bool is_set = ((((mi_bfield_t)1 << bit) & field) != 0);
    void* start = mi_arena_slice_start(arena, slice_index + bit);
    mi_page_t* page = _mi_safe_ptr_page(start);
    char c = ' ';
    if (page!=NULL && start==mi_page_slice_start(page)) {
      mi_assert_internal(bit_of_page <= 0);
      bit_set_count++;
      c = 'p';
      color = MI_GRAY;
      if (_mi_meta_is_meta_page(arena->subproc,page))  { c = 'm'; }
      else if (mi_page_is_singleton(page)) { c = 's'; }
      else if (mi_page_is_full(page)) { c = 'f'; }
      if (!mi_page_is_abandoned(page)) { c = _mi_toupper(c); }
      int commit_usage = mi_page_commit_usage(page);
      if (commit_usage < 25) { color = MI_MAROON; }
      else if (commit_usage < 50) { color = MI_ORANGE; }
      else if (commit_usage < 75) { color = MI_TEAL; }
      else color = MI_DARKGREEN;
      bit_of_page = (long)page->memid.mem.arena.slice_count;
    }
    else {
      c = '?';
      if (bit_of_page > 0) { c = '-'; }
      // else if (_mi_meta_is_meta_page(arena->subproc,start)) { c = 'm'; color = MI_GRAY; }
      else if (slice_index + bit < arena->info_slices) { c = 'i'; color = MI_GRAY; }
      #if MI_PAGE_META_IS_ALIGNED
      else if ((slice_index % MI_PAGE_META_ALIGNED_COUNT) == 0 && (size_t)bit <= mi_arena_page_meta_aligned_slice_count()) {
        { c = 'i'; color = MI_GRAY; }
      }
      #endif
      // else if (mi_bitmap_is_setN(arena->pages_purge, slice_index + bit, NULL)) { c = '*'; }
      else if (mi_bbitmap_is_setN(arena->slices_free, slice_index+bit,1)) {
        if (mi_bitmap_is_set(arena->slices_purge, slice_index + bit)) { c = '~'; color = MI_ORANGE; }
        else if (mi_bitmap_is_set(arena->slices_committed, slice_index + bit)) { c = '_'; color = MI_GRAY; }
        else { c = '.'; color = MI_GRAY; }
      }
      if (bit==MI_BFIELD_BITS-1 && bit_of_page > 1) { c = '>'; }
    }
    if (color != prev_color) {
      mi_debug_color(buf, k, color);
      prev_color = color;
    }
    buf[*k] = c; *k += 1;
  }
  mi_debug_color(buf, k, MI_GRAY);
  *pbit_of_page = bit_of_page;
  *pcolor_of_page = color;
  return bit_set_count;
}

static size_t mi_debug_show_chunks(const char* header1, const char* header2, const char* header3,
                                   size_t slice_count, size_t chunk_count,
                                   mi_bchunk_t* chunks, mi_bchunkmap_t* chunk_bins, bool invert, mi_arena_t* arena, bool narrow)
{
  _mi_raw_message("\x1B[37m%s%s%s (use/commit: \x1B[31m0 - 25%%\x1B[33m - 50%%\x1B[36m - 75%%\x1B[32m - 100%%\x1B[0m)\n", header1, header2, header3);
  const size_t fields_per_line = (narrow ? 2 : 4);
  const size_t used_slice_count = mi_arena_used_slices(arena);
  size_t bit_count = 0;
  size_t bit_set_count = 0;
  long bit_of_page = 0;
  mi_ansi_color_t color_of_page = MI_GRAY;
  for (size_t i = 0; i < chunk_count && bit_count < slice_count; i++) {
    char buf[5*MI_BCHUNK_BITS + 64]; mi_memzero(buf, sizeof(buf));
    if (bit_count > used_slice_count && i+2 < chunk_count) {
      const size_t diff = chunk_count - 1 - i;
      bit_count += diff*MI_BCHUNK_BITS;
      _mi_raw_message("  |\n");
      i = chunk_count-1;
    }

    size_t k = 0;

    if (i<10)        { buf[k++] = ('0' + (char)i); buf[k++] = ' '; buf[k++] = ' '; }
    else if (i<100)  { buf[k++] = ('0' + (char)(i/10)); buf[k++] = ('0' + (char)(i%10)); buf[k++] = ' '; }
    else if (i<1000) { buf[k++] = ('0' + (char)(i/100)); buf[k++] = ('0' + (char)((i%100)/10)); buf[k++] = ('0' + (char)(i%10)); }

    char chunk_kind = ' ';
    if (chunk_bins != NULL) {
      switch (mi_bbitmap_debug_get_bin(chunk_bins,i)) {
        case MI_CBIN_SMALL:  chunk_kind = 'S'; break;
        case MI_CBIN_MEDIUM: chunk_kind = 'M'; break;
        case MI_CBIN_LARGE:  chunk_kind = 'L'; break;
        case MI_CBIN_HUGE:   chunk_kind = 'H'; break;
        case MI_CBIN_OTHER:  chunk_kind = 'X'; break;
        default: chunk_kind = ' '; break; // suppress warning
        // case MI_CBIN_NONE: chunk_kind = 'N'; break;
      }
    }
    buf[k++] = chunk_kind;
    buf[k++] = ' ';

    for (size_t j = 0; j < MI_BCHUNK_FIELDS; j++) {
      if (j > 0 && (j % fields_per_line) == 0) {
        // buf[k++] = '\n'; mi_memset(buf+k,' ',7); k += 7;
        _mi_raw_message("  %s\n\x1B[37m", buf);
        mi_memzero(buf, sizeof(buf));
        mi_memset(buf, ' ', 5); k = 5;
      }
      if (bit_count < slice_count) {
        mi_bfield_t bfield = 0;
        if (chunks!=NULL) {
          bfield = chunks[i].bfields[j];
        }
        if (invert) bfield = ~bfield;
        size_t xcount = (chunks==NULL ? mi_debug_show_page_bfield(buf, &k, arena, bit_count, &bit_of_page, &color_of_page)
                                      : mi_debug_show_bfield(bfield, buf, &k));
        if (invert) xcount = MI_BFIELD_BITS - xcount;
        bit_set_count += xcount;
        buf[k++] = ' ';
      }
      else {
        mi_memset(buf + k, 'o', MI_BFIELD_BITS);
        k += MI_BFIELD_BITS;
      }
      bit_count += MI_BFIELD_BITS;
    }
    _mi_raw_message("  %s\n\x1B[37m", buf);
  }
  _mi_raw_message("\x1B[0m  total pages: %zu\n", bit_set_count);
  return bit_set_count;
}

//static size_t mi_debug_show_bitmap_binned(const char* header1, const char* header2, const char* header3, size_t slice_count,
//                                           mi_bitmap_t* bitmap, mi_bchunkmap_t* chunk_bins, bool invert, mi_arena_t* arena, bool narrow) {
//  return mi_debug_show_chunks(header1, header2, header3, slice_count, mi_bitmap_chunk_count(bitmap), &bitmap->chunks[0], chunk_bins, invert, arena, narrow);
//}

static void mi_debug_show_arenas_ex(mi_heap_t* heap, bool show_pages, bool narrow) mi_attr_noexcept {
  mi_subproc_t* subproc = heap->subproc;
  size_t max_arenas = mi_arenas_get_count(subproc);
  //size_t free_total = 0;
  //size_t slice_total = 0;
  //size_t abandoned_total = 0;
  size_t page_total = 0;
  for (size_t i = 0; i < max_arenas; i++) {
    mi_arena_t* arena = mi_atomic_load_ptr_acquire(mi_arena_t, &subproc->arenas[i]);
    if (arena == NULL) continue;
    mi_assert(arena->subproc == subproc);
    // slice_total += arena->slice_count;
    _mi_raw_message("%sarena %zu at %p: %zu slices (%zu MiB)%s%s, subproc: %zu, numa: %i\n",
        (arena->parent==NULL ? "" : "(sub)"), i, arena, arena->slice_count, (size_t)(_mi_size_of_slices(arena->slice_count)/MI_MiB),
        (arena->memid.is_pinned ? ", pinned" : ""), (arena->is_exclusive ? ", exclusive" : ""),
        arena->subproc->subproc_seq, arena->numa_node);
    //if (show_inuse) {
    //  free_total += mi_debug_show_bbitmap("in-use slices", arena->slice_count, arena->slices_free, true, NULL);
    //}
    //if (show_committed) {
    //  mi_debug_show_bitmap("committed slices", arena->slice_count, arena->slices_committed, false, NULL);
    //}
    // todo: abandoned slices
    //if (show_purge) {
    //  purge_total += mi_debug_show_bitmap("purgeable slices", arena->slice_count, arena->slices_purge, false, NULL);
    //}
    if (show_pages) {
      // mi_arena_pages_t* arena_pages = mi_heap_arena_pages(heap, arena);
      // if (arena_pages != NULL)
      {
        const char* header1 = "chunks (p:page, f:full, s:single, m:meta-data, i:arena-info, P,F,S,M:not abandoned, ~:free-purgable, _:free-committed, .:free-reserved)";
        const char* header2 = (narrow ? "\n       " : " ");
        const char* header3 = "(chunk bin: S:small, M : medium, L : large, X : other)";
        page_total += mi_debug_show_chunks(header1, header2, header3, arena->slice_count,
                                           mi_bbitmap_chunk_count(arena->slices_free), NULL,
                                           arena->slices_free->chunkmap_bins, false, arena, narrow);
      }
    }
  }
  // if (show_inuse)     _mi_raw_message("total inuse slices    : %zu\n", slice_total - free_total);
  // if (show_abandoned) _mi_raw_message("total abandoned slices: %zu\n", abandoned_total);
  if (show_pages) _mi_raw_message("total pages in arenas: %zu\n", page_total);
}

void mi_debug_show_arenas(void) mi_attr_noexcept {
  mi_debug_show_arenas_ex(mi_heap_main(), true /* show pages */, true /* narrow? */);
}

void mi_arenas_print(void) mi_attr_noexcept {
  mi_debug_show_arenas();
}


/* -----------------------------------------------------------
  Reserve a huge page arena.
----------------------------------------------------------- */
// reserve at a specific numa node
int mi_reserve_huge_os_pages_at_ex(size_t pages, int numa_node, size_t timeout_msecs, bool exclusive, mi_arena_id_t* arena_id) mi_attr_noexcept {
  if (arena_id != NULL) *arena_id = NULL;
  if (pages==0) return 0;
  if (numa_node < -1) numa_node = -1;
  if (numa_node >= 0) numa_node = numa_node % _mi_os_numa_node_count();
  mi_subproc_t* subproc = _mi_subproc();
  size_t hsize = 0;
  size_t pages_reserved = 0;
  mi_memid_t memid;
  void* p = _mi_os_alloc_huge_os_pages(subproc, pages, numa_node, timeout_msecs, &pages_reserved, &hsize, &memid);
  if (p==NULL || pages_reserved==0) {
    _mi_warning_message("failed to reserve %zu GiB huge pages\n", pages);
    return ENOMEM;
  }
  _mi_verbose_message("numa node %i: reserved %zu GiB huge pages (of the %zu GiB requested)\n", numa_node, pages_reserved, pages);

  if (!mi_manage_os_memory_ex2(subproc, p, hsize, numa_node, exclusive, memid, NULL, NULL, arena_id)) {
    _mi_os_free(subproc, p, hsize, memid);
    return ENOMEM;
  }
  return 0;
}

int mi_reserve_huge_os_pages_at(size_t pages, int numa_node, size_t timeout_msecs) mi_attr_noexcept {
  return mi_reserve_huge_os_pages_at_ex(pages, numa_node, timeout_msecs, false, NULL);
}

// reserve huge pages evenly among the given number of numa nodes (or use the available ones as detected)
int mi_reserve_huge_os_pages_interleave(size_t pages, size_t numa_nodes, size_t timeout_msecs) mi_attr_noexcept {
  if (pages == 0) return 0;

  // pages per numa node
  int numa_count = (numa_nodes > 0 && numa_nodes <= INT_MAX ? (int)numa_nodes : _mi_os_numa_node_count());
  if (numa_count <= 0) { numa_count = 1; }
  const size_t pages_per = pages / numa_count;
  const size_t pages_mod = pages % numa_count;
  const size_t timeout_per = (timeout_msecs==0 ? 0 : (timeout_msecs / numa_count) + 50);

  // reserve evenly among numa nodes
  for (int numa_node = 0; numa_node < numa_count && pages > 0; numa_node++) {
    size_t node_pages = pages_per;  // can be 0
    if ((size_t)numa_node < pages_mod) { node_pages++; }
    int err = mi_reserve_huge_os_pages_at(node_pages, numa_node, timeout_per);
    if (err) return err;
    if (pages < node_pages) {
      pages = 0;
    }
    else {
      pages -= node_pages;
    }
  }

  return 0;
}

int mi_reserve_huge_os_pages(size_t pages, double max_secs, size_t* pages_reserved) mi_attr_noexcept {
  MI_UNUSED(max_secs);
  _mi_warning_message("mi_reserve_huge_os_pages is deprecated: use mi_reserve_huge_os_pages_interleave/at instead\n");
  if (pages_reserved != NULL) *pages_reserved = 0;
  int err = mi_reserve_huge_os_pages_interleave(pages, 0, (size_t)(max_secs * 1000.0));
  if (err==0 && pages_reserved!=NULL) *pages_reserved = pages;
  return err;
}


/* -----------------------------------------------------------
  Unloading and reloading an arena.
----------------------------------------------------------- */
/*
static bool mi_arena_page_register(size_t slice_index, size_t slice_count, mi_arena_t* arena, void* arg) {
  MI_UNUSED(arg); MI_UNUSED(slice_count);
  mi_assert_internal(slice_count == 1);
  mi_page_t* page = mi_arena_page_at_slice(arena, slice_index);
  mi_assert_internal(mi_bitmap_is_setN(page->memid.mem.arena.arena->pages, page->memid.mem.arena.slice_index, 1));
  if (!_mi_page_map_register(page)) return false; // break
  mi_assert_internal(mi_ptr_page(page)==page);
  return true;
}

mi_decl_nodiscard static bool mi_arena_pages_reregister(mi_arena_t* arena) {
  return _mi_bitmap_forall_set(arena->pages, &mi_arena_page_register, arena, NULL);
}

mi_decl_export bool mi_arena_unload(mi_arena_id_t arena_id, void** base, size_t* accessed_size, size_t* full_size) {
  mi_arena_t* arena = _mi_arena_from_id(arena_id);
  if (arena==NULL) {
    return false;
  }
  else if (!arena->is_exclusive) {
    _mi_warning_message("cannot unload a non-exclusive arena (id %zu at %p)\n", arena_id, arena);
    return false;
  }
  else if (arena->memid.memkind != MI_MEM_EXTERNAL) {
    _mi_warning_message("can only unload managed arena's for external memory (id %zu at %p)\n", arena_id, arena);
    return false;
  }

  // find accessed size
  const size_t asize = _mi_size_of_slices(mi_arena_used_slices(arena));
  if (base != NULL) { *base = (void*)arena; }
  if (full_size != NULL) { *full_size = arena->memid.mem.os.size;  }
  if (accessed_size != NULL) { *accessed_size = asize; }

  // adjust abandoned page count
  mi_subproc_t* const subproc = arena->subproc;
  for (size_t bin = 0; bin < MI_ARENA_BIN_COUNT; bin++) {
    const size_t count = mi_bitmap_popcount(arena->pages_abandoned[bin]);
    if (count > 0) { mi_atomic_decrement_acq_rel(&subproc->abandoned_count[bin]); }
  }

  // unregister the pages
  _mi_page_map_unregister_range(arena, asize);

  // set arena entry to NULL
  const size_t count = mi_arenas_get_count(subproc);
  for(size_t i = 0; i < count; i++) {
    if (mi_arena_from_index(subproc, i) == arena) {
      mi_atomic_store_ptr_release(mi_arena_t, &subproc->arenas[i], NULL);
      if (i + 1 == count) { // try adjust the count?
        size_t expected = count;
        mi_atomic_cas_strong_acq_rel(&subproc->arena_count, &expected, count-1);
      }
      break;
    }
  }
  return true;
}

mi_decl_export bool mi_arena_reload(void* start, size_t size, mi_commit_fun_t* commit_fun, void* commit_fun_arg, mi_arena_id_t* arena_id) {
  // assume the memory area is already containing the arena
  if (arena_id != NULL) { *arena_id = _mi_arena_id_none(); }
  if (start == NULL || size == 0) return false;
  mi_arena_t* arena = (mi_arena_t*)start;
  mi_memid_t memid = arena->memid;
  if (memid.memkind != MI_MEM_EXTERNAL) {
    _mi_warning_message("can only reload arena's from external memory (%p)\n", arena);
    return false;
  }
  if (memid.mem.os.base != start) {
    _mi_warning_message("the reloaded arena base address differs from the external memory (arena: %p, external: %p)\n", arena, start);
    return false;
  }
  if (memid.mem.os.size != size) {
    _mi_warning_message("the reloaded arena size differs from the external memory (arena size: %zu, external size: %zu)\n", arena->memid.mem.os.size, size);
    return false;
  }
  if (!arena->is_exclusive) {
    _mi_warning_message("the reloaded arena is not exclusive\n");
    return false;
  }

  // re-initialize
  arena->is_exclusive = true;
  arena->commit_fun = commit_fun;
  arena->commit_fun_arg = commit_fun_arg;
  arena->subproc = _mi_subproc();
  if (!mi_arenas_add(arena->subproc, arena, arena_id)) {
    return false;
  }
  if (!mi_arena_pages_reregister(arena)) {
    // todo: clear arena entry in the subproc?
    return false;
  }

  // adjust abandoned page count
  for (size_t bin = 0; bin < MI_ARENA_BIN_COUNT; bin++) {
    const size_t count = mi_bitmap_popcount(arena->pages_abandoned[bin]);
    if (count > 0) { mi_atomic_decrement_acq_rel(&arena->subproc->abandoned_count[bin]); }
  }

  return true;
}

*/
