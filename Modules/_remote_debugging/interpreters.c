/******************************************************************************
 * Remote Debugging Module - Interpreters Functions
 *
 * This file contains function for iterating interpreters.
 ******************************************************************************/

#include "_remote_debugging.h"

#ifndef MS_WINDOWS
#include <unistd.h>
#endif

#ifdef __linux__
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#endif

/* ============================================================================
 * INTERPRETERS ITERATION FUNCTION
 * ============================================================================ */

int
iterate_interpreters(
    RuntimeOffsets *offsets,
    interpreter_processor_func processor,
    void *context
) {

    uintptr_t interpreter_state_list_head =
        (uintptr_t)offsets->debug_offsets.runtime_state.interpreters_head;
    uintptr_t interpreter_state_offset =
        offsets->runtime_start_address + interpreter_state_list_head;
    uintptr_t interpreter_id_offset =
        (uintptr_t)offsets->debug_offsets.interpreter_state.id;
    uintptr_t interpreter_next_offset =
        (uintptr_t)offsets->debug_offsets.interpreter_state.next;

    unsigned long iid = 0;
    uintptr_t interpreter_state_addr;
    if (_Py_RemoteDebug_ReadRemoteMemory(&offsets->handle,
                                         interpreter_state_offset,
                                         sizeof(void*),
                                         &interpreter_state_addr) < 0) {
        _set_debug_exception_cause(PyExc_RuntimeError, "Failed to read interpreter state address");
        return -1;
    }

    if (interpreter_state_addr == 0) {
        _set_debug_exception_cause(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    while (interpreter_state_addr != 0) {
        if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                    &offsets->handle,
                    interpreter_state_addr + interpreter_id_offset,
                    sizeof(iid),
                    &iid)) {
            _set_debug_exception_cause(PyExc_RuntimeError, "Failed to read next interpreter state");
            return -1;
        }

        // Call the processor function for this interpreter
        if (processor(offsets, interpreter_state_addr, iid, context) < 0) {
            return -1;
        }

        if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                    &offsets->handle,
                    interpreter_state_addr + interpreter_next_offset,
                    sizeof(void*),
                    &interpreter_state_addr)) {
            _set_debug_exception_cause(PyExc_RuntimeError, "Failed to read next interpreter state");
            return -1;
        }
    }

    return 0;
}
