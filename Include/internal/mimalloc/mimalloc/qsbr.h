/* ----------------------------------------------------------------------------
Copyright (c) 2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
  Quiescent State-Based Reclamation (qsbr).
  QSBR as used in CPython is a technique for safe memory reclamation in concurrent
  environments. In such case we need to delay free-ing pages until there are no
  more concurrent readers; the interface below provides the necessary primitives
  for managing QSBR state so mimalloc can delay freeing pages until it is
  safe to do so (and efficiently reuse them as well).

  See `/test/test-qsbr.c` for a minimal example implementation.
-----------------------------------------------------------------------------*/

#pragma once
#ifndef MI_QSBR_H
#define MI_QSBR_H

#include <stddef.h>
#include <stdbool.h>

#ifndef MI_QSBR
#if defined(MI_CPYTHON)
#define MI_QSBR 1
#else
#define MI_QSBR 0
#endif
#endif

// Create a qsbr enabled heap
mi_decl_nodiscard mi_decl_export mi_heap_t* mi_heap_new_qsbr(void);

#if MI_QSBR
// Define these for the host runtime to implement qsbr bookkeeping.
typedef struct mi_page_qsbr_s mi_page_qsbr_t;
typedef struct mi_heap_qsbr_s mi_heap_qsbr_t;

// Host-defined runtime bookkeeping primitives; see doc/qsbr.md. (called by mimalloc for pages in qsbr heaps)
#ifdef __cplusplus
extern "C" {
#endif
void mi_heap_qsbr_init(mi_heap_qsbr_t* heap);
void mi_page_qsbr_clear(mi_page_qsbr_t* page);
bool mi_page_qsbr_is_clear(const mi_page_qsbr_t* page);
// Establish a goal if needed, then check whether release of the empty page must be delayed.
bool mi_page_qsbr_should_delay(mi_page_qsbr_t* page, mi_heap_qsbr_t* heap, size_t page_bytes);
#ifdef __cplusplus
}
#endif
#endif

#endif
