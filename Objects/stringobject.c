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

/* String object implementation */

#include "Python.h"

#include "mymath.h"
#include <ctype.h>

#ifdef COUNT_ALLOCS
int null_strings, one_strings;
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#else
#ifndef UCHAR_MAX
#define UCHAR_MAX 255
#endif
#endif

static PyStringObject *characters[UCHAR_MAX + 1];
#ifndef DONT_SHARE_SHORT_STRINGS
static PyStringObject *nullstring;
#endif

/*
   Newsizedstringobject() and newstringobject() try in certain cases
   to share string objects.  When the size of the string is zero,
   these routines always return a pointer to the same string object;
   when the size is one, they return a pointer to an already existing
   object if the contents of the string is known.  For
   newstringobject() this is always the case, for
   newsizedstringobject() this is the case when the first argument in
   not NULL.
   A common practice to allocate a string and then fill it in or
   change it must be done carefully.  It is only allowed to change the
   contents of the string if the obect was gotten from
   newsizedstringobject() with a NULL first argument, because in the
   future these routines may try to do even more sharing of objects.
*/
PyObject *
PyString_FromStringAndSize(str, size)
	const char *str;
	int size;
{
	register PyStringObject *op;
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
	if (size == 1 && str != NULL &&
	    (op = characters[*str & UCHAR_MAX]) != NULL)
	{
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */
	op = (PyStringObject *)
		malloc(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	op->ob_type = &PyString_Type;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	_Py_NewReference(op);
	if (str != NULL)
		memcpy(op->ob_sval, str, size);
	op->ob_sval[size] = '\0';
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		Py_INCREF(op);
	} else if (size == 1 && str != NULL) {
		characters[*str & UCHAR_MAX] = op;
		Py_INCREF(op);
	}
#endif
	return (PyObject *) op;
}

PyObject *
PyString_FromString(str)
	const char *str;
{
	register unsigned int size = strlen(str);
	register PyStringObject *op;
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0 && (op = nullstring) != NULL) {
#ifdef COUNT_ALLOCS
		null_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
	if (size == 1 && (op = characters[*str & UCHAR_MAX]) != NULL) {
#ifdef COUNT_ALLOCS
		one_strings++;
#endif
		Py_INCREF(op);
		return (PyObject *)op;
	}
#endif /* DONT_SHARE_SHORT_STRINGS */
	op = (PyStringObject *)
		malloc(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	op->ob_type = &PyString_Type;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	_Py_NewReference(op);
	strcpy(op->ob_sval, str);
#ifndef DONT_SHARE_SHORT_STRINGS
	if (size == 0) {
		nullstring = op;
		Py_INCREF(op);
	} else if (size == 1) {
		characters[*str & UCHAR_MAX] = op;
		Py_INCREF(op);
	}
#endif
	return (PyObject *) op;
}

static void
string_dealloc(op)
	PyObject *op;
{
	PyMem_DEL(op);
}

int
PyString_Size(op)
	register PyObject *op;
{
	if (!PyString_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return ((PyStringObject *)op) -> ob_size;
}

/*const*/ char *
PyString_AsString(op)
	register PyObject *op;
{
	if (!PyString_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return ((PyStringObject *)op) -> ob_sval;
}

/* Methods */

static int
string_print(op, fp, flags)
	PyStringObject *op;
	FILE *fp;
	int flags;
{
	int i;
	char c;
	int quote;
	/* XXX Ought to check for interrupts when writing long strings */
	if (flags & Py_PRINT_RAW) {
		fwrite(op->ob_sval, 1, (int) op->ob_size, fp);
		return 0;
	}

	/* figure out which quote to use; single is prefered */
	quote = '\'';
	if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
		quote = '"';

	fputc(quote, fp);
	for (i = 0; i < op->ob_size; i++) {
		c = op->ob_sval[i];
		if (c == quote || c == '\\')
			fprintf(fp, "\\%c", c);
		else if (c < ' ' || c >= 0177)
			fprintf(fp, "\\%03o", c & 0377);
		else
			fputc(c, fp);
	}
	fputc(quote, fp);
	return 0;
}

static PyObject *
string_repr(op)
	register PyStringObject *op;
{
	/* XXX overflow? */
	int newsize = 2 + 4 * op->ob_size * sizeof(char);
	PyObject *v = PyString_FromStringAndSize((char *)NULL, newsize);
	if (v == NULL) {
		return NULL;
	}
	else {
		register int i;
		register char c;
		register char *p;
		int quote;

		/* figure out which quote to use; single is prefered */
		quote = '\'';
		if (strchr(op->ob_sval, '\'') && !strchr(op->ob_sval, '"'))
			quote = '"';

		p = ((PyStringObject *)v)->ob_sval;
		*p++ = quote;
		for (i = 0; i < op->ob_size; i++) {
			c = op->ob_sval[i];
			if (c == quote || c == '\\')
				*p++ = '\\', *p++ = c;
			else if (c < ' ' || c >= 0177) {
				sprintf(p, "\\%03o", c & 0377);
				while (*p != '\0')
					p++;
			}
			else
				*p++ = c;
		}
		*p++ = quote;
		*p = '\0';
		_PyString_Resize(
			&v, (int) (p - ((PyStringObject *)v)->ob_sval));
		return v;
	}
}

static int
string_length(a)
	PyStringObject *a;
{
	return a->ob_size;
}

static PyObject *
string_concat(a, bb)
	register PyStringObject *a;
	register PyObject *bb;
{
	register unsigned int size;
	register PyStringObject *op;
	if (!PyString_Check(bb)) {
		PyErr_BadArgument();
		return NULL;
	}
#define b ((PyStringObject *)bb)
	/* Optimize cases with empty left or right operand */
	if (a->ob_size == 0) {
		Py_INCREF(bb);
		return bb;
	}
	if (b->ob_size == 0) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	size = a->ob_size + b->ob_size;
	op = (PyStringObject *)
		malloc(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	op->ob_type = &PyString_Type;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	_Py_NewReference(op);
	memcpy(op->ob_sval, a->ob_sval, (int) a->ob_size);
	memcpy(op->ob_sval + a->ob_size, b->ob_sval, (int) b->ob_size);
	op->ob_sval[size] = '\0';
	return (PyObject *) op;
#undef b
}

static PyObject *
string_repeat(a, n)
	register PyStringObject *a;
	register int n;
{
	register int i;
	register int size;
	register PyStringObject *op;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	if (size == a->ob_size) {
		Py_INCREF(a);
		return (PyObject *)a;
	}
	op = (PyStringObject *)
		malloc(sizeof(PyStringObject) + size * sizeof(char));
	if (op == NULL)
		return PyErr_NoMemory();
	op->ob_type = &PyString_Type;
	op->ob_size = size;
#ifdef CACHE_HASH
	op->ob_shash = -1;
#endif
#ifdef INTERN_STRINGS
	op->ob_sinterned = NULL;
#endif
	_Py_NewReference(op);
	for (i = 0; i < size; i += a->ob_size)
		memcpy(op->ob_sval+i, a->ob_sval, (int) a->ob_size);
	op->ob_sval[size] = '\0';
	return (PyObject *) op;
}

/* String slice a[i:j] consists of characters a[i] ... a[j-1] */

static PyObject *
string_slice(a, i, j)
	register PyStringObject *a;
	register int i, j; /* May be negative! */
{
	if (i < 0)
		i = 0;
	if (j < 0)
		j = 0; /* Avoid signed/unsigned bug in next line */
	if (j > a->ob_size)
		j = a->ob_size;
	if (i == 0 && j == a->ob_size) { /* It's the same as a */
		Py_INCREF(a);
		return (PyObject *)a;
	}
	if (j < i)
		j = i;
	return PyString_FromStringAndSize(a->ob_sval + i, (int) (j-i));
}

static PyObject *
string_item(a, i)
	PyStringObject *a;
	register int i;
{
	int c;
	PyObject *v;
	if (i < 0 || i >= a->ob_size) {
		PyErr_SetString(PyExc_IndexError, "string index out of range");
		return NULL;
	}
	c = a->ob_sval[i] & UCHAR_MAX;
	v = (PyObject *) characters[c];
#ifdef COUNT_ALLOCS
	if (v != NULL)
		one_strings++;
#endif
	if (v == NULL) {
		v = PyString_FromStringAndSize((char *)NULL, 1);
		if (v == NULL)
			return NULL;
		characters[c] = (PyStringObject *) v;
		((PyStringObject *)v)->ob_sval[0] = c;
	}
	Py_INCREF(v);
	return v;
}

static int
string_compare(a, b)
	PyStringObject *a, *b;
{
	int len_a = a->ob_size, len_b = b->ob_size;
	int min_len = (len_a < len_b) ? len_a : len_b;
	int cmp;
	if (min_len > 0) {
		cmp = Py_CHARMASK(*a->ob_sval) - Py_CHARMASK(*b->ob_sval);
		if (cmp == 0)
			cmp = memcmp(a->ob_sval, b->ob_sval, min_len);
		if (cmp != 0)
			return cmp;
	}
	return (len_a < len_b) ? -1 : (len_a > len_b) ? 1 : 0;
}

static long
string_hash(a)
	PyStringObject *a;
{
	register int len;
	register unsigned char *p;
	register long x;

#ifdef CACHE_HASH
	if (a->ob_shash != -1)
		return a->ob_shash;
#ifdef INTERN_STRINGS
	if (a->ob_sinterned != NULL)
		return (a->ob_shash =
			((PyStringObject *)(a->ob_sinterned))->ob_shash);
#endif
#endif
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= a->ob_size;
	if (x == -1)
		x = -2;
#ifdef CACHE_HASH
	a->ob_shash = x;
#endif
	return x;
}

static int
string_buffer_getreadbuf(self, index, ptr)
	PyStringObject *self;
	int index;
	const void **ptr;
{
	if ( index != 0 ) {
		PyErr_SetString(PyExc_SystemError,
				"Accessing non-existent string segment");
		return -1;
	}
	*ptr = (void *)self->ob_sval;
	return self->ob_size;
}

static int
string_buffer_getwritebuf(self, index, ptr)
	PyStringObject *self;
	int index;
	const void **ptr;
{
	PyErr_SetString(PyExc_TypeError,
			"Cannot use string as modifyable buffer");
	return -1;
}

static int
string_buffer_getsegcount(self, lenp)
	PyStringObject *self;
	int *lenp;
{
	if ( lenp )
		*lenp = self->ob_size;
	return 1;
}

static PySequenceMethods string_as_sequence = {
	(inquiry)string_length, /*sq_length*/
	(binaryfunc)string_concat, /*sq_concat*/
	(intargfunc)string_repeat, /*sq_repeat*/
	(intargfunc)string_item, /*sq_item*/
	(intintargfunc)string_slice, /*sq_slice*/
	0,		/*sq_ass_item*/
	0,		/*sq_ass_slice*/
};

static PyBufferProcs string_as_buffer = {
	(getreadbufferproc)string_buffer_getreadbuf,
	(getwritebufferproc)string_buffer_getwritebuf,
	(getsegcountproc)string_buffer_getsegcount,
};

PyTypeObject PyString_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"string",
	sizeof(PyStringObject),
	sizeof(char),
	(destructor)string_dealloc, /*tp_dealloc*/
	(printfunc)string_print, /*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)string_compare, /*tp_compare*/
	(reprfunc)string_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&string_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)string_hash, /*tp_hash*/
	0,		/*tp_call*/
	0,		/*tp_str*/
	0,		/*tp_getattro*/
	0,		/*tp_setattro*/
	&string_as_buffer,	/*tp_as_buffer*/
	0,		/*tp_xxx4*/
	0,		/*tp_doc*/
};

void
PyString_Concat(pv, w)
	register PyObject **pv;
	register PyObject *w;
{
	register PyObject *v;
	if (*pv == NULL)
		return;
	if (w == NULL || !PyString_Check(*pv)) {
		Py_DECREF(*pv);
		*pv = NULL;
		return;
	}
	v = string_concat((PyStringObject *) *pv, w);
	Py_DECREF(*pv);
	*pv = v;
}

void
PyString_ConcatAndDel(pv, w)
	register PyObject **pv;
	register PyObject *w;
{
	PyString_Concat(pv, w);
	Py_XDECREF(w);
}


/* The following function breaks the notion that strings are immutable:
   it changes the size of a string.  We get away with this only if there
   is only one module referencing the object.  You can also think of it
   as creating a new string object and destroying the old one, only
   more efficiently.  In any case, don't use this if the string may
   already be known to some other part of the code... */

int
_PyString_Resize(pv, newsize)
	PyObject **pv;
	int newsize;
{
	register PyObject *v;
	register PyStringObject *sv;
	v = *pv;
	if (!PyString_Check(v) || v->ob_refcnt != 1) {
		*pv = 0;
		Py_DECREF(v);
		PyErr_BadInternalCall();
		return -1;
	}
	/* XXX UNREF/NEWREF interface should be more symmetrical */
#ifdef Py_REF_DEBUG
	--_Py_RefTotal;
#endif
	_Py_ForgetReference(v);
	*pv = (PyObject *)
		realloc((char *)v,
			sizeof(PyStringObject) + newsize * sizeof(char));
	if (*pv == NULL) {
		PyMem_DEL(v);
		PyErr_NoMemory();
		return -1;
	}
	_Py_NewReference(*pv);
	sv = (PyStringObject *) *pv;
	sv->ob_size = newsize;
	sv->ob_sval[newsize] = '\0';
	return 0;
}

/* Helpers for formatstring */

static PyObject *
getnextarg(args, arglen, p_argidx)
	PyObject *args;
	int arglen;
	int *p_argidx;
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

static int
formatfloat(buf, flags, prec, type, v)
	char *buf;
	int flags;
	int prec;
	int type;
	PyObject *v;
{
	char fmt[20];
	double x;
	if (!PyArg_Parse(v, "d;float argument required", &x))
		return -1;
	if (prec < 0)
		prec = 6;
	if (prec > 50)
		prec = 50; /* Arbitrary limitation */
	if (type == 'f' && fabs(x)/1e25 >= 1e25)
		type = 'g';
	sprintf(fmt, "%%%s.%d%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return strlen(buf);
}

static int
formatint(buf, flags, prec, type, v)
	char *buf;
	int flags;
	int prec;
	int type;
	PyObject *v;
{
	char fmt[20];
	long x;
	if (!PyArg_Parse(v, "l;int argument required", &x))
		return -1;
	if (prec < 0)
		prec = 1;
	sprintf(fmt, "%%%s.%dl%c", (flags&F_ALT) ? "#" : "", prec, type);
	sprintf(buf, fmt, x);
	return strlen(buf);
}

static int
formatchar(buf, v)
	char *buf;
	PyObject *v;
{
	if (PyString_Check(v)) {
		if (!PyArg_Parse(v, "c;%c requires int or char", &buf[0]))
			return -1;
	}
	else {
		if (!PyArg_Parse(v, "b;%c requires int or char", &buf[0]))
			return -1;
	}
	buf[1] = '\0';
	return 1;
}


/* fmt%(v1,v2,...) is roughly equivalent to sprintf(fmt, v1, v2, ...) */

PyObject *
PyString_Format(format, args)
	PyObject *format;
	PyObject *args;
{
	char *fmt, *res;
	int fmtcnt, rescnt, reslen, arglen, argidx;
	int args_owned = 0;
	PyObject *result;
	PyObject *dict = NULL;
	if (format == NULL || !PyString_Check(format) || args == NULL) {
		PyErr_BadInternalCall();
		return NULL;
	}
	fmt = PyString_AsString(format);
	fmtcnt = PyString_Size(format);
	reslen = rescnt = fmtcnt + 100;
	result = PyString_FromStringAndSize((char *)NULL, reslen);
	if (result == NULL)
		return NULL;
	res = PyString_AsString(result);
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
				if (_PyString_Resize(&result, reslen) < 0)
					return NULL;
				res = PyString_AsString(result)
					+ reslen - rescnt;
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
			int c = '\0';
			int fill;
			PyObject *v = NULL;
			PyObject *temp = NULL;
			char *buf;
			int sign;
			int len;
			char tmpbuf[120]; /* For format{float,int,char}() */
			fmt++;
			if (*fmt == '(') {
				char *keystart;
				int keylen;
				PyObject *key;
				int pcount = 1;

				if (dict == NULL) {
					PyErr_SetString(PyExc_TypeError,
						 "format requires a mapping"); 
					goto error;
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
					goto error;
				}
				key = PyString_FromStringAndSize(keystart,
								 keylen);
				if (key == NULL)
					goto error;
				if (args_owned) {
					Py_DECREF(args);
					args_owned = 0;
				}
				args = PyObject_GetItem(dict, key);
				Py_DECREF(key);
				if (args == NULL) {
					goto error;
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
					goto error;
				if (!PyInt_Check(v)) {
					PyErr_SetString(PyExc_TypeError,
							"* wants int");
					goto error;
				}
				width = PyInt_AsLong(v);
				if (width < 0)
					width = 0;
				if (--fmtcnt >= 0)
					c = *fmt++;
			}
			else if (c >= 0 && isdigit(c)) {
				width = c - '0';
				while (--fmtcnt >= 0) {
					c = Py_CHARMASK(*fmt++);
					if (!isdigit(c))
						break;
					if ((width*10) / 10 != width) {
						PyErr_SetString(
							PyExc_ValueError,
							"width too big");
						goto error;
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
						goto error;
					if (!PyInt_Check(v)) {
						PyErr_SetString(
							PyExc_TypeError,
							"* wants int");
						goto error;
					}
					prec = PyInt_AsLong(v);
					if (prec < 0)
						prec = 0;
					if (--fmtcnt >= 0)
						c = *fmt++;
				}
				else if (c >= 0 && isdigit(c)) {
					prec = c - '0';
					while (--fmtcnt >= 0) {
						c = Py_CHARMASK(*fmt++);
						if (!isdigit(c))
							break;
						if ((prec*10) / 10 != prec) {
							PyErr_SetString(
							    PyExc_ValueError,
							    "prec too big");
							goto error;
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
				goto error;
			}
			if (c != '%') {
				v = getnextarg(args, arglen, &argidx);
				if (v == NULL)
					goto error;
			}
			sign = 0;
			fill = ' ';
			switch (c) {
			case '%':
				buf = "%";
				len = 1;
				break;
			case 's':
				temp = PyObject_Str(v);
				if (temp == NULL)
					goto error;
				if (!PyString_Check(temp)) {
					PyErr_SetString(PyExc_TypeError,
					  "%s argument has non-string str()");
					goto error;
				}
				buf = PyString_AsString(temp);
				len = PyString_Size(temp);
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
				buf = tmpbuf;
				len = formatint(buf, flags, prec, c, v);
				if (len < 0)
					goto error;
				sign = (c == 'd');
				if (flags&F_ZERO) {
					fill = '0';
					if ((flags&F_ALT) &&
					    (c == 'x' || c == 'X') &&
					    buf[0] == '0' && buf[1] == c) {
						*res++ = *buf++;
						*res++ = *buf++;
						rescnt -= 2;
						len -= 2;
						width -= 2;
						if (width < 0)
							width = 0;
					}
				}
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				buf = tmpbuf;
				len = formatfloat(buf, flags, prec, c, v);
				if (len < 0)
					goto error;
				sign = 1;
				if (flags&F_ZERO)
					fill = '0';
				break;
			case 'c':
				buf = tmpbuf;
				len = formatchar(buf, v);
				if (len < 0)
					goto error;
				break;
			default:
				PyErr_Format(PyExc_ValueError,
				"unsupported format character '%c' (0x%x)",
					c, c);
				goto error;
			}
			if (sign) {
				if (*buf == '-' || *buf == '+') {
					sign = *buf++;
					len--;
				}
				else if (flags & F_SIGN)
					sign = '+';
				else if (flags & F_BLANK)
					sign = ' ';
				else
					sign = '\0';
			}
			if (width < len)
				width = len;
			if (rescnt < width + (sign != '\0')) {
				reslen -= rescnt;
				rescnt = width + fmtcnt + 100;
				reslen += rescnt;
				if (_PyString_Resize(&result, reslen) < 0)
					return NULL;
				res = PyString_AsString(result)
					+ reslen - rescnt;
			}
			if (sign) {
				if (fill != ' ')
					*res++ = sign;
				rescnt--;
				if (width > len)
					width--;
			}
			if (width > len && !(flags&F_LJUST)) {
				do {
					--rescnt;
					*res++ = fill;
				} while (--width > len);
			}
			if (sign && fill == ' ')
				*res++ = sign;
			memcpy(res, buf, len);
			res += len;
			rescnt -= len;
			while (--width >= len) {
				--rescnt;
				*res++ = ' ';
			}
                        if (dict && (argidx < arglen) && c != '%') {
                                PyErr_SetString(PyExc_TypeError,
                                           "not all arguments converted");
                                goto error;
                        }
			Py_XDECREF(temp);
		} /* '%' */
	} /* until end */
	if (argidx < arglen && !dict) {
		PyErr_SetString(PyExc_TypeError,
				"not all arguments converted");
		goto error;
	}
	if (args_owned) {
		Py_DECREF(args);
	}
	_PyString_Resize(&result, reslen - rescnt);
	return result;
 error:
	Py_DECREF(result);
	if (args_owned) {
		Py_DECREF(args);
	}
	return NULL;
}


#ifdef INTERN_STRINGS

static PyObject *interned;

void
PyString_InternInPlace(p)
	PyObject **p;
{
	register PyStringObject *s = (PyStringObject *)(*p);
	PyObject *t;
	if (s == NULL || !PyString_Check(s))
		Py_FatalError("PyString_InternInPlace: strings only please!");
	if ((t = s->ob_sinterned) != NULL) {
		if (t == (PyObject *)s)
			return;
		Py_INCREF(t);
		*p = t;
		Py_DECREF(s);
		return;
	}
	if (interned == NULL) {
		interned = PyDict_New();
		if (interned == NULL)
			return;
	}
	if ((t = PyDict_GetItem(interned, (PyObject *)s)) != NULL) {
		Py_INCREF(t);
		*p = s->ob_sinterned = t;
		Py_DECREF(s);
		return;
	}
	t = (PyObject *)s;
	if (PyDict_SetItem(interned, t, t) == 0) {
		s->ob_sinterned = t;
		return;
	}
	PyErr_Clear();
}


PyObject *
PyString_InternFromString(cp)
	const char *cp;
{
	PyObject *s = PyString_FromString(cp);
	if (s == NULL)
		return NULL;
	PyString_InternInPlace(&s);
	return s;
}

#endif

void
PyString_Fini()
{
	int i;
	for (i = 0; i < UCHAR_MAX + 1; i++) {
		Py_XDECREF(characters[i]);
		characters[i] = NULL;
	}
#ifndef DONT_SHARE_SHORT_STRINGS
	Py_XDECREF(nullstring);
	nullstring = NULL;
#endif
#ifdef INTERN_STRINGS
	if (interned) {
		int pos, changed;
		PyObject *key, *value;
		do {
			changed = 0;
			pos = 0;
			while (PyDict_Next(interned, &pos, &key, &value)) {
				if (key->ob_refcnt == 2 && key == value) {
					PyDict_DelItem(interned, key);
					changed = 1;
				}
			}
		} while (changed);
	}
#endif
}
