/******************************************************************************
 * Remote Debugging Module - Main Module Implementation
 *
 * This file contains the main module initialization, the RemoteUnwinder
 * class implementation, and utility functions.
 ******************************************************************************/

#include "_remote_debugging.h"
#include "clinic/module.c.h"

/* ============================================================================
 * STRUCTSEQ TYPE DEFINITIONS
 * ============================================================================ */

// TaskInfo structseq type
static PyStructSequence_Field TaskInfo_fields[] = {
    {"task_id", "Task ID (memory address)"},
    {"task_name", "Task name"},
    {"coroutine_stack", "Coroutine call stack"},
    {"awaited_by", "Tasks awaiting this task"},
    {NULL}
};

PyStructSequence_Desc TaskInfo_desc = {
    "_remote_debugging.TaskInfo",
    "Information about an asyncio task",
    TaskInfo_fields,
    4
};

// FrameInfo structseq type
static PyStructSequence_Field FrameInfo_fields[] = {
    {"filename", "Source code filename"},
    {"lineno", "Line number"},
    {"funcname", "Function name"},
    {NULL}
};

PyStructSequence_Desc FrameInfo_desc = {
    "_remote_debugging.FrameInfo",
    "Information about a frame",
    FrameInfo_fields,
    3
};

// CoroInfo structseq type
static PyStructSequence_Field CoroInfo_fields[] = {
    {"call_stack", "Coroutine call stack"},
    {"task_name", "Task name"},
    {NULL}
};

PyStructSequence_Desc CoroInfo_desc = {
    "_remote_debugging.CoroInfo",
    "Information about a coroutine",
    CoroInfo_fields,
    2
};

// ThreadInfo structseq type
static PyStructSequence_Field ThreadInfo_fields[] = {
    {"thread_id", "Thread ID"},
    {"status", "Thread status (flags: HAS_GIL, ON_CPU, UNKNOWN or legacy enum)"},
    {"frame_info", "Frame information"},
    {NULL}
};

PyStructSequence_Desc ThreadInfo_desc = {
    "_remote_debugging.ThreadInfo",
    "Information about a thread",
    ThreadInfo_fields,
    3
};

// InterpreterInfo structseq type
static PyStructSequence_Field InterpreterInfo_fields[] = {
    {"interpreter_id", "Interpreter ID"},
    {"threads", "List of threads in this interpreter"},
    {NULL}
};

PyStructSequence_Desc InterpreterInfo_desc = {
    "_remote_debugging.InterpreterInfo",
    "Information about an interpreter",
    InterpreterInfo_fields,
    2
};

// AwaitedInfo structseq type
static PyStructSequence_Field AwaitedInfo_fields[] = {
    {"thread_id", "Thread ID"},
    {"awaited_by", "List of tasks awaited by this thread"},
    {NULL}
};

PyStructSequence_Desc AwaitedInfo_desc = {
    "_remote_debugging.AwaitedInfo",
    "Information about what a thread is awaiting",
    AwaitedInfo_fields,
    2
};

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void
cached_code_metadata_destroy(void *ptr)
{
    CachedCodeMetadata *meta = (CachedCodeMetadata *)ptr;
    Py_DECREF(meta->func_name);
    Py_DECREF(meta->file_name);
    Py_DECREF(meta->linetable);
    PyMem_RawFree(meta);
}

RemoteDebuggingState *
RemoteDebugging_GetState(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (RemoteDebuggingState *)state;
}

RemoteDebuggingState *
RemoteDebugging_GetStateFromType(PyTypeObject *type)
{
    PyObject *module = PyType_GetModule(type);
    assert(module != NULL);
    return RemoteDebugging_GetState(module);
}

RemoteDebuggingState *
RemoteDebugging_GetStateFromObject(PyObject *obj)
{
    RemoteUnwinderObject *unwinder = (RemoteUnwinderObject *)obj;
    if (unwinder->cached_state == NULL) {
        unwinder->cached_state = RemoteDebugging_GetStateFromType(Py_TYPE(obj));
    }
    return unwinder->cached_state;
}

int
RemoteDebugging_InitState(RemoteDebuggingState *st)
{
    return 0;
}

int
is_prerelease_version(uint64_t version)
{
    return (version & 0xF0) != 0xF0;
}

int
validate_debug_offsets(struct _Py_DebugOffsets *debug_offsets)
{
    if (memcmp(debug_offsets->cookie, _Py_Debug_Cookie, sizeof(debug_offsets->cookie)) != 0) {
        // The remote is probably running a Python version predating debug offsets.
        PyErr_SetString(
            PyExc_RuntimeError,
            "Can't determine the Python version of the remote process");
        return -1;
    }

    // Assume debug offsets could change from one pre-release version to another,
    // or one minor version to another, but are stable across patch versions.
    if (is_prerelease_version(Py_Version) && Py_Version != debug_offsets->version) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Can't attach from a pre-release Python interpreter"
            " to a process running a different Python version");
        return -1;
    }

    if (is_prerelease_version(debug_offsets->version) && Py_Version != debug_offsets->version) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Can't attach to a pre-release Python interpreter"
            " from a process running a different Python version");
        return -1;
    }

    unsigned int remote_major = (debug_offsets->version >> 24) & 0xFF;
    unsigned int remote_minor = (debug_offsets->version >> 16) & 0xFF;

    if (PY_MAJOR_VERSION != remote_major || PY_MINOR_VERSION != remote_minor) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Can't attach from a Python %d.%d process to a Python %d.%d process",
            PY_MAJOR_VERSION, PY_MINOR_VERSION, remote_major, remote_minor);
        return -1;
    }

    // The debug offsets differ between free threaded and non-free threaded builds.
    if (_Py_Debug_Free_Threaded && !debug_offsets->free_threaded) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Cannot attach from a free-threaded Python process"
            " to a process running a non-free-threaded version");
        return -1;
    }

    if (!_Py_Debug_Free_Threaded && debug_offsets->free_threaded) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Cannot attach to a free-threaded Python process"
            " from a process running a non-free-threaded version");
        return -1;
    }

    return 0;
}

/* ============================================================================
 * REMOTEUNWINDER CLASS IMPLEMENTATION
 * ============================================================================ */

/*[clinic input]
module _remote_debugging
class _remote_debugging.RemoteUnwinder "RemoteUnwinderObject *" "&RemoteUnwinder_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=12b4dce200381115]*/

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
_remote_debugging.RemoteUnwinder.__init__
    pid: int
    *
    all_threads: bool = False
    only_active_thread: bool = False
    mode: int = 0
    debug: bool = False
    skip_non_matching_threads: bool = True
    native: bool = False
    gc: bool = False

Initialize a new RemoteUnwinder object for debugging a remote Python process.

Args:
    pid: Process ID of the target Python process to debug
    all_threads: If True, initialize state for all threads in the process.
                If False, only initialize for the main thread.
    only_active_thread: If True, only sample the thread holding the GIL.
    mode: Profiling mode: 0=WALL (wall-time), 1=CPU (cpu-time), 2=GIL (gil-time).
                       Cannot be used together with all_threads=True.
    debug: If True, chain exceptions to explain the sequence of events that
           lead to the exception.
    skip_non_matching_threads: If True, skip threads that don't match the selected mode.
                              If False, include all threads regardless of mode.
    native: If True, include artificial "<native>" frames to denote calls to
            non-Python code.
    gc: If True, include artificial "<GC>" frames to denote active garbage
        collection.

The RemoteUnwinder provides functionality to inspect and debug a running Python
process, including examining thread states, stack frames and other runtime data.

Raises:
    PermissionError: If access to the target process is denied
    OSError: If unable to attach to the target process or access its memory
    RuntimeError: If unable to read debug information from the target process
    ValueError: If both all_threads and only_active_thread are True
[clinic start generated code]*/

static int
_remote_debugging_RemoteUnwinder___init___impl(RemoteUnwinderObject *self,
                                               int pid, int all_threads,
                                               int only_active_thread,
                                               int mode, int debug,
                                               int skip_non_matching_threads,
                                               int native, int gc)
/*[clinic end generated code: output=e9eb6b4df119f6e0 input=606d099059207df2]*/
{
    // Validate that all_threads and only_active_thread are not both True
    if (all_threads && only_active_thread) {
        PyErr_SetString(PyExc_ValueError,
                       "all_threads and only_active_thread cannot both be True");
        return -1;
    }

#ifdef Py_GIL_DISABLED
    if (only_active_thread) {
        PyErr_SetString(PyExc_ValueError,
                       "only_active_thread is not supported when Py_GIL_DISABLED is not defined");
        return -1;
    }
#endif

    self->native = native;
    self->gc = gc;
    self->debug = debug;
    self->only_active_thread = only_active_thread;
    self->mode = mode;
    self->skip_non_matching_threads = skip_non_matching_threads;
    self->cached_state = NULL;
    if (_Py_RemoteDebug_InitProcHandle(&self->handle, pid) < 0) {
        set_exception_cause(self, PyExc_RuntimeError, "Failed to initialize process handle");
        return -1;
    }

    self->runtime_start_address = _Py_RemoteDebug_GetPyRuntimeAddress(&self->handle);
    if (self->runtime_start_address == 0) {
        set_exception_cause(self, PyExc_RuntimeError, "Failed to get Python runtime address");
        return -1;
    }

    if (_Py_RemoteDebug_ReadDebugOffsets(&self->handle,
                                         &self->runtime_start_address,
                                         &self->debug_offsets) < 0)
    {
        set_exception_cause(self, PyExc_RuntimeError, "Failed to read debug offsets");
        return -1;
    }

    // Validate that the debug offsets are valid
    if(validate_debug_offsets(&self->debug_offsets) == -1) {
        set_exception_cause(self, PyExc_RuntimeError, "Invalid debug offsets found");
        return -1;
    }

    // Try to read async debug offsets, but don't fail if they're not available
    self->async_debug_offsets_available = 1;
    if (read_async_debug(self) < 0) {
        PyErr_Clear();
        memset(&self->async_debug_offsets, 0, sizeof(self->async_debug_offsets));
        self->async_debug_offsets_available = 0;
    }

    if (populate_initial_state_data(all_threads, self, self->runtime_start_address,
                    &self->interpreter_addr ,&self->tstate_addr) < 0)
    {
        set_exception_cause(self, PyExc_RuntimeError, "Failed to populate initial state data");
        return -1;
    }

    self->code_object_cache = _Py_hashtable_new_full(
        _Py_hashtable_hash_ptr,
        _Py_hashtable_compare_direct,
        NULL,  // keys are stable pointers, don't destroy
        cached_code_metadata_destroy,
        NULL
    );
    if (self->code_object_cache == NULL) {
        PyErr_NoMemory();
        set_exception_cause(self, PyExc_MemoryError, "Failed to create code object cache");
        return -1;
    }

#ifdef Py_GIL_DISABLED
    // Initialize TLBC cache
    self->tlbc_generation = 0;
    self->tlbc_cache = _Py_hashtable_new_full(
        _Py_hashtable_hash_ptr,
        _Py_hashtable_compare_direct,
        NULL,  // keys are stable pointers, don't destroy
        tlbc_cache_entry_destroy,
        NULL
    );
    if (self->tlbc_cache == NULL) {
        _Py_hashtable_destroy(self->code_object_cache);
        PyErr_NoMemory();
        set_exception_cause(self, PyExc_MemoryError, "Failed to create TLBC cache");
        return -1;
    }
#endif

#if defined(__APPLE__)
    self->thread_id_offset = 0;
#endif

#ifdef MS_WINDOWS
    self->win_process_buffer = NULL;
    self->win_process_buffer_size = 0;
#endif

    return 0;
}

/*[clinic input]
@permit_long_docstring_body
@critical_section
_remote_debugging.RemoteUnwinder.get_stack_trace

Returns stack traces for all interpreters and threads in process.

Each element in the returned list is a tuple of (interpreter_id, thread_list), where:
- interpreter_id is the interpreter identifier
- thread_list is a list of tuples (thread_id, frame_list) for threads in that interpreter
  - thread_id is the OS thread identifier
  - frame_list is a list of tuples (function_name, filename, line_number) representing
    the Python stack frames for that thread, ordered from most recent to oldest

The threads returned depend on the initialization parameters:
- If only_active_thread was True: returns only the thread holding the GIL across all interpreters
- If all_threads was True: returns all threads across all interpreters
- Otherwise: returns only the main thread of each interpreter

Example:
    [
        (0, [  # Main interpreter
            (1234, [
                ('process_data', 'worker.py', 127),
                ('run_worker', 'worker.py', 45),
                ('main', 'app.py', 23)
            ]),
            (1235, [
                ('handle_request', 'server.py', 89),
                ('serve_forever', 'server.py', 52)
            ])
        ]),
        (1, [  # Sub-interpreter
            (1236, [
                ('sub_worker', 'sub.py', 15)
            ])
        ])
    ]

Raises:
    RuntimeError: If there is an error copying memory from the target process
    OSError: If there is an error accessing the target process
    PermissionError: If access to the target process is denied
    UnicodeDecodeError: If there is an error decoding strings from the target process

[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=666192b90c69d567 input=bcff01c73cccc1c0]*/
{
    PyObject* result = PyList_New(0);
    if (!result) {
        set_exception_cause(self, PyExc_MemoryError, "Failed to create stack trace result list");
        return NULL;
    }

    // Iterate over all interpreters
    uintptr_t current_interpreter = self->interpreter_addr;
    while (current_interpreter != 0) {
        // Read interpreter state to get the interpreter ID
        char interp_state_buffer[INTERP_STATE_BUFFER_SIZE];
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &self->handle,
                current_interpreter,
                INTERP_STATE_BUFFER_SIZE,
                interp_state_buffer) < 0) {
            set_exception_cause(self, PyExc_RuntimeError, "Failed to read interpreter state buffer");
            Py_CLEAR(result);
            goto exit;
        }

        uintptr_t gc_frame = 0;
        if (self->gc) {
            gc_frame = GET_MEMBER(uintptr_t, interp_state_buffer,
                                  self->debug_offsets.interpreter_state.gc
                                  + self->debug_offsets.gc.frame);
        }

        int64_t interpreter_id = GET_MEMBER(int64_t, interp_state_buffer,
                self->debug_offsets.interpreter_state.id);

        // Get code object generation from buffer
        uint64_t code_object_generation = GET_MEMBER(uint64_t, interp_state_buffer,
                self->debug_offsets.interpreter_state.code_object_generation);

        if (code_object_generation != self->code_object_generation) {
            self->code_object_generation = code_object_generation;
            _Py_hashtable_clear(self->code_object_cache);
        }

#ifdef Py_GIL_DISABLED
        // Check TLBC generation and invalidate cache if needed
        uint32_t current_tlbc_generation = GET_MEMBER(uint32_t, interp_state_buffer,
                                                      self->debug_offsets.interpreter_state.tlbc_generation);
        if (current_tlbc_generation != self->tlbc_generation) {
            self->tlbc_generation = current_tlbc_generation;
            _Py_hashtable_clear(self->tlbc_cache);
        }
#endif

        // Create a list to hold threads for this interpreter
        PyObject *interpreter_threads = PyList_New(0);
        if (!interpreter_threads) {
            set_exception_cause(self, PyExc_MemoryError, "Failed to create interpreter threads list");
            Py_CLEAR(result);
            goto exit;
        }

        // Get the GIL holder for this interpreter (needed for GIL_WAIT logic)
        uintptr_t gil_holder_tstate = 0;
        int gil_locked = GET_MEMBER(int, interp_state_buffer,
            self->debug_offsets.interpreter_state.gil_runtime_state_locked);
        if (gil_locked) {
            gil_holder_tstate = (uintptr_t)GET_MEMBER(PyThreadState*, interp_state_buffer,
                self->debug_offsets.interpreter_state.gil_runtime_state_holder);
        }

        uintptr_t current_tstate;
        if (self->only_active_thread) {
            // Find the GIL holder for THIS interpreter
            if (!gil_locked) {
                // This interpreter's GIL is not locked, skip it
                Py_DECREF(interpreter_threads);
                goto next_interpreter;
            }

            current_tstate = gil_holder_tstate;
        } else if (self->tstate_addr == 0) {
            // Get all threads for this interpreter
            current_tstate = GET_MEMBER(uintptr_t, interp_state_buffer,
                    self->debug_offsets.interpreter_state.threads_head);
        } else {
            // Target specific thread (only process first interpreter)
            current_tstate = self->tstate_addr;
        }

        while (current_tstate != 0) {
            PyObject* frame_info = unwind_stack_for_thread(self, &current_tstate,
                                                           gil_holder_tstate,
                                                           gc_frame);
            if (!frame_info) {
                // Check if this was an intentional skip due to mode-based filtering
                if ((self->mode == PROFILING_MODE_CPU || self->mode == PROFILING_MODE_GIL) && !PyErr_Occurred()) {
                    // Thread was skipped due to mode filtering, continue to next thread
                    continue;
                }
                // This was an actual error
                Py_DECREF(interpreter_threads);
                set_exception_cause(self, PyExc_RuntimeError, "Failed to unwind stack for thread");
                Py_CLEAR(result);
                goto exit;
            }

            if (PyList_Append(interpreter_threads, frame_info) == -1) {
                Py_DECREF(frame_info);
                Py_DECREF(interpreter_threads);
                set_exception_cause(self, PyExc_RuntimeError, "Failed to append thread frame info");
                Py_CLEAR(result);
                goto exit;
            }
            Py_DECREF(frame_info);

            // If targeting specific thread or only active thread, process just one
            if (self->tstate_addr || self->only_active_thread) {
                break;
            }
        }

        // Create the InterpreterInfo StructSequence
        RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)self);
        PyObject *interpreter_info = PyStructSequence_New(state->InterpreterInfo_Type);
        if (!interpreter_info) {
            Py_DECREF(interpreter_threads);
            set_exception_cause(self, PyExc_MemoryError, "Failed to create InterpreterInfo");
            Py_CLEAR(result);
            goto exit;
        }

        PyObject *interp_id = PyLong_FromLongLong(interpreter_id);
        if (!interp_id) {
            Py_DECREF(interpreter_threads);
            Py_DECREF(interpreter_info);
            set_exception_cause(self, PyExc_MemoryError, "Failed to create interpreter ID");
            Py_CLEAR(result);
            goto exit;
        }

        PyStructSequence_SetItem(interpreter_info, 0, interp_id);  // steals reference
        PyStructSequence_SetItem(interpreter_info, 1, interpreter_threads);  // steals reference

        // Add this interpreter to the result list
        if (PyList_Append(result, interpreter_info) == -1) {
            Py_DECREF(interpreter_info);
            set_exception_cause(self, PyExc_RuntimeError, "Failed to append interpreter info");
            Py_CLEAR(result);
            goto exit;
        }
        Py_DECREF(interpreter_info);

next_interpreter:

        // Get the next interpreter address
        current_interpreter = GET_MEMBER(uintptr_t, interp_state_buffer,
                self->debug_offsets.interpreter_state.next);

        // If we're targeting a specific thread, stop after first interpreter
        if (self->tstate_addr != 0) {
            break;
        }
    }

exit:
   _Py_RemoteDebug_ClearCache(&self->handle);
    return result;
}

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
@critical_section
_remote_debugging.RemoteUnwinder.get_all_awaited_by

Get all tasks and their awaited_by relationships from the remote process.

This provides a tree structure showing which tasks are waiting for other tasks.

For each task, returns:
1. The call stack frames leading to where the task is currently executing
2. The name of the task
3. A list of tasks that this task is waiting for, with their own frames/names/etc

Returns a list of [frames, task_name, subtasks] where:
- frames: List of (func_name, filename, lineno) showing the call stack
- task_name: String identifier for the task
- subtasks: List of tasks being awaited by this task, in same format

Raises:
    RuntimeError: If AsyncioDebug section is not available in the remote process
    MemoryError: If memory allocation fails
    OSError: If reading from the remote process fails

Example output:
[
    # Task c2_root waiting for two subtasks
    [
        # Call stack of c2_root
        [("c5", "script.py", 10), ("c4", "script.py", 14)],
        "c2_root",
        [
            # First subtask (sub_main_2) and what it's waiting for
            [
                [("c1", "script.py", 23)],
                "sub_main_2",
                [...]
            ],
            # Second subtask and its waiters
            [...]
        ]
    ]
]
[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=6a49cd345e8aec53 input=307f754cbe38250c]*/
{
    if (!self->async_debug_offsets_available) {
        PyErr_SetString(PyExc_RuntimeError, "AsyncioDebug section not available");
        set_exception_cause(self, PyExc_RuntimeError, "AsyncioDebug section unavailable in get_all_awaited_by");
        return NULL;
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        set_exception_cause(self, PyExc_MemoryError, "Failed to create awaited_by result list");
        goto result_err;
    }

    // Process all threads
    if (iterate_threads(self, process_thread_for_awaited_by, result) < 0) {
        goto result_err;
    }

    uintptr_t head_addr = self->interpreter_addr
        + (uintptr_t)self->async_debug_offsets.asyncio_interpreter_state.asyncio_tasks_head;

    // On top of a per-thread task lists used by default by asyncio to avoid
    // contention, there is also a fallback per-interpreter list of tasks;
    // any tasks still pending when a thread is destroyed will be moved to the
    // per-interpreter task list.  It's unlikely we'll find anything here, but
    // interesting for debugging.
    if (append_awaited_by(self, 0, head_addr, result))
    {
        set_exception_cause(self, PyExc_RuntimeError, "Failed to append interpreter awaited_by in get_all_awaited_by");
        goto result_err;
    }

    _Py_RemoteDebug_ClearCache(&self->handle);
    return result;

result_err:
    _Py_RemoteDebug_ClearCache(&self->handle);
    Py_XDECREF(result);
    return NULL;
}

/*[clinic input]
@permit_long_summary
@permit_long_docstring_body
@critical_section
_remote_debugging.RemoteUnwinder.get_async_stack_trace

Get the currently running async tasks and their dependency graphs from the remote process.

This returns information about running tasks and all tasks that are waiting for them,
forming a complete dependency graph for each thread's active task.

For each thread with a running task, returns the running task plus all tasks that
transitively depend on it (tasks waiting for the running task, tasks waiting for
those tasks, etc.).

Returns a list of per-thread results, where each thread result contains:
- Thread ID
- List of task information for the running task and all its waiters

Each task info contains:
- Task ID (memory address)
- Task name
- Call stack frames: List of (func_name, filename, lineno)
- List of tasks waiting for this task (recursive structure)

Raises:
    RuntimeError: If AsyncioDebug section is not available in the target process
    MemoryError: If memory allocation fails
    OSError: If reading from the remote process fails

Example output (similar structure to get_all_awaited_by but only for running tasks):
[
    # Thread 140234 results
    (140234, [
        # Running task and its complete waiter dependency graph
        (4345585712, 'main_task',
         [("run_server", "server.py", 127), ("main", "app.py", 23)],
         [
             # Tasks waiting for main_task
             (4345585800, 'worker_1', [...], [...]),
             (4345585900, 'worker_2', [...], [...])
         ])
    ])
]

[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=6433d52b55e87bbe input=6129b7d509a887c9]*/
{
    if (!self->async_debug_offsets_available) {
        PyErr_SetString(PyExc_RuntimeError, "AsyncioDebug section not available");
        set_exception_cause(self, PyExc_RuntimeError, "AsyncioDebug section unavailable in get_async_stack_trace");
        return NULL;
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        set_exception_cause(self, PyExc_MemoryError, "Failed to create result list in get_async_stack_trace");
        return NULL;
    }

    // Process all threads
    if (iterate_threads(self, process_thread_for_async_stack_trace, result) < 0) {
        goto result_err;
    }

    _Py_RemoteDebug_ClearCache(&self->handle);
    return result;
result_err:
    _Py_RemoteDebug_ClearCache(&self->handle);
    Py_XDECREF(result);
    return NULL;
}

static PyMethodDef RemoteUnwinder_methods[] = {
    _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STACK_TRACE_METHODDEF
    _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ALL_AWAITED_BY_METHODDEF
    _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ASYNC_STACK_TRACE_METHODDEF
    {NULL, NULL}
};

static void
RemoteUnwinder_dealloc(PyObject *op)
{
    RemoteUnwinderObject *self = RemoteUnwinder_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    if (self->code_object_cache) {
        _Py_hashtable_destroy(self->code_object_cache);
    }
#ifdef MS_WINDOWS
    if(self->win_process_buffer != NULL) {
        PyMem_Free(self->win_process_buffer);
    }
#endif

#ifdef Py_GIL_DISABLED
    if (self->tlbc_cache) {
        _Py_hashtable_destroy(self->tlbc_cache);
    }
#endif
    if (self->handle.pid != 0) {
        _Py_RemoteDebug_ClearCache(&self->handle);
        _Py_RemoteDebug_CleanupProcHandle(&self->handle);
    }
    PyObject_Del(self);
    Py_DECREF(tp);
}

static PyType_Slot RemoteUnwinder_slots[] = {
    {Py_tp_doc, (void *)"RemoteUnwinder(pid): Inspect stack of a remote Python process."},
    {Py_tp_methods, RemoteUnwinder_methods},
    {Py_tp_init, _remote_debugging_RemoteUnwinder___init__},
    {Py_tp_dealloc, RemoteUnwinder_dealloc},
    {0, NULL}
};

static PyType_Spec RemoteUnwinder_spec = {
    .name = "_remote_debugging.RemoteUnwinder",
    .basicsize = sizeof(RemoteUnwinderObject),
    .flags = (
        Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_IMMUTABLETYPE
    ),
    .slots = RemoteUnwinder_slots,
};

/* ============================================================================
 * MODULE INITIALIZATION
 * ============================================================================ */

static int
_remote_debugging_exec(PyObject *m)
{
    RemoteDebuggingState *st = RemoteDebugging_GetState(m);
#define CREATE_TYPE(mod, type, spec)                                        \
    do {                                                                    \
        type = (PyTypeObject *)PyType_FromMetaclass(NULL, mod, spec, NULL); \
        if (type == NULL) {                                                 \
            return -1;                                                      \
        }                                                                   \
    } while (0)

    CREATE_TYPE(m, st->RemoteDebugging_Type, &RemoteUnwinder_spec);

    if (PyModule_AddType(m, st->RemoteDebugging_Type) < 0) {
        return -1;
    }

    // Initialize structseq types
    st->TaskInfo_Type = PyStructSequence_NewType(&TaskInfo_desc);
    if (st->TaskInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->TaskInfo_Type) < 0) {
        return -1;
    }

    st->FrameInfo_Type = PyStructSequence_NewType(&FrameInfo_desc);
    if (st->FrameInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->FrameInfo_Type) < 0) {
        return -1;
    }

    st->CoroInfo_Type = PyStructSequence_NewType(&CoroInfo_desc);
    if (st->CoroInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->CoroInfo_Type) < 0) {
        return -1;
    }

    st->ThreadInfo_Type = PyStructSequence_NewType(&ThreadInfo_desc);
    if (st->ThreadInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->ThreadInfo_Type) < 0) {
        return -1;
    }

    st->InterpreterInfo_Type = PyStructSequence_NewType(&InterpreterInfo_desc);
    if (st->InterpreterInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->InterpreterInfo_Type) < 0) {
        return -1;
    }

    st->AwaitedInfo_Type = PyStructSequence_NewType(&AwaitedInfo_desc);
    if (st->AwaitedInfo_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, st->AwaitedInfo_Type) < 0) {
        return -1;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(m, Py_MOD_GIL_NOT_USED);
#endif
    int rc = PyModule_AddIntConstant(m, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
        return -1;
    }

    // Add thread status flag constants
    if (PyModule_AddIntConstant(m, "THREAD_STATUS_HAS_GIL", THREAD_STATUS_HAS_GIL) < 0) {
        return -1;
    }
    if (PyModule_AddIntConstant(m, "THREAD_STATUS_ON_CPU", THREAD_STATUS_ON_CPU) < 0) {
        return -1;
    }
    if (PyModule_AddIntConstant(m, "THREAD_STATUS_UNKNOWN", THREAD_STATUS_UNKNOWN) < 0) {
        return -1;
    }
    if (PyModule_AddIntConstant(m, "THREAD_STATUS_GIL_REQUESTED", THREAD_STATUS_GIL_REQUESTED) < 0) {
        return -1;
    }

    if (RemoteDebugging_InitState(st) < 0) {
        return -1;
    }
    return 0;
}

static int
remote_debugging_traverse(PyObject *mod, visitproc visit, void *arg)
{
    RemoteDebuggingState *state = RemoteDebugging_GetState(mod);
    Py_VISIT(state->RemoteDebugging_Type);
    Py_VISIT(state->TaskInfo_Type);
    Py_VISIT(state->FrameInfo_Type);
    Py_VISIT(state->CoroInfo_Type);
    Py_VISIT(state->ThreadInfo_Type);
    Py_VISIT(state->InterpreterInfo_Type);
    Py_VISIT(state->AwaitedInfo_Type);
    return 0;
}

static int
remote_debugging_clear(PyObject *mod)
{
    RemoteDebuggingState *state = RemoteDebugging_GetState(mod);
    Py_CLEAR(state->RemoteDebugging_Type);
    Py_CLEAR(state->TaskInfo_Type);
    Py_CLEAR(state->FrameInfo_Type);
    Py_CLEAR(state->CoroInfo_Type);
    Py_CLEAR(state->ThreadInfo_Type);
    Py_CLEAR(state->InterpreterInfo_Type);
    Py_CLEAR(state->AwaitedInfo_Type);
    return 0;
}

static void
remote_debugging_free(void *mod)
{
    (void)remote_debugging_clear((PyObject *)mod);
}

static PyModuleDef_Slot remote_debugging_slots[] = {
    {Py_mod_exec, _remote_debugging_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static PyMethodDef remote_debugging_methods[] = {
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef remote_debugging_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_remote_debugging",
    .m_size = sizeof(RemoteDebuggingState),
    .m_methods = remote_debugging_methods,
    .m_slots = remote_debugging_slots,
    .m_traverse = remote_debugging_traverse,
    .m_clear = remote_debugging_clear,
    .m_free = remote_debugging_free,
};

PyMODINIT_FUNC
PyInit__remote_debugging(void)
{
    return PyModuleDef_Init(&remote_debugging_module);
}
