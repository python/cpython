#if defined (__SVR4) && defined (__sun)
#   include <alloca.h>
#endif

#include <stdbool.h>

#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()
#include "pycore_critical_section.h"
#include "pycore_pyatomic_ft_wrappers.h"

// Do we support C99 complex types in ffi?
// For Apple's libffi, this must be determined at runtime (see gh-128156).
#if defined(Py_FFI_SUPPORT_C_COMPLEX)
#   if USING_APPLE_OS_LIBFFI && defined(__has_builtin)
#       if __has_builtin(__builtin_available)
#           define Py_FFI_COMPLEX_AVAILABLE __builtin_available(macOS 10.15, *)
#       else
#           define Py_FFI_COMPLEX_AVAILABLE 1
#       endif
#   else
#       define Py_FFI_COMPLEX_AVAILABLE 1
#   endif
#else
#   define Py_FFI_COMPLEX_AVAILABLE 0
#endif

#ifndef MS_WIN32
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#define PARAMFLAG_FIN 0x1
#define PARAMFLAG_FOUT 0x2
#define PARAMFLAG_FLCID 0x4
#endif

/*
 * bpo-13097: Max number of arguments CFuncPtr._argtypes_ and
 * _ctypes_callproc() will accept.
 *
 * This limit is enforced for the `alloca()` call in `_ctypes_callproc`,
 * to avoid allocating a massive buffer on the stack.
 */
#ifndef CTYPES_MAX_ARGCOUNT
  #ifdef __EMSCRIPTEN__
    #define CTYPES_MAX_ARGCOUNT 1000
  #else
    #define CTYPES_MAX_ARGCOUNT 1024
  #endif
#endif

#if defined(__has_builtin)
#if __has_builtin(__builtin_available)
#define HAVE_BUILTIN_AVAILABLE 1
#endif
#endif

#ifdef MS_WIN32
#include <Unknwn.h> // for IUnknown interface
#endif

typedef struct {
    PyTypeObject *DictRemover_Type;
    PyTypeObject *PyCArg_Type;
    PyTypeObject *PyCField_Type;
    PyTypeObject *PyCThunk_Type;
    PyTypeObject *StructParam_Type;
    PyTypeObject *PyCType_Type;
    PyTypeObject *PyCStructType_Type;
    PyTypeObject *UnionType_Type;
    PyTypeObject *PyCPointerType_Type;
    PyTypeObject *PyCArrayType_Type;
    PyTypeObject *PyCSimpleType_Type;
    PyTypeObject *PyCFuncPtrType_Type;
    PyTypeObject *PyCData_Type;
    PyTypeObject *Struct_Type;
    PyTypeObject *Union_Type;
    PyTypeObject *PyCArray_Type;
    PyTypeObject *Simple_Type;
    PyTypeObject *PyCPointer_Type;
    PyTypeObject *PyCFuncPtr_Type;
#ifdef MS_WIN32
    PyTypeObject *PyComError_Type;
#endif
    /* a callable object used for unpickling:
       strong reference to _ctypes._unpickle() function */
    PyObject *_unpickle;
    PyObject *array_cache;
    PyObject *error_object_name;  // callproc.c
    PyObject *PyExc_ArgError;
    PyObject *swapped_suffix;
} ctypes_state;


extern struct PyModuleDef _ctypesmodule;


static inline ctypes_state *
get_module_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (ctypes_state *)state;
}

static inline ctypes_state *
get_module_state_by_class(PyTypeObject *cls)
{
    ctypes_state *state = (ctypes_state *)_PyType_GetModuleState(cls);
    assert(state != NULL);
    return state;
}

static inline ctypes_state *
get_module_state_by_def(PyTypeObject *cls)
{
    PyObject *mod = PyType_GetModuleByDef(cls, &_ctypesmodule);
    assert(mod != NULL);
    return get_module_state(mod);
}


extern PyType_Spec pyctype_type_spec;
extern PyType_Spec carg_spec;
extern PyType_Spec cfield_spec;
extern PyType_Spec cthunk_spec;

typedef struct tagPyCArgObject PyCArgObject;
#define _PyCArgObject_CAST(op)  ((PyCArgObject *)(op))

typedef struct tagCDataObject CDataObject;
#define _CDataObject_CAST(op)   ((CDataObject *)(op))

// GETFUNC: convert the C value at *ptr* to Python object, return the object
// SETFUNC: write content of the PyObject *value* to the location at *ptr*;
//   return a new reference to either *value*, or None for simple types
//   (see _CTYPES_DEBUG_KEEP).
// Note that the *size* arg can have different meanings depending on context:
//     for string-like arrays it's the size in bytes
//     for int-style fields it's either the type size, or bitfiled info
//         that can be unpacked using the LOW_BIT & NUM_BITS macros.
typedef PyObject *(* GETFUNC)(void *ptr, Py_ssize_t size);
typedef PyObject *(* SETFUNC)(void *ptr, PyObject *value, Py_ssize_t size);
typedef PyCArgObject *(* PARAMFUNC)(ctypes_state *st, CDataObject *obj);

/* A default buffer in CDataObject, which can be used for small C types.  If
this buffer is too small, PyMem_Malloc will be called to create a larger one,
and this one is not used.

Making CDataObject a variable size object would be a better solution, but more
difficult in the presence of PyCFuncPtrObject.  Maybe later.
*/
union value {
                char c[16];
                short s;
                int i;
                long l;
                float f;
                double d;
                long long ll;
                long double D;
};

/*
  Hm. Are there CDataObject's which do not need the b_objects member?  In
  this case we probably should introduce b_flags to mark it as present...  If
  b_objects is not present/unused b_length is unneeded as well.
*/

struct tagCDataObject {
    PyObject_HEAD
    char *b_ptr;                /* pointer to memory block */
    int  b_needsfree;           /* need _we_ free the memory? */
    CDataObject *b_base;        /* pointer to base object or NULL */
    Py_ssize_t b_size;          /* size of memory block in bytes */
    Py_ssize_t b_length;        /* number of references we need */
    Py_ssize_t b_index;         /* index of this object into base's
                               b_object list */
    PyObject *b_objects;        /* dictionary of references we need to keep, or Py_None */
    union value b_value;
};

typedef struct {
    PyObject_VAR_HEAD
    ffi_closure *pcl_write; /* the C callable, writeable */
    void *pcl_exec;         /* the C callable, executable */
    ffi_cif cif;
    int flags;
    PyObject *converters;
    PyObject *callable;
    PyObject *restype;
    SETFUNC setfunc;
    ffi_type *ffi_restype;
    ffi_type *atypes[1];
} CThunkObject;

#define _CThunkObject_CAST(op)          ((CThunkObject *)(op))
#define CThunk_CheckExact(st, v)        Py_IS_TYPE(v, st->PyCThunk_Type)

typedef struct {
    /* First part identical to tagCDataObject */
    PyObject_HEAD
    char *b_ptr;                /* pointer to memory block */
    int  b_needsfree;           /* need _we_ free the memory? */
    CDataObject *b_base;        /* pointer to base object or NULL */
    Py_ssize_t b_size;          /* size of memory block in bytes */
    Py_ssize_t b_length;        /* number of references we need */
    Py_ssize_t b_index;         /* index of this object into base's
                                   b_object list */
    PyObject *b_objects;        /* list of references we need to keep */
    union value b_value;
    /* end of tagCDataObject, additional fields follow */

    CThunkObject *thunk;
    PyObject *callable;

    /* These two fields will override the ones in the type's stginfo if
       they are set */
    PyObject *converters;
    PyObject *argtypes;
    PyObject *restype;
    PyObject *checker;
    PyObject *errcheck;
#ifdef MS_WIN32
    int index;
    GUID *iid;
#endif
    PyObject *paramflags;
} PyCFuncPtrObject;

#define _PyCFuncPtrObject_CAST(op)  ((PyCFuncPtrObject *)(op))

extern int PyCStructUnionType_update_stginfo(PyObject *fields, PyObject *type, int isStruct);
extern int PyType_stginfo(PyTypeObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);
extern int PyObject_stginfo(PyObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);



#define CDataObject_CheckExact(st, v)  Py_IS_TYPE((v), (st)->PyCData_Type)
#define CDataObject_Check(st, v)       PyObject_TypeCheck((v), (st)->PyCData_Type)
#define _CDataObject_HasExternalBuffer(v)  ((v)->b_ptr != (char *)&(v)->b_value)

#define PyCSimpleTypeObject_CheckExact(st, v)  Py_IS_TYPE((v), (st)->PyCSimpleType_Type)
#define PyCSimpleTypeObject_Check(st, v)       PyObject_TypeCheck((v), (st)->PyCSimpleType_Type)

extern struct fielddesc *_ctypes_get_fielddesc(const char *fmt);

extern PyObject *PyCData_AtAddress(ctypes_state *st, PyObject *type, void *buf);
extern PyObject *PyCData_FromBytes(ctypes_state *st, PyObject *type, char *data, Py_ssize_t length);

#define PyCArrayTypeObject_Check(st, v)   PyObject_TypeCheck((v), (st)->PyCArrayType_Type)
#define ArrayObject_Check(st, v)          PyObject_TypeCheck((v), (st)->PyCArray_Type)
#define PointerObject_Check(st, v)        PyObject_TypeCheck((v), (st)->PyCPointer_Type)
#define PyCPointerTypeObject_Check(st, v) PyObject_TypeCheck((v), (st)->PyCPointerType_Type)
#define PyCFuncPtrObject_Check(st,v)      PyObject_TypeCheck((v), (st)->PyCFuncPtr_Type)
#define PyCFuncPtrTypeObject_Check(st, v) PyObject_TypeCheck((v), (st)->PyCFuncPtrType_Type)
#define PyCStructTypeObject_Check(st, v)  PyObject_TypeCheck((v), (st)->PyCStructType_Type)

extern PyObject *
PyCArrayType_from_ctype(ctypes_state *st, PyObject *itemtype, Py_ssize_t length);

extern PyMethodDef _ctypes_module_methods[];

extern CThunkObject *_ctypes_alloc_callback(ctypes_state *st,
                                           PyObject *callable,
                                           PyObject *converters,
                                           PyObject *restype,
                                           int flags);
/* a table entry describing a predefined ctypes type */
struct fielddesc {
    char code;
    ffi_type *pffi_type; /* always statically allocated */
    SETFUNC setfunc;
    GETFUNC getfunc;
    SETFUNC setfunc_swapped;
    GETFUNC getfunc_swapped;
};

// Get all single-character type codes (for use in error messages)
extern char *_ctypes_get_simple_type_chars(void);

typedef struct CFieldObject {
    PyObject_HEAD

    /* byte size & offset
     * For bit fields, this identifies a chunk of memory that the bits are
     *  extracted from. The entire chunk needs to be contained in the enclosing
     *  struct/union.
     * byte_size is the same as the underlying ctype size (and thus it is
     *  redundant and could be eliminated).
     * Note that byte_offset might not be aligned to proto's alignment.
     */
    Py_ssize_t byte_offset;
    Py_ssize_t byte_size;

    Py_ssize_t index;                   /* Index into CDataObject's object array */
    PyObject *proto;                    /* underlying ctype; must have StgInfo */
    GETFUNC getfunc;                    /* getter function if proto is NULL */
    SETFUNC setfunc;                    /* setter function if proto is NULL */
    bool anonymous: 1;

    /* If this is a bit field, bitfield_size must be positive.
     *   bitfield_size and bit_offset specify the field inside the chunk of
     *   memory identified by byte_offset & byte_size.
     * Otherwise, these are both zero.
     *
     * Note that for NON-bitfields:
     *  - `bit_size` (user-facing Python attribute) `is byte_size*8`
     *  - `bitfield_size` (this) is zero
     * Hence the different name.
     */
    uint8_t bitfield_size;
    uint8_t bit_offset;

    PyObject *name;                     /* exact PyUnicode */
} CFieldObject;

#define _CFieldObject_CAST(op)  ((CFieldObject *)(op))

/****************************************************************
 StgInfo

 Since Python 3.13, ctypes-specific type information is stored in the
 corresponding type object, in a `StgInfo` struct accessed by the helpers
 below.
 Before that, each type's `tp_dict` was set to a dict *subclass* that included
 the fields that are now in StgInfo. The mechanism was called "StgDict"; a few
 references to that name might remain.

 Functions for accessing StgInfo are `static inline` for performance;
 see later in this file.

 ****************************************************************

 StgInfo fields

 setfunc and getfunc is only set for simple data types, it is copied from the
 corresponding fielddesc entry.  These are functions to set and get the value
 in a memory block.
 They should probably by used by other types as well.

 proto is only used for Pointer and Array types - it points to the item type
 object.

 Probably all the magic ctypes methods (like from_param) should have C
 callable wrappers in the StgInfo.  For simple data type, for example,
 the fielddesc table could have entries for C codec from_param functions or
 other methods as well, if a subtype overrides this method in Python at
 construction time, or assigns to it later, tp_setattro should update the
 StgInfo function to a generic one.

 Currently, PyCFuncPtr types have 'converters' and 'checker' entries in their
 type dict.  They are only used to cache attributes from other entries, which
 is wrong.

 One use case is the .value attribute that all simple types have.  But some
 complex structures, like VARIANT, represent a single value also, and should
 have this attribute.

 Another use case is a _check_retval_ function, which is called when a ctypes
 type is used as return type of a function to validate and compute the return
 value.

 Common ctypes protocol:

  - setfunc: store a python value in a memory block
  - getfunc: convert data from a memory block into a python value

  - checkfunc: validate and convert a return value from a function call
  - toparamfunc: convert a python value into a function argument

*****************************************************************/

typedef struct {
    int initialized;
    Py_ssize_t size;            /* number of bytes */
    Py_ssize_t align;           /* alignment reqwuirements */
    Py_ssize_t length;          /* number of fields */
    ffi_type ffi_type_pointer;
    PyObject *proto;            /* Only for Pointer/ArrayObject */
    SETFUNC setfunc;            /* Only for simple objects */
    GETFUNC getfunc;            /* Only for simple objects */
    PARAMFUNC paramfunc;

    /* Following fields only used by PyCFuncPtrType_Type instances */
    PyObject *argtypes;         /* tuple of CDataObjects */
    PyObject *converters;       /* tuple([t.from_param for t in argtypes]) */
    PyObject *restype;          /* CDataObject or NULL */
    PyObject *checker;
    PyObject *pointer_type;     /* __pointer_type__ attribute;
                                   arbitrary object or NULL */
    PyObject *module;
    int flags;                  /* calling convention and such */
#ifdef Py_GIL_DISABLED
    PyMutex mutex;              /* critical section mutex */
#endif
    uint8_t dict_final;

    /* pep3118 fields, pointers need PyMem_Free */
    char *format;
    int ndim;
    Py_ssize_t *shape;
    /*      Py_ssize_t *strides;    */ /* unused in ctypes */
    /*      Py_ssize_t *suboffsets; */ /* unused in ctypes */
} StgInfo;


/*
    To ensure thread safety in the free threading build, the `STGINFO_LOCK` and
    `STGINFO_UNLOCK` macros use critical sections to protect against concurrent
    modifications to `StgInfo` and assignment of the `dict_final` field. Once
    `dict_final` is set, `StgInfo` is treated as read-only, and no further
    modifications are allowed. This approach allows most read operations to
    proceed without acquiring the critical section lock.

    The `dict_final` field is written only after all other modifications to
    `StgInfo` are complete. The reads and writes of `dict_final` use the
    sequentially consistent memory ordering to ensure that all other fields are
    visible to other threads before the `dict_final` bit is set.
*/

#define STGINFO_LOCK(stginfo)   Py_BEGIN_CRITICAL_SECTION_MUT(&(stginfo)->mutex)
#define STGINFO_UNLOCK()        Py_END_CRITICAL_SECTION()

static inline uint8_t
stginfo_get_dict_final(StgInfo *info)
{
    return FT_ATOMIC_LOAD_UINT8(info->dict_final);
}

static inline void
stginfo_set_dict_final_lock_held(StgInfo *info)
{
    _Py_CRITICAL_SECTION_ASSERT_MUTEX_LOCKED(&info->mutex);
    FT_ATOMIC_STORE_UINT8(info->dict_final, 1);
}


// Set the `dict_final` bit in StgInfo. It checks if the bit is already set
// and in that avoids acquiring the critical section (general case).
static inline void
stginfo_set_dict_final(StgInfo *info)
{
    if (stginfo_get_dict_final(info) == 1) {
        return;
    }
    STGINFO_LOCK(info);
    stginfo_set_dict_final_lock_held(info);
    STGINFO_UNLOCK();
}

extern int PyCStgInfo_clone(StgInfo *dst_info, StgInfo *src_info);
extern void ctype_clear_stginfo(StgInfo *info);
extern void ctype_free_stginfo_members(StgInfo *info);

typedef int(* PPROC)(void);

PyObject *_ctypes_callproc(ctypes_state *st,
                    PPROC pProc,
                    PyObject *arguments,
#ifdef MS_WIN32
                    IUnknown *pIUnk,
                    GUID *iid,
#endif
                    int flags,
                    PyObject *argtypes,
                    PyObject *restype,
                    PyObject *checker);


#define FUNCFLAG_STDCALL 0x0
#define FUNCFLAG_CDECL   0x1
#define FUNCFLAG_HRESULT 0x2
#define FUNCFLAG_PYTHONAPI 0x4
#define FUNCFLAG_USE_ERRNO 0x8
#define FUNCFLAG_USE_LASTERROR 0x10

#define TYPEFLAG_ISPOINTER 0x100
#define TYPEFLAG_HASPOINTER 0x200

struct tagPyCArgObject {
    PyObject_HEAD
    ffi_type *pffi_type;
    char tag;
    union {
        char c;
        char b;
        short h;
        int i;
        long l;
        long long q;
        long double g;
        double d;
        float f;
        void *p;
        double D[2];
        float F[2];
        long double G[2];
    } value;
    PyObject *obj;
    Py_ssize_t size; /* for the 'V' tag */
};

#define _PyCArgObject_CAST(op)  ((PyCArgObject *)(op))

#define PyCArg_CheckExact(st, v)        Py_IS_TYPE(v, st->PyCArg_Type)
extern PyCArgObject *PyCArgObject_new(ctypes_state *st);

extern PyObject *
PyCData_get(ctypes_state *st, PyObject *type, GETFUNC getfunc, PyObject *src,
          Py_ssize_t index, Py_ssize_t size, char *ptr);

extern int
PyCData_set(ctypes_state *st,
          PyObject *dst, PyObject *type, SETFUNC setfunc, PyObject *value,
          Py_ssize_t index, Py_ssize_t size, char *ptr);

extern void _ctypes_extend_error(PyObject *exc_class, const char *fmt, ...);

struct basespec {
    CDataObject *base;
    Py_ssize_t index;
    char *adr;
};

extern ffi_type *_ctypes_get_ffi_type(ctypes_state *st, PyObject *obj);

extern void _ctypes_free_closure(void *);
extern void *_ctypes_alloc_closure(void);

extern PyObject *PyCData_FromBaseObj(ctypes_state *st, PyObject *type,
                                     PyObject *base, Py_ssize_t index, char *adr);

extern int _ctypes_simple_instance(ctypes_state *st, PyObject *obj);

PyObject *_ctypes_get_errobj(ctypes_state *st, int **pspace);

extern void _ctypes_init_fielddesc(void);

#ifdef USING_MALLOC_CLOSURE_DOT_C
void Py_ffi_closure_free(void *p);
void *Py_ffi_closure_alloc(size_t size, void** codeloc);
#else
#define Py_ffi_closure_free ffi_closure_free
#define Py_ffi_closure_alloc ffi_closure_alloc
#endif


/****************************************************************
 * Accessing StgInfo -- these are inlined for performance reasons.
 */

// `PyStgInfo_From**` functions get a PyCTypeDataObject.
// These return -1 on error, 0 if "not found", 1 on OK.
// (Currently, these do not return -1 in practice. This might change
// in the future.)

//
// Common helper:
static inline int
_stginfo_from_type(ctypes_state *state, PyTypeObject *type, StgInfo **result)
{
    *result = NULL;
    if (!PyObject_IsInstance((PyObject *)type, (PyObject *)state->PyCType_Type)) {
        // not a ctypes class.
        return 0;
    }
    StgInfo *info = PyObject_GetTypeData((PyObject *)type, state->PyCType_Type);
    assert(info != NULL);
    if (!info->initialized) {
        // StgInfo is not initialized. This happens in abstract classes.
        return 0;
    }
    *result = info;
    return 1;
}
// from a type:
static inline int
PyStgInfo_FromType(ctypes_state *state, PyObject *type, StgInfo **result)
{
    return _stginfo_from_type(state, (PyTypeObject *)type, result);
}
// from an instance:
static inline int
PyStgInfo_FromObject(ctypes_state *state, PyObject *obj, StgInfo **result)
{
    return _stginfo_from_type(state, Py_TYPE(obj), result);
}
// from either a type or an instance:
static inline int
PyStgInfo_FromAny(ctypes_state *state, PyObject *obj, StgInfo **result)
{
    if (PyType_Check(obj)) {
        return _stginfo_from_type(state, (PyTypeObject *)obj, result);
    }
    return _stginfo_from_type(state, Py_TYPE(obj), result);
}

/* A variant of PyStgInfo_FromType that doesn't need the state,
 * so it can be called from finalization functions when the module
 * state is torn down.
 */
static inline StgInfo *
_PyStgInfo_FromType_NoState(PyObject *type)
{
    PyTypeObject *PyCType_Type;
    if (PyType_GetBaseByToken(Py_TYPE(type), &pyctype_type_spec, &PyCType_Type) < 0) {
        return NULL;
    }
    if (PyCType_Type == NULL) {
        PyErr_Format(PyExc_TypeError, "expected a ctypes type, got '%N'", type);
        return NULL;
    }

    StgInfo *info = PyObject_GetTypeData(type, PyCType_Type);
    Py_DECREF(PyCType_Type);
    return info;
}

// Initialize StgInfo on a newly created type
static inline StgInfo *
PyStgInfo_Init(ctypes_state *state, PyTypeObject *type)
{
    if (!PyObject_IsInstance((PyObject *)type, (PyObject *)state->PyCType_Type)) {
        PyErr_Format(PyExc_SystemError,
                     "'%s' is not a ctypes class.",
                     type->tp_name);
        return NULL;
    }
    StgInfo *info = PyObject_GetTypeData((PyObject *)type, state->PyCType_Type);
    if (info->initialized) {
        PyErr_Format(PyExc_SystemError,
                     "StgInfo of '%s' is already initialized.",
                     type->tp_name);
        return NULL;
    }
    PyObject *module = PyType_GetModule(state->PyCType_Type);
    if (!module) {
        return NULL;
    }
    info->pointer_type = NULL;
    info->module = Py_NewRef(module);

    info->initialized = 1;
    return info;
}
