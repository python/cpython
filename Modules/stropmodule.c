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

/* strop module */

#include "allobjects.h"
#include "modsupport.h"

#include <ctype.h>
/* XXX This file assumes that the <ctype.h> is*() functions
   XXX are defined for all 8-bit characters! */

#include <errno.h>

/* The lstrip(), rstrip() and strip() functions are implemented
   in do_strip(), which uses an additional parameter to indicate what
   type of strip should occur. */

#define LEFTSTRIP 0
#define RIGHTSTRIP 1
#define BOTHSTRIP 2


static object *
split_whitespace(s, len, maxsplit)
	char *s;
	int len;
	int maxsplit;
{
	int i, j, err;
	int countsplit;
	object *list, *item;

	list = newlistobject(0);
	if (list == NULL)
		return NULL;

	i = 0;
	countsplit = 0;

	while (i < len) {
		while (i < len && isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		j = i;
		while (i < len && !isspace(Py_CHARMASK(s[i]))) {
			i = i+1;
		}
		if (j < i) {
			item = newsizedstringobject(s+j, (int)(i-j));
			if (item == NULL) {
				DECREF(list);
				return NULL;
			}
			err = addlistitem(list, item);
			DECREF(item);
			if (err < 0) {
				DECREF(list);
				return NULL;
			}

			countsplit++;
			if (maxsplit && (countsplit >= maxsplit)) {
				item = newsizedstringobject(s+i, (int)(len - i));
				if (item == NULL) {
					DECREF(list);
					return NULL;
				}
				err = addlistitem(list, item);
				DECREF(item);
				if (err < 0) {
					DECREF(list);
					return NULL;
				}
				i = len;
			}
	
		}
	}

	return list;
}


static object *
strop_splitfields(self, args)
	object *self; /* Not used */
	object *args;
{
	int len, n, i, j, err;
	int splitcount, maxsplit;
	char *s, *sub;
	object *list, *item;

	sub = NULL;
	n = 0;
	splitcount = 0;
	maxsplit = 0;
	if (!newgetargs(args, "s#|z#i", &s, &len, &sub, &n, &maxsplit))
		return NULL;
	if (sub == NULL)
		return split_whitespace(s, len, maxsplit);
	if (n == 0) {
		err_setstr(ValueError, "empty separator");
		return NULL;
	}

	list = newlistobject(0);
	if (list == NULL)
		return NULL;

	i = j = 0;
	while (i+n <= len) {
		if (s[i] == sub[0] && (n == 1 || strncmp(s+i, sub, n) == 0)) {
			item = newsizedstringobject(s+j, (int)(i-j));
			if (item == NULL)
				goto fail;
			err = addlistitem(list, item);
			DECREF(item);
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
	item = newsizedstringobject(s+j, (int)(len-j));
	if (item == NULL)
		goto fail;
	err = addlistitem(list, item);
	DECREF(item);
	if (err < 0)
		goto fail;

	return list;

 fail:
	DECREF(list);
	return NULL;
}


static object *
strop_joinfields(self, args)
	object *self; /* Not used */
	object *args;
{
	object *seq, *item, *res;
	object * (*getitem) FPROTO((object *, int));
	char *sep, *p;
	int seplen, seqlen, reslen, itemlen, i;

	sep = NULL;
	seplen = 0;
	if (!newgetargs(args, "O|s#", &seq, &sep, &seplen))
		return NULL;
	if (sep == NULL) {
		sep = " ";
		seplen = 1;
	}
	if (is_listobject(seq)) {
		getitem = getlistitem;
		seqlen = getlistsize(seq);
	}
	else if (is_tupleobject(seq)) {
		getitem = gettupleitem;
		seqlen = gettuplesize(seq);
	}
	else {
		err_setstr(TypeError, "first argument must be list/tuple");
		return NULL;
	}
	reslen = 0;
	for (i = 0; i < seqlen; i++) {
		item = getitem(seq, i);
		if (!is_stringobject(item)) {
			err_setstr(TypeError,
			   "first argument must be list/tuple of strings");
			return NULL;
		}
		if (i > 0)
			reslen = reslen + seplen;
		reslen = reslen + getstringsize(item);
	}
	if (seqlen == 1) {
		/* Optimization if there's only one item */
		item = getitem(seq, 0);
		INCREF(item);
		return item;
	}
	res = newsizedstringobject((char *)NULL, reslen);
	if (res == NULL)
		return NULL;
	p = getstringvalue(res);
	for (i = 0; i < seqlen; i++) {
		item = getitem(seq, i);
		if (i > 0) {
			memcpy(p, sep, seplen);
			p += seplen;
		}
		itemlen = getstringsize(item);
		memcpy(p, getstringvalue(item), itemlen);
		p += itemlen;
	}
	if (p != getstringvalue(res) + reslen) {
		err_setstr(SystemError, "strop.joinfields: assertion failed");
		return NULL;
	}
	return res;
}


static object *
strop_find(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *sub;
	int len, n, i;

	if (getargs(args, "(s#s#i)", &s, &len, &sub, &n, &i)) {
		if (i < 0)
			i += len;
		if (i < 0)
			i = 0;
	}
	else {
		err_clear();
		if (!getargs(args, "(s#s#)", &s, &len, &sub, &n))
			return NULL;
		i = 0;
	}

	if (n == 0)
		return newintobject((long)i);

	len -= n;
	for (; i <= len; ++i)
		if (s[i] == sub[0] &&
		    (n == 1 || strncmp(&s[i+1], &sub[1], n-1) == 0))
			return newintobject((long)i);

	return newintobject(-1L);
}


static object *
strop_rfind(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *sub;
	int len, n, i, j;

	if (getargs(args, "(s#s#i)", &s, &len, &sub, &n, &i)) {
		if (i < 0)
			i += len;
		if (i < 0)
			i = 0;
	}
	else {
		err_clear();
		if (!getargs(args, "(s#s#)", &s, &len, &sub, &n))
			return NULL;
		i = 0;
	}

	if (n == 0)
		return newintobject((long)len);

	for (j = len-n; j >= i; --j)
		if (s[j] == sub[0] &&
		    (n == 1 || strncmp(&s[j+1], &sub[1], n-1) == 0))
			return newintobject((long)j);

	return newintobject(-1L);
}

static object *
do_strip(args, striptype)
	object *args;
	int striptype;
{
	char *s;
	int len, i, j;


	if (!getargs(args, "s#", &s, &len))
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
		INCREF(args);
		return args;
	}
	else
		return newsizedstringobject(s+i, j-i);
}


static object *
strop_strip(self, args)
	object *self; /* Not used */
	object *args;
{
	return do_strip(args, BOTHSTRIP);
}

static object *
strop_lstrip(self, args)
	object *self; /* Not used */
	object *args;
{
	return do_strip(args, LEFTSTRIP);
}

static object *
strop_rstrip(self, args)
	object *self; /* Not used */
	object *args;
{
	return do_strip(args, RIGHTSTRIP);
}


static object *
strop_lower(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *s_new;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = getstringvalue(new);
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
		DECREF(new);
		INCREF(args);
		return args;
	}
	return new;
}


static object *
strop_upper(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *s_new;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = getstringvalue(new);
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
		DECREF(new);
		INCREF(args);
		return args;
	}
	return new;
}


static object *
strop_capitalize(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *s_new;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = getstringvalue(new);
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
		DECREF(new);
		INCREF(args);
		return args;
	}
	return new;
}


static object *
strop_swapcase(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *s_new;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(NULL, n);
	if (new == NULL)
		return NULL;
	s_new = getstringvalue(new);
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
		DECREF(new);
		INCREF(args);
		return args;
	}
	return new;
}


static object *
strop_atoi(self, args)
	object *self; /* Not used */
	object *args;
{
	extern long mystrtol PROTO((const char *, char **, int));
	extern unsigned long mystrtoul PROTO((const char *, char **, int));
	char *s, *end;
	int base = 10;
	long x;
	char buffer[256]; /* For errors */

	if (args != NULL && is_tupleobject(args)) {
		if (!getargs(args, "(si)", &s, &base))
			return NULL;
		if (base != 0 && base < 2 || base > 36) {
			err_setstr(ValueError, "invalid base for atoi()");
			return NULL;
		}
	}
	else if (!getargs(args, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		err_setstr(ValueError, "empty string for atoi()");
		return NULL;
	}
	errno = 0;
	if (base == 0 && s[0] == '0')
		x = (long) mystrtoul(s, &end, base);
	else
		x = mystrtol(s, &end, base);
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atoi(): %.200s", s);
		err_setstr(ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "atoi() literal too large: %.200s", s);
		err_setstr(ValueError, buffer);
		return NULL;
	}
	return newintobject(x);
}


static object *
strop_atol(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *end;
	int base = 10;
	object *x;
	char buffer[256]; /* For errors */

	if (args != NULL && is_tupleobject(args)) {
		if (!getargs(args, "(si)", &s, &base))
			return NULL;
		if (base != 0 && base < 2 || base > 36) {
			err_setstr(ValueError, "invalid base for atol()");
			return NULL;
		}
	}
	else if (!getargs(args, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		err_setstr(ValueError, "empty string for atol()");
		return NULL;
	}
	x = long_escan(s, &end, base);
	if (x == NULL)
		return NULL;
	if (base == 0 && (*end == 'l' || *end == 'L'))
		end++;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atol(): %.200s", s);
		err_setstr(ValueError, buffer);
		DECREF(x);
		return NULL;
	}
	return x;
}


static object *
strop_atof(self, args)
	object *self; /* Not used */
	object *args;
{
	extern double strtod PROTO((const char *, char **));
	char *s, *end;
	double x;
	char buffer[256]; /* For errors */

	if (!getargs(args, "s", &s))
		return NULL;
	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	if (s[0] == '\0') {
		err_setstr(ValueError, "empty string for atof()");
		return NULL;
	}
	errno = 0;
	x = strtod(s, &end);
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
		sprintf(buffer, "invalid literal for atof(): %.200s", s);
		err_setstr(ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "atof() literal too large: %.200s", s);
		err_setstr(ValueError, buffer);
		return NULL;
	}
	return newfloatobject(x);
}


static PyObject *
strop_maketrans(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	unsigned char c[256], *from=NULL, *to=NULL;
	int i, fromlen=0, tolen=0;

	if (PyTuple_Size(args)!=0) {
		if (!PyArg_ParseTuple(args, "s#s#", &from, &fromlen, 
				      &to, &tolen)) 
			return NULL;	
	}

	if (fromlen!=tolen) {
		PyErr_SetString(ValueError,
				"maketrans arguments must have same length");
		return NULL;
	}
	for(i=0; i<256; i++)
		c[i]=(unsigned char)i;
	for(i=0; i<fromlen; i++) {
		c[from[i]]=to[i];
	}
	return PyString_FromStringAndSize((char *)c, 256);
}


static object *
strop_translate(self, args)
	object *self;
	object *args;
{
	char *input, *table, *output, *output_start, *delete=NULL;
	int inlen, tablen, dellen;
	PyObject *result;
	int i, trans_table[256];

	if (!PyArg_ParseTuple(args, "s#s#|s#", &input, &inlen,
			      &table, &tablen, &delete, &dellen))
		return NULL;
	if (tablen != 256) {
		PyErr_SetString(ValueError,
			   "translation table must be 256 characters long");
		return NULL;
	}
	for(i=0; i<256; i++)
		trans_table[i]=Py_CHARMASK(table[i]);
	if (delete!=NULL) {
		for(i=0; i<dellen; i++) 
			trans_table[delete[i]]=-1;
	}

	result = PyString_FromStringAndSize((char *)NULL, inlen);
	if (result == NULL)
		return NULL;
	output_start = output = PyString_AsString(result);
	if (delete!=NULL && dellen!=0) {
		for (i = 0; i < inlen; i++) {
			int c = Py_CHARMASK(*input++);
			if (trans_table[c]!=-1) 
				*output++ = (char)trans_table[c];
		}
		/* Fix the size of the resulting string */
		if (inlen > 0 &&_PyString_Resize(&result, output-output_start))
			return NULL; 
	} else {
		/* If no deletions are required, use a faster loop */
		for (i = 0; i < inlen; i++) {
			int c = Py_CHARMASK(*input++);
			*output++ = (char)trans_table[c];
		}
	}
	return result;
}


/* List of functions defined in the module */

static struct methodlist strop_methods[] = {
	{"atof",	strop_atof},
	{"atoi",	strop_atoi},
	{"atol",	strop_atol},
	{"capitalize",	strop_capitalize},
	{"find",	strop_find},
	{"join",	strop_joinfields, 1},
	{"joinfields",	strop_joinfields, 1},
	{"lstrip",	strop_lstrip},
	{"lower",	strop_lower},
	{"rfind",	strop_rfind},
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
	object *m, *d, *s;
	char buf[256];
	int c, n;
	m = initmodule("strop", strop_methods);
	d = getmoduledict(m);

	/* Create 'whitespace' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isspace(c))
			buf[n++] = c;
	}
	s = newsizedstringobject(buf, n);
	if (s) {
		dictinsert(d, "whitespace", s);
		DECREF(s);
	}
	/* Create 'lowercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (islower(c))
			buf[n++] = c;
	}
	s = newsizedstringobject(buf, n);
	if (s) {
		dictinsert(d, "lowercase", s);
		DECREF(s);
	}

	/* Create 'uppercase' object */
	n = 0;
	for (c = 0; c < 256; c++) {
		if (isupper(c))
			buf[n++] = c;
	}
	s = newsizedstringobject(buf, n);
	if (s) {
		dictinsert(d, "uppercase", s);
		DECREF(s);
	}

	if (err_occurred())
		fatal("can't initialize module strop");
}
