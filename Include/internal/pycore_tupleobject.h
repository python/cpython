#ifndef Py_INTERNAL_TUPLEOBJECT_H
#define Py_INTERNAL_TUPLEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

#include "tupleobject.h"

#define _PyTuple_ITEMS(op) (_PyTuple_CAST(op)->ob_item)

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLEOBJECT_H */
