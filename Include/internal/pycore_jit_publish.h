#ifndef Py_INTERNAL_JIT_PUBLISH_H
#define Py_INTERNAL_JIT_PUBLISH_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>

typedef struct _PyJitCodeRegistration _PyJitCodeRegistration;

#ifdef _Py_JIT

/* Publish JIT code to optional tooling backends.
 *
 * The return value is a backend-specific deregistration handle, not a
 * success/failure indicator. NULL means there is nothing to unregister later:
 * perf does not need a handle, and GDB/GNU backtrace registration failures
 * are intentionally non-fatal because tooling support must not make JIT
 * compilation fail.
 */
_PyJitCodeRegistration *_PyJit_RegisterCode(const void *code_addr,
                                            size_t code_size,
                                            const char *entry,
                                            const char *filename);

void _PyJit_UnregisterCode(_PyJitCodeRegistration *registration);

#endif  // _Py_JIT

#endif  // Py_INTERNAL_JIT_PUBLISH_H
