/*

Unicode implementation based on original code by Fredrik Lundh,
modified by Marc-Andre Lemburg <mal@lemburg.com> according to the
Unicode Integration Proposal (see file Misc/unicode.txt).

Major speed upgrades to the method implementations at the Reykjavik
NeedForSpeed sprint, by Fredrik Lundh and Andrew Dalke.

Copyright (c) Corporation for National Research Initiatives.

--------------------------------------------------------------------
The original string type implementation is:

    Copyright (c) 1999 by Secret Labs AB
    Copyright (c) 1999 by Fredrik Lundh

By obtaining, using, and/or copying this software and/or its
associated documentation, you agree that you have read, understood,
and will comply with the following terms and conditions:

Permission to use, copy, modify, and distribute this software and its
associated documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies, and that both that copyright notice and this permission notice
appear in supporting documentation, and that the name of Secret Labs
AB or the author not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
--------------------------------------------------------------------

*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include "formatter_unicode.h"

#include "unicodeobject.h"
#include "ucnhash.h"

#ifdef MS_WINDOWS
#include <windows.h>
#endif

/* Limit for the Unicode object free list */

#define PyUnicode_MAXFREELIST       1024

/* Limit for the Unicode object free list stay alive optimization.

   The implementation will keep allocated Unicode memory intact for
   all objects on the free list having a size less than this
   limit. This reduces malloc() overhead for small Unicode objects.

   At worst this will result in PyUnicode_MAXFREELIST *
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


#ifdef __cplusplus
extern "C" {
#endif

/* Free list for Unicode objects */
static PyUnicodeObject *free_list;
static int numfree;

/* The empty Unicode object is shared to improve performance. */
static PyUnicodeObject *unicode_empty;

/* Single character Unicode strings in the Latin-1 range are being
   shared as well. */
static PyUnicodeObject *unicode_latin1[256];

/* Default encoding to use and assume when NULL is passed as encoding
   parameter; it is initialized by _PyUnicode_Init().

   Always use the PyUnicode_SetDefaultEncoding() and
   PyUnicode_GetDefaultEncoding() APIs to access this global.

*/
static char unicode_default_encoding[100];

/* Fast detection of the most frequent whitespace characters */
const unsigned char _Py_ascii_whitespace[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
//     case 0x0009: /* HORIZONTAL TABULATION */
//     case 0x000A: /* LINE FEED */
//     case 0x000B: /* VERTICAL TABULATION */
//     case 0x000C: /* FORM FEED */
//     case 0x000D: /* CARRIAGE RETURN */
	0, 1, 1, 1, 1, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
//     case 0x001C: /* FILE SEPARATOR */
//     case 0x001D: /* GROUP SEPARATOR */
//     case 0x001E: /* RECORD SEPARATOR */
//     case 0x001F: /* UNIT SEPARATOR */
	0, 0, 0, 0, 1, 1, 1, 1,
//     case 0x0020: /* SPACE */
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};

/* Same for linebreaks */
static unsigned char ascii_linebreak[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
//         0x000A, /* LINE FEED */
//         0x000D, /* CARRIAGE RETURN */
	0, 0, 1, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
//         0x001C, /* FILE SEPARATOR */
//         0x001D, /* GROUP SEPARATOR */
//         0x001E, /* RECORD SEPARATOR */
	0, 0, 0, 0, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


Py_UNICODE
PyUnicode_GetMax(void)
{
#ifdef Py_UNICODE_WIDE
	return 0x10FFFF;
#else
	/* This is actually an illegal character, so it should
	   not be passed to unichr. */
	return 0xFFFF;
#endif
}

/* --- Bloom Filters ----------------------------------------------------- */

/* stuff to implement simple "bloom filters" for Unicode characters.
   to keep things simple, we use a single bitmask, using the least 5
   bits from each unicode characters as the bit index. */

/* the linebreak mask is set up by Unicode_Init below */

#define BLOOM_MASK unsigned long

static BLOOM_MASK bloom_linebreak;

#define BLOOM(mask, ch) ((mask & (1 << ((ch) & 0x1F))))

#define BLOOM_LINEBREAK(ch) \
    ((ch) < 128U ? ascii_linebreak[(ch)] : \
    (BLOOM(bloom_linebreak, (ch)) && Py_UNICODE_ISLINEBREAK(ch)))

Py_LOCAL_INLINE(BLOOM_MASK) make_bloom_mask(Py_UNICODE* ptr, Py_ssize_t len)
{
    /* calculate simple bloom-style bitmask for a given unicode string */

    long mask;
    Py_ssize_t i;

    mask = 0;
    for (i = 0; i < len; i++)
        mask |= (1 << (ptr[i] & 0x1F));

    return mask;
}

Py_LOCAL_INLINE(int) unicode_member(Py_UNICODE chr, Py_UNICODE* set, Py_ssize_t setlen)
{
    Py_ssize_t i;

    for (i = 0; i < setlen; i++)
        if (set[i] == chr)
            return 1;

    return 0;
}

#define BLOOM_MEMBER(mask, chr, set, setlen)\
    BLOOM(mask, chr) && unicode_member(chr, set, setlen)

/* --- Unicode Object ----------------------------------------------------- */

static
int unicode_resize(register PyUnicodeObject *unicode,
                      Py_ssize_t length)
{
    void *oldstr;

    /* Shortcut if there's nothing much to do. */
    if (unicode->length == length)
	goto reset;

    /* Resizing shared object (unicode_empty or single character
       objects) in-place is not allowed. Use PyUnicode_Resize()
       instead ! */

    if (unicode == unicode_empty || 
	(unicode->length == 1 && 
	 unicode->str[0] < 256U &&
	 unicode_latin1[unicode->str[0]] == unicode)) {
        PyErr_SetString(PyExc_SystemError,
                        "can't resize shared unicode objects");
        return -1;
    }

    /* We allocate one more byte to make sure the string is Ux0000 terminated.
       The overallocation is also used by fastsearch, which assumes that it's
       safe to look at str[length] (without making any assumptions about what
       it contains). */

    oldstr = unicode->str;
    PyMem_RESIZE(unicode->str, Py_UNICODE, length + 1);
    if (!unicode->str) {
	unicode->str = (Py_UNICODE *)oldstr;
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

/* We allocate one more byte to make sure the string is
   Ux0000 terminated -- XXX is this needed ?

   XXX This allocator could further be enhanced by assuring that the
       free list never reduces its size below 1.

*/

static
PyUnicodeObject *_PyUnicode_New(Py_ssize_t length)
{
    register PyUnicodeObject *unicode;

    /* Optimization for empty strings */
    if (length == 0 && unicode_empty != NULL) {
        Py_INCREF(unicode_empty);
        return unicode_empty;
    }

    /* Unicode freelist & memory allocation */
    if (free_list) {
        unicode = free_list;
        free_list = *(PyUnicodeObject **)unicode;
        numfree--;
	if (unicode->str) {
	    /* Keep-Alive optimization: we only upsize the buffer,
	       never downsize it. */
	    if ((unicode->length < length) &&
                unicode_resize(unicode, length) < 0) {
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
        unicode = PyObject_New(PyUnicodeObject, &PyUnicode_Type);
        if (unicode == NULL)
            return NULL;
	unicode->str = PyMem_NEW(Py_UNICODE, length + 1);
    }

    if (!unicode->str) {
	PyErr_NoMemory();
	goto onError;
    }
    /* Initialize the first element to guard against cases where
     * the caller fails before initializing str -- unicode_resize()
     * reads str[0], and the Keep-Alive optimization can keep memory
     * allocated for str alive across a call to unicode_dealloc(unicode).
     * We don't want unicode_resize to read uninitialized memory in
     * that case.
     */
    unicode->str[0] = 0;
    unicode->str[length] = 0;
    unicode->length = length;
    unicode->hash = -1;
    unicode->defenc = NULL;
    return unicode;

 onError:
    _Py_ForgetReference((PyObject *)unicode);
    PyObject_Del(unicode);
    return NULL;
}

static
void unicode_dealloc(register PyUnicodeObject *unicode)
{
    if (PyUnicode_CheckExact(unicode) &&
	numfree < PyUnicode_MAXFREELIST) {
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
        *(PyUnicodeObject **)unicode = free_list;
        free_list = unicode;
        numfree++;
    }
    else {
	PyMem_DEL(unicode->str);
	Py_XDECREF(unicode->defenc);
	Py_TYPE(unicode)->tp_free((PyObject *)unicode);
    }
}

int PyUnicode_Resize(PyObject **unicode, Py_ssize_t length)
{
    register PyUnicodeObject *v;

    /* Argument checks */
    if (unicode == NULL) {
	PyErr_BadInternalCall();
	return -1;
    }
    v = (PyUnicodeObject *)*unicode;
    if (v == NULL || !PyUnicode_Check(v) || Py_REFCNT(v) != 1 || length < 0) {
	PyErr_BadInternalCall();
	return -1;
    }

    /* Resizing unicode_empty and single character objects is not
       possible since these are being shared. We simply return a fresh
       copy with the same Unicode content. */
    if (v->length != length &&
	(v == unicode_empty || v->length == 1)) {
	PyUnicodeObject *w = _PyUnicode_New(length);
	if (w == NULL)
	    return -1;
	Py_UNICODE_COPY(w->str, v->str,
			length < v->length ? length : v->length);
	Py_DECREF(*unicode);
	*unicode = (PyObject *)w;
	return 0;
    }

    /* Note that we don't have to modify *unicode for unshared Unicode
       objects, since we can modify them in-place. */
    return unicode_resize(v, length);
}

/* Internal API for use in unicodeobject.c only ! */
#define _PyUnicode_Resize(unicodevar, length) \
        PyUnicode_Resize(((PyObject **)(unicodevar)), length)

PyObject *PyUnicode_FromUnicode(const Py_UNICODE *u,
				Py_ssize_t size)
{
    PyUnicodeObject *unicode;

    /* If the Unicode data is known at construction time, we can apply
       some optimizations which share commonly used objects. */
    if (u != NULL) {

	/* Optimization for empty strings */
	if (size == 0 && unicode_empty != NULL) {
	    Py_INCREF(unicode_empty);
	    return (PyObject *)unicode_empty;
	}

	/* Single character Unicode objects in the Latin-1 range are
	   shared when using this constructor */
	if (size == 1 && *u < 256) {
	    unicode = unicode_latin1[*u];
	    if (!unicode) {
		unicode = _PyUnicode_New(1);
		if (!unicode)
		    return NULL;
		unicode->str[0] = *u;
		unicode_latin1[*u] = unicode;
	    }
	    Py_INCREF(unicode);
	    return (PyObject *)unicode;
	}
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;

    /* Copy the Unicode data into the new object */
    if (u != NULL)
	Py_UNICODE_COPY(unicode->str, u, size);

    return (PyObject *)unicode;
}

PyObject *PyUnicode_FromStringAndSize(const char *u, Py_ssize_t size)
{
    PyUnicodeObject *unicode;
    /* If the Unicode data is known at construction time, we can apply
       some optimizations which share commonly used objects.
       Also, this means the input must be UTF-8, so fall back to the
       UTF-8 decoder at the end. */
    if (u != NULL) {

	/* Optimization for empty strings */
	if (size == 0 && unicode_empty != NULL) {
	    Py_INCREF(unicode_empty);
	    return (PyObject *)unicode_empty;
	}

	/* Single characters are shared when using this constructor.
           Restrict to ASCII, since the input must be UTF-8. */
	if (size == 1 && Py_CHARMASK(*u) < 128) {
	    unicode = unicode_latin1[Py_CHARMASK(*u)];
	    if (!unicode) {
		unicode = _PyUnicode_New(1);
		if (!unicode)
		    return NULL;
		unicode->str[0] = Py_CHARMASK(*u);
		unicode_latin1[Py_CHARMASK(*u)] = unicode;
	    }
	    Py_INCREF(unicode);
	    return (PyObject *)unicode;
	}

        return PyUnicode_DecodeUTF8(u, size, NULL);
    }

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;

    return (PyObject *)unicode;
}

PyObject *PyUnicode_FromString(const char *u)
{
    size_t size = strlen(u);
    if (size > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "input too long");
        return NULL;
    }

    return PyUnicode_FromStringAndSize(u, size);
}

#ifdef HAVE_WCHAR_H

PyObject *PyUnicode_FromWideChar(register const wchar_t *w,
				 Py_ssize_t size)
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
	register Py_ssize_t i;
	u = PyUnicode_AS_UNICODE(unicode);
	for (i = size; i > 0; i--)
	    *u++ = *w++;
    }
#endif

    return (PyObject *)unicode;
}

static void
makefmt(char *fmt, int longflag, int size_tflag, int zeropad, int width, int precision, char c)
{
	*fmt++ = '%';
	if (width) {
		if (zeropad)
			*fmt++ = '0';
		fmt += sprintf(fmt, "%d", width);
	}
	if (precision)
		fmt += sprintf(fmt, ".%d", precision);
	if (longflag)
		*fmt++ = 'l';
	else if (size_tflag) {
		char *f = PY_FORMAT_SIZE_T;
		while (*f)
			*fmt++ = *f++;
	}
	*fmt++ = c;
	*fmt = '\0';
}

#define appendstring(string) {for (copy = string;*copy;) *s++ = *copy++;}

PyObject *
PyUnicode_FromFormatV(const char *format, va_list vargs)
{
	va_list count;
	Py_ssize_t callcount = 0;
	PyObject **callresults = NULL;
	PyObject **callresult = NULL;
	Py_ssize_t n = 0;
	int width = 0;
	int precision = 0;
	int zeropad;
	const char* f;
	Py_UNICODE *s;
	PyObject *string;
	/* used by sprintf */
	char buffer[21];
	/* use abuffer instead of buffer, if we need more space
	 * (which can happen if there's a format specifier with width). */
	char *abuffer = NULL;
	char *realbuffer;
	Py_ssize_t abuffersize = 0;
	char fmt[60]; /* should be enough for %0width.precisionld */
	const char *copy;

#ifdef VA_LIST_IS_ARRAY
	Py_MEMCPY(count, vargs, sizeof(va_list));
#else
#ifdef  __va_copy
	__va_copy(count, vargs);
#else
	count = vargs;
#endif
#endif
	/* step 1: count the number of %S/%R format specifications
	 * (we call PyObject_Str()/PyObject_Repr() for these objects
	 * once during step 3 and put the result in an array) */
	for (f = format; *f; f++) {
		if (*f == '%' && (*(f+1)=='S' || *(f+1)=='R'))
			++callcount;
	}
	/* step 2: allocate memory for the results of
	 * PyObject_Str()/PyObject_Repr() calls */
	if (callcount) {
		callresults = PyMem_Malloc(sizeof(PyObject *)*callcount);
		if (!callresults) {
			PyErr_NoMemory();
			return NULL;
		}
		callresult = callresults;
	}
	/* step 3: figure out how large a buffer we need */
	for (f = format; *f; f++) {
		if (*f == '%') {
			const char* p = f;
			width = 0;
			while (isdigit(*f))
				width = (width*10) + *f++ - '0';
			while (*++f && *f != '%' && !isalpha(*f))
				;

			/* skip the 'l' or 'z' in {%ld, %zd, %lu, %zu} since
			 * they don't affect the amount of space we reserve.
			 */
			if ((*f == 'l' || *f == 'z') &&
					(f[1] == 'd' || f[1] == 'u'))
                                ++f;

			switch (*f) {
			case 'c':
				(void)va_arg(count, int);
				/* fall through... */
			case '%':
				n++;
				break;
			case 'd': case 'u': case 'i': case 'x':
				(void) va_arg(count, int);
				/* 20 bytes is enough to hold a 64-bit
				   integer.  Decimal takes the most space.
				   This isn't enough for octal.
				   If a width is specified we need more
				   (which we allocate later). */
				if (width < 20)
					width = 20;
				n += width;
				if (abuffersize < width)
					abuffersize = width;
				break;
			case 's':
			{
				/* UTF-8 */
				unsigned char*s;
				s = va_arg(count, unsigned char*);
				while (*s) {
					if (*s < 128) {
						n++; s++;
					} else if (*s < 0xc0) {
						/* invalid UTF-8 */
						n++; s++;
					} else if (*s < 0xc0) {
						n++;
						s++; if(!*s)break;
						s++;
					} else if (*s < 0xe0) {
						n++;
						s++; if(!*s)break;
						s++; if(!*s)break;
						s++;
					} else {
						#ifdef Py_UNICODE_WIDE
						n++;
						#else
						n+=2;
						#endif
						s++; if(!*s)break;
						s++; if(!*s)break;
						s++; if(!*s)break;
						s++;
					}
				}
				break;
			}
			case 'U':
			{
				PyObject *obj = va_arg(count, PyObject *);
				assert(obj && PyUnicode_Check(obj));
				n += PyUnicode_GET_SIZE(obj);
				break;
			}
			case 'V':
			{
				PyObject *obj = va_arg(count, PyObject *);
				const char *str = va_arg(count, const char *);
				assert(obj || str);
				assert(!obj || PyUnicode_Check(obj));
				if (obj)
					n += PyUnicode_GET_SIZE(obj);
				else
					n += strlen(str);
				break;
			}
			case 'S':
			{
				PyObject *obj = va_arg(count, PyObject *);
				PyObject *str;
				assert(obj);
				str = PyObject_Str(obj);
				if (!str)
					goto fail;
				n += PyUnicode_GET_SIZE(str);
				/* Remember the str and switch to the next slot */
				*callresult++ = str;
				break;
			}
			case 'R':
			{
				PyObject *obj = va_arg(count, PyObject *);
				PyObject *repr;
				assert(obj);
				repr = PyObject_Repr(obj);
				if (!repr)
					goto fail;
				n += PyUnicode_GET_SIZE(repr);
				/* Remember the repr and switch to the next slot */
				*callresult++ = repr;
				break;
			}
			case 'p':
				(void) va_arg(count, int);
				/* maximum 64-bit pointer representation:
				 * 0xffffffffffffffff
				 * so 19 characters is enough.
				 * XXX I count 18 -- what's the extra for?
				 */
				n += 19;
				break;
			default:
				/* if we stumble upon an unknown
				   formatting code, copy the rest of
				   the format string to the output
				   string. (we cannot just skip the
				   code, since there's no way to know
				   what's in the argument list) */
				n += strlen(p);
				goto expand;
			}
		} else
			n++;
	}
 expand:
	if (abuffersize > 20) {
		abuffer = PyMem_Malloc(abuffersize);
		if (!abuffer) {
			PyErr_NoMemory();
			goto fail;
		}
		realbuffer = abuffer;
	}
	else
		realbuffer = buffer;
	/* step 4: fill the buffer */
	/* Since we've analyzed how much space we need for the worst case,
	   we don't have to resize the string.
	   There can be no errors beyond this point. */
	string = PyUnicode_FromUnicode(NULL, n);
	if (!string)
		goto fail;

	s = PyUnicode_AS_UNICODE(string);
	callresult = callresults;

	for (f = format; *f; f++) {
		if (*f == '%') {
			const char* p = f++;
			int longflag = 0;
			int size_tflag = 0;
			zeropad = (*f == '0');
			/* parse the width.precision part */
			width = 0;
			while (isdigit(*f))
				width = (width*10) + *f++ - '0';
			precision = 0;
			if (*f == '.') {
				f++;
				while (isdigit(*f))
					precision = (precision*10) + *f++ - '0';
			}
			/* handle the long flag, but only for %ld and %lu.
			   others can be added when necessary. */
			if (*f == 'l' && (f[1] == 'd' || f[1] == 'u')) {
				longflag = 1;
				++f;
			}
			/* handle the size_t flag. */
			if (*f == 'z' && (f[1] == 'd' || f[1] == 'u')) {
				size_tflag = 1;
				++f;
			}

			switch (*f) {
			case 'c':
				*s++ = va_arg(vargs, int);
				break;
			case 'd':
				makefmt(fmt, longflag, size_tflag, zeropad, width, precision, 'd');
				if (longflag)
					sprintf(realbuffer, fmt, va_arg(vargs, long));
				else if (size_tflag)
					sprintf(realbuffer, fmt, va_arg(vargs, Py_ssize_t));
				else
					sprintf(realbuffer, fmt, va_arg(vargs, int));
				appendstring(realbuffer);
				break;
			case 'u':
				makefmt(fmt, longflag, size_tflag, zeropad, width, precision, 'u');
				if (longflag)
					sprintf(realbuffer, fmt, va_arg(vargs, unsigned long));
				else if (size_tflag)
					sprintf(realbuffer, fmt, va_arg(vargs, size_t));
				else
					sprintf(realbuffer, fmt, va_arg(vargs, unsigned int));
				appendstring(realbuffer);
				break;
			case 'i':
				makefmt(fmt, 0, 0, zeropad, width, precision, 'i');
				sprintf(realbuffer, fmt, va_arg(vargs, int));
				appendstring(realbuffer);
				break;
			case 'x':
				makefmt(fmt, 0, 0, zeropad, width, precision, 'x');
				sprintf(realbuffer, fmt, va_arg(vargs, int));
				appendstring(realbuffer);
				break;
			case 's':
			{
				/* Parameter must be UTF-8 encoded.
				   In case of encoding errors, use
				   the replacement character. */
				PyObject *u;
				p = va_arg(vargs, char*);
				u = PyUnicode_DecodeUTF8(p, strlen(p), 
							 "replace");
				if (!u)
					goto fail;
				Py_UNICODE_COPY(s, PyUnicode_AS_UNICODE(u),
						PyUnicode_GET_SIZE(u));
				s += PyUnicode_GET_SIZE(u);
				Py_DECREF(u);
				break;
			}
			case 'U':
			{
				PyObject *obj = va_arg(vargs, PyObject *);
				Py_ssize_t size = PyUnicode_GET_SIZE(obj);
				Py_UNICODE_COPY(s, PyUnicode_AS_UNICODE(obj), size);
				s += size;
				break;
			}
			case 'V':
			{
				PyObject *obj = va_arg(vargs, PyObject *);
				const char *str = va_arg(vargs, const char *);
				if (obj) {
					Py_ssize_t size = PyUnicode_GET_SIZE(obj);
					Py_UNICODE_COPY(s, PyUnicode_AS_UNICODE(obj), size);
					s += size;
				} else {
					appendstring(str);
				}
				break;
			}
			case 'S':
			case 'R':
			{
				Py_UNICODE *ucopy;
				Py_ssize_t usize;
				Py_ssize_t upos;
				/* unused, since we already have the result */
				(void) va_arg(vargs, PyObject *);
				ucopy = PyUnicode_AS_UNICODE(*callresult);
				usize = PyUnicode_GET_SIZE(*callresult);
				for (upos = 0; upos<usize;)
					*s++ = ucopy[upos++];
				/* We're done with the unicode()/repr() => forget it */
				Py_DECREF(*callresult);
				/* switch to next unicode()/repr() result */
				++callresult;
				break;
			}
			case 'p':
				sprintf(buffer, "%p", va_arg(vargs, void*));
				/* %p is ill-defined:  ensure leading 0x. */
				if (buffer[1] == 'X')
					buffer[1] = 'x';
				else if (buffer[1] != 'x') {
					memmove(buffer+2, buffer, strlen(buffer)+1);
					buffer[0] = '0';
					buffer[1] = 'x';
				}
				appendstring(buffer);
				break;
			case '%':
				*s++ = '%';
				break;
			default:
				appendstring(p);
				goto end;
			}
		} else
			*s++ = *f;
	}

 end:
	if (callresults)
		PyMem_Free(callresults);
	if (abuffer)
		PyMem_Free(abuffer);
	_PyUnicode_Resize(&string, s - PyUnicode_AS_UNICODE(string));
	return string;
 fail:
	if (callresults) {
		PyObject **callresult2 = callresults;
		while (callresult2 < callresult) {
			Py_DECREF(*callresult2);
			++callresult2;
		}
		PyMem_Free(callresults);
	}
	if (abuffer)
		PyMem_Free(abuffer);
	return NULL;
}

#undef appendstring

PyObject *
PyUnicode_FromFormat(const char *format, ...)
{
	PyObject* ret;
	va_list vargs;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif
	ret = PyUnicode_FromFormatV(format, vargs);
	va_end(vargs);
	return ret;
}

Py_ssize_t PyUnicode_AsWideChar(PyUnicodeObject *unicode,
				wchar_t *w,
				Py_ssize_t size)
{
    if (unicode == NULL) {
	PyErr_BadInternalCall();
	return -1;
    }

    /* If possible, try to copy the 0-termination as well */
    if (size > PyUnicode_GET_SIZE(unicode))
	size = PyUnicode_GET_SIZE(unicode) + 1;

#ifdef HAVE_USABLE_WCHAR_T
    memcpy(w, unicode->str, size * sizeof(wchar_t));
#else
    {
	register Py_UNICODE *u;
	register Py_ssize_t i;
	u = PyUnicode_AS_UNICODE(unicode);
	for (i = size; i > 0; i--)
	    *w++ = *u++;
    }
#endif

    if (size > PyUnicode_GET_SIZE(unicode))
        return PyUnicode_GET_SIZE(unicode);
    else
    return size;
}

#endif

PyObject *PyUnicode_FromOrdinal(int ordinal)
{
    Py_UNICODE s[1];

#ifdef Py_UNICODE_WIDE
    if (ordinal < 0 || ordinal > 0x10ffff) {
	PyErr_SetString(PyExc_ValueError,
			"unichr() arg not in range(0x110000) "
			"(wide Python build)");
	return NULL;
    }
#else
    if (ordinal < 0 || ordinal > 0xffff) {
	PyErr_SetString(PyExc_ValueError,
			"unichr() arg not in range(0x10000) "
			"(narrow Python build)");
	return NULL;
    }
#endif

    s[0] = (Py_UNICODE)ordinal;
    return PyUnicode_FromUnicode(s, 1);
}

PyObject *PyUnicode_FromObject(register PyObject *obj)
{
    /* XXX Perhaps we should make this API an alias of
           PyObject_Unicode() instead ?! */
    if (PyUnicode_CheckExact(obj)) {
	Py_INCREF(obj);
	return obj;
    }
    if (PyUnicode_Check(obj)) {
	/* For a Unicode subtype that's not a Unicode object,
	   return a true Unicode object with the same data. */
	return PyUnicode_FromUnicode(PyUnicode_AS_UNICODE(obj),
				     PyUnicode_GET_SIZE(obj));
    }
    return PyUnicode_FromEncodedObject(obj, NULL, "strict");
}

PyObject *PyUnicode_FromEncodedObject(register PyObject *obj,
				      const char *encoding,
				      const char *errors)
{
    const char *s = NULL;
    Py_ssize_t len;
    PyObject *v;

    if (obj == NULL) {
	PyErr_BadInternalCall();
	return NULL;
    }

#if 0
    /* For b/w compatibility we also accept Unicode objects provided
       that no encodings is given and then redirect to
       PyObject_Unicode() which then applies the additional logic for
       Unicode subclasses.

       NOTE: This API should really only be used for object which
             represent *encoded* Unicode !

    */
	if (PyUnicode_Check(obj)) {
	    if (encoding) {
		PyErr_SetString(PyExc_TypeError,
				"decoding Unicode is not supported");
	    return NULL;
	    }
	return PyObject_Unicode(obj);
	    }
#else
    if (PyUnicode_Check(obj)) {
	PyErr_SetString(PyExc_TypeError,
			"decoding Unicode is not supported");
	return NULL;
	}
#endif

    /* Coerce object */
    if (PyString_Check(obj)) {
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
		     Py_TYPE(obj)->tp_name);
	goto onError;
    }

    /* Convert to Unicode */
    if (len == 0) {
	Py_INCREF(unicode_empty);
	v = (PyObject *)unicode_empty;
    }
    else
	v = PyUnicode_Decode(s, len, encoding, errors);

    return v;

 onError:
    return NULL;
}

PyObject *PyUnicode_Decode(const char *s,
			   Py_ssize_t size,
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
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
    else if (strcmp(encoding, "mbcs") == 0)
        return PyUnicode_DecodeMBCS(s, size, errors);
#endif
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
                     Py_TYPE(unicode)->tp_name);
        Py_DECREF(unicode);
        goto onError;
    }
    Py_DECREF(buffer);
    return unicode;

 onError:
    Py_XDECREF(buffer);
    return NULL;
}

PyObject *PyUnicode_AsDecodedObject(PyObject *unicode,
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

    /* Decode via the codec registry */
    v = PyCodec_Decode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    return v;

 onError:
    return NULL;
}

PyObject *PyUnicode_Encode(const Py_UNICODE *s,
			   Py_ssize_t size,
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

PyObject *PyUnicode_AsEncodedObject(PyObject *unicode,
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

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    return v;

 onError:
    return NULL;
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
#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)
	else if (strcmp(encoding, "mbcs") == 0)
	    return PyUnicode_AsMBCSString(unicode);
#endif
	else if (strcmp(encoding, "ascii") == 0)
	    return PyUnicode_AsASCIIString(unicode);
    }

    /* Encode via the codec registry */
    v = PyCodec_Encode(unicode, encoding, errors);
    if (v == NULL)
        goto onError;
    if (!PyString_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "encoder did not return a string object (type=%.400s)",
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        goto onError;
    }
    return v;

 onError:
    return NULL;
}

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

Py_ssize_t PyUnicode_GetSize(PyObject *unicode)
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

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   if no exception occurred, copy the replacement to the output
   and adjust various state variables.
   return 0 on success, -1 on error
*/

static
int unicode_decode_call_errorhandler(const char *errors, PyObject **errorHandler,
                 const char *encoding, const char *reason,
                 const char *input, Py_ssize_t insize, Py_ssize_t *startinpos,
                 Py_ssize_t *endinpos, PyObject **exceptionObject, const char **inptr,
                 PyObject **output, Py_ssize_t *outpos, Py_UNICODE **outptr)
{
    static char *argparse = "O!n;decoding error handler must return (unicode, int) tuple";

    PyObject *restuple = NULL;
    PyObject *repunicode = NULL;
    Py_ssize_t outsize = PyUnicode_GET_SIZE(*output);
    Py_ssize_t requiredsize;
    Py_ssize_t newpos;
    Py_UNICODE *repptr;
    Py_ssize_t repsize;
    int res = -1;

    if (*errorHandler == NULL) {
	*errorHandler = PyCodec_LookupError(errors);
	if (*errorHandler == NULL)
	   goto onError;
    }

    if (*exceptionObject == NULL) {
    	*exceptionObject = PyUnicodeDecodeError_Create(
	    encoding, input, insize, *startinpos, *endinpos, reason);
	if (*exceptionObject == NULL)
	   goto onError;
    }
    else {
	if (PyUnicodeDecodeError_SetStart(*exceptionObject, *startinpos))
	    goto onError;
	if (PyUnicodeDecodeError_SetEnd(*exceptionObject, *endinpos))
	    goto onError;
	if (PyUnicodeDecodeError_SetReason(*exceptionObject, reason))
	    goto onError;
    }

    restuple = PyObject_CallFunctionObjArgs(*errorHandler, *exceptionObject, NULL);
    if (restuple == NULL)
	goto onError;
    if (!PyTuple_Check(restuple)) {
	PyErr_Format(PyExc_TypeError, &argparse[4]);
	goto onError;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &PyUnicode_Type, &repunicode, &newpos))
	goto onError;
    if (newpos<0)
	newpos = insize+newpos;
    if (newpos<0 || newpos>insize) {
	PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", newpos);
	goto onError;
    }

    /* need more space? (at least enough for what we
       have+the replacement+the rest of the string (starting
       at the new input position), so we won't have to check space
       when there are no errors in the rest of the string) */
    repptr = PyUnicode_AS_UNICODE(repunicode);
    repsize = PyUnicode_GET_SIZE(repunicode);
    requiredsize = *outpos + repsize + insize-newpos;
    if (requiredsize > outsize) {
	if (requiredsize<2*outsize)
	    requiredsize = 2*outsize;
	if (PyUnicode_Resize(output, requiredsize) < 0)
	    goto onError;
	*outptr = PyUnicode_AS_UNICODE(*output) + *outpos;
    }
    *endinpos = newpos;
    *inptr = input + newpos;
    Py_UNICODE_COPY(*outptr, repptr, repsize);
    *outptr += repsize;
    *outpos += repsize;
    /* we made it! */
    res = 0;

    onError:
    Py_XDECREF(restuple);
    return res;
}

/* --- UTF-7 Codec -------------------------------------------------------- */

/* see RFC2152 for details */

static
char utf7_special[128] = {
    /* indicate whether a UTF-7 character is special i.e. cannot be directly
       encoded:
	   0 - not special
	   1 - special
	   2 - whitespace (optional)
	   3 - RFC2152 Set O (optional) */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 1, 2, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 3, 3, 3, 3, 3, 3, 0, 0, 0, 3, 1, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 0,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 3, 3, 3,
    3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 1, 1,

};

/* Note: The comparison (c) <= 0 is a trick to work-around gcc
   warnings about the comparison always being false; since
   utf7_special[0] is 1, we can safely make that one comparison
   true  */

#define SPECIAL(c, encodeO, encodeWS) \
    ((c) > 127 || (c) <= 0 || utf7_special[(c)] == 1 || \
     (encodeWS && (utf7_special[(c)] == 2)) || \
     (encodeO && (utf7_special[(c)] == 3)))

#define B64(n)  \
    ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(n) & 0x3f])
#define B64CHAR(c) \
    (isalnum(c) || (c) == '+' || (c) == '/')
#define UB64(c) \
    ((c) == '+' ? 62 : (c) == '/' ? 63 : (c) >= 'a' ?                   \
     (c) - 71 : (c) >= 'A' ? (c) - 65 : (c) + 4 )

#define ENCODE(out, ch, bits)                   \
    while (bits >= 6) {                         \
        *out++ = B64(ch >> (bits-6));           \
        bits -= 6;                              \
    }

#define DECODE(out, ch, bits, surrogate)                                \
    while (bits >= 16) {                                                \
        Py_UNICODE outCh = (Py_UNICODE) ((ch >> (bits-16)) & 0xffff);   \
        bits -= 16;                                                     \
        if (surrogate) {                                                \
            /* We have already generated an error for the high surrogate \
               so let's not bother seeing if the low surrogate is correct or not */ \
            surrogate = 0;                                              \
        } else if (0xDC00 <= outCh && outCh <= 0xDFFF) {                \
            /* This is a surrogate pair. Unfortunately we can't represent \
               it in a 16-bit character */                              \
            surrogate = 1;                                              \
            errmsg = "code pairs are not supported";                    \
            goto utf7Error;                                             \
        } else {                                                        \
            *out++ = outCh;                                             \
        }                                                               \
    }

PyObject *PyUnicode_DecodeUTF7(const char *s,
			       Py_ssize_t size,
			       const char *errors)
{
    return PyUnicode_DecodeUTF7Stateful(s, size, errors, NULL);
}

PyObject *PyUnicode_DecodeUTF7Stateful(const char *s,
			       Py_ssize_t size,
			       const char *errors,
			       Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";
    int inShift = 0;
    unsigned int bitsleft = 0;
    unsigned long charsleft = 0;
    int surrogate = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0) {
        if (consumed)
            *consumed = 0;
        return (PyObject *)unicode;
    }

    p = unicode->str;
    e = s + size;

    while (s < e) {
        Py_UNICODE ch;
        restart:
        ch = *s;

        if (inShift) {
            if ((ch == '-') || !B64CHAR(ch)) {
                inShift = 0;
                s++;

                /* p, charsleft, bitsleft, surrogate = */ DECODE(p, charsleft, bitsleft, surrogate);
                if (bitsleft >= 6) {
                    /* The shift sequence has a partial character in it. If
                       bitsleft < 6 then we could just classify it as padding
                       but that is not the case here */

                    errmsg = "partial character in shift sequence";
                    goto utf7Error;
                }
                /* According to RFC2152 the remaining bits should be zero. We
                   choose to signal an error/insert a replacement character
                   here so indicate the potential of a misencoded character. */

                /* On x86, a << b == a << (b%32) so make sure that bitsleft != 0 */
                if (bitsleft && charsleft << (sizeof(charsleft) * 8 - bitsleft)) {
                    errmsg = "non-zero padding bits in shift sequence";
                    goto utf7Error;
                }

                if (ch == '-') {
                    if ((s < e) && (*(s) == '-')) {
                        *p++ = '-';
                        inShift = 1;
                    }
                } else if (SPECIAL(ch,0,0)) {
                    errmsg = "unexpected special character";
	                goto utf7Error;
                } else  {
                    *p++ = ch;
                }
            } else {
                charsleft = (charsleft << 6) | UB64(ch);
                bitsleft += 6;
                s++;
                /* p, charsleft, bitsleft, surrogate = */ DECODE(p, charsleft, bitsleft, surrogate);
            }
        }
        else if ( ch == '+' ) {
            startinpos = s-starts;
            s++;
            if (s < e && *s == '-') {
                s++;
                *p++ = '+';
            } else
            {
                inShift = 1;
                bitsleft = 0;
            }
        }
        else if (SPECIAL(ch,0,0)) {
            startinpos = s-starts;
            errmsg = "unexpected special character";
            s++;
	        goto utf7Error;
        }
        else {
            *p++ = ch;
            s++;
        }
        continue;
    utf7Error:
        outpos = p-PyUnicode_AS_UNICODE(unicode);
        endinpos = s-starts;
        if (unicode_decode_call_errorhandler(
             errors, &errorHandler,
             "utf7", errmsg,
             starts, size, &startinpos, &endinpos, &exc, &s,
             (PyObject **)&unicode, &outpos, &p))
        goto onError;
    }

    if (inShift && !consumed) {
        outpos = p-PyUnicode_AS_UNICODE(unicode);
        endinpos = size;
        if (unicode_decode_call_errorhandler(
             errors, &errorHandler,
             "utf7", "unterminated shift sequence",
             starts, size, &startinpos, &endinpos, &exc, &s,
             (PyObject **)&unicode, &outpos, &p))
            goto onError;
        if (s < e)
           goto restart;
    }
    if (consumed) {
        if(inShift)
            *consumed = startinpos;
        else
            *consumed = s-starts;
    }

    if (_PyUnicode_Resize(&unicode, p - PyUnicode_AS_UNICODE(unicode)) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)unicode;

onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_DECREF(unicode);
    return NULL;
}


PyObject *PyUnicode_EncodeUTF7(const Py_UNICODE *s,
                   Py_ssize_t size,
                   int encodeSetO,
                   int encodeWhiteSpace,
                   const char *errors)
{
    PyObject *v;
    /* It might be possible to tighten this worst case */
    Py_ssize_t cbAllocated = 5 * size;
    int inShift = 0;
    Py_ssize_t i = 0;
    unsigned int bitsleft = 0;
    unsigned long charsleft = 0;
    char * out;
    char * start;

    if (size == 0)
		return PyString_FromStringAndSize(NULL, 0);

    v = PyString_FromStringAndSize(NULL, cbAllocated);
    if (v == NULL)
        return NULL;

    start = out = PyString_AS_STRING(v);
    for (;i < size; ++i) {
        Py_UNICODE ch = s[i];

        if (!inShift) {
            if (ch == '+') {
                *out++ = '+';
                *out++ = '-';
            } else if (SPECIAL(ch, encodeSetO, encodeWhiteSpace)) {
                charsleft = ch;
                bitsleft = 16;
                *out++ = '+';
                /* out, charsleft, bitsleft = */ ENCODE(out, charsleft, bitsleft);
                inShift = bitsleft > 0;
            } else {
                *out++ = (char) ch;
            }
        } else {
            if (!SPECIAL(ch, encodeSetO, encodeWhiteSpace)) {
                *out++ = B64(charsleft << (6-bitsleft));
                charsleft = 0;
                bitsleft = 0;
                /* Characters not in the BASE64 set implicitly unshift the sequence
                   so no '-' is required, except if the character is itself a '-' */
                if (B64CHAR(ch) || ch == '-') {
                    *out++ = '-';
                }
                inShift = 0;
                *out++ = (char) ch;
            } else {
                bitsleft += 16;
                charsleft = (charsleft << 16) | ch;
                /* out, charsleft, bitsleft = */ ENCODE(out, charsleft, bitsleft);

                /* If the next character is special then we dont' need to terminate
                   the shift sequence. If the next character is not a BASE64 character
                   or '-' then the shift sequence will be terminated implicitly and we
                   don't have to insert a '-'. */

                if (bitsleft == 0) {
                    if (i + 1 < size) {
                        Py_UNICODE ch2 = s[i+1];

                        if (SPECIAL(ch2, encodeSetO, encodeWhiteSpace)) {

                        } else if (B64CHAR(ch2) || ch2 == '-') {
                            *out++ = '-';
                            inShift = 0;
                        } else {
                            inShift = 0;
                        }

                    }
                    else {
                        *out++ = '-';
                        inShift = 0;
                    }
                }
            }
        }
    }
    if (bitsleft) {
        *out++= B64(charsleft << (6-bitsleft) );
        *out++ = '-';
    }

    _PyString_Resize(&v, out - start);
    return v;
}

#undef SPECIAL
#undef B64
#undef B64CHAR
#undef UB64
#undef ENCODE
#undef DECODE

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

PyObject *PyUnicode_DecodeUTF8(const char *s,
			       Py_ssize_t size,
			       const char *errors)
{
    return PyUnicode_DecodeUTF8Stateful(s, size, errors, NULL);
}

PyObject *PyUnicode_DecodeUTF8Stateful(const char *s,
			                Py_ssize_t size,
			                const char *errors,
			                Py_ssize_t *consumed)
{
    const char *starts = s;
    int n;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const char *errmsg = "";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Note: size will always be longer than the resulting Unicode
       character count */
    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0) {
        if (consumed)
            *consumed = 0;
        return (PyObject *)unicode;
    }

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
	    if (consumed)
		break;
	    else {
		errmsg = "unexpected end of data";
		startinpos = s-starts;
		endinpos = size;
		goto utf8Error;
	    }
	}

        switch (n) {

        case 0:
            errmsg = "unexpected code byte";
	    startinpos = s-starts;
	    endinpos = startinpos+1;
	    goto utf8Error;

        case 1:
            errmsg = "internal error";
	    startinpos = s-starts;
	    endinpos = startinpos+1;
	    goto utf8Error;

        case 2:
            if ((s[1] & 0xc0) != 0x80) {
                errmsg = "invalid data";
		startinpos = s-starts;
		endinpos = startinpos+2;
		goto utf8Error;
	    }
            ch = ((s[0] & 0x1f) << 6) + (s[1] & 0x3f);
            if (ch < 0x80) {
		startinpos = s-starts;
		endinpos = startinpos+2;
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
		startinpos = s-starts;
		endinpos = startinpos+3;
		goto utf8Error;
	    }
            ch = ((s[0] & 0x0f) << 12) + ((s[1] & 0x3f) << 6) + (s[2] & 0x3f);
            if (ch < 0x0800) {
		/* Note: UTF-8 encodings of surrogates are considered
		   legal UTF-8 sequences;

		   XXX For wide builds (UCS-4) we should probably try
		       to recombine the surrogates into a single code
		       unit.
		*/
                errmsg = "illegal encoding";
		startinpos = s-starts;
		endinpos = startinpos+3;
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
		startinpos = s-starts;
		endinpos = startinpos+4;
		goto utf8Error;
	    }
            ch = ((s[0] & 0x7) << 18) + ((s[1] & 0x3f) << 12) +
                 ((s[2] & 0x3f) << 6) + (s[3] & 0x3f);
            /* validate and convert to UTF-16 */
            if ((ch < 0x10000)        /* minimum value allowed for 4
					 byte encoding */
                || (ch > 0x10ffff))   /* maximum value allowed for
					 UTF-16 */
	    {
                errmsg = "illegal encoding";
		startinpos = s-starts;
		endinpos = startinpos+4;
		goto utf8Error;
	    }
#ifdef Py_UNICODE_WIDE
	    *p++ = (Py_UNICODE)ch;
#else
            /*  compute and append the two surrogates: */

            /*  translate from 10000..10FFFF to 0..FFFF */
            ch -= 0x10000;

            /*  high surrogate = top 10 bits added to D800 */
            *p++ = (Py_UNICODE)(0xD800 + (ch >> 10));

            /*  low surrogate = bottom 10 bits added to DC00 */
            *p++ = (Py_UNICODE)(0xDC00 + (ch & 0x03FF));
#endif
            break;

        default:
            /* Other sizes are only needed for UCS-4 */
            errmsg = "unsupported Unicode code range";
	    startinpos = s-starts;
	    endinpos = startinpos+n;
	    goto utf8Error;
        }
        s += n;
	continue;

    utf8Error:
    outpos = p-PyUnicode_AS_UNICODE(unicode);
    if (unicode_decode_call_errorhandler(
	     errors, &errorHandler,
	     "utf8", errmsg,
	     starts, size, &startinpos, &endinpos, &exc, &s,
	     (PyObject **)&unicode, &outpos, &p))
	goto onError;
    }
    if (consumed)
	*consumed = s-starts;

    /* Adjust length */
    if (_PyUnicode_Resize(&unicode, p - unicode->str) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)unicode;

onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_DECREF(unicode);
    return NULL;
}

/* Allocation strategy:  if the string is short, convert into a stack buffer
   and allocate exactly as much space needed at the end.  Else allocate the
   maximum possible needed (4 result bytes per Unicode character), and return
   the excess memory at the end.
*/
PyObject *
PyUnicode_EncodeUTF8(const Py_UNICODE *s,
		     Py_ssize_t size,
		     const char *errors)
{
#define MAX_SHORT_UNICHARS 300  /* largest size we'll do on the stack */

    Py_ssize_t i;           /* index into s of next input byte */
    PyObject *v;        /* result string object */
    char *p;            /* next free byte in output buffer */
    Py_ssize_t nallocated;  /* number of result bytes allocated */
    Py_ssize_t nneeded;        /* number of result bytes needed */
    char stackbuf[MAX_SHORT_UNICHARS * 4];

    assert(s != NULL);
    assert(size >= 0);

    if (size <= MAX_SHORT_UNICHARS) {
        /* Write into the stack buffer; nallocated can't overflow.
         * At the end, we'll allocate exactly as much heap space as it
         * turns out we need.
         */
        nallocated = Py_SAFE_DOWNCAST(sizeof(stackbuf), size_t, int);
        v = NULL;   /* will allocate after we're done */
        p = stackbuf;
    }
    else {
        /* Overallocate on the heap, and give the excess back at the end. */
        nallocated = size * 4;
        if (nallocated / 4 != size)  /* overflow! */
            return PyErr_NoMemory();
        v = PyString_FromStringAndSize(NULL, nallocated);
        if (v == NULL)
            return NULL;
        p = PyString_AS_STRING(v);
    }

    for (i = 0; i < size;) {
        Py_UCS4 ch = s[i++];

        if (ch < 0x80)
            /* Encode ASCII */
            *p++ = (char) ch;

        else if (ch < 0x0800) {
            /* Encode Latin-1 */
            *p++ = (char)(0xc0 | (ch >> 6));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
        else {
            /* Encode UCS2 Unicode ordinals */
            if (ch < 0x10000) {
                /* Special case: check for high surrogate */
                if (0xD800 <= ch && ch <= 0xDBFF && i != size) {
                    Py_UCS4 ch2 = s[i];
                    /* Check for low surrogate and combine the two to
                       form a UCS4 value */
                    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
                        ch = ((ch - 0xD800) << 10 | (ch2 - 0xDC00)) + 0x10000;
                        i++;
                        goto encodeUCS4;
                    }
                    /* Fall through: handles isolated high surrogates */
                }
                *p++ = (char)(0xe0 | (ch >> 12));
                *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
                *p++ = (char)(0x80 | (ch & 0x3f));
                continue;
    	    }
encodeUCS4:
            /* Encode UCS4 Unicode ordinals */
            *p++ = (char)(0xf0 | (ch >> 18));
            *p++ = (char)(0x80 | ((ch >> 12) & 0x3f));
            *p++ = (char)(0x80 | ((ch >> 6) & 0x3f));
            *p++ = (char)(0x80 | (ch & 0x3f));
        }
    }

    if (v == NULL) {
        /* This was stack allocated. */
        nneeded = p - stackbuf;
        assert(nneeded <= nallocated);
        v = PyString_FromStringAndSize(stackbuf, nneeded);
    }
    else {
    	/* Cut back to size actually needed. */
        nneeded = p - PyString_AS_STRING(v);
        assert(nneeded <= nallocated);
        _PyString_Resize(&v, nneeded);
    }
    return v;

#undef MAX_SHORT_UNICHARS
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

/* --- UTF-32 Codec ------------------------------------------------------- */

PyObject *
PyUnicode_DecodeUTF32(const char *s,
		      Py_ssize_t size,
		      const char *errors,
		      int *byteorder)
{
    return PyUnicode_DecodeUTF32Stateful(s, size, errors, byteorder, NULL);
}

PyObject *
PyUnicode_DecodeUTF32Stateful(const char *s,
			      Py_ssize_t size,
			      const char *errors,
			      int *byteorder,
			      Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
#ifndef Py_UNICODE_WIDE
    int i, pairs;
#else
    const int pairs = 0;
#endif
    const unsigned char *q, *e;
    int bo = 0;       /* assume native ordering by default */
    const char *errmsg = "";
    /* Offsets from q for retrieving bytes in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int iorder[] = {0, 1, 2, 3};
#else
    int iorder[] = {3, 2, 1, 0};
#endif
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* On narrow builds we split characters outside the BMP into two
       codepoints => count how much extra space we need. */
#ifndef Py_UNICODE_WIDE
    for (i = pairs = 0; i < size/4; i++)
	if (((Py_UCS4 *)s)[i] >= 0x10000)
	    pairs++;
#endif

    /* This might be one to much, because of a BOM */
    unicode = _PyUnicode_New((size+3)/4+pairs);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-32 encoded data */
    p = unicode->str;
    q = (unsigned char *)s;
    e = q + size;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0) {
        if (size >= 4) {
            const Py_UCS4 bom = (q[iorder[3]] << 24) | (q[iorder[2]] << 16) |
                                (q[iorder[1]] << 8) | q[iorder[0]];
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
	    if (bom == 0x0000FEFF) {
		q += 4;
		bo = -1;
	    }
	    else if (bom == 0xFFFE0000) {
		q += 4;
		bo = 1;
	    }
#else
	    if (bom == 0x0000FEFF) {
		q += 4;
		bo = 1;
	    }
	    else if (bom == 0xFFFE0000) {
		q += 4;
		bo = -1;
	    }
#endif
	}
    }

    if (bo == -1) {
        /* force LE */
        iorder[0] = 0;
        iorder[1] = 1;
        iorder[2] = 2;
        iorder[3] = 3;
    }
    else if (bo == 1) {
        /* force BE */
        iorder[0] = 3;
        iorder[1] = 2;
        iorder[2] = 1;
        iorder[3] = 0;
    }

    while (q < e) {
	Py_UCS4 ch;
	/* remaining bytes at the end? (size should be divisible by 4) */
	if (e-q<4) {
	    if (consumed)
		break;
	    errmsg = "truncated data";
	    startinpos = ((const char *)q)-starts;
	    endinpos = ((const char *)e)-starts;
	    goto utf32Error;
	    /* The remaining input chars are ignored if the callback
	       chooses to skip the input */
	}
	ch = (q[iorder[3]] << 24) | (q[iorder[2]] << 16) |
	     (q[iorder[1]] << 8) | q[iorder[0]];

	if (ch >= 0x110000)
	{
	    errmsg = "codepoint not in range(0x110000)";
	    startinpos = ((const char *)q)-starts;
	    endinpos = startinpos+4;
	    goto utf32Error;
	}
#ifndef Py_UNICODE_WIDE
	if (ch >= 0x10000)
	{
	    *p++ = 0xD800 | ((ch-0x10000) >> 10);
	    *p++ = 0xDC00 | ((ch-0x10000) & 0x3FF);
	}
	else
#endif
	    *p++ = ch;
	q += 4;
	continue;
    utf32Error:
	outpos = p-PyUnicode_AS_UNICODE(unicode);
    if (unicode_decode_call_errorhandler(
         errors, &errorHandler,
         "utf32", errmsg,
         starts, size, &startinpos, &endinpos, &exc, &s,
         (PyObject **)&unicode, &outpos, &p))
	    goto onError;
    }

    if (byteorder)
        *byteorder = bo;

    if (consumed)
	*consumed = (const char *)q-starts;

    /* Adjust length */
    if (_PyUnicode_Resize(&unicode, p - unicode->str) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeUTF32(const Py_UNICODE *s,
		      Py_ssize_t size,
		      const char *errors,
		      int byteorder)
{
    PyObject *v;
    unsigned char *p;
#ifndef Py_UNICODE_WIDE
    int i, pairs;
#else
    const int pairs = 0;
#endif
    /* Offsets from p for storing byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int iorder[] = {0, 1, 2, 3};
#else
    int iorder[] = {3, 2, 1, 0};
#endif

#define STORECHAR(CH)                       \
    do {                                    \
        p[iorder[3]] = ((CH) >> 24) & 0xff; \
        p[iorder[2]] = ((CH) >> 16) & 0xff; \
        p[iorder[1]] = ((CH) >> 8) & 0xff;  \
        p[iorder[0]] = (CH) & 0xff;         \
        p += 4;                             \
    } while(0)

    /* In narrow builds we can output surrogate pairs as one codepoint,
       so we need less space. */
#ifndef Py_UNICODE_WIDE
    for (i = pairs = 0; i < size-1; i++)
	if (0xD800 <= s[i] && s[i] <= 0xDBFF &&
	    0xDC00 <= s[i+1] && s[i+1] <= 0xDFFF)
	    pairs++;
#endif
    v = PyString_FromStringAndSize(NULL,
		  4 * (size - pairs + (byteorder == 0)));
    if (v == NULL)
        return NULL;

    p = (unsigned char *)PyString_AS_STRING(v);
    if (byteorder == 0)
	STORECHAR(0xFEFF);
    if (size == 0)
        return v;

    if (byteorder == -1) {
        /* force LE */
        iorder[0] = 0;
        iorder[1] = 1;
        iorder[2] = 2;
        iorder[3] = 3;
    }
    else if (byteorder == 1) {
        /* force BE */
        iorder[0] = 3;
        iorder[1] = 2;
        iorder[2] = 1;
        iorder[3] = 0;
    }

    while (size-- > 0) {
	Py_UCS4 ch = *s++;
#ifndef Py_UNICODE_WIDE
	if (0xD800 <= ch && ch <= 0xDBFF && size > 0) {
	    Py_UCS4 ch2 = *s;
	    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
		ch = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
		s++;
		size--;
	    }
	}
#endif
        STORECHAR(ch);
    }
    return v;
#undef STORECHAR
}

PyObject *PyUnicode_AsUTF32String(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeUTF32(PyUnicode_AS_UNICODE(unicode),
				 PyUnicode_GET_SIZE(unicode),
				 NULL,
				 0);
}

/* --- UTF-16 Codec ------------------------------------------------------- */

PyObject *
PyUnicode_DecodeUTF16(const char *s,
		      Py_ssize_t size,
		      const char *errors,
		      int *byteorder)
{
    return PyUnicode_DecodeUTF16Stateful(s, size, errors, byteorder, NULL);
}

PyObject *
PyUnicode_DecodeUTF16Stateful(const char *s,
			      Py_ssize_t size,
			      const char *errors,
			      int *byteorder,
			      Py_ssize_t *consumed)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *unicode;
    Py_UNICODE *p;
    const unsigned char *q, *e;
    int bo = 0;       /* assume native ordering by default */
    const char *errmsg = "";
    /* Offsets from q for retrieving byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int ihi = 1, ilo = 0;
#else
    int ihi = 0, ilo = 1;
#endif
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Note: size will always be longer than the resulting Unicode
       character count */
    unicode = _PyUnicode_New(size);
    if (!unicode)
        return NULL;
    if (size == 0)
        return (PyObject *)unicode;

    /* Unpack UTF-16 encoded data */
    p = unicode->str;
    q = (unsigned char *)s;
    e = q + size;

    if (byteorder)
        bo = *byteorder;

    /* Check for BOM marks (U+FEFF) in the input and adjust current
       byte order setting accordingly. In native mode, the leading BOM
       mark is skipped, in all other modes, it is copied to the output
       stream as-is (giving a ZWNBSP character). */
    if (bo == 0) {
        if (size >= 2) {
            const Py_UNICODE bom = (q[ihi] << 8) | q[ilo];
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
	    if (bom == 0xFEFF) {
		q += 2;
		bo = -1;
	    }
	    else if (bom == 0xFFFE) {
		q += 2;
		bo = 1;
	    }
#else
	    if (bom == 0xFEFF) {
		q += 2;
		bo = 1;
	    }
	    else if (bom == 0xFFFE) {
		q += 2;
		bo = -1;
	    }
#endif
	}
    }

    if (bo == -1) {
        /* force LE */
        ihi = 1;
        ilo = 0;
    }
    else if (bo == 1) {
        /* force BE */
        ihi = 0;
        ilo = 1;
    }

    while (q < e) {
	Py_UNICODE ch;
	/* remaining bytes at the end? (size should be even) */
	if (e-q<2) {
	    if (consumed)
		break;
	    errmsg = "truncated data";
	    startinpos = ((const char *)q)-starts;
	    endinpos = ((const char *)e)-starts;
	    goto utf16Error;
	    /* The remaining input chars are ignored if the callback
	       chooses to skip the input */
	}
	ch = (q[ihi] << 8) | q[ilo];

	q += 2;

	if (ch < 0xD800 || ch > 0xDFFF) {
	    *p++ = ch;
	    continue;
	}

	/* UTF-16 code pair: */
	if (q >= e) {
	    errmsg = "unexpected end of data";
	    startinpos = (((const char *)q)-2)-starts;
	    endinpos = ((const char *)e)-starts;
	    goto utf16Error;
	}
	if (0xD800 <= ch && ch <= 0xDBFF) {
	    Py_UNICODE ch2 = (q[ihi] << 8) | q[ilo];
	    q += 2;
	    if (0xDC00 <= ch2 && ch2 <= 0xDFFF) {
#ifndef Py_UNICODE_WIDE
		*p++ = ch;
		*p++ = ch2;
#else
		*p++ = (((ch & 0x3FF)<<10) | (ch2 & 0x3FF)) + 0x10000;
#endif
		continue;
	    }
	    else {
                errmsg = "illegal UTF-16 surrogate";
		startinpos = (((const char *)q)-4)-starts;
		endinpos = startinpos+2;
		goto utf16Error;
	    }

	}
	errmsg = "illegal encoding";
	startinpos = (((const char *)q)-2)-starts;
	endinpos = startinpos+2;
	/* Fall through to report the error */

    utf16Error:
	outpos = p-PyUnicode_AS_UNICODE(unicode);
	if (unicode_decode_call_errorhandler(
	         errors, &errorHandler,
	         "utf16", errmsg,
	         starts, size, &startinpos, &endinpos, &exc, (const char **)&q,
	         (PyObject **)&unicode, &outpos, &p))
	    goto onError;
    }

    if (byteorder)
        *byteorder = bo;

    if (consumed)
	*consumed = (const char *)q-starts;

    /* Adjust length */
    if (_PyUnicode_Resize(&unicode, p - unicode->str) < 0)
        goto onError;

    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)unicode;

onError:
    Py_DECREF(unicode);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *
PyUnicode_EncodeUTF16(const Py_UNICODE *s,
		      Py_ssize_t size,
		      const char *errors,
		      int byteorder)
{
    PyObject *v;
    unsigned char *p;
#ifdef Py_UNICODE_WIDE
    int i, pairs;
#else
    const int pairs = 0;
#endif
    /* Offsets from p for storing byte pairs in the right order. */
#ifdef BYTEORDER_IS_LITTLE_ENDIAN
    int ihi = 1, ilo = 0;
#else
    int ihi = 0, ilo = 1;
#endif

#define STORECHAR(CH)                   \
    do {                                \
        p[ihi] = ((CH) >> 8) & 0xff;    \
        p[ilo] = (CH) & 0xff;           \
        p += 2;                         \
    } while(0)

#ifdef Py_UNICODE_WIDE
    for (i = pairs = 0; i < size; i++)
	if (s[i] >= 0x10000)
	    pairs++;
#endif
    v = PyString_FromStringAndSize(NULL,
		  2 * (size + pairs + (byteorder == 0)));
    if (v == NULL)
        return NULL;

    p = (unsigned char *)PyString_AS_STRING(v);
    if (byteorder == 0)
	STORECHAR(0xFEFF);
    if (size == 0)
        return v;

    if (byteorder == -1) {
        /* force LE */
        ihi = 1;
        ilo = 0;
    }
    else if (byteorder == 1) {
        /* force BE */
        ihi = 0;
        ilo = 1;
    }

    while (size-- > 0) {
	Py_UNICODE ch = *s++;
	Py_UNICODE ch2 = 0;
#ifdef Py_UNICODE_WIDE
	if (ch >= 0x10000) {
	    ch2 = 0xDC00 | ((ch-0x10000) & 0x3FF);
	    ch  = 0xD800 | ((ch-0x10000) >> 10);
	}
#endif
        STORECHAR(ch);
        if (ch2)
            STORECHAR(ch2);
    }
    return v;
#undef STORECHAR
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

static _PyUnicode_Name_CAPI *ucnhash_CAPI = NULL;

PyObject *PyUnicode_DecodeUnicodeEscape(const char *s,
					Py_ssize_t size,
					const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    int i;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    char* message;
    Py_UCS4 chr = 0xffffffff; /* in case 'getcode' messes up */
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value.
       (but if the error callback returns a long replacement string
       we'll have to allocate more space) */
    v = _PyUnicode_New(size);
    if (v == NULL)
        goto onError;
    if (size == 0)
        return (PyObject *)v;

    p = PyUnicode_AS_UNICODE(v);
    end = s + size;

    while (s < end) {
        unsigned char c;
        Py_UNICODE x;
        int digits;

        /* Non-escape characters are interpreted as Unicode ordinals */
        if (*s != '\\') {
            *p++ = (unsigned char) *s++;
            continue;
        }

        startinpos = s-starts;
        /* \ - Escapes */
        s++;
        c = *s++;
        if (s > end)
            c = '\0'; /* Invalid after \ */
        switch (c) {

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
            if (s < end && '0' <= *s && *s <= '7') {
                x = (x<<3) + *s++ - '0';
                if (s < end && '0' <= *s && *s <= '7')
                    x = (x<<3) + *s++ - '0';
            }
            *p++ = x;
            break;

        /* hex escapes */
        /* \xXX */
        case 'x':
            digits = 2;
            message = "truncated \\xXX escape";
            goto hexescape;

        /* \uXXXX */
        case 'u':
            digits = 4;
            message = "truncated \\uXXXX escape";
            goto hexescape;

        /* \UXXXXXXXX */
        case 'U':
            digits = 8;
            message = "truncated \\UXXXXXXXX escape";
        hexescape:
            chr = 0;
            outpos = p-PyUnicode_AS_UNICODE(v);
            if (s+digits>end) {
                endinpos = size;
                if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicodeescape", "end of string in escape sequence",
                    starts, size, &startinpos, &endinpos, &exc, &s,
                    (PyObject **)&v, &outpos, &p))
                    goto onError;
                goto nextByte;
            }
            for (i = 0; i < digits; ++i) {
                c = (unsigned char) s[i];
                if (!isxdigit(c)) {
                    endinpos = (s+i+1)-starts;
                    if (unicode_decode_call_errorhandler(
                        errors, &errorHandler,
                        "unicodeescape", message,
                        starts, size, &startinpos, &endinpos, &exc, &s,
                        (PyObject **)&v, &outpos, &p))
                        goto onError;
                    goto nextByte;
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
            if (chr == 0xffffffff && PyErr_Occurred())
                /* _decoding_error will have already written into the
                   target buffer. */
                break;
        store:
            /* when we get here, chr is a 32-bit unicode character */
            if (chr <= 0xffff)
                /* UCS-2 character */
                *p++ = (Py_UNICODE) chr;
            else if (chr <= 0x10ffff) {
                /* UCS-4 character. Either store directly, or as
                   surrogate pair. */
#ifdef Py_UNICODE_WIDE
                *p++ = chr;
#else
                chr -= 0x10000L;
                *p++ = 0xD800 + (Py_UNICODE) (chr >> 10);
                *p++ = 0xDC00 + (Py_UNICODE) (chr & 0x03FF);
#endif
            } else {
                endinpos = s-starts;
                outpos = p-PyUnicode_AS_UNICODE(v);
                if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicodeescape", "illegal Unicode character",
                    starts, size, &startinpos, &endinpos, &exc, &s,
                    (PyObject **)&v, &outpos, &p))
                    goto onError;
            }
            break;

        /* \N{name} */
        case 'N':
            message = "malformed \\N character escape";
            if (ucnhash_CAPI == NULL) {
                /* load the unicode data module */
                PyObject *m, *api;
                m = PyImport_ImportModuleNoBlock("unicodedata");
                if (m == NULL)
                    goto ucnhashError;
                api = PyObject_GetAttrString(m, "ucnhash_CAPI");
                Py_DECREF(m);
                if (api == NULL)
                    goto ucnhashError;
                ucnhash_CAPI = (_PyUnicode_Name_CAPI *)PyCObject_AsVoidPtr(api);
                Py_DECREF(api);
                if (ucnhash_CAPI == NULL)
                    goto ucnhashError;
            }
            if (*s == '{') {
                const char *start = s+1;
                /* look for the closing brace */
                while (*s != '}' && s < end)
                    s++;
                if (s > start && s < end && *s == '}') {
                    /* found a name.  look it up in the unicode database */
                    message = "unknown Unicode character name";
                    s++;
                    if (ucnhash_CAPI->getcode(NULL, start, (int)(s-start-1), &chr))
                        goto store;
                }
            }
            endinpos = s-starts;
            outpos = p-PyUnicode_AS_UNICODE(v);
            if (unicode_decode_call_errorhandler(
                errors, &errorHandler,
                "unicodeescape", message,
                starts, size, &startinpos, &endinpos, &exc, &s,
                (PyObject **)&v, &outpos, &p))
                goto onError;
            break;

        default:
            if (s > end) {
                message = "\\ at end of string";
                s--;
                endinpos = s-starts;
                outpos = p-PyUnicode_AS_UNICODE(v);
                if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicodeescape", message,
                    starts, size, &startinpos, &endinpos, &exc, &s,
                    (PyObject **)&v, &outpos, &p))
                    goto onError;
            }
            else {
                *p++ = '\\';
                *p++ = (unsigned char)s[-1];
            }
            break;
        }
        nextByte:
        ;
    }
    if (_PyUnicode_Resize(&v, p - PyUnicode_AS_UNICODE(v)) < 0)
        goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)v;

ucnhashError:
    PyErr_SetString(
        PyExc_UnicodeError,
        "\\N escapes not supported (can't load unicodedata module)"
        );
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;

onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

/* Return a Unicode-Escape string version of the Unicode object.

   If quotes is true, the string is enclosed in u"" or u'' quotes as
   appropriate.

*/

Py_LOCAL_INLINE(const Py_UNICODE *) findchar(const Py_UNICODE *s,
                                      Py_ssize_t size,
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

static
PyObject *unicodeescape_string(const Py_UNICODE *s,
                               Py_ssize_t size,
                               int quotes)
{
    PyObject *repr;
    char *p;

    static const char *hexdigit = "0123456789abcdef";

    /* XXX(nnorwitz): rather than over-allocating, it would be
       better to choose a different scheme.  Perhaps scan the
       first N-chars of the string and allocate based on that size.
    */
    /* Initial allocation is based on the longest-possible unichr
       escape.

       In wide (UTF-32) builds '\U00xxxxxx' is 10 chars per source
       unichr, so in this case it's the longest unichr escape. In
       narrow (UTF-16) builds this is five chars per source unichr
       since there are two unichrs in the surrogate pair, so in narrow
       (UTF-16) builds it's not the longest unichr escape.

       In wide or narrow builds '\uxxxx' is 6 chars per source unichr,
       so in the narrow (UTF-16) build case it's the longest unichr
       escape.
    */

    repr = PyString_FromStringAndSize(NULL,
        2
#ifdef Py_UNICODE_WIDE
        + 10*size
#else
        + 6*size
#endif
        + 1);
    if (repr == NULL)
        return NULL;

    p = PyString_AS_STRING(repr);

    if (quotes) {
        *p++ = 'u';
        *p++ = (findchar(s, size, '\'') &&
                !findchar(s, size, '"')) ? '"' : '\'';
    }
    while (size-- > 0) {
        Py_UNICODE ch = *s++;

        /* Escape quotes and backslashes */
        if ((quotes &&
	     ch == (Py_UNICODE) PyString_AS_STRING(repr)[1]) || ch == '\\') {
            *p++ = '\\';
            *p++ = (char) ch;
	    continue;
        }

#ifdef Py_UNICODE_WIDE
        /* Map 21-bit characters to '\U00xxxxxx' */
        else if (ch >= 0x10000) {
            *p++ = '\\';
            *p++ = 'U';
            *p++ = hexdigit[(ch >> 28) & 0x0000000F];
            *p++ = hexdigit[(ch >> 24) & 0x0000000F];
            *p++ = hexdigit[(ch >> 20) & 0x0000000F];
            *p++ = hexdigit[(ch >> 16) & 0x0000000F];
            *p++ = hexdigit[(ch >> 12) & 0x0000000F];
            *p++ = hexdigit[(ch >> 8) & 0x0000000F];
            *p++ = hexdigit[(ch >> 4) & 0x0000000F];
            *p++ = hexdigit[ch & 0x0000000F];
	    continue;
        }
#else
	/* Map UTF-16 surrogate pairs to '\U00xxxxxx' */
	else if (ch >= 0xD800 && ch < 0xDC00) {
	    Py_UNICODE ch2;
	    Py_UCS4 ucs;

	    ch2 = *s++;
	    size--;
	    if (ch2 >= 0xDC00 && ch2 <= 0xDFFF) {
		ucs = (((ch & 0x03FF) << 10) | (ch2 & 0x03FF)) + 0x00010000;
		*p++ = '\\';
		*p++ = 'U';
		*p++ = hexdigit[(ucs >> 28) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 24) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 20) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 16) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 12) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 8) & 0x0000000F];
		*p++ = hexdigit[(ucs >> 4) & 0x0000000F];
		*p++ = hexdigit[ucs & 0x0000000F];
		continue;
	    }
	    /* Fall through: isolated surrogates are copied as-is */
	    s--;
	    size++;
	}
#endif

        /* Map 16-bit characters to '\uxxxx' */
        if (ch >= 256) {
            *p++ = '\\';
            *p++ = 'u';
            *p++ = hexdigit[(ch >> 12) & 0x000F];
            *p++ = hexdigit[(ch >> 8) & 0x000F];
            *p++ = hexdigit[(ch >> 4) & 0x000F];
            *p++ = hexdigit[ch & 0x000F];
        }

        /* Map special whitespace to '\t', \n', '\r' */
        else if (ch == '\t') {
            *p++ = '\\';
            *p++ = 't';
        }
        else if (ch == '\n') {
            *p++ = '\\';
            *p++ = 'n';
        }
        else if (ch == '\r') {
            *p++ = '\\';
            *p++ = 'r';
        }

        /* Map non-printable US ASCII to '\xhh' */
        else if (ch < ' ' || ch >= 0x7F) {
            *p++ = '\\';
            *p++ = 'x';
            *p++ = hexdigit[(ch >> 4) & 0x000F];
            *p++ = hexdigit[ch & 0x000F];
        }

        /* Copy everything else as-is */
        else
            *p++ = (char) ch;
    }
    if (quotes)
        *p++ = PyString_AS_STRING(repr)[1];

    *p = '\0';
    _PyString_Resize(&repr, p - PyString_AS_STRING(repr));
    return repr;
}

PyObject *PyUnicode_EncodeUnicodeEscape(const Py_UNICODE *s,
					Py_ssize_t size)
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
					   Py_ssize_t size,
					   const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    const char *bs;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* Escaped strings will always be longer than the resulting
       Unicode string, so we start with size here and then reduce the
       length after conversion to the true value. (But decoding error
       handler might have to resize the string) */
    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    end = s + size;
    while (s < end) {
	unsigned char c;
	Py_UCS4 x;
	int i;
        int count;

	/* Non-escape characters are interpreted as Unicode ordinals */
	if (*s != '\\') {
	    *p++ = (unsigned char)*s++;
	    continue;
	}
	startinpos = s-starts;

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
	    (*s != 'u' && *s != 'U')) {
	    continue;
	}
	p--;
        count = *s=='u' ? 4 : 8;
	s++;

	/* \uXXXX with 4 hex digits, \Uxxxxxxxx with 8 */
	outpos = p-PyUnicode_AS_UNICODE(v);
	for (x = 0, i = 0; i < count; ++i, ++s) {
	    c = (unsigned char)*s;
	    if (!isxdigit(c)) {
		endinpos = s-starts;
		if (unicode_decode_call_errorhandler(
		    errors, &errorHandler,
		    "rawunicodeescape", "truncated \\uXXXX",
		    starts, size, &startinpos, &endinpos, &exc, &s,
		    (PyObject **)&v, &outpos, &p))
		    goto onError;
		goto nextByte;
	    }
	    x = (x<<4) & ~0xF;
	    if (c >= '0' && c <= '9')
		x += c - '0';
	    else if (c >= 'a' && c <= 'f')
		x += 10 + c - 'a';
	    else
		x += 10 + c - 'A';
	}
#ifndef Py_UNICODE_WIDE
        if (x > 0x10000) {
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "rawunicodeescape", "\\Uxxxxxxxx out of range",
		    starts, size, &startinpos, &endinpos, &exc, &s,
		    (PyObject **)&v, &outpos, &p))
		    goto onError;
        }
#endif
	*p++ = x;
	nextByte:
	;
    }
    if (_PyUnicode_Resize(&v, p - PyUnicode_AS_UNICODE(v)) < 0)
	goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)v;

 onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *PyUnicode_EncodeRawUnicodeEscape(const Py_UNICODE *s,
					   Py_ssize_t size)
{
    PyObject *repr;
    char *p;
    char *q;

    static const char *hexdigit = "0123456789abcdef";

#ifdef Py_UNICODE_WIDE
    repr = PyString_FromStringAndSize(NULL, 10 * size);
#else
    repr = PyString_FromStringAndSize(NULL, 6 * size);
#endif
    if (repr == NULL)
        return NULL;
    if (size == 0)
	return repr;

    p = q = PyString_AS_STRING(repr);
    while (size-- > 0) {
        Py_UNICODE ch = *s++;
#ifdef Py_UNICODE_WIDE
	/* Map 32-bit characters to '\Uxxxxxxxx' */
	if (ch >= 0x10000) {
            *p++ = '\\';
            *p++ = 'U';
            *p++ = hexdigit[(ch >> 28) & 0xf];
            *p++ = hexdigit[(ch >> 24) & 0xf];
            *p++ = hexdigit[(ch >> 20) & 0xf];
            *p++ = hexdigit[(ch >> 16) & 0xf];
            *p++ = hexdigit[(ch >> 12) & 0xf];
            *p++ = hexdigit[(ch >> 8) & 0xf];
            *p++ = hexdigit[(ch >> 4) & 0xf];
            *p++ = hexdigit[ch & 15];
        }
        else
#endif
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
    _PyString_Resize(&repr, p - q);
    return repr;
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

/* --- Unicode Internal Codec ------------------------------------------- */

PyObject *_PyUnicode_DecodeUnicodeInternal(const char *s,
					   Py_ssize_t size,
					   const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    const char *end;
    const char *reason;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

#ifdef Py_UNICODE_WIDE
    Py_UNICODE unimax = PyUnicode_GetMax();
#endif

    /* XXX overflow detection missing */
    v = _PyUnicode_New((size+Py_UNICODE_SIZE-1)/ Py_UNICODE_SIZE);
    if (v == NULL)
	goto onError;
    if (PyUnicode_GetSize((PyObject *)v) == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    end = s + size;

    while (s < end) {
        memcpy(p, s, sizeof(Py_UNICODE));
        /* We have to sanity check the raw data, otherwise doom looms for
           some malformed UCS-4 data. */
        if (
            #ifdef Py_UNICODE_WIDE
            *p > unimax || *p < 0 ||
            #endif
            end-s < Py_UNICODE_SIZE
            )
            {
            startinpos = s - starts;
            if (end-s < Py_UNICODE_SIZE) {
                endinpos = end-starts;
                reason = "truncated input";
            }
            else {
                endinpos = s - starts + Py_UNICODE_SIZE;
                reason = "illegal code point (> 0x10FFFF)";
            }
            outpos = p - PyUnicode_AS_UNICODE(v);
            if (unicode_decode_call_errorhandler(
                    errors, &errorHandler,
                    "unicode_internal", reason,
                    starts, size, &startinpos, &endinpos, &exc, &s,
                    (PyObject **)&v, &outpos, &p)) {
                goto onError;
            }
        }
        else {
            p++;
            s += Py_UNICODE_SIZE;
        }
    }

    if (_PyUnicode_Resize(&v, p - PyUnicode_AS_UNICODE(v)) < 0)
        goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)v;

 onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

/* --- Latin-1 Codec ------------------------------------------------------ */

PyObject *PyUnicode_DecodeLatin1(const char *s,
				 Py_ssize_t size,
				 const char *errors)
{
    PyUnicodeObject *v;
    Py_UNICODE *p;

    /* Latin-1 is equivalent to the first 256 ordinals in Unicode. */
    if (size == 1) {
	Py_UNICODE r = *(unsigned char*)s;
	return PyUnicode_FromUnicode(&r, 1);
    }

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

/* create or adjust a UnicodeEncodeError */
static void make_encode_exception(PyObject **exceptionObject,
    const char *encoding,
    const Py_UNICODE *unicode, Py_ssize_t size,
    Py_ssize_t startpos, Py_ssize_t endpos,
    const char *reason)
{
    if (*exceptionObject == NULL) {
	*exceptionObject = PyUnicodeEncodeError_Create(
	    encoding, unicode, size, startpos, endpos, reason);
    }
    else {
	if (PyUnicodeEncodeError_SetStart(*exceptionObject, startpos))
	    goto onError;
	if (PyUnicodeEncodeError_SetEnd(*exceptionObject, endpos))
	    goto onError;
	if (PyUnicodeEncodeError_SetReason(*exceptionObject, reason))
	    goto onError;
	return;
	onError:
	Py_DECREF(*exceptionObject);
	*exceptionObject = NULL;
    }
}

/* raises a UnicodeEncodeError */
static void raise_encode_exception(PyObject **exceptionObject,
    const char *encoding,
    const Py_UNICODE *unicode, Py_ssize_t size,
    Py_ssize_t startpos, Py_ssize_t endpos,
    const char *reason)
{
    make_encode_exception(exceptionObject,
	encoding, unicode, size, startpos, endpos, reason);
    if (*exceptionObject != NULL)
	PyCodec_StrictErrors(*exceptionObject);
}

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   put the result into newpos and return the replacement string, which
   has to be freed by the caller */
static PyObject *unicode_encode_call_errorhandler(const char *errors,
    PyObject **errorHandler,
    const char *encoding, const char *reason,
    const Py_UNICODE *unicode, Py_ssize_t size, PyObject **exceptionObject,
    Py_ssize_t startpos, Py_ssize_t endpos,
    Py_ssize_t *newpos)
{
    static char *argparse = "O!n;encoding error handler must return (unicode, int) tuple";

    PyObject *restuple;
    PyObject *resunicode;

    if (*errorHandler == NULL) {
	*errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
	    return NULL;
    }

    make_encode_exception(exceptionObject,
	encoding, unicode, size, startpos, endpos, reason);
    if (*exceptionObject == NULL)
	return NULL;

    restuple = PyObject_CallFunctionObjArgs(
	*errorHandler, *exceptionObject, NULL);
    if (restuple == NULL)
	return NULL;
    if (!PyTuple_Check(restuple)) {
	PyErr_Format(PyExc_TypeError, &argparse[4]);
	Py_DECREF(restuple);
	return NULL;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &PyUnicode_Type,
	&resunicode, newpos)) {
	Py_DECREF(restuple);
	return NULL;
    }
    if (*newpos<0)
	*newpos = size+*newpos;
    if (*newpos<0 || *newpos>size) {
	PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", *newpos);
	Py_DECREF(restuple);
	return NULL;
    }
    Py_INCREF(resunicode);
    Py_DECREF(restuple);
    return resunicode;
}

static PyObject *unicode_encode_ucs1(const Py_UNICODE *p,
				 Py_ssize_t size,
				 const char *errors,
				 int limit)
{
    /* output object */
    PyObject *res;
    /* pointers to the beginning and end+1 of input */
    const Py_UNICODE *startp = p;
    const Py_UNICODE *endp = p + size;
    /* pointer to the beginning of the unencodable characters */
    /* const Py_UNICODE *badp = NULL; */
    /* pointer into the output */
    char *str;
    /* current output position */
    Py_ssize_t respos = 0;
    Py_ssize_t ressize;
    const char *encoding = (limit == 256) ? "latin-1" : "ascii";
    const char *reason = (limit == 256) ? "ordinal not in range(256)" : "ordinal not in range(128)";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace, 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    res = PyString_FromStringAndSize(NULL, size);
    if (res == NULL)
        goto onError;
    if (size == 0)
	return res;
    str = PyString_AS_STRING(res);
    ressize = size;

    while (p<endp) {
	Py_UNICODE c = *p;

	/* can we encode this? */
	if (c<limit) {
	    /* no overflow check, because we know that the space is enough */
	    *str++ = (char)c;
	    ++p;
	}
	else {
	    Py_ssize_t unicodepos = p-startp;
	    Py_ssize_t requiredsize;
	    PyObject *repunicode;
	    Py_ssize_t repsize;
	    Py_ssize_t newpos;
	    Py_ssize_t respos;
	    Py_UNICODE *uni2;
	    /* startpos for collecting unencodable chars */
	    const Py_UNICODE *collstart = p;
	    const Py_UNICODE *collend = p;
	    /* find all unecodable characters */
	    while ((collend < endp) && ((*collend)>=limit))
		++collend;
	    /* cache callback name lookup (if not done yet, i.e. it's the first error) */
	    if (known_errorHandler==-1) {
		if ((errors==NULL) || (!strcmp(errors, "strict")))
		    known_errorHandler = 1;
		else if (!strcmp(errors, "replace"))
		    known_errorHandler = 2;
		else if (!strcmp(errors, "ignore"))
		    known_errorHandler = 3;
		else if (!strcmp(errors, "xmlcharrefreplace"))
		    known_errorHandler = 4;
		else
		    known_errorHandler = 0;
	    }
	    switch (known_errorHandler) {
		case 1: /* strict */
		    raise_encode_exception(&exc, encoding, startp, size, collstart-startp, collend-startp, reason);
		    goto onError;
		case 2: /* replace */
		    while (collstart++<collend)
			*str++ = '?'; /* fall through */
		case 3: /* ignore */
		    p = collend;
		    break;
		case 4: /* xmlcharrefreplace */
		    respos = str-PyString_AS_STRING(res);
		    /* determine replacement size (temporarily (mis)uses p) */
		    for (p = collstart, repsize = 0; p < collend; ++p) {
			if (*p<10)
			    repsize += 2+1+1;
			else if (*p<100)
			    repsize += 2+2+1;
			else if (*p<1000)
			    repsize += 2+3+1;
			else if (*p<10000)
			    repsize += 2+4+1;
#ifndef Py_UNICODE_WIDE
			else
			    repsize += 2+5+1;
#else
			else if (*p<100000)
			    repsize += 2+5+1;
			else if (*p<1000000)
			    repsize += 2+6+1;
			else
			    repsize += 2+7+1;
#endif
		    }
		    requiredsize = respos+repsize+(endp-collend);
		    if (requiredsize > ressize) {
			if (requiredsize<2*ressize)
			    requiredsize = 2*ressize;
			if (_PyString_Resize(&res, requiredsize))
			    goto onError;
			str = PyString_AS_STRING(res) + respos;
			ressize = requiredsize;
		    }
		    /* generate replacement (temporarily (mis)uses p) */
		    for (p = collstart; p < collend; ++p) {
			str += sprintf(str, "&#%d;", (int)*p);
		    }
		    p = collend;
		    break;
		default:
		    repunicode = unicode_encode_call_errorhandler(errors, &errorHandler,
			encoding, reason, startp, size, &exc,
			collstart-startp, collend-startp, &newpos);
		    if (repunicode == NULL)
			goto onError;
		    /* need more space? (at least enough for what we
		       have+the replacement+the rest of the string, so
		       we won't have to check space for encodable characters) */
		    respos = str-PyString_AS_STRING(res);
		    repsize = PyUnicode_GET_SIZE(repunicode);
		    requiredsize = respos+repsize+(endp-collend);
		    if (requiredsize > ressize) {
			if (requiredsize<2*ressize)
			    requiredsize = 2*ressize;
			if (_PyString_Resize(&res, requiredsize)) {
			    Py_DECREF(repunicode);
			    goto onError;
			}
			str = PyString_AS_STRING(res) + respos;
			ressize = requiredsize;
		    }
		    /* check if there is anything unencodable in the replacement
		       and copy it to the output */
		    for (uni2 = PyUnicode_AS_UNICODE(repunicode);repsize-->0; ++uni2, ++str) {
			c = *uni2;
			if (c >= limit) {
			    raise_encode_exception(&exc, encoding, startp, size,
				unicodepos, unicodepos+1, reason);
			    Py_DECREF(repunicode);
			    goto onError;
			}
			*str = (char)c;
		    }
		    p = startp + newpos;
		    Py_DECREF(repunicode);
	    }
	}
    }
    /* Resize if we allocated to much */
    respos = str-PyString_AS_STRING(res);
    if (respos<ressize)
       /* If this falls res will be NULL */
	_PyString_Resize(&res, respos);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return res;

    onError:
    Py_XDECREF(res);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *PyUnicode_EncodeLatin1(const Py_UNICODE *p,
				 Py_ssize_t size,
				 const char *errors)
{
    return unicode_encode_ucs1(p, size, errors, 256);
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

PyObject *PyUnicode_DecodeASCII(const char *s,
				Py_ssize_t size,
				const char *errors)
{
    const char *starts = s;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;

    /* ASCII is equivalent to the first 128 ordinals in Unicode. */
    if (size == 1 && *(unsigned char*)s < 128) {
	Py_UNICODE r = *(unsigned char*)s;
	return PyUnicode_FromUnicode(&r, 1);
    }

    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    e = s + size;
    while (s < e) {
	register unsigned char c = (unsigned char)*s;
	if (c < 128) {
	    *p++ = c;
	    ++s;
	}
	else {
	    startinpos = s-starts;
	    endinpos = startinpos + 1;
	    outpos = p - (Py_UNICODE *)PyUnicode_AS_UNICODE(v);
	    if (unicode_decode_call_errorhandler(
		 errors, &errorHandler,
		 "ascii", "ordinal not in range(128)",
		 starts, size, &startinpos, &endinpos, &exc, &s,
		 (PyObject **)&v, &outpos, &p))
		goto onError;
	}
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyString_GET_SIZE(v))
	if (_PyUnicode_Resize(&v, p - PyUnicode_AS_UNICODE(v)) < 0)
	    goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)v;

 onError:
    Py_XDECREF(v);
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return NULL;
}

PyObject *PyUnicode_EncodeASCII(const Py_UNICODE *p,
				Py_ssize_t size,
				const char *errors)
{
    return unicode_encode_ucs1(p, size, errors, 128);
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

#if defined(MS_WINDOWS) && defined(HAVE_USABLE_WCHAR_T)

/* --- MBCS codecs for Windows -------------------------------------------- */

#if SIZEOF_INT < SIZEOF_SSIZE_T
#define NEED_RETRY
#endif

/* XXX This code is limited to "true" double-byte encodings, as
   a) it assumes an incomplete character consists of a single byte, and
   b) IsDBCSLeadByte (probably) does not work for non-DBCS multi-byte
      encodings, see IsDBCSLeadByteEx documentation. */

static int is_dbcs_lead_byte(const char *s, int offset)
{
    const char *curr = s + offset;

    if (IsDBCSLeadByte(*curr)) {
	const char *prev = CharPrev(s, curr);
	return (prev == curr) || !IsDBCSLeadByte(*prev) || (curr - prev == 2);
    }
    return 0;
}

/*
 * Decode MBCS string into unicode object. If 'final' is set, converts
 * trailing lead-byte too. Returns consumed size if succeed, -1 otherwise.
 */
static int decode_mbcs(PyUnicodeObject **v,
			const char *s, /* MBCS string */
			int size, /* sizeof MBCS string */
			int final)
{
    Py_UNICODE *p;
    Py_ssize_t n = 0;
    int usize = 0;

    assert(size >= 0);

    /* Skip trailing lead-byte unless 'final' is set */
    if (!final && size >= 1 && is_dbcs_lead_byte(s, size - 1))
	--size;

    /* First get the size of the result */
    if (size > 0) {
	usize = MultiByteToWideChar(CP_ACP, 0, s, size, NULL, 0);
	if (usize == 0) {
	    PyErr_SetFromWindowsErrWithFilename(0, NULL);
	    return -1;
	}
    }

    if (*v == NULL) {
	/* Create unicode object */
	*v = _PyUnicode_New(usize);
	if (*v == NULL)
	    return -1;
    }
    else {
	/* Extend unicode object */
	n = PyUnicode_GET_SIZE(*v);
	if (_PyUnicode_Resize(v, n + usize) < 0)
	    return -1;
    }

    /* Do the conversion */
    if (size > 0) {
	p = PyUnicode_AS_UNICODE(*v) + n;
	if (0 == MultiByteToWideChar(CP_ACP, 0, s, size, p, usize)) {
	    PyErr_SetFromWindowsErrWithFilename(0, NULL);
	    return -1;
	}
    }

    return size;
}

PyObject *PyUnicode_DecodeMBCSStateful(const char *s,
					Py_ssize_t size,
					const char *errors,
					Py_ssize_t *consumed)
{
    PyUnicodeObject *v = NULL;
    int done;

    if (consumed)
	*consumed = 0;

#ifdef NEED_RETRY
  retry:
    if (size > INT_MAX)
	done = decode_mbcs(&v, s, INT_MAX, 0);
    else
#endif
	done = decode_mbcs(&v, s, (int)size, !consumed);

    if (done < 0) {
        Py_XDECREF(v);
	return NULL;
    }

    if (consumed)
	*consumed += done;

#ifdef NEED_RETRY
    if (size > INT_MAX) {
	s += done;
	size -= done;
	goto retry;
    }
#endif

    return (PyObject *)v;
}

PyObject *PyUnicode_DecodeMBCS(const char *s,
				Py_ssize_t size,
				const char *errors)
{
    return PyUnicode_DecodeMBCSStateful(s, size, errors, NULL);
}

/*
 * Convert unicode into string object (MBCS).
 * Returns 0 if succeed, -1 otherwise.
 */
static int encode_mbcs(PyObject **repr,
			const Py_UNICODE *p, /* unicode */
			int size) /* size of unicode */
{
    int mbcssize = 0;
    Py_ssize_t n = 0;

    assert(size >= 0);

    /* First get the size of the result */
    if (size > 0) {
	mbcssize = WideCharToMultiByte(CP_ACP, 0, p, size, NULL, 0, NULL, NULL);
	if (mbcssize == 0) {
	    PyErr_SetFromWindowsErrWithFilename(0, NULL);
	    return -1;
	}
    }

    if (*repr == NULL) {
	/* Create string object */
	*repr = PyString_FromStringAndSize(NULL, mbcssize);
	if (*repr == NULL)
	    return -1;
    }
    else {
	/* Extend string object */
	n = PyString_Size(*repr);
	if (_PyString_Resize(repr, n + mbcssize) < 0)
	    return -1;
    }

    /* Do the conversion */
    if (size > 0) {
	char *s = PyString_AS_STRING(*repr) + n;
	if (0 == WideCharToMultiByte(CP_ACP, 0, p, size, s, mbcssize, NULL, NULL)) {
	    PyErr_SetFromWindowsErrWithFilename(0, NULL);
	    return -1;
	}
    }

    return 0;
}

PyObject *PyUnicode_EncodeMBCS(const Py_UNICODE *p,
				Py_ssize_t size,
				const char *errors)
{
    PyObject *repr = NULL;
    int ret;

#ifdef NEED_RETRY
 retry:
    if (size > INT_MAX)
	ret = encode_mbcs(&repr, p, INT_MAX);
    else
#endif
	ret = encode_mbcs(&repr, p, (int)size);

    if (ret < 0) {
	Py_XDECREF(repr);
	return NULL;
    }

#ifdef NEED_RETRY
    if (size > INT_MAX) {
	p += INT_MAX;
	size -= INT_MAX;
	goto retry;
    }
#endif

    return repr;
}

PyObject *PyUnicode_AsMBCSString(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return NULL;
    }
    return PyUnicode_EncodeMBCS(PyUnicode_AS_UNICODE(unicode),
				PyUnicode_GET_SIZE(unicode),
				NULL);
}

#undef NEED_RETRY

#endif /* MS_WINDOWS */

/* --- Character Mapping Codec -------------------------------------------- */

PyObject *PyUnicode_DecodeCharmap(const char *s,
				  Py_ssize_t size,
				  PyObject *mapping,
				  const char *errors)
{
    const char *starts = s;
    Py_ssize_t startinpos;
    Py_ssize_t endinpos;
    Py_ssize_t outpos;
    const char *e;
    PyUnicodeObject *v;
    Py_UNICODE *p;
    Py_ssize_t extrachars = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    Py_UNICODE *mapstring = NULL;
    Py_ssize_t maplen = 0;

    /* Default to Latin-1 */
    if (mapping == NULL)
	return PyUnicode_DecodeLatin1(s, size, errors);

    v = _PyUnicode_New(size);
    if (v == NULL)
	goto onError;
    if (size == 0)
	return (PyObject *)v;
    p = PyUnicode_AS_UNICODE(v);
    e = s + size;
    if (PyUnicode_CheckExact(mapping)) {
	mapstring = PyUnicode_AS_UNICODE(mapping);
	maplen = PyUnicode_GET_SIZE(mapping);
	while (s < e) {
	    unsigned char ch = *s;
	    Py_UNICODE x = 0xfffe; /* illegal value */

	    if (ch < maplen)
		x = mapstring[ch];

	    if (x == 0xfffe) {
		/* undefined mapping */
		outpos = p-PyUnicode_AS_UNICODE(v);
		startinpos = s-starts;
		endinpos = startinpos+1;
		if (unicode_decode_call_errorhandler(
		     errors, &errorHandler,
		     "charmap", "character maps to <undefined>",
		     starts, size, &startinpos, &endinpos, &exc, &s,
		     (PyObject **)&v, &outpos, &p)) {
		    goto onError;
		}
		continue;
	    }
	    *p++ = x;
	    ++s;
	}
    }
    else {
	while (s < e) {
	    unsigned char ch = *s;
	    PyObject *w, *x;

	    /* Get mapping (char ordinal -> integer, Unicode char or None) */
	    w = PyInt_FromLong((long)ch);
	    if (w == NULL)
		goto onError;
	    x = PyObject_GetItem(mapping, w);
	    Py_DECREF(w);
	    if (x == NULL) {
		if (PyErr_ExceptionMatches(PyExc_LookupError)) {
		    /* No mapping found means: mapping is undefined. */
		    PyErr_Clear();
		    x = Py_None;
		    Py_INCREF(x);
		} else
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
		outpos = p-PyUnicode_AS_UNICODE(v);
		startinpos = s-starts;
		endinpos = startinpos+1;
		if (unicode_decode_call_errorhandler(
		     errors, &errorHandler,
		     "charmap", "character maps to <undefined>",
		     starts, size, &startinpos, &endinpos, &exc, &s,
		     (PyObject **)&v, &outpos, &p)) {
		    Py_DECREF(x);
		    goto onError;
		}
		Py_DECREF(x);
		continue;
	    }
	    else if (PyUnicode_Check(x)) {
		Py_ssize_t targetsize = PyUnicode_GET_SIZE(x);
    
		if (targetsize == 1)
		    /* 1-1 mapping */
		    *p++ = *PyUnicode_AS_UNICODE(x);
    
		else if (targetsize > 1) {
		    /* 1-n mapping */
		    if (targetsize > extrachars) {
			/* resize first */
			Py_ssize_t oldpos = p - PyUnicode_AS_UNICODE(v);
			Py_ssize_t needed = (targetsize - extrachars) + \
				     (targetsize << 2);
			extrachars += needed;
			/* XXX overflow detection missing */
			if (_PyUnicode_Resize(&v,
					     PyUnicode_GET_SIZE(v) + needed) < 0) {
			    Py_DECREF(x);
			    goto onError;
			}
			p = PyUnicode_AS_UNICODE(v) + oldpos;
		    }
		    Py_UNICODE_COPY(p,
				    PyUnicode_AS_UNICODE(x),
				    targetsize);
		    p += targetsize;
		    extrachars -= targetsize;
		}
		/* 1-0 mapping: skip the character */
	    }
	    else {
		/* wrong return value */
		PyErr_SetString(PyExc_TypeError,
		      "character mapping must return integer, None or unicode");
		Py_DECREF(x);
		goto onError;
	    }
	    Py_DECREF(x);
	    ++s;
	}
    }
    if (p - PyUnicode_AS_UNICODE(v) < PyUnicode_GET_SIZE(v))
	if (_PyUnicode_Resize(&v, p - PyUnicode_AS_UNICODE(v)) < 0)
	    goto onError;
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    return (PyObject *)v;

 onError:
    Py_XDECREF(errorHandler);
    Py_XDECREF(exc);
    Py_XDECREF(v);
    return NULL;
}

/* Charmap encoding: the lookup table */

struct encoding_map{
  PyObject_HEAD
  unsigned char level1[32];
  int count2, count3;
  unsigned char level23[1];
};

static PyObject*
encoding_map_size(PyObject *obj, PyObject* args)
{
    struct encoding_map *map = (struct encoding_map*)obj;
    return PyInt_FromLong(sizeof(*map) - 1 + 16*map->count2 + 
                          128*map->count3);
}

static PyMethodDef encoding_map_methods[] = {
	{"size", encoding_map_size, METH_NOARGS, 
         PyDoc_STR("Return the size (in bytes) of this object") },
        { 0 }
};

static void
encoding_map_dealloc(PyObject* o)
{
	PyObject_FREE(o);
}

static PyTypeObject EncodingMapType = {
	PyVarObject_HEAD_INIT(NULL, 0)
        "EncodingMap",          /*tp_name*/
        sizeof(struct encoding_map),   /*tp_basicsize*/
        0,                      /*tp_itemsize*/
        /* methods */
        encoding_map_dealloc,   /*tp_dealloc*/
        0,                      /*tp_print*/
        0,                      /*tp_getattr*/
        0,                      /*tp_setattr*/
        0,                      /*tp_compare*/
        0,                      /*tp_repr*/
        0,                      /*tp_as_number*/
        0,                      /*tp_as_sequence*/
        0,                      /*tp_as_mapping*/
        0,                      /*tp_hash*/
        0,                      /*tp_call*/
        0,                      /*tp_str*/
        0,                      /*tp_getattro*/
        0,                      /*tp_setattro*/
        0,                      /*tp_as_buffer*/
        Py_TPFLAGS_DEFAULT,     /*tp_flags*/
        0,                      /*tp_doc*/
        0,                      /*tp_traverse*/
        0,                      /*tp_clear*/
        0,                      /*tp_richcompare*/
        0,                      /*tp_weaklistoffset*/
        0,                      /*tp_iter*/
        0,                      /*tp_iternext*/
        encoding_map_methods,   /*tp_methods*/
        0,                      /*tp_members*/
        0,                      /*tp_getset*/
        0,                      /*tp_base*/
        0,                      /*tp_dict*/
        0,                      /*tp_descr_get*/
        0,                      /*tp_descr_set*/
        0,                      /*tp_dictoffset*/
        0,                      /*tp_init*/
        0,                      /*tp_alloc*/
        0,                      /*tp_new*/
        0,                      /*tp_free*/
        0,                      /*tp_is_gc*/
};

PyObject*
PyUnicode_BuildEncodingMap(PyObject* string)
{
    Py_UNICODE *decode;
    PyObject *result;
    struct encoding_map *mresult;
    int i;
    int need_dict = 0;
    unsigned char level1[32];
    unsigned char level2[512];
    unsigned char *mlevel1, *mlevel2, *mlevel3;
    int count2 = 0, count3 = 0;

    if (!PyUnicode_Check(string) || PyUnicode_GetSize(string) != 256) {
        PyErr_BadArgument();
        return NULL;
    }
    decode = PyUnicode_AS_UNICODE(string);
    memset(level1, 0xFF, sizeof level1);
    memset(level2, 0xFF, sizeof level2);

    /* If there isn't a one-to-one mapping of NULL to \0,
       or if there are non-BMP characters, we need to use
       a mapping dictionary. */
    if (decode[0] != 0)
        need_dict = 1;
    for (i = 1; i < 256; i++) {
        int l1, l2;
        if (decode[i] == 0
            #ifdef Py_UNICODE_WIDE
            || decode[i] > 0xFFFF
            #endif
        ) {
            need_dict = 1;
            break;
        }
        if (decode[i] == 0xFFFE)
            /* unmapped character */
            continue;
        l1 = decode[i] >> 11;
        l2 = decode[i] >> 7;
        if (level1[l1] == 0xFF)
            level1[l1] = count2++;
        if (level2[l2] == 0xFF)
            level2[l2] = count3++; 
    }

    if (count2 >= 0xFF || count3 >= 0xFF)
        need_dict = 1;

    if (need_dict) {
        PyObject *result = PyDict_New();
        PyObject *key, *value;
        if (!result)
            return NULL;
        for (i = 0; i < 256; i++) {
            key = value = NULL;
            key = PyInt_FromLong(decode[i]);
            value = PyInt_FromLong(i);
            if (!key || !value)
                goto failed1;
            if (PyDict_SetItem(result, key, value) == -1)
                goto failed1;
            Py_DECREF(key);
            Py_DECREF(value);
        }
        return result;
      failed1:
        Py_XDECREF(key);
        Py_XDECREF(value);
        Py_DECREF(result);
        return NULL;
    }

    /* Create a three-level trie */
    result = PyObject_MALLOC(sizeof(struct encoding_map) +
                             16*count2 + 128*count3 - 1);
    if (!result)
        return PyErr_NoMemory();
    PyObject_Init(result, &EncodingMapType);
    mresult = (struct encoding_map*)result;
    mresult->count2 = count2;
    mresult->count3 = count3;
    mlevel1 = mresult->level1;
    mlevel2 = mresult->level23;
    mlevel3 = mresult->level23 + 16*count2;
    memcpy(mlevel1, level1, 32);
    memset(mlevel2, 0xFF, 16*count2);
    memset(mlevel3, 0, 128*count3);
    count3 = 0;
    for (i = 1; i < 256; i++) {
        int o1, o2, o3, i2, i3;
        if (decode[i] == 0xFFFE)
            /* unmapped character */
            continue;
        o1 = decode[i]>>11;
        o2 = (decode[i]>>7) & 0xF;
        i2 = 16*mlevel1[o1] + o2;
        if (mlevel2[i2] == 0xFF)
            mlevel2[i2] = count3++;
        o3 = decode[i] & 0x7F;
        i3 = 128*mlevel2[i2] + o3;
        mlevel3[i3] = i;
    }
    return result;
}

static int
encoding_map_lookup(Py_UNICODE c, PyObject *mapping)
{
    struct encoding_map *map = (struct encoding_map*)mapping;
    int l1 = c>>11;
    int l2 = (c>>7) & 0xF;
    int l3 = c & 0x7F;
    int i;

#ifdef Py_UNICODE_WIDE
    if (c > 0xFFFF) {
	return -1;
    }
#endif
    if (c == 0)
        return 0;
    /* level 1*/
    i = map->level1[l1];
    if (i == 0xFF) {
        return -1;
    }
    /* level 2*/
    i = map->level23[16*i+l2];
    if (i == 0xFF) {
        return -1;
    }
    /* level 3 */
    i = map->level23[16*map->count2 + 128*i + l3];
    if (i == 0) {
        return -1;
    }
    return i;
}

/* Lookup the character ch in the mapping. If the character
   can't be found, Py_None is returned (or NULL, if another
   error occurred). */
static PyObject *charmapencode_lookup(Py_UNICODE c, PyObject *mapping)
{
    PyObject *w = PyInt_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
	 return NULL;
    x = PyObject_GetItem(mapping, w);
    Py_DECREF(w);
    if (x == NULL) {
	if (PyErr_ExceptionMatches(PyExc_LookupError)) {
	    /* No mapping found means: mapping is undefined. */
	    PyErr_Clear();
	    x = Py_None;
	    Py_INCREF(x);
	    return x;
	} else
	    return NULL;
    }
    else if (x == Py_None)
	return x;
    else if (PyInt_Check(x)) {
	long value = PyInt_AS_LONG(x);
	if (value < 0 || value > 255) {
	    PyErr_SetString(PyExc_TypeError,
			     "character mapping must be in range(256)");
	    Py_DECREF(x);
	    return NULL;
	}
	return x;
    }
    else if (PyString_Check(x))
	return x;
    else {
	/* wrong return value */
	PyErr_SetString(PyExc_TypeError,
	      "character mapping must return integer, None or str");
	Py_DECREF(x);
	return NULL;
    }
}

static int
charmapencode_resize(PyObject **outobj, Py_ssize_t *outpos, Py_ssize_t requiredsize)
{
	Py_ssize_t outsize = PyString_GET_SIZE(*outobj);
	/* exponentially overallocate to minimize reallocations */
	if (requiredsize < 2*outsize)
	    requiredsize = 2*outsize;
	if (_PyString_Resize(outobj, requiredsize)) {
	    return 0;
	}
	return 1;
}

typedef enum charmapencode_result { 
  enc_SUCCESS, enc_FAILED, enc_EXCEPTION 
}charmapencode_result;
/* lookup the character, put the result in the output string and adjust
   various state variables. Reallocate the output string if not enough
   space is available. Return a new reference to the object that
   was put in the output buffer, or Py_None, if the mapping was undefined
   (in which case no character was written) or NULL, if a
   reallocation error occurred. The caller must decref the result */
static
charmapencode_result charmapencode_output(Py_UNICODE c, PyObject *mapping,
    PyObject **outobj, Py_ssize_t *outpos)
{
    PyObject *rep;
    char *outstart;
    Py_ssize_t outsize = PyString_GET_SIZE(*outobj);

    if (Py_TYPE(mapping) == &EncodingMapType) {
        int res = encoding_map_lookup(c, mapping);
	Py_ssize_t requiredsize = *outpos+1;
        if (res == -1)
            return enc_FAILED;
	if (outsize<requiredsize) 
	    if (!charmapencode_resize(outobj, outpos, requiredsize))
		return enc_EXCEPTION;
        outstart = PyString_AS_STRING(*outobj);
	outstart[(*outpos)++] = (char)res;
	return enc_SUCCESS;
    }

    rep = charmapencode_lookup(c, mapping);
    if (rep==NULL)
	return enc_EXCEPTION;
    else if (rep==Py_None) {
	Py_DECREF(rep);
	return enc_FAILED;
    } else {
	if (PyInt_Check(rep)) {
	    Py_ssize_t requiredsize = *outpos+1;
	    if (outsize<requiredsize)
		if (!charmapencode_resize(outobj, outpos, requiredsize)) {
		    Py_DECREF(rep);
		    return enc_EXCEPTION;
		}
            outstart = PyString_AS_STRING(*outobj);
	    outstart[(*outpos)++] = (char)PyInt_AS_LONG(rep);
	}
	else {
	    const char *repchars = PyString_AS_STRING(rep);
	    Py_ssize_t repsize = PyString_GET_SIZE(rep);
	    Py_ssize_t requiredsize = *outpos+repsize;
	    if (outsize<requiredsize)
		if (!charmapencode_resize(outobj, outpos, requiredsize)) {
		    Py_DECREF(rep);
		    return enc_EXCEPTION;
		}
            outstart = PyString_AS_STRING(*outobj);
	    memcpy(outstart + *outpos, repchars, repsize);
	    *outpos += repsize;
	}
    }
    Py_DECREF(rep);
    return enc_SUCCESS;
}

/* handle an error in PyUnicode_EncodeCharmap
   Return 0 on success, -1 on error */
static
int charmap_encoding_error(
    const Py_UNICODE *p, Py_ssize_t size, Py_ssize_t *inpos, PyObject *mapping,
    PyObject **exceptionObject,
    int *known_errorHandler, PyObject **errorHandler, const char *errors,
    PyObject **res, Py_ssize_t *respos)
{
    PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
    Py_ssize_t repsize;
    Py_ssize_t newpos;
    Py_UNICODE *uni2;
    /* startpos for collecting unencodable chars */
    Py_ssize_t collstartpos = *inpos;
    Py_ssize_t collendpos = *inpos+1;
    Py_ssize_t collpos;
    char *encoding = "charmap";
    char *reason = "character maps to <undefined>";
    charmapencode_result x;

    /* find all unencodable characters */
    while (collendpos < size) {
        PyObject *rep;
        if (Py_TYPE(mapping) == &EncodingMapType) {
	    int res = encoding_map_lookup(p[collendpos], mapping);
	    if (res != -1)
		break;
	    ++collendpos;
	    continue;
	}
            
	rep = charmapencode_lookup(p[collendpos], mapping);
	if (rep==NULL)
	    return -1;
	else if (rep!=Py_None) {
	    Py_DECREF(rep);
	    break;
	}
	Py_DECREF(rep);
	++collendpos;
    }
    /* cache callback name lookup
     * (if not done yet, i.e. it's the first error) */
    if (*known_errorHandler==-1) {
	if ((errors==NULL) || (!strcmp(errors, "strict")))
	    *known_errorHandler = 1;
	else if (!strcmp(errors, "replace"))
	    *known_errorHandler = 2;
	else if (!strcmp(errors, "ignore"))
	    *known_errorHandler = 3;
	else if (!strcmp(errors, "xmlcharrefreplace"))
	    *known_errorHandler = 4;
	else
	    *known_errorHandler = 0;
    }
    switch (*known_errorHandler) {
	case 1: /* strict */
	    raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
	    return -1;
	case 2: /* replace */
	    for (collpos = collstartpos; collpos<collendpos; ++collpos) {
		x = charmapencode_output('?', mapping, res, respos);
		if (x==enc_EXCEPTION) {
		    return -1;
		}
		else if (x==enc_FAILED) {
		    raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
		    return -1;
		}
	    }
	    /* fall through */
	case 3: /* ignore */
	    *inpos = collendpos;
	    break;
	case 4: /* xmlcharrefreplace */
	    /* generate replacement (temporarily (mis)uses p) */
	    for (collpos = collstartpos; collpos < collendpos; ++collpos) {
		char buffer[2+29+1+1];
		char *cp;
		sprintf(buffer, "&#%d;", (int)p[collpos]);
		for (cp = buffer; *cp; ++cp) {
		    x = charmapencode_output(*cp, mapping, res, respos);
		    if (x==enc_EXCEPTION)
			return -1;
		    else if (x==enc_FAILED) {
			raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
			return -1;
		    }
		}
	    }
	    *inpos = collendpos;
	    break;
	default:
	    repunicode = unicode_encode_call_errorhandler(errors, errorHandler,
		encoding, reason, p, size, exceptionObject,
		collstartpos, collendpos, &newpos);
	    if (repunicode == NULL)
		return -1;
	    /* generate replacement  */
	    repsize = PyUnicode_GET_SIZE(repunicode);
	    for (uni2 = PyUnicode_AS_UNICODE(repunicode); repsize-->0; ++uni2) {
		x = charmapencode_output(*uni2, mapping, res, respos);
		if (x==enc_EXCEPTION) {
		    return -1;
		}
		else if (x==enc_FAILED) {
		    Py_DECREF(repunicode);
		    raise_encode_exception(exceptionObject, encoding, p, size, collstartpos, collendpos, reason);
		    return -1;
		}
	    }
	    *inpos = newpos;
	    Py_DECREF(repunicode);
    }
    return 0;
}

PyObject *PyUnicode_EncodeCharmap(const Py_UNICODE *p,
				  Py_ssize_t size,
				  PyObject *mapping,
				  const char *errors)
{
    /* output object */
    PyObject *res = NULL;
    /* current input position */
    Py_ssize_t inpos = 0;
    /* current output position */
    Py_ssize_t respos = 0;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace,
     * 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    /* Default to Latin-1 */
    if (mapping == NULL)
	return PyUnicode_EncodeLatin1(p, size, errors);

    /* allocate enough for a simple encoding without
       replacements, if we need more, we'll resize */
    res = PyString_FromStringAndSize(NULL, size);
    if (res == NULL)
        goto onError;
    if (size == 0)
	return res;

    while (inpos<size) {
	/* try to encode it */
	charmapencode_result x = charmapencode_output(p[inpos], mapping, &res, &respos);
	if (x==enc_EXCEPTION) /* error */
	    goto onError;
	if (x==enc_FAILED) { /* unencodable character */
	    if (charmap_encoding_error(p, size, &inpos, mapping,
		&exc,
		&known_errorHandler, &errorHandler, errors,
		&res, &respos)) {
		goto onError;
	    }
	}
	else
	    /* done with this character => adjust input position */
	    ++inpos;
    }

    /* Resize if we allocated to much */
    if (respos<PyString_GET_SIZE(res)) {
	if (_PyString_Resize(&res, respos))
	    goto onError;
    }
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return res;

    onError:
    Py_XDECREF(res);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
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

/* create or adjust a UnicodeTranslateError */
static void make_translate_exception(PyObject **exceptionObject,
    const Py_UNICODE *unicode, Py_ssize_t size,
    Py_ssize_t startpos, Py_ssize_t endpos,
    const char *reason)
{
    if (*exceptionObject == NULL) {
    	*exceptionObject = PyUnicodeTranslateError_Create(
	    unicode, size, startpos, endpos, reason);
    }
    else {
	if (PyUnicodeTranslateError_SetStart(*exceptionObject, startpos))
	    goto onError;
	if (PyUnicodeTranslateError_SetEnd(*exceptionObject, endpos))
	    goto onError;
	if (PyUnicodeTranslateError_SetReason(*exceptionObject, reason))
	    goto onError;
	return;
	onError:
	Py_DECREF(*exceptionObject);
	*exceptionObject = NULL;
    }
}

/* raises a UnicodeTranslateError */
static void raise_translate_exception(PyObject **exceptionObject,
    const Py_UNICODE *unicode, Py_ssize_t size,
    Py_ssize_t startpos, Py_ssize_t endpos,
    const char *reason)
{
    make_translate_exception(exceptionObject,
	unicode, size, startpos, endpos, reason);
    if (*exceptionObject != NULL)
	PyCodec_StrictErrors(*exceptionObject);
}

/* error handling callback helper:
   build arguments, call the callback and check the arguments,
   put the result into newpos and return the replacement string, which
   has to be freed by the caller */
static PyObject *unicode_translate_call_errorhandler(const char *errors,
    PyObject **errorHandler,
    const char *reason,
    const Py_UNICODE *unicode, Py_ssize_t size, PyObject **exceptionObject,
    Py_ssize_t startpos, Py_ssize_t endpos,
    Py_ssize_t *newpos)
{
    static char *argparse = "O!n;translating error handler must return (unicode, int) tuple";

    Py_ssize_t i_newpos;
    PyObject *restuple;
    PyObject *resunicode;

    if (*errorHandler == NULL) {
	*errorHandler = PyCodec_LookupError(errors);
        if (*errorHandler == NULL)
	    return NULL;
    }

    make_translate_exception(exceptionObject,
	unicode, size, startpos, endpos, reason);
    if (*exceptionObject == NULL)
	return NULL;

    restuple = PyObject_CallFunctionObjArgs(
	*errorHandler, *exceptionObject, NULL);
    if (restuple == NULL)
	return NULL;
    if (!PyTuple_Check(restuple)) {
	PyErr_Format(PyExc_TypeError, &argparse[4]);
	Py_DECREF(restuple);
	return NULL;
    }
    if (!PyArg_ParseTuple(restuple, argparse, &PyUnicode_Type,
	&resunicode, &i_newpos)) {
	Py_DECREF(restuple);
	return NULL;
    }
    if (i_newpos<0)
	*newpos = size+i_newpos;
    else
        *newpos = i_newpos;
    if (*newpos<0 || *newpos>size) {
	PyErr_Format(PyExc_IndexError, "position %zd from error handler out of bounds", *newpos);
	Py_DECREF(restuple);
	return NULL;
    }
    Py_INCREF(resunicode);
    Py_DECREF(restuple);
    return resunicode;
}

/* Lookup the character ch in the mapping and put the result in result,
   which must be decrefed by the caller.
   Return 0 on success, -1 on error */
static
int charmaptranslate_lookup(Py_UNICODE c, PyObject *mapping, PyObject **result)
{
    PyObject *w = PyInt_FromLong((long)c);
    PyObject *x;

    if (w == NULL)
	 return -1;
    x = PyObject_GetItem(mapping, w);
    Py_DECREF(w);
    if (x == NULL) {
	if (PyErr_ExceptionMatches(PyExc_LookupError)) {
	    /* No mapping found means: use 1:1 mapping. */
	    PyErr_Clear();
	    *result = NULL;
	    return 0;
	} else
	    return -1;
    }
    else if (x == Py_None) {
	*result = x;
	return 0;
    }
    else if (PyInt_Check(x)) {
	long value = PyInt_AS_LONG(x);
	long max = PyUnicode_GetMax();
	if (value < 0 || value > max) {
	    PyErr_Format(PyExc_TypeError,
			     "character mapping must be in range(0x%lx)", max+1);
	    Py_DECREF(x);
	    return -1;
	}
	*result = x;
	return 0;
    }
    else if (PyUnicode_Check(x)) {
	*result = x;
	return 0;
    }
    else {
	/* wrong return value */
	PyErr_SetString(PyExc_TypeError,
	      "character mapping must return integer, None or unicode");
	Py_DECREF(x);
	return -1;
    }
}
/* ensure that *outobj is at least requiredsize characters long,
if not reallocate and adjust various state variables.
Return 0 on success, -1 on error */
static
int charmaptranslate_makespace(PyObject **outobj, Py_UNICODE **outp,
    Py_ssize_t requiredsize)
{
    Py_ssize_t oldsize = PyUnicode_GET_SIZE(*outobj);
    if (requiredsize > oldsize) {
	/* remember old output position */
	Py_ssize_t outpos = *outp-PyUnicode_AS_UNICODE(*outobj);
	/* exponentially overallocate to minimize reallocations */
	if (requiredsize < 2 * oldsize)
	    requiredsize = 2 * oldsize;
	if (_PyUnicode_Resize(outobj, requiredsize) < 0)
	    return -1;
	*outp = PyUnicode_AS_UNICODE(*outobj) + outpos;
    }
    return 0;
}
/* lookup the character, put the result in the output string and adjust
   various state variables. Return a new reference to the object that
   was put in the output buffer in *result, or Py_None, if the mapping was
   undefined (in which case no character was written).
   The called must decref result.
   Return 0 on success, -1 on error. */
static
int charmaptranslate_output(const Py_UNICODE *startinp, const Py_UNICODE *curinp,
    Py_ssize_t insize, PyObject *mapping, PyObject **outobj, Py_UNICODE **outp,
    PyObject **res)
{
    if (charmaptranslate_lookup(*curinp, mapping, res))
	return -1;
    if (*res==NULL) {
	/* not found => default to 1:1 mapping */
	*(*outp)++ = *curinp;
    }
    else if (*res==Py_None)
	;
    else if (PyInt_Check(*res)) {
	/* no overflow check, because we know that the space is enough */
	*(*outp)++ = (Py_UNICODE)PyInt_AS_LONG(*res);
    }
    else if (PyUnicode_Check(*res)) {
	Py_ssize_t repsize = PyUnicode_GET_SIZE(*res);
	if (repsize==1) {
	    /* no overflow check, because we know that the space is enough */
	    *(*outp)++ = *PyUnicode_AS_UNICODE(*res);
	}
	else if (repsize!=0) {
	    /* more than one character */
	    Py_ssize_t requiredsize = (*outp-PyUnicode_AS_UNICODE(*outobj)) +
		(insize - (curinp-startinp)) +
		repsize - 1;
	    if (charmaptranslate_makespace(outobj, outp, requiredsize))
		return -1;
	    memcpy(*outp, PyUnicode_AS_UNICODE(*res), sizeof(Py_UNICODE)*repsize);
	    *outp += repsize;
	}
    }
    else
	return -1;
    return 0;
}

PyObject *PyUnicode_TranslateCharmap(const Py_UNICODE *p,
				     Py_ssize_t size,
				     PyObject *mapping,
				     const char *errors)
{
    /* output object */
    PyObject *res = NULL;
    /* pointers to the beginning and end+1 of input */
    const Py_UNICODE *startp = p;
    const Py_UNICODE *endp = p + size;
    /* pointer into the output */
    Py_UNICODE *str;
    /* current output position */
    Py_ssize_t respos = 0;
    char *reason = "character maps to <undefined>";
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace,
     * 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    if (mapping == NULL) {
	PyErr_BadArgument();
	return NULL;
    }

    /* allocate enough for a simple 1:1 translation without
       replacements, if we need more, we'll resize */
    res = PyUnicode_FromUnicode(NULL, size);
    if (res == NULL)
	goto onError;
    if (size == 0)
	return res;
    str = PyUnicode_AS_UNICODE(res);

    while (p<endp) {
	/* try to encode it */
	PyObject *x = NULL;
	if (charmaptranslate_output(startp, p, size, mapping, &res, &str, &x)) {
	    Py_XDECREF(x);
	    goto onError;
	}
	Py_XDECREF(x);
	if (x!=Py_None) /* it worked => adjust input pointer */
	    ++p;
	else { /* untranslatable character */
	    PyObject *repunicode = NULL; /* initialize to prevent gcc warning */
	    Py_ssize_t repsize;
	    Py_ssize_t newpos;
	    Py_UNICODE *uni2;
	    /* startpos for collecting untranslatable chars */
	    const Py_UNICODE *collstart = p;
	    const Py_UNICODE *collend = p+1;
	    const Py_UNICODE *coll;

	    /* find all untranslatable characters */
	    while (collend < endp) {
		if (charmaptranslate_lookup(*collend, mapping, &x))
		    goto onError;
		Py_XDECREF(x);
		if (x!=Py_None)
		    break;
		++collend;
	    }
	    /* cache callback name lookup
	     * (if not done yet, i.e. it's the first error) */
	    if (known_errorHandler==-1) {
		if ((errors==NULL) || (!strcmp(errors, "strict")))
		    known_errorHandler = 1;
		else if (!strcmp(errors, "replace"))
		    known_errorHandler = 2;
		else if (!strcmp(errors, "ignore"))
		    known_errorHandler = 3;
		else if (!strcmp(errors, "xmlcharrefreplace"))
		    known_errorHandler = 4;
		else
		    known_errorHandler = 0;
	    }
	    switch (known_errorHandler) {
		case 1: /* strict */
		    raise_translate_exception(&exc, startp, size, collstart-startp, collend-startp, reason);
		    goto onError;
		case 2: /* replace */
		    /* No need to check for space, this is a 1:1 replacement */
		    for (coll = collstart; coll<collend; ++coll)
			*str++ = '?';
		    /* fall through */
		case 3: /* ignore */
		    p = collend;
		    break;
		case 4: /* xmlcharrefreplace */
		    /* generate replacement (temporarily (mis)uses p) */
		    for (p = collstart; p < collend; ++p) {
			char buffer[2+29+1+1];
			char *cp;
			sprintf(buffer, "&#%d;", (int)*p);
			if (charmaptranslate_makespace(&res, &str,
			    (str-PyUnicode_AS_UNICODE(res))+strlen(buffer)+(endp-collend)))
			    goto onError;
			for (cp = buffer; *cp; ++cp)
			    *str++ = *cp;
		    }
		    p = collend;
		    break;
		default:
		    repunicode = unicode_translate_call_errorhandler(errors, &errorHandler,
			reason, startp, size, &exc,
			collstart-startp, collend-startp, &newpos);
		    if (repunicode == NULL)
			goto onError;
		    /* generate replacement  */
		    repsize = PyUnicode_GET_SIZE(repunicode);
		    if (charmaptranslate_makespace(&res, &str,
			(str-PyUnicode_AS_UNICODE(res))+repsize+(endp-collend))) {
			Py_DECREF(repunicode);
			goto onError;
		    }
		    for (uni2 = PyUnicode_AS_UNICODE(repunicode); repsize-->0; ++uni2)
			*str++ = *uni2;
		    p = startp + newpos;
		    Py_DECREF(repunicode);
	    }
	}
    }
    /* Resize if we allocated to much */
    respos = str-PyUnicode_AS_UNICODE(res);
    if (respos<PyUnicode_GET_SIZE(res)) {
	if (_PyUnicode_Resize(&res, respos) < 0)
	    goto onError;
    }
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return res;

    onError:
    Py_XDECREF(res);
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
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
			    Py_ssize_t length,
			    char *output,
			    const char *errors)
{
    Py_UNICODE *p, *end;
    PyObject *errorHandler = NULL;
    PyObject *exc = NULL;
    const char *encoding = "decimal";
    const char *reason = "invalid decimal Unicode string";
    /* the following variable is used for caching string comparisons
     * -1=not initialized, 0=unknown, 1=strict, 2=replace, 3=ignore, 4=xmlcharrefreplace */
    int known_errorHandler = -1;

    if (output == NULL) {
	PyErr_BadArgument();
	return -1;
    }

    p = s;
    end = s + length;
    while (p < end) {
	register Py_UNICODE ch = *p;
	int decimal;
	PyObject *repunicode;
	Py_ssize_t repsize;
	Py_ssize_t newpos;
	Py_UNICODE *uni2;
	Py_UNICODE *collstart;
	Py_UNICODE *collend;

	if (Py_UNICODE_ISSPACE(ch)) {
	    *output++ = ' ';
	    ++p;
	    continue;
	}
	decimal = Py_UNICODE_TODECIMAL(ch);
	if (decimal >= 0) {
	    *output++ = '0' + decimal;
	    ++p;
	    continue;
	}
	if (0 < ch && ch < 256) {
	    *output++ = (char)ch;
	    ++p;
	    continue;
	}
	/* All other characters are considered unencodable */
	collstart = p;
	collend = p+1;
	while (collend < end) {
	    if ((0 < *collend && *collend < 256) ||
	        !Py_UNICODE_ISSPACE(*collend) ||
	        Py_UNICODE_TODECIMAL(*collend))
		break;
	}
	/* cache callback name lookup
	 * (if not done yet, i.e. it's the first error) */
	if (known_errorHandler==-1) {
	    if ((errors==NULL) || (!strcmp(errors, "strict")))
		known_errorHandler = 1;
	    else if (!strcmp(errors, "replace"))
		known_errorHandler = 2;
	    else if (!strcmp(errors, "ignore"))
		known_errorHandler = 3;
	    else if (!strcmp(errors, "xmlcharrefreplace"))
		known_errorHandler = 4;
	    else
		known_errorHandler = 0;
	}
	switch (known_errorHandler) {
	    case 1: /* strict */
		raise_encode_exception(&exc, encoding, s, length, collstart-s, collend-s, reason);
		goto onError;
	    case 2: /* replace */
		for (p = collstart; p < collend; ++p)
		    *output++ = '?';
		/* fall through */
	    case 3: /* ignore */
		p = collend;
		break;
	    case 4: /* xmlcharrefreplace */
		/* generate replacement (temporarily (mis)uses p) */
		for (p = collstart; p < collend; ++p)
		    output += sprintf(output, "&#%d;", (int)*p);
		p = collend;
		break;
	    default:
		repunicode = unicode_encode_call_errorhandler(errors, &errorHandler,
		    encoding, reason, s, length, &exc,
		    collstart-s, collend-s, &newpos);
		if (repunicode == NULL)
		    goto onError;
		/* generate replacement  */
		repsize = PyUnicode_GET_SIZE(repunicode);
		for (uni2 = PyUnicode_AS_UNICODE(repunicode); repsize-->0; ++uni2) {
		    Py_UNICODE ch = *uni2;
		    if (Py_UNICODE_ISSPACE(ch))
			*output++ = ' ';
		    else {
			decimal = Py_UNICODE_TODECIMAL(ch);
			if (decimal >= 0)
			    *output++ = '0' + decimal;
			else if (0 < ch && ch < 256)
			    *output++ = (char)ch;
			else {
			    Py_DECREF(repunicode);
			    raise_encode_exception(&exc, encoding,
				s, length, collstart-s, collend-s, reason);
			    goto onError;
			}
		    }
		}
		p = s + newpos;
		Py_DECREF(repunicode);
	}
    }
    /* 0-terminate the output string */
    *output++ = '\0';
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return 0;

 onError:
    Py_XDECREF(exc);
    Py_XDECREF(errorHandler);
    return -1;
}

/* --- Helpers ------------------------------------------------------------ */

#include "stringlib/unicodedefs.h"

#define FROM_UNICODE

#include "stringlib/fastsearch.h"

#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/partition.h"

/* helper macro to fixup start/end slice values */
#define FIX_START_END(obj)                      \
    if (start < 0)                              \
        start += (obj)->length;                 \
    if (start < 0)                              \
        start = 0;                              \
    if (end > (obj)->length)                    \
        end = (obj)->length;                    \
    if (end < 0)                                \
        end += (obj)->length;                   \
    if (end < 0)                                \
        end = 0;

Py_ssize_t PyUnicode_Count(PyObject *str,
                           PyObject *substr,
                           Py_ssize_t start,
                           Py_ssize_t end)
{
    Py_ssize_t result;
    PyUnicodeObject* str_obj;
    PyUnicodeObject* sub_obj;

    str_obj = (PyUnicodeObject*) PyUnicode_FromObject(str);
    if (!str_obj)
	return -1;
    sub_obj = (PyUnicodeObject*) PyUnicode_FromObject(substr);
    if (!sub_obj) {
	Py_DECREF(str_obj);
	return -1;
    }

    FIX_START_END(str_obj);

    result = stringlib_count(
        str_obj->str + start, end - start, sub_obj->str, sub_obj->length
        );

    Py_DECREF(sub_obj);
    Py_DECREF(str_obj);

    return result;
}

Py_ssize_t PyUnicode_Find(PyObject *str,
                          PyObject *sub,
                          Py_ssize_t start,
                          Py_ssize_t end,
                          int direction)
{
    Py_ssize_t result;

    str = PyUnicode_FromObject(str);
    if (!str)
	return -2;
    sub = PyUnicode_FromObject(sub);
    if (!sub) {
	Py_DECREF(str);
	return -2;
    }

    if (direction > 0)
        result = stringlib_find_slice(
            PyUnicode_AS_UNICODE(str), PyUnicode_GET_SIZE(str),
            PyUnicode_AS_UNICODE(sub), PyUnicode_GET_SIZE(sub),
            start, end
            );
    else
        result = stringlib_rfind_slice(
            PyUnicode_AS_UNICODE(str), PyUnicode_GET_SIZE(str),
            PyUnicode_AS_UNICODE(sub), PyUnicode_GET_SIZE(sub),
            start, end
            );

    Py_DECREF(str);
    Py_DECREF(sub);

    return result;
}

static
int tailmatch(PyUnicodeObject *self,
	      PyUnicodeObject *substring,
	      Py_ssize_t start,
	      Py_ssize_t end,
	      int direction)
{
    if (substring->length == 0)
        return 1;

    FIX_START_END(self);

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

Py_ssize_t PyUnicode_Tailmatch(PyObject *str,
			PyObject *substr,
			Py_ssize_t start,
			Py_ssize_t end,
			int direction)
{
    Py_ssize_t result;

    str = PyUnicode_FromObject(str);
    if (str == NULL)
	return -1;
    substr = PyUnicode_FromObject(substr);
    if (substr == NULL) {
	Py_DECREF(str);
	return -1;
    }

    result = tailmatch((PyUnicodeObject *)str,
		       (PyUnicodeObject *)substr,
		       start, end, direction);
    Py_DECREF(str);
    Py_DECREF(substr);
    return result;
}

/* Apply fixfct filter to the Unicode object self and return a
   reference to the modified object */

static
PyObject *fixup(PyUnicodeObject *self,
		int (*fixfct)(PyUnicodeObject *s))
{

    PyUnicodeObject *u;

    u = (PyUnicodeObject*) PyUnicode_FromUnicode(NULL, self->length);
    if (u == NULL)
	return NULL;

    Py_UNICODE_COPY(u->str, self->str, self->length);

    if (!fixfct(u) && PyUnicode_CheckExact(self)) {
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
    Py_ssize_t len = self->length;
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
    Py_ssize_t len = self->length;
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
    Py_ssize_t len = self->length;
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
    Py_ssize_t len = self->length;
    Py_UNICODE *s = self->str;
    int status = 0;

    if (len == 0)
	return 0;
    if (Py_UNICODE_ISLOWER(*s)) {
	*s = Py_UNICODE_TOUPPER(*s);
	status = 1;
    }
    s++;
    while (--len > 0) {
        if (Py_UNICODE_ISUPPER(*s)) {
            *s = Py_UNICODE_TOLOWER(*s);
            status = 1;
        }
        s++;
    }
    return status;
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

PyObject *
PyUnicode_Join(PyObject *separator, PyObject *seq)
{
    PyObject *internal_separator = NULL;
    const Py_UNICODE blank = ' ';
    const Py_UNICODE *sep = &blank;
    Py_ssize_t seplen = 1;
    PyUnicodeObject *res = NULL; /* the result */
    Py_ssize_t res_alloc = 100;  /* # allocated bytes for string in res */
    Py_ssize_t res_used;         /* # used bytes */
    Py_UNICODE *res_p;       /* pointer to free byte in res's string area */
    PyObject *fseq;          /* PySequence_Fast(seq) */
    Py_ssize_t seqlen;              /* len(fseq) -- number of items in sequence */
    PyObject *item;
    Py_ssize_t i;

    fseq = PySequence_Fast(seq, "");
    if (fseq == NULL) {
    	return NULL;
    }

    /* Grrrr.  A codec may be invoked to convert str objects to
     * Unicode, and so it's possible to call back into Python code
     * during PyUnicode_FromObject(), and so it's possible for a sick
     * codec to change the size of fseq (if seq is a list).  Therefore
     * we have to keep refetching the size -- can't assume seqlen
     * is invariant.
     */
    seqlen = PySequence_Fast_GET_SIZE(fseq);
    /* If empty sequence, return u"". */
    if (seqlen == 0) {
    	res = _PyUnicode_New(0);  /* empty sequence; return u"" */
    	goto Done;
    }
    /* If singleton sequence with an exact Unicode, return that. */
    if (seqlen == 1) {
	item = PySequence_Fast_GET_ITEM(fseq, 0);
	if (PyUnicode_CheckExact(item)) {
	    Py_INCREF(item);
	    res = (PyUnicodeObject *)item;
	    goto Done;
	}
    }

    /* At least two items to join, or one that isn't exact Unicode. */
    if (seqlen > 1) {
        /* Set up sep and seplen -- they're needed. */
    	if (separator == NULL) {
	    sep = &blank;
	    seplen = 1;
        }
    	else {
	    internal_separator = PyUnicode_FromObject(separator);
	    if (internal_separator == NULL)
	        goto onError;
	    sep = PyUnicode_AS_UNICODE(internal_separator);
	    seplen = PyUnicode_GET_SIZE(internal_separator);
	    /* In case PyUnicode_FromObject() mutated seq. */
	    seqlen = PySequence_Fast_GET_SIZE(fseq);
        }
    }

    /* Get space. */
    res = _PyUnicode_New(res_alloc);
    if (res == NULL)
        goto onError;
    res_p = PyUnicode_AS_UNICODE(res);
    res_used = 0;

    for (i = 0; i < seqlen; ++i) {
	Py_ssize_t itemlen;
	Py_ssize_t new_res_used;

	item = PySequence_Fast_GET_ITEM(fseq, i);
	/* Convert item to Unicode. */
	if (! PyUnicode_Check(item) && ! PyString_Check(item)) {
	    PyErr_Format(PyExc_TypeError,
			 "sequence item %zd: expected string or Unicode,"
			 " %.80s found",
			 i, Py_TYPE(item)->tp_name);
	    goto onError;
	}
	item = PyUnicode_FromObject(item);
	if (item == NULL)
	    goto onError;
	/* We own a reference to item from here on. */

	/* In case PyUnicode_FromObject() mutated seq. */
	seqlen = PySequence_Fast_GET_SIZE(fseq);

        /* Make sure we have enough space for the separator and the item. */
	itemlen = PyUnicode_GET_SIZE(item);
	new_res_used = res_used + itemlen;
	if (new_res_used < 0)
	    goto Overflow;
	if (i < seqlen - 1) {
	    new_res_used += seplen;
	    if (new_res_used < 0)
		goto Overflow;
	}
	if (new_res_used > res_alloc) {
	    /* double allocated size until it's big enough */
	    do {
	        res_alloc += res_alloc;
	        if (res_alloc <= 0)
	            goto Overflow;
	    } while (new_res_used > res_alloc);
	    if (_PyUnicode_Resize(&res, res_alloc) < 0) {
		Py_DECREF(item);
		goto onError;
	    }
            res_p = PyUnicode_AS_UNICODE(res) + res_used;
	}

	/* Copy item, and maybe the separator. */
	Py_UNICODE_COPY(res_p, PyUnicode_AS_UNICODE(item), itemlen);
	res_p += itemlen;
	if (i < seqlen - 1) {
	    Py_UNICODE_COPY(res_p, sep, seplen);
	    res_p += seplen;
	}
	Py_DECREF(item);
	res_used = new_res_used;
    }

    /* Shrink res to match the used area; this probably can't fail,
     * but it's cheap to check.
     */
    if (_PyUnicode_Resize(&res, res_used) < 0)
	goto onError;

 Done:
    Py_XDECREF(internal_separator);
    Py_DECREF(fseq);
    return (PyObject *)res;

 Overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "join() result is too long for a Python string");
    Py_DECREF(item);
    /* fall through */

 onError:
    Py_XDECREF(internal_separator);
    Py_DECREF(fseq);
    Py_XDECREF(res);
    return NULL;
}

static
PyUnicodeObject *pad(PyUnicodeObject *self,
		     Py_ssize_t left,
		     Py_ssize_t right,
		     Py_UNICODE fill)
{
    PyUnicodeObject *u;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0 && PyUnicode_CheckExact(self)) {
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
	str = PyUnicode_FromUnicode((data) + (left), (right) - (left));	\
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
			   Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    PyObject *str;
    register const Py_UNICODE *buf = self->str;

    for (i = j = 0; i < len; ) {
	/* find a token */
	while (i < len && Py_UNICODE_ISSPACE(buf[i]))
	    i++;
	j = i;
	while (i < len && !Py_UNICODE_ISSPACE(buf[i]))
	    i++;
	if (j < i) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(buf, j, i);
	    while (i < len && Py_UNICODE_ISSPACE(buf[i]))
		i++;
	    j = i;
	}
    }
    if (j < len) {
	SPLIT_APPEND(buf, j, len);
    }
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

PyObject *PyUnicode_Splitlines(PyObject *string,
			       int keepends)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len;
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
	Py_ssize_t eol;

	/* Find a line and append it */
	while (i < len && !BLOOM_LINEBREAK(data[i]))
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
    Py_XDECREF(list);
    Py_DECREF(string);
    return NULL;
}

static
PyObject *split_char(PyUnicodeObject *self,
		     PyObject *list,
		     Py_UNICODE ch,
		     Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    PyObject *str;
    register const Py_UNICODE *buf = self->str;

    for (i = j = 0; i < len; ) {
	if (buf[i] == ch) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(buf, j, i);
	    i = j = i + 1;
	} else
	    i++;
    }
    if (j <= len) {
	SPLIT_APPEND(buf, j, len);
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
			  Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    Py_ssize_t sublen = substring->length;
    PyObject *str;

    for (i = j = 0; i <= len - sublen; ) {
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

static
PyObject *rsplit_whitespace(PyUnicodeObject *self,
			    PyObject *list,
			    Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    PyObject *str;
    register const Py_UNICODE *buf = self->str;

    for (i = j = len - 1; i >= 0; ) {
	/* find a token */
	while (i >= 0 && Py_UNICODE_ISSPACE(buf[i]))
	    i--;
	j = i;
	while (i >= 0 && !Py_UNICODE_ISSPACE(buf[i]))
	    i--;
	if (j > i) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(buf, i + 1, j + 1);
	    while (i >= 0 && Py_UNICODE_ISSPACE(buf[i]))
		i--;
	    j = i;
	}
    }
    if (j >= 0) {
	SPLIT_APPEND(buf, 0, j + 1);
    }
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

static 
PyObject *rsplit_char(PyUnicodeObject *self,
		      PyObject *list,
		      Py_UNICODE ch,
		      Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    PyObject *str;
    register const Py_UNICODE *buf = self->str;

    for (i = j = len - 1; i >= 0; ) {
	if (buf[i] == ch) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(buf, i + 1, j + 1);
	    j = i = i - 1;
	} else
	    i--;
    }
    if (j >= -1) {
	SPLIT_APPEND(buf, 0, j + 1);
    }
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

static 
PyObject *rsplit_substring(PyUnicodeObject *self,
			   PyObject *list,
			   PyUnicodeObject *substring,
			   Py_ssize_t maxcount)
{
    register Py_ssize_t i;
    register Py_ssize_t j;
    Py_ssize_t len = self->length;
    Py_ssize_t sublen = substring->length;
    PyObject *str;

    for (i = len - sublen, j = len; i >= 0; ) {
	if (Py_UNICODE_MATCH(self, i, substring)) {
	    if (maxcount-- <= 0)
		break;
	    SPLIT_APPEND(self->str, i + sublen, j);
	    j = i;
	    i -= sublen;
	} else
	    i--;
    }
    if (j >= 0) {
	SPLIT_APPEND(self->str, 0, j);
    }
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

 onError:
    Py_DECREF(list);
    return NULL;
}

#undef SPLIT_APPEND

static
PyObject *split(PyUnicodeObject *self,
		PyUnicodeObject *substring,
		Py_ssize_t maxcount)
{
    PyObject *list;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;

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
PyObject *rsplit(PyUnicodeObject *self,
		 PyUnicodeObject *substring,
		 Py_ssize_t maxcount)
{
    PyObject *list;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;

    list = PyList_New(0);
    if (!list)
        return NULL;

    if (substring == NULL)
	return rsplit_whitespace(self,list,maxcount);

    else if (substring->length == 1)
	return rsplit_char(self,list,substring->str[0],maxcount);

    else if (substring->length == 0) {
	Py_DECREF(list);
	PyErr_SetString(PyExc_ValueError, "empty separator");
	return NULL;
    }
    else
	return rsplit_substring(self,list,substring,maxcount);
}

static
PyObject *replace(PyUnicodeObject *self,
		  PyUnicodeObject *str1,
		  PyUnicodeObject *str2,
		  Py_ssize_t maxcount)
{
    PyUnicodeObject *u;

    if (maxcount < 0)
	maxcount = PY_SSIZE_T_MAX;

    if (str1->length == str2->length) {
        /* same length */
        Py_ssize_t i;
        if (str1->length == 1) {
            /* replace characters */
            Py_UNICODE u1, u2;
            if (!findchar(self->str, self->length, str1->str[0]))
                goto nothing;
            u = (PyUnicodeObject*) PyUnicode_FromUnicode(NULL, self->length);
            if (!u)
                return NULL;
            Py_UNICODE_COPY(u->str, self->str, self->length);
            u1 = str1->str[0];
            u2 = str2->str[0];
            for (i = 0; i < u->length; i++)
                if (u->str[i] == u1) {
                    if (--maxcount < 0)
                        break;
                    u->str[i] = u2;
                }
        } else {
            i = fastsearch(
                self->str, self->length, str1->str, str1->length, FAST_SEARCH
                );
            if (i < 0)
                goto nothing;
            u = (PyUnicodeObject*) PyUnicode_FromUnicode(NULL, self->length);
            if (!u)
                return NULL;
            Py_UNICODE_COPY(u->str, self->str, self->length);
            while (i <= self->length - str1->length)
                if (Py_UNICODE_MATCH(self, i, str1)) {
                    if (--maxcount < 0)
                        break;
                    Py_UNICODE_COPY(u->str+i, str2->str, str2->length);
                    i += str1->length;
                } else
                    i++;
        }
    } else {

        Py_ssize_t n, i, j, e;
        Py_ssize_t product, new_size, delta;
        Py_UNICODE *p;

        /* replace strings */
        n = stringlib_count(self->str, self->length, str1->str, str1->length);
        if (n > maxcount)
            n = maxcount;
        if (n == 0)
            goto nothing;
        /* new_size = self->length + n * (str2->length - str1->length)); */
        delta = (str2->length - str1->length);
        if (delta == 0) {
            new_size = self->length;
        } else {
            product = n * (str2->length - str1->length);
            if ((product / (str2->length - str1->length)) != n) {
                PyErr_SetString(PyExc_OverflowError,
                                "replace string is too long");
                return NULL;
            }
            new_size = self->length + product;
            if (new_size < 0) {
                PyErr_SetString(PyExc_OverflowError,
                                "replace string is too long");
                return NULL;
            }
        }
        u = _PyUnicode_New(new_size);
        if (!u)
            return NULL;
        i = 0;
        p = u->str;
        e = self->length - str1->length;
        if (str1->length > 0) {
            while (n-- > 0) {
                /* look for next match */
                j = i;
                while (j <= e) {
                    if (Py_UNICODE_MATCH(self, j, str1))
                        break;
                    j++;
                }
		if (j > i) {
                    if (j > e)
                        break;
                    /* copy unchanged part [i:j] */
                    Py_UNICODE_COPY(p, self->str+i, j-i);
                    p += j - i;
                }
                /* copy substitution string */
                if (str2->length > 0) {
                    Py_UNICODE_COPY(p, str2->str, str2->length);
                    p += str2->length;
                }
                i = j + str1->length;
            }
            if (i < self->length)
                /* copy tail [i:] */
                Py_UNICODE_COPY(p, self->str+i, self->length-i);
        } else {
            /* interleave */
            while (n > 0) {
                Py_UNICODE_COPY(p, str2->str, str2->length);
                p += str2->length;
                if (--n <= 0)
                    break;
                *p++ = self->str[i++];
            }
            Py_UNICODE_COPY(p, self->str+i, self->length-i);
        }
    }
    return (PyObject *) u;

nothing:
    /* nothing to replace; return original string (when possible) */
    if (PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject *) self;
    }
    return PyUnicode_FromUnicode(self->str, self->length);
}

/* --- Unicode Object Methods --------------------------------------------- */

PyDoc_STRVAR(title__doc__,
"S.title() -> unicode\n\
\n\
Return a titlecased version of S, i.e. words start with title case\n\
characters, all remaining cased characters have lower case.");

static PyObject*
unicode_title(PyUnicodeObject *self)
{
    return fixup(self, fixtitle);
}

PyDoc_STRVAR(capitalize__doc__,
"S.capitalize() -> unicode\n\
\n\
Return a capitalized version of S, i.e. make the first character\n\
have upper case.");

static PyObject*
unicode_capitalize(PyUnicodeObject *self)
{
    return fixup(self, fixcapitalize);
}

#if 0
PyDoc_STRVAR(capwords__doc__,
"S.capwords() -> unicode\n\
\n\
Apply .capitalize() to all words in S and return the result with\n\
normalized whitespace (all whitespace strings are replaced by ' ').");

static PyObject*
unicode_capwords(PyUnicodeObject *self)
{
    PyObject *list;
    PyObject *item;
    Py_ssize_t i;

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

/* Argument converter.  Coerces to a single unicode character */

static int
convert_uc(PyObject *obj, void *addr)
{
	Py_UNICODE *fillcharloc = (Py_UNICODE *)addr;
	PyObject *uniobj;
	Py_UNICODE *unistr;

	uniobj = PyUnicode_FromObject(obj);
	if (uniobj == NULL) {
		PyErr_SetString(PyExc_TypeError,
			"The fill character cannot be converted to Unicode");
		return 0;
	}
	if (PyUnicode_GET_SIZE(uniobj) != 1) {
		PyErr_SetString(PyExc_TypeError,
			"The fill character must be exactly one character long");
		Py_DECREF(uniobj);
		return 0;
	}
	unistr = PyUnicode_AS_UNICODE(uniobj);
	*fillcharloc = unistr[0];
	Py_DECREF(uniobj);
	return 1;
}

PyDoc_STRVAR(center__doc__,
"S.center(width[, fillchar]) -> unicode\n\
\n\
Return S centered in a Unicode string of length width. Padding is\n\
done using the specified fill character (default is a space)");

static PyObject *
unicode_center(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t marg, left;
    Py_ssize_t width;
    Py_UNICODE fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|O&:center", &width, convert_uc, &fillchar))
        return NULL;

    if (self->length >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    marg = width - self->length;
    left = marg / 2 + (marg & width & 1);

    return (PyObject*) pad(self, left, marg - left, fillchar);
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
    Py_ssize_t len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->length;
    len2 = str2->length;

    while (len1 > 0 && len2 > 0) {
        Py_UNICODE c1, c2;

        c1 = *s1++;
        c2 = *s2++;

	if (c1 > (1<<11) * 26)
	    c1 += utf16Fixup[c1>>11];
	if (c2 > (1<<11) * 26)
            c2 += utf16Fixup[c2>>11];
        /* now c1 and c2 are in UTF-32-compatible order */

        if (c1 != c2)
            return (c1 < c2) ? -1 : 1;

        len1--; len2--;
    }

    return (len1 < len2) ? -1 : (len1 != len2);
}

#else

static int
unicode_compare(PyUnicodeObject *str1, PyUnicodeObject *str2)
{
    register Py_ssize_t len1, len2;

    Py_UNICODE *s1 = str1->str;
    Py_UNICODE *s2 = str2->str;

    len1 = str1->length;
    len2 = str2->length;

    while (len1 > 0 && len2 > 0) {
        Py_UNICODE c1, c2;

        c1 = *s1++;
        c2 = *s2++;

        if (c1 != c2)
            return (c1 < c2) ? -1 : 1;

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

PyObject *PyUnicode_RichCompare(PyObject *left,
                                PyObject *right,
                                int op)
{
    int result;

    result = PyUnicode_Compare(left, right);
    if (result == -1 && PyErr_Occurred())
        goto onError;

    /* Convert the return value to a Boolean */
    switch (op) {
    case Py_EQ:
        result = (result == 0);
        break;
    case Py_NE:
        result = (result != 0);
        break;
    case Py_LE:
        result = (result <= 0);
        break;
    case Py_GE:
        result = (result >= 0);
        break;
    case Py_LT:
        result = (result == -1);
        break;
    case Py_GT:
        result = (result == 1);
        break;
    }
    return PyBool_FromLong(result);

 onError:

    /* Standard case

       Type errors mean that PyUnicode_FromObject() could not convert
       one of the arguments (usually the right hand side) to Unicode,
       ie. we can't handle the comparison request. However, it is
       possible that the other object knows a comparison method, which
       is why we return Py_NotImplemented to give the other object a
       chance.

    */
    if (PyErr_ExceptionMatches(PyExc_TypeError)) {
        PyErr_Clear();
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (op != Py_EQ && op != Py_NE)
        return NULL;

    /* Equality comparison.

       This is a special case: we silence any PyExc_UnicodeDecodeError
       and instead turn it into a PyErr_UnicodeWarning.

    */
    if (!PyErr_ExceptionMatches(PyExc_UnicodeDecodeError))
        return NULL;
    PyErr_Clear();
    if (PyErr_Warn(PyExc_UnicodeWarning, 
                   (op == Py_EQ) ? 
                   "Unicode equal comparison "
                   "failed to convert both arguments to Unicode - "
                   "interpreting them as being unequal" :
                   "Unicode unequal comparison "
                   "failed to convert both arguments to Unicode - "
                   "interpreting them as being unequal"
                   ) < 0)
        return NULL;
    result = (op == Py_NE);
    return PyBool_FromLong(result);
}

int PyUnicode_Contains(PyObject *container,
		       PyObject *element)
{
    PyObject *str, *sub;
    int result;

    /* Coerce the two arguments */
    sub = PyUnicode_FromObject(element);
    if (!sub) {
	PyErr_SetString(PyExc_TypeError,
	    "'in <string>' requires string as left operand");
        return -1;
    }

    str = PyUnicode_FromObject(container);
    if (!str) {
        Py_DECREF(sub);
        return -1;
    }

    result = stringlib_contains_obj(str, sub);

    Py_DECREF(str);
    Py_DECREF(sub);

    return result;
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

PyDoc_STRVAR(count__doc__,
"S.count(sub[, start[, end]]) -> int\n\
\n\
Return the number of non-overlapping occurrences of substring sub in\n\
Unicode string S[start:end].  Optional arguments start and end are\n\
interpreted as in slice notation.");

static PyObject *
unicode_count(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "O|O&O&:count", &substring,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
        return NULL;

    substring = (PyUnicodeObject *)PyUnicode_FromObject(
        (PyObject *)substring);
    if (substring == NULL)
	return NULL;

    FIX_START_END(self);

    result = PyInt_FromSsize_t(
        stringlib_count(self->str + start, end - start,
                        substring->str, substring->length)
        );

    Py_DECREF(substring);

    return result;
}

PyDoc_STRVAR(encode__doc__,
"S.encode([encoding[,errors]]) -> string or unicode\n\
\n\
Encodes S using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a UnicodeEncodeError. Other possible values are 'ignore', 'replace' and\n\
'xmlcharrefreplace' as well as any other name registered with\n\
codecs.register_error that can handle UnicodeEncodeErrors.");

static PyObject *
unicode_encode(PyUnicodeObject *self, PyObject *args)
{
    char *encoding = NULL;
    char *errors = NULL;
    PyObject *v;
    
    if (!PyArg_ParseTuple(args, "|ss:encode", &encoding, &errors))
        return NULL;
    v = PyUnicode_AsEncodedObject((PyObject *)self, encoding, errors);
    if (v == NULL)
        goto onError;
    if (!PyString_Check(v) && !PyUnicode_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "encoder did not return a string/unicode object "
                     "(type=%.400s)",
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        return NULL;
    }
    return v;

 onError:
    return NULL;
}

PyDoc_STRVAR(decode__doc__,
"S.decode([encoding[,errors]]) -> string or unicode\n\
\n\
Decodes S using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a UnicodeDecodeError. Other possible values are 'ignore' and 'replace'\n\
as well as any other name registerd with codecs.register_error that is\n\
able to handle UnicodeDecodeErrors.");

static PyObject *
unicode_decode(PyUnicodeObject *self, PyObject *args)
{
    char *encoding = NULL;
    char *errors = NULL;
    PyObject *v;
    
    if (!PyArg_ParseTuple(args, "|ss:decode", &encoding, &errors))
        return NULL;
    v = PyUnicode_AsDecodedObject((PyObject *)self, encoding, errors);
    if (v == NULL)
        goto onError;
    if (!PyString_Check(v) && !PyUnicode_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "decoder did not return a string/unicode object "
                     "(type=%.400s)",
                     Py_TYPE(v)->tp_name);
        Py_DECREF(v);
        return NULL;
    }
    return v;

 onError:
    return NULL;
}

PyDoc_STRVAR(expandtabs__doc__,
"S.expandtabs([tabsize]) -> unicode\n\
\n\
Return a copy of S where all tab characters are expanded using spaces.\n\
If tabsize is not given, a tab size of 8 characters is assumed.");

static PyObject*
unicode_expandtabs(PyUnicodeObject *self, PyObject *args)
{
    Py_UNICODE *e;
    Py_UNICODE *p;
    Py_UNICODE *q;
    Py_ssize_t i, j, old_j;
    PyUnicodeObject *u;
    int tabsize = 8;

    if (!PyArg_ParseTuple(args, "|i:expandtabs", &tabsize))
	return NULL;

    /* First pass: determine size of output string */
    i = j = old_j = 0;
    e = self->str + self->length;
    for (p = self->str; p < e; p++)
        if (*p == '\t') {
	    if (tabsize > 0) {
		j += tabsize - (j % tabsize);
		if (old_j > j) {
		    PyErr_SetString(PyExc_OverflowError,
				    "new string is too long");
		    return NULL;
		}
		old_j = j;
	    }
	}
        else {
            j++;
            if (*p == '\n' || *p == '\r') {
                i += j;
                old_j = j = 0;
                if (i < 0) {
                    PyErr_SetString(PyExc_OverflowError,
                                    "new string is too long");
                    return NULL;
                }
            }
        }

    if ((i + j) < 0) {
        PyErr_SetString(PyExc_OverflowError, "new string is too long");
        return NULL;
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

PyDoc_STRVAR(find__doc__,
"S.find(sub [,start [,end]]) -> int\n\
\n\
Return the lowest index in S where substring sub is found,\n\
such that sub is contained within s[start:end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
unicode_find(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!_ParseTupleFinds(args, &substring, &start, &end))
        return NULL;

    result = stringlib_find_slice(
        PyUnicode_AS_UNICODE(self), PyUnicode_GET_SIZE(self),
        PyUnicode_AS_UNICODE(substring), PyUnicode_GET_SIZE(substring),
        start, end
        );

    Py_DECREF(substring);

    return PyInt_FromSsize_t(result);
}

static PyObject *
unicode_getitem(PyUnicodeObject *self, Py_ssize_t index)
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

    register Py_ssize_t len;
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

PyDoc_STRVAR(index__doc__,
"S.index(sub [,start [,end]]) -> int\n\
\n\
Like S.find() but raise ValueError when the substring is not found.");

static PyObject *
unicode_index(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t result;
    PyObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;

    if (!_ParseTupleFinds(args, &substring, &start, &end))
        return NULL;

    result = stringlib_find_slice(
        PyUnicode_AS_UNICODE(self), PyUnicode_GET_SIZE(self),
        PyUnicode_AS_UNICODE(substring), PyUnicode_GET_SIZE(substring),
        start, end
        );

    Py_DECREF(substring);

    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }

    return PyInt_FromSsize_t(result);
}

PyDoc_STRVAR(islower__doc__,
"S.islower() -> bool\n\
\n\
Return True if all cased characters in S are lowercase and there is\n\
at least one cased character in S, False otherwise.");

static PyObject*
unicode_islower(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyBool_FromLong(Py_UNICODE_ISLOWER(*p));

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;

	if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch))
	    return PyBool_FromLong(0);
	else if (!cased && Py_UNICODE_ISLOWER(ch))
	    cased = 1;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(isupper__doc__,
"S.isupper() -> bool\n\
\n\
Return True if all cased characters in S are uppercase and there is\n\
at least one cased character in S, False otherwise.");

static PyObject*
unicode_isupper(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyBool_FromLong(Py_UNICODE_ISUPPER(*p) != 0);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;

	if (Py_UNICODE_ISLOWER(ch) || Py_UNICODE_ISTITLE(ch))
	    return PyBool_FromLong(0);
	else if (!cased && Py_UNICODE_ISUPPER(ch))
	    cased = 1;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(istitle__doc__,
"S.istitle() -> bool\n\
\n\
Return True if S is a titlecased string and there is at least one\n\
character in S, i.e. upper- and titlecase characters may only\n\
follow uncased characters and lowercase characters only cased ones.\n\
Return False otherwise.");

static PyObject*
unicode_istitle(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;
    int cased, previous_is_cased;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1)
	return PyBool_FromLong((Py_UNICODE_ISTITLE(*p) != 0) ||
			       (Py_UNICODE_ISUPPER(*p) != 0));

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    cased = 0;
    previous_is_cased = 0;
    for (; p < e; p++) {
	register const Py_UNICODE ch = *p;

	if (Py_UNICODE_ISUPPER(ch) || Py_UNICODE_ISTITLE(ch)) {
	    if (previous_is_cased)
		return PyBool_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else if (Py_UNICODE_ISLOWER(ch)) {
	    if (!previous_is_cased)
		return PyBool_FromLong(0);
	    previous_is_cased = 1;
	    cased = 1;
	}
	else
	    previous_is_cased = 0;
    }
    return PyBool_FromLong(cased);
}

PyDoc_STRVAR(isspace__doc__,
"S.isspace() -> bool\n\
\n\
Return True if all characters in S are whitespace\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isspace(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISSPACE(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISSPACE(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isalpha__doc__,
"S.isalpha() -> bool\n\
\n\
Return True if all characters in S are alphabetic\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isalpha(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISALPHA(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISALPHA(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isalnum__doc__,
"S.isalnum() -> bool\n\
\n\
Return True if all characters in S are alphanumeric\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isalnum(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISALNUM(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISALNUM(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isdecimal__doc__,
"S.isdecimal() -> bool\n\
\n\
Return True if there are only decimal characters in S,\n\
False otherwise.");

static PyObject*
unicode_isdecimal(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISDECIMAL(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISDECIMAL(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isdigit__doc__,
"S.isdigit() -> bool\n\
\n\
Return True if all characters in S are digits\n\
and there is at least one character in S, False otherwise.");

static PyObject*
unicode_isdigit(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISDIGIT(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISDIGIT(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(isnumeric__doc__,
"S.isnumeric() -> bool\n\
\n\
Return True if there are only numeric characters in S,\n\
False otherwise.");

static PyObject*
unicode_isnumeric(PyUnicodeObject *self)
{
    register const Py_UNICODE *p = PyUnicode_AS_UNICODE(self);
    register const Py_UNICODE *e;

    /* Shortcut for single character strings */
    if (PyUnicode_GET_SIZE(self) == 1 &&
	Py_UNICODE_ISNUMERIC(*p))
	return PyBool_FromLong(1);

    /* Special case for empty strings */
    if (PyUnicode_GET_SIZE(self) == 0)
	return PyBool_FromLong(0);

    e = p + PyUnicode_GET_SIZE(self);
    for (; p < e; p++) {
	if (!Py_UNICODE_ISNUMERIC(*p))
	    return PyBool_FromLong(0);
    }
    return PyBool_FromLong(1);
}

PyDoc_STRVAR(join__doc__,
"S.join(sequence) -> unicode\n\
\n\
Return a string which is the concatenation of the strings in the\n\
sequence.  The separator between elements is S.");

static PyObject*
unicode_join(PyObject *self, PyObject *data)
{
    return PyUnicode_Join(self, data);
}

static Py_ssize_t
unicode_length(PyUnicodeObject *self)
{
    return self->length;
}

PyDoc_STRVAR(ljust__doc__,
"S.ljust(width[, fillchar]) -> int\n\
\n\
Return S left justified in a Unicode string of length width. Padding is\n\
done using the specified fill character (default is a space).");

static PyObject *
unicode_ljust(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t width;
    Py_UNICODE fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|O&:ljust", &width, convert_uc, &fillchar))
        return NULL;

    if (self->length >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, 0, width - self->length, fillchar);
}

PyDoc_STRVAR(lower__doc__,
"S.lower() -> unicode\n\
\n\
Return a copy of the string S converted to lowercase.");

static PyObject*
unicode_lower(PyUnicodeObject *self)
{
    return fixup(self, fixlower);
}

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2

/* Arrays indexed by above */
static const char *stripformat[] = {"|O:lstrip", "|O:rstrip", "|O:strip"};

#define STRIPNAME(i) (stripformat[i]+3)

/* externally visible for str.strip(unicode) */
PyObject *
_PyUnicode_XStrip(PyUnicodeObject *self, int striptype, PyObject *sepobj)
{
	Py_UNICODE *s = PyUnicode_AS_UNICODE(self);
	Py_ssize_t len = PyUnicode_GET_SIZE(self);
	Py_UNICODE *sep = PyUnicode_AS_UNICODE(sepobj);
	Py_ssize_t seplen = PyUnicode_GET_SIZE(sepobj);
	Py_ssize_t i, j;

        BLOOM_MASK sepmask = make_bloom_mask(sep, seplen);

	i = 0;
	if (striptype != RIGHTSTRIP) {
            while (i < len && BLOOM_MEMBER(sepmask, s[i], sep, seplen)) {
                i++;
            }
	}

	j = len;
	if (striptype != LEFTSTRIP) {
            do {
                j--;
            } while (j >= i && BLOOM_MEMBER(sepmask, s[j], sep, seplen));
            j++;
	}

	if (i == 0 && j == len && PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject*)self;
	}
	else
            return PyUnicode_FromUnicode(s+i, j-i);
}


static PyObject *
do_strip(PyUnicodeObject *self, int striptype)
{
	Py_UNICODE *s = PyUnicode_AS_UNICODE(self);
	Py_ssize_t len = PyUnicode_GET_SIZE(self), i, j;

	i = 0;
	if (striptype != RIGHTSTRIP) {
		while (i < len && Py_UNICODE_ISSPACE(s[i])) {
			i++;
		}
	}

	j = len;
	if (striptype != LEFTSTRIP) {
		do {
			j--;
		} while (j >= i && Py_UNICODE_ISSPACE(s[j]));
		j++;
	}

	if (i == 0 && j == len && PyUnicode_CheckExact(self)) {
		Py_INCREF(self);
		return (PyObject*)self;
	}
	else
		return PyUnicode_FromUnicode(s+i, j-i);
}


static PyObject *
do_argstrip(PyUnicodeObject *self, int striptype, PyObject *args)
{
	PyObject *sep = NULL;

	if (!PyArg_ParseTuple(args, (char *)stripformat[striptype], &sep))
		return NULL;

	if (sep != NULL && sep != Py_None) {
		if (PyUnicode_Check(sep))
			return _PyUnicode_XStrip(self, striptype, sep);
		else if (PyString_Check(sep)) {
			PyObject *res;
			sep = PyUnicode_FromObject(sep);
			if (sep==NULL)
				return NULL;
			res = _PyUnicode_XStrip(self, striptype, sep);
			Py_DECREF(sep);
			return res;
		}
		else {
			PyErr_Format(PyExc_TypeError,
				     "%s arg must be None, unicode or str",
				     STRIPNAME(striptype));
			return NULL;
		}
	}

	return do_strip(self, striptype);
}


PyDoc_STRVAR(strip__doc__,
"S.strip([chars]) -> unicode\n\
\n\
Return a copy of the string S with leading and trailing\n\
whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.\n\
If chars is a str, it will be converted to unicode before stripping");

static PyObject *
unicode_strip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
		return do_strip(self, BOTHSTRIP); /* Common case */
	else
		return do_argstrip(self, BOTHSTRIP, args);
}


PyDoc_STRVAR(lstrip__doc__,
"S.lstrip([chars]) -> unicode\n\
\n\
Return a copy of the string S with leading whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.\n\
If chars is a str, it will be converted to unicode before stripping");

static PyObject *
unicode_lstrip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
		return do_strip(self, LEFTSTRIP); /* Common case */
	else
		return do_argstrip(self, LEFTSTRIP, args);
}


PyDoc_STRVAR(rstrip__doc__,
"S.rstrip([chars]) -> unicode\n\
\n\
Return a copy of the string S with trailing whitespace removed.\n\
If chars is given and not None, remove characters in chars instead.\n\
If chars is a str, it will be converted to unicode before stripping");

static PyObject *
unicode_rstrip(PyUnicodeObject *self, PyObject *args)
{
	if (PyTuple_GET_SIZE(args) == 0)
		return do_strip(self, RIGHTSTRIP); /* Common case */
	else
		return do_argstrip(self, RIGHTSTRIP, args);
}


static PyObject*
unicode_repeat(PyUnicodeObject *str, Py_ssize_t len)
{
    PyUnicodeObject *u;
    Py_UNICODE *p;
    Py_ssize_t nchars;
    size_t nbytes;

    if (len < 0)
        len = 0;

    if (len == 1 && PyUnicode_CheckExact(str)) {
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

    if (str->length == 1 && len > 0) {
        Py_UNICODE_FILL(p, str->str[0], len);
    } else {
	Py_ssize_t done = 0; /* number of characters copied this far */
	if (done < nchars) {
            Py_UNICODE_COPY(p, str->str, str->length);
            done = str->length;
	}
	while (done < nchars) {
            int n = (done <= nchars-done) ? done : nchars-done;
            Py_UNICODE_COPY(p+done, p, n);
            done += n;
	}
    }

    return (PyObject*) u;
}

PyObject *PyUnicode_Replace(PyObject *obj,
			    PyObject *subobj,
			    PyObject *replobj,
			    Py_ssize_t maxcount)
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

PyDoc_STRVAR(replace__doc__,
"S.replace (old, new[, maxsplit]) -> unicode\n\
\n\
Return a copy of S with all occurrences of substring\n\
old replaced by new.  If the optional argument maxsplit is\n\
given, only the first maxsplit occurrences are replaced.");

static PyObject*
unicode_replace(PyUnicodeObject *self, PyObject *args)
{
    PyUnicodeObject *str1;
    PyUnicodeObject *str2;
    Py_ssize_t maxcount = -1;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "OO|n:replace", &str1, &str2, &maxcount))
        return NULL;
    str1 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str1);
    if (str1 == NULL)
	return NULL;
    str2 = (PyUnicodeObject *)PyUnicode_FromObject((PyObject *)str2);
    if (str2 == NULL) {
	Py_DECREF(str1);
	return NULL;
    }

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

PyDoc_STRVAR(rfind__doc__,
"S.rfind(sub [,start [,end]]) -> int\n\
\n\
Return the highest index in S where substring sub is found,\n\
such that sub is contained within s[start:end].  Optional\n\
arguments start and end are interpreted as in slice notation.\n\
\n\
Return -1 on failure.");

static PyObject *
unicode_rfind(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!_ParseTupleFinds(args, &substring, &start, &end))
	    return NULL;

    result = stringlib_rfind_slice(
        PyUnicode_AS_UNICODE(self), PyUnicode_GET_SIZE(self),
        PyUnicode_AS_UNICODE(substring), PyUnicode_GET_SIZE(substring),
        start, end
        );

    Py_DECREF(substring);

    return PyInt_FromSsize_t(result);
}

PyDoc_STRVAR(rindex__doc__,
"S.rindex(sub [,start [,end]]) -> int\n\
\n\
Like S.rfind() but raise ValueError when the substring is not found.");

static PyObject *
unicode_rindex(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring;
    Py_ssize_t start;
    Py_ssize_t end;
    Py_ssize_t result;

    if (!_ParseTupleFinds(args, &substring, &start, &end))
	    return NULL;

    result = stringlib_rfind_slice(
        PyUnicode_AS_UNICODE(self), PyUnicode_GET_SIZE(self),
        PyUnicode_AS_UNICODE(substring), PyUnicode_GET_SIZE(substring),
        start, end
        );

    Py_DECREF(substring);

    if (result < 0) {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    return PyInt_FromSsize_t(result);
}

PyDoc_STRVAR(rjust__doc__,
"S.rjust(width[, fillchar]) -> unicode\n\
\n\
Return S right justified in a Unicode string of length width. Padding is\n\
done using the specified fill character (default is a space).");

static PyObject *
unicode_rjust(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t width;
    Py_UNICODE fillchar = ' ';

    if (!PyArg_ParseTuple(args, "n|O&:rjust", &width, convert_uc, &fillchar))
        return NULL;

    if (self->length >= width && PyUnicode_CheckExact(self)) {
        Py_INCREF(self);
        return (PyObject*) self;
    }

    return (PyObject*) pad(self, width - self->length, 0, fillchar);
}

static PyObject*
unicode_slice(PyUnicodeObject *self, Py_ssize_t start, Py_ssize_t end)
{
    /* standard clamping */
    if (start < 0)
        start = 0;
    if (end < 0)
        end = 0;
    if (end > self->length)
        end = self->length;
    if (start == 0 && end == self->length && PyUnicode_CheckExact(self)) {
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
			  Py_ssize_t maxsplit)
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

PyDoc_STRVAR(split__doc__,
"S.split([sep [,maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in S, using sep as the\n\
delimiter string.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified or is None,\n\
any whitespace string is a separator.");

static PyObject*
unicode_split(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    Py_ssize_t maxcount = -1;

    if (!PyArg_ParseTuple(args, "|On:split", &substring, &maxcount))
        return NULL;

    if (substring == Py_None)
	return split(self, NULL, maxcount);
    else if (PyUnicode_Check(substring))
	return split(self, (PyUnicodeObject *)substring, maxcount);
    else
	return PyUnicode_Split((PyObject *)self, substring, maxcount);
}

PyObject *
PyUnicode_Partition(PyObject *str_in, PyObject *sep_in)
{
    PyObject* str_obj;
    PyObject* sep_obj;
    PyObject* out;

    str_obj = PyUnicode_FromObject(str_in);
    if (!str_obj)
	return NULL;
    sep_obj = PyUnicode_FromObject(sep_in);
    if (!sep_obj) {
        Py_DECREF(str_obj);
        return NULL;
    }

    out = stringlib_partition(
        str_obj, PyUnicode_AS_UNICODE(str_obj), PyUnicode_GET_SIZE(str_obj),
        sep_obj, PyUnicode_AS_UNICODE(sep_obj), PyUnicode_GET_SIZE(sep_obj)
        );

    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);

    return out;
}


PyObject *
PyUnicode_RPartition(PyObject *str_in, PyObject *sep_in)
{
    PyObject* str_obj;
    PyObject* sep_obj;
    PyObject* out;

    str_obj = PyUnicode_FromObject(str_in);
    if (!str_obj)
	return NULL;
    sep_obj = PyUnicode_FromObject(sep_in);
    if (!sep_obj) {
        Py_DECREF(str_obj);
        return NULL;
    }

    out = stringlib_rpartition(
        str_obj, PyUnicode_AS_UNICODE(str_obj), PyUnicode_GET_SIZE(str_obj),
        sep_obj, PyUnicode_AS_UNICODE(sep_obj), PyUnicode_GET_SIZE(sep_obj)
        );

    Py_DECREF(sep_obj);
    Py_DECREF(str_obj);

    return out;
}

PyDoc_STRVAR(partition__doc__,
"S.partition(sep) -> (head, sep, tail)\n\
\n\
Searches for the separator sep in S, and returns the part before it,\n\
the separator itself, and the part after it.  If the separator is not\n\
found, returns S and two empty strings.");

static PyObject*
unicode_partition(PyUnicodeObject *self, PyObject *separator)
{
    return PyUnicode_Partition((PyObject *)self, separator);
}

PyDoc_STRVAR(rpartition__doc__,
"S.rpartition(sep) -> (tail, sep, head)\n\
\n\
Searches for the separator sep in S, starting at the end of S, and returns\n\
the part before it, the separator itself, and the part after it.  If the\n\
separator is not found, returns two empty strings and S.");

static PyObject*
unicode_rpartition(PyUnicodeObject *self, PyObject *separator)
{
    return PyUnicode_RPartition((PyObject *)self, separator);
}

PyObject *PyUnicode_RSplit(PyObject *s,
			   PyObject *sep,
			   Py_ssize_t maxsplit)
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

    result = rsplit((PyUnicodeObject *)s, (PyUnicodeObject *)sep, maxsplit);

    Py_DECREF(s);
    Py_XDECREF(sep);
    return result;
}

PyDoc_STRVAR(rsplit__doc__,
"S.rsplit([sep [,maxsplit]]) -> list of strings\n\
\n\
Return a list of the words in S, using sep as the\n\
delimiter string, starting at the end of the string and\n\
working to the front.  If maxsplit is given, at most maxsplit\n\
splits are done. If sep is not specified, any whitespace string\n\
is a separator.");

static PyObject*
unicode_rsplit(PyUnicodeObject *self, PyObject *args)
{
    PyObject *substring = Py_None;
    Py_ssize_t maxcount = -1;

    if (!PyArg_ParseTuple(args, "|On:rsplit", &substring, &maxcount))
        return NULL;

    if (substring == Py_None)
	return rsplit(self, NULL, maxcount);
    else if (PyUnicode_Check(substring))
	return rsplit(self, (PyUnicodeObject *)substring, maxcount);
    else
	return PyUnicode_RSplit((PyObject *)self, substring, maxcount);
}

PyDoc_STRVAR(splitlines__doc__,
"S.splitlines([keepends]]) -> list of strings\n\
\n\
Return a list of the lines in S, breaking at line boundaries.\n\
Line breaks are not included in the resulting list unless keepends\n\
is given and true.");

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

PyDoc_STRVAR(swapcase__doc__,
"S.swapcase() -> unicode\n\
\n\
Return a copy of S with uppercase characters converted to lowercase\n\
and vice versa.");

static PyObject*
unicode_swapcase(PyUnicodeObject *self)
{
    return fixup(self, fixswapcase);
}

PyDoc_STRVAR(translate__doc__,
"S.translate(table) -> unicode\n\
\n\
Return a copy of the string S, where all characters have been mapped\n\
through the given translation table, which must be a mapping of\n\
Unicode ordinals to Unicode ordinals, Unicode strings or None.\n\
Unmapped characters are left untouched. Characters mapped to None\n\
are deleted.");

static PyObject*
unicode_translate(PyUnicodeObject *self, PyObject *table)
{
    return PyUnicode_TranslateCharmap(self->str,
				      self->length,
				      table,
				      "ignore");
}

PyDoc_STRVAR(upper__doc__,
"S.upper() -> unicode\n\
\n\
Return a copy of S converted to uppercase.");

static PyObject*
unicode_upper(PyUnicodeObject *self)
{
    return fixup(self, fixupper);
}

PyDoc_STRVAR(zfill__doc__,
"S.zfill(width) -> unicode\n\
\n\
Pad a numeric string x with zeros on the left, to fill a field\n\
of the specified width. The string x is never truncated.");

static PyObject *
unicode_zfill(PyUnicodeObject *self, PyObject *args)
{
    Py_ssize_t fill;
    PyUnicodeObject *u;

    Py_ssize_t width;
    if (!PyArg_ParseTuple(args, "n:zfill", &width))
        return NULL;

    if (self->length >= width) {
        if (PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject*) self;
        }
        else
            return PyUnicode_FromUnicode(
                PyUnicode_AS_UNICODE(self),
                PyUnicode_GET_SIZE(self)
            );
    }

    fill = width - self->length;

    u = pad(self, fill, 0, '0');

    if (u == NULL)
        return NULL;

    if (u->str[fill] == '+' || u->str[fill] == '-') {
        /* move sign to beginning of string */
        u->str[0] = u->str[fill];
        u->str[fill] = '0';
    }

    return (PyObject*) u;
}

#if 0
static PyObject*
free_listsize(PyUnicodeObject *self)
{
    return PyInt_FromLong(numfree);
}
#endif

PyDoc_STRVAR(startswith__doc__,
"S.startswith(prefix[, start[, end]]) -> bool\n\
\n\
Return True if S starts with the specified prefix, False otherwise.\n\
With optional start, test S beginning at that position.\n\
With optional end, stop comparing S at that position.\n\
prefix can also be a tuple of strings to try.");

static PyObject *
unicode_startswith(PyUnicodeObject *self,
		   PyObject *args)
{
    PyObject *subobj;
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    int result;

    if (!PyArg_ParseTuple(args, "O|O&O&:startswith", &subobj,
		_PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            substring = (PyUnicodeObject *)PyUnicode_FromObject(
                            PyTuple_GET_ITEM(subobj, i));
            if (substring == NULL)
                return NULL;
            result = tailmatch(self, substring, start, end, -1);
            Py_DECREF(substring);
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        /* nothing matched */
        Py_RETURN_FALSE;
    }
    substring = (PyUnicodeObject *)PyUnicode_FromObject(subobj);
    if (substring == NULL)
         return NULL;
    result = tailmatch(self, substring, start, end, -1);
    Py_DECREF(substring);
    return PyBool_FromLong(result);
}


PyDoc_STRVAR(endswith__doc__,
"S.endswith(suffix[, start[, end]]) -> bool\n\
\n\
Return True if S ends with the specified suffix, False otherwise.\n\
With optional start, test S beginning at that position.\n\
With optional end, stop comparing S at that position.\n\
suffix can also be a tuple of strings to try.");

static PyObject *
unicode_endswith(PyUnicodeObject *self,
		 PyObject *args)
{
    PyObject *subobj;
    PyUnicodeObject *substring;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    int result;

    if (!PyArg_ParseTuple(args, "O|O&O&:endswith", &subobj,
        _PyEval_SliceIndex, &start, _PyEval_SliceIndex, &end))
	return NULL;
    if (PyTuple_Check(subobj)) {
        Py_ssize_t i;
        for (i = 0; i < PyTuple_GET_SIZE(subobj); i++) {
            substring = (PyUnicodeObject *)PyUnicode_FromObject(
                            PyTuple_GET_ITEM(subobj, i));
            if (substring == NULL)
            return NULL;
            result = tailmatch(self, substring, start, end, +1);
            Py_DECREF(substring);
            if (result) {
                Py_RETURN_TRUE;
            }
        }
        Py_RETURN_FALSE;
    }
    substring = (PyUnicodeObject *)PyUnicode_FromObject(subobj);
    if (substring == NULL)
    return NULL;

    result = tailmatch(self, substring, start, end, +1);
    Py_DECREF(substring);
    return PyBool_FromLong(result);
}


/* Implements do_string_format, which is unicode because of stringlib */
#include "stringlib/string_format.h"

PyDoc_STRVAR(format__doc__,
"S.format(*args, **kwargs) -> unicode\n\
\n\
");

PyDoc_STRVAR(p_format__doc__,
"S.__format__(format_spec) -> unicode\n\
\n\
");


static PyObject *
unicode_getnewargs(PyUnicodeObject *v)
{
	return Py_BuildValue("(u#)", v->str, v->length);
}


static PyMethodDef unicode_methods[] = {

    /* Order is according to common usage: often used methods should
       appear first, since lookup is done sequentially. */

    {"encode", (PyCFunction) unicode_encode, METH_VARARGS, encode__doc__},
    {"replace", (PyCFunction) unicode_replace, METH_VARARGS, replace__doc__},
    {"split", (PyCFunction) unicode_split, METH_VARARGS, split__doc__},
    {"rsplit", (PyCFunction) unicode_rsplit, METH_VARARGS, rsplit__doc__},
    {"join", (PyCFunction) unicode_join, METH_O, join__doc__},
    {"capitalize", (PyCFunction) unicode_capitalize, METH_NOARGS, capitalize__doc__},
    {"title", (PyCFunction) unicode_title, METH_NOARGS, title__doc__},
    {"center", (PyCFunction) unicode_center, METH_VARARGS, center__doc__},
    {"count", (PyCFunction) unicode_count, METH_VARARGS, count__doc__},
    {"expandtabs", (PyCFunction) unicode_expandtabs, METH_VARARGS, expandtabs__doc__},
    {"find", (PyCFunction) unicode_find, METH_VARARGS, find__doc__},
    {"partition", (PyCFunction) unicode_partition, METH_O, partition__doc__},
    {"index", (PyCFunction) unicode_index, METH_VARARGS, index__doc__},
    {"ljust", (PyCFunction) unicode_ljust, METH_VARARGS, ljust__doc__},
    {"lower", (PyCFunction) unicode_lower, METH_NOARGS, lower__doc__},
    {"lstrip", (PyCFunction) unicode_lstrip, METH_VARARGS, lstrip__doc__},
    {"decode", (PyCFunction) unicode_decode, METH_VARARGS, decode__doc__},
/*  {"maketrans", (PyCFunction) unicode_maketrans, METH_VARARGS, maketrans__doc__}, */
    {"rfind", (PyCFunction) unicode_rfind, METH_VARARGS, rfind__doc__},
    {"rindex", (PyCFunction) unicode_rindex, METH_VARARGS, rindex__doc__},
    {"rjust", (PyCFunction) unicode_rjust, METH_VARARGS, rjust__doc__},
    {"rstrip", (PyCFunction) unicode_rstrip, METH_VARARGS, rstrip__doc__},
    {"rpartition", (PyCFunction) unicode_rpartition, METH_O, rpartition__doc__},
    {"splitlines", (PyCFunction) unicode_splitlines, METH_VARARGS, splitlines__doc__},
    {"strip", (PyCFunction) unicode_strip, METH_VARARGS, strip__doc__},
    {"swapcase", (PyCFunction) unicode_swapcase, METH_NOARGS, swapcase__doc__},
    {"translate", (PyCFunction) unicode_translate, METH_O, translate__doc__},
    {"upper", (PyCFunction) unicode_upper, METH_NOARGS, upper__doc__},
    {"startswith", (PyCFunction) unicode_startswith, METH_VARARGS, startswith__doc__},
    {"endswith", (PyCFunction) unicode_endswith, METH_VARARGS, endswith__doc__},
    {"islower", (PyCFunction) unicode_islower, METH_NOARGS, islower__doc__},
    {"isupper", (PyCFunction) unicode_isupper, METH_NOARGS, isupper__doc__},
    {"istitle", (PyCFunction) unicode_istitle, METH_NOARGS, istitle__doc__},
    {"isspace", (PyCFunction) unicode_isspace, METH_NOARGS, isspace__doc__},
    {"isdecimal", (PyCFunction) unicode_isdecimal, METH_NOARGS, isdecimal__doc__},
    {"isdigit", (PyCFunction) unicode_isdigit, METH_NOARGS, isdigit__doc__},
    {"isnumeric", (PyCFunction) unicode_isnumeric, METH_NOARGS, isnumeric__doc__},
    {"isalpha", (PyCFunction) unicode_isalpha, METH_NOARGS, isalpha__doc__},
    {"isalnum", (PyCFunction) unicode_isalnum, METH_NOARGS, isalnum__doc__},
    {"zfill", (PyCFunction) unicode_zfill, METH_VARARGS, zfill__doc__},
    {"format", (PyCFunction) do_string_format, METH_VARARGS | METH_KEYWORDS, format__doc__},
    {"__format__", (PyCFunction) unicode__format__, METH_VARARGS, p_format__doc__},
    {"_formatter_field_name_split", (PyCFunction) formatter_field_name_split, METH_NOARGS},
    {"_formatter_parser", (PyCFunction) formatter_parser, METH_NOARGS},
#if 0
    {"capwords", (PyCFunction) unicode_capwords, METH_NOARGS, capwords__doc__},
#endif

#if 0
    /* This one is just used for debugging the implementation. */
    {"freelistsize", (PyCFunction) free_listsize, METH_NOARGS},
#endif

    {"__getnewargs__",	(PyCFunction)unicode_getnewargs, METH_NOARGS},
    {NULL, NULL}
};

static PyObject *
unicode_mod(PyObject *v, PyObject *w)
{
       if (!PyUnicode_Check(v)) {
               Py_INCREF(Py_NotImplemented);
               return Py_NotImplemented;
       }
       return PyUnicode_Format(v, w);
}

static PyNumberMethods unicode_as_number = {
	0,				/*nb_add*/
	0,				/*nb_subtract*/
	0,				/*nb_multiply*/
	0,				/*nb_divide*/
	unicode_mod,			/*nb_remainder*/
};

static PySequenceMethods unicode_as_sequence = {
    (lenfunc) unicode_length, 		/* sq_length */
    PyUnicode_Concat,		 	/* sq_concat */
    (ssizeargfunc) unicode_repeat, 	/* sq_repeat */
    (ssizeargfunc) unicode_getitem, 	/* sq_item */
    (ssizessizeargfunc) unicode_slice, 	/* sq_slice */
    0, 					/* sq_ass_item */
    0, 					/* sq_ass_slice */
    PyUnicode_Contains, 		/* sq_contains */
};

static PyObject*
unicode_subscript(PyUnicodeObject* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyUnicode_GET_SIZE(self);
        return unicode_getitem(self, i);
    } else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;
        Py_UNICODE* source_buf;
        Py_UNICODE* result_buf;
        PyObject* result;

        if (PySlice_GetIndicesEx((PySliceObject*)item, PyUnicode_GET_SIZE(self),
				 &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0) {
            return PyUnicode_FromUnicode(NULL, 0);
        } else if (start == 0 && step == 1 && slicelength == self->length &&
                   PyUnicode_CheckExact(self)) {
            Py_INCREF(self);
            return (PyObject *)self;
        } else if (step == 1) {
            return PyUnicode_FromUnicode(self->str + start, slicelength);
        } else {
            source_buf = PyUnicode_AS_UNICODE((PyObject*)self);
            result_buf = (Py_UNICODE *)PyMem_MALLOC(slicelength*
                                                    sizeof(Py_UNICODE));
	    
	    if (result_buf == NULL)
		    return PyErr_NoMemory();

            for (cur = start, i = 0; i < slicelength; cur += step, i++) {
                result_buf[i] = source_buf[cur];
            }

            result = PyUnicode_FromUnicode(result_buf, slicelength);
            PyMem_FREE(result_buf);
            return result;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "string indices must be integers");
        return NULL;
    }
}

static PyMappingMethods unicode_as_mapping = {
    (lenfunc)unicode_length,		/* mp_length */
    (binaryfunc)unicode_subscript,	/* mp_subscript */
    (objobjargproc)0,			/* mp_ass_subscript */
};

static Py_ssize_t
unicode_buffer_getreadbuf(PyUnicodeObject *self,
			  Py_ssize_t index,
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

static Py_ssize_t
unicode_buffer_getwritebuf(PyUnicodeObject *self, Py_ssize_t index,
			   const void **ptr)
{
    PyErr_SetString(PyExc_TypeError,
		    "cannot use unicode as modifiable buffer");
    return -1;
}

static int
unicode_buffer_getsegcount(PyUnicodeObject *self,
			   Py_ssize_t *lenp)
{
    if (lenp)
        *lenp = PyUnicode_GET_DATA_SIZE(self);
    return 1;
}

static Py_ssize_t
unicode_buffer_getcharbuf(PyUnicodeObject *self,
			  Py_ssize_t index,
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
getnextarg(PyObject *args, Py_ssize_t arglen, Py_ssize_t *p_argidx)
{
    Py_ssize_t argidx = *p_argidx;
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

static Py_ssize_t
strtounicode(Py_UNICODE *buffer, const char *charbuffer)
{
    register Py_ssize_t i;
    Py_ssize_t len = strlen(charbuffer);
    for (i = len - 1; i >= 0; i--)
	buffer[i] = (Py_UNICODE) charbuffer[i];

    return len;
}

static int
doubletounicode(Py_UNICODE *buffer, size_t len, const char *format, double x)
{
    Py_ssize_t result;

    PyOS_ascii_formatd((char *)buffer, len, format, x);
    result = strtounicode(buffer, (char *)buffer);
    return Py_SAFE_DOWNCAST(result, Py_ssize_t, int);
}

static int
longtounicode(Py_UNICODE *buffer, size_t len, const char *format, long x)
{
    Py_ssize_t result;

    PyOS_snprintf((char *)buffer, len, format, x);
    result = strtounicode(buffer, (char *)buffer);
    return Py_SAFE_DOWNCAST(result, Py_ssize_t, int);
}

/* XXX To save some code duplication, formatfloat/long/int could have been
   shared with stringobject.c, converting from 8-bit to Unicode after the
   formatting is done. */

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
    /* Worst case length calc to ensure no buffer overrun:

       'g' formats:
	 fmt = %#.<prec>g
	 buf = '-' + [0-9]*prec + '.' + 'e+' + (longest exp
	    for any double rep.)
	 len = 1 + prec + 1 + 2 + 5 = 9 + prec

       'f' formats:
	 buf = '-' + [0-9]*x + '.' + [0-9]*prec (with x < 50)
	 len = 1 + 50 + 1 + prec = 52 + prec

       If prec=0 the effective precision is 1 (the leading digit is
       always given), therefore increase the length by one.

    */
    if (((type == 'g' || type == 'G') && 
          buflen <= (size_t)10 + (size_t)prec) ||
	(type == 'f' && buflen <= (size_t)53 + (size_t)prec)) {
	PyErr_SetString(PyExc_OverflowError,
			"formatted float is too long (precision too large?)");
	return -1;
    }
    PyOS_snprintf(fmt, sizeof(fmt), "%%%s.%d%c",
		  (flags&F_ALT) ? "#" : "",
		  prec, type);
    return doubletounicode(buf, buflen, fmt, x);
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
	if (!result) {
		Py_DECREF(str);
		return NULL;
	}
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
     * worst case length = 3 + 19 (worst len of INT_MAX on 64-bit machine)
     *                     + 1 + 1
     *                   = 24
     */
    char fmt[64]; /* plenty big enough! */
    char *sign;
    long x;

    x = PyInt_AsLong(v);
    if (x == -1 && PyErr_Occurred())
        return -1;
    if (x < 0 && type == 'u') {
        type = 'd';
    }
    if (x < 0 && (type == 'x' || type == 'X' || type == 'o'))
        sign = "-";
    else
        sign = "";
    if (prec < 0)
        prec = 1;

    /* buf = '+'/'-'/'' + '0'/'0x'/'' + '[0-9]'*max(prec, len(x in octal))
     * worst case buf = '-0x' + [0-9]*prec, where prec >= 11
     */
    if (buflen <= 14 || buflen <= (size_t)3 + (size_t)prec) {
        PyErr_SetString(PyExc_OverflowError,
    	        "formatted integer is too long (precision too large?)");
        return -1;
    }

    if ((flags & F_ALT) &&
        (type == 'x' || type == 'X')) {
        /* When converting under %#x or %#X, there are a number
         * of issues that cause pain:
         * - when 0 is being converted, the C standard leaves off
         *   the '0x' or '0X', which is inconsistent with other
         *   %#x/%#X conversions and inconsistent with Python's
         *   hex() function
         * - there are platforms that violate the standard and
         *   convert 0 with the '0x' or '0X'
         *   (Metrowerks, Compaq Tru64)
         * - there are platforms that give '0x' when converting
         *   under %#X, but convert 0 in accordance with the
         *   standard (OS/2 EMX)
         *
         * We can achieve the desired consistency by inserting our
         * own '0x' or '0X' prefix, and substituting %x/%X in place
         * of %#x/%#X.
         *
         * Note that this is the same approach as used in
         * formatint() in stringobject.c
         */
        PyOS_snprintf(fmt, sizeof(fmt), "%s0%c%%.%dl%c",
                      sign, type, prec, type);
    }
    else {
        PyOS_snprintf(fmt, sizeof(fmt), "%s%%%s.%dl%c",
                      sign, (flags&F_ALT) ? "#" : "",
                      prec, type);
    }
    if (sign[0])
        return longtounicode(buf, buflen, fmt, -x);
    else
        return longtounicode(buf, buflen, fmt, x);
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
#ifdef Py_UNICODE_WIDE
	if (x < 0 || x > 0x10ffff) {
	    PyErr_SetString(PyExc_OverflowError,
			    "%c arg not in range(0x110000) "
			    "(wide Python build)");
	    return -1;
	}
#else
	if (x < 0 || x > 0xffff) {
	    PyErr_SetString(PyExc_OverflowError,
			    "%c arg not in range(0x10000) "
			    "(narrow Python build)");
	    return -1;
	}
#endif
	buf[0] = (Py_UNICODE) x;
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
    Py_ssize_t fmtcnt, rescnt, reslen, arglen, argidx;
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
    if (Py_TYPE(args)->tp_as_mapping && !PyTuple_Check(args) &&
        !PyObject_TypeCheck(args, &PyBaseString_Type))
	dict = args;

    while (--fmtcnt >= 0) {
	if (*fmt != '%') {
	    if (--rescnt < 0) {
		rescnt = fmtcnt + 100;
		reslen += rescnt;
		if (_PyUnicode_Resize(&result, reslen) < 0)
		    goto onError;
		res = PyUnicode_AS_UNICODE(result) + reslen - rescnt;
		--rescnt;
	    }
	    *res++ = *fmt++;
	}
	else {
	    /* Got a format specifier */
	    int flags = 0;
	    Py_ssize_t width = -1;
	    int prec = -1;
	    Py_UNICODE c = '\0';
	    Py_UNICODE fill;
	    PyObject *v = NULL;
	    PyObject *temp = NULL;
	    Py_UNICODE *pbuf;
	    Py_UNICODE sign;
	    Py_ssize_t len;
	    Py_UNICODE formatbuf[FORMATBUFLEN]; /* For format{float,int,char}() */

	    fmt++;
	    if (*fmt == '(') {
		Py_UNICODE *keystart;
		Py_ssize_t keylen;
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
#if 0
		/* keys are converted to strings using UTF-8 and
		   then looked up since Python uses strings to hold
		   variables names etc. in its namespaces and we
		   wouldn't want to break common idioms. */
		key = PyUnicode_EncodeUTF8(keystart,
					   keylen,
					   NULL);
#else
		key = PyUnicode_FromUnicode(keystart, keylen);
#endif
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
			temp = PyObject_Unicode(v);
		    else
			temp = PyObject_Repr(v);
		    if (temp == NULL)
			goto onError;
                    if (PyUnicode_Check(temp))
                        /* nothing to do */;
                    else if (PyString_Check(temp)) {
                        /* convert to string to Unicode */
		        unicode = PyUnicode_Decode(PyString_AS_STRING(temp),
						   PyString_GET_SIZE(temp),
						   NULL,
						   "strict");
		        Py_DECREF(temp);
		        temp = unicode;
		        if (temp == NULL)
			    goto onError;
		    }
		    else {
			Py_DECREF(temp);
			PyErr_SetString(PyExc_TypeError,
					"%s argument has non-string str()");
			goto onError;
		    }
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
		if (PyLong_Check(v)) {
		    temp = formatlong(v, flags, prec, c);
		    if (!temp)
			goto onError;
		    pbuf = PyUnicode_AS_UNICODE(temp);
		    len = PyUnicode_GET_SIZE(temp);
		    sign = 1;
		}
		else {
		    pbuf = formatbuf;
		    len = formatint(pbuf, sizeof(formatbuf)/sizeof(Py_UNICODE),
				    flags, prec, c, v);
		    if (len < 0)
			goto onError;
		    sign = 1;
		}
		if (flags & F_ZERO)
		    fill = '0';
		break;

	    case 'e':
	    case 'E':
	    case 'f':
	    case 'F':
	    case 'g':
	    case 'G':
		if (c == 'F')
			c = 'f';
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
			     "unsupported format character '%c' (0x%x) "
			     "at index %zd",
			     (31<=c && c<=126) ? (char)c : '?',
                             (int)c,
			     (Py_ssize_t)(fmt - 1 -
					  PyUnicode_AS_UNICODE(uformat)));
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
	    if (rescnt - (sign != 0) < width) {
		reslen -= rescnt;
		rescnt = width + fmtcnt + 100;
		reslen += rescnt;
		if (reslen < 0) {
		    Py_XDECREF(temp);
		    PyErr_NoMemory();
		    goto onError;
		}
		if (_PyUnicode_Resize(&result, reslen) < 0) {
		    Py_XDECREF(temp);
		    goto onError;
		}
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
	    Py_UNICODE_COPY(res, pbuf, len);
	    res += len;
	    rescnt -= len;
	    while (--width >= len) {
		--rescnt;
		*res++ = ' ';
	    }
	    if (dict && (argidx < arglen) && c != '%') {
		PyErr_SetString(PyExc_TypeError,
				"not all arguments converted during string formatting");
                Py_XDECREF(temp);
		goto onError;
	    }
	    Py_XDECREF(temp);
	} /* '%' */
    } /* until end */
    if (argidx < arglen && !dict) {
	PyErr_SetString(PyExc_TypeError,
			"not all arguments converted during string formatting");
	goto onError;
    }

    if (_PyUnicode_Resize(&result, reslen - rescnt) < 0)
	goto onError;
    if (args_owned) {
	Py_DECREF(args);
    }
    Py_DECREF(uformat);
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
    (readbufferproc) unicode_buffer_getreadbuf,
    (writebufferproc) unicode_buffer_getwritebuf,
    (segcountproc) unicode_buffer_getsegcount,
    (charbufferproc) unicode_buffer_getcharbuf,
};

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyObject *
unicode_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
        PyObject *x = NULL;
	static char *kwlist[] = {"string", "encoding", "errors", 0};
	char *encoding = NULL;
	char *errors = NULL;

	if (type != &PyUnicode_Type)
		return unicode_subtype_new(type, args, kwds);
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oss:unicode",
					  kwlist, &x, &encoding, &errors))
	    return NULL;
	if (x == NULL)
		return (PyObject *)_PyUnicode_New(0);
	if (encoding == NULL && errors == NULL)
	    return PyObject_Unicode(x);
	else
	return PyUnicode_FromEncodedObject(x, encoding, errors);
}

static PyObject *
unicode_subtype_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyUnicodeObject *tmp, *pnew;
	Py_ssize_t n;

	assert(PyType_IsSubtype(type, &PyUnicode_Type));
	tmp = (PyUnicodeObject *)unicode_new(&PyUnicode_Type, args, kwds);
	if (tmp == NULL)
		return NULL;
	assert(PyUnicode_Check(tmp));
	pnew = (PyUnicodeObject *) type->tp_alloc(type, n = tmp->length);
	if (pnew == NULL) {
		Py_DECREF(tmp);
		return NULL;
	}
	pnew->str = PyMem_NEW(Py_UNICODE, n+1);
	if (pnew->str == NULL) {
		_Py_ForgetReference((PyObject *)pnew);
		PyObject_Del(pnew);
		Py_DECREF(tmp);
		return PyErr_NoMemory();
	}
	Py_UNICODE_COPY(pnew->str, tmp->str, n+1);
	pnew->length = n;
	pnew->hash = tmp->hash;
	Py_DECREF(tmp);
	return (PyObject *)pnew;
}

PyDoc_STRVAR(unicode_doc,
"unicode(string [, encoding[, errors]]) -> object\n\
\n\
Create a new Unicode object from the given encoded string.\n\
encoding defaults to the current default string encoding.\n\
errors can be 'strict', 'replace' or 'ignore' and defaults to 'strict'.");

PyTypeObject PyUnicode_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "unicode", 				/* tp_name */
    sizeof(PyUnicodeObject), 		/* tp_size */
    0, 					/* tp_itemsize */
    /* Slots */
    (destructor)unicode_dealloc, 	/* tp_dealloc */
    0, 					/* tp_print */
    0,				 	/* tp_getattr */
    0, 					/* tp_setattr */
    0, 					/* tp_compare */
    unicode_repr, 			/* tp_repr */
    &unicode_as_number, 		/* tp_as_number */
    &unicode_as_sequence, 		/* tp_as_sequence */
    &unicode_as_mapping, 		/* tp_as_mapping */
    (hashfunc) unicode_hash, 		/* tp_hash*/
    0, 					/* tp_call*/
    (reprfunc) unicode_str,	 	/* tp_str */
    PyObject_GenericGetAttr, 		/* tp_getattro */
    0,			 		/* tp_setattro */
    &unicode_as_buffer,			/* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES |
	    Py_TPFLAGS_BASETYPE | Py_TPFLAGS_UNICODE_SUBCLASS,	/* tp_flags */
    unicode_doc,			/* tp_doc */
    0,					/* tp_traverse */
    0,					/* tp_clear */
    PyUnicode_RichCompare,		/* tp_richcompare */
    0,					/* tp_weaklistoffset */
    0,					/* tp_iter */
    0,					/* tp_iternext */
    unicode_methods,			/* tp_methods */
    0,					/* tp_members */
    0,					/* tp_getset */
    &PyBaseString_Type,			/* tp_base */
    0,					/* tp_dict */
    0,					/* tp_descr_get */
    0,					/* tp_descr_set */
    0,					/* tp_dictoffset */
    0,					/* tp_init */
    0,					/* tp_alloc */
    unicode_new,			/* tp_new */
    PyObject_Del,      		/* tp_free */
};

/* Initialize the Unicode implementation */

void _PyUnicode_Init(void)
{
    int i;

    /* XXX - move this array to unicodectype.c ? */
    Py_UNICODE linebreak[] = {
        0x000A, /* LINE FEED */
        0x000D, /* CARRIAGE RETURN */
        0x001C, /* FILE SEPARATOR */
        0x001D, /* GROUP SEPARATOR */
        0x001E, /* RECORD SEPARATOR */
        0x0085, /* NEXT LINE */
        0x2028, /* LINE SEPARATOR */
        0x2029, /* PARAGRAPH SEPARATOR */
    };

    /* Init the implementation */
    free_list = NULL;
    numfree = 0;
    unicode_empty = _PyUnicode_New(0);
    if (!unicode_empty)
	return;

    strcpy(unicode_default_encoding, "ascii");
    for (i = 0; i < 256; i++)
	unicode_latin1[i] = NULL;
    if (PyType_Ready(&PyUnicode_Type) < 0)
	Py_FatalError("Can't initialize 'unicode'");

    /* initialize the linebreak bloom filter */
    bloom_linebreak = make_bloom_mask(
        linebreak, sizeof(linebreak) / sizeof(linebreak[0])
        );

    PyType_Ready(&EncodingMapType);
}

/* Finalize the Unicode implementation */

int
PyUnicode_ClearFreeList(void)
{
    int freelist_size = numfree;
    PyUnicodeObject *u;

    for (u = free_list; u != NULL;) {
	PyUnicodeObject *v = u;
	u = *(PyUnicodeObject **)u;
	if (v->str)
	    PyMem_DEL(v->str);
	Py_XDECREF(v->defenc);
	PyObject_Del(v);
	numfree--;
    }
    free_list = NULL;
    assert(numfree == 0);
    return freelist_size;
}

void
_PyUnicode_Fini(void)
{
    int i;

    Py_XDECREF(unicode_empty);
    unicode_empty = NULL;

    for (i = 0; i < 256; i++) {
	if (unicode_latin1[i]) {
	    Py_DECREF(unicode_latin1[i]);
	    unicode_latin1[i] = NULL;
	}
    }
    (void)PyUnicode_ClearFreeList();
}

#ifdef __cplusplus
}
#endif


/*
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
