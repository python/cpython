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

/* List object implementation */

#include "allobjects.h"
#include "modsupport.h"
#include "ceval.h"
#ifdef STDC_HEADERS
#include <stddef.h>
#else
#include <sys/types.h>		/* For size_t */
#endif

#define ROUNDUP(n, block) ((((n)+(block)-1)/(block))*(block))

static int
roundup(n)
	int n;
{
	if (n < 500)
		return ROUNDUP(n, 10);
	else
		return ROUNDUP(n, 100);
}

#define NRESIZE(var, type, nitems) RESIZE(var, type, roundup(nitems))

object *
newlistobject(size)
	int size;
{
	int i;
	listobject *op;
	size_t nbytes;
	if (size < 0) {
		err_badcall();
		return NULL;
	}
	nbytes = size * sizeof(object *);
	/* Check for overflow */
	if (nbytes / sizeof(object *) != size) {
		return err_nomem();
	}
	op = (listobject *) malloc(sizeof(listobject));
	if (op == NULL) {
		return err_nomem();
	}
	if (size <= 0) {
		op->ob_item = NULL;
	}
	else {
		op->ob_item = (object **) malloc(nbytes);
		if (op->ob_item == NULL) {
			free((ANY *)op);
			return err_nomem();
		}
	}
	op->ob_type = &Listtype;
	op->ob_size = size;
	for (i = 0; i < size; i++)
		op->ob_item[i] = NULL;
	NEWREF(op);
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

static object *indexerr;

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
		if (indexerr == NULL)
			indexerr = newstringobject("list index out of range");
		err_setval(IndexError, indexerr);
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
	register object **p;
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
	p = ((listobject *)op) -> ob_item + i;
	olditem = *p;
	*p = newitem;
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
	NRESIZE(items, object *, self->ob_size+1);
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
	if (op->ob_item != NULL) {
		for (i = 0; i < op->ob_size; i++) {
			XDECREF(op->ob_item[i]);
		}
		free((ANY *)op->ob_item);
	}
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
		if (printobject(op->ob_item[i], fp, 0) != 0)
			return -1;
	}
	fprintf(fp, "]");
	return 0;
}

static object *
list_repr(v)
	listobject *v;
{
	object *s, *comma;
	int i;
	s = newstringobject("[");
	comma = newstringobject(", ");
	for (i = 0; i < v->ob_size && s != NULL; i++) {
		if (i > 0)
			joinstring(&s, comma);
		joinstring_decref(&s, reprobject(v->ob_item[i]));
	}
	XDECREF(comma);
	joinstring_decref(&s, newstringobject("]"));
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
		if (indexerr == NULL)
			indexerr = newstringobject("list index out of range");
		err_setval(IndexError, indexerr);
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

object *
getlistslice(a, ilow, ihigh)
	object *a;
	int ilow, ihigh;
{
	if (!is_listobject(a)) {
		err_badcall();
		return NULL;
	}
	return list_slice((listobject *)a, ilow, ihigh);
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

static int
list_ass_slice(a, ilow, ihigh, v)
	listobject *a;
	int ilow, ihigh;
	object *v;
{
	/* Because [X]DECREF can recursively invoke list operations on
	   this list, we must postpone all [X]DECREF activity until
	   after the list is back in its canonical shape.  Therefore
	   we must allocate an additional array, 'recycle', into which
	   we temporarily copy the items that are deleted from the
	   list. :-( */
	object **recycle, **p;
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
	if (ihigh > ilow)
		p = recycle = NEW(object *, (ihigh-ilow));
	else
		p = recycle = NULL;
	if (d <= 0) { /* Delete -d items; recycle ihigh-ilow items */
		for (k = ilow; k < ihigh; k++)
			*p++ = item[k];
		if (d < 0) {
			for (/*k = ihigh*/; k < a->ob_size; k++)
				item[k+d] = item[k];
			a->ob_size += d;
			NRESIZE(item, object *, a->ob_size); /* Can't fail */
			a->ob_item = item;
		}
	}
	else { /* Insert d items; recycle ihigh-ilow items */
		NRESIZE(item, object *, a->ob_size + d);
		if (item == NULL) {
			XDEL(recycle);
			err_nomem();
			return -1;
		}
		for (k = a->ob_size; --k >= ihigh; )
			item[k+d] = item[k];
		for (/*k = ihigh-1*/; k >= ilow; --k)
			*p++ = item[k];
		a->ob_item = item;
		a->ob_size += d;
	}
	for (k = 0; k < n; k++, ilow++) {
		object *w = b->ob_item[k];
		XINCREF(w);
		item[ilow] = w;
	}
	if (recycle) {
		while (--p >= recycle)
			XDECREF(*p);
		DEL(recycle);
	}
	return 0;
#undef b
}

int
setlistslice(a, ilow, ihigh, v)
	object *a;
	int ilow, ihigh;
	object *v;
{
	if (!is_listobject(a)) {
		err_badcall();
		return -1;
	}
	return list_ass_slice((listobject *)a, ilow, ihigh, v);
}

static int
list_ass_item(a, i, v)
	listobject *a;
	int i;
	object *v;
{
	object *old_value;
	if (i < 0 || i >= a->ob_size) {
		err_setstr(IndexError, "list assignment index out of range");
		return -1;
	}
	if (v == NULL)
		return list_ass_slice(a, i, i+1, v);
	INCREF(v);
	old_value = a->ob_item[i];
	a->ob_item[i] = v;
	DECREF(old_value); 
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
	object *v;
	if (!getargs(args, "(iO)", &i, &v))
		return NULL;
	return ins(self, i, v);
}

static object *
listappend(self, args)
	listobject *self;
	object *args;
{
	object *v;
	if (!getargs(args, "O", &v))
		return NULL;
	return ins(self, (int) self->ob_size, v);
}

#define NEWSORT

#ifdef NEWSORT

/* New quicksort implementation for arrays of object pointers.
   Thanks to discussions with Tim Peters. */

/* CMPERROR is returned by our comparison function when an error
   occurred.  This is the largest negative integer (0x80000000 on a
   32-bit system). */
#define CMPERROR ( (int) ((unsigned int)1 << (8*sizeof(int) - 1)) )

/* Comparison function.  Takes care of calling a user-supplied
   comparison function (any callable Python object).  Calls the
   standard comparison function, cmpobject(), if the user-supplied
   function is NULL. */

static int
docompare(x, y, compare)
	object *x;
	object *y;
	object *compare;
{
	object *args, *res;
	int i;

	if (compare == NULL)
		return cmpobject(x, y);

	args = mkvalue("(OO)", x, y);
	if (args == NULL)
		return CMPERROR;
	res = call_object(compare, args);
	DECREF(args);
	if (res == NULL)
		return CMPERROR;
	if (!is_intobject(res)) {
		DECREF(res);
		err_setstr(TypeError, "comparison function should return int");
		return CMPERROR;
	}
	i = getintvalue(res);
	DECREF(res);
	if (i < 0)
		return -1;
	if (i > 0)
		return 1;
	return 0;
}

/* Straight insertion sort.  More efficient for sorting small arrays. */

static int
insertionsort(array, size, compare)
	object **array;	/* Start of array to sort */
	int size;	/* Number of elements to sort */
	object *compare;/* Comparison function object, or NULL for default */
{
	register object **a = array;
	register object **end = array+size;
	register object **p;

	for (p = a+1; p < end; p++) {
		register object *key = *p;
		register object **q = p;
		while (--q >= a) {
			register int k = docompare(*q, key, compare);
			if (k == CMPERROR)
				return -1;
			if (k <= 0)
				break;
			*(q+1) = *q;
			*q = key; /* For consistency */
		}
	}

	return 0;
}

/* MINSIZE is the smallest array we care to partition; smaller arrays
   are sorted using a straight insertion sort (above).  It must be at
   least 2 for the quicksort implementation to work.  Assuming that
   comparisons are more expensive than everything else (and this is a
   good assumption for Python), it should be 10, which is the cutoff
   point: quicksort requires more comparisons than insertion sort for
   smaller arrays. */
#define MINSIZE 10

/* STACKSIZE is the size of our work stack.  A rough estimate is that
   this allows us to sort arrays of MINSIZE * 2**STACKSIZE, or large
   enough.  (Because of the way we push the biggest partition first,
   the worst case occurs when all subarrays are always partitioned
   exactly in two.) */
#define STACKSIZE 64

/* Quicksort algorithm.  Return -1 if an exception occurred; in this
   case we leave the array partly sorted but otherwise in good health
   (i.e. no items have been removed or duplicated). */

static int
quicksort(array, size, compare)
	object **array;	/* Start of array to sort */
	int size;	/* Number of elements to sort */
	object *compare;/* Comparison function object, or NULL for default */
{
	register object *tmp, *pivot;
	register object **lo, **hi, **l, **r;
	int top, k, n, n2;
	object **lostack[STACKSIZE];
	object **histack[STACKSIZE];

	/* Start out with the whole array on the work stack */
	lostack[0] = array;
	histack[0] = array+size;
	top = 1;

	/* Repeat until the work stack is empty */
	while (--top >= 0) {
		lo = lostack[top];
		hi = histack[top];

		/* If it's a small one, use straight insertion sort */
		n = hi - lo;
		if (n < MINSIZE) {
			if (insertionsort(lo, n, compare) < 0)
				return -1;
			continue;
		}

		/* Choose median of first, middle and last item as pivot */
		
		l = lo + (n>>1); /* Middle */
		r = hi - 1;	/* Last */
		k = docompare(*lo, *l, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *lo; *lo = *l; *l = tmp; }
		k = docompare(*r, *l, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *r; *r = *l; *l = tmp; }
		k = docompare(*r, *lo, compare);
		if (k == CMPERROR)
			return -1;
		if (k < 0)
			{ tmp = *r; *r = *lo; *lo = tmp; }
		pivot = *lo;

		/* Partition the array */
		l = lo;
		r = hi;
		for (;;) {
			/* Move left index to element > pivot */
			while (++l < hi) {
				k = docompare(*l, pivot, compare);
				if (k == CMPERROR)
					return -1;
				if (k > 0)
					break;
			}
			/* Move right index to element < pivot */
			while (--r > lo) {
				k = docompare(*r, pivot, compare);
				if (k == CMPERROR)
					return -1;
				if (k < 0)
					break;
			}
			/* If they met, we're through */
			if (r < l)
				break;
			/* Swap elements and continue */
			{ tmp = *l; *l = *r; *r = tmp; }
		}

		/* Move the pivot into the middle */
		{ tmp = *lo; *lo = *r; *r = tmp; }

		/* We have now reached the following conditions:
			lo <= r < l <= hi
			all x in [lo,r) are <= pivot
			all x in [r,l)  are == pivot
			all x in [l,hi) are >= pivot
		   The partitions are [lo,r) and [l,hi)
		 */

		/* Push biggest partition first */
		n = r - lo;
		n2 = hi - l;
		if (n > n2) {
			/* First one is bigger */
			if (n > 1) {
				lostack[top] = lo;
				histack[top++] = r;
				if (n2 > 1) {
					lostack[top] = l;
					histack[top++] = hi;
				}
			}
		} else {
			/* Second one is bigger */
			if (n2 > 1) {
				lostack[top] = l;
				histack[top++] = hi;
				if (n > 1) {
					lostack[top] = lo;
					histack[top++] = r;
				}
			}
		}

		/* Should assert top < STACKSIZE-1 */
	}
	
	/* Succes */
	return 0;
}

static object *
listsort(self, compare)
	listobject *self;
	object *compare;
{
	/* XXX Don't you *dare* changing the list's length in compare()! */
	if (quicksort(self->ob_item, self->ob_size, compare) < 0)
		return NULL;
	INCREF(None);
	return None;
}

#else /* !NEWSORT */

static object *comparefunc;

static int
cmp(v, w)
	const ANY *v, *w;
{
	object *t, *res;
	long i;

	if (err_occurred())
		return 0;

	if (comparefunc == NULL)
		return cmpobject(* (object **) v, * (object **) w);

	/* Call the user-supplied comparison function */
	t = mkvalue("(OO)", * (object **) v, * (object **) w);
	if (t == NULL)
		return 0;
	res = call_object(comparefunc, t);
	DECREF(t);
	if (res == NULL)
		return 0;
	if (!is_intobject(res)) {
		err_setstr(TypeError, "comparison function should return int");
		i = 0;
	}
	else {
		i = getintvalue(res);
		if (i < 0)
			i = -1;
		else if (i > 0)
			i = 1;
	}
	DECREF(res);
	return (int) i;
}

static object *
listsort(self, args)
	listobject *self;
	object *args;
{
	object *save_comparefunc;
	if (self->ob_size <= 1) {
		INCREF(None);
		return None;
	}
	save_comparefunc = comparefunc;
	comparefunc = args;
	if (comparefunc != NULL) {
		/* Test the comparison function for obvious errors */
		(void) cmp((ANY *)&self->ob_item[0], (ANY *)&self->ob_item[1]);
		if (err_occurred()) {
			comparefunc = save_comparefunc;
			return NULL;
		}
	}
	qsort((char *)self->ob_item,
				(int) self->ob_size, sizeof(object *), cmp);
	comparefunc = save_comparefunc;
	if (err_occurred())
		return NULL;
	INCREF(None);
	return None;
}

#endif

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
reverselist(v)
	object *v;
{
	if (v == NULL || !is_listobject(v)) {
		err_badcall();
		return -1;
	}
	v = listreverse((listobject *)v, (object *)NULL);
	if (v == NULL)
		return -1;
	DECREF(v);
	return 0;
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

object *
listtuple(v)
	object *v;
{
	object *w;
	object **p;
	int n;
	if (v == NULL || !is_listobject(v)) {
		err_badcall();
		return NULL;
	}
	n = ((listobject *)v)->ob_size;
	w = newtupleobject(n);
	if (w == NULL)
		return NULL;
	p = ((tupleobject *)w)->ob_item;
	memcpy((ANY *)p,
	       (ANY *)((listobject *)v)->ob_item,
	       n*sizeof(object *));
	while (--n >= 0) {
		INCREF(*p);
		p++;
	}
	return w;
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
			return newintobject((long)i);
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
	{"append",	(method)listappend},
	{"count",	(method)listcount},
	{"index",	(method)listindex},
	{"insert",	(method)listinsert},
	{"sort",	(method)listsort, 0},
	{"remove",	(method)listremove},
	{"reverse",	(method)listreverse},
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
	(inquiry)list_length, /*sq_length*/
	(binaryfunc)list_concat, /*sq_concat*/
	(intargfunc)list_repeat, /*sq_repeat*/
	(intargfunc)list_item, /*sq_item*/
	(intintargfunc)list_slice, /*sq_slice*/
	(intobjargproc)list_ass_item, /*sq_ass_item*/
	(intintobjargproc)list_ass_slice, /*sq_ass_slice*/
};

typeobject Listtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"list",
	sizeof(listobject),
	0,
	(destructor)list_dealloc, /*tp_dealloc*/
	(printfunc)list_print, /*tp_print*/
	(getattrfunc)list_getattr, /*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)list_compare, /*tp_compare*/
	(reprfunc)list_repr, /*tp_repr*/
	0,		/*tp_as_number*/
	&list_as_sequence,	/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
};
