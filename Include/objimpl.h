/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

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
