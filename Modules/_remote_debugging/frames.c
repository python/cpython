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
    uintptr_t gc_frame,
    uintptr_t last_profiled_frame,
    int *stopped_at_cached_frame,
    PyObject *frame_addresses)  // optional: list to receive frame addresses
{
    uintptr_t frame_addr = initial_frame_addr;
    uintptr_t prev_frame_addr = 0;
    const size_t MAX_FRAMES = 1024;
    size_t frame_count = 0;

    // Initialize output flag
    if (stopped_at_cached_frame) {
        *stopped_at_cached_frame = 0;
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
            PyObject *extra_frame_info = make_frame_info(
                unwinder, _Py_LATIN1_CHR('~'), _PyLong_GetZero(), extra_frame);
            if (extra_frame_info == NULL) {
                return -1;
            }
            if (PyList_Append(frame_info, extra_frame_info) < 0) {
                Py_DECREF(extra_frame_info);
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append extra frame");
                return -1;
            }
            // Extra frames use 0 as address (they're synthetic)
            if (frame_addresses) {
                PyObject *addr_obj = PyLong_FromUnsignedLongLong(0);
                if (!addr_obj || PyList_Append(frame_addresses, addr_obj) < 0) {
                    Py_XDECREF(addr_obj);
                    Py_DECREF(extra_frame_info);
                    return -1;
                }
                Py_DECREF(addr_obj);
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
            if (frame_addresses) {
                PyObject *addr_obj = PyLong_FromUnsignedLongLong(frame_addr);
                if (!addr_obj || PyList_Append(frame_addresses, addr_obj) < 0) {
                    Py_XDECREF(addr_obj);
                    Py_DECREF(frame);
                    return -1;
                }
                Py_DECREF(addr_obj);
            }
            Py_DECREF(frame);
        }

        prev_frame_addr = next_frame_addr;
        frame_addr = next_frame_addr;
    }

    return 0;
}

/* ============================================================================
 * FRAME CACHE - stores (address, frame_info) pairs per thread
 * ============================================================================ */

int
frame_cache_init(RemoteUnwinderObject *unwinder)
{
    unwinder->frame_cache = PyDict_New();
    return unwinder->frame_cache ? 0 : -1;
}

void
frame_cache_cleanup(RemoteUnwinderObject *unwinder)
{
    Py_CLEAR(unwinder->frame_cache);
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

    // Build set of seen thread IDs from result
    PyObject *seen = PySet_New(NULL);
    if (!seen) {
        PyErr_Clear();
        return;
    }

    // Iterate: result -> interpreters -> threads -> thread_id
    Py_ssize_t num_interps = PyList_GET_SIZE(result);
    for (Py_ssize_t i = 0; i < num_interps; i++) {
        PyObject *interp_info = PyList_GET_ITEM(result, i);
        // InterpreterInfo[1] is the threads list
        PyObject *threads = PyStructSequence_GetItem(interp_info, 1);
        if (!threads || !PyList_Check(threads)) {
            continue;
        }
        Py_ssize_t num_threads = PyList_GET_SIZE(threads);
        for (Py_ssize_t j = 0; j < num_threads; j++) {
            PyObject *thread_info = PyList_GET_ITEM(threads, j);
            // ThreadInfo[0] is the thread_id
            PyObject *thread_id = PyStructSequence_GetItem(thread_info, 0);
            if (thread_id && PySet_Add(seen, thread_id) < 0) {
                PyErr_Clear();
            }
        }
    }

    // Collect keys to remove (can't modify dict while iterating)
    PyObject *keys_to_remove = PyList_New(0);
    if (!keys_to_remove) {
        Py_DECREF(seen);
        PyErr_Clear();
        return;
    }

    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(unwinder->frame_cache, &pos, &key, &value)) {
        int contains = PySet_Contains(seen, key);
        if (contains == 0) {
            // Thread ID not seen in current sample, mark for removal
            if (PyList_Append(keys_to_remove, key) < 0) {
                PyErr_Clear();
                break;
            }
        } else if (contains < 0) {
            PyErr_Clear();
        }
    }

    // Remove stale entries
    Py_ssize_t num_to_remove = PyList_GET_SIZE(keys_to_remove);
    for (Py_ssize_t i = 0; i < num_to_remove; i++) {
        PyObject *k = PyList_GET_ITEM(keys_to_remove, i);
        if (PyDict_DelItem(unwinder->frame_cache, k) < 0) {
            PyErr_Clear();
        }
    }

    Py_DECREF(keys_to_remove);
    Py_DECREF(seen);
}

// Find last_profiled_frame in cache and extend frame_info with cached continuation
// Cache format: tuple of (bytes_of_addresses, frame_list)
// - bytes_of_addresses: raw uintptr_t values packed as bytes
// - frame_list: Python list of frame info tuples
// If frame_addresses is provided (not NULL), also extends it with cached addresses
int
frame_cache_lookup_and_extend(
    RemoteUnwinderObject *unwinder,
    uint64_t thread_id,
    uintptr_t last_profiled_frame,
    PyObject *frame_info,
    PyObject *frame_addresses)
{
    if (!unwinder->frame_cache || last_profiled_frame == 0) {
        return 0;
    }

    PyObject *key = PyLong_FromUnsignedLongLong(thread_id);
    if (!key) {
        PyErr_Clear();
        return 0;
    }

    PyObject *cached = PyDict_GetItemWithError(unwinder->frame_cache, key);
    Py_DECREF(key);
    if (!cached || PyErr_Occurred()) {
        PyErr_Clear();
        return 0;
    }

    // cached is tuple of (addresses_bytes, frame_list)
    if (!PyTuple_Check(cached) || PyTuple_GET_SIZE(cached) != 2) {
        return 0;
    }

    PyObject *addr_bytes = PyTuple_GET_ITEM(cached, 0);
    PyObject *frame_list = PyTuple_GET_ITEM(cached, 1);

    if (!PyBytes_Check(addr_bytes) || !PyList_Check(frame_list)) {
        return 0;
    }

    const uintptr_t *addrs = (const uintptr_t *)PyBytes_AS_STRING(addr_bytes);
    Py_ssize_t num_addrs = PyBytes_GET_SIZE(addr_bytes) / sizeof(uintptr_t);
    Py_ssize_t num_frames = PyList_GET_SIZE(frame_list);

    // Find the index where last_profiled_frame matches
    Py_ssize_t start_idx = -1;
    for (Py_ssize_t i = 0; i < num_addrs; i++) {
        if (addrs[i] == last_profiled_frame) {
            start_idx = i;
            break;
        }
    }

    if (start_idx < 0) {
        return 0;  // Not found
    }

    // Extend frame_info with frames from start_idx onwards
    // Use list slice for efficiency: frame_info.extend(frame_list[start_idx:])
    PyObject *slice = PyList_GetSlice(frame_list, start_idx, num_frames);
    if (!slice) {
        return -1;
    }

    // Extend frame_info with the slice using SetSlice at the end
    Py_ssize_t cur_size = PyList_GET_SIZE(frame_info);
    int result = PyList_SetSlice(frame_info, cur_size, cur_size, slice);
    Py_DECREF(slice);

    if (result < 0) {
        return -1;
    }

    // Also extend frame_addresses with cached addresses if provided
    if (frame_addresses) {
        for (Py_ssize_t i = start_idx; i < num_addrs; i++) {
            PyObject *addr_obj = PyLong_FromUnsignedLongLong(addrs[i]);
            if (!addr_obj) {
                return -1;
            }
            if (PyList_Append(frame_addresses, addr_obj) < 0) {
                Py_DECREF(addr_obj);
                return -1;
            }
            Py_DECREF(addr_obj);
        }
    }

    return 1;
}

// Store frame list with addresses in cache
// Stores as tuple of (addresses_bytes, frame_list)
int
frame_cache_store(
    RemoteUnwinderObject *unwinder,
    uint64_t thread_id,
    PyObject *frame_list,
    const uintptr_t *addrs,
    Py_ssize_t num_addrs)
{
    if (!unwinder->frame_cache) {
        return 0;
    }

    PyObject *key = PyLong_FromUnsignedLongLong(thread_id);
    if (!key) {
        PyErr_Clear();
        return 0;
    }

    // Create bytes object from addresses array
    PyObject *addr_bytes = PyBytes_FromStringAndSize(
        (const char *)addrs, num_addrs * sizeof(uintptr_t));
    if (!addr_bytes) {
        Py_DECREF(key);
        PyErr_Clear();
        return 0;
    }

    // Create tuple (addr_bytes, frame_list)
    PyObject *cache_entry = PyTuple_Pack(2, addr_bytes, frame_list);
    Py_DECREF(addr_bytes);
    if (!cache_entry) {
        Py_DECREF(key);
        PyErr_Clear();
        return 0;
    }

    int result = PyDict_SetItem(unwinder->frame_cache, key, cache_entry);
    Py_DECREF(key);
    Py_DECREF(cache_entry);
    if (result < 0) {
        PyErr_Clear();
    }
    return 0;
}

// Fast path: check if we have a full cache hit (entire stack unchanged)
// Returns: 1 if full hit (frame_info populated), 0 if miss, -1 on error
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

    PyObject *key = PyLong_FromUnsignedLongLong(thread_id);
    if (!key) {
        PyErr_Clear();
        return 0;
    }

    PyObject *cached = PyDict_GetItemWithError(unwinder->frame_cache, key);
    Py_DECREF(key);
    if (!cached || PyErr_Occurred()) {
        PyErr_Clear();
        return 0;
    }

    if (!PyTuple_Check(cached) || PyTuple_GET_SIZE(cached) != 2) {
        return 0;
    }

    PyObject *addr_bytes = PyTuple_GET_ITEM(cached, 0);
    PyObject *frame_list = PyTuple_GET_ITEM(cached, 1);

    if (!PyBytes_Check(addr_bytes) || !PyList_Check(frame_list)) {
        return 0;
    }

    // Verify first address matches (sanity check)
    const uintptr_t *addrs = (const uintptr_t *)PyBytes_AS_STRING(addr_bytes);
    Py_ssize_t num_addrs = PyBytes_GET_SIZE(addr_bytes) / sizeof(uintptr_t);
    if (num_addrs == 0 || addrs[0] != frame_addr) {
        return 0;
    }

    // Full hit! Extend frame_info with entire cached list
    Py_ssize_t cur_size = PyList_GET_SIZE(frame_info);
    int result = PyList_SetSlice(frame_info, cur_size, cur_size, frame_list);
    return result < 0 ? -1 : 1;
}

// High-level helper: collect frames with cache optimization
// Returns complete frame_info list, handling all cache logic internally
int
collect_frames_with_cache(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    StackChunkList *chunks,
    PyObject *frame_info,
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

    // Slow path: need to walk frames
    PyObject *frame_addresses = PyList_New(0);
    if (!frame_addresses) {
        return -1;
    }

    int stopped_at_cached = 0;
    if (process_frame_chain(unwinder, frame_addr, chunks, frame_info, gc_frame,
                            last_profiled_frame, &stopped_at_cached, frame_addresses) < 0) {
        Py_DECREF(frame_addresses);
        return -1;
    }

    // If stopped at cached frame, extend with cached continuation (both frames and addresses)
    if (stopped_at_cached) {
        int cache_result = frame_cache_lookup_and_extend(unwinder, thread_id, last_profiled_frame,
                                                         frame_info, frame_addresses);
        if (cache_result < 0) {
            Py_DECREF(frame_addresses);
            return -1;
        }
    }

    // Convert frame_addresses (list of PyLong) to C array for efficient cache storage
    Py_ssize_t num_addrs = PyList_GET_SIZE(frame_addresses);

    // After extending from cache, frames and addresses should be in sync
    assert(PyList_GET_SIZE(frame_info) == num_addrs);

    // Allocate C array for addresses
    uintptr_t *addrs = (uintptr_t *)PyMem_Malloc(num_addrs * sizeof(uintptr_t));
    if (!addrs) {
        Py_DECREF(frame_addresses);
        PyErr_NoMemory();
        return -1;
    }

    // Fill addresses from the Python list
    for (Py_ssize_t i = 0; i < num_addrs; i++) {
        PyObject *addr_obj = PyList_GET_ITEM(frame_addresses, i);
        addrs[i] = PyLong_AsUnsignedLongLong(addr_obj);
        if (PyErr_Occurred()) {
            PyErr_Clear();
            addrs[i] = 0;
        }
    }

    // Store in cache
    frame_cache_store(unwinder, thread_id, frame_info, addrs, num_addrs);

    PyMem_Free(addrs);
    Py_DECREF(frame_addresses);

    return 0;
}
