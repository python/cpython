
/* Dictionary object implementation using a hash table */

#include "Python.h"


/*
 * MINSIZE is the minimum size of a dictionary.
 */

#define MINSIZE 4

/* define this out if you don't want conversion statistics on exit */
#undef SHOW_CONVERSION_COUNTS

/*
Table of irreducible polynomials to efficiently cycle through
GF(2^n)-{0}, 2<=n<=30.  A table size is always a power of 2.
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
There are three kinds of slots in the table:

1. Unused.  me_key == me_value == NULL
   Does not hold an active (key, value) pair now and never did.  Unused can
   transition to Active upon key insertion.  This is the only case in which
   me_key is NULL, and is each slot's initial state.

2. Active.  me_key != NULL and me_key != dummy and me_value != NULL
   Holds an active (key, value) pair.  Active can transition to Dummy upon
   key deletion.  This is the only case in which me_value != NULL.

3. Dummy.  me_key == dummy and me_value == NULL
   Previously held an active (key, value) pair, but that was deleted and an
   active pair has not yet overwritten the slot.  Dummy can transition to
   Active upon key insertion.  Dummy slots cannot be made Unused again
   (cannot have me_key set to NULL), else the probe sequence in case of
   collision would have no way to know they were once active.

Note: .popitem() abuses the me_hash field of an Unused or Dummy slot to
hold a search finger.  The me_hash field of Unused or Dummy slots has no
meaning otherwise.
*/
typedef struct {
	long me_hash;      /* cached hash code of me_key */
	PyObject *me_key;
	PyObject *me_value;
#ifdef USE_CACHE_ALIGNED
	long	aligner;
#endif
} dictentry;

/*
To ensure the lookup algorithm terminates, there must be at least one Unused
slot (NULL key) in the table.
The value ma_fill is the number of non-NULL keys (sum of Active and Dummy);
ma_used is the number of non-NULL, non-dummy keys (== the number of non-NULL
values == the number of Active items).
To avoid slowing down lookups on a near-full table, we resize the table when
it's two-thirds full.
*/
typedef struct dictobject dictobject;
struct dictobject {
	PyObject_HEAD
	int ma_fill;  /* # Active + # Dummy */
	int ma_used;  /* # Active */
	int ma_size;  /* total # slots in ma_table */
	int ma_poly;  /* appopriate entry from polys vector */
	dictentry *ma_table;
	dictentry *(*ma_lookup)(dictobject *mp, PyObject *key, long hash);
};

/* forward declarations */
static dictentry *
lookdict_string(dictobject *mp, PyObject *key, long hash);

#ifdef SHOW_CONVERSION_COUNTS
static long created = 0L;
static long converted = 0L;

static void
show_counts(void)
{
	fprintf(stderr, "created %ld string dicts\n", created);
	fprintf(stderr, "converted %ld to normal dicts\n", converted);
	fprintf(stderr, "%.2f%% conversion rate\n", (100.0*converted)/created);
}
#endif

PyObject *
PyDict_New(void)
{
	register dictobject *mp;
	if (dummy == NULL) { /* Auto-initialize dummy */
		dummy = PyString_FromString("<dummy key>");
		if (dummy == NULL)
			return NULL;
#ifdef SHOW_CONVERSION_COUNTS
		Py_AtExit(show_counts);
#endif
	}
	mp = PyObject_NEW(dictobject, &PyDict_Type);
	if (mp == NULL)
		return NULL;
	mp->ma_size = 0;
	mp->ma_poly = 0;
	mp->ma_table = NULL;
	mp->ma_fill = 0;
	mp->ma_used = 0;
	mp->ma_lookup = lookdict_string;
#ifdef SHOW_CONVERSION_COUNTS
	++created;
#endif
	PyObject_GC_Init(mp);
	return (PyObject *)mp;
}

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).
However, instead of going through the table at constant steps, we cycle
through the values of GF(2^n).  This avoids modulo computations, being
much cheaper on RISC machines, without leading to clustering.

The initial probe index is computed as hash mod the table size.
Subsequent probe indices use the values of x^i in GF(2^n)-{0} as an offset,
where x is a root. The initial offset is derived from hash, too.

All arithmetic on hash should ignore overflow.

(This version is due to Reimer Behrends, some ideas are also due to
Jyrki Alakuijala and Vladimir Marangozov.)

This function must never return NULL; failures are indicated by returning
a dictentry* for which the me_value field is NULL.  Exceptions are never
reported by this function, and outstanding exceptions are maintained.
*/
static dictentry *
lookdict(dictobject *mp, PyObject *key, register long hash)
{
	register int i;
	register unsigned incr;
	register dictentry *freeslot;
	register unsigned int mask = mp->ma_size-1;
	dictentry *ep0 = mp->ma_table;
	register dictentry *ep;
	register int restore_error = 0;
	register int checked_error = 0;
	register int cmp;
	PyObject *err_type, *err_value, *err_tb;
	/* We must come up with (i, incr) such that 0 <= i < ma_size
	   and 0 < incr < ma_size and both are a function of hash.
	   i is the initial table index and incr the initial probe offset. */
	i = (~hash) & mask;
	/* We use ~hash instead of hash, as degenerate hash functions, such
	   as for ints <sigh>, can have lots of leading zeros. It's not
	   really a performance risk, but better safe than sorry.
	   12-Dec-00 tim:  so ~hash produces lots of leading ones instead --
	   what's the gain? */
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
		return ep;
	if (ep->me_key == dummy)
		freeslot = ep;
	else {
		if (ep->me_hash == hash) {
			/* error can't have been checked yet */
			checked_error = 1;
			if (PyErr_Occurred()) {
				restore_error = 1;
				PyErr_Fetch(&err_type, &err_value, &err_tb);
			}
			cmp = PyObject_RichCompareBool(ep->me_key, key, Py_EQ);
			if (cmp > 0) {
				if (restore_error)
					PyErr_Restore(err_type, err_value,
						      err_tb);
				return ep;
			}
			else if (cmp < 0)
				PyErr_Clear();
		}
		freeslot = NULL;
	}
	/* Derive incr from hash, just to make it more arbitrary. Note that
	   incr must not be 0, or we will get into an infinite loop.*/
	incr = (hash ^ ((unsigned long)hash >> 3)) & mask;
	if (!incr)
		incr = mask;
	for (;;) {
		ep = &ep0[(i+incr)&mask];
		if (ep->me_key == NULL) {
			if (restore_error)
				PyErr_Restore(err_type, err_value, err_tb);
			if (freeslot != NULL)
				return freeslot;
			else
				return ep;
		}
		if (ep->me_key == dummy) {
			if (freeslot == NULL)
				freeslot = ep;
		}
		else if (ep->me_key == key) {
			if (restore_error)
				PyErr_Restore(err_type, err_value, err_tb);
			return ep;
		}
		else if (ep->me_hash == hash) {
			if (!checked_error) {
				checked_error = 1;
				if (PyErr_Occurred()) {
					restore_error = 1;
					PyErr_Fetch(&err_type, &err_value,
						    &err_tb);
				}
			}
			cmp = PyObject_RichCompareBool(ep->me_key, key, Py_EQ);
			if (cmp > 0) {
				if (restore_error)
					PyErr_Restore(err_type, err_value,
						      err_tb);
				return ep;
			}
			else if (cmp < 0)
				PyErr_Clear();
		}
		/* Cycle through GF(2^n)-{0} */
		incr = incr << 1;
		if (incr > mask)
			incr ^= mp->ma_poly; /* This will implicitly clear
						the highest bit */
	}
}

/*
 * Hacked up version of lookdict which can assume keys are always strings;
 * this assumption allows testing for errors during PyObject_Compare() to
 * be dropped; string-string comparisons never raise exceptions.  This also
 * means we don't need to go through PyObject_Compare(); we can always use
 * the tp_compare slot of the string type object directly.
 *
 * This really only becomes meaningful if proper error handling in lookdict()
 * is too expensive.
 */
static dictentry *
lookdict_string(dictobject *mp, PyObject *key, register long hash)
{
	register int i;
	register unsigned incr;
	register dictentry *freeslot;
	register unsigned int mask = mp->ma_size-1;
	dictentry *ep0 = mp->ma_table;
	register dictentry *ep;
	cmpfunc compare = PyString_Type.tp_compare;

	/* make sure this function doesn't have to handle non-string keys */
	if (!PyString_Check(key)) {
#ifdef SHOW_CONVERSION_COUNTS
		++converted;
#endif
		mp->ma_lookup = lookdict;
		return lookdict(mp, key, hash);
	}
	/* We must come up with (i, incr) such that 0 <= i < ma_size
	   and 0 < incr < ma_size and both are a function of hash */
	i = (~hash) & mask;
	/* We use ~hash instead of hash, as degenerate hash functions, such
	   as for ints <sigh>, can have lots of leading zeros. It's not
	   really a performance risk, but better safe than sorry. */
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
		return ep;
	if (ep->me_key == dummy)
		freeslot = ep;
	else {
		if (ep->me_hash == hash
		    && compare(ep->me_key, key) == 0) {
			return ep;
		}
		freeslot = NULL;
	}
	/* Derive incr from hash, just to make it more arbitrary. Note that
	   incr must not be 0, or we will get into an infinite loop.*/
	incr = (hash ^ ((unsigned long)hash >> 3)) & mask;
	if (!incr)
		incr = mask;
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
		else if (ep->me_key == key
			 || (ep->me_hash == hash
			     && compare(ep->me_key, key) == 0)) {
			return ep;
		}
		/* Cycle through GF(2^n)-{0} */
		incr = incr << 1;
		if (incr > mask)
			incr ^= mp->ma_poly; /* This will implicitly clear
						the highest bit */
	}
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Eats a reference to key and one to value.
*/
static void
insertdict(register dictobject *mp, PyObject *key, long hash, PyObject *value)
{
	PyObject *old_value;
	register dictentry *ep;
	ep = (mp->ma_lookup)(mp, key, hash);
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
static int
dictresize(dictobject *mp, int minused)
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
	newtable = PyMem_NEW(dictentry, newsize);
	if (newtable == NULL) {
		PyErr_NoMemory();
		return -1;
	}
	memset(newtable, '\0', sizeof(dictentry) * newsize);
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
		if (ep->me_value == NULL) {
			Py_XDECREF(ep->me_key);
		}
	}

	if (oldtable != NULL)
		PyMem_DEL(oldtable);
	return 0;
}

PyObject *
PyDict_GetItem(PyObject *op, PyObject *key)
{
	long hash;
	dictobject *mp = (dictobject *)op;
	if (!PyDict_Check(op)) {
		return NULL;
	}
	if (mp->ma_table == NULL)
		return NULL;
#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1) {
			PyErr_Clear();
			return NULL;
		}
	}
	return (mp->ma_lookup)(mp, key, hash)->me_value;
}

/* CAUTION: PyDict_SetItem() must guarantee that it won't resize the
 * dictionary if it is merely replacing the value for an existing key.
 * This is means that it's safe to loop over a dictionary with
 * PyDict_Next() and occasionally replace a value -- but you can't
 * insert new keys or remove them.
 */
int
PyDict_SetItem(register PyObject *op, PyObject *key, PyObject *value)
{
	register dictobject *mp;
	register long hash;
	register int n_used;

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
	if (mp->ma_fill >= mp->ma_size) {
		/* No room for a new key.
		 * This only happens when the dict is empty.
		 * Let dictresize() create a minimal dict.
		 */
		assert(mp->ma_used == 0);
		if (dictresize(mp, 0) != 0)
			return -1;
		assert(mp->ma_fill < mp->ma_size);
	}
	n_used = mp->ma_used;
	Py_INCREF(value);
	Py_INCREF(key);
	insertdict(mp, key, hash, value);
	/* If we added a key, we can safely resize.  Otherwise skip this!
	 * If fill >= 2/3 size, adjust size.  Normally, this doubles the
	 * size, but it's also possible for the dict to shrink (if ma_fill is
	 * much larger than ma_used, meaning a lot of dict keys have been
	 * deleted).
	 */
	if (mp->ma_used > n_used && mp->ma_fill*3 >= mp->ma_size*2) {
		if (dictresize(mp, mp->ma_used*2) != 0)
			return -1;
	}
	return 0;
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
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
	ep = (mp->ma_lookup)(mp, key, hash);
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
PyDict_Clear(PyObject *op)
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

/* CAUTION:  In general, it isn't safe to use PyDict_Next in a loop that
 * mutates the dict.  One exception:  it is safe if the loop merely changes
 * the values associated with the keys (but doesn't insert new keys or
 * delete keys), via PyDict_SetItem().
 */
int
PyDict_Next(PyObject *op, int *ppos, PyObject **pkey, PyObject **pvalue)
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
dict_dealloc(register dictobject *mp)
{
	register int i;
	register dictentry *ep;
	Py_TRASHCAN_SAFE_BEGIN(mp)
	PyObject_GC_Fini(mp);
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_key != NULL) {
			Py_DECREF(ep->me_key);
		}
		if (ep->me_value != NULL) {
			Py_DECREF(ep->me_value);
		}
	}
	if (mp->ma_table != NULL)
		PyMem_DEL(mp->ma_table);
	mp = (dictobject *) PyObject_AS_GC(mp);
	PyObject_DEL(mp);
	Py_TRASHCAN_SAFE_END(mp)
}

static int
dict_print(register dictobject *mp, register FILE *fp, register int flags)
{
	register int i;
	register int any;
	register dictentry *ep;

	i = Py_ReprEnter((PyObject*)mp);
	if (i != 0) {
		if (i < 0)
			return i;
		fprintf(fp, "{...}");
		return 0;
	}

	fprintf(fp, "{");
	any = 0;
	for (i = 0, ep = mp->ma_table; i < mp->ma_size; i++, ep++) {
		if (ep->me_value != NULL) {
			if (any++ > 0)
				fprintf(fp, ", ");
			if (PyObject_Print((PyObject *)ep->me_key, fp, 0)!=0) {
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
			fprintf(fp, ": ");
			if (PyObject_Print(ep->me_value, fp, 0) != 0) {
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
		}
	}
	fprintf(fp, "}");
	Py_ReprLeave((PyObject*)mp);
	return 0;
}

static PyObject *
dict_repr(dictobject *mp)
{
	auto PyObject *v;
	PyObject *sepa, *colon;
	register int i;
	register int any;
	register dictentry *ep;

	i = Py_ReprEnter((PyObject*)mp);
	if (i != 0) {
		if (i > 0)
			return PyString_FromString("{...}");
		return NULL;
	}

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
	Py_ReprLeave((PyObject*)mp);
	Py_XDECREF(sepa);
	Py_XDECREF(colon);
	return v;
}

static int
dict_length(dictobject *mp)
{
	return mp->ma_used;
}

static PyObject *
dict_subscript(dictobject *mp, register PyObject *key)
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
	v = (mp->ma_lookup)(mp, key, hash) -> me_value;
	if (v == NULL)
		PyErr_SetObject(PyExc_KeyError, key);
	else
		Py_INCREF(v);
	return v;
}

static int
dict_ass_sub(dictobject *mp, PyObject *v, PyObject *w)
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
dict_keys(register dictobject *mp, PyObject *args)
{
	register PyObject *v;
	register int i, j, n;

	if (!PyArg_NoArgs(args))
		return NULL;
  again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	if (n != mp->ma_used) {
		/* Durnit.  The allocations caused the dict to resize.
		 * Just start over, this shouldn't normally happen.
		 */
		Py_DECREF(v);
		goto again;
	}
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			PyObject *key = mp->ma_table[i].me_key;
			Py_INCREF(key);
			PyList_SET_ITEM(v, j, key);
			j++;
		}
	}
	return v;
}

static PyObject *
dict_values(register dictobject *mp, PyObject *args)
{
	register PyObject *v;
	register int i, j, n;

	if (!PyArg_NoArgs(args))
		return NULL;
  again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	if (n != mp->ma_used) {
		/* Durnit.  The allocations caused the dict to resize.
		 * Just start over, this shouldn't normally happen.
		 */
		Py_DECREF(v);
		goto again;
	}
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			PyObject *value = mp->ma_table[i].me_value;
			Py_INCREF(value);
			PyList_SET_ITEM(v, j, value);
			j++;
		}
	}
	return v;
}

static PyObject *
dict_items(register dictobject *mp, PyObject *args)
{
	register PyObject *v;
	register int i, j, n;
	PyObject *item, *key, *value;

	if (!PyArg_NoArgs(args))
		return NULL;
	/* Preallocate the list of tuples, to avoid allocations during
	 * the loop over the items, which could trigger GC, which
	 * could resize the dict. :-(
	 */
  again:
	n = mp->ma_used;
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		item = PyTuple_New(2);
		if (item == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SET_ITEM(v, i, item);
	}
	if (n != mp->ma_used) {
		/* Durnit.  The allocations caused the dict to resize.
		 * Just start over, this shouldn't normally happen.
		 */
		Py_DECREF(v);
		goto again;
	}
	/* Nothing we do below makes any function calls. */
	for (i = 0, j = 0; i < mp->ma_size; i++) {
		if (mp->ma_table[i].me_value != NULL) {
			key = mp->ma_table[i].me_key;
			value = mp->ma_table[i].me_value;
			item = PyList_GET_ITEM(v, j);
			Py_INCREF(key);
			PyTuple_SET_ITEM(item, 0, key);
			Py_INCREF(value);
			PyTuple_SET_ITEM(item, 1, value);
			j++;
		}
	}
	assert(j == n);
	return v;
}

static PyObject *
dict_update(register dictobject *mp, PyObject *args)
{
	register int i;
	dictobject *other;
	dictentry *entry;
	if (!PyArg_Parse(args, "O!", &PyDict_Type, &other))
		return NULL;
	if (other == mp || other->ma_used == 0)
		goto done; /* a.update(a) or a.update({}); nothing to do */
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
dict_copy(register dictobject *mp, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;
	return PyDict_Copy((PyObject*)mp);
}

PyObject *
PyDict_Copy(PyObject *o)
{
	register dictobject *mp;
	register int i;
	dictobject *copy;
	dictentry *entry;

	if (o == NULL || !PyDict_Check(o)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	mp = (dictobject *)o;
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
PyDict_Size(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return 0;
	}
	return ((dictobject *)mp)->ma_used;
}

PyObject *
PyDict_Keys(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_keys((dictobject *)mp, (PyObject *)NULL);
}

PyObject *
PyDict_Values(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_values((dictobject *)mp, (PyObject *)NULL);
}

PyObject *
PyDict_Items(PyObject *mp)
{
	if (mp == NULL || !PyDict_Check(mp)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return dict_items((dictobject *)mp, (PyObject *)NULL);
}

/* Subroutine which returns the smallest key in a for which b's value
   is different or absent.  The value is returned too, through the
   pval argument.  Both are NULL if no key in a is found for which b's status
   differs.  The refcounts on (and only on) non-NULL *pval and function return
   values must be decremented by the caller (characterize() increments them
   to ensure that mutating comparison and PyDict_GetItem calls can't delete
   them before the caller is done looking at them). */

static PyObject *
characterize(dictobject *a, dictobject *b, PyObject **pval)
{
	PyObject *akey = NULL; /* smallest key in a s.t. a[akey] != b[akey] */
	PyObject *aval = NULL; /* a[akey] */
	int i, cmp;

	for (i = 0; i < a->ma_size; i++) {
		PyObject *thiskey, *thisaval, *thisbval;
		if (a->ma_table[i].me_value == NULL)
			continue;
		thiskey = a->ma_table[i].me_key;
		Py_INCREF(thiskey);  /* keep alive across compares */
		if (akey != NULL) {
			cmp = PyObject_RichCompareBool(akey, thiskey, Py_LT);
			if (cmp < 0) {
				Py_DECREF(thiskey);
				goto Fail;
			}
			if (cmp > 0 ||
			    i >= a->ma_size ||
			    a->ma_table[i].me_value == NULL)
			{
				/* Not the *smallest* a key; or maybe it is
				 * but the compare shrunk the dict so we can't
				 * find its associated value anymore; or
				 * maybe it is but the compare deleted the
				 * a[thiskey] entry.
				 */
				Py_DECREF(thiskey);
				continue;
			}
		}

		/* Compare a[thiskey] to b[thiskey]; cmp <- true iff equal. */
		thisaval = a->ma_table[i].me_value;
		assert(thisaval);
		Py_INCREF(thisaval);   /* keep alive */
		thisbval = PyDict_GetItem((PyObject *)b, thiskey);
		if (thisbval == NULL)
			cmp = 0;
		else {
			/* both dicts have thiskey:  same values? */
			cmp = PyObject_RichCompareBool(
						thisaval, thisbval, Py_EQ);
			if (cmp < 0) {
		    		Py_DECREF(thiskey);
		    		Py_DECREF(thisaval);
		    		goto Fail;
			}
		}
		if (cmp == 0) {
			/* New winner. */
			Py_XDECREF(akey);
			Py_XDECREF(aval);
			akey = thiskey;
			aval = thisaval;
		}
		else {
			Py_DECREF(thiskey);
			Py_DECREF(thisaval);
		}
	}
	*pval = aval;
	return akey;

Fail:
	Py_XDECREF(akey);
	Py_XDECREF(aval);
	*pval = NULL;
	return NULL;
}

static int
dict_compare(dictobject *a, dictobject *b)
{
	PyObject *adiff, *bdiff, *aval, *bval;
	int res;

	/* Compare lengths first */
	if (a->ma_used < b->ma_used)
		return -1;	/* a is shorter */
	else if (a->ma_used > b->ma_used)
		return 1;	/* b is shorter */

	/* Same length -- check all keys */
	bdiff = bval = NULL;
	adiff = characterize(a, b, &aval);
	if (adiff == NULL) {
		assert(!aval);
		/* Either an error, or a is a subst with the same length so
		 * must be equal.
		 */
		res = PyErr_Occurred() ? -1 : 0;
		goto Finished;
	}
	bdiff = characterize(b, a, &bval);
	if (bdiff == NULL && PyErr_Occurred()) {
		assert(!bval);
		res = -1;
		goto Finished;
	}
	res = 0;
	if (bdiff) {
		/* bdiff == NULL "should be" impossible now, but perhaps
		 * the last comparison done by the characterize() on a had
		 * the side effect of making the dicts equal!
		 */
		res = PyObject_Compare(adiff, bdiff);
	}
	if (res == 0 && bval != NULL)
		res = PyObject_Compare(aval, bval);

Finished:
	Py_XDECREF(adiff);
	Py_XDECREF(bdiff);
	Py_XDECREF(aval);
	Py_XDECREF(bval);
	return res;
}

static PyObject *
dict_has_key(register dictobject *mp, PyObject *args)
{
	PyObject *key;
	long hash;
	register long ok;
	if (!PyArg_ParseTuple(args, "O:has_key", &key))
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
	ok = (mp->ma_size != 0
	      && (mp->ma_lookup)(mp, key, hash)->me_value != NULL);
	return PyInt_FromLong(ok);
}

static PyObject *
dict_get(register dictobject *mp, PyObject *args)
{
	PyObject *key;
	PyObject *failobj = Py_None;
	PyObject *val = NULL;
	long hash;

	if (!PyArg_ParseTuple(args, "O|O:get", &key, &failobj))
		return NULL;
	if (mp->ma_table == NULL)
		goto finally;

#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	val = (mp->ma_lookup)(mp, key, hash)->me_value;

  finally:
	if (val == NULL)
		val = failobj;
	Py_INCREF(val);
	return val;
}


static PyObject *
dict_setdefault(register dictobject *mp, PyObject *args)
{
	PyObject *key;
	PyObject *failobj = Py_None;
	PyObject *val = NULL;
	long hash;

	if (!PyArg_ParseTuple(args, "O|O:setdefault", &key, &failobj))
		return NULL;
	if (mp->ma_table == NULL)
		goto finally;

#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return NULL;
	}
	val = (mp->ma_lookup)(mp, key, hash)->me_value;

  finally:
	if (val == NULL) {
		val = failobj;
		if (PyDict_SetItem((PyObject*)mp, key, failobj))
			val = NULL;
	}
	Py_XINCREF(val);
	return val;
}


static PyObject *
dict_clear(register dictobject *mp, PyObject *args)
{
	if (!PyArg_NoArgs(args))
		return NULL;
	PyDict_Clear((PyObject *)mp);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
dict_popitem(dictobject *mp, PyObject *args)
{
	int i = 0;
	dictentry *ep;
	PyObject *res;

	if (!PyArg_NoArgs(args))
		return NULL;
	/* Allocate the result tuple first.  Believe it or not,
	 * this allocation could trigger a garbage collection which
	 * could resize the dict, which would invalidate the pointer
	 * (ep) into the dict calculated below.
	 * So we have to do this first.
	 */
	res = PyTuple_New(2);
	if (res == NULL)
		return NULL;
	if (mp->ma_used == 0) {
		Py_DECREF(res);
		PyErr_SetString(PyExc_KeyError,
				"popitem(): dictionary is empty");
		return NULL;
	}
	/* Set ep to "the first" dict entry with a value.  We abuse the hash
	 * field of slot 0 to hold a search finger:
	 * If slot 0 has a value, use slot 0.
	 * Else slot 0 is being used to hold a search finger,
	 * and we use its hash value as the first index to look.
	 */
	ep = &mp->ma_table[0];
	if (ep->me_value == NULL) {
		i = (int)ep->me_hash;
		/* The hash field may be uninitialized trash, or it
		 * may be a real hash value, or it may be a legit
		 * search finger, or it may be a once-legit search
		 * finger that's out of bounds now because it
		 * wrapped around or the table shrunk -- simply
		 * make sure it's in bounds now.
		 */
		if (i >= mp->ma_size || i < 1)
			i = 1;	/* skip slot 0 */
		while ((ep = &mp->ma_table[i])->me_value == NULL) {
			i++;
			if (i >= mp->ma_size)
				i = 1;
		}
	}
	PyTuple_SET_ITEM(res, 0, ep->me_key);
	PyTuple_SET_ITEM(res, 1, ep->me_value);
	Py_INCREF(dummy);
	ep->me_key = dummy;
	ep->me_value = NULL;
	mp->ma_used--;
	assert(mp->ma_table[0].me_value == NULL);
	mp->ma_table[0].me_hash = i + 1;  /* next place to start */
	return res;
}

static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
	int i = 0, err;
	PyObject *pk;
	PyObject *pv;

	while (PyDict_Next(op, &i, &pk, &pv)) {
		err = visit(pk, arg);
		if (err)
			return err;
		err = visit(pv, arg);
		if (err)
			return err;
	}
	return 0;
}

static int
dict_tp_clear(PyObject *op)
{
	PyDict_Clear(op);
	return 0;
}


static char has_key__doc__[] =
"D.has_key(k) -> 1 if D has a key k, else 0";

static char get__doc__[] =
"D.get(k[,d]) -> D[k] if D.has_key(k), else d.  d defaults to None.";

static char setdefault_doc__[] =
"D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if not D.has_key(k)";

static char popitem__doc__[] =
"D.popitem() -> (k, v), remove and return some (key, value) pair as a\n\
2-tuple; but raise KeyError if D is empty";

static char keys__doc__[] =
"D.keys() -> list of D's keys";

static char items__doc__[] =
"D.items() -> list of D's (key, value) pairs, as 2-tuples";

static char values__doc__[] =
"D.values() -> list of D's values";

static char update__doc__[] =
"D.update(E) -> None.  Update D from E: for k in E.keys(): D[k] = E[k]";

static char clear__doc__[] =
"D.clear() -> None.  Remove all items from D.";

static char copy__doc__[] =
"D.copy() -> a shallow copy of D";

static PyMethodDef mapp_methods[] = {
	{"has_key",	(PyCFunction)dict_has_key,      METH_VARARGS,
	 has_key__doc__},
	{"get",         (PyCFunction)dict_get,          METH_VARARGS,
	 get__doc__},
	{"setdefault",  (PyCFunction)dict_setdefault,   METH_VARARGS,
	 setdefault_doc__},
	{"popitem",	(PyCFunction)dict_popitem,	METH_OLDARGS,
	 popitem__doc__},
	{"keys",	(PyCFunction)dict_keys,		METH_OLDARGS,
	keys__doc__},
	{"items",	(PyCFunction)dict_items,	METH_OLDARGS,
	 items__doc__},
	{"values",	(PyCFunction)dict_values,	METH_OLDARGS,
	 values__doc__},
	{"update",	(PyCFunction)dict_update,	METH_OLDARGS,
	 update__doc__},
	{"clear",	(PyCFunction)dict_clear,	METH_OLDARGS,
	 clear__doc__},
	{"copy",	(PyCFunction)dict_copy,		METH_OLDARGS,
	 copy__doc__},
	{NULL,		NULL}	/* sentinel */
};

static PyObject *
dict_getattr(dictobject *mp, char *name)
{
	return Py_FindMethod(mapp_methods, (PyObject *)mp, name);
}

PyTypeObject PyDict_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"dictionary",
	sizeof(dictobject) + PyGC_HEAD_SIZE,
	0,
	(destructor)dict_dealloc,		/* tp_dealloc */
	(printfunc)dict_print,			/* tp_print */
	(getattrfunc)dict_getattr,		/* tp_getattr */
	0,					/* tp_setattr */
	(cmpfunc)dict_compare,			/* tp_compare */
	(reprfunc)dict_repr,			/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	&dict_as_mapping,			/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_GC,	/* tp_flags */
	0,					/* tp_doc */
	(traverseproc)dict_traverse,		/* tp_traverse */
	(inquiry)dict_tp_clear,			/* tp_clear */
	0,					/* tp_richcompare */
};

/* For backward compatibility with old dictionary interface */

PyObject *
PyDict_GetItemString(PyObject *v, char *key)
{
	PyObject *kv, *rv;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return NULL;
	rv = PyDict_GetItem(v, kv);
	Py_DECREF(kv);
	return rv;
}

int
PyDict_SetItemString(PyObject *v, char *key, PyObject *item)
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return -1;
	PyString_InternInPlace(&kv); /* XXX Should we really? */
	err = PyDict_SetItem(v, kv, item);
	Py_DECREF(kv);
	return err;
}

int
PyDict_DelItemString(PyObject *v, char *key)
{
	PyObject *kv;
	int err;
	kv = PyString_FromString(key);
	if (kv == NULL)
		return -1;
	err = PyDict_DelItem(v, kv);
	Py_DECREF(kv);
	return err;
}
