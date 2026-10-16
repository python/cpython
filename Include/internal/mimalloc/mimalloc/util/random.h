/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_RANDOM_H
#define MI_INTERNAL_RANDOM_H

#include "../types.h"

void          _mi_random_init(mi_random_ctx_t* ctx);
void          _mi_random_init_weak(mi_random_ctx_t* ctx);
void          _mi_random_reinit_if_weak(mi_random_ctx_t * ctx);
void          _mi_random_split(mi_random_ctx_t* ctx, mi_random_ctx_t* new_ctx);
size_t        _mi_random_next(mi_random_ctx_t* ctx);
size_t        _mi_os_random_weak(size_t extra_seed);

// -------------------------------------------------------------------
// Fast "random" shuffle
// -------------------------------------------------------------------

static inline size_t mi_random_shuffle(size_t x) {
  if (x==0) { x = 17; }   // ensure we don't get stuck in generating zeros
#if (MI_SIZE_SIZE>=8)
  // by Sebastiano Vigna, see: <http://xoshiro.di.unimi.it/splitmix64.c>
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9UL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebUL;
  x ^= x >> 31;
#elif (MI_SIZE_SIZE==4)
  // by Chris Wellons, see: <https://nullprogram.com/blog/2018/07/31/>
  x ^= x >> 16;
  x *= 0x7feb352dUL;
  x ^= x >> 15;
  x *= 0x846ca68bUL;
  x ^= x >> 16;
#endif
  return x;
}

#endif
