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

/* Two speedup hacks.  Caching the hash saves recalculation of a
   string's hash value.  Interning strings (which requires hash
   caching) tries to ensure that only one string object with a given
   value exists, so equality tests are one pointer comparison.
   Together, these can speed the interpreter up by as much as 20%.
   Each costs the size of a long or pointer per string object.  In
   addition, interned strings live until the end of times.  If you are
   concerned about memory footprint, simply comment the #define out
   here (and rebuild everything!). */
#define CACHE_HASH
#ifdef CACHE_HASH
#define INTERN_STRINGS
#endif

typedef struct {
	PyObject_VAR_HEAD
#ifdef CACHE_HASH
	long ob_shash;
#endif
#ifdef INTERN_STRINGS
	PyObject *ob_sinterned;
#endif
	char ob_sval[1];
} PyStringObject;

extern DL_IMPORT(PyTypeObject) PyString_Type;

#define PyString_Check(op) ((op)->ob_type == &PyString_Type)

extern PyObject *PyString_FromStringAndSize Py_PROTO((const char *, int));
extern PyObject *PyString_FromString Py_PROTO((const char *));
extern int PyString_Size Py_PROTO((PyObject *));
extern char *PyString_AsString Py_PROTO((PyObject *));
extern void PyString_Concat Py_PROTO((PyObject **, PyObject *));
extern void PyString_ConcatAndDel Py_PROTO((PyObject **, PyObject *));
extern int _PyString_Resize Py_PROTO((PyObject **, int));
extern PyObject *PyString_Format Py_PROTO((PyObject *, PyObject *));

#ifdef INTERN_STRINGS
extern void PyString_InternInPlace Py_PROTO((PyObject **));
extern PyObject *PyString_InternFromString Py_PROTO((const char *));
#else
#define PyString_InternInPlace(p)
#define PyString_InternFromString(cp) PyString_FromString(cp)
#endif

/* Macro, trading safety for speed */
#define PyString_AS_STRING(op) (((PyStringObject *)(op))->ob_sval)
#define PyString_GET_SIZE(op)  (((PyStringObject *)(op))->ob_size)

#ifdef __cplusplus
}
#endif
#endif /* !Py_STRINGOBJECT_H */
