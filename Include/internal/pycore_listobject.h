#ifndef Py_INTERNAL_LISTOBJECT_H
#define Py_INTERNAL_LISTOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

#include "listobject.h"

PyAPI_FUNC(PyObject *) _PyList_ConvertToTuple(PyObject *);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LISTOBJECT_H */
