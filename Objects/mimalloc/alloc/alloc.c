/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   // for realpath() on Linux
#endif

#include <stdlib.h>                  // malloc, abort, realpath

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/prim/prim-tls.h"   // _mi_prim_thread_id()
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/alloc.h"
#include "mimalloc/alloc/free.h"
#include "mimalloc/profile/sample-profile.h"

#define MI_IN_ALLOC_C
#include "alloc-override.c"
#include "free.c"
#undef MI_IN_ALLOC_C


// ------------------------------------------------------
// Core allocation functions.
// Every allocation goes through `mi_page_malloc_zero`
// ------------------------------------------------------

#if MI_PADDING
static mi_decl_noinline void mi_page_block_setup_padding(mi_page_t* page, mi_block_t* block, size_t size) mi_attr_noexcept {
  const size_t bsize = mi_page_usable_block_size(page);
  mi_padding_t* const padding = (mi_padding_t*)((uint8_t*)block + bsize);
  const ptrdiff_t delta = ((uint8_t*)padding - (uint8_t*)block - (size - MI_PADDING_SIZE));
  mi_assert_internal(delta >= 0 && bsize >= (size - MI_PADDING_SIZE + delta));
  mi_track_mem_defined(padding,sizeof(mi_padding_t));  // note: re-enable since mi_page_usable_block_size may set noaccess
  padding->canary = mi_ptr_encode_canary(page,block,page->keys);
  padding->delta  = (uint32_t)(delta);
  #if MI_PADDING_CHECK_BYTES
  if (!mi_page_is_huge(page)) {
    uint8_t* fill = (uint8_t*)padding - delta;
    const size_t maxpad = (delta > MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : delta); // set at most N initial padding bytes
    for (size_t i = 0; i < maxpad; i++) { fill[i] = MI_DEBUG_PADDING; }
  }
  #endif
}
#endif

// C++ new handler support
static void* mi_theap_new_handler(mi_theap_t* theap, size_t size);
static mi_decl_noinline void* mi_malloc_generic_new(mi_theap_t* theap, size_t size, size_t zero_huge_alignment, mi_page_t** ppage) {
  void* p = _mi_malloc_generic(theap, size, zero_huge_alignment, ppage);
  if (p == NULL) { return mi_theap_new_handler(theap, size - MI_PADDING_SIZE); }
  return p;
}
#if defined(_MSC_VER) && !MI_CLANG_CL
// for the msvc compiler to get tail-calls
static mi_decl_noinline void* mi_malloc_generic_msvc(mi_theap_t* theap, size_t size, size_t zero_new_huge_alignment, mi_page_t** ppage) mi_attr_noexcept {
  const bool is_new = ((zero_new_huge_alignment & 2) != 0);
  const size_t zero_huge_alignment = (zero_new_huge_alignment & ~2);
  if (is_new) { return mi_malloc_generic_new(theap, size, zero_huge_alignment, ppage); }
         else { return _mi_malloc_generic(theap, size, zero_huge_alignment, ppage); }
}
#endif

// C++ new calls a new-handler on failure.
#if MI_SAMPLE==2
static mi_decl_noinline mi_decl_restrict void* mi_theap_malloc_sampled_new(mi_theap_t* theap, size_t req_size, bool zero, mi_page_t** ppage);
#endif

// Fast allocation in a page: just pop from the free list.
// Fall back to generic allocation only if the list is empty.
// Note: even though there is a lot of checks etc in the source,
// in release mode the (inlined) routine is about 7 to 10 instructions with a single test.
static mi_decl_forceinline mi_decl_restrict void* mi_page_malloc_zero(mi_theap_t* theap, mi_page_t* page, size_t size, size_t sample_countdown, bool zero, bool is_new, mi_page_t** ppage)
{
  if (page->block_size != 0) { // not the empty theap
    mi_assert_internal(mi_page_block_size(page) >= size);
    mi_assert_internal(mi_is_aligned(mi_page_slice_start(page), MI_PAGE_ALIGN));
    mi_assert_internal(mi_ptr_page(mi_page_start(page))==page);
    mi_assert_internal(mi_page_alloc_count(page) + mi_page_last_used(page) >= mi_page_used(page));
  }

  // check the free list
  mi_block_t* const block = page->free;
  mi_used_t xused = page->xused;
  #if defined(__GNUC__)
  __asm("" : : "r"(xused) : );     // load the `xused` field before the test
  #endif
  xused.used_alloc += 0x10001;
  if mi_unlikely(block == NULL) {
    #if defined(_MSC_VER) && !MI_CLANG_CL
    return mi_malloc_generic_msvc(theap, size, (zero ? 1 : 0) | (is_new ? 2 : 0), ppage);
    #else
    if (is_new) { return mi_malloc_generic_new(theap, size, (zero ? 1 : 0), ppage); }
           else { return _mi_malloc_generic(theap, size, (zero ? 1 : 0), ppage); }
    #endif
  }
  mi_assert_internal(block != NULL && mi_ptr_page(block) == page);
  if (ppage != NULL) { *ppage = page; };

  // pop from the free list
  mi_block_t* next = mi_block_next(page,block);
  mi_track_mem_undefined(block,sizeof(*block));

  #if MI_SECURE
  if (!zero) block->next = 0;  // don't leak internal data
  #endif

  // update the page free list and used count
  page->free  = next;
  page->xused = xused;

  mi_assert_internal(page->free == NULL || mi_ptr_page(page->free) == page);
  mi_assert_internal(page->block_size < MI_MAX_ALIGN_SIZE || mi_is_aligned(block, MI_MAX_ALIGN_SIZE));

  #if MI_SAMPLE==2
  const size_t req_size = size - MI_PADDING_SIZE;
  mi_assert_internal(theap->sample_countdown == sample_countdown); // we pass it to improve codegen
  mi_assert_internal(theap->sample_countdown >= req_size);
  theap->sample_countdown = sample_countdown - req_size;
  #else
  MI_UNUSED(sample_countdown);
  #endif

  #if MI_STATS>=2
  mi_theap_stat_counter_increase(theap,malloc_requested,size - MI_PADDING_SIZE);
  #endif

  #if MI_DEBUG>3
  if (page->free_is_zero && size > sizeof(*block)) {
    mi_assert_expensive(mi_mem_is_zero(block+1,size - sizeof(*block)));
  }
  #endif

  // allow use of the block internally
  // note: when tracking we need to avoid ever touching the MI_PADDING since
  // that is tracked by valgrind etc. as non-accessible (through the red-zone, see `mimalloc/track.h`)
  const size_t bsize = mi_page_usable_block_size(page);
  mi_track_mem_undefined(block, bsize);

  // track per-block statistics
  mi_assert_internal(mi_page_alloc_count(page) + mi_page_last_used(page) >= mi_page_used(page));

  // in debug mode initialize with 0xD0
  #if (MI_DEBUG>0) && !MI_TRACK_ENABLED && !MI_TSAN
  if mi_likely(!zero && !mi_page_is_huge(page)) { mi_debug_fill(page, block, MI_DEBUG_UNINIT, bsize); }
  #endif

  // zero the block? note: we need to zero the full block size (issue #63)
  if mi_unlikely(zero) {
    if (!page->free_is_zero) {
      mi_assert_internal(bsize%MI_SIZE_SIZE == 0);
      #if !MI_PADDING
      return // use tail-call
      #endif
      mi_memzero_block(block,bsize);
    }
    else {
      block->next = 0;
      mi_track_mem_defined(block, bsize);
    }
  }

  // setup padding (must come after zero'ing)
  #if MI_PADDING // && !MI_TRACK_ENABLED
  mi_page_block_setup_padding(page,block,size);
  #endif

  return block;
}

// extra entries for improved efficiency in `alloc/alloc-aligned.c` (and in `alloc/alloc-generic.c:mi_malloc_generic`.
extern mi_decl_restrict void* _mi_page_malloc_zero(mi_theap_t* theap, mi_page_t* page, size_t size, bool zero) mi_attr_noexcept {
  return mi_page_malloc_zero(theap, page, size, theap->sample_countdown, zero, false, NULL);
}


// Internal small size allocation; assumes the `theap` is non-NULL.
// This looks up the page directly from a the direct page entries in the theap.
// The size can be in words (as a wsize); we assume this will be inlined for best code for `mi_wmalloc_small` and similar functions.
static mi_decl_forceinline mi_decl_restrict void* mi_theap_nonnull_xmalloc_small_zero(mi_theap_t* theap, size_t xsize, bool is_wsize, bool zero, bool is_new, mi_page_t** ppage)
{
  size_t size = (is_wsize ? xsize * MI_SIZE_SIZE : xsize);
  mi_assert(theap != NULL);
  mi_assert(size <= MI_SMALL_SIZE_MAX);
  #if MI_DEBUG
  mi_assert(mi_theap_matches_thread(theap)); // theaps are thread local
  #endif
  #if (MI_PADDING) // || MI_GUARDED)
  if mi_unlikely(xsize == 0) { xsize = (is_wsize ? 1 : MI_SIZE_SIZE); size += MI_SIZE_SIZE; }
  #endif

  // we only sample if fine-grained sampling is enabled (otherwise we sample in mi_malloc_generic)
  #if MI_SAMPLE==2
  if mi_unlikely(mi_theap_should_sample(theap,size)) {
    if (is_new) { return mi_theap_malloc_sampled_new(theap,size,zero,ppage); }
           else { return _mi_theap_malloc_sampled(theap,size,zero,ppage); }
  }
  #endif

  // get page in constant time
  mi_page_t* page = mi_theap_get_free_small_page(theap, xsize + (is_wsize ? MI_PADDING_WSIZE : MI_PADDING_SIZE), is_wsize);

  // and allocate
  void* const p = mi_page_malloc_zero(theap, page, size + MI_PADDING_SIZE, theap->sample_countdown, zero, is_new, ppage);
  mi_track_malloc(p,size,zero);

  #if MI_DEBUG>3
  if (p != NULL && zero) {
    mi_assert_expensive(mi_mem_is_zero(p, size));
  }
  #endif
  return p;
}

// Internal generic allocation.
// Goes to `alloc/alloc-generic.c:_mi_malloc_generic` which handles large and aligned allocations.
// Every small allocation eventually goes through this generic allocation path if it cannot be satisfied from the small page cache.
static mi_decl_forceinline void* mi_theap_malloc_generic(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage) mi_attr_noexcept
{
  #if !MI_THEAP_INITASNULL
  mi_assert(theap!=NULL);
  #endif
  mi_assert(mi_theap_matches_thread(theap));   // theaps are thread local
  mi_assert((huge_alignment & 1)==0);
  void* const p = _mi_malloc_generic(theap, size + MI_PADDING_SIZE, (zero ? 1 : 0) | huge_alignment, ppage);  // note: size can overflow but it is detected in malloc_generic
  mi_track_malloc(p, size, zero);

  #if MI_DEBUG>3
  if (p != NULL && zero) {
    mi_assert_expensive(mi_mem_is_zero(p, size));
  }
  #endif
  return p;
}

// ------------------------------------------------------------------------
// Small allocations: mi_malloc_small and variants (zalloc, wmalloc, wzalloc)
// ------------------------------------------------------------------------

// internal small allocation where the theap can be NULL on some platforms (Windows and macOS)
static mi_decl_forceinline mi_decl_restrict void* mi_theap_xmalloc_small_zero(mi_theap_t* theap, size_t xsize, bool is_wsize, bool zero, mi_page_t** ppage) mi_attr_noexcept {
  #if !MI_THEAP_INITASNULL
  return mi_theap_nonnull_xmalloc_small_zero(theap, xsize, is_wsize, zero, false, ppage);
  #else
  if (theap!=NULL) {
    return mi_theap_nonnull_xmalloc_small_zero(theap, xsize, is_wsize, zero, false, ppage);
  }
  else {
    return mi_theap_malloc_generic(theap, (is_wsize ? xsize * MI_SIZE_SIZE : xsize), zero, 0, ppage);    // tailcall
  }
  #endif
}

// Allocate a small block from a theap; these routines assume a non-NULL theap.
mi_decl_nodiscard extern inline mi_decl_restrict void* mi_theap_malloc_small(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_xmalloc_small_zero(theap, size, false, false, false, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_theap_wmalloc_small(mi_theap_t* theap, size_t wsize) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_xmalloc_small_zero(theap, wsize, true, false, false, NULL);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_theap_zalloc_small(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_xmalloc_small_zero(theap, size, false, true, false, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_theap_wzalloc_small(mi_theap_t* theap, size_t wsize) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_xmalloc_small_zero(theap, wsize, true, true, false, NULL);
}

// Regular small allocation functions that use the default theap.
mi_decl_nodiscard mi_decl_restrict void* mi_malloc_small(size_t size) mi_attr_noexcept {
  return mi_theap_xmalloc_small_zero(_mi_theap_default(), size, false, false, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_small(size_t size) mi_attr_noexcept {
  return mi_theap_xmalloc_small_zero(_mi_theap_default(), size, false, true, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_wmalloc_small(size_t wsize) mi_attr_noexcept {
  return mi_theap_xmalloc_small_zero(_mi_theap_default(), wsize, true, false, NULL );
}

mi_decl_nodiscard mi_decl_restrict void* mi_wzalloc_small(size_t wsize) mi_attr_noexcept {
  return mi_theap_xmalloc_small_zero(_mi_theap_default(), wsize, true, true, NULL);
}


mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_malloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_theap_malloc_small(_mi_heap_theap_get_or_init(heap), size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  // we could also use: mi_theap_malloc_small_zero_nonnull(_mi_heap_theap(theap), size, false, NULL); }
  // but the following prevents using a stack frame. We use this to optimize some select functions only.
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_malloc_small(theap, size); }
                       else { return mi_heap_init_malloc_small(heap, size); }
}

mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_zalloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_theap_zalloc_small(_mi_heap_theap_get_or_init(heap), size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  // optimize: return mi_theap_malloc_small_zero_nonnull(_mi_heap_theap(heap), size, true, NULL);
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_zalloc_small(theap, size); }
                       else { return mi_heap_init_zalloc_small(heap, size); }
}


// ------------------------------------------------------------------------------
// Main allocation functions: mi_malloc and variants (zalloc, wmalloc, wzalloc)
// ------------------------------------------------------------------------------

mi_decl_nodiscard static mi_decl_forceinline mi_decl_restrict void* mi_theap_nonnull_malloc_zero(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  // fast path for small objects
  if mi_likely(size <= MI_SMALL_SIZE_MAX) {
    mi_assert_internal(huge_alignment == 0);
    return mi_theap_nonnull_xmalloc_small_zero(theap, size, false, zero, false, ppage);
  }
  else {
    return mi_theap_malloc_generic(theap, size, zero, huge_alignment, ppage);
  }
}

mi_decl_nodiscard extern mi_decl_forceinline mi_decl_restrict void* _mi_theap_malloc_zero(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage) mi_attr_noexcept {
  // fast path for small objects
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL && size <= MI_SMALL_SIZE_MAX)
  #else
  if mi_likely(size <= MI_SMALL_SIZE_MAX)
  #endif
  {
    mi_assert_internal(huge_alignment == 0);
    return mi_theap_nonnull_xmalloc_small_zero(theap, size, false, zero, false, ppage);
  }
  else {
    return mi_theap_malloc_generic(theap, size, zero, huge_alignment, ppage);
  }
}

mi_decl_nodiscard static inline mi_decl_restrict void* mi_theap_nonnull_malloc(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_malloc_zero(theap, size, false, 0, NULL);
}

mi_decl_nodiscard static inline mi_decl_restrict void* mi_theap_nonnull_zalloc(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  mi_assert(theap!=NULL);
  return mi_theap_nonnull_malloc_zero(theap, size, true, 0, NULL);
}

// Main allocation functions
// (note: to reduce variants, we only use nonnull theap variants for small allocations)

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_theap_malloc(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  return _mi_theap_malloc_zero(theap, size, false, 0, NULL);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_theap_zalloc(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  return _mi_theap_malloc_zero(theap, size, true, 0, NULL);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc(size_t size) mi_attr_noexcept {
  return mi_theap_malloc(_mi_theap_default(), size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc(size_t size) mi_attr_noexcept {
  return mi_theap_zalloc(_mi_theap_default(), size);
}

mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_malloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_theap_nonnull_malloc(_mi_heap_theap_get_or_init(heap), size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  // return mi_theap_malloc_zero_nonnull(_mi_heap_theap(heap), size, false, 0, NULL);
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_nonnull_malloc(theap, size); }
                       else { return mi_heap_init_malloc(heap, size); }

}

mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_zalloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_theap_nonnull_zalloc(_mi_heap_theap_get_or_init(heap), size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  // optimize: return mi_theap_malloc_zero_nonnull(_mi_heap_theap(heap), size, true, 0, NULL);
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_nonnull_zalloc(theap, size); }
                       else { return mi_heap_init_zalloc(heap, size); }
}

// ----------------------------------------------------------
// mi_calloc variants
// ----------------------------------------------------------

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_theap_calloc(mi_theap_t* theap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count,size,&total)) return NULL;
  return mi_theap_zalloc(theap,total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc(size_t count, size_t size) mi_attr_noexcept {
  return mi_theap_calloc(_mi_theap_default(), count, size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  return mi_theap_calloc(_mi_heap_theap(heap), count, size);
}

// ----------------------------------------------------------
// Usable block sizes
// ----------------------------------------------------------

// Return usable size
static void* mi_ublock_size( void* p, mi_page_t* page, size_t* pblock_size ) {
  mi_assert_internal(page == mi_ptr_page(p));
  if (pblock_size!=NULL) {
    if (p!=NULL) { *pblock_size = mi_page_block_size(page); }
  }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_umalloc_small(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_theap_xmalloc_small_zero(_mi_theap_default(), size, false, false, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc_small(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = mi_theap_xmalloc_small_zero(_mi_theap_default(), size, false, true, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_umalloc(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = _mi_theap_malloc_zero(_mi_theap_default(), size, false, 0, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_uzalloc(size_t size, size_t* pblock_size) mi_attr_noexcept {
  mi_page_t* page;
  void* p = _mi_theap_malloc_zero(_mi_theap_default(), size, true, 0, &page);
  return mi_ublock_size(p,page,pblock_size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_ucalloc(size_t count, size_t size, size_t* pblock_size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count,size,&total)) return NULL;
  return mi_uzalloc(total, pblock_size);
}

// ----------------------------------------------------------
// Uninitialized `calloc`
// ----------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_mallocn(size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_malloc(total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_mallocn(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_malloc(heap, total);
}

// ----------------------------------------------------------
// Reallocation
// ----------------------------------------------------------

// Expand (or shrink) in place (or fail)
void* mi_expand(void* p, size_t newsize) mi_attr_noexcept {
  #if MI_PADDING
  // we do not shrink/expand with padding enabled
  MI_UNUSED(p); MI_UNUSED(newsize);
  return NULL;
  #else
  if (p == NULL) return NULL;
  const mi_page_t* const page = mi_ptr_page_validate(p,"mi_expand");
  const size_t size = _mi_page_usable_size(page,p);
  if (newsize > size) return NULL;
  return p; // it fits
  #endif
}

static mi_decl_forceinline void* mi_theap_realloc_zero_ex(mi_theap_t* theap, void* p, size_t newsize, bool zero, size_t* pblock_size_pre, size_t* pblock_size_post) mi_attr_noexcept {
  // if p == NULL then behave as malloc.
  // else if size == 0 then reallocate to a zero-sized block (and don't return NULL, just as mi_malloc(0)).
  // (this means that returning NULL always indicates an error, and `p` will not have been freed in that case.)
  const mi_page_t* page;
  size_t size;
  if (p==NULL) {
    page = NULL;
    size = 0;
    if (pblock_size_pre!=NULL) { *pblock_size_pre = 0; }
  }
  else {
    page = mi_ptr_page_validate(p,"mi_realloc");
    if mi_unlikely(page==NULL) {  // invalid pointer
      if (pblock_size_pre!=NULL) { *pblock_size_pre = 0; }
      if (pblock_size_post!=NULL) { *pblock_size_post = 0; }
      return NULL;
    }
    size = _mi_page_usable_size(page,p);
    if (pblock_size_pre!=NULL) { *pblock_size_pre = mi_page_block_size(page); }
  }
  // check if we can reuse the existing block
  if mi_unlikely(newsize<=size && newsize>=(size/2) && newsize>0) { // note: newsize must be > 0 or otherwise we return NULL for realloc(NULL,0)
    mi_assert_internal(page!=NULL); // note: page!=NULL (since if p==NULL, we have size=0 and size>=newsize>0
    #if MI_THEAP_INITASNULL
    if (theap!=NULL)
    #endif
    {
      if (mi_page_heap(page)==mi_theap_heap_peek(theap)) {  // and within the same heap
        mi_assert_internal(p!=NULL);
        // todo: do not track as the usable size is still the same in the free; adjust potential padding?
        // mi_track_resize(p,size,newsize)
        // if (newsize < size) { mi_track_mem_noaccess((uint8_t*)p + newsize, size - newsize); }
        if (pblock_size_post!=NULL) { *pblock_size_post = mi_page_block_size(page); }
        return p;  // reallocation still fits and not more than 50% waste
      }
    }
  }
  // note: we don't zero allocate upfront so we only zero initialize the expanded part
  mi_page_t* newpage; // use block_size for zero-ing, issue #763
  void* const newp = _mi_theap_malloc_zero(theap,newsize,false /* no zero */,0,&newpage);
  if mi_likely(newp != NULL) {
    if (pblock_size_post!=NULL) { *pblock_size_post = mi_page_block_size(newpage); }
    const size_t copy_size  = (newsize > size ? size : newsize);
    const size_t zero_start = mi_align_down( (copy_size >= sizeof(intptr_t) ? copy_size - sizeof(intptr_t) : 0), sizeof(intptr_t)); // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
    const size_t usable = _mi_page_usable_size(newpage,newp);
    mi_assert_internal(usable >= newsize);
    if (zero && usable > zero_start) {
      mi_memzero_aligned((uint8_t*)newp + zero_start, usable - zero_start);
    }
    else if (newsize == 0) {
      ((uint8_t*)newp)[0] = 0; // work around for applications that expect zero-reallocation to be zero initialized (issue #725)
    }
    if mi_likely(p != NULL) {
      mi_track_mem_defined(p,copy_size);  // _mi_useable_size may be too large for byte precise memory tracking..
      mi_memcpy_aligned(newp, p, copy_size);
      mi_free(p); // only free the original pointer if successful
    }
  }
  return newp;
}

void* _mi_theap_realloc_zero(mi_theap_t* theap, void* p, size_t newsize, bool zero) mi_attr_noexcept {
  return mi_theap_realloc_zero_ex(theap,p,newsize,zero,NULL,NULL);
}

// for theap_realloc/rezalloc we allow theap==NULL to reduce variants
mi_decl_nodiscard void* mi_theap_realloc(mi_theap_t* theap, void* p, size_t newsize) mi_attr_noexcept {
  if (p==NULL) {
    return mi_theap_malloc(theap,newsize);
  }
  else {
    return _mi_theap_realloc_zero(theap, p, newsize, false);
  }
}

mi_decl_nodiscard void* mi_theap_rezalloc(mi_theap_t* theap, void* p, size_t newsize) mi_attr_noexcept {
  if (p==NULL) {
    return mi_theap_zalloc(theap,newsize);
  }
  else {
    return _mi_theap_realloc_zero(theap, p, newsize, true);
  }
}

static void* mi_theap_recalloc(mi_theap_t* theap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_theap_rezalloc(theap, p, total);
}

mi_decl_nodiscard void* mi_realloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_realloc(_mi_theap_default(),p,newsize);
}

mi_decl_nodiscard void* mi_reallocn(void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_theap_realloc(_mi_theap_default(),p,total);
}

mi_decl_nodiscard void* mi_urealloc(void* p, size_t newsize, size_t* pblock_size_pre, size_t* pblock_size_post) mi_attr_noexcept {
  return mi_theap_realloc_zero_ex(_mi_theap_default(),p,newsize, false, pblock_size_pre, pblock_size_post);
}

// Reallocate but free `p` on errors
static void* mi_theap_reallocf(mi_theap_t* xtheap, void* p, size_t newsize) mi_attr_noexcept {
  void* newp = mi_theap_realloc(xtheap, p, newsize);
  if (newp==NULL && p!=NULL) mi_free(p);
  return newp;
}

mi_decl_nodiscard void* mi_reallocf(void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_reallocf(_mi_theap_default(),p,newsize);
}

mi_decl_nodiscard void* mi_rezalloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_rezalloc(_mi_theap_default(), p, newsize);
}

mi_decl_nodiscard void* mi_recalloc(void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_theap_recalloc(_mi_theap_default(), p, count, size);
}

mi_decl_nodiscard void* mi_heap_realloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_realloc(_mi_heap_theap(heap), p, newsize);
}

mi_decl_nodiscard void* mi_heap_reallocn(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_theap_realloc(_mi_heap_theap(heap), p, total);
}

// Reallocate but free `p` on errors
mi_decl_nodiscard void* mi_heap_reallocf(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_reallocf(_mi_heap_theap(heap), p, newsize);
}

mi_decl_nodiscard void* mi_heap_rezalloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return mi_theap_rezalloc(_mi_heap_theap(heap), p, newsize);
}

mi_decl_nodiscard void* mi_heap_recalloc(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_theap_recalloc(_mi_heap_theap(heap), p, count, size);
}


// ------------------------------------------------------
// strdup, strndup, and realpath
// ------------------------------------------------------

// `strdup` using mi_malloc
mi_decl_nodiscard static mi_decl_restrict char* mi_theap_strdup(mi_theap_t* xtheap, const char* s) mi_attr_noexcept {
  if (s == NULL) return NULL;
  size_t len = _mi_strlen(s);
  if (len > MI_MAX_ALLOC_SIZE - 1) return NULL;  // prevent overflow on len+1
  char* t = (char*)mi_theap_malloc(xtheap,len+1);
  if (t == NULL) return NULL;
  mi_memcpy(t, s, len);
  t[len] = 0;
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strdup(const char* s) mi_attr_noexcept {
  return mi_theap_strdup(_mi_theap_default(), s);
}

mi_decl_nodiscard mi_decl_restrict char* mi_heap_strdup(mi_heap_t* heap, const char* s) mi_attr_noexcept {
  return mi_theap_strdup(_mi_heap_theap(heap), s);
}

// `strndup` using mi_malloc
mi_decl_nodiscard static mi_decl_restrict char* mi_theap_strndup(mi_theap_t* xtheap, const char* s, size_t n) mi_attr_noexcept {
  if (s == NULL) return NULL;
  const size_t len = _mi_strnlen(s,n);  // len <= n
  if (len > MI_MAX_ALLOC_SIZE - 1) return NULL;  // prevent overflow on len+1
  char* t = (char*)mi_theap_malloc(xtheap, len+1);
  if (t == NULL) return NULL;
  mi_memcpy(t, s, len);
  t[len] = 0;
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strndup(const char* s, size_t n) mi_attr_noexcept {
  return mi_theap_strndup(_mi_theap_default(),s,n);
}

mi_decl_nodiscard mi_decl_restrict char* mi_heap_strndup(mi_heap_t* heap, const char* s, size_t n) mi_attr_noexcept {
  return mi_theap_strndup(_mi_heap_theap(heap), s, n);
}

#ifndef __wasi__
// `realpath` using mi_malloc
#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

mi_decl_nodiscard static mi_decl_restrict char* mi_theap_realpath(mi_theap_t* xtheap, const char* fname, char* resolved_name) mi_attr_noexcept {
  // todo: use GetFullPathNameW to allow longer file names
  if (fname==NULL || *fname==0) {
    errno = EINVAL;
    return NULL;
  }
  char buf[PATH_MAX];
  DWORD res = GetFullPathNameA(fname, PATH_MAX, (resolved_name == NULL ? buf : resolved_name), NULL);
  if (res == 0) {
    DWORD err = GetLastError();
    switch (err) {
      case ERROR_LOCK_VIOLATION:
      case ERROR_SHARING_VIOLATION:
      case ERROR_INVALID_ACCESS:    errno = EACCES; break;
      case ERROR_INVALID_HANDLE:
      case ERROR_INVALID_FUNCTION:  errno = EINVAL; break;
      case ERROR_PATH_NOT_FOUND:    errno = ENOTDIR; break;
      case ERROR_FILE_NOT_FOUND:    errno = ENOENT; break;
      case ERROR_NOT_ENOUGH_MEMORY: errno = ENOMEM; break;
      default:                      errno = EIO;
    }
    return NULL;
  }
  else if (res > PATH_MAX) {
    errno = ENAMETOOLONG;
    return NULL;
  }
  else if (resolved_name != NULL) {
    return resolved_name;
  }
  else {
    return mi_theap_strndup(xtheap, buf, PATH_MAX);
  }
}

#else

#include <unistd.h>  // pathconf

static size_t mi_path_max(void) {
  static _Atomic(size_t) path_max; // = 0
  size_t pmax = mi_atomic_load_acquire(&path_max);
  if (pmax == 0) {
    long m = 0;
    #ifdef _PC_PATH_MAX
    m = pathconf("/",_PC_PATH_MAX);
    #endif
    if (m <= 0) pmax = 4096;      // guess
    else if (m < 256) pmax = 256; // at least 256
    else if (m > 64*1024) pmax = 64*1024;  // at most 64 KiB
    else pmax = m;
    size_t expected = 0;
    mi_atomic_cas_strong_acq_rel(&path_max, &expected, pmax);
  }
  return pmax;
}

char* mi_theap_realpath(mi_theap_t* xtheap, const char* fname, char* resolved_name) mi_attr_noexcept {
  if (resolved_name != NULL) {
    return realpath(fname,resolved_name);
  }
  else {
  /*
    char* rname = realpath(fname, NULL);
    if (rname == NULL) return NULL;
    char* result = mi_heap_strdup(heap, rname);
    mi_cfree(rname);  // note: may leak the original pointer if allocated internally with the system allocator
    // note: with ASAN realpath is intercepted and mi_cfree may leak the returned pointer :-(
    return result;
  */
    const size_t n  = mi_path_max();
    char* const buf = (char*)mi_theap_zalloc(xtheap,n+1);
    if (buf == NULL) {
      errno = ENOMEM;
      return NULL;
    }
    char* rname  = realpath(fname,buf);
    char* result = mi_theap_strndup(xtheap,rname,n); // ok if `rname==NULL`
    mi_free(buf);
    return result;
  }
}
#endif

mi_decl_nodiscard mi_decl_restrict char* mi_realpath(const char* fname, char* resolved_name) mi_attr_noexcept {
  return mi_theap_realpath(_mi_theap_default(),fname,resolved_name);
}

mi_decl_nodiscard mi_decl_restrict char* mi_heap_realpath(mi_heap_t* heap, const char* fname, char* resolved_name) mi_attr_noexcept {
  return mi_theap_realpath(_mi_heap_theap(heap), fname, resolved_name);
}
#endif

/*-------------------------------------------------------
C++ new and new_aligned
The standard requires calling into `get_new_handler` and
throwing the bad_alloc exception on failure. If we compile
with a C++ compiler we can implement this precisely. If we
use a C compiler we cannot throw a `bad_alloc` exception
but we call `abort` instead (i.e. not returning).
Also, the standard requires calling the new handler until
it returns false, but we limit the total calls.
-------------------------------------------------------*/
#define MI_TRY_NEW_MAX (4)

#ifdef __cplusplus
#include <new>
static bool mi_try_new_handler(void) {
  #if defined(_MSC_VER) || (__cplusplus >= 201103L)
    std::new_handler h = std::get_new_handler();
  #else
    std::new_handler h = std::set_new_handler();
    std::set_new_handler(h);
  #endif
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");
    #if defined(_CPPUNWIND) || defined(__cpp_exceptions)  // exceptions are not always enabled
    throw std::bad_alloc();
    #endif
    return false;
  }
  else {
    h();
    return true;
  }
}
#else
typedef void (*std_new_handler_t)(void);

// Define MI_NO_CXX_NEW_HANDLER to not define a weak `std::get_new_handler` (`_ZSt15get_new_handlerv`)
// when the C++ new-handler is never used (e.g. when only the C API is used).
#if (defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER))) && !defined(MI_NO_CXX_NEW_HANDLER)  // exclude clang-cl, see issue #631
std_new_handler_t __attribute__((weak)) _ZSt15get_new_handlerv(void) {
  return NULL;
}
static std_new_handler_t mi_get_new_handler(void) {
  return _ZSt15get_new_handlerv();
}
#else
// note: on windows we could dynamically link to `?get_new_handler@std@@YAP6AXXZXZ`.
static std_new_handler_t mi_get_new_handler(void) {
  return NULL;
}
#endif

static bool mi_try_new_handler(void) {
  std_new_handler_t h = mi_get_new_handler();
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");
    abort();  // cannot throw in plain C, use abort
    return false;
  }
  else {
    h();
    return true;
  }
}
#endif

static mi_decl_noinline mi_decl_restrict void* mi_try_new_handler_null(void) {
  mi_try_new_handler();
  return NULL;
}

// called when an allocation fails and the new handler needs to be invoked
static mi_decl_noinline void* mi_theap_new_handler(mi_theap_t* theap, size_t size) {
  void* p = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && p == NULL && mi_try_new_handler(); i++) {
    if (size > MI_MAX_ALLOC_SIZE) return NULL; // call try_new_handler at least once
    p = mi_theap_malloc(theap,size);
  }
  return p;
}

#if MI_SAMPLE==2
static mi_decl_noinline void* mi_theap_malloc_sampled_new(mi_theap_t* theap, size_t req_size, bool zero, mi_page_t** ppage) {
  void* p = _mi_theap_malloc_sampled(theap, req_size, zero, ppage);
  if mi_unlikely(p == NULL) {
    return mi_theap_new_handler(theap, req_size);
  }
  return p;
}
#endif

// Fast tail calling allocation that invokes the new handler on failure (through `mi_malloc_generic_new`)
mi_decl_nodiscard mi_decl_restrict void* mi_theap_alloc_new(mi_theap_t* theap, size_t size) {
  // fast path for small objects
  #if MI_THEAP_INITASNULL
  if mi_likely(theap!=NULL && size <= MI_SMALL_SIZE_MAX)
  #else
  if mi_likely(size <= MI_SMALL_SIZE_MAX)
  #endif
  {
    return mi_theap_nonnull_xmalloc_small_zero(theap, size, false /* is wsize? */, false /* zero */, true /* is_new */, NULL);
  }
  else {
    return mi_malloc_generic_new(theap, size + MI_PADDING_SIZE, 0, NULL);
  }
}

mi_decl_nodiscard mi_decl_restrict void* mi_new(size_t size) {
  return mi_theap_alloc_new(_mi_theap_default(), size);
}

mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_new(mi_heap_t* heap, size_t size) {
  return mi_theap_alloc_new(_mi_heap_theap_get_or_init(heap), size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_alloc_new(mi_heap_t* heap, size_t size) {
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_alloc_new(theap, size); }
                       else { return mi_heap_init_new(heap, size); }
}

mi_decl_nodiscard mi_decl_restrict void* mi_theap_alloc_new_n(mi_theap_t* theap, size_t count, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(count, size, &total)) {
    return mi_try_new_handler_null();  // on overflow we tail invoke the try_new_handler once to potentially throw std::bad_alloc
  }
  else {
    return mi_theap_alloc_new(theap,total);
  }
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_n(size_t count, size_t size) {
  return mi_theap_alloc_new_n(_mi_theap_default(), count, size);
}

mi_decl_nodiscard static mi_decl_noinline mi_decl_restrict void* mi_heap_init_new_n(mi_heap_t* heap, size_t count, size_t size) {
  return mi_theap_alloc_new_n(_mi_heap_theap_get_or_init(heap), count, size);
}
mi_decl_nodiscard mi_decl_restrict void* mi_heap_alloc_new_n(mi_heap_t* heap, size_t count, size_t size) {
  mi_theap_t* const theap = _mi_heap_theap_cached(heap);
  if mi_likely(theap!=NULL) { return mi_theap_alloc_new_n(theap, count, size); }
                       else { return mi_heap_init_new_n(heap, count, size); }
}

mi_decl_nodiscard mi_decl_restrict void* mi_theap_alloc_new_nothrow(mi_theap_t* theap, size_t size) mi_attr_noexcept {
  return mi_theap_malloc(theap,size);
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_nothrow(size_t size) mi_attr_noexcept {
  return mi_malloc(size);
}

static mi_decl_noinline void* mi_try_new_aligned(size_t size, size_t alignment) {
  void* p = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && p==NULL && mi_try_new_handler(); i++) {
    if (!mi_alignment_is_valid(alignment)) return NULL;
    p = mi_malloc_aligned(size,alignment);
  }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned(size_t size, size_t alignment) {
  void* p = mi_malloc_aligned(size, alignment);
  if mi_unlikely(p==NULL) return mi_try_new_aligned(size,alignment);
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned_nothrow(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_malloc_aligned(size, alignment);
}

static mi_decl_noinline void* mi_try_new_realloc(void* p, size_t newsize) {
  void* q = NULL;
  for(int i = 0; i < MI_TRY_NEW_MAX && q==NULL && mi_try_new_handler(); i++) {
    if (newsize > MI_MAX_ALLOC_SIZE) return NULL;
    q = mi_realloc(p,newsize);
  }
  return q;
}

mi_decl_nodiscard void* mi_new_realloc(void* p, size_t newsize) {
  void* q = mi_realloc(p, newsize);
  if (q == NULL) return mi_try_new_realloc(p,newsize);
  return q;
}

mi_decl_nodiscard void* mi_new_reallocn(void* p, size_t newcount, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(newcount, size, &total)) {
    return mi_try_new_handler_null();
  }
  else {
    return mi_new_realloc(p, total);
  }
}

// ------------------------------------------------------
// ensure explicit external inline definitions are emitted!
// ------------------------------------------------------

#ifdef __cplusplus
void* _mi_externs[] = {
  (void*)&_mi_page_malloc_zero,
  (void*)&_mi_theap_malloc_zero,
  (void*)&mi_theap_malloc,
  (void*)&mi_theap_zalloc,
  (void*)&mi_theap_malloc_small,
  (void*)&mi_theap_zalloc_small,
  (void*)&mi_theap_calloc,
  (void*)&mi_malloc,
  (void*)&mi_malloc_small,
  (void*)&mi_zalloc,
  (void*)&mi_zalloc_small,
  (void*)&mi_heap_malloc,
  (void*)&mi_heap_malloc_small,
  (void*)&mi_malloc_aligned
  // (void*)&mi_theap_alloc_new,
  // (void*)&mi_theap_alloc_new_n
};
#endif
