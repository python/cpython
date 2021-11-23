#ifndef Py_INTERNAL_GLOBAL_OBJECTS_H
#define Py_INTERNAL_GLOBAL_OBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_runtime.h"       // _PyRuntimeState


/**************************************
 * lifecycle
 **************************************/

extern PyStatus _PyRuntimeState_TypesInit(_PyRuntimeState *runtime);
extern void _PyRuntimeState_TypesFini(_PyRuntimeState *runtime);

extern PyStatus _PyInterpreterState_CoreObjectsInit(PyInterpreterState *interp);
extern void _PyInterpreterState_CoreObjectsFini(PyInterpreterState *interp);

extern PyStatus _PyInterpreterState_ObjectsInit(PyInterpreterState *interp);
extern void _PyInterpreterState_ObjectsFini(PyInterpreterState *interp);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GLOBAL_OBJECTS_H */
