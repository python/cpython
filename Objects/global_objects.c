
#include "Python.h"

#include "pycore_global_objects.h"  // PyStatus
#include "pycore_initconfig.h"    // PyStatus
#include "pycore_pylifecycle.h"   // _Py_InitCoreObjects()
#include "pycore_runtime.h"       // _PyRuntimeState
#include "pycore_pystate.h"       // _Py_IsMainInterpreter()
#include "pycore_typeobject.h"    // _PyType_Init()
#include "pycore_long.h"          // _PyLong_InitCoreObjects()
#include "pycore_bytesobject.h"   // _PyBytes_InitCoreObjects()
#include "pycore_unicodeobject.h" // _PyUnicode_InitCoreObjects()
#include "pycore_tuple.h"         // _PyTuple_InitCoreObjects()
#include "pycore_dict.h"          // _PyDict_Fini()
#include "pycore_list.h"          // _PyList_Fini()
#include "pycore_structseq.h"     // _PyStructSequence_Init()
#include "pycore_floatobject.h"   // _PyFloat_InitTypes()
#include "pycore_frame.h"         // _PyFrame_Fini()
#include "pycore_sliceobject.h"   // _PySlice_Fini()
#include "pycore_genobject.h"     // _PyAsyncGen_Fini()
#include "pycore_context.h"       // _PyContext_Init()
#include "pycore_hamt.h"          // _PyHamt_Init()
#include "pycore_namespace.h"     // _PyNamespace_Type
#include "pycore_symtable.h"      // PySTEntry_Type
#include "pycore_unionobject.h"   // _PyUnion_Type
#include "frameobject.h"          // PyFrame_Type
#include "pycore_pyerrors.h"      // _PyErr_InitTypes()
#include "pycore_interpreteridobject.h"  // _PyInterpreterID_Type


/**************************************
 * runtime-global type-related state
 **************************************/

PyStatus
_PyRuntimeState_TypesInit(_PyRuntimeState *runtime)
{
    PyStatus status;

    status = _PyUnicode_InitRuntimeState(runtime);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyFloat_InitRuntimeState(runtime);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

void
_PyRuntimeState_TypesFini(_PyRuntimeState *runtime)
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

    status = _PyLong_InitCoreObjects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyBytes_InitCoreObjects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyUnicode_InitCoreObjects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyTuple_InitCoreObjects(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

PyStatus
_PyInterpreterState_CoreObjectsInit(PyInterpreterState *interp)
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
_PyInterpreterState_CoreObjectsFini(PyInterpreterState *interp)
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
    PyStatus status;

    status = _PyStructSequence_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

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

    // XXX Init per-interpreter.
#define INIT_TYPE(TYPE) \
    if (PyType_Ready(&(TYPE)) < 0) { \
        return _PyStatus_ERR("Can't initialize " #TYPE " type"); \
    }

    // Base types:
    INIT_TYPE(PyBaseObject_Type);
    INIT_TYPE(PyType_Type);
    assert(PyBaseObject_Type.tp_base == NULL);
    assert(PyType_Type.tp_base == &PyBaseObject_Type);

    // All other static types (in alphabetical order):
    INIT_TYPE(PyAsyncGen_Type);
    INIT_TYPE(PyBool_Type);
    INIT_TYPE(PyByteArrayIter_Type);
    INIT_TYPE(PyByteArray_Type);
    INIT_TYPE(PyBytesIter_Type);
    INIT_TYPE(PyBytes_Type);
    INIT_TYPE(PyCFunction_Type);
    INIT_TYPE(PyCMethod_Type);
    INIT_TYPE(PyCallIter_Type);
    INIT_TYPE(PyCapsule_Type);
    INIT_TYPE(PyCell_Type);
    INIT_TYPE(PyClassMethodDescr_Type);
    INIT_TYPE(PyClassMethod_Type);
    INIT_TYPE(PyCode_Type);
    INIT_TYPE(PyComplex_Type);
    INIT_TYPE(PyCoro_Type);
    INIT_TYPE(PyDictItems_Type);
    INIT_TYPE(PyDictIterItem_Type);
    INIT_TYPE(PyDictIterKey_Type);
    INIT_TYPE(PyDictIterValue_Type);
    INIT_TYPE(PyDictKeys_Type);
    INIT_TYPE(PyDictProxy_Type);
    INIT_TYPE(PyDictRevIterItem_Type);
    INIT_TYPE(PyDictRevIterKey_Type);
    INIT_TYPE(PyDictRevIterValue_Type);
    INIT_TYPE(PyDictValues_Type);
    INIT_TYPE(PyDict_Type);
    INIT_TYPE(PyEllipsis_Type);
    INIT_TYPE(PyEnum_Type);
    INIT_TYPE(PyFloat_Type);
    INIT_TYPE(PyFrame_Type);
    INIT_TYPE(PyFrozenSet_Type);
    INIT_TYPE(PyFunction_Type);
    INIT_TYPE(PyGen_Type);
    INIT_TYPE(PyGetSetDescr_Type);
    INIT_TYPE(PyInstanceMethod_Type);
    INIT_TYPE(PyListIter_Type);
    INIT_TYPE(PyListRevIter_Type);
    INIT_TYPE(PyList_Type);
    INIT_TYPE(PyLongRangeIter_Type);
    INIT_TYPE(PyLong_Type);
    INIT_TYPE(PyMemberDescr_Type);
    INIT_TYPE(PyMemoryView_Type);
    INIT_TYPE(PyMethodDescr_Type);
    INIT_TYPE(PyMethod_Type);
    INIT_TYPE(PyModuleDef_Type);
    INIT_TYPE(PyModule_Type);
    INIT_TYPE(PyODictItems_Type);
    INIT_TYPE(PyODictIter_Type);
    INIT_TYPE(PyODictKeys_Type);
    INIT_TYPE(PyODictValues_Type);
    INIT_TYPE(PyODict_Type);
    INIT_TYPE(PyPickleBuffer_Type);
    INIT_TYPE(PyProperty_Type);
    INIT_TYPE(PyRangeIter_Type);
    INIT_TYPE(PyRange_Type);
    INIT_TYPE(PyReversed_Type);
    INIT_TYPE(PySTEntry_Type);
    INIT_TYPE(PySeqIter_Type);
    INIT_TYPE(PySetIter_Type);
    INIT_TYPE(PySet_Type);
    INIT_TYPE(PySlice_Type);
    INIT_TYPE(PyStaticMethod_Type);
    INIT_TYPE(PyStdPrinter_Type);
    INIT_TYPE(PySuper_Type);
    INIT_TYPE(PyTraceBack_Type);
    INIT_TYPE(PyTupleIter_Type);
    INIT_TYPE(PyTuple_Type);
    INIT_TYPE(PyUnicodeIter_Type);
    INIT_TYPE(PyUnicode_Type);
    INIT_TYPE(PyWrapperDescr_Type);
    INIT_TYPE(Py_GenericAliasType);
    INIT_TYPE(_PyAnextAwaitable_Type);
    INIT_TYPE(_PyAsyncGenASend_Type);
    INIT_TYPE(_PyAsyncGenAThrow_Type);
    INIT_TYPE(_PyAsyncGenWrappedValue_Type);
    INIT_TYPE(_PyCoroWrapper_Type);
    INIT_TYPE(_PyInterpreterID_Type);
    INIT_TYPE(_PyManagedBuffer_Type);
    INIT_TYPE(_PyMethodWrapper_Type);
    INIT_TYPE(_PyNamespace_Type);
    INIT_TYPE(_PyNone_Type);
    INIT_TYPE(_PyNotImplemented_Type);
    INIT_TYPE(_PyUnion_Type);
    INIT_TYPE(_PyWeakref_CallableProxyType);
    INIT_TYPE(_PyWeakref_ProxyType);
    INIT_TYPE(_PyWeakref_RefType);
#undef INIT_TYPE

    status = _PyLong_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyUnicode_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyFloat_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyExc_Init(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyErr_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyHamt_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    status = _PyContext_InitTypes(interp);
    if (_PyStatus_EXCEPTION(status)) {
        return status;
    }

    return _PyStatus_OK();
}

static PyStatus
init_objects(PyInterpreterState *interp)
{
    return _PyStatus_OK();
}

PyStatus
_PyInterpreterState_ObjectsInit(PyInterpreterState *interp)
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
_PyInterpreterState_ObjectsFini(PyInterpreterState *interp)
{
    _PyExc_Fini(interp);
    _PyFrame_Fini(interp);
    _PyAsyncGen_Fini(interp);
    _PyContext_Fini(interp);
    _PyHamt_Fini(interp);
    _PyType_Fini(interp);
    _PySlice_Fini(interp);
    _PyFloat_Fini(interp);
}
