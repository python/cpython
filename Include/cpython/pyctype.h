#ifndef Py_LIMITED_API
#ifndef PYCTYPE_H
#define PYCTYPE_H
#ifdef __cplusplus
extern "C" {
#endif

#define PY_CTF_LOWER  0x01
#define PY_CTF_UPPER  0x02
#define PY_CTF_ALPHA  (PY_CTF_LOWER|PY_CTF_UPPER)
#define PY_CTF_DIGIT  0x04
#define PY_CTF_ALNUM  (PY_CTF_ALPHA|PY_CTF_DIGIT)
#define PY_CTF_SPACE  0x08
#define PY_CTF_XDIGIT 0x10

PyAPI_DATA(const unsigned int) _Py_ctype_table[256];

/* Unlike their C counterparts, the following macros are not meant to
 * handle an int with any of the values [EOF, 0-UCHAR_MAX]. The argument
 * must be a signed/unsigned char. */
static inline int Py_ISLOWER(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_LOWER);
}
static inline int Py_ISUPPER(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_UPPER);
}
static inline int Py_ISALPHA(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_ALPHA);
}
static inline int Py_ISDIGIT(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_DIGIT);
}
static inline int Py_ISXDIGIT(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_XDIGIT);
}
static inline int Py_ISALNUM(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_ALNUM);
}
static inline int Py_ISSPACE(int c) {
    return (_Py_ctype_table[Py_CHARMASK(c)] & PY_CTF_SPACE);
}

PyAPI_DATA(const unsigned char) _Py_ctype_tolower[256];
PyAPI_DATA(const unsigned char) _Py_ctype_toupper[256];

static inline int Py_TOLOWER(int c) {
    return _Py_ctype_tolower[Py_CHARMASK(c)];
}
static inline int Py_TOUPPER(int c) {
    return _Py_ctype_toupper[Py_CHARMASK(c)];
}

#ifdef __cplusplus
}
#endif
#endif /* !PYCTYPE_H */
#endif /* !Py_LIMITED_API */
