
#include "Python.h"

#include "pycore_initconfig.h"    // PyStatus()
#include "pycore_pylifecycle.h"   // _Py_InitCoreObjects()
#include "pycore_runtime.h"       // _PyRuntimeState
#include "pycore_pystate.h"       // _Py_IsMainInterpreter()
#include "pycore_typeobject.h"    // _PyType_Init()
#include "pycore_context.h"       // _PyContext_Init()

/**************************************
 * runtime-global type-related state
 **************************************/

PyStatus
_Py_RuntimeTypesStateInit(_PyRuntimeState *runtime)
{
    return _PyStatus_OK();
}

void
_Py_RuntimeTypesStateFini(_PyRuntimeState *runtime)
{
    // ...
}


/**************************************
 * per-interpreter "core" types
 **************************************/

// int, str, bytes, tuple, list, dict

static PyStatus
init_core_state(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

static PyStatus
init_core_types(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

static PyStatus
init_core_objects(PyInterpreterState *interp)
{
    PyStatus status;

    _PyLong_Init(interp);

    status = _PyBytes_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyUnicode_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyTuple_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

PyStatus
_Py_CoreObjectsInit(PyInterpreterState *interp)
{
    PyStatus status;

    status = init_core_state(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    /* Initializing these cannot rely on any of the core objects. */
    status = init_core_types(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = init_core_objects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

void
_Py_CoreObjectsFini(PyInterpreterState *interp)
{
    // Call _PyUnicode_ClearInterned() before _PyDict_Fini() since it uses
    // a dict internally.
    _PyUnicode_ClearInterned(interp);

    _PyDict_Fini(interp);
    _PyList_Fini(interp);
    _PyTuple_Fini(interp);
    _PyBytes_Fini(interp);
    _PyUnicode_Fini(interp);
}


/**************************************
 * full per-interpreter types
 **************************************/

static PyStatus
init_state(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

static PyStatus
init_types(PyInterpreterState *interp)
{
    PyStatus status;

    status = _PyType_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    int is_main_interp = _Py_IsMainInterpreter(interp);
    if (is_main_interp) {
        if (_PyStructSequence_Init() < 0) {
            return _PyStatus_ERR("can't initialize structseq");
        }

        status = _PyTypes_Init();
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        if (_PyLong_InitTypes() < 0) {
            return _PyStatus_ERR("can't init int type");
        }

        status = _PyUnicode_InitTypes();
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (is_main_interp) {
        if (_PyFloat_InitTypes() < 0) {
            return _PyStatus_ERR("can't init float");
        }
    }

    status = _PyExc_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyErr_InitTypes();
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    if (is_main_interp) {
        if (!_PyContext_Init()) {
            return _PyStatus_ERR("can't init context");
        }
    }
    return _PyStatus_OK();
}

static PyStatus
init_objects(PyInterpreterState *interp)
{
    PyStatus status;

    if (_Py_IsMainInterpreter(interp)) {
        _PyFloat_Init();
    }

    return _PyStatus_OK();
}

PyStatus
_Py_GlobalObjectsInit(PyInterpreterState *interp)
{
    PyStatus status;

    status = init_state(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = init_types(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = init_objects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

void
_Py_GlobalObjectsFini(PyInterpreterState *interp)
{
    _PyExc_Fini(interp);
    _PyFrame_Fini(interp);
    _PyAsyncGen_Fini(interp);
    _PyContext_Fini(interp);
    _PyType_Fini(interp);
    _PySlice_Fini(interp);
    _PyFloat_Fini(interp);
}
