#include "Python.h"

#include "pycore_ceval.h"
#include "pycore_jit_publish.h"
#include "pycore_jit_unwind.h"

#ifdef _Py_JIT

#if defined(PY_HAVE_JIT_GDB_UNWIND) \
    || defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
struct _PyJitCodeRegistration {
    void *gdb_handle;
    void *gnu_backtrace_handle;
};
#endif

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

#if defined(PY_HAVE_JIT_GDB_UNWIND)
static void
jit_register_gdb_code(_PyJitCodeRegistration *registration,
                      const void *code_addr, size_t code_size,
                      const char *entry, const char *filename)
{
    registration->gdb_handle = _PyJitUnwind_GdbRegisterCode(
        code_addr, code_size, entry, filename);
}

static void
jit_unregister_gdb_code(_PyJitCodeRegistration *registration)
{
    if (registration->gdb_handle != NULL) {
        _PyJitUnwind_GdbUnregisterCode(registration->gdb_handle);
        registration->gdb_handle = NULL;
    }
}
#endif

#if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
static void
jit_register_gnu_backtrace_code(_PyJitCodeRegistration *registration,
                                const void *code_addr, size_t code_size)
{
    registration->gnu_backtrace_handle =
        _PyJitUnwind_GnuBacktraceRegisterCode(code_addr, code_size);
}

static void
jit_unregister_gnu_backtrace_code(_PyJitCodeRegistration *registration)
{
    if (registration->gnu_backtrace_handle != NULL) {
        _PyJitUnwind_GnuBacktraceUnregisterCode(
            registration->gnu_backtrace_handle);
        registration->gnu_backtrace_handle = NULL;
    }
}
#endif

_PyJitCodeRegistration *
_PyJit_RegisterCode(const void *code_addr, size_t code_size,
                    const char *entry, const char *filename)
{
    jit_register_perf_code(code_addr, code_size, entry, filename);
    // Perf publication has no teardown handle, so it is intentionally
    // not counted below.

#if !defined(PY_HAVE_JIT_GDB_UNWIND) \
    && !defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    return NULL;
#else
    _PyJitCodeRegistration *registration = PyMem_RawCalloc(
        1, sizeof(*registration));
    if (registration == NULL) {
        return NULL;
    }

    // Partial failures are non-fatal: the JIT code can still execute, but
    // unavailable tooling may not be able to unwind it.
    int any_registered = 0;
#  if defined(PY_HAVE_JIT_GDB_UNWIND)
    jit_register_gdb_code(
        registration, code_addr, code_size, entry, filename);
    any_registered |= registration->gdb_handle != NULL;
#  endif
#  if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    jit_register_gnu_backtrace_code(
        registration, code_addr, code_size);
    any_registered |= registration->gnu_backtrace_handle != NULL;
#  endif
    if (!any_registered) {
        PyMem_RawFree(registration);
        return NULL;
    }
    return registration;
#endif
}

void
_PyJit_UnregisterCode(_PyJitCodeRegistration *registration)
{
#if !defined(PY_HAVE_JIT_GDB_UNWIND) \
    && !defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    assert(registration == NULL);
    (void)registration;
#else
    if (registration == NULL) {
        return;
    }

#  if defined(PY_HAVE_JIT_GNU_BACKTRACE_UNWIND)
    jit_unregister_gnu_backtrace_code(registration);
#  endif
#  if defined(PY_HAVE_JIT_GDB_UNWIND)
    jit_unregister_gdb_code(registration);
#  endif
    PyMem_RawFree(registration);
#endif
}

#endif  // _Py_JIT
