#ifndef Py_INTERNAL_STATIC_BUILTIN_TYPES_H
#define Py_INTERNAL_STATIC_BUILTIN_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_exceptions.h"    // _Py_FOREACH_STATIC_EXCEPTION_TYPE


/* Single source of truth for every static builtin type registered at
   startup, partitioned by where it is registered.  Adding a
   registration anywhere in the tree requires adding an entry to the
   matching list; _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES then adjusts at
   compile time (gh-145497). */


#ifdef _Py_TIER2
#  define _Py_FOREACH_TIER2_PREINIT_TYPE(TYPE) TYPE(_PyUOpExecutor_Type)
#else
#  define _Py_FOREACH_TIER2_PREINIT_TYPE(TYPE)
#endif

/* Registered via static_types[] in Objects/object.c.
   Order matters: base types first; trailing subclasses last so
   _PyTypes_FiniTypes() tears them down before their base. */
#define _Py_FOREACH_STATIC_PREINIT_TYPE(TYPE) \
    TYPE(PyBaseObject_Type) \
    TYPE(PyType_Type) \
    TYPE(PyStaticMethod_Type) \
    TYPE(PyCFunction_Type) \
    \
    TYPE(PyAsyncGen_Type) \
    TYPE(PyByteArrayIter_Type) \
    TYPE(PyByteArray_Type) \
    TYPE(PyBytesIter_Type) \
    TYPE(PyBytes_Type) \
    TYPE(PyCallIter_Type) \
    TYPE(PyCapsule_Type) \
    TYPE(PyCell_Type) \
    TYPE(PyClassMethodDescr_Type) \
    TYPE(PyClassMethod_Type) \
    TYPE(PyCode_Type) \
    TYPE(PyComplex_Type) \
    TYPE(PyContextToken_Type) \
    TYPE(PyContextVar_Type) \
    TYPE(PyContext_Type) \
    TYPE(PyCoro_Type) \
    TYPE(PyDictItems_Type) \
    TYPE(PyDictIterItem_Type) \
    TYPE(PyDictIterKey_Type) \
    TYPE(PyDictIterValue_Type) \
    TYPE(PyDictKeys_Type) \
    TYPE(PyDictProxy_Type) \
    TYPE(PyDictRevIterItem_Type) \
    TYPE(PyDictRevIterKey_Type) \
    TYPE(PyDictRevIterValue_Type) \
    TYPE(PyDictValues_Type) \
    TYPE(PyDict_Type) \
    TYPE(PyEllipsis_Type) \
    TYPE(PyEnum_Type) \
    TYPE(PyFilter_Type) \
    TYPE(PyFloat_Type) \
    TYPE(PyFrameLocalsProxy_Type) \
    TYPE(PyFrame_Type) \
    TYPE(PyFrozenDict_Type) \
    TYPE(PyFrozenSet_Type) \
    TYPE(PyFunction_Type) \
    TYPE(PyGen_Type) \
    TYPE(PyGetSetDescr_Type) \
    TYPE(PyInstanceMethod_Type) \
    TYPE(PyLazyImport_Type) \
    TYPE(PyListIter_Type) \
    TYPE(PyListRevIter_Type) \
    TYPE(PyList_Type) \
    TYPE(PyLongRangeIter_Type) \
    TYPE(PyLong_Type) \
    TYPE(PyMap_Type) \
    TYPE(PyMemberDescr_Type) \
    TYPE(PyMemoryView_Type) \
    TYPE(PyMethodDescr_Type) \
    TYPE(PyMethod_Type) \
    TYPE(PyModuleDef_Type) \
    TYPE(PyModule_Type) \
    TYPE(PyODictIter_Type) \
    TYPE(PyPickleBuffer_Type) \
    TYPE(PyProperty_Type) \
    TYPE(PyRangeIter_Type) \
    TYPE(PyRange_Type) \
    TYPE(PyReversed_Type) \
    TYPE(PySTEntry_Type) \
    TYPE(PySentinel_Type) \
    TYPE(PySeqIter_Type) \
    TYPE(PySetIter_Type) \
    TYPE(PySet_Type) \
    TYPE(PySlice_Type) \
    TYPE(PyStdPrinter_Type) \
    TYPE(PySuper_Type) \
    TYPE(PyTraceBack_Type) \
    TYPE(PyTupleIter_Type) \
    TYPE(PyTuple_Type) \
    TYPE(PyUnicodeIter_Type) \
    TYPE(PyUnicode_Type) \
    TYPE(PyWrapperDescr_Type) \
    TYPE(PyZip_Type) \
    TYPE(Py_GenericAliasType) \
    TYPE(_PyAnextAwaitable_Type) \
    TYPE(_PyAsyncGenASend_Type) \
    TYPE(_PyAsyncGenAThrow_Type) \
    TYPE(_PyAsyncGenWrappedValue_Type) \
    TYPE(_PyBufferWrapper_Type) \
    TYPE(_PyContextTokenMissing_Type) \
    TYPE(_PyCoroWrapper_Type) \
    TYPE(_Py_GenericAliasIterType) \
    TYPE(_PyHamtItems_Type) \
    TYPE(_PyHamtKeys_Type) \
    TYPE(_PyHamtValues_Type) \
    TYPE(_PyHamt_ArrayNode_Type) \
    TYPE(_PyHamt_BitmapNode_Type) \
    TYPE(_PyHamt_CollisionNode_Type) \
    TYPE(_PyHamt_Type) \
    TYPE(_PyInstructionSequence_Type) \
    TYPE(_PyInterpolation_Type) \
    TYPE(_PyLegacyEventHandler_Type) \
    TYPE(_PyLineIterator) \
    TYPE(_PyManagedBuffer_Type) \
    TYPE(_PyMemoryIter_Type) \
    TYPE(_PyMethodWrapper_Type) \
    TYPE(_PyNamespace_Type) \
    TYPE(_PyNone_Type) \
    TYPE(_PyNotImplemented_Type) \
    TYPE(_PyPositionsIterator) \
    TYPE(_PyTemplate_Type) \
    TYPE(_PyTemplateIter_Type) \
    TYPE(_PyUnicodeASCIIIter_Type) \
    TYPE(_PyUnion_Type) \
    _Py_FOREACH_TIER2_PREINIT_TYPE(TYPE) \
    TYPE(_PyWeakref_CallableProxyType) \
    TYPE(_PyWeakref_ProxyType) \
    TYPE(_PyWeakref_RefType) \
    TYPE(_PyTypeAlias_Type) \
    TYPE(_PyNoDefault_Type) \
    \
    TYPE(PyBool_Type) \
    TYPE(PyCMethod_Type) \
    TYPE(PyODictItems_Type) \
    TYPE(PyODictKeys_Type) \
    TYPE(PyODictValues_Type) \
    TYPE(PyODict_Type)


/* One TYPE(name) per textual _PyStaticType_InitBuiltin /
   _PyStructSequence_InitBuiltin{,WithFlags} call site outside the two
   arrays above.  Count-only -- the calls themselves stay at their
   existing locations.  Forgetting an entry trips the
   `index < _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES` assertion in
   Objects/typeobject.c on interpreter start. */
#define _Py_FOREACH_STATIC_EXTRA_TYPE(TYPE) \
    /* Python/crossinterp_exceptions.h */ \
    TYPE(_PyExc_InterpreterError) \
    TYPE(_PyExc_InterpreterNotFoundError) \
    /* Objects/unicodeobject.c */ \
    TYPE(EncodingMapType) \
    TYPE(PyFieldNameIter_Type) \
    TYPE(PyFormatterIter_Type) \
    /* Python/thread.c */ \
    TYPE(ThreadInfoType) \
    /* Python/sysmodule.c */ \
    TYPE(Hash_InfoType) \
    TYPE(AsyncGenHooksType) \
    TYPE(VersionInfoType) \
    TYPE(FlagsType) \
    TYPE(WindowsVersionType) \
    /* Python/errors.c */ \
    TYPE(UnraisableHookArgsType) \
    /* Objects/longobject.c */ \
    TYPE(Int_InfoType) \
    /* Objects/floatobject.c */ \
    TYPE(FloatInfoType)


/* Variadic so it accepts both single-arg X(name) and two-arg X(name, str). */
#define _PY_COUNT_STATIC_TYPE_(...) + 1

#define _Py_NUM_MANAGED_PREINITIALIZED_TYPES \
    (0 _Py_FOREACH_STATIC_PREINIT_TYPE(_PY_COUNT_STATIC_TYPE_))

#define _Py_NUM_MANAGED_STATIC_EXCEPTION_TYPES        \
    (0 _Py_FOREACH_STATIC_EXCEPTION_TYPE(_PY_COUNT_STATIC_TYPE_) \
       _Py_FOREACH_RENAMED_EXCEPTION_TYPE(_PY_COUNT_STATIC_TYPE_))

#define _Py_NUM_MANAGED_STATIC_EXTRA_TYPES \
    (0 _Py_FOREACH_STATIC_EXTRA_TYPE(_PY_COUNT_STATIC_TYPE_))

#define _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES   \
    (_Py_NUM_MANAGED_PREINITIALIZED_TYPES +    \
     _Py_NUM_MANAGED_STATIC_EXCEPTION_TYPES +  \
     _Py_NUM_MANAGED_STATIC_EXTRA_TYPES)

#define _Py_MAX_MANAGED_STATIC_EXT_TYPES 10

#define _Py_MAX_MANAGED_STATIC_TYPES \
    (_Py_MAX_MANAGED_STATIC_BUILTIN_TYPES + _Py_MAX_MANAGED_STATIC_EXT_TYPES)


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STATIC_BUILTIN_TYPES_H */
