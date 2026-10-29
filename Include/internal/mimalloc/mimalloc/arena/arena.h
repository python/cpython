/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_ARENA_H
#define MI_INTERNAL_ARENA_H

#include "../types.h"

mi_arena_id_t _mi_arena_id_none(void);
mi_arena_t*   _mi_arena_from_id(mi_arena_id_t id);
bool          _mi_arena_memid_is_suitable(mi_memid_t memid, mi_arena_t* request_arena);

void          _mi_arenas_unsafe_destroy_all(mi_subproc_t* subproc);

mi_heap_t*    _mi_arena_heap_main(const mi_arena_t* arena);
size_t        _mi_slice_count_of_size(size_t size);
size_t        _mi_size_of_slices(size_t bcount);

#endif
