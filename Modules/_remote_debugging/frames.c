/******************************************************************************
 * Remote Debugging Module - Frame Functions
 *
 * This file contains functions for parsing interpreter frames and
 * managing stack chunks from remote process memory.
 ******************************************************************************/

#include "_remote_debugging.h"

/* ============================================================================
 * STACK CHUNK MANAGEMENT FUNCTIONS
 * ============================================================================ */

void
cleanup_stack_chunks(StackChunkList *chunks)
{
    for (size_t i = 0; i < chunks->count; ++i) {
        PyMem_RawFree(chunks->chunks[i].local_copy);
    }
    PyMem_RawFree(chunks->chunks);
}

static int
process_single_stack_chunk(
    RemoteUnwinderObject *unwinder,
    uintptr_t chunk_addr,
    StackChunkInfo *chunk_info
) {
    // Start with default size assumption
    size_t current_size = _PY_DATA_STACK_CHUNK_SIZE;

    char *this_chunk = PyMem_RawMalloc(current_size);
    if (!this_chunk) {
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate stack chunk buffer");
        return -1;
    }

    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, chunk_addr, current_size, this_chunk) < 0) {
        PyMem_RawFree(this_chunk);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read stack chunk");
        return -1;
    }

    // Check actual size and reread if necessary
    size_t actual_size = GET_MEMBER(size_t, this_chunk, offsetof(_PyStackChunk, size));
    if (actual_size != current_size) {
        this_chunk = PyMem_RawRealloc(this_chunk, actual_size);
        if (!this_chunk) {
            PyErr_NoMemory();
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to reallocate stack chunk buffer");
            return -1;
        }

        if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, chunk_addr, actual_size, this_chunk) < 0) {
            PyMem_RawFree(this_chunk);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to reread stack chunk with correct size");
            return -1;
        }
        current_size = actual_size;
    }

    chunk_info->remote_addr = chunk_addr;
    chunk_info->size = current_size;
    chunk_info->local_copy = this_chunk;
    return 0;
}

int
copy_stack_chunks(RemoteUnwinderObject *unwinder,
                  uintptr_t tstate_addr,
                  StackChunkList *out_chunks)
{
    uintptr_t chunk_addr;
    StackChunkInfo *chunks = NULL;
    size_t count = 0;
    size_t max_chunks = 16;

    if (read_ptr(unwinder, tstate_addr + (uintptr_t)unwinder->debug_offsets.thread_state.datastack_chunk, &chunk_addr)) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read initial stack chunk address");
        return -1;
    }

    chunks = PyMem_RawMalloc(max_chunks * sizeof(StackChunkInfo));
    if (!chunks) {
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate stack chunks array");
        return -1;
    }

    while (chunk_addr != 0) {
        // Grow array if needed
        if (count >= max_chunks) {
            max_chunks *= 2;
            StackChunkInfo *new_chunks = PyMem_RawRealloc(chunks, max_chunks * sizeof(StackChunkInfo));
            if (!new_chunks) {
                PyErr_NoMemory();
                set_exception_cause(unwinder, PyExc_MemoryError, "Failed to grow stack chunks array");
                goto error;
            }
            chunks = new_chunks;
        }

        // Process this chunk
        if (process_single_stack_chunk(unwinder, chunk_addr, &chunks[count]) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to process stack chunk");
            goto error;
        }

        // Get next chunk address and increment count
        chunk_addr = GET_MEMBER(uintptr_t, chunks[count].local_copy, offsetof(_PyStackChunk, previous));
        count++;
    }

    out_chunks->chunks = chunks;
    out_chunks->count = count;
    return 0;

error:
    for (size_t i = 0; i < count; ++i) {
        PyMem_RawFree(chunks[i].local_copy);
    }
    PyMem_RawFree(chunks);
    return -1;
}

void *
find_frame_in_chunks(StackChunkList *chunks, uintptr_t remote_ptr)
{
    for (size_t i = 0; i < chunks->count; ++i) {
        uintptr_t base = chunks->chunks[i].remote_addr + offsetof(_PyStackChunk, data);
        size_t payload = chunks->chunks[i].size - offsetof(_PyStackChunk, data);

        if (remote_ptr >= base && remote_ptr < base + payload) {
            return (char *)chunks->chunks[i].local_copy + (remote_ptr - chunks->chunks[i].remote_addr);
        }
    }
    return NULL;
}

/* ============================================================================
 * FRAME PARSING FUNCTIONS
 * ============================================================================ */

int
is_frame_valid(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    uintptr_t code_object_addr
) {
    if ((void*)code_object_addr == NULL) {
        return 0;
    }

    void* frame = (void*)frame_addr;

    char owner = GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner);
    if (owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;  // C frame or sentinel base frame
    }

    if (owner != FRAME_OWNED_BY_GENERATOR && owner != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n", owner);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Unhandled frame owner type in async frame");
        return -1;
    }
    return 1;
}

int
parse_frame_object(
    RemoteUnwinderObject *unwinder,
    PyObject** result,
    uintptr_t address,
    uintptr_t* address_of_code_object,
    uintptr_t* previous_frame
) {
    char frame[SIZEOF_INTERP_FRAME];
    *address_of_code_object = 0;

    Py_ssize_t bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        address,
        SIZEOF_INTERP_FRAME,
        frame
    );
    if (bytes_read < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read interpreter frame");
        return -1;
    }
    STATS_INC(unwinder, memory_reads);
    STATS_ADD(unwinder, memory_bytes_read, SIZEOF_INTERP_FRAME);

    *previous_frame = GET_MEMBER(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.previous);
    uintptr_t code_object = GET_MEMBER_NO_TAG(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.executable);
    int frame_valid = is_frame_valid(unwinder, (uintptr_t)frame, code_object);
    if (frame_valid != 1) {
        return frame_valid;
    }

    uintptr_t instruction_pointer = GET_MEMBER(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.instr_ptr);

    // Get tlbc_index for free threading builds
    int32_t tlbc_index = 0;
#ifdef Py_GIL_DISABLED
    if (unwinder->debug_offsets.interpreter_frame.tlbc_index != 0) {
        tlbc_index = GET_MEMBER(int32_t, frame, unwinder->debug_offsets.interpreter_frame.tlbc_index);
    }
#endif

    *address_of_code_object = code_object;
    return parse_code_object(unwinder, result, code_object, instruction_pointer, previous_frame, tlbc_index);
}

int
parse_frame_from_chunks(
    RemoteUnwinderObject *unwinder,
    PyObject **result,
    uintptr_t address,
    uintptr_t *previous_frame,
    uintptr_t *stackpointer,
    StackChunkList *chunks
) {
    void *frame_ptr = find_frame_in_chunks(chunks, address);
    if (!frame_ptr) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Frame not found in stack chunks");
        return -1;
    }

    char *frame = (char *)frame_ptr;
    *previous_frame = GET_MEMBER(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.previous);
    *stackpointer = GET_MEMBER(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.stackpointer);
    uintptr_t code_object = GET_MEMBER_NO_TAG(uintptr_t, frame_ptr, unwinder->debug_offsets.interpreter_frame.executable);
    int frame_valid = is_frame_valid(unwinder, (uintptr_t)frame, code_object);
    if (frame_valid != 1) {
        return frame_valid;
    }

    uintptr_t instruction_pointer = GET_MEMBER(uintptr_t, frame, unwinder->debug_offsets.interpreter_frame.instr_ptr);

    // Get tlbc_index for free threading builds
    int32_t tlbc_index = 0;
#ifdef Py_GIL_DISABLED
    if (unwinder->debug_offsets.interpreter_frame.tlbc_index != 0) {
        tlbc_index = GET_MEMBER(int32_t, frame, unwinder->debug_offsets.interpreter_frame.tlbc_index);
    }
#endif

    return parse_code_object(unwinder, result, code_object, instruction_pointer, previous_frame, tlbc_index);
}

/* ============================================================================
 * FRAME CHAIN PROCESSING
 * ============================================================================ */

int
process_frame_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t initial_frame_addr,
    StackChunkList *chunks,
    PyObject *frame_info,
    uintptr_t base_frame_addr,
    uintptr_t gc_frame,
    uintptr_t last_profiled_frame,
    int *stopped_at_cached_frame,
    uintptr_t *frame_addrs,      // optional: C array to receive frame addresses
    Py_ssize_t *num_addrs,       // in/out: current count / updated count
    Py_ssize_t max_addrs,        // max capacity of frame_addrs array
    uintptr_t *out_last_frame_addr)  // optional: receives last frame address visited
{
    uintptr_t frame_addr = initial_frame_addr;
    uintptr_t prev_frame_addr = 0;
    uintptr_t last_frame_addr = 0;  // Track last frame visited for validation
    const size_t MAX_FRAMES = 1024 + 512;
    size_t frame_count = 0;

    // Initialize output parameters
    if (stopped_at_cached_frame) {
        *stopped_at_cached_frame = 0;
    }
    if (out_last_frame_addr) {
        *out_last_frame_addr = 0;
    }

    // Quick check: if current_frame == last_profiled_frame, entire stack is unchanged
    if (last_profiled_frame != 0 && initial_frame_addr == last_profiled_frame) {
        if (stopped_at_cached_frame) {
            *stopped_at_cached_frame = 1;
        }
        return 0;
    }

    while ((void*)frame_addr != NULL) {
        // Check if we've reached the cached frame - if so, stop here
        if (last_profiled_frame != 0 && frame_addr == last_profiled_frame) {
            if (stopped_at_cached_frame) {
                *stopped_at_cached_frame = 1;
            }
            break;
        }
        PyObject *frame = NULL;
        uintptr_t next_frame_addr = 0;
        uintptr_t stackpointer = 0;
        last_frame_addr = frame_addr;  // Remember this frame address

        if (++frame_count > MAX_FRAMES) {
            PyErr_SetString(PyExc_RuntimeError, "Too many stack frames (possible infinite loop)");
            set_exception_cause(unwinder, PyExc_RuntimeError, "Frame chain iteration limit exceeded");
            return -1;
        }

        if (parse_frame_from_chunks(unwinder, &frame, frame_addr, &next_frame_addr, &stackpointer, chunks) < 0) {
            PyErr_Clear();
            uintptr_t address_of_code_object = 0;
            if (parse_frame_object(unwinder, &frame, frame_addr, &address_of_code_object ,&next_frame_addr) < 0) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse frame object in chain");
                return -1;
            }
        }
        if (frame == NULL && PyList_GET_SIZE(frame_info) == 0) {
            const char *e = "Failed to parse initial frame in chain";
            PyErr_SetString(PyExc_RuntimeError, e);
            return -1;
        }
        PyObject *extra_frame = NULL;
        // This frame kicked off the current GC collection:
        if (unwinder->gc && frame_addr == gc_frame) {
            _Py_DECLARE_STR(gc, "<GC>");
            extra_frame = &_Py_STR(gc);
        }
        // Otherwise, check for native frames to insert:
        else if (unwinder->native &&
                 // We've reached an interpreter trampoline frame:
                 frame == NULL &&
                 // Bottommost frame is always native, so skip that one:
                 next_frame_addr &&
                 // Only suppress native frames if GC tracking is enabled and the next frame will be a GC frame:
                 !(unwinder->gc && next_frame_addr == gc_frame))
        {
            _Py_DECLARE_STR(native, "<native>");
            extra_frame = &_Py_STR(native);
        }
        if (extra_frame) {
            // Use "~" as file, None as location (synthetic frame), None as opcode
            PyObject *extra_frame_info = make_frame_info(
                unwinder, _Py_LATIN1_CHR('~'), Py_None, extra_frame, Py_None);
            if (extra_frame_info == NULL) {
                return -1;
            }
            if (PyList_Append(frame_info, extra_frame_info) < 0) {
                Py_DECREF(extra_frame_info);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append extra frame");
                return -1;
            }
            // Extra frames use 0 as address (they're synthetic)
            if (frame_addrs && *num_addrs < max_addrs) {
                frame_addrs[(*num_addrs)++] = 0;
            }
            Py_DECREF(extra_frame_info);
        }
        if (frame) {
            if (prev_frame_addr && frame_addr != prev_frame_addr) {
                const char *f = "Broken frame chain: expected frame at 0x%lx, got 0x%lx";
                PyErr_Format(PyExc_RuntimeError, f, prev_frame_addr, frame_addr);
                Py_DECREF(frame);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Frame chain consistency check failed");
                return -1;
            }

            if (PyList_Append(frame_info, frame) < 0) {
                Py_DECREF(frame);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append frame");
                return -1;
            }
            // Track the address for this frame
            if (frame_addrs && *num_addrs < max_addrs) {
                frame_addrs[(*num_addrs)++] = frame_addr;
            }
            Py_DECREF(frame);
        }

        prev_frame_addr = next_frame_addr;
        frame_addr = next_frame_addr;
    }

    // Validate we reached the base frame (sentinel at bottom of stack)
    // Only validate if we walked the full chain (didn't stop at cached frame)
    // and base_frame_addr is provided (non-zero)
    int stopped_early = stopped_at_cached_frame && *stopped_at_cached_frame;
    if (!stopped_early && base_frame_addr != 0 && last_frame_addr != base_frame_addr) {
        PyErr_Format(PyExc_RuntimeError,
            "Incomplete sample: did not reach base frame (expected 0x%lx, got 0x%lx)",
            base_frame_addr, last_frame_addr);
        return -1;
    }

    // Set output parameter for caller (needed for cache validation)
    if (out_last_frame_addr) {
        *out_last_frame_addr = last_frame_addr;
    }

    return 0;
}

// Clear last_profiled_frame for all threads in the target process.
// This must be called at the start of profiling to avoid stale values
// from previous profilers causing us to stop frame walking early.
int
clear_last_profiled_frames(RemoteUnwinderObject *unwinder)
{
    uintptr_t current_interp = unwinder->interpreter_addr;
    uintptr_t zero = 0;

    while (current_interp != 0) {
        // Get first thread in this interpreter
        uintptr_t tstate_addr;
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                current_interp + unwinder->debug_offsets.interpreter_state.threads_head,
                sizeof(void*),
                &tstate_addr) < 0) {
            // Non-fatal: just skip clearing
            PyErr_Clear();
            return 0;
        }

        // Iterate all threads in this interpreter
        while (tstate_addr != 0) {
            // Clear last_profiled_frame
            uintptr_t lpf_addr = tstate_addr + unwinder->debug_offsets.thread_state.last_profiled_frame;
            if (_Py_RemoteDebug_WriteRemoteMemory(&unwinder->handle, lpf_addr,
                                                  sizeof(uintptr_t), &zero) < 0) {
                // Non-fatal: just continue
                PyErr_Clear();
            }

            // Move to next thread
            if (_Py_RemoteDebug_PagedReadRemoteMemory(
                    &unwinder->handle,
                    tstate_addr + unwinder->debug_offsets.thread_state.next,
                    sizeof(void*),
                    &tstate_addr) < 0) {
                PyErr_Clear();
                break;
            }
        }

        // Move to next interpreter
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                current_interp + unwinder->debug_offsets.interpreter_state.next,
                sizeof(void*),
                &current_interp) < 0) {
            PyErr_Clear();
            break;
        }
    }

    return 0;
}

// Fast path: check if we have a full cache hit (parent stack unchanged)
// A "full hit" means current frame == last profiled frame, so we can reuse
// cached parent frames. We always read the current frame from memory to get
// updated line numbers (the line within a frame can change between samples).
// Returns: 1 if full hit (frame_info populated with current frame + cached parents),
//          0 if miss, -1 on error
static int
try_full_cache_hit(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    uintptr_t last_profiled_frame,
    uint64_t thread_id,
    PyObject *frame_info)
{
    if (!unwinder->frame_cache || last_profiled_frame == 0) {
        return 0;
    }
    // Full hit only if current frame == last profiled frame
    if (frame_addr != last_profiled_frame) {
        return 0;
    }

    FrameCacheEntry *entry = frame_cache_find(unwinder, thread_id);
    if (!entry || !entry->frame_list) {
        return 0;
    }

    // Verify first address matches (sanity check)
    if (entry->num_addrs == 0 || entry->addrs[0] != frame_addr) {
        return 0;
    }

    // Always read the current frame from memory to get updated line number
    PyObject *current_frame = NULL;
    uintptr_t code_object_addr = 0;
    uintptr_t previous_frame = 0;
    int parse_result = parse_frame_object(unwinder, &current_frame, frame_addr,
                                          &code_object_addr, &previous_frame);
    if (parse_result < 0) {
        return -1;
    }

    // Get cached parent frames first (before modifying frame_info)
    Py_ssize_t cached_size = PyList_GET_SIZE(entry->frame_list);
    PyObject *parent_slice = NULL;
    if (cached_size > 1) {
        parent_slice = PyList_GetSlice(entry->frame_list, 1, cached_size);
        if (!parent_slice) {
            Py_XDECREF(current_frame);
            return -1;
        }
    }

    // Now safe to modify frame_info - add current frame if valid
    if (current_frame != NULL) {
        if (PyList_Append(frame_info, current_frame) < 0) {
            Py_DECREF(current_frame);
            Py_XDECREF(parent_slice);
            return -1;
        }
        Py_DECREF(current_frame);
        STATS_ADD(unwinder, frames_read_from_memory, 1);
    }

    // Extend with cached parent frames
    if (parent_slice) {
        Py_ssize_t cur_size = PyList_GET_SIZE(frame_info);
        int result = PyList_SetSlice(frame_info, cur_size, cur_size, parent_slice);
        Py_DECREF(parent_slice);
        if (result < 0) {
            return -1;
        }
        STATS_ADD(unwinder, frames_read_from_cache, cached_size - 1);
    }

    STATS_INC(unwinder, frame_cache_hits);
    return 1;
}

// High-level helper: collect frames with cache optimization
// Returns complete frame_info list, handling all cache logic internally
int
collect_frames_with_cache(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    StackChunkList *chunks,
    PyObject *frame_info,
    uintptr_t base_frame_addr,
    uintptr_t gc_frame,
    uintptr_t last_profiled_frame,
    uint64_t thread_id)
{
    // Fast path: check for full cache hit first (no allocations needed)
    int full_hit = try_full_cache_hit(unwinder, frame_addr, last_profiled_frame,
                                       thread_id, frame_info);
    if (full_hit != 0) {
        return full_hit < 0 ? -1 : 0;  // Either error or success
    }

    uintptr_t addrs[FRAME_CACHE_MAX_FRAMES];
    Py_ssize_t num_addrs = 0;
    Py_ssize_t frames_before = PyList_GET_SIZE(frame_info);
    uintptr_t last_frame_visited = 0;

    int stopped_at_cached = 0;
    if (process_frame_chain(unwinder, frame_addr, chunks, frame_info, base_frame_addr, gc_frame,
                            last_profiled_frame, &stopped_at_cached,
                            addrs, &num_addrs, FRAME_CACHE_MAX_FRAMES,
                            &last_frame_visited) < 0) {
        return -1;
    }

    // Track frames read from memory (frames added by process_frame_chain)
    STATS_ADD(unwinder, frames_read_from_memory, PyList_GET_SIZE(frame_info) - frames_before);

    // If stopped at cached frame, extend with cached continuation (both frames and addresses)
    if (stopped_at_cached) {
        Py_ssize_t frames_before_cache = PyList_GET_SIZE(frame_info);
        int cache_result = frame_cache_lookup_and_extend(unwinder, thread_id, last_profiled_frame,
                                                         frame_info, addrs, &num_addrs,
                                                         FRAME_CACHE_MAX_FRAMES);
        if (cache_result < 0) {
            return -1;
        }
        if (cache_result == 0) {
            // Cache miss - continue walking from last_profiled_frame to get the rest
            STATS_INC(unwinder, frame_cache_misses);
            Py_ssize_t frames_before_walk = PyList_GET_SIZE(frame_info);
            if (process_frame_chain(unwinder, last_profiled_frame, chunks, frame_info, base_frame_addr, gc_frame,
                                    0, NULL, addrs, &num_addrs, FRAME_CACHE_MAX_FRAMES,
                                    &last_frame_visited) < 0) {
                return -1;
            }
            STATS_ADD(unwinder, frames_read_from_memory, PyList_GET_SIZE(frame_info) - frames_before_walk);
        } else {
            // Partial cache hit - cache was validated when stored, so we trust it
            STATS_INC(unwinder, frame_cache_partial_hits);
            STATS_ADD(unwinder, frames_read_from_cache, PyList_GET_SIZE(frame_info) - frames_before_cache);
        }
    } else {
        if (last_profiled_frame == 0) {
            // No cache involvement (no last_profiled_frame or cache disabled)
            STATS_INC(unwinder, frame_cache_misses);
        }
    }

    // Store in cache - frame_cache_store validates internally that we have a
    // complete stack (reached base_frame_addr) before actually storing
    if (frame_cache_store(unwinder, thread_id, frame_info, addrs, num_addrs,
                          base_frame_addr, last_frame_visited) < 0) {
        return -1;
    }

    return 0;
}
