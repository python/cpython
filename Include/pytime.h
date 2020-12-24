#ifndef Py_PYTIME_H
#define Py_PYTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API

#include "pyconfig.h" /* include for defines */
#include "object.h"

#  define Py_CPYTHON_PYTIME_H
#  include  "cpython/pytime.h"
#  undef Py_CPYTHON_PYTIME_H
#endif /* Py_LIMITED_API */

#ifdef __cplusplus
}
#endif

#endif /* Py_PYTIME_H */
