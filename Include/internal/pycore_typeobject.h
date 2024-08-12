#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_moduleobject.h"  // PyModuleObject
#include "pycore_lock.h"          // PyMutex


/* state */

#define _Py_TYPE_BASE_VERSION_TAG (2<<16)
#define _Py_MAX_GLOBAL_TYPE_VERSION_TAG (_Py_TYPE_BASE_VERSION_TAG - 1)

/* For now we hard-code this to a value for which we are confident
   all the static builtin types will fit (for all builds). */
#define _Py_MAX_MANAGED_STATIC_BUILTIN_TYPES 200
#define _Py_MAX_MANAGED_STATIC_EXT_TYPES 10
#define _Py_MAX_MANAGED_STATIC_TYPES \
    (_Py_MAX_MANAGED_STATIC_BUILTIN_TYPES + _Py_MAX_MANAGED_STATIC_EXT_TYPES)

struct _types_runtime_state {
    /* Used to set PyTypeObject.tp_version_tag for core static types. */
    // bpo-42745: next_version_tag remains shared by all interpreters
    // because of static types.
    unsigned int next_version_tag;

    struct {
        struct {
            PyTypeObject *type;
            int64_t interp_count;
        } types[_Py_MAX_MANAGED_STATIC_TYPES];
    } managed_static;
};


// Type attribute lookup cache: speed up attribute and method lookups,
// see _PyType_Lookup().
struct type_cache_entry {
    unsigned int version;  // initialized from type->tp_version_tag
#ifdef Py_GIL_DISABLED
   _PySeqLock sequence;
#endif
    PyObject *name;        // reference to exactly a str or None
    PyObject *value;       // borrowed reference or NULL
};

#define MCACHE_SIZE_EXP 12

struct type_cache {
    struct type_cache_entry hashtable[1 << MCACHE_SIZE_EXP];
};

typedef struct {
    PyTypeObject *type;
    int isbuiltin;
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
} managed_static_type_state;

struct types_state {
    /* Used to set PyTypeObject.tp_version_tag.
       It starts at _Py_MAX_GLOBAL_TYPE_VERSION_TAG + 1,
       where all those lower numbers are used for core static types. */
    unsigned int next_version_tag;

    struct type_cache type_cache;

    /* Every static builtin type is initialized for each interpreter
       during its own initialization, including for the main interpreter
       during global runtime initialization.  This is done by calling
       _PyStaticType_InitBuiltin().

       The first time a static builtin type is initialized, all the
       normal PyType_Ready() stuff happens.  The only difference from
       normal is that there are three PyTypeObject fields holding
       objects which are stored here (on PyInterpreterState) rather
       than in the corresponding PyTypeObject fields.  Those are:
       tp_dict (cls.__dict__), tp_subclasses (cls.__subclasses__),
       and tp_weaklist.

       When a subinterpreter is initialized, each static builtin type
       is still initialized, but only the interpreter-specific portion,
       namely those three objects.

       Those objects are stored in the PyInterpreterState.types.builtins
       array, at the index corresponding to each specific static builtin
       type.  That index (a size_t value) is stored in the tp_subclasses
       field.  For static builtin types, we re-purposed the now-unused
       tp_subclasses to avoid adding another field to PyTypeObject.
       In all other cases tp_subclasses holds a dict like before.
       (The field was previously defined as PyObject*, but is now void*
       to reflect its dual use.)

       The index for each static builtin type isn't statically assigned.
       Instead it is calculated the first time a type is initialized
       (by the main interpreter).  The index matches the order in which
       the type was initialized relative to the others.  The actual
       value comes from the current value of num_builtins_initialized,
       as each type is initialized for the main interpreter.

       num_builtins_initialized is incremented once for each static
       builtin type.  Once initialization is over for a subinterpreter,
       the value will be the same as for all other interpreters.  */
    struct {
        size_t num_initialized;
        managed_static_type_state initialized[_Py_MAX_MANAGED_STATIC_BUILTIN_TYPES];
    } builtins;
    /* We apply a similar strategy for managed extension modules. */
    struct {
        size_t num_initialized;
        size_t next_index;
        managed_static_type_state initialized[_Py_MAX_MANAGED_STATIC_EXT_TYPES];
    } for_extensions;
    PyMutex mutex;
};


/* runtime lifecycle */

extern PyStatus _PyTypes_InitTypes(PyInterpreterState *);
extern void _PyTypes_FiniTypes(PyInterpreterState *);
extern void _PyTypes_FiniExtTypes(PyInterpreterState *interp);
extern void _PyTypes_Fini(PyInterpreterState *);
extern void _PyTypes_AfterFork(void);

/* other API */

/* Length of array of slotdef pointers used to store slots with the
   same __name__.  There should be at most MAX_EQUIV-1 slotdef entries with
   the same __name__, for any __name__. Since that's a static property, it is
   appropriate to declare fixed-size arrays for this. */
#define MAX_EQUIV 10

typedef struct wrapperbase pytype_slotdef;


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

extern PyObject * _PyType_GetBases(PyTypeObject *type);
extern PyObject * _PyType_GetMRO(PyTypeObject *type);
extern PyObject* _PyType_GetSubclasses(PyTypeObject *);
extern int _PyType_HasSubclasses(PyTypeObject *);
PyAPI_FUNC(PyObject *) _PyType_GetModuleByDef2(PyTypeObject *, PyTypeObject *, PyModuleDef *);

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

// Like _PyType_SetFlags(), but apply the operation to self and any of its
// subclasses without Py_TPFLAGS_IMMUTABLETYPE set.
extern void _PyType_SetFlagsRecursive(PyTypeObject *self, unsigned long mask,
                                      unsigned long flags);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
