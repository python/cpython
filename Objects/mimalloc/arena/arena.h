/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#pragma once
#ifndef MI_ARENA_H
#define MI_ARENA_H

#include "mimalloc/util/libc.h"
#include "mimalloc/arena/arena.h"
#include "bitmap.h"

bool                    _mi_arena_reserve(mi_subproc_t* subproc, size_t req_size, bool allow_large, mi_arena_id_t* arena_id);
mi_decl_nodiscard bool  _mi_arena_commit(mi_subproc_t* subproc, mi_arena_t* arena, void* start, size_t size, bool* is_zero, size_t stat_already_committed);
mi_decl_noinline  void* _mi_arenas_try_alloc(mi_heap_t* heap, size_t slice_count, size_t alignment, bool commit, bool allow_large, mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid);
void*                   _mi_arena_os_alloc_aligned(mi_subproc_t* subproc, size_t size, size_t alignment, size_t align_offset, bool commit, bool allow_large, mi_arena_id_t req_arena_id, mi_memid_t* memid);

// Check if an arena is suitable for a request arena
static inline bool mi_arena_is_suitable(mi_arena_t* arena, mi_arena_t* req_arena) {
  if (arena == req_arena) return true;                         // they match
  if (arena == NULL) return false;
  if (req_arena == NULL && !arena->is_exclusive) return true;  // or the arena is not exclusive, and we didn't request a specific one
  if (arena->parent != NULL && arena->parent == req_arena) return true;  // sub-arena? (note that req_arena is never a sub arena)
  return false;
}

// Get the number of arenas in a subproc
static inline size_t mi_arenas_get_count(mi_subproc_t* subproc) {
  return mi_atomic_load_relaxed(&subproc->arena_count);
}

static inline mi_arena_t* mi_arena_from_index(mi_subproc_t* subproc, size_t idx) {
  mi_assert_internal(idx < mi_arenas_get_count(subproc));
  return mi_atomic_load_ptr_acquire(mi_arena_t, &subproc->arenas[idx]);
}

static inline size_t mi_arena_info_slices(mi_arena_t* arena) {
  return arena->info_slices;
}

// slices reserved for page meta info at the start of aligned chunks
static inline size_t mi_arena_page_meta_aligned_slice_count(void) {
  #if MI_PAGE_META_IS_ALIGNED
  return mi_divide_up(MI_PAGE_META_ALIGNED_COUNT * sizeof(mi_page_t), MI_ARENA_SLICE_SIZE);
  #else
  return 0;
  #endif
}

// Start of the arena memory area
static inline uint8_t* mi_arena_start(mi_arena_t* arena) {
  return ((uint8_t*)arena->start);
}

// Start of a slice
static inline uint8_t* mi_arena_slice_start(mi_arena_t* arena, size_t slice_index) {
  mi_assert_internal(slice_index < arena->slice_count);
  return (mi_arena_start(arena) + _mi_size_of_slices(slice_index));
}

// get the arena and slice span
static inline mi_arena_t* mi_arena_from_memid(mi_memid_t memid, size_t* slice_index, size_t* slice_count) {
  mi_assert_internal(memid.memkind == MI_MEM_ARENA);
  mi_arena_t* arena = memid.mem.arena.arena;
  if (slice_index!=NULL) { *slice_index = memid.mem.arena.slice_index; }
  if (slice_count!=NULL) { *slice_count = memid.mem.arena.slice_count; }
  return arena;
}

// initialize the bitmap for `slice_count` slices at base (and advance the base pointer)
static inline mi_bitmap_t* mi_arena_bitmap_init(size_t slice_count, uint8_t** base) {
  mi_bitmap_t* bitmap = (mi_bitmap_t*)(*base);
  *base = (*base) + mi_bitmap_init(bitmap, slice_count, true /* already zero */);
  return bitmap;
}

typedef bool (mi_forall_arena_fun_t)(mi_arena_t* arena, const void* arg, void** result);

bool mi_forall_arenas(mi_heap_t* heap, mi_arena_t* req_arena, size_t tseq, mi_forall_arena_fun_t* visit, const void* arg, void** result);
bool mi_forall_suitable_arenas(mi_heap_t* heap, mi_arena_t* req_arena, size_t tseq, bool match_numa, int numa_node,
                              bool allow_large, mi_forall_arena_fun_t* visit, const void* arg, void** result);


#endif
