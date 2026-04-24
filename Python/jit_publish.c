#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_jit_publish.h"
#include "pycore_jit_unwind.h"

#ifdef _Py_JIT

struct _PyJitCodeRegistration {
    int perf_published;
    void *gdb_handle;
    void *gnu_backtrace_handle;
};

static int
jit_register_perf_code(_PyJitCodeRegistration *registration,
                       const void *code_addr, size_t code_size,
                       const char *entry, const char *filename)
{
#ifdef PY_HAVE_PERF_TRAMPOLINE
    _PyPerf_Callbacks callbacks;
    _PyPerfTrampoline_GetCallbacks(&callbacks);
    if (callbacks.write_state == _Py_perfmap_jit_callbacks.write_state) {
        _PyPerfJit_WriteNamedCode(
            code_addr, code_size, entry, filename);
        registration->perf_published = 1;
        return 1;
    }
#else
    (void)registration;
    (void)code_addr;
    (void)code_size;
    (void)entry;
    (void)filename;
#endif
    return 0;
}

static void
jit_unregister_perf_code(_PyJitCodeRegistration *registration)
{
    if (registration == NULL) {
        return;
    }
    registration->perf_published = 0;
}

static int
jit_register_gdb_code(_PyJitCodeRegistration *registration,
                      const void *code_addr, size_t code_size,
                      const char *entry, const char *filename)
{
#if defined(__linux__) && defined(__ELF__)
    registration->gdb_handle = _PyJitUnwind_GdbRegisterCode(
        code_addr, code_size, entry, filename);
    if (registration->gdb_handle != NULL) {
        return 1;
    }
#else
    (void)registration;
    (void)code_addr;
    (void)code_size;
    (void)entry;
    (void)filename;
#endif
    return 0;
}

static void
jit_unregister_gdb_code(_PyJitCodeRegistration *registration)
{
#if defined(__linux__) && defined(__ELF__)
    if (registration != NULL && registration->gdb_handle != NULL) {
        _PyJitUnwind_GdbUnregisterCode(registration->gdb_handle);
        registration->gdb_handle = NULL;
    }
#else
    (void)registration;
#endif
}

static int
jit_register_gnu_backtrace_code(_PyJitCodeRegistration *registration,
                                const void *code_addr, size_t code_size)
{
#if defined(__linux__) && defined(__ELF__)
    registration->gnu_backtrace_handle =
        _PyJitUnwind_GnuBacktraceRegisterCode(code_addr, code_size);
    if (registration->gnu_backtrace_handle != NULL) {
        return 1;
    }
#else
    (void)registration;
    (void)code_addr;
    (void)code_size;
#endif
    return 0;
}

static void
jit_unregister_gnu_backtrace_code(_PyJitCodeRegistration *registration)
{
#if defined(__linux__) && defined(__ELF__)
    if (registration != NULL && registration->gnu_backtrace_handle != NULL) {
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
    _PyJitCodeRegistration *registration = PyMem_RawCalloc(1, sizeof(*registration));
    if (registration == NULL) {
        return NULL;
    }

    jit_register_perf_code(
        registration, code_addr, code_size, entry, filename);
    jit_register_gdb_code(
        registration, code_addr, code_size, entry, filename);
    jit_register_gnu_backtrace_code(
        registration, code_addr, code_size);
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
    jit_unregister_perf_code(registration);
    PyMem_RawFree(registration);
}

#endif  // _Py_JIT
