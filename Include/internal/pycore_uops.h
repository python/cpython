#ifndef Py_INTERNAL_UOPS_H
#define Py_INTERNAL_UOPS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_frame.h"         // _PyInterpreterFrame

#define _Py_UOP_MAX_TRACE_LENGTH 512

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UOPS_H */
