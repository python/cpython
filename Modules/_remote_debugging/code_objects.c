/******************************************************************************
 * Remote Debugging Module - Code Object Functions
 *
 * This file contains functions for parsing code objects and line tables
 * from remote process memory.
 ******************************************************************************/

#include "_remote_debugging.h"

/* ============================================================================
 * TLBC CACHING FUNCTIONS (Py_GIL_DISABLED only)
 * ============================================================================ */

#ifdef Py_GIL_DISABLED

void
tlbc_cache_entry_destroy(void *ptr)
{
    TLBCCacheEntry *entry = (TLBCCacheEntry *)ptr;
    if (entry->tlbc_array) {
        PyMem_RawFree(entry->tlbc_array);
    }
    PyMem_RawFree(entry);
}

TLBCCacheEntry *
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

int
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

bool
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
 * CODE OBJECT AND FRAME INFO FUNCTIONS
 * ============================================================================ */

PyObject *
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

int
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
