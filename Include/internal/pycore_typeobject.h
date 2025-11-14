#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_interp_structs.h" // managed_static_type_state
#include "pycore_moduleobject.h"  // PyModuleObject


/* state */

#define _Py_TYPE_VERSION_INT 1
#define _Py_TYPE_VERSION_FLOAT 2
#define _Py_TYPE_VERSION_LIST 3
#define _Py_TYPE_VERSION_TUPLE 4
#define _Py_TYPE_VERSION_STR 5
#define _Py_TYPE_VERSION_SET 6
#define _Py_TYPE_VERSION_FROZEN_SET 7
#define _Py_TYPE_VERSION_DICT 8
#define _Py_TYPE_VERSION_BYTEARRAY 9
#define _Py_TYPE_VERSION_BYTES 10
#define _Py_TYPE_VERSION_COMPLEX 11

#define _Py_TYPE_VERSION_NEXT 16


#define _Py_TYPE_BASE_VERSION_TAG (2<<16)
#define _Py_MAX_GLOBAL_TYPE_VERSION_TAG (_Py_TYPE_BASE_VERSION_TAG - 1)


/* runtime lifecycle */

extern PyStatus _PyTypes_InitTypes(PyInterpreterState *);
extern void _PyTypes_FiniTypes(PyInterpreterState *);
extern void _PyTypes_FiniExtTypes(PyInterpreterState *interp);
extern void _PyTypes_Fini(PyInterpreterState *);
extern void _PyTypes_AfterFork(void);
extern void _PyTypes_FiniCachedDescriptors(PyInterpreterState *);

static inline PyObject **
_PyStaticType_GET_WEAKREFS_LISTPTR(managed_static_type_state *state)
{
    assert(state != NULL);
    return &state->tp_weaklist;
}

extern int _PyStaticType_InitBuiltin(
    PyInterpreterState *interp,
    PyTypeObject *type);
extern void _PyStaticType_FiniBuiltin(
    PyInterpreterState *interp,
    PyTypeObject *type);
extern void _PyStaticType_ClearWeakRefs(
    PyInterpreterState *interp,
    PyTypeObject *type);
extern managed_static_type_state * _PyStaticType_GetState(
    PyInterpreterState *interp,
    PyTypeObject *type);

// Export for '_datetime' shared extension.
PyAPI_FUNC(int) _PyStaticType_InitForExtension(
    PyInterpreterState *interp,
     PyTypeObject *self);

// Export for _testinternalcapi extension.
PyAPI_FUNC(PyObject *) _PyStaticType_GetBuiltins(void);


/* Like PyType_GetModuleState, but skips verification
 * that type is a heap type with an associated module */
static inline void *
_PyType_GetModuleState(PyTypeObject *type)
{
    assert(PyType_Check(type));
    assert(type->tp_flags & Py_TPFLAGS_HEAPTYPE);
    PyHeapTypeObject *et = (PyHeapTypeObject *)type;
    assert(et->ht_module);
    PyModuleObject *mod = (PyModuleObject *)(et->ht_module);
    assert(mod != NULL);
    return mod->md_state;
}


// Export for 'math' shared extension, used via _PyType_IsReady() static inline
// function
PyAPI_FUNC(PyObject *) _PyType_GetDict(PyTypeObject *);

PyAPI_FUNC(PyObject *) _PyType_LookupSubclasses(PyTypeObject *);
PyAPI_FUNC(PyObject *) _PyType_InitSubclasses(PyTypeObject *);

extern PyObject * _PyType_GetBases(PyTypeObject *type);
extern PyObject * _PyType_GetMRO(PyTypeObject *type);
extern PyObject* _PyType_GetSubclasses(PyTypeObject *);
extern int _PyType_HasSubclasses(PyTypeObject *);

// Export for _testinternalcapi extension.
PyAPI_FUNC(PyObject *) _PyType_GetSlotWrapperNames(void);

// PyType_Ready() must be called if _PyType_IsReady() is false.
// See also the Py_TPFLAGS_READY flag.
static inline int
_PyType_IsReady(PyTypeObject *type)
{
    return _PyType_GetDict(type) != NULL;
}

extern PyObject* _Py_type_getattro_impl(PyTypeObject *type, PyObject *name,
                                        int *suppress_missing_attribute);
extern PyObject* _Py_type_getattro(PyObject *type, PyObject *name);

extern PyObject* _Py_BaseObject_RichCompare(PyObject* self, PyObject* other, int op);

extern PyObject* _Py_slot_tp_getattro(PyObject *self, PyObject *name);
extern PyObject* _Py_slot_tp_getattr_hook(PyObject *self, PyObject *name);

extern PyTypeObject _PyBufferWrapper_Type;

PyAPI_FUNC(PyObject*) _PySuper_Lookup(PyTypeObject *su_type, PyObject *su_obj,
                                 PyObject *name, int *meth_found);

extern PyObject* _PyType_GetFullyQualifiedName(PyTypeObject *type, char sep);

// Perform the following operation, in a thread-safe way when required by the
// build mode.
//
// self->tp_flags = (self->tp_flags & ~mask) | flags;
extern void _PyType_SetFlags(PyTypeObject *self, unsigned long mask,
                             unsigned long flags);
extern int _PyType_AddMethod(PyTypeObject *, PyMethodDef *);

// Like _PyType_SetFlags(), but apply the operation to self and any of its
// subclasses without Py_TPFLAGS_IMMUTABLETYPE set.
extern void _PyType_SetFlagsRecursive(PyTypeObject *self, unsigned long mask,
                                      unsigned long flags);

PyAPI_FUNC(void) _PyType_SetVersion(PyTypeObject *tp, unsigned int version);
PyTypeObject *_PyType_LookupByVersion(unsigned int version);

// Function pointer type for user-defined validation function that will be
// called by _PyType_Validate().
// It should return 0 if the validation is passed, otherwise it will return -1.
typedef int (*_py_validate_type)(PyTypeObject *);

// It will verify the ``ty`` through user-defined validation function ``validate``,
// and if the validation is passed, it will set the ``tp_version`` as valid
// tp_version_tag from the ``ty``.
extern int _PyType_Validate(PyTypeObject *ty, _py_validate_type validate, unsigned int *tp_version);
extern int _PyType_CacheGetItemForSpecialization(PyHeapTypeObject *ht, PyObject *descriptor, uint32_t tp_version);

// Precalculates count of non-unique slots and fills wrapperbase.name_count.
extern int _PyType_InitSlotDefs(PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
