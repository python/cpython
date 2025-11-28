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

    if (GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner) == FRAME_OWNED_BY_INTERPRETER) {
        return 0;  // C frame
    }

    if (GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner) != FRAME_OWNED_BY_GENERATOR
        && GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner) != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n",
                    GET_MEMBER(char, frame, unwinder->debug_offsets.interpreter_frame.owner));
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
    uintptr_t gc_frame)
{
    uintptr_t frame_addr = initial_frame_addr;
    uintptr_t prev_frame_addr = 0;
    const size_t MAX_FRAMES = 1024;
    size_t frame_count = 0;

    while ((void*)frame_addr != NULL) {
        PyObject *frame = NULL;
        uintptr_t next_frame_addr = 0;
        uintptr_t stackpointer = 0;

        if (++frame_count > MAX_FRAMES) {
            PyErr_SetString(PyExc_RuntimeError, "Too many stack frames (possible infinite loop)");
            set_exception_cause(unwinder, PyExc_RuntimeError, "Frame chain iteration limit exceeded");
            return -1;
        }

        // Try chunks first, fallback to direct memory read
        if (parse_frame_from_chunks(unwinder, &frame, frame_addr, &next_frame_addr, &stackpointer, chunks) < 0) {
            PyErr_Clear();
            uintptr_t address_of_code_object = 0;
            if (parse_frame_object(unwinder, &frame, frame_addr, &address_of_code_object ,&next_frame_addr) < 0) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse frame object in chain");
                return -1;
            }
        }
        if (frame == NULL && PyList_GET_SIZE(frame_info) == 0) {
            // If the first frame is missing, the chain is broken:
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
            // Use "~" as file and 0 as line, since that's what pstats uses:
            PyObject *extra_frame_info = make_frame_info(
                unwinder, _Py_LATIN1_CHR('~'), _PyLong_GetZero(), extra_frame);
            if (extra_frame_info == NULL) {
                return -1;
            }
            int error = PyList_Append(frame_info, extra_frame_info);
            Py_DECREF(extra_frame_info);
            if (error) {
                const char *e = "Failed to append extra frame to frame info list";
                set_exception_cause(unwinder, PyExc_RuntimeError, e);
                return -1;
            }
        }
        if (frame) {
            if (prev_frame_addr && frame_addr != prev_frame_addr) {
                const char *f = "Broken frame chain: expected frame at 0x%lx, got 0x%lx";
                PyErr_Format(PyExc_RuntimeError, f, prev_frame_addr, frame_addr);
                Py_DECREF(frame);
                const char *e = "Frame chain consistency check failed";
                set_exception_cause(unwinder, PyExc_RuntimeError, e);
                return -1;
            }

            if (PyList_Append(frame_info, frame) == -1) {
                Py_DECREF(frame);
                const char *e = "Failed to append frame to frame info list";
                set_exception_cause(unwinder, PyExc_RuntimeError, e);
                return -1;
            }
            Py_DECREF(frame);
        }

        prev_frame_addr = next_frame_addr;
        frame_addr = next_frame_addr;
    }

    return 0;
}
