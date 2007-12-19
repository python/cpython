
/* set object implementation 
   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Derived from Lib/sets.py and Objects/dictobject.c.

   Copyright (c) 2003-2007 Python Software Foundation.
   All rights reserved.
*/

#include "Python.h"
#include "structmember.h"

/* Set a key error with the specified argument, wrapping it in a
 * tuple automatically so that tuple keys are not unpacked as the
 * exception arguments. */
static void
set_key_error(PyObject *arg)
{
	PyObject *tup;
	tup = PyTuple_Pack(1, arg);
	if (!tup)
		return; /* caller will expect error to be set anyway */
	PyErr_SetObject(PyExc_KeyError, tup);
	Py_DECREF(tup);
}

/* This must be >= 1. */
#define PERTURB_SHIFT 5

/* Object used as dummy key to fill deleted entries */
static PyObject *dummy = NULL; /* Initialized by first call to make_new_set() */

#ifdef Py_REF_DEBUG
PyObject *
_PySet_Dummy(void)
{
	return dummy;
}
#endif

#define INIT_NONZERO_SET_SLOTS(so) do {				\
	(so)->table = (so)->smalltable;				\
	(so)->mask = PySet_MINSIZE - 1;				\
	(so)->hash = -1;					\
    } while(0)

#define EMPTY_TO_MINSIZE(so) do {				\
	memset((so)->smalltable, 0, sizeof((so)->smalltable));	\
	(so)->used = (so)->fill = 0;				\
	INIT_NONZERO_SET_SLOTS(so);				\
    } while(0)

/* Reuse scheme to save calls to malloc, free, and memset */
#define MAXFREESETS 80
static PySetObject *free_sets[MAXFREESETS];
static int num_free_sets = 0;

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained in Objects/dictobject.c.

All arithmetic on hash should ignore overflow.

Unlike the dictionary implementation, the lookkey functions can return
NULL if the rich comparison returns an error.
*/

static setentry *
set_lookkey(PySetObject *so, PyObject *key, register long hash)
{
	register Py_ssize_t i;
	register size_t perturb;
	register setentry *freeslot;
	register size_t mask = so->mask;
	setentry *table = so->table;
	register setentry *entry;
	register int cmp;
	PyObject *startkey;

	i = hash & mask;
	entry = &table[i];
	if (entry->key == NULL || entry->key == key)
		return entry;

	if (entry->key == dummy)
		freeslot = entry;
	else {
		if (entry->hash == hash) {
			startkey = entry->key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				return NULL;
			if (table == so->table && entry->key == startkey) {
				if (cmp > 0)
					return entry;
			}
			else {
				/* The compare did major nasty stuff to the
				 * set:  start over.
 				 */
 				return set_lookkey(so, key, hash);
 			}
		}
		freeslot = NULL;
	}

	/* In the loop, key == dummy is by far (factor of 100s) the
	   least likely outcome, so test for that last. */
	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		entry = &table[i & mask];
		if (entry->key == NULL) {
			if (freeslot != NULL)
				entry = freeslot;
			break;
		}
		if (entry->key == key)
			break;
		if (entry->hash == hash && entry->key != dummy) {
			startkey = entry->key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				return NULL;
			if (table == so->table && entry->key == startkey) {
				if (cmp > 0)
					break;
			}
			else {
				/* The compare did major nasty stuff to the
				 * set:  start over.
 				 */
 				return set_lookkey(so, key, hash);
 			}
		}
		else if (entry->key == dummy && freeslot == NULL)
			freeslot = entry;
	}
	return entry;
}

/*
 * Hacked up version of set_lookkey which can assume keys are always strings;
 * This means we can always use _PyString_Eq directly and not have to check to
 * see if the comparison altered the table.
 */
static setentry *
set_lookkey_string(PySetObject *so, PyObject *key, register long hash)
{
	register Py_ssize_t i;
	register size_t perturb;
	register setentry *freeslot;
	register size_t mask = so->mask;
	setentry *table = so->table;
	register setentry *entry;

	/* Make sure this function doesn't have to handle non-string keys,
	   including subclasses of str; e.g., one reason to subclass
	   strings is to override __eq__, and for speed we don't cater to
	   that here. */
	if (!PyString_CheckExact(key)) {
		so->lookup = set_lookkey;
		return set_lookkey(so, key, hash);
	}
	i = hash & mask;
	entry = &table[i];
	if (entry->key == NULL || entry->key == key)
		return entry;
	if (entry->key == dummy)
		freeslot = entry;
	else {
		if (entry->hash == hash && _PyString_Eq(entry->key, key))
			return entry;
		freeslot = NULL;
	}

	/* In the loop, key == dummy is by far (factor of 100s) the
	   least likely outcome, so test for that last. */
	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		entry = &table[i & mask];
		if (entry->key == NULL)
			return freeslot == NULL ? entry : freeslot;
		if (entry->key == key
		    || (entry->hash == hash
			&& entry->key != dummy
			&& _PyString_Eq(entry->key, key)))
			return entry;
		if (entry->key == dummy && freeslot == NULL)
			freeslot = entry;
	}
	assert(0);	/* NOT REACHED */
	return 0;
}

/*
Internal routine to insert a new key into the table.
Used by the public insert routine.
Eats a reference to key.
*/
static int
set_insert_key(register PySetObject *so, PyObject *key, long hash)
{
	register setentry *entry;
	typedef setentry *(*lookupfunc)(PySetObject *, PyObject *, long);

	assert(so->lookup != NULL);
	entry = so->lookup(so, key, hash);
	if (entry == NULL)
		return -1;
	if (entry->key == NULL) {
		/* UNUSED */
		so->fill++; 
		entry->key = key;
		entry->hash = hash;
		so->used++;
	} else if (entry->key == dummy) {
		/* DUMMY */
		entry->key = key;
		entry->hash = hash;
		so->used++;
		Py_DECREF(dummy);
	} else {
		/* ACTIVE */
		Py_DECREF(key);
	}
	return 0;
}

/*
Internal routine used by set_table_resize() to insert an item which is
known to be absent from the set.  This routine also assumes that
the set contains no deleted entries.  Besides the performance benefit,
using set_insert_clean() in set_table_resize() is dangerous (SF bug #1456209).
Note that no refcounts are changed by this routine; if needed, the caller
is responsible for incref'ing `key`.
*/
static void
set_insert_clean(register PySetObject *so, PyObject *key, long hash)
{
	register size_t i;
	register size_t perturb;
	register size_t mask = (size_t)so->mask;
	setentry *table = so->table;
	register setentry *entry;

	i = hash & mask;
	entry = &table[i];
	for (perturb = hash; entry->key != NULL; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		entry = &table[i & mask];
	}
	so->fill++;
	entry->key = key;
	entry->hash = hash;
	so->used++;
}

/*
Restructure the table by allocating a new table and reinserting all
keys again.  When entries have been deleted, the new table may
actually be smaller than the old one.
*/
static int
set_table_resize(PySetObject *so, Py_ssize_t minused)
{
	Py_ssize_t newsize;
	setentry *oldtable, *newtable, *entry;
	Py_ssize_t i;
	int is_oldtable_malloced;
	setentry small_copy[PySet_MINSIZE];

	assert(minused >= 0);

	/* Find the smallest table size > minused. */
	for (newsize = PySet_MINSIZE;
	     newsize <= minused && newsize > 0;
	     newsize <<= 1)
		;
	if (newsize <= 0) {
		PyErr_NoMemory();
		return -1;
	}

	/* Get space for a new table. */
	oldtable = so->table;
	assert(oldtable != NULL);
	is_oldtable_malloced = oldtable != so->smalltable;

	if (newsize == PySet_MINSIZE) {
		/* A large table is shrinking, or we can't get any smaller. */
		newtable = so->smalltable;
		if (newtable == oldtable) {
			if (so->fill == so->used) {
				/* No dummies, so no point doing anything. */
				return 0;
			}
			/* We're not going to resize it, but rebuild the
			   table anyway to purge old dummy entries.
			   Subtle:  This is *necessary* if fill==size,
			   as set_lookkey needs at least one virgin slot to
			   terminate failing searches.  If fill < size, it's
			   merely desirable, as dummies slow searches. */
			assert(so->fill > so->used);
			memcpy(small_copy, oldtable, sizeof(small_copy));
			oldtable = small_copy;
		}
	}
	else {
		newtable = PyMem_NEW(setentry, newsize);
		if (newtable == NULL) {
			PyErr_NoMemory();
			return -1;
		}
	}

	/* Make the set empty, using the new table. */
	assert(newtable != oldtable);
	so->table = newtable;
	so->mask = newsize - 1;
	memset(newtable, 0, sizeof(setentry) * newsize);
	so->used = 0;
	i = so->fill;
	so->fill = 0;

	/* Copy the data over; this is refcount-neutral for active entries;
	   dummy entries aren't copied over, of course */
	for (entry = oldtable; i > 0; entry++) {
		if (entry->key == NULL) {
			/* UNUSED */
			;
		} else if (entry->key == dummy) {
			/* DUMMY */
			--i;
			assert(entry->key == dummy);
			Py_DECREF(entry->key);
		} else {
			/* ACTIVE */
			--i;
			set_insert_clean(so, entry->key, entry->hash);
		}
	}

	if (is_oldtable_malloced)
		PyMem_DEL(oldtable);
	return 0;
}

/* CAUTION: set_add_key/entry() must guarantee it won't resize the table */

static int
set_add_entry(register PySetObject *so, setentry *entry)
{
	register Py_ssize_t n_used;

	assert(so->fill <= so->mask);  /* at least one empty slot */
	n_used = so->used;
	Py_INCREF(entry->key);
	if (set_insert_key(so, entry->key, entry->hash) == -1) {
		Py_DECREF(entry->key);
		return -1;
	}
	if (!(so->used > n_used && so->fill*3 >= (so->mask+1)*2))
		return 0;
	return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);
}

static int
set_add_key(register PySetObject *so, PyObject *key)
{
	register long hash;
	register Py_ssize_t n_used;

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1) {
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	assert(so->fill <= so->mask);  /* at least one empty slot */
	n_used = so->used;
	Py_INCREF(key);
	if (set_insert_key(so, key, hash) == -1) {
		Py_DECREF(key);
		return -1;
	}
	if (!(so->used > n_used && so->fill*3 >= (so->mask+1)*2))
		return 0;
	return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);
}

#define DISCARD_NOTFOUND 0
#define DISCARD_FOUND 1

static int
set_discard_entry(PySetObject *so, setentry *oldentry)
{	register setentry *entry;
	PyObject *old_key;

	entry = (so->lookup)(so, oldentry->key, oldentry->hash);
	if (entry == NULL)
		return -1;
	if (entry->key == NULL  ||  entry->key == dummy)
		return DISCARD_NOTFOUND;
	old_key = entry->key;
	Py_INCREF(dummy);
	entry->key = dummy;
	so->used--;
	Py_DECREF(old_key);
	return DISCARD_FOUND;
}

static int
set_discard_key(PySetObject *so, PyObject *key)
{
	register long hash;
	register setentry *entry;
	PyObject *old_key;

	assert (PyAnySet_Check(so));
	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1) {
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	entry = (so->lookup)(so, key, hash);
	if (entry == NULL)
		return -1;
	if (entry->key == NULL  ||  entry->key == dummy)
		return DISCARD_NOTFOUND;
	old_key = entry->key;
	Py_INCREF(dummy);
	entry->key = dummy;
	so->used--;
	Py_DECREF(old_key);
	return DISCARD_FOUND;
}

static int
set_clear_internal(PySetObject *so)
{
	setentry *entry, *table;
	int table_is_malloced;
	Py_ssize_t fill;
	setentry small_copy[PySet_MINSIZE];
#ifdef Py_DEBUG
	Py_ssize_t i, n;
	assert (PyAnySet_Check(so));

	n = so->mask + 1;
	i = 0;
#endif

	table = so->table;
	assert(table != NULL);
	table_is_malloced = table != so->smalltable;

	/* This is delicate.  During the process of clearing the set,
	 * decrefs can cause the set to mutate.  To avoid fatal confusion
	 * (voice of experience), we have to make the set empty before
	 * clearing the slots, and never refer to anything via so->ref while
	 * clearing.
	 */
	fill = so->fill;
	if (table_is_malloced)
		EMPTY_TO_MINSIZE(so);

	else if (fill > 0) {
		/* It's a small table with something that needs to be cleared.
		 * Afraid the only safe way is to copy the set entries into
		 * another small table first.
		 */
		memcpy(small_copy, table, sizeof(small_copy));
		table = small_copy;
		EMPTY_TO_MINSIZE(so);
	}
	/* else it's a small table that's already empty */

	/* Now we can finally clear things.  If C had refcounts, we could
	 * assert that the refcount on table is 1 now, i.e. that this function
	 * has unique access to it, so decref side-effects can't alter it.
	 */
	for (entry = table; fill > 0; ++entry) {
#ifdef Py_DEBUG
		assert(i < n);
		++i;
#endif
		if (entry->key) {
			--fill;
			Py_DECREF(entry->key);
		}
#ifdef Py_DEBUG
		else
			assert(entry->key == NULL);
#endif
	}

	if (table_is_malloced)
		PyMem_DEL(table);
	return 0;
}

/*
 * Iterate over a set table.  Use like so:
 *
 *     Py_ssize_t pos;
 *     setentry *entry;
 *     pos = 0;   # important!  pos should not otherwise be changed by you
 *     while (set_next(yourset, &pos, &entry)) {
 *              Refer to borrowed reference in entry->key.
 *     }
 *
 * CAUTION:  In general, it isn't safe to use set_next in a loop that
 * mutates the table.  
 */
static int
set_next(PySetObject *so, Py_ssize_t *pos_ptr, setentry **entry_ptr)
{
	Py_ssize_t i;
	Py_ssize_t mask;
	register setentry *table;

	assert (PyAnySet_Check(so));
	i = *pos_ptr;
	assert(i >= 0);
	table = so->table;
	mask = so->mask;
	while (i <= mask && (table[i].key == NULL || table[i].key == dummy))
		i++;
	*pos_ptr = i+1;
	if (i > mask)
		return 0;
	assert(table[i].key != NULL);
	*entry_ptr = &table[i];
	return 1;
}

static void
set_dealloc(PySetObject *so)
{
	register setentry *entry;
	Py_ssize_t fill = so->fill;
	PyObject_GC_UnTrack(so);
	Py_TRASHCAN_SAFE_BEGIN(so)
	if (so->weakreflist != NULL)
		PyObject_ClearWeakRefs((PyObject *) so);

	for (entry = so->table; fill > 0; entry++) {
		if (entry->key) {
			--fill;
			Py_DECREF(entry->key);
		}
	}
	if (so->table != so->smalltable)
		PyMem_DEL(so->table);
	if (num_free_sets < MAXFREESETS && PyAnySet_CheckExact(so))
		free_sets[num_free_sets++] = so;
	else 
		Py_TYPE(so)->tp_free(so);
	Py_TRASHCAN_SAFE_END(so)
}

static int
set_tp_print(PySetObject *so, FILE *fp, int flags)
{
	setentry *entry;
	Py_ssize_t pos=0;
	char *emit = "";	/* No separator emitted on first pass */
	char *separator = ", ";
	int status = Py_ReprEnter((PyObject*)so);

	if (status != 0) {
		if (status < 0)
			return status;
		Py_BEGIN_ALLOW_THREADS
		fprintf(fp, "%s(...)", so->ob_type->tp_name);
		Py_END_ALLOW_THREADS
		return 0;
	}        

	Py_BEGIN_ALLOW_THREADS
	fprintf(fp, "%s([", so->ob_type->tp_name);
	Py_END_ALLOW_THREADS
	while (set_next(so, &pos, &entry)) {
		Py_BEGIN_ALLOW_THREADS
		fputs(emit, fp);
		Py_END_ALLOW_THREADS
		emit = separator;
		if (PyObject_Print(entry->key, fp, 0) != 0) {
			Py_ReprLeave((PyObject*)so);
			return -1;
		}
	}
	Py_BEGIN_ALLOW_THREADS
	fputs("])", fp);
	Py_END_ALLOW_THREADS
	Py_ReprLeave((PyObject*)so);        
	return 0;
}

static PyObject *
set_repr(PySetObject *so)
{
	PyObject *keys, *result=NULL, *listrepr;
	int status = Py_ReprEnter((PyObject*)so);

	if (status != 0) {
		if (status < 0)
			return NULL;
		return PyString_FromFormat("%s(...)", so->ob_type->tp_name);
	}

	keys = PySequence_List((PyObject *)so);
	if (keys == NULL)
		goto done;
	listrepr = PyObject_Repr(keys);
	Py_DECREF(keys);
	if (listrepr == NULL)
		goto done;

	result = PyString_FromFormat("%s(%s)", so->ob_type->tp_name,
		PyString_AS_STRING(listrepr));
	Py_DECREF(listrepr);
done:
	Py_ReprLeave((PyObject*)so);
	return result;
}

static Py_ssize_t
set_len(PyObject *so)
{
	return ((PySetObject *)so)->used;
}

static int
set_merge(PySetObject *so, PyObject *otherset)
{
	PySetObject *other;
	register Py_ssize_t i;
	register setentry *entry;

	assert (PyAnySet_Check(so));
	assert (PyAnySet_Check(otherset));

	other = (PySetObject*)otherset;
	if (other == so || other->used == 0)
		/* a.update(a) or a.update({}); nothing to do */
		return 0;
	/* Do one big resize at the start, rather than
	 * incrementally resizing as we insert new keys.  Expect
	 * that there will be no (or few) overlapping keys.
	 */
	if ((so->fill + other->used)*3 >= (so->mask+1)*2) {
	   if (set_table_resize(so, (so->used + other->used)*2) != 0)
		   return -1;
	}
	for (i = 0; i <= other->mask; i++) {
		entry = &other->table[i];
		if (entry->key != NULL && 
		    entry->key != dummy) {
			Py_INCREF(entry->key);
			if (set_insert_key(so, entry->key, entry->hash) == -1) {
				Py_DECREF(entry->key);
				return -1;
			}
		}
	}
	return 0;
}

static int
set_contains_key(PySetObject *so, PyObject *key)
{
	long hash;
	setentry *entry;

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1) {
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	entry = (so->lookup)(so, key, hash);
	if (entry == NULL)
		return -1;
	key = entry->key;
	return key != NULL && key != dummy;
}

static int
set_contains_entry(PySetObject *so, setentry *entry)
{
	PyObject *key;
	setentry *lu_entry;

	lu_entry = (so->lookup)(so, entry->key, entry->hash);
	if (lu_entry == NULL)
		return -1;
	key = lu_entry->key; 
	return key != NULL && key != dummy;
}

static PyObject *
set_pop(PySetObject *so)
{
	register Py_ssize_t i = 0;
	register setentry *entry;
	PyObject *key;

	assert (PyAnySet_Check(so));
	if (so->used == 0) {
		PyErr_SetString(PyExc_KeyError, "pop from an empty set");
		return NULL;
	}

	/* Set entry to "the first" unused or dummy set entry.  We abuse
	 * the hash field of slot 0 to hold a search finger:
	 * If slot 0 has a value, use slot 0.
	 * Else slot 0 is being used to hold a search finger,
	 * and we use its hash value as the first index to look.
	 */
	entry = &so->table[0];
	if (entry->key == NULL || entry->key == dummy) {
		i = entry->hash;
		/* The hash field may be a real hash value, or it may be a
		 * legit search finger, or it may be a once-legit search
		 * finger that's out of bounds now because it wrapped around
		 * or the table shrunk -- simply make sure it's in bounds now.
		 */
		if (i > so->mask || i < 1)
			i = 1;	/* skip slot 0 */
		while ((entry = &so->table[i])->key == NULL || entry->key==dummy) {
			i++;
			if (i > so->mask)
				i = 1;
		}
	}
	key = entry->key;
	Py_INCREF(dummy);
	entry->key = dummy;
	so->used--;
	so->table[0].hash = i + 1;  /* next place to start */
	return key;
}

PyDoc_STRVAR(pop_doc, "Remove and return an arbitrary set element.");

static int
set_traverse(PySetObject *so, visitproc visit, void *arg)
{
	Py_ssize_t pos = 0;
	setentry *entry;

	while (set_next(so, &pos, &entry))
		Py_VISIT(entry->key);
	return 0;
}

static long
frozenset_hash(PyObject *self)
{
	PySetObject *so = (PySetObject *)self;
	long h, hash = 1927868237L;
	setentry *entry;
	Py_ssize_t pos = 0;

	if (so->hash != -1)
		return so->hash;

	hash *= PySet_GET_SIZE(self) + 1;
	while (set_next(so, &pos, &entry)) {
		/* Work to increase the bit dispersion for closely spaced hash
		   values.  The is important because some use cases have many 
		   combinations of a small number of elements with nearby 
		   hashes so that many distinct combinations collapse to only 
		   a handful of distinct hash values. */
		h = entry->hash;
		hash ^= (h ^ (h << 16) ^ 89869747L)  * 3644798167u;
	}
	hash = hash * 69069L + 907133923L;
	if (hash == -1)
		hash = 590923713L;
	so->hash = hash;
	return hash;
}

/***** Set iterator type ***********************************************/

typedef struct {
	PyObject_HEAD
	PySetObject *si_set; /* Set to NULL when iterator is exhausted */
	Py_ssize_t si_used;
	Py_ssize_t si_pos;
	Py_ssize_t len;
} setiterobject;

static void
setiter_dealloc(setiterobject *si)
{
	Py_XDECREF(si->si_set);
	PyObject_Del(si);
}

static PyObject *
setiter_len(setiterobject *si)
{
	Py_ssize_t len = 0;
	if (si->si_set != NULL && si->si_used == si->si_set->used)
		len = si->len;
	return PyInt_FromLong(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef setiter_methods[] = {
	{"__length_hint__", (PyCFunction)setiter_len, METH_NOARGS, length_hint_doc},
 	{NULL,		NULL}		/* sentinel */
};

static PyObject *setiter_iternext(setiterobject *si)
{
	PyObject *key;
	register Py_ssize_t i, mask;
	register setentry *entry;
	PySetObject *so = si->si_set;

	if (so == NULL)
		return NULL;
	assert (PyAnySet_Check(so));

	if (si->si_used != so->used) {
		PyErr_SetString(PyExc_RuntimeError,
				"Set changed size during iteration");
		si->si_used = -1; /* Make this state sticky */
		return NULL;
	}

	i = si->si_pos;
	assert(i>=0);
	entry = so->table;
	mask = so->mask;
	while (i <= mask && (entry[i].key == NULL || entry[i].key == dummy))
		i++;
	si->si_pos = i+1;
	if (i > mask)
		goto fail;
	si->len--;
	key = entry[i].key;
	Py_INCREF(key);
	return key;

fail:
	Py_DECREF(so);
	si->si_set = NULL;
	return NULL;
}

static PyTypeObject PySetIter_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"setiterator",				/* tp_name */
	sizeof(setiterobject),			/* tp_basicsize */
	0,					/* tp_itemsize */
	/* methods */
	(destructor)setiter_dealloc, 		/* tp_dealloc */
	0,					/* tp_print */
	0,					/* tp_getattr */
	0,					/* tp_setattr */
	0,					/* tp_compare */
	0,					/* tp_repr */
	0,					/* tp_as_number */
	0,					/* tp_as_sequence */
	0,					/* tp_as_mapping */
	0,					/* tp_hash */
	0,					/* tp_call */
	0,					/* tp_str */
	PyObject_GenericGetAttr,		/* tp_getattro */
	0,					/* tp_setattro */
	0,					/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,			/* tp_flags */
 	0,					/* tp_doc */
 	0,					/* tp_traverse */
 	0,					/* tp_clear */
	0,					/* tp_richcompare */
	0,					/* tp_weaklistoffset */
	PyObject_SelfIter,			/* tp_iter */
	(iternextfunc)setiter_iternext,		/* tp_iternext */
	setiter_methods,			/* tp_methods */
	0,
};

static PyObject *
set_iter(PySetObject *so)
{
	setiterobject *si = PyObject_New(setiterobject, &PySetIter_Type);
	if (si == NULL)
		return NULL;
	Py_INCREF(so);
	si->si_set = so;
	si->si_used = so->used;
	si->si_pos = 0;
	si->len = so->used;
	return (PyObject *)si;
}

static int
set_update_internal(PySetObject *so, PyObject *other)
{
	PyObject *key, *it;

	if (PyAnySet_CheckExact(other))
		return set_merge(so, other);

	if (PyDict_CheckExact(other)) {
		PyObject *value;
		Py_ssize_t pos = 0;
		long hash;
		Py_ssize_t dictsize = PyDict_Size(other);

		/* Do one big resize at the start, rather than
		* incrementally resizing as we insert new keys.  Expect
		* that there will be no (or few) overlapping keys.
		*/
		if (dictsize == -1)
			return -1;
		if ((so->fill + dictsize)*3 >= (so->mask+1)*2) {
			if (set_table_resize(so, (so->used + dictsize)*2) != 0)
				return -1;
		}
		while (_PyDict_Next(other, &pos, &key, &value, &hash)) {
			setentry an_entry;

			an_entry.hash = hash;
			an_entry.key = key;
			if (set_add_entry(so, &an_entry) == -1)
				return -1;
		}
		return 0;
	}

	it = PyObject_GetIter(other);
	if (it == NULL)
		return -1;

	while ((key = PyIter_Next(it)) != NULL) {
                if (set_add_key(so, key) == -1) {
			Py_DECREF(it);
			Py_DECREF(key);
			return -1;
                } 
		Py_DECREF(key);
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return -1;
	return 0;
}

static PyObject *
set_update(PySetObject *so, PyObject *other)
{
	if (set_update_internal(so, other) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc, 
"Update a set with the union of itself and another.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
	register PySetObject *so = NULL;

	if (dummy == NULL) { /* Auto-initialize dummy */
		dummy = PyString_FromString("<dummy key>");
		if (dummy == NULL)
			return NULL;
	}

	/* create PySetObject structure */
	if (num_free_sets && 
	    (type == &PySet_Type  ||  type == &PyFrozenSet_Type)) {
		so = free_sets[--num_free_sets];
		assert (so != NULL && PyAnySet_CheckExact(so));
		Py_TYPE(so) = type;
		_Py_NewReference((PyObject *)so);
		EMPTY_TO_MINSIZE(so);
		PyObject_GC_Track(so);
	} else {
		so = (PySetObject *)type->tp_alloc(type, 0);
		if (so == NULL)
			return NULL;
		/* tp_alloc has already zeroed the structure */
		assert(so->table == NULL && so->fill == 0 && so->used == 0);
		INIT_NONZERO_SET_SLOTS(so);
	}

	so->lookup = set_lookkey_string;
	so->weakreflist = NULL;

	if (iterable != NULL) {
		if (set_update_internal(so, iterable) == -1) {
			Py_DECREF(so);
			return NULL;
		}
	}

	return (PyObject *)so;
}

/* The empty frozenset is a singleton */
static PyObject *emptyfrozenset = NULL;

static PyObject *
frozenset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL, *result;

	if (type == &PyFrozenSet_Type && !_PyArg_NoKeywords("frozenset()", kwds))
		return NULL;

	if (!PyArg_UnpackTuple(args, type->tp_name, 0, 1, &iterable))
		return NULL;

	if (type != &PyFrozenSet_Type)
		return make_new_set(type, iterable);

	if (iterable != NULL) {
		/* frozenset(f) is idempotent */
		if (PyFrozenSet_CheckExact(iterable)) {
			Py_INCREF(iterable);
			return iterable;
		}
		result = make_new_set(type, iterable);
		if (result == NULL || PySet_GET_SIZE(result))
			return result;
		Py_DECREF(result);
	}
	/* The empty frozenset is a singleton */
	if (emptyfrozenset == NULL)
		emptyfrozenset = make_new_set(type, NULL);
	Py_XINCREF(emptyfrozenset);
	return emptyfrozenset;
}

void
PySet_Fini(void)
{
	PySetObject *so;

	while (num_free_sets) {
		num_free_sets--;
		so = free_sets[num_free_sets];
		PyObject_GC_Del(so);
	}
	Py_CLEAR(dummy);
	Py_CLEAR(emptyfrozenset);
}

static PyObject *
set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	if (type == &PySet_Type && !_PyArg_NoKeywords("set()", kwds))
		return NULL;
	
	return make_new_set(type, NULL);
}

/* set_swap_bodies() switches the contents of any two sets by moving their
   internal data pointers and, if needed, copying the internal smalltables.
   Semantically equivalent to:

     t=set(a); a.clear(); a.update(b); b.clear(); b.update(t); del t

   The function always succeeds and it leaves both objects in a stable state.
   Useful for creating temporary frozensets from sets for membership testing 
   in __contains__(), discard(), and remove().  Also useful for operations
   that update in-place (by allowing an intermediate result to be swapped 
   into one of the original inputs).
*/

static void
set_swap_bodies(PySetObject *a, PySetObject *b)
{
	Py_ssize_t t;
	setentry *u;
	setentry *(*f)(PySetObject *so, PyObject *key, long hash);
	setentry tab[PySet_MINSIZE];
	long h;

	t = a->fill;     a->fill   = b->fill;        b->fill  = t;
	t = a->used;     a->used   = b->used;        b->used  = t;
	t = a->mask;     a->mask   = b->mask;        b->mask  = t;

	u = a->table;
	if (a->table == a->smalltable)
		u = b->smalltable;
	a->table  = b->table;
	if (b->table == b->smalltable)
		a->table = a->smalltable;
	b->table = u;

	f = a->lookup;   a->lookup = b->lookup;      b->lookup = f;

	if (a->table == a->smalltable || b->table == b->smalltable) {
		memcpy(tab, a->smalltable, sizeof(tab));
		memcpy(a->smalltable, b->smalltable, sizeof(tab));
		memcpy(b->smalltable, tab, sizeof(tab));
	}

	if (PyType_IsSubtype(Py_TYPE(a), &PyFrozenSet_Type)  &&
	    PyType_IsSubtype(Py_TYPE(b), &PyFrozenSet_Type)) {
		h = a->hash;     a->hash = b->hash;  b->hash = h;
	} else {
		a->hash = -1;
		b->hash = -1;
	}
}

static PyObject *
set_copy(PySetObject *so)
{
	return make_new_set(Py_TYPE(so), (PyObject *)so);
}

static PyObject *
frozenset_copy(PySetObject *so)
{
	if (PyFrozenSet_CheckExact(so)) {
		Py_INCREF(so);
		return (PyObject *)so;
	}
	return set_copy(so);
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a set.");

static PyObject *
set_clear(PySetObject *so)
{
	set_clear_internal(so);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from this set.");

static PyObject *
set_union(PySetObject *so, PyObject *other)
{
	PySetObject *result;

	result = (PySetObject *)set_copy(so);
	if (result == NULL)
		return NULL;
	if ((PyObject *)so == other)
		return (PyObject *)result;
	if (set_update_internal(result, other) == -1) {
		Py_DECREF(result);
		return NULL;
	}
	return (PyObject *)result;
}

PyDoc_STRVAR(union_doc,
 "Return the union of two sets as a new set.\n\
\n\
(i.e. all elements that are in either set.)");

static PyObject *
set_or(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_union(so, other);
}

static PyObject *
set_ior(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	if (set_update_internal(so, other) == -1)
		return NULL;
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_intersection(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *key, *it, *tmp;

	if ((PyObject *)so == other)
		return set_copy(so);

	result = (PySetObject *)make_new_set(Py_TYPE(so), NULL);
	if (result == NULL)
		return NULL;

	if (PyAnySet_CheckExact(other)) {		
		Py_ssize_t pos = 0;
		setentry *entry;

		if (PySet_GET_SIZE(other) > PySet_GET_SIZE(so)) {
			tmp = (PyObject *)so;
			so = (PySetObject *)other;
			other = tmp;
		}

		while (set_next((PySetObject *)other, &pos, &entry)) {
			int rv = set_contains_entry(so, entry);
			if (rv == -1) {
				Py_DECREF(result);
				return NULL;
			}
			if (rv) {
				if (set_add_entry(result, entry) == -1) {
					Py_DECREF(result);
					return NULL;
				}
			}
		}
		return (PyObject *)result;
	}

	it = PyObject_GetIter(other);
	if (it == NULL) {
		Py_DECREF(result);
		return NULL;
	}

	while ((key = PyIter_Next(it)) != NULL) {
		int rv;
		setentry entry;
		long hash = PyObject_Hash(key);

		if (hash == -1) {
			Py_DECREF(it);
			Py_DECREF(result);
			Py_DECREF(key);
			return NULL;
		}
		entry.hash = hash;
		entry.key = key;
		rv = set_contains_entry(so, &entry);
		if (rv == -1) {
			Py_DECREF(it);
			Py_DECREF(result);
			Py_DECREF(key);
			return NULL;
		}
		if (rv) {
			if (set_add_entry(result, &entry) == -1) {
				Py_DECREF(it);
				Py_DECREF(result);
				Py_DECREF(key);
				return NULL;
			}
		}
		Py_DECREF(key);
	}
	Py_DECREF(it);
	if (PyErr_Occurred()) {
		Py_DECREF(result);
		return NULL;
	}
	return (PyObject *)result;
}

PyDoc_STRVAR(intersection_doc,
"Return the intersection of two sets as a new set.\n\
\n\
(i.e. all elements that are in both sets.)");

static PyObject *
set_intersection_update(PySetObject *so, PyObject *other)
{
	PyObject *tmp;

	tmp = set_intersection(so, other);
	if (tmp == NULL)
		return NULL;
	set_swap_bodies(so, (PySetObject *)tmp);
	Py_DECREF(tmp);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(intersection_update_doc,
"Update a set with the intersection of itself and another.");

static PyObject *
set_and(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_intersection(so, other);
}

static PyObject *
set_iand(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_intersection_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_isdisjoint(PySetObject *so, PyObject *other)
{
	PyObject *key, *it, *tmp;

	if ((PyObject *)so == other) {
		if (PySet_GET_SIZE(so) == 0)
			Py_RETURN_TRUE;
		else
			Py_RETURN_FALSE;
	}

	if (PyAnySet_CheckExact(other)) {		
		Py_ssize_t pos = 0;
		setentry *entry;

		if (PySet_GET_SIZE(other) > PySet_GET_SIZE(so)) {
			tmp = (PyObject *)so;
			so = (PySetObject *)other;
			other = tmp;
		}
		while (set_next((PySetObject *)other, &pos, &entry)) {
			int rv = set_contains_entry(so, entry);
			if (rv == -1)
				return NULL;
			if (rv)
				Py_RETURN_FALSE;
		}
		Py_RETURN_TRUE;
	}

	it = PyObject_GetIter(other);
	if (it == NULL)
		return NULL;

	while ((key = PyIter_Next(it)) != NULL) {
		int rv;
		setentry entry;
		long hash = PyObject_Hash(key);

		if (hash == -1) {
			Py_DECREF(key);
			Py_DECREF(it);
			return NULL;
		}
		entry.hash = hash;
		entry.key = key;
		rv = set_contains_entry(so, &entry);
		Py_DECREF(key);
		if (rv == -1) {
			Py_DECREF(it);
			return NULL;
		}
		if (rv) {
			Py_DECREF(it);
			Py_RETURN_FALSE;
		}
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(isdisjoint_doc,
"Return True if two sets have a null intersection.");

static int
set_difference_update_internal(PySetObject *so, PyObject *other)
{
	if ((PyObject *)so == other)
		return set_clear_internal(so);
	
	if (PyAnySet_CheckExact(other)) {
		setentry *entry;
		Py_ssize_t pos = 0;

		while (set_next((PySetObject *)other, &pos, &entry))
			if (set_discard_entry(so, entry) == -1)
				return -1;
	} else {
		PyObject *key, *it;
		it = PyObject_GetIter(other);
		if (it == NULL)
			return -1;

		while ((key = PyIter_Next(it)) != NULL) {
			if (set_discard_key(so, key) == -1) {
				Py_DECREF(it);
				Py_DECREF(key);
				return -1;
			}
			Py_DECREF(key);
		}
		Py_DECREF(it);
		if (PyErr_Occurred())
			return -1;
	}
	/* If more than 1/5 are dummies, then resize them away. */
	if ((so->fill - so->used) * 5 < so->mask)
		return 0;
	return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);
}

static PyObject *
set_difference_update(PySetObject *so, PyObject *other)
{
	if (set_difference_update_internal(so, other) != -1)
		Py_RETURN_NONE;
	return NULL;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another set from this set.");

static PyObject *
set_difference(PySetObject *so, PyObject *other)
{
	PyObject *result;
	setentry *entry;
	Py_ssize_t pos = 0;

	if (!PyAnySet_CheckExact(other)  && !PyDict_CheckExact(other)) {
		result = set_copy(so);
		if (result == NULL)
			return NULL;
		if (set_difference_update_internal((PySetObject *)result, other) != -1)
			return result;
		Py_DECREF(result);
		return NULL;
	}
	
	result = make_new_set(Py_TYPE(so), NULL);
	if (result == NULL)
		return NULL;

	if (PyDict_CheckExact(other)) {
		while (set_next(so, &pos, &entry)) {
			setentry entrycopy;
			entrycopy.hash = entry->hash;
			entrycopy.key = entry->key;
			if (!_PyDict_Contains(other, entry->key, entry->hash)) {
				if (set_add_entry((PySetObject *)result, &entrycopy) == -1) {
					Py_DECREF(result);
					return NULL;
				}
			}
		}
		return result;
	}

	while (set_next(so, &pos, &entry)) {
		int rv = set_contains_entry((PySetObject *)other, entry);
		if (rv == -1) {
			Py_DECREF(result);
			return NULL;
		}
		if (!rv) {
			if (set_add_entry((PySetObject *)result, entry) == -1) {
				Py_DECREF(result);
				return NULL;
			}
		}
	}
	return result;
}

PyDoc_STRVAR(difference_doc,
"Return the difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in this set but not the other.)");
static PyObject *
set_sub(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_difference(so, other);
}

static PyObject *
set_isub(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_difference_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_symmetric_difference_update(PySetObject *so, PyObject *other)
{
	PySetObject *otherset;
	PyObject *key;
	Py_ssize_t pos = 0;
	setentry *entry;

	if ((PyObject *)so == other)
		return set_clear(so);

	if (PyDict_CheckExact(other)) {
		PyObject *value;
		int rv;
		long hash;
		while (_PyDict_Next(other, &pos, &key, &value, &hash)) {
			setentry an_entry;

			an_entry.hash = hash;
			an_entry.key = key;
			rv = set_discard_entry(so, &an_entry);
			if (rv == -1)
				return NULL;
			if (rv == DISCARD_NOTFOUND) {
				if (set_add_entry(so, &an_entry) == -1)
					return NULL;
			}
		}
		Py_RETURN_NONE;
	}

	if (PyAnySet_CheckExact(other)) {
		Py_INCREF(other);
		otherset = (PySetObject *)other;
	} else {
		otherset = (PySetObject *)make_new_set(Py_TYPE(so), other);
		if (otherset == NULL)
			return NULL;
	}

	while (set_next(otherset, &pos, &entry)) {
		int rv = set_discard_entry(so, entry);
		if (rv == -1) {
			Py_DECREF(otherset);
			return NULL;
		}
		if (rv == DISCARD_NOTFOUND) {
			if (set_add_entry(so, entry) == -1) {
				Py_DECREF(otherset);
				return NULL;
			}
		}
	}
	Py_DECREF(otherset);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(symmetric_difference_update_doc,
"Update a set with the symmetric difference of itself and another.");

static PyObject *
set_symmetric_difference(PySetObject *so, PyObject *other)
{
	PyObject *rv;
	PySetObject *otherset;

	otherset = (PySetObject *)make_new_set(Py_TYPE(so), other);
	if (otherset == NULL)
		return NULL;
	rv = set_symmetric_difference_update(otherset, (PyObject *)so);
	if (rv == NULL)
		return NULL;
	Py_DECREF(rv);
	return (PyObject *)otherset;
}

PyDoc_STRVAR(symmetric_difference_doc,
"Return the symmetric difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in exactly one of the sets.)");

static PyObject *
set_xor(PySetObject *so, PyObject *other)
{
	if (!PyAnySet_Check(so) || !PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	return set_symmetric_difference(so, other);
}

static PyObject *
set_ixor(PySetObject *so, PyObject *other)
{
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_symmetric_difference_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_issubset(PySetObject *so, PyObject *other)
{
	setentry *entry;
	Py_ssize_t pos = 0;

	if (!PyAnySet_CheckExact(other)) {
		PyObject *tmp, *result;
		tmp = make_new_set(&PySet_Type, other);
		if (tmp == NULL)
			return NULL;
		result = set_issubset(so, tmp);
		Py_DECREF(tmp);
		return result;
	}
	if (PySet_GET_SIZE(so) > PySet_GET_SIZE(other)) 
		Py_RETURN_FALSE;

	while (set_next(so, &pos, &entry)) {
		int rv = set_contains_entry((PySetObject *)other, entry);
		if (rv == -1)
			return NULL;
		if (!rv)
			Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another set contains this set.");

static PyObject *
set_issuperset(PySetObject *so, PyObject *other)
{
	PyObject *tmp, *result;

	if (!PyAnySet_CheckExact(other)) {
		tmp = make_new_set(&PySet_Type, other);
		if (tmp == NULL)
			return NULL;
		result = set_issuperset(so, tmp);
		Py_DECREF(tmp);
		return result;
	}
	return set_issubset((PySetObject *)other, (PyObject *)so);
}

PyDoc_STRVAR(issuperset_doc, "Report whether this set contains another set.");

static PyObject *
set_richcompare(PySetObject *v, PyObject *w, int op)
{
	PyObject *r1, *r2;

	if(!PyAnySet_Check(w)) {
		if (op == Py_EQ)
			Py_RETURN_FALSE;
		if (op == Py_NE)
			Py_RETURN_TRUE;
		PyErr_SetString(PyExc_TypeError, "can only compare to a set");
		return NULL;
	}
	switch (op) {
	case Py_EQ:
		if (PySet_GET_SIZE(v) != PySet_GET_SIZE(w))
			Py_RETURN_FALSE;
		if (v->hash != -1  &&
		    ((PySetObject *)w)->hash != -1 &&
		    v->hash != ((PySetObject *)w)->hash)
			Py_RETURN_FALSE;
		return set_issubset(v, w);
	case Py_NE:
		r1 = set_richcompare(v, w, Py_EQ);
		if (r1 == NULL)
			return NULL;
		r2 = PyBool_FromLong(PyObject_Not(r1));
		Py_DECREF(r1);
		return r2;
	case Py_LE:
		return set_issubset(v, w);
	case Py_GE:
		return set_issuperset(v, w);
	case Py_LT:
		if (PySet_GET_SIZE(v) >= PySet_GET_SIZE(w))
			Py_RETURN_FALSE;		
		return set_issubset(v, w);
	case Py_GT:
		if (PySet_GET_SIZE(v) <= PySet_GET_SIZE(w))
			Py_RETURN_FALSE;
		return set_issuperset(v, w);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static int
set_nocmp(PyObject *self, PyObject *other)
{
	PyErr_SetString(PyExc_TypeError, "cannot compare sets using cmp()");
	return -1;
}

static PyObject *
set_add(PySetObject *so, PyObject *key)
{
	if (set_add_key(so, key) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc, 
"Add an element to a set.\n\
\n\
This has no effect if the element is already present.");

static int
set_contains(PySetObject *so, PyObject *key)
{
	PyObject *tmpkey;
	int rv;

	rv = set_contains_key(so, key);
	if (rv == -1) {
		if (!PyAnySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
			return -1;
		PyErr_Clear();
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return -1;
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		rv = set_contains(so, tmpkey);
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		Py_DECREF(tmpkey);
	}
	return rv;
}

static PyObject *
set_direct_contains(PySetObject *so, PyObject *key)
{
	long result;

	result = set_contains(so, key);
	if (result == -1)
		return NULL;
	return PyBool_FromLong(result);
}

PyDoc_STRVAR(contains_doc, "x.__contains__(y) <==> y in x.");

static PyObject *
set_remove(PySetObject *so, PyObject *key)
{
	PyObject *tmpkey, *result;
	int rv;

	rv = set_discard_key(so, key);
	if (rv == -1) {
		if (!PyAnySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
			return NULL;
		PyErr_Clear();
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return NULL;
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		result = set_remove(so, tmpkey);
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		Py_DECREF(tmpkey);
		return result;
	} else if (rv == DISCARD_NOTFOUND) {
		set_key_error(key);
		return NULL;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_doc,
"Remove an element from a set; it must be a member.\n\
\n\
If the element is not a member, raise a KeyError.");

static PyObject *
set_discard(PySetObject *so, PyObject *key)
{
	PyObject *tmpkey, *result;
	int rv;

	rv = set_discard_key(so, key);
	if (rv == -1) {
		if (!PyAnySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
			return NULL;
		PyErr_Clear();
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return NULL;
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		result = set_discard(so, tmpkey);
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		Py_DECREF(tmpkey);
		return result;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a set if it is a member.\n\
\n\
If the element is not a member, do nothing."); 

static PyObject *
set_reduce(PySetObject *so)
{
	PyObject *keys=NULL, *args=NULL, *result=NULL, *dict=NULL;

	keys = PySequence_List((PyObject *)so);
	if (keys == NULL)
		goto done;
	args = PyTuple_Pack(1, keys);
	if (args == NULL)
		goto done;
	dict = PyObject_GetAttrString((PyObject *)so, "__dict__");
	if (dict == NULL) {
		PyErr_Clear();
		dict = Py_None;
		Py_INCREF(dict);
	}
	result = PyTuple_Pack(3, Py_TYPE(so), args, dict);
done:
	Py_XDECREF(args);
	Py_XDECREF(keys);
	Py_XDECREF(dict);
	return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static int
set_init(PySetObject *self, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL;

	if (!PyAnySet_Check(self))
		return -1;
	if (!PyArg_UnpackTuple(args, Py_TYPE(self)->tp_name, 0, 1, &iterable))
		return -1;
	set_clear_internal(self);
	self->hash = -1;
	if (iterable == NULL)
		return 0;
	return set_update_internal(self, iterable);
}

static PySequenceMethods set_as_sequence = {
	set_len,			/* sq_length */
	0,				/* sq_concat */
	0,				/* sq_repeat */
	0,				/* sq_item */
	0,				/* sq_slice */
	0,				/* sq_ass_item */
	0,				/* sq_ass_slice */
	(objobjproc)set_contains,	/* sq_contains */
};

/* set object ********************************************************/

#ifdef Py_DEBUG
static PyObject *test_c_api(PySetObject *so);

PyDoc_STRVAR(test_c_api_doc, "Exercises C API.  Returns True.\n\
All is well if assertions don't fail.");
#endif

static PyMethodDef set_methods[] = {
	{"add",		(PyCFunction)set_add,		METH_O,
	 add_doc},
	{"clear",	(PyCFunction)set_clear,		METH_NOARGS,
	 clear_doc},
	{"__contains__",(PyCFunction)set_direct_contains,	METH_O | METH_COEXIST,
	 contains_doc},
	{"copy",	(PyCFunction)set_copy,		METH_NOARGS,
	 copy_doc},
	{"discard",	(PyCFunction)set_discard,	METH_O,
	 discard_doc},
	{"difference",	(PyCFunction)set_difference,	METH_O,
	 difference_doc},
	{"difference_update",	(PyCFunction)set_difference_update,	METH_O,
	 difference_update_doc},
	{"intersection",(PyCFunction)set_intersection,	METH_O,
	 intersection_doc},
	{"intersection_update",(PyCFunction)set_intersection_update,	METH_O,
	 intersection_update_doc},
	{"isdisjoint",	(PyCFunction)set_isdisjoint,	METH_O,
	 isdisjoint_doc},
	{"issubset",	(PyCFunction)set_issubset,	METH_O,
	 issubset_doc},
	{"issuperset",	(PyCFunction)set_issuperset,	METH_O,
	 issuperset_doc},
	{"pop",		(PyCFunction)set_pop,		METH_NOARGS,
	 pop_doc},
	{"__reduce__",	(PyCFunction)set_reduce,	METH_NOARGS,
	 reduce_doc},
	{"remove",	(PyCFunction)set_remove,	METH_O,
	 remove_doc},
	{"symmetric_difference",(PyCFunction)set_symmetric_difference,	METH_O,
	 symmetric_difference_doc},
	{"symmetric_difference_update",(PyCFunction)set_symmetric_difference_update,	METH_O,
	 symmetric_difference_update_doc},
#ifdef Py_DEBUG
	{"test_c_api",	(PyCFunction)test_c_api,	METH_NOARGS,
	 test_c_api_doc},
#endif
	{"union",	(PyCFunction)set_union,		METH_O,
	 union_doc},
	{"update",	(PyCFunction)set_update,	METH_O,
	 update_doc},
	{NULL,		NULL}	/* sentinel */
};

static PyNumberMethods set_as_number = {
	0,				/*nb_add*/
	(binaryfunc)set_sub,		/*nb_subtract*/
	0,				/*nb_multiply*/
	0,				/*nb_divide*/
	0,				/*nb_remainder*/
	0,				/*nb_divmod*/
	0,				/*nb_power*/
	0,				/*nb_negative*/
	0,				/*nb_positive*/
	0,				/*nb_absolute*/
	0,				/*nb_nonzero*/
	0,				/*nb_invert*/
	0,				/*nb_lshift*/
	0,				/*nb_rshift*/
	(binaryfunc)set_and,		/*nb_and*/
	(binaryfunc)set_xor,		/*nb_xor*/
	(binaryfunc)set_or,		/*nb_or*/
	0,				/*nb_coerce*/
	0,				/*nb_int*/
	0,				/*nb_long*/
	0,				/*nb_float*/
	0,				/*nb_oct*/
	0, 				/*nb_hex*/
	0,				/*nb_inplace_add*/
	(binaryfunc)set_isub,		/*nb_inplace_subtract*/
	0,				/*nb_inplace_multiply*/
	0,				/*nb_inplace_divide*/
	0,				/*nb_inplace_remainder*/
	0,				/*nb_inplace_power*/
	0,				/*nb_inplace_lshift*/
	0,				/*nb_inplace_rshift*/
	(binaryfunc)set_iand,		/*nb_inplace_and*/
	(binaryfunc)set_ixor,		/*nb_inplace_xor*/
	(binaryfunc)set_ior,		/*nb_inplace_or*/
};

PyDoc_STRVAR(set_doc,
"set(iterable) --> set object\n\
\n\
Build an unordered collection of unique elements.");

PyTypeObject PySet_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"set",				/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	set_nocmp,			/* tp_compare */
	(reprfunc)set_repr,		/* tp_repr */
	&set_as_number,			/* tp_as_number */
	&set_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	set_doc,			/* tp_doc */
	(traverseproc)set_traverse,	/* tp_traverse */
	(inquiry)set_clear_internal,	/* tp_clear */
	(richcmpfunc)set_richcompare,	/* tp_richcompare */
	offsetof(PySetObject, weakreflist),	/* tp_weaklistoffset */
	(getiterfunc)set_iter,	/* tp_iter */
	0,				/* tp_iternext */
	set_methods,			/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	(initproc)set_init,		/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	set_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};

/* frozenset object ********************************************************/


static PyMethodDef frozenset_methods[] = {
	{"__contains__",(PyCFunction)set_direct_contains,	METH_O | METH_COEXIST,
	 contains_doc},
	{"copy",	(PyCFunction)frozenset_copy,	METH_NOARGS,
	 copy_doc},
	{"difference",	(PyCFunction)set_difference,	METH_O,
	 difference_doc},
	{"intersection",(PyCFunction)set_intersection,	METH_O,
	 intersection_doc},
	{"isdisjoint",	(PyCFunction)set_isdisjoint,	METH_O,
	 isdisjoint_doc},
	{"issubset",	(PyCFunction)set_issubset,	METH_O,
	 issubset_doc},
	{"issuperset",	(PyCFunction)set_issuperset,	METH_O,
	 issuperset_doc},
	{"__reduce__",	(PyCFunction)set_reduce,	METH_NOARGS,
	 reduce_doc},
	{"symmetric_difference",(PyCFunction)set_symmetric_difference,	METH_O,
	 symmetric_difference_doc},
	{"union",	(PyCFunction)set_union,		METH_O,
	 union_doc},
	{NULL,		NULL}	/* sentinel */
};

static PyNumberMethods frozenset_as_number = {
	0,				/*nb_add*/
	(binaryfunc)set_sub,		/*nb_subtract*/
	0,				/*nb_multiply*/
	0,				/*nb_divide*/
	0,				/*nb_remainder*/
	0,				/*nb_divmod*/
	0,				/*nb_power*/
	0,				/*nb_negative*/
	0,				/*nb_positive*/
	0,				/*nb_absolute*/
	0,				/*nb_nonzero*/
	0,				/*nb_invert*/
	0,				/*nb_lshift*/
	0,				/*nb_rshift*/
	(binaryfunc)set_and,		/*nb_and*/
	(binaryfunc)set_xor,		/*nb_xor*/
	(binaryfunc)set_or,		/*nb_or*/
};

PyDoc_STRVAR(frozenset_doc,
"frozenset(iterable) --> frozenset object\n\
\n\
Build an immutable unordered collection of unique elements.");

PyTypeObject PyFrozenSet_Type = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"frozenset",			/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	set_nocmp,			/* tp_compare */
	(reprfunc)set_repr,		/* tp_repr */
	&frozenset_as_number,		/* tp_as_number */
	&set_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	frozenset_hash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	frozenset_doc,			/* tp_doc */
	(traverseproc)set_traverse,	/* tp_traverse */
	(inquiry)set_clear_internal,	/* tp_clear */
	(richcmpfunc)set_richcompare,	/* tp_richcompare */
	offsetof(PySetObject, weakreflist),	/* tp_weaklistoffset */
	(getiterfunc)set_iter,		/* tp_iter */
	0,				/* tp_iternext */
	frozenset_methods,		/* tp_methods */
	0,				/* tp_members */
	0,				/* tp_getset */
	0,				/* tp_base */
	0,				/* tp_dict */
	0,				/* tp_descr_get */
	0,				/* tp_descr_set */
	0,				/* tp_dictoffset */
	0,				/* tp_init */
	PyType_GenericAlloc,		/* tp_alloc */
	frozenset_new,			/* tp_new */
	PyObject_GC_Del,		/* tp_free */
};


/***** C API functions *************************************************/

PyObject *
PySet_New(PyObject *iterable)
{
	return make_new_set(&PySet_Type, iterable);
}

PyObject *
PyFrozenSet_New(PyObject *iterable)
{
	PyObject *args, *result;

	if (iterable == NULL)
		args = PyTuple_New(0);
	else
		args = PyTuple_Pack(1, iterable);
	if (args == NULL)
		return NULL;
	result = frozenset_new(&PyFrozenSet_Type, args, NULL);
	Py_DECREF(args);
	return result;
}

Py_ssize_t
PySet_Size(PyObject *anyset)
{
	if (!PyAnySet_Check(anyset)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return PySet_GET_SIZE(anyset);
}

int
PySet_Clear(PyObject *set)
{
	if (!PyType_IsSubtype(Py_TYPE(set), &PySet_Type)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return set_clear_internal((PySetObject *)set);
}

int
PySet_Contains(PyObject *anyset, PyObject *key)
{
	if (!PyAnySet_Check(anyset)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return set_contains_key((PySetObject *)anyset, key);
}

int
PySet_Discard(PyObject *set, PyObject *key)
{
	if (!PyType_IsSubtype(Py_TYPE(set), &PySet_Type)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return set_discard_key((PySetObject *)set, key);
}

int
PySet_Add(PyObject *set, PyObject *key)
{
	if (!PyType_IsSubtype(Py_TYPE(set), &PySet_Type)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return set_add_key((PySetObject *)set, key);
}

int
_PySet_Next(PyObject *set, Py_ssize_t *pos, PyObject **key)
{
	setentry *entry_ptr;

	if (!PyAnySet_Check(set)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (set_next((PySetObject *)set, pos, &entry_ptr) == 0)
		return 0;
	*key = entry_ptr->key;
	return 1;
}

int
_PySet_NextEntry(PyObject *set, Py_ssize_t *pos, PyObject **key, long *hash)
{
	setentry *entry;

	if (!PyAnySet_Check(set)) {
		PyErr_BadInternalCall();
		return -1;
	}
	if (set_next((PySetObject *)set, pos, &entry) == 0)
		return 0;
	*key = entry->key;
	*hash = entry->hash;
	return 1;
}

PyObject *
PySet_Pop(PyObject *set)
{
	if (!PyType_IsSubtype(Py_TYPE(set), &PySet_Type)) {
		PyErr_BadInternalCall();
		return NULL;
	}
	return set_pop((PySetObject *)set);
}

int
_PySet_Update(PyObject *set, PyObject *iterable)
{
	if (!PyType_IsSubtype(Py_TYPE(set), &PySet_Type)) {
		PyErr_BadInternalCall();
		return -1;
	}
	return set_update_internal((PySetObject *)set, iterable);
}

#ifdef Py_DEBUG

/* Test code to be called with any three element set. 
   Returns True and original set is restored. */

#define assertRaises(call_return_value, exception)		\
	do {							\
		assert(call_return_value);			\
		assert(PyErr_ExceptionMatches(exception));	\
		PyErr_Clear();					\
	} while(0)

static PyObject *
test_c_api(PySetObject *so)
{
	Py_ssize_t count;
	char *s;
	Py_ssize_t i;
	PyObject *elem=NULL, *dup=NULL, *t, *f, *dup2, *x;
	PyObject *ob = (PyObject *)so;

	/* Verify preconditions and exercise type/size checks */
	assert(PyAnySet_Check(ob));
	assert(PyAnySet_CheckExact(ob));
	assert(!PyFrozenSet_CheckExact(ob));
	assert(PySet_Size(ob) == 3);
	assert(PySet_GET_SIZE(ob) == 3);

	/* Raise TypeError for non-iterable constructor arguments */
	assertRaises(PySet_New(Py_None) == NULL, PyExc_TypeError);
	assertRaises(PyFrozenSet_New(Py_None) == NULL, PyExc_TypeError);

	/* Raise TypeError for unhashable key */
	dup = PySet_New(ob);
	assertRaises(PySet_Discard(ob, dup) == -1, PyExc_TypeError);
	assertRaises(PySet_Contains(ob, dup) == -1, PyExc_TypeError);
	assertRaises(PySet_Add(ob, dup) == -1, PyExc_TypeError);

	/* Exercise successful pop, contains, add, and discard */
	elem = PySet_Pop(ob);
	assert(PySet_Contains(ob, elem) == 0);
	assert(PySet_GET_SIZE(ob) == 2);
	assert(PySet_Add(ob, elem) == 0);
	assert(PySet_Contains(ob, elem) == 1);
	assert(PySet_GET_SIZE(ob) == 3);
	assert(PySet_Discard(ob, elem) == 1);
	assert(PySet_GET_SIZE(ob) == 2);
	assert(PySet_Discard(ob, elem) == 0);
	assert(PySet_GET_SIZE(ob) == 2);

	/* Exercise clear */
	dup2 = PySet_New(dup);
	assert(PySet_Clear(dup2) == 0);
	assert(PySet_Size(dup2) == 0);
	Py_DECREF(dup2);

	/* Raise SystemError on clear or update of frozen set */
	f = PyFrozenSet_New(dup);
	assertRaises(PySet_Clear(f) == -1, PyExc_SystemError);
	assertRaises(_PySet_Update(f, dup) == -1, PyExc_SystemError);
	Py_DECREF(f);

	/* Exercise direct iteration */
	i = 0, count = 0;
	while (_PySet_Next((PyObject *)dup, &i, &x)) {
		s = PyString_AsString(x);
		assert(s && (s[0] == 'a' || s[0] == 'b' || s[0] == 'c'));
		count++;
	}
	assert(count == 3);

	/* Exercise updates */
	dup2 = PySet_New(NULL);
	assert(_PySet_Update(dup2, dup) == 0);
	assert(PySet_Size(dup2) == 3);
	assert(_PySet_Update(dup2, dup) == 0);
	assert(PySet_Size(dup2) == 3);
	Py_DECREF(dup2);

	/* Raise SystemError when self argument is not a set or frozenset. */
	t = PyTuple_New(0);
	assertRaises(PySet_Size(t) == -1, PyExc_SystemError);
	assertRaises(PySet_Contains(t, elem) == -1, PyExc_SystemError);
	Py_DECREF(t);

	/* Raise SystemError when self argument is not a set. */
	f = PyFrozenSet_New(dup);
	assert(PySet_Size(f) == 3);
	assert(PyFrozenSet_CheckExact(f));
	assertRaises(PySet_Add(f, elem) == -1, PyExc_SystemError);
	assertRaises(PySet_Discard(f, elem) == -1, PyExc_SystemError);
	assertRaises(PySet_Pop(f) == NULL, PyExc_SystemError);
	Py_DECREF(f);

	/* Raise KeyError when popping from an empty set */
	assert(PyNumber_InPlaceSubtract(ob, ob) == ob);
	Py_DECREF(ob);
	assert(PySet_GET_SIZE(ob) == 0);
	assertRaises(PySet_Pop(ob) == NULL, PyExc_KeyError);

	/* Restore the set from the copy using the PyNumber API */
	assert(PyNumber_InPlaceOr(ob, dup) == ob);
	Py_DECREF(ob);

	/* Verify constructors accept NULL arguments */
	f = PySet_New(NULL);
	assert(f != NULL);
	assert(PySet_GET_SIZE(f) == 0);
	Py_DECREF(f);
	f = PyFrozenSet_New(NULL);
	assert(f != NULL);
	assert(PyFrozenSet_CheckExact(f));
	assert(PySet_GET_SIZE(f) == 0);
	Py_DECREF(f);

	Py_DECREF(elem);
	Py_DECREF(dup);
	Py_RETURN_TRUE;
}

#undef assertRaises

#endif
