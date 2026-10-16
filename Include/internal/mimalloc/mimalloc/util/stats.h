/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_STATS_H
#define MI_INTERNAL_STATS_H

#include "../types.h"

void          _mi_stats_init(void);
void          _mi_stats_merge_into(mi_stats_t* to, mi_stats_t* from);

mi_msecs_t    _mi_clock_now(void);
mi_msecs_t    _mi_clock_end(mi_msecs_t start);
mi_msecs_t    _mi_clock_start(void);

/* -----------------------------------------------------------
  Statistics (in `util/stats.c`)
----------------------------------------------------------- */

// add to stat keeping track of the peak
void __mi_stat_increase(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_decrease(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_increase_mt(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_decrease_mt(mi_stat_count_t* stat, uint64_t amount);

// adjust stat in special cases to compensate for double counting (and does not adjust peak values and can decrease the total)
void __mi_stat_adjust_increase(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_adjust_decrease(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_adjust_increase_mt(mi_stat_count_t* stat, uint64_t amount);
void __mi_stat_adjust_decrease_mt(mi_stat_count_t* stat, uint64_t amount);

// counters can just be increased
static inline void __mi_stat_counter_increase_mt(mi_stat_counter_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_atomic_volatile_addi64_relaxed(&stat->total, (int64_t)amount);
}

static inline void __mi_stat_counter_increase(mi_stat_counter_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  stat->total += (int64_t)amount;
}

static inline void __mi_stat_counter_decrease(mi_stat_counter_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  stat->total -= (int64_t)amount;
}

#define mi_heap_stat_counter_increase(heap,stat,amount)         __mi_stat_counter_increase_mt( &(heap)->stats.stat, amount)
#define mi_heap_stat_increase(heap,stat,amount)                 __mi_stat_increase_mt( &(heap)->stats.stat, amount)
#define mi_heap_stat_decrease(heap,stat,amount)                 __mi_stat_decrease_mt( &(heap)->stats.stat, amount)
#define mi_heap_stat_adjust_increase(heap,stat,amnt)            __mi_stat_adjust_increase_mt( &(heap)->stats.stat, amnt)
#define mi_heap_stat_adjust_decrease(heap,stat,amnt)            __mi_stat_adjust_decrease_mt( &(heap)->stats.stat, amnt)

#define mi_subproc_stat_counter_increase(subproc,stat,amount)   __mi_stat_counter_increase_mt( &(subproc)->stats.stat, amount)
#define mi_subproc_stat_increase(subproc,stat,amount)           __mi_stat_increase_mt( &(subproc)->stats.stat, amount)
#define mi_subproc_stat_decrease(subproc,stat,amount)           __mi_stat_decrease_mt( &(subproc)->stats.stat, amount)
#define mi_subproc_stat_adjust_increase(subproc,stat,amount)    __mi_stat_adjust_increase_mt( &(subproc)->stats.stat, amount)
#define mi_subproc_stat_adjust_decrease(subproc,stat,amount)    __mi_stat_adjust_decrease_mt( &(subproc)->stats.stat, amount)

#define mi_theap_stat_counter_increase(theap,stat,amount)       __mi_stat_counter_increase( &(theap)->stats.stat, amount)
#define mi_theap_stat_counter_decrease(theap,stat,amount)       __mi_stat_counter_decrease( &(theap)->stats.stat, amount)
#define mi_theap_stat_increase(theap,stat,amount)               __mi_stat_increase( &(theap)->stats.stat, amount)
#define mi_theap_stat_decrease(theap,stat,amount)               __mi_stat_decrease( &(theap)->stats.stat, amount)
#define mi_theap_stat_adjust_increase(theap,stat,amnt)          __mi_stat_adjust_increase( &(theap)->stats.stat, amnt)
#define mi_theap_stat_adjust_decrease(theap,stat,amnt)          __mi_stat_adjust_decrease( &(theap)->stats.stat, amnt)

#define mi_theapx_stat_counter_increase(heap,theap,stat,amount) if (theap!=NULL) { mi_theap_stat_counter_increase(theap,stat,amount); } else { mi_heap_stat_counter_increase(heap,stat,amount); }
#define mi_theapx_stat_adjust_decrease(heap,theap,stat,amount)  if (theap!=NULL) { mi_theap_stat_adjust_decrease(theap,stat,amount); } else { mi_heap_stat_adjust_decrease(heap,stat,amount); }
#define mi_theapx_stat_increase(heap,theap,stat,amount)         if (theap!=NULL) { mi_theap_stat_increase(theap,stat,amount); } else { mi_heap_stat_increase(heap,stat,amount); }
#define mi_theapx_stat_decrease(heap,theap,stat,amount)         if (theap!=NULL) { mi_theap_stat_decrease(theap,stat,amount); } else { mi_heap_stat_decrease(heap,stat,amount); }

#endif
