/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_SAMPLE_GUARDED_H
#define MI_INTERNAL_SAMPLE_GUARDED_H

#include "../types.h"
#include "sample-profile.h"

void          _mi_page_unguard_all(mi_page_t* page);
mi_decl_restrict void* _mi_theap_malloc_guarded(mi_theap_t* theap, size_t size, bool zero, mi_page_t** ppage) mi_attr_noexcept;
void          _mi_page_block_unguard(mi_page_t* page, mi_block_t* block, void* p);
void          _mi_theap_guarded_init(mi_theap_t* theap);

#define MI_BLOCK_TAG_GUARDED   (~MI_BLOCK_TAG_ALIGNED)

static inline bool mi_block_ptr_is_guarded(const mi_block_t* block, const void* p) {
#if MI_GUARDED
  const ptrdiff_t offset = (uint8_t*)p - (uint8_t*)block;
  return (offset >= (ptrdiff_t)(sizeof(mi_block_t)) && block->next == MI_BLOCK_TAG_GUARDED);
#else
  MI_UNUSED(block); MI_UNUSED(p);
  return false;
#endif
}

#endif
