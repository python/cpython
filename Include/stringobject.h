#ifndef Py_STRINGOBJECT_H
#define Py_STRINGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

extern DL_IMPORT(PyObject *) PyString_FromStringAndSize Py_PROTO((const char *, int));
extern DL_IMPORT(PyObject *) PyString_FromString Py_PROTO((const char *));
extern DL_IMPORT(int) PyString_Size Py_PROTO((PyObject *));
extern DL_IMPORT(char *) PyString_AsString Py_PROTO((PyObject *));
extern DL_IMPORT(void) PyString_Concat Py_PROTO((PyObject **, PyObject *));
extern DL_IMPORT(void) PyString_ConcatAndDel Py_PROTO((PyObject **, PyObject *));
extern DL_IMPORT(int) _PyString_Resize Py_PROTO((PyObject **, int));
extern DL_IMPORT(PyObject *) PyString_Format Py_PROTO((PyObject *, PyObject *));

#ifdef INTERN_STRINGS
extern DL_IMPORT(void) PyString_InternInPlace Py_PROTO((PyObject **));
extern DL_IMPORT(PyObject *) PyString_InternFromString Py_PROTO((const char *));
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
