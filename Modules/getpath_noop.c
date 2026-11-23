/* Implements the getpath API for compiling with no functionality */

#include "Python.h"
#include "pycore_initconfig.h"    // _Py_Get_Getpath_CodeObject()
#include "pycore_pathconfig.h"    // _PyConfig_InitPathConfig()

PyStatus
_PyConfig_InitPathConfig(PyConfig *config, int compute_path_config)
{
    return PyStatus_Error("path configuration is unsupported");
}

/* Used by _testinternalcapi, which is linked into Programs/_freeze_module
   when extension modules are built in (MODULE_BUILDTYPE=static). */
PyObject *
_Py_Get_Getpath_CodeObject(void)
{
    PyErr_SetString(PyExc_RuntimeError,
                    "path configuration is unsupported");
    return NULL;
}
