#ifndef Py_INTOBJECT_H
#define Py_INTOBJECT_H
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

PyIntObject represents a (long) integer.  This is an immutable object;
an integer cannot change its value after creation.

There are functions to create new integer objects, to test an object
for integer-ness, and to get the integer value.  The latter functions
returns -1 and sets errno to EBADF if the object is not an PyIntObject.
None of the functions should be applied to nil objects.

The type PyIntObject is (unfortunately) exposed here so we can declare
_Py_TrueStruct and _Py_ZeroStruct below; don't use this.
*/

typedef struct {
	PyObject_HEAD
	long ob_ival;
} PyIntObject;

extern DL_IMPORT PyTypeObject PyInt_Type;

#define PyInt_Check(op) ((op)->ob_type == &PyInt_Type)

extern PyObject *PyInt_FromLong Py_PROTO((long));
extern long PyInt_AsLong Py_PROTO((PyObject *));


/*
123456789-123456789-123456789-123456789-123456789-123456789-123456789-12

False and True are special intobjects used by Boolean expressions.
All values of type Boolean must point to either of these; but in
contexts where integers are required they are integers (valued 0 and 1).
Hope these macros don't conflict with other people's.

Don't forget to apply Py_INCREF() when returning True or False!!!
*/

extern DL_IMPORT PyIntObject _Py_ZeroStruct, _Py_TrueStruct; /* Don't use these directly */

#define Py_False ((PyObject *) &_Py_ZeroStruct)
#define Py_True ((PyObject *) &_Py_TrueStruct)

/* Macro, trading safety for speed */
#define PyInt_AS_LONG(op) ((op)->ob_ival)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTOBJECT_H */
