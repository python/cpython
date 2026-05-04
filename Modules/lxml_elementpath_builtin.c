/*
 * lxml_elementpath_builtin.c - Shim for lxml._elementpath built-in module.
 *
 * Registers the Cython _elementpath extension under the flat name
 * "_lxml_elementpath" for Modules/Setup.local.
 */

#include "Python.h"

extern PyObject* PyInit__elementpath(void);

PyMODINIT_FUNC
PyInit__lxml_elementpath(void)
{
    return PyInit__elementpath();
}
