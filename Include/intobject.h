/* Integer object interface */

/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

intobject represents a (long) integer.  This is an immutable object;
an integer cannot change its value after creation.

There are functions to create new integer objects, to test an object
for integer-ness, and to get the integer value.  The latter functions
returns -1 and sets errno to EBADF if the object is not an intobject.
None of the functions should be applied to nil objects.

The type intobject is (unfortunately) exposed bere so we can declare
TrueObject and FalseObject below; don't use this.
*/

typedef struct {
	OB_HEAD
	long ob_ival;
} intobject;

extern typeobject Inttype;

#define is_intobject(op) ((op)->ob_type == &Inttype)

extern object *newintobject PROTO((long));
extern long getintvalue PROTO((object *));


/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

False and True are special intobjects used by Boolean expressions.
All values of type Boolean must point to either of these; but in
contexts where integers are required they are integers (valued 0 and 1).
Hope these macros don't conflict with other people's.

Don't forget to apply INCREF() when returning True or False!!!
*/

extern intobject FalseObject, TrueObject; /* Don't use these directly */

#define False ((object *) &FalseObject)
#define True ((object *) &TrueObject)

/* Macro, trading safety for speed */
#define GETINTVALUE(op) ((op)->ob_ival)
