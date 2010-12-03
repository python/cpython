#ifndef Py_LIMITED_API
#ifndef Py_BYTES_CTYPE_H
#define Py_BYTES_CTYPE_H

/*
 * The internal implementation behind PyBytes (bytes) and PyByteArray (bytearray)
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

/* This one gets the raw argument list. */
extern PyObject* _Py_bytes_maketrans(PyObject *args);

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
extern const char _Py_maketrans__doc__[];

/* this is needed because some docs are shared from the .o, not static */
#define PyDoc_STRVAR_shared(name,str) const char name[] = PyDoc_STR(str)

#endif /* !Py_BYTES_CTYPE_H */
#endif /* !Py_LIMITED_API */
