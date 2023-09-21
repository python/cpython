#include "parts.h"
#include "clinic/complex.c.h"


/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

/*[clinic input]
_testcapi.complex_real_as_double

    op: object
    /

Test PyComplex_RealAsDouble().
[clinic start generated code]*/

static PyObject *
_testcapi_complex_real_as_double(PyObject *module, PyObject *op)
/*[clinic end generated code: output=91427770a4583095 input=7ffd3ffd53495b3d]*/
{
    double real = PyComplex_RealAsDouble(op);

    if (real == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyFloat_FromDouble(real);
}

/*[clinic input]
_testcapi.complex_imag_as_double

    op: object
    /

Test PyComplex_ImagAsDouble().
[clinic start generated code]*/

static PyObject *
_testcapi_complex_imag_as_double(PyObject *module, PyObject *op)
/*[clinic end generated code: output=f66f10a3d47beec4 input=e2b4b00761e141ea]*/
{
    double imag = PyComplex_ImagAsDouble(op);

    if (imag == -1. && PyErr_Occurred()) {
        return NULL;
    }

    return PyFloat_FromDouble(imag);
}

static PyMethodDef test_methods[] = {
    _TESTCAPI_COMPLEX_REAL_AS_DOUBLE_METHODDEF
    _TESTCAPI_COMPLEX_IMAG_AS_DOUBLE_METHODDEF
    {NULL},
};

int
_PyTestCapi_Init_Complex(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
