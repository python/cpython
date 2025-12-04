#include "pyconfig.h"   // Py_GIL_DISABLED

// Need limited C API version 3.5 for PyCodec_NameReplaceErrors()
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x03050000
#endif

#include "parts.h"

static PyObject *
codec_namereplace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_NameReplaceErrors(exc);
}

static PyMethodDef test_methods[] = {
    {"codec_namereplace_errors", codec_namereplace_errors, METH_O},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Codec(PyObject *module)
{
    if (PyModule_AddFunctions(module, test_methods) < 0) {
        return -1;
    }
    return 0;
}
