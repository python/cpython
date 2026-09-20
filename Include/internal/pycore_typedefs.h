#ifndef Py_INTERNAL_TYPEDEFS_H
#define Py_INTERNAL_TYPEDEFS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PyInterpreterFrame _PyInterpreterFrame;
typedef struct pyruntimestate _PyRuntimeState;

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_TYPEDEFS_H
