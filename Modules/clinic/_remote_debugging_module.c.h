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
"               debug=False)\n"
"--\n"
"\n"
"Initialize a new RemoteUnwinder object for debugging a remote Python process.\n"
"\n"
"Args:\n"
"    pid: Process ID of the target Python process to debug\n"
"    all_threads: If True, initialize state for all threads in the process.\n"
"                If False, only initialize for the main thread.\n"
"    only_active_thread: If True, only sample the thread holding the GIL.\n"
"                       Cannot be used together with all_threads=True.\n"
"    debug: If True, chain exceptions to explain the sequence of events that\n"
"           lead to the exception.\n"
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
                                               int debug);

static int
_remote_debugging_RemoteUnwinder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(pid), &_Py_ID(all_threads), &_Py_ID(only_active_thread), &_Py_ID(debug), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"pid", "all_threads", "only_active_thread", "debug", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "RemoteUnwinder",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 1;
    int pid;
    int all_threads = 0;
    int only_active_thread = 0;
    int debug = 0;

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
    debug = PyObject_IsTrue(fastargs[3]);
    if (debug < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _remote_debugging_RemoteUnwinder___init___impl((RemoteUnwinderObject *)self, pid, all_threads, only_active_thread, debug);

exit:
    return return_value;
}

PyDoc_STRVAR(_remote_debugging_RemoteUnwinder_get_stack_trace__doc__,
"get_stack_trace($self, /)\n"
"--\n"
"\n"
"Returns a list of stack traces for threads in the target process.\n"
"\n"
"Each element in the returned list is a tuple of (thread_id, frame_list), where:\n"
"- thread_id is the OS thread identifier\n"
"- frame_list is a list of tuples (function_name, filename, line_number) representing\n"
"  the Python stack frames for that thread, ordered from most recent to oldest\n"
"\n"
"The threads returned depend on the initialization parameters:\n"
"- If only_active_thread was True: returns only the thread holding the GIL\n"
"- If all_threads was True: returns all threads\n"
"- Otherwise: returns only the main thread\n"
"\n"
"Example:\n"
"    [\n"
"        (1234, [\n"
"            (\'process_data\', \'worker.py\', 127),\n"
"            (\'run_worker\', \'worker.py\', 45),\n"
"            (\'main\', \'app.py\', 23)\n"
"        ]),\n"
"        (1235, [\n"
"            (\'handle_request\', \'server.py\', 89),\n"
"            (\'serve_forever\', \'server.py\', 52)\n"
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
/*[clinic end generated code: output=0dd1e6e8bab2a8b1 input=a9049054013a1b77]*/
