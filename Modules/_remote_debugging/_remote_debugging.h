/******************************************************************************
 * Python Remote Debugging Module - Shared Header
 *
 * This header provides common declarations, types, and utilities shared
 * across the remote debugging module implementation files.
 ******************************************************************************/

#ifndef Py_REMOTE_DEBUGGING_H
#define Py_REMOTE_DEBUGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#define _GNU_SOURCE

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
#include "../../Python/remote_debug.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_PROCESS_VM_READV
#    define HAVE_PROCESS_VM_READV 0
#endif

#if defined(__APPLE__) && TARGET_OS_OSX
#include <libproc.h>
#include <sys/types.h>
#define MAX_NATIVE_THREADS 4096
#endif

#ifdef MS_WINDOWS
#include <windows.h>
#include <winternl.h>
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
typedef enum _WIN32_THREADSTATE {
    WIN32_THREADSTATE_INITIALIZED = 0,
    WIN32_THREADSTATE_READY       = 1,
    WIN32_THREADSTATE_RUNNING     = 2,
    WIN32_THREADSTATE_STANDBY     = 3,
    WIN32_THREADSTATE_TERMINATED  = 4,
    WIN32_THREADSTATE_WAITING     = 5,
    WIN32_THREADSTATE_TRANSITION  = 6,
    WIN32_THREADSTATE_UNKNOWN     = 7
} WIN32_THREADSTATE;
#endif

/* ============================================================================
 * MACROS AND CONSTANTS
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

/* Thread status flags */
#define THREAD_STATUS_HAS_GIL        (1 << 0)
#define THREAD_STATUS_ON_CPU         (1 << 1)
#define THREAD_STATUS_UNKNOWN        (1 << 2)
#define THREAD_STATUS_GIL_REQUESTED  (1 << 3)

/* Exception cause macro */
#define set_exception_cause(unwinder, exc_type, message) \
    if (unwinder->debug) { \
        _set_debug_exception_cause(exc_type, message); \
    }

/* ============================================================================
 * TYPE DEFINITIONS
 * ============================================================================ */

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
    PyObject *func_name;
    PyObject *file_name;
    int first_lineno;
    PyObject *linetable;  // bytes
    uintptr_t addr_code_adaptive;
} CachedCodeMetadata;

typedef struct {
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
    PROFILING_MODE_ALL = 3
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
    int debug;
    int only_active_thread;
    int mode;
    int skip_non_matching_threads;
    int native;
    int gc;
    RemoteDebuggingState *cached_state;
#ifdef Py_GIL_DISABLED
    uint32_t tlbc_generation;
    _Py_hashtable_t *tlbc_cache;
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

typedef struct {
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

/* Function pointer types for iteration callbacks */
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

/* ============================================================================
 * STRUCTSEQ DESCRIPTORS (extern declarations)
 * ============================================================================ */

extern PyStructSequence_Desc TaskInfo_desc;
extern PyStructSequence_Desc FrameInfo_desc;
extern PyStructSequence_Desc CoroInfo_desc;
extern PyStructSequence_Desc ThreadInfo_desc;
extern PyStructSequence_Desc InterpreterInfo_desc;
extern PyStructSequence_Desc AwaitedInfo_desc;

/* ============================================================================
 * UTILITY FUNCTION DECLARATIONS
 * ============================================================================ */

/* State access functions */
extern RemoteDebuggingState *RemoteDebugging_GetState(PyObject *module);
extern RemoteDebuggingState *RemoteDebugging_GetStateFromType(PyTypeObject *type);
extern RemoteDebuggingState *RemoteDebugging_GetStateFromObject(PyObject *obj);
extern int RemoteDebugging_InitState(RemoteDebuggingState *st);

/* Cache management */
extern void cached_code_metadata_destroy(void *ptr);

/* Validation */
extern int is_prerelease_version(uint64_t version);
extern int validate_debug_offsets(struct _Py_DebugOffsets *debug_offsets);

/* ============================================================================
 * MEMORY READING FUNCTION DECLARATIONS
 * ============================================================================ */

extern int read_ptr(RemoteUnwinderObject *unwinder, uintptr_t address, uintptr_t *result);
extern int read_Py_ssize_t(RemoteUnwinderObject *unwinder, uintptr_t address, Py_ssize_t *result);
extern int read_char(RemoteUnwinderObject *unwinder, uintptr_t address, char *result);
extern int read_py_ptr(RemoteUnwinderObject *unwinder, uintptr_t address, uintptr_t *ptr_addr);

/* Python object reading */
extern PyObject *read_py_str(RemoteUnwinderObject *unwinder, uintptr_t address, Py_ssize_t max_len);
extern PyObject *read_py_bytes(RemoteUnwinderObject *unwinder, uintptr_t address, Py_ssize_t max_len);
extern long read_py_long(RemoteUnwinderObject *unwinder, uintptr_t address);

/* ============================================================================
 * CODE OBJECT FUNCTION DECLARATIONS
 * ============================================================================ */

extern int parse_code_object(
    RemoteUnwinderObject *unwinder,
    PyObject **result,
    uintptr_t address,
    uintptr_t instruction_pointer,
    uintptr_t *previous_frame,
    int32_t tlbc_index
);

extern PyObject *make_frame_info(
    RemoteUnwinderObject *unwinder,
    PyObject *file,
    PyObject *line,
    PyObject *func
);

/* Line table parsing */
extern bool parse_linetable(
    const uintptr_t addrq,
    const char* linetable,
    int firstlineno,
    LocationInfo* info
);

/* TLBC cache (only for Py_GIL_DISABLED) */
#ifdef Py_GIL_DISABLED
typedef struct {
    void *tlbc_array;
    Py_ssize_t tlbc_array_size;
    uint32_t generation;
} TLBCCacheEntry;

extern void tlbc_cache_entry_destroy(void *ptr);
extern TLBCCacheEntry *get_tlbc_cache_entry(RemoteUnwinderObject *self, uintptr_t code_addr, uint32_t current_generation);
extern int cache_tlbc_array(RemoteUnwinderObject *unwinder, uintptr_t code_addr, uintptr_t tlbc_array_addr, uint32_t generation);
#endif

/* ============================================================================
 * FRAME FUNCTION DECLARATIONS
 * ============================================================================ */

extern int is_frame_valid(
    RemoteUnwinderObject *unwinder,
    uintptr_t frame_addr,
    uintptr_t code_object_addr
);

extern int parse_frame_object(
    RemoteUnwinderObject *unwinder,
    PyObject** result,
    uintptr_t address,
    uintptr_t* address_of_code_object,
    uintptr_t* previous_frame
);

extern int parse_frame_from_chunks(
    RemoteUnwinderObject *unwinder,
    PyObject **result,
    uintptr_t address,
    uintptr_t *previous_frame,
    uintptr_t *stackpointer,
    StackChunkList *chunks
);

/* Stack chunk management */
extern void cleanup_stack_chunks(StackChunkList *chunks);
extern int copy_stack_chunks(RemoteUnwinderObject *unwinder, uintptr_t tstate_addr, StackChunkList *out_chunks);
extern void *find_frame_in_chunks(StackChunkList *chunks, uintptr_t remote_ptr);

extern int process_frame_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t initial_frame_addr,
    StackChunkList *chunks,
    PyObject *frame_info,
    uintptr_t gc_frame
);

/* ============================================================================
 * THREAD FUNCTION DECLARATIONS
 * ============================================================================ */

extern int iterate_threads(
    RemoteUnwinderObject *unwinder,
    thread_processor_func processor,
    void *context
);

extern int populate_initial_state_data(
    int all_threads,
    RemoteUnwinderObject *unwinder,
    uintptr_t runtime_start_address,
    uintptr_t *interpreter_state,
    uintptr_t *tstate
);

extern int find_running_frame(
    RemoteUnwinderObject *unwinder,
    uintptr_t address_of_thread,
    uintptr_t *frame
);

extern int get_thread_status(RemoteUnwinderObject *unwinder, uint64_t tid, uint64_t pthread_id);

extern PyObject* unwind_stack_for_thread(
    RemoteUnwinderObject *unwinder,
    uintptr_t *current_tstate,
    uintptr_t gil_holder_tstate,
    uintptr_t gc_frame
);

/* ============================================================================
 * ASYNCIO FUNCTION DECLARATIONS
 * ============================================================================ */

extern uintptr_t _Py_RemoteDebug_GetAsyncioDebugAddress(proc_handle_t* handle);
extern int read_async_debug(RemoteUnwinderObject *unwinder);

/* Task parsing */
extern PyObject *parse_task_name(RemoteUnwinderObject *unwinder, uintptr_t task_address);

extern int parse_task(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    PyObject *render_to
);

extern int parse_coro_chain(
    RemoteUnwinderObject *unwinder,
    uintptr_t coro_address,
    PyObject *render_to
);

extern int parse_async_frame_chain(
    RemoteUnwinderObject *unwinder,
    PyObject *calls,
    uintptr_t address_of_thread,
    uintptr_t running_task_code_obj
);

/* Set iteration */
extern int iterate_set_entries(
    RemoteUnwinderObject *unwinder,
    uintptr_t set_addr,
    set_entry_processor_func processor,
    void *context
);

/* Task awaited_by processing */
extern int process_task_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_address,
    set_entry_processor_func processor,
    void *context
);

extern int process_single_task_node(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_addr,
    PyObject **task_info,
    PyObject *result
);

extern int process_task_and_waiters(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_addr,
    PyObject *result
);

extern int find_running_task_in_thread(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    uintptr_t *running_task_addr
);

extern int get_task_code_object(
    RemoteUnwinderObject *unwinder,
    uintptr_t task_addr,
    uintptr_t *code_obj_addr
);

extern int append_awaited_by(
    RemoteUnwinderObject *unwinder,
    unsigned long tid,
    uintptr_t head_addr,
    PyObject *result
);

/* Thread processors for asyncio */
extern int process_thread_for_awaited_by(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    unsigned long tid,
    void *context
);

extern int process_thread_for_async_stack_trace(
    RemoteUnwinderObject *unwinder,
    uintptr_t thread_state_addr,
    unsigned long tid,
    void *context
);

#ifdef __cplusplus
}
#endif

#endif /* Py_REMOTE_DEBUGGING_H */
