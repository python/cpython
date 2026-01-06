#ifndef _Py_AUDIT_H
#define _Py_AUDIT_H
#ifdef __cplusplus
extern "C" {
#endif


#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(int) PySys_Audit(
    const char *event,
    const char *argFormat,
    ...);

PyAPI_FUNC(int) PySys_AuditTuple(
    const char *event,
    PyObject *args);
#endif


#ifndef Py_LIMITED_API
#  define _Py_CPYTHON_AUDIT_H
#  include "cpython/audit.h"
#  undef _Py_CPYTHON_AUDIT_H
#endif


#ifdef __cplusplus
}
#endif
#endif /* !_Py_AUDIT_H */
