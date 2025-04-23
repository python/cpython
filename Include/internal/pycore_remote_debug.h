#ifndef Py_INTERNAL_REMOTE_DEBUG_H
#define Py_INTERNAL_REMOTE_DEBUG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdio.h>

#ifndef MS_WINDOWS
#include <unistd.h>
#endif

// Define a platform-independent process handle structure
typedef struct {
    pid_t pid;
#ifdef MS_WINDOWS
    HANDLE hProcess;
#endif
} proc_handle_t;

// Initialize a process handle
PyAPI_FUNC(int) _Py_RemoteDebug_InitProcHandle(proc_handle_t *handle, pid_t pid);

// Cleanup a process handle
PyAPI_FUNC(void) _Py_RemoteDebug_CleanupProcHandle(proc_handle_t *handle);

// Get the PyRuntime section address from a process
PyAPI_FUNC(uintptr_t) _Py_RemoteDebug_GetPyRuntimeAddress(proc_handle_t *handle);

// Get the PyAsyncioDebug section address from a process
PyAPI_FUNC(uintptr_t) _Py_RemoteDebug_GetAsyncioDebugAddress(proc_handle_t *handle);

// Read memory from a remote process
PyAPI_FUNC(int) _Py_RemoteDebug_ReadRemoteMemory(proc_handle_t *handle, uintptr_t remote_address, size_t len, void* dst);

// Read debug offsets from a remote process
PyAPI_FUNC(int) _Py_RemoteDebug_ReadDebugOffsets(proc_handle_t *handle, uintptr_t *runtime_start_address, _Py_DebugOffsets* debug_offsets);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_DEBUG_OFFSETS_H */
