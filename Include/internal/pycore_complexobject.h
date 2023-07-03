#ifndef Py_INTERNAL_COMPLEXOBJECT_H
#define Py_INTERNAL_COMPLEXOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_unicodeobject.h" // _PyUnicodeWriter

/* Operations on complex numbers from complexmodule.c */

PyAPI_FUNC(Py_complex) _Py_c_sum(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_diff(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_neg(Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_prod(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_quot(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_pow(Py_complex, Py_complex);
PyAPI_FUNC(double) _Py_c_abs(Py_complex);

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyComplex_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_COMPLEXOBJECT_H
