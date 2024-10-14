#ifndef Py_INTERNAL_UNICODEOBJECT_H
#define Py_INTERNAL_UNICODEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_fileutils.h"     // _Py_error_handler
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI

void _PyUnicode_ExactDealloc(PyObject *op);
Py_ssize_t _PyUnicode_InternedSize(void);
Py_ssize_t _PyUnicode_InternedSize_Immortal(void);

/* runtime lifecycle */

extern void _PyUnicode_InitState(PyInterpreterState *);
extern PyStatus _PyUnicode_InitGlobalObjects(PyInterpreterState *);
extern PyStatus _PyUnicode_InitInternDict(PyInterpreterState *);
extern PyStatus _PyUnicode_InitTypes(PyInterpreterState *);
extern void _PyUnicode_Fini(PyInterpreterState *);
extern void _PyUnicode_FiniTypes(PyInterpreterState *);

extern PyTypeObject _PyUnicodeASCIIIter_Type;

/* Interning */

// All these are "ref-neutral", like the public PyUnicode_InternInPlace.

// Explicit interning routines:
PyAPI_FUNC(void) _PyUnicode_InternMortal(PyInterpreterState *interp, PyObject **);
PyAPI_FUNC(void) _PyUnicode_InternImmortal(PyInterpreterState *interp, PyObject **);
// Left here to help backporting:
PyAPI_FUNC(void) _PyUnicode_InternInPlace(PyInterpreterState *interp, PyObject **p);
// Only for statically allocated strings:
extern void _PyUnicode_InternStatic(PyInterpreterState *interp, PyObject **);

/* other API */

struct _Py_unicode_runtime_ids {
    PyThread_type_lock lock;
    // next_index value must be preserved when Py_Initialize()/Py_Finalize()
    // is called multiple times: see _PyUnicode_FromId() implementation.
    Py_ssize_t next_index;
};

struct _Py_unicode_runtime_state {
    struct _Py_unicode_runtime_ids ids;
};

/* fs_codec.encoding is initialized to NULL.
   Later, it is set to a non-NULL string by _PyUnicode_InitEncodings(). */
struct _Py_unicode_fs_codec {
    char *encoding;   // Filesystem encoding (encoded to UTF-8)
    int utf8;         // encoding=="utf-8"?
    char *errors;     // Filesystem errors (encoded to UTF-8)
    _Py_error_handler error_handler;
};

struct _Py_unicode_ids {
    Py_ssize_t size;
    PyObject **array;
};

struct _Py_unicode_state {
    struct _Py_unicode_fs_codec fs_codec;

    _PyUnicode_Name_CAPI *ucnhash_capi;

    // Unicode identifiers (_Py_Identifier): see _PyUnicode_FromId()
    struct _Py_unicode_ids ids;
};

extern void _PyUnicode_ClearInterned(PyInterpreterState *interp);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_UNICODEOBJECT_H */
