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

Objects are always accessed through pointers of the type 'object *'.
The type 'object' is a structure that only contains the reference count
and the type pointer.  The actual memory allocated for an object
contains other data that can only be accessed after casting the pointer
to a pointer to a longer structure type.  This longer type must start
with the reference count and type fields; the macro OB_HEAD should be
used for this (to accomodate for future changes).  The implementation
of a particular object type can cast the object pointer to the proper
type and back.

A standard interface exists for objects that contain an array of items
whose size is determined when the object is allocated.

123456789-123456789-123456789-123456789-123456789-123456789-123456789-12
*/

#ifdef THINK_C
/* Debugging options for THINK_C (which has no -D compiler option): */
/*#define TRACE_REFS*/
/*#define REF_DEBUG*/
#endif

#ifdef TRACE_REFS
#define OB_HEAD \
	struct _object *_ob_next, *_ob_prev; \
	int ob_refcnt; \
	struct _typeobject *ob_type;
#define OB_HEAD_INIT(type) 0, 0, 1, type,
#else
#define OB_HEAD \
	unsigned int ob_refcnt; \
	struct _typeobject *ob_type;
#define OB_HEAD_INIT(type) 1, type,
#endif

#define OB_VARHEAD \
	OB_HEAD \
	unsigned int ob_size; /* Number of items in variable part */
 
typedef struct _object {
	OB_HEAD
} object;

typedef struct {
	OB_VARHEAD
} varobject;


/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

Type objects contain a string containing the type name (to help somewhat
in debugging), the allocation parameters (see newobj() and newvarobj()),
and methods for accessing objects of the type.  Methods are optional,a
nil pointer meaning that particular kind of access is not available for
this type.  The DECREF() macro uses the tp_dealloc method without
checking for a nil pointer; it should always be implemented except if
the implementation can guarantee that the reference count will never
reach zero (e.g., for type objects).

NB: the methods for certain type groups are now contained in separate
method blocks.
*/

typedef struct {
	object *(*nb_add) FPROTO((object *, object *));
	object *(*nb_subtract) FPROTO((object *, object *));
	object *(*nb_multiply) FPROTO((object *, object *));
	object *(*nb_divide) FPROTO((object *, object *));
	object *(*nb_remainder) FPROTO((object *, object *));
	object *(*nb_power) FPROTO((object *, object *));
	object *(*nb_negative) FPROTO((object *));
	object *(*nb_positive) FPROTO((object *));
} number_methods;

typedef struct {
	int (*sq_length) FPROTO((object *));
	object *(*sq_concat) FPROTO((object *, object *));
	object *(*sq_repeat) FPROTO((object *, int));
	object *(*sq_item) FPROTO((object *, int));
	object *(*sq_slice) FPROTO((object *, int, int));
	int (*sq_ass_item) FPROTO((object *, int, object *));
	int (*sq_ass_slice) FPROTO((object *, int, int, object *));
} sequence_methods;

typedef struct {
	int (*mp_length) FPROTO((object *));
	object *(*mp_subscript) FPROTO((object *, object *));
	int (*mp_ass_subscript) FPROTO((object *, object *, object *));
} mapping_methods;

typedef struct _typeobject {
	OB_VARHEAD
	char *tp_name; /* For printing */
	unsigned int tp_basicsize, tp_itemsize; /* For allocation */
	
	/* Methods to implement standard operations */
	
	void (*tp_dealloc) FPROTO((object *));
	void (*tp_print) FPROTO((object *, FILE *, int));
	object *(*tp_getattr) FPROTO((object *, char *));
	int (*tp_setattr) FPROTO((object *, char *, object *));
	int (*tp_compare) FPROTO((object *, object *));
	object *(*tp_repr) FPROTO((object *));
	
	/* Method suites for standard classes */
	
	number_methods *tp_as_number;
	sequence_methods *tp_as_sequence;
	mapping_methods *tp_as_mapping;
} typeobject;

extern typeobject Typetype; /* The type of type objects */

#define is_typeobject(op) ((op)->ob_type == &Typetype)

extern void printobject PROTO((object *, FILE *, int));
extern object * reprobject PROTO((object *));
extern int cmpobject PROTO((object *, object *));

/* Flag bits for printing: */
#define PRINT_RAW	1	/* No string quotes etc. */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

The macros INCREF(op) and DECREF(op) are used to increment or decrement
reference counts.  DECREF calls the object's deallocator function; for
objects that don't contain references to other objects or heap memory
this can be the standard function free().  Both macros can be used
whereever a void expression is allowed.  The argument shouldn't be a
NIL pointer.  The macro NEWREF(op) is used only to initialize reference
counts to 1; it is defined here for convenience.

We assume that the reference count field can never overflow; this can
be proven when the size of the field is the same as the pointer size
but even with a 16-bit reference count field it is pretty unlikely so
we ignore the possibility.  (If you are paranoid, make it a long.)

Type objects should never be deallocated; the type pointer in an object
is not considered to be a reference to the type object, to save
complications in the deallocation function.  (This is actually a
decision that's up to the implementer of each new type so if you want,
you can count such references to the type object.)

*** WARNING*** The DECREF macro must have a side-effect-free argument
since it may evaluate its argument multiple times.  (The alternative
would be to mace it a proper function or assign it to a global temporary
variable first, both of which are slower; and in a multi-threaded
environment the global variable trick is not safe.)
*/

#ifdef TRACE_REFS
#ifndef REF_DEBUG
#define REF_DEBUG
#endif
#endif

#ifndef TRACE_REFS
#define DELREF(op) (*(op)->ob_type->tp_dealloc)((object *)(op))
#define UNREF(op) /*empty*/
#endif

#ifdef REF_DEBUG
extern long ref_total;
#ifndef TRACE_REFS
#define NEWREF(op) (ref_total++, (op)->ob_refcnt = 1)
#endif
#define INCREF(op) (ref_total++, (op)->ob_refcnt++)
#define DECREF(op) \
	if (--ref_total, --(op)->ob_refcnt > 0) \
		; \
	else \
		DELREF(op)
#else
#define NEWREF(op) ((op)->ob_refcnt = 1)
#define INCREF(op) ((op)->ob_refcnt++)
#define DECREF(op) \
	if (--(op)->ob_refcnt > 0) \
		; \
	else \
		DELREF(op)
#endif


/* Definition of NULL, so you don't have to include <stdio.h> */

#ifndef NULL
#define NULL 0
#endif


/*
NoObject is an object of undefined type which can be used in contexts
where NULL (nil) is not suitable (since NULL often means 'error').

Don't forget to apply INCREF() when returning this value!!!
*/

extern object NoObject; /* Don't use this directly */

#define None (&NoObject)


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
memory.  This is communicated to the caller in two ways: 'errno' is set
to indicate the error, and the function result differs: functions that
normally return a pointer return nil for failure, functions returning
an integer return -1 (which can be a legal return value too!), and
other functions return 0 for success and the error number for failure.
Callers should always check for errors before using the result.  The
following error codes are used:

	EBADF		bad object type (first argument only)
	EINVAL		bad argument type (second and further arguments)
	ENOMEM		no memory (malloc failed)
	ENOENT		key not found in dictionary
	EDOM		index out of range or division by zero
	ERANGE		result not representable
	
	XXX any others?

Reference Counts
----------------

It takes a while to get used to the proper usage of reference counts.

Functions that create an object set the reference count to 1; such new
objects must be stored somewhere or destroyed again with DECREF().
Functions that 'store' objects such as settupleitem() and dictinsert()
don't increment the reference count of the object, since the most
frequent use is to store a fresh object.  Functions that 'retrieve'
objects such as gettupleitem() and dictlookup() also don't increment
the reference count, since most frequently the object is only looked at
quickly.  Thus, to retrieve an object and store it again, the caller
must call INCREF() explicitly.

NOTE: functions that 'consume' a reference count like dictinsert() even
consume the reference if the object wasn't stored, to simplify error
handling.

It seems attractive to make other functions that take an object as
argument consume a reference count; however this may quickly get
confusing (even the current practice is already confusing).  Consider
it carefully, it may safe lots of calls to INCREF() and DECREF() at
times.

123456789-123456789-123456789-123456789-123456789-123456789-123456789-12
*/

/* Error number interface */
#include <errno.h>

#ifndef errno
extern int errno;
#endif

#ifdef THINK_C
/* Lightspeed C doesn't define these in <errno.h> */
#define EDOM 33
#define ERANGE 34
#endif
