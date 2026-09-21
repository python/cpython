/******************************************************************************
 * Remote Debugging Module - Interpreters Functions
 *
 * This file contains function for iterating interpreters.
 ******************************************************************************/

#include "_remote_debugging.h"

int
iterate_interpreters(
    RuntimeOffsets *offsets,
    interpreter_processor_func processor,
    void *context
) {
    uintptr_t interpreters_head_addr =
        offsets->runtime_start_address
        + (uintptr_t)offsets->debug_offsets.runtime_state.interpreters_head;
    uintptr_t interpreter_id_offset =
        (uintptr_t)offsets->debug_offsets.interpreter_state.id;
    uintptr_t interpreter_next_offset =
        (uintptr_t)offsets->debug_offsets.interpreter_state.next;

    uintptr_t interpreter_state_addr;
    if (_Py_RemoteDebug_ReadRemoteMemory(&offsets->handle,
                                         interpreters_head_addr,
                                         sizeof(void*),
                                         &interpreter_state_addr) < 0) {
        set_exception_cause(offsets, PyExc_RuntimeError, "Failed to read interpreter state address");
        return -1;
    }

    if (interpreter_state_addr == 0) {
        PyErr_SetString(PyExc_RuntimeError, "No interpreter state found");
        return -1;
    }

    int64_t iid = 0;
    static_assert(
        sizeof((((PyInterpreterState*)NULL)->id)) == sizeof(iid),
        "Sizeof of PyInterpreterState.id mismatch with local iid value");
    while (interpreter_state_addr != 0) {
        if (_Py_RemoteDebug_ReadRemoteMemory(
                    &offsets->handle,
                    interpreter_state_addr + interpreter_id_offset,
                    sizeof(iid),
                    &iid) < 0) {
            set_exception_cause(offsets, PyExc_RuntimeError, "Failed to read interpreter id");
            return -1;
        }

        if (processor(offsets, interpreter_state_addr, iid, context) < 0) {
            return -1;
        }

        if (_Py_RemoteDebug_ReadRemoteMemory(
                    &offsets->handle,
                    interpreter_state_addr + interpreter_next_offset,
                    sizeof(void*),
                    &interpreter_state_addr) < 0) {
            set_exception_cause(offsets, PyExc_RuntimeError, "Failed to read next interpreter state");
            return -1;
        }
    }

    return 0;
}
