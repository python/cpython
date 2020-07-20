/* Peephole optimizations for bytecode compiler. */

#include "Python.h"

/* Retained for API compatibility.
 * Optimization is now done in the compiler */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* Py_UNUSED(consts),
                PyObject *Py_UNUSED(names), PyObject *Py_UNUSED(lnotab_obj))
{
    Py_INCREF(code);
    return code;
}
