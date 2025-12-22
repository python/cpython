/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder___init____doc__,
"RemoteUnwinder(pid, *, all_threads=False, only_active_thread=False,\n"
"               mode=0, debug=False, skip_non_matching_threads=True,\n"
"               native=False, gc=False, opcodes=False,\n"
"               cache_frames=False, stats=False)\n"
"--\n"
"\n"
"Initialize a new RemoteUnwinder object for debugging a remote Python process.\n"
"\n"
"Args:\n"
"    pid: Process ID of the target Python process to debug\n"
"    all_threads: If True, initialize state for all threads in the process.\n"
"                If False, only initialize for the main thread.\n"
"    only_active_thread: If True, only sample the thread holding the GIL.\n"
"    mode: Profiling mode: 0=WALL (wall-time), 1=CPU (cpu-time), 2=GIL (gil-time).\n"
"                       Cannot be used together with all_threads=True.\n"
"    debug: If True, chain exceptions to explain the sequence of events that\n"
"           lead to the exception.\n"
"    skip_non_matching_threads: If True, skip threads that don\'t match the selected mode.\n"
"                              If False, include all threads regardless of mode.\n"
"    native: If True, include artificial \"<native>\" frames to denote calls to\n"
"            non-Python code.\n"
"    gc: If True, include artificial \"<GC>\" frames to denote active garbage\n"
"        collection.\n"
"    opcodes: If True, gather bytecode opcode information for instruction-level\n"
"             profiling.\n"
"    cache_frames: If True, enable frame caching optimization to avoid re-reading\n"
"                 unchanged parent frames between samples.\n"
"    stats: If True, collect statistics about cache hits, memory reads, etc.\n"
"           Use get_stats() to retrieve the collected statistics.\n"
"\n"
"The RemoteUnwinder provides functionality to inspect and debug a running Python\n"
"process, including examining thread states, stack frames and other runtime data.\n"
"\n"
"Raises:\n"
"    PermissionError: If access to the target process is denied\n"
"    OSError: If unable to attach to the target process or access its memory\n"
"    RuntimeError: If unable to read debug information from the target process\n"
"    ValueError: If both all_threads and only_active_thread are True");

static int
_remote_debugging_RemoteUnwinder___init___impl(RemoteUnwinderObject *self,
                                               int pid, int all_threads,
                                               int only_active_thread,
                                               int mode, int debug,
                                               int skip_non_matching_threads,
                                               int native, int gc,
                                               int opcodes, int cache_frames,
                                               int stats);

static int
_remote_debugging_RemoteUnwinder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 11
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pid), &_Py_ID(all_threads), &_Py_ID(only_active_thread), &_Py_ID(mode), &_Py_ID(debug), &_Py_ID(skip_non_matching_threads), &_Py_ID(native), &_Py_ID(gc), &_Py_ID(opcodes), &_Py_ID(cache_frames), &_Py_ID(stats), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "all_threads", "only_active_thread", "mode", "debug", "skip_non_matching_threads", "native", "gc", "opcodes", "cache_frames", "stats", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "RemoteUnwinder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[11];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    int pid;
    int all_threads = 0;
    int only_active_thread = 0;
    int mode = 0;
    int debug = 0;
    int skip_non_matching_threads = 1;
    int native = 0;
    int gc = 0;
    int opcodes = 0;
    int cache_frames = 0;
    int stats = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    pid = PyLong_AsInt(fastargs[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (fastargs[1]) {
        all_threads = PyObject_IsTrue(fastargs[1]);
        if (all_threads < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[2]) {
        only_active_thread = PyObject_IsTrue(fastargs[2]);
        if (only_active_thread < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[3]) {
        mode = PyLong_AsInt(fastargs[3]);
        if (mode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[4]) {
        debug = PyObject_IsTrue(fastargs[4]);
        if (debug < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[5]) {
        skip_non_matching_threads = PyObject_IsTrue(fastargs[5]);
        if (skip_non_matching_threads < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[6]) {
        native = PyObject_IsTrue(fastargs[6]);
        if (native < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[7]) {
        gc = PyObject_IsTrue(fastargs[7]);
        if (gc < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[8]) {
        opcodes = PyObject_IsTrue(fastargs[8]);
        if (opcodes < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    if (fastargs[9]) {
        cache_frames = PyObject_IsTrue(fastargs[9]);
        if (cache_frames < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    stats = PyObject_IsTrue(fastargs[10]);
    if (stats < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _remote_debugging_RemoteUnwinder___init___impl((RemoteUnwinderObject *)self, pid, all_threads, only_active_thread, mode, debug, skip_non_matching_threads, native, gc, opcodes, cache_frames, stats);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stack_trace__doc__,
"get_stack_trace($self, /)\n"
"--\n"
"\n"
"Returns stack traces for all interpreters and threads in process.\n"
"\n"
"Each element in the returned list is a tuple of (interpreter_id, thread_list), where:\n"
"- interpreter_id is the interpreter identifier\n"
"- thread_list is a list of tuples (thread_id, frame_list) for threads in that interpreter\n"
"  - thread_id is the OS thread identifier\n"
"  - frame_list is a list of tuples (function_name, filename, line_number) representing\n"
"    the Python stack frames for that thread, ordered from most recent to oldest\n"
"\n"
"The threads returned depend on the initialization parameters:\n"
"- If only_active_thread was True: returns only the thread holding the GIL across all interpreters\n"
"- If all_threads was True: returns all threads across all interpreters\n"
"- Otherwise: returns only the main thread of each interpreter\n"
"\n"
"Example:\n"
"    [\n"
"        (0, [  # Main interpreter\n"
"            (1234, [\n"
"                (\'process_data\', \'worker.py\', 127),\n"
"                (\'run_worker\', \'worker.py\', 45),\n"
"                (\'main\', \'app.py\', 23)\n"
"            ]),\n"
"            (1235, [\n"
"                (\'handle_request\', \'server.py\', 89),\n"
"                (\'serve_forever\', \'server.py\', 52)\n"
"            ])\n"
"        ]),\n"
"        (1, [  # Sub-interpreter\n"
"            (1236, [\n"
"                (\'sub_worker\', \'sub.py\', 15)\n"
"            ])\n"
"        ])\n"
"    ]\n"
"\n"
"Raises:\n"
"    RuntimeError: If there is an error copying memory from the target process\n"
"    OSError: If there is an error accessing the target process\n"
"    PermissionError: If access to the target process is denied\n"
"    UnicodeDecodeError: If there is an error decoding strings from the target process");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STACK_TRACE_METHODDEF    \
    {"get_stack_trace", (PyCFunction)_remote_debugging_RemoteUnwinder_get_stack_trace, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_stack_trace__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_stack_trace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_stack_trace_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__,
"get_all_awaited_by($self, /)\n"
"--\n"
"\n"
"Get all tasks and their awaited_by relationships from the remote process.\n"
"\n"
"This provides a tree structure showing which tasks are waiting for other tasks.\n"
"\n"
"For each task, returns:\n"
"1. The call stack frames leading to where the task is currently executing\n"
"2. The name of the task\n"
"3. A list of tasks that this task is waiting for, with their own frames/names/etc\n"
"\n"
"Returns a list of [frames, task_name, subtasks] where:\n"
"- frames: List of (func_name, filename, lineno) showing the call stack\n"
"- task_name: String identifier for the task\n"
"- subtasks: List of tasks being awaited by this task, in same format\n"
"\n"
"Raises:\n"
"    RuntimeError: If AsyncioDebug section is not available in the remote process\n"
"    MemoryError: If memory allocation fails\n"
"    OSError: If reading from the remote process fails\n"
"\n"
"Example output:\n"
"[\n"
"    [\n"
"        [(\"c5\", \"script.py\", 10), (\"c4\", \"script.py\", 14)],\n"
"        \"c2_root\",\n"
"        [\n"
"            [\n"
"                [(\"c1\", \"script.py\", 23)],\n"
"                \"sub_main_2\",\n"
"                [...]\n"
"            ],\n"
"            [...]\n"
"        ]\n"
"    ]\n"
"]");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ALL_AWAITED_BY_METHODDEF    \
    {"get_all_awaited_by", (PyCFunction)_remote_debugging_RemoteUnwinder_get_all_awaited_by, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_all_awaited_by__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_all_awaited_by(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_all_awaited_by_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__,
"get_async_stack_trace($self, /)\n"
"--\n"
"\n"
"Get the currently running async tasks and their dependency graphs from the remote process.\n"
"\n"
"This returns information about running tasks and all tasks that are waiting for them,\n"
"forming a complete dependency graph for each thread\'s active task.\n"
"\n"
"For each thread with a running task, returns the running task plus all tasks that\n"
"transitively depend on it (tasks waiting for the running task, tasks waiting for\n"
"those tasks, etc.).\n"
"\n"
"Returns a list of per-thread results, where each thread result contains:\n"
"- Thread ID\n"
"- List of task information for the running task and all its waiters\n"
"\n"
"Each task info contains:\n"
"- Task ID (memory address)\n"
"- Task name\n"
"- Call stack frames: List of (func_name, filename, lineno)\n"
"- List of tasks waiting for this task (recursive structure)\n"
"\n"
"Raises:\n"
"    RuntimeError: If AsyncioDebug section is not available in the target process\n"
"    MemoryError: If memory allocation fails\n"
"    OSError: If reading from the remote process fails\n"
"\n"
"Example output (similar structure to get_all_awaited_by but only for running tasks):\n"
"[\n"
"    (140234, [\n"
"        (4345585712, \'main_task\',\n"
"         [(\"run_server\", \"server.py\", 127), (\"main\", \"app.py\", 23)],\n"
"         [\n"
"             (4345585800, \'worker_1\', [...], [...]),\n"
"             (4345585900, \'worker_2\', [...], [...])\n"
"         ])\n"
"    ])\n"
"]");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_ASYNC_STACK_TRACE_METHODDEF    \
    {"get_async_stack_trace", (PyCFunction)_remote_debugging_RemoteUnwinder_get_async_stack_trace, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_async_stack_trace__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_async_stack_trace(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_async_stack_trace_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stats__doc__,
"get_stats($self, /)\n"
"--\n"
"\n"
"Get collected statistics about profiling performance.\n"
"\n"
"Returns a dictionary containing statistics about cache performance,\n"
"memory reads, and other profiling metrics. Only available if the\n"
"RemoteUnwinder was created with stats=True.\n"
"\n"
"Returns:\n"
"    dict: A dictionary containing:\n"
"        - total_samples: Total number of get_stack_trace calls\n"
"        - frame_cache_hits: Full cache hits (entire stack unchanged)\n"
"        - frame_cache_misses: Cache misses requiring full walk\n"
"        - frame_cache_partial_hits: Partial hits (stopped at cached frame)\n"
"        - frames_read_from_cache: Total frames retrieved from cache\n"
"        - frames_read_from_memory: Total frames read from remote memory\n"
"        - memory_reads: Total remote memory read operations\n"
"        - memory_bytes_read: Total bytes read from remote memory\n"
"        - code_object_cache_hits: Code object cache hits\n"
"        - code_object_cache_misses: Code object cache misses\n"
"        - stale_cache_invalidations: Times stale cache entries were cleared\n"
"        - frame_cache_hit_rate: Percentage of samples that hit the cache\n"
"        - code_object_cache_hit_rate: Percentage of code object lookups that hit cache\n"
"\n"
"Raises:\n"
"    RuntimeError: If stats collection was not enabled (stats=False)");

#define _REMOTE_DEBUGGING_REMOTEUNWINDER_GET_STATS_METHODDEF    \
    {"get_stats", (PyCFunction)_remote_debugging_RemoteUnwinder_get_stats, METH_NOARGS, _remote_debugging_RemoteUnwinder_get_stats__doc__},

static PyObject *
_remote_debugging_RemoteUnwinder_get_stats_impl(RemoteUnwinderObject *self);

static PyObject *
_remote_debugging_RemoteUnwinder_get_stats(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _remote_debugging_RemoteUnwinder_get_stats_impl((RemoteUnwinderObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_remote_debugging_get_child_pids__doc__,
"get_child_pids($module, /, pid, *, recursive=True)\n"
"--\n"
"\n"
"Get all child process IDs of the given process.\n"
"\n"
"  pid\n"
"    Process ID of the parent process\n"
"  recursive\n"
"    If True, return all descendants (children, grandchildren, etc.).\n"
"    If False, return only direct children.\n"
"\n"
"Returns a list of child process IDs. Returns an empty list if no children\n"
"are found.\n"
"\n"
"This function provides a snapshot of child processes at a moment in time.\n"
"Child processes may exit or new ones may be created after the list is returned.\n"
"\n"
"Raises:\n"
"    OSError: If unable to enumerate processes\n"
"    NotImplementedError: If not supported on this platform");

#define _REMOTE_DEBUGGING_GET_CHILD_PIDS_METHODDEF    \
    {"get_child_pids", _PyCFunction_CAST(_remote_debugging_get_child_pids), METH_FASTCALL|METH_KEYWORDS, _remote_debugging_get_child_pids__doc__},

static PyObject *
_remote_debugging_get_child_pids_impl(PyObject *module, int pid,
                                      int recursive);

static PyObject *
_remote_debugging_get_child_pids(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pid), &_Py_ID(recursive), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "recursive", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "get_child_pids",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int pid;
    int recursive = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    pid = PyLong_AsInt(args[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    recursive = PyObject_IsTrue(args[1]);
    if (recursive < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _remote_debugging_get_child_pids_impl(module, pid, recursive);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_is_python_process__doc__,
"is_python_process($module, /, pid)\n"
"--\n"
"\n"
"Check if a process is a Python process.");

#define _REMOTE_DEBUGGING_IS_PYTHON_PROCESS_METHODDEF    \
    {"is_python_process", _PyCFunction_CAST(_remote_debugging_is_python_process), METH_FASTCALL|METH_KEYWORDS, _remote_debugging_is_python_process__doc__},

static PyObject *
_remote_debugging_is_python_process_impl(PyObject *module, int pid);

static PyObject *
_remote_debugging_is_python_process(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pid), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "is_python_process",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int pid;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    pid = PyLong_AsInt(args[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _remote_debugging_is_python_process_impl(module, pid);

exit:
    return return_value;
}
/*[clinic end generated code: output=dc0550ad3d6a409c input=a9049054013a1b77]*/
