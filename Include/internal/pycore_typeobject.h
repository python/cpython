#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pycore_moduleobject.h"

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


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
    PyObject *tp_subclasses;
    /* We never clean up weakrefs for static builtin types since
       they will effectively never get triggered.  However, there
       are also some diagnostic uses for the list of weakrefs,
       so we still keep it. */
    PyObject *tp_weaklist;
} static_builtin_state;

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

struct types_state {
    struct type_cache type_cache;
    size_t num_builtins_initialized;
    static_builtin_state builtins[_Py_MAX_STATIC_BUILTIN_TYPES];
};


extern int _PyStaticType_InitBuiltin(PyTypeObject *type);
extern static_builtin_state * _PyStaticType_GetState(PyTypeObject *);
extern void _PyStaticType_ClearWeakRefs(PyTypeObject *type);
extern void _PyStaticType_Dealloc(PyTypeObject *type);

PyObject *
_Py_type_getattro_impl(PyTypeObject *type, PyObject *name, int *suppress_missing_attribute);
PyObject *
_Py_type_getattro(PyTypeObject *type, PyObject *name);

PyObject *_Py_slot_tp_getattro(PyObject *self, PyObject *name);
PyObject *_Py_slot_tp_getattr_hook(PyObject *self, PyObject *name);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
