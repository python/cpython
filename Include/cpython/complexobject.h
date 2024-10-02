#ifndef Py_CPYTHON_COMPLEXOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    double real;
    double imag;
} Py_complex;

// Operations on complex numbers.
PyAPI_FUNC(Py_complex) _Py_c_sum(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_cd_sum(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_c_diff(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_dc_diff(double, Py_complex);
PyAPI_FUNC(Py_complex) _Py_cd_diff(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_c_neg(Py_complex);
PyAPI_FUNC(Py_complex) _Py_c_prod(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_cd_prod(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_c_quot(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) _Py_dc_quot(double, Py_complex);
PyAPI_FUNC(Py_complex) _Py_cd_quot(Py_complex, double);
PyAPI_FUNC(Py_complex) _Py_c_pow(Py_complex, Py_complex);
PyAPI_FUNC(double) _Py_c_abs(Py_complex);


/* Complex object interface */

/*
PyComplexObject represents a complex number with double-precision
real and imaginary parts.
*/
typedef struct {
    PyObject_HEAD
    Py_complex cval;
} PyComplexObject;

PyAPI_FUNC(PyObject *) PyComplex_FromCComplex(Py_complex);

PyAPI_FUNC(Py_complex) PyComplex_AsCComplex(PyObject *op);
