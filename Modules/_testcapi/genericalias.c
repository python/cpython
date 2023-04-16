#include "parts.h"

#define Py_BUILD_CORE
// Needed to include both
// `Include/internal/pycore_gc.h` and
// `Include/cpython/objimpl.h`
#undef _PyGC_FINALIZED
#include "pycore_interp.h"  // PyInterpreterState

static PyObject *
genericalias_cache_clear(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyThreadState *tstate = PyThreadState_Get();
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    assert(interp != NULL);
    assert(interp->genericalias_cache != NULL);

    PyDict_Clear(interp->genericalias_cache);  // needs full PyInterpreterState

    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"genericalias_cache_clear", genericalias_cache_clear, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_GenericAlias(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
