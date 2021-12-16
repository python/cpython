#ifndef Py_INTERNAL_UNICODEOBJECT_H
#define Py_INTERNAL_UNICODEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern void _PyUnicode_InitState(PyInterpreterState *);
extern PyStatus _PyUnicode_InitGlobalObjects(PyInterpreterState *);
extern PyStatus _PyUnicode_InitTypes(PyInterpreterState *);
extern void _PyUnicode_Fini(PyInterpreterState *);


/* other API */

/* fs_codec.encoding is initialized to NULL.
   Later, it is set to a non-NULL string by _PyUnicode_InitEncodings(). */
struct _Py_unicode_fs_codec {
    char *encoding;   // Filesystem encoding (encoded to UTF-8)
    int utf8;         // encoding=="utf-8"?
    char *errors;     // Filesystem errors (encoded to UTF-8)
    _Py_error_handler error_handler;
};

struct _Py_unicode_state {
    // The empty Unicode object is a singleton to improve performance.
    PyObject *empty_string;
    /* Single character Unicode strings in the Latin-1 range are being
       shared as well. */
    PyObject *latin1[256];
    struct _Py_unicode_fs_codec fs_codec;
};

extern void _PyUnicode_ClearInterned(PyInterpreterState *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEOBJECT_H */
