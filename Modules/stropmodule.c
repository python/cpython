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

/* strop module */

#include "Python.h"

#include <ctype.h>
/* XXX This file assumes that the <ctype.h> is*() functions
   XXX are defined for all 8-bit characters! */

/* The lstrip(), rstrip() and strip() functions are implemented
   in do_strip(), which uses an additional parameter to indicate what
   type of strip should occur. */

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2


static PyObject *
split_whitespace(s, len, maxsplit)
	char *s;
	int len;
	int maxsplit;
{
	int i = 0, j, err;
	int countsplit = 0;
	PyObject* item;
	PyObject *list = PyList_New(0);

	if (list == NULL)
		return NULL;

	while (i < len) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		j = i;
		while (i < len && !isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		if (j < i) {
			item = PyString_FromStringAndSize(s+j, (int)(i-j));
			if (item == NULL)
				goto finally;

			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto finally;

			countsplit++;
			if (maxsplit && (countsplit >= maxsplit)) {
				item = PyString_FromStringAndSize(
                                        s+i, (int)(len - i));
				if (item == NULL)
					goto finally;

				err = PyList_Append(list, item);
				Py_DECREF(item);
				if (err < 0)
					goto finally;

				i = len;
			}
	
		}
	}
	return list;
  finally:
	Py_DECREF(list);
	return NULL;
}


static PyObject *
strop_splitfields(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int len, n, i, j, err;
	int splitcount, maxsplit;
	char *s, *sub;
	PyObject *list, *item;

	sub = NULL;
	n = 0;
	splitcount = 0;
	maxsplit = 0;
	if (!PyArg_ParseTuple(args, "s#|z#i", &s, &len, &sub, &n, &maxsplit))
		return NULL;
	if (sub == NULL)
		return split_whitespace(s, len, maxsplit);
	if (n == 0) {
		PyErr_SetString(PyExc_ValueError, "empty separator");
		return NULL;
	}

	list = PyList_New(0);
	if (list == NULL)
		return NULL;

	i = j = 0;
	while (i+n <= len) {
		if (s[i] == sub[0] && (n == 1 || memcmp(s+i, sub, n) == 0)) {
			item = PyString_FromStringAndSize(s+j, (int)(i-j));
			if (item == NULL)
				goto fail;
			err = PyList_Append(list, item);
			Py_DECREF(item);
			if (err < 0)
				goto fail;
			i = j = i + n;
			splitcount++;
			if (maxsplit && (splitcount >= maxsplit))
				break;
		}
		else
			i++;
	}
	item = PyString_FromStringAndSize(s+j, (int)(len-j));
	if (item == NULL)
		goto fail;
	err = PyList_Append(list, item);
	Py_DECREF(item);
	if (err < 0)
		goto fail;

	return list;

 fail:
	Py_DECREF(list);
	return NULL;
}


static PyObject *
strop_joinfields(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	PyObject *seq;
	char *sep = NULL;
	int seqlen, seplen = 0;
	int i, reslen = 0, slen = 0, sz = 100;
	PyObject *res = NULL;
	char* p = NULL;
	intargfunc getitemfunc;

	if (!PyArg_ParseTuple(args, "O|s#", &seq, &sep, &seplen))
		return NULL;
	if (sep == NULL) {
		sep = " ";
		seplen = 1;
	}

	seqlen = PySequence_Length(seq);
	if (seqlen < 0 && PyErr_Occurred())
		return NULL;

	if (seqlen == 1) {
		/* Optimization if there's only one item */
		PyObject *item = PySequence_GetItem(seq, 0);
		if (item && !PyString_Check(item))
			PyErr_SetString(PyExc_TypeError,
				 "first argument must be sequence of strings");
		return item;
	}

	if (!(res = PyString_FromStringAndSize((char*)NULL, sz)))
		return NULL;
	p = PyString_AsString(res);

	/* optimize for lists, since it's the most common case.  all others
	 * (tuples and arbitrary sequences) just use the sequence abstract
	 * interface.
	 */
	if (PyList_Check(seq)) {
		for (i = 0; i < seqlen; i++) {
			PyObject *item = PyList_GET_ITEM(seq, i);
			if (!PyString_Check(item)) {
				PyErr_SetString(PyExc_TypeError,
				"first argument must be sequence of strings");
				Py_DECREF(res);
				return NULL;
			}
			slen = PyString_GET_SIZE(item);
			while (reslen + slen + seplen >= sz) {
				if (_PyString_Resize(&res, sz * 2)) {
					Py_DECREF(res);
					return NULL;
				}
				sz *= 2;
				p = PyString_AsString(res) + reslen;
			}
			if (i > 0) {
				memcpy(p, sep, seplen);
				p += seplen;
				reslen += seplen;
			}
			memcpy(p, PyString_AS_STRING(item), slen);
			p += slen;
			reslen += slen;
		}
		if (_PyString_Resize(&res, reslen)) {
			Py_DECREF(res);
			res = NULL;
		}
		return res;
	}
	else if (!PySequence_Check(seq)) {
		PyErr_SetString(PyExc_TypeError,
				"first argument must be a sequence");
		return NULL;
	}
	/* type safe */
	getitemfunc = seq->ob_type->tp_as_sequence->sq_item;
	for (i = 0; i < seqlen; i++) {
		PyObject *item = getitemfunc(seq, i);
		if (!item || !PyString_Check(item)) {
			PyErr_SetString(PyExc_TypeError,
				 "first argument must be sequence of strings");
			Py_DECREF(res);
			Py_XDECREF(item);
			return NULL;
		}
		slen = PyString_GET_SIZE(item);
		while (reslen + slen + seplen >= sz) {
			if (_PyString_Resize(&res, sz * 2)) {
				Py_DECREF(res);
				Py_DECREF(item);
				return NULL;
			}
			sz *= 2;
			p = PyString_AsString(res) + reslen;
		}
		if (i > 0) {
			memcpy(p, sep, seplen);
			p += seplen;
			reslen += seplen;
		}
		memcpy(p, PyString_AS_STRING(item), slen);
		p += slen;
		reslen += slen;
		Py_DECREF(item);
	}
	if (_PyString_Resize(&res, reslen)) {
		Py_DECREF(res);
		res = NULL;
	}
	return res;
}

static PyObject *
strop_find(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *sub;
	int len, n, i = 0;

	if (!PyArg_ParseTuple(args, "s#s#|i", &s, &len, &sub, &n, &i))
		return NULL;

	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;

	if (n == 0)
		return PyInt_FromLong((long)i);

	len -= n;
	for (; i <= len; ++i)
		if (s[i] == sub[0] &&
		    (n == 1 || memcmp(&s[i+1], &sub[1], n-1) == 0))
			return PyInt_FromLong((long)i);

	return PyInt_FromLong(-1L);
}


static PyObject *
strop_rfind(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *sub;
	int len, n, j;
	int i = 0;

	if (!PyArg_ParseTuple(args, "s#s#|i", &s, &len, &sub, &n, &i))
		return NULL;

	if (i < 0)
		i += len;
	if (i < 0)
		i = 0;

	if (n == 0)
		return PyInt_FromLong((long)len);

	for (j = len-n; j >= i; --j)
		if (s[j] == sub[0] &&
		    (n == 1 || memcmp(&s[j+1], &sub[1], n-1) == 0))
			return PyInt_FromLong((long)j);

	return PyInt_FromLong(-1L);
}

static PyObject *
do_strip(args, striptype)
	PyObject *args;
	int striptype;
{
	char *s;
	int len, i, j;


	if (!PyArg_Parse(args, "s#", &s, &len))
		return NULL;

	i = 0;
	if (striptype != RIGHTSTRIP) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i++;
		}
	}
	

	j = len;
	if (striptype != LEFTSTRIP) {
		do {
			j--;
		} while (j >= i && isspace(Py_CHARMASK(s[j])));
		j++;
	}

	if (i == 0 && j == len) {
		Py_INCREF(args);
		return args;
	}
	else
		return PyString_FromStringAndSize(s+i, j-i);
}


static PyObject *
strop_strip(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	return do_strip(args, BOTHSTRIP);
}

static PyObject *
strop_lstrip(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	return do_strip(args, LEFTSTRIP);
}

static PyObject *
strop_rstrip(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	return do_strip(args, RIGHTSTRIP);
}


static PyObject *
strop_lower(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *s_new;
	int i, n;
	PyObject *new;
	int changed;

	if (!PyArg_Parse(args, "s#", &s, &n))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(new);
		Py_INCREF(args);
		return args;
	}
	return new;
}


static PyObject *
strop_upper(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *s_new;
	int i, n;
	PyObject *new;
	int changed;

	if (!PyArg_Parse(args, "s#", &s, &n))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(new);
		Py_INCREF(args);
		return args;
	}
	return new;
}


static PyObject *
strop_capitalize(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *s_new;
	int i, n;
	PyObject *new;
	int changed;

	if (!PyArg_Parse(args, "s#", &s, &n))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	changed = 0;
	if (0 < n) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		} else
			*s_new = c;
		s_new++;
	}
	for (i = 1; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		} else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(new);
		Py_INCREF(args);
		return args;
	}
	return new;
}


static PyObject *
strop_swapcase(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *s_new;
	int i, n;
	PyObject *new;
	int changed;

	if (!PyArg_Parse(args, "s#", &s, &n))
		return NULL;
	new = PyString_FromStringAndSize(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = PyString_AsString(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		int c = Py_CHARMASK(*s++);
		if (islower(c)) {
			changed = 1;
			*s_new = toupper(c);
		}
		else if (isupper(c)) {
			changed = 1;
			*s_new = tolower(c);
		}
		else
			*s_new = c;
		s_new++;
	}
	if (!changed) {
		Py_DECREF(new);
		Py_INCREF(args);
		return args;
	}
	return new;
}


static PyObject *
strop_atoi(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	extern long PyOS_strtol Py_PROTO((const char *, char **, int));
	extern unsigned long
                PyOS_strtoul Py_PROTO((const char *, char **, int));
	char *s, *end;
	int base = 10;
	long x;
	char buffer[256]; /* For errors */

	if (!PyArg_ParseTuple(args, "s|i", &s, &base))
		return NULL;

	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError, "invalid base for atoi()");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for atoi()");
		return NULL;
	}
	errno = 0;
	if (base == 0 && s[0] == '0')
		x = (long) PyOS_strtoul(s, &end, base);
	else
		x = PyOS_strtol(s, &end, base);
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atoi(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "atoi() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyInt_FromLong(x);
}


static PyObject *
strop_atol(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	char *s, *end;
	int base = 10;
	PyObject *x;
	char buffer[256]; /* For errors */

	if (!PyArg_ParseTuple(args, "s|i", &s, &base))
		return NULL;

	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError, "invalid base for atol()");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for atol()");
		return NULL;
	}
	x = PyLong_FromString(s, &end, base);
	if (x == NULL)
		return NULL;
	if (base == 0 && (*end == 'l' || *end == 'L'))
		end++;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atol(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		Py_DECREF(x);
		return NULL;
	}
	return x;
}


static PyObject *
strop_atof(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	extern double strtod Py_PROTO((const char *, char **));
	char *s, *end;
	double x;
	char buffer[256]; /* For errors */

	if (!PyArg_Parse(args, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		PyErr_SetString(PyExc_ValueError, "empty string for atof()");
		return NULL;
	}
	errno = 0;
	PyFPE_START_PROTECT("strop_atof", return 0)
	x = strtod(s, &end);
	PyFPE_END_PROTECT
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atof(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "atof() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	return PyFloat_FromDouble(x);
}


static PyObject *
strop_maketrans(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	unsigned char *c, *from=NULL, *to=NULL;
	int i, fromlen=0, tolen=0;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "s#s#", &from, &fromlen, &to, &tolen))
		return NULL;

	if (fromlen != tolen) {
		PyErr_SetString(PyExc_ValueError,
				"maketrans arguments must have same length");
		return NULL;
	}

	result = PyString_FromStringAndSize((char *)NULL, 256);
	if (result == NULL)
		return NULL;
	c = (unsigned char *) PyString_AS_STRING((PyStringObject *)result);
	for (i = 0; i < 256; i++)
		c[i]=(unsigned char)i;
	for (i = 0; i < fromlen; i++)
		c[from[i]]=to[i];

	return result;
}


static PyObject *
strop_translate(self, args)
	PyObject *self;
	PyObject *args;
{
	register char *input, *table, *output;
	register int i, c, changed = 0;
	PyObject *input_obj;
	char *table1, *output_start, *del_table=NULL;
	int inlen, tablen, dellen = 0;
	PyObject *result;
	int trans_table[256];

	if (!PyArg_ParseTuple(args, "Ss#|s#", &input_obj,
			      &table1, &tablen, &del_table, &dellen))
		return NULL;
	if (tablen != 256) {
		PyErr_SetString(PyExc_ValueError,
			      "translation table must be 256 characters long");
		return NULL;
	}

	table = table1;
	inlen = PyString_Size(input_obj);
	result = PyString_FromStringAndSize((char *)NULL, inlen);
	if (result == NULL)
		return NULL;
	output_start = output = PyString_AsString(result);
	input = PyString_AsString(input_obj);

	if (dellen == 0) {
		/* If no deletions are required, use faster code */
		for (i = inlen; --i >= 0; ) {
			c = Py_CHARMASK(*input++);
			if (Py_CHARMASK((*output++ = table[c])) != c)
				changed = 1;
		}
		if (changed)
			return result;
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}

	for (i = 0; i < 256; i++)
		trans_table[i] = Py_CHARMASK(table[i]);

	for (i = 0; i < dellen; i++) 
		trans_table[Py_CHARMASK(del_table[i])] = -1;

	for (i = inlen; --i >= 0; ) {
		c = Py_CHARMASK(*input++);
		if (trans_table[c] != -1)
			if (Py_CHARMASK(*output++ = (char)trans_table[c]) == c)
				continue;
		changed = 1;
	}
	if (!changed) {
		Py_DECREF(result);
		Py_INCREF(input_obj);
		return input_obj;
	}
	/* Fix the size of the resulting string */
	if (inlen > 0 &&_PyString_Resize(&result, output-output_start))
		return NULL; 
	return result;
}


/* List of functions defined in the module */

static PyMethodDef
strop_methods[] = {
	{"atof",	strop_atof},
	{"atoi",	strop_atoi, 1},
	{"atol",	strop_atol, 1},
	{"capitalize",	strop_capitalize},
	{"find",	strop_find, 1},
	{"join",	strop_joinfields, 1},
	{"joinfields",	strop_joinfields, 1},
	{"lstrip",	strop_lstrip},
	{"lower",	strop_lower},
	{"rfind",	strop_rfind, 1},
	{"rstrip",	strop_rstrip},
	{"split",	strop_splitfields, 1},
	{"splitfields",	strop_splitfields, 1},
	{"strip",	strop_strip},
	{"swapcase",	strop_swapcase},
	{"maketrans",	strop_maketrans, 1},
	{"translate",	strop_translate, 1},
	{"upper",	strop_upper},
	{NULL,		NULL}	/* sentinel */
};


void
initstrop()
{
	PyObject *m, *d, *s;
	char buf[256];
	int c, n;
	m = Py_InitModule("strop", strop_methods);
	d = PyModule_GetDict(m);

	/* Create 'whitespace' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isspace(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s) {
		PyDict_SetItemString(d, "whitespace", s);
		Py_DECREF(s);
	}
	/* Create 'lowercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (islower(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s) {
		PyDict_SetItemString(d, "lowercase", s);
		Py_DECREF(s);
	}

	/* Create 'uppercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isupper(c))
			buf[n++] = c;
	}
	s = PyString_FromStringAndSize(buf, n);
	if (s) {
		PyDict_SetItemString(d, "uppercase", s);
		Py_DECREF(s);
	}

	if (PyErr_Occurred())
		Py_FatalError("can't initialize module strop");
}
