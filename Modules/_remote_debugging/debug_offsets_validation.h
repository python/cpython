#ifndef Py_REMOTE_DEBUG_OFFSETS_VALIDATION_H
#define Py_REMOTE_DEBUG_OFFSETS_VALIDATION_H

/*
 * The remote debugging tables are read from the target process and must be
 * treated as untrusted input. This header centralizes the one-time validation
 * that runs immediately after those tables are read, before the unwinder uses
 * any reported sizes or offsets to copy remote structs into fixed local
 * buffers or to interpret those local copies.
 *
 * The key rule is simple: every offset that is later dereferenced against a
 * local buffer or local object view must appear in one of the field lists
 * below. Validation then checks two bounds for each field:
 *
 * 1. The field must fit within the section size reported by the target.
 * 2. The same field must also fit within the local buffer or local layout the
 *    debugger will actually use.
 *
 * Sections that are copied into fixed local buffers also have their reported
 * size checked against the corresponding local buffer size up front.
 *
 * This is intentionally front-loaded. Once validation succeeds, the hot path
 * can keep using the raw offsets without adding per-sample bounds checks.
 *
 * Maintenance rule: if either exported table grows, the static_asserts below
 * should yell at you. When that happens, update the matching field lists in
 * this file in the same change. And if you add a new field that the unwinder
 * is going to poke at later, put it in the right list here too, so nobody has
 * to rediscover this the annoying way.
 */
#define FIELD_SIZE(type, member) sizeof(((type *)0)->member)

enum {
    PY_REMOTE_DEBUG_OFFSETS_TOTAL_SIZE = 840,
    PY_REMOTE_ASYNC_DEBUG_OFFSETS_TOTAL_SIZE = 104,
};

/*
 * These asserts are the coordination tripwire for table growth. If either
 * exported table changes size, update the validation lists below in the same
 * change.
 */
static_assert(
    sizeof(_Py_DebugOffsets) == PY_REMOTE_DEBUG_OFFSETS_TOTAL_SIZE,
    "Update _remote_debugging validation for _Py_DebugOffsets");
static_assert(
    sizeof(struct _Py_AsyncioModuleDebugOffsets) ==
        PY_REMOTE_ASYNC_DEBUG_OFFSETS_TOTAL_SIZE,
    "Update _remote_debugging validation for _Py_AsyncioModuleDebugOffsets");

/*
 * This logic lives in a private header because it is shared by module.c and
 * asyncio.c. Keep the helpers static inline so they stay local to those users
 * without adding another compilation unit or exported symbols.
 */
static inline int
validate_section_size(const char *section_name, uint64_t size)
{
    if (size == 0) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid debug offsets: %s.size must be greater than zero",
            section_name);
        return -1;
    }
    return 0;
}

static inline int
validate_read_size(const char *section_name, uint64_t size, size_t buffer_size)
{
    if (validate_section_size(section_name, size) < 0) {
        return -1;
    }
    if (size > buffer_size) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid debug offsets: %s.size=%llu exceeds local buffer size %zu",
            section_name,
            (unsigned long long)size,
            buffer_size);
        return -1;
    }
    return 0;
}

static inline int
validate_span(
    const char *field_name,
    uint64_t offset,
    size_t width,
    uint64_t limit,
    const char *limit_name)
{
    uint64_t span = (uint64_t)width;
    if (span > limit || offset > limit - span) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid debug offsets: %s=%llu with width %zu exceeds %s %llu",
            field_name,
            (unsigned long long)offset,
            width,
            limit_name,
            (unsigned long long)limit);
        return -1;
    }
    return 0;
}

static inline int
validate_alignment(
    const char *field_name,
    uint64_t offset,
    size_t alignment)
{
    if (alignment > 1 && offset % alignment != 0) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid debug offsets: %s=%llu is not aligned to %zu bytes",
            field_name,
            (unsigned long long)offset,
            alignment);
        return -1;
    }
    return 0;
}

static inline int
validate_field(
    const char *field_name,
    uint64_t reported_size,
    uint64_t offset,
    size_t width,
    size_t alignment,
    size_t buffer_size)
{
    if (validate_alignment(field_name, offset, alignment) < 0) {
        return -1;
    }
    if (validate_span(field_name, offset, width, reported_size, "reported size") < 0) {
        return -1;
    }
    return validate_span(field_name, offset, width, buffer_size, "local buffer size");
}

static inline int
validate_nested_field(
    const char *field_name,
    uint64_t reported_size,
    uint64_t base_offset,
    uint64_t nested_offset,
    size_t width,
    size_t alignment,
    size_t buffer_size)
{
    if (base_offset > UINT64_MAX - nested_offset) {
        PyErr_Format(
            PyExc_RuntimeError,
            "Invalid debug offsets: %s overflows the offset calculation",
            field_name);
        return -1;
    }
    return validate_field(
        field_name,
        reported_size,
        base_offset + nested_offset,
        width,
        alignment,
        buffer_size);
}

static inline int
validate_fixed_field(
    const char *field_name,
    uint64_t offset,
    size_t width,
    size_t alignment,
    size_t buffer_size)
{
    if (validate_alignment(field_name, offset, alignment) < 0) {
        return -1;
    }
    return validate_span(field_name, offset, width, buffer_size, "local buffer size");
}

#define PY_REMOTE_DEBUG_VALIDATE_SECTION(section) \
    do { \
        if (validate_section_size(#section, debug_offsets->section.size) < 0) { \
            return -1; \
        } \
    } while (0)

#define PY_REMOTE_DEBUG_VALIDATE_READ_SECTION(section, buffer_size) \
    do { \
        if (validate_read_size(#section, debug_offsets->section.size, buffer_size) < 0) { \
            return -1; \
        } \
    } while (0)

#define PY_REMOTE_DEBUG_VALIDATE_FIELD(section, field, field_size, field_alignment, buffer_size) \
    do { \
        if (validate_field( \
                #section "." #field, \
                debug_offsets->section.size, \
                debug_offsets->section.field, \
                field_size, \
                field_alignment, \
                buffer_size) < 0) { \
            return -1; \
        } \
    } while (0)

#define PY_REMOTE_DEBUG_VALIDATE_NESTED_FIELD(section, base, nested_section, field, field_size, field_alignment, buffer_size) \
    do { \
        if (validate_nested_field( \
                #section "." #base "." #field, \
                debug_offsets->section.size, \
                debug_offsets->section.base, \
                debug_offsets->nested_section.field, \
                field_size, \
                field_alignment, \
                buffer_size) < 0) { \
            return -1; \
        } \
    } while (0)

#define PY_REMOTE_DEBUG_VALIDATE_FIXED_FIELD(section, field, field_size, field_alignment, buffer_size) \
    do { \
        if (validate_fixed_field( \
                #section "." #field, \
                debug_offsets->section.field, \
                field_size, \
                field_alignment, \
                buffer_size) < 0) { \
            return -1; \
        } \
    } while (0)

/*
 * Each list below must include every offset that is later dereferenced against
 * a local buffer or local object view. The validator checks that each field
 * stays within both the remote table's reported section size and the local
 * buffer size we use when reading that section. If a new dereferenced field is
 * added to the offset tables, add it to the matching list here.
 *
 * Sections not listed here are present in the offset tables but not used by
 * the unwinder, so no validation is needed for them.
 */
#define PY_REMOTE_DEBUG_RUNTIME_STATE_FIELDS(APPLY, buffer_size) \
    APPLY(runtime_state, interpreters_head, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size)

#define PY_REMOTE_DEBUG_THREAD_STATE_FIELDS(APPLY, buffer_size) \
    APPLY(thread_state, native_thread_id, sizeof(unsigned long), _Alignof(long), buffer_size); \
    APPLY(thread_state, interp, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, datastack_chunk, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, status, FIELD_SIZE(PyThreadState, _status), _Alignof(unsigned int), buffer_size); \
    APPLY(thread_state, holds_gil, sizeof(int), _Alignof(int), buffer_size); \
    APPLY(thread_state, gil_requested, sizeof(int), _Alignof(int), buffer_size); \
    APPLY(thread_state, current_exception, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, thread_id, sizeof(unsigned long), _Alignof(long), buffer_size); \
    APPLY(thread_state, next, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, current_frame, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, base_frame, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(thread_state, last_profiled_frame, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size)

#define PY_REMOTE_DEBUG_INTERPRETER_STATE_FIELDS(APPLY, buffer_size) \
    APPLY(interpreter_state, id, sizeof(int64_t), _Alignof(int64_t), buffer_size); \
    APPLY(interpreter_state, next, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_state, threads_head, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_state, threads_main, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_state, gil_runtime_state_locked, sizeof(int), _Alignof(int), buffer_size); \
    APPLY(interpreter_state, gil_runtime_state_holder, sizeof(PyThreadState *), _Alignof(PyThreadState *), buffer_size); \
    APPLY(interpreter_state, code_object_generation, sizeof(uint64_t), _Alignof(uint64_t), buffer_size); \
    APPLY(interpreter_state, tlbc_generation, sizeof(uint32_t), _Alignof(uint32_t), buffer_size)

#define PY_REMOTE_DEBUG_INTERPRETER_FRAME_FIELDS(APPLY, buffer_size) \
    APPLY(interpreter_frame, previous, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_frame, executable, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_frame, instr_ptr, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_frame, owner, sizeof(char), _Alignof(char), buffer_size); \
    APPLY(interpreter_frame, stackpointer, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(interpreter_frame, tlbc_index, sizeof(int32_t), _Alignof(int32_t), buffer_size)

#define PY_REMOTE_DEBUG_CODE_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(code_object, qualname, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(code_object, filename, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(code_object, linetable, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(code_object, firstlineno, sizeof(int), _Alignof(int), buffer_size); \
    APPLY(code_object, co_code_adaptive, sizeof(char), _Alignof(char), buffer_size); \
    APPLY(code_object, co_tlbc, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size)

#define PY_REMOTE_DEBUG_SET_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(set_object, used, sizeof(Py_ssize_t), _Alignof(Py_ssize_t), buffer_size); \
    APPLY(set_object, mask, sizeof(Py_ssize_t), _Alignof(Py_ssize_t), buffer_size); \
    APPLY(set_object, table, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size)

#define PY_REMOTE_DEBUG_LONG_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(long_object, lv_tag, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(long_object, ob_digit, sizeof(digit), _Alignof(digit), buffer_size)

#define PY_REMOTE_DEBUG_BYTES_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(bytes_object, ob_size, sizeof(Py_ssize_t), _Alignof(Py_ssize_t), buffer_size); \
    APPLY(bytes_object, ob_sval, sizeof(char), _Alignof(char), buffer_size)

#define PY_REMOTE_DEBUG_UNICODE_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(unicode_object, length, sizeof(Py_ssize_t), _Alignof(Py_ssize_t), buffer_size); \
    APPLY(unicode_object, asciiobject_size, sizeof(char), _Alignof(char), buffer_size)

#define PY_REMOTE_DEBUG_GEN_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(gen_object, gi_frame_state, sizeof(int8_t), _Alignof(int8_t), buffer_size); \
    APPLY(gen_object, gi_iframe, FIELD_SIZE(PyGenObject, gi_iframe), _Alignof(_PyInterpreterFrame), buffer_size)

#define PY_REMOTE_DEBUG_TASK_OBJECT_FIELDS(APPLY, buffer_size) \
    APPLY(asyncio_task_object, task_name, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(asyncio_task_object, task_awaited_by, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(asyncio_task_object, task_is_task, sizeof(char), _Alignof(char), buffer_size); \
    APPLY(asyncio_task_object, task_awaited_by_is_set, sizeof(char), _Alignof(char), buffer_size); \
    APPLY(asyncio_task_object, task_coro, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(asyncio_task_object, task_node, SIZEOF_LLIST_NODE, _Alignof(struct llist_node), buffer_size)

#define PY_REMOTE_DEBUG_ASYNC_INTERPRETER_STATE_FIELDS(APPLY, buffer_size) \
    APPLY(asyncio_interpreter_state, asyncio_tasks_head, SIZEOF_LLIST_NODE, _Alignof(struct llist_node), buffer_size)

#define PY_REMOTE_DEBUG_ASYNC_THREAD_STATE_FIELDS(APPLY, buffer_size) \
    APPLY(asyncio_thread_state, asyncio_running_loop, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(asyncio_thread_state, asyncio_running_task, sizeof(uintptr_t), _Alignof(uintptr_t), buffer_size); \
    APPLY(asyncio_thread_state, asyncio_tasks_head, SIZEOF_LLIST_NODE, _Alignof(struct llist_node), buffer_size)

/* Called once after reading _Py_DebugOffsets, before any hot-path use. */
static inline int
_PyRemoteDebug_ValidateDebugOffsetsLayout(struct _Py_DebugOffsets *debug_offsets)
{
    /* Validate every field the unwinder dereferences against a local buffer
     * or local object view. Fields used only for remote address arithmetic
     * (e.g. runtime_state.interpreters_head) are also checked as a sanity
     * bound on the offset value. */
    PY_REMOTE_DEBUG_VALIDATE_SECTION(runtime_state);
    PY_REMOTE_DEBUG_RUNTIME_STATE_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        sizeof(_PyRuntimeState));

    PY_REMOTE_DEBUG_VALIDATE_SECTION(interpreter_state);
    PY_REMOTE_DEBUG_INTERPRETER_STATE_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        INTERP_STATE_BUFFER_SIZE);

    PY_REMOTE_DEBUG_VALIDATE_READ_SECTION(thread_state, SIZEOF_THREAD_STATE);
    PY_REMOTE_DEBUG_THREAD_STATE_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_THREAD_STATE);
    PY_REMOTE_DEBUG_VALIDATE_FIXED_FIELD(
        err_stackitem,
        exc_value,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        sizeof(_PyErr_StackItem));
    PY_REMOTE_DEBUG_VALIDATE_NESTED_FIELD(
        thread_state,
        exc_state,
        err_stackitem,
        exc_value,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        SIZEOF_THREAD_STATE);

    PY_REMOTE_DEBUG_VALIDATE_READ_SECTION(gc, SIZEOF_GC_RUNTIME_STATE);
    PY_REMOTE_DEBUG_VALIDATE_FIELD(
        gc,
        frame,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        SIZEOF_GC_RUNTIME_STATE);
    PY_REMOTE_DEBUG_VALIDATE_NESTED_FIELD(
        interpreter_state,
        gc,
        gc,
        frame,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        INTERP_STATE_BUFFER_SIZE);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(interpreter_frame);
    PY_REMOTE_DEBUG_INTERPRETER_FRAME_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_INTERP_FRAME);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(code_object);
    PY_REMOTE_DEBUG_CODE_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_CODE_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(pyobject);
    PY_REMOTE_DEBUG_VALIDATE_FIELD(
        pyobject,
        ob_type,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        SIZEOF_PYOBJECT);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(type_object);
    PY_REMOTE_DEBUG_VALIDATE_FIELD(
        type_object,
        tp_flags,
        sizeof(unsigned long),
        _Alignof(unsigned long),
        SIZEOF_TYPE_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(set_object);
    PY_REMOTE_DEBUG_SET_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_SET_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_READ_SECTION(long_object, SIZEOF_LONG_OBJ);
    PY_REMOTE_DEBUG_LONG_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_LONG_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(bytes_object);
    PY_REMOTE_DEBUG_BYTES_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_BYTES_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(unicode_object);
    PY_REMOTE_DEBUG_UNICODE_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_UNICODE_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_SECTION(gen_object);
    PY_REMOTE_DEBUG_GEN_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_GEN_OBJ);

    PY_REMOTE_DEBUG_VALIDATE_FIXED_FIELD(
        llist_node,
        next,
        sizeof(uintptr_t),
        _Alignof(uintptr_t),
        SIZEOF_LLIST_NODE);

    return 0;
}

/* Called once when AsyncioDebug is loaded, before any task inspection uses it. */
static inline int
_PyRemoteDebug_ValidateAsyncDebugOffsets(
    struct _Py_AsyncioModuleDebugOffsets *debug_offsets)
{
    PY_REMOTE_DEBUG_VALIDATE_READ_SECTION(asyncio_task_object, SIZEOF_TASK_OBJ);
    PY_REMOTE_DEBUG_TASK_OBJECT_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        SIZEOF_TASK_OBJ);
    PY_REMOTE_DEBUG_VALIDATE_SECTION(asyncio_interpreter_state);
    PY_REMOTE_DEBUG_ASYNC_INTERPRETER_STATE_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        sizeof(PyInterpreterState));
    PY_REMOTE_DEBUG_VALIDATE_SECTION(asyncio_thread_state);
    PY_REMOTE_DEBUG_ASYNC_THREAD_STATE_FIELDS(
        PY_REMOTE_DEBUG_VALIDATE_FIELD,
        sizeof(_PyThreadStateImpl));
    return 0;
}

#undef PY_REMOTE_DEBUG_VALIDATE_SECTION
#undef PY_REMOTE_DEBUG_VALIDATE_READ_SECTION
#undef PY_REMOTE_DEBUG_VALIDATE_FIELD
#undef PY_REMOTE_DEBUG_VALIDATE_NESTED_FIELD
#undef PY_REMOTE_DEBUG_VALIDATE_FIXED_FIELD
#undef PY_REMOTE_DEBUG_RUNTIME_STATE_FIELDS
#undef PY_REMOTE_DEBUG_THREAD_STATE_FIELDS
#undef PY_REMOTE_DEBUG_INTERPRETER_STATE_FIELDS
#undef PY_REMOTE_DEBUG_INTERPRETER_FRAME_FIELDS
#undef PY_REMOTE_DEBUG_CODE_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_SET_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_LONG_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_BYTES_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_UNICODE_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_GEN_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_TASK_OBJECT_FIELDS
#undef PY_REMOTE_DEBUG_ASYNC_INTERPRETER_STATE_FIELDS
#undef PY_REMOTE_DEBUG_ASYNC_THREAD_STATE_FIELDS
#undef FIELD_SIZE

#endif
