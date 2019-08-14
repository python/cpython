#ifndef Py_INTERNAL_THREAD_H
#define Py_INTERNAL_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/*
 * Find the size of the stack the system uses by default.
 */
PyAPI_FUNC(size_t) _PyThread_preferred_stacksize(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_THREAD_H */
