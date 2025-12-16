/******************************************************************************
 * Remote Debugging Module - Object Reading Functions
 *
 * This file contains functions for reading Python objects from remote
 * process memory, including strings, bytes, and integers.
 ******************************************************************************/

#include "_remote_debugging.h"

/* ============================================================================
 * MEMORY READING FUNCTIONS
 * ============================================================================ */

#define DEFINE_MEMORY_READER(type_name, c_type, error_msg) \
int \
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

int
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

PyObject *
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

PyObject *
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

long
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
