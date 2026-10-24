/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/arena/arena-alloc.h"
#include "mimalloc/profile/sample-guarded.h"  // _mi_page_unguard_all
#include "mimalloc/arena/arena-page.h"
#include "arena.h"

/* -----------------------------------------------------------
  Util
----------------------------------------------------------- */

#if MI_DEBUG > 1
static bool mi_heap_has_page(mi_heap_t* heap, mi_arena_t* arena, mi_page_t* page) {
  mi_assert(arena->arena_idx < MI_MAX_ARENAS);
  mi_arena_pages_t* arena_pages = heap->arena_pages[arena->arena_idx];
  return (page->memid.memkind == MI_MEM_ARENA &&
          page->memid.mem.arena.arena == arena &&
          arena_pages != NULL &&
          mi_bitmap_is_setN(arena_pages->pages, page->memid.mem.arena.slice_index, 1));
}
#endif

mi_page_t* mi_arena_page_at_slice(mi_arena_t* arena, size_t slice_index) {
  mi_assert_internal(slice_index < arena->slice_count);
  #if MI_PAGE_META_IS_ALIGNED
  mi_page_t* const page = mi_aligned_ptr_page(mi_arena_slice_start(arena,slice_index)); // todo: optimize?
  return page;
  #else
  if (arena->pages_meta != NULL) {
    mi_page_t* const page = &arena->pages_meta[slice_index];
    #if MI_PAGE_META_SMALL_IS_ALIGNED
    // pages with small blocks still have the page at the start of the slice (and set the `block_size` in pages_meta to 0)
    if (page->block_size>0) return page;
    #else
    return page;
    #endif
  }
  // fall through (for MI_PAGE_META_SMALL_IS_ALIGNED)
  return (mi_page_t*)mi_arena_slice_start(arena,slice_index);
  #endif
}

static size_t mi_page_full_size(mi_page_t* page) {
  if (page->memid.memkind == MI_MEM_ARENA) {
    return page->memid.mem.arena.slice_count * MI_ARENA_SLICE_SIZE;
  }
  else if (mi_memid_is_os(page->memid) || page->memid.memkind == MI_MEM_EXTERNAL) {
    mi_assert_internal((uint8_t*)page->memid.mem.os.base <= (uint8_t*)page);
    const ptrdiff_t presize = (uint8_t*)page - (uint8_t*)page->memid.mem.os.base;
    mi_assert_internal((ptrdiff_t)page->memid.mem.os.size >= presize);
    return (presize > (ptrdiff_t)page->memid.mem.os.size ? 0 : page->memid.mem.os.size - presize);
  }
  else {
    return 0;
  }
}

/* -----------------------------------------------------------
  Arena pages: these bitmaps track abandoned pages per size
  bin, and the overall pages belonging to a particular heap.
----------------------------------------------------------- */

static mi_arena_pages_t* mi_heap_arena_pages(mi_heap_t* heap, mi_arena_t* arena) {
  mi_assert_internal(arena!=NULL);
  mi_assert_internal(heap!=NULL);
  mi_assert(arena->arena_idx < MI_MAX_ARENAS);
  return mi_atomic_load_ptr_acquire(mi_arena_pages_t, &heap->arena_pages[arena->arena_idx]);
}


static size_t mi_arena_pages_size(size_t slice_count, size_t* bitmap_base) {
  if (slice_count == 0) slice_count = MI_BCHUNK_BITS;
  mi_assert_internal((slice_count % MI_BCHUNK_BITS) == 0);
  const size_t base_size = mi_align_up(sizeof(mi_arena_pages_t), MI_BCHUNK_SIZE);
  const size_t bitmaps_count = 1 + MI_ARENA_BIN_COUNT; // pages, and abandoned
  const size_t bitmaps_size = bitmaps_count * mi_bitmap_size(slice_count, NULL);
  const size_t size = base_size + bitmaps_size;
  if (bitmap_base != NULL) *bitmap_base = base_size;
  return size;
}

// allocate initial arena_pages from the main heap
static mi_arena_pages_t* mi_arena_pages_alloc(mi_arena_t* arena) {
  const size_t slice_count = arena->slice_count;
  size_t bitmap_base = 0;
  const size_t size = mi_arena_pages_size(slice_count, &bitmap_base);
  mi_arena_pages_t* arena_pages = (mi_arena_pages_t*)mi_heap_zalloc_aligned(arena->subproc->heap_main, size, MI_BCHUNK_SIZE);
  if (arena_pages==NULL) return NULL;
  uint8_t* base = (uint8_t*)arena_pages + bitmap_base;
  mi_assert_internal(mi_is_aligned(base, MI_BCHUNK_SIZE));
  arena_pages->pages = mi_arena_bitmap_init(slice_count, &base);
  for (size_t i = 0; i < MI_ARENA_BIN_COUNT; i++) {
    arena_pages->pages_abandoned[i] = mi_arena_bitmap_init(slice_count, &base);
  }
  return arena_pages;
}

static mi_arena_t* mi_page_arena_pages(mi_page_t* page, size_t* slice_index, size_t* slice_count, mi_arena_pages_t** parena_pages) {
  // todo: maybe store the arena* directly in the page?
  mi_assert_internal(mi_page_is_owned(page));
  mi_arena_t* const arena = mi_arena_from_memid(page->memid, slice_index, slice_count);
  mi_assert_internal(arena != NULL);
  if (parena_pages != NULL) {
    mi_heap_t* heap = mi_page_heap(page);
    mi_arena_pages_t* const arena_pages = mi_heap_arena_pages(heap, arena);
    mi_assert_internal(arena_pages != NULL);
    mi_assert_internal(slice_index==NULL || mi_bitmap_is_set(arena_pages->pages, *slice_index));
    *parena_pages = arena_pages;
  }
  return arena;
}

static mi_arena_pages_t* mi_heap_ensure_arena_pages(mi_heap_t* heap, mi_arena_t* arena) {
  mi_assert_internal(arena!=NULL);
  mi_assert_internal(heap!=NULL);
  mi_assert(arena->arena_idx < MI_MAX_ARENAS);
  mi_arena_pages_t* arena_pages = mi_heap_arena_pages(heap, arena);
  if (arena_pages==NULL) {
    mi_lock(&heap->arena_pages_lock) {
      arena_pages = mi_atomic_load_ptr_acquire(mi_arena_pages_t, &heap->arena_pages[arena->arena_idx]);
      if (arena_pages == NULL) {  // still NULL?
        if (mi_is_heap_main(heap)) {
          // the page info for the main heap is always allocated as part of an arena
          arena_pages = &arena->pages_main;
        }
        else {
          // always allocate the arena pages info from the main heap
          // todo: allocate into the current arena?
          arena_pages = mi_arena_pages_alloc(arena);
        }
        mi_atomic_store_ptr_release(mi_arena_pages_t, &heap->arena_pages[arena->arena_idx], arena_pages);
      }
    }
  }
  if (mi_is_heap_main(heap)) { mi_assert(arena_pages != NULL); }  // can never fail
  return arena_pages;
}


/* -----------------------------------------------------------
  Claim abandoned pages
----------------------------------------------------------- */

static bool mi_arena_try_claim_abandoned(size_t slice_index, mi_arena_t* arena, bool* keep_abandoned) {
  // found an abandoned page of the right size
  mi_page_t* const page  = mi_arena_page_at_slice(arena, slice_index);
  // can we claim ownership?
  if (!mi_page_claim_ownership(page)) {
    // there was a concurrent free that reclaims this page ..
    // we need to keep it in the abandoned map as the free will call `mi_arena_page_unabandon`,
    // and wait for readers (us!) to finish. This is why it is very important to set the abandoned
    // bit again (or otherwise the unabandon will never stop waiting).
    *keep_abandoned = true;
    return false;
  }
  else {
    // yes, we can reclaim it, keep the abandoned map entry clear
    *keep_abandoned = false;
    return true;
  }
}


typedef struct mi_arena_abandoned_visit_info_s {
  mi_theap_t* theap;
  size_t      slice_count;
  size_t      block_size;
  size_t      bin;
} mi_arena_abandoned_visit_info_t;

static bool mi_arena_try_find_abandoned_visit(mi_arena_t* arena, const void* arg, void** result) {
  const mi_arena_abandoned_visit_info_t* info = (const mi_arena_abandoned_visit_info_t*)arg;
  mi_assert_internal(result != NULL);
  mi_theap_t* const theap = info->theap;
  mi_heap_t* const heap = mi_theap_heap(theap);
  const size_t tseq = theap->tld->thread_seq;
  mi_arena_pages_t* const arena_pages = mi_heap_arena_pages(heap, arena);
  if (arena_pages != NULL) {
    size_t slice_index;
    mi_bitmap_t* const bitmap = arena_pages->pages_abandoned[info->bin];

    if (mi_bitmap_try_find_and_claim(bitmap, tseq, &slice_index, &mi_arena_try_claim_abandoned, arena)) {
      // found an abandoned page of the right size
      // and claimed ownership.
      mi_page_t* page = mi_arena_page_at_slice(arena, slice_index);
      mi_assert_internal(mi_page_is_owned(page));
      mi_assert_internal(mi_page_is_abandoned(page));
      mi_assert_internal(mi_heap_has_page(heap, arena, page));
      mi_atomic_decrement_relaxed(&heap->abandoned_count[info->bin]);
      mi_theap_stat_decrease(theap, pages_abandoned, 1);
      mi_theap_stat_counter_increase(theap, pages_reclaim_on_alloc, 1);

      _mi_page_free_collect(page, false);  // update `used` count
      mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, info->slice_count));
      mi_assert_internal(mi_page_slice_committed(page) > 0 || mi_bitmap_is_setN(arena->slices_committed, slice_index, info->slice_count));
      mi_assert_internal(mi_bitmap_is_setN(arena->slices_dirty, slice_index, info->slice_count));
      mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
      mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
      mi_assert_internal(mi_page_block_size(page) == info->block_size);
      mi_assert_internal(!mi_page_is_full(page));
      *result = page;
      return false; // break
    }
  }
  return true; // continue
}

static mi_page_t* mi_arenas_page_try_find_abandoned(mi_theap_t* theap, size_t slice_count, size_t block_size)
{
  mi_heap_t* const heap = mi_theap_heap(theap);
  const size_t tseq = theap->tld->thread_seq;
  mi_arena_t* const req_arena = heap->exclusive_arena;

  const size_t bin = _mi_bin(block_size);
  if (bin >= MI_ARENA_BIN_COUNT) {
    return NULL; // singleton page size
  }

  // any abandoned in our size class?
  mi_assert_internal(heap != NULL);
  if (mi_atomic_load_relaxed(&heap->abandoned_count[bin]) == 0) {
    return NULL;
  }

  // search arena's
  const bool allow_large = true;
  const int  any_numa = -1;
  const bool match_numa = true;
  const mi_arena_abandoned_visit_info_t info = { theap, slice_count, block_size, bin };
  void* page = NULL;
  mi_forall_suitable_arenas(heap, req_arena, tseq, match_numa, any_numa, allow_large, &mi_arena_try_find_abandoned_visit, &info, &page);
  return (mi_page_t*)page;
}


/* -----------------------------------------------------------
  Allocate the memory area and page meta data for a fresh page
  (`mi_arenas_page_alloc_fresh_area`)
----------------------------------------------------------- */

// Offset of the first block when the page meta data is in front of the slices.
static size_t mi_page_meta_in_front_block_start(size_t block_size, bool os_align) {
  size_t offset;
  #if MI_GUARDED
  // in a guarded build, we align pages with blocks a multiple of an OS page size, to the OS page size
  // this ensures that all blocks in such pages are OS page size aligned (which is needed for the guard pages)
  const size_t os_page_size = _mi_os_page_size();
  mi_assert_internal(MI_PAGE_ALIGN >= os_page_size);
  if (!os_align && block_size % os_page_size == 0 && block_size > os_page_size /* at least 2 or more */ ) {
    offset = mi_align_up(mi_page_info_size(), os_page_size);
  }
  else
  #endif
  if (os_align) {
    offset = MI_PAGE_ALIGN;
  }
  else if (block_size != 0 && mi_is_power_of_two(block_size) && block_size <= MI_PAGE_MAX_START_BLOCK_ALIGN2) {
    // naturally align power-of-2 blocks up to MI_PAGE_MAX_START_BLOCK_ALIGN2 size (4KiB)
    offset = mi_align_up(mi_page_info_size(), block_size);
  }
  else if (block_size != 0 && block_size <= MI_SMALL_SIZE_MAX) {
    // align small blocks to their size
    offset = mi_align_up(mi_page_info_size(), block_size);
  }
  else if (block_size != 0 && (block_size % MI_PAGE_OSPAGE_BLOCK_ALIGN2) == 0) {
    // also align large pages that are a multiple of MI_PAGE_OSPAGE_BLOCK_ALIGN2 (4KiB)
    offset = mi_align_up(mi_page_info_size(), MI_PAGE_OSPAGE_BLOCK_ALIGN2);
  }
  else {
    // otherwise start after the info
    offset = mi_page_info_size();
  }
  return mi_align_up(offset,MI_MAX_ALIGN_SIZE);
}

// The usable size of a page area (excluding the guard page at the end in secure mode).
static size_t mi_arenas_page_noguard_size(size_t alloc_size) {
  #if MI_SECURE>=5
  mi_assert(alloc_size > _mi_os_secure_guard_page_size());
  return alloc_size - _mi_os_secure_guard_page_size();
  #else
  return alloc_size;
  #endif
}

// Can we offset the first block to spread cache sets? (see `MI_PAGE_CACHE_OFFSET`)
static bool mi_page_block_can_cache_offset(size_t block_size, bool os_align) {
  if (os_align || !mi_block_size_has_cache_offset(block_size)) return false;
  #if MI_GUARDED
  if (block_size % _mi_os_page_size() == 0) return false;  // guarded blocks must stay OS page aligned
  #endif
  return true;
}

static size_t mi_page_cache_offset_index(uint8_t* slice_start, size_t slice_count, size_t noffsets) {
  // Fibonacci hash of the page index: the top bits spread consecutive pages evenly over the offsets
  const size_t page_index = (size_t)((uintptr_t)slice_start / MI_ARENA_SLICE_SIZE) / slice_count;
  #if MI_SIZE_BITS >= 64
  const size_t hash = page_index * MI_ZU(0x9E3779B97F4A7C15);
  #else
  const size_t hash = page_index * MI_ZU(0x9E3779B9);
  #endif
  return (((hash >> (MI_SIZE_BITS - 16)) * noffsets) >> 16);
}

static size_t mi_page_small_block_cache_offset(uint8_t* slice_start, size_t slice_count, size_t block_size) {
  const size_t unit = mi_max(MI_PAGE_SMALL_CACHE_OFFSET_UNIT, mi_natural_alignment_of_size(block_size));
  if (unit > MI_PAGE_SMALL_CACHE_OFFSET_MAX) return 0;
  return mi_page_cache_offset_index(slice_start, slice_count, 1 + MI_PAGE_SMALL_CACHE_OFFSET_MAX/unit) * unit;
}

// Cache offset for the first block to spread the blocks of different pages over the cache sets (issue #1121).
// The offset fits in the unused space at the end of the page; if there is none, we skip the last block
// if that wastes less than 12.5% of the page (e.g. 64 KiB blocks in a 4 MiB page, but not 512 KiB blocks).
static size_t mi_page_block_cache_offset(uint8_t* slice_start, size_t slice_count, size_t block_size, size_t block_start, bool os_align) {
  if (MI_PAGE_CACHE_OFFSET == 0 || os_align) return 0;
  if (!mi_block_size_has_cache_offset(block_size)) return mi_page_small_block_cache_offset(slice_start, slice_count, block_size);
  if (!mi_page_block_can_cache_offset(block_size, os_align)) return 0;
  const size_t page_noguard_size = mi_arenas_page_noguard_size(_mi_size_of_slices(slice_count));
  mi_assert_internal(block_start + block_size <= page_noguard_size);
  size_t slack = (page_noguard_size - block_start) % block_size;
  if (slack < MI_PAGE_CACHE_OFFSET_UNIT && block_size < page_noguard_size/8) { slack += block_size; }
  const size_t noffsets = 1 + (slack < MI_PAGE_CACHE_OFFSET_MAX ? slack : MI_PAGE_CACHE_OFFSET_MAX) / MI_PAGE_CACHE_OFFSET_UNIT;
  return mi_page_cache_offset_index(slice_start, slice_count, noffsets) * MI_PAGE_CACHE_OFFSET_UNIT;
}

// Use the separate (zero initialized) `page_meta` if available and not preferring in front of the slices,
// and otherwise put the page meta data in front of the slices. Also determines the offset of the first block.
static mi_page_t* mi_page_meta_select(uint8_t* slice_start, size_t slice_count, mi_page_t* page_meta, size_t block_size, bool os_align, size_t* block_start) {
  mi_assert_internal(page_meta == NULL || page_meta->block_size == 0);
  mi_page_t* page;
  size_t offset;
  if (page_meta == NULL
      #if MI_PAGE_META_SMALL_IS_ALIGNED
      // small pages keep the page meta data in front of the slices for a fast `mi_free_small`
      // (note: it is important that the separate `page_meta->block_size == 0` for `mi_arena_page_at_slice`)
      || (!os_align && block_size <= MI_SMALL_MAX_OBJ_SIZE)
      #endif
  ) {
    // put the actual page meta info in front of the slices
    // (note: it is not initialized yet since the slices may not be committed yet)
    offset = mi_page_meta_in_front_block_start(block_size, os_align);
    page = (mi_page_t*)slice_start;
  }
  else {
    // use the separate page meta data
    mi_memzero_aligned(page_meta, sizeof(*page_meta));
    offset = 0;
    page = page_meta;
  }
  offset += mi_page_block_cache_offset(slice_start, slice_count, block_size, offset, os_align);
  mi_assert_internal(os_align || offset < MI_ARENA_SLICE_SIZE);  // so `mi_page_slice_start` can align down the page start
  *block_start = offset;
  return page;
}

#if MI_PAGE_META_IS_ALIGNED
// Ensure the aligned page meta data for the slices at `slice_start` is committed.
static bool mi_arena_page_meta_ensure_committed(mi_arena_t* arena, uint8_t* slice_start) {
  uint8_t* const meta_slices = (uint8_t*)mi_align_down_ptr(slice_start,MI_PAGE_META_ALIGNMENT);
  mi_assert_internal(meta_slices >= mi_arena_start(arena));
  const size_t meta_slice_index  = (meta_slices - mi_arena_start(arena)) / MI_ARENA_SLICE_SIZE;
  if mi_unlikely(mi_bitmap_is_clear(arena->slices_committed, meta_slice_index)) {
    // try to commit all page meta slices now
    const size_t meta_slice_count = mi_arena_page_meta_aligned_slice_count();
    // the following assertion does not hold in a concurrent setting..
    // mi_assert_internal(mi_bitmap_is_clearN(arena->slices_committed, meta_slice_index, meta_slice_count));
    const size_t commit_size = meta_slice_count * MI_ARENA_SLICE_SIZE;
    if (!_mi_arena_commit(arena->subproc, arena, meta_slices, commit_size, NULL, 0)) {
      return false;
    }
    mi_bitmap_setN(arena->slices_committed, meta_slice_index, meta_slice_count, NULL);
  }
  return true;
}
#endif

// Try to allocate slices from free space in the arena's, and determine the page meta data (`*ppage`) and the offset of the first block (`*pblock_start`).
static uint8_t* mi_arenas_page_alloc_from_slices(mi_theap_t* theap, size_t slice_count, size_t block_size, bool commit, bool allow_large,
                                                 mi_memid_t* memid, mi_arena_pages_t** parena_pages, mi_page_t** ppage, size_t* pblock_start) {
  mi_heap_t* const heap = mi_theap_heap(theap);
  mi_tld_t*  const tld  = theap->tld;
  const int numa_node = (heap->numa_node >= 0 ? heap->numa_node : tld->numa_node);

  uint8_t* const start = (uint8_t*)_mi_arenas_try_alloc(heap, slice_count, MI_ARENA_SLICE_ALIGN, commit, allow_large, heap->exclusive_arena, tld->thread_seq, numa_node, memid);
  if (start == NULL) return NULL;
  mi_arena_t* const arena = memid->mem.arena.arena;

  // ensure the arena pages (and the aligned page meta data)
  mi_arena_pages_t* const arena_pages = mi_heap_ensure_arena_pages(heap, arena);
  if (arena_pages==NULL
      #if MI_PAGE_META_IS_ALIGNED
      || !mi_arena_page_meta_ensure_committed(arena, start)
      #endif
     ) {
    _mi_arenas_free(heap->subproc, start, _mi_size_of_slices(slice_count), *memid); // roll back
    return NULL;
  }
  // note: the following assert should hold if we could check it atomically, but in a concurrent setting we may already allocate in slice_count
  // mi_assert_internal(mi_bitmap_is_clearN(arena_pages->pages, memid->mem.arena.slice_index, memid->mem.arena.slice_count));
  mi_assert_internal(mi_bitmap_is_clear(arena_pages->pages, memid->mem.arena.slice_index));
  // don't set yet: mi_bitmap_set(arena_pages->pages, memid->mem.arena.slice_index);
  *parena_pages = arena_pages;

  // the separate page meta data (if available)
  #if MI_PAGE_META_IS_ALIGNED
  mi_page_t* const page_meta = mi_aligned_ptr_page0(start);
  #else
  mi_assert_internal(MI_PAGE_META_IS_SEPARATED!=0 || arena->pages_meta == NULL);
  mi_page_t* const page_meta = (arena->pages_meta == NULL ? NULL : &arena->pages_meta[memid->mem.arena.slice_index]);
  #endif
  *ppage = mi_page_meta_select(start, slice_count, page_meta, block_size, false, pblock_start);
  return start;
}

#if MI_PAGE_META_IS_ALIGNED
// Allocate slices from the OS with the (committed and zero'd) page meta data at the start of the `MI_PAGE_META_ALIGNMENT` aligned area,
// and determine the page meta data (`*ppage`) and the offset of the first block (`*pblock_start`).
static uint8_t* mi_arenas_page_alloc_from_os(mi_heap_t* heap, size_t slice_count, size_t max_page_meta_count, size_t block_size, size_t block_alignment, bool os_align, bool commit, bool allow_large,
                                             mi_memid_t* memid, mi_page_t** ppage, size_t* pblock_start) {
  MI_UNUSED(allow_large);
  mi_arena_t* const req_arena = heap->exclusive_arena;
  const size_t alloc_size = _mi_size_of_slices(slice_count);
  if (block_alignment >= MI_PAGE_META_ALIGNMENT) {
    // if we allow alignment >= MI_PAGE_META_ALIGNMENT we need to substract 1 from a pointer
    // in mi_aligned_ptr_page (and test for (intptr_t)p < 0 instead of NULL). We avoid this by limiting the max alignment.
    _mi_warning_message("requested alignment is too large (%zu KiB)\n", block_alignment / MI_KiB);
    errno = EINVAL;
    return NULL;
    // page_offset = MI_PAGE_META_ALIGNMENT;
    // os_start = (uint8_t*)_mi_arena_os_alloc_aligned(heap->subproc, alloc_size + page_offset, block_alignment, MI_PAGE_META_ALIGNMENT /* align offset */, false /* commit */, false /* allow large */, req_arena, memid);
  }

  // offset in the block for the page area
  const size_t page_offset = (block_alignment < MI_PAGE_ALIGN ? MI_PAGE_ALIGN : block_alignment);
  uint8_t* const os_start = (uint8_t*)_mi_arena_os_alloc_aligned(heap->subproc, alloc_size + page_offset, MI_PAGE_META_ALIGNMENT, 0 /* align offset */, false /* commit */, false /* allow large */, req_arena, memid);
  if (os_start==NULL) return NULL;

  // commit page info and page
  const size_t min_page_count = mi_divide_up(page_offset + MI_ARENA_SLICE_SIZE,MI_ARENA_SLICE_SIZE) + max_page_meta_count;
  const size_t min_page_meta  = min_page_count * sizeof(mi_page_t);
  mi_assert_internal(min_page_meta < page_offset);
  bool is_zero;
  bool ok = _mi_arena_commit(heap->subproc,req_arena,os_start,min_page_meta,&is_zero,min_page_meta /* don't count in stats? */);
  if (ok && commit) {
    ok = _mi_arena_commit(heap->subproc,req_arena,os_start + page_offset,alloc_size,NULL,0);
  }
  if (!ok) { _mi_os_free(heap->subproc,os_start,alloc_size+page_offset,*memid); return NULL; }
  if (!is_zero && !memid->initially_zero) {
    mi_memzero_aligned(os_start,min_page_meta);
  }
  uint8_t* const start = os_start + page_offset;
  mi_assert_internal(mi_is_aligned(start,block_alignment));
  mi_assert_internal(mi_is_aligned(os_start,MI_PAGE_META_ALIGNMENT));
  mi_assert_internal(mi_align_down_ptr(start-1,MI_PAGE_META_ALIGNMENT) == os_start);
  mi_assert_internal((uint8_t*)mi_aligned_ptr_page0(start) < os_start + min_page_meta);
  memid->initially_committed = true; // so we don't commit again
  *ppage = mi_page_meta_select(start, slice_count, mi_aligned_ptr_page0(start), block_size, os_align, pblock_start);
  return start;
}
#else
// Allocate slices from the OS with the page meta data in front of the slices,
// and determine the page meta data (`*ppage`) and the offset of the first block (`*pblock_start`).
static uint8_t* mi_arenas_page_alloc_from_os(mi_heap_t* heap, size_t slice_count, size_t max_page_meta_count, size_t block_size, size_t block_alignment, bool os_align, bool commit, bool allow_large,
                                             mi_memid_t* memid, mi_page_t** ppage, size_t* pblock_start) {
  MI_UNUSED(max_page_meta_count);
  const size_t page_alignment = MI_ARENA_SLICE_ALIGN;
  const size_t alloc_size = _mi_size_of_slices(slice_count);
  uint8_t* start;
  if (os_align) {
    // note: slice_count already includes the page meta data (of MI_PAGE_ALIGN size) in front of the aligned block
    start = (uint8_t*)_mi_arena_os_alloc_aligned(heap->subproc, alloc_size, block_alignment, page_alignment /* align offset */, commit, allow_large, heap->exclusive_arena, memid);
    mi_assert_internal(start==NULL || mi_is_aligned(start + page_alignment, block_alignment));
  }
  else {
    start = (uint8_t*)_mi_arena_os_alloc_aligned(heap->subproc, alloc_size, page_alignment, 0 /* align offset */, commit, allow_large, heap->exclusive_arena, memid);
  }
  if (start == NULL) return NULL;
  // page meta data is always placed in front of the slices
  *ppage = mi_page_meta_select(start, slice_count, NULL, block_size, os_align, pblock_start);
  return start;
}
#endif

// Allocate the slices for a fresh page and determine its page meta data (`*ppage`) and the offset of the first block (`*pblock_start`).
static uint8_t* mi_arenas_page_alloc_fresh_area(mi_theap_t* theap, size_t slice_count, size_t max_page_meta_count, size_t block_size, size_t block_alignment, bool os_align, bool commit,
                                                mi_memid_t* memid, mi_arena_pages_t** parena_pages, mi_page_t** ppage, size_t* pblock_start ) {
  mi_assert_internal(parena_pages!=NULL && ppage!=NULL && pblock_start!=NULL);
  *parena_pages = NULL;
  *memid = _mi_memid_none();
  const bool allow_large = (MI_SECURE < 5); // 5 = guard page at end of each arena page
  mi_heap_t* const heap = mi_theap_heap(theap);

  // try to allocate from free space in arena's
  uint8_t* slice_start = NULL;
  if (!mi_option_is_enabled(mi_option_disallow_arena_alloc) &&       // allowed to allocate from arena's?
      !os_align &&                                                   // not large alignment
      slice_count <= mi_arena_max_object_size()/MI_ARENA_SLICE_SIZE) // and not too large
  {
    slice_start = mi_arenas_page_alloc_from_slices(theap, slice_count, block_size, commit, allow_large, memid, parena_pages, ppage, pblock_start);
  }

  // otherwise fall back to the OS
  if (slice_start == NULL) {
    slice_start = mi_arenas_page_alloc_from_os(heap, slice_count, max_page_meta_count, block_size, block_alignment, os_align, commit, allow_large, memid, ppage, pblock_start);
    if (slice_start == NULL) return NULL;
    mi_heap_stat_increase(heap,pages_os_allocated,1);
  }
  mi_assert_internal(mi_is_aligned(slice_start, MI_PAGE_ALIGN));
  return slice_start;
}


/* -----------------------------------------------------------
  Allocate and initialize a fresh page (`mi_arenas_page_alloc_fresh`)
----------------------------------------------------------- */

// Free a page without modifying page_bin stats
static void mi_arenas_page_free_prim(mi_page_t* page);

// Commit the start of the page up to and including the first block if the slices are not yet committed.
// Sets `*commit_size` to the committed size (or 0 if the slices were already committed).
static bool mi_arenas_page_commit_first_block(mi_theap_t* theap, mi_memid_t memid, uint8_t* slice_start, size_t block_start, size_t block_size, size_t page_noguard_size, size_t* commit_size) {
  *commit_size = 0;
  if (memid.initially_committed) return true;
  size_t csize = mi_align_up(block_start + block_size, mi_page_min_commit_size());
  if (csize > page_noguard_size) { csize = page_noguard_size; }
  bool is_zero = false;
  if mi_unlikely(!_mi_arena_commit(mi_theap_subproc(theap), mi_memid_arena(memid), slice_start, csize, &is_zero, 0)) {
    return false;
  }
  *commit_size = csize;
  return true;
}

// Prepare the page memory: zero the page meta data if it is in front of the slices, set the guard page, etc.
static void mi_arenas_page_init_memory(mi_theap_t* theap, mi_page_t* page, uint8_t* slice_start, size_t slice_count, size_t block_start, size_t page_noguard_size, mi_memid_t* memid)
{
  MI_UNUSED(theap); MI_UNUSED(slice_count); MI_UNUSED(block_start); MI_UNUSED(page_noguard_size);

  // zero initialize the page meta data
  if (!memid->initially_zero && (uint8_t*)page == slice_start) {
    mi_memzero_aligned(page, sizeof(*page));
  }

  // set the guard page
  #if MI_SECURE>=5
  if (memid->initially_committed) {
    _mi_os_secure_guard_page_set_at(mi_theap_subproc(theap), slice_start + page_noguard_size, *memid);
  }
  #endif

  // claimed free slices: initialize the page partly
  if (!memid->initially_zero && memid->initially_committed) {
    mi_track_mem_undefined(slice_start, slice_count * MI_ARENA_SLICE_SIZE);
  }
  else if (memid->initially_committed) {
    mi_track_mem_defined(slice_start, slice_count * MI_ARENA_SLICE_SIZE);
  }
  #if MI_DEBUG > 1
  if (memid->initially_zero && memid->initially_committed) {
    if (!mi_mem_is_zero(slice_start, page_noguard_size)) {
      _mi_error_message(EFAULT, "internal error: page memory was not zero initialized.\n");
      memid->initially_zero = false;
      if (block_start > 0) { mi_memzero_aligned(page, sizeof(*page)); }
    }
  }
  #endif
}

// Initialize the fields of the page meta data.
static void mi_arenas_page_init_meta(mi_theap_t* theap, mi_page_t* page, uint8_t* slice_start, size_t block_start, size_t block_size, bool os_align,
                                     size_t page_noguard_size, size_t commit_size, mi_memid_t memid)
{
  const size_t reserved = (os_align ? 1 : (page_noguard_size - block_start) / block_size);
  mi_assert_internal(reserved > 0 && reserved <= UINT16_MAX);

  // initialize the page start
  uint8_t* const start = slice_start + block_start;
  mi_assert_internal(start > (uint8_t*)page);
  page->page_offset = start - (uint8_t*)page;

  // initialize page meta-data
  page->reserved = (uint16_t)reserved;
  page->block_size = block_size;
  page->memid = memid;
  page->free_is_zero = memid.initially_zero;
  page->slice_pcommitted = (uint16_t)(commit_size / _mi_os_page_size());

  page->heap = mi_theap_heap(theap);
  mi_page_set_theap(page,theap);
  // mi_assert_internal(mi_page_theap(page) == _mi_heap_theap_peek(page->heap))
}

#if MI_PAGE_META_IS_ALIGNED
// Point the aligned page meta entries of the slices to the actual page meta data.
static void mi_arenas_page_link_aligned_meta(mi_page_t* page, uint8_t* slice_start, size_t slice_count, size_t max_page_meta_count)
{
  mi_page_t* const page_meta = mi_aligned_ptr_page0(slice_start);
  mi_atomic_store_ptr_release(mi_page_t,&page_meta->self,page);
  if (slice_count > 1) {
    // at least two for large singleton blocks as guard pages can have a large offset beyond a single slice
    for(size_t i = 1; i < max_page_meta_count; i++) {
      mi_assert_internal(page_meta[i].block_size == 0);
      mi_atomic_store_ptr_release(mi_page_t,&page_meta[i].self,page);
    }
  }
  #if MI_DEBUG>1
  mi_page_t* pstart = mi_aligned_ptr_page0(slice_start);
  mi_assert_internal(mi_atomic_load_ptr_acquire(mi_page_t,&pstart->self)==page);
  if (page->reserved>1) {
    mi_page_t* pend = mi_aligned_ptr_page0(slice_start + (slice_count*MI_ARENA_SLICE_SIZE) - 1);
    mi_assert_internal(mi_atomic_load_ptr_acquire(mi_page_t,&pend->self)==page);
  }
  #endif
}
#endif

// Register a fresh page in its arena pages and the page map, and claim ownership.
// On failure the page is freed.
static bool mi_arenas_page_register(mi_theap_t* theap, mi_page_t* page, mi_arena_pages_t* arena_pages)
{
  // register in the arena_pages
  if (arena_pages!=NULL) {
    mi_assert_internal(page->memid.memkind == MI_MEM_ARENA);
    mi_bitmap_set(arena_pages->pages, page->memid.mem.arena.slice_index);
  }

  // and own it
  mi_page_claim_ownership(page);

  // register in the page map
  if mi_unlikely(!_mi_page_map_register(page)) {
    mi_arenas_page_free_prim(page);
    return false;
  }

  // stats
  mi_theap_stat_increase(theap, pages, 1);
  mi_theap_stat_increase(theap, page_bins[_mi_page_stats_bin(page)], 1);
  return true;
}

// Allocate a fresh page
static mi_page_t* mi_arenas_page_alloc_fresh(mi_theap_t* theap, size_t slice_count, size_t block_size, size_t block_alignment, bool commit)
{
  const bool os_align           = (block_alignment > MI_PAGE_MAX_OVERALLOC_ALIGN);
  const bool singleton          = (os_align || block_size > MI_LARGE_MAX_OBJ_SIZE);
  const size_t max_page_meta_count = (singleton && slice_count > 2 ? 2 : slice_count);

  // allocate the slices and determine the page meta data
  mi_memid_t memid              = _mi_memid_none();
  mi_arena_pages_t* arena_pages = NULL;
  mi_page_t* page               = NULL;
  size_t block_start            = 0;
  uint8_t* const slice_start    = mi_arenas_page_alloc_fresh_area(theap,slice_count,max_page_meta_count,block_size,block_alignment,os_align,commit,&memid,&arena_pages,&page,&block_start);
  if (!slice_start) return NULL;
  mi_assert_internal(block_size < MI_MAX_ALIGN_SIZE || block_start % MI_MAX_ALIGN_SIZE == 0);
  // all blocks must be naturally aligned (as relied upon by `mi_malloc_aligned`)
  mi_assert_internal(block_size % mi_natural_alignment_of_size(block_size) == 0);
  mi_assert_internal(mi_is_aligned(slice_start + block_start, mi_natural_alignment_of_size(block_size)));

  // commit the first block (if needed)
  const size_t alloc_size        = _mi_size_of_slices(slice_count);
  const size_t page_noguard_size = mi_arenas_page_noguard_size(alloc_size);
  size_t commit_size = 0;
  if (!mi_arenas_page_commit_first_block(theap, memid, slice_start, block_start, block_size, page_noguard_size, &commit_size)) {
    _mi_arenas_free(mi_theap_subproc(theap), slice_start, alloc_size, memid);
    return NULL;
  }
  mi_assert_internal((commit && commit_size==0) || (!commit && (commit_size <= UINT16_MAX * _mi_os_page_size())));

  // initialize the page memory and meta data
  mi_arenas_page_init_memory(theap, page, slice_start, slice_count, block_start, page_noguard_size, &memid);
  mi_arenas_page_init_meta(theap, page, slice_start, block_start, block_size, os_align, page_noguard_size, commit_size, memid);
  #if MI_PAGE_META_IS_ALIGNED
  mi_arenas_page_link_aligned_meta(page, slice_start, slice_count, max_page_meta_count);
  #endif
  mi_assert_internal(page->free==NULL);
  mi_assert_internal(((uint8_t*)page != slice_start) == mi_page_meta_is_separated(page));
  mi_assert_internal(mi_page_slice_start(page) == slice_start);
  mi_assert_internal(mi_page_size(page) <= page_noguard_size);

  // now we can register the page (and use `mi_arenas_page_free_prim` on error)
  if (!mi_arenas_page_register(theap, page, arena_pages)) return NULL;

  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page),MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_block_size(page) == block_size);
  #if MI_PAGE_META_SMALL_IS_ALIGNED
  // `mi_free_small` finds the page meta data by aligning a block pointer down to `MI_SMALL_PAGE_SIZE`
  if (mi_page_is_small(page)) {
    mi_assert_internal((uint8_t*)page == slice_start && mi_is_aligned(page, MI_SMALL_PAGE_SIZE));
    mi_assert_internal(mi_page_start(page) + mi_page_size(page) <= (uint8_t*)page + MI_SMALL_PAGE_SIZE);
  }
  #endif
  // mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(mi_page_is_owned(page));
  return page;
}



/* -----------------------------------------------------------
  Allocate page (`_mi_arenas_page_alloc`)
----------------------------------------------------------- */

// Allocate a regular small/medium/large page.
static mi_page_t* mi_arenas_page_regular_alloc(mi_theap_t* theap, size_t slice_count, size_t block_size)
{
  // 1. look for an abandoned page
  mi_page_t* page = mi_arenas_page_try_find_abandoned(theap, slice_count, block_size);
  if (page != NULL) {
    return page;  // return as abandoned
  }

  // 2. find a free block, potentially allocating a new arena
  const long commit_on_demand = mi_option_get(mi_option_page_commit_on_demand);
  const bool commit = (mi_page_min_commit_size() >= slice_count * MI_ARENA_SLICE_SIZE ||           // always commit small pages
                       (slice_count >= _mi_slice_count_of_size(UINT16_MAX * _mi_os_page_size())) ||   // always commit pages too large to hold a 32-bit slice_committed
                        (commit_on_demand == 2 && _mi_os_has_overcommit()) || (commit_on_demand == 0));
  page = mi_arenas_page_alloc_fresh(theap, slice_count, block_size, 1, commit);
  if (page == NULL) return NULL;

  mi_assert_internal(page->memid.memkind != MI_MEM_ARENA || page->memid.mem.arena.slice_count == slice_count);
  if (!_mi_page_init(theap, page)) {
    _mi_arenas_page_free(page,theap);
    return NULL;
  }

  return page;
}

// Allocate a page containing one block (very large, or with large alignment)
static mi_page_t* mi_arenas_page_singleton_alloc(mi_theap_t* theap, size_t block_size, size_t block_alignment)
{
  const bool os_align = (block_alignment > MI_PAGE_MAX_OVERALLOC_ALIGN);
  #if MI_PAGE_META_IS_ALIGNED
  const size_t info_size = 0;
  #else
  const size_t info_size = (os_align ? MI_PAGE_ALIGN : mi_page_info_size());
  #endif
  // room for the cache offset of the block (committed but not touched)
  const size_t cache_offset_size = (mi_page_block_can_cache_offset(block_size, os_align) ? MI_PAGE_CACHE_OFFSET_MAX : 0);
  #if MI_SECURE < 2
  const size_t slice_count = _mi_slice_count_of_size(info_size + cache_offset_size + block_size);
  #else
  const size_t slice_count = _mi_slice_count_of_size(mi_align_up(info_size + cache_offset_size + block_size, _mi_os_secure_guard_page_size()) + _mi_os_secure_guard_page_size());
  #endif

  mi_page_t* page = mi_arenas_page_alloc_fresh(theap, slice_count, block_size, block_alignment, true /* commit singletons always */);
  if (page == NULL) return NULL;

  mi_assert(page->reserved == 1);
  if (!_mi_page_init(theap, page)) {
    _mi_arenas_page_free(page,theap);
    return NULL;
  }

  return page;
}


mi_page_t* _mi_arenas_page_alloc(mi_theap_t* theap, size_t block_size, size_t block_alignment) {
  mi_page_t* page;
  // semi static assert: ensure that all non-singleton block size bins are covered.
  mi_assert(_mi_bin(MI_LARGE_MAX_OBJ_SIZE)  < MI_ARENA_BIN_COUNT);
  if mi_unlikely(block_alignment > MI_PAGE_MAX_OVERALLOC_ALIGN) {
    mi_assert_internal(mi_is_power_of_two(block_alignment));
    page = mi_arenas_page_singleton_alloc(theap, block_size, block_alignment);
  }
  else if (block_size <= MI_SMALL_MAX_OBJ_SIZE) {
    page = mi_arenas_page_regular_alloc(theap, _mi_slice_count_of_size(MI_SMALL_PAGE_SIZE), block_size);
  }
  else if (mi_block_size_is_medium(block_size)) {
    page = mi_arenas_page_regular_alloc(theap, _mi_slice_count_of_size(MI_MEDIUM_PAGE_SIZE), block_size);
  }
  #if MI_ENABLE_LARGE_PAGES
  else if (block_size <= MI_LARGE_MAX_OBJ_SIZE) {
    page = mi_arenas_page_regular_alloc(theap, _mi_slice_count_of_size(MI_LARGE_PAGE_SIZE), block_size);
  }
  #endif
  else {
    page = mi_arenas_page_singleton_alloc(theap, block_size, block_alignment);
  }
  if mi_unlikely(page == NULL) {
    return NULL;
  }
  // mi_assert_internal(page == NULL || _mi_page_segment(page)->subproc == tld->subproc);
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(block_alignment <= MI_PAGE_MAX_OVERALLOC_ALIGN || mi_is_aligned(mi_page_start(page), block_alignment));

  return page;
}


/* -----------------------------------------------------------
  Free a page
----------------------------------------------------------- */

static void mi_arenas_page_free_prim(mi_page_t* page) {
  mi_assert_internal(_mi_page_qsbr_is_clear(page));
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_all_free(page));
  mi_assert_internal(page->next==NULL && page->prev==NULL);

  #if MI_DEBUG>1
  if (page->memid.memkind==MI_MEM_ARENA && !mi_page_is_full(page)) {
    size_t bin = _mi_bin(mi_page_block_size(page));
    size_t slice_index;
    size_t slice_count;
    mi_arena_pages_t* arena_pages = NULL;
    mi_arena_t* const arena = mi_page_arena_pages(page, &slice_index, &slice_count, &arena_pages);
    mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));
    mi_assert_internal(mi_page_slice_committed(page) > 0 || mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count));
    mi_assert_internal(bin >= MI_ARENA_BIN_COUNT || mi_bitmap_is_clearN(arena_pages->pages_abandoned[bin], slice_index, 1));
    mi_assert_internal(mi_bitmap_is_setN(arena_pages->pages, slice_index, 1));
    // note: we cannot check for `!mi_page_is_abandoned_and_mapped` since that may
    // be (temporarily) not true if the free happens while trying to reclaim
    // see `mi_arena_try_claim_abandoned`
  }
  #endif

  // unregister page
  _mi_page_map_unregister(page);

  // recommit guard page at the end?
  // we must do this since we may later allocate large spans over this page and cannot have a guard page in between
  #if MI_SECURE >= 5
  if (!page->memid.is_pinned) {
    _mi_os_secure_guard_page_reset_before(mi_page_subproc(page), mi_page_slice_start(page) + mi_page_full_size(page), page->memid);
  }
  #endif

  // and free
  if (page->memid.memkind == MI_MEM_ARENA) {
    mi_arena_pages_t* arena_pages;
    size_t slice_index;
    size_t slice_count; MI_UNUSED(slice_count);
    mi_arena_t* const arena = mi_page_arena_pages(page, &slice_index, &slice_count, &arena_pages);
    mi_assert_internal(arena_pages!=NULL);
    mi_assert_internal(arena->subproc == mi_page_subproc(page));
    mi_bitmap_clear(arena_pages->pages, slice_index);
    const size_t slice_committed = mi_page_slice_committed(page);
    if (slice_committed > 0) {
      // if committed on-demand, set the commit bits to account commit properly
      mi_assert_internal(mi_page_full_size(page) >= slice_committed);
      const size_t total_slices = slice_committed / MI_ARENA_SLICE_SIZE;  // conservative
      //mi_assert_internal(mi_bitmap_is_clearN(arena->slices_committed, slice_index, total_slices));
      mi_assert_internal(slice_count >= total_slices);
      if (total_slices > 0) {
        mi_bitmap_setN(arena->slices_committed, slice_index, total_slices, NULL);
      }
      // any left over?
      const size_t extra = slice_committed % MI_ARENA_SLICE_SIZE;
      if (extra > 0) {
        // pretend it was decommitted already
        mi_subproc_stat_decrease(arena->subproc, committed, extra);
      }
    }
    else {
      mi_assert_internal(mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count));
    }
  }
  else {
    mi_heap_stat_decrease(page->heap, pages_os_allocated, 1);
  }
  if (mi_page_meta_is_separated(page)) { page->block_size = 0; }  // for assertion checking
  _mi_arenas_free( mi_page_subproc(page), mi_page_slice_start(page), mi_page_full_size(page), page->memid);
}

void _mi_arenas_page_free(mi_page_t* page, mi_theap_t* current_theapx) {
  mi_assert_internal(_mi_page_qsbr_is_clear(page));
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_all_free(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(page->next==NULL && page->prev==NULL);
  mi_assert_internal(mi_theap_matches_thread(current_theapx));

  mi_heap_t* const heap = mi_page_heap(page);
  mi_theapx_stat_decrease(heap, current_theapx, page_bins[_mi_page_stats_bin(page)], 1);
  mi_theapx_stat_decrease(heap, current_theapx, pages, 1);
  _mi_page_free_collect(page,false);  // update used count for cross-thread free's
  _mi_page_update_stats(page);        // and update the stats
  mi_arenas_page_free_prim(page);
}


/* -----------------------------------------------------------
  Abandon a page
----------------------------------------------------------- */


// release ownership of a page. This may free the page if all blocks were concurrently
// freed in the meantime. Returns true if the page was freed.
static bool mi_abandoned_page_unown(mi_page_t* page, mi_theap_t* current_theapx) {
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(mi_theap_matches_thread(current_theapx));
  mi_thread_free_t tf_new;
  mi_thread_free_t tf_old = mi_atomic_load_relaxed(&page->xthread_free);
  do {
    mi_assert_internal(mi_tf_is_owned(tf_old));
    while mi_unlikely(mi_tf_block(tf_old) != NULL) {
      _mi_page_free_collect(page, false);  // update used
      if (mi_page_all_free(page)) {
        if (!_mi_page_qsbr_should_delay(page)) {
          _mi_page_qsbr_clear(page);
          _mi_arenas_page_unabandon(page, current_theapx);
          _mi_arenas_page_free(page, current_theapx);
          return true;
        }
        #if MI_QSBR
        else if (page->memid.memkind == MI_MEM_ARENA &&
                 !mi_page_is_singleton(page) && !mi_page_is_abandoned_mapped(page))
        {
          // if empty and qsbr delayed, we re-abandon to mapped so it can be reused
          _mi_arenas_page_unabandon(page, current_theapx);
          _mi_arenas_page_abandon(page, current_theapx);   // cannot recurse due to `!mi_page_is_abandoned_mapped(page)`
          return false;
        }
        #endif
      }
      tf_old = mi_atomic_load_relaxed(&page->xthread_free);
    }
    mi_assert_internal(mi_tf_block(tf_old)==NULL);
    tf_new = mi_tf_create(NULL, false);
  } while (!mi_atomic_cas_weak_acq_rel(&page->xthread_free, &tf_old, tf_new));
  return false;
}


void _mi_arenas_page_abandon(mi_page_t* page, mi_theap_t* current_theapx) {
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(MI_QSBR || !mi_page_all_free(page));
  mi_assert_internal(page->next==NULL && page->prev == NULL);
  mi_assert_internal(mi_theap_matches_thread(current_theapx));
  // mi_assert_internal(current_theap == _mi_page_associated_theap(page));
  #if MI_QSBR
  mi_assert_internal(!page->qsbr_in_queue && page->qsbr_next == NULL && page->qsbr_prev == NULL);
  #endif

  // note: somewhat expensive to update here, but might be good as then we attribute
  // the current allocations/frees to the current thread/theap. Otherwise it might be
  // reclaimed later in another thread/theap and those allocations/frees get attributed there...
  _mi_page_update_stats(page);

  // add to abandoned?
  mi_heap_t* heap = mi_page_heap(page);
  if (page->memid.memkind==MI_MEM_ARENA && !mi_page_is_full(page)
      #if MI_QSBR
      && !mi_page_is_singleton(page) // an arena singleton has size > MI_LARGE_MAX_OBJ_SIZE and bin==MI_BIN_HUGE
      #endif
     ) {
    // make available for allocations
    size_t bin = _mi_bin(mi_page_block_size(page));
    mi_assert_internal(bin < MI_ARENA_BIN_COUNT);
    if (bin < MI_ARENA_BIN_COUNT) { // paranoia
      size_t slice_index;
      size_t slice_count;
      mi_arena_pages_t* arena_pages = NULL;
      mi_arena_t* const arena = mi_page_arena_pages(page, &slice_index, &slice_count, &arena_pages); MI_UNUSED(arena);

      mi_assert_internal(!mi_page_is_singleton(page));
      mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));
      mi_assert_internal(mi_page_slice_committed(page) > 0 || mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count));
      mi_assert_internal(mi_bitmap_is_setN(arena->slices_dirty, slice_index, slice_count));

      mi_page_set_abandoned_mapped(page);
      const bool was_clear = mi_bitmap_set(arena_pages->pages_abandoned[bin], slice_index);
      MI_UNUSED(was_clear); mi_assert_internal(was_clear);
      mi_atomic_increment_relaxed(&heap->abandoned_count[bin]);
      mi_theapx_stat_increase(heap, current_theapx, pages_abandoned, 1);
      mi_abandoned_page_unown(page, current_theapx);
      return;
    }
  }
  // otherwise,
  // page is full (or a singleton), or the page is OS/externally allocated
  // leave as is; it will be reclaimed when an object is free'd in the page
  // but for non-arena pages, add to the subproc list so these can be visited
  if (page->memid.memkind != MI_MEM_ARENA) {
    mi_lock(&heap->os_abandoned_pages_lock) {
      // push in front
      page->prev = NULL;
      page->next = heap->os_abandoned_pages;
      if (page->next != NULL) { page->next->prev = page; }
      heap->os_abandoned_pages = page;
    }
    mi_theapx_stat_increase(heap, current_theapx, pages_os_abandoned, 1);
  }
  mi_theapx_stat_increase(heap, current_theapx, pages_abandoned, 1);
  mi_abandoned_page_unown(page, current_theapx);
}


// this is called from `alloc/free.c:mi_free_try_collect_mt` only.
bool _mi_arenas_page_try_reabandon_to_mapped(mi_page_t* page) {
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(!mi_page_is_abandoned_mapped(page));
  mi_assert_internal(!mi_page_is_full(page));
  mi_assert_internal(!mi_page_all_free(page));
  mi_assert_internal(!mi_page_is_singleton(page));
  if (mi_page_is_full(page) || mi_page_is_abandoned_mapped(page) || page->memid.memkind != MI_MEM_ARENA) {
    return false;
  }
  else {
    // do not use _mi_heap_theap as we may call this during shutdown of threads and don't want to reinitialize the theap
    mi_theap_t* const theapx = _mi_page_associated_theap_peek(page); // can be NULL
    mi_heap_t* const heap = mi_page_heap(page);
    // if (theapx==NULL) return false;
    mi_theapx_stat_counter_increase(heap, theapx, pages_reabandon_full, 1);
    mi_theapx_stat_adjust_decrease(heap, theapx, pages_abandoned, 1);  // adjust as we are not abandoning fresh
    _mi_arenas_page_abandon(page, theapx);
    return true;
  }
}

// called from `mi_free` if trying to unabandon an abandoned page
void _mi_arenas_page_unabandon(mi_page_t* page, mi_theap_t* current_theapx) {
  mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
  mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
  mi_assert_internal(mi_page_is_owned(page));
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(mi_theap_matches_thread(current_theapx));

  mi_heap_t* const heap = mi_page_heap(page);
  if (mi_page_is_abandoned_mapped(page)) {
    mi_assert_internal(page->memid.memkind==MI_MEM_ARENA);
    // remove from the abandoned map
    const size_t bin = _mi_bin(mi_page_block_size(page));
    mi_assert_internal(bin < MI_ARENA_BIN_COUNT);
    size_t slice_index;
    size_t slice_count;
    mi_arena_pages_t* arena_pages;
    mi_arena_t* arena = mi_page_arena_pages(page, &slice_index, &slice_count, &arena_pages);  MI_UNUSED(arena);

    mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));
    mi_assert_internal(mi_page_slice_committed(page) > 0 || mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count));

    // this busy waits until a concurrent reader (from alloc_abandoned) is done
    mi_bitmap_clear_once_set(arena->subproc, arena_pages->pages_abandoned[bin], slice_index);
    mi_page_clear_abandoned_mapped(page);
    mi_atomic_decrement_relaxed(&heap->abandoned_count[bin]);
  }
  else {
    // page is full (or a singleton), page is OS allocated
    // if not an arena page, remove from the subproc os pages list
    if (page->memid.memkind != MI_MEM_ARENA) {
      mi_lock(&heap->os_abandoned_pages_lock) {
        if (page->prev != NULL) { page->prev->next = page->next; }
        if (page->next != NULL) { page->next->prev = page->prev; }
        if (heap->os_abandoned_pages == page) { heap->os_abandoned_pages = page->next; }
        page->next = NULL;
        page->prev = NULL;
      }
      mi_theapx_stat_decrease(heap, current_theapx, pages_os_abandoned, 1);
    }
  }
  mi_theapx_stat_decrease(heap, current_theapx, pages_abandoned, 1);
}


/* -----------------------------------------------------------
  Visit all pages and blocks in a heap
----------------------------------------------------------- */

typedef struct mi_heap_visit_info_s {
  mi_heap_t*          heap;
  mi_block_visit_fun* visitor;
  void*               arg;
  bool                visit_blocks;
  bool                abandoned_only;
} mi_heap_visit_info_t;

static bool mi_heap_visit_page(mi_page_t* page, const mi_heap_visit_info_t* vinfo) {
  mi_heap_area_t area;
  #if MI_QSBR
  const bool claimed = (mi_page_is_abandoned(page) && mi_page_claim_ownership(page));
  #endif
  _mi_page_free_collect(page, true);  // collect cross-thread free'd blocks so `area.used` is precise.
  #if MI_QSBR
  if (claimed && mi_abandoned_page_unown(page, NULL)) return true;
  #endif
  _mi_heap_area_init(&area, page);
  mi_assert_internal(vinfo->heap == mi_page_heap(page));
  if (!vinfo->visitor(vinfo->heap, &area, NULL, area.block_size, vinfo->arg)) {
    return false; // break
  }
  if (vinfo->visit_blocks) {
    return _mi_theap_area_visit_blocks(&area, page, vinfo->visitor, vinfo->arg); // continue or break
  }
  else {
    return true; // continue
  }
}

static bool mi_heap_visit_page_at(size_t slice_index, size_t slice_count, mi_arena_t* arena, void* arg) {
  MI_UNUSED(slice_count);
  const mi_heap_visit_info_t* vinfo = (const mi_heap_visit_info_t*)arg;
  mi_page_t* page = mi_arena_page_at_slice(arena, slice_index);
  return mi_heap_visit_page(page, vinfo); // continue or break
}

static bool mi_heap_visit_arena(mi_arena_t* arena, const void* arg, void** result) {
  MI_UNUSED(result);
  const mi_heap_visit_info_t* vinfo = (const mi_heap_visit_info_t*)arg;
  mi_heap_visit_info_t visit_info = *vinfo;
  mi_heap_t* const heap = vinfo->heap;
  mi_arena_pages_t* arena_pages = mi_heap_arena_pages(heap, arena);
  if (arena_pages == NULL) return true; // continue
  if (vinfo->abandoned_only) {
    for (size_t bin = 0; bin < MI_ARENA_BIN_COUNT; bin++) {
      // TODO: if we had a single abandoned page map as well, this can be faster. Maybe for CPython?
      if (mi_atomic_load_relaxed(&heap->abandoned_count[bin]) > 0) {
        if (!_mi_bitmap_forall_set(arena_pages->pages_abandoned[bin], &mi_heap_visit_page_at, arena, &visit_info)) return false; // break
      }
    }
    return true; // continue
  }
  else {
    return _mi_bitmap_forall_set(arena_pages->pages, &mi_heap_visit_page_at, arena, &visit_info); // continue or break
  }
}

// The caller holds heap->theaps_lock to prevent thread-exit abandonment and theap removal.
static bool mi_heap_visit_owned_os_blocks(mi_heap_t* heap, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  const mi_heap_visit_info_t visit_info = { heap, visitor, arg, visit_blocks, false };
  for (mi_theap_t* theap = heap->theaps; theap != NULL; theap = theap->hnext) {
    if (theap->page_count_os == 0) continue;
    for (size_t bin = 0; bin <= MI_BIN_FULL; bin++) {
      mi_page_t* page = theap->pages[bin].first;
      while (page != NULL) {
        mi_page_t* const next = page->next;
        if (page->memid.memkind != MI_MEM_ARENA && !mi_heap_visit_page(page, &visit_info)) return false;
        page = next;
      }
    }
  }
  return true;
}

static bool mi_heap_visit_blocks_locked(mi_heap_t* heap, bool abandoned_only, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  // visit all pages in a heap
  // Other threads must not access this heap; thread-exit abandonment is blocked by the heap lock.
  const mi_heap_visit_info_t visit_info = { heap, visitor, arg, visit_blocks, abandoned_only };
  bool ok = mi_forall_arenas(heap, NULL, 0, &mi_heap_visit_arena, &visit_info, NULL);
  if (!ok) return false;

  // visit abandoned pages in OS allocated memory
  mi_page_t* page = NULL;
  mi_lock(&heap->os_abandoned_pages_lock) {
    page = heap->os_abandoned_pages;
  }
  while (ok && page != NULL) {
    mi_page_t* next = page->next;  // read upfront in case the visitor frees the page
    ok = mi_heap_visit_page(page, &visit_info);
    page = next;
  }

  // visit owned OS allocated blocks in the theaps
  if (ok && !abandoned_only) {
    ok = mi_heap_visit_owned_os_blocks(heap, visit_blocks, visitor, arg);
  }
  return ok;
}

bool _mi_heap_visit_blocks(mi_heap_t* heap, bool abandoned_only, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  mi_assert(visitor!=NULL);
  if (visitor==NULL) return false;
  if (heap==NULL) { heap = mi_heap_main(); }
  bool ok;
  mi_lock(&heap->theaps_lock) {
    ok = mi_heap_visit_blocks_locked(heap, abandoned_only, visit_blocks, visitor, arg);
  }
  return ok;
}

bool mi_heap_visit_blocks(mi_heap_t* heap, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  return _mi_heap_visit_blocks(heap, false, visit_blocks, visitor, arg);
}

bool mi_heap_visit_abandoned_blocks(mi_heap_t* heap, bool visit_blocks, mi_block_visit_fun* visitor, void* arg) {
  return _mi_heap_visit_blocks(heap, true, visit_blocks, visitor, arg);
}

#if MI_QSBR
static bool mi_heap_collect_abandoned_page(mi_page_t* page) {
  if (!mi_page_is_abandoned(page) || !mi_page_claim_ownership(page)) return true;
  _mi_page_free_collect(page, true);
  if (mi_page_all_free(page) && !_mi_page_qsbr_should_delay(page)) {
    _mi_page_qsbr_clear(page);
    _mi_arenas_page_unabandon(page, NULL);
    _mi_arenas_page_free(page, NULL);
  }
  else {
    mi_abandoned_page_unown(page, NULL);
  }
  return true;
}

static bool mi_heap_collect_abandoned_page_at(size_t slice_index, size_t slice_count, mi_arena_t* arena, void* arg) {
  MI_UNUSED(slice_count); MI_UNUSED(arg);
  return mi_heap_collect_abandoned_page(mi_arena_page_at_slice(arena, slice_index));
}

static bool mi_heap_collect_abandoned_arena(mi_arena_t* arena, const void* arg, void** result) {
  MI_UNUSED(result);
  mi_heap_t* heap = (mi_heap_t*)arg;
  mi_arena_pages_t* arena_pages = mi_heap_arena_pages(heap, arena);
  if (arena_pages == NULL) return true;
  return _mi_bitmap_forall_set(arena_pages->pages, &mi_heap_collect_abandoned_page_at, arena, NULL);
}

void _mi_heap_collect_abandoned(mi_heap_t* heap) {
  mi_assert_internal(heap != NULL);
  mi_lock(&heap->theaps_lock) {
    // Use the all-pages map: abandoned size-class maps exclude singletons and full pages.
    mi_forall_arenas(heap, NULL, 0, &mi_heap_collect_abandoned_arena, heap, NULL);
    mi_page_t* page = NULL;
    mi_lock(&heap->os_abandoned_pages_lock) { page = heap->os_abandoned_pages; }
    while (page != NULL) {
      mi_page_t* next = page->next;
      mi_heap_collect_abandoned_page(page);
      page = next;
    }
  }
}
#endif


typedef struct mi_heap_delete_visit_info_s {
  mi_heap_t*  heap_target;
  mi_theap_t* theap_target;
  mi_theap_t* theap;
} mi_heap_delete_visit_info_t;

static bool mi_heap_delete_page(const mi_heap_t* heap, const mi_heap_area_t* area, void* block, size_t block_size, void* arg) {
  MI_UNUSED(block); MI_UNUSED(block_size); MI_UNUSED(heap);
  const mi_heap_delete_visit_info_t* info = (const mi_heap_delete_visit_info_t*)arg;
  mi_heap_t*  heap_target           = info->heap_target;
  mi_theap_t* const theap           = NULL; // info->theap;       mi_assert_internal(mi_theap_heap(theap) == heap);
  mi_page_t*  const page            = (mi_page_t*)area->reserved1;

  mi_page_claim_ownership(page);       // claim ownership
  _mi_page_qsbr_clear(page); // deletion requires an unprotected heap; destruction requires no readers
  if (mi_page_is_abandoned(page)) {
    _mi_arenas_page_unabandon(page,theap);
  }
  else {
    page->next = page->prev = NULL;    // yikes.. better not to try to access this from a thread later on..
    mi_page_set_theap(page,NULL);      // set threadid to abandoned
  }
  mi_assert_internal(mi_page_is_abandoned(page));
  mi_assert_internal(mi_page_is_owned(page));

  if (mi_page_used(page)==0) {
    // free the page
    _mi_arenas_page_free(page, theap);
  }
  else if (heap_target==NULL) {
    #if MI_GUARDED
    _mi_page_unguard_all(page);          // remove potential interior guard pages
    #endif
    // destroy the page
    _mi_page_free_collect(page, false); // collect the thread-free list first while `used` still counts those blocks (as `_mi_arenas_page_free` collects it again) (see #1420)
    mi_page_used_reset(page);           // note: invariant `|local_free| + |free| == reserved - used`  does not hold in this case
    _mi_arenas_page_free(page, theap);
  }
  else {
    // move the page to `heap_target` as an abandoned page
    // first remove it from the current heap
    const size_t sbin = _mi_page_stats_bin(page);
    mi_arena_t* arena = NULL;
    size_t slice_index = 0;
    if (page->memid.memkind == MI_MEM_ARENA) {
      size_t slice_count;
      mi_arena_pages_t* arena_pages = NULL;
      arena = mi_page_arena_pages(page, &slice_index, &slice_count, &arena_pages);
      mi_assert_internal(mi_bitmap_is_set(arena_pages->pages, slice_index));
      mi_bitmap_clear(arena_pages->pages, slice_index);
    }
    else {
      // os allocated
      mi_assert_internal(mi_memid_is_os(page->memid) && page->next == NULL);
    }
    if (theap != NULL) {
      mi_theap_stat_decrease(theap, page_bins[sbin], 1);
      mi_theap_stat_decrease(theap, pages, 1);
    }
    else {
      mi_heap_stat_decrease((mi_heap_t*)heap, page_bins[_mi_page_stats_bin(page)], 1);
      mi_heap_stat_decrease((mi_heap_t*)heap, pages, 1);
    }
    mi_theap_t* theap_target = info->theap_target;

    // and then add it to the new target heap
    if (arena != NULL) {
      mi_arena_pages_t* arena_pages_target = mi_heap_ensure_arena_pages(heap_target, arena);
      if mi_unlikely(arena_pages_target==NULL) {
        // if we cannot allocate this, we move it to the main heap instead (which does not require allocation)
        heap_target = _mi_arena_heap_main(arena);
        theap_target = mi_heap_theap(heap_target);  // todo: find through theap_target tld?
        arena_pages_target = mi_heap_ensure_arena_pages(heap_target, arena);
        mi_assert_internal(arena_pages_target!=NULL);
      }
      mi_assert_internal(mi_bitmap_is_clear(arena_pages_target->pages, slice_index));
      mi_bitmap_set(arena_pages_target->pages, slice_index);
    }
    page->heap = heap_target;
    mi_theap_stat_increase(theap_target, page_bins[sbin], 1);
    mi_theap_stat_increase(theap_target, pages, 1);

    // and abandon in the new heap
    _mi_arenas_page_abandon(page,theap_target);
  }
  return true; // continue
}

static void mi_heap_delete_pages(mi_heap_t* heap, mi_heap_t* heap_target) {
  mi_theap_t* const theap_target = (heap_target != NULL ? _mi_heap_theap(heap_target) : NULL);
  // mi_theap_t* const theap = _mi_heap_theap(heap);
  mi_heap_delete_visit_info_t info = { heap_target, theap_target, NULL };
  _mi_heap_visit_blocks(heap, false, false, &mi_heap_delete_page, &info);
  #if MI_DEBUG>1
  // no more arena pages?
  for (size_t i = 0; i < MI_MAX_ARENAS; i++) {
    mi_arena_pages_t* const arena_pages = mi_atomic_load_ptr_relaxed(mi_arena_pages_t, &heap->arena_pages[i]);
    if (arena_pages!=NULL) {
      mi_assert_internal(mi_bitmap_is_all_clear(arena_pages->pages));
    }
  }
  // nor os abandoned pages?
  mi_lock(&heap->os_abandoned_pages_lock) {

    mi_assert_internal(heap->os_abandoned_pages == NULL);
  }
  // nor arena abandoned pages?
  for (size_t i = 0; i < MI_ARENA_BIN_COUNT; i++) {
    mi_assert_internal(mi_atomic_load_relaxed(&heap->abandoned_count[i])==0);
  }
  #endif
}

void _mi_heap_move_pages(mi_heap_t* heap_from, mi_heap_t* heap_to) {
  if (mi_is_heap_main(heap_from)) return;
  if (heap_to==NULL) { heap_to = heap_from->subproc->heap_main; }
  mi_heap_delete_pages(heap_from, heap_to);
}

void _mi_heap_destroy_pages(mi_heap_t* heap_from) {
  if (mi_is_heap_main(heap_from)) return;
  mi_heap_delete_pages(heap_from, NULL);
}
