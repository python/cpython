/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

Additional macros for modules that implement new object types.
You must first include "object.h".

NEWOBJ(type, typeobj) allocates memory for a new object of the given
type; here 'type' must be the C structure type used to represent the
object and 'typeobj' the address of the corresponding type object.
Reference count and type pointer are filled in; the rest of the bytes of
the object are *undefined*!  The resulting expression type is 'type *'.
The size of the object is actually determined by the tp_basicsize field
of the type object.

NEWVAROBJ(type, typeobj, n) is similar but allocates a variable-size
object with n extra items.  The size is computer as tp_basicsize plus
n * tp_itemsize.  This fills in the ob_size field as well.
*/

extern object *newobject PROTO((typeobject *));
extern varobject *newvarobject PROTO((typeobject *, unsigned int));

#define NEWOBJ(type, typeobj) ((type *) newobject(typeobj))
#define NEWVAROBJ(type, typeobj, n) ((type *) newvarobject(typeobj, n))

extern int StopPrint; /* Set when printing is interrupted */
