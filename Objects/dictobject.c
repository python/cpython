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

/* Dictionaru object implementation using a hash table */

#include "Python.h"


/*
 * MINSIZE is the minimum size of a dictionary.
 */

#define MINSIZE 4

/*
Table of irreducible polynomials to efficiently cycle through
GF(2^n)-{0}, 2<=n<=30.
*/
static long polys[] = {
	4 + 3,
	8 + 3,
	16 + 3,
	32 + 5,
	64 + 3,
	128 + 3,
	256 + 29,
	512 + 17,
	1024 + 9,
	2048 + 5,
	4096 + 83,
	8192 + 27,
	16384 + 43,
	32768 + 3,
	65536 + 45,
	131072 + 9,
	262144 + 39,
	524288 + 39,
	1048576 + 9,
	2097152 + 5,
	4194304 + 3,
	8388608 + 33,
	16777216 + 27,
	33554432 + 9,
	67108864 + 71,
	134217728 + 39,
	268435456 + 9,
	536870912 + 5,
	1073741824 + 83,
	0
};

/* Object used as dummy key to fill deleted entries */
static PyObject *dummy; /* Initialized by first call to newdictobject() */

/*
Invariant for entries: when in use, de_value is not NULL and de_key is
not NULL and not dummy; when not in use, de_value is NULL and de_key
is either NULL or dummy.  A dummy key value cannot be replaced by
NULL, since otherwise other keys may be lost.
*/
typedef struct {
	long me_hash;
	PyObject *me_key;
	PyObject *me_value;
#ifdef USE_CACHE_ALIGNED
	long	aligner;
#endif
} dictentry;

/*
To ensure the lookup algorithm terminates, the table size must be a
prime number and there must be at least one NULL key in the table.
The value ma_fill is the number of non-NULL keys; ma_used is the number
of non-NULL, non-dummy keys.
To avoid slowing down lookups on a near-full table, we resize the table
when it is more than half filled.
*/
typedef struct {
	PyObject_HEAD
	int ma_fill;
	int ma_used;
	int ma_size;
	int ma_poly;
	dictentry *ma_table;
} dictobject;

PyObject *
PyDict_New()
{
	register dictobject *mp;
	if (dummy == NULL) { /* Auto-initialize dummy */
		dummy = PyString_FromString("<dummy key>");
		if (dummy == NULL)
			return NULL;
	}
	mp = PyObject_NEW(dictobject, &PyDict_Type);
	if (mp == NULL)
		return NULL;
	mp->ma_size = 0;
	mp->ma_poly = 0;
	mp->ma_table = NULL;
	mp->ma_fill = 0;
	mp->ma_used = 0;
	return (PyObject *)mp;
}

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).
However, instead of going through the table at constant steps, we cycle
through the values of GF(2^n)-{0}. This avoids modulo computations, being
much cheaper on RISC machines, without leading to clustering.

First a 32-bit hash value, 'sum', is computed from the key string.
The first character is added an extra time shifted by 8 to avoid hashing
single-character keys (often heavily used variables) too close together.
All arithmetic on sum should ignore overflow.

The initial probe index is then computed as sum mod the table size.
Subsequent probe indices use the values of x^i in GF(2^n) as an offset,
where x is a root. The initial value is derived from sum, too.

(This version is due to Reimer Behrends, some ideas are also due to
Jyrki Alakuijala.)
*/
static dictentry *lookdict Py_PROTO((dictobject *, PyObject *, long));
static dictentry *
lookdict(mp, key, hash)
	dictobject *mp;
	PyObject *key;
	long hash;
{
	register int i;
	register unsigned incr;
	register unsigned long sum = (unsigned long) hash;
	register dictentry *freeslot = NULL;
	register unsigned int mask = mp->ma_size-1;
	dictentry *ep0 = mp->ma_table;
	register dictentry *ep;
	/* We must come up with (i, incr) such that 0 <= i < ma_size
	   and 0 < incr < ma_size and both are a function of hash */
	i = (~sum) & mask;
	/* We use ~sum instead if sum, as degenerate hash functions, such
	   as for ints <sigh>, can have lots of leading zeros. It's not
	   really a performance risk, but better safe than sorry. */
	ep = &ep0[i];
	if (ep->me_key == NULL)
		return ep;
	if (ep->me_key == dummy)
		freeslot = ep;
	else if (ep->me_key == key ||
		 (ep->me_hash == hash &&
		  PyObject_Compare(ep->me_key, key) == 0))
	{
		return ep;
	}
	/* XXX What if PyObject_Compare returned an exception? */
	/* Derive incr from sum, just to make it more arbitrary. Note that
	   incr must not be 0, or we will get into an infinite loop.*/
	incr = (sum ^ (sum >> 3)) & mask;
	if (!incr)
		incr = mask;
	if (incr > mask) /* Cycle through GF(2^n)-{0} */
		incr ^= mp->ma_poly; /* This will implicitly clear the
					highest bit */
	for (;;) {
		ep = &ep0[(i+incr)&mask];
		if (ep->me_key == NULL) {
			if (freeslot != NULL)
				return freeslot;
			else
				return ep;
		}
		if (ep->me_key == dummy) {
			if (freeslot == NULL)
				freeslot = ep;
		}
		else if (ep->me_key == key ||
			 (ep->me_hash == hash &&
			  PyObject_Compare(ep->me_key, key) == 0)) {
			return ep;
		}
		/* XXX What if PyObject_Compare returned an exception? */
		/* Cycle through GF(2^n)-{0} */
		incr = incr << 1;
		if (incr > mask)
			incr ^= mp->ma_poly;
	}
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Eats a reference to key and one to value.
*/
static void insertdict
	Py_PROTO((dictobject *, PyObject *, long, PyObject *));
static void
insertdict(mp, key, hash, value)
	register dictobject *mp;
	PyObject *key;
	long hash;
	PyObject *value;
{
	PyObject *old_value;
	register dictentry *ep;
	ep = lookdict(mp, key, hash);
	if (ep->me_value != NULL) {
		old_value = ep->me_value;
		ep->me_value = value;
		Py_DECREF(old_value); /* which **CAN** re-enter */
		Py_DECREF(key);
	}
	else {
		if (ep->me_key == NULL)
			mp->ma_fill++;
		else
			Py_DECREF(ep->me_key);
		ep->me_key = key;
		ep->me_hash = hash;
		ep->me_value = value;
		mp->ma_used++;
	}
}

/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
*/
static int dictresize Py_PROTO((dictobject *, int));
static int
dictresize(mp, minused)
	dictobject *mp;
	int minused;
{
	register int oldsize = mp->ma_size;
	register int newsize, newpoly;
	register dictentry *oldtable = mp->ma_table;
	register dictentry *newtable;
	register dictentry *ep;
	register int i;
	for (i = 0, newsize = MINSIZE; ; i++, newsize <<= 1) {
		if (i > sizeof(polys)/sizeof(polys[0])) {
			/* Ran out of polynomials */
			PyErr_NoMemory();
			return -1;
		}
		if (newsize > minused) {
			newpoly = polys[i];
			break;
		}
	}
	newtable = (dictentry *) calloc(sizeof(dictentry), newsize);
	if (newtable == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	mp->ma_size = newsize;
	mp->ma_poly = newpoly;
	mp->ma_table = newtable;
	mp->ma_fill = 0;
	mp->ma_used = 0;

	/* Make two passes, so we can avoid decrefs
	   (and possible side effects) till the table is copied */
	for (i = 0, ep = oldtable; i < oldsize; i++, ep++) {
		if (ep->me_value != NULL)
			insertdict(mp,ep->me_key,ep->me_hash,ep->me_value);
	}
	for (i = 0, ep = oldtable; i < oldsize; i++, ep++) {
		if (ep->me_value == NULL)
			Py_XDECREF(ep->me_key);
	}

	PyMem_XDEL(oldtable);
	return 0;
}

PyObject *
PyDict_GetItem(op, key)
	PyObject *op;
	PyObject *key;
{
	long hash;
	if (!PyDict_Check(op)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	if (((dictobject *)op)->ma_table == NULL)
		return NULL;
#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	return lookdict((dictobject *)op, key, hash) -> me_value;
}

int
PyDict_SetItem(op, key, value)
	register PyObject *op;
	PyObject *key;
	PyObject *value;
{
	register dictobject *mp;
	register long hash;
	if (!PyDict_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
	mp = (dictobject *)op;
#ifdef CACHE_HASH
	if (PyString_Check(key)) {
#ifdef INTERN_STRINGS
		if (((PyStringObject *)key)->ob_sinterned != NULL) {
			key = ((PyStringObject *)key)->ob_sinterned;
			hash = ((PyStringObject *)key)->ob_shash;
		}
		else
#endif
		{
			hash = ((PyStringObject *)key)->ob_shash;
			if (hash == -1)
				hash = PyObject_Hash(key);
		}
	}
	else
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	/* if fill >= 2/3 size, double in size */
	if (mp->ma_fill*3 >= mp->ma_size*2) {
		if (dictresize(mp, mp->ma_used*2) != 0) {
			if (mp->ma_fill+1 > mp->ma_size)
				return -1;
		}
	}
	Py_INCREF(value);
	Py_INCREF(key);
	insertdict(mp, key, hash, value);
	return 0;
}

int
PyDict_DelItem(op, key)
	PyObject *op;
	PyObject *key;
{
	register dictobject *mp;
	register long hash;
	register dictentry *ep;
	PyObject *old_value, *old_key;

	if (!PyDict_Check(op)) {
		PyErr_BadInternalCall();
		return -1;
	}
#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	mp = (dictobject *)op;
	if (((dictobject *)op)->ma_table == NULL)
		goto empty;
	ep = lookdict(mp, key, hash);
	if (ep->me_value == NULL) {
	empty:
		PyErr_SetObject(PyExc_KeyError, key);
		return -1;
	}
	old_key = ep->me_key;
	Py_INCREF(dummy);
	ep->me_key = dummy;
	old_value = ep->me_value;
	ep->me_value = NULL;
	mp->ma_used--;
	Py_DECREF(old_value); 
	Py_DECREF(old_key); 
	return 0;
}

void
PyDict_Clear(op)
	PyObject *op;
{
	int i, n;
	register dictentry *table;
	dictobject *mp;
	if (!PyDict_Check(op))
		return;
	mp = (dictobject *)op;
	table = mp->ma_table;
	if (table == NULL)
		return;
	n = mp->ma_size;
	mp->ma_size = mp->ma_used = mp->ma_fill = 0;
	mp->ma_table = NULL;
	for (i = 0; i < n; i++) {
		Py_XDECREF(table[i].me_key);
		Py_XDECREF(table[i].me_value);
	}
	PyMem_DEL(table);
}

int
PyDict_Next(op, ppos, pkey, pvalue)
	PyObject *op;
	int *ppos;
	PyObject **pkey;
	PyObject **pvalue;
{
	int i;
	register dictobject *mp;
	if (!PyDict_Check(op))
		return 0;
	mp = (dictobject *)op;
	i = *ppos;
	if (i < 0)
		return 0;
	while (i < mp->ma_size && mp->ma_table[i].me_value == NULL)
		i++;
	*ppos = i+1;
	if (i >= mp->ma_size)
		return 0;
	if (pkey)
		*pkey = mp->ma_table[i].me_key;
	if (pvalue)
		*pvalue = mp->ma_table[i].me_value;
	return 1;
}

/* Methods */

static void
dict_dealloc(mp)
	register dictobject *mp;
{
	register int i;
	register dictentry *ep;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_key != NULL)
			Py_DECREF(ep->me_key);
		if (ep->me_value != NULL)
			Py_DECREF(ep->me_value);
	}
	PyMem_XDEL(mp->ma_table);
	PyMem_DEL(mp);
}

static int
dict_print(mp, fp, flags)
	register dictobject *mp;
	register FILE *fp;
	register int flags;
{
	register int i;
	register int any;
	register dictentry *ep;
	fprintf(fp, "{");
	any = 0;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_value != NULL) {
			if (any++ > 0)
				fprintf(fp, ", ");
			if (PyObject_Print((PyObject *)ep->me_key, fp, 0) != 0)
				return -1;
			fprintf(fp, ": ");
			if (PyObject_Print(ep->me_value, fp, 0) != 0)
				return -1;
		}
	}
	fprintf(fp, "}");
	return 0;
}

static PyObject *
dict_repr(mp)
	dictobject *mp;
{
	auto PyObject *v;
	PyObject *sepa, *colon;
	register int i;
	register int any;
	register dictentry *ep;
	v = PyString_FromString("{");
	sepa = PyString_FromString(", ");
	colon = PyString_FromString(": ");
	any = 0;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size && v; i++, ep++) {
		if (ep->me_value != NULL) {
			if (any++)
				PyString_Concat(&v, sepa);
			PyString_ConcatAndDel(&v, PyObject_Repr(ep->me_key));
			PyString_Concat(&v, colon);
			PyString_ConcatAndDel(&v, PyObject_Repr(ep->me_value));
		}
	}
	PyString_ConcatAndDel(&v, PyString_FromString("}"));
	Py_XDECREF(sepa);
	Py_XDECREF(colon);
	return v;
}

static int
dict_length(mp)
	dictobject *mp;
{
	return mp->ma_used;
}

static PyObject *
dict_subscript(mp, key)
	dictobject *mp;
	register PyObject *key;
{
	PyObject *v;
	long hash;
	if (mp->ma_table == NULL) {
		PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}
#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	v = lookdict(mp, key, hash) -> me_value;
	if (v == NULL)
		PyErr_SetObject(PyExc_KeyError, key);
	else
		Py_INCREF(v);
	return v;
}

static int
dict_ass_sub(mp, v, w)
	dictobject *mp;
	PyObject *v, *w;
{
	if (w == NULL)
		return PyDict_DelItem((PyObject *)mp, v);
	else
		return PyDict_SetItem((PyObject *)mp, v, w);
}

static PyMappingMethods dict_as_mapping = {
	(inquiry)dict_length, /*mp_length*/
	(binaryfunc)dict_subscript, /*mp_subscript*/
	(objobjargproc)dict_ass_sub, /*mp_ass_subscript*/
};

static PyObject *
dict_keys(mp, args)
	register dictobject *mp;
	PyObject *args;
{
	register PyObject *v;
	register int i, j;
	if (!PyArg_NoArgs(args))
		return NULL;
	v = PyList_New(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			PyObject *key = mp->ma_table[i].me_key;
			Py_INCREF(key);
			PyList_SetItem(v, j, key);
			j++;
		}
	}
	return v;
}

static PyObject *
dict_values(mp, args)
	register dictobject *mp;
	PyObject *args;
{
	register PyObject *v;
	register int i, j;
	if (!PyArg_NoArgs(args))
		return NULL;
	v = PyList_New(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			PyObject *value = mp->ma_table[i].me_value;
			Py_INCREF(value);
			PyList_SetItem(v, j, value);
			j++;
		}
	}
	return v;
}

static PyObject *
dict_items(mp, args)
	register dictobject *mp;
	PyObject *args;
{
	register PyObject *v;
	register int i, j;
	if (!PyArg_NoArgs(args))
		return NULL;
	v = PyList_New(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			PyObject *key = mp->ma_table[i].me_key;
			PyObject *value = mp->ma_table[i].me_value;
			PyObject *item = PyTuple_New(2);
			if (item == NULL) {
				Py_DECREF(v);
				return NULL;
			}
			Py_INCREF(key);
			PyTuple_SetItem(item, 0, key);
			Py_INCREF(value);
			PyTuple_SetItem(item, 1, value);
			PyList_SetItem(v, j, item);
			j++;
		}
	}
	return v;
}

static PyObject *
dict_absorb(mp, args)
      register dictobject *mp;
      PyObject *args;
{
	register int i;
	dictobject *other;
        dictentry *entry;
	if (!PyArg_Parse(args, "O!", &PyDict_Type, &other))
		return NULL;
	if (other == mp)
		goto done; /* a.absorb(a); nothing to do */
	/* Do one big resize at the start, rather than incrementally
	   resizing as we insert new items.  Expect that there will be
	   no (or few) overlapping keys. */
	if ((mp->ma_fill + other->ma_used)*3 >= mp->ma_size*2) {
		if (dictresize(mp, (mp->ma_used + other->ma_used)*3/2) != 0)
			return NULL;
	}
	for (i = 0; i < other->ma_size; i++) {
		entry = &other->ma_table[i];
		if (entry->me_value != NULL) {
			Py_INCREF(entry->me_key);
			Py_INCREF(entry->me_value);
			insertdict(mp, entry->me_key, entry->me_hash,
				   entry->me_value);
		}
	}
  done:
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dict_copy(mp, args)
      register dictobject *mp;
      PyObject *args;
{
	register int i;
	dictobject *copy;
        dictentry *entry;
	if (!PyArg_Parse(args, ""))
		return NULL;
	copy = (dictobject *)PyDict_New();
	if (copy == NULL)
		return NULL;
	if (mp->ma_used > 0) {
		if (dictresize(copy, mp->ma_used*3/2) != 0)
			return NULL;
		for (i = 0; i < mp->ma_size; i++) {
			entry = &mp->ma_table[i];
			if (entry->me_value != NULL) {
				Py_INCREF(entry->me_key);
				Py_INCREF(entry->me_value);
				insertdict(copy, entry->me_key, entry->me_hash,
					   entry->me_value);
			}
		}
	}
	return (PyObject *)copy;
}

int
PyDict_Size(mp)
	PyObject *mp;
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return 0;
	}
	return ((dictobject *)mp)->ma_used;
}

PyObject *
PyDict_Keys(mp)
	PyObject *mp;
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_keys((dictobject *)mp, (PyObject *)NULL);
}

PyObject *
PyDict_Values(mp)
	PyObject *mp;
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_values((dictobject *)mp, (PyObject *)NULL);
}

PyObject *
PyDict_Items(mp)
	PyObject *mp;
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_items((dictobject *)mp, (PyObject *)NULL);
}

#define NEWCMP

#ifdef NEWCMP

/* Subroutine which returns the smallest key in a for which b's value
   is different or absent.  The value is returned too, through the
   pval argument.  No reference counts are incremented. */

static PyObject *
characterize(a, b, pval)
	dictobject *a;
	dictobject *b;
	PyObject **pval;
{
	PyObject *diff = NULL;
	int i;

	*pval = NULL;
	for (i = 0; i < a->ma_size; i++) {
		if (a->ma_table[i].me_value != NULL) {
			PyObject *key = a->ma_table[i].me_key;
			PyObject *aval, *bval;
			/* XXX What if PyObject_Compare raises an exception? */
			if (diff != NULL && PyObject_Compare(key, diff) > 0)
				continue;
			aval = a->ma_table[i].me_value;
			bval = PyDict_GetItem((PyObject *)b, key);
			/* XXX What if PyObject_Compare raises an exception? */
			if (bval == NULL || PyObject_Compare(aval, bval) != 0)
			{
				diff = key;
				*pval = aval;
			}
		}
	}
	return diff;
}

static int
dict_compare(a, b)
	dictobject *a, *b;
{
	PyObject *adiff, *bdiff, *aval, *bval;
	int res;

	/* Compare lengths first */
	if (a->ma_used < b->ma_used)
		return -1;	/* a is shorter */
	else if (a->ma_used > b->ma_used)
		return 1;	/* b is shorter */
	/* Same length -- check all keys */
	adiff = characterize(a, b, &aval);
	if (PyErr_Occurred())
		return -1;
	if (adiff == NULL)
		return 0;	/* a is a subset with the same length */
	bdiff = characterize(b, a, &bval);
	if (PyErr_Occurred())
		return -1;
	/* bdiff == NULL would be impossible now */
	res = PyObject_Compare(adiff, bdiff);
	if (res == 0)
		res = PyObject_Compare(aval, bval);
	return res;
}

#else /* !NEWCMP */

static int
dict_compare(a, b)
	dictobject *a, *b;
{
	PyObject *akeys, *bkeys;
	int i, n, res;
	if (a == b)
		return 0;
	if (a->ma_used == 0) {
		if (b->ma_used != 0)
			return -1;
		else
			return 0;
	}
	else {
		if (b->ma_used == 0)
			return 1;
	}
	akeys = dict_keys(a, (PyObject *)NULL);
	bkeys = dict_keys(b, (PyObject *)NULL);
	if (akeys == NULL || bkeys == NULL) {
		/* Oops, out of memory -- what to do? */
		/* For now, sort on address! */
		Py_XDECREF(akeys);
		Py_XDECREF(bkeys);
		if (a < b)
			return -1;
		else
			return 1;
	}
	PyList_Sort(akeys);
	PyList_Sort(bkeys);
	n = a->ma_used < b->ma_used ? a->ma_used : b->ma_used; /* smallest */
	res = 0;
	for (i = 0; i < n; i++) {
		PyObject *akey, *bkey, *aval, *bval;
		long ahash, bhash;
		akey = PyList_GetItem(akeys, i);
		bkey = PyList_GetItem(bkeys, i);
		res = PyObject_Compare(akey, bkey);
		if (res != 0)
			break;
#ifdef CACHE_HASH
		if (!PyString_Check(akey) ||
		    (ahash = ((PyStringObject *) akey)->ob_shash) == -1)
#endif
		{
			ahash = PyObject_Hash(akey);
			if (ahash == -1)
				PyErr_Clear(); /* Don't want errors here */
		}
#ifdef CACHE_HASH
		if (!PyString_Check(bkey) ||
		    (bhash = ((PyStringObject *) bkey)->ob_shash) == -1)
#endif
		{
			bhash = PyObject_Hash(bkey);
			if (bhash == -1)
				PyErr_Clear(); /* Don't want errors here */
		}
		aval = lookdict(a, akey, ahash) -> me_value;
		bval = lookdict(b, bkey, bhash) -> me_value;
		res = PyObject_Compare(aval, bval);
		if (res != 0)
			break;
	}
	if (res == 0) {
		if (a->ma_used < b->ma_used)
			res = -1;
		else if (a->ma_used > b->ma_used)
			res = 1;
	}
	Py_DECREF(akeys);
	Py_DECREF(bkeys);
	return res;
}

#endif /* !NEWCMP */

static PyObject *
dict_has_key(mp, args)
	register dictobject *mp;
	PyObject *args;
{
	PyObject *key;
	long hash;
	register long ok;
	if (!PyArg_Parse(args, "O", &key))
		return NULL;
#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	ok = mp->ma_size != 0 && lookdict(mp, key, hash)->me_value != NULL;
	return PyInt_FromLong(ok);
}

static PyObject *
dict_clear(mp, args)
	register dictobject *mp;
	PyObject *args;
{
	if (!PyArg_NoArgs(args))
		return NULL;
	PyDict_Clear((PyObject *)mp);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef mapp_methods[] = {
	{"absorb",	(PyCFunction)dict_absorb},
	{"clear",	(PyCFunction)dict_clear},
	{"copy",	(PyCFunction)dict_copy},
	{"has_key",	(PyCFunction)dict_has_key},
	{"items",	(PyCFunction)dict_items},
	{"keys",	(PyCFunction)dict_keys},
	{"values",	(PyCFunction)dict_values},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
dict_getattr(mp, name)
	dictobject *mp;
	char *name;
{
	return Py_FindMethod(mapp_methods, (PyObject *)mp, name);
}

PyTypeObject PyDict_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"dictionary",
	sizeof(dictobject),
	0,
	(destructor)dict_dealloc, /*tp_dealloc*/
	(printfunc)dict_print, /*tp_print*/
	(getattrfunc)dict_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)dict_compare, /*tp_compare*/
	(reprfunc)dict_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	&dict_as_mapping,	/*tp_as_mapping*/
};

/* For backward compatibility with old dictionary interface */

PyObject *
PyDict_GetItemString(v, key)
	PyObject *v;
	char *key;
{
	PyObject *kv, *rv;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return NULL;
	PyString_InternInPlace(&kv);
	rv = PyDict_GetItem(v, kv);
	Py_DECREF(kv);
	return rv;
}

int
PyDict_SetItemString(v, key, item)
	PyObject *v;
	char *key;
	PyObject *item;
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return -1;
	PyString_InternInPlace(&kv);
	err = PyDict_SetItem(v, kv, item);
	Py_DECREF(kv);
	return err;
}

int
PyDict_DelItemString(v, key)
	PyObject *v;
	char *key;
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return -1;
	PyString_InternInPlace(&kv);
	err = PyDict_DelItem(v, kv);
	Py_DECREF(kv);
	return err;
}
