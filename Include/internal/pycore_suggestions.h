#include "Python.h"

#ifndef Py_INTERNAL_SUGGESTIONS_H
#define Py_INTERNAL_SUGGESTIONS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

int _Py_offer_suggestions_for_attribute_error(PyAttributeErrorObject* exception_value);


#endif /* !Py_INTERNAL_SUGGESTIONS_H */