/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* List object implementation */

#include "allobjects.h"
#include "modsupport.h"

object *
newlistobject(size)
	int size;
{
	int i;
	listobject *op;
	if (size < 0) {
		err_badcall();
		return NULL;
	}
	op = (listobject *) malloc(sizeof(listobject));
	if (op == NULL) {
		return err_nomem();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = (object **) malloc(size * sizeof(object *));
		if (op->ob_item == NULL) {
			free((ANY *)op);
			return err_nomem();
		}
	}
	NEWREF(op);
	op->ob_type = &Listtype;
	op->ob_size = size;
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	return (object *) op;
}

int
getlistsize(op)
	object *op;
{
	if (!is_listobject(op)) {
		err_badcall();
		return -1;
	}
	else
		return ((listobject *)op) -> ob_size;
}

object *
getlistitem(op, i)
	object *op;
	int i;
{
	if (!is_listobject(op)) {
		err_badcall();
		return NULL;
	}
	if (i < 0 || i >= ((listobject *)op) -> ob_size) {
		err_setstr(IndexError, "list index out of range");
		return NULL;
	}
	return ((listobject *)op) -> ob_item[i];
}

int
setlistitem(op, i, newitem)
	register object *op;
	register int i;
	register object *newitem;
{
	register object *olditem;
	if (!is_listobject(op)) {
		XDECREF(newitem);
		err_badcall();
		return -1;
	}
	if (i < 0 || i >= ((listobject *)op) -> ob_size) {
		XDECREF(newitem);
		err_setstr(IndexError, "list assignment index out of range");
		return -1;
	}
	olditem = ((listobject *)op) -> ob_item[i];
	((listobject *)op) -> ob_item[i] = newitem;
	XDECREF(olditem);
	return 0;
}

static int
ins1(self, where, v)
	listobject *self;
	int where;
	object *v;
{
	int i;
	object **items;
	if (v == NULL) {
		err_badcall();
		return -1;
	}
	items = self->ob_item;
	RESIZE(items, object *, self->ob_size+1);
	if (items == NULL) {
		err_nomem();
		return -1;
	}
	if (where < 0)
		where = 0;
	if (where > self->ob_size)
		where = self->ob_size;
	for (i = self->ob_size; --i >= where; )
		items[i+1] = items[i];
	INCREF(v);
	items[where] = v;
	self->ob_item = items;
	self->ob_size++;
	return 0;
}

int
inslistitem(op, where, newitem)
	object *op;
	int where;
	object *newitem;
{
	if (!is_listobject(op)) {
		err_badcall();
		return -1;
	}
	return ins1((listobject *)op, where, newitem);
}

int
addlistitem(op, newitem)
	object *op;
	object *newitem;
{
	if (!is_listobject(op)) {
		err_badcall();
		return -1;
	}
	return ins1((listobject *)op,
		(int) ((listobject *)op)->ob_size, newitem);
}

/* Methods */

static void
list_dealloc(op)
	listobject *op;
{
	int i;
	for (i = 0; i < op->ob_size; i++) {
		XDECREF(op->ob_item[i]);
	}
	if (op->ob_item != NULL)
		free((ANY *)op->ob_item);
	free((ANY *)op);
}

static int
list_print(op, fp, flags)
	listobject *op;
	FILE *fp;
	int flags;
{
	int i;
	fprintf(fp, "[");
	for (i = 0; i < op->ob_size; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		if (printobject(op->ob_item[i], fp, flags) != 0)
			return -1;
	}
	fprintf(fp, "]");
	return 0;
}

object *
list_repr(v)
	listobject *v;
{
	object *s, *t, *comma;
	int i;
	s = newstringobject("[");
	comma = newstringobject(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			joinstring(&s, comma);
		t = reprobject(v->ob_item[i]);
		joinstring(&s, t);
		DECREF(t);
	}
	DECREF(comma);
	t = newstringobject("]");
	joinstring(&s, t);
	DECREF(t);
	return s;
}

static int
list_compare(v, w)
	listobject *v, *w;
{
	int len = (v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	int i;
	for (i = 0; i < len; i++) {
		int cmp = cmpobject(v->ob_item[i], w->ob_item[i]);
		if (cmp != 0)
			return cmp;
	}
	return v->ob_size - w->ob_size;
}

static int
list_length(a)
	listobject *a;
{
	return a->ob_size;
}

static object *
list_item(a, i)
	listobject *a;
	int i;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "list index out of range");
		return NULL;
	}
	INCREF(a->ob_item[i]);
	return a->ob_item[i];
}

static object *
list_slice(a, ilow, ihigh)
	listobject *a;
	int ilow, ihigh;
{
	listobject *np;
	int i;
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	np = (listobject *) newlistobject(ihigh - ilow);
	if (np == NULL)
		return NULL;
	for (i = ilow; i < ihigh; i++) {
		object *v = a->ob_item[i];
		INCREF(v);
		np->ob_item[i - ilow] = v;
	}
	return (object *)np;
}

static object *
list_concat(a, bb)
	listobject *a;
	object *bb;
{
	int size;
	int i;
	listobject *np;
	if (!is_listobject(bb)) {
		err_badarg();
		return NULL;
	}
#define b ((listobject *)bb)
	size = a->ob_size + b->ob_size;
	np = (listobject *) newlistobject(size);
	if (np == NULL) {
		return NULL;
	}
	for (i = 0; i < a->ob_size; i++) {
		object *v = a->ob_item[i];
		INCREF(v);
		np->ob_item[i] = v;
	}
	for (i = 0; i < b->ob_size; i++) {
		object *v = b->ob_item[i];
		INCREF(v);
		np->ob_item[i + a->ob_size] = v;
	}
	return (object *)np;
#undef b
}

static object *
list_repeat(a, n)
	listobject *a;
	int n;
{
	int i, j;
	int size;
	listobject *np;
	object **p;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	np = (listobject *) newlistobject(size);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	for (i = 0; i < n; i++) {
		for (j = 0; j < a->ob_size; j++) {
			*p = a->ob_item[j];
			INCREF(*p);
			p++;
		}
	}
	return (object *) np;
}

/* XXX The following function has a bug: don't try assigning a
   XXX list object to a slice of itself (e.g., a[:1] = a). */

static int
list_ass_slice(a, ilow, ihigh, v)
	listobject *a;
	int ilow, ihigh;
	object *v;
{
	object **item;
	int n; /* Size of replacement list */
	int d; /* Change in size */
	int k; /* Loop index */
#define b ((listobject *)v)
	if (v == NULL)
		n = 0;
	else if (is_listobject(v)) {
		n = b->ob_size;
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = list_slice(b, 0, n);
			ret = list_ass_slice(a, ilow, ihigh, v);
			DECREF(v);
			return ret;
		}
	}
	else {
		err_badarg();
		return -1;
	}
	if (ilow < 0)
		ilow = 0;
	else if (ilow > a->ob_size)
		ilow = a->ob_size;
	if (ihigh < 0)
		ihigh = 0;
	if (ihigh < ilow)
		ihigh = ilow;
	else if (ihigh > a->ob_size)
		ihigh = a->ob_size;
	item = a->ob_item;
	d = n - (ihigh-ilow);
	if (d <= 0) { /* Delete -d items; DECREF ihigh-ilow items */
		for (k = ilow; k < ihigh; k++)
			DECREF(item[k]);
		if (d < 0) {
			for (/*k = ihigh*/; k < a->ob_size; k++)
				item[k+d] = item[k];
			a->ob_size += d;
			RESIZE(item, object *, a->ob_size); /* Can't fail */
			a->ob_item = item;
		}
	}
	else { /* Insert d items; DECREF ihigh-ilow items */
		RESIZE(item, object *, a->ob_size + d);
		if (item == NULL) {
			err_nomem();
			return -1;
		}
		for (k = a->ob_size; --k >= ihigh; )
			item[k+d] = item[k];
		for (/*k = ihigh-1*/; k >= ilow; --k)
			DECREF(item[k]);
		a->ob_item = item;
		a->ob_size += d;
	}
	for (k = 0; k < n; k++, ilow++) {
		object *w = b->ob_item[k];
		INCREF(w);
		item[ilow] = w;
	}
	return 0;
#undef b
}

static int
list_ass_item(a, i, v)
	listobject *a;
	int i;
	object *v;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "list assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return list_ass_slice(a, i, i+1, v);
	INCREF(v);
	DECREF(a->ob_item[i]);
	a->ob_item[i] = v;
	return 0;
}

static object *
ins(self, where, v)
	listobject *self;
	int where;
	object *v;
{
	if (ins1(self, where, v) != 0)
		return NULL;
	INCREF(None);
	return None;
}

static object *
listinsert(self, args)
	listobject *self;
	object *args;
{
	int i;
	if (args == NULL || !is_tupleobject(args) || gettuplesize(args) != 2) {
		err_badarg();
		return NULL;
	}
	if (!getintarg(gettupleitem(args, 0), &i))
		return NULL;
	return ins(self, i, gettupleitem(args, 1));
}

static object *
listappend(self, args)
	listobject *self;
	object *args;
{
	return ins(self, (int) self->ob_size, args);
}

static int
cmp(v, w)
	char *v, *w;
{
	return cmpobject(* (object **) v, * (object **) w);
}

static object *
listsort(self, args)
	listobject *self;
	object *args;
{
	if (args != NULL) {
		err_badarg();
		return NULL;
	}
	err_clear();
	if (self->ob_size > 1)
		qsort((char *)self->ob_item,
				(int) self->ob_size, sizeof(object *), cmp);
	if (err_occurred())
		return NULL;
	INCREF(None);
	return None;
}

static object *
listreverse(self, args)
	listobject *self;
	object *args;
{
	register object **p, **q;
	register object *tmp;
	
	if (args != NULL) {
		err_badarg();
		return NULL;
	}

	if (self->ob_size > 1) {
		for (p = self->ob_item, q = self->ob_item + self->ob_size - 1;
						p < q; p++, q--) {
			tmp = *p;
			*p = *q;
			*q = tmp;
		}
	}
	
	INCREF(None);
	return None;
}

int
sortlist(v)
	object *v;
{
	if (v == NULL || !is_listobject(v)) {
		err_badcall();
		return -1;
	}
	v = listsort((listobject *)v, (object *)NULL);
	if (v == NULL)
		return -1;
	DECREF(v);
	return 0;
}

static object *
listindex(self, args)
	listobject *self;
	object *args;
{
	int i;
	
	if (args == NULL) {
		err_badarg();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (cmpobject(self->ob_item[i], args) == 0)
			return newintobject(i);
	}
	err_setstr(ValueError, "list.index(x): x not in list");
	return NULL;
}

static object *
listcount(self, args)
	listobject *self;
	object *args;
{
	int count = 0;
	int i;
	
	if (args == NULL) {
		err_badarg();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (cmpobject(self->ob_item[i], args) == 0)
			count++;
	}
	return newintobject((long)count);
}

static object *
listremove(self, args)
	listobject *self;
	object *args;
{
	int i;
	
	if (args == NULL) {
		err_badarg();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (cmpobject(self->ob_item[i], args) == 0) {
			if (list_ass_slice(self, i, i+1, (object *)NULL) != 0)
				return NULL;
			INCREF(None);
			return None;
		}
			
	}
	err_setstr(ValueError, "list.remove(x): x not in list");
	return NULL;
}

static struct methodlist list_methods[] = {
	{"append",	listappend},
	{"count",	listcount},
	{"index",	listindex},
	{"insert",	listinsert},
	{"sort",	listsort},
	{"remove",	listremove},
	{"reverse",	listreverse},
	{NULL,		NULL}		/* sentinel */
};

static object *
list_getattr(f, name)
	listobject *f;
	char *name;
{
	return findmethod(list_methods, (object *)f, name);
}

static sequence_methods list_as_sequence = {
	list_length,	/*sq_length*/
	list_concat,	/*sq_concat*/
	list_repeat,	/*sq_repeat*/
	list_item,	/*sq_item*/
	list_slice,	/*sq_slice*/
	list_ass_item,	/*sq_ass_item*/
	list_ass_slice,	/*sq_ass_slice*/
};

typeobject Listtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"list",
	sizeof(listobject),
	0,
	list_dealloc,	/*tp_dealloc*/
	list_print,	/*tp_print*/
	list_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	list_compare,	/*tp_compare*/
	list_repr,	/*tp_repr*/
	0,		/*tp_as_number*/
	&list_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
