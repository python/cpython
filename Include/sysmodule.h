#ifndef Py_SYSMODULE_H
#define Py_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030f0000
PyAPI_FUNC(PyObject *) PySys_GetAttr(PyObject *);
PyAPI_FUNC(PyObject *) PySys_GetAttrString(const char *);
PyAPI_FUNC(int) PySys_GetOptionalAttr(PyObject *, PyObject **);
PyAPI_FUNC(int) PySys_GetOptionalAttrString(const char *, PyObject **);
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

PyAPI_FUNC(PyObject *) PySys_GetXOptions(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_SYSMODULE_H */
