/******************************************************************************
 * Python Remote Debugging Module
 *
 * This module provides functionality to debug Python processes remotely by
 * reading their memory and reconstructing stack traces and asyncio task states.
 ******************************************************************************/

#define _GNU_SOURCE

/* ============================================================================
 * HEADERS AND INCLUDES
 * ============================================================================ */

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef Py_BUILD_CORE_BUILTIN
#    define Py_BUILD_CORE_MODULE 1
#endif
#include "Python.h"
#include <internal/pycore_debug_offsets.h>  // _Py_DebugOffsets
#include <internal/pycore_frame.h>          // FRAME_SUSPENDED_YIELD_FROM
#include <internal/pycore_interpframe.h>    // FRAME_OWNED_BY_CSTACK
#include <internal/pycore_llist.h>          // struct llist_node
#include <internal/pycore_stackref.h>       // Py_TAG_BITS
#include "../Python/remote_debug.h"

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

/* ============================================================================
 * TYPE DEFINITIONS AND STRUCTURES
 * ============================================================================ */

#define GET_MEMBER(type, obj, offset) (*(type*)((char*)(obj) + (offset)))

/* Size macros for opaque buffers */
#define SIZEOF_UNICODE_OBJ sizeof(PyUnicodeObject)
#define SIZEOF_BYTES_OBJ sizeof(PyBytesObject)
#define SIZEOF_INTERP_FRAME sizeof(_PyInterpreterFrame)
#define SIZEOF_GEN_OBJ sizeof(PyGenObject)
#define SIZEOF_SET_OBJ sizeof(PySetObject)
#define SIZEOF_TYPE_OBJ sizeof(PyTypeObject)
#define SIZEOF_TASK_OBJ 4096
#define SIZEOF_PYOBJECT sizeof(PyObject)
#define SIZEOF_LLIST_NODE sizeof(struct llist_node)
#define SIZEOF_THREAD_STATE sizeof(PyThreadState)

// Calculate the minimum buffer size needed to read both interpreter state fields
#define INTERP_STATE_MIN_SIZE MAX(offsetof(PyInterpreterState, _code_object_generation) + sizeof(uint64_t), \
                                  offsetof(PyInterpreterState, threads.head) + sizeof(void*))
#define INTERP_STATE_BUFFER_SIZE MAX(INTERP_STATE_MIN_SIZE, 256)



// Copied from Modules/_asynciomodule.c because it's not exported

struct _Py_AsyncioModuleDebugOffsets {
    struct _asyncio_task_object {
        uint64_t size;
        uint64_t task_name;
        uint64_t task_awaited_by;
        uint64_t task_is_task;
        uint64_t task_awaited_by_is_set;
        uint64_t task_coro;
        uint64_t task_node;
    } asyncio_task_object;
    struct _asyncio_interpreter_state {
        uint64_t size;
        uint64_t asyncio_tasks_head;
    } asyncio_interpreter_state;
    struct _asyncio_thread_state {
        uint64_t size;
        uint64_t asyncio_running_loop;
        uint64_t asyncio_running_task;
        uint64_t asyncio_tasks_head;
    } asyncio_thread_state;
};

typedef struct {
    PyObject_HEAD
    proc_handle_t handle;
    uintptr_t runtime_start_address;
    struct _Py_DebugOffsets debug_offsets;
    int async_debug_offsets_available;
    struct _Py_AsyncioModuleDebugOffsets async_debug_offsets;
    uintptr_t interpreter_addr;
    uintptr_t tstate_addr;
    uint64_t code_object_generation;
    _Py_hashtable_t *code_object_cache;
} RemoteUnwinderObject;

typedef struct {
    PyObject *func_name;
    PyObject *file_name;
    int first_lineno;
    PyObject *linetable;  // bytes
    uintptr_t addr_code_adaptive;
} CachedCodeMetadata;

typedef struct {
    /* Types */
    PyTypeObject *RemoteDebugging_Type;
} RemoteDebuggingState;

typedef struct
{
    int lineno;
    int end_lineno;
    int column;
    int end_column;
} LocationInfo;

typedef struct {
    uintptr_t remote_addr;
    size_t size;
    void *local_copy;
} StackChunkInfo;

typedef struct {
    StackChunkInfo *chunks;
    size_t count;
} StackChunkList;

#include "clinic/_remote_debugging_module.c.h"

/*[clinic input]
module _remote_debugging
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=5f507d5b2e76a7f7]*/


/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

static int
parse_tasks_in_set(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t set_addr,
    PyObject *awaited_by,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
);

static int
parse_task(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *render_to,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
);

static int
parse_coro_chain(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t coro_address,
    PyObject *render_to,
    _Py_hashtable_t *code_object_cache
);

/* Forward declarations for task parsing functions */
static int parse_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    const struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame,
    _Py_hashtable_t *code_object_cache
);

/* ============================================================================
 * UTILITY FUNCTIONS AND HELPERS
 * ============================================================================ */

static void
cached_code_metadata_destroy(void *ptr)
{
    CachedCodeMetadata *meta = (CachedCodeMetadata *)ptr;
    Py_DECREF(meta->func_name);
    Py_DECREF(meta->file_name);
    Py_DECREF(meta->linetable);
    PyMem_RawFree(meta);
}

static inline RemoteDebuggingState *
RemoteDebugging_GetState(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (RemoteDebuggingState *)state;
}

static inline int
RemoteDebugging_InitState(RemoteDebuggingState *st)
{
    return 0;
}

// Helper to chain exceptions and avoid repetitions
static void
chain_exceptions(PyObject *exception, const char *string)
{
    PyObject *exc = PyErr_GetRaisedException();
    PyErr_SetString(exception, string);
    _PyErr_ChainExceptions1(exc);
}

/* ============================================================================
 * MEMORY READING FUNCTIONS
 * ============================================================================ */

static inline int
read_ptr(proc_handle_t *handle, uintptr_t address, uintptr_t *ptr_addr)
{
    int result = _Py_RemoteDebug_PagedReadRemoteMemory(handle, address, sizeof(void*), ptr_addr);
    if (result < 0) {
        return -1;
    }
    return 0;
}

static inline int
read_Py_ssize_t(proc_handle_t *handle, uintptr_t address, Py_ssize_t *size)
{
    int result = _Py_RemoteDebug_PagedReadRemoteMemory(handle, address, sizeof(Py_ssize_t), size);
    if (result < 0) {
        return -1;
    }
    return 0;
}

static int
read_py_ptr(proc_handle_t *handle, uintptr_t address, uintptr_t *ptr_addr)
{
    if (read_ptr(handle, address, ptr_addr)) {
        return -1;
    }
    *ptr_addr &= ~Py_TAG_BITS;
    return 0;
}

static int
read_char(proc_handle_t *handle, uintptr_t address, char *result)
{
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(handle, address, sizeof(char), result);
    if (res < 0) {
        return -1;
    }
    return 0;
}

/* ============================================================================
 * PYTHON OBJECT READING FUNCTIONS
 * ============================================================================ */

static PyObject *
read_py_str(
    proc_handle_t *handle,
    const _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    Py_ssize_t max_len
) {
    PyObject *result = NULL;
    char *buf = NULL;

    // Read the entire PyUnicodeObject at once
    char unicode_obj[SIZEOF_UNICODE_OBJ];
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        address,
        SIZEOF_UNICODE_OBJ,
        unicode_obj
    );
    if (res < 0) {
        goto err;
    }

    Py_ssize_t len = GET_MEMBER(Py_ssize_t, unicode_obj, debug_offsets->unicode_object.length);
    if (len < 0 || len > max_len) {
        PyErr_Format(PyExc_RuntimeError,
                     "Invalid string length (%zd) at 0x%lx", len, address);
        return NULL;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    size_t offset = debug_offsets->unicode_object.asciiobject_size;
    res = _Py_RemoteDebug_PagedReadRemoteMemory(handle, address + offset, len, buf);
    if (res < 0) {
        goto err;
    }
    buf[len] = '\0';

    result = PyUnicode_FromStringAndSize(buf, len);
    if (result == NULL) {
        goto err;
    }

    PyMem_RawFree(buf);
    assert(result != NULL);
    return result;

err:
    if (buf != NULL) {
        PyMem_RawFree(buf);
    }
    return NULL;
}

static PyObject *
read_py_bytes(
    proc_handle_t *handle,
    const _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    Py_ssize_t max_len
) {
    PyObject *result = NULL;
    char *buf = NULL;

    // Read the entire PyBytesObject at once
    char bytes_obj[SIZEOF_BYTES_OBJ];
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        address,
        SIZEOF_BYTES_OBJ,
        bytes_obj
    );
    if (res < 0) {
        goto err;
    }

    Py_ssize_t len = GET_MEMBER(Py_ssize_t, bytes_obj, debug_offsets->bytes_object.ob_size);
    if (len < 0 || len > max_len) {
        PyErr_Format(PyExc_RuntimeError,
                     "Invalid string length (%zd) at 0x%lx", len, address);
        return NULL;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    size_t offset = debug_offsets->bytes_object.ob_sval;
    res = _Py_RemoteDebug_PagedReadRemoteMemory(handle, address + offset, len, buf);
    if (res < 0) {
        goto err;
    }
    buf[len] = '\0';

    result = PyBytes_FromStringAndSize(buf, len);
    if (result == NULL) {
        goto err;
    }

    PyMem_RawFree(buf);
    assert(result != NULL);
    return result;

err:
    if (buf != NULL) {
        PyMem_RawFree(buf);
    }
    return NULL;
}

static long
read_py_long(proc_handle_t *handle, const _Py_DebugOffsets* offsets, uintptr_t address)
{
    unsigned int shift = PYLONG_BITS_IN_DIGIT;

    // Read the entire PyLongObject at once
    char long_obj[offsets->long_object.size];
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        address,
        offsets->long_object.size,
        long_obj);
    if (bytes_read < 0) {
        return -1;
    }

    uintptr_t lv_tag = GET_MEMBER(uintptr_t, long_obj, offsets->long_object.lv_tag);
    int negative = (lv_tag & 3) == 2;
    Py_ssize_t size = lv_tag >> 3;

    if (size == 0) {
        return 0;
    }

    // If the long object has inline digits, use them directly
    digit *digits;
    if (size <= _PY_NSMALLNEGINTS + _PY_NSMALLPOSINTS) {
        // For small integers, digits are inline in the long_value.ob_digit array
        digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
        if (!digits) {
            PyErr_NoMemory();
            return -1;
        }
        memcpy(digits, long_obj + offsets->long_object.ob_digit, size * sizeof(digit));
    } else {
        // For larger integers, we need to read the digits separately
        digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
        if (!digits) {
            PyErr_NoMemory();
            return -1;
        }

        bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            address + offsets->long_object.ob_digit,
            sizeof(digit) * size,
            digits
        );
        if (bytes_read < 0) {
            goto error;
        }
    }

    long long value = 0;

    // In theory this can overflow, but because of llvm/llvm-project#16778
    // we can't use __builtin_mul_overflow because it fails to link with
    // __muloti4 on aarch64. In practice this is fine because all we're
    // testing here are task numbers that would fit in a single byte.
    for (Py_ssize_t i = 0; i < size; ++i) {
        long long factor = digits[i] * (1UL << (Py_ssize_t)(shift * i));
        value += factor;
    }
    PyMem_RawFree(digits);
    if (negative) {
        value *= -1;
    }
    return (long)value;
error:
    PyMem_RawFree(digits);
    return -1;
}

/* ============================================================================
 * ASYNCIO DEBUG FUNCTIONS
 * ============================================================================ */

// Get the PyAsyncioDebug section address for any platform
static uintptr_t
_Py_RemoteDebug_GetAsyncioDebugAddress(proc_handle_t* handle)
{
    uintptr_t address;

#ifdef MS_WINDOWS
    // On Windows, search for asyncio debug in executable or DLL
    address = search_windows_map_for_section(handle, "AsyncioD", L"_asyncio");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__linux__)
    // On Linux, search for asyncio debug in executable or DLL
    address = search_linux_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    address = search_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
    if (address == 0) {
        PyErr_Clear();
        address = search_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
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

static int
read_async_debug(
    proc_handle_t *handle,
    struct _Py_AsyncioModuleDebugOffsets* async_debug
) {
    uintptr_t async_debug_addr = _Py_RemoteDebug_GetAsyncioDebugAddress(handle);
    if (!async_debug_addr) {
        return -1;
    }

    size_t size = sizeof(struct _Py_AsyncioModuleDebugOffsets);
    int result = _Py_RemoteDebug_PagedReadRemoteMemory(handle, async_debug_addr, size, async_debug);
    return result;
}

/* ============================================================================
 * ASYNCIO TASK PARSING FUNCTIONS
 * ============================================================================ */

static PyObject *
parse_task_name(
    proc_handle_t *handle,
    const _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address
) {
    // Read the entire TaskObj at once
    char task_obj[async_offsets->asyncio_task_object.size];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        task_address,
        async_offsets->asyncio_task_object.size,
        task_obj);
    if (err < 0) {
        return NULL;
    }

    uintptr_t task_name_addr = GET_MEMBER(uintptr_t, task_obj, async_offsets->asyncio_task_object.task_name);
    task_name_addr &= ~Py_TAG_BITS;

    // The task name can be a long or a string so we need to check the type
    char task_name_obj[SIZEOF_PYOBJECT];
    err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        task_name_addr,
        SIZEOF_PYOBJECT,
        task_name_obj);
    if (err < 0) {
        return NULL;
    }

    // Now read the type object to get the flags
    char type_obj[SIZEOF_TYPE_OBJ];
    err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        GET_MEMBER(uintptr_t, task_name_obj, offsets->pyobject.ob_type),
        SIZEOF_TYPE_OBJ,
        type_obj);
    if (err < 0) {
        return NULL;
    }

    if ((GET_MEMBER(unsigned long, type_obj, offsets->type_object.tp_flags) & Py_TPFLAGS_LONG_SUBCLASS)) {
        long res = read_py_long(handle, offsets, task_name_addr);
        if (res == -1) {
            chain_exceptions(PyExc_RuntimeError, "Failed to get task name");
            return NULL;
        }
        return PyUnicode_FromFormat("Task-%d", res);
    }

    if(!(GET_MEMBER(unsigned long, type_obj, offsets->type_object.tp_flags) & Py_TPFLAGS_UNICODE_SUBCLASS)) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid task name object");
        return NULL;
    }

    return read_py_str(
        handle,
        offsets,
        task_name_addr,
        255
    );
}

static int parse_task_awaited_by(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *awaited_by,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
) {
    // Read the entire TaskObj at once
    char task_obj[async_offsets->asyncio_task_object.size];
    if (_Py_RemoteDebug_PagedReadRemoteMemory(handle, task_address,
                                              async_offsets->asyncio_task_object.size,
                                              task_obj) < 0) {
        return -1;
    }

    uintptr_t task_ab_addr = GET_MEMBER(uintptr_t, task_obj, async_offsets->asyncio_task_object.task_awaited_by);
    task_ab_addr &= ~Py_TAG_BITS;

    if ((void*)task_ab_addr == NULL) {
        return 0;
    }

    char awaited_by_is_a_set = GET_MEMBER(char, task_obj, async_offsets->asyncio_task_object.task_awaited_by_is_set);

    if (awaited_by_is_a_set) {
        if (parse_tasks_in_set(handle, offsets, async_offsets, task_ab_addr,
                               awaited_by, recurse_task, code_object_cache)) {
            return -1;
        }
    } else {
        if (parse_task(handle, offsets, async_offsets, task_ab_addr,
                       awaited_by, recurse_task, code_object_cache)) {
            return -1;
        }
    }

    return 0;
}

static int
handle_yield_from_frame(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t gi_iframe_addr,
    uintptr_t gen_type_addr,
    PyObject *render_to,
    _Py_hashtable_t *code_object_cache
) {
    // Read the entire interpreter frame at once
    char iframe[10000];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        gi_iframe_addr,
        SIZEOF_INTERP_FRAME,
        iframe);
    if (err < 0) {
        return -1;
    }

    if (GET_MEMBER(char, iframe, offsets->interpreter_frame.owner) != FRAME_OWNED_BY_GENERATOR) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "generator doesn't own its frame \\_o_/");
        return -1;
    }

    uintptr_t stackpointer_addr = GET_MEMBER(uintptr_t, iframe, offsets->interpreter_frame.stackpointer);
    stackpointer_addr &= ~Py_TAG_BITS;

    if ((void*)stackpointer_addr != NULL) {
        uintptr_t gi_await_addr;
        err = read_py_ptr(
            handle,
            stackpointer_addr - sizeof(void*),
            &gi_await_addr);
        if (err) {
            return -1;
        }

        if ((void*)gi_await_addr != NULL) {
            uintptr_t gi_await_addr_type_addr;
            err = read_ptr(
                handle,
                gi_await_addr + offsets->pyobject.ob_type,
                &gi_await_addr_type_addr);
            if (err) {
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
                err = parse_coro_chain(
                    handle,
                    offsets,
                    async_offsets,
                    gi_await_addr,
                    render_to,
                    code_object_cache
                );
                if (err) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

static int
parse_coro_chain(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t coro_address,
    PyObject *render_to,
    _Py_hashtable_t *code_object_cache
) {
    assert((void*)coro_address != NULL);

    // Read the entire generator object at once
    char gen_object[SIZEOF_GEN_OBJ];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        coro_address,
        SIZEOF_GEN_OBJ,
        gen_object);
    if (err < 0) {
        return -1;
    }

    uintptr_t gen_type_addr = GET_MEMBER(uintptr_t, gen_object, offsets->pyobject.ob_type);

    PyObject* name = NULL;

    // Parse the previous frame using the gi_iframe from local copy
    uintptr_t prev_frame;
    uintptr_t gi_iframe_addr = coro_address + offsets->gen_object.gi_iframe;
    if (parse_frame_object(
                handle,
                &name,
                offsets,
                gi_iframe_addr,
                &prev_frame, code_object_cache)
        < 0)
    {
        return -1;
    }

    if (PyList_Append(render_to, name)) {
        Py_DECREF(name);
        return -1;
    }
    Py_DECREF(name);

    if (GET_MEMBER(char, gen_object, offsets->gen_object.gi_frame_state) == FRAME_SUSPENDED_YIELD_FROM) {
        return handle_yield_from_frame(
            handle, offsets, async_offsets, gi_iframe_addr,
            gen_type_addr, render_to, code_object_cache);
    }

    return 0;
}

static PyObject*
create_task_result(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
) {
    PyObject* result = NULL;
    PyObject *call_stack = NULL;
    PyObject *tn = NULL;
    char task_obj[async_offsets->asyncio_task_object.size];
    uintptr_t coro_addr;
    
    result = PyList_New(0);
    if (result == NULL) {
        goto error;
    }

    call_stack = PyList_New(0);
    if (call_stack == NULL) {
        goto error;
    }
    
    if (PyList_Append(result, call_stack)) {
        goto error;
    }
    Py_CLEAR(call_stack);

    if (recurse_task) {
        tn = parse_task_name(handle, offsets, async_offsets, task_address);
    } else {
        tn = PyLong_FromUnsignedLongLong(task_address);
    }
    if (tn == NULL) {
        goto error;
    }
    
    if (PyList_Append(result, tn)) {
        goto error;
    }
    Py_CLEAR(tn);

    // Parse coroutine chain
    if (_Py_RemoteDebug_PagedReadRemoteMemory(handle, task_address,
                                              async_offsets->asyncio_task_object.size,
                                              task_obj) < 0) {
        goto error;
    }

    coro_addr = GET_MEMBER(uintptr_t, task_obj, async_offsets->asyncio_task_object.task_coro);
    coro_addr &= ~Py_TAG_BITS;

    if ((void*)coro_addr != NULL) {
        call_stack = PyList_New(0);
        if (call_stack == NULL) {
            goto error;
        }
        
        if (parse_coro_chain(handle, offsets, async_offsets, coro_addr,
                             call_stack, code_object_cache) < 0) {
            Py_DECREF(call_stack);
            goto error;
        }

        if (PyList_Reverse(call_stack)) {
            Py_DECREF(call_stack);
            goto error;
        }

        if (PyList_SetItem(result, 0, call_stack) < 0) {
            Py_DECREF(call_stack);
            goto error;
        }
    }

    return result;

error:
    Py_XDECREF(result);
    Py_XDECREF(call_stack);
    Py_XDECREF(tn);
    return NULL;
}

static int
parse_task(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *render_to,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
) {
    char is_task;
    int err = read_char(
        handle,
        task_address + async_offsets->asyncio_task_object.task_is_task,
        &is_task);
    if (err) {
        return -1;
    }

    PyObject* result = NULL;
    if (is_task) {
        result = create_task_result(handle, offsets, async_offsets,
                                   task_address, recurse_task, code_object_cache);
        if (!result) {
            return -1;
        }
    } else {
        result = PyList_New(0);
        if (result == NULL) {
            return -1;
        }
    }

    if (PyList_Append(render_to, result)) {
        Py_DECREF(result);
        return -1;
    }

    if (recurse_task) {
        PyObject *awaited_by = PyList_New(0);
        if (awaited_by == NULL) {
            Py_DECREF(result);
            return -1;
        }
        if (PyList_Append(result, awaited_by)) {
            Py_DECREF(awaited_by);
            Py_DECREF(result);
            return -1;
        }
        /* we can operate on a borrowed one to simplify cleanup */
        Py_DECREF(awaited_by);

        if (parse_task_awaited_by(handle, offsets, async_offsets,
                                task_address, awaited_by, 1, code_object_cache) < 0) {
            Py_DECREF(result);
            return -1;
        }
    }
    Py_DECREF(result);

    return 0;
}

static int
process_set_entry(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t table_ptr,
    PyObject *awaited_by,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
) {
    uintptr_t key_addr;
    if (read_py_ptr(handle, table_ptr, &key_addr)) {
        return -1;
    }

    if ((void*)key_addr != NULL) {
        Py_ssize_t ref_cnt;
        if (read_Py_ssize_t(handle, table_ptr, &ref_cnt)) {
            return -1;
        }

        if (ref_cnt) {
            // if 'ref_cnt=0' it's a set dummy marker
            if (parse_task(
                handle,
                offsets,
                async_offsets,
                key_addr,
                awaited_by,
                recurse_task,
                code_object_cache
            )) {
                return -1;
            }
            return 1; // Successfully processed a valid entry
        }
    }
    return 0; // Entry was NULL or dummy marker
}

static int
parse_tasks_in_set(
    proc_handle_t *handle,
    const struct _Py_DebugOffsets* offsets,
    const struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t set_addr,
    PyObject *awaited_by,
    int recurse_task,
    _Py_hashtable_t *code_object_cache
) {
    char set_object[SIZEOF_SET_OBJ];
    int err = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        set_addr,
        SIZEOF_SET_OBJ,
        set_object);
    if (err < 0) {
        return -1;
    }

    Py_ssize_t num_els = GET_MEMBER(Py_ssize_t, set_object, offsets->set_object.used);
    Py_ssize_t set_len = GET_MEMBER(Py_ssize_t, set_object, offsets->set_object.mask) + 1; // The set contains the `mask+1` element slots.
    uintptr_t table_ptr = GET_MEMBER(uintptr_t, set_object, offsets->set_object.table);

    Py_ssize_t i = 0;
    Py_ssize_t els = 0;
    while (i < set_len && els < num_els) {
        int result = process_set_entry(
            handle, offsets, async_offsets, table_ptr,
            awaited_by, recurse_task, code_object_cache);

        if (result < 0) {
            return -1;
        }
        if (result > 0) {
            els++;
        }

        table_ptr += sizeof(void*) * 2;
        i++;
    }
    return 0;
}


static int
setup_async_result_structure(PyObject **result, PyObject **calls)
{
    *result = PyList_New(1);
    if (*result == NULL) {
        return -1;
    }
    
    *calls = PyList_New(0);
    if (*calls == NULL) {
        Py_DECREF(*result);
        *result = NULL;
        return -1;
    }
    
    if (PyList_SetItem(*result, 0, *calls)) { /* steals ref to 'calls' */
        Py_DECREF(*calls);
        Py_DECREF(*result);
        *result = NULL;
        *calls = NULL;
        return -1;
    }
    
    return 0;
}

static int
add_task_info_to_result(
    RemoteUnwinderObject *self,
    PyObject *result,
    uintptr_t running_task_addr
) {
    PyObject *tn = parse_task_name(
        &self->handle, &self->debug_offsets, &self->async_debug_offsets, 
        running_task_addr);
    if (tn == NULL) {
        return -1;
    }
    
    if (PyList_Append(result, tn)) {
        Py_DECREF(tn);
        return -1;
    }
    Py_DECREF(tn);

    PyObject* awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        return -1;
    }
    
    if (PyList_Append(result, awaited_by)) {
        Py_DECREF(awaited_by);
        return -1;
    }
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(
        &self->handle, &self->debug_offsets, &self->async_debug_offsets,
        running_task_addr, awaited_by, 1, self->code_object_cache) < 0) {
        return -1;
    }

    return 0;
}

static int
process_single_task_node(
    proc_handle_t *handle,
    uintptr_t task_addr,
    const struct _Py_DebugOffsets *debug_offsets,
    const struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    PyObject *result,
    _Py_hashtable_t *code_object_cache
) {
    PyObject *tn = NULL;
    PyObject *current_awaited_by = NULL;
    PyObject *task_id = NULL;
    PyObject *result_item = NULL;

    tn = parse_task_name(handle, debug_offsets, async_offsets, task_addr);
    if (tn == NULL) {
        goto error;
    }

    current_awaited_by = PyList_New(0);
    if (current_awaited_by == NULL) {
        goto error;
    }

    task_id = PyLong_FromUnsignedLongLong(task_addr);
    if (task_id == NULL) {
        goto error;
    }

    result_item = PyTuple_New(3);
    if (result_item == NULL) {
        goto error;
    }

    PyTuple_SET_ITEM(result_item, 0, task_id);  // steals ref
    PyTuple_SET_ITEM(result_item, 1, tn);  // steals ref
    PyTuple_SET_ITEM(result_item, 2, current_awaited_by);  // steals ref
    
    // References transferred to tuple
    task_id = NULL;
    tn = NULL;
    current_awaited_by = NULL;

    if (PyList_Append(result, result_item)) {
        Py_DECREF(result_item);
        return -1;
    }
    Py_DECREF(result_item);

    // Get back current_awaited_by reference for parse_task_awaited_by
    current_awaited_by = PyTuple_GET_ITEM(result_item, 2);
    if (parse_task_awaited_by(handle, debug_offsets, async_offsets,
                              task_addr, current_awaited_by, 0, code_object_cache) < 0) {
        return -1;
    }

    return 0;

error:
    Py_XDECREF(tn);
    Py_XDECREF(current_awaited_by);
    Py_XDECREF(task_id);
    Py_XDECREF(result_item);
    return -1;
}

/* ============================================================================
 * LINE TABLE PARSING FUNCTIONS
 * ============================================================================ */

static int
scan_varint(const uint8_t **ptr)
{
    unsigned int read = **ptr;
    *ptr = *ptr + 1;
    unsigned int val = read & 63;
    unsigned int shift = 0;
    while (read & 64) {
        read = **ptr;
        *ptr = *ptr + 1;
        shift += 6;
        val |= (read & 63) << shift;
    }
    return val;
}

static int
scan_signed_varint(const uint8_t **ptr)
{
    unsigned int uval = scan_varint(ptr);
    if (uval & 1) {
        return -(int)(uval >> 1);
    }
    else {
        return uval >> 1;
    }
}

static bool
parse_linetable(const uintptr_t addrq, const char* linetable, int firstlineno, LocationInfo* info)
{
    const uint8_t* ptr = (const uint8_t*)(linetable);
    uint64_t addr = 0;
    info->lineno = firstlineno;

    while (*ptr != '\0') {
        // See InternalDocs/code_objects.md for where these magic numbers are from
        // and for the decoding algorithm.
        uint8_t first_byte = *(ptr++);
        uint8_t code = (first_byte >> 3) & 15;
        size_t length = (first_byte & 7) + 1;
        uintptr_t end_addr = addr + length;
        switch (code) {
            case PY_CODE_LOCATION_INFO_NONE: {
                break;
            }
            case PY_CODE_LOCATION_INFO_LONG: {
                int line_delta = scan_signed_varint(&ptr);
                info->lineno += line_delta;
                info->end_lineno = info->lineno + scan_varint(&ptr);
                info->column = scan_varint(&ptr) - 1;
                info->end_column = scan_varint(&ptr) - 1;
                break;
            }
            case PY_CODE_LOCATION_INFO_NO_COLUMNS: {
                int line_delta = scan_signed_varint(&ptr);
                info->lineno += line_delta;
                info->column = info->end_column = -1;
                break;
            }
            case PY_CODE_LOCATION_INFO_ONE_LINE0:
            case PY_CODE_LOCATION_INFO_ONE_LINE1:
            case PY_CODE_LOCATION_INFO_ONE_LINE2: {
                int line_delta = code - 10;
                info->lineno += line_delta;
                info->end_lineno = info->lineno;
                info->column = *(ptr++);
                info->end_column = *(ptr++);
                break;
            }
            default: {
                uint8_t second_byte = *(ptr++);
                if ((second_byte & 128) != 0) {
                    return false;
                }
                info->column = code << 3 | (second_byte >> 4);
                info->end_column = info->column + (second_byte & 15);
                break;
            }
        }
        if (addr <= addrq && end_addr > addrq) {
            return true;
        }
        addr = end_addr;
    }
    return false;
}

/* ============================================================================
 * CODE OBJECT AND FRAME PARSING FUNCTIONS
 * ============================================================================ */

static int
parse_code_object(proc_handle_t *handle,
                  PyObject **result,
                  const struct _Py_DebugOffsets *offsets,
                  uintptr_t address,
                  uintptr_t instruction_pointer,
                  uintptr_t *previous_frame,
                  _Py_hashtable_t *code_object_cache)
{
    void *key = (void *)address;
    CachedCodeMetadata *meta = NULL;
    PyObject *func = NULL;
    PyObject *file = NULL;
    PyObject *linetable = NULL;
    PyObject *lineno = NULL;
    PyObject *tuple = NULL;

    if (code_object_cache != NULL) {
        meta = _Py_hashtable_get(code_object_cache, key);
    }

    if (meta == NULL) {
        char code_object[offsets->code_object.size];
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                handle, address, offsets->code_object.size, code_object) < 0)
        {
            goto error;
        }

        func = read_py_str(handle, offsets,
            GET_MEMBER(uintptr_t, code_object, offsets->code_object.qualname), 1024);
        if (!func) {
            goto error;
        }

        file = read_py_str(handle, offsets,
            GET_MEMBER(uintptr_t, code_object, offsets->code_object.filename), 1024);
        if (!file) {
            goto error;
        }

        linetable = read_py_bytes(handle, offsets,
            GET_MEMBER(uintptr_t, code_object, offsets->code_object.linetable), 4096);
        if (!linetable) {
            goto error;
        }

        meta = PyMem_RawMalloc(sizeof(CachedCodeMetadata));
        if (!meta) {
            goto error;
        }

        meta->func_name = func;
        meta->file_name = file;
        meta->linetable = linetable;
        meta->first_lineno = GET_MEMBER(int, code_object, offsets->code_object.firstlineno);
        meta->addr_code_adaptive = address + offsets->code_object.co_code_adaptive;

        if (code_object_cache && _Py_hashtable_set(code_object_cache, key, meta) < 0) {
            cached_code_metadata_destroy(meta);
            goto error;
        }

        // Ownership transferred to meta
        func = NULL;
        file = NULL;
        linetable = NULL;
    }

    uintptr_t ip = instruction_pointer;
    ptrdiff_t addrq = (uint16_t *)ip - (uint16_t *)meta->addr_code_adaptive;

    LocationInfo info = {0};
    bool ok = parse_linetable(addrq, PyBytes_AS_STRING(meta->linetable),
                              meta->first_lineno, &info);
    if (!ok) {
        info.lineno = -1;
    }

    lineno = PyLong_FromLong(info.lineno);
    if (!lineno) {
        goto error;
    }

    tuple = PyTuple_New(3);
    if (!tuple) {
        goto error;
    }

    Py_INCREF(meta->func_name);
    Py_INCREF(meta->file_name);
    PyTuple_SET_ITEM(tuple, 0, meta->func_name);
    PyTuple_SET_ITEM(tuple, 1, meta->file_name);
    PyTuple_SET_ITEM(tuple, 2, lineno);

    *result = tuple;
    return 0;

error:
    Py_XDECREF(func);
    Py_XDECREF(file);
    Py_XDECREF(linetable);
    Py_XDECREF(lineno);
    Py_XDECREF(tuple);
    return -1;
}

/* ============================================================================
 * STACK CHUNK MANAGEMENT FUNCTIONS
 * ============================================================================ */

static void
cleanup_stack_chunks(StackChunkList *chunks)
{
    for (size_t i = 0; i < chunks->count; ++i) {
        PyMem_RawFree(chunks->chunks[i].local_copy);
    }
    PyMem_RawFree(chunks->chunks);
}

static int
process_single_stack_chunk(
    proc_handle_t *handle,
    uintptr_t chunk_addr,
    StackChunkInfo *chunk_info
) {
    // Start with default size assumption
    size_t current_size = _PY_DATA_STACK_CHUNK_SIZE;
    
    char *this_chunk = PyMem_RawMalloc(current_size);
    if (!this_chunk) {
        PyErr_NoMemory();
        return -1;
    }

    if (_Py_RemoteDebug_PagedReadRemoteMemory(handle, chunk_addr, current_size, this_chunk) < 0) {
        PyMem_RawFree(this_chunk);
        return -1;
    }

    // Check actual size and reread if necessary
    size_t actual_size = GET_MEMBER(size_t, this_chunk, offsetof(_PyStackChunk, size));
    if (actual_size != current_size) {
        this_chunk = PyMem_RawRealloc(this_chunk, actual_size);
        if (!this_chunk) {
            PyErr_NoMemory();
            return -1;
        }
        
        if (_Py_RemoteDebug_PagedReadRemoteMemory(handle, chunk_addr, actual_size, this_chunk) < 0) {
            PyMem_RawFree(this_chunk);
            return -1;
        }
        current_size = actual_size;
    }

    chunk_info->remote_addr = chunk_addr;
    chunk_info->size = current_size;
    chunk_info->local_copy = this_chunk;
    return 0;
}

static int
copy_stack_chunks(proc_handle_t *handle,
                  uintptr_t tstate_addr,
                  const _Py_DebugOffsets *offsets,
                  StackChunkList *out_chunks)
{
    uintptr_t chunk_addr;
    StackChunkInfo *chunks = NULL;
    size_t count = 0;
    size_t max_chunks = 16;
    
    if (read_ptr(handle, tstate_addr + offsets->thread_state.datastack_chunk, &chunk_addr)) {
        return -1;
    }

    chunks = PyMem_RawMalloc(max_chunks * sizeof(StackChunkInfo));
    if (!chunks) {
        PyErr_NoMemory();
        return -1;
    }

    while (chunk_addr != 0) {
        // Grow array if needed
        if (count >= max_chunks) {
            max_chunks *= 2;
            StackChunkInfo *new_chunks = PyMem_RawRealloc(chunks, max_chunks * sizeof(StackChunkInfo));
            if (!new_chunks) {
                PyErr_NoMemory();
                goto error;
            }
            chunks = new_chunks;
        }

        // Process this chunk
        if (process_single_stack_chunk(handle, chunk_addr, &chunks[count]) < 0) {
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

static void *
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

static int
parse_frame_from_chunks(
    proc_handle_t *handle,
    PyObject **result,
    const struct _Py_DebugOffsets *offsets,
    uintptr_t address,
    uintptr_t *previous_frame,
    StackChunkList *chunks,
    _Py_hashtable_t *code_object_cache
) {
    void *frame_ptr = find_frame_in_chunks(chunks, address);
    if (!frame_ptr) {
        return -1;
    }

    char *frame = (char *)frame_ptr;
    *previous_frame = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.previous);

    if (GET_MEMBER(char, frame, offsets->interpreter_frame.owner) >= FRAME_OWNED_BY_INTERPRETER ||
        !GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.executable)) {
        return 0;
    }

    uintptr_t instruction_pointer = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.instr_ptr);

    return parse_code_object(
        handle, result, offsets, GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.executable),
        instruction_pointer, previous_frame, code_object_cache);
}

/* ============================================================================
 * INTERPRETER STATE AND THREAD DISCOVERY FUNCTIONS
 * ============================================================================ */

static int
populate_initial_state_data(
    int all_threads,
    proc_handle_t *handle,
    uintptr_t runtime_start_address,
    const _Py_DebugOffsets* local_debug_offsets,
    uintptr_t *interpreter_state,
    uintptr_t *tstate
) {
    uint64_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    *interpreter_state = address_of_interpreter_state;

    if (all_threads) {
        *tstate = 0;
        return 0;
    }

    uintptr_t address_of_thread = address_of_interpreter_state +
                    local_debug_offsets->interpreter_state.threads_main;

    if (_Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            address_of_thread,
            sizeof(void*),
            tstate) < 0) {
        return -1;
    }

    return 0;
}

static int
find_running_frame(
    proc_handle_t *handle,
    uintptr_t runtime_start_address,
    const _Py_DebugOffsets* local_debug_offsets,
    uintptr_t *frame
) {
    uint64_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    uintptr_t address_of_thread;
    bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_main,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        return -1;
    }

    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread != NULL) {
        int err = read_ptr(
            handle,
            address_of_thread + local_debug_offsets->thread_state.current_frame,
            frame);
        if (err) {
            return -1;
        }
        return 0;
    }

    *frame = (uintptr_t)NULL;
    return 0;
}

static int
find_running_task(
    proc_handle_t *handle,
    uintptr_t runtime_start_address,
    const _Py_DebugOffsets *local_debug_offsets,
    struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    uintptr_t *running_task_addr
) {
    *running_task_addr = (uintptr_t)NULL;

    uint64_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &address_of_interpreter_state);
    if (bytes_read < 0) {
        return -1;
    }

    if (address_of_interpreter_state == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    uintptr_t address_of_thread;
    bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            handle,
            address_of_interpreter_state +
                local_debug_offsets->interpreter_state.threads_head,
            sizeof(void*),
            &address_of_thread);
    if (bytes_read < 0) {
        return -1;
    }

    uintptr_t address_of_running_loop;
    // No Python frames are available for us (can happen at tear-down).
    if ((void*)address_of_thread == NULL) {
        return 0;
    }

    bytes_read = read_py_ptr(
        handle,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_loop,
        &address_of_running_loop);
    if (bytes_read == -1) {
        return -1;
    }

    // no asyncio loop is now running
    if ((void*)address_of_running_loop == NULL) {
        return 0;
    }

    int err = read_ptr(
        handle,
        address_of_thread
        + async_offsets->asyncio_thread_state.asyncio_running_task,
        running_task_addr);
    if (err) {
        return -1;
    }

    return 0;
}

static int
find_running_task_and_coro(
    RemoteUnwinderObject *self,
    uintptr_t *running_task_addr,
    uintptr_t *running_coro_addr,
    uintptr_t *running_task_code_obj
) {
    *running_task_addr = (uintptr_t)NULL;
    if (find_running_task(
        &self->handle, self->runtime_start_address, 
        &self->debug_offsets, &self->async_debug_offsets,
        running_task_addr) < 0) {
        chain_exceptions(PyExc_RuntimeError, "Failed to find running task");
        return -1;
    }

    if ((void*)*running_task_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No running task found");
        return -1;
    }

    if (read_py_ptr(
        &self->handle,
        *running_task_addr + self->async_debug_offsets.asyncio_task_object.task_coro,
        running_coro_addr) < 0) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read running task coro");
        return -1;
    }

    if ((void*)*running_coro_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task coro is NULL");
        return -1;
    }

    // note: genobject's gi_iframe is an embedded struct so the address to
    // the offset leads directly to its first field: f_executable
    if (read_py_ptr(
        &self->handle,
        *running_coro_addr + self->debug_offsets.gen_object.gi_iframe,
        running_task_code_obj) < 0) {
        return -1;
    }

    if ((void*)*running_task_code_obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task code object is NULL");
        return -1;
    }

    return 0;
}


/* ============================================================================
 * FRAME PARSING FUNCTIONS
 * ============================================================================ */

static int
parse_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    const struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame,
    _Py_hashtable_t *code_object_cache
) {
    char frame[SIZEOF_INTERP_FRAME];

    Py_ssize_t bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        address,
        SIZEOF_INTERP_FRAME,
        frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    *previous_frame = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.previous);

    if (GET_MEMBER(char, frame, offsets->interpreter_frame.owner) >= FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }

    if ((void*)GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.executable) == NULL) {
        return 0;
    }

    uintptr_t instruction_pointer = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.instr_ptr);

    return parse_code_object(
        handle, result, offsets, GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.executable),
        instruction_pointer, previous_frame, code_object_cache);
}

static int
parse_async_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    const struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame,
    uintptr_t* code_object,
    _Py_hashtable_t *code_object_cache
) {
    char frame[SIZEOF_INTERP_FRAME];

    Py_ssize_t bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle,
        address,
        SIZEOF_INTERP_FRAME,
        frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    *previous_frame = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.previous);

    if (GET_MEMBER(char, frame, offsets->interpreter_frame.owner) == FRAME_OWNED_BY_CSTACK ||
        GET_MEMBER(char, frame, offsets->interpreter_frame.owner) == FRAME_OWNED_BY_INTERPRETER) {
        return 0;  // C frame
    }

    if (GET_MEMBER(char, frame, offsets->interpreter_frame.owner) != FRAME_OWNED_BY_GENERATOR
        && GET_MEMBER(char, frame, offsets->interpreter_frame.owner) != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n",
                    GET_MEMBER(char, frame, offsets->interpreter_frame.owner));
        return -1;
    }

    *code_object = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.executable);

    assert(code_object != NULL);
    if ((void*)*code_object == NULL) {
        return 0;
    }

    uintptr_t instruction_pointer = GET_MEMBER(uintptr_t, frame, offsets->interpreter_frame.instr_ptr);

    if (parse_code_object(
        handle, result, offsets, *code_object, instruction_pointer, previous_frame, code_object_cache)) {
        return -1;
    }

    return 1;
}

static int
parse_async_frame_chain(
    RemoteUnwinderObject *self,
    PyObject *calls,
    uintptr_t running_task_code_obj
) {
    uintptr_t address_of_current_frame;
    if (find_running_frame(
        &self->handle, self->runtime_start_address, &self->debug_offsets,
        &address_of_current_frame) < 0) {
        chain_exceptions(PyExc_RuntimeError, "Failed to find running frame");
        return -1;
    }

    uintptr_t address_of_code_object;
    while ((void*)address_of_current_frame != NULL) {
        PyObject* frame_info = NULL;
        int res = parse_async_frame_object(
            &self->handle,
            &frame_info,
            &self->debug_offsets,
            address_of_current_frame,
            &address_of_current_frame,
            &address_of_code_object,
            self->code_object_cache
        );

        if (res < 0) {
            chain_exceptions(PyExc_RuntimeError, "Failed to parse async frame object");
            return -1;
        }

        if (!frame_info) {
            continue;
        }

        if (PyList_Append(calls, frame_info) == -1) {
            Py_DECREF(frame_info);
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
    proc_handle_t *handle,
    uintptr_t head_addr,
    const struct _Py_DebugOffsets *debug_offsets,
    const struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    PyObject *result,
    _Py_hashtable_t *code_object_cache
) {
    char task_node[SIZEOF_LLIST_NODE];

    if (_Py_RemoteDebug_PagedReadRemoteMemory(handle, head_addr, 
                                              sizeof(task_node), task_node) < 0) {
        return -1;
    }

    size_t iteration_count = 0;
    const size_t MAX_ITERATIONS = 2 << 15;  // A reasonable upper bound
    
    while (GET_MEMBER(uintptr_t, task_node, debug_offsets->llist_node.next) != head_addr) {
        if (++iteration_count > MAX_ITERATIONS) {
            PyErr_SetString(PyExc_RuntimeError, "Task list appears corrupted");
            return -1;
        }

        if (GET_MEMBER(uintptr_t, task_node, debug_offsets->llist_node.next) == 0) {
            PyErr_SetString(PyExc_RuntimeError,
                           "Invalid linked list structure reading remote memory");
            return -1;
        }

        uintptr_t task_addr = (uintptr_t)GET_MEMBER(uintptr_t, task_node, debug_offsets->llist_node.next)
            - async_offsets->asyncio_task_object.task_node;

        if (process_single_task_node(handle, task_addr, debug_offsets, async_offsets,
                                     result, code_object_cache) < 0) {
            return -1;
        }

        // Read next node
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                handle,
                (uintptr_t)GET_MEMBER(uintptr_t, task_node, offsetof(struct llist_node, next)),
                sizeof(task_node),
                task_node) < 0) {
            return -1;
        }
    }

    return 0;
}

static int
append_awaited_by(
    proc_handle_t *handle,
    unsigned long tid,
    uintptr_t head_addr,
    const struct _Py_DebugOffsets *debug_offsets,
    const struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    _Py_hashtable_t *code_object_cache,
    PyObject *result)
{
    PyObject *tid_py = PyLong_FromUnsignedLong(tid);
    if (tid_py == NULL) {
        return -1;
    }

    PyObject *result_item = PyTuple_New(2);
    if (result_item == NULL) {
        Py_DECREF(tid_py);
        return -1;
    }

    PyObject* awaited_by_for_thread = PyList_New(0);
    if (awaited_by_for_thread == NULL) {
        Py_DECREF(tid_py);
        Py_DECREF(result_item);
        return -1;
    }

    PyTuple_SET_ITEM(result_item, 0, tid_py);  // steals ref
    PyTuple_SET_ITEM(result_item, 1, awaited_by_for_thread);  // steals ref
    if (PyList_Append(result, result_item)) {
        Py_DECREF(result_item);
        return -1;
    }
    Py_DECREF(result_item);

    if (append_awaited_by_for_thread(
            handle,
            head_addr,
            debug_offsets,
            async_offsets,
            awaited_by_for_thread,
            code_object_cache))
    {
        return -1;
    }

    return 0;
}

/* ============================================================================
 * STACK UNWINDING FUNCTIONS
 * ============================================================================ */

static int
process_frame_chain(
    proc_handle_t *handle,
    const _Py_DebugOffsets *offsets,
    uintptr_t initial_frame_addr,
    StackChunkList *chunks,
    _Py_hashtable_t *code_object_cache,
    PyObject *frame_info
) {
    uintptr_t frame_addr = initial_frame_addr;
    uintptr_t prev_frame_addr = 0;
    const size_t MAX_FRAMES = 1024;
    size_t frame_count = 0;

    while ((void*)frame_addr != NULL) {
        PyObject *frame = NULL;
        uintptr_t next_frame_addr = 0;

        if (++frame_count > MAX_FRAMES) {
            PyErr_SetString(PyExc_RuntimeError, "Too many stack frames (possible infinite loop)");
            return -1;
        }

        // Try chunks first, fallback to direct memory read
        if (parse_frame_from_chunks(handle, &frame, offsets, frame_addr, 
                                    &next_frame_addr, chunks, code_object_cache) < 0) {
            PyErr_Clear();
            if (parse_frame_object(handle, &frame, offsets, frame_addr, 
                                   &next_frame_addr, code_object_cache) < 0) {
                return -1;
            }
        }

        if (!frame) {
            break;
        }

        if (prev_frame_addr && frame_addr != prev_frame_addr) {
            PyErr_Format(PyExc_RuntimeError,
                        "Broken frame chain: expected frame at 0x%lx, got 0x%lx",
                        prev_frame_addr, frame_addr);
            Py_DECREF(frame);
            return -1;
        }

        if (PyList_Append(frame_info, frame) == -1) {
            Py_DECREF(frame);
            return -1;
        }
        Py_DECREF(frame);

        prev_frame_addr = next_frame_addr;
        frame_addr = next_frame_addr;
    }

    return 0;
}

static PyObject*
unwind_stack_for_thread(
    proc_handle_t *handle,
    uintptr_t *current_tstate,
    const _Py_DebugOffsets *offsets,
    _Py_hashtable_t *code_object_cache
) {
    PyObject *frame_info = NULL;
    PyObject *thread_id = NULL;
    PyObject *result = NULL;
    StackChunkList chunks = {0};

    char ts[SIZEOF_THREAD_STATE];
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        handle, *current_tstate, offsets->thread_state.size, ts);
    if (bytes_read < 0) {
        goto error;
    }

    uintptr_t frame_addr = GET_MEMBER(uintptr_t, ts, offsets->thread_state.current_frame);

    frame_info = PyList_New(0);
    if (!frame_info) {
        goto error;
    }

    if (copy_stack_chunks(handle, *current_tstate, offsets, &chunks) < 0) {
        goto error;
    }

    if (process_frame_chain(handle, offsets, frame_addr, &chunks, 
                            code_object_cache, frame_info) < 0) {
        goto error;
    }

    *current_tstate = GET_MEMBER(uintptr_t, ts, offsets->thread_state.next);

    thread_id = PyLong_FromLongLong(
        GET_MEMBER(long, ts, offsets->thread_state.native_thread_id));
    if (thread_id == NULL) {
        goto error;
    }

    result = PyTuple_New(2);
    if (result == NULL) {
        goto error;
    }

    PyTuple_SET_ITEM(result, 0, thread_id);  // Steals reference
    PyTuple_SET_ITEM(result, 1, frame_info); // Steals reference

    cleanup_stack_chunks(&chunks);
    return result;

error:
    Py_XDECREF(frame_info);
    Py_XDECREF(thread_id);
    Py_XDECREF(result);
    cleanup_stack_chunks(&chunks);
    return NULL;
}


/* ============================================================================
 * REMOTEUNWINDER CLASS IMPLEMENTATION
 * ============================================================================ */

/*[clinic input]
class _remote_debugging.RemoteUnwinder "RemoteUnwinderObject *" "&RemoteUnwinder_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=55f164d8803318be]*/

/*[clinic input]
_remote_debugging.RemoteUnwinder.__init__
    pid: int
    *
    all_threads: bool = False

Something
[clinic start generated code]*/

static int
_remote_debugging_RemoteUnwinder___init___impl(RemoteUnwinderObject *self,
                                               int pid, int all_threads)
/*[clinic end generated code: output=b8027cb247092081 input=1076d886433b1988]*/
{
    if (_Py_RemoteDebug_InitProcHandle(&self->handle, pid) < 0) {
        return -1;
    }

    self->runtime_start_address = _Py_RemoteDebug_GetPyRuntimeAddress(&self->handle);
    if (self->runtime_start_address == 0) {
        return -1;
    }

    if (_Py_RemoteDebug_ReadDebugOffsets(&self->handle,
                                         &self->runtime_start_address,
                                         &self->debug_offsets) < 0)
    {
        return -1;
    }

    // Try to read async debug offsets, but don't fail if they're not available
    self->async_debug_offsets_available = 1;
    if (read_async_debug(&self->handle, &self->async_debug_offsets) < 0) {
        PyErr_Clear();
        memset(&self->async_debug_offsets, 0, sizeof(self->async_debug_offsets));
        self->async_debug_offsets_available = 0;
    }

    if (populate_initial_state_data(all_threads, &self->handle, self->runtime_start_address,
                    &self->debug_offsets, &self->interpreter_addr ,&self->tstate_addr) < 0)
    {
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
        return -1;
    }
    return 0;
}

/*[clinic input]
@critical_section
_remote_debugging.RemoteUnwinder.get_stack_trace
Blah blah blah
[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=666192b90c69d567 input=aa504416483c9467]*/
{
    PyObject* result = NULL;
    // Read interpreter state into opaque buffer
    char interp_state_buffer[INTERP_STATE_BUFFER_SIZE];
    if (_Py_RemoteDebug_PagedReadRemoteMemory(
            &self->handle,
            self->interpreter_addr,
            INTERP_STATE_BUFFER_SIZE,
            interp_state_buffer) < 0) {
        goto exit;
    }

    // Get code object generation from buffer
    uint64_t code_object_generation = GET_MEMBER(uint64_t, interp_state_buffer,
            self->debug_offsets.interpreter_state.code_object_generation);

    if (code_object_generation != self->code_object_generation) {
        self->code_object_generation = code_object_generation;
        _Py_hashtable_clear(self->code_object_cache);
    }

    uintptr_t current_tstate;
    if (self->tstate_addr == 0) {
        // Get threads head from buffer
        current_tstate = GET_MEMBER(uintptr_t, interp_state_buffer,
                self->debug_offsets.interpreter_state.threads_head);
    } else {
        current_tstate = self->tstate_addr;
    }

    result = PyList_New(0);
    if (!result) {
        goto exit;
    }

    while (current_tstate != 0) {
        PyObject* frame_info = unwind_stack_for_thread(&self->handle,
                                                       &current_tstate, &self->debug_offsets,
                                                       self->code_object_cache);
        if (!frame_info) {
            Py_CLEAR(result);
            goto exit;
        }

        if (PyList_Append(result, frame_info) == -1) {
            Py_DECREF(frame_info);
            Py_CLEAR(result);
            goto exit;
        }
        Py_DECREF(frame_info);

        // We are targeting a single tstate, break here
        if (self->tstate_addr) {
            break;
        }
    }

exit:
   _Py_RemoteDebug_ClearCache(&self->handle);
    return result;
}

/*[clinic input]
@critical_section
_remote_debugging.RemoteUnwinder.get_all_awaited_by
Get all tasks and their awaited_by from the remote process
[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=6a49cd345e8aec53 input=40a62dc4725b295e]*/
{
    if (!self->async_debug_offsets_available) {
        PyErr_SetString(PyExc_RuntimeError, "AsyncioDebug section not available");
        return NULL;
    }

    PyObject *result = PyList_New(0);
    if (result == NULL) {
        goto result_err;
    }

    uintptr_t thread_state_addr;
    unsigned long tid = 0;
    if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                &self->handle,
                self->interpreter_addr
                + self->debug_offsets.interpreter_state.threads_main,
                sizeof(void*),
                &thread_state_addr))
    {
        goto result_err;
    }

    uintptr_t head_addr;
    while (thread_state_addr != 0) {
        if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                    &self->handle,
                    thread_state_addr
                    + self->debug_offsets.thread_state.native_thread_id,
                    sizeof(tid),
                    &tid))
        {
            goto result_err;
        }

        head_addr = thread_state_addr
            + self->async_debug_offsets.asyncio_thread_state.asyncio_tasks_head;

        if (append_awaited_by(&self->handle, tid, head_addr, &self->debug_offsets,
                              &self->async_debug_offsets, self->code_object_cache, result))
        {
            goto result_err;
        }

        if (0 > _Py_RemoteDebug_PagedReadRemoteMemory(
                    &self->handle,
                    thread_state_addr + self->debug_offsets.thread_state.next,
                    sizeof(void*),
                    &thread_state_addr))
        {
            goto result_err;
        }
    }

    head_addr = self->interpreter_addr
        + self->async_debug_offsets.asyncio_interpreter_state.asyncio_tasks_head;

    // On top of a per-thread task lists used by default by asyncio to avoid
    // contention, there is also a fallback per-interpreter list of tasks;
    // any tasks still pending when a thread is destroyed will be moved to the
    // per-interpreter task list.  It's unlikely we'll find anything here, but
    // interesting for debugging.
    if (append_awaited_by(&self->handle, 0, head_addr, &self->debug_offsets,
                        &self->async_debug_offsets, self->code_object_cache, result))
    {
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
@critical_section
_remote_debugging.RemoteUnwinder.get_async_stack_trace
Get the asyncio stack from the remote process
[clinic start generated code]*/

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace_impl(RemoteUnwinderObject *self)
/*[clinic end generated code: output=6433d52b55e87bbe input=a94e61c351cc4eed]*/
{
    if (!self->async_debug_offsets_available) {
        PyErr_SetString(PyExc_RuntimeError, "AsyncioDebug section not available");
        return NULL;
    }

    PyObject *result = NULL;
    PyObject *calls = NULL;
    
    if (setup_async_result_structure(&result, &calls) < 0) {
        goto cleanup;
    }

    uintptr_t running_task_addr, running_coro_addr, running_task_code_obj;
    if (find_running_task_and_coro(self, &running_task_addr, 
                                   &running_coro_addr, &running_task_code_obj) < 0) {
        goto cleanup;
    }

    if (parse_async_frame_chain(self, calls, running_task_code_obj) < 0) {
        goto cleanup;
    }

    if (add_task_info_to_result(self, result, running_task_addr) < 0) {
        goto cleanup;
    }

    _Py_RemoteDebug_ClearCache(&self->handle);
    return result;

cleanup:
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
RemoteUnwinder_dealloc(RemoteUnwinderObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    if (self->code_object_cache) {
        _Py_hashtable_destroy(self->code_object_cache);
    }
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
    .flags = Py_TPFLAGS_DEFAULT,
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
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif
    int rc = PyModule_AddIntConstant(m, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
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
    return 0;
}

static int
remote_debugging_clear(PyObject *mod)
{
    RemoteDebuggingState *state = RemoteDebugging_GetState(mod);
    Py_CLEAR(state->RemoteDebugging_Type);
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
