
/* Dictionary object implementation using a hash table */

#include "Python.h"

/* MINSIZE is the minimum size of a dictionary.  This many slots are
 * allocated directly in the dict object (in the ma_smalltable member).
 * It must be a power of 2, and at least 4.  8 allows dicts with no more than
 * 5 active entries to live in ma_smalltable (and so avoid an additional
 * malloc); instrumentation suggested this suffices for the majority of
 * dicts (consisting mostly of usually-small instance dicts and usually-small
 * dicts created to pass keyword arguments).
 */
#define MINSIZE 8

/* Define this out if you don't want conversion statistics on exit. */
#undef SHOW_CONVERSION_COUNTS

/* See large comment block below.  This must be >= 1. */
#define PERTURB_SHIFT 5

/*
Major subtleties ahead:  Most hash schemes depend on having a "good" hash
function, in the sense of simulating randomness.  Python doesn't:  its most
important hash functions (for strings and ints) are very regular in common
cases:

>>> map(hash, (0, 1, 2, 3))
[0, 1, 2, 3]
>>> map(hash, ("namea", "nameb", "namec", "named"))
[-1658398457, -1658398460, -1658398459, -1658398462]
>>>

This isn't necessarily bad!  To the contrary, in a table of size 2**i, taking
the low-order i bits as the initial table index is extremely fast, and there
are no collisions at all for dicts indexed by a contiguous range of ints.
The same is approximately true when keys are "consecutive" strings.  So this
gives better-than-random behavior in common cases, and that's very desirable.

OTOH, when collisions occur, the tendency to fill contiguous slices of the
hash table makes a good collision resolution strategy crucial.  Taking only
the last i bits of the hash code is also vulnerable:  for example, consider
[i << 16 for i in range(20000)] as a set of keys.  Since ints are their own
hash codes, and this fits in a dict of size 2**15, the last 15 bits of every
hash code are all 0:  they *all* map to the same table index.

But catering to unusual cases should not slow the usual ones, so we just take
the last i bits anyway.  It's up to collision resolution to do the rest.  If
we *usually* find the key we're looking for on the first try (and, it turns
out, we usually do -- the table load factor is kept under 2/3, so the odds
are solidly in our favor), then it makes best sense to keep the initial index
computation dirt cheap.

The first half of collision resolution is to visit table indices via this
recurrence:

    j = ((5*j) + 1) mod 2**i

For any initial j in range(2**i), repeating that 2**i times generates each
int in range(2**i) exactly once (see any text on random-number generation for
proof).  By itself, this doesn't help much:  like linear probing (setting
j += 1, or j -= 1, on each loop trip), it scans the table entries in a fixed
order.  This would be bad, except that's not the only thing we do, and it's
actually *good* in the common cases where hash keys are consecutive.  In an
example that's really too small to make this entirely clear, for a table of
size 2**3 the order of indices is:

    0 -> 1 -> 6 -> 7 -> 4 -> 5 -> 2 -> 3 -> 0 [and here it's repeating]

If two things come in at index 5, the first place we look after is index 2,
not 6, so if another comes in at index 6 the collision at 5 didn't hurt it.
Linear probing is deadly in this case because there the fixed probe order
is the *same* as the order consecutive keys are likely to arrive.  But it's
extremely unlikely hash codes will follow a 5*j+1 recurrence by accident,
and certain that consecutive hash codes do not.

The other half of the strategy is to get the other bits of the hash code
into play.  This is done by initializing a (unsigned) vrbl "perturb" to the
full hash code, and changing the recurrence to:

    j = (5*j) + 1 + perturb;
    perturb >>= PERTURB_SHIFT;
    use j % 2**i as the next table index;

Now the probe sequence depends (eventually) on every bit in the hash code,
and the pseudo-scrambling property of recurring on 5*j+1 is more valuable,
because it quickly magnifies small differences in the bits that didn't affect
the initial index.  Note that because perturb is unsigned, if the recurrence
is executed often enough perturb eventually becomes and remains 0.  At that
point (very rarely reached) the recurrence is on (just) 5*j+1 again, and
that's certain to find an empty slot eventually (since it generates every int
in range(2**i), and we make sure there's always at least one empty slot).

Selecting a good value for PERTURB_SHIFT is a balancing act.  You want it
small so that the high bits of the hash code continue to affect the probe
sequence across iterations; but you want it large so that in really bad cases
the high-order hash bits have an effect on early iterations.  5 was "the
best" in minimizing total collisions across experiments Tim Peters ran (on
both normal and pathological cases), but 4 and 6 weren't significantly worse.

Historical:  Reimer Behrends contributed the idea of using a polynomial-based
approach, using repeated multiplication by x in GF(2**n) where an irreducible
polynomial for each table size was chosen such that x was a primitive root.
Christian Tismer later extended that to use division by x instead, as an
efficient way to get the high bits of the hash code into play.  This scheme
also gave excellent collision statistics, but was more expensive:  two
if-tests were required inside the loop; computing "the next" index took about
the same number of operations but without as much potential parallelism
(e.g., computing 5*j can go on at the same time as computing 1+perturb in the
above, and then shifting perturb can be done while the table index is being
masked); and the dictobject struct required a member to hold the table's
polynomial.  In Tim's experiments the current scheme ran faster, produced
equally good collision statistics, needed less code & used less memory.
*/

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
	/* ma_table points to ma_smalltable for small tables, else to
	 * additional malloc'ed memory.  ma_table is never NULL!  This rule
	 * saves repeated runtime null-tests in the workhorse getitem and
	 * setitem calls.
	 */
	dictentry *ma_table;
	dictentry *(*ma_lookup)(dictobject *mp, PyObject *key, long hash);
	dictentry ma_smalltable[MINSIZE];
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

/* Set dictobject* mp to empty but w/ MINSIZE slots, using ma_smalltable. */
#define empty_to_minsize(mp) do { 					\
	memset((mp)->ma_smalltable, 0, sizeof((mp)->ma_smalltable));	\
	(mp)->ma_table = (mp)->ma_smalltable;				\
	(mp)->ma_size = MINSIZE;					\
	(mp)->ma_used = (mp)->ma_fill = 0;				\
    } while(0)

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
	empty_to_minsize(mp);
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

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained earlier.

All arithmetic on hash should ignore overflow.

(The details in this version are due to Tim Peters, building on many past
contributions by Reimer Behrends, Jyrki Alakuijala, Vladimir Marangozov and
Christian Tismer).

This function must never return NULL; failures are indicated by returning
a dictentry* for which the me_value field is NULL.  Exceptions are never
reported by this function, and outstanding exceptions are maintained.
*/

static dictentry *
lookdict(dictobject *mp, PyObject *key, register long hash)
{
	register int i;
	register unsigned int perturb;
	register dictentry *freeslot;
	register unsigned int mask = mp->ma_size-1;
	dictentry *ep0 = mp->ma_table;
	register dictentry *ep;
	register int restore_error;
	register int checked_error;
	register int cmp;
	PyObject *err_type, *err_value, *err_tb;
	PyObject *startkey;

	i = hash & mask;
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
		return ep;

	restore_error = checked_error = 0;
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
			startkey = ep->me_key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				PyErr_Clear();
			if (ep0 == mp->ma_table && ep->me_key == startkey) {
				if (cmp > 0)
					goto Done;
			}
			else {
				/* The compare did major nasty stuff to the
				 * dict:  start over.
				 * XXX A clever adversary could prevent this
				 * XXX from terminating.
 				 */
 				ep = lookdict(mp, key, hash);
 				goto Done;
 			}
		}
		freeslot = NULL;
	}

	/* In the loop, me_key == dummy is by far (factor of 100s) the
	   least likely outcome, so test for that last. */
	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		ep = &ep0[i & mask];
		if (ep->me_key == NULL) {
			if (freeslot != NULL)
				ep = freeslot;
			break;
		}
		if (ep->me_key == key)
			break;
		if (ep->me_hash == hash && ep->me_key != dummy) {
			if (!checked_error) {
				checked_error = 1;
				if (PyErr_Occurred()) {
					restore_error = 1;
					PyErr_Fetch(&err_type, &err_value,
						    &err_tb);
				}
			}
			startkey = ep->me_key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				PyErr_Clear();
			if (ep0 == mp->ma_table && ep->me_key == startkey) {
				if (cmp > 0)
					break;
			}
			else {
				/* The compare did major nasty stuff to the
				 * dict:  start over.
				 * XXX A clever adversary could prevent this
				 * XXX from terminating.
 				 */
 				ep = lookdict(mp, key, hash);
 				break;
 			}
		}
		else if (ep->me_key == dummy && freeslot == NULL)
			freeslot = ep;
	}

Done:
	if (restore_error)
		PyErr_Restore(err_type, err_value, err_tb);
	return ep;
}

/*
 * Hacked up version of lookdict which can assume keys are always strings;
 * this assumption allows testing for errors during PyObject_Compare() to
 * be dropped; string-string comparisons never raise exceptions.  This also
 * means we don't need to go through PyObject_Compare(); we can always use
 * _PyString_Eq directly.
 *
 * This really only becomes meaningful if proper error handling in lookdict()
 * is too expensive.
 */
static dictentry *
lookdict_string(dictobject *mp, PyObject *key, register long hash)
{
	register int i;
	register unsigned int perturb;
	register dictentry *freeslot;
	register unsigned int mask = mp->ma_size-1;
	dictentry *ep0 = mp->ma_table;
	register dictentry *ep;

	/* make sure this function doesn't have to handle non-string keys */
	if (!PyString_Check(key)) {
#ifdef SHOW_CONVERSION_COUNTS
		++converted;
#endif
		mp->ma_lookup = lookdict;
		return lookdict(mp, key, hash);
	}
	i = hash & mask;
	ep = &ep0[i];
	if (ep->me_key == NULL || ep->me_key == key)
		return ep;
	if (ep->me_key == dummy)
		freeslot = ep;
	else {
		if (ep->me_hash == hash
		    && _PyString_Eq(ep->me_key, key)) {
			return ep;
		}
		freeslot = NULL;
	}

	/* In the loop, me_key == dummy is by far (factor of 100s) the
	   least likely outcome, so test for that last. */
	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		ep = &ep0[i & mask];
		if (ep->me_key == NULL)
			return freeslot == NULL ? ep : freeslot;
		if (ep->me_key == key
		    || (ep->me_hash == hash
		        && ep->me_key != dummy
			&& _PyString_Eq(ep->me_key, key)))
			return ep;
		if (ep->me_key == dummy && freeslot == NULL)
			freeslot = ep;
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
	int newsize;
	dictentry *oldtable, *newtable, *ep;
	int i;
	int is_oldtable_malloced;
	dictentry small_copy[MINSIZE];

	assert(minused >= 0);

	/* Find the smallest table size > minused. */
	for (newsize = MINSIZE;
	     newsize <= minused && newsize > 0;
	     newsize <<= 1)
		;
	if (newsize <= 0) {
		PyErr_NoMemory();
		return -1;
	}

	/* Get space for a new table. */
	oldtable = mp->ma_table;
	assert(oldtable != NULL);
	is_oldtable_malloced = oldtable != mp->ma_smalltable;

	if (newsize == MINSIZE) {
		/* A large table is shrinking, or we can't get any smaller. */
		newtable = mp->ma_smalltable;
		if (newtable == oldtable) {
			if (mp->ma_fill == mp->ma_used) {
				/* No dummies, so no point doing anything. */
				return 0;
			}
			/* We're not going to resize it, but rebuild the
			   table anyway to purge old dummy entries.
			   Subtle:  This is *necessary* if fill==size,
			   as lookdict needs at least one virgin slot to
			   terminate failing searches.  If fill < size, it's
			   merely desirable, as dummies slow searches. */
			assert(mp->ma_fill > mp->ma_used);
			memcpy(small_copy, oldtable, sizeof(small_copy));
			oldtable = small_copy;
		}
	}
	else {
		newtable = PyMem_NEW(dictentry, newsize);
		if (newtable == NULL) {
			PyErr_NoMemory();
			return -1;
		}
	}

	/* Make the dict empty, using the new table. */
	assert(newtable != oldtable);
	mp->ma_table = newtable;
	mp->ma_size = newsize;
	memset(newtable, 0, sizeof(dictentry) * newsize);
	mp->ma_used = 0;
	i = mp->ma_fill;
	mp->ma_fill = 0;

	/* Copy the data over; this is refcount-neutral for active entries;
	   dummy entries aren't copied over, of course */
	for (ep = oldtable; i > 0; ep++) {
		if (ep->me_value != NULL) {	/* active entry */
			--i;
			insertdict(mp, ep->me_key, ep->me_hash, ep->me_value);
		}
		else if (ep->me_key != NULL) {	/* dummy entry */
			--i;
			assert(ep->me_key == dummy);
			Py_DECREF(ep->me_key);
		}
		/* else key == value == NULL:  nothing to do */
	}

	if (is_oldtable_malloced)
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
	assert(mp->ma_fill < mp->ma_size);
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
	ep = (mp->ma_lookup)(mp, key, hash);
	if (ep->me_value == NULL) {
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
	dictobject *mp;
	dictentry *ep, *table;
	int table_is_malloced;
	int fill;
	dictentry small_copy[MINSIZE];
#ifdef Py_DEBUG
	int i, n;
#endif

	if (!PyDict_Check(op))
		return;
	mp = (dictobject *)op;
#ifdef Py_DEBUG
	n = mp->ma_size;
	i = 0;
#endif

	table = mp->ma_table;
	assert(table != NULL);
	table_is_malloced = table != mp->ma_smalltable;

	/* This is delicate.  During the process of clearing the dict,
	 * decrefs can cause the dict to mutate.  To avoid fatal confusion
	 * (voice of experience), we have to make the dict empty before
	 * clearing the slots, and never refer to anything via mp->xxx while
	 * clearing.
	 */
	fill = mp->ma_fill;
	if (table_is_malloced)
		empty_to_minsize(mp);

	else if (fill > 0) {
		/* It's a small table with something that needs to be cleared.
		 * Afraid the only safe way is to copy the dict entries into
		 * another small table first.
		 */
		memcpy(small_copy, table, sizeof(small_copy));
		table = small_copy;
		empty_to_minsize(mp);
	}
	/* else it's a small table that's already empty */

	/* Now we can finally clear things.  If C had refcounts, we could
	 * assert that the refcount on table is 1 now, i.e. that this function
	 * has unique access to it, so decref side-effects can't alter it.
	 */
	for (ep = table; fill > 0; ++ep) {
#ifdef Py_DEBUG
		assert(i < n);
		++i;
#endif
		if (ep->me_key) {
			--fill;
			Py_DECREF(ep->me_key);
			Py_XDECREF(ep->me_value);
		}
#ifdef Py_DEBUG
		else
			assert(ep->me_value == NULL);
#endif
	}

	if (table_is_malloced)
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
	register dictentry *ep;
	int fill = mp->ma_fill;
	Py_TRASHCAN_SAFE_BEGIN(mp)
	PyObject_GC_Fini(mp);
	for (ep = mp->ma_table; fill > 0; ep++) {
		if (ep->me_key) {
			--fill;
			Py_DECREF(ep->me_key);
			Py_XDECREF(ep->me_value);
		}
	}
	if (mp->ma_table != mp->ma_smalltable)
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

	i = Py_ReprEnter((PyObject*)mp);
	if (i != 0) {
		if (i < 0)
			return i;
		fprintf(fp, "{...}");
		return 0;
	}

	fprintf(fp, "{");
	any = 0;
	for (i = 0; i < mp->ma_size; i++) {
		dictentry *ep = mp->ma_table + i;
		PyObject *pvalue = ep->me_value;
		if (pvalue != NULL) {
			/* Prevent PyObject_Repr from deleting value during
			   key format */
			Py_INCREF(pvalue);
			if (any++ > 0)
				fprintf(fp, ", ");
			if (PyObject_Print((PyObject *)ep->me_key, fp, 0)!=0) {
				Py_DECREF(pvalue);
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
			fprintf(fp, ": ");
			if (PyObject_Print(pvalue, fp, 0) != 0) {
				Py_DECREF(pvalue);
				Py_ReprLeave((PyObject*)mp);
				return -1;
			}
			Py_DECREF(pvalue);
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
	for (i = 0; i < mp->ma_size && v; i++) {
		dictentry *ep = mp->ma_table + i;
		PyObject *pvalue = ep->me_value;
		if (pvalue != NULL) {
			/* Prevent PyObject_Repr from deleting value during
			   key format */
			Py_INCREF(pvalue);
			if (any++)
				PyString_Concat(&v, sepa);
			PyString_ConcatAndDel(&v, PyObject_Repr(ep->me_key));
			PyString_Concat(&v, colon);
			PyString_ConcatAndDel(&v, PyObject_Repr(pvalue));
			Py_DECREF(pvalue);
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
	assert(mp->ma_table != NULL);
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
		/* Either an error, or a is a subset with the same length so
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

/* Return 1 if dicts equal, 0 if not, -1 if error.
 * Gets out as soon as any difference is detected.
 * Uses only Py_EQ comparison.
 */
static int
dict_equal(dictobject *a, dictobject *b)
{
	int i;

	if (a->ma_used != b->ma_used)
		/* can't be equal if # of entries differ */
		return 0;

	/* Same # of entries -- check all of 'em.  Exit early on any diff. */
	for (i = 0; i < a->ma_size; i++) {
		PyObject *aval = a->ma_table[i].me_value;
		if (aval != NULL) {
			int cmp;
			PyObject *bval;
			PyObject *key = a->ma_table[i].me_key;
			/* temporarily bump aval's refcount to ensure it stays
			   alive until we're done with it */
			Py_INCREF(aval);
			bval = PyDict_GetItem((PyObject *)b, key);
			if (bval == NULL) {
				Py_DECREF(aval);
				return 0;
			}
			cmp = PyObject_RichCompareBool(aval, bval, Py_EQ);
			Py_DECREF(aval);
			if (cmp <= 0)  /* error or not equal */
				return cmp;
 		}
	}
	return 1;
 }

static PyObject *
dict_richcompare(PyObject *v, PyObject *w, int op)
{
	int cmp;
	PyObject *res;

	if (!PyDict_Check(v) || !PyDict_Check(w)) {
		res = Py_NotImplemented;
	}
	else if (op == Py_EQ || op == Py_NE) {
		cmp = dict_equal((dictobject *)v, (dictobject *)w);
		if (cmp < 0)
			return NULL;
		res = (cmp == (op == Py_EQ)) ? Py_True : Py_False;
	}
	else
		res = Py_NotImplemented;
	Py_INCREF(res);
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
	ok = (mp->ma_lookup)(mp, key, hash)->me_value != NULL;
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
	/* Allocate the result tuple before checking the size.  Believe it
	 * or not, this allocation could trigger a garbage collection which
	 * could empty the dict, so if we checked the size first and that
	 * happened, the result would be an infinite loop (searching for an
	 * entry that no longer exists).  Note that the usual popitem()
	 * idiom is "while d: k, v = d.popitem()". so needing to throw the
	 * tuple away  if the dict *is* empty isn't a significant
	 * inefficiency -- possible, but unlikely in practice.
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
		/* The hash field may be a real hash value, or it may be a
		 * legit search finger, or it may be a once-legit search
		 * finger that's out of bounds now because it wrapped around
		 * or the table shrunk -- simply make sure it's in bounds now.
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


staticforward PyObject *dictiter_new(dictobject *, binaryfunc);

static PyObject *
select_key(PyObject *key, PyObject *value)
{
	Py_INCREF(key);
	return key;
}

static PyObject *
select_value(PyObject *key, PyObject *value)
{
	Py_INCREF(value);
	return value;
}

static PyObject *
select_item(PyObject *key, PyObject *value)
{
	PyObject *res = PyTuple_New(2);

	if (res != NULL) {
		Py_INCREF(key);
		Py_INCREF(value);
		PyTuple_SET_ITEM(res, 0, key);
		PyTuple_SET_ITEM(res, 1, value);
	}
	return res;
}

static PyObject *
dict_iterkeys(dictobject *dict, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return dictiter_new(dict, select_key);
}

static PyObject *
dict_itervalues(dictobject *dict, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return dictiter_new(dict, select_value);
}

static PyObject *
dict_iteritems(dictobject *dict, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ""))
		return NULL;
	return dictiter_new(dict, select_item);
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

static char iterkeys__doc__[] =
"D.iterkeys() -> an iterator over the keys of D";

static char itervalues__doc__[] =
"D.itervalues() -> an iterator over the values of D";

static char iteritems__doc__[] =
"D.iteritems() -> an iterator over the (key, value) items of D";

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
	{"iterkeys",	(PyCFunction)dict_iterkeys,	METH_VARARGS,
	 iterkeys__doc__},
	{"itervalues",	(PyCFunction)dict_itervalues,	METH_VARARGS,
	 itervalues__doc__},
	{"iteritems",	(PyCFunction)dict_iteritems,	METH_VARARGS,
	 iteritems__doc__},
	{NULL,		NULL}	/* sentinel */
};

static PyObject *
dict_getattr(dictobject *mp, char *name)
{
	return Py_FindMethod(mapp_methods, (PyObject *)mp, name);
}

static int
dict_contains(dictobject *mp, PyObject *key)
{
	long hash;

#ifdef CACHE_HASH
	if (!PyString_Check(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1)
#endif
	{
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	return (mp->ma_lookup)(mp, key, hash)->me_value != NULL;
}

/* Hack to implement "key in dict" */
static PySequenceMethods dict_as_sequence = {
	0,					/* sq_length */
	0,					/* sq_concat */
	0,					/* sq_repeat */
	0,					/* sq_item */
	0,					/* sq_slice */
	0,					/* sq_ass_item */
	0,					/* sq_ass_slice */
	(objobjproc)dict_contains,		/* sq_contains */
	0,					/* sq_inplace_concat */
	0,					/* sq_inplace_repeat */
};

static PyObject *
dict_iter(dictobject *dict)
{
	return dictiter_new(dict, select_key);
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
	&dict_as_sequence,			/* tp_as_sequence */
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
	dict_richcompare,			/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)dict_iter,			/* tp_iter */
	0,					/* tp_iternext */
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

/* Dictionary iterator type */

extern PyTypeObject PyDictIter_Type; /* Forward */

typedef struct {
	PyObject_HEAD
	dictobject *di_dict;
	int di_used;
	int di_pos;
	binaryfunc di_select;
} dictiterobject;

static PyObject *
dictiter_new(dictobject *dict, binaryfunc select)
{
	dictiterobject *di;
	di = PyObject_NEW(dictiterobject, &PyDictIter_Type);
	if (di == NULL)
		return NULL;
	Py_INCREF(dict);
	di->di_dict = dict;
	di->di_used = dict->ma_used;
	di->di_pos = 0;
	di->di_select = select;
	return (PyObject *)di;
}

static void
dictiter_dealloc(dictiterobject *di)
{
	Py_DECREF(di->di_dict);
	PyObject_DEL(di);
}

static PyObject *
dictiter_next(dictiterobject *di, PyObject *args)
{
	PyObject *key, *value;

	if (di->di_used != di->di_dict->ma_used) {
		PyErr_SetString(PyExc_RuntimeError,
				"dictionary changed size during iteration");
		return NULL;
	}
	if (PyDict_Next((PyObject *)(di->di_dict), &di->di_pos, &key, &value)) {
		return (*di->di_select)(key, value);
	}
	PyErr_SetObject(PyExc_StopIteration, Py_None);
	return NULL;
}

static PyObject *
dictiter_getiter(PyObject *it)
{
	Py_INCREF(it);
	return it;
}

static PyMethodDef dictiter_methods[] = {
	{"next",	(PyCFunction)dictiter_next,	METH_VARARGS,
	 "it.next() -- get the next value, or raise StopIteration"},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
dictiter_getattr(dictiterobject *di, char *name)
{
	return Py_FindMethod(dictiter_methods, (PyObject *)di, name);
}

static PyObject *dictiter_iternext(dictiterobject *di)
{
	PyObject *key, *value;

	if (di->di_used != di->di_dict->ma_used) {
		PyErr_SetString(PyExc_RuntimeError,
				"dictionary changed size during iteration");
		return NULL;
	}
	if (PyDict_Next((PyObject *)(di->di_dict), &di->di_pos, &key, &value)) {
		return (*di->di_select)(key, value);
	}
	return NULL;
}

PyTypeObject PyDictIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
	"dictionary-iterator",			/* tp_name */
	sizeof(dictiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)dictiter_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	(getattrfunc)dictiter_getattr,		/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	0,					/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	(getiterfunc)dictiter_getiter,		/* tp_iter */
	(iternextfunc)dictiter_iternext,	/* tp_iternext */
};
