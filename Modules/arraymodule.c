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

/* Array object implementation */

/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"

#ifdef sun
#define NEED_MEMMOVE
#endif

#ifdef NEED_MEMMOVE
extern char *memmove();
#endif

struct arrayobject; /* Forward */

struct arraydescr {
	int typecode;
	int itemsize;
	object * (*getitem) FPROTO((struct arrayobject *, int));
	int (*setitem) FPROTO((struct arrayobject *, int, object *));
};

typedef struct arrayobject {
	OB_VARHEAD
	char *ob_item;
	struct arraydescr *ob_descr;
} arrayobject;

extern typeobject Arraytype;

#define is_arrayobject(op) ((op)->ob_type == &Arraytype)

extern object *newarrayobject PROTO((int, struct arraydescr *));
extern int getarraysize PROTO((object *));
extern object *getarrayitem PROTO((object *, int));
extern int setarrayitem PROTO((object *, int, object *));
extern int insarrayitem PROTO((object *, int, object *));
extern int addarrayitem PROTO((object *, object *));

static object *
c_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return newsizedstringobject(&((char *)ap->ob_item)[i], 1);
}

static int
c_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	char x;
	if (!getargs(v, "c;array item must be char", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static object *
b_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	long x = ((char *)ap->ob_item)[i];
	if (x >= 128)
		x -= 256;
	return newintobject(x);
}

static int
b_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	char x;
	if (!getargs(v, "b;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((char *)ap->ob_item)[i] = x;
	return 0;
}

static object *
h_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return newintobject((long) ((short *)ap->ob_item)[i]);
}

static int
h_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	short x;
	if (!getargs(v, "h;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((short *)ap->ob_item)[i] = x;
	return 0;
}

static object *
l_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return newintobject(((long *)ap->ob_item)[i]);
}

static int
l_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	long x;
	if (!getargs(v, "l;array item must be integer", &x))
		return -1;
	if (i >= 0)
		     ((long *)ap->ob_item)[i] = x;
	return 0;
}

static object *
f_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return newfloatobject((double) ((float *)ap->ob_item)[i]);
}

static int
f_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	float x;
	if (!getargs(v, "f;array item must be float", &x))
		return -1;
	if (i >= 0)
		     ((float *)ap->ob_item)[i] = x;
	return 0;
}

static object *
d_getitem(ap, i)
	arrayobject *ap;
	int i;
{
	return newfloatobject(((double *)ap->ob_item)[i]);
}

static int
d_setitem(ap, i, v)
	arrayobject *ap;
	int i;
	object *v;
{
	double x;
	if (!getargs(v, "d;array item must be float", &x))
		return -1;
	if (i >= 0)
		     ((double *)ap->ob_item)[i] = x;
	return 0;
}

/* Description of types */
struct arraydescr descriptors[] = {
	{'c', sizeof(char), c_getitem, c_setitem},
	{'b', sizeof(char), b_getitem, b_setitem},
	{'h', sizeof(short), h_getitem, h_setitem},
	{'l', sizeof(long), l_getitem, l_setitem},
	{'f', sizeof(float), f_getitem, f_setitem},
	{'d', sizeof(double), d_getitem, d_setitem},
	{'\0', 0, 0, 0} /* Sentinel */
};
	

object *
newarrayobject(size, descr)
	int size;
	struct arraydescr *descr;
{
	int i;
	arrayobject *op;
	MALLARG nbytes;
	int itemsize;
	if (size < 0) {
		err_badcall();
		return NULL;
	}
	nbytes = size * descr->itemsize;
	/* Check for overflow */
	if (nbytes / descr->itemsize != size) {
		return err_nomem();
	}
	op = (arrayobject *) malloc(sizeof(arrayobject));
	if (op == NULL) {
		return err_nomem();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = malloc(nbytes);
		if (op->ob_item == NULL) {
			free((ANY *)op);
			return err_nomem();
		}
	}
	NEWREF(op);
	op->ob_type = &Arraytype;
	op->ob_size = size;
	op->ob_descr = descr;
	return (object *) op;
}

int
getarraysize(op)
	object *op;
{
	if (!is_arrayobject(op)) {
		err_badcall();
		return -1;
	}
	return ((arrayobject *)op) -> ob_size;
}

object *
getarrayitem(op, i)
	object *op;
	int i;
{
	register arrayobject *ap;
	if (!is_arrayobject(op)) {
		err_badcall();
		return NULL;
	}
	ap = (arrayobject *)op;
	if (i < 0 || i >= ap->ob_size) {
		err_setstr(IndexError, "array index out of range");
		return NULL;
	}
	return (*ap->ob_descr->getitem)(ap, i);
}

static int
ins1(self, where, v)
	arrayobject *self;
	int where;
	object *v;
{
	int i;
	char *items;
	if (v == NULL) {
		err_badcall();
		return -1;
	}
	if ((*self->ob_descr->setitem)(self, -1, v) < 0)
		return -1;
	items = self->ob_item;
	RESIZE(items, char, (self->ob_size+1) * self->ob_descr->itemsize);
	if (items == NULL) {
		err_nomem();
		return -1;
	}
	if (where < 0)
		where = 0;
	if (where > self->ob_size)
		where = self->ob_size;
	memmove(items + (where+1)*self->ob_descr->itemsize,
		items + where*self->ob_descr->itemsize,
		(self->ob_size-where)*self->ob_descr->itemsize);
	self->ob_item = items;
	self->ob_size++;
	return (*self->ob_descr->setitem)(self, where, v);
}

int
insarrayitem(op, where, newitem)
	object *op;
	int where;
	object *newitem;
{
	if (!is_arrayobject(op)) {
		err_badcall();
		return -1;
	}
	return ins1((arrayobject *)op, where, newitem);
}

int
addarrayitem(op, newitem)
	object *op;
	object *newitem;
{
	if (!is_arrayobject(op)) {
		err_badcall();
		return -1;
	}
	return ins1((arrayobject *)op,
		(int) ((arrayobject *)op)->ob_size, newitem);
}

/* Methods */

static void
array_dealloc(op)
	arrayobject *op;
{
	int i;
	if (op->ob_item != NULL)
		free((ANY *)op->ob_item);
	free((ANY *)op);
}

static int
array_compare(v, w)
	arrayobject *v, *w;
{
	int len = (v->ob_size < w->ob_size) ? v->ob_size : w->ob_size;
	int i;
	for (i = 0; i < len; i++) {
		object *ai, *bi;
		int cmp;
		ai = getarrayitem((object *)v, i);
		bi = getarrayitem((object *)w, i);
		if (ai && bi)
			cmp = cmpobject(ai, bi);
		else
			cmp = -1;
		XDECREF(ai);
		XDECREF(bi);
		if (cmp != 0) {
			err_clear(); /* XXX Can't report errors here */
			return cmp;
		}
	}
	return v->ob_size - w->ob_size;
}

static int
array_length(a)
	arrayobject *a;
{
	return a->ob_size;
}

static object *
array_item(a, i)
	arrayobject *a;
	int i;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "array index out of range");
		return NULL;
	}
	return getarrayitem((object *)a, i);
}

static object *
array_slice(a, ilow, ihigh)
	arrayobject *a;
	int ilow, ihigh;
{
	arrayobject *np;
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
	np = (arrayobject *) newarrayobject(ihigh - ilow, a->ob_descr);
	if (np == NULL)
		return NULL;
	memcpy(np->ob_item, a->ob_item + ilow * a->ob_descr->itemsize,
	       (ihigh-ilow) * a->ob_descr->itemsize);
	return (object *)np;
}

static object *
array_concat(a, bb)
	arrayobject *a;
	object *bb;
{
	int size;
	int i;
	arrayobject *np;
	if (!is_arrayobject(bb)) {
		err_badarg();
		return NULL;
	}
#define b ((arrayobject *)bb)
	if (a->ob_descr != b->ob_descr) {
		err_badarg();
		return NULL;
	}
	size = a->ob_size + b->ob_size;
	np = (arrayobject *) newarrayobject(size, a->ob_descr);
	if (np == NULL) {
		return NULL;
	}
	memcpy(np->ob_item, a->ob_item, a->ob_size*a->ob_descr->itemsize);
	memcpy(np->ob_item + a->ob_size*a->ob_descr->itemsize,
	       a->ob_item, a->ob_size*a->ob_descr->itemsize);
	return (object *)np;
#undef b
}

static object *
array_repeat(a, n)
	arrayobject *a;
	int n;
{
	int i, j;
	int size;
	arrayobject *np;
	char *p;
	int nbytes;
	if (n < 0)
		n = 0;
	size = a->ob_size * n;
	np = (arrayobject *) newarrayobject(size, a->ob_descr);
	if (np == NULL)
		return NULL;
	p = np->ob_item;
	nbytes = a->ob_size * a->ob_descr->itemsize;
	for (i = 0; i < n; i++) {
		memcpy(p, a->ob_item, nbytes);
		p += nbytes;
	}
	return (object *) np;
}

static int
array_ass_slice(a, ilow, ihigh, v)
	arrayobject *a;
	int ilow, ihigh;
	object *v;
{
	char *item;
	int n; /* Size of replacement array */
	int d; /* Change in size */
	int k; /* Loop index */
#define b ((arrayobject *)v)
	if (v == NULL)
		n = 0;
	else if (is_arrayobject(v)) {
		n = b->ob_size;
		if (a == b) {
			/* Special case "a[i:j] = a" -- copy b first */
			int ret;
			v = array_slice(b, 0, n);
			ret = array_ass_slice(a, ilow, ihigh, v);
			DECREF(v);
			return ret;
		}
		if (b->ob_descr != a->ob_descr) {
			err_badarg();
			return -1;
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
	if (d < 0) { /* Delete -d items */
		memmove(item + (ihigh+d)*a->ob_descr->itemsize,
			item + ihigh*a->ob_descr->itemsize,
			(a->ob_size-ihigh)*a->ob_descr->itemsize);
		a->ob_size += d;
		RESIZE(item, char, a->ob_size*a->ob_descr->itemsize);
						/* Can't fail */
		a->ob_item = item;
	}
	else if (d > 0) { /* Insert d items */
		RESIZE(item, char, (a->ob_size + d)*a->ob_descr->itemsize);
		if (item == NULL) {
			err_nomem();
			return -1;
		}
		memmove(item + (ihigh+d)*a->ob_descr->itemsize,
			item + ihigh*a->ob_descr->itemsize,
			(a->ob_size-ihigh)*a->ob_descr->itemsize);
		a->ob_item = item;
		a->ob_size += d;
	}
	if (n > 0)
		memcpy(item + ilow*a->ob_descr->itemsize, b->ob_item,
		       n*b->ob_descr->itemsize);
	return 0;
#undef b
}

static int
array_ass_item(a, i, v)
	arrayobject *a;
	int i;
	object *v;
{
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "array assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return array_ass_slice(a, i, i+1, v);
	return (*a->ob_descr->setitem)(a, i, v);
}

static int
setarrayitem(a, i, v)
	object *a;
	int i;
	object *v;
{
	if (!is_arrayobject(a)) {
		err_badcall();
		return -1;
	}
	return array_ass_item((arrayobject *)a, i, v);
}

static object *
ins(self, where, v)
	arrayobject *self;
	int where;
	object *v;
{
	if (ins1(self, where, v) != 0)
		return NULL;
	INCREF(None);
	return None;
}

static object *
array_insert(self, args)
	arrayobject *self;
	object *args;
{
	int i;
	object *v;
	if (!getargs(args, "(iO)", &i, &v))
		return NULL;
	return ins(self, i, v);
}

static object *
array_append(self, args)
	arrayobject *self;
	object *args;
{
	object *v;
	if (!getargs(args, "O", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

static object *
array_byteswap(self, args)
	arrayobject *self;
	object *args;
{
	char *p;
	int i;
	switch (self->ob_descr->itemsize) {
	case 1:
		break;
	case 2:
		for (p = self->ob_item, i = self->ob_size; --i >= 0; p += 2) {
			char p0 = p[0];
			p[0] = p[1];
			p[1] = p0;
		}
		break;
	case 4:
		for (p = self->ob_item, i = self->ob_size; --i >= 0; p += 4) {
			char p0 = p[0];
			char p1 = p[1];
			p[0] = p[3];
			p[1] = p[2];
			p[2] = p1;
			p[3] = p0;
		}
		break;
	default:
		err_setstr(RuntimeError,
			   "don't know how to byteswap this array type");
		return NULL;
	}
	INCREF(None);
	return None;
}

#if 0
static object *
array_reverse(self, args)
	arrayobject *self;
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
#endif

#if 0
static object *
array_index(self, args)
	arrayobject *self;
	object *args;
{
	int i;
	
	if (args == NULL) {
		err_badarg();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (cmpobject(self->ob_item[i], args) == 0)
			return newintobject((long)i);
	}
	err_setstr(ValueError, "array.index(x): x not in array");
	return NULL;
}
#endif

#if 0
static object *
array_count(self, args)
	arrayobject *self;
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
#endif

#if 0
static object *
array_remove(self, args)
	arrayobject *self;
	object *args;
{
	int i;
	
	if (args == NULL) {
		err_badarg();
		return NULL;
	}
	for (i = 0; i < self->ob_size; i++) {
		if (cmpobject(self->ob_item[i], args) == 0) {
			if (array_ass_slice(self, i, i+1, (object *)NULL) != 0)
				return NULL;
			INCREF(None);
			return None;
		}
			
	}
	err_setstr(ValueError, "array.remove(x): x not in array");
	return NULL;
}
#endif

static object *
array_read(self, args)
	arrayobject *self;
	object *args;
{
	object *f;
	int n;
	FILE *fp;
	if (!getargs(args, "(Oi)", &f, &n))
		return NULL;
	fp = getfilefile(f);
	if (fp == NULL) {
		err_setstr(TypeError, "arg1 must be open file");
		return NULL;
	}
	if (n > 0) {
		char *item = self->ob_item;
		int itemsize = self->ob_descr->itemsize;
		int nread;
		RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			err_nomem();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		nread = fread(item + (self->ob_size - n) * itemsize,
			      itemsize, n, fp);
		if (nread < n) {
			self->ob_size -= (n - nread);
			RESIZE(item, char, self->ob_size*itemsize);
			self->ob_item = item;
			err_setstr(EOFError, "not enough items in file");
			return NULL;
		}
	}
	INCREF(None);
	return None;
}

static object *
array_write(self, args)
	arrayobject *self;
	object *args;
{
	object *f;
	FILE *fp;
	if (!getargs(args, "O", &f))
		return NULL;
	fp = getfilefile(f);
	if (fp == NULL) {
		err_setstr(TypeError, "arg must be open file");
		return NULL;
	}
	if (self->ob_size > 0) {
		if (fwrite(self->ob_item, self->ob_descr->itemsize,
			   self->ob_size, fp) != self->ob_size) {
			err_errno(IOError);
			clearerr(fp);
			return NULL;
		}
	}
	INCREF(None);
	return None;
}

static object *
array_fromlist(self, args)
	arrayobject *self;
	object *args;
{
	int n;
	object *list;
	int itemsize = self->ob_descr->itemsize;
	if (!getargs(args, "O", &list))
		return NULL;
	if (!is_listobject(list)) {
		err_setstr(TypeError, "arg must be list");
		return NULL;
	}
	n = getlistsize(list);
	if (n > 0) {
		char *item = self->ob_item;
		int i;
		RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			err_nomem();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		for (i = 0; i < n; i++) {
			object *v = getlistitem(list, i);
			if ((*self->ob_descr->setitem)(self,
					self->ob_size - n + i, v) != 0) {
				self->ob_size -= n;
				RESIZE(item, char, self->ob_size * itemsize);
				self->ob_item = item;
				return NULL;
			}
		}
	}
	INCREF(None);
	return None;
}

static object *
array_tolist(self, args)
	arrayobject *self;
	object *args;
{
	object *list = newlistobject(self->ob_size);
	int i;
	if (list == NULL)
		return NULL;
	for (i = 0; i < self->ob_size; i++) {
		object *v = getarrayitem((object *)self, i);
		if (v == NULL) {
			DECREF(list);
			return NULL;
		}
		setlistitem(list, i, v);
	}
	return list;
}

static object *
array_fromstring(self, args)
	arrayobject *self;
	object *args;
{
	char *str;
	int n;
	int itemsize = self->ob_descr->itemsize;
	if (!getargs(args, "s#", &str, &n))
		return NULL;
	if (n % itemsize != 0) {
		err_setstr(ValueError,
			   "string length not a multiple of item size");
		return NULL;
	}
	n = n / itemsize;
	if (n > 0) {
		char *item = self->ob_item;
		RESIZE(item, char, (self->ob_size + n) * itemsize);
		if (item == NULL) {
			err_nomem();
			return NULL;
		}
		self->ob_item = item;
		self->ob_size += n;
		memcpy(item + (self->ob_size - n) * itemsize, str, itemsize*n);
	}
	INCREF(None);
	return None;
}

static object *
array_tostring(self, args)
	arrayobject *self;
	object *args;
{
	if (!getargs(args, ""))
		return NULL;
	return newsizedstringobject(self->ob_item,
				    self->ob_size * self->ob_descr->itemsize);
}

static struct methodlist array_methods[] = {
	{"append",	array_append},
	{"byteswap",	array_byteswap},
/*	{"count",	array_count},*/
/*	{"index",	array_index},*/
	{"insert",	array_insert},
/*	{"sort",	array_sort},*/
/*	{"remove",	array_remove},*/
/*	{"reverse",	array_reverse},*/
	{"read",	array_read},
	{"write",	array_write},
	{"fromlist",	array_fromlist},
	{"tolist",	array_tolist},
	{"fromstring",	array_fromstring},
	{"tostring",	array_tostring},
	{NULL,		NULL}		/* sentinel */
};

static object *
array_getattr(a, name)
	arrayobject *a;
	char *name;
{
	if (strcmp(name, "typecode") == 0) {
		char tc = a->ob_descr->typecode;
		return newsizedstringobject(&tc, 1);
	}
	if (strcmp(name, "itemsize") == 0) {
		return newintobject((long)a->ob_descr->itemsize);
	}
	return findmethod(array_methods, (object *)a, name);
}

static int
array_print(a, fp, flags)
	arrayobject *a;
	FILE *fp;
	int flags;
{
	int ok = 0;
	int i, len;
	object *v;
	len = a->ob_size;
	if (len == 0) {
		fprintf(fp, "array('%c')", a->ob_descr->typecode);
		return ok;
	}
	if (a->ob_descr->typecode == 'c') {
		fprintf(fp, "array('c', ");
		v = array_tostring(a, (object *)NULL);
		ok = printobject(v, fp, flags);
		XDECREF(v);
		fprintf(fp, ")");
		return ok;
	}
	fprintf(fp, "array('%c', [", a->ob_descr->typecode);
	for (i = 0; i < len && ok == 0; i++) {
		if (i > 0)
			fprintf(fp, ", ");
		v = (a->ob_descr->getitem)(a, i);
		ok = printobject(v, fp, flags);
		XDECREF(v);
	}
	fprintf(fp, "])");
	return ok;
}

static object *
array_repr(a)
	arrayobject *a;
{
	char buf[256];
	object *s, *t, *comma, *v;
	int i, len;
	len = a->ob_size;
	if (len == 0) {
		sprintf(buf, "array('%c')", a->ob_descr->typecode);
		return newstringobject(buf);
	}
	if (a->ob_descr->typecode == 'c') {
		sprintf(buf, "array('c', ");
		s = newstringobject(buf);
		v = array_tostring(a, (object *)NULL);
		t = reprobject(v);
		XDECREF(v);
		joinstring(&s, t);
		XDECREF(t);
		t = newstringobject(")");
		joinstring(&s, t);
		XDECREF(t);
		if (err_occurred()) {
			XDECREF(s);
			s = NULL;
		}
		return s;
	}
	sprintf(buf, "array('%c', [", a->ob_descr->typecode);
	s = newstringobject(buf);
	comma = newstringobject(", ");
	for (i = 0; i < len && !err_occurred(); i++) {
		v = (a->ob_descr->getitem)(a, i);
		t = reprobject(v);
		XDECREF(v);
		if (i > 0)
			joinstring(&s, comma);
		joinstring(&s, t);
		XDECREF(t);
	}
	XDECREF(comma);
	t = newstringobject("])");
	joinstring(&s, t);
	XDECREF(t);
	if (err_occurred()) {
		XDECREF(s);
		s = NULL;
	}
	return s;
}

static sequence_methods array_as_sequence = {
	array_length,	/*sq_length*/
	array_concat,	/*sq_concat*/
	array_repeat,	/*sq_repeat*/
	array_item,	/*sq_item*/
	array_slice,	/*sq_slice*/
	array_ass_item,	/*sq_ass_item*/
	array_ass_slice, /*sq_ass_slice*/
};

typeobject Arraytype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"array",
	sizeof(arrayobject),
	0,
	array_dealloc,	/*tp_dealloc*/
	array_print,	/*tp_print*/
	array_getattr,	/*tp_getattr*/
	0,		/*tp_setattr*/
	array_compare,	/*tp_compare*/
	array_repr,	/*tp_repr*/
	0,		/*tp_as_number*/
	&array_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};


static object *
a_array(self, args)
	object *self;
	object *args;
{
	char c;
	object *initial = NULL;
	struct arraydescr *descr;
	if (!getargs(args, "c", &c)) {
		err_clear();
		if (!getargs(args, "(cO)", &c, &initial))
			return NULL;
		if (!is_listobject(initial) && !is_stringobject(initial)) {
			err_setstr(TypeError,
				   "array initializer must be list or string");
			return NULL;
		}
	}
	for (descr = descriptors; descr->typecode != '\0'; descr++) {
		if (descr->typecode == c) {
			object *a;
			int len;
			if (initial == NULL || !is_listobject(initial))
				len = 0;
			else
				len = getlistsize(initial);
			a = newarrayobject(len, descr);
			if (a == NULL)
				return NULL;
			if (len > 0) {
				int i;
				for (i = 0; i < len; i++) {
					object *v = getlistitem(initial, i);
					if (setarrayitem(a, i, v) != 0) {
						DECREF(a);
						return NULL;
					}
				}
			}
			if (initial != NULL && is_stringobject(initial)) {
				object *v =
				  array_fromstring((arrayobject *)a, initial);
				if (v == NULL) {
					DECREF(a);
					return NULL;
				}
				DECREF(v);
			}
			return a;
		}
	}
	err_setstr(ValueError, "bad typecode (must be c, b, h, l, f or d)");
	return NULL;
}

static struct methodlist a_methods[] = {
	{"array",	a_array},
	{NULL,		NULL}		/* sentinel */
};

void
initarray()
{
	initmodule("array", a_methods);
}


#ifdef NEED_MEMMOVE

/* A perhaps slow but I hope correct implementation of memmove */

char *memmove(dst, src, n)
	char *dst;
	char *src;
	int n;
{
	char *realdst = dst;
	if (n <= 0)
		return dst;
	if (src >= dst+n || dst >= src+n)
		return memcpy(dst, src, n);
	if (src > dst) {
		while (--n >= 0)
			*dst++ = *src++;
	}
	else if (src < dst) {
		src += n;
		dst += n;
		while (--n >= 0)
			*--dst = *--src;
	}
	return realdst;
}

#endif
