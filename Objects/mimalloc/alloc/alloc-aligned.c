/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/prim/prim-tls.h"  // _mi_theap_default
#include "mimalloc/alloc/alloc.h"
#include "mimalloc/alloc/free.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/profile/sample-guarded.h"


// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

static bool mi_malloc_is_naturally_aligned( size_t size, size_t alignment ) {
  // certain blocks are always allocated at a certain natural alignment.
  // (see `alloc/page.h:mi_natural_alignment_of_size`, asserted in `arena/arena-page.c:mi_arenas_page_alloc_fresh`).
  mi_assert_internal(mi_alignment_is_valid(alignment));
  if (alignment > size) return false;
  const size_t bsize = mi_good_size(size);
  const bool ok = (alignment <= mi_natural_alignment_of_size(bsize));
  if (ok) { mi_assert_internal((bsize & (alignment-1)) == 0); } // since the natural alignment always divides `bsize`
  return ok;
}

#if 0
static mi_decl_noinline mi_decl_restrict void* mi_theap_malloc_guarded_aligned(mi_theap_t* theap, size_t size, size_t alignment, bool zero, mi_page_t** ppage) mi_attr_noexcept {
  // use over allocation for guarded blocksl
  #if MI_THEAP_INITASNULL
  if mi_unlikely(theap==NULL) { theap = _mi_theap_empty_get(); }
  #endif
  mi_assert_internal(alignment > 0 && alignment < MI_PAGE_MAX_OVERALLOC_ALIGN);
  if mi_unlikely(alignment >= MI_PAGE_MAX_OVERALLOC_ALIGN || size > (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE - alignment)) {
    _mi_error_message(EOVERFLOW, "(guarded) aligned allocation request is too large (size %zu, alignment %zu)\n", size, alignment);
    return NULL;
  }
  const size_t oversize = size + alignment - 1;
  void* const base = _mi_theap_malloc_guarded(theap, oversize, zero, ppage);
  if (base==NULL) return NULL;
  void* const p = mi_align_up_ptr(base, alignment);
  mi_track_align(base, p, (uint8_t*)p - (uint8_t*)base, size);
  mi_assert_internal(mi_usable_size(p) >= size);
  mi_assert_internal(mi_is_aligned(p, alignment));
  return p;
}
#endif


// Fallback aligned allocation that over-allocates -- split out for better codegen
static mi_decl_noinline void* mi_theap_malloc_zero_aligned_at_overalloc(mi_theap_t* const theap, const size_t size, const size_t alignment, const size_t offset, const bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  mi_assert_internal(size <= (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE));
  mi_assert_internal(mi_alignment_is_valid(alignment));

  void* p;
  size_t oversize;
  mi_page_t* page;
  if mi_unlikely(alignment > MI_PAGE_MAX_OVERALLOC_ALIGN) {
    // use OS allocation for large alignments and allocate inside a singleton page (not in an arena)
    // This can support alignments >= MI_PAGE_ALIGN by ensuring the object can be aligned
    // in the first (and single) page such that the page info is `MI_PAGE_ALIGN` bytes before it (and can be found in the _mi_page_map).
    if mi_unlikely(offset != 0) {
      // todo: cannot support offset alignment for very large alignments yet
      _mi_error_message(EOVERFLOW, "aligned allocation with a large alignment cannot be used with an alignment offset (size %zu, alignment %zu, offset %zu)\n", size, alignment, offset);
      return NULL;
    }
    oversize = (size <= MI_SMALL_SIZE_MAX ? MI_SMALL_SIZE_MAX + 1 /* ensure we use generic malloc path */ : size);
    // note: no guarded as alignment > 0
    p = _mi_theap_malloc_zero(theap, oversize, zero, alignment, &page); // the page block size should be large enough to align in the single huge page block
    if (p == NULL) return NULL;
  }
  else {
    // otherwise over-allocate
    mi_assert_internal(size <= (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE) && alignment <= MI_PAGE_MAX_OVERALLOC_ALIGN);
    mi_assert_internal(size < SIZE_MAX - alignment); // `oversize` cannot overflow
    oversize = (size < MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : size) + alignment - 1;  // adjust for size <= 16; with size 0 and alignment 64k, we would allocate a 64k block and pointing just beyond that.
    p = _mi_theap_malloc_zero(theap, oversize, zero, 0, &page);
    if (p == NULL) return NULL;
  }
  mi_assert_internal(page == mi_ptr_page(p));
  if (ppage!=NULL) { *ppage = page; }

  // .. and align within the allocation
  const uintptr_t align_mask = alignment - 1;  // for any x, `(x & align_mask) == (x % alignment)`
  const uintptr_t poffset = ((uintptr_t)p + offset) & align_mask;
  const uintptr_t adjust  = (poffset == 0 ? 0 : alignment - poffset);
  mi_assert_internal(adjust < alignment);
  void* aligned_p = (void*)((uintptr_t)p + adjust);

  // note: after the above allocation, the page may be abandoned now (as it became full, see `alloc/alloc-generic.c:_mi_malloc_generic`)
  // and we no longer own it. We should be careful to only read constant fields in the page,
  // or use safe atomic access as in `mi_page_set_has_interior_pointers`.
  // (we can access the page though since the just allocated pointer keeps it alive)
  if (aligned_p != p) {
    mi_page_set_has_interior_pointers(page, true);

    #if MI_GUARDED || MI_PROFILE
    // set tag to aligned so mi_usable_size works with guard pages
    // note: we could have gotten a profiled or guarded block but that is still ok
    // and in such case the MI_BLOCK_TAG_ALIGNED is just not at the start of the real heap block
    if (adjust >= sizeof(mi_block_t)) {
      mi_block_t* const block = (mi_block_t*)p;
      block->next = MI_BLOCK_TAG_ALIGNED;
    }
    #endif
    mi_block_t* const base = _mi_page_ptr_unalign(page, p);
    _mi_padding_shrink(page, base, adjust + size);
  }
  // todo: expand padding if overallocated ?

  mi_assert_internal(mi_page_usable_block_size(page) >= adjust + size);
  mi_assert_internal(((uintptr_t)aligned_p + offset) % alignment == 0);
  mi_assert_internal(mi_usable_size(aligned_p)>=size);
  mi_assert_internal(mi_usable_size(p) == mi_usable_size(aligned_p)+adjust);
  #if MI_DEBUG > 1
  mi_page_t* const apage = mi_ptr_page(aligned_p);
  mi_block_t* unalign_p = _mi_page_ptr_unalign(apage, aligned_p);
  mi_assert_internal(p == (void*)unalign_p || mi_block_ptr_is_sampled(unalign_p, aligned_p));
  #endif

  if (p != aligned_p) {
    mi_track_align(p,aligned_p,adjust,mi_usable_size(aligned_p));
    #if MI_GUARDED || MI_PROFILE
    mi_track_mem_defined(p, sizeof(mi_block_t));
    #endif
  }
  return aligned_p;
}

// Generic primitive aligned allocation -- split out for better codegen
static mi_decl_noinline void* mi_theap_malloc_zero_aligned_at_generic(mi_theap_t* const theap, const size_t size, const size_t alignment, const size_t offset, const bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  mi_assert_internal(mi_alignment_is_valid(alignment));
  // we don't allocate more than MI_MAX_ALLOC_SIZE (see <https://sourceware.org/ml/libc-announce/2019/msg00001.html>)
  if mi_unlikely(size > (MI_MAX_ALLOC_SIZE - MI_PADDING_SIZE)) {
    _mi_error_message(EINVAL, "aligned allocation request is too large (size %zu, alignment %zu)\n", size, alignment);
    return NULL;
  }

  // use regular allocation if it is guaranteed to fit the alignment constraints.
  // this is important to try as the fast path in `mi_theap_malloc_zero_aligned` only works when there exist
  // a page with the right block size, and if we always use the over-alloc fallback that would never happen.
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL)
  #endif
  {
    #if MI_SAMPLE  // only try if we would not take a sample
    if mi_likely(!mi_theap_should_sample(theap,size))
    #endif
    {
      if (offset == 0 && mi_malloc_is_naturally_aligned(size,alignment))
      {
        mi_page_t* page = NULL;
        void* p = _mi_theap_malloc_zero(theap, size, zero, 0,&page);
        if (ppage!=NULL) { *ppage = page; }
        const bool is_aligned_or_null = (((uintptr_t)p) & (alignment-1))==0;
        if mi_likely(is_aligned_or_null) {
          return p;
        }
        else {
          // this should never happen if the `mi_malloc_is_naturally_aligned` check is correct..
          // (but it can happen if this allocation just so happens to be sampled but we checked for that)
          mi_assert_internal(false); // p==NULL || mi_block_ptr_is_profiled_or_guarded(_mi_page_ptr_unalign(page,p),p));
          mi_free(p);
        }
      }
    }
  }

  // fall back to over-allocation
  return mi_theap_malloc_zero_aligned_at_overalloc(theap,size,alignment,offset,zero,ppage);
}


static mi_decl_cold mi_decl_noinline void* mi_error_bad_alignment(size_t size, size_t alignment, size_t offset) {
  _mi_error_message(EINVAL, "aligned allocation requires the alignment to be a power-of-two (size %zu, alignment %zu, offset %zu)\n", size, alignment, offset);
  return NULL;
}

// Primitive aligned allocation
static inline void* mi_theap_malloc_zero_aligned_at(mi_theap_t* const theap, size_t size, const size_t alignment, const size_t offset, const bool zero, mi_page_t** ppage) mi_attr_noexcept
{
  // note: we don't require `size > offset`, we just guarantee that the address at offset is aligned regardless of the allocated size.
  if mi_unlikely(!mi_alignment_is_valid(alignment)) { // require power-of-two and multiple of void* (see <https://en.cppreference.com/w/c/memory/aligned_alloc#Notes>)
    return mi_error_bad_alignment(size, alignment, offset);
  }

  // Use a naturally aligned size class instead of over-allocating and adjusting the pointer.
  // Keep the requested size when padding is enabled for byte-precise overflow checks.
  if (MI_PADDING_SIZE == 0 && offset == 0 && size < alignment && alignment <= MI_PAGE_MAX_START_BLOCK_ALIGN2) {
    size = alignment;
  }

  // try first if there happens to be a small block available with just the right alignment
  // since most small power-of-2 blocks (under MI_PAGE_MAX_BLOCK_START_ALIGN2) are already
  // naturally aligned this can be often the case.
  if mi_likely(size <= MI_SMALL_SIZE_MAX && alignment <= size)
  {
    #if MI_THEAP_INITASNULL
    if mi_likely(theap!=NULL)
    #endif
    {
      #if MI_SAMPLE  // check if we shouldn't take a sample
      if mi_likely(!mi_theap_should_sample(theap,size))
      #endif
      {
        const uintptr_t align_mask = alignment-1;       // for any x, `(x & align_mask) == (x % alignment)`
        const size_t padsize = size + MI_PADDING_SIZE;
        mi_page_t* page = mi_theap_get_free_small_page(theap, padsize, false);
        if mi_likely(page->free != NULL) {
          const bool is_aligned = (((uintptr_t)page->free + offset) & align_mask)==0;
          if mi_likely(is_aligned)
          {
            if (ppage!=NULL) { *ppage = page; }
            void* p = _mi_page_malloc_zero(theap, page, padsize, zero); // theap!=NULL
            mi_assert_internal(p != NULL);
            mi_assert_internal(((uintptr_t)p + offset) % alignment == 0);
            mi_track_malloc(p, size, zero);
            return p;
          }
        }
      }
    }
  }

  // fallback to generic aligned allocation
  return mi_theap_malloc_zero_aligned_at_generic(theap, size, alignment, offset, zero, ppage);
}


// ------------------------------------------------------
// Internal mi_theap_malloc_aligned / mi_malloc_aligned
// ------------------------------------------------------

static mi_decl_restrict void* mi_theap_malloc_aligned_at(mi_theap_t* theap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_malloc_zero_aligned_at(theap, size, alignment, offset, false, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_theap_malloc_aligned(mi_theap_t* theap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_malloc_aligned_at(theap, size, alignment, 0);
}

static mi_decl_restrict void* mi_theap_zalloc_aligned_at(mi_theap_t* theap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_malloc_zero_aligned_at(theap, size, alignment, offset, true, NULL);
}

mi_decl_restrict void* mi_theap_zalloc_aligned(mi_theap_t* theap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_zalloc_aligned_at(theap, size, alignment, 0);
}

static mi_decl_restrict void* mi_theap_calloc_aligned_at(mi_theap_t* theap, size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_theap_zalloc_aligned_at(theap, total, alignment, offset);
}

static mi_decl_restrict void* mi_theap_calloc_aligned(mi_theap_t* theap, size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_calloc_aligned_at(theap, count, size, alignment, 0);
}


// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_malloc_aligned_at(_mi_theap_default(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_malloc_aligned(_mi_theap_default(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_umalloc_aligned(size_t size, size_t alignment, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_theap_malloc_zero_aligned_at(_mi_theap_default(), size, alignment, 0, false, &page);
  if (p!=NULL && pblock_size!=NULL) { *pblock_size = mi_page_block_size(page); }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_zalloc_aligned_at(_mi_theap_default(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_zalloc_aligned(_mi_theap_default(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc_aligned(size_t size, size_t alignment, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_theap_malloc_zero_aligned_at(_mi_theap_default(), size, alignment, 0, true, &page);
  if (p!=NULL && pblock_size!=NULL) { *pblock_size = mi_page_block_size(page); }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned_at(size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_calloc_aligned_at(_mi_theap_default(), count, size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned(size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_calloc_aligned(_mi_theap_default(), count, size, alignment);
}


mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_malloc_aligned_at(_mi_heap_theap(heap), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_malloc_aligned(_mi_heap_theap(heap), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_zalloc_aligned_at(_mi_heap_theap(heap), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_zalloc_aligned(_mi_heap_theap(heap), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned_at(mi_heap_t* heap, size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_calloc_aligned_at(_mi_heap_theap(heap), count, size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned(mi_heap_t* heap, size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_calloc_aligned(_mi_heap_theap(heap), count, size, alignment);
}


// ------------------------------------------------------
// Aligned re-allocation
// ------------------------------------------------------

static void* mi_theap_realloc_zero_aligned_at(mi_theap_t* theap, void* p, size_t newsize, size_t alignment, size_t offset, bool zero) mi_attr_noexcept {
  mi_assert(mi_alignment_is_valid(alignment));
  if mi_unlikely(!mi_alignment_is_valid(alignment)) { // require power-of-two (see <https://en.cppreference.com/w/c/memory/aligned_alloc>)
    return mi_error_bad_alignment(newsize,alignment,offset);
  }
  if (alignment <= sizeof(uintptr_t) && offset==0) return _mi_theap_realloc_zero(theap,p,newsize,zero);
  if (p == NULL) return mi_theap_malloc_zero_aligned_at(theap,newsize,alignment,offset,zero,NULL);
  const size_t size = mi_usable_size(p);
  if (newsize <= size && newsize >= (size - (size / 2)) && (((uintptr_t)p + offset) & (alignment-1)) == 0) {
    return p;  // reallocation still fits, is aligned and not more than 50% waste
  }
  else {
    // note: we don't zero allocate upfront so we only zero initialize the expanded part (at the cost of calling mi_usable_size)
    mi_page_t* newpage;
    void* const newp = mi_theap_malloc_zero_aligned_at(theap,newsize,alignment,offset,false/*zero?*/,&newpage);
    if (newp != NULL) {
      const size_t copy_size  = (newsize > size ? size : newsize);
      const size_t zero_start = (copy_size >= sizeof(intptr_t) ? copy_size - sizeof(intptr_t) : 0); // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
      const size_t usable     = _mi_page_usable_size(newpage,newp);
      mi_assert_internal(usable >= newsize); // use usable for zero'ing, issue #763
      if (zero && usable > zero_start) {
        // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
        mi_memzero((uint8_t*)newp + zero_start, usable - zero_start);
      }
      mi_memcpy(newp, p, copy_size); // cannot be aligned due to abitrary offset... (todo: require offset to be a multiple of sizeof(void*)?)
      mi_free(p); // only free if successful
    }
    return newp;
  }
}

static void* mi_theap_realloc_zero_aligned(mi_theap_t* theap, void* p, size_t newsize, size_t alignment, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_theap_realloc_zero(theap,p,newsize,zero);
  return mi_theap_realloc_zero_aligned_at(theap,p,newsize,alignment,0,zero);
}

static void* mi_theap_realloc_aligned_at(mi_theap_t* theap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_realloc_zero_aligned_at(theap,p,newsize,alignment,offset,false);
}

static void* mi_theap_realloc_aligned(mi_theap_t* theap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_realloc_zero_aligned(theap,p,newsize,alignment,false);
}

static void* mi_theap_rezalloc_aligned_at(mi_theap_t* theap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_realloc_zero_aligned_at(theap, p, newsize, alignment, offset, true);
}

static void* mi_theap_rezalloc_aligned(mi_theap_t* theap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_realloc_zero_aligned(theap, p, newsize, alignment, true);
}

static void* mi_theap_recalloc_aligned_at(mi_theap_t* theap, void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_theap_rezalloc_aligned_at(theap, p, total, alignment, offset);
}

static void* mi_theap_recalloc_aligned(mi_theap_t* theap, void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_theap_rezalloc_aligned(theap, p, total, alignment);
}


mi_decl_nodiscard void* mi_realloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_realloc_aligned_at(_mi_theap_default(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_realloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_realloc_aligned(_mi_theap_default(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_rezalloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_rezalloc_aligned_at(_mi_theap_default(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_rezalloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_rezalloc_aligned(_mi_theap_default(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_recalloc_aligned_at(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_recalloc_aligned_at(_mi_theap_default(), p, newcount, size, alignment, offset);
}

mi_decl_nodiscard void* mi_recalloc_aligned(void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_recalloc_aligned(_mi_theap_default(), p, newcount, size, alignment);
}


mi_decl_nodiscard void* mi_heap_realloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_realloc_aligned_at(_mi_heap_theap(heap), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_heap_realloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_realloc_aligned(_mi_heap_theap(heap), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_rezalloc_aligned_at(_mi_heap_theap(heap), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_theap_rezalloc_aligned(_mi_heap_theap(heap), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned_at(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_theap_recalloc_aligned_at(_mi_heap_theap(heap), p, newcount, size, alignment, offset);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_theap_recalloc_aligned(_mi_heap_theap(heap), p, newcount, size, alignment);
}
