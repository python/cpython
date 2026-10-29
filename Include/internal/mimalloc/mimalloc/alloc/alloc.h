/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_ALLOC_H
#define MI_INTERNAL_ALLOC_H

#include "../types.h"
#include "../util/libc.h"
#include "../util/options.h"

mi_decl_restrict void* _mi_page_malloc_zero(mi_theap_t* theap, mi_page_t* page, size_t size, bool zero) mi_attr_noexcept;                  // called from `_mi_theap_malloc_aligned`
mi_decl_restrict void* _mi_theap_malloc_zero(mi_theap_t* theap, size_t size, bool zero, size_t huge_alignment, mi_page_t** ppage) mi_attr_noexcept;     // called from `_mi_theap_malloc_aligned`
void*                  _mi_theap_realloc_zero(mi_theap_t* theap, void* p, size_t newsize, bool zero) mi_attr_noexcept;

// Safe multiply `count*size` into `total`; return `true` on overflow.
static inline bool mi_count_size_overflow(size_t count, size_t size, size_t* total) {
  if (count==1) {  // quick check for the case where count is one (common for C++ allocators)
    *total = size;
    return false;
  }
  else if mi_likely(!mi_mul_overflow(count, size, total)) {
    return false;
  }
  else {
    #if MI_DEBUG > 0
    _mi_error_message(EOVERFLOW, "allocation request is too large (%zu * %zu bytes)\n", count, size);
    #endif
    *total = SIZE_MAX;
    return true;
  }
}

#endif
