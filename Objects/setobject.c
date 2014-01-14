
/* set object implementation

   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Derived from Lib/sets.py and Objects/dictobject.c.

   Copyright (c) 2003-2013 Python Software Foundation.
   All rights reserved.

   The basic lookup function used by all operations.
   This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.

   The initial probe index is computed as hash mod the table size.
   Subsequent probe indices are computed as explained in Objects/dictobject.c.

   To improve cache locality, each probe inspects a series of consecutive
   nearby entries before moving on to probes elsewhere in memory.  This leaves
   us with a hybrid of linear probing and open addressing.  The linear probing
   reduces the cost of hash collisions because consecutive memory accesses
   tend to be much cheaper than scattered probes.  After LINEAR_PROBES steps,
   we then use open addressing with the upper bits from the hash value.  This
   helps break-up long chains of collisions.

   All arithmetic on hash should ignore overflow.

   Unlike the dictionary implementation, the lookkey functions can return
   NULL if the rich comparison returns an error.
*/

#include "Python.h"
#include "structmember.h"
#include "stringlib/eq.h"

/* Object used as dummy key to fill deleted entries */
static PyObject _dummy_struct;

#define dummy (&_dummy_struct)


/* ======================================================================== */
/* ======= Begin logic for probing the hash table ========================= */

/* Set this to zero to turn-off linear probing */
#ifndef LINEAR_PROBES
#define LINEAR_PROBES 9
#endif

/* This must be >= 1 */
#define PERTURB_SHIFT 5

static setentry *
set_lookkey(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *table = so->table;
    setentry *freeslot = NULL;
    setentry *entry;
    size_t perturb = hash;
    size_t mask = so->mask;
    size_t i = (size_t)hash; /* Unsigned for defined overflow behavior. */
    size_t j;
    int cmp;

    entry = &table[i & mask];
    if (entry->key == NULL)
        return entry;

    while (1) {
        if (entry->key == key)
            return entry;
        if (entry->hash == hash && entry->key != dummy) {
            PyObject *startkey = entry->key;
            Py_INCREF(startkey);
            cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0)
                return NULL;
            if (table != so->table || entry->key != startkey)
                return set_lookkey(so, key, hash);
            if (cmp > 0)
                return entry;
        }
        if (entry->key == dummy && freeslot == NULL)
            freeslot = entry;

        for (j = 1 ; j <= LINEAR_PROBES ; j++) {
            entry = &table[(i + j) & mask];
            if (entry->key == NULL)
                goto found_null;
            if (entry->key == key)
                return entry;
            if (entry->hash == hash && entry->key != dummy) {
                PyObject *startkey = entry->key;
                Py_INCREF(startkey);
                cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
                Py_DECREF(startkey);
                if (cmp < 0)
                    return NULL;
                if (table != so->table || entry->key != startkey)
                    return set_lookkey(so, key, hash);
                if (cmp > 0)
                    return entry;
            }
            if (entry->key == dummy && freeslot == NULL)
                freeslot = entry;
        }

        perturb >>= PERTURB_SHIFT;
        i = i * 5 + 1 + perturb;

        entry = &table[i & mask];
        if (entry->key == NULL)
            goto found_null;
    }
  found_null:
    return freeslot == NULL ? entry : freeslot;
}

/*
 * Hacked up version of set_lookkey which can assume keys are always unicode;
 * This means we can always use unicode_eq directly and not have to check to
 * see if the comparison altered the table.
 */
static setentry *
set_lookkey_unicode(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *table = so->table;
    setentry *freeslot = NULL;
    setentry *entry;
    size_t perturb = hash;
    size_t mask = so->mask;
    size_t i = (size_t)hash;
    size_t j;

    /* Make sure this function doesn't have to handle non-unicode keys,
       including subclasses of str; e.g., one reason to subclass
       strings is to override __eq__, and for speed we don't cater to
       that here. */
    if (!PyUnicode_CheckExact(key)) {
        so->lookup = set_lookkey;
        return set_lookkey(so, key, hash);
    }

    entry = &table[i & mask];
    if (entry->key == NULL)
        return entry;

    while (1) {
        if (entry->key == key
            || (entry->hash == hash
                && entry->key != dummy
                && unicode_eq(entry->key, key)))
            return entry;
        if (entry->key == dummy && freeslot == NULL)
            freeslot = entry;

        for (j = 1 ; j <= LINEAR_PROBES ; j++) {
            entry = &table[(i + j) & mask];
            if (entry->key == NULL)
                goto found_null;
            if (entry->key == key
                || (entry->hash == hash
                    && entry->key != dummy
                    && unicode_eq(entry->key, key)))
                return entry;
            if (entry->key == dummy && freeslot == NULL)
                freeslot = entry;
        }

        perturb >>= PERTURB_SHIFT;
        i = i * 5 + 1 + perturb;

        entry = &table[i & mask];
        if (entry->key == NULL)
            goto found_null;
    }
  found_null:
    return freeslot == NULL ? entry : freeslot;
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
set_insert_clean(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *table = so->table;
    setentry *entry;
    size_t perturb = hash;
    size_t mask = (size_t)so->mask;
    size_t i = (size_t)hash;
    size_t j;

    while (1) {
        entry = &table[i & mask];
        if (entry->key == NULL)
            goto found_null;
        for (j = 1 ; j <= LINEAR_PROBES ; j++) {
            entry = &table[(i + j) & mask];
            if (entry->key == NULL)
                goto found_null;
        }
        perturb >>= PERTURB_SHIFT;
        i = i * 5 + 1 + perturb;
    }
  found_null:
    entry->key = key;
    entry->hash = hash;
    so->fill++;
    so->used++;
}

/* ======== End logic for probing the hash table ========================== */
/* ======================================================================== */


/*
Internal routine to insert a new key into the table.
Used by the public insert routine.
Eats a reference to key.
*/
static int
set_insert_key(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *entry;

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
    } else {
        /* ACTIVE */
        Py_DECREF(key);
    }
    return 0;
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
    i = so->used;
    so->used = 0;
    so->fill = 0;

    /* Copy the data over; this is refcount-neutral for active entries;
       dummy entries aren't copied over, of course */
    for (entry = oldtable; i > 0; entry++) {
        if (entry->key != NULL && entry->key != dummy) {
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
set_add_entry(PySetObject *so, setentry *entry)
{
    Py_ssize_t n_used;
    PyObject *key = entry->key;
    Py_hash_t hash = entry->hash;

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

static int
set_add_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;
    Py_ssize_t n_used;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
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
{       setentry *entry;
    PyObject *old_key;

    entry = (so->lookup)(so, oldentry->key, oldentry->hash);
    if (entry == NULL)
        return -1;
    if (entry->key == NULL  ||  entry->key == dummy)
        return DISCARD_NOTFOUND;
    old_key = entry->key;
    entry->key = dummy;
    so->used--;
    Py_DECREF(old_key);
    return DISCARD_FOUND;
}

static int
set_discard_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;
    setentry *entry;
    PyObject *old_key;

    assert (PyAnySet_Check(so));

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
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
    entry->key = dummy;
    so->used--;
    Py_DECREF(old_key);
    return DISCARD_FOUND;
}

static void
set_empty_to_minsize(PySetObject *so)
{
    memset(so->smalltable, 0, sizeof(so->smalltable));
    so->fill = 0;
    so->used = 0;
    so->mask = PySet_MINSIZE - 1;
    so->table = so->smalltable;
    so->hash = -1;
}

static int
set_clear_internal(PySetObject *so)
{
    setentry *entry, *table;
    int table_is_malloced;
    Py_ssize_t fill;
    setentry small_copy[PySet_MINSIZE];

#ifdef Py_DEBUG
    Py_ssize_t i = 0;
    Py_ssize_t n = so->mask + 1;
#endif

    assert (PyAnySet_Check(so));
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
        set_empty_to_minsize(so);

    else if (fill > 0) {
        /* It's a small table with something that needs to be cleared.
         * Afraid the only safe way is to copy the set entries into
         * another small table first.
         */
        memcpy(small_copy, table, sizeof(small_copy));
        table = small_copy;
        set_empty_to_minsize(so);
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
            if (entry->key != dummy)
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
    setentry *table;

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
    setentry *entry;
    Py_ssize_t fill = so->fill;
    PyObject_GC_UnTrack(so);
    Py_TRASHCAN_SAFE_BEGIN(so)
    if (so->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) so);

    for (entry = so->table; fill > 0; entry++) {
        if (entry->key) {
            --fill;
            if (entry->key != dummy)
                Py_DECREF(entry->key);
        }
    }
    if (so->table != so->smalltable)
        PyMem_DEL(so->table);
    Py_TYPE(so)->tp_free(so);
    Py_TRASHCAN_SAFE_END(so)
}

static PyObject *
set_repr(PySetObject *so)
{
    PyObject *result=NULL, *keys, *listrepr, *tmp;
    int status = Py_ReprEnter((PyObject*)so);

    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyUnicode_FromFormat("%s(...)", Py_TYPE(so)->tp_name);
    }

    /* shortcut for the empty set */
    if (!so->used) {
        Py_ReprLeave((PyObject*)so);
        return PyUnicode_FromFormat("%s()", Py_TYPE(so)->tp_name);
    }

    keys = PySequence_List((PyObject *)so);
    if (keys == NULL)
        goto done;

    /* repr(keys)[1:-1] */
    listrepr = PyObject_Repr(keys);
    Py_DECREF(keys);
    if (listrepr == NULL)
        goto done;
    tmp = PyUnicode_Substring(listrepr, 1, PyUnicode_GET_LENGTH(listrepr)-1);
    Py_DECREF(listrepr);
    if (tmp == NULL)
        goto done;
    listrepr = tmp;

    if (Py_TYPE(so) != &PySet_Type)
        result = PyUnicode_FromFormat("%s({%U})",
                                      Py_TYPE(so)->tp_name,
                                      listrepr);
    else
        result = PyUnicode_FromFormat("{%U}", listrepr);
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
    PyObject *key;
    Py_hash_t hash;
    Py_ssize_t i;
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
        key = entry->key;
        hash = entry->hash;
        if (key != NULL &&
            key != dummy) {
            Py_INCREF(key);
            if (set_insert_key(so, key, hash) == -1) {
                Py_DECREF(key);
                return -1;
            }
        }
    }
    return 0;
}

static int
set_contains_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;
    setentry *entry;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
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
    Py_ssize_t i = 0;
    setentry *entry;
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
            i = 1;              /* skip slot 0 */
        while ((entry = &so->table[i])->key == NULL || entry->key==dummy) {
            i++;
            if (i > so->mask)
                i = 1;
        }
    }
    key = entry->key;
    entry->key = dummy;
    so->used--;
    so->table[0].hash = i + 1;  /* next place to start */
    return key;
}

PyDoc_STRVAR(pop_doc, "Remove and return an arbitrary set element.\n\
Raises KeyError if the set is empty.");

static int
set_traverse(PySetObject *so, visitproc visit, void *arg)
{
    Py_ssize_t pos = 0;
    setentry *entry;

    while (set_next(so, &pos, &entry))
        Py_VISIT(entry->key);
    return 0;
}

static Py_hash_t
frozenset_hash(PyObject *self)
{
    /* Most of the constants in this hash algorithm are randomly choosen
       large primes with "interesting bit patterns" and that passed
       tests for good collision statistics on a variety of problematic
       datasets such as:

          ps = []
          for r in range(21):
              ps += itertools.combinations(range(20), r)
          num_distinct_hashes = len({hash(frozenset(s)) for s in ps})

    */
    PySetObject *so = (PySetObject *)self;
    Py_uhash_t h, hash = 1927868237UL;
    setentry *entry;
    Py_ssize_t pos = 0;

    if (so->hash != -1)
        return so->hash;

    hash *= (Py_uhash_t)PySet_GET_SIZE(self) + 1;
    while (set_next(so, &pos, &entry)) {
        /* Work to increase the bit dispersion for closely spaced hash
           values.  The is important because some use cases have many
           combinations of a small number of elements with nearby
           hashes so that many distinct combinations collapse to only
           a handful of distinct hash values. */
        h = entry->hash;
        hash ^= ((h ^ 89869747UL) ^ (h << 16)) * 3644798167UL;
    }
    /* Make the final result spread-out in a different pattern
       than the algorithm for tuples or other python objects. */
    hash = hash * 69069U + 907133923UL;
    if (hash == -1)
        hash = 590923713UL;
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
    PyObject_GC_Del(si);
}

static int
setiter_traverse(setiterobject *si, visitproc visit, void *arg)
{
    Py_VISIT(si->si_set);
    return 0;
}

static PyObject *
setiter_len(setiterobject *si)
{
    Py_ssize_t len = 0;
    if (si->si_set != NULL && si->si_used == si->si_set->used)
        len = si->len;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *setiter_iternext(setiterobject *si);

static PyObject *
setiter_reduce(setiterobject *si)
{
    PyObject *list;
    setiterobject tmp;

    list = PyList_New(0);
    if (!list)
        return NULL;

    /* copy the iterator state */
    tmp = *si;
    Py_XINCREF(tmp.si_set);

    /* iterate the temporary into a list */
    for(;;) {
        PyObject *element = setiter_iternext(&tmp);
        if (element) {
            if (PyList_Append(list, element)) {
                Py_DECREF(element);
                Py_DECREF(list);
                Py_XDECREF(tmp.si_set);
                return NULL;
            }
            Py_DECREF(element);
        } else
            break;
    }
    Py_XDECREF(tmp.si_set);
    /* check for error */
    if (tmp.si_set != NULL) {
        /* we have an error */
        Py_DECREF(list);
        return NULL;
    }
    return Py_BuildValue("N(N)", _PyObject_GetBuiltin("iter"), list);
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef setiter_methods[] = {
    {"__length_hint__", (PyCFunction)setiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", (PyCFunction)setiter_reduce, METH_NOARGS, reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *setiter_iternext(setiterobject *si)
{
    PyObject *key;
    Py_ssize_t i, mask;
    setentry *entry;
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

PyTypeObject PySetIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "set_iterator",                             /* tp_name */
    sizeof(setiterobject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)setiter_dealloc,                /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)setiter_traverse,             /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)setiter_iternext,             /* tp_iternext */
    setiter_methods,                            /* tp_methods */
    0,
};

static PyObject *
set_iter(PySetObject *so)
{
    setiterobject *si = PyObject_GC_New(setiterobject, &PySetIter_Type);
    if (si == NULL)
        return NULL;
    Py_INCREF(so);
    si->si_set = so;
    si->si_used = so->used;
    si->si_pos = 0;
    si->len = so->used;
    _PyObject_GC_TRACK(si);
    return (PyObject *)si;
}

static int
set_update_internal(PySetObject *so, PyObject *other)
{
    PyObject *key, *it;

    if (PyAnySet_Check(other))
        return set_merge(so, other);

    if (PyDict_CheckExact(other)) {
        PyObject *value;
        Py_ssize_t pos = 0;
        Py_hash_t hash;
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
set_update(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;

    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        PyObject *other = PyTuple_GET_ITEM(args, i);
        if (set_update_internal(so, other) == -1)
            return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc,
"Update a set with the union of itself and others.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
    PySetObject *so = NULL;

    /* create PySetObject structure */
    so = (PySetObject *)type->tp_alloc(type, 0);
    if (so == NULL)
        return NULL;

    so->fill = 0;
    so->used = 0;
    so->mask = PySet_MINSIZE - 1;
    so->table = so->smalltable;
    so->lookup = set_lookkey_unicode;
    so->hash = -1;
    so->weakreflist = NULL;

    if (iterable != NULL) {
        if (set_update_internal(so, iterable) == -1) {
            Py_DECREF(so);
            return NULL;
        }
    }

    return (PyObject *)so;
}

static PyObject *
make_new_set_basetype(PyTypeObject *type, PyObject *iterable)
{
    if (type != &PySet_Type && type != &PyFrozenSet_Type) {
        if (PyType_IsSubtype(type, &PySet_Type))
            type = &PySet_Type;
        else
            type = &PyFrozenSet_Type;
    }
    return make_new_set(type, iterable);
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

int
PySet_ClearFreeList(void)
{
    return 0;
}

void
PySet_Fini(void)
{
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
    setentry *(*f)(PySetObject *so, PyObject *key, Py_ssize_t hash);
    setentry tab[PySet_MINSIZE];
    Py_hash_t h;

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
    return make_new_set_basetype(Py_TYPE(so), (PyObject *)so);
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
set_union(PySetObject *so, PyObject *args)
{
    PySetObject *result;
    PyObject *other;
    Py_ssize_t i;

    result = (PySetObject *)set_copy(so);
    if (result == NULL)
        return NULL;

    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        other = PyTuple_GET_ITEM(args, i);
        if ((PyObject *)so == other)
            continue;
        if (set_update_internal(result, other) == -1) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return (PyObject *)result;
}

PyDoc_STRVAR(union_doc,
 "Return the union of sets as a new set.\n\
\n\
(i.e. all elements that are in either set.)");

static PyObject *
set_or(PySetObject *so, PyObject *other)
{
    PySetObject *result;

    if (!PyAnySet_Check(so) || !PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;

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

static PyObject *
set_ior(PySetObject *so, PyObject *other)
{
    if (!PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;

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

    result = (PySetObject *)make_new_set_basetype(Py_TYPE(so), NULL);
    if (result == NULL)
        return NULL;

    if (PyAnySet_Check(other)) {
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
        Py_hash_t hash = PyObject_Hash(key);

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

static PyObject *
set_intersection_multi(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;
    PyObject *result = (PyObject *)so;

    if (PyTuple_GET_SIZE(args) == 0)
        return set_copy(so);

    Py_INCREF(so);
    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        PyObject *other = PyTuple_GET_ITEM(args, i);
        PyObject *newresult = set_intersection((PySetObject *)result, other);
        if (newresult == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        Py_DECREF(result);
        result = newresult;
    }
    return result;
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

static PyObject *
set_intersection_update_multi(PySetObject *so, PyObject *args)
{
    PyObject *tmp;

    tmp = set_intersection_multi(so, args);
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
    if (!PyAnySet_Check(so) || !PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    return set_intersection(so, other);
}

static PyObject *
set_iand(PySetObject *so, PyObject *other)
{
    PyObject *result;

    if (!PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
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
        Py_hash_t hash = PyObject_Hash(key);

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

    if (PyAnySet_Check(other)) {
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
set_difference_update(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;

    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        PyObject *other = PyTuple_GET_ITEM(args, i);
        if (set_difference_update_internal(so, other) == -1)
            return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another set from this set.");

static PyObject *
set_copy_and_difference(PySetObject *so, PyObject *other)
{
    PyObject *result;

    result = set_copy(so);
    if (result == NULL)
        return NULL;
    if (set_difference_update_internal((PySetObject *) result, other) != -1)
        return result;
    Py_DECREF(result);
    return NULL;
}

static PyObject *
set_difference(PySetObject *so, PyObject *other)
{
    PyObject *result;
    setentry *entry;
    Py_ssize_t pos = 0;

    if (!PyAnySet_Check(other)  && !PyDict_CheckExact(other)) {
        return set_copy_and_difference(so, other);
    }

    /* If len(so) much more than len(other), it's more efficient to simply copy
     * so and then iterate other looking for common elements. */
    if ((PySet_GET_SIZE(so) >> 2) > PyObject_Size(other)) {
        return set_copy_and_difference(so, other);
    }

    result = make_new_set_basetype(Py_TYPE(so), NULL);
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

    /* Iterate over so, checking for common elements in other. */
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

static PyObject *
set_difference_multi(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;
    PyObject *result, *other;

    if (PyTuple_GET_SIZE(args) == 0)
        return set_copy(so);

    other = PyTuple_GET_ITEM(args, 0);
    result = set_difference(so, other);
    if (result == NULL)
        return NULL;

    for (i=1 ; i<PyTuple_GET_SIZE(args) ; i++) {
        other = PyTuple_GET_ITEM(args, i);
        if (set_difference_update_internal((PySetObject *)result, other) == -1) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

PyDoc_STRVAR(difference_doc,
"Return the difference of two or more sets as a new set.\n\
\n\
(i.e. all elements that are in this set but not the others.)");
static PyObject *
set_sub(PySetObject *so, PyObject *other)
{
    if (!PyAnySet_Check(so) || !PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    return set_difference(so, other);
}

static PyObject *
set_isub(PySetObject *so, PyObject *other)
{
    if (!PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    if (set_difference_update_internal(so, other) == -1)
        return NULL;
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
        Py_hash_t hash;
        while (_PyDict_Next(other, &pos, &key, &value, &hash)) {
            setentry an_entry;

            Py_INCREF(key);
            an_entry.hash = hash;
            an_entry.key = key;

            rv = set_discard_entry(so, &an_entry);
            if (rv == -1) {
                Py_DECREF(key);
                return NULL;
            }
            if (rv == DISCARD_NOTFOUND) {
                if (set_add_entry(so, &an_entry) == -1) {
                    Py_DECREF(key);
                    return NULL;
                }
            }
            Py_DECREF(key);
        }
        Py_RETURN_NONE;
    }

    if (PyAnySet_Check(other)) {
        Py_INCREF(other);
        otherset = (PySetObject *)other;
    } else {
        otherset = (PySetObject *)make_new_set_basetype(Py_TYPE(so), other);
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

    otherset = (PySetObject *)make_new_set_basetype(Py_TYPE(so), other);
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
    if (!PyAnySet_Check(so) || !PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
    return set_symmetric_difference(so, other);
}

static PyObject *
set_ixor(PySetObject *so, PyObject *other)
{
    PyObject *result;

    if (!PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;
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

    if (!PyAnySet_Check(other)) {
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

static PyObject *
set_richcompare(PySetObject *v, PyObject *w, int op)
{
    PyObject *r1, *r2;

    if(!PyAnySet_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

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
    Py_RETURN_NOTIMPLEMENTED;
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
        if (!PySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return -1;
        PyErr_Clear();
        tmpkey = make_new_set(&PyFrozenSet_Type, key);
        if (tmpkey == NULL)
            return -1;
        rv = set_contains_key(so, tmpkey);
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
    PyObject *tmpkey;
    int rv;

    rv = set_discard_key(so, key);
    if (rv == -1) {
        if (!PySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
        tmpkey = make_new_set(&PyFrozenSet_Type, key);
        if (tmpkey == NULL)
            return NULL;
        rv = set_discard_key(so, tmpkey);
        Py_DECREF(tmpkey);
        if (rv == -1)
            return NULL;
    }

    if (rv == DISCARD_NOTFOUND) {
        _PyErr_SetKeyError(key);
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
    PyObject *tmpkey;
    int rv;

    rv = set_discard_key(so, key);
    if (rv == -1) {
        if (!PySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
        tmpkey = make_new_set(&PyFrozenSet_Type, key);
        if (tmpkey == NULL)
            return NULL;
        rv = set_discard_key(so, tmpkey);
        Py_DECREF(tmpkey);
        if (rv == -1)
            return NULL;
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
    _Py_IDENTIFIER(__dict__);

    keys = PySequence_List((PyObject *)so);
    if (keys == NULL)
        goto done;
    args = PyTuple_Pack(1, keys);
    if (args == NULL)
        goto done;
    dict = _PyObject_GetAttrId((PyObject *)so, &PyId___dict__);
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

static PyObject *
set_sizeof(PySetObject *so)
{
    Py_ssize_t res;

    res = sizeof(PySetObject);
    if (so->table != so->smalltable)
        res = res + (so->mask + 1) * sizeof(setentry);
    return PyLong_FromSsize_t(res);
}

PyDoc_STRVAR(sizeof_doc, "S.__sizeof__() -> size of S in memory, in bytes");
static int
set_init(PySetObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;

    if (!PyAnySet_Check(self))
        return -1;
    if (PySet_Check(self) && !_PyArg_NoKeywords("set()", kwds))
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
    set_len,                            /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)set_contains,           /* sq_contains */
};

/* set object ********************************************************/

#ifdef Py_DEBUG
static PyObject *test_c_api(PySetObject *so);

PyDoc_STRVAR(test_c_api_doc, "Exercises C API.  Returns True.\n\
All is well if assertions don't fail.");
#endif

static PyMethodDef set_methods[] = {
    {"add",             (PyCFunction)set_add,           METH_O,
     add_doc},
    {"clear",           (PyCFunction)set_clear,         METH_NOARGS,
     clear_doc},
    {"__contains__",(PyCFunction)set_direct_contains,           METH_O | METH_COEXIST,
     contains_doc},
    {"copy",            (PyCFunction)set_copy,          METH_NOARGS,
     copy_doc},
    {"discard",         (PyCFunction)set_discard,       METH_O,
     discard_doc},
    {"difference",      (PyCFunction)set_difference_multi,      METH_VARARGS,
     difference_doc},
    {"difference_update",       (PyCFunction)set_difference_update,     METH_VARARGS,
     difference_update_doc},
    {"intersection",(PyCFunction)set_intersection_multi,        METH_VARARGS,
     intersection_doc},
    {"intersection_update",(PyCFunction)set_intersection_update_multi,          METH_VARARGS,
     intersection_update_doc},
    {"isdisjoint",      (PyCFunction)set_isdisjoint,    METH_O,
     isdisjoint_doc},
    {"issubset",        (PyCFunction)set_issubset,      METH_O,
     issubset_doc},
    {"issuperset",      (PyCFunction)set_issuperset,    METH_O,
     issuperset_doc},
    {"pop",             (PyCFunction)set_pop,           METH_NOARGS,
     pop_doc},
    {"__reduce__",      (PyCFunction)set_reduce,        METH_NOARGS,
     reduce_doc},
    {"remove",          (PyCFunction)set_remove,        METH_O,
     remove_doc},
    {"__sizeof__",      (PyCFunction)set_sizeof,        METH_NOARGS,
     sizeof_doc},
    {"symmetric_difference",(PyCFunction)set_symmetric_difference,      METH_O,
     symmetric_difference_doc},
    {"symmetric_difference_update",(PyCFunction)set_symmetric_difference_update,        METH_O,
     symmetric_difference_update_doc},
#ifdef Py_DEBUG
    {"test_c_api",      (PyCFunction)test_c_api,        METH_NOARGS,
     test_c_api_doc},
#endif
    {"union",           (PyCFunction)set_union,         METH_VARARGS,
     union_doc},
    {"update",          (PyCFunction)set_update,        METH_VARARGS,
     update_doc},
    {NULL,              NULL}   /* sentinel */
};

static PyNumberMethods set_as_number = {
    0,                                  /*nb_add*/
    (binaryfunc)set_sub,                /*nb_subtract*/
    0,                                  /*nb_multiply*/
    0,                                  /*nb_remainder*/
    0,                                  /*nb_divmod*/
    0,                                  /*nb_power*/
    0,                                  /*nb_negative*/
    0,                                  /*nb_positive*/
    0,                                  /*nb_absolute*/
    0,                                  /*nb_bool*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    (binaryfunc)set_and,                /*nb_and*/
    (binaryfunc)set_xor,                /*nb_xor*/
    (binaryfunc)set_or,                 /*nb_or*/
    0,                                  /*nb_int*/
    0,                                  /*nb_reserved*/
    0,                                  /*nb_float*/
    0,                                  /*nb_inplace_add*/
    (binaryfunc)set_isub,               /*nb_inplace_subtract*/
    0,                                  /*nb_inplace_multiply*/
    0,                                  /*nb_inplace_remainder*/
    0,                                  /*nb_inplace_power*/
    0,                                  /*nb_inplace_lshift*/
    0,                                  /*nb_inplace_rshift*/
    (binaryfunc)set_iand,               /*nb_inplace_and*/
    (binaryfunc)set_ixor,               /*nb_inplace_xor*/
    (binaryfunc)set_ior,                /*nb_inplace_or*/
};

PyDoc_STRVAR(set_doc,
"set() -> new empty set object\n\
set(iterable) -> new set object\n\
\n\
Build an unordered collection of unique elements.");

PyTypeObject PySet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "set",                              /* tp_name */
    sizeof(PySetObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)set_dealloc,            /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    (reprfunc)set_repr,                 /* tp_repr */
    &set_as_number,                     /* tp_as_number */
    &set_as_sequence,                   /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    PyObject_HashNotImplemented,        /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    set_doc,                            /* tp_doc */
    (traverseproc)set_traverse,         /* tp_traverse */
    (inquiry)set_clear_internal,        /* tp_clear */
    (richcmpfunc)set_richcompare,       /* tp_richcompare */
    offsetof(PySetObject, weakreflist), /* tp_weaklistoffset */
    (getiterfunc)set_iter,              /* tp_iter */
    0,                                  /* tp_iternext */
    set_methods,                        /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)set_init,                 /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    set_new,                            /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};

/* frozenset object ********************************************************/


static PyMethodDef frozenset_methods[] = {
    {"__contains__",(PyCFunction)set_direct_contains,           METH_O | METH_COEXIST,
     contains_doc},
    {"copy",            (PyCFunction)frozenset_copy,    METH_NOARGS,
     copy_doc},
    {"difference",      (PyCFunction)set_difference_multi,      METH_VARARGS,
     difference_doc},
    {"intersection",(PyCFunction)set_intersection_multi,        METH_VARARGS,
     intersection_doc},
    {"isdisjoint",      (PyCFunction)set_isdisjoint,    METH_O,
     isdisjoint_doc},
    {"issubset",        (PyCFunction)set_issubset,      METH_O,
     issubset_doc},
    {"issuperset",      (PyCFunction)set_issuperset,    METH_O,
     issuperset_doc},
    {"__reduce__",      (PyCFunction)set_reduce,        METH_NOARGS,
     reduce_doc},
    {"__sizeof__",      (PyCFunction)set_sizeof,        METH_NOARGS,
     sizeof_doc},
    {"symmetric_difference",(PyCFunction)set_symmetric_difference,      METH_O,
     symmetric_difference_doc},
    {"union",           (PyCFunction)set_union,         METH_VARARGS,
     union_doc},
    {NULL,              NULL}   /* sentinel */
};

static PyNumberMethods frozenset_as_number = {
    0,                                  /*nb_add*/
    (binaryfunc)set_sub,                /*nb_subtract*/
    0,                                  /*nb_multiply*/
    0,                                  /*nb_remainder*/
    0,                                  /*nb_divmod*/
    0,                                  /*nb_power*/
    0,                                  /*nb_negative*/
    0,                                  /*nb_positive*/
    0,                                  /*nb_absolute*/
    0,                                  /*nb_bool*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    (binaryfunc)set_and,                /*nb_and*/
    (binaryfunc)set_xor,                /*nb_xor*/
    (binaryfunc)set_or,                 /*nb_or*/
};

PyDoc_STRVAR(frozenset_doc,
"frozenset() -> empty frozenset object\n\
frozenset(iterable) -> frozenset object\n\
\n\
Build an immutable unordered collection of unique elements.");

PyTypeObject PyFrozenSet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frozenset",                        /* tp_name */
    sizeof(PySetObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)set_dealloc,            /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    (reprfunc)set_repr,                 /* tp_repr */
    &frozenset_as_number,               /* tp_as_number */
    &set_as_sequence,                   /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    frozenset_hash,                     /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,            /* tp_flags */
    frozenset_doc,                      /* tp_doc */
    (traverseproc)set_traverse,         /* tp_traverse */
    (inquiry)set_clear_internal,        /* tp_clear */
    (richcmpfunc)set_richcompare,       /* tp_richcompare */
    offsetof(PySetObject, weakreflist),         /* tp_weaklistoffset */
    (getiterfunc)set_iter,              /* tp_iter */
    0,                                  /* tp_iternext */
    frozenset_methods,                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    frozenset_new,                      /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
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
    return make_new_set(&PyFrozenSet_Type, iterable);
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
    if (!PySet_Check(set)) {
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
    if (!PySet_Check(set)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return set_discard_key((PySetObject *)set, key);
}

int
PySet_Add(PyObject *anyset, PyObject *key)
{
    if (!PySet_Check(anyset) &&
        (!PyFrozenSet_Check(anyset) || Py_REFCNT(anyset) != 1)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return set_add_key((PySetObject *)anyset, key);
}

int
_PySet_NextEntry(PyObject *set, Py_ssize_t *pos, PyObject **key, Py_hash_t *hash)
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
    if (!PySet_Check(set)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return set_pop((PySetObject *)set);
}

int
_PySet_Update(PyObject *set, PyObject *iterable)
{
    if (!PySet_Check(set)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return set_update_internal((PySetObject *)set, iterable);
}

/* Exported for the gdb plugin's benefit. */
PyObject *_PySet_Dummy = dummy;

#ifdef Py_DEBUG

/* Test code to be called with any three element set.
   Returns True and original set is restored. */

#define assertRaises(call_return_value, exception)              \
    do {                                                        \
        assert(call_return_value);                              \
        assert(PyErr_ExceptionMatches(exception));              \
        PyErr_Clear();                                          \
    } while(0)

static PyObject *
test_c_api(PySetObject *so)
{
    Py_ssize_t count;
    char *s;
    Py_ssize_t i;
    PyObject *elem=NULL, *dup=NULL, *t, *f, *dup2, *x=NULL;
    PyObject *ob = (PyObject *)so;
    Py_hash_t hash;
    PyObject *str;

    /* Verify preconditions */
    assert(PyAnySet_Check(ob));
    assert(PyAnySet_CheckExact(ob));
    assert(!PyFrozenSet_CheckExact(ob));

    /* so.clear(); so |= set("abc"); */
    str = PyUnicode_FromString("abc");
    if (str == NULL)
        return NULL;
    set_clear_internal(so);
    if (set_update_internal(so, str) == -1) {
        Py_DECREF(str);
        return NULL;
    }
    Py_DECREF(str);

    /* Exercise type/size checks */
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
    assert(PySet_Add(f, elem) == 0);
    Py_INCREF(f);
    assertRaises(PySet_Add(f, elem) == -1, PyExc_SystemError);
    Py_DECREF(f);
    Py_DECREF(f);

    /* Exercise direct iteration */
    i = 0, count = 0;
    while (_PySet_NextEntry((PyObject *)dup, &i, &x, &hash)) {
        s = _PyUnicode_AsString(x);
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

/***** Dummy Struct  *************************************************/

static PyObject *
dummy_repr(PyObject *op)
{
    return PyUnicode_FromString("<dummy key>");
}

static void
dummy_dealloc(PyObject* ignore)
{
    Py_FatalError("deallocating <dummy key>");
}

static PyTypeObject _PySetDummy_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "<dummy key> type",
    0,
    0,
    dummy_dealloc,      /*tp_dealloc*/ /*never called*/
    0,                  /*tp_print*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_reserved*/
    dummy_repr,         /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /*tp_hash */
    0,                  /*tp_call */
    0,                  /*tp_str */
    0,                  /*tp_getattro */
    0,                  /*tp_setattro */
    0,                  /*tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /*tp_flags */
};

static PyObject _dummy_struct = {
  _PyObject_EXTRA_INIT
  2, &_PySetDummy_Type
};

