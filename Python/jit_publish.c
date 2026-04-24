#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_jit_publish.h"
#include "pycore_jit_unwind.h"

#ifdef _Py_JIT

struct _PyJitCodeRegistration {
    void *gdb_handle;
    void *gnu_backtrace_handle;
};

static void
jit_register_perf_code(const void *code_addr, size_t code_size,
                       const char *entry, const char *filename)
{
#ifdef PY_HAVE_PERF_TRAMPOLINE
    _PyPerf_Callbacks callbacks;
    _PyPerfTrampoline_GetCallbacks(&callbacks);
    if (callbacks.write_state == _Py_perfmap_jit_callbacks.write_state) {
        _PyPerfJit_WriteNamedCode(
            code_addr, code_size, entry, filename);
    }
#else
    (void)code_addr;
    (void)code_size;
    (void)entry;
    (void)filename;
#endif
}

static void
jit_register_gdb_code(_PyJitCodeRegistration *registration,
                      const void *code_addr, size_t code_size,
                      const char *entry, const char *filename)
{
#if defined(PY_HAVE_JIT_GDB_UNWIND)
    registration->gdb_handle = _PyJitUnwind_GdbRegisterCode(
        code_addr, code_size, entry, filename);
#else
    (void)registration;
    (void)code_addr;
    (void)code_size;
    (void)entry;
    (void)filename;
#endif
}

static void
jit_unregister_gdb_code(_PyJitCodeRegistration *registration)
{
#if defined(PY_HAVE_JIT_GDB_UNWIND)
    if (registration->gdb_handle != NULL) {
        _PyJitUnwind_GdbUnregisterCode(registration->gdb_handle);
        registration->gdb_handle = NULL;
    }
#else
    (void)registration;
#endif
}

static void
jit_register_gnu_backtrace_code(_PyJitCodeRegistration *registration,
                                const void *code_addr, size_t code_size)
{
#if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    registration->gnu_backtrace_handle =
        _PyJitUnwind_GnuBacktraceRegisterCode(code_addr, code_size);
#else
    (void)registration;
    (void)code_addr;
    (void)code_size;
#endif
}

static void
jit_unregister_gnu_backtrace_code(_PyJitCodeRegistration *registration)
{
#if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    if (registration->gnu_backtrace_handle != NULL) {
        _PyJitUnwind_GnuBacktraceUnregisterCode(
            registration->gnu_backtrace_handle);
        registration->gnu_backtrace_handle = NULL;
    }
#else
    (void)registration;
#endif
}

_PyJitCodeRegistration *
_PyJit_RegisterCode(const void *code_addr, size_t code_size,
                    const char *entry, const char *filename)
{
    jit_register_perf_code(code_addr, code_size, entry, filename);

    _PyJitCodeRegistration *registration = PyMem_RawCalloc(
        1, sizeof(*registration));
    if (registration == NULL) {
        return NULL;
    }

    jit_register_gdb_code(
        registration, code_addr, code_size, entry, filename);
    jit_register_gnu_backtrace_code(
        registration, code_addr, code_size);
    if (registration->gdb_handle == NULL &&
        registration->gnu_backtrace_handle == NULL)
    {
        PyMem_RawFree(registration);
        return NULL;
    }
    return registration;
}

void
_PyJit_UnregisterCode(_PyJitCodeRegistration *registration)
{
    if (registration == NULL) {
        return;
    }

    jit_unregister_gnu_backtrace_code(registration);
    jit_unregister_gdb_code(registration);
    PyMem_RawFree(registration);
}

#endif  // _Py_JIT
