#ifndef Py_BYTES_CTYPE_H
#define Py_BYTES_CTYPE_H

/*
 * The internal implementation behind PyString (bytes) and PyBytes (buffer)
 * methods of the given names, they operate on ASCII byte strings.
 */
extern PyObject* _Py_bytes_isspace(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_isalpha(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_isalnum(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_isdigit(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_islower(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_isupper(const char *cptr, Py_ssize_t len);
extern PyObject* _Py_bytes_istitle(const char *cptr, Py_ssize_t len);

/* These store their len sized answer in the given preallocated *result arg. */
extern void _Py_bytes_lower(char *result, const char *cptr, Py_ssize_t len);
extern void _Py_bytes_upper(char *result, const char *cptr, Py_ssize_t len);
extern void _Py_bytes_title(char *result, char *s, Py_ssize_t len);
extern void _Py_bytes_capitalize(char *result, char *s, Py_ssize_t len);
extern void _Py_bytes_swapcase(char *result, char *s, Py_ssize_t len);

/* Shared __doc__ strings. */
extern const char _Py_isspace__doc__[];
extern const char _Py_isalpha__doc__[];
extern const char _Py_isalnum__doc__[];
extern const char _Py_isdigit__doc__[];
extern const char _Py_islower__doc__[];
extern const char _Py_isupper__doc__[];
extern const char _Py_istitle__doc__[];
extern const char _Py_lower__doc__[];
extern const char _Py_upper__doc__[];
extern const char _Py_title__doc__[];
extern const char _Py_capitalize__doc__[];
extern const char _Py_swapcase__doc__[];

#define FLAG_LOWER  0x01
#define FLAG_UPPER  0x02
#define FLAG_ALPHA  (FLAG_LOWER|FLAG_UPPER)
#define FLAG_DIGIT  0x04
#define FLAG_ALNUM  (FLAG_ALPHA|FLAG_DIGIT)
#define FLAG_SPACE  0x08
#define FLAG_XDIGIT 0x10

extern const unsigned int _Py_ctype_table[256];

#define ISLOWER(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_LOWER)
#define ISUPPER(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_UPPER)
#define ISALPHA(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_ALPHA)
#define ISDIGIT(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_DIGIT)
#define ISXDIGIT(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_XDIGIT)
#define ISALNUM(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_ALNUM)
#define ISSPACE(c) (_Py_ctype_table[Py_CHARMASK(c)] & FLAG_SPACE)

#undef islower
#define islower(c) undefined_islower(c)
#undef isupper
#define isupper(c) undefined_isupper(c)
#undef isalpha
#define isalpha(c) undefined_isalpha(c)
#undef isdigit
#define isdigit(c) undefined_isdigit(c)
#undef isxdigit
#define isxdigit(c) undefined_isxdigit(c)
#undef isalnum
#define isalnum(c) undefined_isalnum(c)
#undef isspace
#define isspace(c) undefined_isspace(c)

extern const unsigned char _Py_ctype_tolower[256];
extern const unsigned char _Py_ctype_toupper[256];

#define TOLOWER(c) (_Py_ctype_tolower[Py_CHARMASK(c)])
#define TOUPPER(c) (_Py_ctype_toupper[Py_CHARMASK(c)])

#undef tolower
#define tolower(c) undefined_tolower(c)
#undef toupper
#define toupper(c) undefined_toupper(c)

/* this is needed because some docs are shared from the .o, not static */
#define PyDoc_STRVAR_shared(name,str) const char name[] = PyDoc_STR(str)

#endif /* !Py_BYTES_CTYPE_H */
