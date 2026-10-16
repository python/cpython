/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_HEAP_H
#define MI_INTERNAL_HEAP_H

#include "../types.h"
#include "init.h"
#include "subproc.h"

void          _mi_heap_init(mi_heap_t* heap, mi_thread_local_t theap, mi_subproc_t* subproc, mi_arena_id_t exclusive_arena_id, bool use_qsbr);
mi_decl_cold  mi_theap_t* _mi_heap_theap_get_or_init(const mi_heap_t* heap);  // get (and possible create) the theap belonging to a heap
void          _mi_heap_force_destroy(mi_heap_t* heap, bool acquire_heaps_lock); // allow destroying the main heap
mi_heap_t*    _mi_heap_new_for_subproc(mi_subproc_t* subproc, mi_arena_id_t exclusive_arena_id, bool is_heap_main, bool use_qsbr);
bool          _mi_heap_theap_set(mi_heap_t* heap, mi_theap_t* theap);

static inline bool mi_heap_qsbr_is_enabled(const mi_heap_t* heap) {
#if MI_QSBR
  return heap->qsbr_enabled;
#else
  MI_UNUSED(heap);
  return false;
#endif
}

static inline long mi_heap_reclaim_on_free(mi_heap_t* heap) {
  return heap->reclaim_on_free;
}

static inline mi_heap_t* mi_heap_get_heap_main(const mi_heap_t* heap) {
  return _mi_subproc_heap_main(heap->subproc);
}

static inline bool mi_is_heap_main(const mi_heap_t* heap) {
  mi_assert_internal(heap!=NULL);
  return (mi_heap_get_heap_main(heap) == heap);
}

static inline bool mi_is_process_heap_main(const mi_heap_t* heap) {
  mi_assert_internal(heap!=NULL);
  return (_mi_subproc_main()->heap_main == heap);
}

#endif
