#include "Python.h"
#include "pycore_fileutils.h"     // fileutils definitions
#include "pycore_runtime.h"       // _PyRuntime
#include "osdefs.h"               // SEP
#include <locale.h>
#include <stdlib.h>               // mbstowcs()

#ifdef MS_WINDOWS
#  include <malloc.h>
#  include <windows.h>
#  include <pathcch.h>            // PathCchCombineEx
extern int winerror_to_errno(int);
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
#include <iconv.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef O_CLOEXEC
/* Does open() support the O_CLOEXEC flag? Possible values:

   -1: unknown
    0: open() ignores O_CLOEXEC flag, ex: Linux kernel older than 2.6.23
    1: open() supports O_CLOEXEC flag, close-on-exec is set

   The flag is used by _Py_open(), _Py_open_noraise(), io.FileIO
   and os.open(). */
int _Py_open_cloexec_works = -1;
#endif

// The value must be the same in unicodeobject.c.
#define MAX_UNICODE 0x10ffff

// mbstowcs() and mbrtowc() errors
static const size_t DECODE_ERROR = ((size_t)-1);
static const size_t INCOMPLETE_CHARACTER = (size_t)-2;


static int
get_surrogateescape(_Py_error_handler errors, int *surrogateescape)
{
    switch (errors)
    {
    case _Py_ERROR_STRICT:
        *surrogateescape = 0;
        return 0;
    case _Py_ERROR_SURROGATEESCAPE:
        *surrogateescape = 1;
        return 0;
    default:
        return -1;
    }
}


PyObject *
_Py_device_encoding(int fd)
{
    int valid;
    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    valid = isatty(fd);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    if (!valid)
        Py_RETURN_NONE;

#if defined(MS_WINDOWS)
    UINT cp;
    if (fd == 0)
        cp = GetConsoleCP();
    else if (fd == 1 || fd == 2)
        cp = GetConsoleOutputCP();
    else
        cp = 0;
    /* GetConsoleCP() and GetConsoleOutputCP() return 0 if the application
       has no console */
    if (cp == 0) {
        Py_RETURN_NONE;
    }

    return PyUnicode_FromFormat("cp%u", (unsigned int)cp);
#else
    if (_PyRuntime.preconfig.utf8_mode) {
        _Py_DECLARE_STR(utf_8, "utf-8");
        return Py_NewRef(&_Py_STR(utf_8));
    }
    return _Py_GetLocaleEncodingObject();
#endif
}


static size_t
is_valid_wide_char(wchar_t ch)
{
#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION
    /* Oracle Solaris doesn't use Unicode code points as wchar_t encoding
       for non-Unicode locales, which makes values higher than MAX_UNICODE
       possibly valid. */
    return 1;
#endif
    if (Py_UNICODE_IS_SURROGATE(ch)) {
        // Reject lone surrogate characters
        return 0;
    }
    if (ch > MAX_UNICODE) {
        // bpo-35883: Reject characters outside [U+0000; U+10ffff] range.
        // The glibc mbstowcs() UTF-8 decoder does not respect the RFC 3629,
        // it creates characters outside the [U+0000; U+10ffff] range:
        // https://sourceware.org/bugzilla/show_bug.cgi?id=2373
        return 0;
    }
    return 1;
}


static size_t
_Py_mbstowcs(wchar_t *dest, const char *src, size_t n)
{
    size_t count = mbstowcs(dest, src, n);
    if (dest != NULL && count != DECODE_ERROR) {
        for (size_t i=0; i < count; i++) {
            wchar_t ch = dest[i];
            if (!is_valid_wide_char(ch)) {
                return DECODE_ERROR;
            }
        }
    }
    return count;
}


#ifdef HAVE_MBRTOWC
static size_t
_Py_mbrtowc(wchar_t *pwc, const char *str, size_t len, mbstate_t *pmbs)
{
    assert(pwc != NULL);
    size_t count = mbrtowc(pwc, str, len, pmbs);
    if (count != 0 && count != DECODE_ERROR && count != INCOMPLETE_CHARACTER) {
        if (!is_valid_wide_char(*pwc)) {
            return DECODE_ERROR;
        }
    }
    return count;
}
#endif


#if !defined(_Py_FORCE_UTF8_FS_ENCODING) && !defined(MS_WINDOWS)

#define USE_FORCE_ASCII

extern int _Py_normalize_encoding(const char *, char *, size_t);

/* Workaround FreeBSD and OpenIndiana locale encoding issue with the C locale
   and POSIX locale. nl_langinfo(CODESET) announces an alias of the
   ASCII encoding, whereas mbstowcs() and wcstombs() functions use the
   ISO-8859-1 encoding. The problem is that os.fsencode() and os.fsdecode() use
   locale.getpreferredencoding() codec. For example, if command line arguments
   are decoded by mbstowcs() and encoded back by os.fsencode(), we get a
   UnicodeEncodeError instead of retrieving the original byte string.

   The workaround is enabled if setlocale(LC_CTYPE, NULL) returns "C",
   nl_langinfo(CODESET) announces "ascii" (or an alias to ASCII), and at least
   one byte in range 0x80-0xff can be decoded from the locale encoding. The
   workaround is also enabled on error, for example if getting the locale
   failed.

   On HP-UX with the C locale or the POSIX locale, nl_langinfo(CODESET)
   announces "roman8" but mbstowcs() uses Latin1 in practice. Force also the
   ASCII encoding in this case.

   Values of force_ascii:

       1: the workaround is used: Py_EncodeLocale() uses
          encode_ascii_surrogateescape() and Py_DecodeLocale() uses
          decode_ascii()
       0: the workaround is not used: Py_EncodeLocale() uses wcstombs() and
          Py_DecodeLocale() uses mbstowcs()
      -1: unknown, need to call check_force_ascii() to get the value
*/
static int force_ascii = -1;

static int
check_force_ascii(void)
{
    char *loc = setlocale(LC_CTYPE, NULL);
    if (loc == NULL) {
        goto error;
    }
    if (strcmp(loc, "C") != 0 && strcmp(loc, "POSIX") != 0) {
        /* the LC_CTYPE locale is different than C and POSIX */
        return 0;
    }

#if defined(HAVE_LANGINFO_H) && defined(CODESET)
    const char *codeset = nl_langinfo(CODESET);
    if (!codeset || codeset[0] == '\0') {
        /* CODESET is not set or empty */
        goto error;
    }

    char encoding[20];   /* longest name: "iso_646.irv_1991\0" */
    if (!_Py_normalize_encoding(codeset, encoding, sizeof(encoding))) {
        goto error;
    }

#ifdef __hpux
    if (strcmp(encoding, "roman8") == 0) {
        unsigned char ch;
        wchar_t wch;
        size_t res;

        ch = (unsigned char)0xA7;
        res = _Py_mbstowcs(&wch, (char*)&ch, 1);
        if (res != DECODE_ERROR && wch == L'\xA7') {
            /* On HP-UX with C locale or the POSIX locale,
               nl_langinfo(CODESET) announces "roman8", whereas mbstowcs() uses
               Latin1 encoding in practice. Force ASCII in this case.

               Roman8 decodes 0xA7 to U+00CF. Latin1 decodes 0xA7 to U+00A7. */
            return 1;
        }
    }
#else
    const char* ascii_aliases[] = {
        "ascii",
        /* Aliases from Lib/encodings/aliases.py */
        "646",
        "ansi_x3.4_1968",
        "ansi_x3.4_1986",
        "ansi_x3_4_1968",
        "cp367",
        "csascii",
        "ibm367",
        "iso646_us",
        "iso_646.irv_1991",
        "iso_ir_6",
        "us",
        "us_ascii",
        NULL
    };

    int is_ascii = 0;
    for (const char **alias=ascii_aliases; *alias != NULL; alias++) {
        if (strcmp(encoding, *alias) == 0) {
            is_ascii = 1;
            break;
        }
    }
    if (!is_ascii) {
        /* nl_langinfo(CODESET) is not "ascii" or an alias of ASCII */
        return 0;
    }

    for (unsigned int i=0x80; i<=0xff; i++) {
        char ch[1];
        wchar_t wch[1];
        size_t res;

        unsigned uch = (unsigned char)i;
        ch[0] = (char)uch;
        res = _Py_mbstowcs(wch, ch, 1);
        if (res != DECODE_ERROR) {
            /* decoding a non-ASCII character from the locale encoding succeed:
               the locale encoding is not ASCII, force ASCII */
            return 1;
        }
    }
    /* None of the bytes in the range 0x80-0xff can be decoded from the locale
       encoding: the locale encoding is really ASCII */
#endif   /* !defined(__hpux) */
    return 0;
#else
    /* nl_langinfo(CODESET) is not available: always force ASCII */
    return 1;
#endif   /* defined(HAVE_LANGINFO_H) && defined(CODESET) */

error:
    /* if an error occurred, force the ASCII encoding */
    return 1;
}


int
_Py_GetForceASCII(void)
{
    if (force_ascii == -1) {
        force_ascii = check_force_ascii();
    }
    return force_ascii;
}


void
_Py_ResetForceASCII(void)
{
    force_ascii = -1;
}


static int
encode_ascii(const wchar_t *text, char **str,
             size_t *error_pos, const char **reason,
             int raw_malloc, _Py_error_handler errors)
{
    char *result = NULL, *out;
    size_t len, i;
    wchar_t ch;

    int surrogateescape;
    if (get_surrogateescape(errors, &surrogateescape) < 0) {
        return -3;
    }

    len = wcslen(text);

    /* +1 for NULL byte */
    if (raw_malloc) {
        result = PyMem_RawMalloc(len + 1);
    }
    else {
        result = PyMem_Malloc(len + 1);
    }
    if (result == NULL) {
        return -1;
    }

    out = result;
    for (i=0; i<len; i++) {
        ch = text[i];

        if (ch <= 0x7f) {
            /* ASCII character */
            *out++ = (char)ch;
        }
        else if (surrogateescape && 0xdc80 <= ch && ch <= 0xdcff) {
            /* UTF-8b surrogate */
            *out++ = (char)(ch - 0xdc00);
        }
        else {
            if (raw_malloc) {
                PyMem_RawFree(result);
            }
            else {
                PyMem_Free(result);
            }
            if (error_pos != NULL) {
                *error_pos = i;
            }
            if (reason) {
                *reason = "encoding error";
            }
            return -2;
        }
    }
    *out = '\0';
    *str = result;
    return 0;
}
#else
int
_Py_GetForceASCII(void)
{
    return 0;
}

void
_Py_ResetForceASCII(void)
{
    /* nothing to do */
}
#endif   /* !defined(_Py_FORCE_UTF8_FS_ENCODING) && !defined(MS_WINDOWS) */


#if !defined(HAVE_MBRTOWC) || defined(USE_FORCE_ASCII)
static int
decode_ascii(const char *arg, wchar_t **wstr, size_t *wlen,
             const char **reason, _Py_error_handler errors)
{
    wchar_t *res;
    unsigned char *in;
    wchar_t *out;
    size_t argsize = strlen(arg) + 1;

    int surrogateescape;
    if (get_surrogateescape(errors, &surrogateescape) < 0) {
        return -3;
    }

    if (argsize > PY_SSIZE_T_MAX / sizeof(wchar_t)) {
        return -1;
    }
    res = PyMem_RawMalloc(argsize * sizeof(wchar_t));
    if (!res) {
        return -1;
    }

    out = res;
    for (in = (unsigned char*)arg; *in; in++) {
        unsigned char ch = *in;
        if (ch < 128) {
            *out++ = ch;
        }
        else {
            if (!surrogateescape) {
                PyMem_RawFree(res);
                if (wlen) {
                    *wlen = in - (unsigned char*)arg;
                }
                if (reason) {
                    *reason = "decoding error";
                }
                return -2;
            }
            *out++ = 0xdc00 + ch;
        }
    }
    *out = 0;

    if (wlen != NULL) {
        *wlen = out - res;
    }
    *wstr = res;
    return 0;
}
#endif   /* !HAVE_MBRTOWC */

static int
decode_current_locale(const char* arg, wchar_t **wstr, size_t *wlen,
                      const char **reason, _Py_error_handler errors)
{
    wchar_t *res;
    size_t argsize;
    size_t count;
#ifdef HAVE_MBRTOWC
    unsigned char *in;
    wchar_t *out;
    mbstate_t mbs;
#endif

    int surrogateescape;
    if (get_surrogateescape(errors, &surrogateescape) < 0) {
        return -3;
    }

#ifdef HAVE_BROKEN_MBSTOWCS
    /* Some platforms have a broken implementation of
     * mbstowcs which does not count the characters that
     * would result from conversion.  Use an upper bound.
     */
    argsize = strlen(arg);
#else
    argsize = _Py_mbstowcs(NULL, arg, 0);
#endif
    if (argsize != DECODE_ERROR) {
        if (argsize > PY_SSIZE_T_MAX / sizeof(wchar_t) - 1) {
            return -1;
        }
        res = (wchar_t *)PyMem_RawMalloc((argsize + 1) * sizeof(wchar_t));
        if (!res) {
            return -1;
        }

        count = _Py_mbstowcs(res, arg, argsize + 1);
        if (count != DECODE_ERROR) {
            *wstr = res;
            if (wlen != NULL) {
                *wlen = count;
            }
            return 0;
        }
        PyMem_RawFree(res);
    }

    /* Conversion failed. Fall back to escaping with surrogateescape. */
#ifdef HAVE_MBRTOWC
    /* Try conversion with mbrtwoc (C99), and escape non-decodable bytes. */

    /* Overallocate; as multi-byte characters are in the argument, the
       actual output could use less memory. */
    argsize = strlen(arg) + 1;
    if (argsize > PY_SSIZE_T_MAX / sizeof(wchar_t)) {
        return -1;
    }
    res = (wchar_t*)PyMem_RawMalloc(argsize * sizeof(wchar_t));
    if (!res) {
        return -1;
    }

    in = (unsigned char*)arg;
    out = res;
    memset(&mbs, 0, sizeof mbs);
    while (argsize) {
        size_t converted = _Py_mbrtowc(out, (char*)in, argsize, &mbs);
        if (converted == 0) {
            /* Reached end of string; null char stored. */
            break;
        }

        if (converted == INCOMPLETE_CHARACTER) {
            /* Incomplete character. This should never happen,
               since we provide everything that we have -
               unless there is a bug in the C library, or I
               misunderstood how mbrtowc works. */
            goto decode_error;
        }

        if (converted == DECODE_ERROR) {
            if (!surrogateescape) {
                goto decode_error;
            }

            /* Decoding error. Escape as UTF-8b, and start over in the initial
               shift state. */
            *out++ = 0xdc00 + *in++;
            argsize--;
            memset(&mbs, 0, sizeof mbs);
            continue;
        }

        // _Py_mbrtowc() reject lone surrogate characters
        assert(!Py_UNICODE_IS_SURROGATE(*out));

        /* successfully converted some bytes */
        in += converted;
        argsize -= converted;
        out++;
    }
    if (wlen != NULL) {
        *wlen = out - res;
    }
    *wstr = res;
    return 0;

decode_error:
    PyMem_RawFree(res);
    if (wlen) {
        *wlen = in - (unsigned char*)arg;
    }
    if (reason) {
        *reason = "decoding error";
    }
    return -2;
#else   /* HAVE_MBRTOWC */
    /* Cannot use C locale for escaping; manually escape as if charset
       is ASCII (i.e. escape all bytes > 128. This will still roundtrip
       correctly in the locale's charset, which must be an ASCII superset. */
    return decode_ascii(arg, wstr, wlen, reason, errors);
#endif   /* HAVE_MBRTOWC */
}


/* Decode a byte string from the locale encoding.

   Use the strict error handler if 'surrogateescape' is zero.  Use the
   surrogateescape error handler if 'surrogateescape' is non-zero: undecodable
   bytes are decoded as characters in range U+DC80..U+DCFF. If a byte sequence
   can be decoded as a surrogate character, escape the bytes using the
   surrogateescape error handler instead of decoding them.

   On success, return 0 and write the newly allocated wide character string into
   *wstr (use PyMem_RawFree() to free the memory). If wlen is not NULL, write
   the number of wide characters excluding the null character into *wlen.

   On memory allocation failure, return -1.

   On decoding error, return -2. If wlen is not NULL, write the start of
   invalid byte sequence in the input string into *wlen. If reason is not NULL,
   write the decoding error message into *reason.

   Return -3 if the error handler 'errors' is not supported.

   Use the Py_EncodeLocaleEx() function to encode the character string back to
   a byte string. */
int
_Py_DecodeLocaleEx(const char* arg, wchar_t **wstr, size_t *wlen,
                   const char **reason,
                   int current_locale, _Py_error_handler errors)
{
    if (current_locale) {
#ifdef _Py_FORCE_UTF8_LOCALE
        return _Py_DecodeUTF8Ex(arg, strlen(arg), wstr, wlen, reason,
                                errors);
#else
        return decode_current_locale(arg, wstr, wlen, reason, errors);
#endif
    }

#ifdef _Py_FORCE_UTF8_FS_ENCODING
    return _Py_DecodeUTF8Ex(arg, strlen(arg), wstr, wlen, reason,
                            errors);
#else
    int use_utf8 = (Py_UTF8Mode == 1);
#ifdef MS_WINDOWS
    use_utf8 |= !Py_LegacyWindowsFSEncodingFlag;
#endif
    if (use_utf8) {
        return _Py_DecodeUTF8Ex(arg, strlen(arg), wstr, wlen, reason,
                                errors);
    }

#ifdef USE_FORCE_ASCII
    if (force_ascii == -1) {
        force_ascii = check_force_ascii();
    }

    if (force_ascii) {
        /* force ASCII encoding to workaround mbstowcs() issue */
        return decode_ascii(arg, wstr, wlen, reason, errors);
    }
#endif

    return decode_current_locale(arg, wstr, wlen, reason, errors);
#endif   /* !_Py_FORCE_UTF8_FS_ENCODING */
}


/* Decode a byte string from the locale encoding with the
   surrogateescape error handler: undecodable bytes are decoded as characters
   in range U+DC80..U+DCFF. If a byte sequence can be decoded as a surrogate
   character, escape the bytes using the surrogateescape error handler instead
   of decoding them.

   Return a pointer to a newly allocated wide character string, use
   PyMem_RawFree() to free the memory. If size is not NULL, write the number of
   wide characters excluding the null character into *size

   Return NULL on decoding error or memory allocation error. If *size* is not
   NULL, *size is set to (size_t)-1 on memory error or set to (size_t)-2 on
   decoding error.

   Decoding errors should never happen, unless there is a bug in the C
   library.

   Use the Py_EncodeLocale() function to encode the character string back to a
   byte string. */
wchar_t*
Py_DecodeLocale(const char* arg, size_t *wlen)
{
    wchar_t *wstr;
    int res = _Py_DecodeLocaleEx(arg, &wstr, wlen,
                                 NULL, 0,
                                 _Py_ERROR_SURROGATEESCAPE);
    if (res != 0) {
        assert(res != -3);
        if (wlen != NULL) {
            *wlen = (size_t)res;
        }
        return NULL;
    }
    return wstr;
}


static int
encode_current_locale(const wchar_t *text, char **str,
                      size_t *error_pos, const char **reason,
                      int raw_malloc, _Py_error_handler errors)
{
    const size_t len = wcslen(text);
    char *result = NULL, *bytes = NULL;
    size_t i, size, converted;
    wchar_t c, buf[2];

    int surrogateescape;
    if (get_surrogateescape(errors, &surrogateescape) < 0) {
        return -3;
    }

    /* The function works in two steps:
       1. compute the length of the output buffer in bytes (size)
       2. outputs the bytes */
    size = 0;
    buf[1] = 0;
    while (1) {
        for (i=0; i < len; i++) {
            c = text[i];
            if (c >= 0xdc80 && c <= 0xdcff) {
                if (!surrogateescape) {
                    goto encode_error;
                }
                /* UTF-8b surrogate */
                if (bytes != NULL) {
                    *bytes++ = c - 0xdc00;
                    size--;
                }
                else {
                    size++;
                }
                continue;
            }
            else {
                buf[0] = c;
                if (bytes != NULL) {
                    converted = wcstombs(bytes, buf, size);
                }
                else {
                    converted = wcstombs(NULL, buf, 0);
                }
                if (converted == DECODE_ERROR) {
                    goto encode_error;
                }
                if (bytes != NULL) {
                    bytes += converted;
                    size -= converted;
                }
                else {
                    size += converted;
                }
            }
        }
        if (result != NULL) {
            *bytes = '\0';
            break;
        }

        size += 1; /* nul byte at the end */
        if (raw_malloc) {
            result = PyMem_RawMalloc(size);
        }
        else {
            result = PyMem_Malloc(size);
        }
        if (result == NULL) {
            return -1;
        }
        bytes = result;
    }
    *str = result;
    return 0;

encode_error:
    if (raw_malloc) {
        PyMem_RawFree(result);
    }
    else {
        PyMem_Free(result);
    }
    if (error_pos != NULL) {
        *error_pos = i;
    }
    if (reason) {
        *reason = "encoding error";
    }
    return -2;
}


/* Encode a string to the locale encoding.

   Parameters:

   * raw_malloc: if non-zero, allocate memory using PyMem_RawMalloc() instead
     of PyMem_Malloc().
   * current_locale: if non-zero, use the current LC_CTYPE, otherwise use
     Python filesystem encoding.
   * errors: error handler like "strict" or "surrogateescape".

   Return value:

    0: success, *str is set to a newly allocated decoded string.
   -1: memory allocation failure
   -2: encoding error, set *error_pos and *reason (if set).
   -3: the error handler 'errors' is not supported.
 */
static int
encode_locale_ex(const wchar_t *text, char **str, size_t *error_pos,
                 const char **reason,
                 int raw_malloc, int current_locale, _Py_error_handler errors)
{
    if (current_locale) {
#ifdef _Py_FORCE_UTF8_LOCALE
        return _Py_EncodeUTF8Ex(text, str, error_pos, reason,
                                raw_malloc, errors);
#else
        return encode_current_locale(text, str, error_pos, reason,
                                     raw_malloc, errors);
#endif
    }

#ifdef _Py_FORCE_UTF8_FS_ENCODING
    return _Py_EncodeUTF8Ex(text, str, error_pos, reason,
                            raw_malloc, errors);
#else
    int use_utf8 = (Py_UTF8Mode == 1);
#ifdef MS_WINDOWS
    use_utf8 |= !Py_LegacyWindowsFSEncodingFlag;
#endif
    if (use_utf8) {
        return _Py_EncodeUTF8Ex(text, str, error_pos, reason,
                                raw_malloc, errors);
    }

#ifdef USE_FORCE_ASCII
    if (force_ascii == -1) {
        force_ascii = check_force_ascii();
    }

    if (force_ascii) {
        return encode_ascii(text, str, error_pos, reason,
                            raw_malloc, errors);
    }
#endif

    return encode_current_locale(text, str, error_pos, reason,
                                 raw_malloc, errors);
#endif   /* _Py_FORCE_UTF8_FS_ENCODING */
}

static char*
encode_locale(const wchar_t *text, size_t *error_pos,
              int raw_malloc, int current_locale)
{
    char *str;
    int res = encode_locale_ex(text, &str, error_pos, NULL,
                               raw_malloc, current_locale,
                               _Py_ERROR_SURROGATEESCAPE);
    if (res != -2 && error_pos) {
        *error_pos = (size_t)-1;
    }
    if (res != 0) {
        return NULL;
    }
    return str;
}

/* Encode a wide character string to the locale encoding with the
   surrogateescape error handler: surrogate characters in the range
   U+DC80..U+DCFF are converted to bytes 0x80..0xFF.

   Return a pointer to a newly allocated byte string, use PyMem_Free() to free
   the memory. Return NULL on encoding or memory allocation error.

   If error_pos is not NULL, *error_pos is set to (size_t)-1 on success, or set
   to the index of the invalid character on encoding error.

   Use the Py_DecodeLocale() function to decode the bytes string back to a wide
   character string. */
char*
Py_EncodeLocale(const wchar_t *text, size_t *error_pos)
{
    return encode_locale(text, error_pos, 0, 0);
}


/* Similar to Py_EncodeLocale(), but result must be freed by PyMem_RawFree()
   instead of PyMem_Free(). */
char*
_Py_EncodeLocaleRaw(const wchar_t *text, size_t *error_pos)
{
    return encode_locale(text, error_pos, 1, 0);
}


int
_Py_EncodeLocaleEx(const wchar_t *text, char **str,
                   size_t *error_pos, const char **reason,
                   int current_locale, _Py_error_handler errors)
{
    return encode_locale_ex(text, str, error_pos, reason, 1,
                            current_locale, errors);
}


// Get the current locale encoding name:
//
// - Return "utf-8" if _Py_FORCE_UTF8_LOCALE macro is defined (ex: on Android)
// - Return "utf-8" if the UTF-8 Mode is enabled
// - On Windows, return the ANSI code page (ex: "cp1250")
// - Return "utf-8" if nl_langinfo(CODESET) returns an empty string.
// - Otherwise, return nl_langinfo(CODESET).
//
// Return NULL on memory allocation failure.
//
// See also config_get_locale_encoding()
wchar_t*
_Py_GetLocaleEncoding(void)
{
#ifdef _Py_FORCE_UTF8_LOCALE
    // On Android langinfo.h and CODESET are missing,
    // and UTF-8 is always used in mbstowcs() and wcstombs().
    return _PyMem_RawWcsdup(L"utf-8");
#else

#ifdef MS_WINDOWS
    wchar_t encoding[23];
    unsigned int ansi_codepage = GetACP();
    swprintf(encoding, Py_ARRAY_LENGTH(encoding), L"cp%u", ansi_codepage);
    encoding[Py_ARRAY_LENGTH(encoding) - 1] = 0;
    return _PyMem_RawWcsdup(encoding);
#else
    const char *encoding = nl_langinfo(CODESET);
    if (!encoding || encoding[0] == '\0') {
        // Use UTF-8 if nl_langinfo() returns an empty string. It can happen on
        // macOS if the LC_CTYPE locale is not supported.
        return _PyMem_RawWcsdup(L"utf-8");
    }

    wchar_t *wstr;
    int res = decode_current_locale(encoding, &wstr, NULL,
                                    NULL, _Py_ERROR_SURROGATEESCAPE);
    if (res < 0) {
        return NULL;
    }
    return wstr;
#endif  // !MS_WINDOWS

#endif  // !_Py_FORCE_UTF8_LOCALE
}


PyObject *
_Py_GetLocaleEncodingObject(void)
{
    wchar_t *encoding = _Py_GetLocaleEncoding();
    if (encoding == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    PyObject *str = PyUnicode_FromWideChar(encoding, -1);
    PyMem_RawFree(encoding);
    return str;
}

#ifdef HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION

/* Check whether current locale uses Unicode as internal wchar_t form. */
int
_Py_LocaleUsesNonUnicodeWchar(void)
{
    /* Oracle Solaris uses non-Unicode internal wchar_t form for
       non-Unicode locales and hence needs conversion to UTF first. */
    char* codeset = nl_langinfo(CODESET);
    if (!codeset) {
        return 0;
    }
    /* 646 refers to ISO/IEC 646 standard that corresponds to ASCII encoding */
    return (strcmp(codeset, "UTF-8") != 0 && strcmp(codeset, "646") != 0);
}

static wchar_t *
_Py_ConvertWCharForm(const wchar_t *source, Py_ssize_t size,
                     const char *tocode, const char *fromcode)
{
    static_assert(sizeof(wchar_t) == 4, "wchar_t must be 32-bit");

    /* Ensure we won't overflow the size. */
    if (size > (PY_SSIZE_T_MAX / (Py_ssize_t)sizeof(wchar_t))) {
        PyErr_NoMemory();
        return NULL;
    }

    /* the string doesn't have to be NULL terminated */
    wchar_t* target = PyMem_Malloc(size * sizeof(wchar_t));
    if (target == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    iconv_t cd = iconv_open(tocode, fromcode);
    if (cd == (iconv_t)-1) {
        PyErr_Format(PyExc_ValueError, "iconv_open() failed");
        PyMem_Free(target);
        return NULL;
    }

    char *inbuf = (char *) source;
    char *outbuf = (char *) target;
    size_t inbytesleft = sizeof(wchar_t) * size;
    size_t outbytesleft = inbytesleft;

    size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    if (ret == DECODE_ERROR) {
        PyErr_Format(PyExc_ValueError, "iconv() failed");
        PyMem_Free(target);
        iconv_close(cd);
        return NULL;
    }

    iconv_close(cd);
    return target;
}

/* Convert a wide character string to the UCS-4 encoded string. This
   is necessary on systems where internal form of wchar_t are not Unicode
   code points (e.g. Oracle Solaris).

   Return a pointer to a newly allocated string, use PyMem_Free() to free
   the memory. Return NULL and raise exception on conversion or memory
   allocation error. */
wchar_t *
_Py_DecodeNonUnicodeWchar(const wchar_t *native, Py_ssize_t size)
{
    return _Py_ConvertWCharForm(native, size, "UCS-4-INTERNAL", "wchar_t");
}

/* Convert a UCS-4 encoded string to native wide character string. This
   is necessary on systems where internal form of wchar_t are not Unicode
   code points (e.g. Oracle Solaris).

   The conversion is done in place. This can be done because both wchar_t
   and UCS-4 use 4-byte encoding, and one wchar_t symbol always correspond
   to a single UCS-4 symbol and vice versa. (This is true for Oracle Solaris,
   which is currently the only system using these functions; it doesn't have
   to be for other systems).

   Return 0 on success. Return -1 and raise exception on conversion
   or memory allocation error. */
int
_Py_EncodeNonUnicodeWchar_InPlace(wchar_t *unicode, Py_ssize_t size)
{
    wchar_t* result = _Py_ConvertWCharForm(unicode, size, "wchar_t", "UCS-4-INTERNAL");
    if (!result) {
        return -1;
    }
    memcpy(unicode, result, size * sizeof(wchar_t));
    PyMem_Free(result);
    return 0;
}
#endif /* HAVE_NON_UNICODE_WCHAR_T_REPRESENTATION */

#ifdef MS_WINDOWS
static __int64 secs_between_epochs = 11644473600; /* Seconds between 1.1.1601 and 1.1.1970 */

static void
FILE_TIME_to_time_t_nsec(FILETIME *in_ptr, time_t *time_out, int* nsec_out)
{
    /* XXX endianness. Shouldn't matter, as all Windows implementations are little-endian */
    /* Cannot simply cast and dereference in_ptr,
       since it might not be aligned properly */
    __int64 in;
    memcpy(&in, in_ptr, sizeof(in));
    *nsec_out = (int)(in % 10000000) * 100; /* FILETIME is in units of 100 nsec. */
    *time_out = Py_SAFE_DOWNCAST((in / 10000000) - secs_between_epochs, __int64, time_t);
}

void
_Py_time_t_to_FILE_TIME(time_t time_in, int nsec_in, FILETIME *out_ptr)
{
    /* XXX endianness */
    __int64 out;
    out = time_in + secs_between_epochs;
    out = out * 10000000 + nsec_in / 100;
    memcpy(out_ptr, &out, sizeof(out));
}

/* Below, we *know* that ugo+r is 0444 */
#if _S_IREAD != 0400
#error Unsupported C library
#endif
static int
attributes_to_mode(DWORD attr)
{
    int m = 0;
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        m |= _S_IFDIR | 0111; /* IFEXEC for user,group,other */
    else
        m |= _S_IFREG;
    if (attr & FILE_ATTRIBUTE_READONLY)
        m |= 0444;
    else
        m |= 0666;
    return m;
}

void
_Py_attribute_data_to_stat(BY_HANDLE_FILE_INFORMATION *info, ULONG reparse_tag,
                           struct _Py_stat_struct *result)
{
    memset(result, 0, sizeof(*result));
    result->st_mode = attributes_to_mode(info->dwFileAttributes);
    result->st_size = (((__int64)info->nFileSizeHigh)<<32) + info->nFileSizeLow;
    result->st_dev = info->dwVolumeSerialNumber;
    result->st_rdev = result->st_dev;
    FILE_TIME_to_time_t_nsec(&info->ftCreationTime, &result->st_ctime, &result->st_ctime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastWriteTime, &result->st_mtime, &result->st_mtime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastAccessTime, &result->st_atime, &result->st_atime_nsec);
    result->st_nlink = info->nNumberOfLinks;
    result->st_ino = (((uint64_t)info->nFileIndexHigh) << 32) + info->nFileIndexLow;
    /* bpo-37834: Only actual symlinks set the S_IFLNK flag. But lstat() will
       open other name surrogate reparse points without traversing them. To
       detect/handle these, check st_file_attributes and st_reparse_tag. */
    result->st_reparse_tag = reparse_tag;
    if (info->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
        reparse_tag == IO_REPARSE_TAG_SYMLINK) {
        /* first clear the S_IFMT bits */
        result->st_mode ^= (result->st_mode & S_IFMT);
        /* now set the bits that make this a symlink */
        result->st_mode |= S_IFLNK;
    }
    result->st_file_attributes = info->dwFileAttributes;
}
#endif

/* Return information about a file.

   On POSIX, use fstat().

   On Windows, use GetFileType() and GetFileInformationByHandle() which support
   files larger than 2 GiB.  fstat() may fail with EOVERFLOW on files larger
   than 2 GiB because the file size type is a signed 32-bit integer: see issue
   #23152.

   On Windows, set the last Windows error and return nonzero on error. On
   POSIX, set errno and return nonzero on error. Fill status and return 0 on
   success. */
int
_Py_fstat_noraise(int fd, struct _Py_stat_struct *status)
{
#ifdef MS_WINDOWS
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int type;

    h = _Py_get_osfhandle_noraise(fd);

    if (h == INVALID_HANDLE_VALUE) {
        /* errno is already set by _get_osfhandle, but we also set
           the Win32 error for callers who expect that */
        SetLastError(ERROR_INVALID_HANDLE);
        return -1;
    }
    memset(status, 0, sizeof(*status));

    type = GetFileType(h);
    if (type == FILE_TYPE_UNKNOWN) {
        DWORD error = GetLastError();
        if (error != 0) {
            errno = winerror_to_errno(error);
            return -1;
        }
        /* else: valid but unknown file */
    }

    if (type != FILE_TYPE_DISK) {
        if (type == FILE_TYPE_CHAR)
            status->st_mode = _S_IFCHR;
        else if (type == FILE_TYPE_PIPE)
            status->st_mode = _S_IFIFO;
        return 0;
    }

    if (!GetFileInformationByHandle(h, &info)) {
        /* The Win32 error is already set, but we also set errno for
           callers who expect it */
        errno = winerror_to_errno(GetLastError());
        return -1;
    }

    _Py_attribute_data_to_stat(&info, 0, status);
    /* specific to fstat() */
    status->st_ino = (((uint64_t)info.nFileIndexHigh) << 32) + info.nFileIndexLow;
    return 0;
#else
    return fstat(fd, status);
#endif
}

/* Return information about a file.

   On POSIX, use fstat().

   On Windows, use GetFileType() and GetFileInformationByHandle() which support
   files larger than 2 GiB.  fstat() may fail with EOVERFLOW on files larger
   than 2 GiB because the file size type is a signed 32-bit integer: see issue
   #23152.

   Raise an exception and return -1 on error. On Windows, set the last Windows
   error on error. On POSIX, set errno on error. Fill status and return 0 on
   success.

   Release the GIL to call GetFileType() and GetFileInformationByHandle(), or
   to call fstat(). The caller must hold the GIL. */
int
_Py_fstat(int fd, struct _Py_stat_struct *status)
{
    int res;

    assert(PyGILState_Check());

    Py_BEGIN_ALLOW_THREADS
    res = _Py_fstat_noraise(fd, status);
    Py_END_ALLOW_THREADS

    if (res != 0) {
#ifdef MS_WINDOWS
        PyErr_SetFromWindowsErr(0);
#else
        PyErr_SetFromErrno(PyExc_OSError);
#endif
        return -1;
    }
    return 0;
}

/* Like _Py_stat() but with a raw filename. */
int
_Py_wstat(const wchar_t* path, struct stat *buf)
{
    int err;
#ifdef MS_WINDOWS
    struct _stat wstatbuf;
    err = _wstat(path, &wstatbuf);
    if (!err) {
        buf->st_mode = wstatbuf.st_mode;
    }
#else
    char *fname;
    fname = _Py_EncodeLocaleRaw(path, NULL);
    if (fname == NULL) {
        errno = EINVAL;
        return -1;
    }
    err = stat(fname, buf);
    PyMem_RawFree(fname);
#endif
    return err;
}


/* Call _wstat() on Windows, or encode the path to the filesystem encoding and
   call stat() otherwise. Only fill st_mode attribute on Windows.

   Return 0 on success, -1 on _wstat() / stat() error, -2 if an exception was
   raised. */

int
_Py_stat(PyObject *path, struct stat *statbuf)
{
#ifdef MS_WINDOWS
    int err;

#if USE_UNICODE_WCHAR_CACHE
    const wchar_t *wpath = _PyUnicode_AsUnicode(path);
#else /* USE_UNICODE_WCHAR_CACHE */
    wchar_t *wpath = PyUnicode_AsWideCharString(path, NULL);
#endif /* USE_UNICODE_WCHAR_CACHE */
    if (wpath == NULL)
        return -2;

    err = _Py_wstat(wpath, statbuf);
#if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free(wpath);
#endif /* USE_UNICODE_WCHAR_CACHE */
    return err;
#else
    int ret;
    PyObject *bytes;
    char *cpath;

    bytes = PyUnicode_EncodeFSDefault(path);
    if (bytes == NULL)
        return -2;

    /* check for embedded null bytes */
    if (PyBytes_AsStringAndSize(bytes, &cpath, NULL) == -1) {
        Py_DECREF(bytes);
        return -2;
    }

    ret = stat(cpath, statbuf);
    Py_DECREF(bytes);
    return ret;
#endif
}


/* This function MUST be kept async-signal-safe on POSIX when raise=0. */
static int
get_inheritable(int fd, int raise)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    DWORD flags;

    handle = _Py_get_osfhandle_noraise(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (!GetHandleInformation(handle, &flags)) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }

    return (flags & HANDLE_FLAG_INHERIT);
#else
    int flags;

    flags = fcntl(fd, F_GETFD, 0);
    if (flags == -1) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return !(flags & FD_CLOEXEC);
#endif
}

/* Get the inheritable flag of the specified file descriptor.
   Return 1 if the file descriptor can be inherited, 0 if it cannot,
   raise an exception and return -1 on error. */
int
_Py_get_inheritable(int fd)
{
    return get_inheritable(fd, 1);
}


/* This function MUST be kept async-signal-safe on POSIX when raise=0. */
static int
set_inheritable(int fd, int inheritable, int raise, int *atomic_flag_works)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    DWORD flags;
#else
#if defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    static int ioctl_works = -1;
    int request;
    int err;
#endif
    int flags, new_flags;
    int res;
#endif

    /* atomic_flag_works can only be used to make the file descriptor
       non-inheritable */
    assert(!(atomic_flag_works != NULL && inheritable));

    if (atomic_flag_works != NULL && !inheritable) {
        if (*atomic_flag_works == -1) {
            int isInheritable = get_inheritable(fd, raise);
            if (isInheritable == -1)
                return -1;
            *atomic_flag_works = !isInheritable;
        }

        if (*atomic_flag_works)
            return 0;
    }

#ifdef MS_WINDOWS
    handle = _Py_get_osfhandle_noraise(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (inheritable)
        flags = HANDLE_FLAG_INHERIT;
    else
        flags = 0;

    /* This check can be removed once support for Windows 7 ends. */
#define CONSOLE_PSEUDOHANDLE(handle) (((ULONG_PTR)(handle) & 0x3) == 0x3 && \
        GetFileType(handle) == FILE_TYPE_CHAR)

    if (!CONSOLE_PSEUDOHANDLE(handle) &&
        !SetHandleInformation(handle, HANDLE_FLAG_INHERIT, flags)) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }
#undef CONSOLE_PSEUDOHANDLE
    return 0;

#else

#if defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    if (ioctl_works != 0 && raise != 0) {
        /* fast-path: ioctl() only requires one syscall */
        /* caveat: raise=0 is an indicator that we must be async-signal-safe
         * thus avoid using ioctl() so we skip the fast-path. */
        if (inheritable)
            request = FIONCLEX;
        else
            request = FIOCLEX;
        err = ioctl(fd, request, NULL);
        if (!err) {
            ioctl_works = 1;
            return 0;
        }

#ifdef O_PATH
        if (errno == EBADF) {
            // bpo-44849: On Linux and FreeBSD, ioctl(FIOCLEX) fails with EBADF
            // on O_PATH file descriptors. Fall through to the fcntl()
            // implementation.
        }
        else
#endif
        if (errno != ENOTTY && errno != EACCES) {
            if (raise)
                PyErr_SetFromErrno(PyExc_OSError);
            return -1;
        }
        else {
            /* Issue #22258: Here, ENOTTY means "Inappropriate ioctl for
               device". The ioctl is declared but not supported by the kernel.
               Remember that ioctl() doesn't work. It is the case on
               Illumos-based OS for example.

               Issue #27057: When SELinux policy disallows ioctl it will fail
               with EACCES. While FIOCLEX is safe operation it may be
               unavailable because ioctl was denied altogether.
               This can be the case on Android. */
            ioctl_works = 0;
        }
        /* fallback to fcntl() if ioctl() does not work */
    }
#endif

    /* slow-path: fcntl() requires two syscalls */
    flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (inheritable) {
        new_flags = flags & ~FD_CLOEXEC;
    }
    else {
        new_flags = flags | FD_CLOEXEC;
    }

    if (new_flags == flags) {
        /* FD_CLOEXEC flag already set/cleared: nothing to do */
        return 0;
    }

    res = fcntl(fd, F_SETFD, new_flags);
    if (res < 0) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#endif
}

/* Make the file descriptor non-inheritable.
   Return 0 on success, set errno and return -1 on error. */
static int
make_non_inheritable(int fd)
{
    return set_inheritable(fd, 0, 0, NULL);
}

/* Set the inheritable flag of the specified file descriptor.
   On success: return 0, on error: raise an exception and return -1.

   If atomic_flag_works is not NULL:

    * if *atomic_flag_works==-1, check if the inheritable is set on the file
      descriptor: if yes, set *atomic_flag_works to 1, otherwise set to 0 and
      set the inheritable flag
    * if *atomic_flag_works==1: do nothing
    * if *atomic_flag_works==0: set inheritable flag to False

   Set atomic_flag_works to NULL if no atomic flag was used to create the
   file descriptor.

   atomic_flag_works can only be used to make a file descriptor
   non-inheritable: atomic_flag_works must be NULL if inheritable=1. */
int
_Py_set_inheritable(int fd, int inheritable, int *atomic_flag_works)
{
    return set_inheritable(fd, inheritable, 1, atomic_flag_works);
}

/* Same as _Py_set_inheritable() but on error, set errno and
   don't raise an exception.
   This function is async-signal-safe. */
int
_Py_set_inheritable_async_safe(int fd, int inheritable, int *atomic_flag_works)
{
    return set_inheritable(fd, inheritable, 0, atomic_flag_works);
}

static int
_Py_open_impl(const char *pathname, int flags, int gil_held)
{
    int fd;
    int async_err = 0;
#ifndef MS_WINDOWS
    int *atomic_flag_works;
#endif

#ifdef MS_WINDOWS
    flags |= O_NOINHERIT;
#elif defined(O_CLOEXEC)
    atomic_flag_works = &_Py_open_cloexec_works;
    flags |= O_CLOEXEC;
#else
    atomic_flag_works = NULL;
#endif

    if (gil_held) {
        PyObject *pathname_obj = PyUnicode_DecodeFSDefault(pathname);
        if (pathname_obj == NULL) {
            return -1;
        }
        if (PySys_Audit("open", "OOi", pathname_obj, Py_None, flags) < 0) {
            Py_DECREF(pathname_obj);
            return -1;
        }

        do {
            Py_BEGIN_ALLOW_THREADS
            fd = open(pathname, flags);
            Py_END_ALLOW_THREADS
        } while (fd < 0
                 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
        if (async_err) {
            Py_DECREF(pathname_obj);
            return -1;
        }
        if (fd < 0) {
            PyErr_SetFromErrnoWithFilenameObjects(PyExc_OSError, pathname_obj, NULL);
            Py_DECREF(pathname_obj);
            return -1;
        }
        Py_DECREF(pathname_obj);
    }
    else {
        fd = open(pathname, flags);
        if (fd < 0)
            return -1;
    }

#ifndef MS_WINDOWS
    if (set_inheritable(fd, 0, gil_held, atomic_flag_works) < 0) {
        close(fd);
        return -1;
    }
#endif

    return fd;
}

/* Open a file with the specified flags (wrapper to open() function).
   Return a file descriptor on success. Raise an exception and return -1 on
   error.

   The file descriptor is created non-inheritable.

   When interrupted by a signal (open() fails with EINTR), retry the syscall,
   except if the Python signal handler raises an exception.

   Release the GIL to call open(). The caller must hold the GIL. */
int
_Py_open(const char *pathname, int flags)
{
    /* _Py_open() must be called with the GIL held. */
    assert(PyGILState_Check());
    return _Py_open_impl(pathname, flags, 1);
}

/* Open a file with the specified flags (wrapper to open() function).
   Return a file descriptor on success. Set errno and return -1 on error.

   The file descriptor is created non-inheritable.

   If interrupted by a signal, fail with EINTR. */
int
_Py_open_noraise(const char *pathname, int flags)
{
    return _Py_open_impl(pathname, flags, 0);
}

/* Open a file. Use _wfopen() on Windows, encode the path to the locale
   encoding and use fopen() otherwise.

   The file descriptor is created non-inheritable.

   If interrupted by a signal, fail with EINTR. */
FILE *
_Py_wfopen(const wchar_t *path, const wchar_t *mode)
{
    FILE *f;
    if (PySys_Audit("open", "uui", path, mode, 0) < 0) {
        return NULL;
    }
#ifndef MS_WINDOWS
    char *cpath;
    char cmode[10];
    size_t r;
    r = wcstombs(cmode, mode, 10);
    if (r == DECODE_ERROR || r >= 10) {
        errno = EINVAL;
        return NULL;
    }
    cpath = _Py_EncodeLocaleRaw(path, NULL);
    if (cpath == NULL) {
        return NULL;
    }
    f = fopen(cpath, cmode);
    PyMem_RawFree(cpath);
#else
    f = _wfopen(path, mode);
#endif
    if (f == NULL)
        return NULL;
    if (make_non_inheritable(fileno(f)) < 0) {
        fclose(f);
        return NULL;
    }
    return f;
}


/* Open a file. Call _wfopen() on Windows, or encode the path to the filesystem
   encoding and call fopen() otherwise.

   Return the new file object on success. Raise an exception and return NULL
   on error.

   The file descriptor is created non-inheritable.

   When interrupted by a signal (open() fails with EINTR), retry the syscall,
   except if the Python signal handler raises an exception.

   Release the GIL to call _wfopen() or fopen(). The caller must hold
   the GIL. */
FILE*
_Py_fopen_obj(PyObject *path, const char *mode)
{
    FILE *f;
    int async_err = 0;
#ifdef MS_WINDOWS
    wchar_t wmode[10];
    int usize;

    assert(PyGILState_Check());

    if (PySys_Audit("open", "Osi", path, mode, 0) < 0) {
        return NULL;
    }
    if (!PyUnicode_Check(path)) {
        PyErr_Format(PyExc_TypeError,
                     "str file path expected under Windows, got %R",
                     Py_TYPE(path));
        return NULL;
    }
#if USE_UNICODE_WCHAR_CACHE
    const wchar_t *wpath = _PyUnicode_AsUnicode(path);
#else /* USE_UNICODE_WCHAR_CACHE */
    wchar_t *wpath = PyUnicode_AsWideCharString(path, NULL);
#endif /* USE_UNICODE_WCHAR_CACHE */
    if (wpath == NULL)
        return NULL;

    usize = MultiByteToWideChar(CP_ACP, 0, mode, -1,
                                wmode, Py_ARRAY_LENGTH(wmode));
    if (usize == 0) {
        PyErr_SetFromWindowsErr(0);
#if !USE_UNICODE_WCHAR_CACHE
        PyMem_Free(wpath);
#endif /* USE_UNICODE_WCHAR_CACHE */
        return NULL;
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        f = _wfopen(wpath, wmode);
        Py_END_ALLOW_THREADS
    } while (f == NULL
             && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    int saved_errno = errno;
#if !USE_UNICODE_WCHAR_CACHE
    PyMem_Free(wpath);
#endif /* USE_UNICODE_WCHAR_CACHE */
#else
    PyObject *bytes;
    const char *path_bytes;

    assert(PyGILState_Check());

    if (!PyUnicode_FSConverter(path, &bytes))
        return NULL;
    path_bytes = PyBytes_AS_STRING(bytes);

    if (PySys_Audit("open", "Osi", path, mode, 0) < 0) {
        Py_DECREF(bytes);
        return NULL;
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        f = fopen(path_bytes, mode);
        Py_END_ALLOW_THREADS
    } while (f == NULL
             && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    int saved_errno = errno;
    Py_DECREF(bytes);
#endif
    if (async_err)
        return NULL;

    if (f == NULL) {
        errno = saved_errno;
        PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path);
        return NULL;
    }

    if (set_inheritable(fileno(f), 0, 1, NULL) < 0) {
        fclose(f);
        return NULL;
    }
    return f;
}

/* Read count bytes from fd into buf.

   On success, return the number of read bytes, it can be lower than count.
   If the current file offset is at or past the end of file, no bytes are read,
   and read() returns zero.

   On error, raise an exception, set errno and return -1.

   When interrupted by a signal (read() fails with EINTR), retry the syscall.
   If the Python signal handler raises an exception, the function returns -1
   (the syscall is not retried).

   Release the GIL to call read(). The caller must hold the GIL. */
Py_ssize_t
_Py_read(int fd, void *buf, size_t count)
{
    Py_ssize_t n;
    int err;
    int async_err = 0;

    assert(PyGILState_Check());

    /* _Py_read() must not be called with an exception set, otherwise the
     * caller may think that read() was interrupted by a signal and the signal
     * handler raised an exception. */
    assert(!PyErr_Occurred());

    if (count > _PY_READ_MAX) {
        count = _PY_READ_MAX;
    }

    _Py_BEGIN_SUPPRESS_IPH
    do {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
#ifdef MS_WINDOWS
        n = read(fd, buf, (int)count);
#else
        n = read(fd, buf, count);
#endif
        /* save/restore errno because PyErr_CheckSignals()
         * and PyErr_SetFromErrno() can modify it */
        err = errno;
        Py_END_ALLOW_THREADS
    } while (n < 0 && err == EINTR &&
            !(async_err = PyErr_CheckSignals()));
    _Py_END_SUPPRESS_IPH

    if (async_err) {
        /* read() was interrupted by a signal (failed with EINTR)
         * and the Python signal handler raised an exception */
        errno = err;
        assert(errno == EINTR && PyErr_Occurred());
        return -1;
    }
    if (n < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        errno = err;
        return -1;
    }

    return n;
}

static Py_ssize_t
_Py_write_impl(int fd, const void *buf, size_t count, int gil_held)
{
    Py_ssize_t n;
    int err;
    int async_err = 0;

    _Py_BEGIN_SUPPRESS_IPH
#ifdef MS_WINDOWS
    if (count > 32767) {
        /* Issue #11395: the Windows console returns an error (12: not
           enough space error) on writing into stdout if stdout mode is
           binary and the length is greater than 66,000 bytes (or less,
           depending on heap usage). */
        if (gil_held) {
            Py_BEGIN_ALLOW_THREADS
            if (isatty(fd)) {
                count = 32767;
            }
            Py_END_ALLOW_THREADS
        } else {
            if (isatty(fd)) {
                count = 32767;
            }
        }
    }
#endif
    if (count > _PY_WRITE_MAX) {
        count = _PY_WRITE_MAX;
    }

    if (gil_held) {
        do {
            Py_BEGIN_ALLOW_THREADS
            errno = 0;
#ifdef MS_WINDOWS
            n = write(fd, buf, (int)count);
#else
            n = write(fd, buf, count);
#endif
            /* save/restore errno because PyErr_CheckSignals()
             * and PyErr_SetFromErrno() can modify it */
            err = errno;
            Py_END_ALLOW_THREADS
        } while (n < 0 && err == EINTR &&
                !(async_err = PyErr_CheckSignals()));
    }
    else {
        do {
            errno = 0;
#ifdef MS_WINDOWS
            n = write(fd, buf, (int)count);
#else
            n = write(fd, buf, count);
#endif
            err = errno;
        } while (n < 0 && err == EINTR);
    }
    _Py_END_SUPPRESS_IPH

    if (async_err) {
        /* write() was interrupted by a signal (failed with EINTR)
           and the Python signal handler raised an exception (if gil_held is
           nonzero). */
        errno = err;
        assert(errno == EINTR && (!gil_held || PyErr_Occurred()));
        return -1;
    }
    if (n < 0) {
        if (gil_held)
            PyErr_SetFromErrno(PyExc_OSError);
        errno = err;
        return -1;
    }

    return n;
}

/* Write count bytes of buf into fd.

   On success, return the number of written bytes, it can be lower than count
   including 0. On error, raise an exception, set errno and return -1.

   When interrupted by a signal (write() fails with EINTR), retry the syscall.
   If the Python signal handler raises an exception, the function returns -1
   (the syscall is not retried).

   Release the GIL to call write(). The caller must hold the GIL. */
Py_ssize_t
_Py_write(int fd, const void *buf, size_t count)
{
    assert(PyGILState_Check());

    /* _Py_write() must not be called with an exception set, otherwise the
     * caller may think that write() was interrupted by a signal and the signal
     * handler raised an exception. */
    assert(!PyErr_Occurred());

    return _Py_write_impl(fd, buf, count, 1);
}

/* Write count bytes of buf into fd.
 *
 * On success, return the number of written bytes, it can be lower than count
 * including 0. On error, set errno and return -1.
 *
 * When interrupted by a signal (write() fails with EINTR), retry the syscall
 * without calling the Python signal handler. */
Py_ssize_t
_Py_write_noraise(int fd, const void *buf, size_t count)
{
    return _Py_write_impl(fd, buf, count, 0);
}

#ifdef HAVE_READLINK

/* Read value of symbolic link. Encode the path to the locale encoding, decode
   the result from the locale encoding.

   Return -1 on encoding error, on readlink() error, if the internal buffer is
   too short, on decoding error, or if 'buf' is too short. */
int
_Py_wreadlink(const wchar_t *path, wchar_t *buf, size_t buflen)
{
    char *cpath;
    char cbuf[MAXPATHLEN];
    size_t cbuf_len = Py_ARRAY_LENGTH(cbuf);
    wchar_t *wbuf;
    Py_ssize_t res;
    size_t r1;

    cpath = _Py_EncodeLocaleRaw(path, NULL);
    if (cpath == NULL) {
        errno = EINVAL;
        return -1;
    }
    res = readlink(cpath, cbuf, cbuf_len);
    PyMem_RawFree(cpath);
    if (res == -1) {
        return -1;
    }
    if ((size_t)res == cbuf_len) {
        errno = EINVAL;
        return -1;
    }
    cbuf[res] = '\0'; /* buf will be null terminated */
    wbuf = Py_DecodeLocale(cbuf, &r1);
    if (wbuf == NULL) {
        errno = EINVAL;
        return -1;
    }
    /* wbuf must have space to store the trailing NUL character */
    if (buflen <= r1) {
        PyMem_RawFree(wbuf);
        errno = EINVAL;
        return -1;
    }
    wcsncpy(buf, wbuf, buflen);
    PyMem_RawFree(wbuf);
    return (int)r1;
}
#endif

#ifdef HAVE_REALPATH

/* Return the canonicalized absolute pathname. Encode path to the locale
   encoding, decode the result from the locale encoding.

   Return NULL on encoding error, realpath() error, decoding error
   or if 'resolved_path' is too short. */
wchar_t*
_Py_wrealpath(const wchar_t *path,
              wchar_t *resolved_path, size_t resolved_path_len)
{
    char *cpath;
    char cresolved_path[MAXPATHLEN];
    wchar_t *wresolved_path;
    char *res;
    size_t r;
    cpath = _Py_EncodeLocaleRaw(path, NULL);
    if (cpath == NULL) {
        errno = EINVAL;
        return NULL;
    }
    res = realpath(cpath, cresolved_path);
    PyMem_RawFree(cpath);
    if (res == NULL)
        return NULL;

    wresolved_path = Py_DecodeLocale(cresolved_path, &r);
    if (wresolved_path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    /* wresolved_path must have space to store the trailing NUL character */
    if (resolved_path_len <= r) {
        PyMem_RawFree(wresolved_path);
        errno = EINVAL;
        return NULL;
    }
    wcsncpy(resolved_path, wresolved_path, resolved_path_len);
    PyMem_RawFree(wresolved_path);
    return resolved_path;
}
#endif


int
_Py_isabs(const wchar_t *path)
{
#ifdef MS_WINDOWS
    const wchar_t *tail;
    HRESULT hr = PathCchSkipRoot(path, &tail);
    if (FAILED(hr) || path == tail) {
        return 0;
    }
    if (tail == &path[1] && (path[0] == SEP || path[0] == ALTSEP)) {
        // Exclude paths with leading SEP
        return 0;
    }
    if (tail == &path[2] && path[1] == L':') {
        // Exclude drive-relative paths (e.g. C:filename.ext)
        return 0;
    }
    return 1;
#else
    return (path[0] == SEP);
#endif
}


/* Get an absolute path.
   On error (ex: fail to get the current directory), return -1.
   On memory allocation failure, set *abspath_p to NULL and return 0.
   On success, return a newly allocated to *abspath_p to and return 0.
   The string must be freed by PyMem_RawFree(). */
int
_Py_abspath(const wchar_t *path, wchar_t **abspath_p)
{
    if (path[0] == '\0' || !wcscmp(path, L".")) {
        wchar_t cwd[MAXPATHLEN + 1];
        cwd[Py_ARRAY_LENGTH(cwd) - 1] = 0;
        if (!_Py_wgetcwd(cwd, Py_ARRAY_LENGTH(cwd) - 1)) {
            /* unable to get the current directory */
            return -1;
        }
        *abspath_p = _PyMem_RawWcsdup(cwd);
        return 0;
    }

    if (_Py_isabs(path)) {
        *abspath_p = _PyMem_RawWcsdup(path);
        return 0;
    }

#ifdef MS_WINDOWS
    return _PyOS_getfullpathname(path, abspath_p);
#else
    wchar_t cwd[MAXPATHLEN + 1];
    cwd[Py_ARRAY_LENGTH(cwd) - 1] = 0;
    if (!_Py_wgetcwd(cwd, Py_ARRAY_LENGTH(cwd) - 1)) {
        /* unable to get the current directory */
        return -1;
    }

    size_t cwd_len = wcslen(cwd);
    size_t path_len = wcslen(path);
    size_t len = cwd_len + 1 + path_len + 1;
    if (len <= (size_t)PY_SSIZE_T_MAX / sizeof(wchar_t)) {
        *abspath_p = PyMem_RawMalloc(len * sizeof(wchar_t));
    }
    else {
        *abspath_p = NULL;
    }
    if (*abspath_p == NULL) {
        return 0;
    }

    wchar_t *abspath = *abspath_p;
    memcpy(abspath, cwd, cwd_len * sizeof(wchar_t));
    abspath += cwd_len;

    *abspath = (wchar_t)SEP;
    abspath++;

    memcpy(abspath, path, path_len * sizeof(wchar_t));
    abspath += path_len;

    *abspath = 0;
    return 0;
#endif
}


// The caller must ensure "buffer" is big enough.
static int
join_relfile(wchar_t *buffer, size_t bufsize,
             const wchar_t *dirname, const wchar_t *relfile)
{
#ifdef MS_WINDOWS
    if (FAILED(PathCchCombineEx(buffer, bufsize, dirname, relfile,
        PATHCCH_ALLOW_LONG_PATHS))) {
        return -1;
    }
#else
    assert(!_Py_isabs(relfile));
    size_t dirlen = wcslen(dirname);
    size_t rellen = wcslen(relfile);
    size_t maxlen = bufsize - 1;
    if (maxlen > MAXPATHLEN || dirlen >= maxlen || rellen >= maxlen - dirlen) {
        return -1;
    }
    if (dirlen == 0) {
        // We do not add a leading separator.
        wcscpy(buffer, relfile);
    }
    else {
        if (dirname != buffer) {
            wcscpy(buffer, dirname);
        }
        size_t relstart = dirlen;
        if (dirlen > 1 && dirname[dirlen - 1] != SEP) {
            buffer[dirlen] = SEP;
            relstart += 1;
        }
        wcscpy(&buffer[relstart], relfile);
    }
#endif
    return 0;
}

/* Join the two paths together, like os.path.join().  Return NULL
   if memory could not be allocated.  The caller is responsible
   for calling PyMem_RawFree() on the result. */
wchar_t *
_Py_join_relfile(const wchar_t *dirname, const wchar_t *relfile)
{
    assert(dirname != NULL && relfile != NULL);
#ifndef MS_WINDOWS
    assert(!_Py_isabs(relfile));
#endif
    size_t maxlen = wcslen(dirname) + 1 + wcslen(relfile);
    size_t bufsize = maxlen + 1;
    wchar_t *filename = PyMem_RawMalloc(bufsize * sizeof(wchar_t));
    if (filename == NULL) {
        return NULL;
    }
    assert(wcslen(dirname) < MAXPATHLEN);
    assert(wcslen(relfile) < MAXPATHLEN - wcslen(dirname));
    if (join_relfile(filename, bufsize, dirname, relfile) < 0) {
        PyMem_RawFree(filename);
        return NULL;
    }
    return filename;
}

/* Join the two paths together, like os.path.join().
     dirname: the target buffer with the dirname already in place,
              including trailing NUL
     relfile: this must be a relative path
     bufsize: total allocated size of the buffer
   Return -1 if anything is wrong with the path lengths. */
int
_Py_add_relfile(wchar_t *dirname, const wchar_t *relfile, size_t bufsize)
{
    assert(dirname != NULL && relfile != NULL);
    assert(bufsize > 0);
    return join_relfile(dirname, bufsize, dirname, relfile);
}


size_t
_Py_find_basename(const wchar_t *filename)
{
    for (size_t i = wcslen(filename); i > 0; --i) {
        if (filename[i] == SEP) {
            return i + 1;
        }
    }
    return 0;
}

/* In-place path normalisation. Returns the start of the normalized
   path, which will be within the original buffer. Guaranteed to not
   make the path longer, and will not fail. 'size' is the length of
   the path, if known. If -1, the first null character will be assumed
   to be the end of the path. 'normsize' will be set to contain the
   length of the resulting normalized path. */
wchar_t *
_Py_normpath_and_size(wchar_t *path, Py_ssize_t size, Py_ssize_t *normsize)
{
    assert(path != NULL);
    if ((size < 0 && !path[0]) || size == 0) {
        *normsize = 0;
        return path;
    }
    wchar_t *pEnd = size >= 0 ? &path[size] : NULL;
    wchar_t *p1 = path;     // sequentially scanned address in the path
    wchar_t *p2 = path;     // destination of a scanned character to be ljusted
    wchar_t *minP2 = path;  // the beginning of the destination range
    wchar_t lastC = L'\0';  // the last ljusted character, p2[-1] in most cases

#define IS_END(x) (pEnd ? (x) == pEnd : !*(x))
#ifdef ALTSEP
#define IS_SEP(x) (*(x) == SEP || *(x) == ALTSEP)
#else
#define IS_SEP(x) (*(x) == SEP)
#endif
#define SEP_OR_END(x) (IS_SEP(x) || IS_END(x))

    // Skip leading '.\'
    if (p1[0] == L'.' && IS_SEP(&p1[1])) {
        path = &path[2];
        while (IS_SEP(path) && !IS_END(path)) {
            path++;
        }
        p1 = p2 = minP2 = path;
        lastC = SEP;
    }
#ifdef MS_WINDOWS
    // Skip past drive segment and update minP2
    else if (p1[0] && p1[1] == L':') {
        *p2++ = *p1++;
        *p2++ = *p1++;
        minP2 = p2;
        lastC = L':';
    }
    // Skip past all \\-prefixed paths, including \\?\, \\.\,
    // and network paths, including the first segment.
    else if (IS_SEP(&p1[0]) && IS_SEP(&p1[1])) {
        int sepCount = 2;
        *p2++ = SEP;
        *p2++ = SEP;
        p1 += 2;
        for (; !IS_END(p1) && sepCount; ++p1) {
            if (IS_SEP(p1)) {
                --sepCount;
                *p2++ = lastC = SEP;
            } else {
                *p2++ = lastC = *p1;
            }
        }
        minP2 = p2 - 1;
    }
#else
    // Skip past two leading SEPs
    else if (IS_SEP(&p1[0]) && IS_SEP(&p1[1]) && !IS_SEP(&p1[2])) {
        *p2++ = *p1++;
        *p2++ = *p1++;
        minP2 = p2 - 1;  // Absolute path has SEP at minP2
        lastC = SEP;
    }
#endif /* MS_WINDOWS */

    /* if pEnd is specified, check that. Else, check for null terminator */
    for (; !IS_END(p1); ++p1) {
        wchar_t c = *p1;
#ifdef ALTSEP
        if (c == ALTSEP) {
            c = SEP;
        }
#endif
        if (lastC == SEP) {
            if (c == L'.') {
                int sep_at_1 = SEP_OR_END(&p1[1]);
                int sep_at_2 = !sep_at_1 && SEP_OR_END(&p1[2]);
                if (sep_at_2 && p1[1] == L'.') {
                    wchar_t *p3 = p2;
                    while (p3 != minP2 && *--p3 == SEP) { }
                    while (p3 != minP2 && *(p3 - 1) != SEP) { --p3; }
                    if (p2 == minP2
                        || (p3[0] == L'.' && p3[1] == L'.' && IS_SEP(&p3[2])))
                    {
                        // Previous segment is also ../, so append instead.
                        // Relative path does not absorb ../ at minP2 as well.
                        *p2++ = L'.';
                        *p2++ = L'.';
                        lastC = L'.';
                    } else if (p3[0] == SEP) {
                        // Absolute path, so absorb segment
                        p2 = p3 + 1;
                    } else {
                        p2 = p3;
                    }
                    p1 += 1;
                } else if (sep_at_1) {
                } else {
                    *p2++ = lastC = c;
                }
            } else if (c == SEP) {
            } else {
                *p2++ = lastC = c;
            }
        } else {
            *p2++ = lastC = c;
        }
    }
    *p2 = L'\0';
    if (p2 != minP2) {
        while (--p2 != minP2 && *p2 == SEP) {
            *p2 = L'\0';
        }
    } else {
        --p2;
    }
    *normsize = p2 - path + 1;
#undef SEP_OR_END
#undef IS_SEP
#undef IS_END
    return path;
}

/* In-place path normalisation. Returns the start of the normalized
   path, which will be within the original buffer. Guaranteed to not
   make the path longer, and will not fail. 'size' is the length of
   the path, if known. If -1, the first null character will be assumed
   to be the end of the path. */
wchar_t *
_Py_normpath(wchar_t *path, Py_ssize_t size)
{
    Py_ssize_t norm_length;
    return _Py_normpath_and_size(path, size, &norm_length);
}


/* Get the current directory. buflen is the buffer size in wide characters
   including the null character. Decode the path from the locale encoding.

   Return NULL on getcwd() error, on decoding error, or if 'buf' is
   too short. */
wchar_t*
_Py_wgetcwd(wchar_t *buf, size_t buflen)
{
#ifdef MS_WINDOWS
    int ibuflen = (int)Py_MIN(buflen, INT_MAX);
    return _wgetcwd(buf, ibuflen);
#else
    char fname[MAXPATHLEN];
    wchar_t *wname;
    size_t len;

    if (getcwd(fname, Py_ARRAY_LENGTH(fname)) == NULL)
        return NULL;
    wname = Py_DecodeLocale(fname, &len);
    if (wname == NULL)
        return NULL;
    /* wname must have space to store the trailing NUL character */
    if (buflen <= len) {
        PyMem_RawFree(wname);
        return NULL;
    }
    wcsncpy(buf, wname, buflen);
    PyMem_RawFree(wname);
    return buf;
#endif
}

/* Duplicate a file descriptor. The new file descriptor is created as
   non-inheritable. Return a new file descriptor on success, raise an OSError
   exception and return -1 on error.

   The GIL is released to call dup(). The caller must hold the GIL. */
int
_Py_dup(int fd)
{
#ifdef MS_WINDOWS
    HANDLE handle;
#endif

    assert(PyGILState_Check());

#ifdef MS_WINDOWS
    handle = _Py_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    fd = dup(fd);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (_Py_set_inheritable(fd, 0, NULL) < 0) {
        _Py_BEGIN_SUPPRESS_IPH
        close(fd);
        _Py_END_SUPPRESS_IPH
        return -1;
    }
#elif defined(HAVE_FCNTL_H) && defined(F_DUPFD_CLOEXEC)
    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

#elif HAVE_DUP
    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    fd = dup(fd);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (_Py_set_inheritable(fd, 0, NULL) < 0) {
        _Py_BEGIN_SUPPRESS_IPH
        close(fd);
        _Py_END_SUPPRESS_IPH
        return -1;
    }
#else
    errno = ENOTSUP;
    PyErr_SetFromErrno(PyExc_OSError);
    return -1;
#endif
    return fd;
}

#ifndef MS_WINDOWS
/* Get the blocking mode of the file descriptor.
   Return 0 if the O_NONBLOCK flag is set, 1 if the flag is cleared,
   raise an exception and return -1 on error. */
int
_Py_get_blocking(int fd)
{
    int flags;
    _Py_BEGIN_SUPPRESS_IPH
    flags = fcntl(fd, F_GETFL, 0);
    _Py_END_SUPPRESS_IPH
    if (flags < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    return !(flags & O_NONBLOCK);
}

/* Set the blocking mode of the specified file descriptor.

   Set the O_NONBLOCK flag if blocking is False, clear the O_NONBLOCK flag
   otherwise.

   Return 0 on success, raise an exception and return -1 on error. */
int
_Py_set_blocking(int fd, int blocking)
{
/* bpo-41462: On VxWorks, ioctl(FIONBIO) only works on sockets.
   Use fcntl() instead. */
#if defined(HAVE_SYS_IOCTL_H) && defined(FIONBIO) && !defined(__VXWORKS__)
    int arg = !blocking;
    if (ioctl(fd, FIONBIO, &arg) < 0)
        goto error;
#else
    int flags, res;

    _Py_BEGIN_SUPPRESS_IPH
    flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        if (blocking)
            flags = flags & (~O_NONBLOCK);
        else
            flags = flags | O_NONBLOCK;

        res = fcntl(fd, F_SETFL, flags);
    } else {
        res = -1;
    }
    _Py_END_SUPPRESS_IPH

    if (res < 0)
        goto error;
#endif
    return 0;

error:
    PyErr_SetFromErrno(PyExc_OSError);
    return -1;
}
#else   /* MS_WINDOWS */
void*
_Py_get_osfhandle_noraise(int fd)
{
    void *handle;
    _Py_BEGIN_SUPPRESS_IPH
    handle = (void*)_get_osfhandle(fd);
    _Py_END_SUPPRESS_IPH
    return handle;
}

void*
_Py_get_osfhandle(int fd)
{
    void *handle = _Py_get_osfhandle_noraise(fd);
    if (handle == INVALID_HANDLE_VALUE)
        PyErr_SetFromErrno(PyExc_OSError);

    return handle;
}

int
_Py_open_osfhandle_noraise(void *handle, int flags)
{
    int fd;
    _Py_BEGIN_SUPPRESS_IPH
    fd = _open_osfhandle((intptr_t)handle, flags);
    _Py_END_SUPPRESS_IPH
    return fd;
}

int
_Py_open_osfhandle(void *handle, int flags)
{
    int fd = _Py_open_osfhandle_noraise(handle, flags);
    if (fd == -1)
        PyErr_SetFromErrno(PyExc_OSError);

    return fd;
}
#endif  /* MS_WINDOWS */

int
_Py_GetLocaleconvNumeric(struct lconv *lc,
                         PyObject **decimal_point, PyObject **thousands_sep)
{
    assert(decimal_point != NULL);
    assert(thousands_sep != NULL);

#ifndef MS_WINDOWS
    int change_locale = 0;
    if ((strlen(lc->decimal_point) > 1 || ((unsigned char)lc->decimal_point[0]) > 127)) {
        change_locale = 1;
    }
    if ((strlen(lc->thousands_sep) > 1 || ((unsigned char)lc->thousands_sep[0]) > 127)) {
        change_locale = 1;
    }

    /* Keep a copy of the LC_CTYPE locale */
    char *oldloc = NULL, *loc = NULL;
    if (change_locale) {
        oldloc = setlocale(LC_CTYPE, NULL);
        if (!oldloc) {
            PyErr_SetString(PyExc_RuntimeWarning,
                            "failed to get LC_CTYPE locale");
            return -1;
        }

        oldloc = _PyMem_Strdup(oldloc);
        if (!oldloc) {
            PyErr_NoMemory();
            return -1;
        }

        loc = setlocale(LC_NUMERIC, NULL);
        if (loc != NULL && strcmp(loc, oldloc) == 0) {
            loc = NULL;
        }

        if (loc != NULL) {
            /* Only set the locale temporarily the LC_CTYPE locale
               if LC_NUMERIC locale is different than LC_CTYPE locale and
               decimal_point and/or thousands_sep are non-ASCII or longer than
               1 byte */
            setlocale(LC_CTYPE, loc);
        }
    }

#define GET_LOCALE_STRING(ATTR) PyUnicode_DecodeLocale(lc->ATTR, NULL)
#else /* MS_WINDOWS */
/* Use _W_* fields of Windows strcut lconv */
#define GET_LOCALE_STRING(ATTR) PyUnicode_FromWideChar(lc->_W_ ## ATTR, -1)
#endif /* MS_WINDOWS */

    int res = -1;

    *decimal_point = GET_LOCALE_STRING(decimal_point);
    if (*decimal_point == NULL) {
        goto done;
    }

    *thousands_sep = GET_LOCALE_STRING(thousands_sep);
    if (*thousands_sep == NULL) {
        goto done;
    }

    res = 0;

done:
#ifndef MS_WINDOWS
    if (loc != NULL) {
        setlocale(LC_CTYPE, oldloc);
    }
    PyMem_Free(oldloc);
#endif
    return res;

#undef GET_LOCALE_STRING
}

/* Our selection logic for which function to use is as follows:
 * 1. If close_range(2) is available, always prefer that; it's better for
 *    contiguous ranges like this than fdwalk(3) which entails iterating over
 *    the entire fd space and simply doing nothing for those outside the range.
 * 2. If closefrom(2) is available, we'll attempt to use that next if we're
 *    closing up to sysconf(_SC_OPEN_MAX).
 * 2a. Fallback to fdwalk(3) if we're not closing up to sysconf(_SC_OPEN_MAX),
 *    as that will be more performant if the range happens to have any chunk of
 *    non-opened fd in the middle.
 * 2b. If fdwalk(3) isn't available, just do a plain close(2) loop.
 */
#ifdef __FreeBSD__
#  define USE_CLOSEFROM
#endif /* __FreeBSD__ */

#ifdef HAVE_FDWALK
#  define USE_FDWALK
#endif /* HAVE_FDWALK */

#ifdef USE_FDWALK
static int
_fdwalk_close_func(void *lohi, int fd)
{
    int lo = ((int *)lohi)[0];
    int hi = ((int *)lohi)[1];

    if (fd >= hi) {
        return 1;
    }
    else if (fd >= lo) {
        /* Ignore errors */
        (void)close(fd);
    }
    return 0;
}
#endif /* USE_FDWALK */

/* Closes all file descriptors in [first, last], ignoring errors. */
void
_Py_closerange(int first, int last)
{
    first = Py_MAX(first, 0);
    _Py_BEGIN_SUPPRESS_IPH
#ifdef HAVE_CLOSE_RANGE
    if (close_range(first, last, 0) == 0) {
        /* close_range() ignores errors when it closes file descriptors.
         * Possible reasons of an error return are lack of kernel support
         * or denial of the underlying syscall by a seccomp sandbox on Linux.
         * Fallback to other methods in case of any error. */
    }
    else
#endif /* HAVE_CLOSE_RANGE */
#ifdef USE_CLOSEFROM
    if (last >= sysconf(_SC_OPEN_MAX)) {
        /* Any errors encountered while closing file descriptors are ignored */
        closefrom(first);
    }
    else
#endif /* USE_CLOSEFROM */
#ifdef USE_FDWALK
    {
        int lohi[2];
        lohi[0] = first;
        lohi[1] = last + 1;
        fdwalk(_fdwalk_close_func, lohi);
    }
#else
    {
        for (int i = first; i <= last; i++) {
            /* Ignore errors */
            (void)close(i);
        }
    }
#endif /* USE_FDWALK */
    _Py_END_SUPPRESS_IPH
}
