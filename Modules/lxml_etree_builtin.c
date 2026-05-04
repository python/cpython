/*
 * lxml_etree_builtin.c - Shim to register lxml.etree as a CPython built-in.
 *
 * makesetup does not support dotted module names, so the Cython extension
 * is registered under the flat name "_lxml_etree". A pure-Python shim at
 * lxml/etree.py re-exports everything via `from _lxml_etree import *`.
 *
 * The Cython-generated code in liblxml_etree.a exports PyInit_etree.
 * This wrapper provides PyInit__lxml_etree so the name matches the
 * entry in Modules/Setup.local.
 */

#include "Python.h"

extern PyObject* PyInit_etree(void);

PyMODINIT_FUNC
PyInit__lxml_etree(void)
{
    return PyInit_etree();
}
