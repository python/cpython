/* Peephole optimizations for bytecode compiler. */

#include "Python.h"

/* Retained for API compatibility.
 * Optimization is now done in the compiler */

PyObject *
PyCode_Optimize(PyObject *code, PyObject* consts, PyObject *names,
                PyObject *lnotab_obj)
{
    Py_INCREF(code);
    (void)consts;
    (void)names;
    (void)lnotab_obj;
    return code;
}
