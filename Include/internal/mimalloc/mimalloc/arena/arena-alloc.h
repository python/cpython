/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_ARENA_ALLOC_H
#define MI_INTERNAL_ARENA_ALLOC_H

#include "../types.h"

void*         _mi_arenas_alloc(mi_heap_t* heap, size_t size, bool commit, bool allow_pinned, mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid);
void*         _mi_arenas_alloc_aligned(mi_heap_t* heap, size_t size, size_t alignment, size_t align_offset, bool commit, bool allow_pinned, mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid);
void          _mi_arenas_free(mi_subproc_t* subproc, void* p, size_t size, mi_memid_t memid);
void          _mi_arenas_collect(bool force_purge, bool visit_all, mi_tld_t* tld);

#endif
