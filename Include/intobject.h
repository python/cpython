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
