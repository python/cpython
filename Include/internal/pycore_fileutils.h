#ifndef Py_INTERNAL_FILEUTILS_H
#define Py_INTERNAL_FILEUTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "Py_BUILD_CORE must be defined to include this header"
#endif

PyAPI_FUNC(int) _Py_DecodeUTF8Ex(
    const char *arg,
    Py_ssize_t arglen,
    wchar_t **wstr,
    size_t *wlen,
    const char **reason,
    _Py_error_handler errors);

PyAPI_FUNC(int) _Py_EncodeUTF8Ex(
    const wchar_t *text,
    char **str,
    size_t *error_pos,
    const char **reason,
    int raw_malloc,
    _Py_error_handler errors);

PyAPI_FUNC(wchar_t*) _Py_DecodeUTF8_surrogateescape(
    const char *arg,
    Py_ssize_t arglen);

PyAPI_FUNC(int) _Py_GetForceASCII(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FILEUTILS_H */
