/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc/prim.h"       // _mi_prim_clock_now, mi_process_info_t
#include "mimalloc/prim/prim-tls.h"
#include "mimalloc/alloc/page.h"
#include "mimalloc/util/stats.h"

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#pragma warning(disable:4204)  // non-constant aggregate initializer
#endif

/* -----------------------------------------------------------
  Statistics operations
----------------------------------------------------------- */

static void mi_stat_update_mt(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  // add atomically
  int64_t current = mi_atomic_volatile_addi64_relaxed(&stat->current, amount);
  mi_atomic_volatile_maxi64_relaxed(&stat->peak, current + amount);
  if (amount > 0) {
    mi_atomic_volatile_addi64_relaxed(&stat->total, amount);
  }
}

static void mi_stat_update(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  // add thread local
  stat->current += amount;
  if (stat->current > stat->peak) { stat->peak = stat->current; }
  if (amount > 0) { stat->total += amount; }
}

void __mi_stat_increase_mt(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_update_mt(stat, (int64_t)amount);
}
void __mi_stat_increase(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_update(stat, (int64_t)amount);
}

void __mi_stat_decrease_mt(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_update_mt(stat, -((int64_t)amount));
}
void __mi_stat_decrease(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_update(stat, -((int64_t)amount));
}


// Adjust stats to compensate; for example before committing a range,
// first adjust downwards with parts that were already committed so
// we avoid double counting.
static void mi_stat_adjust_mt(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  // adjust atomically
  const int64_t peak = mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&stat->peak);
  mi_atomic_volatile_addi64_relaxed(&stat->current, amount);
  const int64_t prev_total = mi_atomic_volatile_addi64_relaxed(&stat->total, amount);
  if (prev_total == peak) { mi_atomic_volatile_addi64_relaxed(&stat->peak, amount); }
}

static void mi_stat_adjust(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  stat->current += amount;
  if (stat->total==stat->peak) { stat->peak += amount; }
  stat->total += amount;
}

void __mi_stat_adjust_increase_mt(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_adjust_mt(stat, (int64_t)amount);
}
void __mi_stat_adjust_increase(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_adjust(stat, (int64_t)amount);
}
void __mi_stat_adjust_decrease_mt(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_adjust_mt(stat, -((int64_t)amount));
}
void __mi_stat_adjust_decrease(mi_stat_count_t* stat, uint64_t amount) {
  mi_assert_internal(amount<=INT64_MAX);
  mi_stat_adjust(stat, -((int64_t)amount));
}


// must be thread safe as it is called from stats_merge
static void mi_stat_count_add_mt(mi_stat_count_t* stat, const mi_stat_count_t* src) {
  if (stat==src) return;
  mi_atomic_volatile_void_addi64_relaxed(&stat->total, &src->total);
  const int64_t src_peak = mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&src->peak);
  const int64_t src_current = mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&src->current);
  // avoid atomic operations for common cases
  if (src_current == 0 && src_peak == 0) return;
  const int64_t prev_current = (src_current == 0 ? mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&stat->current)
                                                 : mi_atomic_volatile_addi64_relaxed(&stat->current, src_current));

  // Global current plus thread peak approximates new global peak
  // note: peak scores do not really work across threads.
  // we used to just add them together but that often overestimates in practice.
  // similarly, max does not seem to work well. The current approach
  // by Artem Kharytoniuk (@artem-lunarg) seems to work better, see PR#1112
  // for a longer description.
  mi_atomic_volatile_maxi64_relaxed(&stat->peak, prev_current + src_peak);
}

static void mi_stat_counter_add_mt(mi_stat_counter_t* stat, const mi_stat_counter_t* src) {
  if (stat==src) return;
  mi_atomic_volatile_void_addi64_relaxed(&stat->total, &src->total);
}

#define MI_STAT_COUNT(stat)    mi_stat_count_add_mt(&stats->stat, &src->stat);
#define MI_STAT_COUNTER(stat)  mi_stat_counter_add_mt(&stats->stat, &src->stat);

// must be thread safe as it is called from stats_merge
static void mi_stats_add(mi_stats_t* stats, const mi_stats_t* src) {
  if (stats==NULL || src==NULL || stats==src) return;

  // copy all fields
  MI_STAT_FIELDS()

  #if MI_STATS
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_stat_count_add_mt(&stats->malloc_bins[i], &src->malloc_bins[i]);
  }
  #endif
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_stat_count_add_mt(&stats->page_bins[i], &src->page_bins[i]);
  }
  for (size_t i = 0; i < MI_CBIN_COUNT; i++) {
    mi_stat_count_add_mt(&stats->chunk_bins[i], &src->chunk_bins[i]);
  }
}

#undef MI_STAT_COUNT
#undef MI_STAT_COUNTER

/* -----------------------------------------------------------
  Display statistics
----------------------------------------------------------- */

static void mi_print_number(int64_t n, mi_output_fun* out, void* arg, bool limitwidth, bool as_bytes, int64_t base) {
  char buf[32]; _mi_memzero_var(buf);
  int  len = 32;
  const int64_t pos = (n < 0 ? -n : n);
  if (pos < base) {
    if (!(n==1 && as_bytes)) {  // skip printing 1 B for the unit column
      _mi_snprintf(buf, len, "%lld   %-3s", (long long)n, "");
    }
  }
  else {
    int64_t divider = base;
    const char* magnitude = "K";
    if (pos >= divider*base) { divider *= base; magnitude = "M"; }
    if (pos >= divider*base) { divider *= base; magnitude = "G"; }
    const int64_t tens = (n / (divider/10));
    const long whole = (long)(tens/10);
    const long frac1 = (long)(tens%10);
    char unitdesc[8];
    _mi_snprintf(unitdesc, 8, "%s%s%s", magnitude, (base==1024 ? "i" : ""), (as_bytes ? "B" : ""));
    _mi_snprintf(buf, len, "%ld.%ld %-3s", whole, (frac1 < 0 ? -frac1 : frac1), unitdesc);
  }
  if (limitwidth) {
    _mi_fprintf(out, arg, "%12s", buf);
  }
  else {
    _mi_fprintf(out, arg, "%s", buf);
  }
}

static void mi_print_size_narrow(int64_t n, mi_output_fun* out, void* arg) {
  mi_print_number(n, out, arg, false, true, 1024);
}
static void mi_print_size(int64_t n, mi_output_fun* out, void* arg) {
  mi_print_number(n, out, arg, true, true, 1024);
}
static void mi_print_decimal(int64_t n, mi_output_fun* out, void* arg) {
  mi_print_number(n, out, arg, true, false, 1000);
}
// static void mi_print_binary(int64_t n, mi_output_fun* out, void* arg) {
//   mi_print_number(n, out, arg, true, false, 1024);
// }

static void mi_print_label(const char* label, mi_output_fun* out, void* arg) {
  _mi_fprintf(out, arg, "  %-12s:", label);
}

static void mi_print_newline(mi_output_fun* out, void* arg) {
  _mi_fprintf(out, arg, "\n");
}

static void mi_print_stat_decimal(const mi_stat_count_t* stat, const char* msg, mi_output_fun* out, void* arg) {
  mi_print_label(msg, out, arg);
  mi_print_decimal(stat->peak, out, arg);
  mi_print_decimal(stat->total, out, arg);
  mi_print_decimal(stat->current, out, arg);
  mi_print_newline(out, arg);
}

static void mi_print_stat_size(const mi_stat_count_t* stat, const char* msg, mi_output_fun* out, void* arg) {
  mi_print_label(msg, out, arg);
  mi_print_size(stat->peak, out, arg);
  mi_print_size(stat->total, out, arg);
  mi_print_size(stat->current, out, arg);
  mi_print_newline(out, arg);
}

static void mi_print_stat_counter(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg ) {
  mi_print_label(msg, out, arg);
  mi_print_decimal(stat->total, out, arg);
  mi_print_newline(out, arg);
}

static void mi_print_stat_counter_size(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg ) {
  mi_print_label(msg, out, arg);
  mi_print_size(stat->total, out, arg);
  mi_print_newline(out, arg);
}

static void mi_print_average(int64_t count, int64_t total, const char* msg, mi_output_fun* out, void* arg) {
  const int64_t avg_tens = (count == 0 ? 0 : (total*10 / count));
  const int64_t avg_whole = avg_tens/10;
  const int64_t avg_frac1 = avg_tens%10;
  mi_print_label(msg, out, arg);
  _mi_fprintf(out, arg, " %5lld.%lld avg\n", avg_whole, avg_frac1);
}

static void mi_print_header(const char* name, mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, " %-13s %11s %11s %11s\n", name, "peak   ", "total   ", "current   ");
}

#if MI_STATS
static void mi_print_empty(mi_output_fun* out, void* arg) {
  _mi_fprintf(out, arg, "%12s", " ");
}

static void mi_print_stat_total_size(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg) {
  mi_print_label(msg, out, arg);
  mi_print_empty(out, arg);  // no peak
  mi_print_size(stat->total, out, arg);
  mi_print_newline(out, arg);
}

static void mi_print_check(const mi_stat_count_t* stat, mi_output_fun* out, void* arg, const char* not_ok) {
  _mi_fprintf(out, arg, "  %s\n", (stat->current==0 ? "ok" : (not_ok == NULL ? "not all freed" : not_ok)));
}

static void mi_print_stat_bin(const mi_stat_count_t* stat, const char* msg, int64_t blocksize, int64_t total, mi_output_fun* out, void* arg) {
  mi_print_label(msg, out, arg);
  mi_print_size(stat->peak * blocksize, out, arg);
  mi_print_size(stat->total * blocksize, out, arg);
  mi_print_size(stat->current * blocksize, out, arg);
  mi_print_size(blocksize, out, arg);
  mi_print_decimal(total, out, arg);
  mi_print_check(stat, out, arg, NULL);
}

static void mi_print_header_bins(const char* name, mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, " %-13s %11s %11s %11s %11s %11s\n",
                        name, "peak   ", "total   ", "current   ", "block   ", "total#   ");
}

static bool mi_stats_print_bins(const mi_stat_count_t* bins, const mi_stat_count_t* stat_huge, size_t max, mi_output_fun* out, void* arg) {
  bool found = false;
  char buf[64];
  for (size_t i = 0; i <= max; i++) {
    if (bins[i].total > 0) {
      found = true;
      const size_t blocksize = _mi_bin_size((uint8_t)i);
      const char* pagekind = (blocksize <= MI_SMALL_MAX_OBJ_SIZE ? "S" :
                               (mi_block_size_is_medium(blocksize) ? "M" :
                                 (blocksize <= MI_LARGE_MAX_OBJ_SIZE ? "L" : "H")));
      _mi_snprintf(buf, 64, "bin%2s  %3lu", pagekind, (long)i);
      if (i!=MI_BIN_HUGE) {
        mi_print_stat_bin(&bins[i], buf, (int64_t)blocksize, bins[i].total, out, arg);
      }
      else {
        mi_print_stat_bin(stat_huge, buf, 1, bins[i].total, out, arg);
      }
    }
  }
  if (found) {
    mi_print_newline(out, arg);
  }
  return found;
}
#endif



//------------------------------------------------------------
// Use an output wrapper for line-buffered output
// (which is nice when using loggers etc.)
//------------------------------------------------------------
typedef struct buffered_s {
  mi_output_fun* out;   // original output function
  void*          arg;   // and state
  char*          buf;   // local buffer of at least size `count+1`
  size_t         used;  // currently used chars `used <= count`
  size_t         count; // total chars available for output
} buffered_t;

static void mi_buffered_flush(buffered_t* buf) {
  buf->buf[buf->used] = 0;
  _mi_fputs(buf->out, buf->arg, NULL, buf->buf);
  buf->used = 0;
}

static void mi_cdecl mi_buffered_out(const char* msg, void* arg) {
  buffered_t* buf = (buffered_t*)arg;
  if (msg==NULL || buf==NULL) return;
  for (const char* src = msg; *src != 0; src++) {
    char c = *src;
    if (buf->used >= buf->count) mi_buffered_flush(buf);
    mi_assert_internal(buf->used < buf->count);
    buf->buf[buf->used++] = c;
    if (c == '\n') mi_buffered_flush(buf);
  }
}

//------------------------------------------------------------
// Print statistics
//------------------------------------------------------------

mi_decl_export void mi_process_info_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept
{
  size_t elapsed;
  size_t user_time;
  size_t sys_time;
  size_t current_rss;
  size_t peak_rss;
  size_t current_commit;
  size_t peak_commit;
  size_t page_faults;
  mi_process_info(&elapsed, &user_time, &sys_time, &current_rss, &peak_rss, &current_commit, &peak_commit, &page_faults);
  _mi_fprintf(out, arg, "  %-12s: %5zu.%03zu s\n", "elapsed", elapsed/1000, elapsed%1000);
  _mi_fprintf(out, arg, "  %-12s: user: %zu.%03zu s, system: %zu.%03zu s, faults: %zu, peak rss: ", "process",
    user_time/1000, user_time%1000, sys_time/1000, sys_time%1000, page_faults);
  mi_print_size_narrow((int64_t)peak_rss, out, arg);
  if (peak_commit > 0) {
    _mi_fprintf(out, arg, ", peak commit: ");
    mi_print_size_narrow((int64_t)peak_commit, out, arg);
  }
  mi_print_newline(out, arg);
}

void _mi_stats_print(const char* name, size_t id, const mi_stats_t* stats, mi_output_fun* out0, void* arg0) mi_attr_noexcept {
  // wrap the output function to be line buffered
  char buf[256]; _mi_memzero_var(buf);
  buffered_t buffer = { out0, arg0, NULL, 0, 255 };
  buffer.buf = buf;
  mi_output_fun* out = &mi_buffered_out;
  void* arg = &buffer;

  // and print using that
  _mi_fprintf(out, arg, "%s %zu\n", name, id);

  if (stats->allocated.total != 0) {
    #if MI_STATS
    mi_print_header_bins("blocks", out, arg);
    mi_stats_print_bins(stats->malloc_bins, &stats->malloc_huge, MI_BIN_HUGE, out, arg);
    #endif
    #if MI_STATS
    mi_print_stat_size(&stats->allocated, "allocated", out, arg);
    mi_print_stat_total_size(&stats->freed, "freed", out, arg);
    #if MI_STATS>=2
    mi_print_stat_total_size(&stats->malloc_requested, "requested", out, arg);
    #else
    mi_print_stat_total_size(&stats->malloc_requested, "requested~", out, arg);
    #endif
    mi_print_newline(out, arg);
    #endif
  }

  if (stats->pages.total != 0) {
    mi_print_header("pages", out, arg);
    mi_print_stat_size(&stats->page_committed, "touched", out, arg);
    // mi_print_stat(&stats->segments, "segments", -1, out, arg);
    // mi_print_stat(&stats->segments_abandoned, "-abandoned", -1, out, arg);
    // mi_print_stat(&stats->segments_cache, "-cached", -1, out, arg);
    mi_print_stat_decimal(&stats->pages, "pages", out, arg);
    mi_print_stat_decimal(&stats->pages_abandoned, "abandoned", out, arg);
    mi_print_stat_decimal(&stats->pages_os_abandoned, "os abandoned", out, arg);
    mi_print_stat_decimal(&stats->pages_os_allocated, "os allocated", out, arg);

    mi_print_stat_counter(&stats->pages_reclaim_on_alloc, "reclaima", out, arg);
    mi_print_stat_counter(&stats->pages_reclaim_on_free, "reclaimf", out, arg);
    mi_print_stat_counter(&stats->pages_reabandon_full, "reabandon", out, arg);
    mi_print_stat_counter(&stats->pages_unabandon_busy_wait, "unabdn retry", out, arg);
    mi_print_stat_counter(&stats->pages_unown_cas_retry, "unown retry", out, arg);
    mi_print_stat_counter(&stats->pages_extended, "extended", out, arg);
    mi_print_stat_counter(&stats->pages_retire, "retire", out, arg);
    mi_print_stat_counter(&stats->pages_stat_updates, "stat updates", out, arg);
    mi_print_average(stats->pages_stat_updates.total, stats->pages_stat_update_count.total, "stat avg", out, arg);
    mi_print_average(stats->page_searches_count.total, stats->page_searches.total, "searches", out, arg);
    mi_print_stat_counter(&stats->profile_samples, "prof samples", out, arg);
    mi_print_newline(out, arg);
  }

  if (stats->arena_count.total > 0) {
    mi_print_header("arenas", out, arg);
    mi_print_stat_size(&stats->reserved, "reserved", out, arg);
    mi_print_stat_size(&stats->committed, "committed", out, arg);
    mi_print_stat_counter_size(&stats->reset, "reset", out, arg);
    mi_print_stat_counter_size(&stats->purged, "purged", out, arg);

    mi_print_stat_counter(&stats->arena_count, "arenas", out, arg);
    mi_print_stat_counter(&stats->arena_rollback_count, "rollback", out, arg);
    mi_print_stat_counter(&stats->mmap_calls, "mmaps", out, arg);
    mi_print_stat_counter(&stats->commit_calls, "commits", out, arg);
    mi_print_stat_counter(&stats->reset_calls, "resets", out, arg);
    mi_print_stat_counter(&stats->purge_calls, "purges", out, arg);
    mi_print_stat_counter(&stats->malloc_guarded_count, "guarded", out, arg);
    mi_print_stat_decimal(&stats->theaps, "theaps", out, arg);
    mi_print_stat_decimal(&stats->heaps, "heaps", out, arg);
    mi_print_stat_counter(&stats->heaps_delete_wait, "heap waits", out, arg);
    mi_print_newline(out, arg);
  }

  mi_print_header("process", out, arg);
  mi_print_stat_decimal(&stats->threads, "threads", out, arg);
  mi_stat_counter_t numa_count = { _mi_os_numa_node_count() };
  mi_print_stat_counter(&numa_count, "numa nodes", out, arg);
  mi_process_info_print_out(out, arg);

  mi_print_newline(out, arg);
}


static mi_msecs_t mi_process_start; // = 0

// called on process init
void _mi_stats_init(void) {
  if (mi_process_start == 0) { mi_process_start = _mi_clock_start(); };
}

static void mi_stats_add_into(mi_stats_t* to, const mi_stats_t* from) {
  mi_assert_internal(to != NULL && from != NULL);
  if (to == from) return;
  mi_stats_add(to, from);
}

void _mi_stats_merge_into(mi_stats_t* to, mi_stats_t* from) {
  mi_assert_internal(to != NULL && from != NULL);
  if (to == from) return;
  mi_stats_add(to, from);
  mi_stats_init(from); // zero field and keep the header
}

static const mi_stats_t* mi_stats_merge_theap_to_heap(mi_theap_t* theap) mi_attr_noexcept {
  mi_stats_t* stats = &theap->stats;
  mi_stats_t* heap_stats = &mi_theap_heap(theap)->stats;
  _mi_stats_merge_into( heap_stats, stats );
  return heap_stats;
}

static const mi_stats_t* mi_heap_get_stats(mi_heap_t* heap) {
  if (heap==NULL) { heap = mi_heap_main(); }
  mi_theap_t* theap = _mi_heap_theap_peek(heap);
  if (theap==NULL) return &heap->stats;
              else return mi_stats_merge_theap_to_heap(theap);
}

static const mi_stats_t* mi_theap_get_stats(mi_theap_t* theap) {
  return &theap->stats;
}

// deprecated
void mi_stats_reset(void) mi_attr_noexcept {
  if (!mi_theap_is_initialized(_mi_theap_default())) return;
  mi_heap_t* heap_main = mi_heap_main();
  mi_heap_get_stats(heap_main);
  mi_heap_stats_merge_to_subproc(heap_main);
}


void mi_heap_stats_print_out(mi_heap_t* heap, mi_output_fun* out, void* arg) mi_attr_noexcept {
  if (heap==NULL) { heap = mi_heap_main(); }
  _mi_stats_print("heap", heap->heap_seq, mi_heap_get_stats(heap), out, arg);
}

typedef struct mi_heap_print_visit_info_s {
  mi_output_fun* out;
  void* out_arg;
} mi_heap_print_visit_info_t;

static bool mi_cdecl mi_heap_print_visitor(mi_heap_t* heap, void* arg) {
  mi_heap_print_visit_info_t* vinfo = (mi_heap_print_visit_info_t*)(arg);
  mi_heap_stats_print_out(heap, vinfo->out, vinfo->out_arg);
  return true;
}


// show each heap and then the subproc
void mi_subproc_heap_stats_print_out(mi_subproc_id_t subproc_id, mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL) return;
  mi_heap_print_visit_info_t vinfo = { out, arg };
  mi_subproc_visit_heaps(subproc_id, &mi_heap_print_visitor, &vinfo);
  if (subproc->theap_meta!=NULL) {
    _mi_stats_print("meta", subproc->subproc_seq, &subproc->theap_meta->stats, out, arg);
  }
  // if (subproc->theap_meta!=NULL) { _mi_stats_merge_into(&subproc->stats, &subproc->theap_meta->stats); }
  _mi_stats_print("subproc", subproc->subproc_seq, &subproc->stats, out, arg);
}


// aggregate all stats from the heaps and subproc and print those
void mi_subproc_stats_print_out(mi_subproc_id_t subproc_id, mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL) return;
  mi_stats_t_decl(stats);
  if (mi_subproc_stats_get(subproc_id, &stats)) {
    _mi_stats_print("subproc", subproc->subproc_seq, &stats, out, arg);
  }
}

void mi_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_subproc_stats_print_out(mi_subproc_current(),out, arg);
}

// deprecated
void mi_stats_print(void* out) mi_attr_noexcept {
  // for compatibility there is an `out` parameter (which can be `stdout` or `stderr`)
  mi_stats_print_out((mi_output_fun*)out, NULL);
}

// deprecated
void mi_thread_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_theap_t* theap = _mi_theap_default();
  if (theap==NULL || !mi_theap_is_initialized(theap)) return;
  _mi_stats_print("heap", mi_theap_heap(theap)->heap_seq, &theap->stats, out, arg);
  mi_stats_merge_theap_to_heap(_mi_theap_default());
}


// ----------------------------------------------------------------
// Basic timer for convenience; use milli-seconds to avoid doubles
// ----------------------------------------------------------------

static mi_msecs_t mi_clock_diff;

mi_msecs_t _mi_clock_now(void) {
  return _mi_prim_clock_now();
}

mi_msecs_t _mi_clock_start(void) {
  if (mi_clock_diff == 0.0) {
    mi_msecs_t t0 = _mi_clock_now();
    mi_clock_diff = _mi_clock_now() - t0;
  }
  return _mi_clock_now();
}

mi_msecs_t _mi_clock_end(mi_msecs_t start) {
  mi_msecs_t end = _mi_clock_now();
  return (end - start - mi_clock_diff);
}


// --------------------------------------------------------
// Basic process statistics
// --------------------------------------------------------

mi_decl_export void mi_process_info(size_t* elapsed_msecs, size_t* user_msecs, size_t* system_msecs, size_t* current_rss, size_t* peak_rss, size_t* current_commit, size_t* peak_commit, size_t* page_faults) mi_attr_noexcept
{
  mi_process_info_t pinfo;
  _mi_memzero_var(pinfo);
  pinfo.elapsed        = _mi_clock_end(mi_process_start);
  { const mi_subproc_t* subproc = _mi_subproc_main();
    if (subproc!=NULL) {
      const int64_t current = mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)(&subproc->stats.committed.current));
      const int64_t peak    = mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)(&subproc->stats.committed.peak));
      pinfo.current_commit  = (current < 0 ? 0 : (current < PTRDIFF_MAX ? (size_t)current : PTRDIFF_MAX));
      pinfo.peak_commit     = (peak < 0 ? 0 : (peak < PTRDIFF_MAX ? (size_t)peak : PTRDIFF_MAX));
    }
  }
  pinfo.current_rss    = pinfo.current_commit;
  pinfo.peak_rss       = pinfo.peak_commit;
  pinfo.utime          = 0;
  pinfo.stime          = 0;
  pinfo.page_faults    = 0;

  _mi_prim_process_info(&pinfo);

  if (elapsed_msecs!=NULL)  *elapsed_msecs  = (pinfo.elapsed < 0 ? 0 : (pinfo.elapsed < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.elapsed : PTRDIFF_MAX));
  if (user_msecs!=NULL)     *user_msecs     = (pinfo.utime < 0 ? 0 : (pinfo.utime < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.utime : PTRDIFF_MAX));
  if (system_msecs!=NULL)   *system_msecs   = (pinfo.stime < 0 ? 0 : (pinfo.stime < (mi_msecs_t)PTRDIFF_MAX ? (size_t)pinfo.stime : PTRDIFF_MAX));
  if (current_rss!=NULL)    *current_rss    = pinfo.current_rss;
  if (peak_rss!=NULL)       *peak_rss       = pinfo.peak_rss;
  if (current_commit!=NULL) *current_commit = pinfo.current_commit;
  if (peak_commit!=NULL)    *peak_commit    = pinfo.peak_commit;
  if (page_faults!=NULL)    *page_faults    = pinfo.page_faults;
}

mi_decl_export void mi_process_info_print(void) mi_attr_noexcept {
  mi_process_info_print_out(NULL, NULL);
}


// --------------------------------------------------------
// Return statistics
// --------------------------------------------------------

size_t mi_stats_get_bin_size(size_t bin) mi_attr_noexcept {
  if (bin > MI_BIN_HUGE) return 0;
  return _mi_bin_size(bin);
}

static bool mi_stats_copy(mi_stats_t* stats_to, const mi_stats_t* stats_from) mi_attr_noexcept {
  if (stats_to == NULL || stats_to->size != sizeof(mi_stats_t) || stats_to->version != MI_STAT_VERSION) return false;
  if (stats_from == NULL || stats_from->size != stats_to->size) return false;
  mi_memcpy(stats_to, stats_from, stats_to->size);
  return true;
}

bool mi_subproc_stats_get_exclusive(mi_subproc_id_t subproc_id, mi_stats_t* stats) mi_attr_noexcept {
  const mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL) return false;
  return mi_stats_copy(stats,&subproc->stats);
}

bool mi_heap_stats_get(mi_heap_t* heap, mi_stats_t* stats) mi_attr_noexcept {
  return mi_stats_copy(stats, mi_heap_get_stats(heap));
}

bool mi_theap_stats_get(mi_theap_t* theap, mi_stats_t* stats) mi_attr_noexcept {
  return mi_stats_copy(stats, mi_theap_get_stats(theap));
}

void mi_theap_stats_merge_to_heap(mi_theap_t* theap) mi_attr_noexcept {
  if (theap == NULL) return;
  _mi_theap_merge_stats(theap);
}

static bool mi_cdecl mi_heap_aggregate_visitor(mi_heap_t* heap, void* arg) {
  mi_stats_t* stats = (mi_stats_t*)arg;
  mi_stats_add_into(stats, mi_heap_get_stats(heap));
  return true;
}

bool mi_subproc_stats_get(mi_subproc_id_t subproc_id, mi_stats_t* stats) mi_attr_noexcept {
  if (stats==NULL) return false;
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc == NULL) return false;
  if (!mi_stats_copy(stats, &subproc->stats)) return false;
  mi_subproc_visit_heaps(subproc_id, &mi_heap_aggregate_visitor, stats);
  // hide meta data stats
  // if (subproc->theap_meta!=NULL) { mi_stats_add_into(stats, &subproc->theap_meta->stats); }
  return true;
}

bool mi_stats_get(mi_stats_t* stats) mi_attr_noexcept {
  return mi_subproc_stats_get(mi_subproc_current(), stats);
}


// --------------------------------------------------------
// Statics in json format
// --------------------------------------------------------

typedef struct mi_json_buf_s {
  char*   buf;
  size_t  size;
  size_t  used;
  bool    can_realloc;
} mi_json_buf_t;

static bool mi_json_buf_expand(mi_json_buf_t* hbuf) {
  if (hbuf==NULL) return false;
  if (hbuf->buf != NULL && hbuf->size>0) {
    hbuf->buf[hbuf->size-1] = 0;
  }
  if (hbuf->size > SIZE_MAX/2 || !hbuf->can_realloc) return false;
  const size_t newsize = (hbuf->size == 0 ? mi_good_size(12*MI_KiB) : 2*hbuf->size);
  char* const  newbuf  = (char*)mi_rezalloc(hbuf->buf, newsize);
  if (newbuf == NULL) return false;
  hbuf->buf = newbuf;
  hbuf->size = newsize;
  return true;
}

static void mi_json_buf_print(mi_json_buf_t* hbuf, const char* msg) {
  if (msg==NULL || hbuf==NULL) return;
  if (hbuf->used + 1 >= hbuf->size && !hbuf->can_realloc) return;
  for (const char* src = msg; *src != 0; src++) {
    char c = *src;
    if (hbuf->used + 1 >= hbuf->size) {
      if (!mi_json_buf_expand(hbuf)) return;
    }
    mi_assert_internal(hbuf->used < hbuf->size);
    hbuf->buf[hbuf->used++] = c;
  }
  mi_assert_internal(hbuf->used < hbuf->size);
  hbuf->buf[hbuf->used] = 0;
}

static void mi_json_buf_print_count_bin(mi_json_buf_t* hbuf, const char* prefix, const mi_stat_count_t* stat, size_t bin, bool add_comma) {
  const size_t binsize = mi_stats_get_bin_size(bin);
  const size_t pagesize = (binsize <= MI_SMALL_MAX_OBJ_SIZE ? MI_SMALL_PAGE_SIZE :
                            (mi_block_size_is_medium(binsize) ? MI_MEDIUM_PAGE_SIZE :
                              (binsize <= MI_LARGE_MAX_OBJ_SIZE ? MI_LARGE_PAGE_SIZE : 0)));
  char buf[128];
  _mi_snprintf(buf, 128, "%s{ \"total\": %lld, \"peak\": %lld, \"current\": %lld, \"block_size\": %zu, \"page_size\": %zu }%s\n", prefix, stat->total, stat->peak, stat->current, binsize, pagesize, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
}

static void mi_json_buf_print_count_cbin(mi_json_buf_t* hbuf, const char* prefix, const mi_stat_count_t* stat, mi_chunkbin_t bin, bool add_comma) {
  const char* cbin = " ";
  switch(bin) {
    case MI_CBIN_SMALL:  cbin = "S"; break;
    case MI_CBIN_MEDIUM: cbin = "M"; break;
    case MI_CBIN_LARGE:  cbin = "L"; break;
    case MI_CBIN_HUGE:   cbin = "H"; break;
    case MI_CBIN_OTHER:  cbin = "X"; break;
    default: cbin = " "; break;
  }
  char buf[128];
  _mi_snprintf(buf, 128, "%s{ \"total\": %lld, \"peak\": %lld, \"current\": %lld, \"bin\": \"%s\" }%s\n", prefix, stat->total, stat->peak, stat->current, cbin, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
}

static void mi_json_buf_print_count(mi_json_buf_t* hbuf, const char* prefix, const mi_stat_count_t* stat, bool add_comma) {
  char buf[128];
  _mi_snprintf(buf, 128, "%s{ \"total\": %lld, \"peak\": %lld, \"current\": %lld }%s\n", prefix, stat->total, stat->peak, stat->current, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
}

static void mi_json_buf_print_count_value(mi_json_buf_t* hbuf, const char* name, const mi_stat_count_t* stat) {
  char buf[128];
  _mi_snprintf(buf, 128, "  \"%s\": ", name);
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
  mi_json_buf_print_count(hbuf, "", stat, true);
}

static void mi_json_buf_print_value(mi_json_buf_t* hbuf, const char* name, int64_t val) {
  char buf[128];
  _mi_snprintf(buf, 128, "  \"%s\": %lld,\n", name, val);
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
}

static void mi_json_buf_print_size(mi_json_buf_t* hbuf, const char* name, size_t val, bool add_comma) {
  char buf[128];
  _mi_snprintf(buf, 128, "    \"%s\": %zu%s\n", name, val, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_json_buf_print(hbuf, buf);
}

static void mi_json_buf_print_counter_value(mi_json_buf_t* hbuf, const char* name, const mi_stat_counter_t* stat) {
  mi_json_buf_print_value(hbuf, name, stat->total);
}

static char* mi_stats_get_json_from(const mi_stats_t* stats, size_t output_size, char* output_buf) mi_attr_noexcept {
  if (stats==NULL || stats->size!=sizeof(mi_stats_t) || stats->version!=MI_STAT_VERSION) return NULL;
  mi_json_buf_t hbuf = { NULL, 0, 0, true };
  if (output_size > 0 && output_buf != NULL) {
    mi_memzero(output_buf, output_size);
    hbuf.buf = output_buf;
    hbuf.size = output_size;
    hbuf.can_realloc = false;
  }
  else {
    if (!mi_json_buf_expand(&hbuf)) return NULL;
  }
  mi_json_buf_print(&hbuf, "{\n");
  mi_json_buf_print_value(&hbuf, "stat_version", MI_STAT_VERSION);
  mi_json_buf_print_value(&hbuf, "mimalloc_version", MI_MALLOC_VERSION);

  // process info
  mi_json_buf_print(&hbuf, "  \"process\": {\n");
  size_t elapsed;
  size_t user_time;
  size_t sys_time;
  size_t current_rss;
  size_t peak_rss;
  size_t current_commit;
  size_t peak_commit;
  size_t page_faults;
  mi_process_info(&elapsed, &user_time, &sys_time, &current_rss, &peak_rss, &current_commit, &peak_commit, &page_faults);
  mi_json_buf_print_size(&hbuf, "elapsed_msecs", elapsed, true);
  mi_json_buf_print_size(&hbuf, "user_msecs", user_time, true);
  mi_json_buf_print_size(&hbuf, "system_msecs", sys_time, true);
  mi_json_buf_print_size(&hbuf, "page_faults", page_faults, true);
  mi_json_buf_print_size(&hbuf, "rss_current", current_rss, true);
  mi_json_buf_print_size(&hbuf, "rss_peak", peak_rss, true);
  mi_json_buf_print_size(&hbuf, "commit_current", current_commit, true);
  mi_json_buf_print_size(&hbuf, "commit_peak", peak_commit, false);
  mi_json_buf_print(&hbuf, "  },\n");

  // statistics
  #define MI_STAT_COUNT(stat)    mi_json_buf_print_count_value(&hbuf, #stat, &stats->stat);
  #define MI_STAT_COUNTER(stat)  mi_json_buf_print_counter_value(&hbuf, #stat, &stats->stat);

  MI_STAT_FIELDS()

  #undef MI_STAT_COUNT
  #undef MI_STAT_COUNTER

  // size bins
  mi_json_buf_print(&hbuf, "  \"malloc_bins\": [\n");
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_json_buf_print_count_bin(&hbuf, "    ", &stats->malloc_bins[i], i, i!=MI_BIN_HUGE);
  }
  mi_json_buf_print(&hbuf, "  ],\n");
  mi_json_buf_print(&hbuf, "  \"page_bins\": [\n");
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_json_buf_print_count_bin(&hbuf, "    ", &stats->page_bins[i], i, i!=MI_BIN_HUGE);
  }
  mi_json_buf_print(&hbuf, "  ],\n");
  mi_json_buf_print(&hbuf, "  \"chunk_bins\": [\n");
  for (size_t i = 0; i < MI_CBIN_COUNT; i++) {
    mi_json_buf_print_count_cbin(&hbuf, "    ", &stats->chunk_bins[i], (mi_chunkbin_t)i, i!=MI_CBIN_COUNT-1);
  }
  mi_json_buf_print(&hbuf, "  ]\n");
  mi_json_buf_print(&hbuf, "}\n");
  if (hbuf.used + 1 >= hbuf.size) {
    // failed
    if (hbuf.can_realloc) { mi_free(hbuf.buf); }
    return NULL;
  }
  else {
    return hbuf.buf;
  }
}

char* mi_subproc_stats_get_json(mi_subproc_id_t subproc_id, size_t buf_size, char* buf) mi_attr_noexcept {
  mi_subproc_t* subproc = _mi_subproc_from_id(subproc_id);
  if (subproc==NULL) return NULL;
  mi_stats_t_decl(stats);
  if (!mi_subproc_stats_get(subproc_id,&stats)) return NULL;
  return mi_stats_get_json_from(&stats, buf_size, buf);
}

char* mi_heap_stats_get_json(mi_heap_t* heap, size_t buf_size, char* buf) mi_attr_noexcept {
  return mi_stats_get_json_from(mi_heap_get_stats(heap), buf_size, buf);
}

char* mi_stats_get_json(size_t buf_size, char* buf) mi_attr_noexcept {
  return mi_subproc_stats_get_json(mi_subproc_current(), buf_size, buf);
}

char* mi_stats_as_json(mi_stats_t* stats, size_t buf_size, char* buf) mi_attr_noexcept {
  return mi_stats_get_json_from(stats, buf_size, buf);
}
