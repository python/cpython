#ifndef Py_MONITORING_H
#define Py_MONITORING_H
#ifdef __cplusplus
extern "C" {
#endif

// There is currently no limited API for monitoring

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_MONITORING_H
#  include "cpython/monitoring.h"
#  undef Py_CPYTHON_MONITORING_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_MONITORING_H */
