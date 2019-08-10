#ifndef Py_INTERNAL_LONGOBJECT_H
#define Py_INTERNAL_LONGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "longobject.h"

PyAPI_FUNC(PyObject *) _PyLong_FromUnsignedChar(unsigned char);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_LONGOBJECT_H */
