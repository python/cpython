#ifndef Py_OBJECT_H
#define Py_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Object and type object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

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
used for this (to accomodate for future changes).  The implementation
of a particular object type can cast the object pointer to the proper
type and back.

A standard interface exists for objects that contain an array of items
whose size is determined when the object is allocated.

123456789-123456789-123456789-123456789-123456789-123456789-123456789-12
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
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

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

typedef PyObject * (*unaryfunc) Py_PROTO((PyObject *));
typedef PyObject * (*binaryfunc) Py_PROTO((PyObject *, PyObject *));
typedef PyObject * (*ternaryfunc) Py_PROTO((PyObject *, PyObject *, PyObject *));
typedef int (*inquiry) Py_PROTO((PyObject *));
typedef int (*coercion) Py_PROTO((PyObject **, PyObject **));
typedef PyObject *(*intargfunc) Py_PROTO((PyObject *, int));
typedef PyObject *(*intintargfunc) Py_PROTO((PyObject *, int, int));
typedef int(*intobjargproc) Py_PROTO((PyObject *, int, PyObject *));
typedef int(*intintobjargproc) Py_PROTO((PyObject *, int, int, PyObject *));
typedef int(*objobjargproc) Py_PROTO((PyObject *, PyObject *, PyObject *));
typedef int (*getreadbufferproc) Py_PROTO((PyObject *, int, void **));
typedef int (*getwritebufferproc) Py_PROTO((PyObject *, int, void **));
typedef int (*getsegcountproc) Py_PROTO((PyObject *, int *));
typedef int (*getcharbufferproc) Py_PROTO((PyObject *, int, const char **));

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
} PyNumberMethods;

typedef struct {
	inquiry sq_length;
	binaryfunc sq_concat;
	intargfunc sq_repeat;
	intargfunc sq_item;
	intintargfunc sq_slice;
	intobjargproc sq_ass_item;
	intintobjargproc sq_ass_slice;
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
	

typedef void (*destructor) Py_PROTO((PyObject *));
typedef int (*printfunc) Py_PROTO((PyObject *, FILE *, int));
typedef PyObject *(*getattrfunc) Py_PROTO((PyObject *, char *));
typedef PyObject *(*getattrofunc) Py_PROTO((PyObject *, PyObject *));
typedef int (*setattrfunc) Py_PROTO((PyObject *, char *, PyObject *));
typedef int (*setattrofunc) Py_PROTO((PyObject *, PyObject *, PyObject *));
typedef int (*cmpfunc) Py_PROTO((PyObject *, PyObject *));
typedef PyObject *(*reprfunc) Py_PROTO((PyObject *));
typedef long (*hashfunc) Py_PROTO((PyObject *));

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

	/* More standard operations (at end for binary compatibility) */

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

	/* More spares */
	long tp_xxx5;
	long tp_xxx6;
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
extern int PyObject_Print Py_PROTO((PyObject *, FILE *, int));
extern PyObject * PyObject_Repr Py_PROTO((PyObject *));
extern PyObject * PyObject_Str Py_PROTO((PyObject *));
extern int PyObject_Compare Py_PROTO((PyObject *, PyObject *));
extern PyObject *PyObject_GetAttrString Py_PROTO((PyObject *, char *));
extern int PyObject_SetAttrString Py_PROTO((PyObject *, char *, PyObject *));
extern int PyObject_HasAttrString Py_PROTO((PyObject *, char *));
extern PyObject *PyObject_GetAttr Py_PROTO((PyObject *, PyObject *));
extern int PyObject_SetAttr Py_PROTO((PyObject *, PyObject *, PyObject *));
extern int PyObject_HasAttr Py_PROTO((PyObject *, PyObject *));
extern long PyObject_Hash Py_PROTO((PyObject *));
extern int PyObject_IsTrue Py_PROTO((PyObject *));
extern int PyObject_Not Py_PROTO((PyObject *));
extern int PyCallable_Check Py_PROTO((PyObject *));
extern int PyNumber_Coerce Py_PROTO((PyObject **, PyObject **));
extern int PyNumber_CoerceEx Py_PROTO((PyObject **, PyObject **));

/* Helpers for printing recursive container types */
extern int Py_ReprEnter Py_PROTO((PyObject *));
extern void Py_ReprLeave Py_PROTO((PyObject *));

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

#define Py_TPFLAGS_DEFAULT  (Py_TPFLAGS_HAVE_GETCHARBUFFER)

#define PyType_HasFeature(t,f)  (((t)->tp_flags & (f)) != 0)


/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

The macros Py_INCREF(op) and Py_DECREF(op) are used to increment or decrement
reference counts.  Py_DECREF calls the object's deallocator function; for
objects that don't contain references to other objects or heap memory
this can be the standard function free().  Both macros can be used
whereever a void expression is allowed.  The argument shouldn't be a
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
extern void _Py_Dealloc Py_PROTO((PyObject *));
extern void _Py_NewReference Py_PROTO((PyObject *));
extern void _Py_ForgetReference Py_PROTO((PyObject *));
extern void _Py_PrintReferences Py_PROTO((FILE *));
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
extern void inc_count Py_PROTO((PyTypeObject *));
#endif

#ifdef Py_REF_DEBUG

extern long _Py_RefTotal;

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

/* Definition of NULL, so you don't have to include <stdio.h> */

#ifndef NULL
#define NULL 0
#endif


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
#ifdef __SC__
#define statichere
#else
#define statichere static
#endif /* __SC__ */
#else /* !BAD_STATIC_FORWARD */
#define staticforward static
#define statichere static
#endif /* !BAD_STATIC_FORWARD */


/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

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

123456789-123456789-123456789-123456789-123456789-123456789-123456789-12
*/

#ifdef __cplusplus
}
#endif
#endif /* !Py_OBJECT_H */
