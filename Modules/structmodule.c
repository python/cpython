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

#include "Python.h"

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


/* Align a size according to a format code */

static int
align(size, c)
	int size;
	int c;
{
	int a;

	switch (c) {
	case 'h': a = SHORT_ALIGN; break;
	case 'i': a = INT_ALIGN; break;
	case 'l': a = LONG_ALIGN; break;
	case 'f': a = FLOAT_ALIGN; break;
	case 'd': a = DOUBLE_ALIGN; break;
	default: return size;
	}
	return (size + a - 1) / a * a;
}


/* calculate the size of a format string */

static int
calcsize(fmt)
	char *fmt;
{
	char *s;
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
					PyErr_SetString(StructError,
                                                        "int overflow in fmt");
					return -1;
				}
				num = x;
			}
			if (c == '\0')
				break;
		}
		else
			num = 1;
		
		size = align(size, c);
		
		switch (c) {
			
		case 'x': /* pad byte */
		case 'b': /* byte-sized int */
		case 'c': /* char */
			itemsize = 1;
			break;

		case 'h': /* short ("half-size)" int */
			itemsize = sizeof(short);
			break;

		case 'i': /* natural-size int */
			itemsize = sizeof(int);
			break;

		case 'l': /* long int */
			itemsize = sizeof(long);
			break;

		case 'f': /* float */
			itemsize = sizeof(float);
			break;

		case 'd': /* double */
			itemsize = sizeof(double);
			break;

		default:
			PyErr_SetString(StructError, "bad char in fmt");
			return -1;

		}
		
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
	int size;

	if (!PyArg_Parse(args, "s", &fmt))
		return NULL;
	size = calcsize(fmt);
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
	PyObject *format, *result, *v;
	char *fmt;
	int size, num;
	int i, n;
	char *s, *res, *restart;
	char c;
	long ival;
	double fval;

	if (args == NULL || !PyTuple_Check(args) ||
	    (n = PyTuple_Size(args)) < 1)
        {
		PyErr_BadArgument();
		return NULL;
	}
	format = PyTuple_GetItem(args, 0);
	if (!PyArg_Parse(format, "s", &fmt))
		return NULL;
	size = calcsize(fmt);
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

		res = restart + align((int)(res-restart), c);

		while (--num >= 0) {
			switch (c) {

			case 'x': /* pad byte */
				*res++ = '\0';
				break;

			case 'l':
			case 'i':
			case 'h':
			case 'b':
				if (i >= n) {
					PyErr_SetString(StructError,
					     "insufficient arguments to pack");
					goto fail;
				}
				v = PyTuple_GetItem(args, i++);
				if (!PyInt_Check(v)) {
					PyErr_SetString(StructError,
					          "bad argument type to pack");
					goto fail;
				}
				ival = PyInt_AsLong(v);
				switch (c) {
				case 'b':
					*res++ = ival;
					break;
				case 'h':
					*(short*)res = ival;
					res += sizeof(short);
					break;
				case 'i':
					*(int*)res = ival;
					res += sizeof(int);
					break;
				case 'l':
					*(long*)res = ival;
					res += sizeof(long);
					break;
				}
				break;

			case 'c':
				if (i >= n) {
					PyErr_SetString(StructError,
					     "insufficient arguments to pack");
					goto fail;
				}
				v = PyTuple_GetItem(args, i++);
				if (!PyString_Check(v) ||
				    PyString_Size(v) != 1)
				{
					PyErr_SetString(StructError,
					          "bad argument type to pack");
					goto fail;
				}
				*res++ = PyString_AsString(v)[0];
				break;

			case 'd':
			case 'f':
				if (i >= n) {
					PyErr_SetString(StructError,
					     "insufficient arguments to pack");
					goto fail;
				}
				v = PyTuple_GetItem(args, i++);
				if (!PyFloat_Check(v)) {
					PyErr_SetString(StructError,
					          "bad argument type to pack");
					goto fail;
				}
				fval = PyFloat_AsDouble(v);
				switch (c) {
				case 'f':
					*(float*)res = (float)fval;
					res += sizeof(float);
					break;
				case 'd':
					memcpy(res, (char*)&fval, sizeof fval);
					res += sizeof(double);
					break;
				}
				break;

			default:
				PyErr_SetString(StructError,
						"bad char in fmt");
				goto fail;

			}
		}
	}

	if (i < n) {
		PyErr_SetString(StructError,
				"too many arguments for pack fmt");
		goto fail;
	}

	return result;

 fail:
	Py_DECREF(result);
	return NULL;
}


/* Helper to convert a list to a tuple */

static PyObject *
totuple(list)
	PyObject *list;
{
	int len = PyList_Size(list);
	PyObject *tuple = PyTuple_New(len);
	if (tuple != NULL) {
		int i;
		for (i = 0; i < len; i++) {
			PyObject *v = PyList_GetItem(list, i);
			Py_INCREF(v);
			PyTuple_SetItem(tuple, i, v);
		}
	}
	Py_DECREF(list);
	return tuple;
}


/* unpack(fmt, string) --> (v1, v2, ...) */

static PyObject *
struct_unpack(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *str, *start, *fmt, *s;
	char c;
	int len, size, num, x;
	PyObject *res, *v;

	if (!PyArg_Parse(args, "(ss#)", &fmt, &start, &len))
		return NULL;
	size = calcsize(fmt);
	if (size != len) {
		PyErr_SetString(StructError,
				"unpack str size does not match fmt");
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

		str = start + align((int)(str-start), c);

		while (--num >= 0) {
			switch (c) {

			case 'x':
				str++;
				continue;

			case 'b':
				x = *str++;
				if (x >= 128)
					x -= 256;
				v = PyInt_FromLong((long)x);
				break;

			case 'c':
				v = PyString_FromStringAndSize(str, 1);
				str++;
				break;

			case 'h':
				v = PyInt_FromLong((long)*(short*)str);
				str += sizeof(short);
				break;

			case 'i':
				v = PyInt_FromLong((long)*(int*)str);
				str += sizeof(int);
				break;

			case 'l':
				v = PyInt_FromLong(*(long*)str);
				str += sizeof(long);
				break;

			case 'f':
				v = PyFloat_FromDouble((double)*(float*)str);
				str += sizeof(float);
				break;

			case 'd':
			    {
				double d;
				memcpy((char *)&d, str, sizeof d);
				v = PyFloat_FromDouble(d);
				str += sizeof(double);
				break;
			    }

			default:
				PyErr_SetString(StructError,
						"bad char in fmt");
				goto fail;

			}
			if (v == NULL || PyList_Append(res, v) < 0)
				goto fail;
			Py_DECREF(v);
		}
	}

	return totuple(res);

 fail:
	Py_DECREF(res);
	return NULL;
}


/* List of functions */

static PyMethodDef struct_methods[] = {
	{"calcsize",	struct_calcsize},
	{"pack",	struct_pack,	1/*varargs*/},
	{"unpack",	struct_unpack},
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
