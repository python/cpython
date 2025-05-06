#define _GNU_SOURCE
#include "pyconfig.h"

#include "Python.h"
#include "internal/pycore_runtime.h"
#include "internal/pycore_ceval.h"

#if defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)
#include "remote_debug.h"

static int
init_proc_handle(proc_handle_t *handle, pid_t pid) {
    return _Py_RemoteDebug_InitProcHandle(handle, pid);
}

static void
cleanup_proc_handle(proc_handle_t *handle) {
    _Py_RemoteDebug_CleanupProcHandle(handle);
}

static int
read_memory(proc_handle_t *handle, uint64_t remote_address, size_t len, void* dst)
{
    return _Py_RemoteDebug_ReadRemoteMemory(handle, remote_address, len, dst);
}

static int
write_memory(proc_handle_t *handle, uintptr_t remote_address, size_t len, const void* src)
{
#ifdef MS_WINDOWS
    SIZE_T written = 0;
    SIZE_T result = 0;
    do {
        if (!WriteProcessMemory(handle->hProcess, (LPVOID)(remote_address + result), (const char*)src + result, len - result, &written)) {
            PyErr_SetFromWindowsErr(0);
            return -1;
        }
        result += written;
    } while (result < len);
    return 0;
#elif defined(__linux__) && HAVE_PROCESS_VM_READV
    struct iovec local[1];
    struct iovec remote[1];
    Py_ssize_t result = 0;
    Py_ssize_t written = 0;

    do {
        local[0].iov_base = (void*)((char*)src + result);
        local[0].iov_len = len - result;
        remote[0].iov_base = (void*)((char*)remote_address + result);
        remote[0].iov_len = len - result;

        written = process_vm_writev(handle->pid, local, 1, remote, 1, 0);
        if (written < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }

        result += written;
    } while ((size_t)written != local[0].iov_len);
    return 0;
#elif defined(__APPLE__) && TARGET_OS_OSX
    kern_return_t kr = mach_vm_write(
        pid_to_task(handle->pid),
        (mach_vm_address_t)remote_address,
        (vm_offset_t)src,
        (mach_msg_type_number_t)len);

    if (kr != KERN_SUCCESS) {
        switch (kr) {
        case KERN_PROTECTION_FAILURE:
            PyErr_SetString(PyExc_PermissionError, "Not enough permissions to write memory");
            break;
        case KERN_INVALID_ARGUMENT:
            PyErr_SetString(PyExc_PermissionError, "Invalid argument to mach_vm_write");
            break;
        default:
            PyErr_Format(PyExc_RuntimeError, "Unknown error writing memory: %d", (int)kr);
        }
        return -1;
    }
    return 0;
#else
    Py_UNREACHABLE();
#endif
}

static int
is_prerelease_version(uint64_t version)
{
    return (version & 0xF0) != 0xF0;
}

static int
ensure_debug_offset_compatibility(const _Py_DebugOffsets* debug_offsets)
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
            "Can't send commands from a pre-release Python interpreter"
            " to a process running a different Python version");
        return -1;
    }

    if (is_prerelease_version(debug_offsets->version) && Py_Version != debug_offsets->version) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Can't send commands to a pre-release Python interpreter"
            " from a process running a different Python version");
        return -1;
    }

    unsigned int remote_major = (debug_offsets->version >> 24) & 0xFF;
    unsigned int remote_minor = (debug_offsets->version >> 16) & 0xFF;

    if (PY_MAJOR_VERSION != remote_major || PY_MINOR_VERSION != remote_minor) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Can't send commands from a Python %d.%d process to a Python %d.%d process",
            PY_MAJOR_VERSION, PY_MINOR_VERSION, remote_major, remote_minor);
        return -1;
    }

    // The debug offsets differ between free threaded and non-free threaded builds.
    if (_Py_Debug_Free_Threaded && !debug_offsets->free_threaded) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Cannot send commands from a free-threaded Python process"
            " to a process running a non-free-threaded version");
        return -1;
    }

    if (!_Py_Debug_Free_Threaded && debug_offsets->free_threaded) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Cannot send commands to a free-threaded Python process"
            " from a process running a non-free-threaded version");
        return -1;
    }

    return 0;
}

static int
read_offsets(
    proc_handle_t *handle,
    uintptr_t *runtime_start_address,
    _Py_DebugOffsets* debug_offsets
) {
    if (_Py_RemoteDebug_ReadDebugOffsets(handle, runtime_start_address, debug_offsets)) {
        return -1;
    }
    if (ensure_debug_offset_compatibility(debug_offsets)) {
        return -1;
    }
    return 0;
}

static int
send_exec_to_proc_handle(proc_handle_t *handle, int tid, const char *debugger_script_path)
{
    uintptr_t runtime_start_address;
    struct _Py_DebugOffsets debug_offsets;

    if (read_offsets(handle, &runtime_start_address, &debug_offsets)) {
        return -1;
    }

    uintptr_t interpreter_state_list_head = (uintptr_t)debug_offsets.runtime_state.interpreters_head;

    uintptr_t interpreter_state_addr;
    if (0 != read_memory(
            handle,
            runtime_start_address + interpreter_state_list_head,
            sizeof(void*),
            &interpreter_state_addr))
    {
        return -1;
    }

    if (interpreter_state_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Can't find a running interpreter in the remote process");
        return -1;
    }

    int is_remote_debugging_enabled = 0;
    if (0 != read_memory(
            handle,
            interpreter_state_addr + debug_offsets.debugger_support.remote_debugging_enabled,
            sizeof(int),
            &is_remote_debugging_enabled))
    {
        return -1;
    }

    if (is_remote_debugging_enabled != 1) {
        PyErr_SetString(
            PyExc_RuntimeError,
            "Remote debugging is not enabled in the remote process");
        return -1;
    }

    uintptr_t thread_state_addr;
    unsigned long this_tid = 0;

    if (tid != 0) {
        if (0 != read_memory(
                handle,
                interpreter_state_addr + debug_offsets.interpreter_state.threads_head,
                sizeof(void*),
                &thread_state_addr))
        {
            return -1;
        }
        while (thread_state_addr != 0) {
            if (0 != read_memory(
                    handle,
                    thread_state_addr + debug_offsets.thread_state.native_thread_id,
                    sizeof(this_tid),
                    &this_tid))
            {
                return -1;
            }

            if (this_tid == (unsigned long)tid) {
                break;
            }

            if (0 != read_memory(
                    handle,
                    thread_state_addr + debug_offsets.thread_state.next,
                    sizeof(void*),
                    &thread_state_addr))
            {
                return -1;
            }
        }

        if (thread_state_addr == 0) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "Can't find the specified thread in the remote process");
            return -1;
        }
    } else {
        if (0 != read_memory(
                handle,
                interpreter_state_addr + debug_offsets.interpreter_state.threads_main,
                sizeof(void*),
                &thread_state_addr))
        {
            return -1;
        }

        if (thread_state_addr == 0) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "Can't find the main thread in the remote process");
            return -1;
        }
    }

    // Ensure our path is not too long
    if (debug_offsets.debugger_support.debugger_script_path_size <= strlen(debugger_script_path)) {
        PyErr_SetString(PyExc_ValueError, "Debugger script path is too long");
        return -1;
    }

    uintptr_t debugger_script_path_addr = (uintptr_t)(
        thread_state_addr +
        debug_offsets.debugger_support.remote_debugger_support +
        debug_offsets.debugger_support.debugger_script_path);
    if (0 != write_memory(
            handle,
            debugger_script_path_addr,
            strlen(debugger_script_path) + 1,
            debugger_script_path))
    {
        return -1;
    }

    int pending_call = 1;
    uintptr_t debugger_pending_call_addr = (uintptr_t)(
        thread_state_addr +
        debug_offsets.debugger_support.remote_debugger_support +
        debug_offsets.debugger_support.debugger_pending_call);
    if (0 != write_memory(
            handle,
            debugger_pending_call_addr,
            sizeof(int),
            &pending_call))

    {
        return -1;
    }

    uintptr_t eval_breaker;
    if (0 != read_memory(
            handle,
            thread_state_addr + debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker))
    {
        return -1;
    }

    eval_breaker |= _PY_EVAL_PLEASE_STOP_BIT;

    if (0 != write_memory(
            handle,
            thread_state_addr + (uintptr_t)debug_offsets.debugger_support.eval_breaker,
            sizeof(uintptr_t),
            &eval_breaker))

    {
        return -1;
    }

    return 0;
}

#endif // defined(Py_REMOTE_DEBUG) && defined(Py_SUPPORTS_REMOTE_DEBUG)

int
_PySysRemoteDebug_SendExec(int pid, int tid, const char *debugger_script_path)
{
#if !defined(Py_SUPPORTS_REMOTE_DEBUG)
    PyErr_SetString(PyExc_RuntimeError, "Remote debugging is not supported on this platform");
    return -1;
#elif !defined(Py_REMOTE_DEBUG)
    PyErr_SetString(PyExc_RuntimeError, "Remote debugging support has not been compiled in");
    return -1;
#else

    PyThreadState *tstate = _PyThreadState_GET();
    const PyConfig *config = _PyInterpreterState_GetConfig(tstate->interp);
    if (config->remote_debug != 1) {
        PyErr_SetString(PyExc_RuntimeError, "Remote debugging is not enabled");
        return -1;
    }

    proc_handle_t handle;
    if (init_proc_handle(&handle, pid) < 0) {
        return -1;
    }

    int rc = send_exec_to_proc_handle(&handle, tid, debugger_script_path);
    cleanup_proc_handle(&handle);
    return rc;
#endif
}

