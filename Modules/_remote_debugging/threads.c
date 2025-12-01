/******************************************************************************
 * Remote Debugging Module - Thread Functions
 *
 * This file contains functions for iterating threads and determining
 * thread status in remote process memory.
 ******************************************************************************/

#include "_remote_debugging.h"

#ifndef MS_WINDOWS
#include <unistd.h>
#endif

/* ============================================================================
 * THREAD ITERATION FUNCTIONS
 * ============================================================================ */

int
iterate_threads(
    RemoteUnwinderObject *unwinder,
    thread_processor_func processor,
    void *context
) {
    uintptr_t thread_state_addr;
    unsigned long tid = 0;

    if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                unwinder->interpreter_addr + (uintptr_t)unwinder->debug_offsets.interpreter_state.threads_main,
                sizeof(void*),
                &thread_state_addr))
    {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read main thread state");
        return -1;
    }

    while (thread_state_addr != 0) {
        if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                    &unwinder->handle,
                    thread_state_addr + (uintptr_t)unwinder->debug_offsets.thread_state.native_thread_id,
                    sizeof(tid),
                    &tid))
        {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read thread ID");
            return -1;
        }

        // Call the processor function for this thread
        if (processor(unwinder, thread_state_addr, tid, context) < 0) {
            return -1;
        }

        // Move to next thread
        if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                    &unwinder->handle,
                    thread_state_addr + (uintptr_t)unwinder->debug_offsets.thread_state.next,
                    sizeof(void*),
                    &thread_state_addr))
        {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read next thread state");
            return -1;
        }
    }

    return 0;
}

/* ============================================================================
 * INTERPRETER STATE AND THREAD DISCOVERY FUNCTIONS
 * ============================================================================ */

int
populate_initial_state_data(
    int all_threads,
    RemoteUnwinderObject *unwinder,
    uintptr_t runtime_start_address,
    uintptr_t *interpreter_state,
    uintptr_t *tstate
) {
    uintptr_t interpreter_state_list_head =
        (uintptr_t)unwinder->debug_offsets.runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            &unwinder->handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read interpreter state address");
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Interpreter state is NULL");
        return -1;
    }

    *interpreter_state = address_of_interpreter_state;

    if (all_threads) {
        *tstate = 0;
        return 0;
    }

    uintptr_t address_of_thread = address_of_interpreter_state +
            (uintptr_t)unwinder->debug_offsets.interpreter_state.threads_main;

    if (_Py_RemoteDebug_PagedReadRemoteMemory(
            &unwinder->handle,
            address_of_thread,
            sizeof(void*),
            tstate) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read main thread state address");
        return -1;
    }

    return 0;
}

int
find_running_frame(
    RemoteUnwinderObject *unwinder,
    uintptr_t address_of_thread,
    uintptr_t *frame
) {
    if ((void*)address_of_thread != NULL) {
        int err = read_ptr(
            unwinder,
            address_of_thread + (uintptr_t)unwinder->debug_offsets.thread_state.current_frame,
            frame);
        if (err) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read current frame pointer");
            return -1;
        }
        return 0;
    }

    *frame = (uintptr_t)NULL;
    return 0;
}

/* ============================================================================
 * THREAD STATUS FUNCTIONS
 * ============================================================================ */

int
get_thread_status(RemoteUnwinderObject *unwinder, uint64_t tid, uint64_t pthread_id) {
#if defined(__APPLE__) && TARGET_OS_OSX
   if (unwinder->thread_id_offset == 0) {
        uint64_t *tids = (uint64_t *)PyMem_Malloc(MAX_NATIVE_THREADS * sizeof(uint64_t));
        if (!tids) {
            PyErr_NoMemory();
            return -1;
        }
        int n = proc_pidinfo(unwinder->handle.pid, PROC_PIDLISTTHREADS, 0, tids, MAX_NATIVE_THREADS * sizeof(uint64_t)) / sizeof(uint64_t);
        if (n <= 0) {
            PyMem_Free(tids);
            return THREAD_STATE_UNKNOWN;
        }
        uint64_t min_offset = UINT64_MAX;
        for (int i = 0; i < n; i++) {
            uint64_t offset = tids[i] - pthread_id;
            if (offset < min_offset) {
                min_offset = offset;
            }
        }
        unwinder->thread_id_offset = min_offset;
        PyMem_Free(tids);
    }
    struct proc_threadinfo ti;
    uint64_t tid_with_offset = pthread_id + unwinder->thread_id_offset;
    if (proc_pidinfo(unwinder->handle.pid, PROC_PIDTHREADINFO, tid_with_offset, &ti, sizeof(ti)) != sizeof(ti)) {
        return THREAD_STATE_UNKNOWN;
    }
    if (ti.pth_run_state == TH_STATE_RUNNING) {
        return THREAD_STATE_RUNNING;
    }
    return THREAD_STATE_IDLE;
#elif defined(__linux__)
    char stat_path[256];
    char buffer[2048] = "";

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/task/%lu/stat", unwinder->handle.pid, tid);

    int fd = open(stat_path, O_RDONLY);
    if (fd == -1) {
        return THREAD_STATE_UNKNOWN;
    }

    if (read(fd, buffer, 2047) == 0) {
        close(fd);
        return THREAD_STATE_UNKNOWN;
    }
    close(fd);

    char *p = strchr(buffer, ')');
    if (!p) {
        return THREAD_STATE_UNKNOWN;
    }

    p += 2;  // Skip ") "
    if (*p == ' ') {
        p++;
    }

    switch (*p) {
        case 'R':  // Running
            return THREAD_STATE_RUNNING;
        case 'S':  // Interruptible sleep
        case 'D':  // Uninterruptible sleep
        case 'T':  // Stopped
        case 'Z':  // Zombie
        case 'I':  // Idle kernel thread
            return THREAD_STATE_IDLE;
        default:
            return THREAD_STATE_UNKNOWN;
    }
#elif defined(MS_WINDOWS)
    ULONG n;
    NTSTATUS status = NtQuerySystemInformation(
        SystemProcessInformation,
        unwinder->win_process_buffer,
        unwinder->win_process_buffer_size,
        &n
    );
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        // Buffer was too small so we reallocate a larger one and try again.
        unwinder->win_process_buffer_size = n;
        PVOID new_buffer = PyMem_Realloc(unwinder->win_process_buffer, n);
        if (!new_buffer) {
            return -1;
        }
        unwinder->win_process_buffer = new_buffer;
        return get_thread_status(unwinder, tid, pthread_id);
    }
    if (status != STATUS_SUCCESS) {
        return -1;
    }

    SYSTEM_PROCESS_INFORMATION *pi = (SYSTEM_PROCESS_INFORMATION *)unwinder->win_process_buffer;
    while ((ULONG)(ULONG_PTR)pi->UniqueProcessId != unwinder->handle.pid) {
        if (pi->NextEntryOffset == 0) {
            // We didn't find the process
            return -1;
        }
        pi = (SYSTEM_PROCESS_INFORMATION *)(((BYTE *)pi) + pi->NextEntryOffset);
    }

    SYSTEM_THREAD_INFORMATION *ti = (SYSTEM_THREAD_INFORMATION *)((char *)pi + sizeof(SYSTEM_PROCESS_INFORMATION));
    for (size_t i = 0; i < pi->NumberOfThreads; i++, ti++) {
        if (ti->ClientId.UniqueThread == (HANDLE)tid) {
            return ti->ThreadState != WIN32_THREADSTATE_RUNNING ? THREAD_STATE_IDLE : THREAD_STATE_RUNNING;
        }
    }

    return -1;
#else
    return THREAD_STATE_UNKNOWN;
#endif
}

/* ============================================================================
 * STACK UNWINDING FUNCTIONS
 * ============================================================================ */

typedef struct {
    unsigned int initialized:1;
    unsigned int bound:1;
    unsigned int unbound:1;
    unsigned int bound_gilstate:1;
    unsigned int active:1;
    unsigned int finalizing:1;
    unsigned int cleared:1;
    unsigned int finalized:1;
    unsigned int :24;
} _thread_status;

PyObject*
unwind_stack_for_thread(
    RemoteUnwinderObject *unwinder,
    uintptr_t *current_tstate,
    uintptr_t gil_holder_tstate,
    uintptr_t gc_frame
) {
    PyObject *frame_info = NULL;
    PyObject *thread_id = NULL;
    PyObject *result = NULL;
    StackChunkList chunks = {0};

    char ts[SIZEOF_THREAD_STATE];
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle, *current_tstate, (size_t)unwinder->debug_offsets.thread_state.size, ts);
    if (bytes_read < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read thread state");
        goto error;
    }

    long tid = GET_MEMBER(long, ts, unwinder->debug_offsets.thread_state.native_thread_id);

    // Read GC collecting state from the interpreter (before any skip checks)
    uintptr_t interp_addr = GET_MEMBER(uintptr_t, ts, unwinder->debug_offsets.thread_state.interp);

    // Read the GC runtime state from the interpreter state
    uintptr_t gc_addr = interp_addr + unwinder->debug_offsets.interpreter_state.gc;
    char gc_state[SIZEOF_GC_RUNTIME_STATE];
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, gc_addr, unwinder->debug_offsets.gc.size, gc_state) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read GC state");
        goto error;
    }

    // Calculate thread status using flags (always)
    int status_flags = 0;

    // Check GIL status
    int has_gil = 0;
    int gil_requested = 0;
#ifdef Py_GIL_DISABLED
    int active = GET_MEMBER(_thread_status, ts, unwinder->debug_offsets.thread_state.status).active;
    has_gil = active;
#else
    // Read holds_gil directly from thread state
    has_gil = GET_MEMBER(int, ts, unwinder->debug_offsets.thread_state.holds_gil);

    // Check if thread is actively requesting the GIL
    if (unwinder->debug_offsets.thread_state.gil_requested != 0) {
        gil_requested = GET_MEMBER(int, ts, unwinder->debug_offsets.thread_state.gil_requested);
    }

    // Set GIL_REQUESTED flag if thread is waiting
    if (!has_gil && gil_requested) {
        status_flags |= THREAD_STATUS_GIL_REQUESTED;
    }
#endif
    if (has_gil) {
        status_flags |= THREAD_STATUS_HAS_GIL;
    }

    // Assert that we never have both HAS_GIL and GIL_REQUESTED set at the same time
    // This would indicate a race condition in the GIL state tracking
    assert(!(has_gil && gil_requested));

    // Check CPU status
    long pthread_id = GET_MEMBER(long, ts, unwinder->debug_offsets.thread_state.thread_id);

    // Optimization: only check CPU status if needed by mode because it's expensive
    int cpu_status = -1;
    if (unwinder->mode == PROFILING_MODE_CPU || unwinder->mode == PROFILING_MODE_ALL) {
        cpu_status = get_thread_status(unwinder, tid, pthread_id);
    }

    if (cpu_status == -1) {
        status_flags |= THREAD_STATUS_UNKNOWN;
    } else if (cpu_status == THREAD_STATE_RUNNING) {
        status_flags |= THREAD_STATUS_ON_CPU;
    }

    // Check if we should skip this thread based on mode
    int should_skip = 0;
    if (unwinder->skip_non_matching_threads) {
        if (unwinder->mode == PROFILING_MODE_CPU) {
            // Skip if not on CPU
            should_skip = !(status_flags & THREAD_STATUS_ON_CPU);
        } else if (unwinder->mode == PROFILING_MODE_GIL) {
            // Skip if doesn't have GIL
            should_skip = !(status_flags & THREAD_STATUS_HAS_GIL);
        }
        // PROFILING_MODE_WALL and PROFILING_MODE_ALL never skip
    }

    if (should_skip) {
        // Advance to next thread and return NULL to skip processing
        *current_tstate = GET_MEMBER(uintptr_t, ts, unwinder->debug_offsets.thread_state.next);
        return NULL;
    }

    uintptr_t frame_addr = GET_MEMBER(uintptr_t, ts, unwinder->debug_offsets.thread_state.current_frame);

    frame_info = PyList_New(0);
    if (!frame_info) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create frame info list");
        goto error;
    }

    if (copy_stack_chunks(unwinder, *current_tstate, &chunks) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to copy stack chunks");
        goto error;
    }

    if (process_frame_chain(unwinder, frame_addr, &chunks, frame_info, gc_frame) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to process frame chain");
        goto error;
    }

    *current_tstate = GET_MEMBER(uintptr_t, ts, unwinder->debug_offsets.thread_state.next);

    thread_id = PyLong_FromLongLong(tid);
    if (thread_id == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create thread ID");
        goto error;
    }

    RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
    result = PyStructSequence_New(state->ThreadInfo_Type);
    if (result == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create ThreadInfo");
        goto error;
    }

    // Always use status_flags
    PyObject *py_status = PyLong_FromLong(status_flags);
    if (py_status == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create thread status");
        goto error;
    }

    // py_status contains status flags (bitfield)
    PyStructSequence_SetItem(result, 0, thread_id);
    PyStructSequence_SetItem(result, 1, py_status);  // Steals reference
    PyStructSequence_SetItem(result, 2, frame_info); // Steals reference

    cleanup_stack_chunks(&chunks);
    return result;

error:
    Py_XDECREF(frame_info);
    Py_XDECREF(thread_id);
    Py_XDECREF(result);
    cleanup_stack_chunks(&chunks);
    return NULL;
}
