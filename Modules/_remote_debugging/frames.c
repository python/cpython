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

    const size_t MAX_STACK_CHUNKS = 4096;
    while (chunk_addr != 0 && count < MAX_STACK_CHUNKS) {
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
        assert(chunks->chunks[i].size > offsetof(_PyStackChunk, data));
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

    CodeObjectContext code_ctx = {
        .code_addr = code_object,
        .instruction_pointer = instruction_pointer,
        .tlbc_index = tlbc_index,
    };
    return parse_code_object(unwinder, result, &code_ctx);
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

    CodeObjectContext code_ctx = {
        .code_addr = code_object,
        .instruction_pointer = instruction_pointer,
        .tlbc_index = tlbc_index,
    };
    return parse_code_object(unwinder, result, &code_ctx);
}

/* ============================================================================
 * FRAME CHAIN PROCESSING
 * ============================================================================ */

int
process_frame_chain(
    RemoteUnwinderObject *unwinder,
    FrameWalkContext *ctx)
{
    uintptr_t frame_addr = ctx->frame_addr;
    uintptr_t prev_frame_addr = 0;
    uintptr_t last_frame_addr = 0;
    const size_t MAX_FRAMES = 1024 + 512;
    size_t frame_count = 0;
    assert(MAX_FRAMES > 0 && MAX_FRAMES < 10000);

    ctx->stopped_at_cached_frame = 0;
    ctx->last_frame_visited = 0;

    while ((void*)frame_addr != NULL) {
        PyObject *frame = NULL;
        uintptr_t next_frame_addr = 0;
        uintptr_t stackpointer = 0;
        last_frame_addr = frame_addr;

        if (++frame_count > MAX_FRAMES) {
            PyErr_SetString(PyExc_RuntimeError, "Too many stack frames (possible infinite loop)");
            set_exception_cause(unwinder, PyExc_RuntimeError, "Frame chain iteration limit exceeded");
            return -1;
        }
        assert(frame_count <= MAX_FRAMES);

        if (parse_frame_from_chunks(unwinder, &frame, frame_addr, &next_frame_addr, &stackpointer, ctx->chunks) < 0) {
            PyErr_Clear();
            uintptr_t address_of_code_object = 0;
            if (parse_frame_object(unwinder, &frame, frame_addr, &address_of_code_object, &next_frame_addr) < 0) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse frame object in chain");
                return -1;
            }
        }

        // Skip first frame if requested (used for cache miss continuation)
        if (ctx->skip_first_frame && frame_count == 1) {
            Py_XDECREF(frame);
            frame_addr = next_frame_addr;
            continue;
        }

        if (frame == NULL && PyList_GET_SIZE(ctx->frame_info) == 0) {
            const char *e = "Failed to parse initial frame in chain";
            PyErr_SetString(PyExc_RuntimeError, e);
            return -1;
        }
        PyObject *extra_frame = NULL;
        if (unwinder->gc && frame_addr == ctx->gc_frame) {
            _Py_DECLARE_STR(gc, "<GC>");
            extra_frame = &_Py_STR(gc);
        }
        else if (unwinder->native &&
                 frame == NULL &&
                 next_frame_addr &&
                 !(unwinder->gc && next_frame_addr == ctx->gc_frame))
        {
            _Py_DECLARE_STR(native, "<native>");
            extra_frame = &_Py_STR(native);
        }
        if (extra_frame) {
            PyObject *extra_frame_info = make_frame_info(
                unwinder, _Py_LATIN1_CHR('~'), Py_None, extra_frame, Py_None);
            if (extra_frame_info == NULL) {
                return -1;
            }
            if (PyList_Append(ctx->frame_info, extra_frame_info) < 0) {
                Py_DECREF(extra_frame_info);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append extra frame");
                return -1;
            }
            if (ctx->frame_addrs && ctx->num_addrs < ctx->max_addrs) {
                assert(ctx->num_addrs >= 0);
                ctx->frame_addrs[ctx->num_addrs++] = 0;
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

            if (PyList_Append(ctx->frame_info, frame) < 0) {
                Py_DECREF(frame);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append frame");
                return -1;
            }
            if (ctx->frame_addrs && ctx->num_addrs < ctx->max_addrs) {
                assert(ctx->num_addrs >= 0);
                ctx->frame_addrs[ctx->num_addrs++] = frame_addr;
            }
            Py_DECREF(frame);
        }

        if (ctx->last_profiled_frame != 0 && frame_addr == ctx->last_profiled_frame) {
            ctx->stopped_at_cached_frame = 1;
            break;
        }

        prev_frame_addr = next_frame_addr;
        frame_addr = next_frame_addr;
    }

    if (!ctx->stopped_at_cached_frame && ctx->base_frame_addr != 0 && last_frame_addr != ctx->base_frame_addr) {
        PyErr_Format(PyExc_RuntimeError,
            "Incomplete sample: did not reach base frame (expected 0x%lx, got 0x%lx)",
            ctx->base_frame_addr, last_frame_addr);
        return -1;
    }

    ctx->last_frame_visited = last_frame_addr;

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
    const size_t MAX_INTERPRETERS = 256;
    size_t interp_count = 0;

    while (current_interp != 0 && interp_count < MAX_INTERPRETERS) {
        interp_count++;
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
        const size_t MAX_THREADS_PER_INTERP = 8192;
        size_t thread_count = 0;
        while (tstate_addr != 0 && thread_count < MAX_THREADS_PER_INTERP) {
            thread_count++;
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
    const FrameWalkContext *ctx,
    uint64_t thread_id)
{
    if (!unwinder->frame_cache || ctx->last_profiled_frame == 0) {
        return 0;
    }
    if (ctx->frame_addr != ctx->last_profiled_frame) {
        return 0;
    }

    FrameCacheEntry *entry = frame_cache_find(unwinder, thread_id);
    if (!entry || !entry->frame_list) {
        return 0;
    }

    if (entry->num_addrs == 0 || entry->addrs[0] != ctx->frame_addr) {
        return 0;
    }

    PyObject *current_frame = NULL;
    uintptr_t code_object_addr = 0;
    uintptr_t previous_frame = 0;
    int parse_result = parse_frame_object(unwinder, &current_frame, ctx->frame_addr,
                                          &code_object_addr, &previous_frame);
    if (parse_result < 0) {
        return -1;
    }

    Py_ssize_t cached_size = PyList_GET_SIZE(entry->frame_list);
    PyObject *parent_slice = NULL;
    if (cached_size > 1) {
        parent_slice = PyList_GetSlice(entry->frame_list, 1, cached_size);
        if (!parent_slice) {
            Py_XDECREF(current_frame);
            return -1;
        }
    }

    if (current_frame != NULL) {
        if (PyList_Append(ctx->frame_info, current_frame) < 0) {
            Py_DECREF(current_frame);
            Py_XDECREF(parent_slice);
            return -1;
        }
        Py_DECREF(current_frame);
        STATS_ADD(unwinder, frames_read_from_memory, 1);
    }

    if (parent_slice) {
        Py_ssize_t cur_size = PyList_GET_SIZE(ctx->frame_info);
        int result = PyList_SetSlice(ctx->frame_info, cur_size, cur_size, parent_slice);
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
    FrameWalkContext *ctx,
    uint64_t thread_id)
{
    int full_hit = try_full_cache_hit(unwinder, ctx, thread_id);
    if (full_hit != 0) {
        return full_hit < 0 ? -1 : 0;
    }

    Py_ssize_t frames_before = PyList_GET_SIZE(ctx->frame_info);

    if (process_frame_chain(unwinder, ctx) < 0) {
        return -1;
    }

    STATS_ADD(unwinder, frames_read_from_memory, PyList_GET_SIZE(ctx->frame_info) - frames_before);

    if (ctx->stopped_at_cached_frame) {
        Py_ssize_t frames_before_cache = PyList_GET_SIZE(ctx->frame_info);
        int cache_result = frame_cache_lookup_and_extend(unwinder, thread_id, ctx->last_profiled_frame,
                                                         ctx->frame_info, ctx->frame_addrs, &ctx->num_addrs,
                                                         ctx->max_addrs);
        if (cache_result < 0) {
            return -1;
        }
        if (cache_result == 0) {
            STATS_INC(unwinder, frame_cache_misses);

            // Continue walking from last_profiled_frame, skipping it (already processed)
            Py_ssize_t frames_before_walk = PyList_GET_SIZE(ctx->frame_info);
            FrameWalkContext continue_ctx = {
                .frame_addr = ctx->last_profiled_frame,
                .base_frame_addr = ctx->base_frame_addr,
                .gc_frame = ctx->gc_frame,
                .chunks = ctx->chunks,
                .skip_first_frame = 1,
                .frame_info = ctx->frame_info,
                .frame_addrs = ctx->frame_addrs,
                .num_addrs = ctx->num_addrs,
                .max_addrs = ctx->max_addrs,
            };
            if (process_frame_chain(unwinder, &continue_ctx) < 0) {
                return -1;
            }
            ctx->num_addrs = continue_ctx.num_addrs;
            ctx->last_frame_visited = continue_ctx.last_frame_visited;
            STATS_ADD(unwinder, frames_read_from_memory, PyList_GET_SIZE(ctx->frame_info) - frames_before_walk);
        } else {
            // Partial cache hit - cached stack was validated as complete when stored,
            // so set last_frame_visited to base_frame_addr for validation in frame_cache_store
            ctx->last_frame_visited = ctx->base_frame_addr;
            STATS_INC(unwinder, frame_cache_partial_hits);
            STATS_ADD(unwinder, frames_read_from_cache, PyList_GET_SIZE(ctx->frame_info) - frames_before_cache);
        }
    } else {
        if (ctx->last_profiled_frame == 0) {
            STATS_INC(unwinder, frame_cache_misses);
        }
    }

    if (frame_cache_store(unwinder, thread_id, ctx->frame_info, ctx->frame_addrs, ctx->num_addrs,
                          ctx->base_frame_addr, ctx->last_frame_visited) < 0) {
        return -1;
    }

    return 0;
}
