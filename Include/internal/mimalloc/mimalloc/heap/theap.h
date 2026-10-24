/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_THEAP_H
#define MI_INTERNAL_THEAP_H

#include "../types.h"

size_t        _mi_theap_random_next(mi_theap_t* theap);
void          _mi_theap_init(mi_theap_t* theap, mi_heap_t* heap, mi_tld_t* tld);
mi_theap_t*   _mi_theap_alloc(mi_heap_t* heap, mi_tld_t* tld);
mi_theap_t*   _mi_theap_create(mi_heap_t* heap, mi_tld_t* tld);
void          _mi_theap_collect_abandon(mi_theap_t* theap);
void          _mi_theap_abandon_owned_pages(mi_theap_t* theap);
bool          _mi_theap_area_visit_blocks(const mi_heap_area_t* area, mi_page_t* page, mi_block_visit_fun* visitor, void* arg);

void          _mi_heap_detach_theaps( mi_heap_t* heap );
void          _mi_tld_detach_theaps( mi_tld_t* tld );
void          _mi_theap_incref(mi_theap_t* theap);
void          _mi_theap_decref(mi_theap_t* theap);
void          _mi_theap_merge_stats(mi_theap_t* theap);

void          _mi_heap_area_init(mi_heap_area_t* area, mi_page_t* page);

/*----------------------------------------------------------------------------------------
  Heap functions
------------------------------------------------------------------------------------------- */

static inline mi_heap_t* mi_theap_heap_peek(const mi_theap_t* theap) {
  mi_assert_internal(theap!=NULL);
  return mi_atomic_load_ptr_relaxed(mi_heap_t,&theap->heap);
}

static inline mi_heap_t* mi_theap_heap(const mi_theap_t* theap) {
  mi_heap_t* const heap = mi_theap_heap_peek(theap);
  mi_assert_internal(heap!=NULL);
  return heap;
}

static inline bool mi_theap_is_initialized(const mi_theap_t* theap) {
  return (theap != NULL && mi_theap_heap_peek(theap) != NULL);
}

static inline mi_subproc_t* mi_theap_subproc(const mi_theap_t* theap) {
  mi_subproc_t* const subproc = mi_atomic_load_ptr_relaxed(mi_subproc_t,&theap->subproc);
  mi_assert_internal(!mi_theap_is_initialized(theap) || mi_theap_heap(theap)->subproc == subproc);
  return subproc;
}

static inline mi_page_t* mi_theap_get_free_small_page(mi_theap_t* theap, size_t xsize, bool is_wsize) {
  mi_assert_internal(is_wsize ? xsize <= (MI_SMALL_WSIZE_MAX + MI_PADDING_WSIZE) : xsize <= (MI_SMALL_SIZE_MAX + MI_PADDING_SIZE));
  const size_t idx = (is_wsize ? xsize : mi_wsize_from_size(xsize));
  mi_assert_internal(idx < MI_PAGES_DIRECT);
  return theap->pages_free_direct[idx];
}

static inline bool mi_theap_is_detached(mi_theap_t* theap) {
  return (theap!=NULL && theap->tld->thread_id == MI_THREADID_DETACHED);
}

#endif
