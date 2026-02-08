/******************************************************************************
 * Remote Debugging Module - Asyncio Functions
 *
 * This file contains functions for parsing asyncio tasks, coroutines,
 * and awaited_by relationships from remote process memory.
 ******************************************************************************/

#include "_remote_debugging.h"

/* ============================================================================
 * ASYNCIO DEBUG ADDRESS FUNCTIONS
 * ============================================================================ */

uintptr_t
_Py_RemoteDebug_GetAsyncioDebugAddress(proc_handle_t* handle)
{
    uintptr_t address;

#ifdef MS_WINDOWS
    // On Windows, search for asyncio debug in executable or DLL
    address = search_windows_map_for_section(handle, "AsyncioD", L"_asyncio",
                                             NULL);
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__linux__) && HAVE_PROCESS_VM_READV
    // On Linux, search for asyncio debug in executable or DLL
    address = search_linux_map_for_section(handle, "AsyncioDebug", "python",
                                           NULL);
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    address = search_map_for_section(handle, "AsyncioDebug", "libpython",
                                     NULL);
    if (address == 0) {
        PyErr_Clear();
        address = search_map_for_section(handle, "AsyncioDebug", "python",
                                         NULL);
    }
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#else
    Py_UNREACHABLE();
#endif

    return address;
}

int
read_async_debug(RemoteUnwinderObject *unwinder)
{
    uintptr_t async_debug_addr = _Py_RemoteDebug_GetAsyncioDebugAddress(&unwinder->handle);
    if (!async_debug_addr) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to get AsyncioDebug address");
        return -1;
    }

    size_t size = sizeof(struct _Py_AsyncioModuleDebugOffsets);
    int result = _Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, async_debug_addr, size, &unwinder->async_debug_offsets);
    if (result < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read AsyncioDebug offsets");
    }
    return result;
}

int
ensure_async_debug_offsets(RemoteUnwinderObject *unwinder)
{
    // If already available, nothing to do
    if (unwinder->async_debug_offsets_available) {
        return 0;
    }

    // Try to load async debug offsets (the target process may have
    // loaded asyncio since we last checked)
    if (read_async_debug(unwinder) < 0) {
        PyErr_Clear();
        PyErr_SetString(PyExc_RuntimeError, "AsyncioDebug section not available");
        set_exception_cause(unwinder, PyExc_RuntimeError,
            "AsyncioDebug section unavailable - asyncio module may not be loaded in target process");
        return -1;
    }

    unwinder->async_debug_offsets_available = 1;
    return 0;
}

/* ============================================================================
 * SET ITERATION FUNCTIONS
 * ============================================================================ */

int
iterate_set_entries(
    RemoteUnwinderObject *unwinder,
    uintptr_t set_addr,
    set_entry_processor_func processor,
    void *context
) {
    char set_object[SIZEOF_SET_OBJ];
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, set_addr,
                                              SIZEOF_SET_OBJ, set_object) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read set object");
        return -1;
    }

    Py_ssize_t num_els = GET_MEMBER(Py_ssize_t, set_object, unwinder->debug_offsets.set_object.used);
    Py_ssize_t mask = GET_MEMBER(Py_ssize_t, set_object, unwinder->debug_offsets.set_object.mask);
    uintptr_t table_ptr = GET_MEMBER(uintptr_t, set_object, unwinder->debug_offsets.set_object.table);

    // Validate mask and num_els to prevent huge loop iterations from garbage data
    if (mask < 0 || mask >= MAX_SET_TABLE_SIZE || num_els < 0 || num_els > mask + 1) {
        set_exception_cause(unwinder, PyExc_RuntimeError,
            "Invalid set object (corrupted remote memory)");
        return -1;
    }
    Py_ssize_t set_len = mask + 1;

    Py_ssize_t i = 0;
    Py_ssize_t els = 0;
    while (i < set_len && els < num_els) {
        uintptr_t key_addr;
        if (read_py_ptr(unwinder, table_ptr, &key_addr) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read set entry key");
            return -1;
        }

        if ((void*)key_addr != NULL) {
            Py_ssize_t ref_cnt;
            if (read_Py_ssize_t(unwinder, table_ptr, &ref_cnt) < 0) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read set entry ref count");
                return -1;
            }

            if (ref_cnt) {
                // Process this valid set entry
                if (processor(unwinder, key_addr, context) < 0) {
                    return -1;
                }
                els++;
            }
        }
        table_ptr += sizeof(void*) * 2;
        i++;
    }

    return 0;
}

/* ============================================================================
 * TASK NAME PARSING
 * ============================================================================ */

PyObject *
parse_task_name(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address
) {
    // Read the entire TaskObj at once
    char task_obj[SIZEOF_TASK_OBJ];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        task_address,
        (size_t)unwinder->async_debug_offsets.asyncio_task_object.size,
        task_obj);
    if (err < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task object");
        return NULL;
    }

    uintptr_t task_name_addr = GET_MEMBER_NO_TAG(uintptr_t, task_obj, unwinder->async_debug_offsets.asyncio_task_object.task_name);

    // The task name can be a long or a string so we need to check the type
    char task_name_obj[SIZEOF_PYOBJECT];
    err = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        task_name_addr,
        SIZEOF_PYOBJECT,
        task_name_obj);
    if (err < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task name object");
        return NULL;
    }

    // Now read the type object to get the flags
    char type_obj[SIZEOF_TYPE_OBJ];
    err = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        GET_MEMBER(uintptr_t, task_name_obj, unwinder->debug_offsets.pyobject.ob_type),
        SIZEOF_TYPE_OBJ,
        type_obj);
    if (err < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task name type object");
        return NULL;
    }

    if ((GET_MEMBER(unsigned long, type_obj, unwinder->debug_offsets.type_object.tp_flags) & Py_TPFLAGS_LONG_SUBCLASS)) {
        long res = read_py_long(unwinder, task_name_addr);
        if (res == -1) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Task name PyLong parsing failed");
            return NULL;
        }
        return PyUnicode_FromFormat("Task-%d", res);
    }

    if(!(GET_MEMBER(unsigned long, type_obj, unwinder->debug_offsets.type_object.tp_flags) & Py_TPFLAGS_UNICODE_SUBCLASS)) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid task name object");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Task name object is neither long nor unicode");
        return NULL;
    }

    return read_py_str(
        unwinder,
        task_name_addr,
        255
    );
}

/* ============================================================================
 * COROUTINE CHAIN PARSING
 * ============================================================================ */

static int
handle_yield_from_frame(
    RemoteUnwinderObject *unwinder,
    uintptr_t gi_iframe_addr,
    uintptr_t gen_type_addr,
    PyObject *render_to
) {
    // Read the entire interpreter frame at once
    char iframe[SIZEOF_INTERP_FRAME];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        gi_iframe_addr,
        SIZEOF_INTERP_FRAME,
        iframe);
    if (err < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read interpreter frame in yield_from handler");
        return -1;
    }

    if (GET_MEMBER(char, iframe, unwinder->debug_offsets.interpreter_frame.owner) != FRAME_OWNED_BY_GENERATOR) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "generator doesn't own its frame \\_o_/");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Frame ownership mismatch in yield_from");
        return -1;
    }

    uintptr_t stackpointer_addr = GET_MEMBER_NO_TAG(uintptr_t, iframe, unwinder->debug_offsets.interpreter_frame.stackpointer);

    if ((void*)stackpointer_addr != NULL) {
        uintptr_t gi_await_addr;
        err = read_py_ptr(
            unwinder,
            stackpointer_addr - sizeof(void*),
            &gi_await_addr);
        if (err) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read gi_await address");
            return -1;
        }

        if ((void*)gi_await_addr != NULL) {
            uintptr_t gi_await_addr_type_addr;
            err = read_ptr(
                unwinder,
                gi_await_addr + (uintptr_t)unwinder->debug_offsets.pyobject.ob_type,
                &gi_await_addr_type_addr);
            if (err) {
                set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read gi_await type address");
                return -1;
            }

            if (gen_type_addr == gi_await_addr_type_addr) {
                /* This needs an explanation. We always start with parsing
                   native coroutine / generator frames. Ultimately they
                   are awaiting on something. That something can be
                   a native coroutine frame or... an iterator.
                   If it's the latter -- we can't continue building
                   our chain. So the condition to bail out of this is
                   to do that when the type of the current coroutine
                   doesn't match the type of whatever it points to
                   in its cr_await.
                */
                err = parse_coro_chain(unwinder, gi_await_addr, render_to);
                if (err) {
                    set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse coroutine chain in yield_from");
                    return -1;
                }
            }
        }
    }

    return 0;
}

int
parse_coro_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t coro_address,
    PyObject *render_to
) {
    assert((void*)coro_address != NULL);

    // Read the entire generator object at once
    char gen_object[SIZEOF_GEN_OBJ];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        coro_address,
        SIZEOF_GEN_OBJ,
        gen_object);
    if (err < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read generator object in coro chain");
        return -1;
    }

    int8_t frame_state = GET_MEMBER(int8_t, gen_object, unwinder->debug_offsets.gen_object.gi_frame_state);
    if (frame_state == FRAME_CLEARED) {
        return 0;
    }

    uintptr_t gen_type_addr = GET_MEMBER(uintptr_t, gen_object, unwinder->debug_offsets.pyobject.ob_type);

    PyObject* name = NULL;

    // Parse the previous frame using the gi_iframe from local copy
    uintptr_t prev_frame;
    uintptr_t gi_iframe_addr = coro_address + (uintptr_t)unwinder->debug_offsets.gen_object.gi_iframe;
    uintptr_t address_of_code_object = 0;
    if (parse_frame_object(unwinder, &name, gi_iframe_addr, &address_of_code_object, &prev_frame) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse frame object in coro chain");
        return -1;
    }

    if (!name) {
        return 0;
    }

    if (PyList_Append(render_to, name)) {
        Py_DECREF(name);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append frame to coro chain");
        return -1;
    }
    Py_DECREF(name);

    if (frame_state == FRAME_SUSPENDED_YIELD_FROM) {
        return handle_yield_from_frame(unwinder, gi_iframe_addr, gen_type_addr, render_to);
    }

    return 0;
}

/* ============================================================================
 * TASK PARSING FUNCTIONS
 * ============================================================================ */

static PyObject*
create_task_result(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address
) {
    PyObject* result = NULL;
    PyObject *call_stack = NULL;
    PyObject *tn = NULL;
    char task_obj[SIZEOF_TASK_OBJ];
    uintptr_t coro_addr;

    // Create call_stack first since it's the first tuple element
    call_stack = PyList_New(0);
    if (call_stack == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create call stack list");
        goto error;
    }

    // Create task name/address for second tuple element
    tn = PyLong_FromUnsignedLongLong(task_address);
    if (tn == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create task name/address");
        goto error;
    }

    // Parse coroutine chain
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, task_address,
                                              (size_t)unwinder->async_debug_offsets.asyncio_task_object.size,
                                              task_obj) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task object for coro chain");
        goto error;
    }

    coro_addr = GET_MEMBER_NO_TAG(uintptr_t, task_obj, unwinder->async_debug_offsets.asyncio_task_object.task_coro);

    if ((void*)coro_addr != NULL) {
        if (parse_coro_chain(unwinder, coro_addr, call_stack) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse coroutine chain");
            goto error;
        }

        if (PyList_Reverse(call_stack)) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to reverse call stack");
            goto error;
        }
    }

    // Create final CoroInfo result
    RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
    result = PyStructSequence_New(state->CoroInfo_Type);
    if (result == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create CoroInfo");
        goto error;
    }

    // PyStructSequence_SetItem steals references, so we don't need to DECREF on success
    PyStructSequence_SetItem(result, 0, call_stack);  // This steals the reference
    PyStructSequence_SetItem(result, 1, tn);  // This steals the reference

    return result;

error:
    Py_XDECREF(result);
    Py_XDECREF(call_stack);
    Py_XDECREF(tn);
    return NULL;
}

int
parse_task(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    PyObject *render_to
) {
    char is_task;
    PyObject* result = NULL;
    int err;

    err = read_char(
        unwinder,
        task_address + (uintptr_t)unwinder->async_debug_offsets.asyncio_task_object.task_is_task,
        &is_task);
    if (err) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read is_task flag");
        goto error;
    }

    if (is_task) {
        result = create_task_result(unwinder, task_address);
        if (!result) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create task result");
            goto error;
        }
    } else {
        // Create an empty CoroInfo for non-task objects
        RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
        result = PyStructSequence_New(state->CoroInfo_Type);
        if (result == NULL) {
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create empty CoroInfo");
            goto error;
        }
        PyObject *empty_list = PyList_New(0);
        if (empty_list == NULL) {
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create empty list");
            goto error;
        }
        PyObject *task_name = PyLong_FromUnsignedLongLong(task_address);
        if (task_name == NULL) {
            Py_DECREF(empty_list);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create task name");
            goto error;
        }
        PyStructSequence_SetItem(result, 0, empty_list);  // This steals the reference
        PyStructSequence_SetItem(result, 1, task_name);  // This steals the reference
    }
    if (PyList_Append(render_to, result)) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append task result to render list");
        goto error;
    }

    Py_DECREF(result);
    return 0;

error:
    Py_XDECREF(result);
    return -1;
}

/* ============================================================================
 * TASK AWAITED_BY PROCESSING
 * ============================================================================ */

// Forward declaration for mutual recursion
static int process_waiter_task(RemoteUnwinderObject *unwinder, uintptr_t key_addr, void *context);

// Processor function for parsing tasks in sets
static int
process_task_parser(
    RemoteUnwinderObject *unwinder,
    uintptr_t key_addr,
    void *context
) {
    PyObject *awaited_by = (PyObject *)context;
    return parse_task(unwinder, key_addr, awaited_by);
}

static int
parse_task_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    PyObject *awaited_by
) {
    return process_task_awaited_by(unwinder, task_address, process_task_parser, awaited_by);
}

int
process_task_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    set_entry_processor_func processor,
    void *context
) {
    // Read the entire TaskObj at once
    char task_obj[SIZEOF_TASK_OBJ];
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, task_address,
                                              (size_t)unwinder->async_debug_offsets.asyncio_task_object.size,
                                              task_obj) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task object");
        return -1;
    }

    uintptr_t task_ab_addr = GET_MEMBER_NO_TAG(uintptr_t, task_obj, unwinder->async_debug_offsets.asyncio_task_object.task_awaited_by);
    if ((void*)task_ab_addr == NULL) {
        return 0;  // No tasks waiting for this one
    }

    char awaited_by_is_a_set = GET_MEMBER(char, task_obj, unwinder->async_debug_offsets.asyncio_task_object.task_awaited_by_is_set);

    if (awaited_by_is_a_set) {
        return iterate_set_entries(unwinder, task_ab_addr, processor, context);
    } else {
        // Single task waiting
        return processor(unwinder, task_ab_addr, context);
    }
}

int
process_single_task_node(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_addr,
    PyObject **task_info,
    PyObject *result
) {
    PyObject *tn = NULL;
    PyObject *current_awaited_by = NULL;
    PyObject *task_id = NULL;
    PyObject *result_item = NULL;
    PyObject *coroutine_stack = NULL;

    tn = parse_task_name(unwinder, task_addr);
    if (tn == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse task name in single task node");
        goto error;
    }

    current_awaited_by = PyList_New(0);
    if (current_awaited_by == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create awaited_by list in single task node");
        goto error;
    }

    // Extract the coroutine stack for this task
    coroutine_stack = PyList_New(0);
    if (coroutine_stack == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create coroutine stack list in single task node");
        goto error;
    }

    if (parse_task(unwinder, task_addr, coroutine_stack) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse task coroutine stack in single task node");
        goto error;
    }

    task_id = PyLong_FromUnsignedLongLong(task_addr);
    if (task_id == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create task ID in single task node");
        goto error;
    }

    RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
    result_item = PyStructSequence_New(state->TaskInfo_Type);
    if (result_item == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create TaskInfo in single task node");
        goto error;
    }

    PyStructSequence_SetItem(result_item, 0, task_id);  // steals ref
    PyStructSequence_SetItem(result_item, 1, tn);  // steals ref
    PyStructSequence_SetItem(result_item, 2, coroutine_stack);  // steals ref
    PyStructSequence_SetItem(result_item, 3, current_awaited_by);  // steals ref

    // References transferred to tuple
    task_id = NULL;
    tn = NULL;
    coroutine_stack = NULL;
    current_awaited_by = NULL;

    if (PyList_Append(result, result_item)) {
        Py_DECREF(result_item);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append result item in single task node");
        return -1;
    }
    if (task_info != NULL) {
        *task_info = result_item;
    }
    Py_DECREF(result_item);

    // Get back current_awaited_by reference for parse_task_awaited_by
    current_awaited_by = PyStructSequence_GetItem(result_item, 3);
    if (parse_task_awaited_by(unwinder, task_addr, current_awaited_by) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to parse awaited_by in single task node");
        // No cleanup needed here since all references were transferred to result_item
        // and result_item was already added to result list and decreffed
        return -1;
    }

    return 0;

error:
    Py_XDECREF(tn);
    Py_XDECREF(current_awaited_by);
    Py_XDECREF(task_id);
    Py_XDECREF(result_item);
    Py_XDECREF(coroutine_stack);
    return -1;
}

int
process_task_and_waiters(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_addr,
    PyObject *result
) {
    // First, add this task to the result
    if (process_single_task_node(unwinder, task_addr, NULL, result) < 0) {
        return -1;
    }

    // Now find all tasks that are waiting for this task and process them
    return process_task_awaited_by(unwinder, task_addr, process_waiter_task, result);
}

// Processor function for task waiters
static int
process_waiter_task(
    RemoteUnwinderObject *unwinder,
    uintptr_t key_addr,
    void *context
) {
    PyObject *result = (PyObject *)context;
    return process_task_and_waiters(unwinder, key_addr, result);
}

/* ============================================================================
 * RUNNING TASK FUNCTIONS
 * ============================================================================ */

int
find_running_task_in_thread(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    uintptr_t *running_task_addr
) {
    *running_task_addr = (uintptr_t)NULL;

    uintptr_t address_of_running_loop;
    int bytes_read = read_py_ptr(
        unwinder,
        thread_state_addr + (uintptr_t)unwinder->async_debug_offsets.asyncio_thread_state.asyncio_running_loop,
        &address_of_running_loop);
    if (bytes_read == -1) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read running loop address");
        return -1;
    }

    // no asyncio loop is now running
    if ((void*)address_of_running_loop == NULL) {
        return 0;
    }

    int err = read_ptr(
        unwinder,
        thread_state_addr + (uintptr_t)unwinder->async_debug_offsets.asyncio_thread_state.asyncio_running_task,
        running_task_addr);
    if (err) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read running task address");
        return -1;
    }

    return 0;
}

int
get_task_code_object(RemoteUnwinderObject *unwinder, uintptr_t task_addr, uintptr_t *code_obj_addr) {
    uintptr_t running_coro_addr = 0;

    if(read_py_ptr(
        unwinder,
        task_addr + (uintptr_t)unwinder->async_debug_offsets.asyncio_task_object.task_coro,
        &running_coro_addr) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Running task coro read failed");
        return -1;
    }

    if (running_coro_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Running task coro is NULL");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Running task coro address is NULL");
        return -1;
    }

    // note: genobject's gi_iframe is an embedded struct so the address to
    // the offset leads directly to its first field: f_executable
    if (read_py_ptr(
        unwinder,
        running_coro_addr + (uintptr_t)unwinder->debug_offsets.gen_object.gi_iframe, code_obj_addr) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read running task code object");
        return -1;
    }

    if (*code_obj_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Running task code object is NULL");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Running task code object address is NULL");
        return -1;
    }

    return 0;
}

/* ============================================================================
 * ASYNC FRAME CHAIN PARSING
 * ============================================================================ */

int
parse_async_frame_chain(
    RemoteUnwinderObject *unwinder,
    PyObject *calls,
    uintptr_t address_of_thread,
    uintptr_t running_task_code_obj
) {
    uintptr_t address_of_current_frame;
    if (find_running_frame(unwinder, address_of_thread, &address_of_current_frame) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Running frame search failed in async chain");
        return -1;
    }

    while ((void*)address_of_current_frame != NULL) {
        PyObject* frame_info = NULL;
        uintptr_t address_of_code_object;
        int res = parse_frame_object(
            unwinder,
            &frame_info,
            address_of_current_frame,
            &address_of_code_object,
            &address_of_current_frame
        );

        if (res < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Async frame object parsing failed in chain");
            return -1;
        }

        if (!frame_info) {
            continue;
        }

        if (PyList_Append(calls, frame_info) == -1) {
            Py_DECREF(frame_info);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append frame info to async chain");
            return -1;
        }

        Py_DECREF(frame_info);

        if (address_of_code_object == running_task_code_obj) {
            break;
        }
    }

    return 0;
}

/* ============================================================================
 * AWAITED BY PARSING FUNCTIONS
 * ============================================================================ */

static int
append_awaited_by_for_thread(
    RemoteUnwinderObject *unwinder,
    uintptr_t head_addr,
    PyObject *result
) {
    char task_node[SIZEOF_LLIST_NODE];

    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, head_addr,
                                              sizeof(task_node), task_node) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read task node head");
        return -1;
    }

    size_t iteration_count = 0;
    const size_t MAX_ITERATIONS = 2 << 15;  // A reasonable upper bound

    while (GET_MEMBER(uintptr_t, task_node, unwinder->debug_offsets.llist_node.next) != head_addr) {
        if (++iteration_count > MAX_ITERATIONS) {
            PyErr_SetString(PyExc_RuntimeError, "Task list appears corrupted");
            set_exception_cause(unwinder, PyExc_RuntimeError, "Task list iteration limit exceeded");
            return -1;
        }

        uintptr_t next_node = GET_MEMBER(uintptr_t, task_node, unwinder->debug_offsets.llist_node.next);
        if (next_node == 0) {
            PyErr_SetString(PyExc_RuntimeError,
                           "Invalid linked list structure reading remote memory");
            set_exception_cause(unwinder, PyExc_RuntimeError, "NULL pointer in task linked list");
            return -1;
        }

        uintptr_t task_addr = next_node
            - (uintptr_t)unwinder->async_debug_offsets.asyncio_task_object.task_node;

        if (process_single_task_node(unwinder, task_addr, NULL, result) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to process task node in awaited_by");
            return -1;
        }

        // Read next node
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                next_node,
                sizeof(task_node),
                task_node) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read next task node in awaited_by");
            return -1;
        }
    }

    return 0;
}

int
append_awaited_by(
    RemoteUnwinderObject *unwinder,
    unsigned long tid,
    uintptr_t head_addr,
    PyObject *result)
{
    PyObject *tid_py = PyLong_FromUnsignedLong(tid);
    if (tid_py == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create thread ID object");
        return -1;
    }

    PyObject* awaited_by_for_thread = PyList_New(0);
    if (awaited_by_for_thread == NULL) {
        Py_DECREF(tid_py);
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create awaited_by thread list");
        return -1;
    }

    RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
    PyObject *result_item = PyStructSequence_New(state->AwaitedInfo_Type);
    if (result_item == NULL) {
        Py_DECREF(tid_py);
        Py_DECREF(awaited_by_for_thread);
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create AwaitedInfo");
        return -1;
    }

    PyStructSequence_SetItem(result_item, 0, tid_py);  // steals ref
    PyStructSequence_SetItem(result_item, 1, awaited_by_for_thread);  // steals ref
    if (PyList_Append(result, result_item)) {
        Py_DECREF(result_item);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append awaited_by result item");
        return -1;
    }
    Py_DECREF(result_item);

    if (append_awaited_by_for_thread(unwinder, head_addr, awaited_by_for_thread))
    {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append awaited_by for thread");
        return -1;
    }

    return 0;
}

/* ============================================================================
 * THREAD PROCESSOR FUNCTIONS FOR ASYNCIO
 * ============================================================================ */

int
process_thread_for_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    unsigned long tid,
    void *context
) {
    PyObject *result = (PyObject *)context;
    uintptr_t head_addr = thread_state_addr + (uintptr_t)unwinder->async_debug_offsets.asyncio_thread_state.asyncio_tasks_head;
    return append_awaited_by(unwinder, tid, head_addr, result);
}

static int
process_running_task_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t running_task_addr,
    uintptr_t thread_state_addr,
    PyObject *result
) {
    uintptr_t running_task_code_obj = 0;
    if(get_task_code_object(unwinder, running_task_addr, &running_task_code_obj) < 0) {
        return -1;
    }

    // First, add this task to the result
    PyObject *task_info = NULL;
    if (process_single_task_node(unwinder, running_task_addr, &task_info, result) < 0) {
        return -1;
    }

    // Get the chain from the current frame to this task
    PyObject *coro_chain = PyStructSequence_GET_ITEM(task_info, 2);
    assert(coro_chain != NULL);
    if (PyList_GET_SIZE(coro_chain) != 1) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Coro chain is not a single item");
        return -1;
    }
    PyObject *coro_info = PyList_GET_ITEM(coro_chain, 0);
    assert(coro_info != NULL);
    PyObject *frame_chain = PyStructSequence_GET_ITEM(coro_info, 0);
    assert(frame_chain != NULL);

    // Clear the coro_chain
    if (PyList_Clear(frame_chain) < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to clear coroutine chain");
        return -1;
    }

    // Add the chain from the current frame to this task
    if (parse_async_frame_chain(unwinder, frame_chain, thread_state_addr, running_task_code_obj) < 0) {
        return -1;
    }

    // Now find all tasks that are waiting for this task and process them
    if (process_task_awaited_by(unwinder, running_task_addr, process_waiter_task, result) < 0) {
        return -1;
    }

    return 0;
}

int
process_thread_for_async_stack_trace(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    unsigned long tid,
    void *context
) {
    PyObject *result = (PyObject *)context;

    // Find running task in this thread
    uintptr_t running_task_addr;
    if (find_running_task_in_thread(unwinder, thread_state_addr, &running_task_addr) < 0) {
        return 0;
    }

    // If we found a running task, process it and its waiters
    if ((void*)running_task_addr != NULL) {
        PyObject *task_list = PyList_New(0);
        if (task_list == NULL) {
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create task list for thread");
            return -1;
        }

        if (process_running_task_chain(unwinder, running_task_addr, thread_state_addr, task_list) < 0) {
            Py_DECREF(task_list);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to process running task chain");
            return -1;
        }

        // Create AwaitedInfo structure for this thread
        PyObject *tid_py = PyLong_FromUnsignedLong(tid);
        if (tid_py == NULL) {
            Py_DECREF(task_list);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create thread ID");
            return -1;
        }

        RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
        PyObject *awaited_info = PyStructSequence_New(state->AwaitedInfo_Type);
        if (awaited_info == NULL) {
            Py_DECREF(tid_py);
            Py_DECREF(task_list);
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create AwaitedInfo");
            return -1;
        }

        PyStructSequence_SetItem(awaited_info, 0, tid_py);  // steals ref
        PyStructSequence_SetItem(awaited_info, 1, task_list);  // steals ref

        if (PyList_Append(result, awaited_info)) {
            Py_DECREF(awaited_info);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to append AwaitedInfo to result");
            return -1;
        }
        Py_DECREF(awaited_info);
    }

    return 0;
}
