#ifndef Py_STRINGOBJECT_H
#define Py_STRINGOBJECT_H
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

/* String object interface */

/*
Type PyStringObject represents a character string.  An extra zero byte is
reserved at the end to ensure it is zero-terminated, but a size is
present so strings with null bytes in them can be represented.  This
is an immutable object type.

There are functions to create new string objects, to test
an object for string-ness, and to get the
string value.  The latter function returns a null pointer
if the object is not of the proper type.
There is a variant that takes an explicit size as well as a
variant that assumes a zero-terminated string.  Note that none of the
functions should be applied to nil objects.
*/

/* NB The type is revealed here only because it is used in dictobject.c */

typedef struct {
	PyObject_VAR_HEAD
#ifdef CACHE_HASH
	long ob_shash;
#endif
	char ob_sval[1];
} PyStringObject;

extern DL_IMPORT PyTypeObject PyString_Type;

#define PyString_Check(op) ((op)->ob_type == &PyString_Type)

extern PyObject *PyString_FromStringAndSize Py_PROTO((char *, int));
extern PyObject *PyString_FromString Py_PROTO((char *));
extern int PyString_Size Py_PROTO((PyObject *));
extern char *PyString_AsString Py_PROTO((PyObject *));
extern void PyString_Concat Py_PROTO((PyObject **, PyObject *));
extern void PyString_ConcatAndDel Py_PROTO((PyObject **, PyObject *));
extern int _PyString_Resize Py_PROTO((PyObject **, int));
extern PyObject *PyString_Format Py_PROTO((PyObject *, PyObject *));

/* Macro, trading safety for speed */
#define PyString_AS_STRING(op) ((op)->ob_sval)

#ifdef __cplusplus
}
#endif
#endif /* !Py_STRINGOBJECT_H */
