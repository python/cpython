/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

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


static object *
strop_split(self, args)
	object *self; /* Not used */
	object *args;
{
	int len, i, j;
	char *s;
	char c;
	object *list, *item;

	if (!getargs(args, "s#", &s, &len))
		return NULL;
	list = newlistobject(0);
	if (list == NULL)
		return NULL;

	i = 0;
	while (i < len) {
		while (i < len &&
		       ((c = s[i]), isspace(c))) {
			i = i+1;
		}
		j = i;
		while (i < len &&
		       !((c = s[i]), isspace(c))) {
			i = i+1;
		}
		if (j < i) {
			item = newsizedstringobject(s+j, (int)(i-j));
			if (item == NULL || addlistitem(list, item) < 0) {
				DECREF(list);
				return NULL;
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
	int len, n, i, j;
	char *s, *sub;
	char c;
	object *list, *item;

	if (!getargs(args, "(s#s#)", &s, &len, &sub, &n))
		return NULL;
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
			if (item == NULL || addlistitem(list, item) < 0) {
				DECREF(list);
				return NULL;
			}
			i = j = i + n;
		}
		else
			i++;
	}
	item = newsizedstringobject(s+j, (int)(len-j));
	if (item == NULL || addlistitem(list, item) < 0) {
		DECREF(list);
		return NULL;
	}

	return list;
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

	if (!getargs(args, "(Os#)", &seq, &sep, &seplen))
		return NULL;
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
strop_index(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s, *sub;
	int len, n, i;

	if (getargs(args, "(s#s#i)", &s, &len, &sub, &n, &i)) {
		if (i < 0 || i+n > len) {
			err_setstr(ValueError, "start offset out of range");
			return NULL;
		}
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
	for (; i <= len; i++) {
		if (s[i] == sub[0]) {
			if (n == 1 || strncmp(s+i, sub, n) == 0)
				return newintobject((long)i);
		}
	}

	err_setstr(ValueError, "substring not found");
	return NULL;
}


static object *
strop_strip(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s;
	int len, i, j;
	char c;

	if (!getargs(args, "s#", &s, &len))
		return NULL;

	i = 0;
	while (i < len && ((c = s[i]), isspace(c))) {
		i++;
	}

	j = len;
	do {
		j--;
	} while (j >= i &&  ((c = s[j]), isspace(c)));
	j++;

	if (i == 0 && j == len) {
		INCREF(args);
		return args;
	}
	else
		return newsizedstringobject(s+i, j-i);
}


static object *
strop_lower(self, args)
	object *self; /* Not used */
	object *args;
{
	char *s;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(s, n);
	if (new == NULL)
		return NULL;
	s = getstringvalue(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		char c = s[i];
		if (isupper(c)) {
			changed = 1;
			s[i] = tolower(c);
		}
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
	char *s;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(s, n);
	if (new == NULL)
		return NULL;
	s = getstringvalue(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		char c = s[i];
		if (islower(c)) {
			changed = 1;
			s[i] = toupper(c);
		}
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
	char *s;
	int i, n;
	object *new;
	int changed;

	if (!getargs(args, "s#", &s, &n))
		return NULL;
	new = newsizedstringobject(s, n);
	if (new == NULL)
		return NULL;
	s = getstringvalue(new);
	changed = 0;
	for (i = 0; i < n; i++) {
		char c = s[i];
		if (islower(c)) {
			changed = 1;
			s[i] = toupper(c);
		}
		else if (isupper(c)) {
			changed = 1;
			s[i] = tolower(c);
		}
	}
	if (!changed) {
		DECREF(new);
		INCREF(args);
		return args;
	}
	return new;
}


/* List of functions defined in the module */

static struct methodlist strop_methods[] = {
	{"index",	strop_index},
	{"joinfields",	strop_joinfields},
	{"lower",	strop_lower},
	{"split",	strop_split},
	{"splitfields",	strop_splitfields},
	{"strip",	strop_strip},
	{"swapcase",	strop_swapcase},
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
	for (c = 1; c < 256; c++) {
		if (isspace(c))
			buf[n++] = c;
	}
	if (s) {
		s = newsizedstringobject(buf, n);
		dictinsert(d, "whitespace", s);
		DECREF(s);
	}
	/* Create 'lowercase' object */
	n = 0;
	for (c = 1; c < 256; c++) {
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
	for (c = 1; c < 256; c++) {
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
