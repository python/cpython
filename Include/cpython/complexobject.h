#ifndef Py_CPYTHON_COMPLEXOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    double real;
    double imag;
} Py_complex;

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
