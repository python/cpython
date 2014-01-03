#ifndef Py_FILEUTILS_H
#define Py_FILEUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) _Py_device_encoding(int);

PyAPI_FUNC(wchar_t *) _Py_char2wchar(
    const char *arg,
    size_t *size);

PyAPI_FUNC(char*) _Py_wchar2char(
    const wchar_t *text,
    size_t *error_pos);

#if defined(HAVE_STAT) && !defined(MS_WINDOWS)
PyAPI_FUNC(int) _Py_wstat(
    const wchar_t* path,
    struct stat *buf);
#endif

#ifdef HAVE_STAT
PyAPI_FUNC(int) _Py_stat(
    PyObject *path,
    struct stat *statbuf);
#endif

#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _Py_open(
    const char *pathname,
    int flags);
#endif

PyAPI_FUNC(FILE *) _Py_wfopen(
    const wchar_t *path,
    const wchar_t *mode);

PyAPI_FUNC(FILE*) _Py_fopen(
    const char *pathname,
    const char *mode);

PyAPI_FUNC(FILE*) _Py_fopen_obj(
    PyObject *path,
    const char *mode);

#ifdef HAVE_READLINK
PyAPI_FUNC(int) _Py_wreadlink(
    const wchar_t *path,
    wchar_t *buf,
    size_t bufsiz);
#endif

#ifdef HAVE_REALPATH
PyAPI_FUNC(wchar_t*) _Py_wrealpath(
    const wchar_t *path,
    wchar_t *resolved_path,
    size_t resolved_path_size);
#endif

PyAPI_FUNC(wchar_t*) _Py_wgetcwd(
    wchar_t *buf,
    size_t size);

#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _Py_get_inheritable(int fd);

PyAPI_FUNC(int) _Py_set_inheritable(int fd, int inheritable,
                                    int *atomic_flag_works);

PyAPI_FUNC(int) _Py_dup(int fd);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !Py_FILEUTILS_H */
