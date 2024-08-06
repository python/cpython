/* Dictionary object implementation using a hash table */

/* The distribution includes a separate file, Objects/dictnotes.txt,
   describing explorations into dictionary design and optimization.
   It covers typical dictionary use patterns, the parameters for
   tuning dictionaries, and several ideas for possible optimizations.
*/

/* PyDictKeysObject

This implements the dictionary's hashtable.

As of Python 3.6, this is compact and ordered. Basic idea is described here:
* https://mail.python.org/pipermail/python-dev/2012-December/123028.html
* https://morepypy.blogspot.com/2015/01/faster-more-memory-efficient-and-more.html

layout:

+---------------------+
| dk_refcnt           |
| dk_log2_size        |
| dk_log2_index_bytes |
| dk_kind             |
| dk_usable           |
| dk_nentries         |
+---------------------+
| dk_indices[]        |
|                     |
+---------------------+
| dk_entries[]        |
|                     |
+---------------------+

dk_indices is actual hashtable.  It holds index in entries, or DKIX_EMPTY(-1)
or DKIX_DUMMY(-2).
Size of indices is dk_size.  Type of each index in indices is vary on dk_size:

* int8  for          dk_size <= 128
* int16 for 256   <= dk_size <= 2**15
* int32 for 2**16 <= dk_size <= 2**31
* int64 for 2**32 <= dk_size

dk_entries is array of PyDictKeyEntry when dk_kind == DICT_KEYS_GENERAL or
PyDictUnicodeEntry otherwise. Its length is USABLE_FRACTION(dk_size).

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

To preserve the order in a split table, a bit vector is used  to record the
insertion order. When a key is inserted the bit vector is shifted up by 4 bits
and the index of the key is stored in the low 4 bits.
As a consequence of this, split keys have a maximum size of 16.
*/

/* PyDict_MINSIZE is the starting size for any new dict.
 * 8 allows dicts with no more than 5 active entries; experiments suggested
 * this suffices for the majority of dicts (consisting mostly of usually-small
 * dicts created to pass keyword arguments).
 * Making this 8, rather than 4 reduces the number of resizes for most
 * dictionaries, without any significant extra memory use.
 */
#define PyDict_LOG_MINSIZE 3
#define PyDict_MINSIZE 8

#include "Python.h"
#include "pycore_bitutils.h"      // _Py_bit_length
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_code.h"          // stats
#include "pycore_dict.h"          // PyDictKeysObject
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "stringlib/eq.h"         // unicode_eq()

#include <stdbool.h>

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

static int dictresize(PyInterpreterState *interp, PyDictObject *mp,
                      uint8_t log_newsize, int unicode);

static PyObject* dict_iter(PyDictObject *dict);

#include "clinic/dictobject.c.h"


#if PyDict_MAXFREELIST > 0
static struct _Py_dict_state *
get_dict_state(PyInterpreterState *interp)
{
    return &interp->dict_state;
}
#endif


void
_PyDict_ClearFreeList(PyInterpreterState *interp)
{
#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = &interp->dict_state;
    while (state->numfree) {
        PyDictObject *op = state->free_list[--state->numfree];
        assert(PyDict_CheckExact(op));
        PyObject_GC_Del(op);
    }
    while (state->keys_numfree) {
        PyObject_Free(state->keys_free_list[--state->keys_numfree]);
    }
#endif
}


void
_PyDict_Fini(PyInterpreterState *interp)
{
    _PyDict_ClearFreeList(interp);
#if defined(Py_DEBUG) && PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = &interp->dict_state;
    state->numfree = -1;
    state->keys_numfree = -1;
#endif
}

static inline Py_hash_t
unicode_get_hash(PyObject *o)
{
    assert(PyUnicode_CheckExact(o));
    return _PyASCIIObject_CAST(o)->hash;
}

/* Print summary info about the state of the optimized allocator */
void
_PyDict_DebugMallocStats(FILE *out)
{
#if PyDict_MAXFREELIST > 0
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_dict_state *state = get_dict_state(interp);
    _PyDebugAllocatorStats(out, "free PyDictObject",
                           state->numfree, sizeof(PyDictObject));
#endif
}

#define DK_MASK(dk) (DK_SIZE(dk)-1)

static void free_keys_object(PyInterpreterState *interp, PyDictKeysObject *keys);

/* PyDictKeysObject has refcounts like PyObject does, so we have the
   following two functions to mirror what Py_INCREF() and Py_DECREF() do.
   (Keep in mind that PyDictKeysObject isn't actually a PyObject.)
   Likewise a PyDictKeysObject can be immortal (e.g. Py_EMPTY_KEYS),
   so we apply a naive version of what Py_INCREF() and Py_DECREF() do
   for immortal objects. */

static inline void
dictkeys_incref(PyDictKeysObject *dk)
{
    if (dk->dk_refcnt == _Py_IMMORTAL_REFCNT) {
        return;
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyInterpreterState_GET());
#endif
    dk->dk_refcnt++;
}

static inline void
dictkeys_decref(PyInterpreterState *interp, PyDictKeysObject *dk)
{
    if (dk->dk_refcnt == _Py_IMMORTAL_REFCNT) {
        return;
    }
    assert(dk->dk_refcnt > 0);
#ifdef Py_REF_DEBUG
    _Py_DecRefTotal(_PyInterpreterState_GET());
#endif
    if (--dk->dk_refcnt == 0) {
        free_keys_object(interp, dk);
    }
}

/* lookup indices.  returns DKIX_EMPTY, DKIX_DUMMY, or ix >=0 */
static inline Py_ssize_t
dictkeys_get_index(const PyDictKeysObject *keys, Py_ssize_t i)
{
    int log2size = DK_LOG_SIZE(keys);
    Py_ssize_t ix;

    if (log2size < 8) {
        const int8_t *indices = (const int8_t*)(keys->dk_indices);
        ix = indices[i];
    }
    else if (log2size < 16) {
        const int16_t *indices = (const int16_t*)(keys->dk_indices);
        ix = indices[i];
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        const int64_t *indices = (const int64_t*)(keys->dk_indices);
        ix = indices[i];
    }
#endif
    else {
        const int32_t *indices = (const int32_t*)(keys->dk_indices);
        ix = indices[i];
    }
    assert(ix >= DKIX_DUMMY);
    return ix;
}

/* write to indices. */
static inline void
dictkeys_set_index(PyDictKeysObject *keys, Py_ssize_t i, Py_ssize_t ix)
{
    int log2size = DK_LOG_SIZE(keys);

    assert(ix >= DKIX_DUMMY);
    assert(keys->dk_version == 0);

    if (log2size < 8) {
        int8_t *indices = (int8_t*)(keys->dk_indices);
        assert(ix <= 0x7f);
        indices[i] = (char)ix;
    }
    else if (log2size < 16) {
        int16_t *indices = (int16_t*)(keys->dk_indices);
        assert(ix <= 0x7fff);
        indices[i] = (int16_t)ix;
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        int64_t *indices = (int64_t*)(keys->dk_indices);
        indices[i] = ix;
    }
#endif
    else {
        int32_t *indices = (int32_t*)(keys->dk_indices);
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

/* Find the smallest dk_size >= minsize. */
static inline uint8_t
calculate_log2_keysize(Py_ssize_t minsize)
{
#if SIZEOF_LONG == SIZEOF_SIZE_T
    minsize = (minsize | PyDict_MINSIZE) - 1;
    return _Py_bit_length(minsize | (PyDict_MINSIZE-1));
#elif defined(_MSC_VER)
    // On 64bit Windows, sizeof(long) == 4.
    minsize = (minsize | PyDict_MINSIZE) - 1;
    unsigned long msb;
    _BitScanReverse64(&msb, (uint64_t)minsize);
    return (uint8_t)(msb + 1);
#else
    uint8_t log2_size;
    for (log2_size = PyDict_LOG_MINSIZE;
            (((Py_ssize_t)1) << log2_size) < minsize;
            log2_size++)
        ;
    return log2_size;
#endif
}

/* estimate_keysize is reverse function of USABLE_FRACTION.
 *
 * This can be used to reserve enough size to insert n entries without
 * resizing.
 */
static inline uint8_t
estimate_log2_keysize(Py_ssize_t n)
{
    return calculate_log2_keysize((n*3 + 1) / 2);
}


/* GROWTH_RATE. Growth rate upon hitting maximum load.
 * Currently set to used*3.
 * This means that dicts double in size when growing without deletions,
 * but have more head room when the number of deletions is on a par with the
 * number of insertions.  See also bpo-17563 and bpo-33205.
 *
 * GROWTH_RATE was set to used*4 up to version 3.2.
 * GROWTH_RATE was set to used*2 in version 3.3.0
 * GROWTH_RATE was set to used*2 + capacity/2 in 3.4.0-3.6.0.
 */
#define GROWTH_RATE(d) ((d)->ma_used*3)

/* This immutable, empty PyDictKeysObject is used for PyDict_Clear()
 * (which cannot fail and thus can do no allocation).
 */
static PyDictKeysObject empty_keys_struct = {
        _Py_IMMORTAL_REFCNT, /* dk_refcnt */
        0, /* dk_log2_size */
        0, /* dk_log2_index_bytes */
        DICT_KEYS_UNICODE, /* dk_kind */
        1, /* dk_version */
        0, /* dk_usable (immutable) */
        0, /* dk_nentries */
        {DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY,
         DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY, DKIX_EMPTY}, /* dk_indices */
};

#define Py_EMPTY_KEYS &empty_keys_struct

/* Uncomment to check the dict content in _PyDict_CheckConsistency() */
// #define DEBUG_PYDICT

#ifdef DEBUG_PYDICT
#  define ASSERT_CONSISTENT(op) assert(_PyDict_CheckConsistency((PyObject *)(op), 1))
#else
#  define ASSERT_CONSISTENT(op) assert(_PyDict_CheckConsistency((PyObject *)(op), 0))
#endif

static inline int
get_index_from_order(PyDictObject *mp, Py_ssize_t i)
{
    assert(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
    assert(i < (((char *)mp->ma_values)[-2]));
    return ((char *)mp->ma_values)[-3-i];
}

#ifdef DEBUG_PYDICT
static void
dump_entries(PyDictKeysObject *dk)
{
    for (Py_ssize_t i = 0; i < dk->dk_nentries; i++) {
        if (DK_IS_UNICODE(dk)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(dk)[i];
            printf("key=%p value=%p\n", ep->me_key, ep->me_value);
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(dk)[i];
            printf("key=%p hash=%lx value=%p\n", ep->me_key, ep->me_hash, ep->me_value);
        }
    }
}
#endif

int
_PyDict_CheckConsistency(PyObject *op, int check_content)
{
#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    assert(op != NULL);
    CHECK(PyDict_Check(op));
    PyDictObject *mp = (PyDictObject *)op;

    PyDictKeysObject *keys = mp->ma_keys;
    int splitted = _PyDict_HasSplitTable(mp);
    Py_ssize_t usable = USABLE_FRACTION(DK_SIZE(keys));

    CHECK(0 <= mp->ma_used && mp->ma_used <= usable);
    CHECK(0 <= keys->dk_usable && keys->dk_usable <= usable);
    CHECK(0 <= keys->dk_nentries && keys->dk_nentries <= usable);
    CHECK(keys->dk_usable + keys->dk_nentries <= usable);

    if (!splitted) {
        /* combined table */
        CHECK(keys->dk_kind != DICT_KEYS_SPLIT);
        CHECK(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
    }
    else {
        CHECK(keys->dk_kind == DICT_KEYS_SPLIT);
        CHECK(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
    }

    if (check_content) {
        for (Py_ssize_t i=0; i < DK_SIZE(keys); i++) {
            Py_ssize_t ix = dictkeys_get_index(keys, i);
            CHECK(DKIX_DUMMY <= ix && ix <= usable);
        }

        if (keys->dk_kind == DICT_KEYS_GENERAL) {
            PyDictKeyEntry *entries = DK_ENTRIES(keys);
            for (Py_ssize_t i=0; i < usable; i++) {
                PyDictKeyEntry *entry = &entries[i];
                PyObject *key = entry->me_key;

                if (key != NULL) {
                    /* test_dict fails if PyObject_Hash() is called again */
                    CHECK(entry->me_hash != -1);
                    CHECK(entry->me_value != NULL);

                    if (PyUnicode_CheckExact(key)) {
                        Py_hash_t hash = unicode_get_hash(key);
                        CHECK(entry->me_hash == hash);
                    }
                }
            }
        }
        else {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            for (Py_ssize_t i=0; i < usable; i++) {
                PyDictUnicodeEntry *entry = &entries[i];
                PyObject *key = entry->me_key;

                if (key != NULL) {
                    CHECK(PyUnicode_CheckExact(key));
                    Py_hash_t hash = unicode_get_hash(key);
                    CHECK(hash != -1);
                    if (!splitted) {
                        CHECK(entry->me_value != NULL);
                    }
                }

                if (splitted) {
                    CHECK(entry->me_value == NULL);
                }
            }
        }

        if (splitted) {
            CHECK(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
            /* splitted table */
            int duplicate_check = 0;
            for (Py_ssize_t i=0; i < mp->ma_used; i++) {
                int index = get_index_from_order(mp, i);
                CHECK((duplicate_check & (1<<index)) == 0);
                duplicate_check |= (1<<index);
                CHECK(mp->ma_values->values[index] != NULL);
            }
        }
    }
    return 1;

#undef CHECK
}


static PyDictKeysObject*
new_keys_object(PyInterpreterState *interp, uint8_t log2_size, bool unicode)
{
    PyDictKeysObject *dk;
    Py_ssize_t usable;
    int log2_bytes;
    size_t entry_size = unicode ? sizeof(PyDictUnicodeEntry) : sizeof(PyDictKeyEntry);

    assert(log2_size >= PyDict_LOG_MINSIZE);

    usable = USABLE_FRACTION((size_t)1<<log2_size);
    if (log2_size < 8) {
        log2_bytes = log2_size;
    }
    else if (log2_size < 16) {
        log2_bytes = log2_size + 1;
    }
#if SIZEOF_VOID_P > 4
    else if (log2_size >= 32) {
        log2_bytes = log2_size + 3;
    }
#endif
    else {
        log2_bytes = log2_size + 2;
    }

#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = get_dict_state(interp);
#ifdef Py_DEBUG
    // new_keys_object() must not be called after _PyDict_Fini()
    assert(state->keys_numfree != -1);
#endif
    if (log2_size == PyDict_LOG_MINSIZE && unicode && state->keys_numfree > 0) {
        dk = state->keys_free_list[--state->keys_numfree];
        OBJECT_STAT_INC(from_freelist);
    }
    else
#endif
    {
        dk = PyObject_Malloc(sizeof(PyDictKeysObject)
                             + ((size_t)1 << log2_bytes)
                             + entry_size * usable);
        if (dk == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyInterpreterState_GET());
#endif
    dk->dk_refcnt = 1;
    dk->dk_log2_size = log2_size;
    dk->dk_log2_index_bytes = log2_bytes;
    dk->dk_kind = unicode ? DICT_KEYS_UNICODE : DICT_KEYS_GENERAL;
    dk->dk_nentries = 0;
    dk->dk_usable = usable;
    dk->dk_version = 0;
    memset(&dk->dk_indices[0], 0xff, ((size_t)1 << log2_bytes));
    memset(&dk->dk_indices[(size_t)1 << log2_bytes], 0, entry_size * usable);
    return dk;
}

static void
free_keys_object(PyInterpreterState *interp, PyDictKeysObject *keys)
{
    assert(keys != Py_EMPTY_KEYS);
    if (DK_IS_UNICODE(keys)) {
        PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
        Py_ssize_t i, n;
        for (i = 0, n = keys->dk_nentries; i < n; i++) {
            Py_XDECREF(entries[i].me_key);
            Py_XDECREF(entries[i].me_value);
        }
    }
    else {
        PyDictKeyEntry *entries = DK_ENTRIES(keys);
        Py_ssize_t i, n;
        for (i = 0, n = keys->dk_nentries; i < n; i++) {
            Py_XDECREF(entries[i].me_key);
            Py_XDECREF(entries[i].me_value);
        }
    }
#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = get_dict_state(interp);
#ifdef Py_DEBUG
    // free_keys_object() must not be called after _PyDict_Fini()
    assert(state->keys_numfree != -1);
#endif
    if (DK_LOG_SIZE(keys) == PyDict_LOG_MINSIZE
            && state->keys_numfree < PyDict_MAXFREELIST
            && DK_IS_UNICODE(keys)) {
        state->keys_free_list[state->keys_numfree++] = keys;
        OBJECT_STAT_INC(to_freelist);
        return;
    }
#endif
    PyObject_Free(keys);
}

static inline PyDictValues*
new_values(size_t size)
{
    assert(size >= 1);
    size_t prefix_size = _Py_SIZE_ROUND_UP(size+2, sizeof(PyObject *));
    assert(prefix_size < 256);
    size_t n = prefix_size + size * sizeof(PyObject *);
    uint8_t *mem = PyMem_Malloc(n);
    if (mem == NULL) {
        return NULL;
    }
    assert(prefix_size % sizeof(PyObject *) == 0);
    mem[prefix_size-1] = (uint8_t)prefix_size;
    return (PyDictValues*)(mem + prefix_size);
}

static inline void
free_values(PyDictValues *values)
{
    int prefix_size = ((uint8_t *)values)[-1];
    PyMem_Free(((char *)values)-prefix_size);
}

/* Consumes a reference to the keys object */
static PyObject *
new_dict(PyInterpreterState *interp,
         PyDictKeysObject *keys, PyDictValues *values,
         Py_ssize_t used, int free_values_on_failure)
{
    PyDictObject *mp;
    assert(keys != NULL);
#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = get_dict_state(interp);
#ifdef Py_DEBUG
    // new_dict() must not be called after _PyDict_Fini()
    assert(state->numfree != -1);
#endif
    if (state->numfree) {
        mp = state->free_list[--state->numfree];
        assert (mp != NULL);
        assert (Py_IS_TYPE(mp, &PyDict_Type));
        OBJECT_STAT_INC(from_freelist);
        _Py_NewReference((PyObject *)mp);
    }
    else
#endif
    {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (mp == NULL) {
            dictkeys_decref(interp, keys);
            if (free_values_on_failure) {
                free_values(values);
            }
            return NULL;
        }
    }
    mp->ma_keys = keys;
    mp->ma_values = values;
    mp->ma_used = used;
    mp->ma_version_tag = DICT_NEXT_VERSION(interp);
    ASSERT_CONSISTENT(mp);
    return (PyObject *)mp;
}

static inline size_t
shared_keys_usable_size(PyDictKeysObject *keys)
{
    return (size_t)keys->dk_nentries + (size_t)keys->dk_usable;
}

/* Consumes a reference to the keys object */
static PyObject *
new_dict_with_shared_keys(PyInterpreterState *interp, PyDictKeysObject *keys)
{
    size_t size = shared_keys_usable_size(keys);
    PyDictValues *values = new_values(size);
    if (values == NULL) {
        dictkeys_decref(interp, keys);
        return PyErr_NoMemory();
    }
    ((char *)values)[-2] = 0;
    for (size_t i = 0; i < size; i++) {
        values->values[i] = NULL;
    }
    return new_dict(interp, keys, values, 0, 1);
}


static PyDictKeysObject *
clone_combined_dict_keys(PyDictObject *orig)
{
    assert(PyDict_Check(orig));
    assert(Py_TYPE(orig)->tp_iter == (getiterfunc)dict_iter);
    assert(orig->ma_values == NULL);
    assert(orig->ma_keys != Py_EMPTY_KEYS);
    assert(orig->ma_keys->dk_refcnt == 1);

    size_t keys_size = _PyDict_KeysSize(orig->ma_keys);
    PyDictKeysObject *keys = PyObject_Malloc(keys_size);
    if (keys == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    memcpy(keys, orig->ma_keys, keys_size);

    /* After copying key/value pairs, we need to incref all
       keys and values and they are about to be co-owned by a
       new dict object. */
    PyObject **pkey, **pvalue;
    size_t offs;
    if (DK_IS_UNICODE(orig->ma_keys)) {
        PyDictUnicodeEntry *ep0 = DK_UNICODE_ENTRIES(keys);
        pkey = &ep0->me_key;
        pvalue = &ep0->me_value;
        offs = sizeof(PyDictUnicodeEntry) / sizeof(PyObject*);
    }
    else {
        PyDictKeyEntry *ep0 = DK_ENTRIES(keys);
        pkey = &ep0->me_key;
        pvalue = &ep0->me_value;
        offs = sizeof(PyDictKeyEntry) / sizeof(PyObject*);
    }

    Py_ssize_t n = keys->dk_nentries;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *value = *pvalue;
        if (value != NULL) {
            Py_INCREF(value);
            Py_INCREF(*pkey);
        }
        pvalue += offs;
        pkey += offs;
    }

    /* Since we copied the keys table we now have an extra reference
       in the system.  Manually call increment _Py_RefTotal to signal that
       we have it now; calling dictkeys_incref would be an error as
       keys->dk_refcnt is already set to 1 (after memcpy). */
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyInterpreterState_GET());
#endif
    return keys;
}

PyObject *
PyDict_New(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    /* We don't incref Py_EMPTY_KEYS here because it is immortal. */
    return new_dict(interp, Py_EMPTY_KEYS, NULL, 0, 0);
}

/* Search index of hash table from offset of entry table */
static Py_ssize_t
lookdict_index(PyDictKeysObject *k, Py_hash_t hash, Py_ssize_t index)
{
    size_t mask = DK_MASK(k);
    size_t perturb = (size_t)hash;
    size_t i = (size_t)hash & mask;

    for (;;) {
        Py_ssize_t ix = dictkeys_get_index(k, i);
        if (ix == index) {
            return i;
        }
        if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

// Search non-Unicode key from Unicode table
static Py_ssize_t
unicodekeys_lookup_generic(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep0 = DK_UNICODE_ENTRIES(dk);
    size_t mask = DK_MASK(dk);
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    Py_ssize_t ix;
    for (;;) {
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            PyDictUnicodeEntry *ep = &ep0[ix];
            assert(ep->me_key != NULL);
            assert(PyUnicode_CheckExact(ep->me_key));
            if (ep->me_key == key) {
                return ix;
            }
            if (unicode_get_hash(ep->me_key) == hash) {
                PyObject *startkey = ep->me_key;
                Py_INCREF(startkey);
                int cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
                Py_DECREF(startkey);
                if (cmp < 0) {
                    return DKIX_ERROR;
                }
                if (dk == mp->ma_keys && ep->me_key == startkey) {
                    if (cmp > 0) {
                        return ix;
                    }
                }
                else {
                    /* The dict was mutated, restart */
                    return DKIX_KEY_CHANGED;
                }
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

// Search Unicode key from Unicode table.
static Py_ssize_t _Py_HOT_FUNCTION
unicodekeys_lookup_unicode(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep0 = DK_UNICODE_ENTRIES(dk);
    size_t mask = DK_MASK(dk);
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    Py_ssize_t ix;
    for (;;) {
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            PyDictUnicodeEntry *ep = &ep0[ix];
            assert(ep->me_key != NULL);
            assert(PyUnicode_CheckExact(ep->me_key));
            if (ep->me_key == key ||
                    (unicode_get_hash(ep->me_key) == hash && unicode_eq(ep->me_key, key))) {
                return ix;
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
        // Manual loop unrolling
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            PyDictUnicodeEntry *ep = &ep0[ix];
            assert(ep->me_key != NULL);
            assert(PyUnicode_CheckExact(ep->me_key));
            if (ep->me_key == key ||
                    (unicode_get_hash(ep->me_key) == hash && unicode_eq(ep->me_key, key))) {
                return ix;
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

// Search key from Generic table.
static Py_ssize_t
dictkeys_generic_lookup(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    PyDictKeyEntry *ep0 = DK_ENTRIES(dk);
    size_t mask = DK_MASK(dk);
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    Py_ssize_t ix;
    for (;;) {
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            PyDictKeyEntry *ep = &ep0[ix];
            assert(ep->me_key != NULL);
            if (ep->me_key == key) {
                return ix;
            }
            if (ep->me_hash == hash) {
                PyObject *startkey = ep->me_key;
                Py_INCREF(startkey);
                int cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
                Py_DECREF(startkey);
                if (cmp < 0) {
                    return DKIX_ERROR;
                }
                if (dk == mp->ma_keys && ep->me_key == startkey) {
                    if (cmp > 0) {
                        return ix;
                    }
                }
                else {
                    /* The dict was mutated, restart */
                    return DKIX_KEY_CHANGED;
                }
            }
        }
        else if (ix == DKIX_EMPTY) {
            return DKIX_EMPTY;
        }
        perturb >>= PERTURB_SHIFT;
        i = mask & (i*5 + perturb + 1);
    }
    Py_UNREACHABLE();
}

/* Lookup a string in a (all unicode) dict keys.
 * Returns DKIX_ERROR if key is not a string,
 * or if the dict keys is not all strings.
 * If the keys is present then return the index of key.
 * If the key is not present then return DKIX_EMPTY.
 */
Py_ssize_t
_PyDictKeys_StringLookup(PyDictKeysObject* dk, PyObject *key)
{
    DictKeysKind kind = dk->dk_kind;
    if (!PyUnicode_CheckExact(key) || kind == DICT_KEYS_GENERAL) {
        return DKIX_ERROR;
    }
    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = PyUnicode_Type.tp_hash(key);
        if (hash == -1) {
            PyErr_Clear();
            return DKIX_ERROR;
        }
    }
    return unicodekeys_lookup_unicode(dk, key, hash);
}

/*
The basic lookup function used by all operations.
This is based on Algorithm D from Knuth Vol. 3, Sec. 6.4.
Open addressing is preferred over chaining since the link overhead for
chaining would be substantial (100% with typical malloc overhead).

The initial probe index is computed as hash mod the table size. Subsequent
probe indices are computed as explained earlier.

All arithmetic on hash should ignore overflow.

_Py_dict_lookup() is general-purpose, and may return DKIX_ERROR if (and only if) a
comparison raises an exception.
When the key isn't found a DKIX_EMPTY is returned.
*/
Py_ssize_t
_Py_dict_lookup(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr)
{
    PyDictKeysObject *dk;
    DictKeysKind kind;
    Py_ssize_t ix;

start:
    dk = mp->ma_keys;
    kind = dk->dk_kind;

    if (kind != DICT_KEYS_GENERAL) {
        if (PyUnicode_CheckExact(key)) {
            ix = unicodekeys_lookup_unicode(dk, key, hash);
        }
        else {
            ix = unicodekeys_lookup_generic(mp, dk, key, hash);
            if (ix == DKIX_KEY_CHANGED) {
                goto start;
            }
        }

        if (ix >= 0) {
            if (kind == DICT_KEYS_SPLIT) {
                *value_addr = mp->ma_values->values[ix];
            }
            else {
                *value_addr = DK_UNICODE_ENTRIES(dk)[ix].me_value;
            }
        }
        else {
            *value_addr = NULL;
        }
    }
    else {
        ix = dictkeys_generic_lookup(mp, dk, key, hash);
        if (ix == DKIX_KEY_CHANGED) {
            goto start;
        }
        if (ix >= 0) {
            *value_addr = DK_ENTRIES(dk)[ix].me_value;
        }
        else {
            *value_addr = NULL;
        }
    }

    return ix;
}

int
_PyDict_HasOnlyStringKeys(PyObject *dict)
{
    Py_ssize_t pos = 0;
    PyObject *key, *value;
    assert(PyDict_Check(dict));
    /* Shortcut */
    if (((PyDictObject *)dict)->ma_keys->dk_kind != DICT_KEYS_GENERAL)
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

    if (!PyDict_CheckExact(op) || !_PyObject_GC_IS_TRACKED(op))
        return;

    mp = (PyDictObject *) op;
    numentries = mp->ma_keys->dk_nentries;
    if (_PyDict_HasSplitTable(mp)) {
        for (i = 0; i < numentries; i++) {
            if ((value = mp->ma_values->values[i]) == NULL)
                continue;
            if (_PyObject_GC_MAY_BE_TRACKED(value)) {
                return;
            }
        }
    }
    else {
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep0 = DK_UNICODE_ENTRIES(mp->ma_keys);
            for (i = 0; i < numentries; i++) {
                if ((value = ep0[i].me_value) == NULL)
                    continue;
                if (_PyObject_GC_MAY_BE_TRACKED(value))
                    return;
            }
        }
        else {
            PyDictKeyEntry *ep0 = DK_ENTRIES(mp->ma_keys);
            for (i = 0; i < numentries; i++) {
                if ((value = ep0[i].me_value) == NULL)
                    continue;
                if (_PyObject_GC_MAY_BE_TRACKED(value) ||
                    _PyObject_GC_MAY_BE_TRACKED(ep0[i].me_key))
                    return;
            }
        }
    }
    _PyObject_GC_UNTRACK(op);
}

/* Internal function to find slot for an item from its hash
   when it is known that the key is not present in the dict.

   The dict must be combined. */
static Py_ssize_t
find_empty_slot(PyDictKeysObject *keys, Py_hash_t hash)
{
    assert(keys != NULL);

    const size_t mask = DK_MASK(keys);
    size_t i = hash & mask;
    Py_ssize_t ix = dictkeys_get_index(keys, i);
    for (size_t perturb = hash; ix >= 0;) {
        perturb >>= PERTURB_SHIFT;
        i = (i*5 + perturb + 1) & mask;
        ix = dictkeys_get_index(keys, i);
    }
    return i;
}

static int
insertion_resize(PyInterpreterState *interp, PyDictObject *mp, int unicode)
{
    return dictresize(interp, mp, calculate_log2_keysize(GROWTH_RATE(mp)), unicode);
}

static Py_ssize_t
insert_into_dictkeys(PyDictKeysObject *keys, PyObject *name)
{
    assert(PyUnicode_CheckExact(name));
    Py_hash_t hash = unicode_get_hash(name);
    if (hash == -1) {
        hash = PyUnicode_Type.tp_hash(name);
        if (hash == -1) {
            PyErr_Clear();
            return DKIX_EMPTY;
        }
    }
    Py_ssize_t ix = unicodekeys_lookup_unicode(keys, name, hash);
    if (ix == DKIX_EMPTY) {
        if (keys->dk_usable <= 0) {
            return DKIX_EMPTY;
        }
        /* Insert into new slot. */
        keys->dk_version = 0;
        Py_ssize_t hashpos = find_empty_slot(keys, hash);
        ix = keys->dk_nentries;
        PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(keys)[ix];
        dictkeys_set_index(keys, hashpos, ix);
        assert(ep->me_key == NULL);
        ep->me_key = Py_NewRef(name);
        keys->dk_usable--;
        keys->dk_nentries++;
    }
    assert (ix < SHARED_KEYS_MAX_SIZE);
    return ix;
}

/*
Internal routine to insert a new item into the table.
Used both by the internal resize routine and by the public insert routine.
Returns -1 if an error occurred, or 0 on success.
Consumes key and value references.
*/
static int
insertdict(PyInterpreterState *interp, PyDictObject *mp,
           PyObject *key, Py_hash_t hash, PyObject *value)
{
    PyObject *old_value;

    if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
        if (insertion_resize(interp, mp, 0) < 0)
            goto Fail;
        assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
    }

    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR)
        goto Fail;

    MAINTAIN_TRACKING(mp, key, value);

    if (ix == DKIX_EMPTY) {
        assert(old_value == NULL);
        if (mp->ma_keys->dk_usable <= 0) {
            /* Need to resize. */
            if (insertion_resize(interp, mp, 1) < 0)
                goto Fail;
        }

        uint64_t new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_ADDED, mp, key, value);
        /* Insert into new slot. */
        mp->ma_keys->dk_version = 0;

        Py_ssize_t hashpos = find_empty_slot(mp->ma_keys, hash);
        dictkeys_set_index(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);

        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep;
            ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
            ep->me_key = key;
            if (mp->ma_values) {
                Py_ssize_t index = mp->ma_keys->dk_nentries;
                _PyDictValues_AddToInsertionOrder(mp->ma_values, index);
                assert (mp->ma_values->values[index] == NULL);
                mp->ma_values->values[index] = value;
            }
            else {
                ep->me_value = value;
            }
        }
        else {
            PyDictKeyEntry *ep;
            ep = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
            ep->me_key = key;
            ep->me_hash = hash;
            ep->me_value = value;
        }
        mp->ma_used++;
        mp->ma_version_tag = new_version;
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;
        assert(mp->ma_keys->dk_usable >= 0);
        ASSERT_CONSISTENT(mp);
        return 0;
    }

    if (old_value != value) {
        uint64_t new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_MODIFIED, mp, key, value);
        if (_PyDict_HasSplitTable(mp)) {
            mp->ma_values->values[ix] = value;
            if (old_value == NULL) {
                _PyDictValues_AddToInsertionOrder(mp->ma_values, ix);
                mp->ma_used++;
            }
        }
        else {
            assert(old_value != NULL);
            if (DK_IS_UNICODE(mp->ma_keys)) {
                DK_UNICODE_ENTRIES(mp->ma_keys)[ix].me_value = value;
            }
            else {
                DK_ENTRIES(mp->ma_keys)[ix].me_value = value;
            }
        }
        mp->ma_version_tag = new_version;
    }
    Py_XDECREF(old_value); /* which **CAN** re-enter (see issue #22653) */
    ASSERT_CONSISTENT(mp);
    Py_DECREF(key);
    return 0;

Fail:
    Py_DECREF(value);
    Py_DECREF(key);
    return -1;
}

// Same to insertdict but specialized for ma_keys = Py_EMPTY_KEYS.
// Consumes key and value references.
static int
insert_to_emptydict(PyInterpreterState *interp, PyDictObject *mp,
                    PyObject *key, Py_hash_t hash, PyObject *value)
{
    assert(mp->ma_keys == Py_EMPTY_KEYS);

    int unicode = PyUnicode_CheckExact(key);
    PyDictKeysObject *newkeys = new_keys_object(
            interp, PyDict_LOG_MINSIZE, unicode);
    if (newkeys == NULL) {
        Py_DECREF(key);
        Py_DECREF(value);
        return -1;
    }
    uint64_t new_version = _PyDict_NotifyEvent(
            interp, PyDict_EVENT_ADDED, mp, key, value);

    /* We don't decref Py_EMPTY_KEYS here because it is immortal. */
    mp->ma_keys = newkeys;
    mp->ma_values = NULL;

    MAINTAIN_TRACKING(mp, key, value);

    size_t hashpos = (size_t)hash & (PyDict_MINSIZE-1);
    dictkeys_set_index(mp->ma_keys, hashpos, 0);
    if (unicode) {
        PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(mp->ma_keys);
        ep->me_key = key;
        ep->me_value = value;
    }
    else {
        PyDictKeyEntry *ep = DK_ENTRIES(mp->ma_keys);
        ep->me_key = key;
        ep->me_hash = hash;
        ep->me_value = value;
    }
    mp->ma_used++;
    mp->ma_version_tag = new_version;
    mp->ma_keys->dk_usable--;
    mp->ma_keys->dk_nentries++;
    return 0;
}

/*
Internal routine used by dictresize() to build a hashtable of entries.
*/
static void
build_indices_generic(PyDictKeysObject *keys, PyDictKeyEntry *ep, Py_ssize_t n)
{
    size_t mask = DK_MASK(keys);
    for (Py_ssize_t ix = 0; ix != n; ix++, ep++) {
        Py_hash_t hash = ep->me_hash;
        size_t i = hash & mask;
        for (size_t perturb = hash; dictkeys_get_index(keys, i) != DKIX_EMPTY;) {
            perturb >>= PERTURB_SHIFT;
            i = mask & (i*5 + perturb + 1);
        }
        dictkeys_set_index(keys, i, ix);
    }
}

static void
build_indices_unicode(PyDictKeysObject *keys, PyDictUnicodeEntry *ep, Py_ssize_t n)
{
    size_t mask = DK_MASK(keys);
    for (Py_ssize_t ix = 0; ix != n; ix++, ep++) {
        Py_hash_t hash = unicode_get_hash(ep->me_key);
        assert(hash != -1);
        size_t i = hash & mask;
        for (size_t perturb = hash; dictkeys_get_index(keys, i) != DKIX_EMPTY;) {
            perturb >>= PERTURB_SHIFT;
            i = mask & (i*5 + perturb + 1);
        }
        dictkeys_set_index(keys, i, ix);
    }
}

/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
If a table is split (its keys and hashes are shared, its values are not),
then the values are temporarily copied into the table, it is resized as
a combined table, then the me_value slots in the old table are NULLed out.
After resizing a table is always combined.

This function supports:
 - Unicode split -> Unicode combined or Generic
 - Unicode combined -> Unicode combined or Generic
 - Generic -> Generic
*/
static int
dictresize(PyInterpreterState *interp, PyDictObject *mp,
           uint8_t log2_newsize, int unicode)
{
    PyDictKeysObject *oldkeys;
    PyDictValues *oldvalues;

    if (log2_newsize >= SIZEOF_SIZE_T*8) {
        PyErr_NoMemory();
        return -1;
    }
    assert(log2_newsize >= PyDict_LOG_MINSIZE);

    oldkeys = mp->ma_keys;
    oldvalues = mp->ma_values;

    if (!DK_IS_UNICODE(oldkeys)) {
        unicode = 0;
    }

    /* NOTE: Current odict checks mp->ma_keys to detect resize happen.
     * So we can't reuse oldkeys even if oldkeys->dk_size == newsize.
     * TODO: Try reusing oldkeys when reimplement odict.
     */

    /* Allocate a new table. */
    mp->ma_keys = new_keys_object(interp, log2_newsize, unicode);
    if (mp->ma_keys == NULL) {
        mp->ma_keys = oldkeys;
        return -1;
    }
    // New table must be large enough.
    assert(mp->ma_keys->dk_usable >= mp->ma_used);

    Py_ssize_t numentries = mp->ma_used;

    if (oldvalues != NULL) {
         PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
        /* Convert split table into new combined table.
         * We must incref keys; we can transfer values.
         */
        if (mp->ma_keys->dk_kind == DICT_KEYS_GENERAL) {
            // split -> generic
            PyDictKeyEntry *newentries = DK_ENTRIES(mp->ma_keys);

            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                PyDictUnicodeEntry *ep = &oldentries[index];
                assert(oldvalues->values[index] != NULL);
                newentries[i].me_key = Py_NewRef(ep->me_key);
                newentries[i].me_hash = unicode_get_hash(ep->me_key);
                newentries[i].me_value = oldvalues->values[index];
            }
            build_indices_generic(mp->ma_keys, newentries, numentries);
        }
        else { // split -> combined unicode
            PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(mp->ma_keys);

            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                PyDictUnicodeEntry *ep = &oldentries[index];
                assert(oldvalues->values[index] != NULL);
                newentries[i].me_key = Py_NewRef(ep->me_key);
                newentries[i].me_value = oldvalues->values[index];
            }
            build_indices_unicode(mp->ma_keys, newentries, numentries);
        }
        dictkeys_decref(interp, oldkeys);
        mp->ma_values = NULL;
        free_values(oldvalues);
    }
    else {  // oldkeys is combined.
        if (oldkeys->dk_kind == DICT_KEYS_GENERAL) {
            // generic -> generic
            assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
            PyDictKeyEntry *oldentries = DK_ENTRIES(oldkeys);
            PyDictKeyEntry *newentries = DK_ENTRIES(mp->ma_keys);
            if (oldkeys->dk_nentries == numentries) {
                memcpy(newentries, oldentries, numentries * sizeof(PyDictKeyEntry));
            }
            else {
                PyDictKeyEntry *ep = oldentries;
                for (Py_ssize_t i = 0; i < numentries; i++) {
                    while (ep->me_value == NULL)
                        ep++;
                    newentries[i] = *ep++;
                }
            }
            build_indices_generic(mp->ma_keys, newentries, numentries);
        }
        else {  // oldkeys is combined unicode
            PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
            if (unicode) { // combined unicode -> combined unicode
                PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(mp->ma_keys);
                if (oldkeys->dk_nentries == numentries && mp->ma_keys->dk_kind == DICT_KEYS_UNICODE) {
                    memcpy(newentries, oldentries, numentries * sizeof(PyDictUnicodeEntry));
                }
                else {
                    PyDictUnicodeEntry *ep = oldentries;
                    for (Py_ssize_t i = 0; i < numentries; i++) {
                        while (ep->me_value == NULL)
                            ep++;
                        newentries[i] = *ep++;
                    }
                }
                build_indices_unicode(mp->ma_keys, newentries, numentries);
            }
            else { // combined unicode -> generic
                PyDictKeyEntry *newentries = DK_ENTRIES(mp->ma_keys);
                PyDictUnicodeEntry *ep = oldentries;
                for (Py_ssize_t i = 0; i < numentries; i++) {
                    while (ep->me_value == NULL)
                        ep++;
                    newentries[i].me_key = ep->me_key;
                    newentries[i].me_hash = unicode_get_hash(ep->me_key);
                    newentries[i].me_value = ep->me_value;
                    ep++;
                }
                build_indices_generic(mp->ma_keys, newentries, numentries);
            }
        }

        // We can not use free_keys_object here because key's reference
        // are moved already.
        if (oldkeys != Py_EMPTY_KEYS) {
#ifdef Py_REF_DEBUG
            _Py_DecRefTotal(_PyInterpreterState_GET());
#endif
            assert(oldkeys->dk_kind != DICT_KEYS_SPLIT);
            assert(oldkeys->dk_refcnt == 1);
#if PyDict_MAXFREELIST > 0
            struct _Py_dict_state *state = get_dict_state(interp);
#ifdef Py_DEBUG
            // dictresize() must not be called after _PyDict_Fini()
            assert(state->keys_numfree != -1);
#endif
            if (DK_LOG_SIZE(oldkeys) == PyDict_LOG_MINSIZE &&
                    DK_IS_UNICODE(oldkeys) &&
                    state->keys_numfree < PyDict_MAXFREELIST)
            {
                state->keys_free_list[state->keys_numfree++] = oldkeys;
                OBJECT_STAT_INC(to_freelist);
            }
            else
#endif
            {
                PyObject_Free(oldkeys);
            }
        }
    }

    mp->ma_keys->dk_usable -= numentries;
    mp->ma_keys->dk_nentries = numentries;
    ASSERT_CONSISTENT(mp);
    return 0;
}

static PyObject *
dict_new_presized(PyInterpreterState *interp, Py_ssize_t minused, bool unicode)
{
    const uint8_t log2_max_presize = 17;
    const Py_ssize_t max_presize = ((Py_ssize_t)1) << log2_max_presize;
    uint8_t log2_newsize;
    PyDictKeysObject *new_keys;

    if (minused <= USABLE_FRACTION(PyDict_MINSIZE)) {
        return PyDict_New();
    }
    /* There are no strict guarantee that returned dict can contain minused
     * items without resize.  So we create medium size dict instead of very
     * large dict or MemoryError.
     */
    if (minused > USABLE_FRACTION(max_presize)) {
        log2_newsize = log2_max_presize;
    }
    else {
        log2_newsize = estimate_log2_keysize(minused);
    }

    new_keys = new_keys_object(interp, log2_newsize, unicode);
    if (new_keys == NULL)
        return NULL;
    return new_dict(interp, new_keys, NULL, 0, 0);
}

PyObject *
_PyDict_NewPresized(Py_ssize_t minused)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return dict_new_presized(interp, minused, false);
}

PyObject *
_PyDict_FromItems(PyObject *const *keys, Py_ssize_t keys_offset,
                  PyObject *const *values, Py_ssize_t values_offset,
                  Py_ssize_t length)
{
    bool unicode = true;
    PyObject *const *ks = keys;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    for (Py_ssize_t i = 0; i < length; i++) {
        if (!PyUnicode_CheckExact(*ks)) {
            unicode = false;
            break;
        }
        ks += keys_offset;
    }

    PyObject *dict = dict_new_presized(interp, length, unicode);
    if (dict == NULL) {
        return NULL;
    }

    ks = keys;
    PyObject *const *vs = values;

    for (Py_ssize_t i = 0; i < length; i++) {
        PyObject *key = *ks;
        PyObject *value = *vs;
        if (PyDict_SetItem(dict, key, value) < 0) {
            Py_DECREF(dict);
            return NULL;
        }
        ks += keys_offset;
        vs += values_offset;
    }

    return dict;
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
    if (!PyDict_Check(op)) {
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)op;

    Py_hash_t hash;
    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            PyErr_Clear();
            return NULL;
        }
    }

    PyThreadState *tstate = _PyThreadState_GET();
#ifdef Py_DEBUG
    // bpo-40839: Before Python 3.10, it was possible to call PyDict_GetItem()
    // with the GIL released.
    _Py_EnsureTstateNotNULL(tstate);
#endif

    /* Preserve the existing exception */
    PyObject *value;
    Py_ssize_t ix; (void)ix;


    PyObject *exc = _PyErr_GetRaisedException(tstate);
    ix = _Py_dict_lookup(mp, key, hash, &value);

    /* Ignore any exception raised by the lookup */
    _PyErr_SetRaisedException(tstate, exc);


    assert(ix >= 0 || value == NULL);
    return value;
}

Py_ssize_t
_PyDict_LookupIndex(PyDictObject *mp, PyObject *key)
{
    PyObject *value;
    assert(PyDict_CheckExact((PyObject*)mp));
    assert(PyUnicode_CheckExact(key));

    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            return -1;
        }
    }

    return _Py_dict_lookup(mp, key, hash, &value);
}

/* Same as PyDict_GetItemWithError() but with hash supplied by caller.
   This returns NULL *with* an exception set if an exception occurred.
   It returns NULL *without* an exception set if the key wasn't present.
*/
PyObject *
_PyDict_GetItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix; (void)ix;
    PyDictObject *mp = (PyDictObject *)op;
    PyObject *value;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    ix = _Py_dict_lookup(mp, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    return value;
}

/* Variant of PyDict_GetItem() that doesn't suppress exceptions.
   This returns NULL *with* an exception set if an exception occurred.
   It returns NULL *without* an exception set if the key wasn't present.
*/
PyObject *
PyDict_GetItemWithError(PyObject *op, PyObject *key)
{
    Py_ssize_t ix; (void)ix;
    Py_hash_t hash;
    PyDictObject*mp = (PyDictObject *)op;
    PyObject *value;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1)
    {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            return NULL;
        }
    }

    ix = _Py_dict_lookup(mp, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    return value;
}

PyObject *
_PyDict_GetItemWithError(PyObject *dp, PyObject *kv)
{
    assert(PyUnicode_CheckExact(kv));
    Py_hash_t hash = kv->ob_type->tp_hash(kv);
    if (hash == -1) {
        return NULL;
    }
    return _PyDict_GetItem_KnownHash(dp, kv, hash);
}

PyObject *
_PyDict_GetItemIdWithError(PyObject *dp, _Py_Identifier *key)
{
    PyObject *kv;
    kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL)
        return NULL;
    Py_hash_t hash = unicode_get_hash(kv);
    assert (hash != -1);  /* interned strings have their hash value initialised */
    return _PyDict_GetItem_KnownHash(dp, kv, hash);
}

PyObject *
_PyDict_GetItemStringWithError(PyObject *v, const char *key)
{
    PyObject *kv, *rv;
    kv = PyUnicode_FromString(key);
    if (kv == NULL) {
        return NULL;
    }
    rv = PyDict_GetItemWithError(v, kv);
    Py_DECREF(kv);
    return rv;
}

/* Fast version of global value lookup (LOAD_GLOBAL).
 * Lookup in globals, then builtins.
 *
 *
 *
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
    PyObject *value;

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }

    /* namespace 1: globals */
    ix = _Py_dict_lookup(globals, key, hash, &value);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix != DKIX_EMPTY && value != NULL)
        return value;

    /* namespace 2: builtins */
    ix = _Py_dict_lookup(builtins, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    return value;
}

/* Consumes references to key and value */
int
_PyDict_SetItem_Take2(PyDictObject *mp, PyObject *key, PyObject *value)
{
    assert(key);
    assert(value);
    assert(PyDict_Check(mp));
    Py_hash_t hash;
    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            Py_DECREF(key);
            Py_DECREF(value);
            return -1;
        }
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return insert_to_emptydict(interp, mp, key, hash, value);
    }
    /* insertdict() handles any resizing that might be necessary */
    return insertdict(interp, mp, key, hash, value);
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
    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(value);
    return _PyDict_SetItem_Take2((PyDictObject *)op,
                                 Py_NewRef(key), Py_NewRef(value));
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

    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return insert_to_emptydict(interp, mp, Py_NewRef(key), hash, Py_NewRef(value));
    }
    /* insertdict() handles any resizing that might be necessary */
    return insertdict(interp, mp, Py_NewRef(key), hash, Py_NewRef(value));
}

static void
delete_index_from_values(PyDictValues *values, Py_ssize_t ix)
{
    uint8_t *size_ptr = ((uint8_t *)values)-2;
    int size = *size_ptr;
    int i;
    for (i = 1; size_ptr[-i] != ix; i++) {
        assert(i <= size);
    }
    assert(i <= size);
    for (; i < size; i++) {
        size_ptr[-i] = size_ptr[-i-1];
    }
    *size_ptr = size -1;
}

static int
delitem_common(PyDictObject *mp, Py_hash_t hash, Py_ssize_t ix,
               PyObject *old_value, uint64_t new_version)
{
    PyObject *old_key;

    Py_ssize_t hashpos = lookdict_index(mp->ma_keys, hash, ix);
    assert(hashpos >= 0);

    mp->ma_used--;
    mp->ma_version_tag = new_version;
    if (mp->ma_values) {
        assert(old_value == mp->ma_values->values[ix]);
        mp->ma_values->values[ix] = NULL;
        assert(ix < SHARED_KEYS_MAX_SIZE);
        /* Update order */
        delete_index_from_values(mp->ma_values, ix);
        ASSERT_CONSISTENT(mp);
    }
    else {
        mp->ma_keys->dk_version = 0;
        dictkeys_set_index(mp->ma_keys, hashpos, DKIX_DUMMY);
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[ix];
            old_key = ep->me_key;
            ep->me_key = NULL;
            ep->me_value = NULL;
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[ix];
            old_key = ep->me_key;
            ep->me_key = NULL;
            ep->me_value = NULL;
            ep->me_hash = 0;
        }
        Py_DECREF(old_key);
    }
    Py_DECREF(old_value);

    ASSERT_CONSISTENT(mp);
    return 0;
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    assert(key);
    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }

    return _PyDict_DelItem_KnownHash(op, key, hash);
}

int
_PyDict_DelItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix;
    PyDictObject *mp;
    PyObject *old_value;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(hash != -1);
    mp = (PyDictObject *)op;
    ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR)
        return -1;
    if (ix == DKIX_EMPTY || old_value == NULL) {
        _PyErr_SetKeyError(key);
        return -1;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    uint64_t new_version = _PyDict_NotifyEvent(
            interp, PyDict_EVENT_DELETED, mp, key, NULL);
    return delitem_common(mp, hash, ix, old_value, new_version);
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
    PyObject *old_value;
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
    ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR)
        return -1;
    if (ix == DKIX_EMPTY || old_value == NULL) {
        _PyErr_SetKeyError(key);
        return -1;
    }

    res = predicate(old_value);
    if (res == -1)
        return -1;

    hashpos = lookdict_index(mp->ma_keys, hash, ix);
    assert(hashpos >= 0);

    if (res > 0) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        uint64_t new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_DELETED, mp, key, NULL);
        return delitem_common(mp, hashpos, ix, old_value, new_version);
    } else {
        return 0;
    }
}


void
PyDict_Clear(PyObject *op)
{
    PyDictObject *mp;
    PyDictKeysObject *oldkeys;
    PyDictValues *oldvalues;
    Py_ssize_t i, n;

    if (!PyDict_Check(op))
        return;
    mp = ((PyDictObject *)op);
    oldkeys = mp->ma_keys;
    oldvalues = mp->ma_values;
    if (oldkeys == Py_EMPTY_KEYS) {
        return;
    }
    /* Empty the dict... */
    PyInterpreterState *interp = _PyInterpreterState_GET();
    uint64_t new_version = _PyDict_NotifyEvent(
            interp, PyDict_EVENT_CLEARED, mp, NULL, NULL);
    dictkeys_incref(Py_EMPTY_KEYS);
    mp->ma_keys = Py_EMPTY_KEYS;
    mp->ma_values = NULL;
    mp->ma_used = 0;
    mp->ma_version_tag = new_version;
    /* ...then clear the keys and values */
    if (oldvalues != NULL) {
        n = oldkeys->dk_nentries;
        for (i = 0; i < n; i++)
            Py_CLEAR(oldvalues->values[i]);
        free_values(oldvalues);
        dictkeys_decref(interp, oldkeys);
    }
    else {
        assert(oldkeys->dk_refcnt == 1);
        dictkeys_decref(interp, oldkeys);
    }
    ASSERT_CONSISTENT(mp);
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
    Py_ssize_t i;
    PyDictObject *mp;
    PyObject *key, *value;
    Py_hash_t hash;

    if (!PyDict_Check(op))
        return 0;
    mp = (PyDictObject *)op;
    i = *ppos;
    if (mp->ma_values) {
        assert(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
        if (i < 0 || i >= mp->ma_used)
            return 0;
        int index = get_index_from_order(mp, i);
        value = mp->ma_values->values[index];

        key = DK_UNICODE_ENTRIES(mp->ma_keys)[index].me_key;
        hash = unicode_get_hash(key);
        assert(value != NULL);
    }
    else {
        Py_ssize_t n = mp->ma_keys->dk_nentries;
        if (i < 0 || i >= n)
            return 0;
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(mp->ma_keys)[i];
            while (i < n && entry_ptr->me_value == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                return 0;
            key = entry_ptr->me_key;
            hash = unicode_get_hash(entry_ptr->me_key);
            value = entry_ptr->me_value;
        }
        else {
            PyDictKeyEntry *entry_ptr = &DK_ENTRIES(mp->ma_keys)[i];
            while (i < n && entry_ptr->me_value == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                return 0;
            key = entry_ptr->me_key;
            hash = entry_ptr->me_hash;
            value = entry_ptr->me_value;
        }
    }
    *ppos = i+1;
    if (pkey)
        *pkey = key;
    if (pvalue)
        *pvalue = value;
    if (phash)
        *phash = hash;
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
    Py_ssize_t ix;
    PyObject *old_value;
    PyDictObject *mp;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    assert(PyDict_Check(dict));
    mp = (PyDictObject *)dict;

    if (mp->ma_used == 0) {
        if (deflt) {
            return Py_NewRef(deflt);
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || old_value == NULL) {
        if (deflt) {
            return Py_NewRef(deflt);
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    assert(old_value != NULL);
    uint64_t new_version = _PyDict_NotifyEvent(
            interp, PyDict_EVENT_DELETED, mp, key, NULL);
    delitem_common(mp, hash, ix, Py_NewRef(old_value), new_version);

    ASSERT_CONSISTENT(mp);
    return old_value;
}

PyObject *
_PyDict_Pop(PyObject *dict, PyObject *key, PyObject *deflt)
{
    Py_hash_t hash;

    if (((PyDictObject *)dict)->ma_used == 0) {
        if (deflt) {
            return Py_NewRef(deflt);
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
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
    PyInterpreterState *interp = _PyInterpreterState_GET();

    d = _PyObject_CallNoArgs(cls);
    if (d == NULL)
        return NULL;

    if (PyDict_CheckExact(d) && ((PyDictObject *)d)->ma_used == 0) {
        if (PyDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;
            PyObject *oldvalue;
            Py_ssize_t pos = 0;
            PyObject *key;
            Py_hash_t hash;

            int unicode = DK_IS_UNICODE(((PyDictObject*)iterable)->ma_keys);
            if (dictresize(interp, mp,
                           estimate_log2_keysize(PyDict_GET_SIZE(iterable)),
                           unicode)) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PyDict_Next(iterable, &pos, &key, &oldvalue, &hash)) {
                if (insertdict(interp, mp,
                               Py_NewRef(key), hash, Py_NewRef(value))) {
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

            if (dictresize(interp, mp,
                           estimate_log2_keysize(PySet_GET_SIZE(iterable)), 0)) {
                Py_DECREF(d);
                return NULL;
            }

            while (_PySet_NextEntry(iterable, &pos, &key, &hash)) {
                if (insertdict(interp, mp, Py_NewRef(key), hash, Py_NewRef(value))) {
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
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(Py_REFCNT(mp) == 0);
    Py_SET_REFCNT(mp, 1);
    _PyDict_NotifyEvent(interp, PyDict_EVENT_DEALLOCATED, mp, NULL, NULL);
    if (Py_REFCNT(mp) > 1) {
        Py_SET_REFCNT(mp, Py_REFCNT(mp) - 1);
        return;
    }
    Py_SET_REFCNT(mp, 0);
    PyDictValues *values = mp->ma_values;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n;

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(mp);
    Py_TRASHCAN_BEGIN(mp, dict_dealloc)
    if (values != NULL) {
        for (i = 0, n = mp->ma_keys->dk_nentries; i < n; i++) {
            Py_XDECREF(values->values[i]);
        }
        free_values(values);
        dictkeys_decref(interp, keys);
    }
    else if (keys != NULL) {
        assert(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
        dictkeys_decref(interp, keys);
    }
#if PyDict_MAXFREELIST > 0
    struct _Py_dict_state *state = get_dict_state(interp);
#ifdef Py_DEBUG
    // new_dict() must not be called after _PyDict_Fini()
    assert(state->numfree != -1);
#endif
    if (state->numfree < PyDict_MAXFREELIST && Py_IS_TYPE(mp, &PyDict_Type)) {
        state->free_list[state->numfree++] = mp;
        OBJECT_STAT_INC(to_freelist);
    }
    else
#endif
    {
        Py_TYPE(mp)->tp_free((PyObject *)mp);
    }
    Py_TRASHCAN_END
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
    Py_ssize_t ix;
    Py_hash_t hash;
    PyObject *value;

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || value == NULL) {
        if (!PyDict_CheckExact(mp)) {
            /* Look up __missing__ method if we're a subclass. */
            PyObject *missing, *res;
            missing = _PyObject_LookupSpecial(
                    (PyObject *)mp, &_Py_ID(__missing__));
            if (missing != NULL) {
                res = PyObject_CallOneArg(missing, key);
                Py_DECREF(missing);
                return res;
            }
            else if (PyErr_Occurred())
                return NULL;
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    return Py_NewRef(value);
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
    Py_ssize_t n;

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

    /* Nothing we do below makes any function calls. */
    Py_ssize_t j = 0, pos = 0;
    PyObject *key;
    while (_PyDict_Next((PyObject*)mp, &pos, &key, NULL, NULL)) {
        assert(j < n);
        PyList_SET_ITEM(v, j, Py_NewRef(key));
        j++;
    }
    assert(j == n);
    return v;
}

static PyObject *
dict_values(PyDictObject *mp)
{
    PyObject *v;
    Py_ssize_t n;

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

    /* Nothing we do below makes any function calls. */
    Py_ssize_t j = 0, pos = 0;
    PyObject *value;
    while (_PyDict_Next((PyObject*)mp, &pos, NULL, &value, NULL)) {
        assert(j < n);
        PyList_SET_ITEM(v, j, Py_NewRef(value));
        j++;
    }
    assert(j == n);
    return v;
}

static PyObject *
dict_items(PyDictObject *mp)
{
    PyObject *v;
    Py_ssize_t i, n;
    PyObject *item;

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
    Py_ssize_t j = 0, pos = 0;
    PyObject *key, *value;
    while (_PyDict_Next((PyObject*)mp, &pos, &key, &value, NULL)) {
        assert(j < n);
        PyObject *item = PyList_GET_ITEM(v, j);
        PyTuple_SET_ITEM(item, 0, Py_NewRef(key));
        PyTuple_SET_ITEM(item, 1, Py_NewRef(value));
        j++;
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

Create a new dictionary with keys from iterable and values set to value.
[clinic start generated code]*/

static PyObject *
dict_fromkeys_impl(PyTypeObject *type, PyObject *iterable, PyObject *value)
/*[clinic end generated code: output=8fb98e4b10384999 input=382ba4855d0f74c3]*/
{
    return _PyDict_FromKeys((PyObject *)type, iterable, value);
}

/* Single-arg dict update; used by dict_update_common and operators. */
static int
dict_update_arg(PyObject *self, PyObject *arg)
{
    if (PyDict_CheckExact(arg)) {
        return PyDict_Merge(self, arg, 1);
    }
    PyObject *func;
    if (_PyObject_LookupAttr(arg, &_Py_ID(keys), &func) < 0) {
        return -1;
    }
    if (func != NULL) {
        Py_DECREF(func);
        return PyDict_Merge(self, arg, 1);
    }
    return PyDict_MergeFromSeq2(self, arg, 1);
}

static int
dict_update_common(PyObject *self, PyObject *args, PyObject *kwds,
                   const char *methname)
{
    PyObject *arg = NULL;
    int result = 0;

    if (!PyArg_UnpackTuple(args, methname, 0, 1, &arg)) {
        result = -1;
    }
    else if (arg != NULL) {
        result = dict_update_arg(self, arg);
    }

    if (result == 0 && kwds != NULL) {
        if (PyArg_ValidateKeywordArguments(kwds))
            result = PyDict_Merge(self, kwds, 1);
        else
            result = -1;
    }
    return result;
}

/* Note: dict.update() uses the METH_VARARGS|METH_KEYWORDS calling convention.
   Using METH_FASTCALL|METH_KEYWORDS would make dict.update(**dict2) calls
   slower, see the issue #29312. */
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
        if (override) {
            if (PyDict_SetItem(d, key, value) < 0) {
                Py_DECREF(key);
                Py_DECREF(value);
                goto Fail;
            }
        }
        else {
            if (PyDict_SetDefault(d, key, value) == NULL) {
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
    ASSERT_CONSISTENT(d);
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
dict_merge(PyInterpreterState *interp, PyObject *a, PyObject *b, int override)
{
    PyDictObject *mp, *other;

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
    if (PyDict_Check(b) && (Py_TYPE(b)->tp_iter == (getiterfunc)dict_iter)) {
        other = (PyDictObject*)b;
        if (other == mp || other->ma_used == 0)
            /* a.update(a) or a.update({}); nothing to do */
            return 0;
        if (mp->ma_used == 0) {
            /* Since the target dict is empty, PyDict_GetItem()
             * always returns NULL.  Setting override to 1
             * skips the unnecessary test.
             */
            override = 1;
            PyDictKeysObject *okeys = other->ma_keys;

            // If other is clean, combined, and just allocated, just clone it.
            if (other->ma_values == NULL &&
                    other->ma_used == okeys->dk_nentries &&
                    (DK_LOG_SIZE(okeys) == PyDict_LOG_MINSIZE ||
                     USABLE_FRACTION(DK_SIZE(okeys)/2) < other->ma_used)) {
                uint64_t new_version = _PyDict_NotifyEvent(
                        interp, PyDict_EVENT_CLONED, mp, b, NULL);
                PyDictKeysObject *keys = clone_combined_dict_keys(other);
                if (keys == NULL) {
                    return -1;
                }

                dictkeys_decref(interp, mp->ma_keys);
                mp->ma_keys = keys;
                if (mp->ma_values != NULL) {
                    free_values(mp->ma_values);
                    mp->ma_values = NULL;
                }

                mp->ma_used = other->ma_used;
                mp->ma_version_tag = new_version;
                ASSERT_CONSISTENT(mp);

                if (_PyObject_GC_IS_TRACKED(other) && !_PyObject_GC_IS_TRACKED(mp)) {
                    /* Maintain tracking. */
                    _PyObject_GC_TRACK(mp);
                }

                return 0;
            }
        }
        /* Do one big resize at the start, rather than
         * incrementally resizing as we insert new items.  Expect
         * that there will be no (or few) overlapping keys.
         */
        if (USABLE_FRACTION(DK_SIZE(mp->ma_keys)) < other->ma_used) {
            int unicode = DK_IS_UNICODE(other->ma_keys);
            if (dictresize(interp, mp,
                           estimate_log2_keysize(mp->ma_used + other->ma_used),
                           unicode)) {
               return -1;
            }
        }

        Py_ssize_t orig_size = other->ma_keys->dk_nentries;
        Py_ssize_t pos = 0;
        Py_hash_t hash;
        PyObject *key, *value;

        while (_PyDict_Next((PyObject*)other, &pos, &key, &value, &hash)) {
            int err = 0;
            Py_INCREF(key);
            Py_INCREF(value);
            if (override == 1) {
                err = insertdict(interp, mp,
                                 Py_NewRef(key), hash, Py_NewRef(value));
            }
            else {
                err = _PyDict_Contains_KnownHash(a, key, hash);
                if (err == 0) {
                    err = insertdict(interp, mp,
                                     Py_NewRef(key), hash, Py_NewRef(value));
                }
                else if (err > 0) {
                    if (override != 0) {
                        _PyErr_SetKeyError(key);
                        Py_DECREF(value);
                        Py_DECREF(key);
                        return -1;
                    }
                    err = 0;
                }
            }
            Py_DECREF(value);
            Py_DECREF(key);
            if (err != 0)
                return -1;

            if (orig_size != other->ma_keys->dk_nentries) {
                PyErr_SetString(PyExc_RuntimeError,
                        "dict mutated during update");
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
            if (override != 1) {
                status = PyDict_Contains(a, key);
                if (status != 0) {
                    if (status > 0) {
                        if (override == 0) {
                            Py_DECREF(key);
                            continue;
                        }
                        _PyErr_SetKeyError(key);
                    }
                    Py_DECREF(key);
                    Py_DECREF(iter);
                    return -1;
                }
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
    ASSERT_CONSISTENT(a);
    return 0;
}

int
PyDict_Update(PyObject *a, PyObject *b)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return dict_merge(interp, a, b, 1);
}

int
PyDict_Merge(PyObject *a, PyObject *b, int override)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    /* XXX Deprecate override not in (0, 1). */
    return dict_merge(interp, a, b, override != 0);
}

int
_PyDict_MergeEx(PyObject *a, PyObject *b, int override)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return dict_merge(interp, a, b, override);
}

static PyObject *
dict_copy(PyDictObject *mp, PyObject *Py_UNUSED(ignored))
{
    return PyDict_Copy((PyObject*)mp);
}

PyObject *
PyDict_Copy(PyObject *o)
{
    PyObject *copy;
    PyDictObject *mp;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (o == NULL || !PyDict_Check(o)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    mp = (PyDictObject *)o;
    if (mp->ma_used == 0) {
        /* The dict is empty; just return a new dict. */
        return PyDict_New();
    }

    if (_PyDict_HasSplitTable(mp)) {
        PyDictObject *split_copy;
        size_t size = shared_keys_usable_size(mp->ma_keys);
        PyDictValues *newvalues = new_values(size);
        if (newvalues == NULL)
            return PyErr_NoMemory();
        split_copy = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (split_copy == NULL) {
            free_values(newvalues);
            return NULL;
        }
        size_t prefix_size = ((uint8_t *)newvalues)[-1];
        memcpy(((char *)newvalues)-prefix_size, ((char *)mp->ma_values)-prefix_size, prefix_size-1);
        split_copy->ma_values = newvalues;
        split_copy->ma_keys = mp->ma_keys;
        split_copy->ma_used = mp->ma_used;
        split_copy->ma_version_tag = DICT_NEXT_VERSION(interp);
        dictkeys_incref(mp->ma_keys);
        for (size_t i = 0; i < size; i++) {
            PyObject *value = mp->ma_values->values[i];
            split_copy->ma_values->values[i] = Py_XNewRef(value);
        }
        if (_PyObject_GC_IS_TRACKED(mp))
            _PyObject_GC_TRACK(split_copy);
        return (PyObject *)split_copy;
    }

    if (Py_TYPE(mp)->tp_iter == (getiterfunc)dict_iter &&
            mp->ma_values == NULL &&
            (mp->ma_used >= (mp->ma_keys->dk_nentries * 2) / 3))
    {
        /* Use fast-copy if:

           (1) type(mp) doesn't override tp_iter; and

           (2) 'mp' is not a split-dict; and

           (3) if 'mp' is non-compact ('del' operation does not resize dicts),
               do fast-copy only if it has at most 1/3 non-used keys.

           The last condition (3) is important to guard against a pathological
           case when a large dict is almost emptied with multiple del/pop
           operations and copied after that.  In cases like this, we defer to
           PyDict_Merge, which produces a compacted copy.
        */
        PyDictKeysObject *keys = clone_combined_dict_keys(mp);
        if (keys == NULL) {
            return NULL;
        }
        PyDictObject *new = (PyDictObject *)new_dict(interp, keys, NULL, 0, 0);
        if (new == NULL) {
            /* In case of an error, `new_dict()` takes care of
               cleaning up `keys`. */
            return NULL;
        }

        new->ma_used = mp->ma_used;
        ASSERT_CONSISTENT(new);
        if (_PyObject_GC_IS_TRACKED(mp)) {
            /* Maintain tracking. */
            _PyObject_GC_TRACK(new);
        }

        return (PyObject *)new;
    }

    copy = PyDict_New();
    if (copy == NULL)
        return NULL;
    if (dict_merge(interp, copy, o, 1) == 0)
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
        PyObject *key, *aval;
        Py_hash_t hash;
        if (DK_IS_UNICODE(a->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(a->ma_keys)[i];
            key = ep->me_key;
            if (key == NULL) {
                continue;
            }
            hash = unicode_get_hash(key);
            if (a->ma_values)
                aval = a->ma_values->values[i];
            else
                aval = ep->me_value;
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(a->ma_keys)[i];
            key = ep->me_key;
            aval = ep->me_value;
            hash = ep->me_hash;
        }
        if (aval != NULL) {
            int cmp;
            PyObject *bval;
            /* temporarily bump aval's refcount to ensure it stays
               alive until we're done with it */
            Py_INCREF(aval);
            /* ditto for key */
            Py_INCREF(key);
            /* reuse the known hash value */
            _Py_dict_lookup(b, key, hash, &bval);
            if (bval == NULL) {
                Py_DECREF(key);
                Py_DECREF(aval);
                if (PyErr_Occurred())
                    return -1;
                return 0;
            }
            Py_INCREF(bval);
            cmp = PyObject_RichCompareBool(aval, bval, Py_EQ);
            Py_DECREF(key);
            Py_DECREF(aval);
            Py_DECREF(bval);
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
    return Py_NewRef(res);
}

/*[clinic input]

@coexist
dict.__contains__

  key: object
  /

True if the dictionary has the specified key, else False.
[clinic start generated code]*/

static PyObject *
dict___contains__(PyDictObject *self, PyObject *key)
/*[clinic end generated code: output=a3d03db709ed6e6b input=fe1cb42ad831e820]*/
{
    register PyDictObject *mp = self;
    Py_hash_t hash;
    Py_ssize_t ix;
    PyObject *value;

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || value == NULL)
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;
}

/*[clinic input]
dict.get

    key: object
    default: object = None
    /

Return the value for key if key is in the dictionary, else default.
[clinic start generated code]*/

static PyObject *
dict_get_impl(PyDictObject *self, PyObject *key, PyObject *default_value)
/*[clinic end generated code: output=bba707729dee05bf input=279ddb5790b6b107]*/
{
    PyObject *val = NULL;
    Py_hash_t hash;
    Py_ssize_t ix;

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }
    ix = _Py_dict_lookup(self, key, hash, &val);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || val == NULL) {
        val = default_value;
    }
    return Py_NewRef(val);
}

PyObject *
PyDict_SetDefault(PyObject *d, PyObject *key, PyObject *defaultobj)
{
    PyDictObject *mp = (PyDictObject *)d;
    PyObject *value;
    Py_hash_t hash;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (!PyDict_Check(d)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return NULL;
    }

    if (mp->ma_keys == Py_EMPTY_KEYS) {
        if (insert_to_emptydict(interp, mp, Py_NewRef(key), hash,
                                Py_NewRef(defaultobj)) < 0) {
            return NULL;
        }
        return defaultobj;
    }

    if (!PyUnicode_CheckExact(key) && DK_IS_UNICODE(mp->ma_keys)) {
        if (insertion_resize(interp, mp, 0) < 0) {
            return NULL;
        }
    }

    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR)
        return NULL;

    if (ix == DKIX_EMPTY) {
        value = defaultobj;
        if (mp->ma_keys->dk_usable <= 0) {
            if (insertion_resize(interp, mp, 1) < 0) {
                return NULL;
            }
        }
        uint64_t new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_ADDED, mp, key, defaultobj);
        mp->ma_keys->dk_version = 0;
        Py_ssize_t hashpos = find_empty_slot(mp->ma_keys, hash);
        dictkeys_set_index(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);
        if (DK_IS_UNICODE(mp->ma_keys)) {
            assert(PyUnicode_CheckExact(key));
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
            ep->me_key = Py_NewRef(key);
            if (_PyDict_HasSplitTable(mp)) {
                Py_ssize_t index = (int)mp->ma_keys->dk_nentries;
                assert(index < SHARED_KEYS_MAX_SIZE);
                assert(mp->ma_values->values[index] == NULL);
                mp->ma_values->values[index] = Py_NewRef(value);
                _PyDictValues_AddToInsertionOrder(mp->ma_values, index);
            }
            else {
                ep->me_value = Py_NewRef(value);
            }
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
            ep->me_key = Py_NewRef(key);
            ep->me_hash = hash;
            ep->me_value = Py_NewRef(value);
        }
        MAINTAIN_TRACKING(mp, key, value);
        mp->ma_used++;
        mp->ma_version_tag = new_version;
        mp->ma_keys->dk_usable--;
        mp->ma_keys->dk_nentries++;
        assert(mp->ma_keys->dk_usable >= 0);
    }
    else if (value == NULL) {
        uint64_t new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_ADDED, mp, key, defaultobj);
        value = defaultobj;
        assert(_PyDict_HasSplitTable(mp));
        assert(mp->ma_values->values[ix] == NULL);
        MAINTAIN_TRACKING(mp, key, value);
        mp->ma_values->values[ix] = Py_NewRef(value);
        _PyDictValues_AddToInsertionOrder(mp->ma_values, ix);
        mp->ma_used++;
        mp->ma_version_tag = new_version;
    }

    ASSERT_CONSISTENT(mp);
    return value;
}

/*[clinic input]
dict.setdefault

    key: object
    default: object = None
    /

Insert key with a value of default if key is not in the dictionary.

Return the value for key if key is in the dictionary, else default.
[clinic start generated code]*/

static PyObject *
dict_setdefault_impl(PyDictObject *self, PyObject *key,
                     PyObject *default_value)
/*[clinic end generated code: output=f8c1101ebf69e220 input=0f063756e815fd9d]*/
{
    PyObject *val;

    val = PyDict_SetDefault((PyObject *)self, key, default_value);
    return Py_XNewRef(val);
}

static PyObject *
dict_clear(PyDictObject *mp, PyObject *Py_UNUSED(ignored))
{
    PyDict_Clear((PyObject *)mp);
    Py_RETURN_NONE;
}

/*[clinic input]
dict.pop

    key: object
    default: object = NULL
    /

D.pop(k[,d]) -> v, remove specified key and return the corresponding value.

If the key is not found, return the default if given; otherwise,
raise a KeyError.
[clinic start generated code]*/

static PyObject *
dict_pop_impl(PyDictObject *self, PyObject *key, PyObject *default_value)
/*[clinic end generated code: output=3abb47b89f24c21c input=e221baa01044c44c]*/
{
    return _PyDict_Pop((PyObject*)self, key, default_value);
}

/*[clinic input]
dict.popitem

Remove and return a (key, value) pair as a 2-tuple.

Pairs are returned in LIFO (last-in, first-out) order.
Raises KeyError if the dict is empty.
[clinic start generated code]*/

static PyObject *
dict_popitem_impl(PyDictObject *self)
/*[clinic end generated code: output=e65fcb04420d230d input=1c38a49f21f64941]*/
{
    Py_ssize_t i, j;
    PyObject *res;
    uint64_t new_version;
    PyInterpreterState *interp = _PyInterpreterState_GET();

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
    if (self->ma_used == 0) {
        Py_DECREF(res);
        PyErr_SetString(PyExc_KeyError, "popitem(): dictionary is empty");
        return NULL;
    }
    /* Convert split table to combined table */
    if (self->ma_keys->dk_kind == DICT_KEYS_SPLIT) {
        if (dictresize(interp, self, DK_LOG_SIZE(self->ma_keys), 1)) {
            Py_DECREF(res);
            return NULL;
        }
    }
    self->ma_keys->dk_version = 0;

    /* Pop last item */
    PyObject *key, *value;
    Py_hash_t hash;
    if (DK_IS_UNICODE(self->ma_keys)) {
        PyDictUnicodeEntry *ep0 = DK_UNICODE_ENTRIES(self->ma_keys);
        i = self->ma_keys->dk_nentries - 1;
        while (i >= 0 && ep0[i].me_value == NULL) {
            i--;
        }
        assert(i >= 0);

        key = ep0[i].me_key;
        new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_DELETED, self, key, NULL);
        hash = unicode_get_hash(key);
        value = ep0[i].me_value;
        ep0[i].me_key = NULL;
        ep0[i].me_value = NULL;
    }
    else {
        PyDictKeyEntry *ep0 = DK_ENTRIES(self->ma_keys);
        i = self->ma_keys->dk_nentries - 1;
        while (i >= 0 && ep0[i].me_value == NULL) {
            i--;
        }
        assert(i >= 0);

        key = ep0[i].me_key;
        new_version = _PyDict_NotifyEvent(
                interp, PyDict_EVENT_DELETED, self, key, NULL);
        hash = ep0[i].me_hash;
        value = ep0[i].me_value;
        ep0[i].me_key = NULL;
        ep0[i].me_hash = -1;
        ep0[i].me_value = NULL;
    }

    j = lookdict_index(self->ma_keys, hash, i);
    assert(j >= 0);
    assert(dictkeys_get_index(self->ma_keys, j) == i);
    dictkeys_set_index(self->ma_keys, j, DKIX_DUMMY);

    PyTuple_SET_ITEM(res, 0, key);
    PyTuple_SET_ITEM(res, 1, value);
    /* We can't dk_usable++ since there is DKIX_DUMMY in indices */
    self->ma_keys->dk_nentries = i;
    self->ma_used--;
    self->ma_version_tag = new_version;
    ASSERT_CONSISTENT(self);
    return res;
}

static int
dict_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n = keys->dk_nentries;

    if (DK_IS_UNICODE(keys)) {
        if (mp->ma_values != NULL) {
            for (i = 0; i < n; i++) {
                Py_VISIT(mp->ma_values->values[i]);
            }
        }
        else {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(keys);
            for (i = 0; i < n; i++) {
                Py_VISIT(entries[i].me_value);
            }
        }
    }
    else {
        PyDictKeyEntry *entries = DK_ENTRIES(keys);
        for (i = 0; i < n; i++) {
            if (entries[i].me_value != NULL) {
                Py_VISIT(entries[i].me_value);
                Py_VISIT(entries[i].me_key);
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
    size_t res = _PyObject_SIZE(Py_TYPE(mp));
    if (mp->ma_values) {
        res += shared_keys_usable_size(mp->ma_keys) * sizeof(PyObject*);
    }
    /* If the dictionary is split, the keys portion is accounted-for
       in the type object. */
    if (mp->ma_keys->dk_refcnt == 1) {
        res += _PyDict_KeysSize(mp->ma_keys);
    }
    assert(res <= (size_t)PY_SSIZE_T_MAX);
    return (Py_ssize_t)res;
}

size_t
_PyDict_KeysSize(PyDictKeysObject *keys)
{
    size_t es = (keys->dk_kind == DICT_KEYS_GENERAL
                 ? sizeof(PyDictKeyEntry) : sizeof(PyDictUnicodeEntry));
    size_t size = sizeof(PyDictKeysObject);
    size += (size_t)1 << keys->dk_log2_index_bytes;
    size += USABLE_FRACTION((size_t)DK_SIZE(keys)) * es;
    return size;
}

static PyObject *
dict_sizeof(PyDictObject *mp, PyObject *Py_UNUSED(ignored))
{
    return PyLong_FromSsize_t(_PyDict_SizeOf(mp));
}

static PyObject *
dict_or(PyObject *self, PyObject *other)
{
    if (!PyDict_Check(self) || !PyDict_Check(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyObject *new = PyDict_Copy(self);
    if (new == NULL) {
        return NULL;
    }
    if (dict_update_arg(new, other)) {
        Py_DECREF(new);
        return NULL;
    }
    return new;
}

static PyObject *
dict_ior(PyObject *self, PyObject *other)
{
    if (dict_update_arg(self, other)) {
        return NULL;
    }
    return Py_NewRef(self);
}

PyDoc_STRVAR(getitem__doc__,
"__getitem__($self, key, /)\n--\n\nReturn self[key].");

PyDoc_STRVAR(sizeof__doc__,
"D.__sizeof__() -> size of D in memory, in bytes");

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
static PyObject *dictkeys_new(PyObject *, PyObject *);
static PyObject *dictitems_new(PyObject *, PyObject *);
static PyObject *dictvalues_new(PyObject *, PyObject *);

PyDoc_STRVAR(keys__doc__,
             "D.keys() -> a set-like object providing a view on D's keys");
PyDoc_STRVAR(items__doc__,
             "D.items() -> a set-like object providing a view on D's items");
PyDoc_STRVAR(values__doc__,
             "D.values() -> an object providing a view on D's values");

static PyMethodDef mapp_methods[] = {
    DICT___CONTAINS___METHODDEF
    {"__getitem__", _PyCFunction_CAST(dict_subscript),        METH_O | METH_COEXIST,
     getitem__doc__},
    {"__sizeof__",      _PyCFunction_CAST(dict_sizeof),       METH_NOARGS,
     sizeof__doc__},
    DICT_GET_METHODDEF
    DICT_SETDEFAULT_METHODDEF
    DICT_POP_METHODDEF
    DICT_POPITEM_METHODDEF
    {"keys",            dictkeys_new,                   METH_NOARGS,
    keys__doc__},
    {"items",           dictitems_new,                  METH_NOARGS,
    items__doc__},
    {"values",          dictvalues_new,                 METH_NOARGS,
    values__doc__},
    {"update",          _PyCFunction_CAST(dict_update), METH_VARARGS | METH_KEYWORDS,
     update__doc__},
    DICT_FROMKEYS_METHODDEF
    {"clear",           (PyCFunction)dict_clear,        METH_NOARGS,
     clear__doc__},
    {"copy",            (PyCFunction)dict_copy,         METH_NOARGS,
     copy__doc__},
    DICT___REVERSED___METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL,              NULL}   /* sentinel */
};

/* Return 1 if `key` is in dict `op`, 0 if not, and -1 on error. */
int
PyDict_Contains(PyObject *op, PyObject *key)
{
    Py_hash_t hash;
    Py_ssize_t ix;
    PyDictObject *mp = (PyDictObject *)op;
    PyObject *value;

    if (!PyUnicode_CheckExact(key) || (hash = unicode_get_hash(key)) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }
    ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR)
        return -1;
    return (ix != DKIX_EMPTY && value != NULL);
}

/* Internal version of PyDict_Contains used when the hash value is already known */
int
_PyDict_Contains_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyObject *value;
    Py_ssize_t ix;

    ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR)
        return -1;
    return (ix != DKIX_EMPTY && value != NULL);
}

int
_PyDict_ContainsId(PyObject *op, _Py_Identifier *key)
{
    PyObject *kv = _PyUnicode_FromId(key); /* borrowed */
    if (kv == NULL) {
        return -1;
    }
    return PyDict_Contains(op, kv);
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

static PyNumberMethods dict_as_number = {
    .nb_or = dict_or,
    .nb_inplace_or = dict_ior,
};

static PyObject *
dict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    assert(type != NULL);
    assert(type->tp_alloc != NULL);
    // dict subclasses must implement the GC protocol
    assert(_PyType_IS_GC(type));

    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    PyDictObject *d = (PyDictObject *)self;

    d->ma_used = 0;
    d->ma_version_tag = DICT_NEXT_VERSION(
            _PyInterpreterState_GET());
    dictkeys_incref(Py_EMPTY_KEYS);
    d->ma_keys = Py_EMPTY_KEYS;
    d->ma_values = NULL;
    ASSERT_CONSISTENT(d);

    if (type != &PyDict_Type) {
        // Don't track if a subclass tp_alloc is PyType_GenericAlloc()
        if (!_PyObject_GC_IS_TRACKED(d)) {
            _PyObject_GC_TRACK(d);
        }
    }
    else {
        // _PyType_AllocNoTrack() does not track the created object
        assert(!_PyObject_GC_IS_TRACKED(d));
    }
    return self;
}

static int
dict_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    return dict_update_common(self, args, kwds, "dict");
}

static PyObject *
dict_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("dict", nargs, 0, 1)) {
        return NULL;
    }

    PyObject *self = dict_new(_PyType_CAST(type), NULL, NULL);
    if (self == NULL) {
        return NULL;
    }
    if (nargs == 1) {
        if (dict_update_arg(self, args[0]) < 0) {
            Py_DECREF(self);
            return NULL;
        }
        args++;
    }
    if (kwnames != NULL) {
        for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(kwnames); i++) {
            if (PyDict_SetItem(self, PyTuple_GET_ITEM(kwnames, i), args[i]) < 0) {
                Py_DECREF(self);
                return NULL;
            }
        }
    }
    return self;
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
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)dict_repr,                        /* tp_repr */
    &dict_as_number,                            /* tp_as_number */
    &dict_as_sequence,                          /* tp_as_sequence */
    &dict_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_DICT_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_MAPPING,  /* tp_flags */
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
    _PyType_AllocNoTrack,                       /* tp_alloc */
    dict_new,                                   /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_vectorcall = dict_vectorcall,
};

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
_PyDict_SetItemId(PyObject *v, _Py_Identifier *key, PyObject *item)
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
    if (di == NULL) {
        return NULL;
    }
    di->di_dict = (PyDictObject*)Py_NewRef(dict);
    di->di_used = dict->ma_used;
    di->len = dict->ma_used;
    if (itertype == &PyDictRevIterKey_Type ||
         itertype == &PyDictRevIterItem_Type ||
         itertype == &PyDictRevIterValue_Type) {
        if (dict->ma_values) {
            di->di_pos = dict->ma_used - 1;
        }
        else {
            di->di_pos = dict->ma_keys->dk_nentries - 1;
        }
    }
    else {
        di->di_pos = 0;
    }
    if (itertype == &PyDictIterItem_Type ||
        itertype == &PyDictRevIterItem_Type) {
        di->di_result = PyTuple_Pack(2, Py_None, Py_None);
        if (di->di_result == NULL) {
            Py_DECREF(di);
            return NULL;
        }
    }
    else {
        di->di_result = NULL;
    }
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
dictiter_len(dictiterobject *di, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t len = 0;
    if (di->di_dict != NULL && di->di_used == di->di_dict->ma_used)
        len = di->len;
    return PyLong_FromSize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyObject *
dictiter_reduce(dictiterobject *di, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef dictiter_methods[] = {
    {"__length_hint__", _PyCFunction_CAST(dictiter_len), METH_NOARGS,
     length_hint_doc},
     {"__reduce__", _PyCFunction_CAST(dictiter_reduce), METH_NOARGS,
     reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

static PyObject*
dictiter_iternextkey(dictiterobject *di)
{
    PyObject *key;
    Py_ssize_t i;
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
    assert(i >= 0);
    if (d->ma_values) {
        if (i >= d->ma_used)
            goto fail;
        int index = get_index_from_order(d, i);
        key = DK_UNICODE_ENTRIES(k)[index].me_key;
        assert(d->ma_values->values[index] != NULL);
    }
    else {
        Py_ssize_t n = k->dk_nentries;
        if (DK_IS_UNICODE(k)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(k)[i];
            while (i < n && entry_ptr->me_value == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                goto fail;
            key = entry_ptr->me_key;
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
    }
    // We found an element (key), but did not expect it
    if (di->len == 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary keys changed during iteration");
        goto fail;
    }
    di->di_pos = i+1;
    di->len--;
    return Py_NewRef(key);

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
    Py_ssize_t i;
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
    assert(i >= 0);
    if (d->ma_values) {
        if (i >= d->ma_used)
            goto fail;
        int index = get_index_from_order(d, i);
        value = d->ma_values->values[index];
        assert(value != NULL);
    }
    else {
        Py_ssize_t n = d->ma_keys->dk_nentries;
        if (DK_IS_UNICODE(d->ma_keys)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(d->ma_keys)[i];
            while (i < n && entry_ptr->me_value == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                goto fail;
            value = entry_ptr->me_value;
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
    }
    // We found an element, but did not expect it
    if (di->len == 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary keys changed during iteration");
        goto fail;
    }
    di->di_pos = i+1;
    di->len--;
    return Py_NewRef(value);

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
    Py_ssize_t i;
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
    assert(i >= 0);
    if (d->ma_values) {
        if (i >= d->ma_used)
            goto fail;
        int index = get_index_from_order(d, i);
        key = DK_UNICODE_ENTRIES(d->ma_keys)[index].me_key;
        value = d->ma_values->values[index];
        assert(value != NULL);
    }
    else {
        Py_ssize_t n = d->ma_keys->dk_nentries;
        if (DK_IS_UNICODE(d->ma_keys)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(d->ma_keys)[i];
            while (i < n && entry_ptr->me_value == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                goto fail;
            key = entry_ptr->me_key;
            value = entry_ptr->me_value;
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
    }
    // We found an element, but did not expect it
    if (di->len == 0) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary keys changed during iteration");
        goto fail;
    }
    di->di_pos = i+1;
    di->len--;
    result = di->di_result;
    if (Py_REFCNT(result) == 1) {
        PyObject *oldkey = PyTuple_GET_ITEM(result, 0);
        PyObject *oldvalue = PyTuple_GET_ITEM(result, 1);
        PyTuple_SET_ITEM(result, 0, Py_NewRef(key));
        PyTuple_SET_ITEM(result, 1, Py_NewRef(value));
        Py_INCREF(result);
        Py_DECREF(oldkey);
        Py_DECREF(oldvalue);
        // bpo-42536: The GC may have untracked this result tuple. Since we're
        // recycling it, make sure it's tracked again:
        if (!_PyObject_GC_IS_TRACKED(result)) {
            _PyObject_GC_TRACK(result);
        }
    }
    else {
        result = PyTuple_New(2);
        if (result == NULL)
            return NULL;
        PyTuple_SET_ITEM(result, 0, Py_NewRef(key));
        PyTuple_SET_ITEM(result, 1, Py_NewRef(value));
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


/* dictreviter */

static PyObject *
dictreviter_iternext(dictiterobject *di)
{
    PyDictObject *d = di->di_dict;

    if (d == NULL) {
        return NULL;
    }
    assert (PyDict_Check(d));

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                         "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    Py_ssize_t i = di->di_pos;
    PyDictKeysObject *k = d->ma_keys;
    PyObject *key, *value, *result;

    if (i < 0) {
        goto fail;
    }
    if (d->ma_values) {
        int index = get_index_from_order(d, i);
        key = DK_UNICODE_ENTRIES(k)[index].me_key;
        value = d->ma_values->values[index];
        assert (value != NULL);
    }
    else {
        if (DK_IS_UNICODE(k)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(k)[i];
            while (entry_ptr->me_value == NULL) {
                if (--i < 0) {
                    goto fail;
                }
                entry_ptr--;
            }
            key = entry_ptr->me_key;
            value = entry_ptr->me_value;
        }
        else {
            PyDictKeyEntry *entry_ptr = &DK_ENTRIES(k)[i];
            while (entry_ptr->me_value == NULL) {
                if (--i < 0) {
                    goto fail;
                }
                entry_ptr--;
            }
            key = entry_ptr->me_key;
            value = entry_ptr->me_value;
        }
    }
    di->di_pos = i-1;
    di->len--;

    if (Py_IS_TYPE(di, &PyDictRevIterKey_Type)) {
        return Py_NewRef(key);
    }
    else if (Py_IS_TYPE(di, &PyDictRevIterValue_Type)) {
        return Py_NewRef(value);
    }
    else if (Py_IS_TYPE(di, &PyDictRevIterItem_Type)) {
        result = di->di_result;
        if (Py_REFCNT(result) == 1) {
            PyObject *oldkey = PyTuple_GET_ITEM(result, 0);
            PyObject *oldvalue = PyTuple_GET_ITEM(result, 1);
            PyTuple_SET_ITEM(result, 0, Py_NewRef(key));
            PyTuple_SET_ITEM(result, 1, Py_NewRef(value));
            Py_INCREF(result);
            Py_DECREF(oldkey);
            Py_DECREF(oldvalue);
            // bpo-42536: The GC may have untracked this result tuple. Since
            // we're recycling it, make sure it's tracked again:
            if (!_PyObject_GC_IS_TRACKED(result)) {
                _PyObject_GC_TRACK(result);
            }
        }
        else {
            result = PyTuple_New(2);
            if (result == NULL) {
                return NULL;
            }
            PyTuple_SET_ITEM(result, 0, Py_NewRef(key));
            PyTuple_SET_ITEM(result, 1, Py_NewRef(value));
        }
        return result;
    }
    else {
        Py_UNREACHABLE();
    }

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
    return NULL;
}

PyTypeObject PyDictRevIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversekeyiterator",
    sizeof(dictiterobject),
    .tp_dealloc = (destructor)dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)dictreviter_iternext,
    .tp_methods = dictiter_methods
};


/*[clinic input]
dict.__reversed__

Return a reverse iterator over the dict keys.
[clinic start generated code]*/

static PyObject *
dict___reversed___impl(PyDictObject *self)
/*[clinic end generated code: output=e674483336d1ed51 input=23210ef3477d8c4d]*/
{
    assert (PyDict_Check(self));
    return dictiter_new(self, &PyDictRevIterKey_Type);
}

static PyObject *
dictiter_reduce(dictiterobject *di, PyObject *Py_UNUSED(ignored))
{
    /* copy the iterator state */
    dictiterobject tmp = *di;
    Py_XINCREF(tmp.di_dict);
    PyObject *list = PySequence_List((PyObject*)&tmp);
    Py_XDECREF(tmp.di_dict);
    if (list == NULL) {
        return NULL;
    }
    return Py_BuildValue("N(N)", _PyEval_GetBuiltin(&_Py_ID(iter)), list);
}

PyTypeObject PyDictRevIterItem_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reverseitemiterator",
    sizeof(dictiterobject),
    .tp_dealloc = (destructor)dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)dictreviter_iternext,
    .tp_methods = dictiter_methods
};

PyTypeObject PyDictRevIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversevalueiterator",
    sizeof(dictiterobject),
    .tp_dealloc = (destructor)dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = (traverseproc)dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = (iternextfunc)dictreviter_iternext,
    .tp_methods = dictiter_methods
};

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
                     type->tp_name, Py_TYPE(dict)->tp_name);
        return NULL;
    }
    dv = PyObject_GC_New(_PyDictViewObject, type);
    if (dv == NULL)
        return NULL;
    dv->dv_dict = (PyDictObject *)Py_NewRef(dict);
    _PyObject_GC_TRACK(dv);
    return (PyObject *)dv;
}

static PyObject *
dictview_mapping(PyObject *view, void *Py_UNUSED(ignored)) {
    assert(view != NULL);
    assert(PyDictKeys_Check(view)
           || PyDictValues_Check(view)
           || PyDictItems_Check(view));
    PyObject *mapping = (PyObject *)((_PyDictViewObject *)view)->dv_dict;
    return PyDictProxy_New(mapping);
}

static PyGetSetDef dictview_getset[] = {
    {"mapping", dictview_mapping, (setter)NULL,
     "dictionary that this view refers to", NULL},
    {0}
};

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
    return Py_NewRef(result);
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

// Create a set object from dictviews object.
// Returns a new reference.
// This utility function is used by set operations.
static PyObject*
dictviews_to_set(PyObject *self)
{
    PyObject *left = self;
    if (PyDictKeys_Check(self)) {
        // PySet_New() has fast path for the dict object.
        PyObject *dict = (PyObject *)((_PyDictViewObject *)self)->dv_dict;
        if (PyDict_CheckExact(dict)) {
            left = dict;
        }
    }
    return PySet_New(left);
}

static PyObject*
dictviews_sub(PyObject *self, PyObject *other)
{
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    PyObject *tmp = PyObject_CallMethodOneArg(
            result, &_Py_ID(difference_update), other);
    if (tmp == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    Py_DECREF(tmp);
    return result;
}

static int
dictitems_contains(_PyDictViewObject *dv, PyObject *obj);

PyObject *
_PyDictView_Intersect(PyObject* self, PyObject *other)
{
    PyObject *result;
    PyObject *it;
    PyObject *key;
    Py_ssize_t len_self;
    int rv;
    int (*dict_contains)(_PyDictViewObject *, PyObject *);

    /* Python interpreter swaps parameters when dict view
       is on right side of & */
    if (!PyDictViewSet_Check(self)) {
        PyObject *tmp = other;
        other = self;
        self = tmp;
    }

    len_self = dictview_len((_PyDictViewObject *)self);

    /* if other is a set and self is smaller than other,
       reuse set intersection logic */
    if (PySet_CheckExact(other) && len_self <= PyObject_Size(other)) {
        return PyObject_CallMethodObjArgs(
                other, &_Py_ID(intersection), self, NULL);
    }

    /* if other is another dict view, and it is bigger than self,
       swap them */
    if (PyDictViewSet_Check(other)) {
        Py_ssize_t len_other = dictview_len((_PyDictViewObject *)other);
        if (len_other > len_self) {
            PyObject *tmp = other;
            other = self;
            self = tmp;
        }
    }

    /* at this point, two things should be true
       1. self is a dictview
       2. if other is a dictview then it is smaller than self */
    result = PySet_New(NULL);
    if (result == NULL)
        return NULL;

    it = PyObject_GetIter(other);
    if (it == NULL) {
        Py_DECREF(result);
        return NULL;
    }

    if (PyDictKeys_Check(self)) {
        dict_contains = dictkeys_contains;
    }
    /* else PyDictItems_Check(self) */
    else {
        dict_contains = dictitems_contains;
    }

    while ((key = PyIter_Next(it)) != NULL) {
        rv = dict_contains((_PyDictViewObject *)self, key);
        if (rv < 0) {
            goto error;
        }
        if (rv) {
            if (PySet_Add(result, key)) {
                goto error;
            }
        }
        Py_DECREF(key);
    }
    Py_DECREF(it);
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    return result;

error:
    Py_DECREF(it);
    Py_DECREF(result);
    Py_DECREF(key);
    return NULL;
}

static PyObject*
dictviews_or(PyObject* self, PyObject *other)
{
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    if (_PySet_Update(result, other) < 0) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

static PyObject *
dictitems_xor(PyObject *self, PyObject *other)
{
    assert(PyDictItems_Check(self));
    assert(PyDictItems_Check(other));
    PyObject *d1 = (PyObject *)((_PyDictViewObject *)self)->dv_dict;
    PyObject *d2 = (PyObject *)((_PyDictViewObject *)other)->dv_dict;

    PyObject *temp_dict = PyDict_Copy(d1);
    if (temp_dict == NULL) {
        return NULL;
    }
    PyObject *result_set = PySet_New(NULL);
    if (result_set == NULL) {
        Py_CLEAR(temp_dict);
        return NULL;
    }

    PyObject *key = NULL, *val1 = NULL, *val2 = NULL;
    Py_ssize_t pos = 0;
    Py_hash_t hash;

    while (_PyDict_Next(d2, &pos, &key, &val2, &hash)) {
        Py_INCREF(key);
        Py_INCREF(val2);
        val1 = _PyDict_GetItem_KnownHash(temp_dict, key, hash);

        int to_delete;
        if (val1 == NULL) {
            if (PyErr_Occurred()) {
                goto error;
            }
            to_delete = 0;
        }
        else {
            Py_INCREF(val1);
            to_delete = PyObject_RichCompareBool(val1, val2, Py_EQ);
            if (to_delete < 0) {
                goto error;
            }
        }

        if (to_delete) {
            if (_PyDict_DelItem_KnownHash(temp_dict, key, hash) < 0) {
                goto error;
            }
        }
        else {
            PyObject *pair = PyTuple_Pack(2, key, val2);
            if (pair == NULL) {
                goto error;
            }
            if (PySet_Add(result_set, pair) < 0) {
                Py_DECREF(pair);
                goto error;
            }
            Py_DECREF(pair);
        }
        Py_DECREF(key);
        Py_XDECREF(val1);
        Py_DECREF(val2);
    }
    key = val1 = val2 = NULL;

    PyObject *remaining_pairs = PyObject_CallMethodNoArgs(
            temp_dict, &_Py_ID(items));
    if (remaining_pairs == NULL) {
        goto error;
    }
    if (_PySet_Update(result_set, remaining_pairs) < 0) {
        Py_DECREF(remaining_pairs);
        goto error;
    }
    Py_DECREF(temp_dict);
    Py_DECREF(remaining_pairs);
    return result_set;

error:
    Py_XDECREF(temp_dict);
    Py_XDECREF(result_set);
    Py_XDECREF(key);
    Py_XDECREF(val1);
    Py_XDECREF(val2);
    return NULL;
}

static PyObject*
dictviews_xor(PyObject* self, PyObject *other)
{
    if (PyDictItems_Check(self) && PyDictItems_Check(other)) {
        return dictitems_xor(self, other);
    }
    PyObject *result = dictviews_to_set(self);
    if (result == NULL) {
        return NULL;
    }

    PyObject *tmp = PyObject_CallMethodOneArg(
            result, &_Py_ID(symmetric_difference_update), other);
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

static PyObject* dictkeys_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_keys_doc,
"Return a reverse iterator over the dict keys.");

static PyMethodDef dictkeys_methods[] = {
    {"isdisjoint",      (PyCFunction)dictviews_isdisjoint,  METH_O,
     isdisjoint_doc},
    {"__reversed__",    _PyCFunction_CAST(dictkeys_reversed),    METH_NOARGS,
     reversed_keys_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keys",                                /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    .tp_getset = dictview_getset,
};

static PyObject *
dictkeys_new(PyObject *dict, PyObject *Py_UNUSED(ignored))
{
    return _PyDictView_New(dict, &PyDictKeys_Type);
}

static PyObject *
dictkeys_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored))
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterKey_Type);
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
    result = PyObject_RichCompareBool(found, value, Py_EQ);
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

static PyObject* dictitems_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_items_doc,
"Return a reverse iterator over the dict items.");

static PyMethodDef dictitems_methods[] = {
    {"isdisjoint",      (PyCFunction)dictviews_isdisjoint,  METH_O,
     isdisjoint_doc},
    {"__reversed__",    (PyCFunction)dictitems_reversed,    METH_NOARGS,
     reversed_items_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_items",                               /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    .tp_getset = dictview_getset,
};

static PyObject *
dictitems_new(PyObject *dict, PyObject *Py_UNUSED(ignored))
{
    return _PyDictView_New(dict, &PyDictItems_Type);
}

static PyObject *
dictitems_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored))
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterItem_Type);
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

static PyObject* dictvalues_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_values_doc,
"Return a reverse iterator over the dict values.");

static PyMethodDef dictvalues_methods[] = {
    {"__reversed__",    (PyCFunction)dictvalues_reversed,    METH_NOARGS,
     reversed_values_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_values",                              /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor)dictview_dealloc,               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    .tp_getset = dictview_getset,
};

static PyObject *
dictvalues_new(PyObject *dict, PyObject *Py_UNUSED(ignored))
{
    return _PyDictView_New(dict, &PyDictValues_Type);
}

static PyObject *
dictvalues_reversed(_PyDictViewObject *dv, PyObject *Py_UNUSED(ignored))
{
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterValue_Type);
}


/* Returns NULL if cannot allocate a new PyDictKeysObject,
   but does not set an error */
PyDictKeysObject *
_PyDict_NewKeysForClass(void)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyDictKeysObject *keys = new_keys_object(
            interp, NEXT_LOG2_SHARED_KEYS_MAX_SIZE, 1);
    if (keys == NULL) {
        PyErr_Clear();
    }
    else {
        assert(keys->dk_nentries == 0);
        /* Set to max size+1 as it will shrink by one before each new object */
        keys->dk_usable = SHARED_KEYS_MAX_SIZE;
        keys->dk_kind = DICT_KEYS_SPLIT;
    }
    return keys;
}

#define CACHED_KEYS(tp) (((PyHeapTypeObject*)tp)->ht_cached_keys)

static int
init_inline_values(PyObject *obj, PyTypeObject *tp)
{
    assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
    // assert(type->tp_dictoffset > 0);  -- TO DO Update this assert.
    assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictKeysObject *keys = CACHED_KEYS(tp);
    assert(keys != NULL);
    if (keys->dk_usable > 1) {
        keys->dk_usable--;
    }
    size_t size = shared_keys_usable_size(keys);
    PyDictValues *values = new_values(size);
    if (values == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    assert(((uint8_t *)values)[-1] >= (size + 2));
    ((uint8_t *)values)[-2] = 0;
    for (size_t i = 0; i < size; i++) {
        values->values[i] = NULL;
    }
    _PyDictOrValues_SetValues(_PyObject_DictOrValuesPointer(obj), values);
    return 0;
}

int
_PyObject_InitializeDict(PyObject *obj)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_dictoffset == 0) {
        return 0;
    }
    if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        OBJECT_STAT_INC(new_values);
        return init_inline_values(obj, tp);
    }
    PyObject *dict;
    if (_PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE) && CACHED_KEYS(tp)) {
        dictkeys_incref(CACHED_KEYS(tp));
        dict = new_dict_with_shared_keys(interp, CACHED_KEYS(tp));
    }
    else {
        dict = PyDict_New();
    }
    if (dict == NULL) {
        return -1;
    }
    PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
    *dictptr = dict;
    return 0;
}

static PyObject *
make_dict_from_instance_attributes(PyInterpreterState *interp,
                                   PyDictKeysObject *keys, PyDictValues *values)
{
    dictkeys_incref(keys);
    Py_ssize_t used = 0;
    Py_ssize_t track = 0;
    size_t size = shared_keys_usable_size(keys);
    for (size_t i = 0; i < size; i++) {
        PyObject *val = values->values[i];
        if (val != NULL) {
            used += 1;
            track += _PyObject_GC_MAY_BE_TRACKED(val);
        }
    }
    PyObject *res = new_dict(interp, keys, values, used, 0);
    if (track && res) {
        _PyObject_GC_TRACK(res);
    }
    return res;
}

PyObject *
_PyObject_MakeDictFromInstanceAttributes(PyObject *obj, PyDictValues *values)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    OBJECT_STAT_INC(dict_materialized_on_request);
    return make_dict_from_instance_attributes(interp, keys, values);
}

int
_PyObject_StoreInstanceAttribute(PyObject *obj, PyDictValues *values,
                              PyObject *name, PyObject *value)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    assert(keys != NULL);
    assert(values != NULL);
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    Py_ssize_t ix = DKIX_EMPTY;
    if (PyUnicode_CheckExact(name)) {
        ix = insert_into_dictkeys(keys, name);
    }
    if (ix == DKIX_EMPTY) {
#ifdef Py_STATS
        if (PyUnicode_CheckExact(name)) {
            if (shared_keys_usable_size(keys) == SHARED_KEYS_MAX_SIZE) {
                OBJECT_STAT_INC(dict_materialized_too_big);
            }
            else {
                OBJECT_STAT_INC(dict_materialized_new_key);
            }
        }
        else {
            OBJECT_STAT_INC(dict_materialized_str_subclass);
        }
#endif
        PyObject *dict = make_dict_from_instance_attributes(
                interp, keys, values);
        if (dict == NULL) {
            return -1;
        }
        _PyObject_DictOrValuesPointer(obj)->dict = dict;
        if (value == NULL) {
            return PyDict_DelItem(dict, name);
        }
        else {
            return PyDict_SetItem(dict, name, value);
        }
    }
    PyObject *old_value = values->values[ix];
    values->values[ix] = Py_XNewRef(value);
    if (old_value == NULL) {
        if (value == NULL) {
            PyErr_Format(PyExc_AttributeError,
                         "'%.100s' object has no attribute '%U'",
                         Py_TYPE(obj)->tp_name, name);
            return -1;
        }
        _PyDictValues_AddToInsertionOrder(values, ix);
    }
    else {
        if (value == NULL) {
            delete_index_from_values(values, ix);
        }
        Py_DECREF(old_value);
    }
    return 0;
}

/* Sanity check for managed dicts */
#if 0
#define CHECK(val) assert(val); if (!(val)) { return 0; }

int
_PyObject_ManagedDictValidityCheck(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    CHECK(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictOrValues *dorv_ptr = _PyObject_DictOrValuesPointer(obj);
    if (_PyDictOrValues_IsValues(*dorv_ptr)) {
        PyDictValues *values = _PyDictOrValues_GetValues(*dorv_ptr);
        int size = ((uint8_t *)values)[-2];
        int count = 0;
        PyDictKeysObject *keys = CACHED_KEYS(tp);
        for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
            if (values->values[i] != NULL) {
                count++;
            }
        }
        CHECK(size == count);
    }
    else {
        if (dorv_ptr->dict != NULL) {
            CHECK(PyDict_Check(dorv_ptr->dict));
        }
    }
    return 1;
}
#endif

PyObject *
_PyObject_GetInstanceAttribute(PyObject *obj, PyDictValues *values,
                              PyObject *name)
{
    assert(PyUnicode_CheckExact(name));
    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    assert(keys != NULL);
    Py_ssize_t ix = _PyDictKeys_StringLookup(keys, name);
    if (ix == DKIX_EMPTY) {
        return NULL;
    }
    PyObject *value = values->values[ix];
    return Py_XNewRef(value);
}

int
_PyObject_IsInstanceDictEmpty(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_dictoffset == 0) {
        return 1;
    }
    PyObject *dict;
    if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(obj);
        if (_PyDictOrValues_IsValues(dorv)) {
            PyDictKeysObject *keys = CACHED_KEYS(tp);
            for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
                if (_PyDictOrValues_GetValues(dorv)->values[i] != NULL) {
                    return 0;
                }
            }
            return 1;
        }
        dict = _PyDictOrValues_GetDict(dorv);
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        dict = *dictptr;
    }
    if (dict == NULL) {
        return 1;
    }
    return ((PyDictObject *)dict)->ma_used == 0;
}

void
_PyObject_FreeInstanceAttributes(PyObject *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    assert(Py_TYPE(self)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(self);
    if (!_PyDictOrValues_IsValues(dorv)) {
        return;
    }
    PyDictValues *values = _PyDictOrValues_GetValues(dorv);
    PyDictKeysObject *keys = CACHED_KEYS(tp);
    for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
        Py_XDECREF(values->values[i]);
    }
    free_values(values);
}

int
_PyObject_VisitManagedDict(PyObject *obj, visitproc visit, void *arg)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return 0;
    }
    assert(tp->tp_dictoffset);
    PyDictOrValues dorv = *_PyObject_DictOrValuesPointer(obj);
    if (_PyDictOrValues_IsValues(dorv)) {
        PyDictValues *values = _PyDictOrValues_GetValues(dorv);
        PyDictKeysObject *keys = CACHED_KEYS(tp);
        for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
            Py_VISIT(values->values[i]);
        }
    }
    else {
        PyObject *dict = _PyDictOrValues_GetDict(dorv);
        Py_VISIT(dict);
    }
    return 0;
}

void
_PyObject_ClearManagedDict(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return;
    }
    PyDictOrValues *dorv_ptr = _PyObject_DictOrValuesPointer(obj);
    if (_PyDictOrValues_IsValues(*dorv_ptr)) {
        PyDictValues *values = _PyDictOrValues_GetValues(*dorv_ptr);
        PyDictKeysObject *keys = CACHED_KEYS(tp);
        for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
            Py_CLEAR(values->values[i]);
        }
        dorv_ptr->dict = NULL;
        free_values(values);
    }
    else {
        PyObject *dict = dorv_ptr->dict;
        if (dict) {
            dorv_ptr->dict = NULL;
            Py_DECREF(dict);
        }
    }
}

PyObject *
PyObject_GenericGetDict(PyObject *obj, void *context)
{
    PyObject *dict;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    PyTypeObject *tp = Py_TYPE(obj);
    if (_PyType_HasFeature(tp, Py_TPFLAGS_MANAGED_DICT)) {
        PyDictOrValues *dorv_ptr = _PyObject_DictOrValuesPointer(obj);
        if (_PyDictOrValues_IsValues(*dorv_ptr)) {
            PyDictValues *values = _PyDictOrValues_GetValues(*dorv_ptr);
            OBJECT_STAT_INC(dict_materialized_on_request);
            dict = make_dict_from_instance_attributes(
                    interp, CACHED_KEYS(tp), values);
            if (dict != NULL) {
                dorv_ptr->dict = dict;
            }
        }
        else {
            dict = _PyDictOrValues_GetDict(*dorv_ptr);
            if (dict == NULL) {
                dictkeys_incref(CACHED_KEYS(tp));
                dict = new_dict_with_shared_keys(interp, CACHED_KEYS(tp));
                dorv_ptr->dict = dict;
            }
        }
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr == NULL) {
            PyErr_SetString(PyExc_AttributeError,
                            "This object has no __dict__");
            return NULL;
        }
        dict = *dictptr;
        if (dict == NULL) {
            PyTypeObject *tp = Py_TYPE(obj);
            if (_PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE) && CACHED_KEYS(tp)) {
                dictkeys_incref(CACHED_KEYS(tp));
                *dictptr = dict = new_dict_with_shared_keys(
                        interp, CACHED_KEYS(tp));
            }
            else {
                *dictptr = dict = PyDict_New();
            }
        }
    }
    return Py_XNewRef(dict);
}

int
_PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr,
                      PyObject *key, PyObject *value)
{
    PyObject *dict;
    int res;
    PyDictKeysObject *cached;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    assert(dictptr != NULL);
    if ((tp->tp_flags & Py_TPFLAGS_HEAPTYPE) && (cached = CACHED_KEYS(tp))) {
        assert(dictptr != NULL);
        dict = *dictptr;
        if (dict == NULL) {
            dictkeys_incref(cached);
            dict = new_dict_with_shared_keys(interp, cached);
            if (dict == NULL)
                return -1;
            *dictptr = dict;
        }
        if (value == NULL) {
            res = PyDict_DelItem(dict, key);
        }
        else {
            res = PyDict_SetItem(dict, key, value);
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
    ASSERT_CONSISTENT(dict);
    return res;
}

void
_PyDictKeys_DecRef(PyDictKeysObject *keys)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    dictkeys_decref(interp, keys);
}

uint32_t _PyDictKeys_GetVersionForCurrentState(PyInterpreterState *interp,
                                               PyDictKeysObject *dictkeys)
{
    if (dictkeys->dk_version != 0) {
        return dictkeys->dk_version;
    }
    if (interp->dict_state.next_keys_version == 0) {
        return 0;
    }
    uint32_t v = interp->dict_state.next_keys_version++;
    dictkeys->dk_version = v;
    return v;
}

static inline int
validate_watcher_id(PyInterpreterState *interp, int watcher_id)
{
    if (watcher_id < 0 || watcher_id >= DICT_MAX_WATCHERS) {
        PyErr_Format(PyExc_ValueError, "Invalid dict watcher ID %d", watcher_id);
        return -1;
    }
    if (!interp->dict_state.watchers[watcher_id]) {
        PyErr_Format(PyExc_ValueError, "No dict watcher set for ID %d", watcher_id);
        return -1;
    }
    return 0;
}

int
PyDict_Watch(int watcher_id, PyObject* dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-dictionary");
        return -1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    ((PyDictObject*)dict)->ma_version_tag |= (1LL << watcher_id);
    return 0;
}

int
PyDict_Unwatch(int watcher_id, PyObject* dict)
{
    if (!PyDict_Check(dict)) {
        PyErr_SetString(PyExc_ValueError, "Cannot watch non-dictionary");
        return -1;
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    ((PyDictObject*)dict)->ma_version_tag &= ~(1LL << watcher_id);
    return 0;
}

int
PyDict_AddWatcher(PyDict_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    for (int i = 0; i < DICT_MAX_WATCHERS; i++) {
        if (!interp->dict_state.watchers[i]) {
            interp->dict_state.watchers[i] = callback;
            return i;
        }
    }

    PyErr_SetString(PyExc_RuntimeError, "no more dict watcher IDs available");
    return -1;
}

int
PyDict_ClearWatcher(int watcher_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (validate_watcher_id(interp, watcher_id)) {
        return -1;
    }
    interp->dict_state.watchers[watcher_id] = NULL;
    return 0;
}

static const char *
dict_event_name(PyDict_WatchEvent event) {
    switch (event) {
        #define CASE(op)                \
        case PyDict_EVENT_##op:         \
            return "PyDict_EVENT_" #op;
        PY_FOREACH_DICT_EVENT(CASE)
        #undef CASE
    }
    Py_UNREACHABLE();
}

void
_PyDict_SendEvent(int watcher_bits,
                  PyDict_WatchEvent event,
                  PyDictObject *mp,
                  PyObject *key,
                  PyObject *value)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    for (int i = 0; i < DICT_MAX_WATCHERS; i++) {
        if (watcher_bits & 1) {
            PyDict_WatchCallback cb = interp->dict_state.watchers[i];
            if (cb && (cb(event, (PyObject*)mp, key, value) < 0)) {
                // We don't want to resurrect the dict by potentially having an
                // unraisablehook keep a reference to it, so we don't pass the
                // dict as context, just an informative string message.  Dict
                // repr can call arbitrary code, so we invent a simpler version.
                PyObject *context = PyUnicode_FromFormat(
                    "%s watcher callback for <dict at %p>",
                    dict_event_name(event), mp);
                if (context == NULL) {
                    context = Py_NewRef(Py_None);
                }
                PyErr_WriteUnraisable(context);
                Py_DECREF(context);
            }
        }
        watcher_bits >>= 1;
    }
}
