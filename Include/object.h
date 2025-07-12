#ifndef Py_OBJECT_H
#define Py_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* Object and type object interface */

/*
Objects are structures allocated on the heap.  Special rules apply to
the use of objects to ensure they are properly garbage-collected.
Objects are never allocated statically or on the stack; they must be
accessed through special macros and functions only.  (Type objects are
exceptions to the first rule; the standard types are represented by
statically initialized type objects, although work on type/class unification
for Python 2.2 made it possible to have heap-allocated type objects too).

An object has a 'reference count' that is increased or decreased when a
pointer to the object is copied or deleted; when the reference count
reaches zero there are no references to the object left and it can be
removed from the heap.

An object has a 'type' that determines what it represents and what kind
of data it contains.  An object's type is fixed when it is created.
Types themselves are represented as objects; an object contains a
pointer to the corresponding type object.  The type itself has a type
pointer pointing to the object representing the type 'type', which
contains a pointer to itself!.

Objects do not float around in memory; once allocated an object keeps
the same size and address.  Objects that must hold variable-size data
can contain pointers to variable-size parts of the object.  Not all
objects of the same type have the same size; but the size cannot change
after allocation.  (These restrictions are made so a reference to an
object can be simply a pointer -- moving an object would require
updating all the pointers, and changing an object's size would require
moving it if there was another object right next to it.)

Objects are always accessed through pointers of the type 'PyObject *'.
The type 'PyObject' is a structure that only contains the reference count
and the type pointer.  The actual memory allocated for an object
contains other data that can only be accessed after casting the pointer
to a pointer to a longer structure type.  This longer type must start
with the reference count and type fields; the macro PyObject_HEAD should be
used for this (to accommodate for future changes).  The implementation
of a particular object type can cast the object pointer to the proper
type and back.

A standard interface exists for objects that contain an array of items
whose size is determined when the object is allocated.
*/

/* Py_DEBUG implies Py_REF_DEBUG. */
#if defined(Py_DEBUG) && !defined(Py_REF_DEBUG)
#  define Py_REF_DEBUG
#endif

/* PyObject_HEAD defines the initial segment of every PyObject. */
#define PyObject_HEAD                   PyObject ob_base;

/*
Immortalization:

The following indicates the immortalization strategy depending on the amount
of available bits in the reference count field. All strategies are backwards
compatible but the specific reference count value or immortalization check
might change depending on the specializations for the underlying system.

Proper deallocation of immortal instances requires distinguishing between
statically allocated immortal instances vs those promoted by the runtime to be
immortal. The latter should be the only instances that require
cleanup during runtime finalization.
*/

#if SIZEOF_VOID_P > 4
/*
In 64+ bit systems, an object will be marked as immortal by setting all of the
lower 32 bits of the reference count field, which is equal to: 0xFFFFFFFF

Using the lower 32 bits makes the value backwards compatible by allowing
C-Extensions without the updated checks in Py_INCREF and Py_DECREF to safely
increase and decrease the objects reference count. The object would lose its
immortality, but the execution would still be correct.

Reference count increases will use saturated arithmetic, taking advantage of
having all the lower 32 bits set, which will avoid the reference count to go
beyond the refcount limit. Immortality checks for reference count decreases will
be done by checking the bit sign flag in the lower 32 bits.
*/
#define _Py_IMMORTAL_REFCNT _Py_CAST(Py_ssize_t, UINT_MAX)

#else
/*
In 32 bit systems, an object will be marked as immortal by setting all of the
lower 30 bits of the reference count field, which is equal to: 0x3FFFFFFF

Using the lower 30 bits makes the value backwards compatible by allowing
C-Extensions without the updated checks in Py_INCREF and Py_DECREF to safely
increase and decrease the objects reference count. The object would lose its
immortality, but the execution would still be correct.

Reference count increases and decreases will first go through an immortality
check by comparing the reference count field to the immortality reference count.
*/
#define _Py_IMMORTAL_REFCNT _Py_CAST(Py_ssize_t, UINT_MAX >> 2)
#endif

// Py_GIL_DISABLED builds indicate immortal objects using `ob_ref_local`, which is
// always 32-bits.
#ifdef Py_GIL_DISABLED
#define _Py_IMMORTAL_REFCNT_LOCAL UINT32_MAX
#endif

// Kept for backward compatibility. It was needed by Py_TRACE_REFS build.
#define _PyObject_EXTRA_INIT

/* Make all uses of PyObject_HEAD_INIT immortal.
 *
 * Statically allocated objects might be shared between
 * interpreters, so must be marked as immortal.
 */
#if defined(Py_GIL_DISABLED)
#define PyObject_HEAD_INIT(type)    \
    {                               \
        0,                          \
        0,                          \
        { 0 },                      \
        0,                          \
        _Py_IMMORTAL_REFCNT_LOCAL,  \
        0,                          \
        (type),                     \
    },
#else
#define PyObject_HEAD_INIT(type)    \
    {                               \
        { _Py_IMMORTAL_REFCNT },    \
        (type)                      \
    },
#endif

#define PyVarObject_HEAD_INIT(type, size) \
    {                                     \
        PyObject_HEAD_INIT(type)          \
        (size)                            \
    },

/* PyObject_VAR_HEAD defines the initial segment of all variable-size
 * container objects.  These end with a declaration of an array with 1
 * element, but enough space is malloc'ed so that the array actually
 * has room for ob_size elements.  Note that ob_size is an element count,
 * not necessarily a byte count.
 */
#define PyObject_VAR_HEAD      PyVarObject ob_base;
#define Py_INVALID_SIZE (Py_ssize_t)-1

/* Nothing is actually declared to be a PyObject, but every pointer to
 * a Python object can be cast to a PyObject*.  This is inheritance built
 * by hand.  Similarly every pointer to a variable-size Python object can,
 * in addition, be cast to PyVarObject*.
 */
#ifndef Py_GIL_DISABLED
struct _object {
#if (defined(__GNUC__) || defined(__clang__)) \
        && !(defined __STDC_VERSION__ && __STDC_VERSION__ >= 201112L)
    // On C99 and older, anonymous union is a GCC and clang extension
    __extension__
#endif
#ifdef _MSC_VER
    // Ignore MSC warning C4201: "nonstandard extension used:
    // nameless struct/union"
    __pragma(warning(push))
    __pragma(warning(disable: 4201))
#endif
    union {
       Py_ssize_t ob_refcnt;
#if SIZEOF_VOID_P > 4
       PY_UINT32_T ob_refcnt_split[2];
#endif
    };
#ifdef _MSC_VER
    __pragma(warning(pop))
#endif

    PyTypeObject *ob_type;
};
#else
// Objects that are not owned by any thread use a thread id (tid) of zero.
// This includes both immortal objects and objects whose reference count
// fields have been merged.
#define _Py_UNOWNED_TID             0

// The shared reference count uses the two least-significant bits to store
// flags. The remaining bits are used to store the reference count.
#define _Py_REF_SHARED_SHIFT        2
#define _Py_REF_SHARED_FLAG_MASK    0x3

// The shared flags are initialized to zero.
#define _Py_REF_SHARED_INIT         0x0
#define _Py_REF_MAYBE_WEAKREF       0x1
#define _Py_REF_QUEUED              0x2
#define _Py_REF_MERGED              0x3

// Create a shared field from a refcnt and desired flags
#define _Py_REF_SHARED(refcnt, flags) (((refcnt) << _Py_REF_SHARED_SHIFT) + (flags))

struct _object {
    // ob_tid stores the thread id (or zero). It is also used by the GC and the
    // trashcan mechanism as a linked list pointer and by the GC to store the
    // computed "gc_refs" refcount.
    uintptr_t ob_tid;
    uint16_t _padding;
    PyMutex ob_mutex;           // per-object lock
    uint8_t ob_gc_bits;         // gc-related state
    uint32_t ob_ref_local;      // local reference count
    Py_ssize_t ob_ref_shared;   // shared (atomic) reference count
    PyTypeObject *ob_type;
};
#endif

/* Cast argument to PyObject* type. */
#define _PyObject_CAST(op) _Py_CAST(PyObject*, (op))

typedef struct {
    PyObject ob_base;
    Py_ssize_t ob_size; /* Number of items in variable part */
} PyVarObject;

/* Cast argument to PyVarObject* type. */
#define _PyVarObject_CAST(op) _Py_CAST(PyVarObject*, (op))


// Test if the 'x' object is the 'y' object, the same as "x is y" in Python.
PyAPI_FUNC(int) Py_Is(PyObject *x, PyObject *y);
#define Py_Is(x, y) ((x) == (y))

#if defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
PyAPI_FUNC(uintptr_t) _Py_GetThreadLocal_Addr(void);

static inline uintptr_t
_Py_ThreadId(void)
{
    uintptr_t tid;
#if defined(_MSC_VER) && defined(_M_X64)
    tid = __readgsqword(48);
#elif defined(_MSC_VER) && defined(_M_IX86)
    tid = __readfsdword(24);
#elif defined(_MSC_VER) && defined(_M_ARM64)
    tid = __getReg(18);
#elif defined(__MINGW32__) && defined(_M_X64)
    tid = __readgsqword(48);
#elif defined(__MINGW32__) && defined(_M_IX86)
    tid = __readfsdword(24);
#elif defined(__MINGW32__) && defined(_M_ARM64)
    tid = __getReg(18);
#elif defined(__i386__)
    __asm__("movl %%gs:0, %0" : "=r" (tid));  // 32-bit always uses GS
#elif defined(__MACH__) && defined(__x86_64__)
    __asm__("movq %%gs:0, %0" : "=r" (tid));  // x86_64 macOSX uses GS
#elif defined(__x86_64__)
   __asm__("movq %%fs:0, %0" : "=r" (tid));  // x86_64 Linux, BSD uses FS
#elif defined(__arm__) && __ARM_ARCH >= 7
    __asm__ ("mrc p15, 0, %0, c13, c0, 3\nbic %0, %0, #3" : "=r" (tid));
#elif defined(__aarch64__) && defined(__APPLE__)
    __asm__ ("mrs %0, tpidrro_el0" : "=r" (tid));
#elif defined(__aarch64__)
    __asm__ ("mrs %0, tpidr_el0" : "=r" (tid));
#elif defined(__powerpc64__)
    #if defined(__clang__) && _Py__has_builtin(__builtin_thread_pointer)
    tid = (uintptr_t)__builtin_thread_pointer();
    #else
    // r13 is reserved for use as system thread ID by the Power 64-bit ABI.
    register uintptr_t tp __asm__ ("r13");
    __asm__("" : "=r" (tp));
    tid = tp;
    #endif
#elif defined(__powerpc__)
    #if defined(__clang__) && _Py__has_builtin(__builtin_thread_pointer)
    tid = (uintptr_t)__builtin_thread_pointer();
    #else
    // r2 is reserved for use as system thread ID by the Power 32-bit ABI.
    register uintptr_t tp __asm__ ("r2");
    __asm__ ("" : "=r" (tp));
    tid = tp;
    #endif
#elif defined(__s390__) && defined(__GNUC__)
    // Both GCC and Clang have supported __builtin_thread_pointer
    // for s390 from long time ago.
    tid = (uintptr_t)__builtin_thread_pointer();
#elif defined(__riscv)
    #if defined(__clang__) && _Py__has_builtin(__builtin_thread_pointer)
    tid = (uintptr_t)__builtin_thread_pointer();
    #else
    // tp is Thread Pointer provided by the RISC-V ABI.
    __asm__ ("mv %0, tp" : "=r" (tid));
    #endif
#else
    // Fallback to a portable implementation if we do not have a faster
    // platform-specific implementation.
    tid = _Py_GetThreadLocal_Addr();
#endif
  return tid;
}

static inline Py_ALWAYS_INLINE int
_Py_IsOwnedByCurrentThread(PyObject *ob)
{
#ifdef _Py_THREAD_SANITIZER
    return _Py_atomic_load_uintptr_relaxed(&ob->ob_tid) == _Py_ThreadId();
#else
    return ob->ob_tid == _Py_ThreadId();
#endif
}
#endif

static inline Py_ssize_t Py_REFCNT(PyObject *ob) {
#if !defined(Py_GIL_DISABLED)
    return ob->ob_refcnt;
#else
    uint32_t local = _Py_atomic_load_uint32_relaxed(&ob->ob_ref_local);
    if (local == _Py_IMMORTAL_REFCNT_LOCAL) {
        return _Py_IMMORTAL_REFCNT;
    }
    Py_ssize_t shared = _Py_atomic_load_ssize_relaxed(&ob->ob_ref_shared);
    return _Py_STATIC_CAST(Py_ssize_t, local) +
           Py_ARITHMETIC_RIGHT_SHIFT(Py_ssize_t, shared, _Py_REF_SHARED_SHIFT);
#endif
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_REFCNT(ob) Py_REFCNT(_PyObject_CAST(ob))
#endif


// bpo-39573: The Py_SET_TYPE() function must be used to set an object type.
static inline PyTypeObject* Py_TYPE(PyObject *ob) {
    return ob->ob_type;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_TYPE(ob) Py_TYPE(_PyObject_CAST(ob))
#endif

PyAPI_DATA(PyTypeObject) PyLong_Type;
PyAPI_DATA(PyTypeObject) PyBool_Type;

// bpo-39573: The Py_SET_SIZE() function must be used to set an object size.
static inline Py_ssize_t Py_SIZE(PyObject *ob) {
    assert(ob->ob_type != &PyLong_Type);
    assert(ob->ob_type != &PyBool_Type);
    return  _PyVarObject_CAST(ob)->ob_size;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_SIZE(ob) Py_SIZE(_PyObject_CAST(ob))
#endif

static inline Py_ALWAYS_INLINE int _Py_IsImmortal(PyObject *op)
{
#if defined(Py_GIL_DISABLED)
    return (_Py_atomic_load_uint32_relaxed(&op->ob_ref_local) ==
            _Py_IMMORTAL_REFCNT_LOCAL);
#elif SIZEOF_VOID_P > 4
    return (_Py_CAST(PY_INT32_T, op->ob_refcnt) < 0);
#else
    return (op->ob_refcnt == _Py_IMMORTAL_REFCNT);
#endif
}
#define _Py_IsImmortal(op) _Py_IsImmortal(_PyObject_CAST(op))

static inline int Py_IS_TYPE(PyObject *ob, PyTypeObject *type) {
    return Py_TYPE(ob) == type;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_IS_TYPE(ob, type) Py_IS_TYPE(_PyObject_CAST(ob), (type))
#endif


// Py_SET_REFCNT() implementation for stable ABI
PyAPI_FUNC(void) _Py_SetRefcnt(PyObject *ob, Py_ssize_t refcnt);

static inline void Py_SET_REFCNT(PyObject *ob, Py_ssize_t refcnt) {
#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030d0000
    // Stable ABI implements Py_SET_REFCNT() as a function call
    // on limited C API version 3.13 and newer.
    _Py_SetRefcnt(ob, refcnt);
#else
    // This immortal check is for code that is unaware of immortal objects.
    // The runtime tracks these objects and we should avoid as much
    // as possible having extensions inadvertently change the refcnt
    // of an immortalized object.
    if (_Py_IsImmortal(ob)) {
        return;
    }

#ifndef Py_GIL_DISABLED
    ob->ob_refcnt = refcnt;
#else
    if (_Py_IsOwnedByCurrentThread(ob)) {
        if ((size_t)refcnt > (size_t)UINT32_MAX) {
            // On overflow, make the object immortal
            ob->ob_tid = _Py_UNOWNED_TID;
            ob->ob_ref_local = _Py_IMMORTAL_REFCNT_LOCAL;
            ob->ob_ref_shared = 0;
        }
        else {
            // Set local refcount to desired refcount and shared refcount
            // to zero, but preserve the shared refcount flags.
            ob->ob_ref_local = _Py_STATIC_CAST(uint32_t, refcnt);
            ob->ob_ref_shared &= _Py_REF_SHARED_FLAG_MASK;
        }
    }
    else {
        // Set local refcount to zero and shared refcount to desired refcount.
        // Mark the object as merged.
        ob->ob_tid = _Py_UNOWNED_TID;
        ob->ob_ref_local = 0;
        ob->ob_ref_shared = _Py_REF_SHARED(refcnt, _Py_REF_MERGED);
    }
#endif  // Py_GIL_DISABLED
#endif  // Py_LIMITED_API+0 < 0x030d0000
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_SET_REFCNT(ob, refcnt) Py_SET_REFCNT(_PyObject_CAST(ob), (refcnt))
#endif


static inline void Py_SET_TYPE(PyObject *ob, PyTypeObject *type) {
    ob->ob_type = type;
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_SET_TYPE(ob, type) Py_SET_TYPE(_PyObject_CAST(ob), type)
#endif

static inline void Py_SET_SIZE(PyVarObject *ob, Py_ssize_t size) {
    assert(ob->ob_base.ob_type != &PyLong_Type);
    assert(ob->ob_base.ob_type != &PyBool_Type);
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_ssize_relaxed(&ob->ob_size, size);
#else
    ob->ob_size = size;
#endif
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_SET_SIZE(ob, size) Py_SET_SIZE(_PyVarObject_CAST(ob), (size))
#endif


/*
Type objects contain a string containing the type name (to help somewhat
in debugging), the allocation parameters (see PyObject_New() and
PyObject_NewVar()),
and methods for accessing objects of the type.  Methods are optional, a
nil pointer meaning that particular kind of access is not available for
this type.  The Py_DECREF() macro uses the tp_dealloc method without
checking for a nil pointer; it should always be implemented except if
the implementation can guarantee that the reference count will never
reach zero (e.g., for statically allocated type objects).

NB: the methods for certain type groups are now contained in separate
method blocks.
*/

typedef PyObject * (*unaryfunc)(PyObject *);
typedef PyObject * (*binaryfunc)(PyObject *, PyObject *);
typedef PyObject * (*ternaryfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*inquiry)(PyObject *);
typedef Py_ssize_t (*lenfunc)(PyObject *);
typedef PyObject *(*ssizeargfunc)(PyObject *, Py_ssize_t);
typedef PyObject *(*ssizessizeargfunc)(PyObject *, Py_ssize_t, Py_ssize_t);
typedef int(*ssizeobjargproc)(PyObject *, Py_ssize_t, PyObject *);
typedef int(*ssizessizeobjargproc)(PyObject *, Py_ssize_t, Py_ssize_t, PyObject *);
typedef int(*objobjargproc)(PyObject *, PyObject *, PyObject *);

typedef int (*objobjproc)(PyObject *, PyObject *);
typedef int (*visitproc)(PyObject *, void *);
typedef int (*traverseproc)(PyObject *, visitproc, void *);


typedef void (*freefunc)(void *);
typedef void (*destructor)(PyObject *);
typedef PyObject *(*getattrfunc)(PyObject *, char *);
typedef PyObject *(*getattrofunc)(PyObject *, PyObject *);
typedef int (*setattrfunc)(PyObject *, char *, PyObject *);
typedef int (*setattrofunc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*reprfunc)(PyObject *);
typedef Py_hash_t (*hashfunc)(PyObject *);
typedef PyObject *(*richcmpfunc) (PyObject *, PyObject *, int);
typedef PyObject *(*getiterfunc) (PyObject *);
typedef PyObject *(*iternextfunc) (PyObject *);
typedef PyObject *(*descrgetfunc) (PyObject *, PyObject *, PyObject *);
typedef int (*descrsetfunc) (PyObject *, PyObject *, PyObject *);
typedef int (*initproc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*newfunc)(PyTypeObject *, PyObject *, PyObject *);
typedef PyObject *(*allocfunc)(PyTypeObject *, Py_ssize_t);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030c0000 // 3.12
typedef PyObject *(*vectorcallfunc)(PyObject *callable, PyObject *const *args,
                                    size_t nargsf, PyObject *kwnames);
#endif

typedef struct{
    int slot;    /* slot id, see below */
    void *pfunc; /* function pointer */
} PyType_Slot;

typedef struct{
    const char* name;
    int basicsize;
    int itemsize;
    unsigned int flags;
    PyType_Slot *slots; /* terminated by slot==0. */
} PyType_Spec;

PyAPI_FUNC(PyObject*) PyType_FromSpec(PyType_Spec*);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(PyObject*) PyType_FromSpecWithBases(PyType_Spec*, PyObject*);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03040000
PyAPI_FUNC(void*) PyType_GetSlot(PyTypeObject*, int);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03090000
PyAPI_FUNC(PyObject*) PyType_FromModuleAndSpec(PyObject *, PyType_Spec *, PyObject *);
PyAPI_FUNC(PyObject *) PyType_GetModule(PyTypeObject *);
PyAPI_FUNC(void *) PyType_GetModuleState(PyTypeObject *);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030B0000
PyAPI_FUNC(PyObject *) PyType_GetName(PyTypeObject *);
PyAPI_FUNC(PyObject *) PyType_GetQualName(PyTypeObject *);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030D0000
PyAPI_FUNC(PyObject *) PyType_GetFullyQualifiedName(PyTypeObject *type);
PyAPI_FUNC(PyObject *) PyType_GetModuleName(PyTypeObject *type);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030C0000
PyAPI_FUNC(PyObject *) PyType_FromMetaclass(PyTypeObject*, PyObject*, PyType_Spec*, PyObject*);
PyAPI_FUNC(void *) PyObject_GetTypeData(PyObject *obj, PyTypeObject *cls);
PyAPI_FUNC(Py_ssize_t) PyType_GetTypeDataSize(PyTypeObject *cls);
#endif

/* Generic type check */
PyAPI_FUNC(int) PyType_IsSubtype(PyTypeObject *, PyTypeObject *);

static inline int PyObject_TypeCheck(PyObject *ob, PyTypeObject *type) {
    return Py_IS_TYPE(ob, type) || PyType_IsSubtype(Py_TYPE(ob), type);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define PyObject_TypeCheck(ob, type) PyObject_TypeCheck(_PyObject_CAST(ob), (type))
#endif

PyAPI_DATA(PyTypeObject) PyType_Type; /* built-in 'type' */
PyAPI_DATA(PyTypeObject) PyBaseObject_Type; /* built-in 'object' */
PyAPI_DATA(PyTypeObject) PySuper_Type; /* built-in 'super' */

PyAPI_FUNC(unsigned long) PyType_GetFlags(PyTypeObject*);

PyAPI_FUNC(int) PyType_Ready(PyTypeObject *);
PyAPI_FUNC(PyObject *) PyType_GenericAlloc(PyTypeObject *, Py_ssize_t);
PyAPI_FUNC(PyObject *) PyType_GenericNew(PyTypeObject *,
                                               PyObject *, PyObject *);
PyAPI_FUNC(unsigned int) PyType_ClearCache(void);
PyAPI_FUNC(void) PyType_Modified(PyTypeObject *);

/* Generic operations on objects */
PyAPI_FUNC(PyObject *) PyObject_Repr(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_Str(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_ASCII(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_Bytes(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_RichCompare(PyObject *, PyObject *, int);
PyAPI_FUNC(int) PyObject_RichCompareBool(PyObject *, PyObject *, int);
PyAPI_FUNC(PyObject *) PyObject_GetAttrString(PyObject *, const char *);
PyAPI_FUNC(int) PyObject_SetAttrString(PyObject *, const char *, PyObject *);
PyAPI_FUNC(int) PyObject_DelAttrString(PyObject *v, const char *name);
PyAPI_FUNC(int) PyObject_HasAttrString(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyObject_GetAttr(PyObject *, PyObject *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(int) PyObject_GetOptionalAttr(PyObject *, PyObject *, PyObject **);
PyAPI_FUNC(int) PyObject_GetOptionalAttrString(PyObject *, const char *, PyObject **);
#endif
PyAPI_FUNC(int) PyObject_SetAttr(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_DelAttr(PyObject *v, PyObject *name);
PyAPI_FUNC(int) PyObject_HasAttr(PyObject *, PyObject *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(int) PyObject_HasAttrWithError(PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_HasAttrStringWithError(PyObject *, const char *);
#endif
PyAPI_FUNC(PyObject *) PyObject_SelfIter(PyObject *);
PyAPI_FUNC(PyObject *) PyObject_GenericGetAttr(PyObject *, PyObject *);
PyAPI_FUNC(int) PyObject_GenericSetAttr(PyObject *, PyObject *, PyObject *);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
PyAPI_FUNC(int) PyObject_GenericSetDict(PyObject *, PyObject *, void *);
#endif
PyAPI_FUNC(Py_hash_t) PyObject_Hash(PyObject *);
PyAPI_FUNC(Py_hash_t) PyObject_HashNotImplemented(PyObject *);
PyAPI_FUNC(int) PyObject_IsTrue(PyObject *);
PyAPI_FUNC(int) PyObject_Not(PyObject *);
PyAPI_FUNC(int) PyCallable_Check(PyObject *);
PyAPI_FUNC(void) PyObject_ClearWeakRefs(PyObject *);

/* PyObject_Dir(obj) acts like Python builtins.dir(obj), returning a
   list of strings.  PyObject_Dir(NULL) is like builtins.dir(),
   returning the names of the current locals.  In this case, if there are
   no current locals, NULL is returned, and PyErr_Occurred() is false.
*/
PyAPI_FUNC(PyObject *) PyObject_Dir(PyObject *);

/* Helpers for printing recursive container types */
PyAPI_FUNC(int) Py_ReprEnter(PyObject *);
PyAPI_FUNC(void) Py_ReprLeave(PyObject *);

/* Flag bits for printing: */
#define Py_PRINT_RAW    1       /* No string quotes etc. */

/*
Type flags (tp_flags)

These flags are used to change expected features and behavior for a
particular type.

Arbitration of the flag bit positions will need to be coordinated among
all extension writers who publicly release their extensions (this will
be fewer than you might expect!).

Most flags were removed as of Python 3.0 to make room for new flags.  (Some
flags are not for backwards compatibility but to indicate the presence of an
optional feature; these flags remain of course.)

Type definitions should use Py_TPFLAGS_DEFAULT for their tp_flags value.

Code can use PyType_HasFeature(type_ob, flag_value) to test whether the
given type object has a specified feature.
*/

#ifndef Py_LIMITED_API

/* Track types initialized using _PyStaticType_InitBuiltin(). */
#define _Py_TPFLAGS_STATIC_BUILTIN (1 << 1)

/* The values array is placed inline directly after the rest of
 * the object. Implies Py_TPFLAGS_HAVE_GC.
 */
#define Py_TPFLAGS_INLINE_VALUES (1 << 2)

/* Placement of weakref pointers are managed by the VM, not by the type.
 * The VM will automatically set tp_weaklistoffset.
 */
#define Py_TPFLAGS_MANAGED_WEAKREF (1 << 3)

/* Placement of dict (and values) pointers are managed by the VM, not by the type.
 * The VM will automatically set tp_dictoffset. Implies Py_TPFLAGS_HAVE_GC.
 */
#define Py_TPFLAGS_MANAGED_DICT (1 << 4)

#define Py_TPFLAGS_PREHEADER (Py_TPFLAGS_MANAGED_WEAKREF | Py_TPFLAGS_MANAGED_DICT)

/* Set if instances of the type object are treated as sequences for pattern matching */
#define Py_TPFLAGS_SEQUENCE (1 << 5)
/* Set if instances of the type object are treated as mappings for pattern matching */
#define Py_TPFLAGS_MAPPING (1 << 6)
#endif

/* Disallow creating instances of the type: set tp_new to NULL and don't create
 * the "__new__" key in the type dictionary. */
#define Py_TPFLAGS_DISALLOW_INSTANTIATION (1UL << 7)

/* Set if the type object is immutable: type attributes cannot be set nor deleted */
#define Py_TPFLAGS_IMMUTABLETYPE (1UL << 8)

/* Set if the type object is dynamically allocated */
#define Py_TPFLAGS_HEAPTYPE (1UL << 9)

/* Set if the type allows subclassing */
#define Py_TPFLAGS_BASETYPE (1UL << 10)

/* Set if the type implements the vectorcall protocol (PEP 590) */
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030C0000
#define Py_TPFLAGS_HAVE_VECTORCALL (1UL << 11)
#ifndef Py_LIMITED_API
// Backwards compatibility alias for API that was provisional in Python 3.8
#define _Py_TPFLAGS_HAVE_VECTORCALL Py_TPFLAGS_HAVE_VECTORCALL
#endif
#endif

/* Set if the type is 'ready' -- fully initialized */
#define Py_TPFLAGS_READY (1UL << 12)

/* Set while the type is being 'readied', to prevent recursive ready calls */
#define Py_TPFLAGS_READYING (1UL << 13)

/* Objects support garbage collection (see objimpl.h) */
#define Py_TPFLAGS_HAVE_GC (1UL << 14)

/* These two bits are preserved for Stackless Python, next after this is 17 */
#ifdef STACKLESS
#define Py_TPFLAGS_HAVE_STACKLESS_EXTENSION (3UL << 15)
#else
#define Py_TPFLAGS_HAVE_STACKLESS_EXTENSION 0
#endif

/* Objects behave like an unbound method */
#define Py_TPFLAGS_METHOD_DESCRIPTOR (1UL << 17)

/* Unused. Legacy flag */
#define Py_TPFLAGS_VALID_VERSION_TAG  (1UL << 19)

/* Type is abstract and cannot be instantiated */
#define Py_TPFLAGS_IS_ABSTRACT (1UL << 20)

// This undocumented flag gives certain built-ins their unique pattern-matching
// behavior, which allows a single positional subpattern to match against the
// subject itself (rather than a mapped attribute on it):
#define _Py_TPFLAGS_MATCH_SELF (1UL << 22)

/* Items (ob_size*tp_itemsize) are found at the end of an instance's memory */
#define Py_TPFLAGS_ITEMS_AT_END (1UL << 23)

/* These flags are used to determine if a type is a subclass. */
#define Py_TPFLAGS_LONG_SUBCLASS        (1UL << 24)
#define Py_TPFLAGS_LIST_SUBCLASS        (1UL << 25)
#define Py_TPFLAGS_TUPLE_SUBCLASS       (1UL << 26)
#define Py_TPFLAGS_BYTES_SUBCLASS       (1UL << 27)
#define Py_TPFLAGS_UNICODE_SUBCLASS     (1UL << 28)
#define Py_TPFLAGS_DICT_SUBCLASS        (1UL << 29)
#define Py_TPFLAGS_BASE_EXC_SUBCLASS    (1UL << 30)
#define Py_TPFLAGS_TYPE_SUBCLASS        (1UL << 31)

#define Py_TPFLAGS_DEFAULT  ( \
                 Py_TPFLAGS_HAVE_STACKLESS_EXTENSION | \
                0)

/* NOTE: Some of the following flags reuse lower bits (removed as part of the
 * Python 3.0 transition). */

/* The following flags are kept for compatibility; in previous
 * versions they indicated presence of newer tp_* fields on the
 * type struct.
 * Starting with 3.8, binary compatibility of C extensions across
 * feature releases of Python is not supported anymore (except when
 * using the stable ABI, in which all classes are created dynamically,
 * using the interpreter's memory layout.)
 * Note that older extensions using the stable ABI set these flags,
 * so the bits must not be repurposed.
 */
#define Py_TPFLAGS_HAVE_FINALIZE (1UL << 0)
#define Py_TPFLAGS_HAVE_VERSION_TAG   (1UL << 18)


/*
The macros Py_INCREF(op) and Py_DECREF(op) are used to increment or decrement
reference counts.  Py_DECREF calls the object's deallocator function when
the refcount falls to 0; for
objects that don't contain references to other objects or heap memory
this can be the standard function free().  Both macros can be used
wherever a void expression is allowed.  The argument must not be a
NULL pointer.  If it may be NULL, use Py_XINCREF/Py_XDECREF instead.
The macro _Py_NewReference(op) initialize reference counts to 1, and
in special builds (Py_REF_DEBUG, Py_TRACE_REFS) performs additional
bookkeeping appropriate to the special build.

We assume that the reference count field can never overflow; this can
be proven when the size of the field is the same as the pointer size, so
we ignore the possibility.  Provided a C int is at least 32 bits (which
is implicitly assumed in many parts of this code), that's enough for
about 2**31 references to an object.

XXX The following became out of date in Python 2.2, but I'm not sure
XXX what the full truth is now.  Certainly, heap-allocated type objects
XXX can and should be deallocated.
Type objects should never be deallocated; the type pointer in an object
is not considered to be a reference to the type object, to save
complications in the deallocation function.  (This is actually a
decision that's up to the implementer of each new type so if you want,
you can count such references to the type object.)
*/

#if defined(Py_REF_DEBUG) && !defined(Py_LIMITED_API)
PyAPI_FUNC(void) _Py_NegativeRefcount(const char *filename, int lineno,
                                      PyObject *op);
PyAPI_FUNC(void) _Py_INCREF_IncRefTotal(void);
PyAPI_FUNC(void) _Py_DECREF_DecRefTotal(void);
#endif  // Py_REF_DEBUG && !Py_LIMITED_API

PyAPI_FUNC(void) _Py_Dealloc(PyObject *);

/*
These are provided as conveniences to Python runtime embedders, so that
they can have object code that is not dependent on Python compilation flags.
*/
PyAPI_FUNC(void) Py_IncRef(PyObject *);
PyAPI_FUNC(void) Py_DecRef(PyObject *);

// Similar to Py_IncRef() and Py_DecRef() but the argument must be non-NULL.
// Private functions used by Py_INCREF() and Py_DECREF().
PyAPI_FUNC(void) _Py_IncRef(PyObject *);
PyAPI_FUNC(void) _Py_DecRef(PyObject *);

static inline Py_ALWAYS_INLINE void Py_INCREF(PyObject *op)
{
#if defined(Py_LIMITED_API) && (Py_LIMITED_API+0 >= 0x030c0000 || defined(Py_REF_DEBUG))
    // Stable ABI implements Py_INCREF() as a function call on limited C API
    // version 3.12 and newer, and on Python built in debug mode. _Py_IncRef()
    // was added to Python 3.10.0a7, use Py_IncRef() on older Python versions.
    // Py_IncRef() accepts NULL whereas _Py_IncRef() doesn't.
#  if Py_LIMITED_API+0 >= 0x030a00A7
    _Py_IncRef(op);
#  else
    Py_IncRef(op);
#  endif
#else
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if defined(Py_GIL_DISABLED)
    uint32_t local = _Py_atomic_load_uint32_relaxed(&op->ob_ref_local);
    uint32_t new_local = local + 1;
    if (new_local == 0) {
        // local is equal to _Py_IMMORTAL_REFCNT: do nothing
        return;
    }
    if (_Py_IsOwnedByCurrentThread(op)) {
        _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, new_local);
    }
    else {
        _Py_atomic_add_ssize(&op->ob_ref_shared, (1 << _Py_REF_SHARED_SHIFT));
    }
#elif SIZEOF_VOID_P > 4
    // Portable saturated add, branching on the carry flag and set low bits
    PY_UINT32_T cur_refcnt = op->ob_refcnt_split[PY_BIG_ENDIAN];
    PY_UINT32_T new_refcnt = cur_refcnt + 1;
    if (new_refcnt == 0) {
        // cur_refcnt is equal to _Py_IMMORTAL_REFCNT: the object is immortal,
        // do nothing
        return;
    }
    op->ob_refcnt_split[PY_BIG_ENDIAN] = new_refcnt;
#else
    // Explicitly check immortality against the immortal value
    if (_Py_IsImmortal(op)) {
        return;
    }
    op->ob_refcnt++;
#endif
    _Py_INCREF_STAT_INC();
#ifdef Py_REF_DEBUG
    _Py_INCREF_IncRefTotal();
#endif
#endif
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_INCREF(op) Py_INCREF(_PyObject_CAST(op))
#endif


#if !defined(Py_LIMITED_API) && defined(Py_GIL_DISABLED)
// Implements Py_DECREF on objects not owned by the current thread.
PyAPI_FUNC(void) _Py_DecRefShared(PyObject *);
PyAPI_FUNC(void) _Py_DecRefSharedDebug(PyObject *, const char *, int);

// Called from Py_DECREF by the owning thread when the local refcount reaches
// zero. The call will deallocate the object if the shared refcount is also
// zero. Otherwise, the thread gives up ownership and merges the reference
// count fields.
PyAPI_FUNC(void) _Py_MergeZeroLocalRefcount(PyObject *);
#endif

#if defined(Py_LIMITED_API) && (Py_LIMITED_API+0 >= 0x030c0000 || defined(Py_REF_DEBUG))
// Stable ABI implements Py_DECREF() as a function call on limited C API
// version 3.12 and newer, and on Python built in debug mode. _Py_DecRef() was
// added to Python 3.10.0a7, use Py_DecRef() on older Python versions.
// Py_DecRef() accepts NULL whereas _Py_IncRef() doesn't.
static inline void Py_DECREF(PyObject *op) {
#  if Py_LIMITED_API+0 >= 0x030a00A7
    _Py_DecRef(op);
#  else
    Py_DecRef(op);
#  endif
}
#define Py_DECREF(op) Py_DECREF(_PyObject_CAST(op))

#elif defined(Py_GIL_DISABLED) && defined(Py_REF_DEBUG)
static inline void Py_DECREF(const char *filename, int lineno, PyObject *op)
{
    uint32_t local = _Py_atomic_load_uint32_relaxed(&op->ob_ref_local);
    if (local == _Py_IMMORTAL_REFCNT_LOCAL) {
        return;
    }
    _Py_DECREF_STAT_INC();
    _Py_DECREF_DecRefTotal();
    if (_Py_IsOwnedByCurrentThread(op)) {
        if (local == 0) {
            _Py_NegativeRefcount(filename, lineno, op);
        }
        local--;
        _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, local);
        if (local == 0) {
            _Py_MergeZeroLocalRefcount(op);
        }
    }
    else {
        _Py_DecRefSharedDebug(op, filename, lineno);
    }
}
#define Py_DECREF(op) Py_DECREF(__FILE__, __LINE__, _PyObject_CAST(op))

#elif defined(Py_GIL_DISABLED)
static inline void Py_DECREF(PyObject *op)
{
    uint32_t local = _Py_atomic_load_uint32_relaxed(&op->ob_ref_local);
    if (local == _Py_IMMORTAL_REFCNT_LOCAL) {
        return;
    }
    _Py_DECREF_STAT_INC();
    if (_Py_IsOwnedByCurrentThread(op)) {
        local--;
        _Py_atomic_store_uint32_relaxed(&op->ob_ref_local, local);
        if (local == 0) {
            _Py_MergeZeroLocalRefcount(op);
        }
    }
    else {
        _Py_DecRefShared(op);
    }
}
#define Py_DECREF(op) Py_DECREF(_PyObject_CAST(op))

#elif defined(Py_REF_DEBUG)
static inline void Py_DECREF(const char *filename, int lineno, PyObject *op)
{
    if (op->ob_refcnt <= 0) {
        _Py_NegativeRefcount(filename, lineno, op);
    }
    if (_Py_IsImmortal(op)) {
        return;
    }
    _Py_DECREF_STAT_INC();
    _Py_DECREF_DecRefTotal();
    if (--op->ob_refcnt == 0) {
        _Py_Dealloc(op);
    }
}
#define Py_DECREF(op) Py_DECREF(__FILE__, __LINE__, _PyObject_CAST(op))

#else
static inline Py_ALWAYS_INLINE void Py_DECREF(PyObject *op)
{
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
    if (_Py_IsImmortal(op)) {
        return;
    }
    _Py_DECREF_STAT_INC();
    if (--op->ob_refcnt == 0) {
        _Py_Dealloc(op);
    }
}
#define Py_DECREF(op) Py_DECREF(_PyObject_CAST(op))
#endif


/* Safely decref `op` and set `op` to NULL, especially useful in tp_clear
 * and tp_dealloc implementations.
 *
 * Note that "the obvious" code can be deadly:
 *
 *     Py_XDECREF(op);
 *     op = NULL;
 *
 * Typically, `op` is something like self->containee, and `self` is done
 * using its `containee` member.  In the code sequence above, suppose
 * `containee` is non-NULL with a refcount of 1.  Its refcount falls to
 * 0 on the first line, which can trigger an arbitrary amount of code,
 * possibly including finalizers (like __del__ methods or weakref callbacks)
 * coded in Python, which in turn can release the GIL and allow other threads
 * to run, etc.  Such code may even invoke methods of `self` again, or cause
 * cyclic gc to trigger, but-- oops! --self->containee still points to the
 * object being torn down, and it may be in an insane state while being torn
 * down.  This has in fact been a rich historic source of miserable (rare &
 * hard-to-diagnose) segfaulting (and other) bugs.
 *
 * The safe way is:
 *
 *      Py_CLEAR(op);
 *
 * That arranges to set `op` to NULL _before_ decref'ing, so that any code
 * triggered as a side-effect of `op` getting torn down no longer believes
 * `op` points to a valid object.
 *
 * There are cases where it's safe to use the naive code, but they're brittle.
 * For example, if `op` points to a Python integer, you know that destroying
 * one of those can't cause problems -- but in part that relies on that
 * Python integers aren't currently weakly referencable.  Best practice is
 * to use Py_CLEAR() even if you can't think of a reason for why you need to.
 *
 * gh-98724: Use a temporary variable to only evaluate the macro argument once,
 * to avoid the duplication of side effects if the argument has side effects.
 *
 * gh-99701: If the PyObject* type is used with casting arguments to PyObject*,
 * the code can be miscompiled with strict aliasing because of type punning.
 * With strict aliasing, a compiler considers that two pointers of different
 * types cannot read or write the same memory which enables optimization
 * opportunities.
 *
 * If available, use _Py_TYPEOF() to use the 'op' type for temporary variables,
 * and so avoid type punning. Otherwise, use memcpy() which causes type erasure
 * and so prevents the compiler to reuse an old cached 'op' value after
 * Py_CLEAR().
 */
#ifdef _Py_TYPEOF
#define Py_CLEAR(op) \
    do { \
        _Py_TYPEOF(op)* _tmp_op_ptr = &(op); \
        _Py_TYPEOF(op) _tmp_old_op = (*_tmp_op_ptr); \
        if (_tmp_old_op != NULL) { \
            *_tmp_op_ptr = _Py_NULL; \
            Py_DECREF(_tmp_old_op); \
        } \
    } while (0)
#else
#define Py_CLEAR(op) \
    do { \
        PyObject **_tmp_op_ptr = _Py_CAST(PyObject**, &(op)); \
        PyObject *_tmp_old_op = (*_tmp_op_ptr); \
        if (_tmp_old_op != NULL) { \
            PyObject *_null_ptr = _Py_NULL; \
            memcpy(_tmp_op_ptr, &_null_ptr, sizeof(PyObject*)); \
            Py_DECREF(_tmp_old_op); \
        } \
    } while (0)
#endif


/* Function to use in case the object pointer can be NULL: */
static inline void Py_XINCREF(PyObject *op)
{
    if (op != _Py_NULL) {
        Py_INCREF(op);
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_XINCREF(op) Py_XINCREF(_PyObject_CAST(op))
#endif

static inline void Py_XDECREF(PyObject *op)
{
    if (op != _Py_NULL) {
        Py_DECREF(op);
    }
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_XDECREF(op) Py_XDECREF(_PyObject_CAST(op))
#endif

// Create a new strong reference to an object:
// increment the reference count of the object and return the object.
PyAPI_FUNC(PyObject*) Py_NewRef(PyObject *obj);

// Similar to Py_NewRef(), but the object can be NULL.
PyAPI_FUNC(PyObject*) Py_XNewRef(PyObject *obj);

static inline PyObject* _Py_NewRef(PyObject *obj)
{
    Py_INCREF(obj);
    return obj;
}

static inline PyObject* _Py_XNewRef(PyObject *obj)
{
    Py_XINCREF(obj);
    return obj;
}

// Py_NewRef() and Py_XNewRef() are exported as functions for the stable ABI.
// Names overridden with macros by static inline functions for best
// performances.
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define Py_NewRef(obj) _Py_NewRef(_PyObject_CAST(obj))
#  define Py_XNewRef(obj) _Py_XNewRef(_PyObject_CAST(obj))
#else
#  define Py_NewRef(obj) _Py_NewRef(obj)
#  define Py_XNewRef(obj) _Py_XNewRef(obj)
#endif


#define Py_CONSTANT_NONE 0
#define Py_CONSTANT_FALSE 1
#define Py_CONSTANT_TRUE 2
#define Py_CONSTANT_ELLIPSIS 3
#define Py_CONSTANT_NOT_IMPLEMENTED 4
#define Py_CONSTANT_ZERO 5
#define Py_CONSTANT_ONE 6
#define Py_CONSTANT_EMPTY_STR 7
#define Py_CONSTANT_EMPTY_BYTES 8
#define Py_CONSTANT_EMPTY_TUPLE 9

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(PyObject*) Py_GetConstant(unsigned int constant_id);
PyAPI_FUNC(PyObject*) Py_GetConstantBorrowed(unsigned int constant_id);
#endif


/*
_Py_NoneStruct is an object of undefined type which can be used in contexts
where NULL (nil) is not suitable (since NULL often means 'error').
*/
PyAPI_DATA(PyObject) _Py_NoneStruct; /* Don't use this directly */

#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030D0000
#  define Py_None Py_GetConstantBorrowed(Py_CONSTANT_NONE)
#else
#  define Py_None (&_Py_NoneStruct)
#endif

// Test if an object is the None singleton, the same as "x is None" in Python.
PyAPI_FUNC(int) Py_IsNone(PyObject *x);
#define Py_IsNone(x) Py_Is((x), Py_None)

/* Macro for returning Py_None from a function.
 * Only treat Py_None as immortal in the limited C API 3.12 and newer. */
#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 < 0x030c0000
#  define Py_RETURN_NONE return Py_NewRef(Py_None)
#else
#  define Py_RETURN_NONE return Py_None
#endif

/*
Py_NotImplemented is a singleton used to signal that an operation is
not implemented for a given type combination.
*/
PyAPI_DATA(PyObject) _Py_NotImplementedStruct; /* Don't use this directly */

#if defined(Py_LIMITED_API) && Py_LIMITED_API+0 >= 0x030D0000
#  define Py_NotImplemented Py_GetConstantBorrowed(Py_CONSTANT_NOT_IMPLEMENTED)
#else
#  define Py_NotImplemented (&_Py_NotImplementedStruct)
#endif

/* Macro for returning Py_NotImplemented from a function */
#define Py_RETURN_NOTIMPLEMENTED return Py_NotImplemented

/* Rich comparison opcodes */
#define Py_LT 0
#define Py_LE 1
#define Py_EQ 2
#define Py_NE 3
#define Py_GT 4
#define Py_GE 5

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030A0000
/* Result of calling PyIter_Send */
typedef enum {
    PYGEN_RETURN = 0,
    PYGEN_ERROR = -1,
    PYGEN_NEXT = 1,
} PySendResult;
#endif

/*
 * Macro for implementing rich comparisons
 *
 * Needs to be a macro because any C-comparable type can be used.
 */
#define Py_RETURN_RICHCOMPARE(val1, val2, op)                               \
    do {                                                                    \
        switch (op) {                                                       \
        case Py_EQ: if ((val1) == (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_NE: if ((val1) != (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_LT: if ((val1) < (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;   \
        case Py_GT: if ((val1) > (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;   \
        case Py_LE: if ((val1) <= (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        case Py_GE: if ((val1) >= (val2)) Py_RETURN_TRUE; Py_RETURN_FALSE;  \
        default:                                                            \
            Py_UNREACHABLE();                                               \
        }                                                                   \
    } while (0)


/*
More conventions
================

Argument Checking
-----------------

Functions that take objects as arguments normally don't check for nil
arguments, but they do check the type of the argument, and return an
error if the function doesn't apply to the type.

Failure Modes
-------------

Functions may fail for a variety of reasons, including running out of
memory.  This is communicated to the caller in two ways: an error string
is set (see errors.h), and the function result differs: functions that
normally return a pointer return NULL for failure, functions returning
an integer return -1 (which could be a legal return value too!), and
other functions return 0 for success and -1 for failure.
Callers should always check for errors before using the result.  If
an error was set, the caller must either explicitly clear it, or pass
the error on to its caller.

Reference Counts
----------------

It takes a while to get used to the proper usage of reference counts.

Functions that create an object set the reference count to 1; such new
objects must be stored somewhere or destroyed again with Py_DECREF().
Some functions that 'store' objects, such as PyTuple_SetItem() and
PyList_SetItem(),
don't increment the reference count of the object, since the most
frequent use is to store a fresh object.  Functions that 'retrieve'
objects, such as PyTuple_GetItem() and PyDict_GetItemString(), also
don't increment
the reference count, since most frequently the object is only looked at
quickly.  Thus, to retrieve an object and store it again, the caller
must call Py_INCREF() explicitly.

NOTE: functions that 'consume' a reference count, like
PyList_SetItem(), consume the reference even if the object wasn't
successfully stored, to simplify error handling.

It seems attractive to make other functions that take an object as
argument consume a reference count; however, this may quickly get
confusing (even the current practice is already confusing).  Consider
it carefully, it may save lots of calls to Py_INCREF() and Py_DECREF() at
times.
*/

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_OBJECT_H
#  include "cpython/object.h"
#  undef Py_CPYTHON_OBJECT_H
#endif


static inline int
PyType_HasFeature(PyTypeObject *type, unsigned long feature)
{
    unsigned long flags;
#ifdef Py_LIMITED_API
    // PyTypeObject is opaque in the limited C API
    flags = PyType_GetFlags(type);
#else
#   ifdef Py_GIL_DISABLED
        flags = _Py_atomic_load_ulong_relaxed(&type->tp_flags);
#   else
        flags = type->tp_flags;
#   endif
#endif
    return ((flags & feature) != 0);
}

#define PyType_FastSubclass(type, flag) PyType_HasFeature((type), (flag))

static inline int PyType_Check(PyObject *op) {
    return PyType_FastSubclass(Py_TYPE(op), Py_TPFLAGS_TYPE_SUBCLASS);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define PyType_Check(op) PyType_Check(_PyObject_CAST(op))
#endif

#define _PyType_CAST(op) \
    (assert(PyType_Check(op)), _Py_CAST(PyTypeObject*, (op)))

static inline int PyType_CheckExact(PyObject *op) {
    return Py_IS_TYPE(op, &PyType_Type);
}
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 < 0x030b0000
#  define PyType_CheckExact(op) PyType_CheckExact(_PyObject_CAST(op))
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030d0000
PyAPI_FUNC(PyObject *) PyType_GetModuleByDef(PyTypeObject *, PyModuleDef *);
#endif

#ifdef __cplusplus
}
#endif
#endif   // !Py_OBJECT_H
