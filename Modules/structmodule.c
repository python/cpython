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

/* struct module -- pack values into and (out of) strings */

/* New version supporting byte order, alignment and size options,
   character strings, and unsigned numbers */

#include "Python.h"

#include <limits.h>


/* Exception */

static PyObject *StructError;


/* Define various structs to figure out the alignments of types */

#ifdef __MWERKS__
/*
** XXXX We have a problem here. There are no unique alignment rules
** on the PowerPC mac. 
*/
#ifdef __powerc
#pragma options align=mac68k
#endif
#endif /* __MWERKS__ */

typedef struct { char c; short x; } s_short;
typedef struct { char c; int x; } s_int;
typedef struct { char c; long x; } s_long;
typedef struct { char c; float x; } s_float;
typedef struct { char c; double x; } s_double;

#define SHORT_ALIGN (sizeof(s_short) - sizeof(short))
#define INT_ALIGN (sizeof(s_int) - sizeof(int))
#define LONG_ALIGN (sizeof(s_long) - sizeof(long))
#define FLOAT_ALIGN (sizeof(s_float) - sizeof(float))
#define DOUBLE_ALIGN (sizeof(s_double) - sizeof(double))

#ifdef __powerc
#pragma options align=reset
#endif


/* Helper routine to turn a (signed) long into an unsigned long */
/* XXX This assumes 2's complement arithmetic */

static PyObject *
make_ulong(x)
	long x;
{
	PyObject *v = PyLong_FromLong(x);
	if (x < 0 && v != NULL) {
		static PyObject *offset = NULL;
		PyObject *result = NULL;
		if (offset == NULL) {
			PyObject *one = PyLong_FromLong(1L);
			PyObject *shiftcount =
				PyLong_FromLong(8 * sizeof(long));
			if (one == NULL || shiftcount == NULL)
				goto bad;
			offset = PyNumber_Lshift(one, shiftcount);
		  bad:
			Py_XDECREF(one);
			Py_XDECREF(shiftcount);
			if (offset == NULL)
				goto finally;
		}
		result = PyNumber_Add(v, offset);
	  finally:
		Py_DECREF(v);
		return result;
	}
	else
		return v;
}

/* Helper routine to get a Python integer and raise the appropriate error
   if it isn't one */

static int
get_long(v, p)
	PyObject *v;
	long *p;
{
	long x = PyInt_AsLong(v);
	if (x == -1 && PyErr_Occurred()) {
		if (PyErr_Occurred() == PyExc_TypeError)
			PyErr_SetString(StructError,
					"required argument is not an integer");
		return -1;
	}
	*p = x;
	return 0;
}


/* The translation function for each format character is table driven */

typedef struct _formatdef {
	char format;
	int size;
	int alignment;
	PyObject* (*unpack) Py_PROTO((const char *,
				      const struct _formatdef *));
	int (*pack) Py_PROTO((char *,
			      PyObject *,
			      const struct _formatdef *));
} formatdef;

static PyObject *
nu_char(p, f)
	const char *p;
	const formatdef *f;
{
	return PyString_FromStringAndSize(p, 1);
}

static PyObject *
nu_byte(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong((long) *(signed char *)p);
}

static PyObject *
nu_ubyte(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong((long) *(unsigned char *)p);
}

static PyObject *
nu_short(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong((long) *(short *)p);
}

static PyObject *
nu_ushort(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong((long) *(unsigned short *)p);
}

static PyObject *
nu_int(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong((long) *(int *)p);
}

static PyObject *
nu_uint(p, f)
	const char *p;
	const formatdef *f;
{
	unsigned int x = *(unsigned int *)p;
#if INT_MAX == LONG_MAX
	return make_ulong((long)x);
#else
	return PyInt_FromLong((long)x);
#endif
}

static PyObject *
nu_long(p, f)
	const char *p;
	const formatdef *f;
{
	return PyInt_FromLong(*(long *)p);
}

static PyObject *
nu_ulong(p, f)
	const char *p;
	const formatdef *f;
{
	return make_ulong(*(long *)p);
}

static PyObject *
nu_float(p, f)
	const char *p;
	const formatdef *f;
{
	float x;
	memcpy((char *)&x, p, sizeof(float));
	return PyFloat_FromDouble((double)x);
}

static PyObject *
nu_double(p, f)
	const char *p;
	const formatdef *f;
{
	double x;
	memcpy((char *)&x, p, sizeof(double));
	return PyFloat_FromDouble(x);
}

static int
np_byte(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	*p = x;
	return 0;
}

static int
np_char(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	if (!PyString_Check(v) || PyString_Size(v) != 1) {
		PyErr_SetString(StructError,
				"char format require string of length 1");
		return -1;
	}
	*p = *PyString_AsString(v);
	return 0;
}

static int
np_short(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	* (short *)p = x;
	return 0;
}

static int
np_int(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	* (int *)p = x;
	return 0;
}

static int
np_long(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	if (get_long(v, &x) < 0)
		return -1;
	* (long *)p = x;
	return 0;
}

static int
np_float(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	float x = (float)PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	memcpy(p, (char *)&x, sizeof(float));
	return 0;
}

static int
np_double(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	double x = PyFloat_AsDouble(v);
	if (x == -1 && PyErr_Occurred()) {
		PyErr_SetString(StructError,
				"required argument is not a float");
		return -1;
	}
	memcpy(p, (char *)&x, sizeof(double));
	return 0;
}

static formatdef native_table[] = {
	{'x',	sizeof(char),	0,		NULL},
	{'b',	sizeof(char),	0,		nu_byte,	np_byte},
	{'B',	sizeof(char),	0,		nu_ubyte,	np_byte},
	{'c',	sizeof(char),	0,		nu_char,	np_char},
	{'s',	sizeof(char),	0,		NULL},
	{'h',	sizeof(short),	SHORT_ALIGN,	nu_short,	np_short},
	{'H',	sizeof(short),	SHORT_ALIGN,	nu_ushort,	np_short},
	{'i',	sizeof(int),	INT_ALIGN,	nu_int,		np_int},
	{'I',	sizeof(int),	INT_ALIGN,	nu_uint,	np_int},
	{'l',	sizeof(long),	LONG_ALIGN,	nu_long,	np_long},
	{'L',	sizeof(long),	LONG_ALIGN,	nu_ulong,	np_long},
	{'f',	sizeof(float),	FLOAT_ALIGN,	nu_float,	np_float},
	{'d',	sizeof(double),	DOUBLE_ALIGN,	nu_double,	np_double},
	{0}
};

static PyObject *
bu_int(p, f)
	const char *p;
	const formatdef *f;
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (*p++ & 0xFF);
	} while (--i > 0);
	i = 8*(sizeof(long) - f->size);
	if (i) {
		x <<= i;
		x >>= i;
	}
	return PyInt_FromLong(x);
}

static PyObject *
bu_uint(p, f)
	const char *p;
	const formatdef *f;
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (*p++ & 0xFF);
	} while (--i > 0);
	if (f->size == sizeof(long))
		return make_ulong(x);
	else
		return PyLong_FromLong(x);
}

static int
bp_int(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	int i;
	if (get_long(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		p[--i] = x;
		x >>= 8;
	} while (i > 0);
	return 0;
}

static formatdef bigendian_table[] = {
	{'x',	1,		0,		NULL},
	{'b',	1,		0,		bu_int,		bp_int},
	{'B',	1,		0,		bu_uint,	bp_int},
	{'c',	1,		0,		nu_char,	np_char},
	{'s',	1,		0,		NULL},
	{'h',	2,		0,		bu_int,		bp_int},
	{'H',	2,		0,		bu_uint,	bp_int},
	{'i',	4,		0,		bu_int,		bp_int},
	{'I',	4,		0,		bu_uint,	bp_int},
	{'l',	4,		0,		bu_int,		bp_int},
	{'L',	4,		0,		bu_uint,	bp_int},
	/* No float and double! */
	{0}
};

static PyObject *
lu_int(p, f)
	const char *p;
	const formatdef *f;
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (p[--i] & 0xFF);
	} while (i > 0);
	i = 8*(sizeof(long) - f->size);
	if (i) {
		x <<= i;
		x >>= i;
	}
	return PyInt_FromLong(x);
}

static PyObject *
lu_uint(p, f)
	const char *p;
	const formatdef *f;
{
	long x = 0;
	int i = f->size;
	do {
		x = (x<<8) | (p[--i] & 0xFF);
	} while (i > 0);
	if (f->size == sizeof(long))
		return make_ulong(x);
	else
		return PyLong_FromLong(x);
}

static int
lp_int(p, v, f)
	char *p;
	PyObject *v;
	const formatdef *f;
{
	long x;
	int i;
	if (get_long(v, &x) < 0)
		return -1;
	i = f->size;
	do {
		*p++ = x;
		x >>= 8;
	} while (--i > 0);
	return 0;
}

static formatdef lilendian_table[] = {
	{'x',	1,		0,		NULL},
	{'b',	1,		0,		lu_int,		lp_int},
	{'B',	1,		0,		lu_uint,	lp_int},
	{'c',	1,		0,		nu_char,	np_char},
	{'s',	1,		0,		NULL},
	{'h',	2,		0,		lu_int,		lp_int},
	{'H',	2,		0,		lu_uint,	lp_int},
	{'i',	4,		0,		lu_int,		lp_int},
	{'I',	4,		0,		lu_uint,	lp_int},
	{'l',	4,		0,		lu_int,		lp_int},
	{'L',	4,		0,		lu_uint,	lp_int},
	/* No float and double! */
	{0}
};


static const formatdef *
whichtable(pfmt)
	const char **pfmt;
{
	const char *fmt = (*pfmt)++; /* May be backed out of later */
	switch (*fmt) {
	case '<':
		return lilendian_table;
	case '>':
	case '!': /* Network byte order is big-endian */
		return bigendian_table;
	case '=': { /* Host byte order -- different from native in aligment! */
		int n = 1;
		char *p = (char *) &n;
		if (*p == 1)
			return lilendian_table;
		else
			return bigendian_table;
	}
	default:
		--*pfmt; /* Back out of pointer increment */
		/* Fall through */
	case '@':
		return native_table;
	}
}


/* Get the table entry for a format code */

static const formatdef *
getentry(c, f)
	int c;
	const formatdef *f;
{
	for (; f->format != '\0'; f++) {
		if (f->format == c) {
			return f;
		}
	}
	PyErr_SetString(StructError, "bad char in struct format");
	return NULL;
}


/* Align a size according to a format code */

static int
align(size, c, e)
	int size;
	int c;
	const formatdef *e;
{
	if (e->format == c) {
		if (e->alignment) {
			size = ((size + e->alignment - 1)
				/ e->alignment)
				* e->alignment;
		}
	}
	return size;
}


/* calculate the size of a format string */

static int
calcsize(fmt, f)
	const char *fmt;
	const formatdef *f;
{
	const formatdef *e;
	const char *s;
	char c;
	int size,  num, itemsize, x;

	s = fmt;
	size = 0;
	while ((c = *s++) != '\0') {
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9') {
				x = num*10 + (c - '0');
				if (x/10 != num) {
					PyErr_SetString(
						StructError,
						"overflow in item count");
					return -1;
				}
				num = x;
			}
			if (c == '\0')
				break;
		}
		else
			num = 1;
		
		e = getentry(c, f);
		if (e == NULL)
			return -1;
		itemsize = e->size;
		size = align(size, c, e);
		x = num * itemsize;
		size += x;
		if (x/itemsize != num || size < 0) {
			PyErr_SetString(StructError,
                                        "total struct size too long");
			return -1;
		}
	}

	return size;
}


/* pack(fmt, v1, v2, ...) --> string */

static PyObject *
struct_calcsize(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *fmt;
	const formatdef *f;
	int size;

	if (!PyArg_ParseTuple(args, "s", &fmt))
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	return PyInt_FromLong((long)size);
}


/* pack(fmt, v1, v2, ...) --> string */

static PyObject *
struct_pack(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	const formatdef *f, *e;
	PyObject *format, *result, *v;
	char *fmt;
	int size, num;
	int i, n;
	char *s, *res, *restart;
	char c;

	if (args == NULL || !PyTuple_Check(args) ||
	    (n = PyTuple_Size(args)) < 1)
        {
		PyErr_BadArgument();
		return NULL;
	}
	format = PyTuple_GetItem(args, 0);
	if (!PyArg_Parse(format, "s", &fmt))
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	result = PyString_FromStringAndSize((char *)NULL, size);
	if (result == NULL)
		return NULL;

	s = fmt;
	i = 1;
	res = restart = PyString_AsString(result);

	while ((c = *s++) != '\0') {
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9')
			       num = num*10 + (c - '0');
			if (c == '\0')
				break;
		}
		else
			num = 1;

		e = getentry(c, f);
		if (e == NULL)
			goto fail;
		res = restart + align((int)(res-restart), c, e);
		if (num == 0 && c != 's')
			continue;
		do {
			if (c == 'x') {
				/* doesn't consume arguments */
				memset(res, '\0', num);
				res += num;
				break;
			}
			if (i >= n) {
				PyErr_SetString(StructError,
					"insufficient arguments to pack");
				goto fail;
				}
			v = PyTuple_GetItem(args, i++);
			if (v == NULL)
				goto fail;
			if (c == 's') {
				/* num is string size, not repeat count */
				int n;
				if (!PyString_Check(v)) {
					PyErr_SetString(StructError,
					  "argument for 's' must be a string");
					goto fail;
				}
				n = PyString_Size(v);
				if (n > num)
					n = num;
				if (n > 0)
					memcpy(res, PyString_AsString(v), n);
				if (n < num)
					memset(res+n, '\0', num-n);
				res += num;
				break;
			}
			else {
				if (e->pack(res, v, e) < 0)
					goto fail;
				res += e->size;
			}
		} while (--num > 0);
	}

	if (i < n) {
		PyErr_SetString(StructError,
				"too many arguments for pack format");
		goto fail;
	}

	return result;

 fail:
	Py_DECREF(result);
	return NULL;
}


/* unpack(fmt, string) --> (v1, v2, ...) */

static PyObject *
struct_unpack(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	const formatdef *f, *e;
	char *str, *start, *fmt, *s;
	char c;
	int len, size, num, x;
	PyObject *res, *v;

	if (!PyArg_ParseTuple(args, "ss#", &fmt, &start, &len))
		return NULL;
	f = whichtable(&fmt);
	size = calcsize(fmt, f);
	if (size < 0)
		return NULL;
	if (size != len) {
		PyErr_SetString(StructError,
				"unpack str size does not match format");
		return NULL;
	}
	res = PyList_New(0);
	if (res == NULL)
		return NULL;
	str = start;
	s = fmt;
	while ((c = *s++) != '\0') {
		if ('0' <= c && c <= '9') {
			num = c - '0';
			while ('0' <= (c = *s++) && c <= '9')
			       num = num*10 + (c - '0');
			if (c == '\0')
				break;
		}
		else
			num = 1;

		e = getentry(c, f);
		if (e == NULL)
			goto fail;
		str = start + align((int)(str-start), c, e);
		if (num == 0 && c != 's')
			continue;

		do {
			if (c == 'x') {
				str += num;
				break;
			}
			if (c == 's') {
				/* num is string size, not repeat count */
				v = PyString_FromStringAndSize(str, num);
				if (v == NULL)
					goto fail;
				str += num;
				num = 0;
			}
			else {
				v = e->unpack(str, e);
				if (v == NULL)
					goto fail;
				str += e->size;
			}
			if (v == NULL || PyList_Append(res, v) < 0)
				goto fail;
			Py_DECREF(v);
		} while (--num > 0);
	}

	v = PyList_AsTuple(res);
	Py_DECREF(res);
	return v;

 fail:
	Py_DECREF(res);
	return NULL;
}


/* List of functions */

static PyMethodDef struct_methods[] = {
	{"calcsize",	struct_calcsize,	METH_VARARGS},
	{"pack",	struct_pack,		METH_VARARGS},
	{"unpack",	struct_unpack,		METH_VARARGS},
	{NULL,		NULL}		/* sentinel */
};


/* Module initialization */

void
initstruct()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("struct", struct_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	StructError = PyString_FromString("struct.error");
	PyDict_SetItemString(d, "error", StructError);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module struct");
}
