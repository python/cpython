
/* set object implementation

   Written and maintained by Raymond D. Hettinger <python@rcn.com>
   Derived from Lib/sets.py and Objects/dictobject.c.

   The basic lookup function used by all operations.
   This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.

   The initial probe index is computed as hash mod the table size.
   Subsequent probe indices are computed as explained in Objects/dictobject.c.

   To improve cache locality, each probe inspects a series of consecutive
   nearby entries before moving on to probes elsewhere in memory.  This leaves
   us with a hybrid of linear probing and randomized probing.  The linear probing
   reduces the cost of hash collisions because consecutive memory accesses
   tend to be much cheaper than scattered probes.  After LINEAR_PROBES steps,
   we then use more of the upper bits from the hash value and apply a simple
   linear congruential random number generator.  This helps break-up long
   chains of collisions.

   All arithmetic on hash should ignore overflow.

   Unlike the dictionary implementation, the lookkey function can return
   NULL if the rich comparison returns an error.

   Use cases for sets differ considerably from dictionaries where looked-up
   keys are more likely to be present.  In contrast, sets are primarily
   about membership testing where the presence of an element is not known in
   advance.  Accordingly, the set implementation needs to optimize for both
   the found and not-found case.
*/

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_dict.h"          // _PyDict_Contains_KnownHash()
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_pyerrors.h"      // _PyErr_SetKeyError()
#include "pycore_setobject.h"     // _PySet_NextEntry() definition
#include <stddef.h>               // offsetof()

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
    setentry *table;
    setentry *entry;
    size_t perturb = hash;
    size_t mask = so->mask;
    size_t i = (size_t)hash & mask; /* Unsigned for defined overflow behavior */
    int probes;
    int cmp;

    while (1) {
        entry = &so->table[i];
        probes = (i + LINEAR_PROBES <= mask) ? LINEAR_PROBES: 0;
        do {
            if (entry->hash == 0 && entry->key == NULL)
                return entry;
            if (entry->hash == hash) {
                PyObject *startkey = entry->key;
                assert(startkey != dummy);
                if (startkey == key)
                    return entry;
                if (PyUnicode_CheckExact(startkey)
                    && PyUnicode_CheckExact(key)
                    && _PyUnicode_EQ(startkey, key))
                    return entry;
                table = so->table;
                Py_INCREF(startkey);
                cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
                Py_DECREF(startkey);
                if (cmp < 0)
                    return NULL;
                if (table != so->table || entry->key != startkey)
                    return set_lookkey(so, key, hash);
                if (cmp > 0)
                    return entry;
                mask = so->mask;
            }
            entry++;
        } while (probes--);
        perturb >>= PERTURB_SHIFT;
        i = (i * 5 + 1 + perturb) & mask;
    }
}

static int set_table_resize(PySetObject *, Py_ssize_t);

static int
set_add_entry(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *table;
    setentry *freeslot;
    setentry *entry;
    size_t perturb;
    size_t mask;
    size_t i;                       /* Unsigned for defined overflow behavior */
    int probes;
    int cmp;

    /* Pre-increment is necessary to prevent arbitrary code in the rich
       comparison from deallocating the key just before the insertion. */
    Py_INCREF(key);

  restart:

    mask = so->mask;
    i = (size_t)hash & mask;
    freeslot = NULL;
    perturb = hash;

    while (1) {
        entry = &so->table[i];
        probes = (i + LINEAR_PROBES <= mask) ? LINEAR_PROBES: 0;
        do {
            if (entry->hash == 0 && entry->key == NULL)
                goto found_unused_or_dummy;
            if (entry->hash == hash) {
                PyObject *startkey = entry->key;
                assert(startkey != dummy);
                if (startkey == key)
                    goto found_active;
                if (PyUnicode_CheckExact(startkey)
                    && PyUnicode_CheckExact(key)
                    && _PyUnicode_EQ(startkey, key))
                    goto found_active;
                table = so->table;
                Py_INCREF(startkey);
                cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
                Py_DECREF(startkey);
                if (cmp > 0)
                    goto found_active;
                if (cmp < 0)
                    goto comparison_error;
                if (table != so->table || entry->key != startkey)
                    goto restart;
                mask = so->mask;
            }
            else if (entry->hash == -1) {
                assert (entry->key == dummy);
                freeslot = entry;
            }
            entry++;
        } while (probes--);
        perturb >>= PERTURB_SHIFT;
        i = (i * 5 + 1 + perturb) & mask;
    }

  found_unused_or_dummy:
    if (freeslot == NULL)
        goto found_unused;
    so->used++;
    freeslot->key = key;
    freeslot->hash = hash;
    return 0;

  found_unused:
    so->fill++;
    so->used++;
    entry->key = key;
    entry->hash = hash;
    if ((size_t)so->fill*5 < mask*3)
        return 0;
    return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);

  found_active:
    Py_DECREF(key);
    return 0;

  comparison_error:
    Py_DECREF(key);
    return -1;
}

/*
Internal routine used by set_table_resize() to insert an item which is
known to be absent from the set.  Besides the performance benefit,
there is also safety benefit since using set_add_entry() risks making
a callback in the middle of a set_table_resize(), see issue 1456209.
The caller is responsible for updating the key's reference count and
the setobject's fill and used fields.
*/
static void
set_insert_clean(setentry *table, size_t mask, PyObject *key, Py_hash_t hash)
{
    setentry *entry;
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    size_t j;

    while (1) {
        entry = &table[i];
        if (entry->key == NULL)
            goto found_null;
        if (i + LINEAR_PROBES <= mask) {
            for (j = 0; j < LINEAR_PROBES; j++) {
                entry++;
                if (entry->key == NULL)
                    goto found_null;
            }
        }
        perturb >>= PERTURB_SHIFT;
        i = (i * 5 + 1 + perturb) & mask;
    }
  found_null:
    entry->key = key;
    entry->hash = hash;
}

/* ======== End logic for probing the hash table ========================== */
/* ======================================================================== */

/*
Restructure the table by allocating a new table and reinserting all
keys again.  When entries have been deleted, the new table may
actually be smaller than the old one.
*/
static int
set_table_resize(PySetObject *so, Py_ssize_t minused)
{
    setentry *oldtable, *newtable, *entry;
    Py_ssize_t oldmask = so->mask;
    size_t newmask;
    int is_oldtable_malloced;
    setentry small_copy[PySet_MINSIZE];

    assert(minused >= 0);

    /* Find the smallest table size > minused. */
    /* XXX speed-up with intrinsics */
    size_t newsize = PySet_MINSIZE;
    while (newsize <= (size_t)minused) {
        newsize <<= 1; // The largest possible value is PY_SSIZE_T_MAX + 1.
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
    memset(newtable, 0, sizeof(setentry) * newsize);
    so->mask = newsize - 1;
    so->table = newtable;

    /* Copy the data over; this is refcount-neutral for active entries;
       dummy entries aren't copied over, of course */
    newmask = (size_t)so->mask;
    if (so->fill == so->used) {
        for (entry = oldtable; entry <= oldtable + oldmask; entry++) {
            if (entry->key != NULL) {
                set_insert_clean(newtable, newmask, entry->key, entry->hash);
            }
        }
    } else {
        so->fill = so->used;
        for (entry = oldtable; entry <= oldtable + oldmask; entry++) {
            if (entry->key != NULL && entry->key != dummy) {
                set_insert_clean(newtable, newmask, entry->key, entry->hash);
            }
        }
    }

    if (is_oldtable_malloced)
        PyMem_Free(oldtable);
    return 0;
}

static int
set_contains_entry(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *entry;

    entry = set_lookkey(so, key, hash);
    if (entry != NULL)
        return entry->key != NULL;
    return -1;
}

#define DISCARD_NOTFOUND 0
#define DISCARD_FOUND 1

static int
set_discard_entry(PySetObject *so, PyObject *key, Py_hash_t hash)
{
    setentry *entry;
    PyObject *old_key;

    entry = set_lookkey(so, key, hash);
    if (entry == NULL)
        return -1;
    if (entry->key == NULL)
        return DISCARD_NOTFOUND;
    old_key = entry->key;
    entry->key = dummy;
    entry->hash = -1;
    so->used--;
    Py_DECREF(old_key);
    return DISCARD_FOUND;
}

static int
set_add_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;

    if (!PyUnicode_CheckExact(key) ||
        (hash = _PyASCIIObject_CAST(key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    return set_add_entry(so, key, hash);
}

static int
set_contains_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;

    if (!PyUnicode_CheckExact(key) ||
        (hash = _PyASCIIObject_CAST(key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    return set_contains_entry(so, key, hash);
}

static int
set_discard_key(PySetObject *so, PyObject *key)
{
    Py_hash_t hash;

    if (!PyUnicode_CheckExact(key) ||
        (hash = _PyASCIIObject_CAST(key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    return set_discard_entry(so, key, hash);
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
    setentry *entry;
    setentry *table = so->table;
    Py_ssize_t fill = so->fill;
    Py_ssize_t used = so->used;
    int table_is_malloced = table != so->smalltable;
    setentry small_copy[PySet_MINSIZE];

    assert (PyAnySet_Check(so));
    assert(table != NULL);

    /* This is delicate.  During the process of clearing the set,
     * decrefs can cause the set to mutate.  To avoid fatal confusion
     * (voice of experience), we have to make the set empty before
     * clearing the slots, and never refer to anything via so->ref while
     * clearing.
     */
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
    for (entry = table; used > 0; entry++) {
        if (entry->key && entry->key != dummy) {
            used--;
            Py_DECREF(entry->key);
        }
    }

    if (table_is_malloced)
        PyMem_Free(table);
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
    setentry *entry;

    assert (PyAnySet_Check(so));
    i = *pos_ptr;
    assert(i >= 0);
    mask = so->mask;
    entry = &so->table[i];
    while (i <= mask && (entry->key == NULL || entry->key == dummy)) {
        i++;
        entry++;
    }
    *pos_ptr = i+1;
    if (i > mask)
        return 0;
    assert(entry != NULL);
    *entry_ptr = entry;
    return 1;
}

static void
set_dealloc(PySetObject *so)
{
    setentry *entry;
    Py_ssize_t used = so->used;

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(so);
    Py_TRASHCAN_BEGIN(so, set_dealloc)
    if (so->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) so);

    for (entry = so->table; used > 0; entry++) {
        if (entry->key && entry->key != dummy) {
                used--;
                Py_DECREF(entry->key);
        }
    }
    if (so->table != so->smalltable)
        PyMem_Free(so->table);
    Py_TYPE(so)->tp_free(so);
    Py_TRASHCAN_END
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

    if (!PySet_CheckExact(so))
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
    Py_ssize_t i;
    setentry *so_entry;
    setentry *other_entry;

    assert (PyAnySet_Check(so));
    assert (PyAnySet_Check(otherset));

    other = (PySetObject*)otherset;
    if (other == so || other->used == 0)
        /* a.update(a) or a.update(set()); nothing to do */
        return 0;
    /* Do one big resize at the start, rather than
     * incrementally resizing as we insert new keys.  Expect
     * that there will be no (or few) overlapping keys.
     */
    if ((so->fill + other->used)*5 >= so->mask*3) {
        if (set_table_resize(so, (so->used + other->used)*2) != 0)
            return -1;
    }
    so_entry = so->table;
    other_entry = other->table;

    /* If our table is empty, and both tables have the same size, and
       there are no dummies to eliminate, then just copy the pointers. */
    if (so->fill == 0 && so->mask == other->mask && other->fill == other->used) {
        for (i = 0; i <= other->mask; i++, so_entry++, other_entry++) {
            key = other_entry->key;
            if (key != NULL) {
                assert(so_entry->key == NULL);
                so_entry->key = Py_NewRef(key);
                so_entry->hash = other_entry->hash;
            }
        }
        so->fill = other->fill;
        so->used = other->used;
        return 0;
    }

    /* If our table is empty, we can use set_insert_clean() */
    if (so->fill == 0) {
        setentry *newtable = so->table;
        size_t newmask = (size_t)so->mask;
        so->fill = other->used;
        so->used = other->used;
        for (i = other->mask + 1; i > 0 ; i--, other_entry++) {
            key = other_entry->key;
            if (key != NULL && key != dummy) {
                set_insert_clean(newtable, newmask, Py_NewRef(key),
                                 other_entry->hash);
            }
        }
        return 0;
    }

    /* We can't assure there are no duplicates, so do normal insertions */
    for (i = 0; i <= other->mask; i++) {
        other_entry = &other->table[i];
        key = other_entry->key;
        if (key != NULL && key != dummy) {
            if (set_add_entry(so, key, other_entry->hash))
                return -1;
        }
    }
    return 0;
}

static PyObject *
set_pop(PySetObject *so, PyObject *Py_UNUSED(ignored))
{
    /* Make sure the search finger is in bounds */
    setentry *entry = so->table + (so->finger & so->mask);
    setentry *limit = so->table + so->mask;
    PyObject *key;

    if (so->used == 0) {
        PyErr_SetString(PyExc_KeyError, "pop from an empty set");
        return NULL;
    }
    while (entry->key == NULL || entry->key==dummy) {
        entry++;
        if (entry > limit)
            entry = so->table;
    }
    key = entry->key;
    entry->key = dummy;
    entry->hash = -1;
    so->used--;
    so->finger = entry - so->table + 1;   /* next place to start */
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

/* Work to increase the bit dispersion for closely spaced hash values.
   This is important because some use cases have many combinations of a
   small number of elements with nearby hashes so that many distinct
   combinations collapse to only a handful of distinct hash values. */

static Py_uhash_t
_shuffle_bits(Py_uhash_t h)
{
    return ((h ^ 89869747UL) ^ (h << 16)) * 3644798167UL;
}

/* Most of the constants in this hash algorithm are randomly chosen
   large primes with "interesting bit patterns" and that passed tests
   for good collision statistics on a variety of problematic datasets
   including powersets and graph structures (such as David Eppstein's
   graph recipes in Lib/test/test_set.py) */

static Py_hash_t
frozenset_hash(PyObject *self)
{
    PySetObject *so = (PySetObject *)self;
    Py_uhash_t hash = 0;
    setentry *entry;

    if (so->hash != -1)
        return so->hash;

    /* Xor-in shuffled bits from every entry's hash field because xor is
       commutative and a frozenset hash should be independent of order.

       For speed, include null entries and dummy entries and then
       subtract out their effect afterwards so that the final hash
       depends only on active entries.  This allows the code to be
       vectorized by the compiler and it saves the unpredictable
       branches that would arise when trying to exclude null and dummy
       entries on every iteration. */

    for (entry = so->table; entry <= &so->table[so->mask]; entry++)
        hash ^= _shuffle_bits(entry->hash);

    /* Remove the effect of an odd number of NULL entries */
    if ((so->mask + 1 - so->fill) & 1)
        hash ^= _shuffle_bits(0);

    /* Remove the effect of an odd number of dummy entries */
    if ((so->fill - so->used) & 1)
        hash ^= _shuffle_bits(-1);

    /* Factor in the number of active entries */
    hash ^= ((Py_uhash_t)PySet_GET_SIZE(self) + 1) * 1927868237UL;

    /* Disperse patterns arising in nested frozensets */
    hash ^= (hash >> 11) ^ (hash >> 25);
    hash = hash * 69069U + 907133923UL;

    /* -1 is reserved as an error code */
    if (hash == (Py_uhash_t)-1)
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
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(si);
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
setiter_len(setiterobject *si, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t len = 0;
    if (si->si_set != NULL && si->si_used == si->si_set->used)
        len = si->len;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *setiter_iternext(setiterobject *si);

static PyObject *
setiter_reduce(setiterobject *si, PyObject *Py_UNUSED(ignored))
{
    /* copy the iterator state */
    setiterobject tmp = *si;
    Py_XINCREF(tmp.si_set);

    /* iterate the temporary into a list */
    PyObject *list = PySequence_List((PyObject*)&tmp);
    Py_XDECREF(tmp.si_set);
    if (list == NULL) {
        return NULL;
    }
    return Py_BuildValue("N(N)", _PyEval_GetBuiltin(&_Py_ID(iter)), list);
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
    return Py_NewRef(key);

fail:
    si->si_set = NULL;
    Py_DECREF(so);
    return NULL;
}

PyTypeObject PySetIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "set_iterator",                             /* tp_name */
    sizeof(setiterobject),                      /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)setiter_dealloc,                /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
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
    si->si_set = (PySetObject*)Py_NewRef(so);
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
        Py_ssize_t dictsize = PyDict_GET_SIZE(other);

        /* Do one big resize at the start, rather than
        * incrementally resizing as we insert new keys.  Expect
        * that there will be no (or few) overlapping keys.
        */
        if (dictsize < 0)
            return -1;
        if ((so->fill + dictsize)*5 >= so->mask*3) {
            if (set_table_resize(so, (so->used + dictsize)*2) != 0)
                return -1;
        }
        while (_PyDict_Next(other, &pos, &key, &value, &hash)) {
            if (set_add_entry(so, key, hash))
                return -1;
        }
        return 0;
    }

    it = PyObject_GetIter(other);
    if (it == NULL)
        return -1;

    while ((key = PyIter_Next(it)) != NULL) {
        if (set_add_key(so, key)) {
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
        if (set_update_internal(so, other))
            return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc,
"update($self, /, *others)\n\
--\n\
\n\
Update the set, adding elements from all others.");

/* XXX Todo:
   If aligned memory allocations become available, make the
   set object 64 byte aligned so that most of the fields
   can be retrieved or updated in a single cache line.
*/

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
    assert(PyType_Check(type));
    PySetObject *so;

    so = (PySetObject *)type->tp_alloc(type, 0);
    if (so == NULL)
        return NULL;

    so->fill = 0;
    so->used = 0;
    so->mask = PySet_MINSIZE - 1;
    so->table = so->smalltable;
    so->hash = -1;
    so->finger = 0;
    so->weakreflist = NULL;

    if (iterable != NULL) {
        if (set_update_internal(so, iterable)) {
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

static PyObject *
make_new_frozenset(PyTypeObject *type, PyObject *iterable)
{
    if (type != &PyFrozenSet_Type) {
        return make_new_set(type, iterable);
    }

    if (iterable != NULL && PyFrozenSet_CheckExact(iterable)) {
        /* frozenset(f) is idempotent */
        return Py_NewRef(iterable);
    }
    return make_new_set(type, iterable);
}

static PyObject *
frozenset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;

    if ((type == &PyFrozenSet_Type ||
         type->tp_init == PyFrozenSet_Type.tp_init) &&
        !_PyArg_NoKeywords("frozenset", kwds)) {
        return NULL;
    }

    if (!PyArg_UnpackTuple(args, type->tp_name, 0, 1, &iterable)) {
        return NULL;
    }

    return make_new_frozenset(type, iterable);
}

static PyObject *
frozenset_vectorcall(PyObject *type, PyObject * const*args,
                     size_t nargsf, PyObject *kwnames)
{
    if (!_PyArg_NoKwnames("frozenset", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("frozenset", nargs, 0, 1)) {
        return NULL;
    }

    PyObject *iterable = (nargs ? args[0] : NULL);
    return make_new_frozenset(_PyType_CAST(type), iterable);
}

static PyObject *
set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return make_new_set(type, NULL);
}

/* set_swap_bodies() switches the contents of any two sets by moving their
   internal data pointers and, if needed, copying the internal smalltables.
   Semantically equivalent to:

     t=set(a); a.clear(); a.update(b); b.clear(); b.update(t); del t

   The function always succeeds and it leaves both objects in a stable state.
   Useful for operations that update in-place (by allowing an intermediate
   result to be swapped into one of the original inputs).
*/

static void
set_swap_bodies(PySetObject *a, PySetObject *b)
{
    Py_ssize_t t;
    setentry *u;
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
set_copy(PySetObject *so, PyObject *Py_UNUSED(ignored))
{
    return make_new_set_basetype(Py_TYPE(so), (PyObject *)so);
}

static PyObject *
frozenset_copy(PySetObject *so, PyObject *Py_UNUSED(ignored))
{
    if (PyFrozenSet_CheckExact(so)) {
        return Py_NewRef(so);
    }
    return set_copy(so, NULL);
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a set.");

static PyObject *
set_clear(PySetObject *so, PyObject *Py_UNUSED(ignored))
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

    result = (PySetObject *)set_copy(so, NULL);
    if (result == NULL)
        return NULL;

    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        other = PyTuple_GET_ITEM(args, i);
        if ((PyObject *)so == other)
            continue;
        if (set_update_internal(result, other)) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return (PyObject *)result;
}

PyDoc_STRVAR(union_doc,
"union($self, /, *others)\n\
--\n\
\n\
Return a new set with elements from the set and all others.");

static PyObject *
set_or(PySetObject *so, PyObject *other)
{
    PySetObject *result;

    if (!PyAnySet_Check(so) || !PyAnySet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;

    result = (PySetObject *)set_copy(so, NULL);
    if (result == NULL)
        return NULL;
    if ((PyObject *)so == other)
        return (PyObject *)result;
    if (set_update_internal(result, other)) {
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

    if (set_update_internal(so, other))
        return NULL;
    return Py_NewRef(so);
}

static PyObject *
set_intersection(PySetObject *so, PyObject *other)
{
    PySetObject *result;
    PyObject *key, *it, *tmp;
    Py_hash_t hash;
    int rv;

    if ((PyObject *)so == other)
        return set_copy(so, NULL);

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
            key = entry->key;
            hash = entry->hash;
            Py_INCREF(key);
            rv = set_contains_entry(so, key, hash);
            if (rv < 0) {
                Py_DECREF(result);
                Py_DECREF(key);
                return NULL;
            }
            if (rv) {
                if (set_add_entry(result, key, hash)) {
                    Py_DECREF(result);
                    Py_DECREF(key);
                    return NULL;
                }
            }
            Py_DECREF(key);
        }
        return (PyObject *)result;
    }

    it = PyObject_GetIter(other);
    if (it == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    while ((key = PyIter_Next(it)) != NULL) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            goto error;
        rv = set_contains_entry(so, key, hash);
        if (rv < 0)
            goto error;
        if (rv) {
            if (set_add_entry(result, key, hash))
                goto error;
            if (PySet_GET_SIZE(result) >= PySet_GET_SIZE(so)) {
                Py_DECREF(key);
                break;
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
  error:
    Py_DECREF(it);
    Py_DECREF(result);
    Py_DECREF(key);
    return NULL;
}

static PyObject *
set_intersection_multi(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;

    if (PyTuple_GET_SIZE(args) == 0)
        return set_copy(so, NULL);

    PyObject *result = Py_NewRef(so);
    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        PyObject *other = PyTuple_GET_ITEM(args, i);
        PyObject *newresult = set_intersection((PySetObject *)result, other);
        if (newresult == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        Py_SETREF(result, newresult);
    }
    return result;
}

PyDoc_STRVAR(intersection_doc,
"intersection($self, /, *others)\n\
--\n\
\n\
Return a new set with elements common to the set and all others.");

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
"intersection_update($self, /, *others)\n\
--\n\
\n\
Update the set, keeping only elements found in it and all others.");

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
    return Py_NewRef(so);
}

static PyObject *
set_isdisjoint(PySetObject *so, PyObject *other)
{
    PyObject *key, *it, *tmp;
    int rv;

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
            PyObject *key = entry->key;
            Py_INCREF(key);
            rv = set_contains_entry(so, key, entry->hash);
            Py_DECREF(key);
            if (rv < 0) {
                return NULL;
            }
            if (rv) {
                Py_RETURN_FALSE;
            }
        }
        Py_RETURN_TRUE;
    }

    it = PyObject_GetIter(other);
    if (it == NULL)
        return NULL;

    while ((key = PyIter_Next(it)) != NULL) {
        rv = set_contains_key(so, key);
        Py_DECREF(key);
        if (rv < 0) {
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

        /* Optimization:  When the other set is more than 8 times
           larger than the base set, replace the other set with
           intersection of the two sets.
        */
        if ((PySet_GET_SIZE(other) >> 3) > PySet_GET_SIZE(so)) {
            other = set_intersection(so, other);
            if (other == NULL)
                return -1;
        } else {
            Py_INCREF(other);
        }

        while (set_next((PySetObject *)other, &pos, &entry)) {
            PyObject *key = entry->key;
            Py_INCREF(key);
            if (set_discard_entry(so, key, entry->hash) < 0) {
                Py_DECREF(other);
                Py_DECREF(key);
                return -1;
            }
            Py_DECREF(key);
        }

        Py_DECREF(other);
    } else {
        PyObject *key, *it;
        it = PyObject_GetIter(other);
        if (it == NULL)
            return -1;

        while ((key = PyIter_Next(it)) != NULL) {
            if (set_discard_key(so, key) < 0) {
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
    /* If more than 1/4th are dummies, then resize them away. */
    if ((size_t)(so->fill - so->used) <= (size_t)so->mask / 4)
        return 0;
    return set_table_resize(so, so->used>50000 ? so->used*2 : so->used*4);
}

static PyObject *
set_difference_update(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;

    for (i=0 ; i<PyTuple_GET_SIZE(args) ; i++) {
        PyObject *other = PyTuple_GET_ITEM(args, i);
        if (set_difference_update_internal(so, other))
            return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"difference_update($self, /, *others)\n\
--\n\
\n\
Update the set, removing elements found in others.");

static PyObject *
set_copy_and_difference(PySetObject *so, PyObject *other)
{
    PyObject *result;

    result = set_copy(so, NULL);
    if (result == NULL)
        return NULL;
    if (set_difference_update_internal((PySetObject *) result, other) == 0)
        return result;
    Py_DECREF(result);
    return NULL;
}

static PyObject *
set_difference(PySetObject *so, PyObject *other)
{
    PyObject *result;
    PyObject *key;
    Py_hash_t hash;
    setentry *entry;
    Py_ssize_t pos = 0, other_size;
    int rv;

    if (PyAnySet_Check(other)) {
        other_size = PySet_GET_SIZE(other);
    }
    else if (PyDict_CheckExact(other)) {
        other_size = PyDict_GET_SIZE(other);
    }
    else {
        return set_copy_and_difference(so, other);
    }

    /* If len(so) much more than len(other), it's more efficient to simply copy
     * so and then iterate other looking for common elements. */
    if ((PySet_GET_SIZE(so) >> 2) > other_size) {
        return set_copy_and_difference(so, other);
    }

    result = make_new_set_basetype(Py_TYPE(so), NULL);
    if (result == NULL)
        return NULL;

    if (PyDict_CheckExact(other)) {
        while (set_next(so, &pos, &entry)) {
            key = entry->key;
            hash = entry->hash;
            Py_INCREF(key);
            rv = _PyDict_Contains_KnownHash(other, key, hash);
            if (rv < 0) {
                Py_DECREF(result);
                Py_DECREF(key);
                return NULL;
            }
            if (!rv) {
                if (set_add_entry((PySetObject *)result, key, hash)) {
                    Py_DECREF(result);
                    Py_DECREF(key);
                    return NULL;
                }
            }
            Py_DECREF(key);
        }
        return result;
    }

    /* Iterate over so, checking for common elements in other. */
    while (set_next(so, &pos, &entry)) {
        key = entry->key;
        hash = entry->hash;
        Py_INCREF(key);
        rv = set_contains_entry((PySetObject *)other, key, hash);
        if (rv < 0) {
            Py_DECREF(result);
            Py_DECREF(key);
            return NULL;
        }
        if (!rv) {
            if (set_add_entry((PySetObject *)result, key, hash)) {
                Py_DECREF(result);
                Py_DECREF(key);
                return NULL;
            }
        }
        Py_DECREF(key);
    }
    return result;
}

static PyObject *
set_difference_multi(PySetObject *so, PyObject *args)
{
    Py_ssize_t i;
    PyObject *result, *other;

    if (PyTuple_GET_SIZE(args) == 0)
        return set_copy(so, NULL);

    other = PyTuple_GET_ITEM(args, 0);
    result = set_difference(so, other);
    if (result == NULL)
        return NULL;

    for (i=1 ; i<PyTuple_GET_SIZE(args) ; i++) {
        other = PyTuple_GET_ITEM(args, i);
        if (set_difference_update_internal((PySetObject *)result, other)) {
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

PyDoc_STRVAR(difference_doc,
"difference($self, /, *others)\n\
--\n\
\n\
Return a new set with elements in the set that are not in the others.");
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
    if (set_difference_update_internal(so, other))
        return NULL;
    return Py_NewRef(so);
}

static PyObject *
set_symmetric_difference_update(PySetObject *so, PyObject *other)
{
    PySetObject *otherset;
    PyObject *key;
    Py_ssize_t pos = 0;
    Py_hash_t hash;
    setentry *entry;
    int rv;

    if ((PyObject *)so == other)
        return set_clear(so, NULL);

    if (PyDict_CheckExact(other)) {
        PyObject *value;
        while (_PyDict_Next(other, &pos, &key, &value, &hash)) {
            Py_INCREF(key);
            rv = set_discard_entry(so, key, hash);
            if (rv < 0) {
                Py_DECREF(key);
                return NULL;
            }
            if (rv == DISCARD_NOTFOUND) {
                if (set_add_entry(so, key, hash)) {
                    Py_DECREF(key);
                    return NULL;
                }
            }
            Py_DECREF(key);
        }
        Py_RETURN_NONE;
    }

    if (PyAnySet_Check(other)) {
        otherset = (PySetObject *)Py_NewRef(other);
    } else {
        otherset = (PySetObject *)make_new_set_basetype(Py_TYPE(so), other);
        if (otherset == NULL)
            return NULL;
    }

    while (set_next(otherset, &pos, &entry)) {
        key = entry->key;
        hash = entry->hash;
        Py_INCREF(key);
        rv = set_discard_entry(so, key, hash);
        if (rv < 0) {
            Py_DECREF(otherset);
            Py_DECREF(key);
            return NULL;
        }
        if (rv == DISCARD_NOTFOUND) {
            if (set_add_entry(so, key, hash)) {
                Py_DECREF(otherset);
                Py_DECREF(key);
                return NULL;
            }
        }
        Py_DECREF(key);
    }
    Py_DECREF(otherset);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(symmetric_difference_update_doc,
"symmetric_difference_update($self, other, /)\n\
--\n\
\n\
Update the set, keeping only elements found in either set, but not in both.");

static PyObject *
set_symmetric_difference(PySetObject *so, PyObject *other)
{
    PyObject *rv;
    PySetObject *otherset;

    otherset = (PySetObject *)make_new_set_basetype(Py_TYPE(so), other);
    if (otherset == NULL)
        return NULL;
    rv = set_symmetric_difference_update(otherset, (PyObject *)so);
    if (rv == NULL) {
        Py_DECREF(otherset);
        return NULL;
    }
    Py_DECREF(rv);
    return (PyObject *)otherset;
}

PyDoc_STRVAR(symmetric_difference_doc,
"symmetric_difference($self, other, /)\n\
--\n\
\n\
Return a new set with elements in either the set or other but not both.");

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
    return Py_NewRef(so);
}

static PyObject *
set_issubset(PySetObject *so, PyObject *other)
{
    setentry *entry;
    Py_ssize_t pos = 0;
    int rv;

    if (!PyAnySet_Check(other)) {
        PyObject *tmp = set_intersection(so, other);
        if (tmp == NULL) {
            return NULL;
        }
        int result = (PySet_GET_SIZE(tmp) == PySet_GET_SIZE(so));
        Py_DECREF(tmp);
        return PyBool_FromLong(result);
    }
    if (PySet_GET_SIZE(so) > PySet_GET_SIZE(other))
        Py_RETURN_FALSE;

    while (set_next(so, &pos, &entry)) {
        PyObject *key = entry->key;
        Py_INCREF(key);
        rv = set_contains_entry((PySetObject *)other, key, entry->hash);
        Py_DECREF(key);
        if (rv < 0) {
            return NULL;
        }
        if (!rv) {
            Py_RETURN_FALSE;
        }
    }
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another set contains this set.");

static PyObject *
set_issuperset(PySetObject *so, PyObject *other)
{
    if (PyAnySet_Check(other)) {
        return set_issubset((PySetObject *)other, (PyObject *)so);
    }

    PyObject *key, *it = PyObject_GetIter(other);
    if (it == NULL) {
        return NULL;
    }
    while ((key = PyIter_Next(it)) != NULL) {
        int rv = set_contains_key(so, key);
        Py_DECREF(key);
        if (rv < 0) {
            Py_DECREF(it);
            return NULL;
        }
        if (!rv) {
            Py_DECREF(it);
            Py_RETURN_FALSE;
        }
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(issuperset_doc, "Report whether this set contains another set.");

static PyObject *
set_richcompare(PySetObject *v, PyObject *w, int op)
{
    PyObject *r1;
    int r2;

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
        r2 = PyObject_IsTrue(r1);
        Py_DECREF(r1);
        if (r2 < 0)
            return NULL;
        return PyBool_FromLong(!r2);
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
    if (set_add_key(so, key))
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
    if (rv < 0) {
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
    if (result < 0)
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
    if (rv < 0) {
        if (!PySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
        tmpkey = make_new_set(&PyFrozenSet_Type, key);
        if (tmpkey == NULL)
            return NULL;
        rv = set_discard_key(so, tmpkey);
        Py_DECREF(tmpkey);
        if (rv < 0)
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
    if (rv < 0) {
        if (!PySet_Check(key) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
        tmpkey = make_new_set(&PyFrozenSet_Type, key);
        if (tmpkey == NULL)
            return NULL;
        rv = set_discard_key(so, tmpkey);
        Py_DECREF(tmpkey);
        if (rv < 0)
            return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a set if it is a member.\n\
\n\
Unlike set.remove(), the discard() method does not raise\n\
an exception when an element is missing from the set.");

static PyObject *
set_reduce(PySetObject *so, PyObject *Py_UNUSED(ignored))
{
    PyObject *keys=NULL, *args=NULL, *result=NULL, *state=NULL;

    keys = PySequence_List((PyObject *)so);
    if (keys == NULL)
        goto done;
    args = PyTuple_Pack(1, keys);
    if (args == NULL)
        goto done;
    state = _PyObject_GetState((PyObject *)so);
    if (state == NULL)
        goto done;
    result = PyTuple_Pack(3, Py_TYPE(so), args, state);
done:
    Py_XDECREF(args);
    Py_XDECREF(keys);
    Py_XDECREF(state);
    return result;
}

static PyObject *
set_sizeof(PySetObject *so, PyObject *Py_UNUSED(ignored))
{
    size_t res = _PyObject_SIZE(Py_TYPE(so));
    if (so->table != so->smalltable) {
        res += ((size_t)so->mask + 1) * sizeof(setentry);
    }
    return PyLong_FromSize_t(res);
}

PyDoc_STRVAR(sizeof_doc, "S.__sizeof__() -> size of S in memory, in bytes");
static int
set_init(PySetObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;

     if (!_PyArg_NoKeywords("set", kwds))
        return -1;
    if (!PyArg_UnpackTuple(args, Py_TYPE(self)->tp_name, 0, 1, &iterable))
        return -1;
    if (self->fill)
        set_clear_internal(self);
    self->hash = -1;
    if (iterable == NULL)
        return 0;
    return set_update_internal(self, iterable);
}

static PyObject*
set_vectorcall(PyObject *type, PyObject * const*args,
               size_t nargsf, PyObject *kwnames)
{
    assert(PyType_Check(type));

    if (!_PyArg_NoKwnames("set", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("set", nargs, 0, 1)) {
        return NULL;
    }

    if (nargs) {
        return make_new_set(_PyType_CAST(type), args[0]);
    }

    return make_new_set(_PyType_CAST(type), NULL);
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
    {"union",           (PyCFunction)set_union,         METH_VARARGS,
     union_doc},
    {"update",          (PyCFunction)set_update,        METH_VARARGS,
     update_doc},
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
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
"set(iterable=(), /)\n\
--\n\
\n\
Build an unordered collection of unique elements.");

PyTypeObject PySet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "set",                              /* tp_name */
    sizeof(PySetObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)set_dealloc,            /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
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
        Py_TPFLAGS_BASETYPE |
        _Py_TPFLAGS_MATCH_SELF,       /* tp_flags */
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
    .tp_vectorcall = set_vectorcall,
};

/* frozenset object ********************************************************/


static PyMethodDef frozenset_methods[] = {
    {"__contains__",(PyCFunction)set_direct_contains,           METH_O | METH_COEXIST,
     contains_doc},
    {"copy",            (PyCFunction)frozenset_copy,    METH_NOARGS,
     copy_doc},
    {"difference",      (PyCFunction)set_difference_multi,      METH_VARARGS,
     difference_doc},
    {"intersection",    (PyCFunction)set_intersection_multi,    METH_VARARGS,
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
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
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
"frozenset(iterable=(), /)\n\
--\n\
\n\
Build an immutable unordered collection of unique elements.");

PyTypeObject PyFrozenSet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "frozenset",                        /* tp_name */
    sizeof(PySetObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)set_dealloc,            /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
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
        Py_TPFLAGS_BASETYPE |
        _Py_TPFLAGS_MATCH_SELF,       /* tp_flags */
    frozenset_doc,                      /* tp_doc */
    (traverseproc)set_traverse,         /* tp_traverse */
    (inquiry)set_clear_internal,        /* tp_clear */
    (richcmpfunc)set_richcompare,       /* tp_richcompare */
    offsetof(PySetObject, weakreflist), /* tp_weaklistoffset */
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
    .tp_vectorcall = frozenset_vectorcall,
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
    return set_pop((PySetObject *)set, NULL);
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

/***** Dummy Struct  *************************************************/

static PyObject *
dummy_repr(PyObject *op)
{
    return PyUnicode_FromString("<dummy key>");
}

static void _Py_NO_RETURN
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
    0,                  /*tp_vectorcall_offset*/
    0,                  /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
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

static PyObject _dummy_struct = _PyObject_HEAD_INIT(&_PySetDummy_Type);
