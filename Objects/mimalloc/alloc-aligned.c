/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc-internal.h"

#include <string.h>  // memset

// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

static void* mi_heap_malloc_zero_aligned_at(mi_heap_t* const heap, const size_t size, const size_t alignment, const size_t offset, const bool zero) mi_attr_noexcept {
  // note: we don't require `size > offset`, we just guarantee that
  // the address at offset is aligned regardless of the allocated size.
  mi_assert(alignment > 0);
  if (mi_unlikely(size > PTRDIFF_MAX)) return NULL;   // we don't allocate more than PTRDIFF_MAX (see <https://sourceware.org/ml/libc-announce/2019/msg00001.html>)
  if (mi_unlikely(alignment==0 || !_mi_is_power_of_two(alignment))) return NULL; // require power-of-two (see <https://en.cppreference.com/w/c/memory/aligned_alloc>)
  const uintptr_t align_mask = alignment-1;  // for any x, `(x & align_mask) == (x % alignment)`
  
  // try if there is a small block available with just the right alignment
  const size_t padsize = size + MI_PADDING_SIZE;
  if (mi_likely(padsize <= MI_SMALL_SIZE_MAX)) {
    mi_page_t* page = _mi_heap_get_free_small_page(heap,padsize);
    const bool is_aligned = (((uintptr_t)page->free+offset) & align_mask)==0;
    if (mi_likely(page->free != NULL && is_aligned))
    {
      #if MI_STAT>1
      mi_heap_stat_increase( heap, malloc, size);
      #endif
      void* p = _mi_page_malloc(heap,page,padsize); // TODO: inline _mi_page_malloc
      mi_assert_internal(p != NULL);
      mi_assert_internal(((uintptr_t)p + offset) % alignment == 0);
      if (zero) _mi_block_zero_init(page,p,size);
      return p;
    }
  }

  // use regular allocation if it is guaranteed to fit the alignment constraints
  if (offset==0 && alignment<=padsize && padsize<=MI_MEDIUM_OBJ_SIZE_MAX && (padsize&align_mask)==0) {
    void* p = _mi_heap_malloc_zero(heap, size, zero);
    mi_assert_internal(p == NULL || ((uintptr_t)p % alignment) == 0);
    return p;
  }
  
  // otherwise over-allocate
  void* p = _mi_heap_malloc_zero(heap, size + alignment - 1, zero);
  if (p == NULL) return NULL;

  // .. and align within the allocation
  uintptr_t adjust = alignment - (((uintptr_t)p + offset) & align_mask);
  mi_assert_internal(adjust <= alignment);
  void* aligned_p = (adjust == alignment ? p : (void*)((uintptr_t)p + adjust));
  if (aligned_p != p) mi_page_set_has_aligned(_mi_ptr_page(p), true); 
  mi_assert_internal(((uintptr_t)aligned_p + offset) % alignment == 0);
  mi_assert_internal( p == _mi_page_ptr_unalign(_mi_ptr_segment(aligned_p),_mi_ptr_page(aligned_p),aligned_p) );
  return aligned_p;
}


mi_decl_restrict void* mi_heap_malloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, false);
}

mi_decl_restrict void* mi_heap_malloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_malloc_aligned_at(heap, size, alignment, 0);
}

mi_decl_restrict void* mi_heap_zalloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, true);
}

mi_decl_restrict void* mi_heap_zalloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(heap, size, alignment, 0);
}

mi_decl_restrict void* mi_heap_calloc_aligned_at(mi_heap_t* heap, size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_zalloc_aligned_at(heap, total, alignment, offset);
}

mi_decl_restrict void* mi_heap_calloc_aligned(mi_heap_t* heap, size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(heap,count,size,alignment,0);
}

mi_decl_restrict void* mi_malloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_aligned_at(mi_get_default_heap(), size, alignment, offset);
}

mi_decl_restrict void* mi_malloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_malloc_aligned(mi_get_default_heap(), size, alignment);
}

mi_decl_restrict void* mi_zalloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(mi_get_default_heap(), size, alignment, offset);
}

mi_decl_restrict void* mi_zalloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned(mi_get_default_heap(), size, alignment);
}

mi_decl_restrict void* mi_calloc_aligned_at(size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(mi_get_default_heap(), count, size, alignment, offset);
}

mi_decl_restrict void* mi_calloc_aligned(size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned(mi_get_default_heap(), count, size, alignment);
}


static void* mi_heap_realloc_zero_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_heap_realloc_zero(heap,p,newsize,zero);
  if (p == NULL) return mi_heap_malloc_zero_aligned_at(heap,newsize,alignment,offset,zero);
  size_t size = mi_usable_size(p);
  if (newsize <= size && newsize >= (size - (size / 2))
      && (((uintptr_t)p + offset) % alignment) == 0) {
    return p;  // reallocation still fits, is aligned and not more than 50% waste
  }
  else {
    void* newp = mi_heap_malloc_aligned_at(heap,newsize,alignment,offset);
    if (newp != NULL) {
      if (zero && newsize > size) {
        const mi_page_t* page = _mi_ptr_page(newp);
        if (page->is_zero) {
          // already zero initialized
          mi_assert_expensive(mi_mem_is_zero(newp,newsize));
        }
        else {
          // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
          size_t start = (size >= sizeof(intptr_t) ? size - sizeof(intptr_t) : 0);
          memset((uint8_t*)newp + start, 0, newsize - start);
        }
      }
      _mi_memcpy_aligned(newp, p, (newsize > size ? size : newsize));
      mi_free(p); // only free if successful
    }
    return newp;
  }
}

static void* mi_heap_realloc_zero_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_heap_realloc_zero(heap,p,newsize,zero);
  size_t offset = ((uintptr_t)p % alignment); // use offset of previous allocation (p can be NULL)
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,offset,zero);
}

void* mi_heap_realloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,offset,false);
}

void* mi_heap_realloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap,p,newsize,alignment,false);
}

void* mi_heap_rezalloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap, p, newsize, alignment, offset, true);
}

void* mi_heap_rezalloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap, p, newsize, alignment, true);
}

void* mi_heap_recalloc_aligned_at(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned_at(heap, p, total, alignment, offset);
}

void* mi_heap_recalloc_aligned(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned(heap, p, total, alignment);
}

void* mi_realloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_aligned_at(mi_get_default_heap(), p, newsize, alignment, offset);
}

void* mi_realloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_aligned(mi_get_default_heap(), p, newsize, alignment);
}

void* mi_rezalloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned_at(mi_get_default_heap(), p, newsize, alignment, offset);
}

void* mi_rezalloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned(mi_get_default_heap(), p, newsize, alignment);
}

void* mi_recalloc_aligned_at(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_recalloc_aligned_at(mi_get_default_heap(), p, newcount, size, alignment, offset);
}

void* mi_recalloc_aligned(void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_recalloc_aligned(mi_get_default_heap(), p, newcount, size, alignment);
}

