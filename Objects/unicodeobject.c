/* 

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg <mal@lemburg.com> according to the
Unicode Integration Proposal (see file Misc/unicode.txt).

Copyright (c) Corporation for National Research Initiatives.


 Original header:
 --------------------------------------------------------------------

 * Yet another Unicode string type for Python.  This type supports the
 * 16-bit Basic Multilingual Plane (BMP) only.
 *
 * Note that this string class supports embedded NULL characters.  End
 * of string is given by the length attribute.  However, the internal
 * representation always stores a trailing NULL to make it easier to
 * use unicode strings with standard APIs.
 *
 * History:
 * 1999-01-23 fl  Created
 * 1999-01-24 fl  Added split, join, capwords; basic UTF-8 support
 * 1999-01-24 fl  Basic UCS-2 support, buffer interface, etc.
 * 1999-03-06 fl  Moved declarations to separate file, etc.
 * 1999-06-13 fl  Changed join method semantics according to Tim's proposal
 * 1999-08-10 fl  Some minor tweaks
 *
 * Written by Fredrik Lundh, January 1999.
 *
 * Copyright (c) 1999 by Secret Labs AB.
 * Copyright (c) 1999 by Fredrik Lundh.
 *
 * fredrik@pythonware.com
 * http://www.pythonware.com
 *
 * --------------------------------------------------------------------
 * This Unicode String Type is
 * 
 * Copyright (c) 1999 by Secret Labs AB
 * Copyright (c) 1999 by Fredrik Lundh
 * 
 * By obtaining, using, and/or copying this software and/or its
 * associated documentation, you agree that you have read, understood,
 * and will comply with the following terms and conditions:
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * associated documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appears in all
 * copies, and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Secret Labs
 * AB or the author not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.
 * 
 * SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * -------------------------------------------------------------------- */

#include "Python.h"

#include "unicodeobject.h"
#include "ucnhash.h"

#ifdef MS_WIN32
#include <windows.h>
#endif

/* Limit for the Unicode object free list */

#define MAX_UNICODE_FREELIST_SIZE       1024

/* Limit for the Unicode object free list stay alive optimization.

   The implementation will keep allocated Unicode memory intact for
   all objects on the free list having a size less than this
   limit. This reduces malloc() overhead for small Unicode objects.  

   At worst this will result in MAX_UNICODE_FREELIST_SIZE *
   (sizeof(PyUnicodeObject) + KEEPALIVE_SIZE_LIMIT +
   malloc()-overhead) bytes of unused garbage.

   Setting the limit to 0 effectively turns the feature off.

   Note: This is an experimental feature ! If you get core dumps when
   using Unicode objects, turn this feature off.

*/

#define KEEPALIVE_SIZE_LIMIT       9

/* Endianness switches; defaults to little endian */

#ifdef WORDS_BIGENDIAN
# define BYTEORDER_IS_BIG_ENDIAN
#else
# define BYTEORDER_IS_LITTLE_ENDIAN
#endif

/* --- Globals ------------------------------------------------------------

   The globals are initialized by the _PyUnicode_Init() API and should
   not be used before calling that API.

*/

/* The empty Unicode object */
static PyUnicodeObject *unicode_empty;

/* Free list for Unicode objects */
static PyUnicodeObject *unicode_freelist;
static int unicode_freelist_size;

/* Default encoding to use and assume when NULL is passed as encoding
   parameter; it is initialized by _PyUnicode_Init().

   Always use the PyUnicode_SetDefaultEncoding() and
   PyUnicode_GetDefaultEncoding() APIs to access this global. 

*/

static char unicode_default_encoding[100];

/* --- Unicode Object ----------------------------------------------------- */

static
int _PyUnicode_Resize(register PyUnicodeObject *unicode,
                      int length)
{
    void *oldstr;
    
    /* Shortcut if there's nothing much to do. */
    if (unicode->length == length)
	goto reset;

    /* Resizing unicode_empty is not allowed. */
    if (unicode == unicode_empty) {
        PyErr_SetString(PyExc_SystemError,
                        "can't resize empty unicode object");
        return -1;
    }

    /* We allocate one more byte to make sure the string is
       Ux0000 terminated -- XXX is this needed ? */
    oldstr = unicode->str;
    PyMem_RESIZE(unicode->str, Py_UNICODE, length + 1);
    if (!unicode->str) {
	unicode->str = oldstr;
        PyErr_NoMemory();
        return -1;
    }
    unicode->str[length] = 0;
    unicode->length = length;

 reset:
    /* Reset the object caches */
    if (unicode->defenc) {
        Py_DECREF(unicode->defenc);
        unicode->defenc = NULL;
    }
    unicode->hash = -1;
    
    return 0;
}

int PyUnicode_Resize(PyObject **unicode,
		     int length)
{
    PyUnicodeObject *v;

    if (unicode == NULL) {
	PyErr_BadInternalCall();
	return -1;
    }
    v = (PyUnicodeObject *)*unicode;
    if (v == NULL || !PyUnicode_Check(v) || v->ob_refcnt != 1) {
	PyErr_BadInternalCall();
	return -1;
    }
    return _PyUnicode_Resize(v, length);
}

/* We allocate one more byte to make sure the string is
   Ux0000 terminated -- XXX is this needed ? 

   XXX This allocator could further be enhanced by assuring that the
       free list never reduces its size below 1.

*/

static
PyUnicodeObject *_PyUnicode_New(int length)
{
    register PyUnicodeObject *unicode;

    /* Optimization for empty strings */
    if (length == 0 && unicode_empty != NULL) {
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

    /* Unicode freelist & memory allocation */
    if (unicode_freelist) {
        unicode = unicode_freelist;
        unicode_freelist = *(PyUnicodeObject **)unicode;
        unicode_freelist_size--;
	if (unicode->str) {
	    /* Keep-Alive optimization: we only upsize the buffer,
	       never downsize it. */
	    if ((unicode->length < length) &&
		_PyUnicode_Resize(unicode, length)) {
		PyMem_DEL(unicode->str);
		goto onError;
	    }
	}
      else {
	    unicode->str = PyMem_NEW(Py_UNICODE, length + 1);
      }
      PyObject_INIT(unicode, &PyUnicode_Type);
    }
    else {
        unicode = PyObject_NEW(PyUnicodeObject, &PyUnicode_Type);
        if (unicode == NULL)
            return NULL;
	unicode->str = PyMem_NEW(Py_UNICODE, length + 1);
    }

    if (!unicode->str) {
	PyErr_NoMemory();
	goto onError;
    }
    unicode->str[length] = 0;
    unicode->length = length;
    unicode->hash = -1;
    unicode->defenc = NULL;
    return unicode;

 onError:
    _Py_ForgetReference((PyObject *)unicode);
    PyObject_DEL(unicode);
    return NULL;
}

static
void _PyUnicode_Free(register PyUnicodeObject *unicode)
{
    if (unicode_freelist_size < MAX_UNICODE_FREELIST_SIZE) {
        /* Keep-Alive optimization */
	if (unicode->length >= KEEPALIVE_SIZE_LIMIT) {
	    PyMem_DEL(unicode->str);
	    unicode->str = NULL;
	    unicode->length = 0;
	}
	if (unicode->defenc) {
	    Py_DECREF(unicode->defenc);
	    unicode->defenc = NULL;
	}
	/* Add to free list */
        *(PyUnicodeObject **)unicode = unicode_freelist;
        unicode_freelist = unicode;
        unicode_freelist_size++;
    }
    else {
	PyMem_DEL(unicode->str);
	Py_XDECREF(unicode->defenc);
	PyObject_DEL(unicode);
    }
}

PyObject *PyUnicode_FromUnicode(const Py_UNICODE *u,
				int size)
{
    PyUnicodeObject *unicode;

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;

    /* Copy the Unicode data into the new object */
    if (u != NULL)
	memcpy(unicode->str, u, size * sizeof(Py_UNICODE));

    return (PyObject *)unicode;
}

#ifdef HAVE_WCHAR_H

PyObject *PyUnicode_FromWideChar(register const wchar_t *w,
				 int size)
{
    PyUnicodeObject *unicode;

    if (w == NULL) {
	PyErr_BadInternalCall();
	return NULL;
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;

    /* Copy the wchar_t data into the new object */
#ifdef HAVE_USABLE_WCHAR_T
    memcpy(unicode->str, w, size * sizeof(wchar_t));
#else    
    {
	register Py_UNICODE *u;
	register int i;
	u = PyUnicode_AS_UNICODE(unicode);
	for (i = size; i >= 0; i--)
	    *u++ = *w++;
    }
#endif

    return (PyObject *)unicode;
}

int PyUnicode_AsWideChar(PyUnicodeObject *unicode,
			 register wchar_t *w,
			 int size)
{
    if (unicode == NULL) {
	PyErr_BadInternalCall();
	return -1;
    }
    if (size > PyUnicode_GET_SIZE(unicode))
	size = PyUnicode_GET_SIZE(unicode);
#ifdef HAVE_USABLE_WCHAR_T
    memcpy(w, unicode->str, size * sizeof(wchar_t));
#else
    {
	register Py_UNICODE *u;
	register int i;
	u = PyUnicode_AS_UNICODE(unicode);
	for (i = size; i >= 0; i--)
	    *w++ = *u++;
    }
#endif

    return size;
}

#endif

PyObject *PyUnicode_FromObject(register PyObject *obj)
{
    return PyUnicode_FromEncodedObject(obj, NULL, "strict");
}

PyObject *PyUnicode_FromEncodedObject(register PyObject *obj,
				      const char *encoding,
				      const char *errors)
{
    const char *s;
    int len;
    int owned = 0;
    PyObject *v;
    
    if (obj == NULL) {
	PyErr_BadInternalCall();
	return NULL;
    }

    /* Coerce object */
    if (PyInstance_Check(obj)) {
	PyObject *func;
	func = PyObject_GetAttrString(obj, "__str__");
	if (func == NULL) {
	    PyErr_SetString(PyExc_TypeError,
		  "coercing to Unicode: instance doesn't define __str__");
	    return NULL;
	}
	obj = PyEval_CallObject(func, NULL);
	Py_DECREF(func);
	if (obj == NULL)
	    return NULL;
	owned = 1;
    }
    if (PyUnicode_Check(obj)) {
	Py_INCREF(obj);
	v = obj;
	if (encoding) {
	    PyErr_SetString(PyExc_TypeError,
			    "decoding Unicode is not supported");
	    return NULL;
	}
	goto done;
    }
    else if (PyString_Check(obj)) {
	s = PyString_AS_STRING(obj);
	len = PyString_GET_SIZE(obj);
    }
    else if (PyObject_AsCharBuffer(obj, &s, &len)) {
	/* Overwrite the error message with something more useful in
	   case of a TypeError. */
	if (PyErr_ExceptionMatches(PyExc_TypeError))
	    PyErr_Format(PyExc_TypeError,
			 "coercing to Unicode: need string or buffer, "
			 "%.80s found",
			 obj->ob_type->tp_name);
	goto onError;
    }

    /* Convert to Unicode */
    if (len == 0) {
	Py_INCREF(unicode_empty);
	v = (PyObject *)unicode_empty;
    }
    else 
	v = PyUnicode_Decode(s, len, encoding, errors);
 done:
    if (owned) {
	Py_DECREF(obj);
    }
    return v;

 onError:
    if (owned) {
	Py_DECREF(obj);
    }
    return NULL;
}

PyObject *PyUnicode_Decode(const char *s,
			   int size,
			   const char *encoding,
			   const char *errors)
{
    PyObject *buffer = NULL, *unicode;
    
    if (encoding == NULL) 
	encoding = PyUnicode_GetDefaultEncoding();

    /* Shortcuts for common default encodings */
    if (strcmp(encoding, "utf-8") == 0)
        return PyUnicode_DecodeUTF8(s, size, errors);
    else if (strcmp(encoding, "latin-1") == 0)
        return PyUnicode_DecodeLatin1(s, size, errors);
    else if (strcmp(encoding, "ascii") == 0)
        return PyUnicode_DecodeASCII(s, size, errors);

    /* Decode via the codec registry */
    buffer = PyBuffer_FromMemory((void *)s, size);
    if (buffer == NULL)
        goto onError;
    unicode = PyCodec_Decode(buffer, encoding, errors);
    if (unicode == NULL)
        goto onError;
    if (!PyUnicode_Check(unicode)) {
        PyErr_Format(PyExc_TypeError,
                     "decoder did not return an unicode object (type=%.400s)",
                     unicode->ob_type->tp_name);
        Py_DECREF(unicode);
        goto onError;
    }
    Py_DECREF(buffer);
    return unicode;
    
 onError:
    Py_XDECREF(buffer);
    return NULL;
}

PyObject *PyUnicode_Encode(const Py_UNICODE *s,
			   int size,
			   const char *encoding,
			   const char *errors)
{
    PyObject *v, *unicode;
    
    unicode = PyUnicode_FromUnicode(s, size);
    if (unicode == NULL)
	return NULL;
    v = PyUnicode_AsEncodedString(unicode, encoding, errors);
    Py_DECREF(unicode);
    return v;
}

PyObject *PyUnicode_AsEncodedString(PyObject *unicode,
                                    const char *encoding,
                                    const char *errors)
{
    PyObject *v;
    
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }

    if (encoding == NULL) 
	encoding = PyUnicode_GetDefaultEncoding();

    /* Shortcuts for common default encodings */
    if (errors == NULL) {
	if (strcmp(encoding, "utf-8") == 0)
        return PyUnicode_AsUTF8String(unicode);
	else if (strcmp(encoding, "latin-1") == 0)
	    return PyUnicode_AsLatin1String(unicode);
	else if (strcmp(encoding, "ascii") == 0)
	    return PyUnicode_AsASCIIString(unicode);
    }

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    /* XXX Should we really enforce this ? */
    if (!PyString_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "encoder did not return a string object (type=%.400s)",
                     v->ob_type->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;
    
 onError:
    return NULL;
}

/* Return a Python string holding the default encoded value of the
   Unicode object. 

   The resulting string is cached in the Unicode object for subsequent
   usage by this function. The cached version is needed to implement
   the character buffer interface and will live (at least) as long as
   the Unicode object itself.

   The refcount of the string is *not* incremented.

   *** Exported for internal use by the interpreter only !!! ***

*/

PyObject *_PyUnicode_AsDefaultEncodedString(PyObject *unicode,
					    const char *errors)
{
    PyObject *v = ((PyUnicodeObject *)unicode)->defenc;

    if (v)
        return v;
    v = PyUnicode_AsEncodedString(unicode, NULL, errors);
    if (v && errors == NULL)
        ((PyUnicodeObject *)unicode)->defenc = v;
    return v;
}

Py_UNICODE *PyUnicode_AsUnicode(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }
    return PyUnicode_AS_UNICODE(unicode);

 onError:
    return NULL;
}

int PyUnicode_GetSize(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        goto onError;
    }
    return PyUnicode_GET_SIZE(unicode);

 onError:
    return -1;
}

const char *PyUnicode_GetDefaultEncoding(void)
{
    return unicode_default_encoding;
}

int PyUnicode_SetDefaultEncoding(const char *encoding)
{
    PyObject *v;
    
    /* Make sure the encoding is valid. As side effect, this also
       loads the encoding into the codec registry cache. */
    v = _PyCodec_Lookup(encoding);
    if (v == NULL)
	goto onError;
    Py_DECREF(v);
    strncpy(unicode_default_encoding,
	    encoding, 
	    sizeof(unicode_default_encoding));
    return 0;

 onError:
    return -1;
}

/* --- UTF-8 Codec -------------------------------------------------------- */

static 
char utf8_code_length[256] = {
    /* Map UTF-8 encoded prefix byte to sequence length.  zero means
       illegal prefix.  see RFC 2279 for details */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 0, 0
};

static
int utf8_decoding_error(const char **source,
                        Py_UNICODE **dest,
                        const char *errors,
                        const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors,"strict") == 0)) {
        PyErr_Format(PyExc_UnicodeError,
                     "UTF-8 decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
        (*source)++;
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
        (*source)++;
        **dest = Py_UNICODE_REPLACEMENT_CHARACTER;
        (*dest)++;
        return 0;
    }
    else {
        PyErr_Format(PyExc_ValueError,
                     "UTF-8 decoding error; unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

PyObject *PyUnicode_DecodeUTF8(const char *s,
			       int size,
			       const char *errors)
{
    int n;
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";

    /* Note: size will always be longer than the resulting Unicode
       character count */
    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-8 encoded data */
    p = unicode->str;
    e = s + size;

    while (s < e) {
        Py_UCS4 ch = (unsigned char)*s;

        if (ch < 0x80) {
            *p++ = (Py_UNICODE)ch;
            s++;
            continue;
        }

        n = utf8_code_length[ch];

        if (s + n > e) {
	    errmsg = "unexpected end of data";
	    goto utf8Error;
	}

        switch (n) {

        case 0:
            errmsg = "unexpected code byte";
	    goto utf8Error;
            break;

        case 1:
            errmsg = "internal error";
	    goto utf8Error;
            break;

        case 2:
            if ((s[1] & 0xc0) != 0x80) {
                errmsg = "invalid data";
		goto utf8Error;
	    }
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            if (ch < 0x80) {
                errmsg = "illegal encoding";
		goto utf8Error;
	    }
	    else
		*p++ = (Py_UNICODE)ch;
            break;

        case 3:
            if ((s[1] & 0xc0) != 0x80 || 
                (s[2] & 0xc0) != 0x80) {
                errmsg = "invalid data";
		goto utf8Error;
	    }
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            if (ch < 0x800 || (ch >= 0xd800 && ch < 0xe000)) {
                errmsg = "illegal encoding";
		goto utf8Error;
	    }
	    else
				*p++ = (Py_UNICODE)ch;
            break;

        case 4:
            if ((s[1] & 0xc0) != 0x80 ||
                (s[2] & 0xc0) != 0x80 ||
                (s[3] & 0xc0) != 0x80) {
                errmsg = "invalid data";
		goto utf8Error;
	    }
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            /* validate and convert to UTF-16 */
            if ((ch < 0x10000) ||   /* minimum value allowed for 4
                                       byte encoding */
                (ch > 0x10ffff)) {  /* maximum value allowed for
                                       UTF-16 */
                errmsg = "illegal encoding";
		goto utf8Error;
	    }
            /*  compute and append the two surrogates: */
            
            /*  translate from 10000..10FFFF to 0..FFFF */
            ch -= 0x10000;
                    
            /*  high surrogate = top 10 bits added to D800 */
            *p++ = (Py_UNICODE)(0xD800 + (ch >> 10));
                    
            /*  low surrogate = bottom 10 bits added to DC00 */
            *p++ = (Py_UNICODE)(0xDC00 + (ch & ~0xFC00));
            break;

        default:
            /* Other sizes are only needed for UCS-4 */
            errmsg = "unsupported Unicode code range";
	    goto utf8Error;
	    break;
        }
        s += n;
	continue;
	
    utf8Error:
      if (utf8_decoding_error(&s, &p, errors, errmsg))
          goto onError;
    }

    /* Adjust length */
    if (_PyUnicode_Resize(unicode, p - unicode->str))
        goto onError;

    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    return NULL;
}

/* Not used anymore, now that the encoder supports UTF-16
   surrogates. */
#if 0
static
int utf8_encoding_error(const Py_UNICODE **source,
			char **dest,
			const char *errors,
			const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "UTF-8 encoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = '?';
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "UTF-8 encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}
#endif

PyObject *PyUnicode_EncodeUTF8(const Py_UNICODE *s,
			       int size,
			       const char *errors)
{
    PyObject *v;
    char *p;
    char *q;
    Py_UCS4 ch2;
    unsigned int cbAllocated = 3 * size;
    unsigned int cbWritten = 0;
    int i = 0;

    v = PyString_FromStringAndSize(NULL, cbAllocated);
    if (v == NULL)
        return NULL;
    if (size == 0)
        return v;

    p = q = PyString_AS_STRING(v);
    while (i < size) {
        Py_UCS4 ch = s[i++];
        if (ch < 0x80) {
            *p++ = (char) ch;
            cbWritten++;
        }
        else if (ch < 0x0800) {
            *p++ = 0xc0 | (ch >> 6);
            *p++ = 0x80 | (ch & 0x3f);
            cbWritten += 2;
        }
        else {
            /* Check for high surrogate */
            if (0xD800 <= ch && ch <= 0xDBFF) {
                if (i != size) {
                    ch2 = s[i];
                    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                        
                        if (cbWritten >= (cbAllocated - 4)) {
			    /* Provide enough room for some more
			       surrogates */
			    cbAllocated += 4*10;
                            if (_PyString_Resize(&v, cbAllocated))
				goto onError;
                        }

                        /* combine the two values */
                        ch = ((ch - 0xD800)<<10 | (ch2-0xDC00))+0x10000;
                    
                        *p++ = (char)((ch >> 18) | 0xf0);
                        *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
                        i++;
                        cbWritten += 4;
                    }
                }
            }
            else {
                *p++ = (char)(0xe0 | (ch >> 12));
                cbWritten += 3;
            }
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
    }
    *p = '\0';
    if (_PyString_Resize(&v, p - q))
	goto onError;
    return v;

 onError:
    Py_DECREF(v);
    return NULL;
}

PyObject *PyUnicode_AsUTF8String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(unicode),
				PyUnicode_GET_SIZE(unicode),
				NULL);
}

/* --- UTF-16 Codec ------------------------------------------------------- */

static
int utf16_decoding_error(const Py_UNICODE **source,
			 Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors,"strict") == 0)) {
        PyErr_Format(PyExc_UnicodeError,
                     "UTF-16 decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	if (dest) {
	    **dest = Py_UNICODE_REPLACEMENT_CHARACTER;
	    (*dest)++;
	}
        return 0;
    }
    else {
        PyErr_Format(PyExc_ValueError,
                     "UTF-16 decoding error; "
		     "unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

PyObject *PyUnicode_DecodeUTF16(const char *s,
				int size,
				const char *errors,
				int *byteorder)
{
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const Py_UNICODE *q, *e;
    int bo = 0;
    const char *errmsg = "";

    /* size should be an even number */
    if (size % sizeof(Py_UNICODE) != 0) {
	if (utf16_decoding_error(NULL, NULL, errors, "truncated data"))
	    return NULL;
	/* The remaining input chars are ignored if we fall through
           here... */
    }

    /* Note: size will always be longer than the resulting Unicode
       character count */
    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-16 encoded data */
    p = unicode->str;
    q = (Py_UNICODE *)s;
    e = q + (size / sizeof(Py_UNICODE));

    if (byteorder)
	bo = *byteorder;

    while (q < e) {
	register Py_UNICODE ch = *q++;

	/* Check for BOM marks (U+FEFF) in the input and adjust
	   current byte order setting accordingly. Swap input
	   bytes if needed. (This assumes sizeof(Py_UNICODE) == 2
	   !) */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
	if (ch == 0xFEFF) {
	    bo = -1;
	    continue;
	} else if (ch == 0xFFFE) {
	    bo = 1;
	    continue;
	}
	if (bo == 1)
	    ch = (ch >> 8) | (ch << 8);
#else    
	if (ch == 0xFEFF) {
	    bo = 1;
	    continue;
	} else if (ch == 0xFFFE) {
	    bo = -1;
	    continue;
	}
	if (bo == -1)
	    ch = (ch >> 8) | (ch << 8);
#endif
	if (ch < 0xD800 || ch > 0xDFFF) {
	    *p++ = ch;
	    continue;
	}

	/* UTF-16 code pair: */
	if (q >= e) {
	    errmsg = "unexpected end of data";
	    goto utf16Error;
	}
	if (0xDC00 <= *q && *q <= 0xDFFF) {
	    q++;
	    if (0xD800 <= *q && *q <= 0xDBFF) {
		/* This is valid data (a UTF-16 surrogate pair), but
		   we are not able to store this information since our
		   Py_UNICODE type only has 16 bits... this might
		   change someday, even though it's unlikely. */
		errmsg = "code pairs are not supported";
		goto utf16Error;
	    }
	    else
		continue;
	}
	errmsg = "illegal encoding";
	/* Fall through to report the error */

    utf16Error:
	if (utf16_decoding_error(&q, &p, errors, errmsg))
	    goto onError;
    }

    if (byteorder)
        *byteorder = bo;

    /* Adjust length */
    if (_PyUnicode_Resize(unicode, p - unicode->str))
        goto onError;

    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    return NULL;
}

#undef UTF16_ERROR

PyObject *PyUnicode_EncodeUTF16(const Py_UNICODE *s,
				int size,
				const char *errors,
				int byteorder)
{
    PyObject *v;
    Py_UNICODE *p;
    char *q;

    /* We don't create UTF-16 pairs... */
    v = PyString_FromStringAndSize(NULL, 
			sizeof(Py_UNICODE) * (size + (byteorder == 0)));
    if (v == NULL)
        return NULL;

    q = PyString_AS_STRING(v);
    p = (Py_UNICODE *)q;
    if (byteorder == 0)
	*p++ = 0xFEFF;
    if (size == 0)
        return v;
    if (byteorder == 0 ||
#ifdef BYTEORDER_IS_LITTLE_ENDIAN	
	byteorder == -1
#else
	byteorder == 1
#endif
	)
	memcpy(p, s, size * sizeof(Py_UNICODE));
    else
	while (size-- > 0) {
	    Py_UNICODE ch = *s++;
	    *p++ = (ch >> 8) | (ch << 8);
	}
    return v;
}

PyObject *PyUnicode_AsUTF16String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(unicode),
				 PyUnicode_GET_SIZE(unicode),
				 NULL,
				 0);
}

/* --- Unicode Escape Codec ----------------------------------------------- */

static
int unicodeescape_decoding_error(const char **source,
                                 Py_UNICODE *x,
                                 const char *errors,
                                 const char *details) 
{
    if ((errors == NULL) ||
        (strcmp(errors,"strict") == 0)) {
        PyErr_Format(PyExc_UnicodeError,
                     "Unicode-Escape decoding error: %.400s",
                     details);
        return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
        return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
        *x = Py_UNICODE_REPLACEMENT_CHARACTER;
        return 0;
    }
    else {
        PyErr_Format(PyExc_ValueError,
                     "Unicode-Escape decoding error; "
                     "unknown error handling code: %.400s",
                     errors);
        return -1;
    }
}

static _Py_UCNHashAPI *pucnHash = NULL;

static
int mystrnicmp(const char *s1, const char *s2, size_t count)
{
    char c1, c2;
    
    if (count)
    {
        do
        {
           c1 = tolower(*(s1++));
           c2 = tolower(*(s2++));
        }
        while(--count && c1 == c2);
        
        return c1 - c2;
    }
    
    return 0;
}

PyObject *PyUnicode_DecodeUnicodeEscape(const char *s,
					int size,
					const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p = NULL, *buf = NULL;
    const char *end;
    Py_UCS4 chr;
    
    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value. */
    v = _PyUnicode_New(size);
    if (v == NULL)
        goto onError;
    if (size == 0)
        return (PyObject *)v;
    p = buf = PyUnicode_AS_UNICODE(v);
    end = s + size;
    while (s < end) {
        unsigned char c;
        Py_UNICODE x;
        int i;

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (*s != '\\') {
            *p++ = (unsigned char)*s++;
            continue;
        }

        /* \ - Escapes */
        s++;
        switch (*s++) {

        /* \x escapes */
        case '\n': break;
        case '\\': *p++ = '\\'; break;
        case '\'': *p++ = '\''; break;
        case '\"': *p++ = '\"'; break;
        case 'b': *p++ = '\b'; break;
        case 'f': *p++ = '\014'; break; /* FF */
        case 't': *p++ = '\t'; break;
        case 'n': *p++ = '\n'; break;
        case 'r': *p++ = '\r'; break;
        case 'v': *p++ = '\013'; break; /* VT */
        case 'a': *p++ = '\007'; break; /* BEL, not classic C */

        /* \OOO (octal) escapes */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
            x = s[-1] - '0';
            if ('0' <= *s && *s <= '7') {
                x = (x<<3) + *s++ - '0';
                if ('0' <= *s && *s <= '7')
                    x = (x<<3) + *s++ - '0';
            }
            *p++ = x;
            break;

        /* \xXX with two hex digits */
        case 'x':
            for (x = 0, i = 0; i < 2; i++) {
                c = (unsigned char)s[i];
                if (!isxdigit(c)) {
                    if (unicodeescape_decoding_error(&s, &x, errors,
                                                     "truncated \\xXX"))
                        goto onError;
                    i++;
                    break;
                }
                x = (x<<4) & ~0xF;
                if (c >= '0' && c <= '9')
                    x += c - '0';
                else if (c >= 'a' && c <= 'f')
                    x += 10 + c - 'a';
                else
                    x += 10 + c - 'A';
            }
            s += i;
            *p++ = x;
            break;

        /* \uXXXX with 4 hex digits */
        case 'u':
            for (x = 0, i = 0; i < 4; i++) {
                c = (unsigned char)s[i];
                if (!isxdigit(c)) {
                    if (unicodeescape_decoding_error(&s, &x, errors,
                                                     "truncated \\uXXXX"))
                        goto onError;
                    i++;
                    break;
                }
                x = (x<<4) & ~0xF;
                if (c >= '0' && c <= '9')
                    x += c - '0';
                else if (c >= 'a' && c <= 'f')
                    x += 10 + c - 'a';
                else
                    x += 10 + c - 'A';
            }
            s += i;
            *p++ = x;
            break;

        /* \UXXXXXXXX with 8 hex digits */
        case 'U':
            for (chr = 0, i = 0; i < 8; i++) {
                c = (unsigned char)s[i];
                if (!isxdigit(c)) {
                    if (unicodeescape_decoding_error(&s, &x, errors,
                                                     "truncated \\uXXXX"))
                        goto onError;
                    i++;
                    break;
                }
                chr = (chr<<4) & ~0xF;
                if (c >= '0' && c <= '9')
                    chr += c - '0';
                else if (c >= 'a' && c <= 'f')
                    chr += 10 + c - 'a';
                else
                    chr += 10 + c - 'A';
            }
            s += i;
            goto store;

        case 'N':
            /* Ok, we need to deal with Unicode Character Names now,
             * make sure we've imported the hash table data...
             */
            if (pucnHash == NULL) {
                PyObject *mod = 0, *v = 0;
                mod = PyImport_ImportModule("ucnhash");
                if (mod == NULL)
                    goto onError;
                v = PyObject_GetAttrString(mod,"ucnhashAPI");
                Py_DECREF(mod);
                if (v == NULL)
                    goto onError;
                pucnHash = PyCObject_AsVoidPtr(v);
                Py_DECREF(v);
                if (pucnHash == NULL)
                    goto onError;
            }
                
            if (*s == '{') {
                const char *start = s + 1;
                const char *endBrace = start;
                unsigned long j;

                /* look for either the closing brace, or we
                 * exceed the maximum length of the unicode character names
                 */
                while (*endBrace != '}' &&
                       (unsigned int)(endBrace - start) <=
                           pucnHash->cchMax &&
                       endBrace < end)
                {
                    endBrace++;
                }
                if (endBrace != end && *endBrace == '}') {
                    j = pucnHash->hash(start, endBrace - start);
                    if (j > pucnHash->cKeys ||
                        mystrnicmp(
                            start,
                            ((_Py_UnicodeCharacterName *) 
                             (pucnHash->getValue(j)))->pszUCN,
                            (int)(endBrace - start)) != 0)
                    {
                        if (unicodeescape_decoding_error(
                                &s, &x, errors,
                                "Invalid Unicode Character Name"))
                        {
                            goto onError;
                        }
                        goto ucnFallthrough;
                    }
                    chr = ((_Py_UnicodeCharacterName *)
                           (pucnHash->getValue(j)))->value;
                    s = endBrace + 1;
                    goto store;
                } else {
                    if (unicodeescape_decoding_error(
                            &s, &x, errors,
                            "Unicode name missing closing brace"))
                        goto onError;
                    goto ucnFallthrough;
                }
                break;                
            }
            if (unicodeescape_decoding_error(
                    &s, &x, errors,
                    "Missing opening brace for Unicode Character Name escape"))
                goto onError;
ucnFallthrough:
            /* fall through on purpose */
		default:
            *p++ = '\\';
            *p++ = (unsigned char)s[-1];
            break;
store:
            /* when we get here, chr is a 32-bit unicode character */
            if (chr <= 0xffff)
                /* UCS-2 character */
                *p++ = (Py_UNICODE) chr;
            else if (chr <= 0x10ffff) {
                /* UCS-4 character.  store as two surrogate characters */
                chr -= 0x10000L;
                *p++ = 0xD800 + (Py_UNICODE) (chr >> 10);
                *p++ = 0xDC00 + (Py_UNICODE) (chr & ~0xFC00);
            } else {
                if (unicodeescape_decoding_error(
                    &s, &x, errors,
                    "Illegal Unicode character")
                    )
                    goto onError;
            }
        }
    }
    if (_PyUnicode_Resize(v, (int)(p - buf)))
		goto onError;
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

/* Return a Unicode-Escape string version of the Unicode object.

   If quotes is true, the string is enclosed in u"" or u'' quotes as
   appropriate.

*/

static const Py_UNICODE *findchar(const Py_UNICODE *s,
				  int size,
				  Py_UNICODE ch);

static
PyObject *unicodeescape_string(const Py_UNICODE *s,
                               int size,
                               int quotes)
{
    PyObject *repr;
    char *p;
    char *q;

    static const char *hexdigit = "0123456789ABCDEF";

    repr = PyString_FromStringAndSize(NULL, 2 + 6*size + 1);
    if (repr == NULL)
        return NULL;

    p = q = PyString_AS_STRING(repr);

    if (quotes) {
        *p++ = 'u';
        *p++ = (findchar(s, size, '\'') && 
                !findchar(s, size, '"')) ? '"' : '\'';
    }
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
        /* Escape quotes */
        if (quotes && (ch == q[1] || ch == '\\')) {
            *p++ = '\\';
            *p++ = (char) ch;
        } 
        /* Map 16-bit characters to '\uxxxx' */
        else if (ch >= 256) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigit[(ch >> 12) & 0xf];
            *p++ = hexdigit[(ch >> 8) & 0xf];
            *p++ = hexdigit[(ch >> 4) & 0xf];
            *p++ = hexdigit[ch & 15];
        }
        /* Map non-printable US ASCII to '\ooo' */
        else if (ch < ' ' || ch >= 128) {
            *p++ = '\\';
            *p++ = hexdigit[(ch >> 6) & 7];
            *p++ = hexdigit[(ch >> 3) & 7];
            *p++ = hexdigit[ch & 7];
        } 
        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }
    if (quotes)
        *p++ = q[1];

    *p = '\0';
    if (_PyString_Resize(&repr, p - q))
	goto onError;

    return repr;

 onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s,
					int size)
{
    return unicodeescape_string(s, size, 0);
}

PyObject *PyUnicode_AsUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
					 PyUnicode_GET_SIZE(unicode));
}

/* --- Raw Unicode Escape Codec ------------------------------------------- */

PyObject *PyUnicode_DecodeRawUnicodeEscape(const char *s,
					   int size,
					   const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p, *buf;
    const char *end;
    const char *bs;
    
    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value. */
    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = buf = PyUnicode_AS_UNICODE(v);
    end = s + size;
    while (s < end) {
	unsigned char c;
	Py_UNICODE x;
	int i;

	/* Non-escape characters are interpreted as Unicode ordinals */
	if (*s != '\\') {
	    *p++ = (unsigned char)*s++;
	    continue;
	}

	/* \u-escapes are only interpreted iff the number of leading
	   backslashes if odd */
	bs = s;
	for (;s < end;) {
	    if (*s != '\\')
		break;
	    *p++ = (unsigned char)*s++;
	}
	if (((s - bs) & 1) == 0 ||
	    s >= end ||
	    *s != 'u') {
	    continue;
	}
	p--;
	s++;

	/* \uXXXX with 4 hex digits */
	for (x = 0, i = 0; i < 4; i++) {
	    c = (unsigned char)s[i];
	    if (!isxdigit(c)) {
		if (unicodeescape_decoding_error(&s, &x, errors,
						 "truncated \\uXXXX"))
		    goto onError;
		i++;
		break;
	    }
	    x = (x<<4) & ~0xF;
	    if (c >= '0' && c <= '9')
		x += c - '0';
	    else if (c >= 'a' && c <= 'f')
		x += 10 + c - 'a';
	    else
		x += 10 + c - 'A';
	}
	s += i;
	*p++ = x;
    }
    if (_PyUnicode_Resize(v, (int)(p - buf)))
	goto onError;
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

PyObject *PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s,
					   int size)
{
    PyObject *repr;
    char *p;
    char *q;

    static const char *hexdigit = "0123456789ABCDEF";

    repr = PyString_FromStringAndSize(NULL, 6 * size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
	return repr;

    p = q = PyString_AS_STRING(repr);
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
	/* Map 16-bit characters to '\uxxxx' */
	if (ch >= 256) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigit[(ch >> 12) & 0xf];
            *p++ = hexdigit[(ch >> 8) & 0xf];
            *p++ = hexdigit[(ch >> 4) & 0xf];
            *p++ = hexdigit[ch & 15];
        }
	/* Copy everything else as-is */
	else
            *p++ = (char) ch;
    }
    *p = '\0';
    if (_PyString_Resize(&repr, p - q))
	goto onError;

    return repr;

 onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_AsRawUnicodeEscapeString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
	PyErr_BadArgument();
	return NULL;
    }
    return PyUnicode_EncodeRawUnicodeEscape(PyUnicode_AS_UNICODE(unicode),
					    PyUnicode_GET_SIZE(unicode));
}

/* --- Latin-1 Codec ------------------------------------------------------ */

PyObject *PyUnicode_DecodeLatin1(const char *s,
				 int size,
				 const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    /* Latin-1 is equivalent to the first 256 ordinals in Unicode. */
    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0)
	*p++ = (unsigned char)*s++;
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

static
int latin1_encoding_error(const Py_UNICODE **source,
			  char **dest,
			  const char *errors,
			  const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "Latin-1 encoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = '?';
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "Latin-1 encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_EncodeLatin1(const Py_UNICODE *p,
				 int size,
				 const char *errors)
{
    PyObject *repr;
    char *s, *start;

    repr = PyString_FromStringAndSize(NULL, size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
	return repr;

    s = PyString_AS_STRING(repr);
    start = s;
    while (size-- > 0) {
        Py_UNICODE ch = *p++;
	if (ch >= 256) {
	    if (latin1_encoding_error(&p, &s, errors, 
				      "ordinal not in range(256)"))
		goto onError;
	}
	else
            *s++ = (char)ch;
    }
    /* Resize if error handling skipped some characters */
    if (s - start < PyString_GET_SIZE(repr))
	if (_PyString_Resize(&repr, s - start))
	    goto onError;
    return repr;

 onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_AsLatin1String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
	PyErr_BadArgument();
	return NULL;
    }
    return PyUnicode_EncodeLatin1(PyUnicode_AS_UNICODE(unicode),
				  PyUnicode_GET_SIZE(unicode),
				  NULL);
}

/* --- 7-bit ASCII Codec -------------------------------------------------- */

static
int ascii_decoding_error(const char **source,
			 Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "ASCII decoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = Py_UNICODE_REPLACEMENT_CHARACTER;
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "ASCII decoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_DecodeASCII(const char *s,
				int size,
				const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    /* ASCII is equivalent to the first 128 ordinals in Unicode. */
    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) {
	register unsigned char c;

	c = (unsigned char)*s++;
	if (c < 128)
	    *p++ = c;
	else if (ascii_decoding_error(&s, &p, errors, 
				      "ordinal not in range(128)"))
		goto onError;
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyString_GET_SIZE(v))
	if (_PyUnicode_Resize(v, (int)(p - PyUnicode_AS_UNICODE(v))))
	    goto onError;
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

static
int ascii_encoding_error(const Py_UNICODE **source,
			 char **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "ASCII encoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = '?';
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "ASCII encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_EncodeASCII(const Py_UNICODE *p,
				int size,
				const char *errors)
{
    PyObject *repr;
    char *s, *start;

    repr = PyString_FromStringAndSize(NULL, size);
    if (repr == NULL)
        return NULL;
    if (size == 0)
	return repr;

    s = PyString_AS_STRING(repr);
    start = s;
    while (size-- > 0) {
        Py_UNICODE ch = *p++;
	if (ch >= 128) {
	    if (ascii_encoding_error(&p, &s, errors, 
				      "ordinal not in range(128)"))
		goto onError;
	}
	else
            *s++ = (char)ch;
    }
    /* Resize if error handling skipped some characters */
    if (s - start < PyString_GET_SIZE(repr))
	if (_PyString_Resize(&repr, s - start))
	    goto onError;
    return repr;

 onError:
    Py_DECREF(repr);
    return NULL;
}

PyObject *PyUnicode_AsASCIIString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
	PyErr_BadArgument();
	return NULL;
    }
    return PyUnicode_EncodeASCII(PyUnicode_AS_UNICODE(unicode),
				 PyUnicode_GET_SIZE(unicode),
				 NULL);
}

#ifdef MS_WIN32

/* --- MBCS codecs for Windows -------------------------------------------- */

PyObject *PyUnicode_DecodeMBCS(const char *s,
				int size,
				const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;

    /* First get the size of the result */
    DWORD usize = MultiByteToWideChar(CP_ACP, 0, s, size, NULL, 0);
    if (size > 0 && usize==0)
        return PyErr_SetFromWindowsErrWithFilename(0, NULL);

    v = _PyUnicode_New(usize);
    if (v == NULL)
        return NULL;
    if (usize == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    if (0 == MultiByteToWideChar(CP_ACP, 0, s, size, p, usize)) {
        Py_DECREF(v);
        return PyErr_SetFromWindowsErrWithFilename(0, NULL);
    }

    return (PyObject *)v;
}

PyObject *PyUnicode_EncodeMBCS(const Py_UNICODE *p,
				int size,
				const char *errors)
{
    PyObject *repr;
    char *s;
    DWORD mbcssize;

    /* If there are no characters, bail now! */
    if (size==0)
	    return PyString_FromString("");

    /* First get the size of the result */
    mbcssize = WideCharToMultiByte(CP_ACP, 0, p, size, NULL, 0, NULL, NULL);
    if (mbcssize==0)
        return PyErr_SetFromWindowsErrWithFilename(0, NULL);

    repr = PyString_FromStringAndSize(NULL, mbcssize);
    if (repr == NULL)
        return NULL;
    if (mbcssize == 0)
        return repr;

    /* Do the conversion */
    s = PyString_AS_STRING(repr);
    if (0 == WideCharToMultiByte(CP_ACP, 0, p, size, s, mbcssize, NULL, NULL)) {
        Py_DECREF(repr);
        return PyErr_SetFromWindowsErrWithFilename(0, NULL);
    }
    return repr;
}

#endif /* MS_WIN32 */

/* --- Character Mapping Codec -------------------------------------------- */

static
int charmap_decoding_error(const char **source,
			 Py_UNICODE **dest,
			 const char *errors,
			 const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "charmap decoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = Py_UNICODE_REPLACEMENT_CHARACTER;
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "charmap decoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_DecodeCharmap(const char *s,
				  int size,
				  PyObject *mapping,
				  const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    /* Default to Latin-1 */
    if (mapping == NULL)
	return PyUnicode_DecodeLatin1(s, size, errors);

    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) {
	unsigned char ch = *s++;
	PyObject *w, *x;

	/* Get mapping (char ordinal -> integer, Unicode char or None) */
	w = PyInt_FromLong((long)ch);
	if (w == NULL)
	    goto onError;
	x = PyObject_GetItem(mapping, w);
	Py_DECREF(w);
	if (x == NULL) {
	    if (PyErr_ExceptionMatches(PyExc_LookupError)) {
		/* No mapping found: default to Latin-1 mapping */
		PyErr_Clear();
		*p++ = (Py_UNICODE)ch;
		continue;
	    }
	    goto onError;
	}

	/* Apply mapping */
	if (PyInt_Check(x)) {
	    long value = PyInt_AS_LONG(x);
	    if (value < 0 || value > 65535) {
		PyErr_SetString(PyExc_TypeError,
				"character mapping must be in range(65536)");
		Py_DECREF(x);
		goto onError;
	    }
	    *p++ = (Py_UNICODE)value;
	}
	else if (x == Py_None) {
	    /* undefined mapping */
	    if (charmap_decoding_error(&s, &p, errors, 
				       "character maps to <undefined>")) {
		Py_DECREF(x);
		goto onError;
	    }
	}
	else if (PyUnicode_Check(x)) {
	    if (PyUnicode_GET_SIZE(x) != 1) {
		/* 1-n mapping */
		PyErr_SetString(PyExc_NotImplementedError,
				"1-n mappings are currently not implemented");
		Py_DECREF(x);
		goto onError;
	    }
	    *p++ = *PyUnicode_AS_UNICODE(x);
	}
	else {
	    /* wrong return value */
	    PyErr_SetString(PyExc_TypeError,
		  "character mapping must return integer, None or unicode");
	    Py_DECREF(x);
	    goto onError;
	}
	Py_DECREF(x);
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
	if (_PyUnicode_Resize(v, (int)(p - PyUnicode_AS_UNICODE(v))))
	    goto onError;
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

static
int charmap_encoding_error(const Py_UNICODE **source,
			   char **dest,
			   const char *errors,
			   const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "charmap encoding error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = '?';
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "charmap encoding error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_EncodeCharmap(const Py_UNICODE *p,
				  int size,
				  PyObject *mapping,
				  const char *errors)
{
    PyObject *v;
    char *s;

    /* Default to Latin-1 */
    if (mapping == NULL)
	return PyUnicode_EncodeLatin1(p, size, errors);

    v = PyString_FromStringAndSize(NULL, size);
    if (v == NULL)
        return NULL;
    if (size == 0)
	return v;
    s = PyString_AS_STRING(v);
    while (size-- > 0) {
	Py_UNICODE ch = *p++;
	PyObject *w, *x;

	/* Get mapping (Unicode ordinal -> string char, integer or None) */
	w = PyInt_FromLong((long)ch);
	if (w == NULL)
	    goto onError;
	x = PyObject_GetItem(mapping, w);
	Py_DECREF(w);
	if (x == NULL) {
	    if (PyErr_ExceptionMatches(PyExc_LookupError)) {
		/* No mapping found: default to Latin-1 mapping if possible */
		PyErr_Clear();
		if (ch < 256) {
		    *s++ = (char)ch;
		    continue;
		}
		else if (!charmap_encoding_error(&p, &s, errors,
				     "missing character mapping"))
		    continue;
	    }
	    goto onError;
	}

	/* Apply mapping */
	if (PyInt_Check(x)) {
	    long value = PyInt_AS_LONG(x);
	    if (value < 0 || value > 255) {
		PyErr_SetString(PyExc_TypeError,
				"character mapping must be in range(256)");
		Py_DECREF(x);
		goto onError;
	    }
	    *s++ = (char)value;
	}
	else if (x == Py_None) {
	    /* undefined mapping */
	    if (charmap_encoding_error(&p, &s, errors, 
				       "character maps to <undefined>")) {
		Py_DECREF(x);
		goto onError;
	    }
	}
	else if (PyString_Check(x)) {
	    if (PyString_GET_SIZE(x) != 1) {
		/* 1-n mapping */
		PyErr_SetString(PyExc_NotImplementedError,
		      "1-n mappings are currently not implemented");
		Py_DECREF(x);
		goto onError;
	    }
	    *s++ = *PyString_AS_STRING(x);
	}
	else {
	    /* wrong return value */
	    PyErr_SetString(PyExc_TypeError,
		  "character mapping must return integer, None or unicode");
	    Py_DECREF(x);
	    goto onError;
	}
	Py_DECREF(x);
    }
    if (s - PyString_AS_STRING(v) < PyString_GET_SIZE(v))
	if (_PyString_Resize(&v, (int)(s - PyString_AS_STRING(v))))
	    goto onError;
    return v;

 onError:
    Py_DECREF(v);
    return NULL;
}

PyObject *PyUnicode_AsCharmapString(PyObject *unicode,
				    PyObject *mapping)
{
    if (!PyUnicode_Check(unicode) || mapping == NULL) {
	PyErr_BadArgument();
	return NULL;
    }
    return PyUnicode_EncodeCharmap(PyUnicode_AS_UNICODE(unicode),
				   PyUnicode_GET_SIZE(unicode),
				   mapping,
				   NULL);
}

static
int translate_error(const Py_UNICODE **source,
		    Py_UNICODE **dest,
		    const char *errors,
		    const char *details) 
{
    if ((errors == NULL) ||
	(strcmp(errors,"strict") == 0)) {
	PyErr_Format(PyExc_UnicodeError,
		     "translate error: %.400s",
		     details);
	return -1;
    }
    else if (strcmp(errors,"ignore") == 0) {
	return 0;
    }
    else if (strcmp(errors,"replace") == 0) {
	**dest = '?';
	(*dest)++;
	return 0;
    }
    else {
	PyErr_Format(PyExc_ValueError,
		     "translate error; "
		     "unknown error handling code: %.400s",
		     errors);
	return -1;
    }
}

PyObject *PyUnicode_TranslateCharmap(const Py_UNICODE *s,
				     int size,
				     PyObject *mapping,
				     const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;
    
    if (mapping == NULL) {
	PyErr_BadArgument();
	return NULL;
    }
    
    /* Output will never be longer than input */
    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	goto done;
    p = PyUnicode_AS_UNICODE(v);
    while (size-- > 0) {
	Py_UNICODE ch = *s++;
	PyObject *w, *x;

	/* Get mapping */
	w = PyInt_FromLong(ch);
	if (w == NULL)
	    goto onError;
	x = PyObject_GetItem(mapping, w);
	Py_DECREF(w);
	if (x == NULL) {
	    if (PyErr_ExceptionMatches(PyExc_LookupError)) {
		/* No mapping found: default to 1-1 mapping */
		PyErr_Clear();
		*p++ = ch;
		continue;
	    }
	    goto onError;
	}

	/* Apply mapping */
	if (PyInt_Check(x))
	    *p++ = (Py_UNICODE)PyInt_AS_LONG(x);
	else if (x == Py_None) {
	    /* undefined mapping */
	    if (translate_error(&s, &p, errors, 
				"character maps to <undefined>")) {
		Py_DECREF(x);
		goto onError;
	    }
	}
	else if (PyUnicode_Check(x)) {
	    if (PyUnicode_GET_SIZE(x) != 1) {
		/* 1-n mapping */
		PyErr_SetString(PyExc_NotImplementedError,
				"1-n mappings are currently not implemented");
		Py_DECREF(x);
		goto onError;
	    }
	    *p++ = *PyUnicode_AS_UNICODE(x);
	}
	else {
	    /* wrong return value */
	    PyErr_SetString(PyExc_TypeError,
		  "translate mapping must return integer, None or unicode");
	    Py_DECREF(x);
	    goto onError;
	}
	Py_DECREF(x);
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
	if (_PyUnicode_Resize(v, (int)(p - PyUnicode_AS_UNICODE(v))))
	    goto onError;

 done:
    return (PyObject *)v;
    
 onError:
    Py_XDECREF(v);
    return NULL;
}

PyObject *PyUnicode_Translate(PyObject *str,
			      PyObject *mapping,
			      const char *errors)
{
    PyObject *result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	goto onError;
    result = PyUnicode_TranslateCharmap(PyUnicode_AS_UNICODE(str),
					PyUnicode_GET_SIZE(str),
					mapping,
					errors);
    Py_DECREF(str);
    return result;
    
 onError:
    Py_XDECREF(str);
    return NULL;
}
    
/* --- Decimal Encoder ---------------------------------------------------- */

int PyUnicode_EncodeDecimal(Py_UNICODE *s,
			    int length,
			    char *output,
			    const char *errors)
{
    Py_UNICODE *p, *end;

    if (output == NULL) {
	PyErr_BadArgument();
	return -1;
    }

    p = s;
    end = s + length;
    while (p < end) {
	register Py_UNICODE ch = *p++;
	int decimal;
	
	if (Py_UNICODE_ISSPACE(ch)) {
	    *output++ = ' ';
	    continue;
	}
	decimal = Py_UNICODE_TODECIMAL(ch);
	if (decimal >= 0) {
	    *output++ = '0' + decimal;
	    continue;
	}
	if (0 < ch && ch < 256) {
	    *output++ = (char)ch;
	    continue;
	}
	/* All other characters are considered invalid */
	if (errors == NULL || strcmp(errors, "strict") == 0) {
	    PyErr_SetString(PyExc_ValueError,
			    "invalid decimal Unicode string");
	    goto onError;
	}
	else if (strcmp(errors, "ignore") == 0)
	    continue;
	else if (strcmp(errors, "replace") == 0) {
	    *output++ = '?';
	    continue;
	}
    }
    /* 0-terminate the output string */
    *output++ = '\0';
    return 0;

 onError:
    return -1;
}

/* --- Helpers ------------------------------------------------------------ */

static 
int count(PyUnicodeObject *self,
	  int start,
	  int end,
	  PyUnicodeObject *substring)
{
    int count = 0;

    if (substring->length == 0)
	return (end - start + 1);

    end -= substring->length;

    while (start <= end)
        if (Py_UNICODE_MATCH(self, start, substring)) {
            count++;
            start += substring->length;
        } else
            start++;

    return count;
}

int PyUnicode_Count(PyObject *str,
		    PyObject *substr,
		    int start,
		    int end)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return -1;
    substr = PyUnicode_FromObject(substr);
    if (substr == NULL) {
	Py_DECREF(str);
	return -1;
    }
    
    result = count((PyUnicodeObject *)str,
		   start, end,
		   (PyUnicodeObject *)substr);
    
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static 
int findstring(PyUnicodeObject *self,
	       PyUnicodeObject *substring,
	       int start,
	       int end,
	       int direction)
{
    if (start < 0)
        start += self->length;
    if (start < 0)
        start = 0;

    if (substring->length == 0)
        return start;

    if (end > self->length)
        end = self->length;
    if (end < 0)
        end += self->length;
    if (end < 0)
        end = 0;

    end -= substring->length;

    if (direction < 0) {
        for (; end >= start; end--)
            if (Py_UNICODE_MATCH(self, end, substring))
                return end;
    } else {
        for (; start <= end; start++)
            if (Py_UNICODE_MATCH(self, start, substring))
                return start;
    }

    return -1;
}

int PyUnicode_Find(PyObject *str,
		   PyObject *substr,
		   int start,
		   int end,
		   int direction)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return -1;
    substr = PyUnicode_FromObject(substr);
    if (substr == NULL) {
	Py_DECREF(substr);
	return -1;
    }
    
    result = findstring((PyUnicodeObject *)str,
			(PyUnicodeObject *)substr,
			start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static 
int tailmatch(PyUnicodeObject *self,
	      PyUnicodeObject *substring,
	      int start,
	      int end,
	      int direction)
{
    if (start < 0)
        start += self->length;
    if (start < 0)
        start = 0;

    if (substring->length == 0)
        return 1;

    if (end > self->length)
        end = self->length;
    if (end < 0)
        end += self->length;
    if (end < 0)
        end = 0;

    end -= substring->length;
    if (end < start)
	return 0;

    if (direction > 0) {
	if (Py_UNICODE_MATCH(self, end, substring))
	    return 1;
    } else {
        if (Py_UNICODE_MATCH(self, start, substring))
	    return 1;
    }

    return 0;
}

int PyUnicode_Tailmatch(PyObject *str,
			PyObject *substr,
			int start,
			int end,
			int direction)
{
    int result;
    
    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return -1;
    substr = PyUnicode_FromObject(substr);
    if (substr == NULL) {
	Py_DECREF(substr);
	return -1;
    }
    
    result = tailmatch((PyUnicodeObject *)str,
		       (PyUnicodeObject *)substr,
		       start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

static 
const Py_UNICODE *findchar(const Py_UNICODE *s,
		     int size,
		     Py_UNICODE ch)
{
    /* like wcschr, but doesn't stop at NULL characters */

    while (size-- > 0) {
        if (*s == ch)
            return s;
        s++;
    }

    return NULL;
}

/* Apply fixfct filter to the Unicode object self and return a
   reference to the modified object */

static 
PyObject *fixup(PyUnicodeObject *self,
		int (*fixfct)(PyUnicodeObject *s))
{

    PyUnicodeObject *u;

    u = (PyUnicodeObject*) PyUnicode_FromUnicode(self->str,
						 self->length);
    if (u == NULL)
	return NULL;
    if (!fixfct(u)) {
	/* fixfct should return TRUE if it modified the buffer. If
	   FALSE, return a reference to the original buffer instead
	   (to save space, not time) */
	Py_INCREF(self);
	Py_DECREF(u);
	return (PyObject*) self;
    }
    return (PyObject*) u;
}

static 
int fixupper(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) {
	register Py_UNICODE ch;
	
	ch = Py_UNICODE_TOUPPER(*s);
	if (ch != *s) {
            status = 1;
	    *s = ch;
	}
        s++;
    }

    return status;
}

static 
int fixlower(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) {
	register Py_UNICODE ch;
	
	ch = Py_UNICODE_TOLOWER(*s);
	if (ch != *s) {
            status = 1;
	    *s = ch;
	}
        s++;
    }

    return status;
}

static 
int fixswapcase(PyUnicodeObject *self)
{
    int len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;
    
    while (len-- > 0) {
        if (Py_UNICODE_ISUPPER(*s)) {
            *s = Py_UNICODE_TOLOWER(*s);
            status = 1;
        } else if (Py_UNICODE_ISLOWER(*s)) {
            *s = Py_UNICODE_TOUPPER(*s);
            status = 1;
        }
        s++;
    }

    return status;
}

static 
int fixcapitalize(PyUnicodeObject *self)
{
    if (self->length > 0 && Py_UNICODE_ISLOWER(self->str[0])) {
	self->str[0] = Py_UNICODE_TOUPPER(self->str[0]);
	return 1;
    }
    return 0;
}

static
int fixtitle(PyUnicodeObject *self)
{
    register Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register Py_UNICODE *e;
    int previous_is_cased;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1) {
	Py_UNICODE ch = Py_UNICODE_TOTITLE(*p);
	if (*p != ch) {
	    *p = ch;
	    return 1;
	}
	else
	    return 0;
    }
    
    e = p + PyUnicode_GET_SIZE(self);
    previous_is_cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;
	
	if (previous_is_cased)
	    *p = Py_UNICODE_TOLOWER(ch);
	else
	    *p = Py_UNICODE_TOTITLE(ch);
	
	if (Py_UNICODE_ISLOWER(ch) || 
	    Py_UNICODE_ISUPPER(ch) || 
	    Py_UNICODE_ISTITLE(ch))
	    previous_is_cased = 1;
	else
	    previous_is_cased = 0;
    }
    return 1;
}

PyObject *PyUnicode_Join(PyObject *separator,
			 PyObject *seq)
{
    Py_UNICODE *sep;
    int seplen;
    PyUnicodeObject *res = NULL;
    int reslen = 0;
    Py_UNICODE *p;
    int seqlen = 0;
    int sz = 100;
    int i;

    seqlen = PySequence_Size(seq);
    if (seqlen < 0 && PyErr_Occurred())
	return NULL;

    if (separator == NULL) {
	Py_UNICODE blank = ' ';
	sep = &blank;
	seplen = 1;
    }
    else {
	separator = PyUnicode_FromObject(separator);
	if (separator == NULL)
	    return NULL;
	sep = PyUnicode_AS_UNICODE(separator);
	seplen = PyUnicode_GET_SIZE(separator);
    }
    
    res = _PyUnicode_New(sz);
    if (res == NULL)
	goto onError;
    p = PyUnicode_AS_UNICODE(res);
    reslen = 0;

    for (i = 0; i < seqlen; i++) {
	int itemlen;
	PyObject *item;

	item = PySequence_GetItem(seq, i);
	if (item == NULL)
	    goto onError;
	if (!PyUnicode_Check(item)) {
	    PyObject *v;
	    v = PyUnicode_FromObject(item);
	    Py_DECREF(item);
	    item = v;
	    if (item == NULL)
		goto onError;
	}
	itemlen = PyUnicode_GET_SIZE(item);
	while (reslen + itemlen + seplen >= sz) {
	    if (_PyUnicode_Resize(res, sz*2))
		goto onError;
	    sz *= 2;
	    p = PyUnicode_AS_UNICODE(res) + reslen;
	}
	if (i > 0) {
	    memcpy(p, sep, seplen * sizeof(Py_UNICODE));
	    p += seplen;
	    reslen += seplen;
	}
	memcpy(p, PyUnicode_AS_UNICODE(item), itemlen * sizeof(Py_UNICODE));
	p += itemlen;
	reslen += itemlen;
	Py_DECREF(item);
    }
    if (_PyUnicode_Resize(res, reslen))
	goto onError;

    Py_XDECREF(separator);
    return (PyObject *)res;

 onError:
    Py_XDECREF(separator);
    Py_DECREF(res);
    return NULL;
}

static 
PyUnicodeObject *pad(PyUnicodeObject *self, 
		     int left, 
		     int right,
		     Py_UNICODE fill)
{
    PyUnicodeObject *u;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0) {
        Py_INCREF(self);
        return self;
    }

    u = _PyUnicode_New(left + self->length + right);
    if (u) {
        if (left)
            Py_UNICODE_FILL(u->str, fill, left);
        Py_UNICODE_COPY(u->str + left, self->str, self->length);
        if (right)
            Py_UNICODE_FILL(u->str + left + self->length, fill, right);
    }

    return u;
}

#define SPLIT_APPEND(data, left, right)					\
	str = PyUnicode_FromUnicode(data + left, right - left);		\
	if (!str)							\
	    goto onError;						\
	if (PyList_Append(list, str)) {					\
	    Py_DECREF(str);						\
	    goto onError;						\
	}								\
        else								\
            Py_DECREF(str);

static
PyObject *split_whitespace(PyUnicodeObject *self,
			   PyObject *list,
			   int maxcount)
{
    register int i;
    register int j;
    int len = self->length;
    PyObject *str;

    for (i = j = 0; i < len; ) {
	/* find a token */
	while (i < len && Py_UNICODE_ISSPACE(self->str[i]))
	    i++;
	j = i;
	while (i < len && !Py_UNICODE_ISSPACE(self->str[i]))
	    i++;
	if (j < i) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(self->str, j, i);
	    while (i < len && Py_UNICODE_ISSPACE(self->str[i]))
		i++;
	    j = i;
	}
    }
    if (j < len) {
	SPLIT_APPEND(self->str, j, len);
    }
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

PyObject *PyUnicode_Splitlines(PyObject *string,
			       int keepends)
{
    register int i;
    register int j;
    int len;
    PyObject *list;
    PyObject *str;
    Py_UNICODE *data;

    string = PyUnicode_FromObject(string);
    if (string == NULL)
	return NULL;
    data = PyUnicode_AS_UNICODE(string);
    len = PyUnicode_GET_SIZE(string);

    list = PyList_New(0);
    if (!list)
        goto onError;

    for (i = j = 0; i < len; ) {
	int eol;
	
	/* Find a line and append it */
	while (i < len && !Py_UNICODE_ISLINEBREAK(data[i]))
	    i++;

	/* Skip the line break reading CRLF as one line break */
	eol = i;
	if (i < len) {
	    if (data[i] == '\r' && i + 1 < len &&
		data[i+1] == '\n')
		i += 2;
	    else
		i++;
	    if (keepends)
		eol = i;
	}
	SPLIT_APPEND(data, j, eol);
	j = i;
    }
    if (j < len) {
	SPLIT_APPEND(data, j, len);
    }

    Py_DECREF(string);
    return list;

 onError:
    Py_DECREF(list);
    Py_DECREF(string);
    return NULL;
}

static 
PyObject *split_char(PyUnicodeObject *self,
		     PyObject *list,
		     Py_UNICODE ch,
		     int maxcount)
{
    register int i;
    register int j;
    int len = self->length;
    PyObject *str;

    for (i = j = 0; i < len; ) {
	if (self->str[i] == ch) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(self->str, j, i);
	    i = j = i + 1;
	} else
	    i++;
    }
    if (j <= len) {
	SPLIT_APPEND(self->str, j, len);
    }
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

static 
PyObject *split_substring(PyUnicodeObject *self,
			  PyObject *list,
			  PyUnicodeObject *substring,
			  int maxcount)
{
    register int i;
    register int j;
    int len = self->length;
    int sublen = substring->length;
    PyObject *str;

    for (i = j = 0; i < len - sublen; ) {
	if (Py_UNICODE_MATCH(self, i, substring)) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(self->str, j, i);
	    i = j = i + sublen;
	} else
	    i++;
    }
    if (j <= len) {
	SPLIT_APPEND(self->str, j, len);
    }
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

#undef SPLIT_APPEND

static
PyObject *split(PyUnicodeObject *self,
		PyUnicodeObject *substring,
		int maxcount)
{
    PyObject *list;

    if (maxcount < 0)
        maxcount = INT_MAX;

    list = PyList_New(0);
    if (!list)
        return NULL;

    if (substring == NULL)
	return split_whitespace(self,list,maxcount);

    else if (substring->length == 1)
	return split_char(self,list,substring->str[0],maxcount);

    else if (substring->length == 0) {
	Py_DECREF(list);
	PyErr_SetString(PyExc_ValueError, "empty separator");
	return NULL;
    }
    else
	return split_substring(self,list,substring,maxcount);
}

static 
PyObject *strip(PyUnicodeObject *self,
		int left,
		int right)
{
    Py_UNICODE *p = self->str;
    int start = 0;
    int end = self->length;

    if (left)
        while (start < end && Py_UNICODE_ISSPACE(p[start]))
            start++;

    if (right)
        while (end > start && Py_UNICODE_ISSPACE(p[end-1]))
            end--;

    if (start == 0 && end == self->length) {
        /* couldn't strip anything off, return original string */
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) PyUnicode_FromUnicode(
        self->str + start,
        end - start
        );
}

static 
PyObject *replace(PyUnicodeObject *self,
		  PyUnicodeObject *str1,
		  PyUnicodeObject *str2,
		  int maxcount)
{
    PyUnicodeObject *u;

    if (maxcount < 0)
	maxcount = INT_MAX;

    if (str1->length == 1 && str2->length == 1) {
        int i;

        /* replace characters */
        if (!findchar(self->str, self->length, str1->str[0])) {
            /* nothing to replace, return original string */
            Py_INCREF(self);
            u = self;
        } else {
	    Py_UNICODE u1 = str1->str[0];
	    Py_UNICODE u2 = str2->str[0];
	    
            u = (PyUnicodeObject*) PyUnicode_FromUnicode(
                self->str,
                self->length
                );
            if (u)
                for (i = 0; i < u->length; i++)
                    if (u->str[i] == u1) {
                        if (--maxcount < 0)
                            break;
                        u->str[i] = u2;
                    }
        }

    } else {
        int n, i;
        Py_UNICODE *p;

        /* replace strings */
        n = count(self, 0, self->length, str1);
        if (n > maxcount)
            n = maxcount;
        if (n == 0) {
            /* nothing to replace, return original string */
            Py_INCREF(self);
            u = self;
        } else {
            u = _PyUnicode_New(
                self->length + n * (str2->length - str1->length));
            if (u) {
                i = 0;
                p = u->str;
                while (i <= self->length - str1->length)
                    if (Py_UNICODE_MATCH(self, i, str1)) {
                        /* replace string segment */
                        Py_UNICODE_COPY(p, str2->str, str2->length);
                        p += str2->length;
                        i += str1->length;
                        if (--n <= 0) {
                            /* copy remaining part */
                            Py_UNICODE_COPY(p, self->str+i, self->length-i);
                            break;
                        }
                    } else
                        *p++ = self->str[i++];
            }
        }
    }
    
    return (PyObject *) u;
}

/* --- Unicode Object Methods --------------------------------------------- */

static char title__doc__[] =
"S.title() -> unicode\n\
\n\
Return a titlecased version of S, i.e. words start with title case\n\
characters, all remaining cased characters have lower case.";

static PyObject*
unicode_title(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return fixup(self, fixtitle);
}

static char capitalize__doc__[] =
"S.capitalize() -> unicode\n\
\n\
Return a capitalized version of S, i.e. make the first character\n\
have upper case.";

static PyObject*
unicode_capitalize(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return fixup(self, fixcapitalize);
}

#if 0
static char capwords__doc__[] =
"S.capwords() -> unicode\n\
\n\
Apply .capitalize() to all words in S and return the result with\n\
normalized whitespace (all whitespace strings are replaced by ' ').";

static PyObject*
unicode_capwords(PyUnicodeObject *self, PyObject *args)
{
    PyObject *list;
    PyObject *item;
    int i;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Split into words */
    list = split(self, NULL, -1);
    if (!list)
        return NULL;

    /* Capitalize each word */
    for (i = 0; i < PyList_GET_SIZE(list); i++) {
        item = fixup((PyUnicodeObject *)PyList_GET_ITEM(list, i),
		     fixcapitalize);
        if (item == NULL)
            goto onError;
        Py_DECREF(PyList_GET_ITEM(list, i));
        PyList_SET_ITEM(list, i, item);
    }

    /* Join the words to form a new string */
    item = PyUnicode_Join(NULL, list);

onError:
    Py_DECREF(list);
    return (PyObject *)item;
}
#endif

static char center__doc__[] =
"S.center(width) -> unicode\n\
\n\
Return S centered in a Unicode string of length width. Padding is done\n\
using spaces.";

static PyObject *
unicode_center(PyUnicodeObject *self, PyObject *args)
{
    int marg, left;
    int width;

    if (!PyArg_ParseTuple(args, "i:center", &width))
        return NULL;

    if (self->length >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    marg = width - self->length;
    left = marg / 2 + (marg & width & 1);

    return (PyObject*) pad(self, left, marg - left, ' ');
}

#if 0

/* This code should go into some future Unicode collation support
   module. The basic comparison should compare ordinals on a naive
   basis (this is what Java does and thus JPython too). */

/* speedy UTF-16 code point order comparison */
/* gleaned from: */
/* http://www-4.ibm.com/software/developer/library/utf16.html?dwzone=unicode */

static short utf16Fixup[32] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 
    0, 0, 0, 0x2000, -0x800, -0x800, -0x800, -0x800
};

static int
unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    int len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->length;
    len2 = str2->length;
    
    while (len1 > 0 && len2 > 0) {
        Py_UNICODE c1, c2;     
	long diff;

        c1 = *s1++;
        c2 = *s2++;
	if (c1 > (1<<11) * 26)
	    c1 += utf16Fixup[c1>>11];
	if (c2 > (1<<11) * 26)
            c2 += utf16Fixup[c2>>11];
        
        /* now c1 and c2 are in UTF-32-compatible order */
        diff = (long)c1 - (long)c2;
        if (diff)
            return (diff < 0) ? -1 : (diff != 0);
        len1--; len2--;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

#else

static int
unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    register int len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->length;
    len2 = str2->length;
    
    while (len1 > 0 && len2 > 0) {
	register long diff;

        diff = (long)*s1++ - (long)*s2++;
        if (diff)
            return (diff < 0) ? -1 : (diff != 0);
        len1--; len2--;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

#endif

int PyUnicode_Compare(PyObject *left,
		      PyObject *right)
{
    PyUnicodeObject *u = NULL, *v = NULL;
    int result;

    /* Coerce the two arguments */
    u = (PyUnicodeObject *)PyUnicode_FromObject(left);
    if (u == NULL)
	goto onError;
    v = (PyUnicodeObject *)PyUnicode_FromObject(right);
    if (v == NULL)
	goto onError;

    /* Shortcut for empty or interned objects */
    if (v == u) {
	Py_DECREF(u);
	Py_DECREF(v);
	return 0;
    }

    result = unicode_compare(u, v);

    Py_DECREF(u);
    Py_DECREF(v);
    return result;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return -1;
}

int PyUnicode_Contains(PyObject *container,
		       PyObject *element)
{
    PyUnicodeObject *u = NULL, *v = NULL;
    int result;
    register const Py_UNICODE *p, *e;
    register Py_UNICODE ch;

    /* Coerce the two arguments */
    v = (PyUnicodeObject *)PyUnicode_FromObject(element);
    if (v == NULL) {
	PyErr_SetString(PyExc_TypeError,
	    "'in <string>' requires character as left operand");
	goto onError;
    }
    u = (PyUnicodeObject *)PyUnicode_FromObject(container);
    if (u == NULL) {
	Py_DECREF(v);
	goto onError;
    }

    /* Check v in u */
    if (PyUnicode_GET_SIZE(v) != 1) {
	PyErr_SetString(PyExc_TypeError,
	    "'in <string>' requires character as left operand");
	goto onError;
    }
    ch = *PyUnicode_AS_UNICODE(v);
    p = PyUnicode_AS_UNICODE(u);
    e = p + PyUnicode_GET_SIZE(u);
    result = 0;
    while (p < e) {
	if (*p++ == ch) {
	    result = 1;
	    break;
	}
    }

    Py_DECREF(u);
    Py_DECREF(v);
    return result;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return -1;
}

/* Concat to string or Unicode object giving a new Unicode object. */

PyObject *PyUnicode_Concat(PyObject *left,
			   PyObject *right)
{
    PyUnicodeObject *u = NULL, *v = NULL, *w;

    /* Coerce the two arguments */
    u = (PyUnicodeObject *)PyUnicode_FromObject(left);
    if (u == NULL)
	goto onError;
    v = (PyUnicodeObject *)PyUnicode_FromObject(right);
    if (v == NULL)
	goto onError;

    /* Shortcuts */
    if (v == unicode_empty) {
	Py_DECREF(v);
	return (PyObject *)u;
    }
    if (u == unicode_empty) {
	Py_DECREF(u);
	return (PyObject *)v;
    }

    /* Concat the two Unicode strings */
    w = _PyUnicode_New(u->length + v->length);
    if (w == NULL)
	goto onError;
    Py_UNICODE_COPY(w->str, u->str, u->length);
    Py_UNICODE_COPY(w->str + u->length, v->str, v->length);

    Py_DECREF(u);
    Py_DECREF(v);
    return (PyObject *)w;

onError:
    Py_XDECREF(u);
    Py_XDECREF(v);
    return NULL;
}

static char count__doc__[] =
"S.count(sub[, start[, end]]) -> int\n\
\n\
Return the number of occurrences of substring sub in Unicode string\n\
S[start:end].  Optional arguments start and end are\n\
interpreted as in slice notation.";

static PyObject *
unicode_count(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:count", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;

    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;
    
    if (start < 0)
        start += self->length;
    if (start < 0)
        start = 0;
    if (end > self->length)
        end = self->length;
    if (end < 0)
        end += self->length;
    if (end < 0)
        end = 0;

    result = PyInt_FromLong((long) count(self, start, end, substring));

    Py_DECREF(substring);
    return result;
}

static char encode__doc__[] =
"S.encode([encoding[,errors]]) -> string\n\
\n\
Return an encoded string version of S. Default encoding is the current\n\
default string encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore' and 'replace'.";

static PyObject *
unicode_encode(PyUnicodeObject *self, PyObject *args)
{
    char *encoding = NULL;
    char *errors = NULL;
    if (!PyArg_ParseTuple(args, "|ss:encode", &encoding, &errors))
        return NULL;
    return PyUnicode_AsEncodedString((PyObject *)self, encoding, errors);
}

static char expandtabs__doc__[] =
"S.expandtabs([tabsize]) -> unicode\n\
\n\
Return a copy of S where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.";

static PyObject*
unicode_expandtabs(PyUnicodeObject *self, PyObject *args)
{
    Py_UNICODE *e;
    Py_UNICODE *p;
    Py_UNICODE *q;
    int i, j;
    PyUnicodeObject *u;
    int tabsize = 8;

    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
	return NULL;

    /* First pass: determine size of output string */
    i = j = 0;
    e = self->str + self->length;
    for (p = self->str; p < e; p++)
        if (*p == '\t') {
	    if (tabsize > 0)
		j += tabsize - (j % tabsize);
	}
        else {
            j++;
            if (*p == '\n' || *p == '\r') {
                i += j;
                j = 0;
            }
        }

    /* Second pass: create output string and fill it */
    u = _PyUnicode_New(i + j);
    if (!u)
        return NULL;

    j = 0;
    q = u->str;

    for (p = self->str; p < e; p++)
        if (*p == '\t') {
	    if (tabsize > 0) {
		i = tabsize - (j % tabsize);
		j += i;
		while (i--)
		    *q++ = ' ';
	    }
	}
	else {
            j++;
	    *q++ = *p;
            if (*p == '\n' || *p == '\r')
                j = 0;
        }

    return (PyObject*) u;
}

static char find__doc__[] =
"S.find(sub [,start [,end]]) -> int\n\
\n\
Return the lowest index in S where substring sub is found,\n\
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.";

static PyObject *
unicode_find(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:find", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = PyInt_FromLong(findstring(self, substring, start, end, 1));

    Py_DECREF(substring);
    return result;
}

static PyObject *
unicode_getitem(PyUnicodeObject *self, int index)
{
    if (index < 0 || index >= self->length) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }

    return (PyObject*) PyUnicode_FromUnicode(&self->str[index], 1);
}

static long
unicode_hash(PyUnicodeObject *self)
{
    /* Since Unicode objects compare equal to their ASCII string
       counterparts, they should use the individual character values
       as basis for their hash value.  This is needed to assure that
       strings and Unicode objects behave in the same way as
       dictionary keys. */

    register int len;
    register Py_UNICODE *p;
    register long x;

    if (self->hash != -1)
	return self->hash;
    len = PyUnicode_GET_SIZE(self);
    p = PyUnicode_AS_UNICODE(self);
    x = *p << 7;
    while (--len >= 0)
	x = (1000003*x) ^ *p++;
    x ^= PyUnicode_GET_SIZE(self);
    if (x == -1)
	x = -2;
    self->hash = x;
    return x;
}

static char index__doc__[] =
"S.index(sub [,start [,end]]) -> int\n\
\n\
Like S.find() but raise ValueError when the substring is not found.";

static PyObject *
unicode_index(PyUnicodeObject *self, PyObject *args)
{
    int result;
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;

    if (!PyArg_ParseTuple(args, "O|O&O&:index", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = findstring(self, substring, start, end, 1);

    Py_DECREF(substring);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    return PyInt_FromLong(result);
}

static char islower__doc__[] =
"S.islower() -> int\n\
\n\
Return 1 if  all cased characters in S are lowercase and there is\n\
at least one cased character in S, 0 otherwise.";

static PyObject*
unicode_islower(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyInt_FromLong(Py_UNICODE_ISLOWER(*p) != 0);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;
	
	if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch))
	    return PyInt_FromLong(0);
	else if (!cased && Py_UNICODE_ISLOWER(ch))
	    cased = 1;
    }
    return PyInt_FromLong(cased);
}

static char isupper__doc__[] =
"S.isupper() -> int\n\
\n\
Return 1 if  all cased characters in S are uppercase and there is\n\
at least one cased character in S, 0 otherwise.";

static PyObject*
unicode_isupper(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyInt_FromLong(Py_UNICODE_ISUPPER(*p) != 0);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;
	
	if (Py_UNICODE_ISLOWER(ch) || Py_UNICODE_ISTITLE(ch))
	    return PyInt_FromLong(0);
	else if (!cased && Py_UNICODE_ISUPPER(ch))
	    cased = 1;
    }
    return PyInt_FromLong(cased);
}

static char istitle__doc__[] =
"S.istitle() -> int\n\
\n\
Return 1 if S is a titlecased string, i.e. upper- and titlecase characters\n\
may only follow uncased characters and lowercase characters only cased\n\
ones. Return 0 otherwise.";

static PyObject*
unicode_istitle(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased, previous_is_cased;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyInt_FromLong((Py_UNICODE_ISTITLE(*p) != 0) ||
			      (Py_UNICODE_ISUPPER(*p) != 0));

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;
	
	if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch)) {
	    if (previous_is_cased)
		return PyInt_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else if (Py_UNICODE_ISLOWER(ch)) {
	    if (!previous_is_cased)
		return PyInt_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else
	    previous_is_cased = 0;
    }
    return PyInt_FromLong(cased);
}

static char isspace__doc__[] =
"S.isspace() -> int\n\
\n\
Return 1 if there are only whitespace characters in S,\n\
0 otherwise.";

static PyObject*
unicode_isspace(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISSPACE(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISSPACE(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char isalpha__doc__[] =
"S.isalpha() -> int\n\
\n\
Return 1 if  all characters in S are alphabetic\n\
and there is at least one character in S, 0 otherwise.";

static PyObject*
unicode_isalpha(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISALPHA(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISALPHA(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char isalnum__doc__[] =
"S.isalnum() -> int\n\
\n\
Return 1 if  all characters in S are alphanumeric\n\
and there is at least one character in S, 0 otherwise.";

static PyObject*
unicode_isalnum(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISALNUM(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISALNUM(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char isdecimal__doc__[] =
"S.isdecimal() -> int\n\
\n\
Return 1 if there are only decimal characters in S,\n\
0 otherwise.";

static PyObject*
unicode_isdecimal(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISDECIMAL(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISDECIMAL(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char isdigit__doc__[] =
"S.isdigit() -> int\n\
\n\
Return 1 if there are only digit characters in S,\n\
0 otherwise.";

static PyObject*
unicode_isdigit(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISDIGIT(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISDIGIT(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char isnumeric__doc__[] =
"S.isnumeric() -> int\n\
\n\
Return 1 if there are only numeric characters in S,\n\
0 otherwise.";

static PyObject*
unicode_isnumeric(PyUnicodeObject *self, PyObject *args)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    if (!PyArg_NoArgs(args))
        return NULL;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISNUMERIC(*p))
	return PyInt_FromLong(1);

    /* Special case for empty strings */
    if (PyString_GET_SIZE(self) == 0)
	return PyInt_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISNUMERIC(*p))
	    return PyInt_FromLong(0);
    }
    return PyInt_FromLong(1);
}

static char join__doc__[] =
"S.join(sequence) -> unicode\n\
\n\
Return a string which is the concatenation of the strings in the\n\
sequence.  The separator between elements is S.";

static PyObject*
unicode_join(PyUnicodeObject *self, PyObject *args)
{
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O:join", &data))
        return NULL;

    return PyUnicode_Join((PyObject *)self, data);
}

static int
unicode_length(PyUnicodeObject *self)
{
    return self->length;
}

static char ljust__doc__[] =
"S.ljust(width) -> unicode\n\
\n\
Return S left justified in a Unicode string of length width. Padding is\n\
done using spaces.";

static PyObject *
unicode_ljust(PyUnicodeObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:ljust", &width))
        return NULL;

    if (self->length >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, 0, width - self->length, ' ');
}

static char lower__doc__[] =
"S.lower() -> unicode\n\
\n\
Return a copy of the string S converted to lowercase.";

static PyObject*
unicode_lower(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return fixup(self, fixlower);
}

static char lstrip__doc__[] =
"S.lstrip() -> unicode\n\
\n\
Return a copy of the string S with leading whitespace removed.";

static PyObject *
unicode_lstrip(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return strip(self, 1, 0);
}

static PyObject*
unicode_repeat(PyUnicodeObject *str, int len)
{
    PyUnicodeObject *u;
    Py_UNICODE *p;
    int nchars;
    size_t nbytes;

    if (len < 0)
        len = 0;

    if (len == 1) {
        /* no repeat, return original string */
        Py_INCREF(str);
        return (PyObject*) str;
    }

    /* ensure # of chars needed doesn't overflow int and # of bytes
     * needed doesn't overflow size_t
     */
    nchars = len * str->length;
    if (len && nchars / len != str->length) {
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    nbytes = (nchars + 1) * sizeof(Py_UNICODE);
    if (nbytes / sizeof(Py_UNICODE) != (size_t)(nchars + 1)) {
        PyErr_SetString(PyExc_OverflowError,
                        "repeated string is too long");
        return NULL;
    }
    u = _PyUnicode_New(nchars);
    if (!u)
        return NULL;

    p = u->str;

    while (len-- > 0) {
        Py_UNICODE_COPY(p, str->str, str->length);
        p += str->length;
    }

    return (PyObject*) u;
}

PyObject *PyUnicode_Replace(PyObject *obj,
			    PyObject *subobj,
			    PyObject *replobj,
			    int maxcount)
{
    PyObject *self;
    PyObject *str1;
    PyObject *str2;
    PyObject *result;

    self = PyUnicode_FromObject(obj);
    if (self == NULL)
	return NULL;
    str1 = PyUnicode_FromObject(subobj);
    if (str1 == NULL) {
	Py_DECREF(self);
	return NULL;
    }
    str2 = PyUnicode_FromObject(replobj);
    if (str2 == NULL) {
	Py_DECREF(self);
	Py_DECREF(str1);
	return NULL;
    }
    result = replace((PyUnicodeObject *)self, 
		     (PyUnicodeObject *)str1, 
		     (PyUnicodeObject *)str2, 
		     maxcount);
    Py_DECREF(self);
    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

static char replace__doc__[] =
"S.replace (old, new[, maxsplit]) -> unicode\n\
\n\
Return a copy of S with all occurrences of substring\n\
old replaced by new.  If the optional argument maxsplit is\n\
given, only the first maxsplit occurrences are replaced.";

static PyObject*
unicode_replace(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *str1;
    PyUnicodeObject *str2;
    int maxcount = -1;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "OO|i:replace", &str1, &str2, &maxcount))
        return NULL;
    str1 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str1);
    if (str1 == NULL)
	return NULL;
    str2 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str2);
    if (str2 == NULL)
	return NULL;

    result = replace(self, str1, str2, maxcount);

    Py_DECREF(str1);
    Py_DECREF(str2);
    return result;
}

static
PyObject *unicode_repr(PyObject *unicode)
{
    return unicodeescape_string(PyUnicode_AS_UNICODE(unicode),
				PyUnicode_GET_SIZE(unicode),
				1);
}

static char rfind__doc__[] =
"S.rfind(sub [,start [,end]]) -> int\n\
\n\
Return the highest index in S where substring sub is found,\n\
such that sub is contained within s[start,end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.";

static PyObject *
unicode_rfind(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:rfind", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = PyInt_FromLong(findstring(self, substring, start, end, -1));

    Py_DECREF(substring);
    return result;
}

static char rindex__doc__[] =
"S.rindex(sub [,start [,end]]) -> int\n\
\n\
Like S.rfind() but raise ValueError when the substring is not found.";

static PyObject *
unicode_rindex(PyUnicodeObject *self, PyObject *args)
{
    int result;
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;

    if (!PyArg_ParseTuple(args, "O|O&O&:rindex", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = findstring(self, substring, start, end, -1);

    Py_DECREF(substring);
    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    return PyInt_FromLong(result);
}

static char rjust__doc__[] =
"S.rjust(width) -> unicode\n\
\n\
Return S right justified in a Unicode string of length width. Padding is\n\
done using spaces.";

static PyObject *
unicode_rjust(PyUnicodeObject *self, PyObject *args)
{
    int width;
    if (!PyArg_ParseTuple(args, "i:rjust", &width))
        return NULL;

    if (self->length >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, width - self->length, 0, ' ');
}

static char rstrip__doc__[] =
"S.rstrip() -> unicode\n\
\n\
Return a copy of the string S with trailing whitespace removed.";

static PyObject *
unicode_rstrip(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return strip(self, 0, 1);
}

static PyObject*
unicode_slice(PyUnicodeObject *self, int start, int end)
{
    /* standard clamping */
    if (start < 0)
        start = 0;
    if (end < 0)
        end = 0;
    if (end > self->length)
        end = self->length;
    if (start == 0 && end == self->length) {
        /* full slice, return original string */
        Py_INCREF(self);
        return (PyObject*) self;
    }
    if (start > end)
        start = end;
    /* copy slice */
    return (PyObject*) PyUnicode_FromUnicode(self->str + start,
					     end - start);
}

PyObject *PyUnicode_Split(PyObject *s,
			  PyObject *sep,
			  int maxsplit)
{
    PyObject *result;
    
    s = PyUnicode_FromObject(s);
    if (s == NULL)
	return NULL;
    if (sep != NULL) {
	sep = PyUnicode_FromObject(sep);
	if (sep == NULL) {
	    Py_DECREF(s);
	    return NULL;
	}
    }

    result = split((PyUnicodeObject *)s, (PyUnicodeObject *)sep, maxsplit);

    Py_DECREF(s);
    Py_XDECREF(sep);
    return result;
}

static char split__doc__[] =
"S.split([sep [,maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in S, using sep as the\n\
delimiter string.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified, any whitespace string\n\
is a separator.";

static PyObject*
unicode_split(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    int maxcount = -1;

    if (!PyArg_ParseTuple(args, "|Oi:split", &substring, &maxcount))
        return NULL;

    if (substring == Py_None)
	return split(self, NULL, maxcount);
    else if (PyUnicode_Check(substring))
	return split(self, (PyUnicodeObject *)substring, maxcount);
    else
	return PyUnicode_Split((PyObject *)self, substring, maxcount);
}

static char splitlines__doc__[] =
"S.splitlines([keepends]]) -> list of strings\n\
\n\
Return a list of the lines in S, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.";

static PyObject*
unicode_splitlines(PyUnicodeObject *self, PyObject *args)
{
    int keepends = 0;

    if (!PyArg_ParseTuple(args, "|i:splitlines", &keepends))
        return NULL;

    return PyUnicode_Splitlines((PyObject *)self, keepends);
}

static
PyObject *unicode_str(PyUnicodeObject *self)
{
    return PyUnicode_AsEncodedString((PyObject *)self, NULL, NULL);
}

static char strip__doc__[] =
"S.strip() -> unicode\n\
\n\
Return a copy of S with leading and trailing whitespace removed.";

static PyObject *
unicode_strip(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return strip(self, 1, 1);
}

static char swapcase__doc__[] =
"S.swapcase() -> unicode\n\
\n\
Return a copy of S with uppercase characters converted to lowercase\n\
and vice versa.";

static PyObject*
unicode_swapcase(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return fixup(self, fixswapcase);
}

static char translate__doc__[] =
"S.translate(table) -> unicode\n\
\n\
Return a copy of the string S, where all characters have been mapped\n\
through the given translation table, which must be a mapping of\n\
Unicode ordinals to Unicode ordinals or None. Unmapped characters\n\
are left untouched. Characters mapped to None are deleted.";

static PyObject*
unicode_translate(PyUnicodeObject *self, PyObject *args)
{
    PyObject *table;
    
    if (!PyArg_ParseTuple(args, "O:translate", &table))
	return NULL;
    return PyUnicode_TranslateCharmap(self->str, 
				      self->length,
				      table, 
				      "ignore");
}

static char upper__doc__[] =
"S.upper() -> unicode\n\
\n\
Return a copy of S converted to uppercase.";

static PyObject*
unicode_upper(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return fixup(self, fixupper);
}

#if 0
static char zfill__doc__[] =
"S.zfill(width) -> unicode\n\
\n\
Pad a numeric string x with zeros on the left, to fill a field\n\
of the specified width. The string x is never truncated.";

static PyObject *
unicode_zfill(PyUnicodeObject *self, PyObject *args)
{
    int fill;
    PyUnicodeObject *u;

    int width;
    if (!PyArg_ParseTuple(args, "i:zfill", &width))
        return NULL;

    if (self->length >= width) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    fill = width - self->length;

    u = pad(self, fill, 0, '0');

    if (u->str[fill] == '+' || u->str[fill] == '-') {
        /* move sign to beginning of string */
        u->str[0] = u->str[fill];
        u->str[fill] = '0';
    }

    return (PyObject*) u;
}
#endif

#if 0
static PyObject*
unicode_freelistsize(PyUnicodeObject *self, PyObject *args)
{
    if (!PyArg_NoArgs(args))
        return NULL;
    return PyInt_FromLong(unicode_freelist_size);
}
#endif

static char startswith__doc__[] =
"S.startswith(prefix[, start[, end]]) -> int\n\
\n\
Return 1 if S starts with the specified prefix, otherwise return 0.  With\n\
optional start, test S beginning at that position.  With optional end, stop\n\
comparing S at that position.";

static PyObject *
unicode_startswith(PyUnicodeObject *self,
		   PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:startswith", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	return NULL;
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = PyInt_FromLong(tailmatch(self, substring, start, end, -1));

    Py_DECREF(substring);
    return result;
}


static char endswith__doc__[] =
"S.endswith(suffix[, start[, end]]) -> int\n\
\n\
Return 1 if S ends with the specified suffix, otherwise return 0.  With\n\
optional start, test S beginning at that position.  With optional end, stop\n\
comparing S at that position.";

static PyObject *
unicode_endswith(PyUnicodeObject *self,
		 PyObject *args)
{
    PyUnicodeObject *substring;
    int start = 0;
    int end = INT_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:endswith", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	return NULL;
    substring = (PyUnicodeObject *)PyUnicode_FromObject(
						(PyObject *)substring);
    if (substring == NULL)
	return NULL;

    result = PyInt_FromLong(tailmatch(self, substring, start, end, +1));

    Py_DECREF(substring);
    return result;
}


static PyMethodDef unicode_methods[] = {

    /* Order is according to common usage: often used methods should
       appear first, since lookup is done sequentially. */

    {"encode", (PyCFunction) unicode_encode, 1, encode__doc__},
    {"replace", (PyCFunction) unicode_replace, 1, replace__doc__},
    {"split", (PyCFunction) unicode_split, 1, split__doc__},
    {"join", (PyCFunction) unicode_join, 1, join__doc__},
    {"capitalize", (PyCFunction) unicode_capitalize, 0, capitalize__doc__},
    {"title", (PyCFunction) unicode_title, 0, title__doc__},
    {"center", (PyCFunction) unicode_center, 1, center__doc__},
    {"count", (PyCFunction) unicode_count, 1, count__doc__},
    {"expandtabs", (PyCFunction) unicode_expandtabs, 1, expandtabs__doc__},
    {"find", (PyCFunction) unicode_find, 1, find__doc__},
    {"index", (PyCFunction) unicode_index, 1, index__doc__},
    {"ljust", (PyCFunction) unicode_ljust, 1, ljust__doc__},
    {"lower", (PyCFunction) unicode_lower, 0, lower__doc__},
    {"lstrip", (PyCFunction) unicode_lstrip, 0, lstrip__doc__},
/*  {"maketrans", (PyCFunction) unicode_maketrans, 1, maketrans__doc__}, */
    {"rfind", (PyCFunction) unicode_rfind, 1, rfind__doc__},
    {"rindex", (PyCFunction) unicode_rindex, 1, rindex__doc__},
    {"rjust", (PyCFunction) unicode_rjust, 1, rjust__doc__},
    {"rstrip", (PyCFunction) unicode_rstrip, 0, rstrip__doc__},
    {"splitlines", (PyCFunction) unicode_splitlines, 1, splitlines__doc__},
    {"strip", (PyCFunction) unicode_strip, 0, strip__doc__},
    {"swapcase", (PyCFunction) unicode_swapcase, 0, swapcase__doc__},
    {"translate", (PyCFunction) unicode_translate, 1, translate__doc__},
    {"upper", (PyCFunction) unicode_upper, 0, upper__doc__},
    {"startswith", (PyCFunction) unicode_startswith, 1, startswith__doc__},
    {"endswith", (PyCFunction) unicode_endswith, 1, endswith__doc__},
    {"islower", (PyCFunction) unicode_islower, 0, islower__doc__},
    {"isupper", (PyCFunction) unicode_isupper, 0, isupper__doc__},
    {"istitle", (PyCFunction) unicode_istitle, 0, istitle__doc__},
    {"isspace", (PyCFunction) unicode_isspace, 0, isspace__doc__},
    {"isdecimal", (PyCFunction) unicode_isdecimal, 0, isdecimal__doc__},
    {"isdigit", (PyCFunction) unicode_isdigit, 0, isdigit__doc__},
    {"isnumeric", (PyCFunction) unicode_isnumeric, 0, isnumeric__doc__},
    {"isalpha", (PyCFunction) unicode_isalpha, 0, isalpha__doc__},
    {"isalnum", (PyCFunction) unicode_isalnum, 0, isalnum__doc__},
#if 0
    {"zfill", (PyCFunction) unicode_zfill, 1, zfill__doc__},
    {"capwords", (PyCFunction) unicode_capwords, 0, capwords__doc__},
#endif

#if 0
    /* This one is just used for debugging the implementation. */
    {"freelistsize", (PyCFunction) unicode_freelistsize, 0},
#endif

    {NULL, NULL}
};

static PyObject * 
unicode_getattr(PyUnicodeObject *self, char *name)
{
    return Py_FindMethod(unicode_methods, (PyObject*) self, name);
}

static PySequenceMethods unicode_as_sequence = {
    (inquiry) unicode_length, 		/* sq_length */
    (binaryfunc) PyUnicode_Concat, 	/* sq_concat */
    (intargfunc) unicode_repeat, 	/* sq_repeat */
    (intargfunc) unicode_getitem, 	/* sq_item */
    (intintargfunc) unicode_slice, 	/* sq_slice */
    0, 					/* sq_ass_item */
    0, 					/* sq_ass_slice */
    (objobjproc)PyUnicode_Contains, 	/*sq_contains*/
};

static int
unicode_buffer_getreadbuf(PyUnicodeObject *self,
			  int index,
			  const void **ptr)
{
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
			"accessing non-existent unicode segment");
        return -1;
    }
    *ptr = (void *) self->str;
    return PyUnicode_GET_DATA_SIZE(self);
}

static int
unicode_buffer_getwritebuf(PyUnicodeObject *self, int index,
			   const void **ptr)
{
    PyErr_SetString(PyExc_TypeError,
		    "cannot use unicode as modifyable buffer");
    return -1;
}

static int
unicode_buffer_getsegcount(PyUnicodeObject *self,
			   int *lenp)
{
    if (lenp)
        *lenp = PyUnicode_GET_DATA_SIZE(self);
    return 1;
}

static int
unicode_buffer_getcharbuf(PyUnicodeObject *self,
			  int index,
			  const void **ptr)
{
    PyObject *str;
    
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
			"accessing non-existent unicode segment");
        return -1;
    }
    str = _PyUnicode_AsDefaultEncodedString((PyObject *)self, NULL);
    if (str == NULL)
	return -1;
    *ptr = (void *) PyString_AS_STRING(str);
    return PyString_GET_SIZE(str);
}

/* Helpers for PyUnicode_Format() */

static PyObject *
getnextarg(PyObject *args, int arglen, int *p_argidx)
{
    int argidx = *p_argidx;
    if (argidx < arglen) {
	(*p_argidx)++;
	if (arglen < 0)
	    return args;
	else
	    return PyTuple_GetItem(args, argidx);
    }
    PyErr_SetString(PyExc_TypeError,
		    "not enough arguments for format string");
    return NULL;
}

#define F_LJUST (1<<0)
#define F_SIGN	(1<<1)
#define F_BLANK (1<<2)
#define F_ALT	(1<<3)
#define F_ZERO	(1<<4)

static
int usprintf(register Py_UNICODE *buffer, char *format, ...)
{
    register int i;
    int len;
    va_list va;
    char *charbuffer;
    va_start(va, format);

    /* First, format the string as char array, then expand to Py_UNICODE
       array. */
    charbuffer = (char *)buffer;
    len = vsprintf(charbuffer, format, va);
    for (i = len - 1; i >= 0; i--)
	buffer[i] = (Py_UNICODE) charbuffer[i];

    va_end(va);
    return len;
}

static int
formatfloat(Py_UNICODE *buf,
	    size_t buflen,
	    int flags,
	    int prec,
	    int type,
	    PyObject *v)
{
    /* fmt = '%#.' + `prec` + `type`
       worst case length = 3 + 10 (len of INT_MAX) + 1 = 14 (use 20)*/
    char fmt[20];
    double x;
    
    x = PyFloat_AsDouble(v);
    if (x == -1.0 && PyErr_Occurred())
	return -1;
    if (prec < 0)
	prec = 6;
    if (type == 'f' && (fabs(x) / 1e25) >= 1e25)
	type = 'g';
    sprintf(fmt, "%%%s.%d%c", (flags & F_ALT) ? "#" : "", prec, type);
    /* worst case length calc to ensure no buffer overrun:
         fmt = %#.<prec>g
         buf = '-' + [0-9]*prec + '.' + 'e+' + (longest exp
            for any double rep.)
         len = 1 + prec + 1 + 2 + 5 = 9 + prec
       If prec=0 the effective precision is 1 (the leading digit is
       always given), therefore increase by one to 10+prec. */
    if (buflen <= (size_t)10 + (size_t)prec) {
	PyErr_SetString(PyExc_OverflowError,
	    "formatted float is too long (precision too long?)");
	return -1;
    }
    return usprintf(buf, fmt, x);
}

static PyObject*
formatlong(PyObject *val, int flags, int prec, int type)
{
	char *buf;
	int i, len;
	PyObject *str; /* temporary string object. */
	PyUnicodeObject *result;

	str = _PyString_FormatLong(val, flags, prec, type, &buf, &len);
	if (!str)
		return NULL;
	result = _PyUnicode_New(len);
	for (i = 0; i < len; i++)
		result->str[i] = buf[i];
	result->str[len] = 0;
	Py_DECREF(str);
	return (PyObject*)result;
}

static int
formatint(Py_UNICODE *buf,
	  size_t buflen,
	  int flags,
	  int prec,
	  int type,
	  PyObject *v)
{
    /* fmt = '%#.' + `prec` + 'l' + `type`
       worst case length = 3 + 19 (worst len of INT_MAX on 64-bit machine)
       + 1 + 1 = 24*/
    char fmt[64]; /* plenty big enough! */
    long x;

    x = PyInt_AsLong(v);
    if (x == -1 && PyErr_Occurred())
	return -1;
    if (prec < 0)
	prec = 1;
    /* buf = '+'/'-'/'0'/'0x' + '[0-9]'*max(prec,len(x in octal))
       worst case buf = '0x' + [0-9]*prec, where prec >= 11 */
    if (buflen <= 13 || buflen <= (size_t)2+(size_t)prec) {
        PyErr_SetString(PyExc_OverflowError,
            "formatted integer is too long (precision too long?)");
        return -1;
    }
    sprintf(fmt, "%%%s.%dl%c", (flags & F_ALT) ? "#" : "", prec, type);
    return usprintf(buf, fmt, x);
}

static int
formatchar(Py_UNICODE *buf,
           size_t buflen,
           PyObject *v)
{
    /* presume that the buffer is at least 2 characters long */
    if (PyUnicode_Check(v)) {
	if (PyUnicode_GET_SIZE(v) != 1)
	    goto onError;
	buf[0] = PyUnicode_AS_UNICODE(v)[0];
    }

    else if (PyString_Check(v)) {
	if (PyString_GET_SIZE(v) != 1) 
	    goto onError;
	buf[0] = (Py_UNICODE)PyString_AS_STRING(v)[0];
    }

    else {
	/* Integer input truncated to a character */
        long x;
	x = PyInt_AsLong(v);
	if (x == -1 && PyErr_Occurred())
	    goto onError;
	buf[0] = (char) x;
    }
    buf[1] = '\0';
    return 1;

 onError:
    PyErr_SetString(PyExc_TypeError,
		    "%c requires int or char");
    return -1;
}

/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...)

   FORMATBUFLEN is the length of the buffer in which the floats, ints, &
   chars are formatted. XXX This is a magic number. Each formatting
   routine does bounds checking to ensure no overflow, but a better
   solution may be to malloc a buffer of appropriate size for each
   format. For now, the current solution is sufficient.
*/
#define FORMATBUFLEN (size_t)120

PyObject *PyUnicode_Format(PyObject *format,
			   PyObject *args)
{
    Py_UNICODE *fmt, *res;
    int fmtcnt, rescnt, reslen, arglen, argidx;
    int args_owned = 0;
    PyUnicodeObject *result = NULL;
    PyObject *dict = NULL;
    PyObject *uformat;
	
    if (format == NULL || args == NULL) {
	PyErr_BadInternalCall();
	return NULL;
    }
    uformat = PyUnicode_FromObject(format);
    if (uformat == NULL)
	return NULL;
    fmt = PyUnicode_AS_UNICODE(uformat);
    fmtcnt = PyUnicode_GET_SIZE(uformat);

    reslen = rescnt = fmtcnt + 100;
    result = _PyUnicode_New(reslen);
    if (result == NULL)
	goto onError;
    res = PyUnicode_AS_UNICODE(result);

    if (PyTuple_Check(args)) {
	arglen = PyTuple_Size(args);
	argidx = 0;
    }
    else {
	arglen = -1;
	argidx = -2;
    }
    if (args->ob_type->tp_as_mapping)
	dict = args;

    while (--fmtcnt >= 0) {
	if (*fmt != '%') {
	    if (--rescnt < 0) {
		rescnt = fmtcnt + 100;
		reslen += rescnt;
		if (_PyUnicode_Resize(result, reslen) < 0)
		    return NULL;
		res = PyUnicode_AS_UNICODE(result) + reslen - rescnt;
		--rescnt;
	    }
	    *res++ = *fmt++;
	}
	else {
	    /* Got a format specifier */
	    int flags = 0;
	    int width = -1;
	    int prec = -1;
	    int size = 0;
	    Py_UNICODE c = '\0';
	    Py_UNICODE fill;
	    PyObject *v = NULL;
	    PyObject *temp = NULL;
	    Py_UNICODE *pbuf;
	    Py_UNICODE sign;
	    int len;
	    Py_UNICODE formatbuf[FORMATBUFLEN]; /* For format{float,int,char}() */

	    fmt++;
	    if (*fmt == '(') {
		Py_UNICODE *keystart;
		int keylen;
		PyObject *key;
		int pcount = 1;

		if (dict == NULL) {
		    PyErr_SetString(PyExc_TypeError,
				    "format requires a mapping"); 
		    goto onError;
		}
		++fmt;
		--fmtcnt;
		keystart = fmt;
		/* Skip over balanced parentheses */
		while (pcount > 0 && --fmtcnt >= 0) {
		    if (*fmt == ')')
			--pcount;
		    else if (*fmt == '(')
			++pcount;
		    fmt++;
		}
		keylen = fmt - keystart - 1;
		if (fmtcnt < 0 || pcount > 0) {
		    PyErr_SetString(PyExc_ValueError,
				    "incomplete format key");
		    goto onError;
		}
		/* keys are converted to strings using UTF-8 and
		   then looked up since Python uses strings to hold
		   variables names etc. in its namespaces and we
		   wouldn't want to break common idioms. */
		key = PyUnicode_EncodeUTF8(keystart,
					   keylen,
					   NULL);
		if (key == NULL)
		    goto onError;
		if (args_owned) {
		    Py_DECREF(args);
		    args_owned = 0;
		}
		args = PyObject_GetItem(dict, key);
		Py_DECREF(key);
		if (args == NULL) {
		    goto onError;
		}
		args_owned = 1;
		arglen = -1;
		argidx = -2;
	    }
	    while (--fmtcnt >= 0) {
		switch (c = *fmt++) {
		case '-': flags |= F_LJUST; continue;
		case '+': flags |= F_SIGN; continue;
		case ' ': flags |= F_BLANK; continue;
		case '#': flags |= F_ALT; continue;
		case '0': flags |= F_ZERO; continue;
		}
		break;
	    }
	    if (c == '*') {
		v = getnextarg(args, arglen, &argidx);
		if (v == NULL)
		    goto onError;
		if (!PyInt_Check(v)) {
		    PyErr_SetString(PyExc_TypeError,
				    "* wants int");
		    goto onError;
		}
		width = PyInt_AsLong(v);
		if (width < 0) {
		    flags |= F_LJUST;
		    width = -width;
		}
		if (--fmtcnt >= 0)
		    c = *fmt++;
	    }
	    else if (c >= '0' && c <= '9') {
		width = c - '0';
		while (--fmtcnt >= 0) {
		    c = *fmt++;
		    if (c < '0' || c > '9')
			break;
		    if ((width*10) / 10 != width) {
			PyErr_SetString(PyExc_ValueError,
					"width too big");
			goto onError;
		    }
		    width = width*10 + (c - '0');
		}
	    }
	    if (c == '.') {
		prec = 0;
		if (--fmtcnt >= 0)
		    c = *fmt++;
		if (c == '*') {
		    v = getnextarg(args, arglen, &argidx);
		    if (v == NULL)
			goto onError;
		    if (!PyInt_Check(v)) {
			PyErr_SetString(PyExc_TypeError,
					"* wants int");
			goto onError;
		    }
		    prec = PyInt_AsLong(v);
		    if (prec < 0)
			prec = 0;
		    if (--fmtcnt >= 0)
			c = *fmt++;
		}
		else if (c >= '0' && c <= '9') {
		    prec = c - '0';
		    while (--fmtcnt >= 0) {
			c = Py_CHARMASK(*fmt++);
			if (c < '0' || c > '9')
			    break;
			if ((prec*10) / 10 != prec) {
			    PyErr_SetString(PyExc_ValueError,
					    "prec too big");
			    goto onError;
			}
			prec = prec*10 + (c - '0');
		    }
		}
	    } /* prec */
	    if (fmtcnt >= 0) {
		if (c == 'h' || c == 'l' || c == 'L') {
		    size = c;
		    if (--fmtcnt >= 0)
			c = *fmt++;
		}
	    }
	    if (fmtcnt < 0) {
		PyErr_SetString(PyExc_ValueError,
				"incomplete format");
		goto onError;
	    }
	    if (c != '%') {
		v = getnextarg(args, arglen, &argidx);
		if (v == NULL)
		    goto onError;
	    }
	    sign = 0;
	    fill = ' ';
	    switch (c) {

	    case '%':
		pbuf = formatbuf;
		/* presume that buffer length is at least 1 */
		pbuf[0] = '%';
		len = 1;
		break;

	    case 's':
	    case 'r':
		if (PyUnicode_Check(v) && c == 's') {
		    temp = v;
		    Py_INCREF(temp);
		}
		else {
		    PyObject *unicode;
		    if (c == 's')
			temp = PyObject_Str(v);
		    else
			temp = PyObject_Repr(v);
		    if (temp == NULL)
			goto onError;
		    if (!PyString_Check(temp)) {
			/* XXX Note: this should never happen, since
   			       PyObject_Repr() and PyObject_Str() assure
			       this */
			Py_DECREF(temp);
			PyErr_SetString(PyExc_TypeError,
					"%s argument has non-string str()");
			goto onError;
		    }
		    unicode = PyUnicode_Decode(PyString_AS_STRING(temp),
						   PyString_GET_SIZE(temp),
					       NULL,
						   "strict");
		    Py_DECREF(temp);
		    temp = unicode;
		    if (temp == NULL)
			goto onError;
		}
		pbuf = PyUnicode_AS_UNICODE(temp);
		len = PyUnicode_GET_SIZE(temp);
		if (prec >= 0 && len > prec)
		    len = prec;
		break;

	    case 'i':
	    case 'd':
	    case 'u':
	    case 'o':
	    case 'x':
	    case 'X':
		if (c == 'i')
		    c = 'd';
		if (PyLong_Check(v) && PyLong_AsLong(v) == -1
		    && PyErr_Occurred()) {
		    PyErr_Clear();
		    temp = formatlong(v, flags, prec, c);
		    if (!temp)
			goto onError;
		    pbuf = PyUnicode_AS_UNICODE(temp);
		    len = PyUnicode_GET_SIZE(temp);
		    /* unbounded ints can always produce
		       a sign character! */
		    sign = 1;
		}
		else {
		    pbuf = formatbuf;
		    len = formatint(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE),
				    flags, prec, c, v);
		    if (len < 0)
			goto onError;
		    /* only d conversion is signed */
		    sign = c == 'd';
		}
		if (flags & F_ZERO)
		    fill = '0';
		break;

	    case 'e':
	    case 'E':
	    case 'f':
	    case 'g':
	    case 'G':
		pbuf = formatbuf;
		len = formatfloat(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE),
			flags, prec, c, v);
		if (len < 0)
		    goto onError;
		sign = 1;
		if (flags & F_ZERO)
		    fill = '0';
		break;

	    case 'c':
		pbuf = formatbuf;
		len = formatchar(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE), v);
		if (len < 0)
		    goto onError;
		break;

	    default:
		PyErr_Format(PyExc_ValueError,
			     "unsupported format character '%c' (0x%x)",
			     c, c);
		goto onError;
	    }
	    if (sign) {
		if (*pbuf == '-' || *pbuf == '+') {
		    sign = *pbuf++;
		    len--;
		}
		else if (flags & F_SIGN)
		    sign = '+';
		else if (flags & F_BLANK)
		    sign = ' ';
		else
		    sign = 0;
	    }
	    if (width < len)
		width = len;
	    if (rescnt < width + (sign != 0)) {
		reslen -= rescnt;
		rescnt = width + fmtcnt + 100;
		reslen += rescnt;
		if (_PyUnicode_Resize(result, reslen) < 0)
		    return NULL;
		res = PyUnicode_AS_UNICODE(result)
		    + reslen - rescnt;
	    }
	    if (sign) {
		if (fill != ' ')
		    *res++ = sign;
		rescnt--;
		if (width > len)
		    width--;
	    }
	    if ((flags & F_ALT) && (c == 'x' || c == 'X')) {
		assert(pbuf[0] == '0');
		assert(pbuf[1] == c);
		if (fill != ' ') {
		    *res++ = *pbuf++;
		    *res++ = *pbuf++;
		}
		rescnt -= 2;
		width -= 2;
		if (width < 0)
		    width = 0;
		len -= 2;
	    }
	    if (width > len && !(flags & F_LJUST)) {
		do {
		    --rescnt;
		    *res++ = fill;
		} while (--width > len);
	    }
	    if (fill == ' ') {
		if (sign)
		    *res++ = sign;
		if ((flags & F_ALT) && (c == 'x' || c == 'X')) {
		    assert(pbuf[0] == '0');
		    assert(pbuf[1] == c);
		    *res++ = *pbuf++;
		    *res++ = *pbuf++;
		}
	    }
	    memcpy(res, pbuf, len * sizeof(Py_UNICODE));
	    res += len;
	    rescnt -= len;
	    while (--width >= len) {
		--rescnt;
		*res++ = ' ';
	    }
	    if (dict && (argidx < arglen) && c != '%') {
		PyErr_SetString(PyExc_TypeError,
				"not all arguments converted");
		goto onError;
	    }
	    Py_XDECREF(temp);
	} /* '%' */
    } /* until end */
    if (argidx < arglen && !dict) {
	PyErr_SetString(PyExc_TypeError,
			"not all arguments converted");
	goto onError;
    }

    if (args_owned) {
	Py_DECREF(args);
    }
    Py_DECREF(uformat);
    if (_PyUnicode_Resize(result, reslen - rescnt))
	goto onError;
    return (PyObject *)result;

 onError:
    Py_XDECREF(result);
    Py_DECREF(uformat);
    if (args_owned) {
	Py_DECREF(args);
    }
    return NULL;
}

static PyBufferProcs unicode_as_buffer = {
    (getreadbufferproc) unicode_buffer_getreadbuf,
    (getwritebufferproc) unicode_buffer_getwritebuf,
    (getsegcountproc) unicode_buffer_getsegcount,
    (getcharbufferproc) unicode_buffer_getcharbuf,
};

PyTypeObject PyUnicode_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0, 					/* ob_size */
    "unicode", 				/* tp_name */
    sizeof(PyUnicodeObject), 		/* tp_size */
    0, 					/* tp_itemsize */
    /* Slots */
    (destructor)_PyUnicode_Free, 	/* tp_dealloc */
    0, 					/* tp_print */
    (getattrfunc)unicode_getattr, 	/* tp_getattr */
    0, 					/* tp_setattr */
    (cmpfunc) unicode_compare, 		/* tp_compare */
    (reprfunc) unicode_repr, 		/* tp_repr */
    0, 					/* tp_as_number */
    &unicode_as_sequence, 		/* tp_as_sequence */
    0, 					/* tp_as_mapping */
    (hashfunc) unicode_hash, 		/* tp_hash*/
    0, 					/* tp_call*/
    (reprfunc) unicode_str,	 	/* tp_str */
    (getattrofunc) NULL, 		/* tp_getattro */
    (setattrofunc) NULL, 		/* tp_setattro */
    &unicode_as_buffer,			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,			/* tp_flags */
};

/* Initialize the Unicode implementation */

void _PyUnicode_Init(void)
{
    /* Doublecheck the configuration... */
    if (sizeof(Py_UNICODE) != 2)
        Py_FatalError("Unicode configuration error: "
		      "sizeof(Py_UNICODE) != 2 bytes");

    /* Init the implementation */
    unicode_freelist = NULL;
    unicode_freelist_size = 0;
    unicode_empty = _PyUnicode_New(0);
    strcpy(unicode_default_encoding, "ascii");
}

/* Finalize the Unicode implementation */

void
_PyUnicode_Fini(void)
{
    PyUnicodeObject *u;

    Py_XDECREF(unicode_empty);
    unicode_empty = NULL;

    for (u = unicode_freelist; u != NULL;) {
	PyUnicodeObject *v = u;
	u = *(PyUnicodeObject **)u;
	if (v->str)
	    PyMem_DEL(v->str);
	Py_XDECREF(v->defenc);
	PyObject_DEL(v);
    }
    unicode_freelist = NULL;
    unicode_freelist_size = 0;
}
