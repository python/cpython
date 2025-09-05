#ifndef Py_INTERNAL_FLOATOBJECT_H
#define Py_INTERNAL_FLOATOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_unicodeobject.h" // _PyUnicodeWriter

/* runtime lifecycle */

extern void _PyFloat_InitState(PyInterpreterState *);
extern PyStatus _PyFloat_InitTypes(PyInterpreterState *);
extern void _PyFloat_FiniType(PyInterpreterState *);




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

extern int _Py_convert_int_to_double(PyObject **v, double *dbl);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FLOATOBJECT_H */
