#ifndef Py_INTERNAL_FILEUTILS_H
#define Py_INTERNAL_FILEUTILS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "Py_BUILD_CORE must be defined to include this header"
#endif

#include <locale.h>   /* struct lconv */

// This is used after getting NULL back from Py_DecodeLocale().
#define DECODE_LOCALE_ERR(NAME, LEN) \
    ((LEN) == (size_t)-2) \
     ? _PyStatus_ERR("cannot decode " NAME) \
     : _PyStatus_NO_MEMORY()

PyAPI_DATA(int) _Py_HasFileSystemDefaultEncodeErrors;

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
    Py_ssize_t arglen,
    size_t *wlen);

extern int
_Py_wstat(const wchar_t *, struct stat *);

PyAPI_FUNC(int) _Py_GetForceASCII(void);

/* Reset "force ASCII" mode (if it was initialized).

   This function should be called when Python changes the LC_CTYPE locale,
   so the "force ASCII" mode can be detected again on the new locale
   encoding. */
PyAPI_FUNC(void) _Py_ResetForceASCII(void);


PyAPI_FUNC(int) _Py_GetLocaleconvNumeric(
    struct lconv *lc,
    PyObject **decimal_point,
    PyObject **thousands_sep);

PyAPI_FUNC(void) _Py_closerange(int first, int last);

PyAPI_FUNC(wchar_t*) _Py_GetLocaleEncoding(void);
PyAPI_FUNC(PyObject*) _Py_GetLocaleEncodingObject(void);

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
extern int _Py_LocaleUsesNonUnicodeWchar(void);

extern wchar_t* _Py_DecodeNonUnicodeWchar(
    const wchar_t* native,
    Py_ssize_t size);

extern int _Py_EncodeNonUnicodeWchar_InPlace(
    wchar_t* unicode,
    Py_ssize_t size);
#endif

extern wchar_t * _Py_join_relfile(const wchar_t *dirname,
                                  const wchar_t *relfile);
extern int _Py_add_relfile(wchar_t *dirname,
                           const wchar_t *relfile,
                           size_t bufsize);
extern size_t _Py_find_basename(const wchar_t *filename);
PyAPI_FUNC(int) _Py_normalize_path(const wchar_t *path,
                                   wchar_t *buf, const size_t buf_len);


// Macros to protect CRT calls against instant termination when passed an
// invalid parameter (bpo-23524). IPH stands for Invalid Parameter Handler.
// Usage:
//
//      _Py_BEGIN_SUPPRESS_IPH
//      ...
//      _Py_END_SUPPRESS_IPH
#if defined _MSC_VER && _MSC_VER >= 1900

#  include <stdlib.h>   // _set_thread_local_invalid_parameter_handler()

   extern _invalid_parameter_handler _Py_silent_invalid_parameter_handler;
#  define _Py_BEGIN_SUPPRESS_IPH \
    { _invalid_parameter_handler _Py_old_handler = \
      _set_thread_local_invalid_parameter_handler(_Py_silent_invalid_parameter_handler);
#  define _Py_END_SUPPRESS_IPH \
    _set_thread_local_invalid_parameter_handler(_Py_old_handler); }
#else
#  define _Py_BEGIN_SUPPRESS_IPH
#  define _Py_END_SUPPRESS_IPH
#endif /* _MSC_VER >= 1900 */

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FILEUTILS_H */
