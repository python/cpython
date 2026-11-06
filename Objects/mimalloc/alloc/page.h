/*----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_PAGE_H
#define MI_PAGE_H

#include "mimalloc.h"
#include "page-queue.h"

bool       _mi_page_extend_free(mi_theap_t* theap, mi_page_t* page);
mi_page_t* _mi_page_fresh_alloc(mi_theap_t* theap, mi_page_queue_t* pq, size_t block_size, size_t page_alignment);
mi_page_t* _mi_page_fresh(mi_theap_t* theap, mi_page_queue_t* pq);
void       _mi_page_to_full(mi_page_t* page, mi_page_queue_t* pq);
void       _mi_page_update_sample_countdown(mi_page_t* page);

static inline void _mi_theap_qsbr_collect_delayed(mi_theap_t* theap) {
#if MI_QSBR
  mi_page_t* page;
  while ((page = theap->qsbr_first) != NULL) {
    _mi_page_free_collect(page, false);
    if (!mi_page_all_free(page)) {
      _mi_page_qsbr_clear(page);
    }
    else if (!_mi_page_free(page, mi_page_queue_of(page)) && theap->qsbr_first == page) {
      break;
    }
  }
#else
  MI_UNUSED(theap);
#endif
}

#endif
