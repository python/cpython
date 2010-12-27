#ifndef Py_WARNINGS_H
#define Py_WARNINGS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject*) _PyWarnings_Init(void);
#endif

PyAPI_FUNC(int) PyErr_WarnEx(
    PyObject *category,
    const char *message,        /* UTF-8 encoded string */
    Py_ssize_t stack_level);
PyAPI_FUNC(int) PyErr_WarnFormat(
    PyObject *category,
    Py_ssize_t stack_level,
    const char *format,         /* ASCII-encoded string  */
    ...);
PyAPI_FUNC(int) PyErr_WarnExplicit(
    PyObject *category,
    const char *message,        /* UTF-8 encoded string */
    const char *filename,       /* UTF-8 encoded string */
    int lineno,
    const char *module,         /* UTF-8 encoded string */
    PyObject *registry);

/* DEPRECATED: Use PyErr_WarnEx() instead. */
#ifndef Py_LIMITED_API
#define PyErr_Warn(category, msg) PyErr_WarnEx(category, msg, 1)
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_WARNINGS_H */

