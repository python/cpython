
/* Set object implementation using a hash table 
   Functions adapted from dictobject.c
*/

#include "Python.h"

/* This must be >= 1. */
#define PERTURB_SHIFT 5

/* Object used as dummy key to fill deleted entries */
static PyObject *dummy; /* Initialized by first call to make_new_set() */

#define EMPTY_TO_MINSIZE(so) do {				\
	memset((so)->smalltable, 0, sizeof((so)->smalltable));	\
	(so)->used = (so)->fill = 0;				\
	(so)->table = (so)->smalltable;				\
	(so)->mask = PySet_MINSIZE - 1;				\
    } while(0)


/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained earlier.

All arithmetic on hash should ignore overflow.

This function must never return NULL; failures are indicated by returning
a setentry* for which the value field is NULL.  Exceptions are never
reported by this function, and outstanding exceptions are maintained.
*/

static setentry *
set_lookkey(PySetObject *so, PyObject *key, register long hash)
{
	register int i;
	register unsigned int perturb;
	register setentry *freeslot;
	register unsigned int mask = so->mask;
	setentry *entry0 = so->table;
	register setentry *entry;
	register int restore_error;
	register int checked_error;
	register int cmp;
	PyObject *err_type, *err_value, *err_tb;
	PyObject *startkey;

	i = hash & mask;
	entry = &entry0[i];
	if (entry->key == NULL || entry->key == key)
		return entry;

	restore_error = checked_error = 0;
	if (entry->key == dummy)
		freeslot = entry;
	else {
		if (entry->hash == hash) {
			/* error can't have been checked yet */
			checked_error = 1;
			if (PyErr_Occurred()) {
				restore_error = 1;
				PyErr_Fetch(&err_type, &err_value, &err_tb);
			}
			startkey = entry->key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				PyErr_Clear();
			if (entry0 == so->table && entry->key == startkey) {
				if (cmp > 0)
					goto Done;
			}
			else {
				/* The compare did major nasty stuff to the
				 * set:  start over.
 				 */
 				entry = set_lookkey(so, key, hash);
 				goto Done;
 			}
		}
		freeslot = NULL;
	}

	/* In the loop, key == dummy is by far (factor of 100s) the
	   least likely outcome, so test for that last. */
	for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
		i = (i << 2) + i + perturb + 1;
		entry = &entry0[i & mask];
		if (entry->key == NULL) {
			if (freeslot != NULL)
				entry = freeslot;
			break;
		}
		if (entry->key == key)
			break;
		if (entry->hash == hash && entry->key != dummy) {
			if (!checked_error) {
				checked_error = 1;
				if (PyErr_Occurred()) {
					restore_error = 1;
					PyErr_Fetch(&err_type, &err_value,
						    &err_tb);
				}
			}
			startkey = entry->key;
			cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
			if (cmp < 0)
				PyErr_Clear();
			if (entry0 == so->table && entry->key == startkey) {
				if (cmp > 0)
					break;
			}
			else {
				/* The compare did major nasty stuff to the
				 * set:  start over.
 				 */
 				entry = set_lookkey(so, key, hash);
 				break;
 			}
		}
		else if (entry->key == dummy && freeslot == NULL)
			freeslot = entry;
	}

Done:
	if (restore_error)
		PyErr_Restore(err_type, err_value, err_tb);
	return entry;
}

/*
 * Hacked up version of set_lookkey which can assume keys are always strings;
 * this assumption allows testing for errors during PyObject_Compare() to
 * be dropped; string-string comparisons never raise exceptions.  This also
 * means we don't need to go through PyObject_Compare(); we can always use
 * _PyString_Eq directly.
 *
 * This is valuable because the general-case error handling in set_lookkey() is
 * expensive, and sets with pure-string keys may be very common.
 */
static setentry *
set_lookkey_string(PySetObject *so, PyObject *key, register long hash)
{
	register int i;
	register unsigned int perturb;
	register setentry *freeslot;
	register unsigned int mask = so->mask;
	setentry *entry0 = so->table;
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
	entry = &entry0[i];
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
		entry = &entry0[i & mask];
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
}

/*
Internal routine to insert a new key into the table.
Used both by the internal resize routine and by the public insert routine.
Eats a reference to key.
*/
static void
set_insert_key(register PySetObject *so, PyObject *key, long hash)
{
	register setentry *entry;
	typedef setentry *(*lookupfunc)(PySetObject *, PyObject *, long);

	assert(so->lookup != NULL);

	entry = so->lookup(so, key, hash);
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
}

/*
Restructure the table by allocating a new table and reinserting all
keys again.  When entries have been deleted, the new table may
actually be smaller than the old one.
*/
static int
set_table_resize(PySetObject *so, int minused)
{
	int newsize;
	setentry *oldtable, *newtable, *entry;
	int i;
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
			set_insert_key(so, entry->key, entry->hash);
		}
	}

	if (is_oldtable_malloced)
		PyMem_DEL(oldtable);
	return 0;
}

/*** Internal functions (derived from public dictionary api functions )  ***/


/* CAUTION: set_add_internal() must guarantee that it won't resize the table */
static int
set_add_internal(register PySetObject *so, PyObject *key)
{
	register long hash;
	register int n_used;

	if (PyString_CheckExact(key)) {
		hash = ((PyStringObject *)key)->ob_shash;
		if (hash == -1)
			hash = PyObject_Hash(key);
	} else {
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	assert(so->fill <= so->mask);  /* at least one empty slot */
	n_used = so->used;
	Py_INCREF(key);
	set_insert_key(so, key, hash);
	if (!(so->used > n_used && so->fill*3 >= (so->mask+1)*2))
		return 0;
	return set_table_resize(so, so->used*(so->used>50000 ? 2 : 4));
}

#define DISCARD_NOTFOUND 0
#define DISCARD_FOUND 1

static int
set_discard_internal(PySetObject *so, PyObject *key)
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
	if (entry->key == NULL  ||  entry->key == dummy)
		return DISCARD_NOTFOUND;
	old_key = entry->key;
	Py_INCREF(dummy);
	entry->key = dummy;
	so->used--;
	Py_DECREF(old_key);
	return DISCARD_FOUND;
}

static void
set_clear_internal(PySetObject *so)
{
	setentry *entry, *table;
	int table_is_malloced;
	int fill;
	setentry small_copy[PySet_MINSIZE];
#ifdef Py_DEBUG
	int i, n;
#endif

	assert (PyAnySet_Check(so));
#ifdef Py_DEBUG
	n = so->mask + 1;
	i = 0;
#endif

	table = so->table;
	assert(table != NULL);
	table_is_malloced = table != so->smalltable;

	/* This is delicate.  During the process of clearing the set,
	 * decrefs can cause the set to mutate.  To avoid fatal confusion
	 * (voice of experience), we have to make the set empty before
	 * clearing the slots, and never refer to anything via mp->ref while
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
			assert(entry->key == NULL || entry->key == dummy);
#endif
	}

	if (table_is_malloced)
		PyMem_DEL(table);
}

/*
 * Iterate over a set table.  Use like so:
 *
 *     int i;
 *     PyObject *key;
 *     i = 0;   # important!  i should not otherwise be changed by you
 *     while (set_next_internal(yourset, &i, &key)) {
 *              Refer to borrowed reference in key.
 *     }
 *
 * CAUTION:  In general, it isn't safe to use set_next_internal in a loop that
 * mutates the table.  
 */
static int
set_next_internal(PySetObject *so, int *pos, PyObject **key)
{
	register int i, mask;
	register setentry *entry;

	assert (PyAnySet_Check(so));
	i = *pos;
	if (i < 0)
		return 0;
	entry = so->table;
	mask = so->mask;
	while (i <= mask && (entry[i].key == NULL || entry[i].key == dummy))
		i++;
	*pos = i+1;
	if (i > mask)
		return 0;
	if (key)
		*key = entry[i].key;
	return 1;
}

/* Methods */

static int
set_merge_internal(PySetObject *so, PyObject *otherset)
{
	register PySetObject *other;
	register int i;
	setentry *entry;

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
			set_insert_key(so, entry->key, entry->hash);
		}
	}
	return 0;
}

static int
set_contains_internal(PySetObject *so, PyObject *key)
{
	long hash;

	if (!PyString_CheckExact(key) ||
	    (hash = ((PyStringObject *) key)->ob_shash) == -1) {
		hash = PyObject_Hash(key);
		if (hash == -1)
			return -1;
	}
	key = (so->lookup)(so, key, hash)->key; 
	return key != NULL && key != dummy;
}

static PyTypeObject PySetIter_Type; /* Forward */

/* Set iterator types */

typedef struct {
	PyObject_HEAD
	PySetObject *si_set; /* Set to NULL when iterator is exhausted */
	int si_used;
	int si_pos;
	long len;
} setiterobject;

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

static void
setiter_dealloc(setiterobject *si)
{
	Py_XDECREF(si->si_set);
	PyObject_Del(si);
}

static int
setiter_len(setiterobject *si)
{
	if (si->si_set != NULL && si->si_used == si->si_set->used)
		return si->len;
	return 0;
}

static PySequenceMethods setiter_as_sequence = {
	(inquiry)setiter_len,		/* sq_length */
	0,				/* sq_concat */
};

static PyObject *setiter_iternext(setiterobject *si)
{
	PyObject *key;
	register int i, mask;
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
	if (i < 0)
		goto fail;
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

PyTypeObject PySetIter_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,					/* ob_size */
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
	&setiter_as_sequence,			/* tp_as_sequence */
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
};

/***** Derived functions (table accesses only done with above primitives *****/

#include "structmember.h"

/* set object implementation 
   written and maintained by Raymond D. Hettinger <python@rcn.com>
   derived from sets.py written by Greg V. Wilson, Alex Martelli, 
   Guido van Rossum, Raymond Hettinger, and Tim Peters.

   Copyright (c) 2003-5 Python Software Foundation.
   All rights reserved.
*/

static PyObject *
set_update(PySetObject *so, PyObject *other)
{
	PyObject *key, *it;

	if (PyAnySet_Check(other)) {
		if (set_merge_internal(so, other) == -1) 
			return NULL;
		Py_RETURN_NONE;
	}

	if (PyDict_Check(other)) {
		PyObject *key, *value;
		int pos = 0;
		while (PyDict_Next(other, &pos, &key, &value)) {
			if (set_add_internal(so, key) == -1)
				return NULL;
		}
		Py_RETURN_NONE;
	}

	it = PyObject_GetIter(other);
	if (it == NULL)
		return NULL;

	while ((key = PyIter_Next(it)) != NULL) {
                if (set_add_internal(so, key) == -1) {
			Py_DECREF(it);
			Py_DECREF(key);
			return NULL;
                } 
		Py_DECREF(key);
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc, 
"Update a set with the union of itself and another.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
	PyObject *tmp;
	register PySetObject *so = NULL;

	if (dummy == NULL) { /* Auto-initialize dummy */
		dummy = PyString_FromString("<dummy key>");
		if (dummy == NULL)
			return NULL;
	}

	/* create PySetObject structure */
	so = (PySetObject *)type->tp_alloc(type, 0);
	if (so == NULL)
		return NULL;

	EMPTY_TO_MINSIZE(so);
	so->lookup = set_lookkey_string;
	so->hash = -1;
	so->weakreflist = NULL;

	if (iterable != NULL) {
		tmp = set_update(so, iterable);
		if (tmp == NULL) {
			Py_DECREF(so);
			return NULL;
		}
		Py_DECREF(tmp);
	}

	return (PyObject *)so;
}

static PyObject *
frozenset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyObject *iterable = NULL;
	static PyObject *emptyfrozenset = NULL;

	if (!PyArg_UnpackTuple(args, type->tp_name, 0, 1, &iterable))
		return NULL;
	if (iterable == NULL) {
		if (type == &PyFrozenSet_Type) {
			if (emptyfrozenset == NULL)
				emptyfrozenset = make_new_set(type, NULL);
			Py_INCREF(emptyfrozenset);
			return emptyfrozenset;
		}
	} else if (PyFrozenSet_CheckExact(iterable)) {
		Py_INCREF(iterable);
		return iterable;
	}
	return make_new_set(type, iterable);
}

static PyObject *
set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	return make_new_set(type, NULL);
}

static void
set_dealloc(PySetObject *so)
{
	register setentry *entry;
	int fill = so->fill;

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

	so->ob_type->tp_free(so);
	Py_TRASHCAN_SAFE_END(so)
}

static int
set_traverse(PySetObject *so, visitproc visit, void *arg)
{
	int pos = 0;
	PyObject *key;

	while (set_next_internal(so, &pos, &key))
		Py_VISIT(key);
	return 0;
}

static int
set_len(PyObject *so)
{
	return ((PySetObject *)so)->used;
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
	int t;
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

	h = a->hash;     a->hash   = b->hash;        b->hash     = h;
}

static int
set_contains(PySetObject *so, PyObject *key)
{
	PyObject *tmpkey;
	int result;

	result = set_contains_internal(so, key);
	if (result == -1 && PyAnySet_Check(key)) {
		PyErr_Clear();
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return -1;
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		result = set_contains_internal(so, tmpkey);
		set_swap_bodies((PySetObject *)tmpkey, (PySetObject *)key);
		Py_DECREF(tmpkey);
	}
	return result;
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
set_copy(PySetObject *so)
{
	return make_new_set(so->ob_type, (PyObject *)so);
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
set_union(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *rv;

	result = (PySetObject *)set_copy(so);
	if (result == NULL)
		return NULL;
	rv = set_update(result, other);
	if (rv == NULL) {
		Py_DECREF(result);
		return NULL;
	}
	Py_DECREF(rv);
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
	PyObject *result;

	if (!PyAnySet_Check(other)) {
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}
	result = set_update(so, other);
	if (result == NULL)
		return NULL;
	Py_DECREF(result);
	Py_INCREF(so);
	return (PyObject *)so;
}

static PyObject *
set_intersection(PySetObject *so, PyObject *other)
{
	PySetObject *result;
	PyObject *key, *it, *tmp;

	result = (PySetObject *)make_new_set(so->ob_type, NULL);
	if (result == NULL)
		return NULL;

	if (PyAnySet_Check(other) && set_len(other) > set_len((PyObject *)so)) {
		tmp = (PyObject *)so;
		so = (PySetObject *)other;
		other = tmp;
	}

	if (PyAnySet_Check(other)) {
		int pos = 0;
		while (set_next_internal((PySetObject *)other, &pos, &key)) {
			if (set_contains_internal(so, key)) {
				if (set_add_internal(result, key) == -1) {
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
		if (set_contains_internal(so, key)) {
			if (set_add_internal(result, key) == -1) {
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
set_difference_update(PySetObject *so, PyObject *other)
{
	PyObject *key, *it;
	
	it = PyObject_GetIter(other);
	if (it == NULL)
		return NULL;

	while ((key = PyIter_Next(it)) != NULL) {
		if (set_discard_internal(so, key) == -1) {
			Py_DECREF(it);
			Py_DECREF(key);
			return NULL;
		}
		Py_DECREF(key);
	}
	Py_DECREF(it);
	if (PyErr_Occurred())
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another set from this set.");

static PyObject *
set_difference(PySetObject *so, PyObject *other)
{
	PyObject *tmp, *key, *result;
	int pos = 0;

	if (!PyAnySet_Check(other)  && !PyDict_Check(other)) {
		result = set_copy(so);
		if (result == NULL)
			return result;
		tmp = set_difference_update((PySetObject *)result, other);
		if (tmp != NULL) {
			Py_DECREF(tmp);
			return result;
		}
		Py_DECREF(result);
		return NULL;
	}
	
	result = make_new_set(so->ob_type, NULL);
	if (result == NULL)
		return NULL;

	if (PyDict_Check(other)) {
		while (set_next_internal(so, &pos, &key)) {
			if (!PyDict_Contains(other, key)) {
				if (set_add_internal((PySetObject *)result, key) == -1)
					return NULL;
			}
		}
		return result;
	}

	while (set_next_internal(so, &pos, &key)) {
		if (!set_contains_internal((PySetObject *)other, key)) {
			if (set_add_internal((PySetObject *)result, key) == -1)
				return NULL;
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
	int pos = 0;

	if (PyDict_Check(other)) {
		PyObject *value;
		int rv;
		while (PyDict_Next(other, &pos, &key, &value)) {
			rv = set_discard_internal(so, key);
			if (rv == -1)
				return NULL;
			if (rv == DISCARD_NOTFOUND) {
				if (set_add_internal(so, key) == -1)
					return NULL;
			}
		}
		Py_RETURN_NONE;
	}

	if (PyAnySet_Check(other)) {
		Py_INCREF(other);
		otherset = (PySetObject *)other;
	} else {
		otherset = (PySetObject *)make_new_set(so->ob_type, other);
		if (otherset == NULL)
			return NULL;
	}

	while (set_next_internal(otherset, &pos, &key)) {
		int rv = set_discard_internal(so, key);
		if (rv == -1) {
			Py_XDECREF(otherset);
			return NULL;
		}
		if (rv == DISCARD_NOTFOUND) {
			if (set_add_internal(so, key) == -1) {
				Py_XDECREF(otherset);
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

	otherset = (PySetObject *)make_new_set(so->ob_type, other);
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
	PyObject *tmp, *result;
	PyObject *key;
	int pos = 0;

	if (!PyAnySet_Check(other)) {
		tmp = make_new_set(&PySet_Type, other);
		if (tmp == NULL)
			return NULL;
		result = set_issubset(so, tmp);
		Py_DECREF(tmp);
		return result;
	}
	if (set_len((PyObject *)so) > set_len(other)) 
		Py_RETURN_FALSE;

	while (set_next_internal(so, &pos, &key)) {
		if (!set_contains_internal((PySetObject *)other, key))
			Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another set contains this set.");

static PyObject *
set_issuperset(PySetObject *so, PyObject *other)
{
	PyObject *tmp, *result;

	if (!PyAnySet_Check(other)) {
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

static long
set_nohash(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "set objects are unhashable");
	return -1;
}

static int
set_nocmp(PyObject *self)
{
	PyErr_SetString(PyExc_TypeError, "cannot compare sets using cmp()");
	return -1;
}

static long
frozenset_hash(PyObject *self)
{
	PyObject *key;
	PySetObject *so = (PySetObject *)self;
	int pos = 0;
	long hash = 1927868237L;

	if (so->hash != -1)
		return so->hash;

	hash *= set_len(self) + 1;
	while (set_next_internal(so, &pos, &key)) {
		/* Work to increase the bit dispersion for closely spaced hash
		   values.  The is important because some use cases have many 
		   combinations of a small number of elements with nearby 
		   hashes so that many distinct combinations collapse to only 
		   a handful of distinct hash values. */
		long h = PyObject_Hash(key);
		hash ^= (h ^ (h << 16) ^ 89869747L)  * 3644798167u;
	}
	hash = hash * 69069L + 907133923L;
	if (hash == -1)
		hash = 590923713L;
	so->hash = hash;
	return hash;
}

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
		if (set_len((PyObject *)v) != set_len(w))
			Py_RETURN_FALSE;
		return set_issubset(v, w);
	case Py_NE:
		if (set_len((PyObject *)v) != set_len(w))
			Py_RETURN_TRUE;
		r1 = set_issubset(v, w);
		assert (r1 != NULL);
		r2 = PyBool_FromLong(PyObject_Not(r1));
		Py_DECREF(r1);
		return r2;
	case Py_LE:
		return set_issubset(v, w);
	case Py_GE:
		return set_issuperset(v, w);
	case Py_LT:
		if (set_len((PyObject *)v) >= set_len(w))
			Py_RETURN_FALSE;		
		return set_issubset(v, w);
	case Py_GT:
		if (set_len((PyObject *)v) <= set_len(w))
			Py_RETURN_FALSE;
		return set_issuperset(v, w);
	}
	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

static PyObject *
set_repr(PySetObject *so)
{
	PyObject *keys, *result, *listrepr;

	keys = PySequence_List((PyObject *)so);
	if (keys == NULL)
		return NULL;
	listrepr = PyObject_Repr(keys);
	Py_DECREF(keys);
	if (listrepr == NULL)
		return NULL;

	result = PyString_FromFormat("%s(%s)", so->ob_type->tp_name,
		PyString_AS_STRING(listrepr));
	Py_DECREF(listrepr);
	return result;
}

static int
set_tp_print(PySetObject *so, FILE *fp, int flags)
{
	PyObject *key;
	int pos=0;
	char *emit = "";	/* No separator emitted on first pass */
	char *separator = ", ";

	fprintf(fp, "%s([", so->ob_type->tp_name);
	while (set_next_internal(so, &pos, &key)) {
		fputs(emit, fp);
		emit = separator;
		if (PyObject_Print(key, fp, 0) != 0)
			return -1;
	}
	fputs("])", fp);
	return 0;
}

static PyObject *
set_clear(PySetObject *so)
{
	set_clear_internal(so);
	so->hash = -1;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from this set.");

static int
set_tp_clear(PySetObject *so)
{
	set_clear_internal(so);
	so->hash = -1;
	return 0;
}

static PyObject *
set_add(PySetObject *so, PyObject *key)
{
	if (set_add_internal(so, key) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc, 
"Add an element to a set.\n\
\n\
This has no effect if the element is already present.");

static PyObject *
set_remove(PySetObject *so, PyObject *key)
{
	PyObject *tmpkey, *result;
	int rv;

	if (PyType_IsSubtype(key->ob_type, &PySet_Type)) {
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return NULL;
		set_swap_bodies((PySetObject *)key, (PySetObject *)tmpkey);
		result = set_remove(so, tmpkey);
		set_swap_bodies((PySetObject *)key, (PySetObject *)tmpkey);
		Py_DECREF(tmpkey);
		return result;
	}

	rv = set_discard_internal(so, key);
	if (rv == -1) 
		return NULL;
	else if (rv == DISCARD_NOTFOUND) {
		PyErr_SetObject(PyExc_KeyError, key);
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

	if (PyType_IsSubtype(key->ob_type, &PySet_Type)) {
		tmpkey = make_new_set(&PyFrozenSet_Type, NULL);
		if (tmpkey == NULL)
			return NULL;
		set_swap_bodies((PySetObject *)key, (PySetObject *)tmpkey);
		result = set_discard(so, tmpkey);
		set_swap_bodies((PySetObject *)key, (PySetObject *)tmpkey);
		Py_DECREF(tmpkey);
		return result;
	}

	if (set_discard_internal(so, key) == -1)
		return NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a set if it is a member.\n\
\n\
If the element is not a member, do nothing."); 

static PyObject *
set_pop(PySetObject *so)
{
	PyObject *key;
	int pos = 0;
	int rv;

	if (!set_next_internal(so, &pos, &key)) {
		PyErr_SetString(PyExc_KeyError, "pop from an empty set");
		return NULL;
	}
	Py_INCREF(key);

	rv = set_discard_internal(so, key);
	if (rv == -1) {
		Py_DECREF(key);
		return NULL;
	} else if (rv == DISCARD_NOTFOUND) {
		Py_DECREF(key);
		PyErr_SetObject(PyExc_KeyError, key);
		return NULL;
	}
	return key;
}

PyDoc_STRVAR(pop_doc, "Remove and return an arbitrary set element.");

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
	result = PyTuple_Pack(3, so->ob_type, args, dict);
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
	PyObject *result;

	if (!PyAnySet_Check(self))
		return -1;
	if (!PyArg_UnpackTuple(args, self->ob_type->tp_name, 0, 1, &iterable))
		return -1;
	set_clear_internal(self);
	self->hash = -1;
	if (iterable == NULL)
		return 0;
	result = set_update(self, iterable);
	if (result != NULL) {
		Py_DECREF(result);
		return 0;
	}
	return -1;
}

static PySequenceMethods set_as_sequence = {
	(inquiry)set_len,		/* sq_length */
	0,				/* sq_concat */
	0,				/* sq_repeat */
	0,				/* sq_item */
	0,				/* sq_slice */
	0,				/* sq_ass_item */
	0,				/* sq_ass_slice */
	(objobjproc)set_contains,	/* sq_contains */
};

/* set object ********************************************************/

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
Build an unordered collection.");

PyTypeObject PySet_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* ob_size */
	"set",				/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc)set_nocmp,		/* tp_compare */
	(reprfunc)set_repr,		/* tp_repr */
	&set_as_number,			/* tp_as_number */
	&set_as_sequence,		/* tp_as_sequence */
	0,				/* tp_as_mapping */
	set_nohash,			/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,	/* tp_getattro */
	0,				/* tp_setattro */
	0,				/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_CHECKTYPES |
		Py_TPFLAGS_BASETYPE,	/* tp_flags */
	set_doc,			/* tp_doc */
	(traverseproc)set_traverse,	/* tp_traverse */
	(inquiry)set_tp_clear,		/* tp_clear */
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
Build an immutable unordered collection.");

PyTypeObject PyFrozenSet_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/* ob_size */
	"frozenset",			/* tp_name */
	sizeof(PySetObject),		/* tp_basicsize */
	0,				/* tp_itemsize */
	/* methods */
	(destructor)set_dealloc,	/* tp_dealloc */
	(printfunc)set_tp_print,	/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	(cmpfunc)set_nocmp,		/* tp_compare */
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
	0,				/* tp_clear */
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
