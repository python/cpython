#ifndef Py_INTERNAL_BYTESOBJECT_H
#define Py_INTERNAL_BYTESOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern PyStatus _PyBytes_InitTypes(PyInterpreterState *);

/* Substring Search */

PyAPI_FUNC(Py_ssize_t)
_PyBytes_Find(const char *haystack, Py_ssize_t len_haystack,
              const char *needle, Py_ssize_t len_needle,
              Py_ssize_t offset);

PyAPI_FUNC(Py_ssize_t)
_PyBytes_ReverseFind(const char *haystack, Py_ssize_t len_haystack,
                     const char *needle, Py_ssize_t len_needle,
                     Py_ssize_t offset);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BYTESOBJECT_H */
