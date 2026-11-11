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

int
copy_stack_chunks(RemoteUnwinderObject *unwinder,
                  uintptr_t tstate_addr,
                  StackChunkList *out_chunks)
{
    // Control records are contiguous from the base record to the current one.
    uintptr_t base;
    uintptr_t top;
    if (read_ptr(unwinder, tstate_addr + (uintptr_t)unwinder->debug_offsets.thread_state.base_frame, &base) ||
        read_ptr(unwinder, tstate_addr + (uintptr_t)unwinder->debug_offsets.thread_state.current_frame, &top)) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read control stack bounds");
        return -1;
    }
    size_t record_size = (size_t)unwinder->debug_offsets.interpreter_frame.size;
    if (base == 0 || top < base || (top - base) % record_size != 0 ||
        top - base >= MAX_STACK_CHUNK_SIZE) {
        PyErr_Format(PyExc_RuntimeError,
            "Invalid control stack bounds 0x%lx-0x%lx (corrupted remote memory)", base, top);
        set_exception_cause(unwinder, PyExc_RuntimeError,
            "Invalid control stack bounds (corrupted remote memory)");
        return -1;
    }
    size_t size = top - base + record_size;

    StackChunkInfo *chunks = PyMem_RawMalloc(sizeof(StackChunkInfo));
    char *copy = PyMem_RawMalloc(size);
    if (!chunks || !copy) {
        PyMem_RawFree(chunks);
        PyMem_RawFree(copy);
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate control stack copy");
        return -1;
    }
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, base, size, copy) < 0) {
        PyMem_RawFree(chunks);
        PyMem_RawFree(copy);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read control stack");
        return -1;
    }
    chunks[0].remote_addr = base;
    chunks[0].size = size;
    chunks[0].local_copy = copy;
    out_chunks->chunks = chunks;
    out_chunks->count = 1;
    return 0;
}

void *
find_frame_in_chunks(StackChunkList *chunks, uintptr_t remote_ptr)
{
    for (size_t i = 0; i < chunks->count; ++i) {
        uintptr_t base = chunks->chunks[i].remote_addr;
        size_t size = chunks->chunks[i].size;
        if (size >= SIZEOF_INTERP_FRAME &&
                remote_ptr >= base &&
                remote_ptr <= base + size - SIZEOF_INTERP_FRAME) {
            return (char *)chunks->chunks[i].local_copy + (remote_ptr - base);
        }
    }
    return NULL;
}

/* ============================================================================
 * FRAME PARSING FUNCTIONS
 * ============================================================================ */

enum { FRAME_PARSE_INTERPRETER = 2, FRAME_PARSE_NATIVE = 3 };

/* Control records are contiguous: a record's caller is the nearest record
 * below it that is not reserved. Base and detached records have no caller. */
static uint8_t
parse_record_link(
    RemoteUnwinderObject *unwinder,
    const char *frame,
    uintptr_t address,
    uintptr_t *previous_frame
) {
    uint8_t state = GET_MEMBER(uint8_t, frame, unwinder->debug_offsets.interpreter_frame.state);
    if (state == CONTROL_DETACHED || state == CONTROL_BASE) {
        *previous_frame = 0;
    }
    else {
        *previous_frame = address - unwinder->debug_offsets.interpreter_frame.size;
    }
    return state;
}

int
is_frame_valid(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    uintptr_t code_object_addr
) {
    if ((void*)code_object_addr == NULL) {
        return 0;  // Frame being cleared
    }

    void* frame = (void*)frame_addr;

    char owner = GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner);
    if (owner == FRAME_OWNED_BY_INTERPRETER) {
        return FRAME_PARSE_INTERPRETER;  // C frame or sentinel base frame
    }

    if (owner != FRAME_OWNED_BY_GENERATOR && owner != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n", owner);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Unhandled frame owner type in async frame");
        return -1;
    }
    return 1;
}

static int
parse_frame_buffer(
    RemoteUnwinderObject *unwinder,
    PyObject** result,
    const char *frame,
    uintptr_t address,
    uintptr_t* address_of_code_object,
    uintptr_t* previous_frame
) {
    *address_of_code_object = 0;

    uint8_t state = parse_record_link(unwinder, frame, address, previous_frame);
    if (state == CONTROL_RESERVED) {
        return 0;
    }
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
    int res = parse_code_object(unwinder, result, &code_ctx);
    return res == 0 && state == CONTROL_NATIVE ? FRAME_PARSE_NATIVE : res;
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
    Py_ssize_t bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
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

    return parse_frame_buffer(unwinder, result, frame, address, address_of_code_object, previous_frame);
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
        PyErr_Format(PyExc_RuntimeError, "Frame at address 0x%lx not found in stack chunks", address);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Frame not found in stack chunks");
        return -1;
    }

    char *frame = (char *)frame_ptr;
    uint8_t state = parse_record_link(unwinder, frame, address, previous_frame);
    if (state == CONTROL_RESERVED) {
        return 0;
    }
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
    int res = parse_code_object(unwinder, result, &code_ctx);
    return res == 0 && state == CONTROL_NATIVE ? FRAME_PARSE_NATIVE : res;
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
    size_t frame_count = 0;
    int pending_native = 0;

    ctx->stopped_at_cached_frame = 0;
    ctx->last_frame_visited = 0;

    while ((void*)frame_addr != NULL) {
        int parse_result = 0;
        PyObject *frame = NULL;
        uintptr_t next_frame_addr = 0;
        uintptr_t stackpointer = 0;
        last_frame_addr = frame_addr;

        if (++frame_count > MAX_FRAME_CHAIN_DEPTH) {
            PyErr_SetString(PyExc_RuntimeError, "Too many stack frames (possible infinite loop)");
            set_exception_cause(unwinder, PyExc_RuntimeError, "Frame chain iteration limit exceeded");
            return -1;
        }
        assert(frame_count <= MAX_FRAME_CHAIN_DEPTH);

        if (ctx->chunks && ctx->chunks->count > 0) {
            parse_result = parse_frame_from_chunks(
                unwinder, &frame, frame_addr, &next_frame_addr, &stackpointer, ctx->chunks);
            if (parse_result == 0 || parse_result == FRAME_PARSE_NATIVE) {
                goto parsed_frame;
            }
            PyErr_Clear();
        }
        {
            uintptr_t address_of_code_object = 0;
            if (ctx->prefetch.frame && ctx->prefetch.frame_addr == frame_addr) {
                parse_result = parse_frame_buffer(
                    unwinder, &frame, ctx->prefetch.frame, frame_addr,
                    &address_of_code_object, &next_frame_addr);
            }
            else {
                parse_result = parse_frame_object(
                    unwinder, &frame, frame_addr,
                    &address_of_code_object, &next_frame_addr);
            }
            if (parse_result < 0) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse frame object in chain");
                return -1;
            }
        }

parsed_frame:
        // Skip first frame if requested (used for cache miss continuation)
        if (ctx->skip_first_frame && frame_count == 1) {
            pending_native = frame != NULL &&
                parse_result == FRAME_PARSE_NATIVE &&
                PyList_GET_SIZE(ctx->frame_info) > 0;
            Py_XDECREF(frame);
            frame_addr = next_frame_addr;
            continue;
        }

        PyObject *extra_frame = NULL;
        if (unwinder->gc && frame_addr == ctx->gc_frame) {
            _Py_DECLARE_STR(gc, "<GC>");
            extra_frame = &_Py_STR(gc);
        }
        // A record entered from C is called by native code.
        else if (unwinder->native && pending_native) {
            _Py_DECLARE_STR(native, "<native>");
            extra_frame = &_Py_STR(native);
        }
        pending_native = 0;
        if (extra_frame) {
            PyObject *extra_frame_info = make_frame_info(
                unwinder, _Py_LATIN1_CHR('~'), Py_None, extra_frame, Py_None);
            if (extra_frame_info == NULL) {
                Py_XDECREF(frame);
                return -1;
            }
            if (PyList_Append(ctx->frame_info, extra_frame_info) < 0) {
                Py_DECREF(extra_frame_info);
                Py_XDECREF(frame);
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
            pending_native = parse_result == FRAME_PARSE_NATIVE;
        }

        if (ctx->last_profiled.frame != 0 && frame_addr == ctx->last_profiled.frame) {
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

int
set_last_profiled_frame(RemoteUnwinderObject *unwinder, uintptr_t tstate_addr,
                        uintptr_t frame_addr)
{
    uintptr_t lpf_addr = tstate_addr +
        (uintptr_t)unwinder->debug_offsets.thread_state.last_profiled_frame;
    return _Py_RemoteDebug_WriteRemoteMemory(&unwinder->handle, lpf_addr,
                                             sizeof(uintptr_t), &frame_addr);
}

// Clear the profiler anchor frame for all threads in the target process.  The
// sequence is intentionally preserved: a zero frame disables cache lookup, and
// the next profiler-owned anchor should use the target's current generation.
int
clear_last_profiled_frames(RemoteUnwinderObject *unwinder)
{
    uintptr_t current_interp = unwinder->interpreter_addr;
    const size_t MAX_INTERPRETERS = 256;
    size_t interp_count = 0;

    while (current_interp != 0 && interp_count < MAX_INTERPRETERS) {
        interp_count++;
        // Get first thread in this interpreter
        uintptr_t tstate_addr;
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                current_interp + (uintptr_t)unwinder->debug_offsets.interpreter_state.threads_head,
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
            uintptr_t no_frame = 0;
            if (set_last_profiled_frame(unwinder, tstate_addr, no_frame) < 0) {
                PyErr_Clear();
            }

            // Move to next thread
            if (_Py_RemoteDebug_PagedReadRemoteMemory(
                    &unwinder->handle,
                    tstate_addr + (uintptr_t)unwinder->debug_offsets.thread_state.next,
                    sizeof(void*),
                    &tstate_addr) < 0) {
                PyErr_Clear();
                break;
            }
        }

        // Move to next interpreter
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                current_interp + (uintptr_t)unwinder->debug_offsets.interpreter_state.next,
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
    if (!unwinder->frame_cache || ctx->last_profiled.frame == 0) {
        return 0;
    }
    if (ctx->frame_addr != ctx->last_profiled.frame) {
        return 0;
    }

    FrameCacheEntry *entry = frame_cache_find(unwinder, thread_id);
    if (!entry || !entry->frame_list) {
        return 0;
    }
    if (entry->thread_state_addr != ctx->thread_state_addr) {
        return 0;
    }

    if (entry->num_addrs == 0 || entry->addrs[0] != ctx->frame_addr) {
        return 0;
    }
    if (entry->last_profiled_frame_seq != ctx->last_profiled.seq) {
        return 0;
    }

    PyObject *current_frame = NULL;
    uintptr_t code_object_addr = 0;
    uintptr_t previous_frame = 0;
    int parse_result;
    if (ctx->prefetch.frame && ctx->prefetch.frame_addr == ctx->frame_addr) {
        parse_result = parse_frame_buffer(unwinder, &current_frame,
                                          ctx->prefetch.frame, ctx->frame_addr,
                                          &code_object_addr, &previous_frame);
    }
    else {
        parse_result = parse_frame_object(unwinder, &current_frame, ctx->frame_addr,
                                          &code_object_addr, &previous_frame);
    }
    if (parse_result < 0) {
        return -1;
    }
    if (!frame_cache_anchor_matches(unwinder, ctx->thread_state_addr,
                                    ctx->last_profiled)) {
        Py_XDECREF(current_frame);
        return 0;
    }

    if (current_frame != NULL) {
        if (PyList_Append(ctx->frame_info, current_frame) < 0) {
            Py_DECREF(current_frame);
            return -1;
        }
        Py_DECREF(current_frame);
        STATS_ADD(unwinder, frames_read_from_memory, 1);
    }

    Py_ssize_t cached_size = PyList_GET_SIZE(entry->frame_list);
    for (Py_ssize_t i = 1; i < cached_size; i++) {
        PyObject *cached_frame = PyList_GET_ITEM(entry->frame_list, i);
        if (PyList_Append(ctx->frame_info, cached_frame) < 0) {
            return -1;
        }
    }
    STATS_ADD(unwinder, frames_read_from_cache, cached_size > 1 ? cached_size - 1 : 0);

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

    assert(ctx->chunks != NULL);

    // Cache misses copy stack chunks before walking. Frames found there are
    // parsed from a stable snapshot, which keeps moving stacks from seeding the
    // cache with an impossible parent chain.
    if (ctx->chunks->count == 0) {
        if (copy_stack_chunks(unwinder, ctx->thread_state_addr, ctx->chunks) < 0) {
            return -1;
        }
    }

    Py_ssize_t frames_before = PyList_GET_SIZE(ctx->frame_info);

    if (process_frame_chain(unwinder, ctx) < 0) {
        return -1;
    }

    STATS_ADD(unwinder, frames_read_from_memory, PyList_GET_SIZE(ctx->frame_info) - frames_before);

    if (ctx->stopped_at_cached_frame) {
        Py_ssize_t frames_before_cache = PyList_GET_SIZE(ctx->frame_info);
        int cache_result = frame_cache_lookup_and_extend(unwinder, thread_id,
                                                         ctx->thread_state_addr,
                                                         ctx->last_profiled,
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
                .frame_addr = ctx->last_profiled.frame,
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
        if (ctx->last_profiled.frame == 0) {
            STATS_INC(unwinder, frame_cache_misses);
        }
    }

    if (frame_cache_store(unwinder, thread_id, ctx->frame_info, ctx->frame_addrs,
                          ctx->num_addrs, ctx->thread_state_addr,
                          ctx->last_profiled.seq, ctx->base_frame_addr,
                          ctx->last_frame_visited) < 0) {
        return -1;
    }

    return 0;
}
