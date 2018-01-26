/* Dictionary object implementation using a hash table */

/* The distribution includes a separate file, Objects/dictnotes.txt,
   describing explorations into dictionary design and optimization.
   It covers typical dictionary use patterns, the parameters for
   tuning dictionaries, and several ideas for possible optimizations.
*/

/* PyDictKeysObject

This implements the dictionary's hashtable.

As of Python 3.6, this is compact and ordered. Basic idea is described here.
https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html

layout:

+---------------+
| dk_refcnt     |
| dk_size       |
| dk_lookup     |
| dk_usable     |
| dk_nentries   |
+---------------+
| dk_indices    |
|               |
+---------------+
| dk_entries    |
|               |
+---------------+

dk_indices is actual hashtable.  It holds index in entries, or DKIX_EMPTY(-1)
or DKIX_DUMMY(-2).
Size of indices is dk_size.  Type of each index in indices is vary on dk_size:

* int8  for          dk_size <= 128
* int16 for 256   <= dk_size <= 2**15
* int32 for 2**16 <= dk_size <= 2**31
* int64 for 2**32 <= dk_size

dk_entries is array of PyDictKeyEntry.  It's size is USABLE_FRACTION(dk_size).
DK_ENTRIES(dk) can be used to get pointer to entries.

NOTE: Since negative value is used for DKIX_EMPTY and DKIX_DUMMY, type of
dk_indices entry is signed integer and int16 is used for table which
dk_size == 256.
*/


/*
The DictObject can be in one of two forms.

Either:
  A combined table:
    ma_values == NULL, dk_refcnt == 1.
    Values are stored in the me_value field of the PyDictKeysObject.
Or:
  A split table:
    ma_values != NULL, dk_refcnt >= 1
    Values are stored in the ma_values array.
    Only string (unicode) keys are allowed.
    All dicts sharing same key must have same insertion order.

There are four kinds of slots in the table (slot is index, and
DK_ENTRIES(keys)[index] if index >= 0):

1. Unused.  index == DKIX_EMPTY
   Does not hold an active (key, value) pair now and never did.  Unused can
   transition to Active upon key insertion.  This is each slot's initial state.

2. Active.  index >= 0, me_key != NULL and me_value != NULL
   Holds an active (key, value) pair.  Active can transition to Dummy or
   Pending upon key deletion (for combined and split tables respectively).
   This is the only case in which me_value != NULL.

3. Dummy.  index == DKIX_DUMMY  (combined only)
   Previously held an active (key, value) pair, but that was deleted and an
   active pair has not yet overwritten the slot.  Dummy can transition to
   Active upon key insertion.  Dummy slots cannot be made Unused again
   else the probe sequence in case of collision would have no way to know
   they were once active.

4. Pending. index >= 0, key != NULL, and value == NULL  (split only)
   Not yet inserted in split-table.
*/

/*
Preserving insertion order

It's simple for combined table.  Since dk_entries is mostly append only, we can
get insertion order by just iterating dk_entries.

One exception is .popitem().  It removes last item in dk_entries and decrement
dk_nentries to achieve amortized O(1).  Since there are DKIX_DUMMY remains in
dk_indices, we can't increment dk_usable even though dk_nentries is
decremented.

In split table, inserting into pending entry is allowed only for dk_entries[ix]
where ix == mp->ma_used. Inserting into other index and deleting item cause
converting the dict to the combined table.
*/

/* PyDict_MINSIZE is the starting size for any new dict.
 * 8 allows dicts with no more than 5 active entries; experiments suggested
 * this suffices for the majority of dicts (consisting mostly of usually-small
 * dicts created to pass keyword arguments).
 * Making this 8, rather than 4 reduces the number of resizes for most
 * dictionaries, without any significant extra memory use.
 */
#define PyDict_MINSIZE 8

#include "Python.h"
#include "dict-common.h"
#include "stringlib/eq.h"    /* to get unicode_eq() */

/*[clinic input]
class dict "PyDictObject *" "&PyDict_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f157a5a0ce9589d6]*/


/*
To ensure the lookup algorithm terminates, there must be at least one Unused
slot (NULL key) in the table.
To avoid slowing down lookups on a near-full table, we resize the table when
it's USABLE_FRACTION (currently two-thirds) full.
*/

#define PERTURB_SHIFT 5

/*
Major subtleties ahead:  Most hash schemes depend on having a "good" hash
function, in the sense of simulating randomness.  Python doesn't:  its most
important hash functions (for ints) are very regular in common
cases:

  >>>[hash(i) for i in range(4)]
  [0, 1, 2, 3]

This isn't necessarily bad!  To the contrary, in a table of size 2**i, taking
the low-order i bits as the initial table index is extremely fast, and there
are no collisions at all for dicts indexed by a contiguous range of ints. So
this gives better-than-random behavior in common cases, and that's very
desirable.

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

    perturb >>= PERTURB_SHIFT;
    j = (5*j) + 1 + perturb;
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

/* forward declarations */
static Py_ssize_t lookdict(PyDictObject *mp, PyObject *key,
                           Py_hash_t hash, PyObject ***value_addr,
                           Py_ssize_t *hashpos);
static Py_ssize_t lookdict_unicode(PyDictObject *mp, PyObject *key,
                                   Py_hash_t hash, PyObject ***value_addr,
                                   Py_ssize_t *hashpos);
static Py_ssize_t
lookdict_unicode_nodummy(PyDictObject *mp, PyObject *key,
                         Py_hash_t hash, PyObject ***value_addr,
                         Py_ssize_t *hashpos);
static Py_ssize_t lookdict_split(PyDictObject *mp, PyObject *key,
                                 Py_hash_t hash, PyObject ***value_addr,
                                 Py_ssize_t *hashpos);

static int dictresize(PyDictObject *mp, Py_ssize_t minused);

/*Global counter used to set ma_version_tag field of dictionary.
 * It is incremented each time that a dictionary is created and each
 * time that a dictionary is modified. */
static uint64_t pydict_global_version = 0;

#define DICT_NEXT_VERSION() (++pydict_global_version)

/* Dictionary reuse scheme to save calls to malloc and free */
#ifndef PyDict_MAXFREELIST
#define PyDict_MAXFREELIST 80
#endif
static PyDictObject *free_list[PyDict_MAXFREELIST];
static int numfree = 0;
static PyDictKeysObject *keys_free_list[PyDict_MAXFREELIST];
static int numfreekeys = 0;

#include "clinic/dictobject.c.h"

int
PyDict_ClearFreeList(void)
{
    PyDictObject *op;
    int ret = numfree + numfreekeys;
    while (numfree) {
        op = free_list[--numfree];
        assert(PyDict_CheckExact(op));
        PyObject_GC_Del(op);
    }
    while (numfreekeys) {
        PyObject_FREE(keys_free_list[--numfreekeys]);
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

#define DK_SIZE(dk) ((dk)->dk_size)
#if SIZEOF_VOID_P > 4
#define DK_IXSIZE(dk)                          \
    (DK_SIZE(dk) <= 0xff ?                     \
        1 : DK_SIZE(dk) <= 0xffff ?            \
            2 : DK_SIZE(dk) <= 0xffffffff ?    \
                4 : sizeof(int64_t))
#else
#define DK_IXSIZE(dk)                          \
    (DK_SIZE(dk) <= 0xff ?                     \
        1 : DK_SIZE(dk) <= 0xffff ?            \
            2 : sizeof(int32_t))
#endif
#define DK_ENTRIES(dk) \
    ((PyDictKeyEntry*)(&(dk)->dk_indices.as_1[DK_SIZE(dk) * DK_IXSIZE(dk)]))

#define DK_DEBUG_INCREF _Py_INC_REFTOTAL _Py_REF_DEBUG_COMMA
#define DK_DEBUG_DECREF _Py_DEC_REFTOTAL _Py_REF_DEBUG_COMMA

#define DK_INCREF(dk) (DK_DEBUG_INCREF ++(dk)->dk_refcnt)
#define DK_DECREF(dk) if (DK_DEBUG_DECREF (--(dk)->dk_refcnt) == 0) free_keys_object(dk)
#define DK_MASK(dk) (((dk)->dk_size)-1)
#define IS_POWER_OF_2(x) (((x) & (x-1)) == 0)

/* lookup indices.  returns DKIX_EMPTY, DKIX_DUMMY, or ix >=0 */
static inline Py_ssize_t
dk_get_index(PyDictKeysObject *keys, Py_ssize_t i)
{
    Py_ssize_t s = DK_SIZE(keys);
    Py_ssize_t ix;

    if (s <= 0xff) {
        int8_t *indices = keys->dk_indices.as_1;
        ix = indices[i];
    }
    else if (s <= 0xffff) {
        int16_t *indices = keys->dk_indices.as_2;
        ix = indices[i];
    }
#if SIZEOF_VOID_P > 4
    else if (s > 0xffffffff) {
        int64_t *indices = keys->dk_indices.as_8;
        ix = indices[i];
    }
#endif
    else {
        int32_t *indices = keys->dk_indices.as_4;
        ix = indices[i];
    }
    assert(ix >= DKIX_DUMMY);
    return ix;
}

/* write to indices. */
static inline void
dk_set_index(PyDictKeysObject *keys, Py_ssize_t i, Py_ssize_t ix)
{
    Py_ssize_t s = DK_SIZE(keys);

    assert(ix >= DKIX_DUMMY);

    if (s <= 0xff) {
        int8_t *indices = keys->dk_indices.as_1;
        assert(ix <= 0x7f);
        indices[i] = (char)ix;
    }
    else if (s <= 0xffff) {
        int16_t *indices = keys->dk_indices.as_2;
        assert(ix <= 0x7fff);
        indices[i] = (int16_t)ix;
    }
#if SIZEOF_VOID_P > 4
    else if (s > 0xffffffff) {
        int64_t *indices = keys->dk_indices.as_8;
        indices[i] = ix;
    }
#endif
    else {
        int32_t *indices = keys->dk_indices.as_4;
        assert(ix <= 0x7fffffff);
        indices[i] = (int32_t)ix;
    }
}


/* USABLE_FRACTION is the maximum dictionary load.
 * Increasing this ratio makes dictionaries more dense resulting in more
 * collisions.  Decreasing it improves sparseness at the expense of spreading
 * indices over more cache lines and at the cost of total memory consumed.
 *
 * USABLE_FRACTION must obey the following:
 *     (0 < USABLE_FRACTION(n) < n) for all n >= 2
 *
 * USABLE_FRACTION should be quick to calculate.
 * Fractions around 1/2 to 2/3 seem to work well in practice.
 */
#define USABLE_FRACTION(n) (((n) << 1)/3)

/* ESTIMATE_SIZE is reverse function of USABLE_FRACTION.
 * This can be used to reserve enough size to insert n entries without
 * resizing.
 */
#define ESTIMATE_SIZE(n)  (((n)*3+1) >> 1)

/* Alternative fraction that is otherwise close enough to 2n/3 to make
 * little difference. 8 * 2/3 == 8 * 5/8 == 5. 16 * 2/3 == 16 * 5/8 == 10.
 * 32 * 2/3 = 21, 32 * 5/8 = 20.
 * Its advantage is that it is faster to compute on machines with slow division.
 * #define USABLE_FRACTION(n) (((n) >> 1) + ((n) >> 2) - ((n) >> 3))
 */

/* GROWTH_RATE. Growth rate upon hitting maximum load.
 * Currently set to used*2 + capacity/2.
 * This means that dicts double in size when growing without deletions,
 * but have more head room when the number of deletions is on a par with the
 * number of insertions.
 * Raising this to used*4 doubles memory consumption depending on the size of
 * the dictionary, but results in half the number of resizes, less effort to
 * resize.
 * GROWTH_RATE was set to used*4 up to version 3.2.
 * GROWTH_RATE was set to used*2 in version 3.3.0
 */
#define GROWTH_RATE(d) (((d)->ma_used*2)+((d)->ma_keys->dk_size>>1))

#define ENSURE_ALLOWS_DELETIONS(d) \
    if ((d)->ma_keys->dk_lookup == lookdict_unicode_nodummy) { \
        (d)->ma_keys->dk_lookup = lookdict_unicode; \
    }

/* This immutable, empty PyDictKeysObject is used for PyDict_Clear()
 * (which cannot fail and thus can do no allocation).
 */
static PyDictKeysObject empty_keys_struct = {
        1, /* dk_refcnt */
        1, /* dk_size */
        lookdict_split, /* dk_lookup */
        0, /* dk_usable (immutable) */
        0, /* dk_nentries */
        .dk_indices = { .as_1 = {DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY,
                                 DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY}},
};

static PyObject *empty_values[1] = { NULL };

#define Py_EMPTY_KEYS &empty_keys_struct

/* Uncomment to check the dict content in _PyDict_CheckConsistency() */
/* #define DEBUG_PYDICT */


#ifndef NDEBUG
static int
_PyDict_CheckConsistency(PyDictObject *mp)
{
    PyDictKeysObject *keys = mp->ma_keys;
    int splitted = _PyDict_HasSplitTable(mp);
    Py_ssize_t usable = USABLE_FRACTION(keys->dk_size);
#ifdef DEBUG_PYDICT
    PyDictKeyEntry *entries = DK_ENTRIES(keys);
    Py_ssize_t i;
#endif

    assert(0 <= mp->ma_used && mp->ma_used <= usable);
    assert(IS_POWER_OF_2(keys->dk_size));
    assert(0 <= keys->dk_usable
           && keys->dk_usable <= usable);
    assert(0 <= keys->dk_nentries
           && keys->dk_nentries <= usable);
    assert(keys->dk_usable + keys->dk_nentries <= usable);

    if (!splitted) {
        /* combined table */
        assert(keys->dk_refcnt == 1);
    }

#ifdef DEBUG_PYDICT
    for (i=0; i < keys->dk_size; i++) {
        Py_ssize_t ix = dk_get_index(keys, i);
        assert(DKIX_DUMMY <= ix && ix <= usable);
    }

    for (i=0; i < usable; i++) {
        PyDictKeyEntry *entry = &entries[i];
        PyObject *key = entry->me_key;

        if (key != NULL) {
            if (PyUnicode_CheckExact(key)) {
                Py_hash_t hash = ((PyASCIIObject *)key)->hash;
                assert(hash != -1);
                assert(entry->me_hash == hash);
            }
            else {
                /* test_dict fails if PyObject_Hash() is called again */
                assert(entry->me_hash != -1);
            }
            if (!splitted) {
                assert(entry->me_value != NULL);
            }
        }

        if (splitted) {
            assert(entry->me_value == NULL);
        }
    }

    if (splitted) {
        /* splitted table */
        for (i=0; i < mp->ma_used; i++) {
            assert(mp->ma_values[i] != NULL);
        }
    }
#endif

    return 1;
}
#endif


static PyDictKeysObject *new_keys_object(Py_ssize_t size)
{
    PyDictKeysObject *dk;
    Py_ssize_t es, usable;

    assert(size >= PyDict_MINSIZE);
    assert(IS_POWER_OF_2(size));

    usable = USABLE_FRACTION(size);
    if (size <= 0xff) {
        es = 1;
    }
    else if (size <= 0xffff) {
        es = 2;
    }
#if SIZEOF_VOID_P > 4
    else if (size <= 0xffffffff) {
        es = 4;
    }
#endif
    else {
        es = sizeof(Py_ssize_t);
    }

    if (size == PyDict_MINSIZE && numfreekeys > 0) {
        dk = keys_free_list[--numfreekeys];
    }
    else {
        dk = PyObject_MALLOC(sizeof(PyDictKeysObject)
                             - Py_MEMBER_SIZE(PyDictKeysObject, dk_indices)
                             + es * size
                             + sizeof(PyDictKeyEntry) * usable);
        if (dk == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }
    DK_DEBUG_INCREF dk->dk_refcnt = 1;
    dk->dk_size = size;
    dk->dk_usable = usable;
    dk->dk_lookup = lookdict_unicode_nodummy;
    dk->dk_nentries = 0;
    memset(&dk->dk_indices.as_1[0], 0xff, es * size);
    memset(DK_ENTRIES(dk), 0, sizeof(PyDictKeyEntry) * usable);
    return dk;
}

static void
free_keys_object(PyDictKeysObject *keys)
{
    PyDictKeyEntry *entries = DK_ENTRIES(keys);
    Py_ssize_t i, n;
    for (i = 0, n = keys->dk_nentries; i < n; i++) {
        Py_XDECREF(entries[i].me_key);
        Py_XDECREF(entries[i].me_value);
    }
    if (keys->dk_size == PyDict_MINSIZE && numfreekeys < PyDict_MAXFREELIST) {
        keys_free_list[numfreekeys++] = keys;
        return;
    }
    PyObject_FREE(keys);
}

#define new_values(size) PyMem_NEW(PyObject *, size)
#define free_values(values) PyMem_FREE(values)

/* Consumes a reference to the keys object */
static PyObject *
new_dict(PyDictKeysObject *keys, PyObject **values)
{
    PyDictObject *mp;
    assert(keys != NULL);
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
    mp->ma_version_tag = DICT_NEXT_VERSION();
    assert(_PyDict_CheckConsistency(mp));
    return (PyObject *)mp;
}

/* Consumes a reference to the keys object */
static PyObject *
new_dict_with_shared_keys(PyDictKeysObject *keys)
{
    PyObject **values;
    Py_ssize_t i, size;

    size = USABLE_FRACTION(DK_SIZE(keys));
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
    PyDictKeysObject *keys = new_keys_object(PyDict_MINSIZE);
    if (keys == NULL)
        return NULL;
    return new_dict(keys, NULL);
}

/* Search index of hash table from offset of entry table */
static Py_ssize_t
lookdict_index(PyDictKeysObject *k, Py_hash_t hash, Py_ssize_t index)
{
    size_t i;
    size_t mask = DK_MASK(k);
    Py_ssize_t ix;

    i = (size_t)hash & mask;
    ix = dk_get_index(k, i);
    if (ix == index) {
        return i;
    }
    if (ix == DKIX_EMPTY) {
        return DKIX_EMPTY;
    }

    for (size_t perturb = hash;;) {
        perturb >>= PERTURB_SHIFT;
        i = mask & ((i << 2) + i + perturb + 1);
        ix = dk_get_index(k, i);
        if (ix == index) {
            return i;
        }
        if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
    }
    assert(0);          /* NOT REACHED */
    return DKIX_ERROR;
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

lookdict() is general-purpose, and may return DKIX_ERROR if (and only if) a
comparison raises an exception.
lookdict_unicode() below is specialized to string keys, comparison of which can
never raise an exception; that function can never return DKIX_ERROR when key
is string.  Otherwise, it falls back to lookdict().
lookdict_unicode_nodummy is further specialized for string keys that cannot be
the <dummy> value.
For both, when the key isn't found a DKIX_EMPTY is returned. hashpos returns
where the key index should be inserted.
*/
static Py_ssize_t
lookdict(PyDictObject *mp, PyObject *key,
         Py_hash_t hash, PyObject ***value_addr, Py_ssize_t *hashpos)
{
    size_t i, mask;
    Py_ssize_t ix, freeslot;
    int cmp;
    PyDictKeysObject *dk;
    PyDictKeyEntry *ep0, *ep;
    PyObject *startkey;

top:
    dk = mp->ma_keys;
    mask = DK_MASK(dk);
    ep0 = DK_ENTRIES(dk);
    i = (size_t)hash & mask;

    ix = dk_get_index(dk, i);
    if (ix == DKIX_EMPTY) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = NULL;
        return DKIX_EMPTY;
    }
    if (ix == DKIX_DUMMY) {
        freeslot = i;
    }
    else {
        ep = &ep0[ix];
        assert(ep->me_key != NULL);
        if (ep->me_key == key) {
            *value_addr = &ep->me_value;
            if (hashpos != NULL)
                *hashpos = i;
            return ix;
        }
        if (ep->me_hash == hash) {
            startkey = ep->me_key;
            Py_INCREF(startkey);
            cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0) {
                *value_addr = NULL;
                return DKIX_ERROR;
            }
            if (dk == mp->ma_keys && ep->me_key == startkey) {
                if (cmp > 0) {
                    *value_addr = &ep->me_value;
                    if (hashpos != NULL)
                        *hashpos = i;
                    return ix;
                }
            }
            else {
                /* The dict was mutated, restart */
                goto top;
            }
        }
        freeslot = -1;
    }

    for (size_t perturb = hash;;) {
        perturb >>= PERTURB_SHIFT;
        i = ((i << 2) + i + perturb + 1) & mask;
        ix = dk_get_index(dk, i);
        if (ix == DKIX_EMPTY) {
            if (hashpos != NULL) {
                *hashpos = (freeslot == -1) ? (Py_ssize_t)i : freeslot;
            }
            *value_addr = NULL;
            return ix;
        }
        if (ix == DKIX_DUMMY) {
            if (freeslot == -1)
                freeslot = i;
            continue;
        }
        ep = &ep0[ix];
        assert(ep->me_key != NULL);
        if (ep->me_key == key) {
            if (hashpos != NULL) {
                *hashpos = i;
            }
            *value_addr = &ep->me_value;
            return ix;
        }
        if (ep->me_hash == hash) {
            startkey = ep->me_key;
            Py_INCREF(startkey);
            cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0) {
                *value_addr = NULL;
                return DKIX_ERROR;
            }
            if (dk == mp->ma_keys && ep->me_key == startkey) {
                if (cmp > 0) {
                    if (hashpos != NULL) {
                        *hashpos = i;
                    }
                    *value_addr = &ep->me_value;
                    return ix;
                }
            }
            else {
                /* The dict was mutated, restart */
                goto top;
            }
        }
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/* Specialized version for string-only keys */
static Py_ssize_t
lookdict_unicode(PyDictObject *mp, PyObject *key,
                 Py_hash_t hash, PyObject ***value_addr, Py_ssize_t *hashpos)
{
    size_t i;
    size_t mask = DK_MASK(mp->ma_keys);
    Py_ssize_t ix, freeslot;
    PyDictKeyEntry *ep, *ep0 = DK_ENTRIES(mp->ma_keys);

    assert(mp->ma_values == NULL);
    /* Make sure this function doesn't have to handle non-unicode keys,
       including subclasses of str; e.g., one reason to subclass
       unicodes is to override __eq__, and for speed we don't cater to
       that here. */
    if (!PyUnicode_CheckExact(key)) {
        mp->ma_keys->dk_lookup = lookdict;
        return lookdict(mp, key, hash, value_addr, hashpos);
    }
    i = (size_t)hash & mask;
    ix = dk_get_index(mp->ma_keys, i);
    if (ix == DKIX_EMPTY) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = NULL;
        return DKIX_EMPTY;
    }
    if (ix == DKIX_DUMMY) {
        freeslot = i;
    }
    else {
        ep = &ep0[ix];
        assert(ep->me_key != NULL);
        if (ep->me_key == key
            || (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            if (hashpos != NULL)
                *hashpos = i;
            *value_addr = &ep->me_value;
            return ix;
        }
        freeslot = -1;
    }

    for (size_t perturb = hash;;) {
        perturb >>= PERTURB_SHIFT;
        i = mask & ((i << 2) + i + perturb + 1);
        ix = dk_get_index(mp->ma_keys, i);
        if (ix == DKIX_EMPTY) {
            if (hashpos != NULL) {
                *hashpos = (freeslot == -1) ? (Py_ssize_t)i : freeslot;
            }
            *value_addr = NULL;
            return DKIX_EMPTY;
        }
        if (ix == DKIX_DUMMY) {
            if (freeslot == -1)
                freeslot = i;
            continue;
        }
        ep = &ep0[ix];
        assert(ep->me_key != NULL);
        if (ep->me_key == key
            || (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            *value_addr = &ep->me_value;
            if (hashpos != NULL) {
                *hashpos = i;
            }
            return ix;
        }
    }
    assert(0);          /* NOT REACHED */
    return 0;
}

/* Faster version of lookdict_unicode when it is known that no <dummy> keys
 * will be present. */
static Py_ssize_t
lookdict_unicode_nodummy(PyDictObject *mp, PyObject *key,
                         Py_hash_t hash, PyObject ***value_addr,
                         Py_ssize_t *hashpos)
{
    size_t i;
    size_t mask = DK_MASK(mp->ma_keys);
    Py_ssize_t ix;
    PyDictKeyEntry *ep, *ep0 = DK_ENTRIES(mp->ma_keys);

    assert(mp->ma_values == NULL);
    /* Make sure this function doesn't have to handle non-unicode keys,
       including subclasses of str; e.g., one reason to subclass
       unicodes is to override __eq__, and for speed we don't cater to
       that here. */
    if (!PyUnicode_CheckExact(key)) {
        mp->ma_keys->dk_lookup = lookdict;
        return lookdict(mp, key, hash, value_addr, hashpos);
    }
    i = (size_t)hash & mask;
    ix = dk_get_index(mp->ma_keys, i);
    assert (ix != DKIX_DUMMY);
    if (ix == DKIX_EMPTY) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = NULL;
        return DKIX_EMPTY;
    }
    ep = &ep0[ix];
    assert(ep->me_key != NULL);
    assert(PyUnicode_CheckExact(ep->me_key));
    if (ep->me_key == key ||
        (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = &ep->me_value;
        return ix;
    }
    for (size_t perturb = hash;;) {
        perturb >>= PERTURB_SHIFT;
        i = mask & ((i << 2) + i + perturb + 1);
        ix = dk_get_index(mp->ma_keys, i);
        assert (ix != DKIX_DUMMY);
        if (ix == DKIX_EMPTY) {
            if (hashpos != NULL)
                *hashpos = i;
            *value_addr = NULL;
            return DKIX_EMPTY;
        }
        ep = &ep0[ix];
        assert(ep->me_key != NULL && PyUnicode_CheckExact(ep->me_key));
        if (ep->me_key == key ||
            (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            if (hashpos != NULL)
                *hashpos = i;
            *value_addr = &ep->me_value;
            return ix;
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
static Py_ssize_t
lookdict_split(PyDictObject *mp, PyObject *key,
               Py_hash_t hash, PyObject ***value_addr, Py_ssize_t *hashpos)
{
    size_t i;
    size_t mask = DK_MASK(mp->ma_keys);
    Py_ssize_t ix;
    PyDictKeyEntry *ep, *ep0 = DK_ENTRIES(mp->ma_keys);

    /* mp must split table */
    assert(mp->ma_values != NULL);
    if (!PyUnicode_CheckExact(key)) {
        ix = lookdict(mp, key, hash, value_addr, hashpos);
        if (ix >= 0) {
            *value_addr = &mp->ma_values[ix];
        }
        return ix;
    }

    i = (size_t)hash & mask;
    ix = dk_get_index(mp->ma_keys, i);
    if (ix == DKIX_EMPTY) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = NULL;
        return DKIX_EMPTY;
    }
    assert(ix >= 0);
    ep = &ep0[ix];
    assert(ep->me_key != NULL && PyUnicode_CheckExact(ep->me_key));
    if (ep->me_key == key ||
        (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
        if (hashpos != NULL)
            *hashpos = i;
        *value_addr = &mp->ma_values[ix];
        return ix;
    }
    for (size_t perturb = hash;;) {
        perturb >>= PERTURB_SHIFT;
        i = mask & ((i << 2) + i + perturb + 1);
        ix = dk_get_index(mp->ma_keys, i);
        if (ix == DKIX_EMPTY) {
            if (hashpos != NULL)
                *hashpos = i;
            *value_addr = NULL;
            return DKIX_EMPTY;
        }
        assert(ix >= 0);
        ep = &ep0[ix];
        assert(ep->me_key != NULL && PyUnicode_CheckExact(ep->me_key));
        if (ep->me_key == key ||
            (ep->me_hash == hash && unicode_eq(ep->me_key, key))) {
            if (hashpos != NULL)
                *hashpos = i;
            *value_addr = &mp->ma_values[ix];
            return ix;
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
    Py_ssize_t i, numentries;
    PyDictKeyEntry *ep0;

    if (!PyDict_CheckExact(op) || !_PyObject_GC_IS_TRACKED(op))
        return;

    mp = (PyDictObject *) op;
    ep0 = DK_ENTRIES(mp->ma_keys);
    numentries = mp->ma_keys->dk_nentries;
    if (_PyDict_HasSplitTable(mp)) {
        for (i = 0; i < numentries; i++) {
            if ((value = mp->ma_values[i]) == NULL)
                continue;
            if (_PyObject_GC_MAY_BE_TRACKED(value)) {
                assert(!_PyObject_GC_MAY_BE_TRACKED(ep0[i].me_key));
                return;
            }
        }
    }
    else {
        for (i = 0; i < numentries; i++) {
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
   when it is known that the key is not present in the dict.

   The dict must be combined. */
static void
find_empty_slot(PyDictObject *mp, PyObject *key, Py_hash_t hash,
                PyObject ***value_addr, Py_ssize_t *hashpos)
{
    size_t i;
    size_t mask = DK_MASK(mp->ma_keys);
    Py_ssize_t ix;
    PyDictKeyEntry *ep, *ep0 = DK_ENTRIES(mp->ma_keys);

    assert(!_PyDict_HasSplitTable(mp));
    assert(hashpos != NULL);
    assert(key != NULL);

    if (!PyUnicode_CheckExact(key))
        mp->ma_keys->dk_lookup = lookdict;
    i = hash & mask;
    ix = dk_get_index(mp->ma_keys, i);
    for (size_t perturb = hash; ix != DKIX_EMPTY;) {
        perturb >>= PERTURB_SHIFT;
        i = (i << 2) + i + perturb + 1;
        ix = dk_get_index(mp->ma_keys, i & mask);
    }
    ep = &ep0[mp->ma_keys->dk_nentries];
    *hashpos = i & mask;
    assert(ep->me_value == NULL);
    *value_addr = &ep->me_value;
}

static int
insertion_resize(PyDictObject *mp)
{
    return dictresize(mp, GROWTH_RATE(mp));
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
    PyDictKeyEntry *ep, *ep0;
    Py_ssize_t hashpos, ix;

    Py_INCREF(key);
    Py_INCREF(value);
    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (insertion_resize(mp) < 0)
            goto Fail;
    }

    ix = mp->ma_keys->dk_lookup(mp, key, hash, &value_addr, &hashpos);
    if (ix == DKIX_ERROR)
        goto Fail;

    assert(PyUnicode_CheckExact(key) || mp->ma_keys->dk_lookup == lookdict);
    MAINTAIN_TRACKING(mp, key, value);

    /* When insertion order is different from shared key, we can't share
     * the key anymore.  Convert this instance to combine table.
     */
    if (_PyDict_HasSplitTable(mp) &&
        ((ix >= 0 && *value_addr == NULL && mp->ma_used != ix) ||
         (ix == DKIX_EMPTY && mp->ma_used != mp->ma_keys->dk_nentries))) {
        if (insertion_resize(mp) < 0)
            goto Fail;
        find_empty_slot(mp, key, hash, &value_addr, &hashpos);
        ix = DKIX_EMPTY;
    }

    if (ix == DKIX_EMPTY) {
        /* Insert into new slot. */
        if (mp->ma_keys->dk_usable <= 0) {
            /* Need to resize. */
            if (insertion_resize(mp) < 0)
                goto Fail;
            find_empty_slot(mp, key, hash, &value_addr, &hashpos);
        }
        ep0 = DK_ENTRIES(mp->ma_keys);
        ep = &ep0[mp->ma_keys->dk_nentries];
        dk_set_index(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);
        ep->me_key = key;
        ep->me_hash = hash;
        if (mp->ma_values) {
            assert (mp->ma_values[mp->ma_keys->dk_nentries] == NULL);
            mp->ma_values[mp->ma_keys->dk_nentries] = value;
        }
        else {
            ep->me_value = value;
        }
        mp->ma_used++;
        mp->ma_version_tag = DICT_NEXT_VERSION();
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;
        assert(mp->ma_keys->dk_usable >= 0);
        assert(_PyDict_CheckConsistency(mp));
        return 0;
    }

    assert(value_addr != NULL);

    old_value = *value_addr;
    if (old_value != NULL) {
        *value_addr = value;
        mp->ma_version_tag = DICT_NEXT_VERSION();
        assert(_PyDict_CheckConsistency(mp));

        Py_DECREF(old_value); /* which **CAN** re-enter (see issue #22653) */
        Py_DECREF(key);
        return 0;
    }

    /* pending state */
    assert(_PyDict_HasSplitTable(mp));
    assert(ix == mp->ma_used);
    *value_addr = value;
    mp->ma_used++;
    mp->ma_version_tag = DICT_NEXT_VERSION();
    assert(_PyDict_CheckConsistency(mp));
    Py_DECREF(key);
    return 0;

Fail:
    Py_DECREF(value);
    Py_DECREF(key);
    return -1;
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
    PyDictKeysObject *k = mp->ma_keys;
    size_t mask = (size_t)DK_SIZE(k)-1;
    PyDictKeyEntry *ep0 = DK_ENTRIES(mp->ma_keys);
    PyDictKeyEntry *ep;

    assert(k->dk_lookup != NULL);
    assert(value != NULL);
    assert(key != NULL);
    assert(PyUnicode_CheckExact(key) || k->dk_lookup == lookdict);
    i = hash & mask;
    for (size_t perturb = hash; dk_get_index(k, i) != DKIX_EMPTY;) {
        perturb >>= PERTURB_SHIFT;
        i = mask & ((i << 2) + i + perturb + 1);
    }
    ep = &ep0[k->dk_nentries];
    assert(ep->me_value == NULL);
    dk_set_index(k, i, k->dk_nentries);
    k->dk_nentries++;
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
dictresize(PyDictObject *mp, Py_ssize_t minsize)
{
    Py_ssize_t i, newsize;
    PyDictKeysObject *oldkeys;
    PyObject **oldvalues;
    PyDictKeyEntry *ep0;

    /* Find the smallest table size > minused. */
    for (newsize = PyDict_MINSIZE;
         newsize < minsize && newsize > 0;
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
    // New table must be large enough.
    assert(mp->ma_keys->dk_usable >= mp->ma_used);
    if (oldkeys->dk_lookup == lookdict)
        mp->ma_keys->dk_lookup = lookdict;
    mp->ma_values = NULL;
    ep0 = DK_ENTRIES(oldkeys);
    /* Main loop below assumes we can transfer refcount to new keys
     * and that value is stored in me_value.
     * Increment ref-counts and copy values here to compensate
     * This (resizing a split table) should be relatively rare */
    if (oldvalues != NULL) {
        for (i = 0; i < oldkeys->dk_nentries; i++) {
            if (oldvalues[i] != NULL) {
                Py_INCREF(ep0[i].me_key);
                ep0[i].me_value = oldvalues[i];
            }
        }
    }
    /* Main loop */
    for (i = 0; i < oldkeys->dk_nentries; i++) {
        PyDictKeyEntry *ep = &ep0[i];
        if (ep->me_value != NULL) {
            insertdict_clean(mp, ep->me_key, ep->me_hash, ep->me_value);
        }
    }
    mp->ma_keys->dk_usable -= mp->ma_used;
    if (oldvalues != NULL) {
        /* NULL out me_value slot in oldkeys, in case it was shared */
        for (i = 0; i < oldkeys->dk_nentries; i++)
            ep0[i].me_value = NULL;
        DK_DECREF(oldkeys);
        if (oldvalues != empty_values) {
            free_values(oldvalues);
        }
    }
    else {
        assert(oldkeys->dk_lookup != lookdict_split);
        assert(oldkeys->dk_refcnt == 1);
        DK_DEBUG_DECREF PyObject_FREE(oldkeys);
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
        ep0 = DK_ENTRIES(mp->ma_keys);
        size = USABLE_FRACTION(DK_SIZE(mp->ma_keys));
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
    const Py_ssize_t max_presize = 128 * 1024;
    Py_ssize_t newsize;
    PyDictKeysObject *new_keys;

    /* There are no strict guarantee that returned dict can contain minused
     * items without resize.  So we create medium size dict instead of very
     * large dict or MemoryError.
     */
    if (minused > USABLE_FRACTION(max_presize)) {
        newsize = max_presize;
    }
    else {
        Py_ssize_t minsize = ESTIMATE_SIZE(minused);
        newsize = PyDict_MINSIZE;
        while (newsize < minsize) {
            newsize <<= 1;
        }
    }
    assert(IS_POWER_OF_2(newsize));

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
    Py_ssize_t ix;
    PyDictObject *mp = (PyDictObject *)op;
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
    tstate = _PyThreadState_UncheckedGet();
    if (tstate != NULL && tstate->curexc_type != NULL) {
        /* preserve the existing exception */
        PyObject *err_type, *err_value, *err_tb;
        PyErr_Fetch(&err_type, &err_value, &err_tb);
        ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
        /* ignore errors */
        PyErr_Restore(err_type, err_value, err_tb);
        if (ix < 0)
            return NULL;
    }
    else {
        ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
        if (ix < 0) {
            PyErr_Clear();
            return NULL;
        }
    }
    return *value_addr;
}

/* Same as PyDict_GetItemWithError() but with hash supplied by caller.
   This returns NULL *with* an exception set if an exception occurred.
   It returns NULL *without* an exception set if the key wasn't present.
*/
PyObject *
_PyDict_GetItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix;
    PyDictObject *mp = (PyDictObject *)op;
    PyObject **value_addr;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix < 0) {
        return NULL;
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
    Py_ssize_t ix;
    Py_hash_t hash;
    PyDictObject*mp = (PyDictObject *)op;
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

    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix < 0)
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

/* Fast version of global value lookup (LOAD_GLOBAL).
 * Lookup in globals, then builtins.
 *
 * Raise an exception and return NULL if an error occurred (ex: computing the
 * key hash failed, key comparison failed, ...). Return NULL if the key doesn't
 * exist. Return the value if the key exists.
 */
PyObject *
_PyDict_LoadGlobal(PyDictObject *globals, PyDictObject *builtins, PyObject *key)
{
    Py_ssize_t ix;
    Py_hash_t hash;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1)
    {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }

    /* namespace 1: globals */
    ix = globals->ma_keys->dk_lookup(globals, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix != DKIX_EMPTY && *value_addr != NULL)
        return *value_addr;

    /* namespace 2: builtins */
    ix = builtins->ma_keys->dk_lookup(builtins, key, hash, &value_addr, NULL);
    if (ix < 0)
        return NULL;
    return *value_addr;
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
_PyDict_SetItem_KnownHash(PyObject *op, PyObject *key, PyObject *value,
                         Py_hash_t hash)
{
    PyDictObject *mp;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(value);
    assert(hash != -1);
    mp = (PyDictObject *)op;

    /* insertdict() handles any resizing that might be necessary */
    return insertdict(mp, key, hash, value);
}

static int
delitem_common(PyDictObject *mp, Py_ssize_t hashpos, Py_ssize_t ix,
               PyObject **value_addr)
{
    PyObject *old_key, *old_value;
    PyDictKeyEntry *ep;

    old_value = *value_addr;
    assert(old_value != NULL);
    *value_addr = NULL;
    mp->ma_used--;
    mp->ma_version_tag = DICT_NEXT_VERSION();
    ep = &DK_ENTRIES(mp->ma_keys)[ix];
    dk_set_index(mp->ma_keys, hashpos, DKIX_DUMMY);
    ENSURE_ALLOWS_DELETIONS(mp);
    old_key = ep->me_key;
    ep->me_key = NULL;
    Py_DECREF(old_key);
    Py_DECREF(old_value);

    assert(_PyDict_CheckConsistency(mp));
    return 0;
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    assert(key);
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }

    return _PyDict_DelItem_KnownHash(op, key, hash);
}

int
_PyDict_DelItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t hashpos, ix;
    PyDictObject *mp;
    PyObject **value_addr;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(hash != -1);
    mp = (PyDictObject *)op;
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
    if (ix == DKIX_ERROR)
        return -1;
    if (ix == DKIX_EMPTY || *value_addr == NULL) {
        _PyErr_SetKeyError(key);
        return -1;
    }
    assert(dk_get_index(mp->ma_keys, hashpos) == ix);

    // Split table doesn't allow deletion.  Combine it.
    if (_PyDict_HasSplitTable(mp)) {
        if (dictresize(mp, DK_SIZE(mp->ma_keys))) {
            return -1;
        }
        ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
        assert(ix >= 0);
    }
    return delitem_common(mp, hashpos, ix, value_addr);
}

/* This function promises that the predicate -> deletion sequence is atomic
 * (i.e. protected by the GIL), assuming the predicate itself doesn't
 * release the GIL.
 */
int
_PyDict_DelItemIf(PyObject *op, PyObject *key,
                  int (*predicate)(PyObject *value))
{
    Py_ssize_t hashpos, ix;
    PyDictObject *mp;
    Py_hash_t hash;
    PyObject **value_addr;
    int res;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    hash = PyObject_Hash(key);
    if (hash == -1)
        return -1;
    mp = (PyDictObject *)op;
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
    if (ix == DKIX_ERROR)
        return -1;
    if (ix == DKIX_EMPTY || *value_addr == NULL) {
        _PyErr_SetKeyError(key);
        return -1;
    }
    assert(dk_get_index(mp->ma_keys, hashpos) == ix);

    // Split table doesn't allow deletion.  Combine it.
    if (_PyDict_HasSplitTable(mp)) {
        if (dictresize(mp, DK_SIZE(mp->ma_keys))) {
            return -1;
        }
        ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
        assert(ix >= 0);
    }

    res = predicate(*value_addr);
    if (res == -1)
        return -1;
    if (res > 0)
        return delitem_common(mp, hashpos, ix, value_addr);
    else
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
    mp->ma_version_tag = DICT_NEXT_VERSION();
    /* ...then clear the keys and values */
    if (oldvalues != NULL) {
        n = oldkeys->dk_nentries;
        for (i = 0; i < n; i++)
            Py_CLEAR(oldvalues[i]);
        free_values(oldvalues);
        DK_DECREF(oldkeys);
    }
    else {
       assert(oldkeys->dk_refcnt == 1);
       DK_DECREF(oldkeys);
    }
    assert(_PyDict_CheckConsistency(mp));
}

/* Internal version of PyDict_Next that returns a hash value in addition
 * to the key and value.
 * Return 1 on success, return 0 when the reached the end of the dictionary
 * (or if op is not a dictionary)
 */
int
_PyDict_Next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey,
             PyObject **pvalue, Py_hash_t *phash)
{
    Py_ssize_t i, n;
    PyDictObject *mp;
    PyDictKeyEntry *entry_ptr;
    PyObject *value;

    if (!PyDict_Check(op))
        return 0;
    mp = (PyDictObject *)op;
    i = *ppos;
    n = mp->ma_keys->dk_nentries;
    if ((size_t)i >= (size_t)n)
        return 0;
    if (mp->ma_values) {
        PyObject **value_ptr = &mp->ma_values[i];
        while (i < n && *value_ptr == NULL) {
            value_ptr++;
            i++;
        }
        if (i >= n)
            return 0;
        entry_ptr = &DK_ENTRIES(mp->ma_keys)[i];
        value = *value_ptr;
    }
    else {
        entry_ptr = &DK_ENTRIES(mp->ma_keys)[i];
        while (i < n && entry_ptr->me_value == NULL) {
            entry_ptr++;
            i++;
        }
        if (i >= n)
            return 0;
        value = entry_ptr->me_value;
    }
    *ppos = i+1;
    if (pkey)
        *pkey = entry_ptr->me_key;
    if (phash)
        *phash = entry_ptr->me_hash;
    if (pvalue)
        *pvalue = value;
    return 1;
}

/*
 * Iterate over a dict.  Use like so:
 *
 *     Py_ssize_t i;
 *     PyObject *key, *value;
 *     i = 0;   # important!  i should not otherwise be changed by you
 *     while (PyDict_Next(yourdict, &i, &key, &value)) {
 *         Refer to borrowed references in key and value.
 *     }
 *
 * Return 1 on success, return 0 when the reached the end of the dictionary
 * (or if op is not a dictionary)
 *
 * CAUTION:  In general, it isn't safe to use PyDict_Next in a loop that
 * mutates the dict.  One exception:  it is safe if the loop merely changes
 * the values associated with the keys (but doesn't insert new keys or
 * delete keys), via PyDict_SetItem().
 */
int
PyDict_Next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey, PyObject **pvalue)
{
    return _PyDict_Next(op, ppos, pkey, pvalue, NULL);
}

/* Internal version of dict.pop(). */
PyObject *
_PyDict_Pop_KnownHash(PyObject *dict, PyObject *key, Py_hash_t hash, PyObject *deflt)
{
    Py_ssize_t ix, hashpos;
    PyObject *old_value, *old_key;
    PyDictKeyEntry *ep;
    PyObject **value_addr;
    PyDictObject *mp;

    assert(PyDict_Check(dict));
    mp = (PyDictObject *)dict;

    if (mp->ma_used == 0) {
        if (deflt) {
            Py_INCREF(deflt);
            return deflt;
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || *value_addr == NULL) {
        if (deflt) {
            Py_INCREF(deflt);
            return deflt;
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }

    // Split table doesn't allow deletion.  Combine it.
    if (_PyDict_HasSplitTable(mp)) {
        if (dictresize(mp, DK_SIZE(mp->ma_keys))) {
            return NULL;
        }
        ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
        assert(ix >= 0);
    }

    old_value = *value_addr;
    assert(old_value != NULL);
    *value_addr = NULL;
    mp->ma_used--;
    mp->ma_version_tag = DICT_NEXT_VERSION();
    dk_set_index(mp->ma_keys, hashpos, DKIX_DUMMY);
    ep = &DK_ENTRIES(mp->ma_keys)[ix];
    ENSURE_ALLOWS_DELETIONS(mp);
    old_key = ep->me_key;
    ep->me_key = NULL;
    Py_DECREF(old_key);

    assert(_PyDict_CheckConsistency(mp));
    return old_value;
}

PyObject *
_PyDict_Pop(PyObject *dict, PyObject *key, PyObject *deflt)
{
    Py_hash_t hash;

    if (((PyDictObject *)dict)->ma_used == 0) {
        if (deflt) {
            Py_INCREF(deflt);
            return deflt;
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    return _PyDict_Pop_KnownHash(dict, key, hash, deflt);
}

/* Internal version of dict.from_keys().  It is subclass-friendly. */
PyObject *
_PyDict_FromKeys(PyObject *cls, PyObject *iterable, PyObject *value)
{
    PyObject *it;       /* iter(iterable) */
    PyObject *key;
    PyObject *d;
    int status;

    d = PyObject_CallObject(cls, NULL);
    if (d == NULL)
        return NULL;

    if (PyDict_CheckExact(d) && ((PyDictObject *)d)->ma_used == 0) {
        if (PyDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;
            PyObject *oldvalue;
            Py_ssize_t pos = 0;
            PyObject *key;
            Py_hash_t hash;

            if (dictresize(mp, ESTIMATE_SIZE(((PyDictObject *)iterable)->ma_used))) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PyDict_Next(iterable, &pos, &key, &oldvalue, &hash)) {
                if (insertdict(mp, key, hash, value)) {
                    Py_DECREF(d);
                    return NULL;
                }
            }
            return d;
        }
        if (PyAnySet_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;
            Py_ssize_t pos = 0;
            PyObject *key;
            Py_hash_t hash;

            if (dictresize(mp, ESTIMATE_SIZE(PySet_GET_SIZE(iterable)))) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PySet_NextEntry(iterable, &pos, &key, &hash)) {
                if (insertdict(mp, key, hash, value)) {
                    Py_DECREF(d);
                    return NULL;
                }
            }
            return d;
        }
    }

    it = PyObject_GetIter(iterable);
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

/* Methods */

static void
dict_dealloc(PyDictObject *mp)
{
    PyObject **values = mp->ma_values;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n;

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(mp);
    Py_TRASHCAN_SAFE_BEGIN(mp)
    if (values != NULL) {
        if (values != empty_values) {
            for (i = 0, n = mp->ma_keys->dk_nentries; i < n; i++) {
                Py_XDECREF(values[i]);
            }
            free_values(values);
        }
        DK_DECREF(keys);
    }
    else if (keys != NULL) {
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
    PyObject *key = NULL, *value = NULL;
    _PyUnicodeWriter writer;
    int first;

    i = Py_ReprEnter((PyObject *)mp);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromString("{...}") : NULL;
    }

    if (mp->ma_used == 0) {
        Py_ReprLeave((PyObject *)mp);
        return PyUnicode_FromString("{}");
    }

    _PyUnicodeWriter_Init(&writer);
    writer.overallocate = 1;
    /* "{" + "1: 2" + ", 3: 4" * (len - 1) + "}" */
    writer.min_length = 1 + 4 + (2 + 4) * (mp->ma_used - 1) + 1;

    if (_PyUnicodeWriter_WriteChar(&writer, '{') < 0)
        goto error;

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    i = 0;
    first = 1;
    while (PyDict_Next((PyObject *)mp, &i, &key, &value)) {
        PyObject *s;
        int res;

        /* Prevent repr from deleting key or value during key format. */
        Py_INCREF(key);
        Py_INCREF(value);

        if (!first) {
            if (_PyUnicodeWriter_WriteASCIIString(&writer, ", ", 2) < 0)
                goto error;
        }
        first = 0;

        s = PyObject_Repr(key);
        if (s == NULL)
            goto error;
        res = _PyUnicodeWriter_WriteStr(&writer, s);
        Py_DECREF(s);
        if (res < 0)
            goto error;

        if (_PyUnicodeWriter_WriteASCIIString(&writer, ": ", 2) < 0)
            goto error;

        s = PyObject_Repr(value);
        if (s == NULL)
            goto error;
        res = _PyUnicodeWriter_WriteStr(&writer, s);
        Py_DECREF(s);
        if (res < 0)
            goto error;

        Py_CLEAR(key);
        Py_CLEAR(value);
    }

    writer.overallocate = 0;
    if (_PyUnicodeWriter_WriteChar(&writer, '}') < 0)
        goto error;

    Py_ReprLeave((PyObject *)mp);

    return _PyUnicodeWriter_Finish(&writer);

error:
    Py_ReprLeave((PyObject *)mp);
    _PyUnicodeWriter_Dealloc(&writer);
    Py_XDECREF(key);
    Py_XDECREF(value);
    return NULL;
}

static Py_ssize_t
dict_length(PyDictObject *mp)
{
    return mp->ma_used;
}

static PyObject *
dict_subscript(PyDictObject *mp, PyObject *key)
{
    PyObject *v;
    Py_ssize_t ix;
    Py_hash_t hash;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || *value_addr == NULL) {
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
        _PyErr_SetKeyError(key);
        return NULL;
    }
    v = *value_addr;
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
dict_keys(PyDictObject *mp)
{
    PyObject *v;
    Py_ssize_t i, j;
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
    ep = DK_ENTRIES(mp->ma_keys);
    size = mp->ma_keys->dk_nentries;
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
dict_values(PyDictObject *mp)
{
    PyObject *v;
    Py_ssize_t i, j;
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
    ep = DK_ENTRIES(mp->ma_keys);
    size = mp->ma_keys->dk_nentries;
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
            Py_INCREF(value);
            PyList_SET_ITEM(v, j, value);
            j++;
        }
    }
    assert(j == n);
    return v;
}

static PyObject *
dict_items(PyDictObject *mp)
{
    PyObject *v;
    Py_ssize_t i, j, n;
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
    ep = DK_ENTRIES(mp->ma_keys);
    size = mp->ma_keys->dk_nentries;
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

/*[clinic input]
@classmethod
dict.fromkeys
    iterable: object
    value: object=None
    /

Returns a new dict with keys from iterable and values equal to value.
[clinic start generated code]*/

static PyObject *
dict_fromkeys_impl(PyTypeObject *type, PyObject *iterable, PyObject *value)
/*[clinic end generated code: output=8fb98e4b10384999 input=b85a667f9bf4669d]*/
{
    return _PyDict_FromKeys((PyObject *)type, iterable, value);
}

static int
dict_update_common(PyObject *self, PyObject *args, PyObject *kwds,
                   const char *methname)
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
        Py_INCREF(key);
        Py_INCREF(value);
        if (override || PyDict_GetItem(d, key) == NULL) {
            int status = PyDict_SetItem(d, key, value);
            if (status < 0) {
                Py_DECREF(key);
                Py_DECREF(value);
                goto Fail;
            }
        }
        Py_DECREF(key);
        Py_DECREF(value);
        Py_DECREF(fast);
        Py_DECREF(item);
    }

    i = 0;
    assert(_PyDict_CheckConsistency((PyDictObject *)d));
    goto Return;
Fail:
    Py_XDECREF(item);
    Py_XDECREF(fast);
    i = -1;
Return:
    Py_DECREF(it);
    return Py_SAFE_DOWNCAST(i, Py_ssize_t, int);
}

static int
dict_merge(PyObject *a, PyObject *b, int override)
{
    PyDictObject *mp, *other;
    Py_ssize_t i, n;
    PyDictKeyEntry *entry, *ep0;

    assert(0 <= override && override <= 2);

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
        if (USABLE_FRACTION(mp->ma_keys->dk_size) < other->ma_used) {
            if (dictresize(mp, ESTIMATE_SIZE(mp->ma_used + other->ma_used))) {
               return -1;
            }
        }
        ep0 = DK_ENTRIES(other->ma_keys);
        for (i = 0, n = other->ma_keys->dk_nentries; i < n; i++) {
            PyObject *key, *value;
            Py_hash_t hash;
            entry = &ep0[i];
            key = entry->me_key;
            hash = entry->me_hash;
            if (other->ma_values)
                value = other->ma_values[i];
            else
                value = entry->me_value;

            if (value != NULL) {
                int err = 0;
                Py_INCREF(key);
                Py_INCREF(value);
                if (override == 1)
                    err = insertdict(mp, key, hash, value);
                else if (_PyDict_GetItem_KnownHash(a, key, hash) == NULL) {
                    if (PyErr_Occurred()) {
                        Py_DECREF(value);
                        Py_DECREF(key);
                        return -1;
                    }
                    err = insertdict(mp, key, hash, value);
                }
                else if (override != 0) {
                    _PyErr_SetKeyError(key);
                    Py_DECREF(value);
                    Py_DECREF(key);
                    return -1;
                }
                Py_DECREF(value);
                Py_DECREF(key);
                if (err != 0)
                    return -1;

                if (n != other->ma_keys->dk_nentries) {
                    PyErr_SetString(PyExc_RuntimeError,
                                    "dict mutated during update");
                    return -1;
                }
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
            if (override != 1 && PyDict_GetItem(a, key) != NULL) {
                if (override != 0) {
                    _PyErr_SetKeyError(key);
                    Py_DECREF(key);
                    Py_DECREF(iter);
                    return -1;
                }
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
    assert(_PyDict_CheckConsistency((PyDictObject *)a));
    return 0;
}

int
PyDict_Update(PyObject *a, PyObject *b)
{
    return dict_merge(a, b, 1);
}

int
PyDict_Merge(PyObject *a, PyObject *b, int override)
{
    /* XXX Deprecate override not in (0, 1). */
    return dict_merge(a, b, override != 0);
}

int
_PyDict_MergeEx(PyObject *a, PyObject *b, int override)
{
    return dict_merge(a, b, override);
}

static PyObject *
dict_copy(PyDictObject *mp)
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
        Py_ssize_t size = USABLE_FRACTION(DK_SIZE(mp->ma_keys));
        PyObject **newvalues;
        newvalues = new_values(size);
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
        for (i = 0, n = size; i < n; i++) {
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
    for (i = 0; i < a->ma_keys->dk_nentries; i++) {
        PyDictKeyEntry *ep = &DK_ENTRIES(a->ma_keys)[i];
        PyObject *aval;
        if (a->ma_values)
            aval = a->ma_values[i];
        else
            aval = ep->me_value;
        if (aval != NULL) {
            int cmp;
            PyObject *bval;
            PyObject **vaddr;
            PyObject *key = ep->me_key;
            /* temporarily bump aval's refcount to ensure it stays
               alive until we're done with it */
            Py_INCREF(aval);
            /* ditto for key */
            Py_INCREF(key);
            /* reuse the known hash value */
            if ((b->ma_keys->dk_lookup)(b, key, ep->me_hash, &vaddr, NULL) < 0)
                bval = NULL;
            else
                bval = *vaddr;
            if (bval == NULL) {
                Py_DECREF(key);
                Py_DECREF(aval);
                if (PyErr_Occurred())
                    return -1;
                return 0;
            }
            cmp = PyObject_RichCompareBool(aval, bval, Py_EQ);
            Py_DECREF(key);
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

/*[clinic input]

@coexist
dict.__contains__

  key: object
  /

True if D has a key k, else False.
[clinic start generated code]*/

static PyObject *
dict___contains__(PyDictObject *self, PyObject *key)
/*[clinic end generated code: output=a3d03db709ed6e6b input=b852b2a19b51ab24]*/
{
    register PyDictObject *mp = self;
    Py_hash_t hash;
    Py_ssize_t ix;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || *value_addr == NULL)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

static PyObject *
dict_get(PyDictObject *mp, PyObject *args)
{
    PyObject *key;
    PyObject *failobj = Py_None;
    PyObject *val = NULL;
    Py_hash_t hash;
    Py_ssize_t ix;
    PyObject **value_addr;

    if (!PyArg_UnpackTuple(args, "get", 1, 2, &key, &failobj))
        return NULL;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || *value_addr == NULL)
        val = failobj;
    else
        val = *value_addr;
    Py_INCREF(val);
    return val;
}

PyObject *
PyDict_SetDefault(PyObject *d, PyObject *key, PyObject *defaultobj)
{
    PyDictObject *mp = (PyDictObject *)d;
    PyObject *value;
    Py_hash_t hash;
    Py_ssize_t hashpos, ix;
    PyObject **value_addr;

    if (!PyDict_Check(d)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }

    if (mp->ma_values != NULL && !PyUnicode_CheckExact(key)) {
        if (insertion_resize(mp) < 0)
            return NULL;
    }

    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, &hashpos);
    if (ix == DKIX_ERROR)
        return NULL;

    if (_PyDict_HasSplitTable(mp) &&
        ((ix >= 0 && *value_addr == NULL && mp->ma_used != ix) ||
         (ix == DKIX_EMPTY && mp->ma_used != mp->ma_keys->dk_nentries))) {
        if (insertion_resize(mp) < 0) {
            return NULL;
        }
        find_empty_slot(mp, key, hash, &value_addr, &hashpos);
        ix = DKIX_EMPTY;
    }

    if (ix == DKIX_EMPTY) {
        PyDictKeyEntry *ep, *ep0;
        value = defaultobj;
        if (mp->ma_keys->dk_usable <= 0) {
            if (insertion_resize(mp) < 0) {
                return NULL;
            }
            find_empty_slot(mp, key, hash, &value_addr, &hashpos);
        }
        ep0 = DK_ENTRIES(mp->ma_keys);
        ep = &ep0[mp->ma_keys->dk_nentries];
        dk_set_index(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);
        Py_INCREF(key);
        Py_INCREF(value);
        MAINTAIN_TRACKING(mp, key, value);
        ep->me_key = key;
        ep->me_hash = hash;
        if (mp->ma_values) {
            assert(mp->ma_values[mp->ma_keys->dk_nentries] == NULL);
            mp->ma_values[mp->ma_keys->dk_nentries] = value;
        }
        else {
            ep->me_value = value;
        }
        mp->ma_used++;
        mp->ma_version_tag = DICT_NEXT_VERSION();
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;
        assert(mp->ma_keys->dk_usable >= 0);
    }
    else if (*value_addr == NULL) {
        value = defaultobj;
        assert(_PyDict_HasSplitTable(mp));
        assert(ix == mp->ma_used);
        Py_INCREF(value);
        MAINTAIN_TRACKING(mp, key, value);
        *value_addr = value;
        mp->ma_used++;
        mp->ma_version_tag = DICT_NEXT_VERSION();
    }
    else {
        value = *value_addr;
    }

    assert(_PyDict_CheckConsistency(mp));
    return value;
}

static PyObject *
dict_setdefault(PyDictObject *mp, PyObject *args)
{
    PyObject *key, *val;
    PyObject *defaultobj = Py_None;

    if (!PyArg_UnpackTuple(args, "setdefault", 1, 2, &key, &defaultobj))
        return NULL;

    val = PyDict_SetDefault((PyObject *)mp, key, defaultobj);
    Py_XINCREF(val);
    return val;
}

static PyObject *
dict_clear(PyDictObject *mp)
{
    PyDict_Clear((PyObject *)mp);
    Py_RETURN_NONE;
}

static PyObject *
dict_pop(PyDictObject *mp, PyObject *args)
{
    PyObject *key, *deflt = NULL;

    if(!PyArg_UnpackTuple(args, "pop", 1, 2, &key, &deflt))
        return NULL;

    return _PyDict_Pop((PyObject*)mp, key, deflt);
}

static PyObject *
dict_popitem(PyDictObject *mp)
{
    Py_ssize_t i, j;
    PyDictKeyEntry *ep0, *ep;
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

    /* Pop last item */
    ep0 = DK_ENTRIES(mp->ma_keys);
    i = mp->ma_keys->dk_nentries - 1;
    while (i >= 0 && ep0[i].me_value == NULL) {
        i--;
    }
    assert(i >= 0);

    ep = &ep0[i];
    j = lookdict_index(mp->ma_keys, ep->me_hash, i);
    assert(j >= 0);
    assert(dk_get_index(mp->ma_keys, j) == i);
    dk_set_index(mp->ma_keys, j, DKIX_DUMMY);

    PyTuple_SET_ITEM(res, 0, ep->me_key);
    PyTuple_SET_ITEM(res, 1, ep->me_value);
    ep->me_key = NULL;
    ep->me_value = NULL;
    /* We can't dk_usable++ since there is DKIX_DUMMY in indices */
    mp->ma_keys->dk_nentries = i;
    mp->ma_used--;
    mp->ma_version_tag = DICT_NEXT_VERSION();
    assert(_PyDict_CheckConsistency(mp));
    return res;
}

static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeysObject *keys = mp->ma_keys;
    PyDictKeyEntry *entries = DK_ENTRIES(keys);
    Py_ssize_t i, n = keys->dk_nentries;

    if (keys->dk_lookup == lookdict) {
        for (i = 0; i < n; i++) {
            if (entries[i].me_value != NULL) {
                Py_VISIT(entries[i].me_value);
                Py_VISIT(entries[i].me_key);
            }
        }
    }
    else {
        if (mp->ma_values != NULL) {
            for (i = 0; i < n; i++) {
                Py_VISIT(mp->ma_values[i]);
            }
        }
        else {
            for (i = 0; i < n; i++) {
                Py_VISIT(entries[i].me_value);
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

Py_ssize_t
_PyDict_SizeOf(PyDictObject *mp)
{
    Py_ssize_t size, usable, res;

    size = DK_SIZE(mp->ma_keys);
    usable = USABLE_FRACTION(size);

    res = _PyObject_SIZE(Py_TYPE(mp));
    if (mp->ma_values)
        res += usable * sizeof(PyObject*);
    /* If the dictionary is split, the keys portion is accounted-for
       in the type object. */
    if (mp->ma_keys->dk_refcnt == 1)
        res += (sizeof(PyDictKeysObject)
                - Py_MEMBER_SIZE(PyDictKeysObject, dk_indices)
                + DK_IXSIZE(mp->ma_keys) * size
                + sizeof(PyDictKeyEntry) * usable);
    return res;
}

Py_ssize_t
_PyDict_KeysSize(PyDictKeysObject *keys)
{
    return (sizeof(PyDictKeysObject)
            - Py_MEMBER_SIZE(PyDictKeysObject, dk_indices)
            + DK_IXSIZE(keys) * DK_SIZE(keys)
            + USABLE_FRACTION(DK_SIZE(keys)) * sizeof(PyDictKeyEntry));
}

static PyObject *
dict_sizeof(PyDictObject *mp)
{
    return PyLong_FromSsize_t(_PyDict_SizeOf(mp));
}

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
"D.update([E, ]**F) -> None.  Update D from dict/iterable E and F.\n\
If E is present and has a .keys() method, then does:  for k in E: D[k] = E[k]\n\
If E is present and lacks a .keys() method, then does:  for k, v in E: D[k] = v\n\
In either case, this is followed by: for k in F:  D[k] = F[k]");

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
    DICT___CONTAINS___METHODDEF
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
    DICT_FROMKEYS_METHODDEF
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
    Py_ssize_t ix;
    PyDictObject *mp = (PyDictObject *)op;
    PyObject **value_addr;

    if (!PyUnicode_CheckExact(key) ||
        (hash = ((PyASCIIObject *) key)->hash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return -1;
    return (ix != DKIX_EMPTY && *value_addr != NULL);
}

/* Internal version of PyDict_Contains used when the hash value is already known */
int
_PyDict_Contains(PyObject *op, PyObject *key, Py_hash_t hash)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyObject **value_addr;
    Py_ssize_t ix;

    ix = (mp->ma_keys->dk_lookup)(mp, key, hash, &value_addr, NULL);
    if (ix == DKIX_ERROR)
        return -1;
    return (ix != DKIX_EMPTY && *value_addr != NULL);
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
    PyDictObject *d;

    assert(type != NULL && type->tp_alloc != NULL);
    self = type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;
    d = (PyDictObject *)self;

    /* The object has been implicitly tracked by tp_alloc */
    if (type == &PyDict_Type)
        _PyObject_GC_UNTRACK(d);

    d->ma_used = 0;
    d->ma_version_tag = DICT_NEXT_VERSION();
    d->ma_keys = new_keys_object(PyDict_MINSIZE);
    if (d->ma_keys == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    assert(_PyDict_CheckConsistency(d));
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
    if (kv == NULL) {
        PyErr_Clear();
        return NULL;
    }
    return PyDict_GetItem(dp, kv);
}

/* For backward compatibility with old dictionary interface */

PyObject *
PyDict_GetItemString(PyObject *v, const char *key)
{
    PyObject *kv, *rv;
    kv = PyUnicode_FromString(key);
    if (kv == NULL) {
        PyErr_Clear();
        return NULL;
    }
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
_PyDict_DelItemId(PyObject *v, _Py_Identifier *key)
{
    PyObject *kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL)
        return -1;
    return PyDict_DelItem(v, kv);
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
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(di);
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

static PyObject*
dictiter_iternextkey(dictiterobject *di)
{
    PyObject *key;
    Py_ssize_t i, n;
    PyDictKeysObject *k;
    PyDictObject *d = di->di_dict;

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
    k = d->ma_keys;
    n = k->dk_nentries;
    if (d->ma_values) {
        PyObject **value_ptr = &d->ma_values[i];
        while (i < n && *value_ptr == NULL) {
            value_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        key = DK_ENTRIES(k)[i].me_key;
    }
    else {
        PyDictKeyEntry *entry_ptr = &DK_ENTRIES(k)[i];
        while (i < n && entry_ptr->me_value == NULL) {
            entry_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        key = entry_ptr->me_key;
    }
    di->di_pos = i+1;
    di->len--;
    Py_INCREF(key);
    return key;

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
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

static PyObject *
dictiter_iternextvalue(dictiterobject *di)
{
    PyObject *value;
    Py_ssize_t i, n;
    PyDictObject *d = di->di_dict;

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
    n = d->ma_keys->dk_nentries;
    if (d->ma_values) {
        PyObject **value_ptr = &d->ma_values[i];
        while (i < n && *value_ptr == NULL) {
            value_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        value = *value_ptr;
    }
    else {
        PyDictKeyEntry *entry_ptr = &DK_ENTRIES(d->ma_keys)[i];
        while (i < n && entry_ptr->me_value == NULL) {
            entry_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        value = entry_ptr->me_value;
    }
    di->di_pos = i+1;
    di->len--;
    Py_INCREF(value);
    return value;

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
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

static PyObject *
dictiter_iternextitem(dictiterobject *di)
{
    PyObject *key, *value, *result;
    Py_ssize_t i, n;
    PyDictObject *d = di->di_dict;

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
    n = d->ma_keys->dk_nentries;
    if (d->ma_values) {
        PyObject **value_ptr = &d->ma_values[i];
        while (i < n && *value_ptr == NULL) {
            value_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        key = DK_ENTRIES(d->ma_keys)[i].me_key;
        value = *value_ptr;
    }
    else {
        PyDictKeyEntry *entry_ptr = &DK_ENTRIES(d->ma_keys)[i];
        while (i < n && entry_ptr->me_value == NULL) {
            entry_ptr++;
            i++;
        }
        if (i >= n)
            goto fail;
        key = entry_ptr->me_key;
        value = entry_ptr->me_value;
    }
    di->di_pos = i+1;
    di->len--;
    Py_INCREF(key);
    Py_INCREF(value);
    result = di->di_result;
    if (Py_REFCNT(result) == 1) {
        PyObject *oldkey = PyTuple_GET_ITEM(result, 0);
        PyObject *oldvalue = PyTuple_GET_ITEM(result, 1);
        PyTuple_SET_ITEM(result, 0, key);  /* steals reference */
        PyTuple_SET_ITEM(result, 1, value);  /* steals reference */
        Py_INCREF(result);
        Py_DECREF(oldkey);
        Py_DECREF(oldvalue);
    }
    else {
        result = PyTuple_New(2);
        if (result == NULL)
            return NULL;
        PyTuple_SET_ITEM(result, 0, key);  /* steals reference */
        PyTuple_SET_ITEM(result, 1, value);  /* steals reference */
    }
    return result;

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
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

static void
dictview_dealloc(_PyDictViewObject *dv)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(dv);
    Py_XDECREF(dv->dv_dict);
    PyObject_GC_Del(dv);
}

static int
dictview_traverse(_PyDictViewObject *dv, visitproc visit, void *arg)
{
    Py_VISIT(dv->dv_dict);
    return 0;
}

static Py_ssize_t
dictview_len(_PyDictViewObject *dv)
{
    Py_ssize_t len = 0;
    if (dv->dv_dict != NULL)
        len = dv->dv_dict->ma_used;
    return len;
}

PyObject *
_PyDictView_New(PyObject *dict, PyTypeObject *type)
{
    _PyDictViewObject *dv;
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
    dv = PyObject_GC_New(_PyDictViewObject, type);
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
dictview_repr(_PyDictViewObject *dv)
{
    PyObject *seq;
    PyObject *result = NULL;
    Py_ssize_t rc;

    rc = Py_ReprEnter((PyObject *)dv);
    if (rc != 0) {
        return rc > 0 ? PyUnicode_FromString("...") : NULL;
    }
    seq = PySequence_List((PyObject *)dv);
    if (seq == NULL) {
        goto Done;
    }
    result = PyUnicode_FromFormat("%s(%R)", Py_TYPE(dv)->tp_name, seq);
    Py_DECREF(seq);

Done:
    Py_ReprLeave((PyObject *)dv);
    return result;
}

/*** dict_keys ***/

static PyObject *
dictkeys_iter(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterKey_Type);
}

static int
dictkeys_contains(_PyDictViewObject *dv, PyObject *obj)
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

    tmp = _PyObject_CallMethodIdObjArgs(result, &PyId_difference_update, other, NULL);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

PyObject*
_PyDictView_Intersect(PyObject* self, PyObject *other)
{
    PyObject *result = PySet_New(self);
    PyObject *tmp;
    _Py_IDENTIFIER(intersection_update);

    if (result == NULL)
        return NULL;

    tmp = _PyObject_CallMethodIdObjArgs(result, &PyId_intersection_update, other, NULL);
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

    tmp = _PyObject_CallMethodIdObjArgs(result, &PyId_update, other, NULL);
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

    tmp = _PyObject_CallMethodIdObjArgs(result, &PyId_symmetric_difference_update, other, NULL);
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
    (binaryfunc)_PyDictView_Intersect,  /*nb_and*/
    (binaryfunc)dictviews_xor,          /*nb_xor*/
    (binaryfunc)dictviews_or,           /*nb_or*/
};

static PyObject*
dictviews_isdisjoint(PyObject *self, PyObject *other)
{
    PyObject *it;
    PyObject *item = NULL;

    if (self == other) {
        if (dictview_len((_PyDictViewObject *)self) == 0)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    /* Iterate over the shorter object (only if other is a set,
     * because PySequence_Contains may be expensive otherwise): */
    if (PyAnySet_Check(other) || PyDictViewSet_Check(other)) {
        Py_ssize_t len_self = dictview_len((_PyDictViewObject *)self);
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
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
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
    return _PyDictView_New(dict, &PyDictKeys_Type);
}

/*** dict_items ***/

static PyObject *
dictitems_iter(_PyDictViewObject *dv)
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterItem_Type);
}

static int
dictitems_contains(_PyDictViewObject *dv, PyObject *obj)
{
    int result;
    PyObject *key, *value, *found;
    if (dv->dv_dict == NULL)
        return 0;
    if (!PyTuple_Check(obj) || PyTuple_GET_SIZE(obj) != 2)
        return 0;
    key = PyTuple_GET_ITEM(obj, 0);
    value = PyTuple_GET_ITEM(obj, 1);
    found = PyDict_GetItemWithError((PyObject *)dv->dv_dict, key);
    if (found == NULL) {
        if (PyErr_Occurred())
            return -1;
        return 0;
    }
    Py_INCREF(found);
    result = PyObject_RichCompareBool(value, found, Py_EQ);
    Py_DECREF(found);
    return result;
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
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
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
    return _PyDictView_New(dict, &PyDictItems_Type);
}

/*** dict_values ***/

static PyObject *
dictvalues_iter(_PyDictViewObject *dv)
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
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
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
    return _PyDictView_New(dict, &PyDictValues_Type);
}

/* Returns NULL if cannot allocate a new PyDictKeysObject,
   but does not set an error */
PyDictKeysObject *
_PyDict_NewKeysForClass(void)
{
    PyDictKeysObject *keys = new_keys_object(PyDict_MINSIZE);
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
            // Since key sharing dict doesn't allow deletion, PyDict_DelItem()
            // always converts dict to combined form.
            if ((cached = CACHED_KEYS(tp)) != NULL) {
                CACHED_KEYS(tp) = NULL;
                DK_DECREF(cached);
            }
        }
        else {
            int was_shared = (cached == ((PyDictObject *)dict)->ma_keys);
            res = PyDict_SetItem(dict, key, value);
            if (was_shared &&
                    (cached = CACHED_KEYS(tp)) != NULL &&
                    cached != ((PyDictObject *)dict)->ma_keys) {
                /* PyDict_SetItem() may call dictresize and convert split table
                 * into combined table.  In such case, convert it to split
                 * table again and update type's shared key only when this is
                 * the only dict sharing key with the type.
                 *
                 * This is to allow using shared key in class like this:
                 *
                 *     class C:
                 *         def __init__(self):
                 *             # one dict resize happens
                 *             self.a, self.b, self.c = 1, 2, 3
                 *             self.d, self.e, self.f = 4, 5, 6
                 *     a = C()
                 */
                if (cached->dk_refcnt == 1) {
                    CACHED_KEYS(tp) = make_keys_shared(dict);
                }
                else {
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
