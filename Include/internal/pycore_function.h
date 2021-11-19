#ifndef Py_INTERNAL_FUNCTION_H
#define Py_INTERNAL_FUNCTION_H


#include "Python.h"

PyFunctionObject *
_PyFunction_FromConstructor(PyFrameConstructor *constr);


#endif /* !Py_INTERNAL_FUNCTION_H */
