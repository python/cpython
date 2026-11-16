/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/internal.h"
#include "mimalloc/prim.h"  // mi_prim_get_default_heap

#include <string.h>     // memset

// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

static bool mi_malloc_is_naturally_aligned( size_t size, size_t alignment ) {
  // objects up to `MI_MAX_ALIGN_GUARANTEE` are allocated aligned to their size (see `segment.c:_mi_segment_page_start`).
  mi_assert_internal(mi_alignment_is_valid(alignment));
  if (alignment > size) return false;
  if (alignment <= MI_MAX_ALIGN_SIZE) return true;
  const size_t bsize = mi_good_size(size);
  return (bsize <= MI_MAX_ALIGN_GUARANTEE && (bsize & (alignment-1)) == 0);
}

#if MI_GUARDED
static mi_decl_noinline mi_decl_restrict void* mi_heap_malloc_guarded_aligned(mi_heap_t* heap, size_t size, size_t alignment, bool zero, mi_page_t** ppage) mi_attr_noexcept {
  // use over allocation for guarded blocksl
  mi_assert_internal(alignment > 0 && alignment < MI_BLOCK_ALIGNMENT_MAX);
  if mi_unlikely(alignment >= MI_BLOCK_ALIGNMENT_MAX || size > (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE - alignment)) {
    _mi_error_message(EOVERFLOW, "(guarded) aligned allocation request is too large (size %zu, alignment %zu)\n", size, alignment);
    return NULL;
  }
  const size_t oversize = size + alignment - 1;
  void* const base = _mi_heap_malloc_guarded(heap, oversize, zero, ppage);
  if (base==NULL) return NULL;
  void* const p = _mi_align_up_ptr(base, alignment);
  mi_track_align(base, p, (uint8_t*)p - (uint8_t*)base, size);
  mi_assert_internal(mi_usable_size(p) >= size);
  mi_assert_internal(_mi_is_aligned(p, alignment));
  return p;
}

static void* mi_heap_malloc_zero_no_guarded(mi_heap_t* heap, size_t size, bool zero, mi_page_t** ppage) {
  const size_t rate = heap->guarded_sample_rate;
  // only write if `rate!=0` so we don't write to the constant `_mi_heap_empty`
  if (rate != 0) { heap->guarded_sample_rate = 0; }
  void* p = _mi_heap_malloc_zero_ex(heap, size, zero, 0, ppage);
  if (rate != 0) { heap->guarded_sample_rate = rate; }
  return p;
}
#else
static void* mi_heap_malloc_zero_no_guarded(mi_heap_t* heap, size_t size, bool zero, mi_page_t** ppage) {
  return _mi_heap_malloc_zero_ex(heap, size, zero, 0, ppage);
}
#endif

// Fallback aligned allocation that over-allocates -- split out for better codegen
static mi_decl_noinline void* mi_heap_malloc_zero_aligned_at_overalloc(mi_heap_t* const heap, const size_t size, const size_t alignment, const size_t offset, const bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  mi_assert_internal(size <= (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE));
  mi_assert_internal(mi_alignment_is_valid(alignment));

  void* p;
  size_t oversize;
  mi_page_t* page;
  if mi_unlikely(alignment > MI_BLOCK_ALIGNMENT_MAX) {
    // use OS allocation for very large alignment and allocate inside a huge page (dedicated segment with 1 page)
    // This can support alignments >= MI_SEGMENT_SIZE by ensuring the object can be aligned at a point in the
    // first (and single) page such that the segment info is `MI_SEGMENT_SIZE` bytes before it (so it can be found by aligning the pointer down)
    if mi_unlikely(offset != 0) {
      // todo: cannot support offset alignment for very large alignments yet
      _mi_error_message(EOVERFLOW, "aligned allocation with a very large alignment cannot be used with an alignment offset (size %zu, alignment %zu, offset %zu)\n", size, alignment, offset);
      return NULL;
    }
    oversize = (size <= MI_SMALL_SIZE_MAX ? MI_SMALL_SIZE_MAX + 1 /* ensure we use generic malloc path */ : size);
    // note: no guarded as alignment > 0
    p = _mi_heap_malloc_zero_ex(heap, oversize, false, alignment, &page); // the page block size should be large enough to align in the single huge page block
    // zero afterwards as only the area from the aligned_p may be committed!
    if (p == NULL) return NULL;
  }
  else {
    // otherwise over-allocate
    mi_assert_internal(size <= (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE) && alignment <= MI_BLOCK_ALIGNMENT_MAX);
    mi_assert_internal(size < SIZE_MAX - alignment); // `oversize` cannot overflow
    oversize = (size < MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : size) + alignment - 1;  // adjust for size <= 16; with size 0 and alignment 64k, we would allocate a 64k block and pointing just beyond that.
    p = mi_heap_malloc_zero_no_guarded(heap, oversize, zero, &page);
    if (p == NULL) return NULL;
  }
  mi_assert_internal(page == _mi_ptr_page(p));
  if (ppage!=NULL) { *ppage = page; }

  // .. and align within the allocation
  const uintptr_t align_mask = alignment - 1;  // for any x, `(x & align_mask) == (x % alignment)`
  const uintptr_t poffset = ((uintptr_t)p + offset) & align_mask;
  const uintptr_t adjust  = (poffset == 0 ? 0 : alignment - poffset);
  mi_assert_internal(adjust < alignment);
  void* aligned_p = (void*)((uintptr_t)p + adjust);
  if (aligned_p != p) {
    mi_page_set_has_aligned(page, true);
    // if (pblock_size!=NULL) {
    //   mi_assert_internal(*pblock_size > adjust);
    //   if (*pblock_size > adjust) { *pblock_size = *pblock_size - adjust; }
    //   mi_assert_internal(*pblock_size >= size);
    // }
    #if MI_GUARDED
    // set tag to aligned so mi_usable_size works with guard pages
    if (adjust >= sizeof(mi_block_t)) {
      mi_block_t* const block = (mi_block_t*)p;
      block->next = MI_BLOCK_TAG_ALIGNED;
    }
    #endif
    _mi_padding_shrink(page, (mi_block_t*)p, adjust + size);
  }
  // todo: expand padding if overallocated ?

  mi_assert_internal(mi_page_usable_block_size(page) >= adjust + size);
  mi_assert_internal(((uintptr_t)aligned_p + offset) % alignment == 0);
  mi_assert_internal(mi_usable_size(aligned_p)>=size);
  mi_assert_internal(mi_usable_size(p) == mi_usable_size(aligned_p)+adjust);
  #if MI_DEBUG > 1
  mi_page_t* const apage = _mi_ptr_page(aligned_p);
  void* unalign_p = _mi_page_ptr_unalign(apage, aligned_p);
  mi_assert_internal(p == unalign_p);
  #endif

  // now zero the block if needed
  if (alignment > MI_BLOCK_ALIGNMENT_MAX) {
    // for the tracker, on huge aligned allocations only the memory from the start of the large block is defined
    mi_track_mem_undefined(aligned_p, size);
    if (zero) {
      _mi_memzero(aligned_p, mi_usable_size(aligned_p));  // cannot be aligned due to abitrary offset... (todo: require offset to be a multiple of sizeof(void*)?)
    }
  }

  if (p != aligned_p) {
    mi_track_align(p,aligned_p,adjust,mi_usable_size(aligned_p));
    #if MI_GUARDED
    mi_track_mem_defined(p, sizeof(mi_block_t));
    #endif
  }
  return aligned_p;
}

// Generic primitive aligned allocation -- split out for better codegen
static mi_decl_noinline void* mi_heap_malloc_zero_aligned_at_generic(mi_heap_t* const heap, const size_t size, const size_t alignment, const size_t offset, const bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  mi_assert_internal(mi_alignment_is_valid(alignment));
  // we don't allocate more than MI_MAX_ALLOC_SIZE (see <https://sourceware.org/ml/libc-announce/2019/msg00001.html>)
  if mi_unlikely(size > (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE)) {
    _mi_error_message(EINVAL, "aligned allocation request is too large (size %zu, alignment %zu)\n", size, alignment);
    return NULL;
  }

  // use regular allocation if it is guaranteed to fit the alignment constraints.
  // this is important to try as the fast path in `mi_heap_malloc_zero_aligned` only works when there exist
  // a page with the right block size, and if we always use the over-alloc fallback that would never happen.
  if (offset == 0 && mi_malloc_is_naturally_aligned(size,alignment)) {
    void* p = mi_heap_malloc_zero_no_guarded(heap, size, zero, ppage);
    mi_assert_internal(p == NULL || ((uintptr_t)p % alignment) == 0);
    const bool is_aligned_or_null = (((uintptr_t)p) & (alignment-1))==0;
    if mi_likely(is_aligned_or_null) {
      return p;
    }
    else {
      // this should never happen if the `mi_malloc_is_naturally_aligned` check is correct..
      mi_assert(false);
      mi_free(p);
    }
  }

  // fall back to over-allocation
  return mi_heap_malloc_zero_aligned_at_overalloc(heap,size,alignment,offset,zero,ppage);
}


static mi_decl_cold mi_decl_noinline void* mi_error_bad_alignment(size_t size, size_t alignment, size_t offset) {
  _mi_error_message(EINVAL, "aligned allocation requires the alignment to be a power-of-two (size %zu, alignment %zu, offset %zu)\n", size, alignment, offset);
  return NULL;
}

// Primitive aligned allocation
static void* mi_heap_malloc_zero_aligned_at(mi_heap_t* const heap, const size_t size,
                                            const size_t alignment, const size_t offset, const bool zero,
                                            mi_page_t** ppage) mi_attr_noexcept
{
  // note: we don't require `size > offset`, we just guarantee that the address at offset is aligned regardless of the allocated size.
  if mi_unlikely(!mi_alignment_is_valid(alignment)) { // require power-of-two and multiple of void* (see <https://en.cppreference.com/w/c/memory/aligned_alloc#Notes>)
    return mi_error_bad_alignment(size, alignment, offset);
  }

  #if MI_GUARDED
  if (offset==0 && alignment < MI_BLOCK_ALIGNMENT_MAX && mi_heap_malloc_use_guarded(heap,size)) {
    return mi_heap_malloc_guarded_aligned(heap, size, alignment, zero, ppage);
  }
  #endif

  // try first if there happens to be a small block available with just the right alignment
  if mi_likely(size <= MI_SMALL_SIZE_MAX && alignment <= size) {
    const uintptr_t align_mask = alignment-1;       // for any x, `(x & align_mask) == (x % alignment)`
    const size_t padsize = size + MI_PADDING_SIZE;
    mi_page_t* page = _mi_heap_get_free_small_page(heap, padsize);
    if mi_likely(page->free != NULL) {
      const bool is_aligned = (((uintptr_t)page->free + offset) & align_mask)==0;
      if mi_likely(is_aligned)
      {
        if (ppage!=NULL) { *ppage = page; }
        void* p = (zero ? _mi_page_malloc_zeroed(heap,page,padsize) : _mi_page_malloc(heap,page,padsize)); // call specific page malloc for better codegen
        mi_assert_internal(p != NULL);
        mi_assert_internal(((uintptr_t)p + offset) % alignment == 0);
        mi_track_malloc(p,size,zero);
        return p;
      }
    }
  }

  // fallback to generic aligned allocation
  return mi_heap_malloc_zero_aligned_at_generic(heap, size, alignment, offset, zero, ppage);
}


// ------------------------------------------------------
// Optimized mi_heap_malloc_aligned / mi_malloc_aligned
// ------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, false, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_malloc_aligned_at(heap, size, alignment, 0);
}

// ensure a definition is emitted
#if defined(__cplusplus)
void* _mi_extern_heap_malloc_aligned = (void*)&mi_heap_malloc_aligned;
#endif

// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, true, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(heap, size, alignment, 0);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned_at(mi_heap_t* heap, size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_zalloc_aligned_at(heap, total, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned(mi_heap_t* heap, size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(heap,count,size,alignment,0);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_aligned_at(mi_prim_get_default_heap(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_malloc_aligned(mi_prim_get_default_heap(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_umalloc_aligned(size_t size, size_t alignment, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_heap_malloc_zero_aligned_at(mi_prim_get_default_heap(), size, alignment, 0, false, &page);
  if (p!=NULL && pblock_size!=NULL) { *pblock_size = mi_page_block_size(page); }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(mi_prim_get_default_heap(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned(mi_prim_get_default_heap(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc_aligned(size_t size, size_t alignment, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_heap_malloc_zero_aligned_at(mi_prim_get_default_heap(), size, alignment, 0, true, &page);
  if (p!=NULL && pblock_size!=NULL) { *pblock_size = mi_page_block_size(page); }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned_at(size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(mi_prim_get_default_heap(), count, size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned(size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned(mi_prim_get_default_heap(), count, size, alignment);
}


// ------------------------------------------------------
// Aligned re-allocation
// ------------------------------------------------------

static void* mi_heap_realloc_zero_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset, bool zero) mi_attr_noexcept {
  mi_assert(mi_alignment_is_valid(alignment));
  if mi_unlikely(!mi_alignment_is_valid(alignment)) { // require power-of-two (see <https://en.cppreference.com/w/c/memory/aligned_alloc>)
    return mi_error_bad_alignment(newsize,alignment,offset);
  }
  if (alignment <= sizeof(uintptr_t) && offset==0) return _mi_heap_realloc_zero(heap,p,newsize,zero,NULL,NULL);
  if (p == NULL) return mi_heap_malloc_zero_aligned_at(heap,newsize,alignment,offset,zero,NULL);
  const size_t size = mi_usable_size(p);
  if (newsize <= size && newsize >= (size - (size / 2)) && (((uintptr_t)p + offset) & (alignment-1)) == 0) {
    return p;  // reallocation still fits, is aligned and not more than 50% waste
  }
  else {
    // note: we don't zero allocate upfront so we only zero initialize the expanded part (at the cost of calling mi_usable_size)
    mi_page_t* newpage;
    void* const newp = mi_heap_malloc_zero_aligned_at(heap,newsize,alignment,offset,false /* no zero*/, &newpage);
    if (newp != NULL) {
      const size_t copy_size  = (newsize > size ? size : newsize);
      const size_t zero_start = (copy_size >= sizeof(intptr_t) ? copy_size - sizeof(intptr_t) : 0); // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
      const size_t usable     = _mi_page_usable_size(newpage,newp);
      mi_assert_internal(usable >= newsize); // use usable for zero'ing, issue #763
      if (zero && usable > zero_start) {
        // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
        _mi_memzero((uint8_t*)newp + zero_start, usable - zero_start);
      }
      _mi_memcpy(newp, p, copy_size); // cannot be aligned due to abitrary offset... (todo: require offset to be a multiple of sizeof(void*)?)
      mi_free(p); // only free if successful
    }
    return newp;
  }
}

static void* mi_heap_realloc_zero_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_heap_realloc_zero(heap,p,newsize,zero,NULL,NULL);
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,0,zero);
}

mi_decl_nodiscard void* mi_heap_realloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,offset,false);
}

mi_decl_nodiscard void* mi_heap_realloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap,p,newsize,alignment,false);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap, p, newsize, alignment, offset, true);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap, p, newsize, alignment, true);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned_at(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned_at(heap, p, total, alignment, offset);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned(heap, p, total, alignment);
}

mi_decl_nodiscard void* mi_realloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_aligned_at(mi_prim_get_default_heap(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_realloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_aligned(mi_prim_get_default_heap(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_rezalloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned_at(mi_prim_get_default_heap(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_rezalloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned(mi_prim_get_default_heap(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_recalloc_aligned_at(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_recalloc_aligned_at(mi_prim_get_default_heap(), p, newcount, size, alignment, offset);
}

mi_decl_nodiscard void* mi_recalloc_aligned(void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_recalloc_aligned(mi_prim_get_default_heap(), p, newcount, size, alignment);
}


