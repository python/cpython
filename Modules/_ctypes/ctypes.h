#if defined (__SVR4) && defined (__sun)
#   include <alloca.h>
#endif

#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_typeobject.h"    // _PyType_GetModuleState()

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
    /* This dict maps ctypes types to POINTER types */
    PyObject *_ctypes_ptrtype_cache;
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


extern PyType_Spec carg_spec;
extern PyType_Spec cfield_spec;
extern PyType_Spec cthunk_spec;

typedef struct tagPyCArgObject PyCArgObject;
typedef struct tagCDataObject CDataObject;
typedef PyObject *(* GETFUNC)(void *, Py_ssize_t size);
typedef PyObject *(* SETFUNC)(void *, PyObject *value, Py_ssize_t size);
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

extern int PyCStructUnionType_update_stginfo(PyObject *fields, PyObject *type, int isStruct);
extern int PyType_stginfo(PyTypeObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);
extern int PyObject_stginfo(PyObject *self, Py_ssize_t *psize, Py_ssize_t *palign, Py_ssize_t *plength);



#define CDataObject_CheckExact(st, v)  Py_IS_TYPE((v), (st)->PyCData_Type)
#define CDataObject_Check(st, v)       PyObject_TypeCheck((v), (st)->PyCData_Type)
#define _CDataObject_HasExternalBuffer(v)  ((v)->b_ptr != (char *)&(v)->b_value)

#define PyCSimpleTypeObject_CheckExact(st, v)  Py_IS_TYPE((v), (st)->PyCSimpleType_Type)
#define PyCSimpleTypeObject_Check(st, v)       PyObject_TypeCheck((v), (st)->PyCSimpleType_Type)

extern struct fielddesc *_ctypes_get_fielddesc(const char *fmt);


extern PyObject *
PyCField_FromDesc(ctypes_state *st, PyObject *desc, Py_ssize_t index,
                Py_ssize_t *pfield_size, int bitsize, int *pbitofs,
                Py_ssize_t *psize, Py_ssize_t *poffset, Py_ssize_t *palign,
                int pack, int is_big_endian);

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
    SETFUNC setfunc;
    GETFUNC getfunc;
    ffi_type *pffi_type; /* always statically allocated */
    SETFUNC setfunc_swapped;
    GETFUNC getfunc_swapped;
};

typedef struct {
    PyObject_HEAD
    Py_ssize_t offset;
    Py_ssize_t size;
    Py_ssize_t index;                   /* Index into CDataObject's
                                       object array */
    PyObject *proto;                    /* a type or NULL */
    GETFUNC getfunc;                    /* getter function if proto is NULL */
    SETFUNC setfunc;                    /* setter function if proto is NULL */
    int anonymous;
} CFieldObject;

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
    Py_ssize_t align;           /* alignment requirements */
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
    PyObject *module;
    int flags;                  /* calling convention and such */

    /* pep3118 fields, pointers need PyMem_Free */
    char *format;
    int ndim;
    Py_ssize_t *shape;
/*      Py_ssize_t *strides;    */ /* unused in ctypes */
/*      Py_ssize_t *suboffsets; */ /* unused in ctypes */
} StgInfo;

extern int PyCStgInfo_clone(StgInfo *dst_info, StgInfo *src_info);
extern void ctype_clear_stginfo(StgInfo *info);

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
#define TYPEFLAG_HASUNION 0x400
#define TYPEFLAG_HASBITFIELD 0x800

#define DICTFLAG_FINAL 0x1000

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
        long double D;
        double d;
        float f;
        void *p;
    } value;
    PyObject *obj;
    Py_ssize_t size; /* for the 'V' tag */
};

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

extern char basespec_string[];

extern ffi_type *_ctypes_get_ffi_type(ctypes_state *st, PyObject *obj);

extern char *_ctypes_conversion_encoding;
extern char *_ctypes_conversion_errors;


extern void _ctypes_free_closure(void *);
extern void *_ctypes_alloc_closure(void);

extern PyObject *PyCData_FromBaseObj(ctypes_state *st, PyObject *type,
                                     PyObject *base, Py_ssize_t index, char *adr);
extern char *_ctypes_alloc_format_string(const char *prefix, const char *suffix);
extern char *_ctypes_alloc_format_string_with_shape(int ndim,
                                                const Py_ssize_t *shape,
                                                const char *prefix, const char *suffix);

extern int _ctypes_simple_instance(ctypes_state *st, PyObject *obj);

PyObject *_ctypes_get_errobj(ctypes_state *st, int **pspace);

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
 * state is torn down. Does no checks; cannot fail.
 * This inlines the current implementation PyObject_GetTypeData,
 * so it might break in the future.
 */
static inline StgInfo *
_PyStgInfo_FromType_NoState(PyObject *type)
{
    size_t type_basicsize =_Py_SIZE_ROUND_UP(PyType_Type.tp_basicsize,
                                             ALIGNOF_MAX_ALIGN_T);
    return (StgInfo *)((char *)type + type_basicsize);
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
    info->module = Py_NewRef(module);

    info->initialized = 1;
    return info;
}
