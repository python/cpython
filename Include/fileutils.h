#ifndef Py_FILEUTILS_H
#define Py_FILEUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(wchar_t *) _Py_char2wchar(
    char *arg);

PyAPI_FUNC(char*) _Py_wchar2char(
    const wchar_t *text);

#if defined(MS_WINDOWS) || defined(HAVE_STAT)
PyAPI_FUNC(int) _Py_wstat(
    const wchar_t* path,
    struct stat *buf);
#endif

#ifdef HAVE_STAT
PyAPI_FUNC(int) _Py_stat(
    PyObject *unicode,
    struct stat *statbuf);
#endif

PyAPI_FUNC(FILE *) _Py_wfopen(
    const wchar_t *path,
    const wchar_t *mode);

PyAPI_FUNC(FILE*) _Py_fopen(
    PyObject *unicode,
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
    wchar_t *resolved_path);
#endif

PyAPI_FUNC(wchar_t*) _Py_wgetcwd(
    wchar_t *buf,
    size_t size);

#ifdef __cplusplus
}
#endif

#endif /* !Py_FILEUTILS_H */
