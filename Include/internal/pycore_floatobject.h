#ifndef Py_INTERNAL_FLOATOBJECT_H
#define Py_INTERNAL_FLOATOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"      // _PyFreeListState
#include "pycore_unicodeobject.h" // _PyUnicodeWriter

/* runtime lifecycle */

extern void _PyFloat_InitState(PyInterpreterState *);
extern PyStatus _PyFloat_InitTypes(PyInterpreterState *);
extern void _PyFloat_FiniType(PyInterpreterState *);


/* other API */

enum _py_float_format_type {
    _py_float_format_unknown,
    _py_float_format_ieee_big_endian,
    _py_float_format_ieee_little_endian,
};

struct _Py_float_runtime_state {
    enum _py_float_format_type float_format;
    enum _py_float_format_type double_format;
};




PyAPI_FUNC(void) _PyFloat_ExactDealloc(PyObject *op);


extern void _PyFloat_DebugMallocStats(FILE* out);


/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyFloat_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

extern PyObject* _Py_string_to_number_with_underscores(
    const char *str, Py_ssize_t len, const char *what, PyObject *obj, void *arg,
    PyObject *(*innerfunc)(const char *, Py_ssize_t, void *));

extern double _Py_parse_inf_or_nan(const char *p, char **endptr);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FLOATOBJECT_H */
