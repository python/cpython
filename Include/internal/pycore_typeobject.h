#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_moduleobject.h"

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* state */

#define _Py_TYPE_BASE_VERSION_TAG (2<<16)
#define _Py_MAX_GLOBAL_TYPE_VERSION_TAG (_Py_TYPE_BASE_VERSION_TAG - 1)

struct _types_runtime_state {
    /* Used to set PyTypeObject.tp_version_tag for core static types. */
    // bpo-42745: next_version_tag remains shared by all interpreters
    // because of static types.
    unsigned int next_version_tag;
};


// Type attribute lookup cache: speed up attribute and method lookups,
// see _PyType_Lookup().
struct type_cache_entry {
    unsigned int version;  // initialized from type->tp_version_tag
    PyObject *name;        // reference to exactly a str or None
    PyObject *value;       // borrowed reference or NULL
};

#define MCACHE_SIZE_EXP 12

struct type_cache {
    struct type_cache_entry hashtable[1 << MCACHE_SIZE_EXP];
};

/* For now we hard-code this to a value for which we are confident
   all the static builtin types will fit (for all builds). */
#define _Py_MAX_STATIC_BUILTIN_TYPES 200

typedef struct {
    PyTypeObject *type;
    int readying;
    int ready;
    // XXX tp_dict can probably be statically allocated,
    // instead of dynamically and stored on the interpreter.
    PyObject *tp_dict;
    PyObject *tp_subclasses;
    /* We never clean up weakrefs for static builtin types since
       they will effectively never get triggered.  However, there
       are also some diagnostic uses for the list of weakrefs,
       so we still keep it. */
    PyObject *tp_weaklist;
} static_builtin_state;

struct types_state {
    /* Used to set PyTypeObject.tp_version_tag.
       It starts at _Py_MAX_GLOBAL_TYPE_VERSION_TAG + 1,
       where all those lower numbers are used for core static types. */
    unsigned int next_version_tag;

    struct type_cache type_cache;
    size_t num_builtins_initialized;
    static_builtin_state builtins[_Py_MAX_STATIC_BUILTIN_TYPES];
};


/* runtime lifecycle */

extern PyStatus _PyTypes_InitTypes(PyInterpreterState *);
extern void _PyTypes_FiniTypes(PyInterpreterState *);
extern void _PyTypes_Fini(PyInterpreterState *);


/* other API */

/* Length of array of slotdef pointers used to store slots with the
   same __name__.  There should be at most MAX_EQUIV-1 slotdef entries with
   the same __name__, for any __name__. Since that's a static property, it is
   appropriate to declare fixed-size arrays for this. */
#define MAX_EQUIV 10

typedef struct wrapperbase pytype_slotdef;


static inline PyObject **
_PyStaticType_GET_WEAKREFS_LISTPTR(static_builtin_state *state)
{
    assert(state != NULL);
    return &state->tp_weaklist;
}

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


extern int _PyStaticType_InitBuiltin(PyInterpreterState *, PyTypeObject *type);
extern static_builtin_state * _PyStaticType_GetState(PyInterpreterState *, PyTypeObject *);
extern void _PyStaticType_ClearWeakRefs(PyInterpreterState *, PyTypeObject *type);
extern void _PyStaticType_Dealloc(PyInterpreterState *, PyTypeObject *);

PyAPI_FUNC(PyObject *) _PyType_GetDict(PyTypeObject *);
extern PyObject * _PyType_GetBases(PyTypeObject *type);
extern PyObject * _PyType_GetMRO(PyTypeObject *type);
extern PyObject* _PyType_GetSubclasses(PyTypeObject *);
extern int _PyType_HasSubclasses(PyTypeObject *);

// PyType_Ready() must be called if _PyType_IsReady() is false.
// See also the Py_TPFLAGS_READY flag.
static inline int
_PyType_IsReady(PyTypeObject *type)
{
    return _PyType_GetDict(type) != NULL;
}

PyObject *
_Py_type_getattro_impl(PyTypeObject *type, PyObject *name, int *suppress_missing_attribute);
PyObject *
_Py_type_getattro(PyTypeObject *type, PyObject *name);

extern PyObject* _Py_BaseObject_RichCompare(PyObject* self, PyObject* other, int op);

PyObject *_Py_slot_tp_getattro(PyObject *self, PyObject *name);
PyObject *_Py_slot_tp_getattr_hook(PyObject *self, PyObject *name);

PyAPI_DATA(PyTypeObject) _PyBufferWrapper_Type;

PyObject *
_PySuper_Lookup(PyTypeObject *su_type, PyObject *su_obj, PyObject *name, int *meth_found);

extern int _PyType_AddMethod(PyTypeObject *, PyMethodDef *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
