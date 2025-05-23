#define _GNU_SOURCE

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

// Helper to chain exceptions and avoid repetitions
static void
chain_exceptions(PyObject *exception, const char *string)
{
    PyObject *exc = PyErr_GetRaisedException();
    PyErr_SetString(exception, string);
    _PyErr_ChainExceptions1(exc);
}

// Get the PyAsyncioDebug section address for any platform
static uintptr_t
_Py_RemoteDebug_GetAsyncioDebugAddress(proc_handle_t* handle)
{
    uintptr_t address;

#ifdef MS_WINDOWS
    // On Windows, search for asyncio debug in executable or DLL
    address = search_windows_map_for_section(handle, "AsyncioD", L"_asyncio");
#elif defined(__linux__)
    // On Linux, search for asyncio debug in executable or DLL
    address = search_linux_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
#elif defined(__APPLE__) && TARGET_OS_OSX
    // On macOS, try libpython first, then fall back to python
    address = search_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
    if (address == 0) {
        PyErr_Clear();
        address = search_map_for_section(handle, "AsyncioDebug", "_asyncio.cpython");
    }
#else
    Py_UNREACHABLE();
#endif

    return address;
}

static inline int
read_ptr(proc_handle_t *handle, uintptr_t address, uintptr_t *ptr_addr)
{
    int result = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(void*), ptr_addr);
    if (result < 0) {
        return -1;
    }
    return 0;
}

static inline int
read_Py_ssize_t(proc_handle_t *handle, uintptr_t address, Py_ssize_t *size)
{
    int result = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(Py_ssize_t), size);
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
    int res = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(char), result);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static int
read_sized_int(proc_handle_t *handle, uintptr_t address, void *result, size_t size)
{
    int res = _Py_RemoteDebug_ReadRemoteMemory(handle, address, size, result);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static int
read_unsigned_long(proc_handle_t *handle, uintptr_t address, unsigned long *result)
{
    int res = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(unsigned long), result);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static int
read_pyobj(proc_handle_t *handle, uintptr_t address, PyObject *ptr_addr)
{
    int res = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(PyObject), ptr_addr);
    if (res < 0) {
        return -1;
    }
    return 0;
}

static PyObject *
read_py_str(
    proc_handle_t *handle,
    _Py_DebugOffsets* debug_offsets,
    uintptr_t address,
    Py_ssize_t max_len
) {
    PyObject *result = NULL;
    char *buf = NULL;

    Py_ssize_t len;
    int res = _Py_RemoteDebug_ReadRemoteMemory(
        handle,
        address + debug_offsets->unicode_object.length,
        sizeof(Py_ssize_t),
        &len
    );
    if (res < 0) {
        goto err;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    size_t offset = debug_offsets->unicode_object.asciiobject_size;
    res = _Py_RemoteDebug_ReadRemoteMemory(handle, address + offset, len, buf);
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
    _Py_DebugOffsets* debug_offsets,
    uintptr_t address
) {
    PyObject *result = NULL;
    char *buf = NULL;

    Py_ssize_t len;
    int res = _Py_RemoteDebug_ReadRemoteMemory(
        handle,
        address + debug_offsets->bytes_object.ob_size,
        sizeof(Py_ssize_t),
        &len
    );
    if (res < 0) {
        goto err;
    }

    buf = (char *)PyMem_RawMalloc(len+1);
    if (buf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    size_t offset = debug_offsets->bytes_object.ob_sval;
    res = _Py_RemoteDebug_ReadRemoteMemory(handle, address + offset, len, buf);
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
read_py_long(proc_handle_t *handle, _Py_DebugOffsets* offsets, uintptr_t address)
{
    unsigned int shift = PYLONG_BITS_IN_DIGIT;

    Py_ssize_t size;
    uintptr_t lv_tag;

    int bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
        handle, address + offsets->long_object.lv_tag,
        sizeof(uintptr_t),
        &lv_tag);
    if (bytes_read < 0) {
        return -1;
    }

    int negative = (lv_tag & 3) == 2;
    size = lv_tag >> 3;

    if (size == 0) {
        return 0;
    }

    digit *digits = (digit *)PyMem_RawMalloc(size * sizeof(digit));
    if (!digits) {
        PyErr_NoMemory();
        return -1;
    }

    bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
        handle,
        address + offsets->long_object.ob_digit,
        sizeof(digit) * size,
        digits
    );
    if (bytes_read < 0) {
        goto error;
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

static PyObject *
parse_task_name(
    proc_handle_t *handle,
    _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address
) {
    uintptr_t task_name_addr;
    int err = read_py_ptr(
        handle,
        task_address + async_offsets->asyncio_task_object.task_name,
        &task_name_addr);
    if (err) {
        return NULL;
    }

    // The task name can be a long or a string so we need to check the type

    PyObject task_name_obj;
    err = read_pyobj(
        handle,
        task_name_addr,
        &task_name_obj);
    if (err) {
        return NULL;
    }

    unsigned long flags;
    err = read_unsigned_long(
        handle,
        (uintptr_t)task_name_obj.ob_type + offsets->type_object.tp_flags,
        &flags);
    if (err) {
        return NULL;
    }

    if ((flags & Py_TPFLAGS_LONG_SUBCLASS)) {
        long res = read_py_long(handle, offsets, task_name_addr);
        if (res == -1) {
            chain_exceptions(PyExc_RuntimeError, "Failed to get task name");
            return NULL;
        }
        return PyUnicode_FromFormat("Task-%d", res);
    }

    if(!(flags & Py_TPFLAGS_UNICODE_SUBCLASS)) {
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

static int
parse_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame
);

static int
parse_coro_chain(
    proc_handle_t *handle,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t coro_address,
    PyObject *render_to
) {
    assert((void*)coro_address != NULL);

    uintptr_t gen_type_addr;
    int err = read_ptr(
        handle,
        coro_address + offsets->pyobject.ob_type,
        &gen_type_addr);
    if (err) {
        return -1;
    }

    PyObject* name = NULL;
    uintptr_t prev_frame;
    if (parse_frame_object(
                handle,
                &name,
                offsets,
                coro_address + offsets->gen_object.gi_iframe,
                &prev_frame)
        < 0)
    {
        return -1;
    }

    if (PyList_Append(render_to, name)) {
        Py_DECREF(name);
        return -1;
    }
    Py_DECREF(name);

    int8_t gi_frame_state;
    err = read_sized_int(
        handle,
        coro_address + offsets->gen_object.gi_frame_state,
        &gi_frame_state,
        sizeof(int8_t)
    );
    if (err) {
        return -1;
    }

    if (gi_frame_state == FRAME_SUSPENDED_YIELD_FROM) {
        char owner;
        err = read_char(
            handle,
            coro_address + offsets->gen_object.gi_iframe +
                offsets->interpreter_frame.owner,
            &owner
        );
        if (err) {
            return -1;
        }
        if (owner != FRAME_OWNED_BY_GENERATOR) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "generator doesn't own its frame \\_o_/");
            return -1;
        }

        uintptr_t stackpointer_addr;
        err = read_py_ptr(
            handle,
            coro_address + offsets->gen_object.gi_iframe +
                offsets->interpreter_frame.stackpointer,
            &stackpointer_addr);
        if (err) {
            return -1;
        }

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
                int err = read_ptr(
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
                        render_to
                    );
                    if (err) {
                        return -1;
                    }
                }
            }
        }

    }

    return 0;
}


static int
parse_task_awaited_by(
    proc_handle_t *handle,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *awaited_by,
    int recurse_task
);


static int
parse_task(
    proc_handle_t *handle,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *render_to,
    int recurse_task
) {
    char is_task;
    int err = read_char(
        handle,
        task_address + async_offsets->asyncio_task_object.task_is_task,
        &is_task);
    if (err) {
        return -1;
    }

    PyObject* result = PyList_New(0);
    if (result == NULL) {
        return -1;
    }

    PyObject *call_stack = PyList_New(0);
    if (call_stack == NULL) {
        goto err;
    }
    if (PyList_Append(result, call_stack)) {
        Py_DECREF(call_stack);
        goto err;
    }
    /* we can operate on a borrowed one to simplify cleanup */
    Py_DECREF(call_stack);

    if (is_task) {
        PyObject *tn = NULL;
        if (recurse_task) {
            tn = parse_task_name(
                handle, offsets, async_offsets, task_address);
        } else {
            tn = PyLong_FromUnsignedLongLong(task_address);
        }
        if (tn == NULL) {
            goto err;
        }
        if (PyList_Append(result, tn)) {
            Py_DECREF(tn);
            goto err;
        }
        Py_DECREF(tn);

        uintptr_t coro_addr;
        err = read_py_ptr(
            handle,
            task_address + async_offsets->asyncio_task_object.task_coro,
            &coro_addr);
        if (err) {
            goto err;
        }

        if ((void*)coro_addr != NULL) {
            err = parse_coro_chain(
                handle,
                offsets,
                async_offsets,
                coro_addr,
                call_stack
            );
            if (err) {
                goto err;
            }

            if (PyList_Reverse(call_stack)) {
                goto err;
            }
        }
    }

    if (PyList_Append(render_to, result)) {
        goto err;
    }

    if (recurse_task) {
        PyObject *awaited_by = PyList_New(0);
        if (awaited_by == NULL) {
            goto err;
        }
        if (PyList_Append(result, awaited_by)) {
            Py_DECREF(awaited_by);
            goto err;
        }
        /* we can operate on a borrowed one to simplify cleanup */
        Py_DECREF(awaited_by);

        if (parse_task_awaited_by(handle, offsets, async_offsets,
                                task_address, awaited_by, 1)
        ) {
            goto err;
        }
    }
    Py_DECREF(result);

    return 0;

err:
    Py_DECREF(result);
    return -1;
}

static int
parse_tasks_in_set(
    proc_handle_t *handle,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t set_addr,
    PyObject *awaited_by,
    int recurse_task
) {
    uintptr_t set_obj;
    if (read_py_ptr(
            handle,
            set_addr,
            &set_obj)
    ) {
        return -1;
    }

    Py_ssize_t num_els;
    if (read_Py_ssize_t(
            handle,
            set_obj + offsets->set_object.used,
            &num_els)
    ) {
        return -1;
    }

    Py_ssize_t set_len;
    if (read_Py_ssize_t(
            handle,
            set_obj + offsets->set_object.mask,
            &set_len)
    ) {
        return -1;
    }
    set_len++; // The set contains the `mask+1` element slots.

    uintptr_t table_ptr;
    if (read_ptr(
            handle,
            set_obj + offsets->set_object.table,
            &table_ptr)
    ) {
        return -1;
    }

    Py_ssize_t i = 0;
    Py_ssize_t els = 0;
    while (i < set_len) {
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
                    recurse_task
                )
                ) {
                    return -1;
                }

                if (++els == num_els) {
                    break;
                }
            }
        }

        table_ptr += sizeof(void*) * 2;
        i++;
    }
    return 0;
}


static int
parse_task_awaited_by(
    proc_handle_t *handle,
    struct _Py_DebugOffsets* offsets,
    struct _Py_AsyncioModuleDebugOffsets* async_offsets,
    uintptr_t task_address,
    PyObject *awaited_by,
    int recurse_task
) {
    uintptr_t task_ab_addr;
    int err = read_py_ptr(
        handle,
        task_address + async_offsets->asyncio_task_object.task_awaited_by,
        &task_ab_addr);
    if (err) {
        return -1;
    }

    if ((void*)task_ab_addr == NULL) {
        return 0;
    }

    char awaited_by_is_a_set;
    err = read_char(
        handle,
        task_address + async_offsets->asyncio_task_object.task_awaited_by_is_set,
        &awaited_by_is_a_set);
    if (err) {
        return -1;
    }

    if (awaited_by_is_a_set) {
        if (parse_tasks_in_set(
            handle,
            offsets,
            async_offsets,
            task_address + async_offsets->asyncio_task_object.task_awaited_by,
            awaited_by,
            recurse_task
        )
         ) {
            return -1;
        }
    } else {
        uintptr_t sub_task;
        if (read_py_ptr(
                handle,
                task_address + async_offsets->asyncio_task_object.task_awaited_by,
                &sub_task)
        ) {
            return -1;
        }

        if (parse_task(
            handle,
            offsets,
            async_offsets,
            sub_task,
            awaited_by,
            recurse_task
        )
        ) {
            return -1;
        }
    }

    return 0;
}

typedef struct
{
    int lineno;
    int end_lineno;
    int column;
    int end_column;
} LocationInfo;

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
                assert((second_byte & 128) == 0);
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

static int
read_remote_pointer(proc_handle_t *handle, uintptr_t address, uintptr_t *out_ptr, const char *error_message)
{
    int bytes_read = _Py_RemoteDebug_ReadRemoteMemory(handle, address, sizeof(void *), out_ptr);
    if (bytes_read < 0) {
        return -1;
    }

    if ((void *)(*out_ptr) == NULL) {
        PyErr_SetString(PyExc_RuntimeError, error_message);
        return -1;
    }

    return 0;
}

static int
read_instruction_ptr(proc_handle_t *handle, struct _Py_DebugOffsets *offsets,
                     uintptr_t current_frame, uintptr_t *instruction_ptr)
{
    return read_remote_pointer(
        handle,
        current_frame + offsets->interpreter_frame.instr_ptr,
        instruction_ptr,
        "No instruction ptr found"
    );
}

static int
parse_code_object(proc_handle_t *handle,
                  PyObject **result,
                  struct _Py_DebugOffsets *offsets,
                  uintptr_t address,
                  uintptr_t current_frame,
                  uintptr_t *previous_frame)
{
    uintptr_t addr_func_name, addr_file_name, addr_linetable, instruction_ptr;

    if (read_remote_pointer(handle, address + offsets->code_object.qualname, &addr_func_name, "No function name found") < 0 ||
        read_remote_pointer(handle, address + offsets->code_object.filename, &addr_file_name, "No file name found") < 0 ||
        read_remote_pointer(handle, address + offsets->code_object.linetable, &addr_linetable, "No linetable found") < 0 ||
        read_instruction_ptr(handle, offsets, current_frame, &instruction_ptr) < 0) {
        return -1;
    }

    int firstlineno;
    if (_Py_RemoteDebug_ReadRemoteMemory(handle,
                                         address + offsets->code_object.firstlineno,
                                         sizeof(int),
                                         &firstlineno) < 0) {
        return -1;
    }

    PyObject *py_linetable = read_py_bytes(handle, offsets, addr_linetable);
    if (!py_linetable) {
        return -1;
    }

    uintptr_t addr_code_adaptive = address + offsets->code_object.co_code_adaptive;
    ptrdiff_t addrq = (uint16_t *)instruction_ptr - (uint16_t *)addr_code_adaptive;

    LocationInfo info;
    parse_linetable(addrq, PyBytes_AS_STRING(py_linetable), firstlineno, &info);
    Py_DECREF(py_linetable);  // Done with linetable

    PyObject *py_line = PyLong_FromLong(info.lineno);
    if (!py_line) {
        return -1;
    }

    PyObject *py_func_name = read_py_str(handle, offsets, addr_func_name, 256);
    if (!py_func_name) {
        Py_DECREF(py_line);
        return -1;
    }

    PyObject *py_file_name = read_py_str(handle, offsets, addr_file_name, 256);
    if (!py_file_name) {
        Py_DECREF(py_line);
        Py_DECREF(py_func_name);
        return -1;
    }

    PyObject *result_tuple = PyTuple_New(3);
    if (!result_tuple) {
        Py_DECREF(py_line);
        Py_DECREF(py_func_name);
        Py_DECREF(py_file_name);
        return -1;
    }

    PyTuple_SET_ITEM(result_tuple, 0, py_func_name);  // steals ref
    PyTuple_SET_ITEM(result_tuple, 1, py_file_name);  // steals ref
    PyTuple_SET_ITEM(result_tuple, 2, py_line);       // steals ref

    *result = result_tuple;
    return 0;
}

static int
parse_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame
) {
    int err;

    Py_ssize_t bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
        handle,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    char owner;
    if (read_char(handle, address + offsets->interpreter_frame.owner, &owner)) {
        return -1;
    }

    if (owner >= FRAME_OWNED_BY_INTERPRETER) {
        return 0;
    }

    uintptr_t address_of_code_object;
    err = read_py_ptr(
        handle,
        address + offsets->interpreter_frame.executable,
        &address_of_code_object
    );
    if (err) {
        return -1;
    }

    if ((void*)address_of_code_object == NULL) {
        return 0;
    }

    return parse_code_object(
        handle, result, offsets, address_of_code_object, address, previous_frame);
}

static int
parse_async_frame_object(
    proc_handle_t *handle,
    PyObject** result,
    struct _Py_DebugOffsets* offsets,
    uintptr_t address,
    uintptr_t* previous_frame,
    uintptr_t* code_object
) {
    int err;

    Py_ssize_t bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
        handle,
        address + offsets->interpreter_frame.previous,
        sizeof(void*),
        previous_frame
    );
    if (bytes_read < 0) {
        return -1;
    }

    char owner;
    bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
        handle, address + offsets->interpreter_frame.owner, sizeof(char), &owner);
    if (bytes_read < 0) {
        return -1;
    }

    if (owner == FRAME_OWNED_BY_CSTACK || owner == FRAME_OWNED_BY_INTERPRETER) {
        return 0;  // C frame
    }

    if (owner != FRAME_OWNED_BY_GENERATOR
        && owner != FRAME_OWNED_BY_THREAD) {
        PyErr_Format(PyExc_RuntimeError, "Unhandled frame owner %d.\n", owner);
        return -1;
    }

    err = read_py_ptr(
        handle,
        address + offsets->interpreter_frame.executable,
        code_object
    );
    if (err) {
        return -1;
    }

    assert(code_object != NULL);
    if ((void*)*code_object == NULL) {
        return 0;
    }

    if (parse_code_object(
        handle, result, offsets, *code_object, address, previous_frame)) {
        return -1;
    }

    return 1;
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
    int result = _Py_RemoteDebug_ReadRemoteMemory(handle, async_debug_addr, size, async_debug);
    return result;
}

static int
find_running_frame(
    proc_handle_t *handle,
    uintptr_t runtime_start_address,
    _Py_DebugOffsets* local_debug_offsets,
    uintptr_t *frame
) {
    uint64_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
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
    bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
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
    _Py_DebugOffsets *local_debug_offsets,
    struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    uintptr_t *running_task_addr
) {
    *running_task_addr = (uintptr_t)NULL;

    uint64_t interpreter_state_list_head =
        local_debug_offsets->runtime_state.interpreters_head;

    uintptr_t address_of_interpreter_state;
    int bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
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
    bytes_read = _Py_RemoteDebug_ReadRemoteMemory(
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
append_awaited_by_for_thread(
    proc_handle_t *handle,
    uintptr_t head_addr,
    struct _Py_DebugOffsets *debug_offsets,
    struct _Py_AsyncioModuleDebugOffsets *async_offsets,
    PyObject *result
) {
    struct llist_node task_node;

    if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                handle,
                head_addr,
                sizeof(task_node),
                &task_node))
    {
        return -1;
    }

    size_t iteration_count = 0;
    const size_t MAX_ITERATIONS = 2 << 15;  // A reasonable upper bound
    while ((uintptr_t)task_node.next != head_addr) {
        if (++iteration_count > MAX_ITERATIONS) {
            PyErr_SetString(PyExc_RuntimeError, "Task list appears corrupted");
            return -1;
        }

        if (task_node.next == NULL) {
            PyErr_SetString(
                PyExc_RuntimeError,
                "Invalid linked list structure reading remote memory");
            return -1;
        }

        uintptr_t task_addr = (uintptr_t)task_node.next
            - async_offsets->asyncio_task_object.task_node;

        PyObject *tn = parse_task_name(
            handle,
            debug_offsets,
            async_offsets,
            task_addr);
        if (tn == NULL) {
            return -1;
        }

        PyObject *current_awaited_by = PyList_New(0);
        if (current_awaited_by == NULL) {
            Py_DECREF(tn);
            return -1;
        }

        PyObject* task_id = PyLong_FromUnsignedLongLong(task_addr);
        if (task_id == NULL) {
            Py_DECREF(tn);
            Py_DECREF(current_awaited_by);
            return -1;
        }

        PyObject *result_item = PyTuple_New(3);
        if (result_item == NULL) {
            Py_DECREF(tn);
            Py_DECREF(current_awaited_by);
            Py_DECREF(task_id);
            return -1;
        }

        PyTuple_SET_ITEM(result_item, 0, task_id);  // steals ref
        PyTuple_SET_ITEM(result_item, 1, tn);  // steals ref
        PyTuple_SET_ITEM(result_item, 2, current_awaited_by);  // steals ref
        if (PyList_Append(result, result_item)) {
            Py_DECREF(result_item);
            return -1;
        }
        Py_DECREF(result_item);

        if (parse_task_awaited_by(handle, debug_offsets, async_offsets,
                                  task_addr, current_awaited_by, 0))
        {
            return -1;
        }

        // onto the next one...
        if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                    handle,
                    (uintptr_t)task_node.next,
                    sizeof(task_node),
                    &task_node))
        {
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
    struct _Py_DebugOffsets *debug_offsets,
    struct _Py_AsyncioModuleDebugOffsets *async_offsets,
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
            awaited_by_for_thread))
    {
        return -1;
    }

    return 0;
}

static PyObject*
get_all_awaited_by(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__))  && !defined(MS_WINDOWS) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_all_awaited_by is not implemented on this platform");
    return NULL;
#endif

    int pid;
    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    proc_handle_t the_handle;
    proc_handle_t *handle = &the_handle;
    if (_Py_RemoteDebug_InitProcHandle(handle, pid) < 0) {
        return 0;
    }

    PyObject *result = NULL;

    uintptr_t runtime_start_addr = _Py_RemoteDebug_GetPyRuntimeAddress(handle);
    if (runtime_start_addr == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        goto result_err;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (_Py_RemoteDebug_ReadDebugOffsets(handle, &runtime_start_addr, &local_debug_offsets)) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read debug offsets");
        goto result_err;
    }

    struct _Py_AsyncioModuleDebugOffsets local_async_debug;
    if (read_async_debug(handle, &local_async_debug)) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read asyncio debug offsets");
        goto result_err;
    }

    result = PyList_New(0);
    if (result == NULL) {
        goto result_err;
    }

    uint64_t interpreter_state_list_head =
        local_debug_offsets.runtime_state.interpreters_head;

    uintptr_t interpreter_state_addr;
    if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                handle,
                runtime_start_addr + interpreter_state_list_head,
                sizeof(void*),
                &interpreter_state_addr))
    {
        goto result_err;
    }

    uintptr_t thread_state_addr;
    unsigned long tid = 0;
    if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                handle,
                interpreter_state_addr
                + local_debug_offsets.interpreter_state.threads_head,
                sizeof(void*),
                &thread_state_addr))
    {
        goto result_err;
    }

    uintptr_t head_addr;
    while (thread_state_addr != 0) {
        if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                    handle,
                    thread_state_addr
                    + local_debug_offsets.thread_state.native_thread_id,
                    sizeof(tid),
                    &tid))
        {
            goto result_err;
        }

        head_addr = thread_state_addr
            + local_async_debug.asyncio_thread_state.asyncio_tasks_head;

        if (append_awaited_by(handle, tid, head_addr, &local_debug_offsets,
                              &local_async_debug, result))
        {
            goto result_err;
        }

        if (0 > _Py_RemoteDebug_ReadRemoteMemory(
                    handle,
                    thread_state_addr + local_debug_offsets.thread_state.next,
                    sizeof(void*),
                    &thread_state_addr))
        {
            goto result_err;
        }
    }

    head_addr = interpreter_state_addr
        + local_async_debug.asyncio_interpreter_state.asyncio_tasks_head;

    // On top of a per-thread task lists used by default by asyncio to avoid
    // contention, there is also a fallback per-interpreter list of tasks;
    // any tasks still pending when a thread is destroyed will be moved to the
    // per-interpreter task list.  It's unlikely we'll find anything here, but
    // interesting for debugging.
    if (append_awaited_by(handle, 0, head_addr, &local_debug_offsets,
                        &local_async_debug, result))
    {
        goto result_err;
    }

    _Py_RemoteDebug_CleanupProcHandle(handle);
    return result;

result_err:
    Py_XDECREF(result);
    _Py_RemoteDebug_CleanupProcHandle(handle);
    return NULL;
}

static PyObject*
get_stack_trace(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__))  && !defined(MS_WINDOWS) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif

    int pid;
    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    proc_handle_t the_handle;
    proc_handle_t *handle = &the_handle;
    if (_Py_RemoteDebug_InitProcHandle(handle, pid) < 0) {
        return 0;
    }

    PyObject* result = NULL;

    uintptr_t runtime_start_address = _Py_RemoteDebug_GetPyRuntimeAddress(handle);
    if (runtime_start_address == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        goto result_err;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (_Py_RemoteDebug_ReadDebugOffsets(handle, &runtime_start_address, &local_debug_offsets)) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read debug offsets");
        goto result_err;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        handle, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        goto result_err;
    }

    result = PyList_New(0);
    if (result == NULL) {
        goto result_err;
    }

    while ((void*)address_of_current_frame != NULL) {
        PyObject* frame_info = NULL;
        if (parse_frame_object(
                    handle,
                    &frame_info,
                    &local_debug_offsets,
                    address_of_current_frame,
                    &address_of_current_frame)
            < 0)
        {
            Py_CLEAR(result);
            goto result_err;
        }

        if (!frame_info) {
            continue;
        }

        if (PyList_Append(result, frame_info) == -1) {
            Py_CLEAR(result);
            goto result_err;
        }

        Py_DECREF(frame_info);
        frame_info = NULL;

    }

result_err:
    _Py_RemoteDebug_CleanupProcHandle(handle);
    return result;
}

static PyObject*
get_async_stack_trace(PyObject* self, PyObject* args)
{
#if (!defined(__linux__) && !defined(__APPLE__))  && !defined(MS_WINDOWS) || \
    (defined(__linux__) && !HAVE_PROCESS_VM_READV)
    PyErr_SetString(
        PyExc_RuntimeError,
        "get_stack_trace is not supported on this platform");
    return NULL;
#endif
    int pid;

    if (!PyArg_ParseTuple(args, "i", &pid)) {
        return NULL;
    }

    proc_handle_t the_handle;
    proc_handle_t *handle = &the_handle;
    if (_Py_RemoteDebug_InitProcHandle(handle, pid) < 0) {
        return 0;
    }

    PyObject *result = NULL;

    uintptr_t runtime_start_address = _Py_RemoteDebug_GetPyRuntimeAddress(handle);
    if (runtime_start_address == 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(
                PyExc_RuntimeError, "Failed to get .PyRuntime address");
        }
        goto result_err;
    }
    struct _Py_DebugOffsets local_debug_offsets;

    if (_Py_RemoteDebug_ReadDebugOffsets(handle, &runtime_start_address, &local_debug_offsets)) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read debug offsets");
        goto result_err;
    }

    struct _Py_AsyncioModuleDebugOffsets local_async_debug;
    if (read_async_debug(handle, &local_async_debug)) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read asyncio debug offsets");
        goto result_err;
    }

    result = PyList_New(1);
    if (result == NULL) {
        goto result_err;
    }
    PyObject* calls = PyList_New(0);
    if (calls == NULL) {
        goto result_err;
    }
    if (PyList_SetItem(result, 0, calls)) { /* steals ref to 'calls' */
        Py_DECREF(calls);
        goto result_err;
    }

    uintptr_t running_task_addr = (uintptr_t)NULL;
    if (find_running_task(
        handle, runtime_start_address, &local_debug_offsets, &local_async_debug,
        &running_task_addr)
    ) {
        chain_exceptions(PyExc_RuntimeError, "Failed to find running task");
        goto result_err;
    }

    if ((void*)running_task_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "No running task found");
        goto result_err;
    }

    uintptr_t running_coro_addr;
    if (read_py_ptr(
        handle,
        running_task_addr + local_async_debug.asyncio_task_object.task_coro,
        &running_coro_addr
    )) {
        chain_exceptions(PyExc_RuntimeError, "Failed to read running task coro");
        goto result_err;
    }

    if ((void*)running_coro_addr == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task coro is NULL");
        goto result_err;
    }

    // note: genobject's gi_iframe is an embedded struct so the address to
    // the offset leads directly to its first field: f_executable
    uintptr_t address_of_running_task_code_obj;
    if (read_py_ptr(
        handle,
        running_coro_addr + local_debug_offsets.gen_object.gi_iframe,
        &address_of_running_task_code_obj
    )) {
        goto result_err;
    }

    if ((void*)address_of_running_task_code_obj == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Running task code object is NULL");
        goto result_err;
    }

    uintptr_t address_of_current_frame;
    if (find_running_frame(
        handle, runtime_start_address, &local_debug_offsets,
        &address_of_current_frame)
    ) {
        chain_exceptions(PyExc_RuntimeError, "Failed to find running frame");
        goto result_err;
    }

    uintptr_t address_of_code_object;
    while ((void*)address_of_current_frame != NULL) {
        PyObject* frame_info = NULL;
        int res = parse_async_frame_object(
            handle,
            &frame_info,
            &local_debug_offsets,
            address_of_current_frame,
            &address_of_current_frame,
            &address_of_code_object
        );

        if (res < 0) {
            chain_exceptions(PyExc_RuntimeError, "Failed to parse async frame object");
            goto result_err;
        }

        if (!frame_info) {
            continue;
        }

        if (PyList_Append(calls, frame_info) == -1) {
            Py_DECREF(calls);
            goto result_err;
        }

        Py_DECREF(frame_info);
        frame_info = NULL;

        if (address_of_code_object == address_of_running_task_code_obj) {
            break;
        }
    }

    PyObject *tn = parse_task_name(
        handle, &local_debug_offsets, &local_async_debug, running_task_addr);
    if (tn == NULL) {
        goto result_err;
    }
    if (PyList_Append(result, tn)) {
        Py_DECREF(tn);
        goto result_err;
    }
    Py_DECREF(tn);

    PyObject* awaited_by = PyList_New(0);
    if (awaited_by == NULL) {
        goto result_err;
    }
    if (PyList_Append(result, awaited_by)) {
        Py_DECREF(awaited_by);
        goto result_err;
    }
    Py_DECREF(awaited_by);

    if (parse_task_awaited_by(
        handle, &local_debug_offsets, &local_async_debug,
        running_task_addr, awaited_by, 1)
    ) {
        goto result_err;
    }

    _Py_RemoteDebug_CleanupProcHandle(handle);
    return result;

result_err:
    _Py_RemoteDebug_CleanupProcHandle(handle);
    Py_XDECREF(result);
    return NULL;
}


static PyMethodDef methods[] = {
    {"get_stack_trace", get_stack_trace, METH_VARARGS,
        "Get the Python stack from a given pid"},
    {"get_async_stack_trace", get_async_stack_trace, METH_VARARGS,
        "Get the asyncio stack from a given pid"},
    {"get_all_awaited_by", get_all_awaited_by, METH_VARARGS,
        "Get all tasks and their awaited_by from a given pid"},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_remote_debugging",
    .m_size = -1,
    .m_methods = methods,
};

PyMODINIT_FUNC
PyInit__remote_debugging(void)
{
    PyObject* mod = PyModule_Create(&module);
    if (mod == NULL) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    PyUnstable_Module_SetGIL(mod, Py_MOD_GIL_NOT_USED);
#endif
    int rc = PyModule_AddIntConstant(
        mod, "PROCESS_VM_READV_SUPPORTED", HAVE_PROCESS_VM_READV);
    if (rc < 0) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}
