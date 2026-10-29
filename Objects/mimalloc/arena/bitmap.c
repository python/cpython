/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026 Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
Concurrent bitmap that can set/reset sequences of bits atomically
---------------------------------------------------------------------------- */

#include "mimalloc.h"
#include "mimalloc/util/libc.h"
#include "mimalloc/util/stats.h"
#include "bitmap-chunk.h"
#include "bitmap.h"

/* --------------------------------------------------------------------------------
 bitmap chunkmap
-------------------------------------------------------------------------------- */

static void mi_bitmap_chunkmap_set(mi_bitmap_t* bitmap, size_t chunk_idx) {
  mi_assert(chunk_idx < mi_bitmap_chunk_count(bitmap));
  mi_bchunk_set(&bitmap->chunkmap, chunk_idx, NULL);
}

static bool mi_bitmap_chunkmap_try_clear(mi_bitmap_t* bitmap, size_t chunk_idx) {
  mi_assert(chunk_idx < mi_bitmap_chunk_count(bitmap));
  // check if the corresponding chunk is all clear
  if (!mi_bchunk_all_are_clear_relaxed(&bitmap->chunks[chunk_idx])) return false;
  // clear the chunkmap bit
  mi_bchunk_clear(&bitmap->chunkmap, chunk_idx, NULL);
  // .. but a concurrent set may have happened in between our all-clear test and the clearing of the
  // bit in the mask. We check again to catch this situation.
  if (!mi_bchunk_all_are_clear_relaxed(&bitmap->chunks[chunk_idx])) {
    mi_bchunk_set(&bitmap->chunkmap, chunk_idx, NULL);
    return false;
  }
  return true;
}


/* --------------------------------------------------------------------------------
  bitmap
-------------------------------------------------------------------------------- */

size_t mi_bitmap_size(size_t bit_count, size_t* pchunk_count) {
  mi_assert_internal((bit_count % MI_BCHUNK_BITS) == 0);
  bit_count = mi_align_up(bit_count, MI_BCHUNK_BITS);
  mi_assert_internal(bit_count <= MI_BITMAP_MAX_BIT_COUNT);
  mi_assert_internal(bit_count > 0);
  const size_t chunk_count = bit_count / MI_BCHUNK_BITS;
  mi_assert_internal(chunk_count >= 1);
  const size_t size = offsetof(mi_bitmap_t,chunks) + (chunk_count * MI_BCHUNK_SIZE);
  mi_assert_internal( (size%MI_BCHUNK_SIZE) == 0 );
  if (pchunk_count != NULL) { *pchunk_count = chunk_count;  }
  return size;
}


// initialize a bitmap to all unset; avoid a mem_zero if `already_zero` is true
// returns the size of the bitmap
size_t mi_bitmap_init(mi_bitmap_t* bitmap, size_t bit_count, bool already_zero) {
  size_t chunk_count;
  const size_t size = mi_bitmap_size(bit_count, &chunk_count);
  if (!already_zero) {
    mi_memzero_aligned(bitmap, size);
  }
  mi_atomic_store_release(&bitmap->chunk_count, chunk_count);
  mi_assert_internal(mi_atomic_load_relaxed(&bitmap->chunk_count) <= MI_BITMAP_MAX_CHUNK_COUNT);
  return size;
}


// Set a sequence of `n` bits in the bitmap (and can cross chunks). Not atomic so only use if local to a thread.
void mi_bitmap_unsafe_setN(mi_bitmap_t* bitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  mi_assert_internal(idx + n <= mi_bitmap_max_bits(bitmap));
  mi_bchunks_unsafe_setN(&bitmap->chunks[0], &bitmap->chunkmap, idx, n);
}




// ------- mi_bitmap_xset ---------------------------------------

// Set a sequence of `n` bits in the bitmap; returns `true` if atomically transitioned from 0's to 1's (or 1's to 0's).
bool mi_bitmap_setN(mi_bitmap_t* bitmap, size_t idx, size_t n, size_t* palready_set) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bitmap_max_bits(bitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return false;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  bool were_allclear = true;
  size_t already_set = 0;
  while (n > 0) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    size_t _already_set = 0;
    were_allclear = mi_bchunk_setN(&bitmap->chunks[chunk_idx], cidx, m, &_already_set) && were_allclear;
    already_set += _already_set;
    mi_bitmap_chunkmap_set(bitmap, chunk_idx); // set afterwards
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  if (palready_set != NULL) { *palready_set = already_set;  }
  return were_allclear;
}

// Clear a sequence of `n` bits in the bitmap; returns `true` if atomically transitioned from 1's to 0's.
bool mi_bitmap_clearN(mi_bitmap_t* bitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bitmap_max_bits(bitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return false;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  bool were_allset = true;
  while (n > 0) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    bool maybe_all_clear = false;
    were_allset = mi_bchunk_clearN(&bitmap->chunks[chunk_idx], cidx, m, &maybe_all_clear) && were_allset;
    if (maybe_all_clear) { mi_bitmap_chunkmap_try_clear(bitmap, chunk_idx); }
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  return were_allset;
}

// Count bits set in a range of `n` bits.
size_t mi_bitmap_popcountN( mi_bitmap_t* bitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bitmap_max_bits(bitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return 0;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  size_t popcount = 0;
  while (n > 0) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    popcount += mi_bchunk_popcountN(&bitmap->chunks[chunk_idx], cidx, m);
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  return popcount;
}


// Set/clear a bit in the bitmap; returns `true` if atomically transitioned from 0 to 1 (or 1 to 0)
bool mi_bitmap_set(mi_bitmap_t* bitmap, size_t idx) {
  return mi_bitmap_setN(bitmap, idx, 1, NULL);
}

bool mi_bitmap_clear(mi_bitmap_t* bitmap, size_t idx) {
  return mi_bitmap_clearN(bitmap, idx, 1);
}



// ------- mi_bitmap_is_xset ---------------------------------------

// Is a sequence of n bits already all set/cleared?
bool mi_bitmap_is_xsetN(mi_xset_t set, mi_bitmap_t* bitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bitmap_max_bits(bitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return false;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  bool xset = true;
  while (n > 0 && xset) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    xset = mi_bchunk_is_xsetN(set, &bitmap->chunks[chunk_idx], cidx, m) && xset;
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  return xset;
}

bool mi_bitmap_is_all_clear(mi_bitmap_t* bitmap) {
  return mi_bitmap_is_xsetN(MI_BIT_CLEAR, bitmap, 0, mi_bitmap_max_bits(bitmap));
}

/* --------------------------------------------------------------------------------
  mi_bitmap_find
  (used to find free pages)
-------------------------------------------------------------------------------- */

typedef bool (mi_bitmap_visit_fun_t)(mi_bitmap_t* bitmap, size_t chunk_idx, size_t n, size_t* idx, void* arg1, void* arg2);

// Go through the bitmap and for every sequence of `n` set bits, call the visitor function.
// If it returns `true` stop the search.
static inline bool mi_bitmap_find(mi_bitmap_t* bitmap, size_t tseq, size_t n, size_t* pidx, mi_bitmap_visit_fun_t* on_find, void* arg1, void* arg2)
{
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for (size_t i = 0; i < chunkmap_max; i++) {
    // and for each chunkmap entry we iterate over its bits to find the chunks
    const mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t hi;
    if (mi_bfield_find_highest_bit(cmap_entry, &hi)) {
      size_t eidx = 0;
      mi_bfield_cycle_iterate(cmap_entry, tseq%8, hi+1, eidx, Y) // reduce the tseq to 8 bins to reduce using extra memory (see `mstress`)
      {
        mi_assert_internal(eidx <= MI_BFIELD_BITS);
        const size_t chunk_idx = i*MI_BFIELD_BITS + eidx;
        mi_assert_internal(chunk_idx < mi_bitmap_chunk_count(bitmap));
        if ((*on_find)(bitmap, chunk_idx, n, pidx, arg1, arg2)) {
          return true;
        }
      }
      mi_bfield_cycle_iterate_end(Y);
    }
  }
  return false;
}


/* --------------------------------------------------------------------------------
  Bitmap: try_find_and_claim  -- used to allocate abandoned pages
  note: the compiler will fully inline the indirect function call
-------------------------------------------------------------------------------- */

typedef struct mi_claim_fun_data_s {
  mi_arena_t*   arena;
} mi_claim_fun_data_t;

static bool mi_bitmap_try_find_and_claim_visit(mi_bitmap_t* bitmap, size_t chunk_idx, size_t n, size_t* pidx, void* arg1, void* arg2)
{
  mi_assert_internal(n==1); MI_UNUSED(n);
  mi_claim_fun_t* claim_fun = (mi_claim_fun_t*)arg1;
  mi_claim_fun_data_t* claim_data = (mi_claim_fun_data_t*)arg2;
  size_t cidx;
  if mi_likely(mi_bchunk_try_find_and_clear(&bitmap->chunks[chunk_idx], &cidx)) {
    const size_t slice_index = (chunk_idx * MI_BCHUNK_BITS) + cidx;
    mi_assert_internal(slice_index < mi_bitmap_max_bits(bitmap));
    bool keep_set = true;
    if ((*claim_fun)(slice_index, claim_data->arena, &keep_set)) {
      // success!
      mi_assert_internal(!keep_set);
      *pidx = slice_index;
      return true;
    }
    else {
      // failed to claim it, set abandoned mapping again (unless the page was freed and keep_set will be false)
      if (keep_set) {
        const bool wasclear = mi_bchunk_set(&bitmap->chunks[chunk_idx], cidx, NULL);
        mi_bitmap_chunkmap_set(bitmap, chunk_idx);
        mi_assert_internal(wasclear); MI_UNUSED(wasclear);
      }
    }
  }
  else {
    // we may find that all are cleared only on a second iteration but that is ok as
    // the chunkmap is a conservative approximation.
    mi_bitmap_chunkmap_try_clear(bitmap, chunk_idx);
  }
  return false;
}

// Find a set bit in the bitmap and try to atomically clear it and claim it.
// (Used to find pages in the pages_abandoned bitmaps.)
mi_decl_nodiscard bool mi_bitmap_try_find_and_claim(mi_bitmap_t* bitmap, size_t tseq, size_t* pidx,
  mi_claim_fun_t* claim, mi_arena_t* arena )
{
  mi_claim_fun_data_t claim_data = { arena };
  return mi_bitmap_find(bitmap, tseq, 1, pidx, &mi_bitmap_try_find_and_claim_visit, (void*)claim, &claim_data);
}


bool mi_bitmap_bsr(mi_bitmap_t* bitmap, size_t* idx) {
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for (size_t i = chunkmap_max; i > 0; ) {
    i--;
    mi_bfield_t cmap = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t cmap_idx;
    if (mi_bsr(cmap,&cmap_idx)) {
      // from highest chunk to lowest (scan all in case the cmap entry was stale)
      for (size_t j = cmap_idx+1; j>0; ) {
        j--;
        const size_t chunk_idx = (i*MI_BFIELD_BITS) + j;
        size_t cidx;
        if (mi_bchunk_bsr(&bitmap->chunks[chunk_idx], &cidx)) {
          *idx = (chunk_idx * MI_BCHUNK_BITS) + cidx;
          return true;
        }
      }
    }
  }
  return false;
}

// Return count of all set bits in a bitmap.
size_t mi_bitmap_popcount(mi_bitmap_t* bitmap) {
  // for all chunkmap entries
  size_t popcount = 0;
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for (size_t i = 0; i < chunkmap_max; i++) {
    mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t cmap_idx;
    // for each chunk (corresponding to a set bit in a chunkmap entry)
    while (mi_bfield_foreach_bit(&cmap_entry, &cmap_idx)) {
      const size_t chunk_idx = i*MI_BFIELD_BITS + cmap_idx;
      // count bits in a chunk
      popcount += mi_bchunk_popcount(&bitmap->chunks[chunk_idx]);
    }
  }
  return popcount;
}



// Clear a bit once it is set.
void mi_bitmap_clear_once_set(mi_subproc_t* subproc, mi_bitmap_t* bitmap, size_t idx) {
  mi_assert_internal(idx < mi_bitmap_max_bits(bitmap));
  const size_t chunk_idx = idx / MI_BCHUNK_BITS;
  const size_t cidx = idx % MI_BCHUNK_BITS;
  mi_assert_internal(chunk_idx < mi_bitmap_chunk_count(bitmap));
  mi_bchunk_clear_once_set(subproc, &bitmap->chunks[chunk_idx], cidx);
}


// Visit all set bits in a bitmap.
// todo: optimize further? maybe use avx512 to directly get all indices using a mask_compressstore?
bool _mi_bitmap_forall_set(mi_bitmap_t* bitmap, mi_forall_set_fun_t* visit, mi_arena_t* arena, void* arg) {
  // for all chunkmap entries
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for(size_t i = 0; i < chunkmap_max; i++) {
    mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t cmap_idx;
    // for each chunk (corresponding to a set bit in a chunkmap entry)
    while (mi_bfield_foreach_bit(&cmap_entry, &cmap_idx)) {
      const size_t chunk_idx = i*MI_BFIELD_BITS + cmap_idx;
      // for each chunk field
      mi_bchunk_t* const chunk = &bitmap->chunks[chunk_idx];
      for (size_t j = 0; j < MI_BCHUNK_FIELDS; j++) {
        const size_t base_idx = (chunk_idx*MI_BCHUNK_BITS) + (j*MI_BFIELD_BITS);
        mi_bfield_t b = mi_atomic_load_relaxed(&chunk->bfields[j]);
        size_t bidx;
        while (mi_bfield_foreach_bit(&b, &bidx)) {
          const size_t idx = base_idx + bidx;
          if (!visit(idx, 1, arena, arg)) return false;
        }
      }
    }
  }
  return true;
}

// Visit all set bits in a bitmap but try to return ranges (within bfields) if possible.
// Also clear those ranges atomically.
// Used by purging to purge larger ranges when possible
// todo: optimize further? maybe use avx512 to directly get all indices using a mask_compressstore?
bool _mi_bitmap_forall_setc_ranges(mi_bitmap_t* bitmap, mi_forall_set_fun_t* visit, mi_arena_t* arena, void* arg) {
  // for all chunkmap entries
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for (size_t i = 0; i < chunkmap_max; i++) {
    mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t cmap_idx;
    // for each chunk (corresponding to a set bit in a chunkmap entry)
    while (mi_bfield_foreach_bit(&cmap_entry, &cmap_idx)) {
      const size_t chunk_idx = i*MI_BFIELD_BITS + cmap_idx;
      // for each chunk field
      mi_bchunk_t* const chunk = &bitmap->chunks[chunk_idx];
      for (size_t j = 0; j < MI_BCHUNK_FIELDS; j++) {
        const size_t base_idx = (chunk_idx*MI_BCHUNK_BITS) + (j*MI_BFIELD_BITS);
        mi_bfield_t b = mi_atomic_exchange_relaxed(&chunk->bfields[j], (mi_bfield_t)0);
        #if MI_DEBUG > 1
        const size_t bpopcount = mi_popcount(b);
        size_t rngcount = 0;
        #endif
        size_t bidx;
        while (mi_bfield_find_least_bit(b, &bidx)) {
          size_t rng = mi_ctz(~(b>>bidx)); // all the set bits from bidx
          #if MI_DEBUG > 1
          rngcount += rng;
          #endif
          const size_t idx = base_idx + bidx;
          mi_assert_internal(rng>=1 && rng<=MI_BFIELD_BITS);
          mi_assert_internal((idx % MI_BFIELD_BITS) + rng <= MI_BFIELD_BITS);
          mi_assert_internal((idx / MI_BCHUNK_BITS) < mi_bitmap_chunk_count(bitmap));
          // clear rng bits in b
          b = b & ~mi_bfield_mask(rng, bidx);
          if (!visit(idx, rng, arena, arg)) {
            // break early: reset the non-visited bits
            if (b!=0) {
              mi_atomic_or_relaxed(&chunk->bfields[j], b);
            }
            return false;
          }
        }
        mi_assert_internal(rngcount == bpopcount);
      }
    }
  }
  return true;
}

// Visit all set bits in a bitmap but try to return ranges (within bfields) if possible,
// but only in chunks of at least `rngslices` slices (that are also aligned at `rngslices`)
// and clear those ranges atomically.
// However, the `rngslices` are capped at `MI_BFIELD_BITS` at most.
// Used by purging to purge larger ranges when possible. With transparent huge pages we only
// want to purge whole huge pages (2 MiB) at a time which is what the `rngslices` parameter achieves.
bool _mi_bitmap_forall_setc_rangesn(mi_bitmap_t* bitmap, size_t rngslices, mi_forall_set_fun_t* visit, mi_arena_t* arena, void* arg)
{
  // use the generic routine for `rngslices<=1` (as that one finds longest ranges at a time)
  if (rngslices<=1) {
    return _mi_bitmap_forall_setc_ranges(bitmap, visit, arena, arg);
  }
  // mi_assert_internal(rngslices <= MI_BFIELD_BITS);
  if (rngslices > MI_BFIELD_BITS) { rngslices = MI_BFIELD_BITS;  } // cap at MI_BFIELD_BITS at most

  // for all chunkmap entries
  const size_t chunkmap_max = mi_divide_up(mi_bitmap_chunk_count(bitmap), MI_BFIELD_BITS);
  for (size_t i = 0; i < chunkmap_max; i++) {
    mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bitmap->chunkmap.bfields[i]);
    size_t cmap_idx;
    // for each chunk (corresponding to a set bit in a chunkmap entry)
    while (mi_bfield_foreach_bit(&cmap_entry, &cmap_idx)) {
      const size_t chunk_idx = i*MI_BFIELD_BITS + cmap_idx;
      // for each chunk field
      mi_bchunk_t* const chunk = &bitmap->chunks[chunk_idx];
      for (size_t j = 0; j < MI_BCHUNK_FIELDS; j++) {
        const size_t base_idx = (chunk_idx*MI_BCHUNK_BITS) + (j*MI_BFIELD_BITS);
        mi_bfield_t b = mi_atomic_exchange_relaxed(&chunk->bfields[j], (mi_bfield_t)0);   // atomic clear
        mi_bfield_t skipped = 0;                                                          // but track which bits we skip so we can restore them
        size_t shift;
        for(shift = 0; rngslices + shift <= MI_BFIELD_BITS; shift += rngslices) {  // per `rngslices` to keep alignment
          const mi_bfield_t rngmask = mi_bfield_mask(rngslices, shift);
          if ((b & rngmask) == rngmask) {
            const size_t idx = base_idx + shift;
            if (!visit(idx, rngslices, arena, arg)) {
              // break early: restore non-visited entries
              mi_bfield_t notyet_visited = 0;
              if (rngslices + shift < MI_BFIELD_BITS) {
                notyet_visited = (b & (~(mi_bfield_t)0 << (shift + rngslices)));
              }
              mi_assert_internal((notyet_visited & skipped) == 0);
              if ((notyet_visited | skipped) != 0) {
                mi_atomic_or_relaxed(&chunk->bfields[j], notyet_visited | skipped);
              }
              return false;
            }
          }
          else {
            skipped = skipped | (b & rngmask);
          }
        }
        if (shift < MI_BFIELD_BITS) {
          // there are some non-visited top bits when `MI_BFIELD_BITS % rngslices != 0`.
          mi_assert_internal(MI_BFIELD_BITS % rngslices != 0);
          skipped = skipped | (b & (~(mi_bfield_t)0 << shift));
        }
        if (skipped != 0) {
          //  restore non-visited entries
          mi_atomic_or_relaxed(&chunk->bfields[j], skipped);
        }
      }
    }
  }
  return true;
}


/* --------------------------------------------------------------------------------
  binned bitmap's
-------------------------------------------------------------------------------- */


size_t mi_bbitmap_size(size_t bit_count, size_t* pchunk_count) {
  // mi_assert_internal((bit_count % MI_BCHUNK_BITS) == 0);
  bit_count = mi_align_up(bit_count, MI_BCHUNK_BITS);
  mi_assert_internal(bit_count <= MI_BITMAP_MAX_BIT_COUNT);
  mi_assert_internal(bit_count > 0);
  const size_t chunk_count = bit_count / MI_BCHUNK_BITS;
  mi_assert_internal(chunk_count >= 1);
  const size_t size = offsetof(mi_bbitmap_t,chunks) + (chunk_count * MI_BCHUNK_SIZE);
  mi_assert_internal( (size%MI_BCHUNK_SIZE) == 0 );
  if (pchunk_count != NULL) { *pchunk_count = chunk_count;  }
  return size;
}

// initialize a bitmap to all unset; avoid a mem_zero if `already_zero` is true
// returns the size of the bitmap
size_t mi_bbitmap_init(mi_subproc_t* subproc, mi_bbitmap_t* bbitmap, size_t bit_count, bool already_zero) {
  size_t chunk_count;
  const size_t size = mi_bbitmap_size(bit_count, &chunk_count);
  if (!already_zero) {
    mi_memzero_aligned(bbitmap, size);
  }
  mi_atomic_store_release(&bbitmap->chunk_count, chunk_count);
  mi_assert_internal(mi_atomic_load_relaxed(&bbitmap->chunk_count) <= MI_BITMAP_MAX_CHUNK_COUNT);
  bbitmap->subproc = subproc;
  return size;
}

void mi_bbitmap_unsafe_setN(mi_bbitmap_t* bbitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  mi_assert_internal(idx + n <= mi_bbitmap_max_bits(bbitmap));
  mi_bchunks_unsafe_setN(&bbitmap->chunks[0], &bbitmap->chunkmap, idx, n);
}

bool mi_bbitmap_bsr_inv(mi_bbitmap_t* bbitmap, size_t* idx) {
  // scan for highest zero bit in the bitmap
  // note: we cannot use the chunkmap since that only conservatively denotes if there might be a set bit in a chuck
  // todo: bbitmap_init rounds up the bitcount to BCHUNK_BITS and we should skip the top-padding!
  const size_t chunk_count = mi_bbitmap_chunk_count(bbitmap);
  for(size_t i = chunk_count; i > 0; ) {
    i--;
    size_t cidx;
    if (mi_bchunk_bsr_inv(&bbitmap->chunks[i], &cidx)) {
      *idx = (i * MI_BCHUNK_BITS) + cidx;
      return true;
    }
  }
  return false;
}


/* --------------------------------------------------------------------------------
 binned bitmap used to track free slices
-------------------------------------------------------------------------------- */

// Assign a specific size bin to a chunk
static void mi_bbitmap_set_chunk_bin(mi_bbitmap_t* bbitmap, size_t chunk_idx, mi_chunkbin_t bin) {
  mi_assert_internal(chunk_idx < mi_bbitmap_chunk_count(bbitmap));
  for (mi_chunkbin_t ibin = MI_CBIN_SMALL; ibin < MI_CBIN_NONE; ibin = mi_chunkbin_inc(ibin)) {
    if (ibin == bin) {
      const bool was_clear = mi_bchunk_set(& bbitmap->chunkmap_bins[ibin], chunk_idx, NULL);
      if (was_clear) { mi_subproc_stat_increase(bbitmap->subproc, chunk_bins[ibin],1); }
    }
    else {
      const bool was_set = mi_bchunk_clear(&bbitmap->chunkmap_bins[ibin], chunk_idx, NULL);
      if (was_set) { mi_subproc_stat_decrease(bbitmap->subproc,chunk_bins[ibin],1); }
    }
  }
}

mi_chunkbin_t mi_bbitmap_debug_get_bin(const mi_bchunkmap_t* chunkmap_bins, size_t chunk_idx) {
  for (mi_chunkbin_t ibin = MI_CBIN_SMALL; ibin < MI_CBIN_NONE; ibin = mi_chunkbin_inc(ibin)) {
    if (mi_bchunk_is_xsetN(MI_BIT_SET, &chunkmap_bins[ibin], chunk_idx, 1)) {
      return ibin;
    }
  }
  return MI_CBIN_NONE;
}

// Track the index of the highest chunk that is accessed.
static void mi_bbitmap_chunkmap_set_max(mi_bbitmap_t* bbitmap, size_t chunk_idx) {
  size_t oldmax = mi_atomic_load_relaxed(&bbitmap->chunk_max_accessed);
  if mi_unlikely(chunk_idx > oldmax) {
    mi_atomic_cas_strong_relaxed(&bbitmap->chunk_max_accessed, &oldmax, chunk_idx);
  }
}

// Set a bit in the chunkmap
static void mi_bbitmap_chunkmap_set(mi_bbitmap_t* bbitmap, size_t chunk_idx, bool check_all_set) {
  mi_assert(chunk_idx < mi_bbitmap_chunk_count(bbitmap));
  if (check_all_set) {
    if (mi_bchunk_all_are_set_relaxed(&bbitmap->chunks[chunk_idx])) {
      // all slices are free in this chunk: return back to the NONE bin
      mi_bbitmap_set_chunk_bin(bbitmap, chunk_idx, MI_CBIN_NONE);
    }
  }
  mi_bchunk_set(&bbitmap->chunkmap, chunk_idx, NULL);
  mi_bbitmap_chunkmap_set_max(bbitmap, chunk_idx);
}

static bool mi_bbitmap_chunkmap_try_clear(mi_bbitmap_t* bbitmap, size_t chunk_idx) {
  mi_assert(chunk_idx < mi_bbitmap_chunk_count(bbitmap));
  // check if the corresponding chunk is all clear
  if (!mi_bchunk_all_are_clear_relaxed(&bbitmap->chunks[chunk_idx])) return false;
  // clear the chunkmap bit
  mi_bchunk_clear(&bbitmap->chunkmap, chunk_idx, NULL);
  // .. but a concurrent set may have happened in between our all-clear test and the clearing of the
  // bit in the mask. We check again to catch this situation. (note: mi_bchunk_clear must be acq-rel)
  if (!mi_bchunk_all_are_clear_relaxed(&bbitmap->chunks[chunk_idx])) {
    mi_bchunk_set(&bbitmap->chunkmap, chunk_idx, NULL);
    return false;
  }
  mi_bbitmap_chunkmap_set_max(bbitmap, chunk_idx);
  return true;
}


/* --------------------------------------------------------------------------------
  mi_bbitmap_setN, try_clearN, and is_xsetN
  (used to find free pages)
-------------------------------------------------------------------------------- */

// Set a sequence of `n` bits in the bitmap; returns `true` if atomically transitioned from 0's to 1's (or 1's to 0's).
bool mi_bbitmap_setN(mi_bbitmap_t* bbitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bbitmap_max_bits(bbitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return false;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  bool were_allclear = true;
  while (n > 0) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    were_allclear = mi_bchunk_setN(&bbitmap->chunks[chunk_idx], cidx, m, NULL) && were_allclear;
    mi_bbitmap_chunkmap_set(bbitmap, chunk_idx, true); // set afterwards
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  return were_allclear;
}

// ------- mi_bbitmap_try_clearNC ---------------------------------------

// Try to clear `n` bits at `idx` where `n <= MI_BCHUNK_BITS`.
bool mi_bbitmap_try_clearNC(mi_bbitmap_t* bbitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  mi_assert_internal(n<=MI_BCHUNK_BITS);
  mi_assert_internal(idx + n <= mi_bbitmap_max_bits(bbitmap));

  const size_t chunk_idx = idx / MI_BCHUNK_BITS;
  const size_t cidx = idx % MI_BCHUNK_BITS;
  mi_assert_internal(cidx + n <= MI_BCHUNK_BITS);  // don't cross chunks (for now)
  mi_assert_internal(chunk_idx < mi_bbitmap_chunk_count(bbitmap));
  if (cidx + n > MI_BCHUNK_BITS) return false;
  bool maybe_all_clear = false;
  bool did_temp_clear_bits = false;
  const bool cleared = mi_bchunk_try_clearN(&bbitmap->chunks[chunk_idx], cidx, n, &maybe_all_clear, &did_temp_clear_bits);
  if (cleared && maybe_all_clear) {
    mi_assert_internal(!did_temp_clear_bits);
    mi_bbitmap_chunkmap_try_clear(bbitmap, chunk_idx);
  } else if (did_temp_clear_bits) {
    // may have raced with a clearer (in on_find) so set the chunkmap bit conservatively
    mi_bbitmap_chunkmap_set(bbitmap, chunk_idx, false);
  }
  // note: we don't set the size class for an explicit try_clearN (only used by purging)
  return cleared;
}



// ------- mi_bbitmap_is_xset ---------------------------------------

// Is a sequence of n bits already all set/cleared?
bool mi_bbitmap_is_xsetN(mi_xset_t set, mi_bbitmap_t* bbitmap, size_t idx, size_t n) {
  mi_assert_internal(n>0);
  const size_t maxbits = mi_bbitmap_max_bits(bbitmap);
  mi_assert_internal(idx + n <= maxbits);
  if (idx+n > maxbits) { // paranoia
    if (idx >= maxbits) return false;
    n = maxbits - idx;
  }

  // iterate through the chunks
  size_t chunk_idx = idx / MI_BCHUNK_BITS;
  size_t cidx = idx % MI_BCHUNK_BITS;
  bool xset = true;
  while (n > 0 && xset) {
    const size_t m = (cidx + n > MI_BCHUNK_BITS ? MI_BCHUNK_BITS - cidx : n);
    xset = mi_bchunk_is_xsetN(set, &bbitmap->chunks[chunk_idx], cidx, m) && xset;
    mi_assert_internal(m <= n);
    n -= m;
    cidx = 0;
    chunk_idx++;
  }
  return xset;
}




/* --------------------------------------------------------------------------------
  mi_bbitmap_find
  (used to find free pages)
-------------------------------------------------------------------------------- */

typedef bool (mi_bchunk_try_find_and_clear_fun_t)(mi_bchunk_t* chunk, size_t n, size_t* idx, bool* did_temp_clear_bits);

// Go through the bbitmap and for every sequence of `n` set bits, call the visitor function.
// If it returns `true` stop the search.
//
// This is used for finding free blocks and it is important to be efficient (with 2-level bitscan)
// but also reduce fragmentation (through size bins).
static bool mi_bbitmap_try_find_and_clear_generic(mi_bbitmap_t* bbitmap, size_t tseq, size_t n, size_t* pidx, mi_bchunk_try_find_and_clear_fun_t* on_find)
{
  // we space out threads to reduce contention
  const size_t cmap_max_count  = mi_divide_up(mi_bbitmap_chunk_count(bbitmap),MI_BFIELD_BITS);
  const size_t chunk_acc       = mi_atomic_load_relaxed(&bbitmap->chunk_max_accessed);
  const size_t cmap_acc        = chunk_acc / MI_BFIELD_BITS;
  const size_t cmap_acc_bits   = 1 + (chunk_acc % MI_BFIELD_BITS);

  // create a mask over the chunkmap entries to iterate over them efficiently
  mi_assert_internal(MI_BFIELD_BITS >= MI_BCHUNK_FIELDS);
  const mi_bfield_t cmap_mask  = mi_bfield_mask(cmap_max_count,0);
  const size_t cmap_cycle      = cmap_acc+1;
  const mi_chunkbin_t bbin = mi_chunkbin_of(n);
  // visit each cmap entry
  size_t cmap_idx = 0;
  mi_bfield_cycle_iterate(cmap_mask, tseq, cmap_cycle, cmap_idx, X)
  {
    // and for each chunkmap entry we iterate over its bits to find the chunks
    const mi_bfield_t cmap_entry = mi_atomic_load_relaxed(&bbitmap->chunkmap.bfields[cmap_idx]);
    const size_t cmap_entry_cycle = (cmap_idx != cmap_acc ? MI_BFIELD_BITS : cmap_acc_bits);
    if (cmap_entry == 0) {
      continue;
    }

    // get size bin masks
    mi_bfield_t cmap_bins[MI_CBIN_COUNT] = { 0 };
    cmap_bins[MI_CBIN_NONE] = cmap_entry;
    for (mi_chunkbin_t ibin = MI_CBIN_SMALL; ibin < MI_CBIN_NONE; ibin = mi_chunkbin_inc(ibin)) {
      const mi_bfield_t cmap_bin = mi_atomic_load_relaxed(&bbitmap->chunkmap_bins[ibin].bfields[cmap_idx]);
      cmap_bins[ibin] = cmap_bin & cmap_entry;
      cmap_bins[MI_CBIN_NONE] &= ~cmap_bin;      // clear bits that are in an assigned size bin
    }

    // consider only chunks for a particular size bin at a time
    // this picks the best bin only within a cmap entry (~ 1GiB address space), but avoids multiple
    // iterations through all entries.
    mi_assert_internal(bbin < MI_CBIN_NONE);
    for (mi_chunkbin_t ibin = MI_CBIN_SMALL; ibin <= MI_CBIN_NONE;
          // skip from bbin to NONE (so, say, a SMALL will never be placed in a OTHER, MEDIUM, or LARGE chunk to reduce fragmentation)
          ibin = (ibin == bbin ? MI_CBIN_NONE : mi_chunkbin_inc(ibin)))
    {
      mi_assert_internal(ibin < MI_CBIN_COUNT);
      const mi_bfield_t cmap_bin = cmap_bins[ibin];
      size_t eidx = 0;
      mi_bfield_cycle_iterate(cmap_bin, tseq, cmap_entry_cycle, eidx, Y)
      {
        // assertion doesn't quite hold as the max_accessed may be out-of-date
        // mi_assert_internal(cmap_entry_cycle > eidx || ibin == MI_CBIN_NONE);

        // get the chunk
        const size_t chunk_idx = cmap_idx*MI_BFIELD_BITS + eidx;
        mi_bchunk_t* chunk = &bbitmap->chunks[chunk_idx];

        size_t cidx;
        bool did_temp_clear_bits = false;
        const bool found = (*on_find)(chunk, n, &cidx, &did_temp_clear_bits);
        if (did_temp_clear_bits) {
          // a concurrent find_and_claim may have cleared the chunkmap bit, restore it now
          mi_bbitmap_chunkmap_set(bbitmap, chunk_idx, false);
        }
        if (found) {
          if (cidx==0 && ibin == MI_CBIN_NONE) { // only the first block determines the size bin
            // this chunk is now reserved for the `bbin` size class
            mi_bbitmap_set_chunk_bin(bbitmap, chunk_idx, bbin);
          }
          *pidx = (chunk_idx * MI_BCHUNK_BITS) + cidx;
          mi_assert_internal(*pidx + n <= mi_bbitmap_max_bits(bbitmap));
          return true;
        }
        else {
          // note: we may find that all are cleared only on a second iteration (when we fail to clear any bits)
          //       but that is ok as the chunkmap is a conservative approximation.
          // todo: should _on_find_ return a boolean if there is a chance all are clear to avoid calling `try_clear?`
          //       probably not as we already only call `try_clear` once we fail to clear any bits.
          if (!did_temp_clear_bits) {
            mi_bbitmap_chunkmap_try_clear(bbitmap, chunk_idx);
          }
        }
      }
      mi_bfield_cycle_iterate_end(Y);
    }
  }
  mi_bfield_cycle_iterate_end(X);
  return false;
}

/* --------------------------------------------------------------------------------
  mi_bbitmap_try_find_and_clear -- used to find free pages
  note: the compiler can inline the indirect function calls in combined-source builds
-------------------------------------------------------------------------------- */

bool mi_bbitmap_try_find_and_clear(mi_bbitmap_t* bbitmap, size_t tseq, size_t* pidx) {
  return mi_bbitmap_try_find_and_clear_generic(bbitmap, tseq, 1, pidx, &mi_bchunk_try_find_and_clear_1);
}

bool mi_bbitmap_try_find_and_clear8(mi_bbitmap_t* bbitmap, size_t tseq, size_t* pidx) {
  return mi_bbitmap_try_find_and_clear_generic(bbitmap, tseq, 8, pidx, &mi_bchunk_try_find_and_clear_8);
}

// bool mi_bbitmap_try_find_and_clearX(mi_bbitmap_t* bbitmap, size_t tseq, size_t* pidx) {
//   return mi_bbitmap_try_find_and_clear_generic(bbitmap, tseq, MI_BFIELD_BITS, pidx, &mi_bchunk_try_find_and_clear_X);
// }

bool mi_bbitmap_try_find_and_clearNX(mi_bbitmap_t* bbitmap, size_t tseq, size_t n, size_t* pidx) {
  mi_assert_internal(n<=MI_BFIELD_BITS);
  return mi_bbitmap_try_find_and_clear_generic(bbitmap, tseq, n, pidx, &mi_bchunk_try_find_and_clearNX);
}

bool mi_bbitmap_try_find_and_clearNC(mi_bbitmap_t* bbitmap, size_t tseq, size_t n, size_t* pidx) {
  mi_assert_internal(n<=MI_BCHUNK_BITS);
  return mi_bbitmap_try_find_and_clear_generic(bbitmap, tseq, n, pidx, &mi_bchunk_try_find_and_clearNC);
}


/* --------------------------------------------------------------------------------
  mi_bbitmap_try_find_and_clear for huge objects spanning multiple chunks
-------------------------------------------------------------------------------- */

// Try to atomically clear `n` bits starting at `chunk_idx` where `n` can span over multiple chunks
static bool mi_bbitmap_try_clearN_(mi_bbitmap_t* bbitmap, size_t chunk_idx, size_t n) {
  mi_assert_internal((chunk_idx * MI_BCHUNK_BITS) + n <= mi_bbitmap_max_bits(bbitmap));
  size_t m = n;      // bits to go
  size_t count = 0;  // chunk count
  while (m > 0) {
    mi_bchunk_t* chunk = &bbitmap->chunks[chunk_idx + count];
    bool did_temp_clear_bits = false;
    if (!mi_bchunk_try_clearN(chunk, 0, (m > MI_BCHUNK_BITS ? MI_BCHUNK_BITS : m), NULL, &did_temp_clear_bits)) {
      if (did_temp_clear_bits) {
        // may have raced with a clearer (in on_find) so set the chunkmap bit conservatively
        mi_bbitmap_chunkmap_set(bbitmap, chunk_idx + count, false);
      }
      goto rollback;
    }
    m = (m <= MI_BCHUNK_BITS ? 0 : m - MI_BCHUNK_BITS);
    count++;
  }
  return true;

rollback:
  // we only need to reset chunks the we just fully cleared
  while (count > 0) {
    count--;
    mi_bchunk_t* chunk = &bbitmap->chunks[chunk_idx + count];
    mi_bchunk_setN(chunk, 0, MI_BCHUNK_BITS, NULL);
    // since we may race with clearing, we need to set the chunkmap conservatively
    mi_bbitmap_chunkmap_set(bbitmap, chunk_idx + count, false);
  }
  return false;
}

// Go through the bbitmap to find a sequence of `n` bits and clear them atomically where `n > MI_ARENA_MAX_CHUNK_OBJ_SIZE`
// Since these are very large object allocations we always search from the start and only consider starting at the start
// of a chunk (for fragmentation and efficiency).
// Todo: for now we try to find full empty chunks to cover `n` but we can allow a partial chunk at the end
// Todo: This scans directly through the chunks -- we might want to consult the cmap as well?
bool mi_bbitmap_try_find_and_clearN_(mi_bbitmap_t* bbitmap, size_t tseq, size_t n, size_t* pidx) {
  MI_UNUSED(tseq);
  mi_assert(n > 0); if (n==0) { return false; }

  const size_t chunk_max = mi_bbitmap_chunk_count(bbitmap);
  const size_t chunk_req = mi_divide_up(n, MI_BCHUNK_BITS);  // minimal number of chunks needed
  if (chunk_max < chunk_req) { return false; }

  // iterate through the chunks
  size_t chunk_idx = 0;
  while (chunk_idx <= chunk_max - chunk_req)
  {
    size_t count = 0;  // chunk count
    do {
      mi_assert_internal(chunk_idx + count < chunk_max);
      mi_bchunk_t* const chunk = &bbitmap->chunks[chunk_idx + count];
      if (!mi_bchunk_all_are_set_relaxed(chunk)) {
        break;
      }
      else {
        count++;
      }
    }
    while (count < chunk_req);

    // did we find a suitable range?
    if (count == chunk_req) {
      // now try to claim it!
      if (mi_bbitmap_try_clearN_(bbitmap, chunk_idx, n)) {
        *pidx = (chunk_idx * MI_BCHUNK_BITS);
        for (size_t i = 0; i < count; i++) {
          mi_bbitmap_set_chunk_bin(bbitmap, chunk_idx + i, MI_CBIN_HUGE);
        }
        mi_assert_internal(*pidx + n <= mi_bbitmap_max_bits(bbitmap));
        return true;
      }
      else {
        // contended: we reset count to retry from the first
        // (we still skip the first chunk to guarantee progress)
        count = 0;
      }
    }

    // keep searching but skip the scanned range
    chunk_idx += count+1;
  }
  return false;
}
