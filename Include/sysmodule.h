#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) PySys_GetObject(const char *);
PyAPI_FUNC(int) PySys_SetObject(const char *, PyObject *);

Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_SetArgv(int, wchar_t **);
Py_DEPRECATED(3.11) PyAPI_FUNC(void) PySys_SetArgvEx(int, wchar_t **, int);

PyAPI_FUNC(void) PySys_WriteStdout(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_WriteStderr(const char *format, ...)
                 Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(void) PySys_FormatStdout(const char *format, ...);
PyAPI_FUNC(void) PySys_FormatStderr(const char *format, ...);

Py_DEPRECATED(3.13) PyAPI_FUNC(void) PySys_ResetWarnOptions(void);

PyAPI_FUNC(PyObject *) PySys_GetXOptions(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
