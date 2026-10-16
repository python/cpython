/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026 Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#pragma once
#ifndef MI_BITMAP_CHUNK_H
#define MI_BITMAP_CHUNK_H

// Type definitions for bitmap chunks
#include "bitmap.h"

// Exported functions for bitmap chunks
size_t      mi_bfield_popcount(mi_bfield_t x);
mi_bfield_t mi_bfield_clear_least_bit(mi_bfield_t x);
bool        mi_bfield_find_least_bit(mi_bfield_t x, size_t* idx);
bool        mi_bfield_find_highest_bit(mi_bfield_t x, size_t* idx);
bool        mi_bfield_foreach_bit(mi_bfield_t* x, size_t* idx);
mi_bfield_t mi_bfield_mask(size_t bit_count, size_t shiftl);

bool        mi_bchunk_set(mi_bchunk_t* chunk, size_t cidx, size_t* already_set);
bool        mi_bchunk_setN(mi_bchunk_t* chunk, size_t cidx, size_t n, size_t* already_set);
bool        mi_bchunk_clear(mi_bchunk_t* chunk, size_t cidx, bool* all_clear);
bool        mi_bchunk_clearN(mi_bchunk_t* chunk, size_t cidx, size_t n, bool* maybe_all_clear);
size_t      mi_bchunk_popcountN(mi_bchunk_t* chunk, size_t cidx, size_t n);
bool        mi_bchunk_is_xsetN(mi_xset_t set, const mi_bchunk_t* chunk, size_t cidx, size_t n);

bool        mi_bchunk_try_clearN(mi_bchunk_t* chunk, size_t cidx, size_t n, bool* maybe_all_clear, bool* did_temp_clear_bits);
bool        mi_bchunk_try_find_and_clear(mi_bchunk_t* chunk, size_t* pidx);
bool        mi_bchunk_try_find_and_clear_1(mi_bchunk_t* chunk, size_t n, size_t* pidx, bool* did_temp_clear_bits);
bool        mi_bchunk_try_find_and_clear_8(mi_bchunk_t* chunk, size_t n, size_t* pidx, bool* did_temp_clear_bits);
mi_decl_noinline bool mi_bchunk_try_find_and_clearNX(mi_bchunk_t* chunk, size_t n, size_t* pidx, bool* did_temp_clear_bits);
mi_decl_noinline bool mi_bchunk_try_find_and_clearNC(mi_bchunk_t* chunk, size_t n, size_t* pidx, bool* did_temp_clear_bits);

void        mi_bchunk_clear_once_set(mi_subproc_t* subproc, mi_bchunk_t* chunk, size_t cidx);
bool        mi_bchunk_all_are_clear_relaxed(mi_bchunk_t* chunk);
bool        mi_bchunk_all_are_set_relaxed(mi_bchunk_t* chunk);
bool        mi_bchunk_bsr(mi_bchunk_t* chunk, size_t* pidx);
bool        mi_bchunk_bsr_inv(mi_bchunk_t* chunk, size_t* pidx);
size_t      mi_bchunk_popcount(mi_bchunk_t* chunk);
void        mi_bchunks_unsafe_setN(mi_bchunk_t* chunks, mi_bchunkmap_t* cmap, size_t idx, size_t n);

/* --------------------------------------------------------------------------------
  Iterate through a bfield
-------------------------------------------------------------------------------- */

// Cycle iteration through a bitfield. This is used to space out threads
// so there is less chance of contention. When searching for a free page we
// like to first search only the accessed part (so we reuse better). This
// high point is called the `cycle`.
//
// We then iterate through the bitfield as:
// first: [start, cycle>
// then : [0, start>
// then : [cycle, MI_BFIELD_BITS>
//
// The start is determined usually as `tseq % cycle` to have each thread
// start at a different spot.
// - We use `popcount` to improve branch prediction (maybe not needed? can we simplify?)
// - The `cycle_mask` is the part `[start, cycle>`.
#define mi_bfield_iterate(bfield,start,cycle,name_idx,SUF) { \
  mi_assert_internal(start <= cycle); \
  mi_assert_internal(start < MI_BFIELD_BITS); \
  mi_assert_internal(cycle <= MI_BFIELD_BITS); \
  const mi_bfield_t _cycle_mask##SUF = mi_bfield_mask(cycle - start, start); \
  size_t _bcount##SUF = mi_bfield_popcount(bfield); \
  mi_bfield_t _b##SUF = bfield & _cycle_mask##SUF; /* process [start, cycle> first*/\
  while(_bcount##SUF > 0) { \
    _bcount##SUF--;\
    if (_b##SUF==0) { _b##SUF = bfield & ~_cycle_mask##SUF; } /* process [0,start> + [cycle, MI_BFIELD_BITS> next */ \
    /* size_t name_idx; */ \
    const bool _found##SUF = mi_bfield_find_least_bit(_b##SUF,&name_idx); \
    _b##SUF = mi_bfield_clear_least_bit(_b##SUF); /* clear early so `continue` works */ \
    mi_assert_internal(_found##SUF); MI_UNUSED(_found##SUF); \
    { \

#define mi_bfield_iterate_end(SUF) \
    } \
  } \
}


#define mi_bfield_cycle_iterate(bfield,tseq,cycle,name_idx,SUF) { \
  const size_t _start##SUF = (uint32_t)(tseq) % (uint32_t)(cycle); /* or: 0 to always search from the start? */\
  mi_bfield_iterate(bfield,_start##SUF,cycle,name_idx,SUF)

#define mi_bfield_cycle_iterate_end(SUF) \
  mi_bfield_iterate_end(SUF); \
}

#endif
