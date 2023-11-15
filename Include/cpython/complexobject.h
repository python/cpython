#ifndef Py_CPYTHON_COMPLEXOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    double real;
    double imag;
} Py_complex;

// Operations on complex numbers.
PyAPI_FUNC(Py_complex) Py_complex_sum(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) Py_complex_diff(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) Py_complex_neg(Py_complex);
PyAPI_FUNC(Py_complex) Py_complex_prod(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) Py_complex_quot(Py_complex, Py_complex);
PyAPI_FUNC(Py_complex) Py_complex_pow(Py_complex, Py_complex);
PyAPI_FUNC(double) Py_complex_abs(Py_complex);

// Keep old Python 3.12 names as aliases to new functions
#define _Py_c_sum  Py_complex_sum
#define _Py_c_diff Py_complex_diff
#define _Py_c_neg  Py_complex_neg
#define _Py_c_prod Py_complex_prod
#define _Py_c_quot Py_complex_quot
#define _Py_c_pow  Py_complex_pow
#define _Py_c_abs  Py_complex_abs


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
