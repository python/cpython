/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc/internal.h"
#include "mimalloc/atomic.h"
#include "mimalloc/prim.h"

#include <string.h> // memset

#if defined(_MSC_VER) && (_MSC_VER < 1920)
#pragma warning(disable:4204)  // non-constant aggregate initializer
#endif

/* -----------------------------------------------------------
  Statistics operations
----------------------------------------------------------- */

static bool mi_is_in_main(void* stat) {
  return ((uint8_t*)stat >= (uint8_t*)&_mi_stats_main
         && (uint8_t*)stat < ((uint8_t*)&_mi_stats_main + sizeof(mi_stats_t)));
}

static void mi_stat_update(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  if mi_unlikely(mi_is_in_main(stat))
  {
    // add atomically (for abandoned pages)
    int64_t current = mi_atomic_addi64_relaxed(&stat->current, amount);
    // if (stat == &_mi_stats_main.committed) { mi_assert_internal(current + amount >= 0); };
    mi_atomic_maxi64_relaxed(&stat->peak, current + amount);
    if (amount > 0) {
      mi_atomic_addi64_relaxed(&stat->total,amount);
    }
  }
  else {
    // add thread local
    stat->current += amount;
    if (stat->current > stat->peak) { stat->peak = stat->current; }
    if (amount > 0) { stat->total += amount; }
  }
}

void _mi_stat_counter_increase(mi_stat_counter_t* stat, size_t amount) {
  if (mi_is_in_main(stat)) {
    mi_atomic_addi64_relaxed( &stat->total, (int64_t)amount );
  }
  else {
    stat->total += amount;
  }
}

void _mi_stat_increase(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update(stat, (int64_t)amount);
}

void _mi_stat_decrease(mi_stat_count_t* stat, size_t amount) {
  mi_stat_update(stat, -((int64_t)amount));
}


static void mi_stat_adjust(mi_stat_count_t* stat, int64_t amount) {
  if (amount == 0) return;
  if mi_unlikely(mi_is_in_main(stat))
  {
    // adjust atomically
    mi_atomic_addi64_relaxed(&stat->current, amount);
    mi_atomic_addi64_relaxed(&stat->total,amount);
  }
  else {
    // adjust local
    stat->current += amount;
    stat->total += amount;
  }
}

void _mi_stat_adjust_decrease(mi_stat_count_t* stat, size_t amount) {
  mi_stat_adjust(stat, -((int64_t)amount));
}


// must be thread safe as it is called from stats_merge
static void mi_stat_count_add_mt(mi_stat_count_t* stat, const mi_stat_count_t* src) {
  if (stat==src) return;
  mi_atomic_void_addi64_relaxed(&stat->total, &src->total);
  const int64_t prev_current = mi_atomic_addi64_relaxed(&stat->current, src->current);

  // Global current plus thread peak approximates new global peak
  // note: peak scores do really not work across threads.
  // we used to just add them together but that often overestimates in practice.
  // similarly, max does not seem to work well. The current approach
  // by Artem Kharytoniuk (@artem-lunarg) seems to work better, see PR#1112
  // for a longer description.
  mi_atomic_maxi64_relaxed(&stat->peak, prev_current + src->peak);
}

static void mi_stat_counter_add_mt(mi_stat_counter_t* stat, const mi_stat_counter_t* src) {
  if (stat==src) return;
  mi_atomic_void_addi64_relaxed(&stat->total, &src->total);
}

#define MI_STAT_COUNT(stat)    mi_stat_count_add_mt(&stats->stat, &src->stat);
#define MI_STAT_COUNTER(stat)  mi_stat_counter_add_mt(&stats->stat, &src->stat);

// must be thread safe as it is called from stats_merge
static void mi_stats_add(mi_stats_t* stats, const mi_stats_t* src) {
  if (stats==src) return;

  // copy all fields
  MI_STAT_FIELDS()

  #if MI_STAT>1
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_stat_count_add_mt(&stats->malloc_bins[i], &src->malloc_bins[i]);
  }
  #endif
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_stat_count_add_mt(&stats->page_bins[i], &src->page_bins[i]);
  }
}

#undef MI_STAT_COUNT
#undef MI_STAT_COUNTER

/* -----------------------------------------------------------
  Display statistics
----------------------------------------------------------- */

// unit > 0 : size in binary bytes
// unit == 0: count as decimal
// unit < 0 : count in binary
static void mi_printf_amount(int64_t n, int64_t unit, mi_output_fun* out, void* arg, bool limitwidth) {
  char buf[32]; buf[0] = 0;
  int  len = 32;
  const char* suffix = (unit <= 0 ? " " : "B");
  const int64_t base = (unit == 0 ? 1000 : 1024);
  if (unit>0) n *= unit;

  const int64_t pos = (n < 0 ? -n : n);
  if (pos < base) {
    if (n!=1 || suffix[0] != 'B') {  // skip printing 1 B for the unit column
      _mi_snprintf(buf, len, "%lld   %-3s", (long long)n, (n==0 ? "" : suffix));
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
    _mi_snprintf(unitdesc, 8, "%s%s%s", magnitude, (base==1024 ? "i" : ""), suffix);
    _mi_snprintf(buf, len, "%ld.%ld %-3s", whole, (frac1 < 0 ? -frac1 : frac1), unitdesc);
  }
  if (limitwidth) {
    _mi_fprintf(out, arg, "%12s", buf);
  }
  else {
    _mi_fprintf(out, arg, "%s", buf);
  }
}


static void mi_print_amount(int64_t n, int64_t unit, mi_output_fun* out, void* arg) {
  mi_printf_amount(n,unit,out,arg,true);
}

static void mi_print_count(int64_t n, int64_t unit, mi_output_fun* out, void* arg) {
  if (unit==1) _mi_fprintf(out, arg, "%12s"," ");
          else mi_print_amount(n,0,out,arg);
}

static void mi_stat_print_ex(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg, const char* notok ) {
  _mi_fprintf(out, arg,"%10s:", msg);
  if (unit != 0) {
    if (unit > 0) {
      mi_print_amount(stat->peak, unit, out, arg);
      mi_print_amount(stat->total, unit, out, arg);
      // mi_print_amount(stat->freed, unit, out, arg);
      mi_print_amount(stat->current, unit, out, arg);
      mi_print_amount(unit, 1, out, arg);
      mi_print_count(stat->total, unit, out, arg);
    }
    else {
      mi_print_amount(stat->peak, -1, out, arg);
      mi_print_amount(stat->total, -1, out, arg);
      // mi_print_amount(stat->freed, -1, out, arg);
      mi_print_amount(stat->current, -1, out, arg);
      if (unit == -1) {
        _mi_fprintf(out, arg, "%24s", "");
      }
      else {
        mi_print_amount(-unit, 1, out, arg);
        mi_print_count((stat->total / -unit), 0, out, arg);
      }
    }
    if (stat->current != 0) {
      _mi_fprintf(out, arg, "  ");
      _mi_fprintf(out, arg, "%s", (notok == NULL ? "not all freed" : notok));
      _mi_fprintf(out, arg, "\n");
    }
    else {
      _mi_fprintf(out, arg, "  ok\n");
    }
  }
  else {
    mi_print_amount(stat->peak, 0, out, arg);
    mi_print_amount(stat->total, 0, out, arg);
    mi_print_amount(stat->current, 0, out, arg);
    _mi_fprintf(out, arg, "\n");
  }
}

static void mi_stat_print(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg) {
  mi_stat_print_ex(stat, msg, unit, out, arg, NULL);
}

#if MI_STAT>1
static void mi_stat_total_print(const mi_stat_count_t* stat, const char* msg, int64_t unit, mi_output_fun* out, void* arg) {
  _mi_fprintf(out, arg, "%10s:", msg);
  _mi_fprintf(out, arg, "%12s", " ");  // no peak
  mi_print_amount(stat->total, unit, out, arg);
  _mi_fprintf(out, arg, "\n");
}
#endif

static void mi_stat_counter_print(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, "%10s:", msg);
  mi_print_amount(stat->total, 0, out, arg);
  _mi_fprintf(out, arg, "\n");
}

static void mi_stat_counter_print_size(const mi_stat_counter_t* stat, const char* msg, mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, "%10s:", msg);
  mi_print_amount(stat->total, 1, out, arg);
  _mi_fprintf(out, arg, "\n");
}

static void mi_stat_average_print(int64_t count, int64_t total, const char* msg, mi_output_fun* out, void* arg) {
  const int64_t avg_tens = (count == 0 ? 0 : (total*10 / count));
  const int64_t avg_whole = avg_tens/10;
  const int64_t avg_frac1 = avg_tens%10;
  _mi_fprintf(out, arg, "%10s: %5lld.%lld avg\n", msg, avg_whole, avg_frac1);
}


static void mi_print_header(mi_output_fun* out, void* arg ) {
  _mi_fprintf(out, arg, "%10s: %11s %11s %11s %11s %11s\n", "heap stats", "peak   ", "total   ", "current   ", "block   ", "total#   ");
}

#if MI_STAT>1
static void mi_stats_print_bins(const mi_stat_count_t* bins, size_t max, const char* fmt, mi_output_fun* out, void* arg) {
  bool found = false;
  char buf[64];
  for (size_t i = 0; i <= max; i++) {
    if (bins[i].total > 0) {
      found = true;
      int64_t unit = _mi_bin_size((uint8_t)i);
      _mi_snprintf(buf, 64, "%s %3lu", fmt, (long)i);
      mi_stat_print(&bins[i], buf, unit, out, arg);
    }
  }
  if (found) {
    _mi_fprintf(out, arg, "\n");
    mi_print_header(out, arg);
  }
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

static void _mi_stats_print(mi_stats_t* stats, mi_output_fun* out0, void* arg0) mi_attr_noexcept {
  // wrap the output function to be line buffered
  char buf[256];
  buffered_t buffer = { out0, arg0, NULL, 0, 255 };
  buffer.buf = buf;
  mi_output_fun* out = &mi_buffered_out;
  void* arg = &buffer;

  // and print using that
  mi_print_header(out,arg);
  #if MI_STAT>1
  mi_stats_print_bins(stats->malloc_bins, MI_BIN_HUGE, "bin",out,arg);
  #endif
  #if MI_STAT
  mi_stat_print(&stats->malloc_normal, "binned", (stats->malloc_normal_count.total == 0 ? -1 : 1), out, arg);
  mi_stat_print(&stats->malloc_huge, "huge", (stats->malloc_huge_count.total == 0 ? -1 : 1), out, arg);
  mi_stat_count_t total = { 0,0,0 };
  mi_stat_count_add_mt(&total, &stats->malloc_normal);
  // mi_stat_count_add(&total, &stats->malloc_large);
  mi_stat_count_add_mt(&total, &stats->malloc_huge);
  mi_stat_print_ex(&total, "total", 1, out, arg, "");
  #endif
  #if MI_STAT>1
  mi_stat_total_print(&stats->malloc_requested, "malloc req", 1, out, arg);
  _mi_fprintf(out, arg, "\n");
  #endif
  mi_stat_print_ex(&stats->reserved, "reserved", 1, out, arg, "");
  mi_stat_print_ex(&stats->committed, "committed", 1, out, arg, "");
  mi_stat_counter_print_size(&stats->reset, "reset", out, arg );
  mi_stat_counter_print_size(&stats->purged, "purged", out, arg );
  mi_stat_print_ex(&stats->page_committed, "touched", 1, out, arg, "");
  mi_stat_print(&stats->segments, "segments", 0, out, arg);
  mi_stat_print(&stats->segments_abandoned, "-abandoned", 0, out, arg);
  mi_stat_print(&stats->segments_cache, "-cached", 0, out, arg);
  mi_stat_print(&stats->pages, "pages", 0, out, arg);
  mi_stat_print(&stats->pages_abandoned, "-abandoned", 0, out, arg);
  mi_stat_counter_print(&stats->pages_extended, "-extended", out, arg);
  mi_stat_counter_print(&stats->pages_retire, "-retire", out, arg);
  mi_stat_counter_print(&stats->arena_count, "arenas", out, arg);
  // mi_stat_counter_print(&stats->arena_crossover_count, "-crossover", out, arg);
  mi_stat_counter_print(&stats->arena_rollback_count, "-rollback", out, arg);
  mi_stat_counter_print(&stats->mmap_calls, "mmaps", out, arg);
  mi_stat_counter_print(&stats->commit_calls, "commits", out, arg);
  mi_stat_counter_print(&stats->reset_calls, "resets", out, arg);
  mi_stat_counter_print(&stats->purge_calls, "purges", out, arg);
  mi_stat_counter_print(&stats->malloc_guarded_count, "guarded", out, arg);
  mi_stat_print(&stats->threads, "threads", 0, out, arg);
  mi_stat_average_print(stats->page_searches_count.total, stats->page_searches.total, "searches", out, arg);
  _mi_fprintf(out, arg, "%10s: %5i\n", "numa nodes", _mi_os_numa_node_count());

  size_t elapsed;
  size_t user_time;
  size_t sys_time;
  size_t current_rss;
  size_t peak_rss;
  size_t current_commit;
  size_t peak_commit;
  size_t page_faults;
  mi_process_info(&elapsed, &user_time, &sys_time, &current_rss, &peak_rss, &current_commit, &peak_commit, &page_faults);
  _mi_fprintf(out, arg, "%10s: %5zu.%03zu s\n", "elapsed", elapsed/1000, elapsed%1000);
  _mi_fprintf(out, arg, "%10s: user: %zu.%03zu s, system: %zu.%03zu s, faults: %zu, peak rss: ", "process",
              user_time/1000, user_time%1000, sys_time/1000, sys_time%1000, page_faults );
  mi_printf_amount((int64_t)peak_rss, 1, out, arg, false);
  if (peak_commit > 0) {
    _mi_fprintf(out, arg, ", peak commit: ");
    mi_printf_amount((int64_t)peak_commit, 1, out, arg, false);
  }
  _mi_fprintf(out, arg, "\n");
}

static mi_msecs_t mi_process_start; // = 0

static mi_stats_t* mi_stats_get_default(void) {
  mi_heap_t* heap = mi_heap_get_default();
  return &heap->tld->stats;
}

static void mi_stats_merge_from(mi_stats_t* stats) {
  if (stats != &_mi_stats_main) {
    mi_stats_add(&_mi_stats_main, stats);
    memset(stats, 0, sizeof(mi_stats_t));
  }
}

void mi_stats_reset(void) mi_attr_noexcept {
  mi_stats_t* stats = mi_stats_get_default();
  if (stats != &_mi_stats_main) { memset(stats, 0, sizeof(mi_stats_t)); }
  memset(&_mi_stats_main, 0, sizeof(mi_stats_t));
  if (mi_process_start == 0) { mi_process_start = _mi_clock_start(); };
}

void mi_stats_merge(void) mi_attr_noexcept {
  mi_stats_merge_from( mi_stats_get_default() );
}

void _mi_stats_merge_thread(mi_tld_t* tld) {
  mi_stats_merge_from( &tld->stats );
}

void _mi_stats_done(mi_stats_t* stats) {  // called from `mi_thread_done`
  mi_stats_merge_from(stats);
}

void mi_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  mi_stats_merge_from(mi_stats_get_default());
  _mi_stats_print(&_mi_stats_main, out, arg);
}

void mi_stats_print(void* out) mi_attr_noexcept {
  // for compatibility there is an `out` parameter (which can be `stdout` or `stderr`)
  mi_stats_print_out((mi_output_fun*)out, NULL);
}

void mi_thread_stats_print_out(mi_output_fun* out, void* arg) mi_attr_noexcept {
  _mi_stats_print(mi_stats_get_default(), out, arg);
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
  pinfo.current_commit = (size_t)(mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&_mi_stats_main.committed.current));
  pinfo.peak_commit    = (size_t)(mi_atomic_loadi64_relaxed((_Atomic(int64_t)*)&_mi_stats_main.committed.peak));
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


// --------------------------------------------------------
// Return statistics
// --------------------------------------------------------

bool mi_stats_get(mi_stats_t* stats) mi_attr_noexcept {
  if (stats == NULL || stats->size != sizeof(mi_stats_t) || stats->version != MI_STAT_VERSION) return false;
  _mi_memzero(stats,stats->size);
  _mi_memcpy(stats, &_mi_stats_main, sizeof(mi_stats_t));
  return true;
}


// --------------------------------------------------------
// Statics in json format
// --------------------------------------------------------

typedef struct mi_heap_buf_s {
  char*   buf;
  size_t  size;
  size_t  used;
  bool    can_realloc;
} mi_heap_buf_t;

static bool mi_heap_buf_expand(mi_heap_buf_t* hbuf) {
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

static void mi_heap_buf_print(mi_heap_buf_t* hbuf, const char* msg) {
  if (msg==NULL || hbuf==NULL) return;
  if (hbuf->used + 1 >= hbuf->size && !hbuf->can_realloc) return;
  for (const char* src = msg; *src != 0; src++) {
    char c = *src;
    if (hbuf->used + 1 >= hbuf->size) {
      if (!mi_heap_buf_expand(hbuf)) return;
    }
    mi_assert_internal(hbuf->used < hbuf->size);
    hbuf->buf[hbuf->used++] = c;
  }
  mi_assert_internal(hbuf->used < hbuf->size);
  hbuf->buf[hbuf->used] = 0;
}

static void mi_heap_buf_print_count_bin(mi_heap_buf_t* hbuf, const char* prefix, mi_stat_count_t* stat, size_t bin, bool add_comma) {
  const size_t binsize = _mi_bin_size(bin);
  const size_t pagesize = (binsize <= MI_SMALL_OBJ_SIZE_MAX ? MI_SMALL_PAGE_SIZE :
                            (binsize <= MI_MEDIUM_OBJ_SIZE_MAX ? MI_MEDIUM_PAGE_SIZE :
                              #if MI_LARGE_PAGE_SIZE
                              (binsize <= MI_LARGE_OBJ_SIZE_MAX ? MI_LARGE_PAGE_SIZE : 0)
                              #else
                              0
                              #endif
                              ));
  char buf[128];
  _mi_snprintf(buf, 128, "%s{ \"total\": %lld, \"peak\": %lld, \"current\": %lld, \"block_size\": %zu, \"page_size\": %zu }%s\n", prefix, stat->total, stat->peak, stat->current, binsize, pagesize, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_heap_buf_print(hbuf, buf);
}

static void mi_heap_buf_print_count(mi_heap_buf_t* hbuf, const char* prefix, mi_stat_count_t* stat, bool add_comma) {
  char buf[128];
  _mi_snprintf(buf, 128, "%s{ \"total\": %lld, \"peak\": %lld, \"current\": %lld }%s\n", prefix, stat->total, stat->peak, stat->current, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_heap_buf_print(hbuf, buf);
}

static void mi_heap_buf_print_count_value(mi_heap_buf_t* hbuf, const char* name, mi_stat_count_t* stat) {
  char buf[128];
  _mi_snprintf(buf, 128, "  \"%s\": ", name);
  buf[127] = 0;
  mi_heap_buf_print(hbuf, buf);
  mi_heap_buf_print_count(hbuf, "", stat, true);
}

static void mi_heap_buf_print_value(mi_heap_buf_t* hbuf, const char* name, int64_t val) {
  char buf[128];
  _mi_snprintf(buf, 128, "  \"%s\": %lld,\n", name, val);
  buf[127] = 0;
  mi_heap_buf_print(hbuf, buf);
}

static void mi_heap_buf_print_size(mi_heap_buf_t* hbuf, const char* name, size_t val, bool add_comma) {
  char buf[128];
  _mi_snprintf(buf, 128, "    \"%s\": %zu%s\n", name, val, (add_comma ? "," : ""));
  buf[127] = 0;
  mi_heap_buf_print(hbuf, buf);
}

static void mi_heap_buf_print_counter_value(mi_heap_buf_t* hbuf, const char* name, mi_stat_counter_t* stat) {
  mi_heap_buf_print_value(hbuf, name, stat->total);
}

#define MI_STAT_COUNT(stat)    mi_heap_buf_print_count_value(&hbuf, #stat, &stats->stat);
#define MI_STAT_COUNTER(stat)  mi_heap_buf_print_counter_value(&hbuf, #stat, &stats->stat);

char* mi_stats_get_json(size_t output_size, char* output_buf) mi_attr_noexcept {
  mi_heap_buf_t hbuf = { NULL, 0, 0, true };
  if (output_size > 0 && output_buf != NULL) {
    _mi_memzero(output_buf, output_size);
    hbuf.buf = output_buf;
    hbuf.size = output_size;
    hbuf.can_realloc = false;
  }
  else {
    if (!mi_heap_buf_expand(&hbuf)) return NULL;
  }
  mi_heap_buf_print(&hbuf, "{\n");
  mi_heap_buf_print_value(&hbuf, "stat_version", MI_STAT_VERSION);
  mi_heap_buf_print_value(&hbuf, "mimalloc_version", MI_MALLOC_VERSION);

  // process info
  mi_heap_buf_print(&hbuf, "  \"process\": {\n");
  size_t elapsed;
  size_t user_time;
  size_t sys_time;
  size_t current_rss;
  size_t peak_rss;
  size_t current_commit;
  size_t peak_commit;
  size_t page_faults;
  mi_process_info(&elapsed, &user_time, &sys_time, &current_rss, &peak_rss, &current_commit, &peak_commit, &page_faults);
  mi_heap_buf_print_size(&hbuf, "elapsed_msecs", elapsed, true);
  mi_heap_buf_print_size(&hbuf, "user_msecs", user_time, true);
  mi_heap_buf_print_size(&hbuf, "system_msecs", sys_time, true);
  mi_heap_buf_print_size(&hbuf, "page_faults", page_faults, true);
  mi_heap_buf_print_size(&hbuf, "rss_current", current_rss, true);
  mi_heap_buf_print_size(&hbuf, "rss_peak", peak_rss, true);
  mi_heap_buf_print_size(&hbuf, "commit_current", current_commit, true);
  mi_heap_buf_print_size(&hbuf, "commit_peak", peak_commit, false);
  mi_heap_buf_print(&hbuf, "  },\n");

  // statistics
  mi_stats_t* stats = &_mi_stats_main;
  MI_STAT_FIELDS()

  // size bins
  mi_heap_buf_print(&hbuf, "  \"malloc_bins\": [\n");
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_heap_buf_print_count_bin(&hbuf, "    ", &stats->malloc_bins[i], i, i!=MI_BIN_HUGE);
  }
  mi_heap_buf_print(&hbuf, "  ],\n");
  mi_heap_buf_print(&hbuf, "  \"page_bins\": [\n");
  for (size_t i = 0; i <= MI_BIN_HUGE; i++) {
    mi_heap_buf_print_count_bin(&hbuf, "    ", &stats->page_bins[i], i, i!=MI_BIN_HUGE);
  }
  mi_heap_buf_print(&hbuf, "  ]\n");
  mi_heap_buf_print(&hbuf, "}\n");
  if (hbuf.used >= hbuf.size) {
    // failed
    if (hbuf.can_realloc) { mi_free(hbuf.buf); }
    return NULL;
  }
  else {
    return hbuf.buf;
  }
}
