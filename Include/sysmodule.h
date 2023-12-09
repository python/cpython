
/* System module interface */

#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) PySys_GetObject(const char *);
PyAPI_FUNC(int) PySys_SetObject(const char *, PyObject *);

Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_SetArgv(int, wchar_t **);
Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_SetArgvEx(int, wchar_t **, int);
Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_SetPath(const wchar_t *);

PyAPI_FUNC(void) PySys_WriteStdout(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_WriteStderr(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_FormatStdout(const char *format, ...);
PyAPI_FUNC(void) PySys_FormatStderr(const char *format, ...);

PyAPI_FUNC(void) PySys_ResetWarnOptions(void);
Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_AddWarnOption(const wchar_t *);
Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_AddWarnOptionUnicode(PyObject *);
Py_DEPRECATED(3.11) PyAPI_FUNC(int) PySys_HasWarnOptions(void);

Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_AddXOption(const wchar_t *);
PyAPI_FUNC(PyObject *) PySys_GetXOptions(void);

#if !defined(Py_LIMITED_API)
typedef struct {
    FILE* perf_map;
    PyThread_type_lock map_lock;
} PerfMapState;

PyAPI_FUNC(int) PyUnstable_PerfMapState_Init(void);

PyAPI_FUNC(int) PyUnstable_WritePerfMapEntry(const void *code_addr, unsigned int code_size, const char *entry_name);

PyAPI_FUNC(void) PyUnstable_PerfMapState_Fini(void);
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
