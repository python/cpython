#ifndef COMPLEXOBJECT_H
#define COMPLEXOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/* Complex number structure */

typedef struct {
   double real;
   double imag;
} complex;

/* Operations on complex numbers from complexmodule.c */

#define c_sum _Py_c_sum
#define c_diff _Py_c_diff
#define c_neg _Py_c_neg
#define c_prod _Py_c_prod
#define c_quot _Py_c_quot
#define c_pow _Py_c_pow

extern complex c_sum();
extern complex c_diff();
extern complex c_neg();
extern complex c_prod();
extern complex c_quot();
extern complex c_pow();


/* Complex object interface */

/*
PyComplexObject represents a complex number with double-precision
real and imaginary parts.
*/

typedef struct {
	PyObject_HEAD
	complex cval;
} PyComplexObject;     

extern DL_IMPORT(PyTypeObject) PyComplex_Type;

#define PyComplex_Check(op) ((op)->ob_type == &PyComplex_Type)

extern PyObject *PyComplex_FromCComplex Py_PROTO((complex));
extern PyObject *PyComplex_FromDoubles Py_PROTO((double real, double imag));

extern double PyComplex_RealAsDouble Py_PROTO((PyObject *op));
extern double PyComplex_ImagAsDouble Py_PROTO((PyObject *op));
extern complex PyComplex_AsCComplex Py_PROTO((PyObject *op));

#ifdef __cplusplus
}
#endif
#endif /* !COMPLEXOBJECT_H */
