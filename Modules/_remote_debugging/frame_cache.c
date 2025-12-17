/******************************************************************************
 * Remote Debugging Module - Frame Cache
 *
 * This file contains functions for caching frame information to optimize
 * repeated stack unwinding for profiling.
 ******************************************************************************/

#include "_remote_debugging.h"

/* ============================================================================
 * FRAME CACHE - stores (address, frame_info) pairs per thread
 * Uses preallocated fixed-size arrays for efficiency and bounded memory.
 * ============================================================================ */

int
frame_cache_init(RemoteUnwinderObject *unwinder)
{
    unwinder->frame_cache = PyMem_Calloc(FRAME_CACHE_MAX_THREADS, sizeof(FrameCacheEntry));
    if (!unwinder->frame_cache) {
        PyErr_NoMemory();
        return -1;
    }
    return 0;
}

void
frame_cache_cleanup(RemoteUnwinderObject *unwinder)
{
    if (!unwinder->frame_cache) {
        return;
    }
    for (int i = 0; i < FRAME_CACHE_MAX_THREADS; i++) {
        Py_CLEAR(unwinder->frame_cache[i].frame_list);
    }
    PyMem_Free(unwinder->frame_cache);
    unwinder->frame_cache = NULL;
}

// Find cache entry by thread_id
FrameCacheEntry *
frame_cache_find(RemoteUnwinderObject *unwinder, uint64_t thread_id)
{
    if (!unwinder->frame_cache || thread_id == 0) {
        return NULL;
    }
    for (int i = 0; i < FRAME_CACHE_MAX_THREADS; i++) {
        assert(i >= 0 && i < FRAME_CACHE_MAX_THREADS);
        if (unwinder->frame_cache[i].thread_id == thread_id) {
            assert(unwinder->frame_cache[i].num_addrs <= FRAME_CACHE_MAX_FRAMES);
            return &unwinder->frame_cache[i];
        }
    }
    return NULL;
}

// Allocate a cache slot for a thread
// Returns NULL if cache is full (graceful degradation)
static FrameCacheEntry *
frame_cache_alloc_slot(RemoteUnwinderObject *unwinder, uint64_t thread_id)
{
    if (!unwinder->frame_cache || thread_id == 0) {
        return NULL;
    }
    // First check if thread already has an entry
    for (int i = 0; i < FRAME_CACHE_MAX_THREADS; i++) {
        if (unwinder->frame_cache[i].thread_id == thread_id) {
            return &unwinder->frame_cache[i];
        }
    }
    // Find empty slot
    for (int i = 0; i < FRAME_CACHE_MAX_THREADS; i++) {
        if (unwinder->frame_cache[i].thread_id == 0) {
            return &unwinder->frame_cache[i];
        }
    }
    // Cache full - graceful degradation
    return NULL;
}

// Remove cache entries for threads not seen in the result
// result structure: list of InterpreterInfo, where InterpreterInfo[1] is threads list,
// and ThreadInfo[0] is the thread_id
void
frame_cache_invalidate_stale(RemoteUnwinderObject *unwinder, PyObject *result)
{
    if (!unwinder->frame_cache || !result || !PyList_Check(result)) {
        return;
    }

    // Build array of seen thread IDs from result
    uint64_t seen_threads[FRAME_CACHE_MAX_THREADS];
    int num_seen = 0;

    Py_ssize_t num_interps = PyList_GET_SIZE(result);
    for (Py_ssize_t i = 0; i < num_interps && num_seen < FRAME_CACHE_MAX_THREADS; i++) {
        PyObject *interp_info = PyList_GET_ITEM(result, i);
        PyObject *threads = PyStructSequence_GetItem(interp_info, 1);
        if (!threads || !PyList_Check(threads)) {
            continue;
        }
        Py_ssize_t num_threads = PyList_GET_SIZE(threads);
        for (Py_ssize_t j = 0; j < num_threads && num_seen < FRAME_CACHE_MAX_THREADS; j++) {
            PyObject *thread_info = PyList_GET_ITEM(threads, j);
            PyObject *tid_obj = PyStructSequence_GetItem(thread_info, 0);
            if (tid_obj) {
                uint64_t tid = PyLong_AsUnsignedLongLong(tid_obj);
                if (!PyErr_Occurred()) {
                    seen_threads[num_seen++] = tid;
                } else {
                    PyErr_Clear();
                }
            }
        }
    }

    // Invalidate entries not in seen list
    for (int i = 0; i < FRAME_CACHE_MAX_THREADS; i++) {
        if (unwinder->frame_cache[i].thread_id == 0) {
            continue;
        }
        int found = 0;
        for (int j = 0; j < num_seen; j++) {
            if (unwinder->frame_cache[i].thread_id == seen_threads[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // Clear this entry
            Py_CLEAR(unwinder->frame_cache[i].frame_list);
            unwinder->frame_cache[i].thread_id = 0;
            unwinder->frame_cache[i].num_addrs = 0;
            STATS_INC(unwinder, stale_cache_invalidations);
        }
    }
}

// Find last_profiled_frame in cache and extend frame_info with cached continuation
// If frame_addrs is provided (not NULL), also extends it with cached addresses
int
frame_cache_lookup_and_extend(
    RemoteUnwinderObject *unwinder,
    uint64_t thread_id,
    uintptr_t last_profiled_frame,
    PyObject *frame_info,
    uintptr_t *frame_addrs,
    Py_ssize_t *num_addrs,
    Py_ssize_t max_addrs)
{
    if (!unwinder->frame_cache || last_profiled_frame == 0) {
        return 0;
    }

    FrameCacheEntry *entry = frame_cache_find(unwinder, thread_id);
    if (!entry || !entry->frame_list) {
        return 0;
    }

    assert(entry->num_addrs >= 0 && entry->num_addrs <= FRAME_CACHE_MAX_FRAMES);

    // Find the index where last_profiled_frame matches
    Py_ssize_t start_idx = -1;
    for (Py_ssize_t i = 0; i < entry->num_addrs; i++) {
        if (entry->addrs[i] == last_profiled_frame) {
            start_idx = i;
            break;
        }
    }

    if (start_idx < 0) {
        return 0;  // Not found
    }
    assert(start_idx < entry->num_addrs);

    Py_ssize_t num_frames = PyList_GET_SIZE(entry->frame_list);

    // Extend frame_info with frames from start_idx onwards
    PyObject *slice = PyList_GetSlice(entry->frame_list, start_idx, num_frames);
    if (!slice) {
        return -1;
    }

    Py_ssize_t cur_size = PyList_GET_SIZE(frame_info);
    int result = PyList_SetSlice(frame_info, cur_size, cur_size, slice);
    Py_DECREF(slice);

    if (result < 0) {
        return -1;
    }

    // Also extend frame_addrs with cached addresses if provided
    if (frame_addrs) {
        for (Py_ssize_t i = start_idx; i < entry->num_addrs && *num_addrs < max_addrs; i++) {
            frame_addrs[(*num_addrs)++] = entry->addrs[i];
        }
    }

    return 1;
}

// Store frame list with addresses in cache
// Only stores complete stacks that reach base_frame_addr (validation done internally)
// Returns: 1 = stored successfully, 0 = not stored (graceful degradation), -1 = error
int
frame_cache_store(
    RemoteUnwinderObject *unwinder,
    uint64_t thread_id,
    PyObject *frame_list,
    const uintptr_t *addrs,
    Py_ssize_t num_addrs,
    uintptr_t base_frame_addr,
    uintptr_t last_frame_visited)
{
    if (!unwinder->frame_cache || thread_id == 0) {
        return 0;
    }

    // Validate we have a complete stack before caching.
    // Only cache if last_frame_visited matches base_frame_addr (the sentinel
    // at the bottom of the stack). Note: we use last_frame_visited rather than
    // addrs[num_addrs-1] because the base frame is visited but not added to the
    // addrs array (it returns frame==NULL from is_frame_valid due to
    // owner==FRAME_OWNED_BY_INTERPRETER).
    if (base_frame_addr != 0 && last_frame_visited != base_frame_addr) {
        // Incomplete stack - don't cache (graceful degradation)
        return 0;
    }

    // Clamp to max frames
    if (num_addrs > FRAME_CACHE_MAX_FRAMES) {
        num_addrs = FRAME_CACHE_MAX_FRAMES;
    }
    assert(num_addrs >= 0 && num_addrs <= FRAME_CACHE_MAX_FRAMES);

    FrameCacheEntry *entry = frame_cache_alloc_slot(unwinder, thread_id);
    if (!entry) {
        // Cache full - graceful degradation
        return 0;
    }

    // Clear old frame_list if replacing
    Py_CLEAR(entry->frame_list);

    // Store full frame list (don't truncate to num_addrs - frames beyond the
    // address array limit are still valid and needed for full cache hits)
    Py_ssize_t num_frames = PyList_GET_SIZE(frame_list);
    entry->frame_list = PyList_GetSlice(frame_list, 0, num_frames);
    if (!entry->frame_list) {
        return -1;
    }
    entry->thread_id = thread_id;
    memcpy(entry->addrs, addrs, num_addrs * sizeof(uintptr_t));
    entry->num_addrs = num_addrs;
    assert(entry->num_addrs == num_addrs);
    assert(entry->thread_id == thread_id);

    return 1;
}
