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

#include <assert.h>
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
#include <internal/pycore_interpframe.h>    // FRAME_OWNED_BY_INTERPRETER
#include <internal/pycore_llist.h>          // struct llist_node
#include <internal/pycore_long.h>           // _PyLong_GetZero
#include <internal/pycore_stackref.h>       // Py_TAG_BITS
#include "../Python/remote_debug.h"

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

// Returns thread status using proc_pidinfo, caches thread_id_offset on first use (macOS only)
#if defined(__APPLE__) && TARGET_OS_OSX
#include <libproc.h>
#include <sys/types.h>
#define MAX_NATIVE_THREADS 4096
#endif

#ifdef MS_WINDOWS
#include <windows.h>
#include <winternl.h>
// ntstatus.h conflicts with windows.h so we have to define the NTSTATUS values we need
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
typedef enum _WIN32_THREADSTATE {
    WIN32_THREADSTATE_INITIALIZED = 0,   // Recognized by the kernel
    WIN32_THREADSTATE_READY       = 1,   // Prepared to run on the next available processor
    WIN32_THREADSTATE_RUNNING     = 2,   // Currently executing
    WIN32_THREADSTATE_STANDBY     = 3,   // About to run, only one thread may be in this state at a time
    WIN32_THREADSTATE_TERMINATED  = 4,   // Finished executing
    WIN32_THREADSTATE_WAITING     = 5,   // Not ready for the processor, when ready, it will be rescheduled
    WIN32_THREADSTATE_TRANSITION  = 6,   // Waiting for resources other than the processor
    WIN32_THREADSTATE_UNKNOWN     = 7    // Thread state is unknown
} WIN32_THREADSTATE;
#endif

/* ============================================================================
 * TYPE DEFINITIONS AND STRUCTURES
 * ============================================================================ */

#define GET_MEMBER(type, obj, offset) (*(type*)((char*)(obj) + (offset)))
#define CLEAR_PTR_TAG(ptr) (((uintptr_t)(ptr) & ~Py_TAG_BITS))
#define GET_MEMBER_NO_TAG(type, obj, offset) (type)(CLEAR_PTR_TAG(*(type*)((char*)(obj) + (offset))))

/* Size macros for opaque buffers */
#define SIZEOF_BYTES_OBJ sizeof(PyBytesObject)
#define SIZEOF_CODE_OBJ sizeof(PyCodeObject)
#define SIZEOF_GEN_OBJ sizeof(PyGenObject)
#define SIZEOF_INTERP_FRAME sizeof(_PyInterpreterFrame)
#define SIZEOF_LLIST_NODE sizeof(struct llist_node)
#define SIZEOF_PAGE_CACHE_ENTRY sizeof(page_cache_entry_t)
#define SIZEOF_PYOBJECT sizeof(PyObject)
#define SIZEOF_SET_OBJ sizeof(PySetObject)
#define SIZEOF_TASK_OBJ 4096
#define SIZEOF_THREAD_STATE sizeof(PyThreadState)
#define SIZEOF_TYPE_OBJ sizeof(PyTypeObject)
#define SIZEOF_UNICODE_OBJ sizeof(PyUnicodeObject)
#define SIZEOF_LONG_OBJ sizeof(PyLongObject)
#define SIZEOF_GC_RUNTIME_STATE sizeof(struct _gc_runtime_state)
#define SIZEOF_INTERPRETER_STATE sizeof(PyInterpreterState)

// Calculate the minimum buffer size needed to read interpreter state fields
// We need to read code_object_generation and potentially tlbc_generation
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifdef Py_GIL_DISABLED
#define INTERP_STATE_MIN_SIZE MAX(MAX(MAX(MAX(offsetof(PyInterpreterState, _code_object_generation) + sizeof(uint64_t), \
                                              offsetof(PyInterpreterState, tlbc_indices.tlbc_generation) + sizeof(uint32_t)), \
                                          offsetof(PyInterpreterState, threads.head) + sizeof(void*)), \
                                      offsetof(PyInterpreterState, _gil.last_holder) + sizeof(PyThreadState*)), \
                                  offsetof(PyInterpreterState, gc.frame) + sizeof(_PyInterpreterFrame *))
#else
#define INTERP_STATE_MIN_SIZE MAX(MAX(MAX(offsetof(PyInterpreterState, _code_object_generation) + sizeof(uint64_t), \
                                          offsetof(PyInterpreterState, threads.head) + sizeof(void*)), \
                                      offsetof(PyInterpreterState, _gil.last_holder) + sizeof(PyThreadState*)), \
                                  offsetof(PyInterpreterState, gc.frame) + sizeof(_PyInterpreterFrame *))
#endif
#define INTERP_STATE_BUFFER_SIZE MAX(INTERP_STATE_MIN_SIZE, 256)

#define MAX_TLBC_SIZE 2048

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

/* ============================================================================
 * STRUCTSEQ TYPE DEFINITIONS
 * ============================================================================ */

// TaskInfo structseq type - replaces 4-tuple (task_id, task_name, coroutine_stack, awaited_by)
static PyStructSequence_Field TaskInfo_fields[] = {
    {"task_id", "Task ID (memory address)"},
    {"task_name", "Task name"},
    {"coroutine_stack", "Coroutine call stack"},
    {"awaited_by", "Tasks awaiting this task"},
    {NULL}
};

static PyStructSequence_Desc TaskInfo_desc = {
    "_remote_debugging.TaskInfo",
    "Information about an asyncio task",
    TaskInfo_fields,
    4
};

// FrameInfo structseq type - replaces 3-tuple (filename, lineno, funcname)
static PyStructSequence_Field FrameInfo_fields[] = {
    {"filename", "Source code filename"},
    {"lineno", "Line number"},
    {"funcname", "Function name"},
    {NULL}
};

static PyStructSequence_Desc FrameInfo_desc = {
    "_remote_debugging.FrameInfo",
    "Information about a frame",
    FrameInfo_fields,
    3
};

// CoroInfo structseq type - replaces 2-tuple (call_stack, task_name)
static PyStructSequence_Field CoroInfo_fields[] = {
    {"call_stack", "Coroutine call stack"},
    {"task_name", "Task name"},
    {NULL}
};

static PyStructSequence_Desc CoroInfo_desc = {
    "_remote_debugging.CoroInfo",
    "Information about a coroutine",
    CoroInfo_fields,
    2
};

// ThreadInfo structseq type - replaces 2-tuple (thread_id, frame_info)
static PyStructSequence_Field ThreadInfo_fields[] = {
    {"thread_id", "Thread ID"},
    {"status", "Thread status (flags: HAS_GIL, ON_CPU, UNKNOWN or legacy enum)"},
    {"frame_info", "Frame information"},
    {NULL}
};

static PyStructSequence_Desc ThreadInfo_desc = {
    "_remote_debugging.ThreadInfo",
    "Information about a thread",
    ThreadInfo_fields,
    3
};

// InterpreterInfo structseq type - replaces 2-tuple (interpreter_id, thread_list)
static PyStructSequence_Field InterpreterInfo_fields[] = {
    {"interpreter_id", "Interpreter ID"},
    {"threads", "List of threads in this interpreter"},
    {NULL}
};

static PyStructSequence_Desc InterpreterInfo_desc = {
    "_remote_debugging.InterpreterInfo",
    "Information about an interpreter",
    InterpreterInfo_fields,
    2
};

// AwaitedInfo structseq type - replaces 2-tuple (tid, awaited_by_list)
static PyStructSequence_Field AwaitedInfo_fields[] = {
    {"thread_id", "Thread ID"},
    {"awaited_by", "List of tasks awaited by this thread"},
    {NULL}
};

static PyStructSequence_Desc AwaitedInfo_desc = {
    "_remote_debugging.AwaitedInfo",
    "Information about what a thread is awaiting",
    AwaitedInfo_fields,
    2
};

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
    PyTypeObject *TaskInfo_Type;
    PyTypeObject *FrameInfo_Type;
    PyTypeObject *CoroInfo_Type;
    PyTypeObject *ThreadInfo_Type;
    PyTypeObject *InterpreterInfo_Type;
    PyTypeObject *AwaitedInfo_Type;
} RemoteDebuggingState;

enum _ThreadState {
    THREAD_STATE_RUNNING,
    THREAD_STATE_IDLE,
    THREAD_STATE_GIL_WAIT,
    THREAD_STATE_UNKNOWN
};

enum _ProfilingMode {
    PROFILING_MODE_WALL = 0,
    PROFILING_MODE_CPU = 1,
    PROFILING_MODE_GIL = 2,
    PROFILING_MODE_ALL = 3  // Combines GIL + CPU checks
};

// Thread status flags (can be combined)
#define THREAD_STATUS_HAS_GIL        (1 << 0)  // Thread has the GIL
#define THREAD_STATUS_ON_CPU         (1 << 1)  // Thread is running on CPU
#define THREAD_STATUS_UNKNOWN        (1 << 2)  // Status could not be determined
#define THREAD_STATUS_GIL_REQUESTED  (1 << 3)  // Thread is waiting for the GIL

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
    int debug;
    int only_active_thread;
    int mode;  // Use enum _ProfilingMode values
    int skip_non_matching_threads;  // New option to skip threads that don't match mode
    int native;
    int gc;
    RemoteDebuggingState *cached_state;  // Cached module state
#ifdef Py_GIL_DISABLED
    // TLBC cache invalidation tracking
    uint32_t tlbc_generation;  // Track TLBC index pool changes
    _Py_hashtable_t *tlbc_cache;  // Cache of TLBC arrays by code object address
#endif
#ifdef __APPLE__
    uint64_t thread_id_offset;
#endif
#ifdef MS_WINDOWS
    PVOID win_process_buffer;
    ULONG win_process_buffer_size;
#endif
} RemoteUnwinderObject;

#define RemoteUnwinder_CAST(op) ((RemoteUnwinderObject *)(op))

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

static inline int
is_frame_valid(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    uintptr_t code_object_addr
);

typedef int (*thread_processor_func)(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    unsigned long tid,
    void *context
);

typedef int (*set_entry_processor_func)(
    RemoteUnwinderObject *unwinder,
    uintptr_t key_addr,
    void *context
);


static int
parse_task(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    PyObject *render_to
);

static int
parse_coro_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t coro_address,
    PyObject *render_to
);

/* Forward declarations for task parsing functions */
static int parse_frame_object(
    RemoteUnwinderObject *unwinder,
    PyObject** result,
    uintptr_t address,
    uintptr_t* address_of_code_object,
    uintptr_t* previous_frame
);

static int
parse_async_frame_chain(
    RemoteUnwinderObject *unwinder,
    PyObject *calls,
    uintptr_t address_of_thread,
    uintptr_t running_task_code_obj
);

static int read_py_ptr(RemoteUnwinderObject *unwinder, uintptr_t address, uintptr_t *ptr_addr);
static int read_Py_ssize_t(RemoteUnwinderObject *unwinder, uintptr_t address, Py_ssize_t *size);

static int process_task_and_waiters(RemoteUnwinderObject *unwinder, uintptr_t task_addr, PyObject *result);
static int process_task_awaited_by(RemoteUnwinderObject *unwinder, uintptr_t task_address, set_entry_processor_func processor, void *context);
static int find_running_task_in_thread(RemoteUnwinderObject *unwinder, uintptr_t thread_state_addr, uintptr_t *running_task_addr);
static int get_task_code_object(RemoteUnwinderObject *unwinder, uintptr_t task_addr, uintptr_t *code_obj_addr);
static int append_awaited_by(RemoteUnwinderObject *unwinder, unsigned long tid, uintptr_t head_addr, PyObject *result);

/* ============================================================================
 * UTILITY FUNCTIONS AND HELPERS
 * ============================================================================ */

#define set_exception_cause(unwinder, exc_type, message) \
    if (unwinder->debug) { \
        _set_debug_exception_cause(exc_type, message); \
    }

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

static inline RemoteDebuggingState *
RemoteDebugging_GetStateFromType(PyTypeObject *type)
{
    PyObject *module = PyType_GetModule(type);
    assert(module != NULL);
    return RemoteDebugging_GetState(module);
}

static inline RemoteDebuggingState *
RemoteDebugging_GetStateFromObject(PyObject *obj)
{
    RemoteUnwinderObject *unwinder = (RemoteUnwinderObject *)obj;
    if (unwinder->cached_state == NULL) {
        unwinder->cached_state = RemoteDebugging_GetStateFromType(Py_TYPE(obj));
    }
    return unwinder->cached_state;
}

static inline int
RemoteDebugging_InitState(RemoteDebuggingState *st)
{
    return 0;
}

static int
is_prerelease_version(uint64_t version)
{
    return (version & 0xF0) != 0xF0;
}

static inline int
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

// Generic function to iterate through all threads
static int
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

// Generic function to iterate through set entries
static int
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
    Py_ssize_t set_len = GET_MEMBER(Py_ssize_t, set_object, unwinder->debug_offsets.set_object.mask) + 1;
    uintptr_t table_ptr = GET_MEMBER(uintptr_t, set_object, unwinder->debug_offsets.set_object.table);

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

/* ============================================================================
 * MEMORY READING FUNCTIONS
 * ============================================================================ */

#define DEFINE_MEMORY_READER(type_name, c_type, error_msg) \
static inline int \
read_##type_name(RemoteUnwinderObject *unwinder, uintptr_t address, c_type *result) \
{ \
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, address, sizeof(c_type), result); \
    if (res < 0) { \
        set_exception_cause(unwinder, PyExc_RuntimeError, error_msg); \
        return -1; \
    } \
    return 0; \
}

DEFINE_MEMORY_READER(ptr, uintptr_t, "Failed to read pointer from remote memory")
DEFINE_MEMORY_READER(Py_ssize_t, Py_ssize_t, "Failed to read Py_ssize_t from remote memory")
DEFINE_MEMORY_READER(char, char, "Failed to read char from remote memory")

static int
read_py_ptr(RemoteUnwinderObject *unwinder, uintptr_t address, uintptr_t *ptr_addr)
{
    if (read_ptr(unwinder, address, ptr_addr)) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read Python pointer");
        return -1;
    }
    *ptr_addr &= ~Py_TAG_BITS;
    return 0;
}

/* ============================================================================
 * PYTHON OBJECT READING FUNCTIONS
 * ============================================================================ */

static PyObject *
read_py_str(
    RemoteUnwinderObject *unwinder,
    uintptr_t address,
    Py_ssize_t max_len
) {
    PyObject *result = NULL;
    char *buf = NULL;

    // Read the entire PyUnicodeObject at once
    char unicode_obj[SIZEOF_UNICODE_OBJ];
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        address,
        SIZEOF_UNICODE_OBJ,
        unicode_obj
    );
    if (res < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read PyUnicodeObject");
        goto err;
    }

    Py_ssize_t len = GET_MEMBER(Py_ssize_t, unicode_obj, unwinder->debug_offsets.unicode_object.length);
    if (len < 0 || len > max_len) {
        PyErr_Format(PyExc_RuntimeError,
                     "Invalid string length (%zd) at 0x%lx", len, address);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Invalid string length in remote Unicode object");
        return NULL;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate buffer for string reading");
        return NULL;
    }

    size_t offset = (size_t)unwinder->debug_offsets.unicode_object.asciiobject_size;
    res = _Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, address + offset, len, buf);
    if (res < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read string data from remote memory");
        goto err;
    }
    buf[len] = '\0';

    result = PyUnicode_FromStringAndSize(buf, len);
    if (result == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create PyUnicode from remote string data");
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
    RemoteUnwinderObject *unwinder,
    uintptr_t address,
    Py_ssize_t max_len
) {
    PyObject *result = NULL;
    char *buf = NULL;

    // Read the entire PyBytesObject at once
    char bytes_obj[SIZEOF_BYTES_OBJ];
    int res = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        address,
        SIZEOF_BYTES_OBJ,
        bytes_obj
    );
    if (res < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read PyBytesObject");
        goto err;
    }

    Py_ssize_t len = GET_MEMBER(Py_ssize_t, bytes_obj, unwinder->debug_offsets.bytes_object.ob_size);
    if (len < 0 || len > max_len) {
        PyErr_Format(PyExc_RuntimeError,
                     "Invalid bytes length (%zd) at 0x%lx", len, address);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Invalid bytes length in remote bytes object");
        return NULL;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate buffer for bytes reading");
        return NULL;
    }

    size_t offset = (size_t)unwinder->debug_offsets.bytes_object.ob_sval;
    res = _Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, address + offset, len, buf);
    if (res < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read bytes data from remote memory");
        goto err;
    }
    buf[len] = '\0';

    result = PyBytes_FromStringAndSize(buf, len);
    if (result == NULL) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create PyBytes from remote bytes data");
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
read_py_long(
    RemoteUnwinderObject *unwinder,
    uintptr_t address
)
{
    unsigned int shift = PYLONG_BITS_IN_DIGIT;

    // Read the entire PyLongObject at once
    char long_obj[SIZEOF_LONG_OBJ];
    int bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
        &unwinder->handle,
        address,
        (size_t)unwinder->debug_offsets.long_object.size,
        long_obj);
    if (bytes_read < 0) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read PyLongObject");
        return -1;
    }

    uintptr_t lv_tag = GET_MEMBER(uintptr_t, long_obj, unwinder->debug_offsets.long_object.lv_tag);
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
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate digits for small PyLong");
            return -1;
        }
        memcpy(digits, long_obj + unwinder->debug_offsets.long_object.ob_digit, size * sizeof(digit));
    } else {
        // For larger integers, we need to read the digits separately
        digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
        if (!digits) {
            PyErr_NoMemory();
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate digits for large PyLong");
            return -1;
        }

        bytes_read = _Py_RemoteDebug_PagedReadRemoteMemory(
            &unwinder->handle,
            address + (uintptr_t)unwinder->debug_offsets.long_object.ob_digit,
            sizeof(digit) * size,
            digits
        );
        if (bytes_read < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read PyLong digits from remote memory");
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
#elif defined(__linux__) && HAVE_PROCESS_VM_READV
    // On Linux, search for asyncio debug in executable or DLL
    address = search_linux_map_for_section(handle, "AsyncioDebug", "python");
    if (address == 0) {
        // Error out: 'python' substring covers both executable and DLL
        PyObject *exc = PyErr_GetRaisedException();
        PyErr_SetString(PyExc_RuntimeError, "Failed to find the AsyncioDebug section in the process.");
        _PyErr_ChainExceptions1(exc);
    }
#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    address = search_map_for_section(handle, "AsyncioDebug", "libpython");
    if (address == 0) {
        PyErr_Clear();
        address = search_map_for_section(handle, "AsyncioDebug", "python");
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
    RemoteUnwinderObject *unwinder
) {
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

/* ============================================================================
 * ASYNCIO TASK PARSING FUNCTIONS
 * ============================================================================ */

static PyObject *
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

static int parse_task_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    PyObject *awaited_by
) {
    return process_task_awaited_by(unwinder, task_address, process_task_parser, awaited_by);
}

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

static int
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

static int
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

static int
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

// Thread processor for get_all_awaited_by
static int
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

// Generic function to process task awaited_by
static int
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

// Thread processor for get_async_stack_trace
static int
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

static int
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

static int
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

static int
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
 * TLBC CACHING FUNCTIONS
 * ============================================================================ */

#ifdef Py_GIL_DISABLED

typedef struct {
    void *tlbc_array;  // Local copy of the TLBC array
    Py_ssize_t tlbc_array_size;  // Size of the TLBC array
    uint32_t generation;  // Generation when this was cached
} TLBCCacheEntry;

static void
tlbc_cache_entry_destroy(void *ptr)
{
    TLBCCacheEntry *entry = (TLBCCacheEntry *)ptr;
    if (entry->tlbc_array) {
        PyMem_RawFree(entry->tlbc_array);
    }
    PyMem_RawFree(entry);
}

static TLBCCacheEntry *
get_tlbc_cache_entry(RemoteUnwinderObject *self, uintptr_t code_addr, uint32_t current_generation)
{
    void *key = (void *)code_addr;
    TLBCCacheEntry *entry = _Py_hashtable_get(self->tlbc_cache, key);

    if (entry && entry->generation != current_generation) {
        // Entry is stale, remove it by setting to NULL
        _Py_hashtable_set(self->tlbc_cache, key, NULL);
        entry = NULL;
    }

    return entry;
}

static int
cache_tlbc_array(RemoteUnwinderObject *unwinder, uintptr_t code_addr, uintptr_t tlbc_array_addr, uint32_t generation)
{
    uintptr_t tlbc_array_ptr;
    void *tlbc_array = NULL;
    TLBCCacheEntry *entry = NULL;

    // Read the TLBC array pointer
    if (read_ptr(unwinder, tlbc_array_addr, &tlbc_array_ptr) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to read TLBC array pointer");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read TLBC array pointer");
        return 0; // Read error
    }

    // Validate TLBC array pointer
    if (tlbc_array_ptr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "TLBC array pointer is NULL");
        return 0; // No TLBC array
    }

    // Read the TLBC array size
    Py_ssize_t tlbc_size;
    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, tlbc_array_ptr, sizeof(tlbc_size), &tlbc_size) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to read TLBC array size");
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read TLBC array size");
        return 0; // Read error
    }

    // Validate TLBC array size
    if (tlbc_size <= 0) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid TLBC array size");
        return 0; // Invalid size
    }

    if (tlbc_size > MAX_TLBC_SIZE) {
        PyErr_SetString(PyExc_RuntimeError, "TLBC array size exceeds maximum limit");
        return 0; // Invalid size
    }

    // Allocate and read the entire TLBC array
    size_t array_data_size = tlbc_size * sizeof(void*);
    tlbc_array = PyMem_RawMalloc(sizeof(Py_ssize_t) + array_data_size);
    if (!tlbc_array) {
        PyErr_NoMemory();
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate TLBC array");
        return 0; // Memory error
    }

    if (_Py_RemoteDebug_PagedReadRemoteMemory(&unwinder->handle, tlbc_array_ptr, sizeof(Py_ssize_t) + array_data_size, tlbc_array) != 0) {
        PyMem_RawFree(tlbc_array);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read TLBC array data");
        return 0; // Read error
    }

    // Create cache entry
    entry = PyMem_RawMalloc(sizeof(TLBCCacheEntry));
    if (!entry) {
        PyErr_NoMemory();
        PyMem_RawFree(tlbc_array);
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate TLBC cache entry");
        return 0; // Memory error
    }

    entry->tlbc_array = tlbc_array;
    entry->tlbc_array_size = tlbc_size;
    entry->generation = generation;

    // Store in cache
    void *key = (void *)code_addr;
    if (_Py_hashtable_set(unwinder->tlbc_cache, key, entry) < 0) {
        tlbc_cache_entry_destroy(entry);
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to store TLBC entry in cache");
        return 0; // Cache error
    }

    return 1; // Success
}



#endif

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
    uintptr_t addr = 0;
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

static PyObject *
make_frame_info(RemoteUnwinderObject *unwinder, PyObject *file, PyObject *line,
                PyObject *func)
{
    RemoteDebuggingState *state = RemoteDebugging_GetStateFromObject((PyObject*)unwinder);
    PyObject *info = PyStructSequence_New(state->FrameInfo_Type);
    if (info == NULL) {
        set_exception_cause(unwinder, PyExc_MemoryError, "Failed to create FrameInfo");
        return NULL;
    }
    Py_INCREF(file);
    Py_INCREF(line);
    Py_INCREF(func);
    PyStructSequence_SetItem(info, 0, file);
    PyStructSequence_SetItem(info, 1, line);
    PyStructSequence_SetItem(info, 2, func);
    return info;
}

static int
parse_code_object(RemoteUnwinderObject *unwinder,
                  PyObject **result,
                  uintptr_t address,
                  uintptr_t instruction_pointer,
                  uintptr_t *previous_frame,
                  int32_t tlbc_index)
{
    void *key = (void *)address;
    CachedCodeMetadata *meta = NULL;
    PyObject *func = NULL;
    PyObject *file = NULL;
    PyObject *linetable = NULL;

#ifdef Py_GIL_DISABLED
    // In free threading builds, code object addresses might have the low bit set
    // as a flag, so we need to mask it off to get the real address
    uintptr_t real_address = address & (~1);
#else
    uintptr_t real_address = address;
#endif

    if (unwinder && unwinder->code_object_cache != NULL) {
        meta = _Py_hashtable_get(unwinder->code_object_cache, key);
    }

    if (meta == NULL) {
        char code_object[SIZEOF_CODE_OBJ];
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle, real_address, SIZEOF_CODE_OBJ, code_object) < 0)
        {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read code object");
            goto error;
        }

        func = read_py_str(unwinder,
            GET_MEMBER(uintptr_t, code_object, unwinder->debug_offsets.code_object.qualname), 1024);
        if (!func) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read function name from code object");
            goto error;
        }

        file = read_py_str(unwinder,
            GET_MEMBER(uintptr_t, code_object, unwinder->debug_offsets.code_object.filename), 1024);
        if (!file) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read filename from code object");
            goto error;
        }

        linetable = read_py_bytes(unwinder,
            GET_MEMBER(uintptr_t, code_object, unwinder->debug_offsets.code_object.linetable), 4096);
        if (!linetable) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read linetable from code object");
            goto error;
        }

        meta = PyMem_RawMalloc(sizeof(CachedCodeMetadata));
        if (!meta) {
            PyErr_NoMemory();
            set_exception_cause(unwinder, PyExc_MemoryError, "Failed to allocate cached code metadata");
            goto error;
        }

        meta->func_name = func;
        meta->file_name = file;
        meta->linetable = linetable;
        meta->first_lineno = GET_MEMBER(int, code_object, unwinder->debug_offsets.code_object.firstlineno);
        meta->addr_code_adaptive = real_address + (uintptr_t)unwinder->debug_offsets.code_object.co_code_adaptive;

        if (unwinder && unwinder->code_object_cache && _Py_hashtable_set(unwinder->code_object_cache, key, meta) < 0) {
            cached_code_metadata_destroy(meta);
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to cache code metadata");
            goto error;
        }

        // Ownership transferred to meta
        func = NULL;
        file = NULL;
        linetable = NULL;
    }

    uintptr_t ip = instruction_pointer;
    ptrdiff_t addrq;

#ifdef Py_GIL_DISABLED
    // Handle thread-local bytecode (TLBC) in free threading builds
    if (tlbc_index == 0 || unwinder->debug_offsets.code_object.co_tlbc == 0 || unwinder == NULL) {
        // No TLBC or no unwinder - use main bytecode directly
        addrq = (uint16_t *)ip - (uint16_t *)meta->addr_code_adaptive;
        goto done_tlbc;
    }

    // Try to get TLBC data from cache (we'll get generation from the caller)
    TLBCCacheEntry *tlbc_entry = get_tlbc_cache_entry(unwinder, real_address, unwinder->tlbc_generation);

    if (!tlbc_entry) {
        // Cache miss - try to read and cache TLBC array
        if (!cache_tlbc_array(unwinder, real_address, real_address + unwinder->debug_offsets.code_object.co_tlbc, unwinder->tlbc_generation)) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to cache TLBC array");
            goto error;
        }
        tlbc_entry = get_tlbc_cache_entry(unwinder, real_address, unwinder->tlbc_generation);
    }

    if (tlbc_entry && tlbc_index < tlbc_entry->tlbc_array_size) {
        // Use cached TLBC data
        uintptr_t *entries = (uintptr_t *)((char *)tlbc_entry->tlbc_array + sizeof(Py_ssize_t));
        uintptr_t tlbc_bytecode_addr = entries[tlbc_index];

        if (tlbc_bytecode_addr != 0) {
            // Calculate offset from TLBC bytecode
            addrq = (uint16_t *)ip - (uint16_t *)tlbc_bytecode_addr;
            goto done_tlbc;
        }
    }

    // Fall back to main bytecode
    addrq = (uint16_t *)ip - (uint16_t *)meta->addr_code_adaptive;

done_tlbc:
#else
    // Non-free-threaded build, always use the main bytecode
    (void)tlbc_index; // Suppress unused parameter warning
    (void)unwinder;   // Suppress unused parameter warning
    addrq = (uint16_t *)ip - (uint16_t *)meta->addr_code_adaptive;
#endif
    ;  // Empty statement to avoid C23 extension warning
    LocationInfo info = {0};
    bool ok = parse_linetable(addrq, PyBytes_AS_STRING(meta->linetable),
                              meta->first_lineno, &info);
    if (!ok) {
        info.lineno = -1;
    }

    PyObject *lineno = PyLong_FromLong(info.lineno);
    if (!lineno) {
        set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to create line number object");
        goto error;
    }

    PyObject *tuple = make_frame_info(unwinder, meta->file_name, lineno, meta->func_name);
    Py_DECREF(lineno);
    if (!tuple) {
        goto error;
    }

    *result = tuple;
    return 0;

error:
    Py_XDECREF(func);
    Py_XDECREF(file);
    Py_XDECREF(linetable);
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

static int
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
 * INTERPRETER STATE AND THREAD DISCOVERY FUNCTIONS
 * ============================================================================ */

static int
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


static int
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
 * FRAME PARSING FUNCTIONS
 * ============================================================================ */

static inline int
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

static int
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

static int
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

        if (GET_MEMBER(uintptr_t, task_node, unwinder->debug_offsets.llist_node.next) == 0) {
            PyErr_SetString(PyExc_RuntimeError,
                           "Invalid linked list structure reading remote memory");
            set_exception_cause(unwinder, PyExc_RuntimeError, "NULL pointer in task linked list");
            return -1;
        }

        uintptr_t task_addr = (uintptr_t)GET_MEMBER(uintptr_t, task_node, unwinder->debug_offsets.llist_node.next)
            - (uintptr_t)unwinder->async_debug_offsets.asyncio_task_object.task_node;

        if (process_single_task_node(unwinder, task_addr, NULL, result) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to process task node in awaited_by");
            return -1;
        }

        // Read next node
        if (_Py_RemoteDebug_PagedReadRemoteMemory(
                &unwinder->handle,
                (uintptr_t)GET_MEMBER(uintptr_t, task_node, unwinder->debug_offsets.llist_node.next),
                sizeof(task_node),
                task_node) < 0) {
            set_exception_cause(unwinder, PyExc_RuntimeError, "Failed to read next task node in awaited_by");
            return -1;
        }
    }

    return 0;
}

static int
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
 * STACK UNWINDING FUNCTIONS
 * ============================================================================ */

static int
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

static int
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

static PyObject*
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
    int cpu_status = get_thread_status(unwinder, tid, pthread_id);
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


/* ============================================================================
 * REMOTEUNWINDER CLASS IMPLEMENTATION
 * ============================================================================ */

/*[clinic input]
class _remote_debugging.RemoteUnwinder "RemoteUnwinderObject *" "&RemoteUnwinder_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=55f164d8803318be]*/

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
