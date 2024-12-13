#ifndef Py_CPYTHON_FILEUTILS_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(FILE*) Py_fopen(
    PyObject *path,
    const char *mode);

PyAPI_FUNC(int) Py_fclose(FILE *file);
