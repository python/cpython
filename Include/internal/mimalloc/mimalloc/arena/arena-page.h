/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_ARENA_PAGE_H
#define MI_INTERNAL_ARENA_PAGE_H

#include "../types.h"

mi_page_t*    _mi_arenas_page_alloc(mi_theap_t* theap, size_t block_size, size_t page_alignment);
void          _mi_arenas_page_free(mi_page_t* page, mi_theap_t* current_theapx /* can be NULL */);
void          _mi_arenas_page_abandon(mi_page_t* page, mi_theap_t* current_theap);
void          _mi_arenas_page_unabandon(mi_page_t* page, mi_theap_t* current_theapx /* can be NULL */);
bool          _mi_arenas_page_try_reabandon_to_mapped(mi_page_t* page);
void          _mi_heap_move_pages(mi_heap_t* heap_from, mi_heap_t* heap_to);
void          _mi_heap_destroy_pages(mi_heap_t* heap_from);
#if MI_QSBR
// The caller must stop all allocation, freeing, and theap lifecycle operations on this heap.
void          _mi_heap_collect_abandoned(mi_heap_t* heap);
#endif

#endif
