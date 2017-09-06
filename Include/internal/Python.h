#ifndef Py_INTERNAL_PYTHON_H
#define Py_INTERNAL_PYTHON_H
/* Since this is a "meta-include" file, no #ifdef __cplusplus / extern "C" { */

/* Include all internal Python header files */

#ifndef Py_BUILD_CORE
#error "Internal headers are not available externally."
#endif

#include "internal/mem.h"
#include "internal/ceval.h"
#include "internal/warnings.h"
#include "internal/pystate.h"
#include "internal/runtime.h"

#endif /* !Py_INTERNAL_PYTHON_H */
