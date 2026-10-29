/*----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#pragma once
#ifndef MI_PAGE_QUEUE_H
#define MI_PAGE_QUEUE_H

#include "mimalloc.h"
#include "mimalloc/types.h"
#include "mimalloc/alloc/page.h"

#if (MI_DEBUG>1)
bool mi_page_queue_contains(mi_page_queue_t* queue, const mi_page_t* page);

static inline bool mi_theap_contains_queue(const mi_theap_t* theap, const mi_page_queue_t* pq) {
  return (pq >= &theap->pages[0] && pq <= &theap->pages[MI_BIN_FULL]);
}
#endif

mi_page_queue_t* mi_theap_page_queue_of(mi_theap_t* theap, const mi_page_t* page);
void mi_page_queue_remove(mi_page_queue_t* queue, mi_page_t* page);
void mi_page_queue_push(mi_theap_t* theap, mi_page_queue_t* queue, mi_page_t* page);
void mi_page_queue_push_at_end(mi_theap_t* theap, mi_page_queue_t* queue, mi_page_t* page);
void mi_page_queue_enqueue_from(mi_page_queue_t* to, mi_page_queue_t* from, mi_page_t* page);
void mi_page_queue_enqueue_from_full(mi_page_queue_t* to, mi_page_queue_t* from, mi_page_t* page);
void mi_page_queue_move_to_front(mi_theap_t* theap, mi_page_queue_t* queue, mi_page_t* page);
void mi_page_queue_move_to_back(mi_theap_t* theap, mi_page_queue_t* queue, mi_page_t* page);


static inline bool mi_page_queue_is_huge(const mi_page_queue_t* pq) {
  return (pq->block_size == (MI_LARGE_MAX_OBJ_SIZE+sizeof(uintptr_t)));
}

static inline bool mi_page_queue_is_full(const mi_page_queue_t* pq) {
  return (pq->block_size == (MI_LARGE_MAX_OBJ_SIZE+(2*sizeof(uintptr_t))));
}

static inline bool mi_page_queue_is_special(const mi_page_queue_t* pq) {
  return (pq->block_size > MI_LARGE_MAX_OBJ_SIZE);
}

static inline size_t mi_page_queue_count(const mi_page_queue_t* pq) {
  return pq->count;
}

static inline mi_page_queue_t* mi_page_queue_of(const mi_page_t* page) {
  mi_theap_t* theap = mi_page_theap(page);
  mi_page_queue_t* pq = mi_theap_page_queue_of(theap, page);
  mi_assert_expensive(mi_page_queue_contains(pq, page));
  return pq;
}

#endif
