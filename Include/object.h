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
statically initialized type objects.)

An object has a 'reference count' that is increased or decreased when a
pointer to the object is copied or deleted; when the reference count
reaches zero there are no references to the object left and it can be
removed from the heap.

An object has a 'type' that determines what it represents and what kind
of data it contains.  An object's type is fixed when it is created.
Types themselves are represented as objects; an object contains a
pointer to the corresponding type object.  The type itself has a type
pointer pointing to the object representing the type 'type', which
contains a pointer to itself!).

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

#ifdef Py_DEBUG

/* Turn on heavy reference debugging */
#define Py_TRACE_REFS

/* Turn on reference counting */
#define Py_REF_DEBUG

#endif /* Py_DEBUG */

#ifdef Py_TRACE_REFS
#define PyObject_HEAD \
	struct _object *_ob_next, *_ob_prev; \
	int ob_refcnt; \
	struct _typeobject *ob_type;
#define PyObject_HEAD_INIT(type) 0, 0, 1, type,
#else /* !Py_TRACE_REFS */
#define PyObject_HEAD \
	int ob_refcnt; \
	struct _typeobject *ob_type;
#define PyObject_HEAD_INIT(type) 1, type,
#endif /* !Py_TRACE_REFS */

#define PyObject_VAR_HEAD \
	PyObject_HEAD \
	int ob_size; /* Number of items in variable part */
 
typedef struct _object {
	PyObject_HEAD
} PyObject;

typedef struct {
	PyObject_VAR_HEAD
} PyVarObject;


/*
Type objects contain a string containing the type name (to help somewhat
in debugging), the allocation parameters (see newobj() and newvarobj()),
and methods for accessing objects of the type.  Methods are optional,a
nil pointer meaning that particular kind of access is not available for
this type.  The Py_DECREF() macro uses the tp_dealloc method without
checking for a nil pointer; it should always be implemented except if
the implementation can guarantee that the reference count will never
reach zero (e.g., for type objects).

NB: the methods for certain type groups are now contained in separate
method blocks.
*/

typedef PyObject * (*unaryfunc)(PyObject *);
typedef PyObject * (*binaryfunc)(PyObject *, PyObject *);
typedef PyObject * (*ternaryfunc)(PyObject *, PyObject *, PyObject *);
typedef int (*inquiry)(PyObject *);
typedef int (*coercion)(PyObject **, PyObject **);
typedef PyObject *(*intargfunc)(PyObject *, int);
typedef PyObject *(*intintargfunc)(PyObject *, int, int);
typedef int(*intobjargproc)(PyObject *, int, PyObject *);
typedef int(*intintobjargproc)(PyObject *, int, int, PyObject *);
typedef int(*objobjargproc)(PyObject *, PyObject *, PyObject *);
typedef int (*getreadbufferproc)(PyObject *, int, void **);
typedef int (*getwritebufferproc)(PyObject *, int, void **);
typedef int (*getsegcountproc)(PyObject *, int *);
typedef int (*getcharbufferproc)(PyObject *, int, const char **);
typedef int (*objobjproc)(PyObject *, PyObject *);
typedef int (*visitproc)(PyObject *, void *);
typedef int (*traverseproc)(PyObject *, visitproc, void *);

typedef struct {
	binaryfunc nb_add;
	binaryfunc nb_subtract;
	binaryfunc nb_multiply;
	binaryfunc nb_divide;
	binaryfunc nb_remainder;
	binaryfunc nb_divmod;
	ternaryfunc nb_power;
	unaryfunc nb_negative;
	unaryfunc nb_positive;
	unaryfunc nb_absolute;
	inquiry nb_nonzero;
	unaryfunc nb_invert;
	binaryfunc nb_lshift;
	binaryfunc nb_rshift;
	binaryfunc nb_and;
	binaryfunc nb_xor;
	binaryfunc nb_or;
	coercion nb_coerce;
	unaryfunc nb_int;
	unaryfunc nb_long;
	unaryfunc nb_float;
	unaryfunc nb_oct;
	unaryfunc nb_hex;
	binaryfunc nb_inplace_add;
	binaryfunc nb_inplace_subtract;
	binaryfunc nb_inplace_multiply;
	binaryfunc nb_inplace_divide;
	binaryfunc nb_inplace_remainder;
	ternaryfunc nb_inplace_power;
	binaryfunc nb_inplace_lshift;
	binaryfunc nb_inplace_rshift;
	binaryfunc nb_inplace_and;
	binaryfunc nb_inplace_xor;
	binaryfunc nb_inplace_or;
} PyNumberMethods;

typedef struct {
	inquiry sq_length;
	binaryfunc sq_concat;
	intargfunc sq_repeat;
	intargfunc sq_item;
	intintargfunc sq_slice;
	intobjargproc sq_ass_item;
	intintobjargproc sq_ass_slice;
	objobjproc sq_contains;
	binaryfunc sq_inplace_concat;
	intargfunc sq_inplace_repeat;
} PySequenceMethods;

typedef struct {
	inquiry mp_length;
	binaryfunc mp_subscript;
	objobjargproc mp_ass_subscript;
} PyMappingMethods;

typedef struct {
	getreadbufferproc bf_getreadbuffer;
	getwritebufferproc bf_getwritebuffer;
	getsegcountproc bf_getsegcount;
	getcharbufferproc bf_getcharbuffer;
} PyBufferProcs;
	

typedef void (*destructor)(PyObject *);
typedef int (*printfunc)(PyObject *, FILE *, int);
typedef PyObject *(*getattrfunc)(PyObject *, char *);
typedef PyObject *(*getattrofunc)(PyObject *, PyObject *);
typedef int (*setattrfunc)(PyObject *, char *, PyObject *);
typedef int (*setattrofunc)(PyObject *, PyObject *, PyObject *);
typedef int (*cmpfunc)(PyObject *, PyObject *);
typedef PyObject *(*reprfunc)(PyObject *);
typedef long (*hashfunc)(PyObject *);

typedef struct _typeobject {
	PyObject_VAR_HEAD
	char *tp_name; /* For printing */
	int tp_basicsize, tp_itemsize; /* For allocation */
	
	/* Methods to implement standard operations */
	
	destructor tp_dealloc;
	printfunc tp_print;
	getattrfunc tp_getattr;
	setattrfunc tp_setattr;
	cmpfunc tp_compare;
	reprfunc tp_repr;
	
	/* Method suites for standard classes */
	
	PyNumberMethods *tp_as_number;
	PySequenceMethods *tp_as_sequence;
	PyMappingMethods *tp_as_mapping;

	/* More standard operations (here for binary compatibility) */

	hashfunc tp_hash;
	ternaryfunc tp_call;
	reprfunc tp_str;
	getattrofunc tp_getattro;
	setattrofunc tp_setattro;

	/* Functions to access object as input/output buffer */
	PyBufferProcs *tp_as_buffer;
	
	/* Flags to define presence of optional/expanded features */
	long tp_flags;

	char *tp_doc; /* Documentation string */

	/* call function for all accessible objects */
	traverseproc tp_traverse;
	
	/* delete references to contained objects */
	inquiry tp_clear;

	/* More spares */
	long tp_xxx7;
	long tp_xxx8;

#ifdef COUNT_ALLOCS
	/* these must be last */
	int tp_alloc;
	int tp_free;
	int tp_maxalloc;
	struct _typeobject *tp_next;
#endif
} PyTypeObject;

extern DL_IMPORT(PyTypeObject) PyType_Type; /* The type of type objects */

#define PyType_Check(op) ((op)->ob_type == &PyType_Type)

/* Generic operations on objects */
extern DL_IMPORT(int) PyObject_Print(PyObject *, FILE *, int);
extern DL_IMPORT(PyObject *) PyObject_Repr(PyObject *);
extern DL_IMPORT(PyObject *) PyObject_Str(PyObject *);
extern DL_IMPORT(int) PyObject_Compare(PyObject *, PyObject *);
extern DL_IMPORT(PyObject *) PyObject_GetAttrString(PyObject *, char *);
extern DL_IMPORT(int) PyObject_SetAttrString(PyObject *, char *, PyObject *);
extern DL_IMPORT(int) PyObject_HasAttrString(PyObject *, char *);
extern DL_IMPORT(PyObject *) PyObject_GetAttr(PyObject *, PyObject *);
extern DL_IMPORT(int) PyObject_SetAttr(PyObject *, PyObject *, PyObject *);
extern DL_IMPORT(int) PyObject_HasAttr(PyObject *, PyObject *);
extern DL_IMPORT(long) PyObject_Hash(PyObject *);
extern DL_IMPORT(int) PyObject_IsTrue(PyObject *);
extern DL_IMPORT(int) PyObject_Not(PyObject *);
extern DL_IMPORT(int) PyCallable_Check(PyObject *);
extern DL_IMPORT(int) PyNumber_Coerce(PyObject **, PyObject **);
extern DL_IMPORT(int) PyNumber_CoerceEx(PyObject **, PyObject **);

/* Helpers for printing recursive container types */
extern DL_IMPORT(int) Py_ReprEnter(PyObject *);
extern DL_IMPORT(void) Py_ReprLeave(PyObject *);

/* tstate dict key for PyObject_Compare helper */
extern PyObject *_PyCompareState_Key;

/* Helpers for hash functions */
extern DL_IMPORT(long) _Py_HashDouble(double);
extern DL_IMPORT(long) _Py_HashPointer(void*);

/* Flag bits for printing: */
#define Py_PRINT_RAW	1	/* No string quotes etc. */

/*

Type flags (tp_flags)

These flags are used to extend the type structure in a backwards-compatible
fashion. Extensions can use the flags to indicate (and test) when a given
type structure contains a new feature. The Python core will use these when
introducing new functionality between major revisions (to avoid mid-version
changes in the PYTHON_API_VERSION).

Arbitration of the flag bit positions will need to be coordinated among
all extension writers who publically release their extensions (this will
be fewer than you might expect!)..

Python 1.5.2 introduced the bf_getcharbuffer slot into PyBufferProcs.

Type definitions should use Py_TPFLAGS_DEFAULT for their tp_flags value.

Code can use PyType_HasFeature(type_ob, flag_value) to test whether the
given type object has a specified feature.

*/

/* PyBufferProcs contains bf_getcharbuffer */
#define Py_TPFLAGS_HAVE_GETCHARBUFFER  (1L<<0)

/* PySequenceMethods contains sq_contains */
#define Py_TPFLAGS_HAVE_SEQUENCE_IN (1L<<1)

/* Objects which participate in garbage collection (see objimp.h) */
#ifdef WITH_CYCLE_GC
#define Py_TPFLAGS_GC (1L<<2)
#else
#define Py_TPFLAGS_GC 0
#endif

/* PySequenceMethods and PyNumberMethods contain in-place operators */
#define Py_TPFLAGS_HAVE_INPLACEOPS (1L<<3)

#define Py_TPFLAGS_DEFAULT  (Py_TPFLAGS_HAVE_GETCHARBUFFER | \
                             Py_TPFLAGS_HAVE_SEQUENCE_IN | \
                             Py_TPFLAGS_HAVE_INPLACEOPS)

#define PyType_HasFeature(t,f)  (((t)->tp_flags & (f)) != 0)


/*
The macros Py_INCREF(op) and Py_DECREF(op) are used to increment or decrement
reference counts.  Py_DECREF calls the object's deallocator function; for
objects that don't contain references to other objects or heap memory
this can be the standard function free().  Both macros can be used
wherever a void expression is allowed.  The argument shouldn't be a
NIL pointer.  The macro _Py_NewReference(op) is used only to initialize
reference counts to 1; it is defined here for convenience.

We assume that the reference count field can never overflow; this can
be proven when the size of the field is the same as the pointer size
but even with a 16-bit reference count field it is pretty unlikely so
we ignore the possibility.  (If you are paranoid, make it a long.)

Type objects should never be deallocated; the type pointer in an object
is not considered to be a reference to the type object, to save
complications in the deallocation function.  (This is actually a
decision that's up to the implementer of each new type so if you want,
you can count such references to the type object.)

*** WARNING*** The Py_DECREF macro must have a side-effect-free argument
since it may evaluate its argument multiple times.  (The alternative
would be to mace it a proper function or assign it to a global temporary
variable first, both of which are slower; and in a multi-threaded
environment the global variable trick is not safe.)
*/

#ifdef Py_TRACE_REFS
#ifndef Py_REF_DEBUG
#define Py_REF_DEBUG
#endif
#endif

#ifdef Py_TRACE_REFS
extern DL_IMPORT(void) _Py_Dealloc(PyObject *);
extern DL_IMPORT(void) _Py_NewReference(PyObject *);
extern DL_IMPORT(void) _Py_ForgetReference(PyObject *);
extern DL_IMPORT(void) _Py_PrintReferences(FILE *);
extern DL_IMPORT(void) _Py_ResetReferences(void);
#endif

#ifndef Py_TRACE_REFS
#ifdef COUNT_ALLOCS
#define _Py_Dealloc(op) ((op)->ob_type->tp_free++, (*(op)->ob_type->tp_dealloc)((PyObject *)(op)))
#define _Py_ForgetReference(op) ((op)->ob_type->tp_free++)
#else /* !COUNT_ALLOCS */
#define _Py_Dealloc(op) (*(op)->ob_type->tp_dealloc)((PyObject *)(op))
#define _Py_ForgetReference(op) /*empty*/
#endif /* !COUNT_ALLOCS */
#endif /* !Py_TRACE_REFS */

#ifdef COUNT_ALLOCS
extern DL_IMPORT(void) inc_count(PyTypeObject *);
#endif

#ifdef Py_REF_DEBUG

extern DL_IMPORT(long) _Py_RefTotal;

#ifndef Py_TRACE_REFS
#ifdef COUNT_ALLOCS
#define _Py_NewReference(op) (inc_count((op)->ob_type), _Py_RefTotal++, (op)->ob_refcnt = 1)
#else
#define _Py_NewReference(op) (_Py_RefTotal++, (op)->ob_refcnt = 1)
#endif
#endif /* !Py_TRACE_REFS */

#define Py_INCREF(op) (_Py_RefTotal++, (op)->ob_refcnt++)
#define Py_DECREF(op) \
	if (--_Py_RefTotal, --(op)->ob_refcnt != 0) \
		; \
	else \
		_Py_Dealloc((PyObject *)(op))
#else /* !Py_REF_DEBUG */

#ifdef COUNT_ALLOCS
#define _Py_NewReference(op) (inc_count((op)->ob_type), (op)->ob_refcnt = 1)
#else
#define _Py_NewReference(op) ((op)->ob_refcnt = 1)
#endif

#define Py_INCREF(op) ((op)->ob_refcnt++)
#define Py_DECREF(op) \
	if (--(op)->ob_refcnt != 0) \
		; \
	else \
		_Py_Dealloc((PyObject *)(op))
#endif /* !Py_REF_DEBUG */

/* Macros to use in case the object pointer may be NULL: */

#define Py_XINCREF(op) if ((op) == NULL) ; else Py_INCREF(op)
#define Py_XDECREF(op) if ((op) == NULL) ; else Py_DECREF(op)

/*
_Py_NoneStruct is an object of undefined type which can be used in contexts
where NULL (nil) is not suitable (since NULL often means 'error').

Don't forget to apply Py_INCREF() when returning this value!!!
*/

extern DL_IMPORT(PyObject) _Py_NoneStruct; /* Don't use this directly */

#define Py_None (&_Py_NoneStruct)


/*
A common programming style in Python requires the forward declaration
of static, initialized structures, e.g. for a type object that is used
by the functions whose address must be used in the initializer.
Some compilers (notably SCO ODT 3.0, I seem to remember early AIX as
well) botch this if you use the static keyword for both declarations
(they allocate two objects, and use the first, uninitialized one until
the second declaration is encountered).  Therefore, the forward
declaration should use the 'forwardstatic' keyword.  This expands to
static on most systems, but to extern on a few.  The actual storage
and name will still be static because the second declaration is
static, so no linker visible symbols will be generated.  (Standard C
compilers take offense to the extern forward declaration of a static
object, so I can't just put extern in all cases. :-( )
*/

#ifdef BAD_STATIC_FORWARD
#define staticforward extern
#define statichere static
#else /* !BAD_STATIC_FORWARD */
#define staticforward static
#define statichere static
#endif /* !BAD_STATIC_FORWARD */


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
Callers should always check for errors before using the result.

Reference Counts
----------------

It takes a while to get used to the proper usage of reference counts.

Functions that create an object set the reference count to 1; such new
objects must be stored somewhere or destroyed again with Py_DECREF().
Functions that 'store' objects such as PyTuple_SetItem() and
PyDict_SetItemString()
don't increment the reference count of the object, since the most
frequent use is to store a fresh object.  Functions that 'retrieve'
objects such as PyTuple_GetItem() and PyDict_GetItemString() also
don't increment
the reference count, since most frequently the object is only looked at
quickly.  Thus, to retrieve an object and store it again, the caller
must call Py_INCREF() explicitly.

NOTE: functions that 'consume' a reference count like
PyList_SetItemString() even consume the reference if the object wasn't
stored, to simplify error handling.

It seems attractive to make other functions that take an object as
argument consume a reference count; however this may quickly get
confusing (even the current practice is already confusing).  Consider
it carefully, it may save lots of calls to Py_INCREF() and Py_DECREF() at
times.
*/

/*
  trashcan
  CT 2k0130
  non-recursively destroy nested objects

  CT 2k0223
  redefinition for better locality and less overhead.

  Objects that want to be recursion safe need to use
  the macro's 
		Py_TRASHCAN_SAFE_BEGIN(name)
  and
		Py_TRASHCAN_SAFE_END(name)
  surrounding their actual deallocation code.

  It would be nice to do this using the thread state.
  Also, we could do an exact stack measure then.
  Unfortunately, deallocations also take place when
  the thread state is undefined.

  CT 2k0422 complete rewrite.
  There is no need to allocate new objects.
  Everything is done vialob_refcnt and ob_type now.
  Adding support for free-threading should be easy, too.
*/

#define PyTrash_UNWIND_LEVEL 50

#define Py_TRASHCAN_SAFE_BEGIN(op) \
	{ \
		++_PyTrash_delete_nesting; \
		if (_PyTrash_delete_nesting < PyTrash_UNWIND_LEVEL) { \

#define Py_TRASHCAN_SAFE_END(op) \
		;} \
		else \
			_PyTrash_deposit_object((PyObject*)op);\
		--_PyTrash_delete_nesting; \
		if (_PyTrash_delete_later && _PyTrash_delete_nesting <= 0) \
			_PyTrash_destroy_chain(); \
	} \

extern DL_IMPORT(void) _PyTrash_deposit_object(PyObject*);
extern DL_IMPORT(void) _PyTrash_destroy_chain(void);

extern DL_IMPORT(int) _PyTrash_delete_nesting;
extern DL_IMPORT(PyObject *) _PyTrash_delete_later;

/* swap the "xx" to check the speed loss */

#define xxPy_TRASHCAN_SAFE_BEGIN(op) 
#define xxPy_TRASHCAN_SAFE_END(op) ;

#ifdef __cplusplus
}
#endif
#endif /* !Py_OBJECT_H */
