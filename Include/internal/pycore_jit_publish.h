#ifndef Py_INTERNAL_JIT_PUBLISH_H
#define Py_INTERNAL_JIT_PUBLISH_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stddef.h>

typedef struct _PyJitCodeRegistration _PyJitCodeRegistration;

#ifdef _Py_JIT

_PyJitCodeRegistration *_PyJit_RegisterCode(const void *code_addr,
                                            size_t code_size,
                                            const char *entry,
                                            const char *filename);

void _PyJit_UnregisterCode(_PyJitCodeRegistration *registration);

#endif  // _Py_JIT

#endif  // Py_INTERNAL_JIT_PUBLISH_H
