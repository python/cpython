#ifndef Py_INTERNAL_UNICODEOBJECT_H
#define Py_INTERNAL_UNICODEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_runtime.h"     // _PyRuntimeState
#include "pycore_initconfig.h"  // PyStatus

extern PyStatus _PyUnicode_InitRuntimeState(_PyRuntimeState *runtime);

extern PyStatus _PyUnicode_InitCoreObjects(PyInterpreterState *interp);
extern void _PyUnicode_FiniCoreObjects(PyInterpreterState *interp);

extern PyStatus _PyUnicode_InitTypes(PyInterpreterState *interp);
extern void _PyUnicode_FiniObjects(PyInterpreterState *interp);
extern void _PyUnicode_FiniState(PyInterpreterState *interp);

extern void _PyUnicode_ClearInterned(PyInterpreterState *interp);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEOBJECT_H */
