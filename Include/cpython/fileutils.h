#ifndef Py_CPYTHON_FILEUTILS_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(FILE*) Py_fopen(
    PyObject *path,
    const char *mode);

// Deprecated alias kept for backward compatibility
Py_DEPRECATED(3.14) static inline FILE*
_Py_fopen_obj(PyObject *path, const char *mode)
{
    return Py_fopen(path, mode);
}

PyAPI_FUNC(int) Py_fclose(FILE *file);
