#ifndef Py_INTERNAL_COMPLEXOBJECT_H
#define Py_INTERNAL_COMPLEXOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_unicodeobject.h" // _PyUnicodeWriter

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
extern int _PyComplex_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

// Operations on complex numbers.
PyAPI_FUNC(Py_complex) _Py_cr_sum(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_cr_diff(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_rc_diff(double, Py_complex);
PyAPI_FUNC(Py_complex) _Py_cr_prod(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_cr_quot(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_rc_quot(double, Py_complex);

static inline bool
_Py_c_isnan(Py_complex a)
{
    return isnan(a.real) || isnan(a.imag);
}

static inline bool
_Py_c_isinf(Py_complex a)
{
    return isinf(a.real) || isinf(a.imag);
}

static inline bool
_Py_c_isfinite(Py_complex a)
{
    return isfinite(a.real) && isfinite(a.imag);
}

static inline bool
_Py_c_iszero(Py_complex a)
{
    return a.real == 0.0 && a.imag == 0.0;
}

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_COMPLEXOBJECT_H
