/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc/util/stats.h"
#include "mimalloc/alloc/free.h"          // mi_free_subproc_safe
#include "mimalloc/alloc/page.h"
#include "mimalloc/heap/subproc.h"
#include "mimalloc/arena/arena-alloc.h"
#include "arena.h"


mi_decl_nodiscard bool _mi_arena_commit(mi_subproc_t* subproc, mi_arena_t* arena, void* start, size_t size, bool* is_zero, size_t stat_already_committed) {
  mi_assert_internal(subproc!=NULL);
  if (arena != NULL && arena->commit_fun != NULL) {
    return (*arena->commit_fun)(true, start, size, is_zero, arena->commit_fun_arg);
  }
  else if (stat_already_committed > 0) {
    return _mi_os_commit_ex(subproc, start, size, is_zero, stat_already_committed);
  }
  else {
    return _mi_os_commit(subproc, start, size, is_zero);
  }
}

// Create an arena memid
static mi_memid_t mi_memid_create_arena(mi_arena_t* arena, size_t slice_index, size_t slice_count) {
  mi_assert_internal(slice_index < UINT32_MAX);
  mi_assert_internal(slice_count < UINT32_MAX);
  mi_assert_internal(slice_count > 0);
  mi_assert_internal(slice_index < arena->slice_count);
  mi_memid_t memid = _mi_memid_create(MI_MEM_ARENA);
  memid.mem.arena.arena = arena;
  memid.mem.arena.slice_index = (uint32_t)slice_index;
  memid.mem.arena.slice_count = (uint32_t)slice_count;
  return memid;
}

/* -----------------------------------------------------------
  Arena Allocation
----------------------------------------------------------- */

static mi_decl_noinline void* mi_arena_try_alloc_at(
  mi_arena_t* arena, size_t slice_count, bool commit, size_t tseq, mi_memid_t* memid)
{
  mi_assert_internal(arena!=NULL);
  mi_assert_internal(slice_count>0);
  size_t slice_index;
  if (!mi_bbitmap_try_find_and_clearN(arena->slices_free, tseq, slice_count, &slice_index)) return NULL;

  // claimed it!
  void* p = mi_arena_slice_start(arena, slice_index);
  *memid = mi_memid_create_arena(arena, slice_index, slice_count);
  memid->is_pinned = arena->memid.is_pinned;

  // set the dirty bits and track which slices become accessible
  size_t touched_slices = slice_count;
  if (arena->memid.initially_zero) {
    size_t already_dirty = 0;
    memid->initially_zero = mi_bitmap_setN(arena->slices_dirty, slice_index, slice_count, &already_dirty);
    mi_assert_internal(already_dirty <= touched_slices);
    touched_slices -= already_dirty;
  }
  else {
    // todo: properly count touched pages with a separate bitmap?
    touched_slices = 0;
  }

  // set commit state
  if (commit) {
    // commit requested, but the range may not be committed as a whole: ensure it is committed now
    const size_t already_committed = mi_bitmap_popcountN(arena->slices_committed, slice_index, slice_count);
    if (already_committed < slice_count) {
      // not all committed, try to commit now
      bool commit_zero = false;
      if (!_mi_arena_commit(arena->subproc, arena, p, _mi_size_of_slices(slice_count), &commit_zero, _mi_size_of_slices(already_committed))) {
        // if the commit fails, release ownership, and return NULL;
        // note: this does not roll back dirty bits but that is ok.
        mi_bbitmap_setN(arena->slices_free, slice_index, slice_count);
        return NULL;
      }
      if (commit_zero) {
        memid->initially_zero = true;
      }

      // set the commit bits
      mi_bitmap_setN(arena->slices_committed, slice_index, slice_count, NULL);

      // committed
      #if MI_DEBUG > 1
      if (memid->initially_zero) {
        if (!mi_mem_is_zero(p, _mi_size_of_slices(slice_count))) {
          _mi_error_message(EFAULT, "internal error: arena allocation was not zero-initialized!\n");
          memid->initially_zero = false;
        }
      }
      #endif
    }
    else {
      // already fully committed.
      _mi_os_reuse(arena->subproc, p, _mi_size_of_slices(slice_count));
      // if the OS has overcommit, and this is the first time we access these pages, then
      // count the commit now (as at arena reserve we didn't count those commits as these are on-demand)
      if (_mi_os_has_overcommit() && touched_slices > 0 && !arena->memid.is_pinned /* huge pages, issue #1236 */) {
        mi_subproc_stat_increase( arena->subproc, committed, _mi_size_of_slices(touched_slices));
      }
    }

    mi_assert_internal(mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count));
    memid->initially_committed = true;

    // tool support
    if (memid->initially_zero) {
      mi_track_mem_defined(p, slice_count * MI_ARENA_SLICE_SIZE);
    }
    else {
      mi_track_mem_undefined(p, slice_count * MI_ARENA_SLICE_SIZE);
    }
  }
  else {
    // no need to commit, but check if it is already fully committed
    memid->initially_committed = mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count);
    if (!memid->initially_committed) {
      // partly committed.. adjust stats
      size_t already_committed_count = 0;
      mi_bitmap_setN(arena->slices_committed, slice_index, slice_count, &already_committed_count);
      mi_bitmap_clearN(arena->slices_committed, slice_index, slice_count);
      mi_subproc_stat_decrease(arena->subproc, committed, _mi_size_of_slices(already_committed_count));
    }
  }

  mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));
  if (commit) { mi_assert_internal(mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count)); }
  if (commit) { mi_assert_internal(memid->initially_committed); }
  mi_assert_internal(mi_bitmap_is_setN(arena->slices_dirty, slice_index, slice_count));

  return p;
}

/* -----------------------------------------------------------
  Arena allocation
----------------------------------------------------------- */

typedef struct mi_arena_alloc_visit_info_s {
  size_t      slice_count;
  bool        commit;
  size_t      tseq;
  mi_memid_t* memid;
} mi_arena_alloc_visit_info_t;

static bool mi_arena_try_alloc_visit(mi_arena_t* arena, const void* arg, void** result) {
  const mi_arena_alloc_visit_info_t* info = (const mi_arena_alloc_visit_info_t*)arg;
  mi_assert_internal(result != NULL);
  *result = mi_arena_try_alloc_at(arena, info->slice_count, info->commit, info->tseq, info->memid);
  if (*result != NULL) return false; // break
  return true; // continue
}

// allocate slices from the arenas
static mi_decl_noinline void* mi_arenas_try_find_free(
  mi_heap_t* heap, size_t slice_count, size_t alignment,
  bool commit, bool allow_large, mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid)
{
  // mi_assert_internal(slice_count <= _mi_slice_count_of_size(MI_ARENA_MAX_CHUNK_OBJ_SIZE));
  mi_assert(alignment <= MI_ARENA_SLICE_ALIGN);
  if (alignment > MI_ARENA_SLICE_ALIGN) return NULL;

  // search arena's
  const mi_arena_alloc_visit_info_t info = { slice_count, commit, tseq, memid };
  void* p = NULL;
  if (!mi_forall_suitable_arenas(heap, req_arena, tseq, true /* only numa matching */, numa_node, allow_large, &mi_arena_try_alloc_visit, &info, &p)) {
    return p;
  }
  if (numa_node < 0) return NULL;

  // search again but now regardless of preferred numa affinity
  mi_forall_suitable_arenas(heap, req_arena, tseq, false /* numa non-matching now */, numa_node, allow_large, &mi_arena_try_alloc_visit, &info, &p);
  return p;
}

// Allocate slices from the arena's -- potentially allocating a fresh arena
mi_decl_noinline void* _mi_arenas_try_alloc(
  mi_heap_t* heap,
  size_t slice_count, size_t alignment,
  bool commit, bool allow_large,
  mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid)
{
  // mi_assert(slice_count <= MI_ARENA_MAX_CHUNK_OBJ_SLICES);
  mi_assert(alignment <= MI_ARENA_SLICE_ALIGN);
  void* p;

  // not too large?
  if (slice_count * MI_ARENA_SLICE_SIZE > MI_ARENA_MAX_SIZE) return NULL;

  // try to find free slices in the arena's
  p = mi_arenas_try_find_free(heap, slice_count, alignment, commit, allow_large, req_arena, tseq, numa_node, memid);
  if (p != NULL) return p;

  // did we need a specific arena?
  if (req_arena != NULL) return NULL;

  // don't create arena's if OS allocation is disallowed
  if (mi_option_is_enabled(mi_option_disallow_os_alloc)) return NULL;

  // otherwise, try to reserve a new arena -- but one thread at a time.. (todo: allow 2 or 4 to reduce contention?)
  mi_subproc_t* const subproc = heap->subproc;
  const size_t arena_count = mi_arenas_get_count(subproc);
  mi_lock(&subproc->arena_reserve_lock) {
    if (arena_count == mi_arenas_get_count(subproc)) {
      // we are the first to enter the lock, reserve a fresh arena
      mi_arena_id_t arena_id = _mi_arena_id_none();
      _mi_arena_reserve(subproc, _mi_size_of_slices(slice_count), allow_large, &arena_id);
    }
    else {
      // another thread already reserved a new arena
    }
  }
  // try once more to allocate in the new arena
  mi_assert_internal(req_arena == NULL);
  p = mi_arenas_try_find_free(heap, slice_count, alignment, commit, allow_large, req_arena, tseq, numa_node, memid);
  if (p != NULL) return p;

  return NULL;
}

// Allocate from the OS (if allowed)
void* _mi_arena_os_alloc_aligned(
  mi_subproc_t* subproc,
  size_t size, size_t alignment, size_t align_offset,
  bool commit, bool allow_large,
  mi_arena_id_t req_arena_id, mi_memid_t* memid)
{
  // if we cannot use OS allocation, return NULL
  if (mi_option_is_enabled(mi_option_disallow_os_alloc) || req_arena_id != _mi_arena_id_none()) {
    errno = ENOMEM;
    return NULL;
  }

  if (align_offset > 0) {
    return _mi_os_alloc_aligned_at_offset(subproc, size, alignment, align_offset, commit, allow_large, memid);
  }
  else {
    return _mi_os_alloc_aligned(subproc, size, alignment, commit, allow_large, memid);
  }
}


// Allocate large sized memory
void* _mi_arenas_alloc_aligned( mi_heap_t* heap,
  size_t size, size_t alignment, size_t align_offset,
  bool commit, bool allow_large,
  mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid)
{
  mi_assert_internal(memid != NULL);
  mi_assert_internal(size > 0);

  // try to allocate in an arena if the alignment is small enough and the object is not too small (as for theap meta data)
  if (!mi_option_is_enabled(mi_option_disallow_arena_alloc) &&                // is arena allocation allowed?
      size >= MI_ARENA_MIN_OBJ_SIZE && size <= mi_arena_max_object_size() &&  // and not too small or too large
      alignment <= MI_ARENA_SLICE_ALIGN && align_offset == 0)                 // and good alignment
  {
    const size_t slice_count = _mi_slice_count_of_size(size);
    void* p = _mi_arenas_try_alloc(heap, slice_count, alignment, commit, allow_large, req_arena, tseq, numa_node, memid);
    if (p != NULL) return p;
  }

  // fall back to the OS
  void* p = _mi_arena_os_alloc_aligned(heap->subproc, size, alignment, align_offset, commit, allow_large, req_arena, memid);
  return p;
}

void* _mi_arenas_alloc(mi_heap_t* heap, size_t size, bool commit, bool allow_large, mi_arena_t* req_arena, size_t tseq, int numa_node, mi_memid_t* memid)
{
  return _mi_arenas_alloc_aligned(heap, size, MI_ARENA_SLICE_SIZE, 0, commit, allow_large, req_arena, tseq, numa_node, memid);
}

/* -----------------------------------------------------------
  Arena free
----------------------------------------------------------- */
static void mi_arena_schedule_purge(mi_arena_t* arena, size_t slice_index, size_t slices);
static void mi_arenas_try_purge(bool force, bool visit_all, mi_subproc_t* subproc, size_t tseq);

void _mi_arenas_free(mi_subproc_t* subproc, void* p, size_t size, mi_memid_t memid) {
  if (p==NULL) return;
  if (size==0) return;

  // need to set all memory to undefined as some parts may still be marked as no_access (like padding etc.)
  mi_track_mem_undefined(p, size);

  if (mi_memkind_is_os(memid.memkind)) {
    // was a direct OS allocation, pass through
    _mi_os_free(subproc, p, size, memid);
  }
  else if (memid.memkind == MI_MEM_ARENA) {
    // allocated in an arena
    size_t slice_count;
    size_t slice_index;
    mi_arena_t* arena = mi_arena_from_memid(memid, &slice_index, &slice_count);
    mi_assert_internal(arena!=NULL);
    mi_assert_internal(arena->subproc == subproc);
    mi_assert_internal((size%MI_ARENA_SLICE_SIZE)==0);
    mi_assert_internal((slice_count*MI_ARENA_SLICE_SIZE)==size);
    mi_assert_internal(mi_arena_slice_start(arena,slice_index) <= (uint8_t*)p);
    mi_assert_internal(mi_arena_slice_start(arena,slice_index) + _mi_size_of_slices(slice_count) > (uint8_t*)p);
    // checks
    if (arena == NULL) {
      _mi_error_message(EINVAL, "trying to free from an invalid arena: %p, size %zu, memkind: 0x%x\n", p, size, memid.memkind);
      return;
    }
    mi_assert_internal(slice_index < arena->slice_count);
    mi_assert_internal(slice_index >= mi_arena_info_slices(arena));
    if (slice_index < mi_arena_info_slices(arena) || slice_index >= arena->slice_count) {
      _mi_error_message(EINVAL, "trying to free from an invalid arena block: %p, size %zu, memkind: 0x%x\n", p, size, memid.memkind);
      return;
    }

    // potentially decommit
    if (!arena->memid.is_pinned /* && !arena->memid.initially_committed */) { // todo: allow decommit even if initially committed?
      // (delay) purge the page
      mi_arena_schedule_purge(arena, slice_index, slice_count);
    }

    // and make it available to others again
    bool all_inuse = mi_bbitmap_setN(arena->slices_free, slice_index, slice_count);
    if (!all_inuse) {
      _mi_error_message(EAGAIN, "trying to free an already freed arena block: %p, size %zu\n", mi_arena_slice_start(arena,slice_index), _mi_size_of_slices(slice_count));
      return;
    };
  }
  else if (memid.memkind == MI_MEM_MALLOC) {
    _mi_free_subproc_safe(p);
  }
  else {
    // arena was none, external, or static; nothing to do
    mi_assert_internal(mi_memid_needs_no_free(memid));
  }

  // try to purge expired decommits
  // mi_arenas_try_purge(false, false, NULL);
}

// Purge the arenas; if `force_purge` is true, amenable parts are purged even if not yet expired
void _mi_arenas_collect(bool force_purge, bool visit_all, mi_tld_t* tld) {
  mi_arenas_try_purge(force_purge, visit_all, tld->subproc, tld->thread_seq);
}

/* -----------------------------------------------------------
  Arena purge
----------------------------------------------------------- */

static long mi_arena_purge_delay(void) {
  // <0 = no purging allowed, 0=immediate purging, >0=milli-second delay
  const long delay = mi_option_get(mi_option_purge_delay);
  const long mult  = mi_option_get(mi_option_arena_purge_mult);
  if (delay<0 || mult<0)   { return -1; }
  if (delay==0 || mult==0) { return 0; }
  size_t total;
  if (mi_mul_overflow((size_t)delay, (size_t)mult, &total)) { return delay; }
  if (total > LONG_MAX) { return delay; }
  return (long)total;
}

// reset or decommit in an arena and update the commit bitmap
// assumes we own the area (i.e. slices_free is claimed by us)
// returns if the memory is no longer committed (versus reset which keeps the commit)
static bool mi_arena_purge(mi_arena_t* arena, size_t slice_index, size_t slice_count) {
  mi_assert_internal(!arena->memid.is_pinned);
  mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));

  const size_t size = _mi_size_of_slices(slice_count);
  void* const p = mi_arena_slice_start(arena, slice_index);
  //const bool all_committed = mi_bitmap_is_setN(arena->slices_committed, slice_index, slice_count);
  size_t already_committed;
  mi_bitmap_setN(arena->slices_committed, slice_index, slice_count, &already_committed); // pretend all committed.. (as we lack a clearN call that counts the already set bits..)
  const bool all_committed = (already_committed == slice_count);
  const bool needs_recommit = _mi_os_purge_ex(arena->subproc, p, size, all_committed /* allow reset? */, _mi_size_of_slices(already_committed), arena->commit_fun, arena->commit_fun_arg);

  if (needs_recommit) {
    // no longer committed
    mi_bitmap_clearN(arena->slices_committed, slice_index, slice_count);
    // we just counted in the purge to decommit all, but the some part was not committed so adjust that here
    // mi_subproc_stat_decrease(arena->subproc, committed, _mi_size_of_slices(slice_count - already_committed));
  }
  else if (!all_committed) {
    // we cannot assume any of these are committed any longer (even with reset since we did setN and may have marked uncommitted slices as committed)
    mi_bitmap_clearN(arena->slices_committed, slice_index, slice_count);
    // we adjust the commit count as parts will be re-committed
    // mi_subproc_stat_decrease(arena->subproc, committed, _mi_size_of_slices(already_committed));
  }

  return needs_recommit;
}


// Schedule a purge. This is usually delayed to avoid repeated decommit/commit calls.
// Note: assumes we (still) own the area as we may purge immediately
static void mi_arena_schedule_purge(mi_arena_t* arena, size_t slice_index, size_t slice_count) {
  const long delay = mi_arena_purge_delay();
  if (arena->memid.is_pinned || delay < 0 || _mi_preloading()) return;  // is purging allowed at all?

  mi_assert_internal(mi_bbitmap_is_clearN(arena->slices_free, slice_index, slice_count));
  if (delay == 0 || _mi_size_of_slices(slice_count) >= mi_option_get_size(mi_option_arena_purge_immediate_size)) {
    // purge directly
    mi_arena_purge(arena, slice_index, slice_count);
  }
  else {
    // schedule purge
    const mi_msecs_t expire = _mi_clock_now() + delay;
    mi_msecs_t expire0 = 0;
    if (mi_atomic_casi64_strong_acq_rel(&arena->purge_expire, &expire0, expire)) {
      // expiration was not yet set
      // maybe set the global arenas expire as well (if it wasn't set already)
      mi_assert_internal(expire0==0);
      mi_atomic_casi64_strong_acq_rel(&arena->subproc->purge_expire, &expire0, expire);
    }
    else {
      // already an expiration was set
    }
    mi_bitmap_setN(arena->slices_purge, slice_index, slice_count, NULL);
  }
}

typedef struct mi_purge_visit_info_s {
  mi_msecs_t now;
  mi_msecs_t delay;
  bool       all_purged;
  bool       any_purged;
} mi_purge_visit_info_t;

static bool mi_arena_try_purge_range(mi_arena_t* arena, size_t slice_index, size_t slice_count) {
  mi_assert(slice_count < MI_BCHUNK_BITS);
  if (mi_bbitmap_try_clearNC(arena->slices_free, slice_index, slice_count)) {
    // purge
    bool decommitted = mi_arena_purge(arena, slice_index, slice_count); MI_UNUSED(decommitted);
    mi_assert_internal(!decommitted || mi_bitmap_is_clearN(arena->slices_committed, slice_index, slice_count));
    // and reset the free range
    mi_bbitmap_setN(arena->slices_free, slice_index, slice_count);
    return true;
  }
  else {
    // was allocated again already
    return false;
  }
}

static bool mi_arena_try_purge_visitor(size_t slice_index, size_t slice_count, mi_arena_t* arena, void* arg) {
  mi_purge_visit_info_t* vinfo = (mi_purge_visit_info_t*)arg;
  // try to purge: first claim the free blocks
  if (mi_arena_try_purge_range(arena, slice_index, slice_count)) {
    vinfo->any_purged = true;
    vinfo->all_purged = true;
  }
  else if (slice_count > 1)
  {
    // failed to claim the full range, try per slice instead
    for (size_t i = 0; i < slice_count; i++) {
      const bool purged = mi_arena_try_purge_range(arena, slice_index + i, 1);
      vinfo->any_purged = vinfo->any_purged || purged;
      vinfo->all_purged = vinfo->all_purged && purged;
    }
  }
  // don't clear the purge bits as that is done atomically be the _bitmap_forall_set_ranges
  // mi_bitmap_clearN(arena->slices_purge, slice_index, slice_count);
  return true; // continue
}

// returns
// -1 = nothing was purged
// 0  = nothing was purged yet because have not yet reached the expire time
// 1  = some pages in the arena were purged
static int mi_arena_try_purge(mi_arena_t* arena, mi_msecs_t now, bool force)
{
  // check pre-conditions
  if (arena->memid.is_pinned) return -1;

  // expired yet?
  mi_msecs_t expire = mi_atomic_loadi64_relaxed(&arena->purge_expire);
  if (expire==0) return -1;
  if (!force && expire > now) return 0;

  // reset expire
  mi_atomic_storei64_release(&arena->purge_expire, (mi_msecs_t)0);
  mi_subproc_stat_counter_increase(arena->subproc, arena_purges, 1);

  // go through all purge info's  (with max MI_BFIELD_BITS ranges at a time)
  // this also clears those ranges atomically (so any newly freed blocks will get purged next
  // time around)
  mi_purge_visit_info_t vinfo = { now, mi_arena_purge_delay(), true /*all?*/, false /*any?*/};

  // we purge by at least `minslices` to not fragment transparent huge pages for example
  const size_t minslices = _mi_slice_count_of_size(_mi_os_minimal_purge_size());
  _mi_bitmap_forall_setc_rangesn(arena->slices_purge, minslices, &mi_arena_try_purge_visitor, arena, &vinfo);

  return (vinfo.any_purged ? 1 : -1);
}


static void mi_arenas_try_purge(bool force, bool visit_all, mi_subproc_t* subproc, size_t tseq)
{
  // try purge can be called often so try to only run when needed
  const long delay = mi_arena_purge_delay();
  if (_mi_preloading() || delay <= 0) return;  // nothing will be scheduled

  // check if any arena needs purging?
  const mi_msecs_t now = _mi_clock_now();
  const mi_msecs_t arenas_expire = mi_atomic_loadi64_acquire(&subproc->purge_expire);
  if (!visit_all && !force && (arenas_expire == 0 || arenas_expire > now)) return;

  const size_t max_arena = mi_arenas_get_count(subproc);
  if (max_arena == 0) return;

  // allow only one thread to purge at a time (todo: allow concurrent purging?)
  static mi_atomic_guard_t purge_guard;
  mi_atomic_guard(&purge_guard)
  {
    // increase global expire: at most one purge per delay cycle
    if (arenas_expire > now) { mi_atomic_storei64_release(&subproc->purge_expire, now + (delay/10)); }
    const size_t arena_start = tseq % max_arena;
    size_t max_purge_count = (visit_all ? max_arena : (max_arena/4)+1);
    bool all_visited = true;
    bool any_purged = false;
    for (size_t _i = 0; _i < max_arena; _i++) {
      size_t i = _i + arena_start;
      if (i >= max_arena) { i -= max_arena; }
      mi_arena_t* arena = mi_arena_from_index(subproc,i);
      if (arena != NULL) {
        const int purged = mi_arena_try_purge(arena, now, force);
        if (purged >= 0) {      // purged, or arena expire is not yet reached
          any_purged = true;
          if (purged >= 1) {    // purged
            if (max_purge_count <= 1) {
              all_visited = false;
              break;
            }
            max_purge_count--;
          }
        }
      }
    }
    if (all_visited && !any_purged) {
      mi_atomic_storei64_release(&subproc->purge_expire, (mi_msecs_t)0);
    }
  }
}

void mi_arenas_purge(void) mi_attr_noexcept {
  mi_arenas_try_purge(true, true, _mi_subproc_main(), 0);
}
