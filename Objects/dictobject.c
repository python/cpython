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

/* Mapping object implementation; using a hash table */

/* This file should really be called "dictobject.c", since "mapping"
  is the generic name for objects with an unorderred arbitrary key
  set (just like lists are sequences), but since it improves (and was
  originally derived from) a file by that name I had to change its
  name.  For the user these objects are still called "dictionaries". */

#include "allobjects.h"
#include "modsupport.h"


/*
 * MINSIZE is the minimum size of a mapping.
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
  /* Not verified by Tim P: */
  268435456 + 3,
  536870912 + 5,
  1073741824 + 3,
  0
};

/* Object used as dummy key to fill deleted entries */
static object *dummy; /* Initialized by first call to newmappingobject() */

/*
Invariant for entries: when in use, de_value is not NULL and de_key is
not NULL and not dummy; when not in use, de_value is NULL and de_key
is either NULL or dummy.  A dummy key value cannot be replaced by
NULL, since otherwise other keys may be lost.
*/
typedef struct {
	long me_hash;
	object *me_key;
	object *me_value;
} mappingentry;

/*
To ensure the lookup algorithm terminates, the table size must be a
prime number and there must be at least one NULL key in the table.
The value ma_fill is the number of non-NULL keys; ma_used is the number
of non-NULL, non-dummy keys.
To avoid slowing down lookups on a near-full table, we resize the table
when it is more than half filled.
*/
typedef struct {
	OB_HEAD
	int ma_fill;
	int ma_used;
	int ma_size;
	int ma_poly;
	mappingentry *ma_table;
} mappingobject;

object *
newmappingobject()
{
	register mappingobject *mp;
	if (dummy == NULL) { /* Auto-initialize dummy */
		dummy = newstringobject("<dummy key>");
		if (dummy == NULL)
			return NULL;
	}
	mp = NEWOBJ(mappingobject, &Mappingtype);
	if (mp == NULL)
		return NULL;
	mp->ma_size = 0;
	mp->ma_poly = 0;
	mp->ma_table = NULL;
	mp->ma_fill = 0;
	mp->ma_used = 0;
	return (object *)mp;
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
static mappingentry *lookmapping PROTO((mappingobject *, object *, long));
static mappingentry *
lookmapping(mp, key, hash)
	mappingobject *mp;
	object *key;
	long hash;
{
	register int i;
	register unsigned incr;
	register unsigned long sum = (unsigned long) hash;
	register mappingentry *freeslot = NULL;
	register int mask = mp->ma_size-1;
	register mappingentry *ep = &mp->ma_table[i];
	/* We must come up with (i, incr) such that 0 <= i < ma_size
	   and 0 < incr < ma_size and both are a function of hash */
	i = (~sum) & mask;
	/* We use ~sum instead if sum, as degenerate hash functions, such
	   as for ints <sigh>, can have lots of leading zeros. It's not
	   really a performance risk, but better safe than sorry. */
	ep = &mp->ma_table[i];
	if (ep->me_key == NULL)
		return ep;
	if (ep->me_key == dummy)
		freeslot = ep;
	else if (ep->me_key == key ||
		 (ep->me_hash == hash && cmpobject(ep->me_key, key) == 0)) {
		return ep;
	}
	/* Derive incr from i, just to make it more arbitrary. Note that
	   incr must not be 0, or we will get into an infinite loop.*/
	incr = i << 1;
	if (!incr)
		incr = mask;
	if (incr > mask) /* Cycle through GF(2^n)-{0} */
		incr ^= mp->ma_poly; /* This will implicitly clear the
					highest bit */
	for (;;) {
		ep = &mp->ma_table[(i+incr)&mask];
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
			  cmpobject(ep->me_key, key) == 0)) {
			return ep;
		}
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
static void insertmapping PROTO((mappingobject *, object *, long, object *));
static void
insertmapping(mp, key, hash, value)
	register mappingobject *mp;
	object *key;
	long hash;
	object *value;
{
	object *old_value;
	register mappingentry *ep;
	ep = lookmapping(mp, key, hash);
	if (ep->me_value != NULL) {
		old_value = ep->me_value;
		ep->me_value = value;
		DECREF(old_value); /* which **CAN** re-enter */
		DECREF(key);
	}
	else {
		if (ep->me_key == NULL)
			mp->ma_fill++;
		else
			DECREF(ep->me_key);
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
static int mappingresize PROTO((mappingobject *));
static int
mappingresize(mp)
	mappingobject *mp;
{
	register int oldsize = mp->ma_size;
	register int newsize, newpoly;
	register mappingentry *oldtable = mp->ma_table;
	register mappingentry *newtable;
	register mappingentry *ep;
	register int i;
	newsize = mp->ma_size;
	for (i = 0, newsize = MINSIZE; ; i++, newsize <<= 1) {
		if (i > sizeof(polys)/sizeof(polys[0])) {
			/* Ran out of polynomials */
			err_nomem();
			return -1;
		}
		if (newsize > mp->ma_used*2) {
			newpoly = polys[i];
			break;
		}
	}
	newtable = (mappingentry *) calloc(sizeof(mappingentry), newsize);
	if (newtable == NULL) {
		err_nomem();
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
			insertmapping(mp,ep->me_key,ep->me_hash,ep->me_value);
	}
	for (i = 0, ep = oldtable; i < oldsize; i++, ep++) {
		if (ep->me_value == NULL)
			XDECREF(ep->me_key);
	}

	XDEL(oldtable);
	return 0;
}

object *
mappinglookup(op, key)
	object *op;
	object *key;
{
	long hash;
	if (!is_mappingobject(op)) {
		err_badcall();
		return NULL;
	}
	if (((mappingobject *)op)->ma_table == NULL)
		return NULL;
#ifdef CACHE_HASH
	if (!is_stringobject(key) ||
	    (hash = ((stringobject *) key)->ob_shash) == -1)
#endif
	{
		hash = hashobject(key);
		if (hash == -1)
			return NULL;
	}
	return lookmapping((mappingobject *)op, key, hash) -> me_value;
}

int
mappinginsert(op, key, value)
	register object *op;
	object *key;
	object *value;
{
	register mappingobject *mp;
	register long hash;
	if (!is_mappingobject(op)) {
		err_badcall();
		return -1;
	}
	mp = (mappingobject *)op;
#ifdef CACHE_HASH
	if (is_stringobject(key)) {
#ifdef INTERN_STRINGS
		if (((stringobject *)key)->ob_sinterned != NULL) {
			key = ((stringobject *)key)->ob_sinterned;
			hash = ((stringobject *)key)->ob_shash;
		}
		else
#endif
		{
			hash = ((stringobject *)key)->ob_shash;
			if (hash == -1)
				hash = hashobject(key);
		}
	}
	else
#endif
	{
		hash = hashobject(key);
		if (hash == -1)
			return -1;
	}
	/* if fill >= 2/3 size, resize */
	if (mp->ma_fill*3 >= mp->ma_size*2) {
		if (mappingresize(mp) != 0) {
			if (mp->ma_fill+1 > mp->ma_size)
				return -1;
		}
	}
	INCREF(value);
	INCREF(key);
	insertmapping(mp, key, hash, value);
	return 0;
}

int
mappingremove(op, key)
	object *op;
	object *key;
{
	register mappingobject *mp;
	register long hash;
	register mappingentry *ep;
	object *old_value, *old_key;

	if (!is_mappingobject(op)) {
		err_badcall();
		return -1;
	}
#ifdef CACHE_HASH
	if (!is_stringobject(key) ||
	    (hash = ((stringobject *) key)->ob_shash) == -1)
#endif
	{
		hash = hashobject(key);
		if (hash == -1)
			return -1;
	}
	mp = (mappingobject *)op;
	if (((mappingobject *)op)->ma_table == NULL)
		goto empty;
	ep = lookmapping(mp, key, hash);
	if (ep->me_value == NULL) {
	empty:
		err_setval(KeyError, key);
		return -1;
	}
	old_key = ep->me_key;
	INCREF(dummy);
	ep->me_key = dummy;
	old_value = ep->me_value;
	ep->me_value = NULL;
	mp->ma_used--;
	DECREF(old_value); 
	DECREF(old_key); 
	return 0;
}

void
mappingclear(op)
	object *op;
{
	int i, n;
	register mappingentry *table;
	mappingobject *mp;
	if (!is_mappingobject(op))
		return;
	mp = (mappingobject *)op;
	table = mp->ma_table;
	if (table == NULL)
		return;
	n = mp->ma_size;
	mp->ma_size = mp->ma_used = mp->ma_fill = 0;
	mp->ma_table = NULL;
	for (i = 0; i < n; i++) {
		XDECREF(table[i].me_key);
		XDECREF(table[i].me_value);
	}
	DEL(table);
}

int
mappinggetnext(op, ppos, pkey, pvalue)
	object *op;
	int *ppos;
	object **pkey;
	object **pvalue;
{
	int i;
	register mappingobject *mp;
	if (!is_dictobject(op))
		return 0;
	mp = (mappingobject *)op;
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
mapping_dealloc(mp)
	register mappingobject *mp;
{
	register int i;
	register mappingentry *ep;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_key != NULL)
			DECREF(ep->me_key);
		if (ep->me_value != NULL)
			DECREF(ep->me_value);
	}
	XDEL(mp->ma_table);
	DEL(mp);
}

static int
mapping_print(mp, fp, flags)
	register mappingobject *mp;
	register FILE *fp;
	register int flags;
{
	register int i;
	register int any;
	register mappingentry *ep;
	fprintf(fp, "{");
	any = 0;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_value != NULL) {
			if (any++ > 0)
				fprintf(fp, ", ");
			if (printobject((object *)ep->me_key, fp, 0) != 0)
				return -1;
			fprintf(fp, ": ");
			if (printobject(ep->me_value, fp, 0) != 0)
				return -1;
		}
	}
	fprintf(fp, "}");
	return 0;
}

static object *
mapping_repr(mp)
	mappingobject *mp;
{
	auto object *v;
	object *sepa, *colon;
	register int i;
	register int any;
	register mappingentry *ep;
	v = newstringobject("{");
	sepa = newstringobject(", ");
	colon = newstringobject(": ");
	any = 0;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size && v; i++, ep++) {
		if (ep->me_value != NULL) {
			if (any++)
				joinstring(&v, sepa);
			joinstring_decref(&v, reprobject(ep->me_key));
			joinstring(&v, colon);
			joinstring_decref(&v, reprobject(ep->me_value));
		}
	}
	joinstring_decref(&v, newstringobject("}"));
	XDECREF(sepa);
	XDECREF(colon);
	return v;
}

static int
mapping_length(mp)
	mappingobject *mp;
{
	return mp->ma_used;
}

static object *
mapping_subscript(mp, key)
	mappingobject *mp;
	register object *key;
{
	object *v;
	long hash;
	if (mp->ma_table == NULL) {
		err_setval(KeyError, key);
		return NULL;
	}
#ifdef CACHE_HASH
	if (!is_stringobject(key) ||
	    (hash = ((stringobject *) key)->ob_shash) == -1)
#endif
	{
		hash = hashobject(key);
		if (hash == -1)
			return NULL;
	}
	v = lookmapping(mp, key, hash) -> me_value;
	if (v == NULL)
		err_setval(KeyError, key);
	else
		INCREF(v);
	return v;
}

static int
mapping_ass_sub(mp, v, w)
	mappingobject *mp;
	object *v, *w;
{
	if (w == NULL)
		return mappingremove((object *)mp, v);
	else
		return mappinginsert((object *)mp, v, w);
}

static mapping_methods mapping_as_mapping = {
	(inquiry)mapping_length, /*mp_length*/
	(binaryfunc)mapping_subscript, /*mp_subscript*/
	(objobjargproc)mapping_ass_sub, /*mp_ass_subscript*/
};

static object *
mapping_keys(mp, args)
	register mappingobject *mp;
	object *args;
{
	register object *v;
	register int i, j;
	if (!getnoarg(args))
		return NULL;
	v = newlistobject(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			object *key = mp->ma_table[i].me_key;
			INCREF(key);
			setlistitem(v, j, key);
			j++;
		}
	}
	return v;
}

static object *
mapping_values(mp, args)
	register mappingobject *mp;
	object *args;
{
	register object *v;
	register int i, j;
	if (!getnoarg(args))
		return NULL;
	v = newlistobject(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			object *value = mp->ma_table[i].me_value;
			INCREF(value);
			setlistitem(v, j, value);
			j++;
		}
	}
	return v;
}

static object *
mapping_items(mp, args)
	register mappingobject *mp;
	object *args;
{
	register object *v;
	register int i, j;
	if (!getnoarg(args))
		return NULL;
	v = newlistobject(mp->ma_used);
	if (v == NULL)
		return NULL;
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			object *key = mp->ma_table[i].me_key;
			object *value = mp->ma_table[i].me_value;
			object *item = newtupleobject(2);
			if (item == NULL) {
				DECREF(v);
				return NULL;
			}
			INCREF(key);
			settupleitem(item, 0, key);
			INCREF(value);
			settupleitem(item, 1, value);
			setlistitem(v, j, item);
			j++;
		}
	}
	return v;
}

int
getmappingsize(mp)
	object *mp;
{
	if (mp == NULL || !is_mappingobject(mp)) {
		err_badcall();
		return 0;
	}
	return ((mappingobject *)mp)->ma_used;
}

object *
getmappingkeys(mp)
	object *mp;
{
	if (mp == NULL || !is_mappingobject(mp)) {
		err_badcall();
		return NULL;
	}
	return mapping_keys((mappingobject *)mp, (object *)NULL);
}

object *
getmappingvalues(mp)
	object *mp;
{
	if (mp == NULL || !is_mappingobject(mp)) {
		err_badcall();
		return NULL;
	}
	return mapping_values((mappingobject *)mp, (object *)NULL);
}

object *
getmappingitems(mp)
	object *mp;
{
	if (mp == NULL || !is_mappingobject(mp)) {
		err_badcall();
		return NULL;
	}
	return mapping_items((mappingobject *)mp, (object *)NULL);
}

#define NEWCMP

#ifdef NEWCMP

/* Subroutine which returns the smallest key in a for which b's value
   is different or absent.  The value is returned too, through the
   pval argument.  No reference counts are incremented. */

static object *
characterize(a, b, pval)
	mappingobject *a;
	mappingobject *b;
	object **pval;
{
	object *diff = NULL;
	int i;

	*pval = NULL;
	for (i = 0; i < a->ma_size; i++) {
		if (a->ma_table[i].me_value != NULL) {
			object *key = a->ma_table[i].me_key;
			object *aval, *bval;
			if (diff != NULL && cmpobject(key, diff) > 0)
				continue;
			aval = a->ma_table[i].me_value;
			bval = mappinglookup((object *)b, key);
			if (bval == NULL || cmpobject(aval, bval) != 0) {
				diff = key;
				*pval = aval;
			}
		}
	}
	return diff;
}

static int
mapping_compare(a, b)
	mappingobject *a, *b;
{
	object *adiff, *bdiff, *aval, *bval;
	int res;

	/* Compare lengths first */
	if (a->ma_used < b->ma_used)
		return -1;	/* a is shorter */
	else if (a->ma_used > b->ma_used)
		return 1;	/* b is shorter */
	/* Same length -- check all keys */
	adiff = characterize(a, b, &aval);
	if (adiff == NULL)
		return 0;	/* a is a subset with the same length */
	bdiff = characterize(b, a, &bval);
	/* bdiff == NULL would be impossible now */
	res = cmpobject(adiff, bdiff);
	if (res == 0)
		res = cmpobject(aval, bval);
	return res;
}

#else /* !NEWCMP */

static int
mapping_compare(a, b)
	mappingobject *a, *b;
{
	object *akeys, *bkeys;
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
	akeys = mapping_keys(a, (object *)NULL);
	bkeys = mapping_keys(b, (object *)NULL);
	if (akeys == NULL || bkeys == NULL) {
		/* Oops, out of memory -- what to do? */
		/* For now, sort on address! */
		XDECREF(akeys);
		XDECREF(bkeys);
		if (a < b)
			return -1;
		else
			return 1;
	}
	sortlist(akeys);
	sortlist(bkeys);
	n = a->ma_used < b->ma_used ? a->ma_used : b->ma_used; /* smallest */
	res = 0;
	for (i = 0; i < n; i++) {
		object *akey, *bkey, *aval, *bval;
		long ahash, bhash;
		akey = getlistitem(akeys, i);
		bkey = getlistitem(bkeys, i);
		res = cmpobject(akey, bkey);
		if (res != 0)
			break;
#ifdef CACHE_HASH
		if (!is_stringobject(akey) ||
		    (ahash = ((stringobject *) akey)->ob_shash) == -1)
#endif
		{
			ahash = hashobject(akey);
			if (ahash == -1)
				err_clear(); /* Don't want errors here */
		}
#ifdef CACHE_HASH
		if (!is_stringobject(bkey) ||
		    (bhash = ((stringobject *) bkey)->ob_shash) == -1)
#endif
		{
			bhash = hashobject(bkey);
			if (bhash == -1)
				err_clear(); /* Don't want errors here */
		}
		aval = lookmapping(a, akey, ahash) -> me_value;
		bval = lookmapping(b, bkey, bhash) -> me_value;
		res = cmpobject(aval, bval);
		if (res != 0)
			break;
	}
	if (res == 0) {
		if (a->ma_used < b->ma_used)
			res = -1;
		else if (a->ma_used > b->ma_used)
			res = 1;
	}
	DECREF(akeys);
	DECREF(bkeys);
	return res;
}

#endif /* !NEWCMP */

static object *
mapping_has_key(mp, args)
	register mappingobject *mp;
	object *args;
{
	object *key;
	long hash;
	register long ok;
	if (!getargs(args, "O", &key))
		return NULL;
#ifdef CACHE_HASH
	if (!is_stringobject(key) ||
	    (hash = ((stringobject *) key)->ob_shash) == -1)
#endif
	{
		hash = hashobject(key);
		if (hash == -1)
			return NULL;
	}
	ok = mp->ma_size != 0 && lookmapping(mp, key, hash)->me_value != NULL;
	return newintobject(ok);
}

static struct methodlist mapp_methods[] = {
	{"has_key",	(method)mapping_has_key},
	{"items",	(method)mapping_items},
	{"keys",	(method)mapping_keys},
	{"values",	(method)mapping_values},
	{NULL,		NULL}		/* sentinel */
};

static object *
mapping_getattr(mp, name)
	mappingobject *mp;
	char *name;
{
	return findmethod(mapp_methods, (object *)mp, name);
}

typeobject Mappingtype = {
	OB_HEAD_INIT(&Typetype)
	0,
	"dictionary",
	sizeof(mappingobject),
	0,
	(destructor)mapping_dealloc, /*tp_dealloc*/
	(printfunc)mapping_print, /*tp_print*/
	(getattrfunc)mapping_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)mapping_compare, /*tp_compare*/
	(reprfunc)mapping_repr, /*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	&mapping_as_mapping,	/*tp_as_mapping*/
};

/* For backward compatibility with old dictionary interface */

static object *last_name_object;
static char *last_name_char; /* NULL or == getstringvalue(last_name_object) */

object *
getattro(v, name)
	object *v;
	object *name;
{
	if (v->ob_type->tp_getattro != NULL)
		return (*v->ob_type->tp_getattro)(v, name);

	if (name != last_name_object) {
		XDECREF(last_name_object);
		INCREF(name);
		last_name_object = name;
		last_name_char = getstringvalue(name);
	}
	return getattr(v, last_name_char);
}

int
setattro(v, name, value)
	object *v;
	object *name;
	object *value;
{
	int err;
	INCREF(name);
	PyString_InternInPlace(&name);
	if (v->ob_type->tp_setattro != NULL)
		err = (*v->ob_type->tp_setattro)(v, name, value);
	else {
		if (name != last_name_object) {
			XDECREF(last_name_object);
			INCREF(name);
			last_name_object = name;
			last_name_char = getstringvalue(name);
		}
		err = setattr(v, last_name_char, value);
	}
	DECREF(name);
	return err;
}

object *
dictlookup(v, key)
	object *v;
	char *key;
{
	if (key != last_name_char) {
		XDECREF(last_name_object);
		last_name_object = newstringobject(key);
		if (last_name_object == NULL) {
			last_name_char = NULL;
			return NULL;
		}
		PyString_InternInPlace(&last_name_object);
		last_name_char = getstringvalue(last_name_object);
	}
	return mappinglookup(v, last_name_object);
}

int
dictinsert(v, key, item)
	object *v;
	char *key;
	object *item;
{
	if (key != last_name_char) {
		XDECREF(last_name_object);
		last_name_object = newstringobject(key);
		if (last_name_object == NULL) {
			last_name_char = NULL;
			return -1;
		}
		PyString_InternInPlace(&last_name_object);
		last_name_char = getstringvalue(last_name_object);
	}
	return mappinginsert(v, last_name_object, item);
}

int
dictremove(v, key)
	object *v;
	char *key;
{
	if (key != last_name_char) {
		XDECREF(last_name_object);
		last_name_object = newstringobject(key);
		if (last_name_object == NULL) {
			last_name_char = NULL;
			return -1;
		}
		last_name_char = getstringvalue(last_name_object);
	}
	return mappingremove(v, last_name_object);
}
