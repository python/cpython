#include "Python.h"

#ifndef Py_INTERNAL_SUGGESTIONS_H
#define Py_INTERNAL_SUGGESTIONS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyObject* _Py_Offer_Suggestions(PyObject* exception);

#endif /* !Py_INTERNAL_SUGGESTIONS_H */
