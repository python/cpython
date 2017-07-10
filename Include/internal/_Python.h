#ifndef _Py_PYTHON_H
#define _Py_PYTHON_H
/* Since this is a "meta-include" file, no #ifdef __cplusplus / extern "C" { */

/* Include all internal Python header files */

#ifndef Py_BUILD_CORE
#error "Internal headers are not available externally."
#endif

#include "_mem.h"
#include "_ceval.h"
#include "_warnings.h"
#include "_pystate.h"

#endif /* !_Py_PYTHON_H */
