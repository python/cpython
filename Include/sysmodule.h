#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) PySys_GetObject(const char *);
PyAPI_FUNC(int) PySys_SetObject(const char *, PyObject *);

PyAPI_FUNC(void) PySys_WriteStdout(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_WriteStderr(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_FormatStdout(const char *format, ...);
PyAPI_FUNC(void) PySys_FormatStderr(const char *format, ...);

Py_DEPRECATED(3.13) PyAPI_FUNC(void) PySys_ResetWarnOptions(void);

PyAPI_FUNC(PyObject *) PySys_GetXOptions(void);

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
#  define Py_CPYTHON_SYSMODULE_H
#  include "cpython/sysmodule.h"
#  undef Py_CPYTHON_SYSMODULE_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
