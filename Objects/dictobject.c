
/* Dictionary object implementation using a hash table */

/* The distribution includes a separate file, Objects/dictnotes.txt,
   describing explorations into dictionary design and optimization.
   It covers typical dictionary use patterns, the parameters for
   tuning dictionaries, and several ideas for possible optimizations.
*/


/*
There are four kinds of slots in the table:

1. Unused.  me_key == me_value == NULL
   Does not hold an active (key, value) pair now and never did.  Unused can
   transition to Active upon key insertion.  This is the only case in which
   me_key is NULL, and is each slot's initial state.

2. Active.  me_key != NULL and me_key != dummy and me_value != NULL
   Holds an active (key, value) pair.  Active can transition to Dummy or
   Pending upon key deletion (for combined and split tables respectively).
   This is the only case in which me_value != NULL.

3. Dummy.  me_key == dummy and me_value == NULL
   Previously held an active (key, value) pair, but that was deleted and an
   active pair has not yet overwritten the slot.  Dummy can transition to
   Active upon key insertion.  Dummy slots cannot be made Unused again
   (cannot have me_key set to NULL), else the probe sequence in case of
   collision would have no way to know they were once active.

4. Pending. Not yet inserted or deleted from a split-table.
   key != NULL, key != dummy and value == NULL

The DictObject can be in one of two forms.
Either:
  A combined table:
    ma_values == NULL, dk_refcnt == 1.
    Values are stored in the me_value field of the PyDictKeysObject.
    Slot kind 4 is not allowed i.e.
        key != NULL, key != dummy and value == NULL is illegal.
Or:
  A split table:
    ma_values != NULL, dk_refcnt >= 1
    Values are stored in the ma_values array.
    Only string (unicode) keys are allowed, no <dummy> keys are present.

Note: .popitem() abuses the me_hash field of an Unused or Dummy slot to
hold a search finger.  The me_hash field of Unused or Dummy slots has no
meaning otherwise. As a consequence of this popitem always converts the dict
to the combined-table form.
*/

/* PyDict_MINSIZE_SPLIT is the minimum size of a split dictionary.
 * It must be a power of 2, and at least 4.
 * Resizing of split dictionaries is very rare, so the saving memory is more
 * important than the cost of resizing.
 */
#define PyDict_MINSIZE_SPLIT 4

/* PyDict_MINSIZE_COMBINED is the starting size for any new, non-split dict.
 * 8 allows dicts with no more than 5 active entries; experiments suggested
 * this suffices for the majority of dicts (consisting mostly of usually-small
 * dicts created to pass keyword arguments).
 * Making this 8, rather than 4 reduces the number of resizes for most
 * dictionaries, without any significant extra memory use.
 */
#define PyDict_MINSIZE_COMBINED 8

#include "Python.h"
#include "stringlib/eq.h"

typedef struct {
    /* Cached hash code of me_key. */
    Py_hash_t me_hash;
    PyObject *me_key;
    PyObject *me_value; /* This field is only meaningful for combined tables */
} PyDictKeyEntry;

typedef PyDictKeyEntry *(*dict_lookup_func)
(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject ***value_addr);

struct _dictkeysobject {
    Py_ssize_t dk_refcnt;
    Py_ssize_t dk_size;
    dict_lookup_func dk_lookup;
    Py_ssize_t dk_usable;
    PyDictKeyEntry dk_entries[1];
};


/*
To ensure the lookup algorithm terminates, there must be at least one Unused
slot (NULL key) in the table.
To avoid slowing down lookups on a near-full table, we resize the table when
it's USABLE_FRACTION (currently two-thirds) full.
*/

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
the list [i << 16 for i in range(20000)] as a set of keys.  Since ints are
their own hash codes, and this fits in a dict of size 2**15, the last 15 bits
 of every hash code are all 0:  they *all* map to the same table index.

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

Historical: Reimer Behrends contributed the idea of using a polynomial-based
approach, using repeated multiplication by x in GF(2**n) where an irreducible
polynomial for each table size was chosen such that x was a primitive root.
Christian Tismer later extended that to use division by x instead, as an
efficient way to get the high bits of the hash code into play.  This scheme
also gave excellent collision statistics, but was more expensive:  two
if-tests were required inside the loop; computing "the next" index took about
the same number of operations but without as much potential parallelism
(e.g., computing 5*j can go on at the same time as computing 1+perturb in the
above, and then shifting perturb can be done while the table index is being
masked); and the PyDictObject struct required a member to hold the table's
polynomial.  In Tim's experiments the current scheme ran faster, produced
equally good collision statistics, needed less code & used less memory.

*/

/* Object used as dummy key to fill deleted entries
 * This could be any unique object,
 * use a custom type in order to minimise coupling.
*/
static PyObject _dummy_struct;

#define dummy (&_dummy_struct)

#ifdef Py_REF_DEBUG
PyObject *
_PyDict_Dummy(void)
{
    return dummy;
}
#endif

/* forward declarations */
static PyDictKeyEntry *lookdict(PyDictObject *mp, PyObject *key,
                                Py_hash_t hash, PyObject ***value_addr);
static PyDictKeyEntry *lookdict_unicode(PyDictObject *mp, PyObject *key,
                                        Py_hash_t hash, PyObject ***value_addr);
static PyDictKeyEntry *
lookdict_unicode_nodummy(PyDictObject *mp, PyObject *key,
                         Py_hash_t hash, PyObject ***value_addr);
static PyDictKeyEntry *lookdict_split(PyDictObject *mp, PyObject *key,
                                      Py_hash_t hash, PyObject ***value_addr);

static int dictresize(PyDictObject *mp, Py_ssize_t minused);

/* Dictionary reuse scheme to save calls to malloc, free, and memset */
#ifndef PyDict_MAXFREELIST
#define PyDict_MAXFREELIST 80
#endif
static PyDictObject *free_list[PyDict_MAXFREELIST];
static int numfree = 0;

int
PyDict_ClearFreeList(void)
{
    PyDictObject *op;
    int ret = numfree;
    while (numfree) {
        op = free_list[--numfree];
        assert(PyDict_CheckExact(op));
        PyObject_GC_Del(op);
    }
    return ret;
}

/* Print summary info about the state of the optimized allocator */
void
_PyDict_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyDictObject", numfree, sizeof(PyDictObject));
}


void
PyDict_Fini(void)
{
    PyDict_ClearFreeList();
}

#define DK_DEBUG_INCREF _Py_INC_REFTOTAL _Py_REF_DEBUG_COMMA
#define DK_DEBUG_DECREF _Py_DEC_REFTOTAL _Py_REF_DEBUG_COMMA

#define DK_INCREF(dk) (DK_DEBUG_INCREF ++(dk)->dk_refcnt)
#define DK_DECREF(dk) if (DK_DEBUG_DECREF (--(dk)->dk_refcnt) == 0) free_keys_object(dk)
#define DK_SIZE(dk) ((dk)->dk_size)
#define DK_MASK(dk) (((dk)->dk_size)-1)
#define IS_POWER_OF_2(x) (((x) & (x-1)) == 0)

/* USABLE_FRACTION is the maximum dictionary load.
 * Currently set to (2n+1)/3. Increasing this ratio makes dictionaries more
 * dense resulting in more collisions.  Decreasing it improves sparseness
 * at the expense of spreading entries over more cache lines and at the
 * cost of total memory consumed.
 *
 * USABLE_FRACTION must obey the following:
 *     (0 < USABLE_FRACTION(n) < n) for all n >= 2
 *
 * USABLE_FRACTION should be very quick to calculate.
 * Fractions around 5/8 to 2/3 seem to work well in practice.
 */

/* Use (2n+1)/3 rather than 2n+3 because: it makes no difference for
 * combined tables (the two fractions round to the same number n < ),
 * but 2*4/3 is 2 whereas (2*4+1)/3 is 3 which potentially saves quite
 * a lot of space for small, split tables */
#define USABLE_FRACTION(n) ((((n) << 1)+1)/3)

/* Alternative fraction that is otherwise close enough to (2n+1)/3 to make
 * little difference. 8 * 2/3 == 8 * 5/8 == 5. 16 * 2/3 == 16 * 5/8 == 10.
 * 32 * 2/3 = 21, 32 * 5/8 = 20.
 * Its advantage is that it is faster to compute on machines with slow division.
 * #define USABLE_FRACTION(n) (((n) >> 1) + ((n) >> 2) - ((n) >> 3))
*/

/* GROWTH_RATE. Growth rate upon hitting maximum load.  Currently set to *2.
 * Raising this to *4 doubles memory consumption depending on the size of
 * the dictionary, but results in half the number of resizes, less effort to
 * resize and better sparseness for some (but not all dict sizes).
 * Setting to *4 eliminates every other resize step.
 * GROWTH_RATE was set to *4 up to version 3.2.
 */
#define GROWTH_RATE(x) ((x) * 2)

#define ENSURE_ALLOWS_DELETIONS(d) \
    if ((d)->ma_keys->dk_lookup == lookdict_unicode_nodummy) { \
        (d)->ma_keys->dk_lookup = lookdict_unicode; \
    }

/* This immutable, empty PyDictKeysObject is used for PyDict_Clear()
 * (which cannot fail and thus can do no allocation).
 */
static PyDictKeysObject empty_keys_struct = {
        2, /* dk_refcnt 1 for this struct, 1 for dummy_struct */
        1, /* dk_size */
        lookdict_split, /* dk_lookup */
        0, /* dk_usable (immutable) */
        {
            { 0, 0, 0 } /* dk_entries (empty) */
        }
};

static PyObject *empty_values[1] = { NULL };

#define Py_EMPTY_KEYS &empty_keys_struct

static PyDictKeysObject *new_keys_object(Py_ssize_t size)
{
    PyDictKeysObject *dk;
    Py_ssize_t i;
    PyDictKeyEntry *ep0;

    assert(size >= PyDict_MINSIZE_SPLIT);
    assert(IS_POWER_OF_2(size));
    dk = PyMem_MALLOC(sizeof(PyDictKeysObject) +
                      sizeof(PyDictKeyEntry) * (size-1));
    if (dk == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    DK_DEBUG_INCREF dk->dk_refcnt = 1;
    dk->dk_size = size;
    dk->dk_usable = USABLE_FRACTION(size);
    ep0 = &dk->dk_entries[0];
    /* Hash value of slot 0 is used by popitem, so it must be initialized */
    ep0->me_hash = 0;
    for (i = 0; i < size; i++) {
        ep0[i].me_key = NULL;
        ep0[i].me_value = NULL;
    }
    dk->dk_lookup = lookdict_unicode_nodummy;
    return dk;
}

static void
free_keys_object(PyDictKeysObject *keys)
{
    PyDictKeyEntry *entries = &keys->dk_entries[0];
    Py_ssize_t i, n;
    for (i = 0, n = DK_SIZE(keys); i < n; i++) {
        Py_XDECREF(entries[i].me_key);
        Py_XDECREF(entries[i].me_value);
    }
    PyMem_FREE(keys);
}

#define new_values(size) PyMem_NEW(PyObject *, size)

#define free_values(values) PyMem_FREE(values)

/* Consumes a reference to the keys object */
static PyObject *
new_dict(PyDictKeysObject *keys, PyObject **values)
{
    PyDictObject *mp;
    if (numfree) {
        mp = free_list[--numfree];
        assert (mp != NULL);
        assert (Py_TYPE(mp) == &PyDict_Type);
        _Py_NewReference((PyObject *)mp);
    }
    else {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (mp == NULL) {
            DK_DECREF(keys);
            free_values(values);
            return NULL;
        }
    }
    mp->ma_keys = keys;
    mp->ma_values = values;
    mp->ma_used = 0;
    return (PyObject *)mp;
}

/* Consumes a reference to the keys object */
static PyObject *
new_dict_with_shared_keys(PyDictKeysObject *keys)
{
    PyObject **values;
    Py_ssize_t i, size;

    size = DK_SIZE(keys);
    values = new_values(size);
    if (values == NULL) {
        DK_DECREF(keys);
        return PyErr_NoMemory();
    }
    for (i = 0; i < size; i++) {
        values[i] = NULL;
    }
    return new_dict(keys, values);
}

PyObject *
PyDict_New(void)
{
    return new_dict(new_keys_object(PyDict_MINSIZE_COMBINED), NULL);
}

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained earlier.

All arithmetic on hash should ignore overflow.

The details in this version are due to Tim Peters, building on many past
contributions by Reimer Behrends, Jyrki Alakuijala, Vladimir Marangozov and
Christian Tismer.

lookdict() is general-purpose, and may return NULL if (and only if) a
comparison raises an exception (this was new in Python 2.5).
lookdict_unicode() below is specialized to string keys, comparison of which can
never raise an exception; that function can never return NULL.
lookdict_unicode_nodummy is further specialized for string keys that cannot be
the <dummy> value.
For both, when the key isn't found a PyDictEntry* is returned
where the key would have been found, *value_addr points to the matching value
slot.
*/
static PyDictKeyEntry *
lookdict(PyDictObject *mp, PyObject *key,
         Py_hash_t hash, PyObject ***value_addr)
{
    register size_t i;
    register size_t perturb;
    register PyDictKeyEntry *freeslot;
    register size_t mask;
    PyDictKeyEntry *ep0;
    register PyDictKeyEntry *ep;
    register int cmp;
    PyObject *startkey;

top:
    mask = DK_MASK(mp->ma_keys);
    ep0 = &mp->ma_keys->dk_entries[0];
    i = (size_t)hash & mask;
    ep = &ep0[i];
    if (ep->me_key == NULL || ep->me_key == key) {
        *value_addr = &ep->me_value;
        return ep;
    }
    if (ep->me_key == dummy)
        freeslot = ep;
    else {
        if (ep->me_hash == hash) {
            startkey = ep->me_key;
            Py_INCREF(startkey);
            cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0)
                return NULL;
            if (ep0 == mp->ma_keys->dk_entries && ep->me_key == startkey) {
                if (cmp > 0) {
                    *value_addr = &ep->me_value;
                    return ep;
                }
            }
            else {
                /* The dict was mutated, restart */
                goto top;
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
            if (freeslot == NULL) {
                *value_addr = &ep->me_value;
                return ep;
            } else {
                *value_addr = &freeslot->me_value;
                return freeslot;
            }
        }
        if (ep->me_key == key) {
            *value_addr = &ep->me_value;
            return ep;
        }
        if (ep->me_hash == hash && ep->me_key != dummy) {
            startkey = ep->me_key;
            Py_INCREF(startkey);
            cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0) {
                *value_addr = NULL;
                return NULL;
            }
            if (ep0 == mp->ma_keys->dk_entries && ep->me_key == startkey) {
                if (cmp > 0) {
                    *value_addr = &ep->me_value;
                    return ep;
                }
            }
            else {
                /* The dict was mutated, restart */
                goto top;
            }
        }
        else if (ep->me_key == dummy && freeslot == NULL)
            freeslot = ep;
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/* Specialized version for string-only keys */
static PyDictKeyEntry *
lookdict_unicode(PyDictObject *mp, PyObject *key,
                 Py_hash_t hash, PyObject ***value_addr)
{
    register size_t i;
    register size_t perturb;
    register PyDictKeyEntry *freeslot;
    register size_t mask = DK_MASK(mp->ma_keys);
    PyDictKeyEntry *ep0 = &mp->ma_keys->dk_entries[0];
    register PyDictKeyEntry *ep;

    /* Make sure this function doesn't have to handle non-unicode keys,
       including subclasses of str; e.g., one reason to subclass
       unicodes is to override __eq__, and for speed we don't cater to
       that here. */
    if (!PyUnicode_CheckExact(key)) {
        mp->ma_keys->dk_lookup = lookdict;
        return lookdict(mp, key, hash, value_addr);
    }
    i = (size_t)hash & mask;
    ep = &ep0[i];
    if (ep->me_key == NULL || ep->me_key == key) {
        *value_addr = &ep->me_value;
        return ep;
    }
    if (ep->me_key == dummy)
        freeslot = ep;
    else {
        if (ep->me_hash == hash && unicode_eq(ep->me_key, key)) {
            *value_addr = &ep->me_value;
            return ep;
        }
        freeslot = NULL;
    }

    /* In the loop, me_key == dummy is by far (factor of 100s) the
       least likely outcome, so test for that last. */
    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = &ep0[i & mask];
        if (ep->me_key == NULL) {
            if (freeslot == NULL) {
                *value_addr = &ep->me_value;
                return ep;
            } else {
                *value_addr = &freeslot->me_value;
                return freeslot;
            }
        }
        if (ep->me_key == key
            || (ep->me_hash == hash
            && ep->me_key != dummy
            && unicode_eq(ep->me_key, key))) {
            *value_addr = &ep->me_value;
            return ep;
        }
        if (ep->me_key == dummy && freeslot == NULL)
            freeslot = ep;
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/* Faster version of lookdict_unicode when it is known that no <dummy> keys
 * will be present. */
static PyDictKeyEntry *
lookdict_unicode_nodummy(PyDictObject *mp, PyObject *key,
                         Py_hash_t hash, PyObject ***value_addr)
{
    register size_t i;
    register size_t perturb;
    register size_t mask = DK_MASK(mp->ma_keys);
    PyDictKeyEntry *ep0 = &mp->ma_keys->dk_entries[0];
    register PyDictKeyEntry *ep;

    /* Make sure this function doesn't have to handle non-unicode keys,
       including subclasses of str; e.g., one reason to subclass
       unicodes is to override __eq__, and for speed we don't cater to
       that here. */
    if (!PyUnicode_CheckExact(key)) {
        mp->ma_keys->dk_lookup = lookdict;
        return lookdict(mp, key, hash, value_addr);
    }
    i = (size_t)hash & mask;
    ep = &ep0[i];
    assert(ep->me_key == NULL || PyUnicode_CheckExact(ep->me_key));
    if (ep->me_key == NULL || ep->me_key == key ||
        (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
        *value_addr = &ep->me_value;
        return ep;
    }
    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = &ep0[i & mask];
        assert(ep->me_key == NULL || PyUnicode_CheckExact(ep->me_key));
        if (ep->me_key == NULL || ep->me_key == key ||
            (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            *value_addr = &ep->me_value;
            return ep;
        }
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/* Version of lookdict for split tables.
 * All split tables and only split tables use this lookup function.
 * Split tables only contain unicode keys and no dummy keys,
 * so algorithm is the same as lookdict_unicode_nodummy.
 */
static PyDictKeyEntry *
lookdict_split(PyDictObject *mp, PyObject *key,
               Py_hash_t hash, PyObject ***value_addr)
{
    register size_t i;
    register size_t perturb;
    register size_t mask = DK_MASK(mp->ma_keys);
    PyDictKeyEntry *ep0 = &mp->ma_keys->dk_entries[0];
    register PyDictKeyEntry *ep;

    if (!PyUnicode_CheckExact(key)) {
        ep = lookdict(mp, key, hash, value_addr);
        /* lookdict expects a combined-table, so fix value_addr */
        i = ep - ep0;
        *value_addr = &mp->ma_values[i];
        return ep;
    }
    i = (size_t)hash & mask;
    ep = &ep0[i];
    assert(ep->me_key == NULL || PyUnicode_CheckExact(ep->me_key));
    if (ep->me_key == NULL || ep->me_key == key ||
        (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
        *value_addr = &mp->ma_values[i];
        return ep;
    }
    for (perturb = hash; ; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = &ep0[i & mask];
        assert(ep->me_key == NULL || PyUnicode_CheckExact(ep->me_key));
        if (ep->me_key == NULL || ep->me_key == key ||
            (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            *value_addr = &mp->ma_values[i & mask];
            return ep;
        }
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

int
_PyDict_HasOnlyStringKeys(PyObject *dict)
{
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    assert(PyDict_Check(dict));
    /* Shortcut */
    if (((PyDictObject *)dict)->ma_keys->dk_lookup != lookdict)
        return 1;
    while (PyDict_Next(dict, &pos, &key, &value))
        if (!PyUnicode_Check(key))
            return 0;
    return 1;
}

#define MAINTAIN_TRACKING(mp, key, value) \
    do { \
        if (!_PyObject_GC_IS_TRACKED(mp)) { \
            if (_PyObject_GC_MAY_BE_TRACKED(key) || \
                _PyObject_GC_MAY_BE_TRACKED(value)) { \
                _PyObject_GC_TRACK(mp); \
            } \
        } \
    } while(0)

void
_PyDict_MaybeUntrack(PyObject *op)
{
    PyDictObject *mp;
    PyObject *value;
    Py_ssize_t i, size;

    if (!PyDict_CheckExact(op) || !_PyObject_GC_IS_TRACKED(op))
        return;

    mp = (PyDictObject *) op;
    size = DK_SIZE(mp->ma_keys);
    if (_PyDict_HasSplitTable(mp)) {
        for (i = 0; i < size; i++) {
            if ((value = mp->ma_values[i]) == NULL)
                continue;
            if (_PyObject_GC_MAY_BE_TRACKED(value)) {
                assert(!_PyObject_GC_MAY_BE_TRACKED(
                    mp->ma_keys->dk_entries[i].me_key));
                return;
            }
        }
    }
    else {
        PyDictKeyEntry *ep0 = &mp->ma_keys->dk_entries[0];
        for (i = 0; i < size; i++) {
            if ((value = ep0[i].me_value) == NULL)
                continue;
            if (_PyObject_GC_MAY_BE_TRACKED(value) ||
                _PyObject_GC_MAY_BE_TRACKED(ep0[i].me_key))
                return;
        }
    }
    _PyObject_GC_UNTRACK(op);
}

/* Internal function to find slot for an item from its hash
 * when it is known that the key is not present in the dict.
 */
static PyDictKeyEntry *
find_empty_slot(PyDictObject *mp, PyObject *key, Py_hash_t hash,
                PyObject ***value_addr)
{
    size_t i;
    size_t perturb;
    size_t mask = DK_MASK(mp->ma_keys);
    PyDictKeyEntry *ep0 = &mp->ma_keys->dk_entries[0];
    PyDictKeyEntry *ep;

    assert(key != NULL);
    if (!PyUnicode_CheckExact(key))
        mp->ma_keys->dk_lookup = lookdict;
    i = hash & mask;
    ep = &ep0[i];
    for (perturb = hash; ep->me_key != NULL; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = &ep0[i & mask];
    }
    assert(ep->me_value == NULL);
    if (mp->ma_values)
        *value_addr = &mp->ma_values[i & mask];
    else
        *value_addr = &ep->me_value;
    return ep;
}

static int
insertion_resize(PyDictObject *mp)
{
    return dictresize(mp, GROWTH_RATE(mp->ma_used));
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Returns -1 if an error occurred, or 0 on success.
*/
static int
insertdict(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject *value)
{
    PyObject *old_value;
    PyObject **value_addr;
    PyDictKeyEntry *ep;
    assert(key != dummy);

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (insertion_resize(mp) < 0)
            return -1;
    }

    ep = mp->ma_keys->dk_lookup(mp, key, hash, &value_addr);
    if (ep == NULL) {
        return -1;
    }
    Py_INCREF(value);
    MAINTAIN_TRACKING(mp, key, value);
    old_value = *value_addr;
    if (old_value != NULL) {
        assert(ep->me_key != NULL && ep->me_key != dummy);
        *value_addr = value;
        Py_DECREF(old_value); /* which **CAN** re-enter */
    }
    else {
        if (ep->me_key == NULL) {
            Py_INCREF(key);
            if (mp->ma_keys->dk_usable <= 0) {
                /* Need to resize. */
                if (insertion_resize(mp) < 0) {
                    Py_DECREF(key);
                    Py_DECREF(value);
                    return -1;
                }
                ep = find_empty_slot(mp, key, hash, &value_addr);
            }
            mp->ma_keys->dk_usable--;
            assert(mp->ma_keys->dk_usable >= 0);
            ep->me_key = key;
            ep->me_hash = hash;
        }
        else {
            if (ep->me_key == dummy) {
                Py_INCREF(key);
                ep->me_key = key;
                ep->me_hash = hash;
                Py_DECREF(dummy);
            } else {
                assert(_PyDict_HasSplitTable(mp));
            }
        }
        mp->ma_used++;
        *value_addr = value;
    }
    assert(ep->me_key != NULL && ep->me_key != dummy);
    assert(PyUnicode_CheckExact(key) || mp->ma_keys->dk_lookup == lookdict);
    return 0;
}

/*
Internal routine used by dictresize() to insert an item which is
known to be absent from the dict.  This routine also assumes that
the dict contains no deleted entries.  Besides the performance benefit,
using insertdict() in dictresize() is dangerous (SF bug #1456209).
Note that no refcounts are changed by this routine; if needed, the caller
is responsible for incref'ing `key` and `value`.
Neither mp->ma_used nor k->dk_usable are modified by this routine; the caller
must set them correctly
*/
static void
insertdict_clean(PyDictObject *mp, PyObject *key, Py_hash_t hash,
                 PyObject *value)
{
    size_t i;
    size_t perturb;
    PyDictKeysObject *k = mp->ma_keys;
    size_t mask = (size_t)DK_SIZE(k)-1;
    PyDictKeyEntry *ep0 = &k->dk_entries[0];
    PyDictKeyEntry *ep;

    assert(k->dk_lookup != NULL);
    assert(value != NULL);
    assert(key != NULL);
    assert(key != dummy);
    assert(PyUnicode_CheckExact(key) || k->dk_lookup == lookdict);
    i = hash & mask;
    ep = &ep0[i];
    for (perturb = hash; ep->me_key != NULL; perturb >>= PERTURB_SHIFT) {
        i = (i << 2) + i + perturb + 1;
        ep = &ep0[i & mask];
    }
    assert(ep->me_value == NULL);
    ep->me_key = key;
    ep->me_hash = hash;
    ep->me_value = value;
}

/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
If a table is split (its keys and hashes are shared, its values are not),
then the values are temporarily copied into the table, it is resized as
a combined table, then the me_value slots in the old table are NULLed out.
After resizing a table is always combined,
but can be resplit by make_keys_shared().
*/
static int
dictresize(PyDictObject *mp, Py_ssize_t minused)
{
    Py_ssize_t newsize;
    PyDictKeysObject *oldkeys;
    PyObject **oldvalues;
    Py_ssize_t i, oldsize;

/* Find the smallest table size > minused. */
    for (newsize = PyDict_MINSIZE_COMBINED;
         newsize <= minused && newsize > 0;
         newsize <<= 1)
        ;
    if (newsize <= 0) {
        PyErr_NoMemory();
        return -1;
    }
    oldkeys = mp->ma_keys;
    oldvalues = mp->ma_values;
    /* Allocate a new table. */
    mp->ma_keys = new_keys_object(newsize);
    if (mp->ma_keys == NULL) {
        mp->ma_keys = oldkeys;
        return -1;
    }
    if (oldkeys->dk_lookup == lookdict)
        mp->ma_keys->dk_lookup = lookdict;
    oldsize = DK_SIZE(oldkeys);
    mp->ma_values = NULL;
    /* If empty then nothing to copy so just return */
    if (oldsize == 1) {
        assert(oldkeys == Py_EMPTY_KEYS);
        DK_DECREF(oldkeys);
        return 0;
    }
    /* Main loop below assumes we can transfer refcount to new keys
     * and that value is stored in me_value.
     * Increment ref-counts and copy values here to compensate
     * This (resizing a split table) should be relatively rare */
    if (oldvalues != NULL) {
        for (i = 0; i < oldsize; i++) {
            if (oldvalues[i] != NULL) {
                Py_INCREF(oldkeys->dk_entries[i].me_key);
                oldkeys->dk_entries[i].me_value = oldvalues[i];
            }
        }
    }
    /* Main loop */
    for (i = 0; i < oldsize; i++) {
        PyDictKeyEntry *ep = &oldkeys->dk_entries[i];
        if (ep->me_value != NULL) {
            assert(ep->me_key != dummy);
            insertdict_clean(mp, ep->me_key, ep->me_hash, ep->me_value);
        }
    }
    mp->ma_keys->dk_usable -= mp->ma_used;
    if (oldvalues != NULL) {
        /* NULL out me_value slot in oldkeys, in case it was shared */
        for (i = 0; i < oldsize; i++)
            oldkeys->dk_entries[i].me_value = NULL;
        assert(oldvalues != empty_values);
        free_values(oldvalues);
        DK_DECREF(oldkeys);
    }
    else {
        assert(oldkeys->dk_lookup != lookdict_split);
        if (oldkeys->dk_lookup != lookdict_unicode_nodummy) {
            PyDictKeyEntry *ep0 = &oldkeys->dk_entries[0];
            for (i = 0; i < oldsize; i++) {
                if (ep0[i].me_key == dummy)
                    Py_DECREF(dummy);
            }
        }
        assert(oldkeys->dk_refcnt == 1);
        DK_DEBUG_DECREF PyMem_FREE(oldkeys);
    }
    return 0;
}

/* Returns NULL if unable to split table.
 * A NULL return does not necessarily indicate an error */
static PyDictKeysObject *
make_keys_shared(PyObject *op)
{
    Py_ssize_t i;
    Py_ssize_t size;
    PyDictObject *mp = (PyDictObject *)op;

    if (!PyDict_CheckExact(op))
        return NULL;
    if (!_PyDict_HasSplitTable(mp)) {
        PyDictKeyEntry *ep0;
        PyObject **values;
        assert(mp->ma_keys->dk_refcnt == 1);
        if (mp->ma_keys->dk_lookup == lookdict) {
            return NULL;
        }
        else if (mp->ma_keys->dk_lookup == lookdict_unicode) {
            /* Remove dummy keys */
            if (dictresize(mp, DK_SIZE(mp->ma_keys)))
                return NULL;
        }
        assert(mp->ma_keys->dk_lookup == lookdict_unicode_nodummy);
        /* Copy values into a new array */
        ep0 = &mp->ma_keys->dk_entries[0];
        size = DK_SIZE(mp->ma_keys);
        values = new_values(size);
        if (values == NULL) {
            PyErr_SetString(PyExc_MemoryError,
                "Not enough memory to allocate new values array");
            return NULL;
        }
        for (i = 0; i < size; i++) {
            values[i] = ep0[i].me_value;
            ep0[i].me_value = NULL;
        }
        mp->ma_keys->dk_lookup = lookdict_split;
        mp->ma_values = values;
    }
    DK_INCREF(mp->ma_keys);
    return mp->ma_keys;
}

PyObject *
_PyDict_NewPresized(Py_ssize_t minused)
{
    Py_ssize_t newsize;
    PyDictKeysObject *new_keys;
    for (newsize = PyDict_MINSIZE_COMBINED;
         newsize <= minused && newsize > 0;
         newsize <<= 1)
        ;
    new_keys = new_keys_object(newsize);
    if (new_keys == NULL)
        return NULL;
    return new_dict(new_keys, NULL);
}

/* Note that, for historical reasons, PyDict_GetItem() suppresses all errors
 * that may occur (originally dicts supported only string keys, and exceptions
 * weren't possible).  So, while the original intent was that a NULL return
 * meant the key wasn't present, in reality it can mean that, or that an error
 * (suppressed) occurred while computing the key's hash, or that some error
 * (suppressed) occurred when comparing keys in the dict's internal probe
 * sequence.  A nasty example of the latter is when a Python-coded comparison
 * function hits a stack-depth error, which can cause this to return NULL
 * even if the key is present.
 */
PyObject *
PyDict_GetItem(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeyEntry *ep;
    PyThreadState *tstate;
    PyObject **value_addr;

    if (!PyDict_Check(op))
        return NULL;
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1)
    {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            PyErr_Clear();
            return NULL;
        }
    }

    /* We can arrive here with a NULL tstate during initialization: try
       running "python -Wi" for an example related to string interning.
       Let's just hope that no exception occurs then...  This must be
       _PyThreadState_Current and not PyThreadState_GET() because in debug
       mode, the latter complains if tstate is NULL. */
    tstate = (PyThreadState*)_Py_atomic_load_relaxed(
        &_PyThreadState_Current);
    if (tstate != NULL && tstate->curexc_type != NULL) {
        /* preserve the existing exception */
        PyObject *err_type, *err_value, *err_tb;
        PyErr_Fetch(&err_type, &err_value, &err_tb);
        ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
        /* ignore errors */
        PyErr_Restore(err_type, err_value, err_tb);
        if (ep == NULL)
            return NULL;
    }
    else {
        ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
        if (ep == NULL) {
            PyErr_Clear();
            return NULL;
        }
    }
    return *value_addr;
}

/* Variant of PyDict_GetItem() that doesn't suppress exceptions.
   This returns NULL *with* an exception set if an exception occurred.
   It returns NULL *without* an exception set if the key wasn't present.
*/
PyObject *
PyDict_GetItemWithError(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    PyDictObject*mp = (PyDictObject *)op;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1)
    {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            return NULL;
        }
    }

    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    return *value_addr;
}

PyObject *
_PyDict_GetItemIdWithError(PyObject *dp, struct _Py_Identifier *key)
{
    PyObject *kv;
    kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL)
        return NULL;
    return PyDict_GetItemWithError(dp, kv);
}

/* Fast version of global value lookup.
 * Lookup in globals, then builtins.
 */
PyObject *
_PyDict_LoadGlobal(PyDictObject *globals, PyDictObject *builtins, PyObject *key)
{
    PyObject *x;
    if (PyUnicode_CheckExact(key)) {
        PyObject **value_addr;
        Py_hash_t hash = ((PyASCIIObject *)key)->hash;
        if (hash != -1) {
            PyDictKeyEntry *e;
            e = globals->ma_keys->dk_lookup(globals, key, hash, &value_addr);
            if (e == NULL) {
                return NULL;
            }
            x = *value_addr;
            if (x != NULL)
                return x;
            e = builtins->ma_keys->dk_lookup(builtins, key, hash, &value_addr);
            if (e == NULL) {
                return NULL;
            }
            x = *value_addr;
            return x;
        }
    }
    x = PyDict_GetItemWithError((PyObject *)globals, key);
    if (x != NULL)
        return x;
    if (PyErr_Occurred())
        return NULL;
    return PyDict_GetItemWithError((PyObject *)builtins, key);
}

/* CAUTION: PyDict_SetItem() must guarantee that it won't resize the
 * dictionary if it's merely replacing the value for an existing key.
 * This means that it's safe to loop over a dictionary with PyDict_Next()
 * and occasionally replace a value -- but you can't insert new keys or
 * remove them.
 */
int
PyDict_SetItem(PyObject *op, PyObject *key, PyObject *value)
{
    PyDictObject *mp;
    Py_hash_t hash;
    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(value);
    mp = (PyDictObject *)op;
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1)
    {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }

    /* insertdict() handles any resizing that might be necessary */
    return insertdict(mp, key, hash, value);
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
{
    PyDictObject *mp;
    Py_hash_t hash;
    PyDictKeyEntry *ep;
    PyObject *old_key, *old_value;
    PyObject **value_addr;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    mp = (PyDictObject *)op;
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return -1;
    if (*value_addr == NULL) {
        set_key_error(key);
        return -1;
    }
    old_value = *value_addr;
    *value_addr = NULL;
    mp->ma_used--;
    if (!_PyDict_HasSplitTable(mp)) {
        ENSURE_ALLOWS_DELETIONS(mp);
        old_key = ep->me_key;
        Py_INCREF(dummy);
        ep->me_key = dummy;
        Py_DECREF(old_key);
    }
    Py_DECREF(old_value);
    return 0;
}

void
PyDict_Clear(PyObject *op)
{
    PyDictObject *mp;
    PyDictKeysObject *oldkeys;
    PyObject **oldvalues;
    Py_ssize_t i, n;

    if (!PyDict_Check(op))
        return;
    mp = ((PyDictObject *)op);
    oldkeys = mp->ma_keys;
    oldvalues = mp->ma_values;
    if (oldvalues == empty_values)
        return;
    /* Empty the dict... */
    DK_INCREF(Py_EMPTY_KEYS);
    mp->ma_keys = Py_EMPTY_KEYS;
    mp->ma_values = empty_values;
    mp->ma_used = 0;
    /* ...then clear the keys and values */
    if (oldvalues != NULL) {
        n = DK_SIZE(oldkeys);
        for (i = 0; i < n; i++)
            Py_CLEAR(oldvalues[i]);
        free_values(oldvalues);
        DK_DECREF(oldkeys);
    }
    else {
       assert(oldkeys->dk_refcnt == 1);
       DK_DECREF(oldkeys);
    }
}

/* Returns -1 if no more items (or op is not a dict),
 * index of item otherwise. Stores value in pvalue
 */
Py_LOCAL_INLINE(Py_ssize_t)
dict_next(PyObject *op, Py_ssize_t i, PyObject **pvalue)
{
    Py_ssize_t mask, offset;
    PyDictObject *mp;
    PyObject **value_ptr;


    if (!PyDict_Check(op))
        return -1;
    mp = (PyDictObject *)op;
    if (i < 0)
        return -1;
    if (mp->ma_values) {
        value_ptr = &mp->ma_values[i];
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &mp->ma_keys->dk_entries[i].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    mask = DK_MASK(mp->ma_keys);
    while (i <= mask && *value_ptr == NULL) {
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        i++;
    }
    if (i > mask)
        return -1;
    if (pvalue)
        *pvalue = *value_ptr;
    return i;
}

/*
 * Iterate over a dict.  Use like so:
 *
 *     Py_ssize_t i;
 *     PyObject *key, *value;
 *     i = 0;   # important!  i should not otherwise be changed by you
 *     while (PyDict_Next(yourdict, &i, &key, &value)) {
 *              Refer to borrowed references in key and value.
 *     }
 *
 * CAUTION:  In general, it isn't safe to use PyDict_Next in a loop that
 * mutates the dict.  One exception:  it is safe if the loop merely changes
 * the values associated with the keys (but doesn't insert new keys or
 * delete keys), via PyDict_SetItem().
 */
int
PyDict_Next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey, PyObject **pvalue)
{
    PyDictObject *mp;
    Py_ssize_t i = dict_next(op, *ppos, pvalue);
    if (i < 0)
        return 0;
    mp = (PyDictObject *)op;
    *ppos = i+1;
    if (pkey)
        *pkey = mp->ma_keys->dk_entries[i].me_key;
    return 1;
}

/* Internal version of PyDict_Next that returns a hash value in addition
 * to the key and value.
 */
int
_PyDict_Next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey,
             PyObject **pvalue, Py_hash_t *phash)
{
    PyDictObject *mp;
    Py_ssize_t i = dict_next(op, *ppos, pvalue);
    if (i < 0)
        return 0;
    mp = (PyDictObject *)op;
    *ppos = i+1;
    *phash = mp->ma_keys->dk_entries[i].me_hash;
    if (pkey)
        *pkey = mp->ma_keys->dk_entries[i].me_key;
    return 1;
}

/* Methods */

static void
dict_dealloc(PyDictObject *mp)
{
    PyObject **values = mp->ma_values;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n;
    PyObject_GC_UnTrack(mp);
    Py_TRASHCAN_SAFE_BEGIN(mp)
    if (values != NULL) {
        if (values != empty_values) {
            for (i = 0, n = DK_SIZE(mp->ma_keys); i < n; i++) {
                Py_XDECREF(values[i]);
            }
            free_values(values);
        }
        DK_DECREF(keys);
    }
    else {
        assert(keys->dk_refcnt == 1);
        DK_DECREF(keys);
    }
    if (numfree < PyDict_MAXFREELIST && Py_TYPE(mp) == &PyDict_Type)
        free_list[numfree++] = mp;
    else
        Py_TYPE(mp)->tp_free((PyObject *)mp);
    Py_TRASHCAN_SAFE_END(mp)
}


static PyObject *
dict_repr(PyDictObject *mp)
{
    Py_ssize_t i;
    PyObject *s, *temp, *colon = NULL;
    PyObject *pieces = NULL, *result = NULL;
    PyObject *key, *value;

    i = Py_ReprEnter((PyObject *)mp);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("{...}") : NULL;
    }

    if (mp->ma_used == 0) {
        result = PyUnicode_FromString("{}");
        goto Done;
    }

    pieces = PyList_New(0);
    if (pieces == NULL)
        goto Done;

    colon = PyUnicode_FromString(": ");
    if (colon == NULL)
        goto Done;

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    i = 0;
    while (PyDict_Next((PyObject *)mp, &i, &key, &value)) {
        int status;
        /* Prevent repr from deleting key or value during key format. */
        Py_INCREF(key);
        Py_INCREF(value);
        s = PyObject_Repr(key);
        PyUnicode_Append(&s, colon);
        PyUnicode_AppendAndDel(&s, PyObject_Repr(value));
        Py_DECREF(key);
        Py_DECREF(value);
        if (s == NULL)
            goto Done;
        status = PyList_Append(pieces, s);
        Py_DECREF(s);  /* append created a new ref */
        if (status < 0)
            goto Done;
    }

    /* Add "{}" decorations to the first and last items. */
    assert(PyList_GET_SIZE(pieces) > 0);
    s = PyUnicode_FromString("{");
    if (s == NULL)
        goto Done;
    temp = PyList_GET_ITEM(pieces, 0);
    PyUnicode_AppendAndDel(&s, temp);
    PyList_SET_ITEM(pieces, 0, s);
    if (s == NULL)
        goto Done;

    s = PyUnicode_FromString("}");
    if (s == NULL)
        goto Done;
    temp = PyList_GET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1);
    PyUnicode_AppendAndDel(&temp, s);
    PyList_SET_ITEM(pieces, PyList_GET_SIZE(pieces) - 1, temp);
    if (temp == NULL)
        goto Done;

    /* Paste them all together with ", " between. */
    s = PyUnicode_FromString(", ");
    if (s == NULL)
        goto Done;
    result = PyUnicode_Join(s, pieces);
    Py_DECREF(s);

Done:
    Py_XDECREF(pieces);
    Py_XDECREF(colon);
    Py_ReprLeave((PyObject *)mp);
    return result;
}

static Py_ssize_t
dict_length(PyDictObject *mp)
{
    return mp->ma_used;
}

static PyObject *
dict_subscript(PyDictObject *mp, register PyObject *key)
{
    PyObject *v;
    Py_hash_t hash;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    v = *value_addr;
    if (v == NULL) {
        if (!PyDict_CheckExact(mp)) {
            /* Look up __missing__ method if we're a subclass. */
            PyObject *missing, *res;
            _Py_IDENTIFIER(__missing__);
            missing = _PyObject_LookupSpecial((PyObject *)mp, &PyId___missing__);
            if (missing != NULL) {
                res = PyObject_CallFunctionObjArgs(missing,
                                                   key, NULL);
                Py_DECREF(missing);
                return res;
            }
            else if (PyErr_Occurred())
                return NULL;
        }
        set_key_error(key);
        return NULL;
    }
    else
        Py_INCREF(v);
    return v;
}

static int
dict_ass_sub(PyDictObject *mp, PyObject *v, PyObject *w)
{
    if (w == NULL)
        return PyDict_DelItem((PyObject *)mp, v);
    else
        return PyDict_SetItem((PyObject *)mp, v, w);
}

static PyMappingMethods dict_as_mapping = {
    (lenfunc)dict_length, /*mp_length*/
    (binaryfunc)dict_subscript, /*mp_subscript*/
    (objobjargproc)dict_ass_sub, /*mp_ass_subscript*/
};

static PyObject *
dict_keys(register PyDictObject *mp)
{
    register PyObject *v;
    register Py_ssize_t i, j;
    PyDictKeyEntry *ep;
    Py_ssize_t size, n, offset;
    PyObject **value_ptr;

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
    ep = &mp->ma_keys->dk_entries[0];
    size = DK_SIZE(mp->ma_keys);
    if (mp->ma_values) {
        value_ptr = mp->ma_values;
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &ep[0].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    for (i = 0, j = 0; i < size; i++) {
        if (*value_ptr != NULL) {
            PyObject *key = ep[i].me_key;
            Py_INCREF(key);
            PyList_SET_ITEM(v, j, key);
            j++;
        }
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
    }
    assert(j == n);
    return v;
}

static PyObject *
dict_values(register PyDictObject *mp)
{
    register PyObject *v;
    register Py_ssize_t i, j;
    Py_ssize_t size, n, offset;
    PyObject **value_ptr;

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
    size = DK_SIZE(mp->ma_keys);
    if (mp->ma_values) {
        value_ptr = mp->ma_values;
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &mp->ma_keys->dk_entries[0].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    for (i = 0, j = 0; i < size; i++) {
        PyObject *value = *value_ptr;
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        if (value != NULL) {
            Py_INCREF(value);
            PyList_SET_ITEM(v, j, value);
            j++;
        }
    }
    assert(j == n);
    return v;
}

static PyObject *
dict_items(register PyDictObject *mp)
{
    register PyObject *v;
    register Py_ssize_t i, j, n;
    Py_ssize_t size, offset;
    PyObject *item, *key;
    PyDictKeyEntry *ep;
    PyObject **value_ptr;

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
    ep = mp->ma_keys->dk_entries;
    size = DK_SIZE(mp->ma_keys);
    if (mp->ma_values) {
        value_ptr = mp->ma_values;
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &ep[0].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    for (i = 0, j = 0; i < size; i++) {
        PyObject *value = *value_ptr;
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        if (value != NULL) {
            key = ep[i].me_key;
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
dict_fromkeys(PyObject *cls, PyObject *args)
{
    PyObject *seq;
    PyObject *value = Py_None;
    PyObject *it;       /* iter(seq) */
    PyObject *key;
    PyObject *d;
    int status;

    if (!PyArg_UnpackTuple(args, "fromkeys", 1, 2, &seq, &value))
        return NULL;

    d = PyObject_CallObject(cls, NULL);
    if (d == NULL)
        return NULL;

    if (PyDict_CheckExact(d) && ((PyDictObject *)d)->ma_used == 0) {
        if (PyDict_CheckExact(seq)) {
            PyDictObject *mp = (PyDictObject *)d;
            PyObject *oldvalue;
            Py_ssize_t pos = 0;
            PyObject *key;
            Py_hash_t hash;

            if (dictresize(mp, Py_SIZE(seq))) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PyDict_Next(seq, &pos, &key, &oldvalue, &hash)) {
                if (insertdict(mp, key, hash, value)) {
                    Py_DECREF(d);
                    return NULL;
                }
            }
            return d;
        }
        if (PyAnySet_CheckExact(seq)) {
            PyDictObject *mp = (PyDictObject *)d;
            Py_ssize_t pos = 0;
            PyObject *key;
            Py_hash_t hash;

            if (dictresize(mp, PySet_GET_SIZE(seq))) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PySet_NextEntry(seq, &pos, &key, &hash)) {
                if (insertdict(mp, key, hash, value)) {
                    Py_DECREF(d);
                    return NULL;
                }
            }
            return d;
        }
    }

    it = PyObject_GetIter(seq);
    if (it == NULL){
        Py_DECREF(d);
        return NULL;
    }

    if (PyDict_CheckExact(d)) {
        while ((key = PyIter_Next(it)) != NULL) {
            status = PyDict_SetItem(d, key, value);
            Py_DECREF(key);
            if (status < 0)
                goto Fail;
        }
    } else {
        while ((key = PyIter_Next(it)) != NULL) {
            status = PyObject_SetItem(d, key, value);
            Py_DECREF(key);
            if (status < 0)
                goto Fail;
        }
    }

    if (PyErr_Occurred())
        goto Fail;
    Py_DECREF(it);
    return d;

Fail:
    Py_DECREF(it);
    Py_DECREF(d);
    return NULL;
}

static int
dict_update_common(PyObject *self, PyObject *args, PyObject *kwds, char *methname)
{
    PyObject *arg = NULL;
    int result = 0;

    if (!PyArg_UnpackTuple(args, methname, 0, 1, &arg))
        result = -1;

    else if (arg != NULL) {
        _Py_IDENTIFIER(keys);
        if (_PyObject_HasAttrId(arg, &PyId_keys))
            result = PyDict_Merge(self, arg, 1);
        else
            result = PyDict_MergeFromSeq2(self, arg, 1);
    }
    if (result == 0 && kwds != NULL) {
        if (PyArg_ValidateKeywordArguments(kwds))
            result = PyDict_Merge(self, kwds, 1);
        else
            result = -1;
    }
    return result;
}

static PyObject *
dict_update(PyObject *self, PyObject *args, PyObject *kwds)
{
    if (dict_update_common(self, args, kwds, "update") != -1)
        Py_RETURN_NONE;
    return NULL;
}

/* Update unconditionally replaces existing items.
   Merge has a 3rd argument 'override'; if set, it acts like Update,
   otherwise it leaves existing items unchanged.

   PyDict_{Update,Merge} update/merge from a mapping object.

   PyDict_MergeFromSeq2 updates/merges from any iterable object
   producing iterable objects of length 2.
*/

int
PyDict_MergeFromSeq2(PyObject *d, PyObject *seq2, int override)
{
    PyObject *it;       /* iter(seq2) */
    Py_ssize_t i;       /* index into seq2 of current element */
    PyObject *item;     /* seq2[i] */
    PyObject *fast;     /* item as a 2-tuple or 2-list */

    assert(d != NULL);
    assert(PyDict_Check(d));
    assert(seq2 != NULL);

    it = PyObject_GetIter(seq2);
    if (it == NULL)
        return -1;

    for (i = 0; ; ++i) {
        PyObject *key, *value;
        Py_ssize_t n;

        fast = NULL;
        item = PyIter_Next(it);
        if (item == NULL) {
            if (PyErr_Occurred())
                goto Fail;
            break;
        }

        /* Convert item to sequence, and verify length 2. */
        fast = PySequence_Fast(item, "");
        if (fast == NULL) {
            if (PyErr_ExceptionMatches(PyExc_TypeError))
                PyErr_Format(PyExc_TypeError,
                    "cannot convert dictionary update "
                    "sequence element #%zd to a sequence",
                    i);
            goto Fail;
        }
        n = PySequence_Fast_GET_SIZE(fast);
        if (n != 2) {
            PyErr_Format(PyExc_ValueError,
                         "dictionary update sequence element #%zd "
                         "has length %zd; 2 is required",
                         i, n);
            goto Fail;
        }

        /* Update/merge with this (key, value) pair. */
        key = PySequence_Fast_GET_ITEM(fast, 0);
        value = PySequence_Fast_GET_ITEM(fast, 1);
        if (override || PyDict_GetItem(d, key) == NULL) {
            int status = PyDict_SetItem(d, key, value);
            if (status < 0)
                goto Fail;
        }
        Py_DECREF(fast);
        Py_DECREF(item);
    }

    i = 0;
    goto Return;
Fail:
    Py_XDECREF(item);
    Py_XDECREF(fast);
    i = -1;
Return:
    Py_DECREF(it);
    return Py_SAFE_DOWNCAST(i, Py_ssize_t, int);
}

int
PyDict_Update(PyObject *a, PyObject *b)
{
    return PyDict_Merge(a, b, 1);
}

int
PyDict_Merge(PyObject *a, PyObject *b, int override)
{
    register PyDictObject *mp, *other;
    register Py_ssize_t i, n;
    PyDictKeyEntry *entry;

    /* We accept for the argument either a concrete dictionary object,
     * or an abstract "mapping" object.  For the former, we can do
     * things quite efficiently.  For the latter, we only require that
     * PyMapping_Keys() and PyObject_GetItem() be supported.
     */
    if (a == NULL || !PyDict_Check(a) || b == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    mp = (PyDictObject*)a;
    if (PyDict_Check(b)) {
        other = (PyDictObject*)b;
        if (other == mp || other->ma_used == 0)
            /* a.update(a) or a.update({}); nothing to do */
            return 0;
        if (mp->ma_used == 0)
            /* Since the target dict is empty, PyDict_GetItem()
             * always returns NULL.  Setting override to 1
             * skips the unnecessary test.
             */
            override = 1;
        /* Do one big resize at the start, rather than
         * incrementally resizing as we insert new items.  Expect
         * that there will be no (or few) overlapping keys.
         */
        if (mp->ma_keys->dk_usable * 3 < other->ma_used * 2)
            if (dictresize(mp, (mp->ma_used + other->ma_used)*2) != 0)
               return -1;
        for (i = 0, n = DK_SIZE(other->ma_keys); i < n; i++) {
            PyObject *value;
            entry = &other->ma_keys->dk_entries[i];
            if (other->ma_values)
                value = other->ma_values[i];
            else
                value = entry->me_value;

            if (value != NULL &&
                (override ||
                 PyDict_GetItem(a, entry->me_key) == NULL)) {
                if (insertdict(mp, entry->me_key,
                               entry->me_hash,
                               value) != 0)
                    return -1;
            }
        }
    }
    else {
        /* Do it the generic, slower way */
        PyObject *keys = PyMapping_Keys(b);
        PyObject *iter;
        PyObject *key, *value;
        int status;

        if (keys == NULL)
            /* Docstring says this is equivalent to E.keys() so
             * if E doesn't have a .keys() method we want
             * AttributeError to percolate up.  Might as well
             * do the same for any other error.
             */
            return -1;

        iter = PyObject_GetIter(keys);
        Py_DECREF(keys);
        if (iter == NULL)
            return -1;

        for (key = PyIter_Next(iter); key; key = PyIter_Next(iter)) {
            if (!override && PyDict_GetItem(a, key) != NULL) {
                Py_DECREF(key);
                continue;
            }
            value = PyObject_GetItem(b, key);
            if (value == NULL) {
                Py_DECREF(iter);
                Py_DECREF(key);
                return -1;
            }
            status = PyDict_SetItem(a, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (status < 0) {
                Py_DECREF(iter);
                return -1;
            }
        }
        Py_DECREF(iter);
        if (PyErr_Occurred())
            /* Iterator completed, via error */
            return -1;
    }
    return 0;
}

static PyObject *
dict_copy(register PyDictObject *mp)
{
    return PyDict_Copy((PyObject*)mp);
}

PyObject *
PyDict_Copy(PyObject *o)
{
    PyObject *copy;
    PyDictObject *mp;
    Py_ssize_t i, n;

    if (o == NULL || !PyDict_Check(o)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    mp = (PyDictObject *)o;
    if (_PyDict_HasSplitTable(mp)) {
        PyDictObject *split_copy;
        PyObject **newvalues = new_values(DK_SIZE(mp->ma_keys));
        if (newvalues == NULL)
            return PyErr_NoMemory();
        split_copy = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (split_copy == NULL) {
            free_values(newvalues);
            return NULL;
        }
        split_copy->ma_values = newvalues;
        split_copy->ma_keys = mp->ma_keys;
        split_copy->ma_used = mp->ma_used;
        DK_INCREF(mp->ma_keys);
        for (i = 0, n = DK_SIZE(mp->ma_keys); i < n; i++) {
            PyObject *value = mp->ma_values[i];
            Py_XINCREF(value);
            split_copy->ma_values[i] = value;
        }
        if (_PyObject_GC_IS_TRACKED(mp))
            _PyObject_GC_TRACK(split_copy);
        return (PyObject *)split_copy;
    }
    copy = PyDict_New();
    if (copy == NULL)
        return NULL;
    if (PyDict_Merge(copy, o, 1) == 0)
        return copy;
    Py_DECREF(copy);
    return NULL;
}

Py_ssize_t
PyDict_Size(PyObject *mp)
{
    if (mp == NULL || !PyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return ((PyDictObject *)mp)->ma_used;
}

PyObject *
PyDict_Keys(PyObject *mp)
{
    if (mp == NULL || !PyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return dict_keys((PyDictObject *)mp);
}

PyObject *
PyDict_Values(PyObject *mp)
{
    if (mp == NULL || !PyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return dict_values((PyDictObject *)mp);
}

PyObject *
PyDict_Items(PyObject *mp)
{
    if (mp == NULL || !PyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return dict_items((PyDictObject *)mp);
}

/* Return 1 if dicts equal, 0 if not, -1 if error.
 * Gets out as soon as any difference is detected.
 * Uses only Py_EQ comparison.
 */
static int
dict_equal(PyDictObject *a, PyDictObject *b)
{
    Py_ssize_t i;

    if (a->ma_used != b->ma_used)
        /* can't be equal if # of entries differ */
        return 0;
    /* Same # of entries -- check all of 'em.  Exit early on any diff. */
    for (i = 0; i < DK_SIZE(a->ma_keys); i++) {
        PyDictKeyEntry *ep = &a->ma_keys->dk_entries[i];
        PyObject *aval;
        if (a->ma_values)
            aval = a->ma_values[i];
        else
            aval = ep->me_value;
        if (aval != NULL) {
            int cmp;
            PyObject *bval;
            PyObject *key = ep->me_key;
            /* temporarily bump aval's refcount to ensure it stays
               alive until we're done with it */
            Py_INCREF(aval);
            /* ditto for key */
            Py_INCREF(key);
            bval = PyDict_GetItemWithError((PyObject *)b, key);
            Py_DECREF(key);
            if (bval == NULL) {
                Py_DECREF(aval);
                if (PyErr_Occurred())
                    return -1;
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
        cmp = dict_equal((PyDictObject *)v, (PyDictObject *)w);
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
dict_contains(register PyDictObject *mp, PyObject *key)
{
    Py_hash_t hash;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    return PyBool_FromLong(*value_addr != NULL);
}

static PyObject *
dict_get(register PyDictObject *mp, PyObject *args)
{
    PyObject *key;
    PyObject *failobj = Py_None;
    PyObject *val = NULL;
    Py_hash_t hash;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &failobj))
        return NULL;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    val = *value_addr;
    if (val == NULL)
        val = failobj;
    Py_INCREF(val);
    return val;
}

static PyObject *
dict_setdefault(register PyDictObject *mp, PyObject *args)
{
    PyObject *key;
    PyObject *failobj = Py_None;
    PyObject *val = NULL;
    Py_hash_t hash;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyArg_UnpackTuple(args, "setdefault", 1, 2, &key, &failobj))
        return NULL;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    val = *value_addr;
    if (val == NULL) {
        if (mp->ma_keys->dk_usable <= 0) {
            /* Need to resize. */
            if (insertion_resize(mp) < 0)
                return NULL;
            ep = find_empty_slot(mp, key, hash, &value_addr);
        }
        Py_INCREF(failobj);
        Py_INCREF(key);
        MAINTAIN_TRACKING(mp, key, failobj);
        ep->me_key = key;
        ep->me_hash = hash;
        *value_addr = failobj;
        val = failobj;
        mp->ma_keys->dk_usable--;
        mp->ma_used++;
    }
    Py_INCREF(val);
    return val;
}


static PyObject *
dict_clear(register PyDictObject *mp)
{
    PyDict_Clear((PyObject *)mp);
    Py_RETURN_NONE;
}

static PyObject *
dict_pop(PyDictObject *mp, PyObject *args)
{
    Py_hash_t hash;
    PyObject *old_value, *old_key;
    PyObject *key, *deflt = NULL;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if(!PyArg_UnpackTuple(args, "pop", 1, 2, &key, &deflt))
        return NULL;
    if (mp->ma_used == 0) {
        if (deflt) {
            Py_INCREF(deflt);
            return deflt;
        }
        set_key_error(key);
        return NULL;
    }
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    if (ep == NULL)
        return NULL;
    old_value = *value_addr;
    if (old_value == NULL) {
        if (deflt) {
            Py_INCREF(deflt);
            return deflt;
        }
        set_key_error(key);
        return NULL;
    }
    *value_addr = NULL;
    mp->ma_used--;
    if (!_PyDict_HasSplitTable(mp)) {
        ENSURE_ALLOWS_DELETIONS(mp);
        old_key = ep->me_key;
        Py_INCREF(dummy);
        ep->me_key = dummy;
        Py_DECREF(old_key);
    }
    return old_value;
}

static PyObject *
dict_popitem(PyDictObject *mp)
{
    Py_hash_t i = 0;
    PyDictKeyEntry *ep;
    PyObject *res;


    /* Allocate the result tuple before checking the size.  Believe it
     * or not, this allocation could trigger a garbage collection which
     * could empty the dict, so if we checked the size first and that
     * happened, the result would be an infinite loop (searching for an
     * entry that no longer exists).  Note that the usual popitem()
     * idiom is "while d: k, v = d.popitem()". so needing to throw the
     * tuple away if the dict *is* empty isn't a significant
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
    /* Convert split table to combined table */
    if (mp->ma_keys->dk_lookup == lookdict_split) {
        if (dictresize(mp, DK_SIZE(mp->ma_keys))) {
            Py_DECREF(res);
            return NULL;
        }
    }
    ENSURE_ALLOWS_DELETIONS(mp);
    /* Set ep to "the first" dict entry with a value.  We abuse the hash
     * field of slot 0 to hold a search finger:
     * If slot 0 has a value, use slot 0.
     * Else slot 0 is being used to hold a search finger,
     * and we use its hash value as the first index to look.
     */
    ep = &mp->ma_keys->dk_entries[0];
    if (ep->me_value == NULL) {
        Py_ssize_t mask = DK_MASK(mp->ma_keys);
        i = ep->me_hash;
        /* The hash field may be a real hash value, or it may be a
         * legit search finger, or it may be a once-legit search
         * finger that's out of bounds now because it wrapped around
         * or the table shrunk -- simply make sure it's in bounds now.
         */
        if (i > mask || i < 1)
            i = 1;              /* skip slot 0 */
        while ((ep = &mp->ma_keys->dk_entries[i])->me_value == NULL) {
            i++;
            if (i > mask)
                i = 1;
        }
    }
    PyTuple_SET_ITEM(res, 0, ep->me_key);
    PyTuple_SET_ITEM(res, 1, ep->me_value);
    Py_INCREF(dummy);
    ep->me_key = dummy;
    ep->me_value = NULL;
    mp->ma_used--;
    assert(mp->ma_keys->dk_entries[0].me_value == NULL);
    mp->ma_keys->dk_entries[0].me_hash = i + 1;  /* next place to start */
    return res;
}

static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
    Py_ssize_t i, n;
    PyDictObject *mp = (PyDictObject *)op;
    if (mp->ma_keys->dk_lookup == lookdict) {
        for (i = 0; i < DK_SIZE(mp->ma_keys); i++) {
            if (mp->ma_keys->dk_entries[i].me_value != NULL) {
                Py_VISIT(mp->ma_keys->dk_entries[i].me_value);
                Py_VISIT(mp->ma_keys->dk_entries[i].me_key);
            }
        }
    } else {
        if (mp->ma_values != NULL) {
            for (i = 0, n = DK_SIZE(mp->ma_keys); i < n; i++) {
                Py_VISIT(mp->ma_values[i]);
            }
        }
        else {
            for (i = 0, n = DK_SIZE(mp->ma_keys); i < n; i++) {
                Py_VISIT(mp->ma_keys->dk_entries[i].me_value);
            }
        }
    }
    return 0;
}

static int
dict_tp_clear(PyObject *op)
{
    PyDict_Clear(op);
    return 0;
}

static PyObject *dictiter_new(PyDictObject *, PyTypeObject *);

static PyObject *
dict_sizeof(PyDictObject *mp)
{
    Py_ssize_t size, res;

    size = DK_SIZE(mp->ma_keys);
    res = sizeof(PyDictObject);
    if (mp->ma_values)
        res += size * sizeof(PyObject*);
    /* If the dictionary is split, the keys portion is accounted-for
       in the type object. */
    if (mp->ma_keys->dk_refcnt == 1)
        res += sizeof(PyDictKeysObject) + (size-1) * sizeof(PyDictKeyEntry);
    return PyLong_FromSsize_t(res);
}

Py_ssize_t
_PyDict_KeysSize(PyDictKeysObject *keys)
{
    return sizeof(PyDictKeysObject) + (DK_SIZE(keys)-1) * sizeof(PyDictKeyEntry);
}

PyDoc_STRVAR(contains__doc__,
"D.__contains__(k) -> True if D has a key k, else False");

PyDoc_STRVAR(getitem__doc__, "x.__getitem__(y) <==> x[y]");

PyDoc_STRVAR(sizeof__doc__,
"D.__sizeof__() -> size of D in memory, in bytes");

PyDoc_STRVAR(get__doc__,
"D.get(k[,d]) -> D[k] if k in D, else d.  d defaults to None.");

PyDoc_STRVAR(setdefault_doc__,
"D.setdefault(k[,d]) -> D.get(k,d), also set D[k]=d if k not in D");

PyDoc_STRVAR(pop__doc__,
"D.pop(k[,d]) -> v, remove specified key and return the corresponding value.\n\
If key is not found, d is returned if given, otherwise KeyError is raised");

PyDoc_STRVAR(popitem__doc__,
"D.popitem() -> (k, v), remove and return some (key, value) pair as a\n\
2-tuple; but raise KeyError if D is empty.");

PyDoc_STRVAR(update__doc__,
"D.update([E, ]**F) -> None.  Update D from dict/iterable E and F.\n"
"If E present and has a .keys() method, does:     for k in E: D[k] = E[k]\n\
If E present and lacks .keys() method, does:     for (k, v) in E: D[k] = v\n\
In either case, this is followed by: for k in F: D[k] = F[k]");

PyDoc_STRVAR(fromkeys__doc__,
"dict.fromkeys(S[,v]) -> New dict with keys from S and values equal to v.\n\
v defaults to None.");

PyDoc_STRVAR(clear__doc__,
"D.clear() -> None.  Remove all items from D.");

PyDoc_STRVAR(copy__doc__,
"D.copy() -> a shallow copy of D");

/* Forward */
static PyObject *dictkeys_new(PyObject *);
static PyObject *dictitems_new(PyObject *);
static PyObject *dictvalues_new(PyObject *);

PyDoc_STRVAR(keys__doc__,
             "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(items__doc__,
             "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(values__doc__,
             "D.values() -> an object providing a view on D's values");

static PyMethodDef mapp_methods[] = {
    {"__contains__",(PyCFunction)dict_contains,     METH_O | METH_COEXIST,
     contains__doc__},
    {"__getitem__", (PyCFunction)dict_subscript,        METH_O | METH_COEXIST,
     getitem__doc__},
    {"__sizeof__",      (PyCFunction)dict_sizeof,       METH_NOARGS,
     sizeof__doc__},
    {"get",         (PyCFunction)dict_get,          METH_VARARGS,
     get__doc__},
    {"setdefault",  (PyCFunction)dict_setdefault,   METH_VARARGS,
     setdefault_doc__},
    {"pop",         (PyCFunction)dict_pop,          METH_VARARGS,
     pop__doc__},
    {"popitem",         (PyCFunction)dict_popitem,      METH_NOARGS,
     popitem__doc__},
    {"keys",            (PyCFunction)dictkeys_new,      METH_NOARGS,
    keys__doc__},
    {"items",           (PyCFunction)dictitems_new,     METH_NOARGS,
    items__doc__},
    {"values",          (PyCFunction)dictvalues_new,    METH_NOARGS,
    values__doc__},
    {"update",          (PyCFunction)dict_update,       METH_VARARGS | METH_KEYWORDS,
     update__doc__},
    {"fromkeys",        (PyCFunction)dict_fromkeys,     METH_VARARGS | METH_CLASS,
     fromkeys__doc__},
    {"clear",           (PyCFunction)dict_clear,        METH_NOARGS,
     clear__doc__},
    {"copy",            (PyCFunction)dict_copy,         METH_NOARGS,
     copy__doc__},
    {NULL,              NULL}   /* sentinel */
};

/* Return 1 if `key` is in dict `op`, 0 if not, and -1 on error. */
int
PyDict_Contains(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    return (ep == NULL) ? -1 : (*value_addr != NULL);
}

/* Internal version of PyDict_Contains used when the hash value is already known */
int
_PyDict_Contains(PyObject *op, PyObject *key, Py_hash_t hash)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeyEntry *ep;
    PyObject **value_addr;

    ep = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr);
    return (ep == NULL) ? -1 : (*value_addr != NULL);
}

/* Hack to implement "key in dict" */
static PySequenceMethods dict_as_sequence = {
    0,                          /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    0,                          /* sq_item */
    0,                          /* sq_slice */
    0,                          /* sq_ass_item */
    0,                          /* sq_ass_slice */
    PyDict_Contains,            /* sq_contains */
    0,                          /* sq_inplace_concat */
    0,                          /* sq_inplace_repeat */
};

static PyObject *
dict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = type->tp_alloc(type, 0);
    if (self != NULL) {
        PyDictObject *d = (PyDictObject *)self;
        d->ma_keys = new_keys_object(PyDict_MINSIZE_COMBINED);
        /* XXX - Should we raise a no-memory error? */
        if (d->ma_keys == NULL) {
            DK_INCREF(Py_EMPTY_KEYS);
            d->ma_keys = Py_EMPTY_KEYS;
            d->ma_values = empty_values;
        }
        d->ma_used = 0;
        /* The object has been implicitly tracked by tp_alloc */
        if (type == &PyDict_Type)
            _PyObject_GC_UNTRACK(d);
    }
    return self;
}

static int
dict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return dict_update_common(self, args, kwds, "dict");
}

static PyObject *
dict_iter(PyDictObject *dict)
{
    return dictiter_new(dict, &PyDictIterKey_Type);
}

PyDoc_STRVAR(dictionary_doc,
"dict() -> new empty dictionary\n"
"dict(mapping) -> new dictionary initialized from a mapping object's\n"
"    (key, value) pairs\n"
"dict(iterable) -> new dictionary initialized as if via:\n"
"    d = {}\n"
"    for k, v in iterable:\n"
"        d[k] = v\n"
"dict(**kwargs) -> new dictionary initialized with the name=value pairs\n"
"    in the keyword argument list.  For example:  dict(one=1, two=2)");

PyTypeObject PyDict_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict",
    sizeof(PyDictObject),
    0,
    (destructor)dict_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)dict_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    &dict_as_sequence,                          /* tp_as_sequence */
    &dict_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DICT_SUBCLASS,         /* tp_flags */
    dictionary_doc,                             /* tp_doc */
    dict_traverse,                              /* tp_traverse */
    dict_tp_clear,                              /* tp_clear */
    dict_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)dict_iter,                     /* tp_iter */
    0,                                          /* tp_iternext */
    mapp_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    dict_init,                                  /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    dict_new,                                   /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

PyObject *
_PyDict_GetItemId(PyObject *dp, struct _Py_Identifier *key)
{
    PyObject *kv;
    kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL)
        return NULL;
    return PyDict_GetItem(dp, kv);
}

/* For backward compatibility with old dictionary interface */

PyObject *
PyDict_GetItemString(PyObject *v, const char *key)
{
    PyObject *kv, *rv;
    kv = PyUnicode_FromString(key);
    if (kv == NULL)
        return NULL;
    rv = PyDict_GetItem(v, kv);
    Py_DECREF(kv);
    return rv;
}

int
_PyDict_SetItemId(PyObject *v, struct _Py_Identifier *key, PyObject *item)
{
    PyObject *kv;
    kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL)
        return -1;
    return PyDict_SetItem(v, kv, item);
}

int
PyDict_SetItemString(PyObject *v, const char *key, PyObject *item)
{
    PyObject *kv;
    int err;
    kv = PyUnicode_FromString(key);
    if (kv == NULL)
        return -1;
    PyUnicode_InternInPlace(&kv); /* XXX Should we really? */
    err = PyDict_SetItem(v, kv, item);
    Py_DECREF(kv);
    return err;
}

int
PyDict_DelItemString(PyObject *v, const char *key)
{
    PyObject *kv;
    int err;
    kv = PyUnicode_FromString(key);
    if (kv == NULL)
        return -1;
    err = PyDict_DelItem(v, kv);
    Py_DECREF(kv);
    return err;
}

/* Dictionary iterator types */

typedef struct {
    PyObject_HEAD
    PyDictObject *di_dict; /* Set to NULL when iterator is exhausted */
    Py_ssize_t di_used;
    Py_ssize_t di_pos;
    PyObject* di_result; /* reusable result tuple for iteritems */
    Py_ssize_t len;
} dictiterobject;

static PyObject *
dictiter_new(PyDictObject *dict, PyTypeObject *itertype)
{
    dictiterobject *di;
    di = PyObject_GC_New(dictiterobject, itertype);
    if (di == NULL)
        return NULL;
    Py_INCREF(dict);
    di->di_dict = dict;
    di->di_used = dict->ma_used;
    di->di_pos = 0;
    di->len = dict->ma_used;
    if (itertype == &PyDictIterItem_Type) {
        di->di_result = PyTuple_Pack(2, Py_None, Py_None);
        if (di->di_result == NULL) {
            Py_DECREF(di);
            return NULL;
        }
    }
    else
        di->di_result = NULL;
    _PyObject_GC_TRACK(di);
    return (PyObject *)di;
}

static void
dictiter_dealloc(dictiterobject *di)
{
    Py_XDECREF(di->di_dict);
    Py_XDECREF(di->di_result);
    PyObject_GC_Del(di);
}

static int
dictiter_traverse(dictiterobject *di, visitproc visit, void *arg)
{
    Py_VISIT(di->di_dict);
    Py_VISIT(di->di_result);
    return 0;
}

static PyObject *
dictiter_len(dictiterobject *di)
{
    Py_ssize_t len = 0;
    if (di->di_dict != NULL && di->di_used == di->di_dict->ma_used)
        len = di->len;
    return PyLong_FromSize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyObject *
dictiter_reduce(dictiterobject *di);

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef dictiter_methods[] = {
    {"__length_hint__", (PyCFunction)dictiter_len, METH_NOARGS,
     length_hint_doc},
     {"__reduce__", (PyCFunction)dictiter_reduce, METH_NOARGS,
     reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyObject *dictiter_iternextkey(dictiterobject *di)
{
    PyObject *key;
    register Py_ssize_t i, mask, offset;
    register PyDictKeysObject *k;
    PyDictObject *d = di->di_dict;
    PyObject **value_ptr;

    if (d == NULL)
        return NULL;
    assert (PyDict_Check(d));

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    i = di->di_pos;
    if (i < 0)
        goto fail;
    k = d->ma_keys;
    if (d->ma_values) {
        value_ptr = &d->ma_values[i];
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &k->dk_entries[i].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    mask = DK_SIZE(k)-1;
    while (i <= mask && *value_ptr == NULL) {
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        i++;
    }
    di->di_pos = i+1;
    if (i > mask)
        goto fail;
    di->len--;
    key = k->dk_entries[i].me_key;
    Py_INCREF(key);
    return key;

fail:
    Py_DECREF(d);
    di->di_dict = NULL;
    return NULL;
}

PyTypeObject PyDictIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keyiterator",                         /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictiter_dealloc,               /* tp_dealloc */
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
    (traverseproc)dictiter_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)dictiter_iternextkey,         /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};

static PyObject *dictiter_iternextvalue(dictiterobject *di)
{
    PyObject *value;
    register Py_ssize_t i, mask, offset;
    PyDictObject *d = di->di_dict;
    PyObject **value_ptr;

    if (d == NULL)
        return NULL;
    assert (PyDict_Check(d));

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    i = di->di_pos;
    mask = DK_SIZE(d->ma_keys)-1;
    if (i < 0 || i > mask)
        goto fail;
    if (d->ma_values) {
        value_ptr = &d->ma_values[i];
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &d->ma_keys->dk_entries[i].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    while (i <= mask && *value_ptr == NULL) {
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        i++;
        if (i > mask)
            goto fail;
    }
    di->di_pos = i+1;
    di->len--;
    value = *value_ptr;
    Py_INCREF(value);
    return value;

fail:
    Py_DECREF(d);
    di->di_dict = NULL;
    return NULL;
}

PyTypeObject PyDictIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_valueiterator",                       /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictiter_dealloc,               /* tp_dealloc */
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
    (traverseproc)dictiter_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)dictiter_iternextvalue,       /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};

static PyObject *dictiter_iternextitem(dictiterobject *di)
{
    PyObject *key, *value, *result = di->di_result;
    register Py_ssize_t i, mask, offset;
    PyDictObject *d = di->di_dict;
    PyObject **value_ptr;

    if (d == NULL)
        return NULL;
    assert (PyDict_Check(d));

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    i = di->di_pos;
    if (i < 0)
        goto fail;
    mask = DK_SIZE(d->ma_keys)-1;
    if (d->ma_values) {
        value_ptr = &d->ma_values[i];
        offset = sizeof(PyObject *);
    }
    else {
        value_ptr = &d->ma_keys->dk_entries[i].me_value;
        offset = sizeof(PyDictKeyEntry);
    }
    while (i <= mask && *value_ptr == NULL) {
        value_ptr = (PyObject **)(((char *)value_ptr) + offset);
        i++;
    }
    di->di_pos = i+1;
    if (i > mask)
        goto fail;

    if (result->ob_refcnt == 1) {
        Py_INCREF(result);
        Py_DECREF(PyTuple_GET_ITEM(result, 0));
        Py_DECREF(PyTuple_GET_ITEM(result, 1));
    } else {
        result = PyTuple_New(2);
        if (result == NULL)
            return NULL;
    }
    di->len--;
    key = d->ma_keys->dk_entries[i].me_key;
    value = *value_ptr;
    Py_INCREF(key);
    Py_INCREF(value);
    PyTuple_SET_ITEM(result, 0, key);
    PyTuple_SET_ITEM(result, 1, value);
    return result;

fail:
    Py_DECREF(d);
    di->di_dict = NULL;
    return NULL;
}

PyTypeObject PyDictIterItem_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_itemiterator",                        /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictiter_dealloc,               /* tp_dealloc */
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
    (traverseproc)dictiter_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    (iternextfunc)dictiter_iternextitem,        /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};


static PyObject *
dictiter_reduce(dictiterobject *di)
{
    PyObject *list;
    dictiterobject tmp;

    list = PyList_New(0);
    if (!list)
        return NULL;

    /* copy the itertor state */
    tmp = *di;
    Py_XINCREF(tmp.di_dict);

    /* iterate the temporary into a list */
    for(;;) {
        PyObject *element = 0;
        if (Py_TYPE(di) == &PyDictIterItem_Type)
            element = dictiter_iternextitem(&tmp);
        else if (Py_TYPE(di) == &PyDictIterKey_Type)
            element = dictiter_iternextkey(&tmp);
        else if (Py_TYPE(di) == &PyDictIterValue_Type)
            element = dictiter_iternextvalue(&tmp);
        else
            assert(0);
        if (element) {
            if (PyList_Append(list, element)) {
                Py_DECREF(element);
                Py_DECREF(list);
                Py_XDECREF(tmp.di_dict);
                return NULL;
            }
            Py_DECREF(element);
        } else
            break;
    }
    Py_XDECREF(tmp.di_dict);
    /* check for error */
    if (tmp.di_dict != NULL) {
        /* we have an error */
        Py_DECREF(list);
        return NULL;
    }
    return Py_BuildValue("N(N)", _PyObject_GetBuiltin("iter"), list);
}

/***********************************************/
/* View objects for keys(), items(), values(). */
/***********************************************/

/* The instance lay-out is the same for all three; but the type differs. */

typedef struct {
    PyObject_HEAD
    PyDictObject *dv_dict;
} dictviewobject;


static void
dictview_dealloc(dictviewobject *dv)
{
    Py_XDECREF(dv->dv_dict);
    PyObject_GC_Del(dv);
}

static int
dictview_traverse(dictviewobject *dv, visitproc visit, void *arg)
{
    Py_VISIT(dv->dv_dict);
    return 0;
}

static Py_ssize_t
dictview_len(dictviewobject *dv)
{
    Py_ssize_t len = 0;
    if (dv->dv_dict != NULL)
        len = dv->dv_dict->ma_used;
    return len;
}

static PyObject *
dictview_new(PyObject *dict, PyTypeObject *type)
{
    dictviewobject *dv;
    if (dict == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!PyDict_Check(dict)) {
        /* XXX Get rid of this restriction later */
        PyErr_Format(PyExc_TypeError,
                     "%s() requires a dict argument, not '%s'",
                     type->tp_name, dict->ob_type->tp_name);
        return NULL;
    }
    dv = PyObject_GC_New(dictviewobject, type);
    if (dv == NULL)
        return NULL;
    Py_INCREF(dict);
    dv->dv_dict = (PyDictObject *)dict;
    _PyObject_GC_TRACK(dv);
    return (PyObject *)dv;
}

/* TODO(guido): The views objects are not complete:

 * support more set operations
 * support arbitrary mappings?
   - either these should be static or exported in dictobject.h
   - if public then they should probably be in builtins
*/

/* Return 1 if self is a subset of other, iterating over self;
   0 if not; -1 if an error occurred. */
static int
all_contained_in(PyObject *self, PyObject *other)
{
    PyObject *iter = PyObject_GetIter(self);
    int ok = 1;

    if (iter == NULL)
        return -1;
    for (;;) {
        PyObject *next = PyIter_Next(iter);
        if (next == NULL) {
            if (PyErr_Occurred())
                ok = -1;
            break;
        }
        ok = PySequence_Contains(other, next);
        Py_DECREF(next);
        if (ok <= 0)
            break;
    }
    Py_DECREF(iter);
    return ok;
}

static PyObject *
dictview_richcompare(PyObject *self, PyObject *other, int op)
{
    Py_ssize_t len_self, len_other;
    int ok;
    PyObject *result;

    assert(self != NULL);
    assert(PyDictViewSet_Check(self));
    assert(other != NULL);

    if (!PyAnySet_Check(other) && !PyDictViewSet_Check(other))
        Py_RETURN_NOTIMPLEMENTED;

    len_self = PyObject_Size(self);
    if (len_self < 0)
        return NULL;
    len_other = PyObject_Size(other);
    if (len_other < 0)
        return NULL;

    ok = 0;
    switch(op) {

    case Py_NE:
    case Py_EQ:
        if (len_self == len_other)
            ok = all_contained_in(self, other);
        if (op == Py_NE && ok >= 0)
            ok = !ok;
        break;

    case Py_LT:
        if (len_self < len_other)
            ok = all_contained_in(self, other);
        break;

      case Py_LE:
          if (len_self <= len_other)
              ok = all_contained_in(self, other);
          break;

    case Py_GT:
        if (len_self > len_other)
            ok = all_contained_in(other, self);
        break;

    case Py_GE:
        if (len_self >= len_other)
            ok = all_contained_in(other, self);
        break;

    }
    if (ok < 0)
        return NULL;
    result = ok ? Py_True : Py_False;
    Py_INCREF(result);
    return result;
}

static PyObject *
dictview_repr(dictviewobject *dv)
{
    PyObject *seq;
    PyObject *result;

    seq = PySequence_List((PyObject *)dv);
    if (seq == NULL)
        return NULL;

    result = PyUnicode_FromFormat("%s(%R)", Py_TYPE(dv)->tp_name, seq);
    Py_DECREF(seq);
    return result;
}

/*** dict_keys ***/

static PyObject *
dictkeys_iter(dictviewobject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterKey_Type);
}

static int
dictkeys_contains(dictviewobject *dv, PyObject *obj)
{
    if (dv->dv_dict == NULL)
        return 0;
    return PyDict_Contains((PyObject *)dv->dv_dict, obj);
}

static PySequenceMethods dictkeys_as_sequence = {
    (lenfunc)dictview_len,              /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)dictkeys_contains,      /* sq_contains */
};

static PyObject*
dictviews_sub(PyObject* self, PyObject *other)
{
    PyObject *result = PySet_New(self);
    PyObject *tmp;
    _Py_IDENTIFIER(difference_update);

    if (result == NULL)
        return NULL;

    tmp = _PyObject_CallMethodId(result, &PyId_difference_update, "O", other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static PyObject*
dictviews_and(PyObject* self, PyObject *other)
{
    PyObject *result = PySet_New(self);
    PyObject *tmp;
    _Py_IDENTIFIER(intersection_update);

    if (result == NULL)
        return NULL;

    tmp = _PyObject_CallMethodId(result, &PyId_intersection_update, "O", other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static PyObject*
dictviews_or(PyObject* self, PyObject *other)
{
    PyObject *result = PySet_New(self);
    PyObject *tmp;
    _Py_IDENTIFIER(update);

    if (result == NULL)
        return NULL;

    tmp = _PyObject_CallMethodId(result, &PyId_update, "O", other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static PyObject*
dictviews_xor(PyObject* self, PyObject *other)
{
    PyObject *result = PySet_New(self);
    PyObject *tmp;
    _Py_IDENTIFIER(symmetric_difference_update);

    if (result == NULL)
        return NULL;

    tmp = _PyObject_CallMethodId(result, &PyId_symmetric_difference_update, "O",
                              other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static PyNumberMethods dictviews_as_number = {
    0,                                  /*nb_add*/
    (binaryfunc)dictviews_sub,          /*nb_subtract*/
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
    (binaryfunc)dictviews_and,          /*nb_and*/
    (binaryfunc)dictviews_xor,          /*nb_xor*/
    (binaryfunc)dictviews_or,           /*nb_or*/
};

static PyObject*
dictviews_isdisjoint(PyObject *self, PyObject *other)
{
    PyObject *it;
    PyObject *item = NULL;

    if (self == other) {
        if (dictview_len((dictviewobject *)self) == 0)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    /* Iterate over the shorter object (only if other is a set,
     * because PySequence_Contains may be expensive otherwise): */
    if (PyAnySet_Check(other) || PyDictViewSet_Check(other)) {
        Py_ssize_t len_self = dictview_len((dictviewobject *)self);
        Py_ssize_t len_other = PyObject_Size(other);
        if (len_other == -1)
            return NULL;

        if ((len_other > len_self)) {
            PyObject *tmp = other;
            other = self;
            self = tmp;
        }
    }

    it = PyObject_GetIter(other);
    if (it == NULL)
        return NULL;

    while ((item = PyIter_Next(it)) != NULL) {
        int contains = PySequence_Contains(self, item);
        Py_DECREF(item);
        if (contains == -1) {
            Py_DECREF(it);
            return NULL;
        }

        if (contains) {
            Py_DECREF(it);
            Py_RETURN_FALSE;
        }
    }
    Py_DECREF(it);
    if (PyErr_Occurred())
        return NULL; /* PyIter_Next raised an exception. */
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(isdisjoint_doc,
"Return True if the view and the given iterable have a null intersection.");

static PyMethodDef dictkeys_methods[] = {
    {"isdisjoint",      (PyCFunction)dictviews_isdisjoint,  METH_O,
     isdisjoint_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keys",                                /* tp_name */
    sizeof(dictviewobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)dictview_repr,                    /* tp_repr */
    &dictviews_as_number,                       /* tp_as_number */
    &dictkeys_as_sequence,                      /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dictview_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)dictkeys_iter,                 /* tp_iter */
    0,                                          /* tp_iternext */
    dictkeys_methods,                           /* tp_methods */
    0,
};

static PyObject *
dictkeys_new(PyObject *dict)
{
    return dictview_new(dict, &PyDictKeys_Type);
}

/*** dict_items ***/

static PyObject *
dictitems_iter(dictviewobject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterItem_Type);
}

static int
dictitems_contains(dictviewobject *dv, PyObject *obj)
{
    PyObject *key, *value, *found;
    if (dv->dv_dict == NULL)
        return 0;
    if (!PyTuple_Check(obj) || PyTuple_GET_SIZE(obj) != 2)
        return 0;
    key = PyTuple_GET_ITEM(obj, 0);
    value = PyTuple_GET_ITEM(obj, 1);
    found = PyDict_GetItem((PyObject *)dv->dv_dict, key);
    if (found == NULL) {
        if (PyErr_Occurred())
            return -1;
        return 0;
    }
    return PyObject_RichCompareBool(value, found, Py_EQ);
}

static PySequenceMethods dictitems_as_sequence = {
    (lenfunc)dictview_len,              /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)dictitems_contains,     /* sq_contains */
};

static PyMethodDef dictitems_methods[] = {
    {"isdisjoint",      (PyCFunction)dictviews_isdisjoint,  METH_O,
     isdisjoint_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_items",                               /* tp_name */
    sizeof(dictviewobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)dictview_repr,                    /* tp_repr */
    &dictviews_as_number,                       /* tp_as_number */
    &dictitems_as_sequence,                     /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dictview_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)dictitems_iter,                /* tp_iter */
    0,                                          /* tp_iternext */
    dictitems_methods,                          /* tp_methods */
    0,
};

static PyObject *
dictitems_new(PyObject *dict)
{
    return dictview_new(dict, &PyDictItems_Type);
}

/*** dict_values ***/

static PyObject *
dictvalues_iter(dictviewobject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterValue_Type);
}

static PySequenceMethods dictvalues_as_sequence = {
    (lenfunc)dictview_len,              /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    (objobjproc)0,                      /* sq_contains */
};

static PyMethodDef dictvalues_methods[] = {
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_values",                              /* tp_name */
    sizeof(dictviewobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)dictview_repr,                    /* tp_repr */
    0,                                          /* tp_as_number */
    &dictvalues_as_sequence,                    /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dictview_traverse,            /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    (getiterfunc)dictvalues_iter,               /* tp_iter */
    0,                                          /* tp_iternext */
    dictvalues_methods,                         /* tp_methods */
    0,
};

static PyObject *
dictvalues_new(PyObject *dict)
{
    return dictview_new(dict, &PyDictValues_Type);
}

/* Returns NULL if cannot allocate a new PyDictKeysObject,
   but does not set an error */
PyDictKeysObject *
_PyDict_NewKeysForClass(void)
{
    PyDictKeysObject *keys = new_keys_object(PyDict_MINSIZE_SPLIT);
    if (keys == NULL)
        PyErr_Clear();
    else
        keys->dk_lookup = lookdict_split;
    return keys;
}

#define CACHED_KEYS(tp) (((PyHeapTypeObject*)tp)->ht_cached_keys)

PyObject *
PyObject_GenericGetDict(PyObject *obj, void *context)
{
    PyObject *dict, **dictptr = _PyObject_GetDictPtr(obj);
    if (dictptr == NULL) {
        PyErr_SetString(PyExc_AttributeError,
                        "This object has no __dict__");
        return NULL;
    }
    dict = *dictptr;
    if (dict == NULL) {
        PyTypeObject *tp = Py_TYPE(obj);
        if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) && CACHED_KEYS(tp)) {
            DK_INCREF(CACHED_KEYS(tp));
            *dictptr = dict = new_dict_with_shared_keys(CACHED_KEYS(tp));
        }
        else {
            *dictptr = dict = PyDict_New();
        }
    }
    Py_XINCREF(dict);
    return dict;
}

int
_PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr,
                     PyObject *key, PyObject *value)
{
    PyObject *dict;
    int res;
    PyDictKeysObject *cached;

    assert(dictptr != NULL);
    if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) && (cached = CACHED_KEYS(tp))) {
        assert(dictptr != NULL);
        dict = *dictptr;
        if (dict == NULL) {
            DK_INCREF(cached);
            dict = new_dict_with_shared_keys(cached);
            if (dict == NULL)
                return -1;
            *dictptr = dict;
        }
        if (value == NULL) {
            res = PyDict_DelItem(dict, key);
            if (cached != ((PyDictObject *)dict)->ma_keys) {
                CACHED_KEYS(tp) = NULL;
                DK_DECREF(cached);
            }
        } else {
            res = PyDict_SetItem(dict, key, value);
            if (cached != ((PyDictObject *)dict)->ma_keys) {
                /* Either update tp->ht_cached_keys or delete it */
                if (cached->dk_refcnt == 1) {
                    CACHED_KEYS(tp) = make_keys_shared(dict);
                } else {
                    CACHED_KEYS(tp) = NULL;
                }
                DK_DECREF(cached);
                if (CACHED_KEYS(tp) == NULL && PyErr_Occurred())
                    return -1;
            }
        }
    } else {
        dict = *dictptr;
        if (dict == NULL) {
            dict = PyDict_New();
            if (dict == NULL)
                return -1;
            *dictptr = dict;
        }
        if (value == NULL) {
            res = PyDict_DelItem(dict, key);
        } else {
            res = PyDict_SetItem(dict, key, value);
        }
    }
    return res;
}

void
_PyDictKeys_DecRef(PyDictKeysObject *keys)
{
    DK_DECREF(keys);
}


/* ARGSUSED */
static PyObject *
dummy_repr(PyObject *op)
{
    return PyUnicode_FromString("<dummy key>");
}

/* ARGUSED */
static void
dummy_dealloc(PyObject* ignore)
{
    /* This should never get called, but we also don't want to SEGV if
     * we accidentally decref dummy-key out of existence.
     */
    Py_FatalError("deallocating <dummy key>");
}

static PyTypeObject PyDictDummy_Type = {
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
  2, &PyDictDummy_Type
};

