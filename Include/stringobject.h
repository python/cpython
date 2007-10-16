
/* String object interface */

#ifndef Py_STRINGOBJECT_H
#define Py_STRINGOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

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

/* Caching the hash (ob_shash) saves recalculation of a string's hash value.
   Interning strings (ob_sstate) tries to ensure that only one string
   object with a given value exists, so equality tests can be one pointer
   comparison.  This is generally restricted to strings that "look like"
   Python identifiers, although the sys.intern() function can be used to force
   interning of any string.
   Together, these sped the interpreter by up to 20%. */

typedef struct {
    PyObject_VAR_HEAD
    long ob_shash;
    int ob_sstate;
    char ob_sval[1];

    /* Invariants:
     *     ob_sval contains space for 'ob_size+1' elements.
     *     ob_sval[ob_size] == 0.
     *     ob_shash is the hash of the string or -1 if not computed yet.
     *     ob_sstate != 0 iff the string object is in stringobject.c's
     *       'interned' dictionary; in this case the two references
     *       from 'interned' to this object are *not counted* in ob_refcnt.
     */
} PyStringObject;

PyAPI_DATA(PyTypeObject) PyString_Type;

#define PyString_Check(op) \
                 PyType_FastSubclass(Py_Type(op), Py_TPFLAGS_STRING_SUBCLASS)
#define PyString_CheckExact(op) (Py_Type(op) == &PyString_Type)

PyAPI_FUNC(PyObject *) PyString_FromStringAndSize(const char *, Py_ssize_t);
PyAPI_FUNC(PyObject *) PyString_FromString(const char *);
PyAPI_FUNC(PyObject *) PyString_FromFormatV(const char*, va_list)
				Py_GCC_ATTRIBUTE((format(printf, 1, 0)));
PyAPI_FUNC(PyObject *) PyString_FromFormat(const char*, ...)
				Py_GCC_ATTRIBUTE((format(printf, 1, 2)));
PyAPI_FUNC(Py_ssize_t) PyString_Size(PyObject *);
PyAPI_FUNC(char *) PyString_AsString(PyObject *);
PyAPI_FUNC(PyObject *) PyString_Repr(PyObject *, int);
PyAPI_FUNC(void) PyString_Concat(PyObject **, PyObject *);
PyAPI_FUNC(void) PyString_ConcatAndDel(PyObject **, PyObject *);
PyAPI_FUNC(int) _PyString_Resize(PyObject **, Py_ssize_t);
PyAPI_FUNC(int) _PyString_Eq(PyObject *, PyObject*);
PyAPI_FUNC(PyObject *) PyString_Format(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyString_FormatLong(PyObject*, int, int,
						  int, char**, int*);
PyAPI_FUNC(PyObject *) PyString_DecodeEscape(const char *, Py_ssize_t, 
						   const char *, Py_ssize_t,
						   const char *);

PyAPI_FUNC(void) PyString_InternInPlace(PyObject **);
PyAPI_FUNC(void) PyString_InternImmortal(PyObject **);
PyAPI_FUNC(PyObject *) PyString_InternFromString(const char *);
PyAPI_FUNC(void) _Py_ReleaseInternedStrings(void);

/* Use only if you know it's a string */
#define PyString_CHECK_INTERNED(op) (((PyStringObject *)(op))->ob_sstate)

/* Macro, trading safety for speed */
#define PyString_AS_STRING(op) (assert(PyString_Check(op)),(((PyStringObject *)(op))->ob_sval))
#define PyString_GET_SIZE(op)  (assert(PyString_Check(op)),Py_Size(op))

/* _PyString_Join(sep, x) is like sep.join(x).  sep must be PyStringObject*,
   x must be an iterable object. */
PyAPI_FUNC(PyObject *) _PyString_Join(PyObject *sep, PyObject *x);

/* --- Generic Codecs ----------------------------------------------------- */

/* Create an object by decoding the encoded string s of the
   given size. */

PyAPI_FUNC(PyObject*) PyString_Decode(
    const char *s,              /* encoded string */
    Py_ssize_t size,            /* size of buffer */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a char buffer of the given size and returns a 
   Python object. */

PyAPI_FUNC(PyObject*) PyString_Encode(
    const char *s,              /* string char buffer */
    Py_ssize_t size,            /* number of chars to encode */
    const char *encoding,       /* encoding */
    const char *errors          /* error handling */
    );

/* Encodes a string object and returns the result as Python 
   object. */

PyAPI_FUNC(PyObject*) PyString_AsEncodedObject(
    PyObject *str,	 	/* string object */
    const char *encoding,	/* encoding */
    const char *errors		/* error handling */
    );

/* Encodes a string object and returns the result as Python string
   object.   
   
   If the codec returns an Unicode object, the object is converted
   back to a string using the default encoding.

   DEPRECATED - use PyString_AsEncodedObject() instead. */

PyAPI_FUNC(PyObject*) PyString_AsEncodedString(
    PyObject *str,	 	/* string object */
    const char *encoding,	/* encoding */
    const char *errors		/* error handling */
    );

/* Decodes a string object and returns the result as Python 
   object. */

PyAPI_FUNC(PyObject*) PyString_AsDecodedObject(
    PyObject *str,	 	/* string object */
    const char *encoding,	/* encoding */
    const char *errors		/* error handling */
    );

/* Decodes a string object and returns the result as Python string
   object.  
   
   If the codec returns an Unicode object, the object is converted
   back to a string using the default encoding.

   DEPRECATED - use PyString_AsDecodedObject() instead. */

PyAPI_FUNC(PyObject*) PyString_AsDecodedString(
    PyObject *str,	 	/* string object */
    const char *encoding,	/* encoding */
    const char *errors		/* error handling */
    );

/* Provides access to the internal data buffer and size of a string
   object or the default encoded version of an Unicode object. Passing
   NULL as *len parameter will force the string buffer to be
   0-terminated (passing a string with embedded NULL characters will
   cause an exception).  */

PyAPI_FUNC(int) PyString_AsStringAndSize(
    register PyObject *obj,	/* string or Unicode object */
    register char **s,		/* pointer to buffer variable */
    register Py_ssize_t *len	/* pointer to length variable or NULL
				   (only possible for 0-terminated
				   strings) */
    );
    

#ifdef __cplusplus
}
#endif
#endif /* !Py_STRINGOBJECT_H */
