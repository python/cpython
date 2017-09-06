#ifndef Py_INTERNAL_RUNTIME_H
#define Py_INTERNAL_RUNTIME_H
#ifdef __cplusplus
extern "C" {
#endif

#include "internal/pystate.h"

/* XXX Fold this back into internal/pystate.h? */
PyAPI_DATA(_PyRuntimeState) _PyRuntime;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_RUNTIME_H */
