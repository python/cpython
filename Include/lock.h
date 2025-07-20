#ifndef Py_LOCK_H
#define Py_LOCK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_LOCK_H
#  include "cpython/lock.h"
#  undef Py_CPYTHON_LOCK_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_LOCK_H */
