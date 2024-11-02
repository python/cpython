#ifndef Py_INTERNAL_CELL_H
#define Py_INTERNAL_CELL_H

#include "pycore_critical_section.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Sets the cell contents to `value` and return previous contents. Steals a
// reference to `value`.
static inline PyObject *
PyCell_SwapTakeRef(PyCellObject *cell, PyObject *value)
{
    PyObject *old_value;
    Py_BEGIN_CRITICAL_SECTION(cell);
    old_value = cell->ob_ref;
    cell->ob_ref = value;
    Py_END_CRITICAL_SECTION();
    return old_value;
}

static inline void
PyCell_SetTakeRef(PyCellObject *cell, PyObject *value)
{
    PyObject *old_value = PyCell_SwapTakeRef(cell, value);
    Py_XDECREF(old_value);
}

// Gets the cell contents. Returns a new reference.
static inline PyObject *
PyCell_GetRef(PyCellObject *cell)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(cell);
    res = Py_XNewRef(cell->ob_ref);
    Py_END_CRITICAL_SECTION();
    return res;
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_CELL_H */
