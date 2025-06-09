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
| dk_version          |
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
Size of indices is dk_size.  Type of each index in indices varies with dk_size:

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
    Values are stored in the me_value field of the PyDictKeyEntry.
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
   In free-threaded builds dummy slots are not re-used to allow lock-free
   lookups to proceed safely.

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
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_code.h"          // stats
#include "pycore_critical_section.h" // Py_BEGIN_CRITICAL_SECTION, Py_END_CRITICAL_SECTION
#include "pycore_dict.h"          // export _PyDict_SizeOf()
#include "pycore_freelist.h"      // _PyFreeListState_GET()
#include "pycore_gc.h"            // _PyObject_GC_IS_TRACKED()
#include "pycore_object.h"        // _PyObject_GC_TRACK(), _PyDebugAllocatorStats()
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_SSIZE_RELAXED
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include "pycore_tuple.h"         // _PyTuple_Recycle()
#include "pycore_unicodeobject.h" // _PyUnicode_InternImmortal()

#include "stringlib/eq.h"                // unicode_eq()
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

#ifdef Py_GIL_DISABLED

static inline void
ASSERT_DICT_LOCKED(PyObject *op)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
}
#define ASSERT_DICT_LOCKED(op) ASSERT_DICT_LOCKED(_Py_CAST(PyObject*, op))
#define ASSERT_WORLD_STOPPED_OR_DICT_LOCKED(op)                         \
    if (!_PyInterpreterState_GET()->stoptheworld.world_stopped) {       \
        ASSERT_DICT_LOCKED(op);                                         \
    }
#define ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(op)                         \
    if (!_PyInterpreterState_GET()->stoptheworld.world_stopped) {      \
        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);                 \
    }

#define IS_DICT_SHARED(mp) _PyObject_GC_IS_SHARED(mp)
#define SET_DICT_SHARED(mp) _PyObject_GC_SET_SHARED(mp)
#define LOAD_INDEX(keys, size, idx) _Py_atomic_load_int##size##_relaxed(&((const int##size##_t*)keys->dk_indices)[idx]);
#define STORE_INDEX(keys, size, idx, value) _Py_atomic_store_int##size##_relaxed(&((int##size##_t*)keys->dk_indices)[idx], (int##size##_t)value);
#define ASSERT_OWNED_OR_SHARED(mp) \
    assert(_Py_IsOwnedByCurrentThread((PyObject *)mp) || IS_DICT_SHARED(mp));

#define LOCK_KEYS_IF_SPLIT(keys, kind) \
        if (kind == DICT_KEYS_SPLIT) { \
            LOCK_KEYS(keys);           \
        }

#define UNLOCK_KEYS_IF_SPLIT(keys, kind) \
        if (kind == DICT_KEYS_SPLIT) {   \
            UNLOCK_KEYS(keys);           \
        }

static inline Py_ssize_t
load_keys_nentries(PyDictObject *mp)
{
    PyDictKeysObject *keys = _Py_atomic_load_ptr(&mp->ma_keys);
    return _Py_atomic_load_ssize(&keys->dk_nentries);
}

static inline void
set_keys(PyDictObject *mp, PyDictKeysObject *keys)
{
    ASSERT_OWNED_OR_SHARED(mp);
    _Py_atomic_store_ptr_release(&mp->ma_keys, keys);
}

static inline void
set_values(PyDictObject *mp, PyDictValues *values)
{
    ASSERT_OWNED_OR_SHARED(mp);
    _Py_atomic_store_ptr_release(&mp->ma_values, values);
}

#define LOCK_KEYS(keys) PyMutex_LockFlags(&keys->dk_mutex, _Py_LOCK_DONT_DETACH)
#define UNLOCK_KEYS(keys) PyMutex_Unlock(&keys->dk_mutex)

#define ASSERT_KEYS_LOCKED(keys) assert(PyMutex_IsLocked(&keys->dk_mutex))
#define LOAD_SHARED_KEY(key) _Py_atomic_load_ptr_acquire(&key)
#define STORE_SHARED_KEY(key, value) _Py_atomic_store_ptr_release(&key, value)
// Inc refs the keys object, giving the previous value
#define INCREF_KEYS(dk)  _Py_atomic_add_ssize(&dk->dk_refcnt, 1)
// Dec refs the keys object, giving the previous value
#define DECREF_KEYS(dk)  _Py_atomic_add_ssize(&dk->dk_refcnt, -1)
#define LOAD_KEYS_NENTRIES(keys) _Py_atomic_load_ssize_relaxed(&keys->dk_nentries)

#define INCREF_KEYS_FT(dk) dictkeys_incref(dk)
#define DECREF_KEYS_FT(dk, shared) dictkeys_decref(_PyInterpreterState_GET(), dk, shared)

static inline void split_keys_entry_added(PyDictKeysObject *keys)
{
    ASSERT_KEYS_LOCKED(keys);

    // We increase before we decrease so we never get too small of a value
    // when we're racing with reads
    _Py_atomic_store_ssize_relaxed(&keys->dk_nentries, keys->dk_nentries + 1);
    _Py_atomic_store_ssize_release(&keys->dk_usable, keys->dk_usable - 1);
}

#else /* Py_GIL_DISABLED */

#define ASSERT_DICT_LOCKED(op)
#define ASSERT_WORLD_STOPPED_OR_DICT_LOCKED(op)
#define ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(op)
#define LOCK_KEYS(keys)
#define UNLOCK_KEYS(keys)
#define ASSERT_KEYS_LOCKED(keys)
#define LOAD_SHARED_KEY(key) key
#define STORE_SHARED_KEY(key, value) key = value
#define INCREF_KEYS(dk)  dk->dk_refcnt++
#define DECREF_KEYS(dk)  dk->dk_refcnt--
#define LOAD_KEYS_NENTRIES(keys) keys->dk_nentries
#define INCREF_KEYS_FT(dk)
#define DECREF_KEYS_FT(dk, shared)
#define LOCK_KEYS_IF_SPLIT(keys, kind)
#define UNLOCK_KEYS_IF_SPLIT(keys, kind)
#define IS_DICT_SHARED(mp) (false)
#define SET_DICT_SHARED(mp)
#define LOAD_INDEX(keys, size, idx) ((const int##size##_t*)(keys->dk_indices))[idx]
#define STORE_INDEX(keys, size, idx, value) ((int##size##_t*)(keys->dk_indices))[idx] = (int##size##_t)value

static inline void split_keys_entry_added(PyDictKeysObject *keys)
{
    keys->dk_usable--;
    keys->dk_nentries++;
}

static inline void
set_keys(PyDictObject *mp, PyDictKeysObject *keys)
{
    mp->ma_keys = keys;
}

static inline void
set_values(PyDictObject *mp, PyDictValues *values)
{
    mp->ma_values = values;
}

static inline Py_ssize_t
load_keys_nentries(PyDictObject *mp)
{
    return mp->ma_keys->dk_nentries;
}


#endif

#define STORE_KEY(ep, key) FT_ATOMIC_STORE_PTR_RELEASE((ep)->me_key, key)
#define STORE_VALUE(ep, value) FT_ATOMIC_STORE_PTR_RELEASE((ep)->me_value, value)
#define STORE_SPLIT_VALUE(mp, idx, value) FT_ATOMIC_STORE_PTR_RELEASE(mp->ma_values->values[idx], value)
#define STORE_HASH(ep, hash) FT_ATOMIC_STORE_SSIZE_RELAXED((ep)->me_hash, hash)
#define STORE_KEYS_USABLE(keys, usable) FT_ATOMIC_STORE_SSIZE_RELAXED(keys->dk_usable, usable)
#define STORE_KEYS_NENTRIES(keys, nentries) FT_ATOMIC_STORE_SSIZE_RELAXED(keys->dk_nentries, nentries)
#define STORE_USED(mp, used) FT_ATOMIC_STORE_SSIZE_RELAXED(mp->ma_used, used)

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

static PyObject* dict_iter(PyObject *dict);

static int
setitem_lock_held(PyDictObject *mp, PyObject *key, PyObject *value);
static int
dict_setdefault_ref_lock_held(PyObject *d, PyObject *key, PyObject *default_value,
                    PyObject **result, int incref_result);

#ifndef NDEBUG
static int _PyObject_InlineValuesConsistencyCheck(PyObject *obj);
#endif

#include "clinic/dictobject.c.h"


static inline Py_hash_t
unicode_get_hash(PyObject *o)
{
    assert(PyUnicode_CheckExact(o));
    return FT_ATOMIC_LOAD_SSIZE_RELAXED(_PyASCIIObject_CAST(o)->hash);
}

/* Print summary info about the state of the optimized allocator */
void
_PyDict_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out, "free PyDictObject",
                           _Py_FREELIST_SIZE(dicts),
                           sizeof(PyDictObject));
    _PyDebugAllocatorStats(out, "free PyDictKeysObject",
                           _Py_FREELIST_SIZE(dictkeys),
                           sizeof(PyDictKeysObject));
}

#define DK_MASK(dk) (DK_SIZE(dk)-1)

#define _Py_DICT_IMMORTAL_INITIAL_REFCNT PY_SSIZE_T_MIN

static void free_keys_object(PyDictKeysObject *keys, bool use_qsbr);

/* PyDictKeysObject has refcounts like PyObject does, so we have the
   following two functions to mirror what Py_INCREF() and Py_DECREF() do.
   (Keep in mind that PyDictKeysObject isn't actually a PyObject.)
   Likewise a PyDictKeysObject can be immortal (e.g. Py_EMPTY_KEYS),
   so we apply a naive version of what Py_INCREF() and Py_DECREF() do
   for immortal objects. */

static inline void
dictkeys_incref(PyDictKeysObject *dk)
{
    if (FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) < 0) {
        assert(FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
        return;
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    INCREF_KEYS(dk);
}

static inline void
dictkeys_decref(PyInterpreterState *interp, PyDictKeysObject *dk, bool use_qsbr)
{
    if (FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) < 0) {
        assert(FT_ATOMIC_LOAD_SSIZE_RELAXED(dk->dk_refcnt) == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
        return;
    }
    assert(FT_ATOMIC_LOAD_SSIZE(dk->dk_refcnt) > 0);
#ifdef Py_REF_DEBUG
    _Py_DecRefTotal(_PyThreadState_GET());
#endif
    if (DECREF_KEYS(dk) == 1) {
        if (DK_IS_UNICODE(dk)) {
            PyDictUnicodeEntry *entries = DK_UNICODE_ENTRIES(dk);
            Py_ssize_t i, n;
            for (i = 0, n = dk->dk_nentries; i < n; i++) {
                Py_XDECREF(entries[i].me_key);
                Py_XDECREF(entries[i].me_value);
            }
        }
        else {
            PyDictKeyEntry *entries = DK_ENTRIES(dk);
            Py_ssize_t i, n;
            for (i = 0, n = dk->dk_nentries; i < n; i++) {
                Py_XDECREF(entries[i].me_key);
                Py_XDECREF(entries[i].me_value);
            }
        }
        free_keys_object(dk, use_qsbr);
    }
}

/* lookup indices.  returns DKIX_EMPTY, DKIX_DUMMY, or ix >=0 */
static inline Py_ssize_t
dictkeys_get_index(const PyDictKeysObject *keys, Py_ssize_t i)
{
    int log2size = DK_LOG_SIZE(keys);
    Py_ssize_t ix;

    if (log2size < 8) {
        ix = LOAD_INDEX(keys, 8, i);
    }
    else if (log2size < 16) {
        ix = LOAD_INDEX(keys, 16, i);
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        ix = LOAD_INDEX(keys, 64, i);
    }
#endif
    else {
        ix = LOAD_INDEX(keys, 32, i);
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
        assert(ix <= 0x7f);
        STORE_INDEX(keys, 8, i, ix);
    }
    else if (log2size < 16) {
        assert(ix <= 0x7fff);
        STORE_INDEX(keys, 16, i, ix);
    }
#if SIZEOF_VOID_P > 4
    else if (log2size >= 32) {
        STORE_INDEX(keys, 64, i, ix);
    }
#endif
    else {
        assert(ix <= 0x7fffffff);
        STORE_INDEX(keys, 32, i, ix);
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
    minsize = Py_MAX(minsize, PyDict_MINSIZE);
    return _Py_bit_length(minsize - 1);
#elif defined(_MSC_VER)
    // On 64bit Windows, sizeof(long) == 4. We cannot use _Py_bit_length.
    minsize = Py_MAX(minsize, PyDict_MINSIZE);
    unsigned long msb;
    _BitScanReverse64(&msb, (uint64_t)minsize - 1);
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
 *
 * See https://github.com/python/cpython/pull/127568#discussion_r1868070614
 * for the rationale of using dk_log2_index_bytes=3 instead of 0.
 */
static PyDictKeysObject empty_keys_struct = {
        _Py_DICT_IMMORTAL_INITIAL_REFCNT, /* dk_refcnt */
        0, /* dk_log2_size */
        3, /* dk_log2_index_bytes */
        DICT_KEYS_UNICODE, /* dk_kind */
#ifdef Py_GIL_DISABLED
        {0}, /* dk_mutex */
#endif
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
    assert(i < mp->ma_values->size);
    uint8_t *array = get_insertion_order_array(mp->ma_values);
    return array[i];
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
    ASSERT_WORLD_STOPPED_OR_DICT_LOCKED(op);

#define CHECK(expr) \
    do { if (!(expr)) { _PyObject_ASSERT_FAILED_MSG(op, Py_STRINGIFY(expr)); } } while (0)

    assert(op != NULL);
    CHECK(PyDict_Check(op));
    PyDictObject *mp = (PyDictObject *)op;

    PyDictKeysObject *keys = mp->ma_keys;
    int splitted = _PyDict_HasSplitTable(mp);
    Py_ssize_t usable = USABLE_FRACTION(DK_SIZE(keys));

    // In the free-threaded build, shared keys may be concurrently modified,
    // so use atomic loads.
    Py_ssize_t dk_usable = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_usable);
    Py_ssize_t dk_nentries = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_nentries);

    CHECK(0 <= mp->ma_used && mp->ma_used <= usable);
    CHECK(0 <= dk_usable && dk_usable <= usable);
    CHECK(0 <= dk_nentries && dk_nentries <= usable);
    CHECK(dk_usable + dk_nentries <= usable);

    if (!splitted) {
        /* combined table */
        CHECK(keys->dk_kind != DICT_KEYS_SPLIT);
        CHECK(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
    }
    else {
        CHECK(keys->dk_kind == DICT_KEYS_SPLIT);
        CHECK(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
        if (mp->ma_values->embedded) {
            CHECK(mp->ma_values->embedded == 1);
            CHECK(mp->ma_values->valid == 1);
        }
    }

    if (check_content) {
        LOCK_KEYS_IF_SPLIT(keys, keys->dk_kind);
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
        UNLOCK_KEYS_IF_SPLIT(keys, keys->dk_kind);
    }
    return 1;

#undef CHECK
}


static PyDictKeysObject*
new_keys_object(PyInterpreterState *interp, uint8_t log2_size, bool unicode)
{
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

    PyDictKeysObject *dk = NULL;
    if (log2_size == PyDict_LOG_MINSIZE && unicode) {
        dk = _Py_FREELIST_POP_MEM(dictkeys);
    }
    if (dk == NULL) {
        dk = PyMem_Malloc(sizeof(PyDictKeysObject)
                          + ((size_t)1 << log2_bytes)
                          + entry_size * usable);
        if (dk == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal(_PyThreadState_GET());
#endif
    dk->dk_refcnt = 1;
    dk->dk_log2_size = log2_size;
    dk->dk_log2_index_bytes = log2_bytes;
    dk->dk_kind = unicode ? DICT_KEYS_UNICODE : DICT_KEYS_GENERAL;
#ifdef Py_GIL_DISABLED
    dk->dk_mutex = (PyMutex){0};
#endif
    dk->dk_nentries = 0;
    dk->dk_usable = usable;
    dk->dk_version = 0;
    memset(&dk->dk_indices[0], 0xff, ((size_t)1 << log2_bytes));
    memset(&dk->dk_indices[(size_t)1 << log2_bytes], 0, entry_size * usable);
    return dk;
}

static void
free_keys_object(PyDictKeysObject *keys, bool use_qsbr)
{
#ifdef Py_GIL_DISABLED
    if (use_qsbr) {
        _PyMem_FreeDelayed(keys);
        return;
    }
#endif
    if (DK_LOG_SIZE(keys) == PyDict_LOG_MINSIZE && keys->dk_kind == DICT_KEYS_UNICODE) {
        _Py_FREELIST_FREE(dictkeys, keys, PyMem_Free);
    }
    else {
        PyMem_Free(keys);
    }
}

static size_t
values_size_from_count(size_t count)
{
    assert(count >= 1);
    size_t suffix_size = _Py_SIZE_ROUND_UP(count, sizeof(PyObject *));
    assert(suffix_size < 128);
    assert(suffix_size % sizeof(PyObject *) == 0);
    return (count + 1) * sizeof(PyObject *) + suffix_size;
}

#define CACHED_KEYS(tp) (((PyHeapTypeObject*)tp)->ht_cached_keys)

static inline PyDictValues*
new_values(size_t size)
{
    size_t n = values_size_from_count(size);
    PyDictValues *res = (PyDictValues *)PyMem_Malloc(n);
    if (res == NULL) {
        return NULL;
    }
    res->embedded = 0;
    res->size = 0;
    assert(size < 256);
    res->capacity = (uint8_t)size;
    return res;
}

static inline void
free_values(PyDictValues *values, bool use_qsbr)
{
    assert(values->embedded == 0);
#ifdef Py_GIL_DISABLED
    if (use_qsbr) {
        _PyMem_FreeDelayed(values);
        return;
    }
#endif
    PyMem_Free(values);
}

/* Consumes a reference to the keys object */
static PyObject *
new_dict(PyInterpreterState *interp,
         PyDictKeysObject *keys, PyDictValues *values,
         Py_ssize_t used, int free_values_on_failure)
{
    assert(keys != NULL);
    PyDictObject *mp = _Py_FREELIST_POP(PyDictObject, dicts);
    if (mp == NULL) {
        mp = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (mp == NULL) {
            dictkeys_decref(interp, keys, false);
            if (free_values_on_failure) {
                free_values(values, false);
            }
            return NULL;
        }
    }
    assert(Py_IS_TYPE(mp, &PyDict_Type));
    mp->ma_keys = keys;
    mp->ma_values = values;
    mp->ma_used = used;
    mp->_ma_watcher_tag = 0;
    ASSERT_CONSISTENT(mp);
    _PyObject_GC_TRACK(mp);
    return (PyObject *)mp;
}

static PyObject *
new_dict_with_shared_keys(PyInterpreterState *interp, PyDictKeysObject *keys)
{
    size_t size = shared_keys_usable_size(keys);
    PyDictValues *values = new_values(size);
    if (values == NULL) {
        return PyErr_NoMemory();
    }
    dictkeys_incref(keys);
    for (size_t i = 0; i < size; i++) {
        values->values[i] = NULL;
    }
    return new_dict(interp, keys, values, 0, 1);
}


static PyDictKeysObject *
clone_combined_dict_keys(PyDictObject *orig)
{
    assert(PyDict_Check(orig));
    assert(Py_TYPE(orig)->tp_iter == dict_iter);
    assert(orig->ma_values == NULL);
    assert(orig->ma_keys != Py_EMPTY_KEYS);
    assert(orig->ma_keys->dk_refcnt == 1);

    ASSERT_DICT_LOCKED(orig);

    size_t keys_size = _PyDict_KeysSize(orig->ma_keys);
    PyDictKeysObject *keys = PyMem_Malloc(keys_size);
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
    _Py_IncRefTotal(_PyThreadState_GET());
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

static inline Py_ALWAYS_INLINE Py_ssize_t
do_lookup(PyDictObject *mp, PyDictKeysObject *dk, PyObject *key, Py_hash_t hash,
          int (*check_lookup)(PyDictObject *, PyDictKeysObject *, void *, Py_ssize_t ix, PyObject *key, Py_hash_t))
{
    void *ep0 = _DK_ENTRIES(dk);
    size_t mask = DK_MASK(dk);
    size_t perturb = hash;
    size_t i = (size_t)hash & mask;
    Py_ssize_t ix;
    for (;;) {
        ix = dictkeys_get_index(dk, i);
        if (ix >= 0) {
            int cmp = check_lookup(mp, dk, ep0, ix, key, hash);
            if (cmp < 0) {
                return cmp;
            } else if (cmp) {
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
            int cmp = check_lookup(mp, dk, ep0, ix, key, hash);
            if (cmp < 0) {
                return cmp;
            } else if (cmp) {
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

static inline int
compare_unicode_generic(PyDictObject *mp, PyDictKeysObject *dk,
                        void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    assert(ep->me_key != NULL);
    assert(PyUnicode_CheckExact(ep->me_key));
    assert(!PyUnicode_CheckExact(key));

    if (unicode_get_hash(ep->me_key) == hash) {
        PyObject *startkey = ep->me_key;
        Py_INCREF(startkey);
        int cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
        Py_DECREF(startkey);
        if (cmp < 0) {
            return DKIX_ERROR;
        }
        if (dk == mp->ma_keys && ep->me_key == startkey) {
            return cmp;
        }
        else {
            /* The dict was mutated, restart */
            return DKIX_KEY_CHANGED;
        }
    }
    return 0;
}

// Search non-Unicode key from Unicode table
static Py_ssize_t
unicodekeys_lookup_generic(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_unicode_generic);
}

static inline int
compare_unicode_unicode(PyDictObject *mp, PyDictKeysObject *dk,
                        void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    PyObject *ep_key = FT_ATOMIC_LOAD_PTR_RELAXED(ep->me_key);
    assert(ep_key != NULL);
    assert(PyUnicode_CheckExact(ep_key));
    if (ep_key == key ||
            (unicode_get_hash(ep_key) == hash && unicode_eq(ep_key, key))) {
        return 1;
    }
    return 0;
}

static Py_ssize_t _Py_HOT_FUNCTION
unicodekeys_lookup_unicode(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(NULL, dk, key, hash, compare_unicode_unicode);
}

static inline int
compare_generic(PyDictObject *mp, PyDictKeysObject *dk,
                void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictKeyEntry *ep = &((PyDictKeyEntry *)ep0)[ix];
    assert(ep->me_key != NULL);
    if (ep->me_key == key) {
        return 1;
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
            return cmp;
        }
        else {
            /* The dict was mutated, restart */
            return DKIX_KEY_CHANGED;
        }
    }
    return 0;
}

static Py_ssize_t
dictkeys_generic_lookup(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_generic);
}

static bool
check_keys_unicode(PyDictKeysObject *dk, PyObject *key)
{
    return PyUnicode_CheckExact(key) && (dk->dk_kind != DICT_KEYS_GENERAL);
}

static Py_ssize_t
hash_unicode_key(PyObject *key)
{
    assert(PyUnicode_CheckExact(key));
    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = PyUnicode_Type.tp_hash(key);
        assert(hash != -1);
    }
    return hash;
}

#ifdef Py_GIL_DISABLED
static Py_ssize_t
unicodekeys_lookup_unicode_threadsafe(PyDictKeysObject* dk, PyObject *key,
                                      Py_hash_t hash);
#endif

static Py_ssize_t
unicodekeys_lookup_split(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix;
    assert(dk->dk_kind == DICT_KEYS_SPLIT);
    assert(PyUnicode_CheckExact(key));

#ifdef Py_GIL_DISABLED
    // A split dictionaries keys can be mutated by other dictionaries
    // but if we have a unicode key we can avoid locking the shared
    // keys.
    ix = unicodekeys_lookup_unicode_threadsafe(dk, key, hash);
    if (ix == DKIX_KEY_CHANGED) {
        LOCK_KEYS(dk);
        ix = unicodekeys_lookup_unicode(dk, key, hash);
        UNLOCK_KEYS(dk);
    }
#else
    ix = unicodekeys_lookup_unicode(dk, key, hash);
#endif
    return ix;
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
    if (!check_keys_unicode(dk, key)) {
        return DKIX_ERROR;
    }
    Py_hash_t hash = hash_unicode_key(key);
    return unicodekeys_lookup_unicode(dk, key, hash);
}

Py_ssize_t
_PyDictKeys_StringLookupAndVersion(PyDictKeysObject *dk, PyObject *key, uint32_t *version)
{
    if (!check_keys_unicode(dk, key)) {
        return DKIX_ERROR;
    }
    Py_ssize_t ix;
    Py_hash_t hash = hash_unicode_key(key);
    LOCK_KEYS(dk);
    ix = unicodekeys_lookup_unicode(dk, key, hash);
    *version = _PyDictKeys_GetVersionForCurrentState(_PyInterpreterState_GET(), dk);
    UNLOCK_KEYS(dk);
    return ix;
}

/* Like _PyDictKeys_StringLookup() but only works on split keys.  Note
 * that in free-threaded builds this locks the keys object as required.
 */
Py_ssize_t
_PyDictKeys_StringLookupSplit(PyDictKeysObject* dk, PyObject *key)
{
    assert(dk->dk_kind == DICT_KEYS_SPLIT);
    assert(PyUnicode_CheckExact(key));
    Py_hash_t hash = unicode_get_hash(key);
    if (hash == -1) {
        hash = PyUnicode_Type.tp_hash(key);
        if (hash == -1) {
            PyErr_Clear();
            return DKIX_ERROR;
        }
    }
    return unicodekeys_lookup_split(dk, key, hash);
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

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(mp);
start:
    dk = mp->ma_keys;
    kind = dk->dk_kind;

    if (kind != DICT_KEYS_GENERAL) {
        if (PyUnicode_CheckExact(key)) {
#ifdef Py_GIL_DISABLED
            if (kind == DICT_KEYS_SPLIT) {
                ix = unicodekeys_lookup_split(dk, key, hash);
            }
            else {
                ix = unicodekeys_lookup_unicode(dk, key, hash);
            }
#else
            ix = unicodekeys_lookup_unicode(dk, key, hash);
#endif
        }
        else {
            INCREF_KEYS_FT(dk);
            LOCK_KEYS_IF_SPLIT(dk, kind);

            ix = unicodekeys_lookup_generic(mp, dk, key, hash);

            UNLOCK_KEYS_IF_SPLIT(dk, kind);
            DECREF_KEYS_FT(dk, IS_DICT_SHARED(mp));
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

#ifdef Py_GIL_DISABLED
static inline void
ensure_shared_on_read(PyDictObject *mp)
{
    if (!_Py_IsOwnedByCurrentThread((PyObject *)mp) && !IS_DICT_SHARED(mp)) {
        // The first time we access a dict from a non-owning thread we mark it
        // as shared. This ensures that a concurrent resize operation will
        // delay freeing the old keys or values using QSBR, which is necessary
        // to safely allow concurrent reads without locking...
        Py_BEGIN_CRITICAL_SECTION(mp);
        if (!IS_DICT_SHARED(mp)) {
            SET_DICT_SHARED(mp);
        }
        Py_END_CRITICAL_SECTION();
    }
}

void
_PyDict_EnsureSharedOnRead(PyDictObject *mp)
{
    ensure_shared_on_read(mp);
}
#endif

static inline void
ensure_shared_on_resize(PyDictObject *mp)
{
#ifdef Py_GIL_DISABLED
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(mp);

    if (!_Py_IsOwnedByCurrentThread((PyObject *)mp) && !IS_DICT_SHARED(mp)) {
        // We are writing to the dict from another thread that owns
        // it and we haven't marked it as shared which will ensure
        // that when we re-size ma_keys or ma_values that we will
        // free using QSBR.  We need to lock the dictionary to
        // contend with writes from the owning thread, mark it as
        // shared, and then we can continue with lock-free reads.
        // Technically this is a little heavy handed, we could just
        // free the individual old keys / old-values using qsbr
        SET_DICT_SHARED(mp);
    }
#endif
}

static inline void
ensure_shared_on_keys_version_assignment(PyDictObject *mp)
{
    ASSERT_DICT_LOCKED((PyObject *) mp);
    #ifdef Py_GIL_DISABLED
    if (!IS_DICT_SHARED(mp)) {
        // This ensures that a concurrent resize operation will delay
        // freeing the old keys or values using QSBR, which is necessary to
        // safely allow concurrent reads without locking.
        SET_DICT_SHARED(mp);
    }
    #endif
}

#ifdef Py_GIL_DISABLED

static inline Py_ALWAYS_INLINE int
compare_unicode_generic_threadsafe(PyDictObject *mp, PyDictKeysObject *dk,
                                   void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    PyObject *startkey = _Py_atomic_load_ptr_relaxed(&ep->me_key);
    assert(startkey == NULL || PyUnicode_CheckExact(ep->me_key));
    assert(!PyUnicode_CheckExact(key));

    if (startkey != NULL) {
        if (!_Py_TryIncrefCompare(&ep->me_key, startkey)) {
            return DKIX_KEY_CHANGED;
        }

        if (unicode_get_hash(startkey) == hash) {
            int cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
            Py_DECREF(startkey);
            if (cmp < 0) {
                return DKIX_ERROR;
            }
            if (dk == _Py_atomic_load_ptr_relaxed(&mp->ma_keys) &&
                startkey == _Py_atomic_load_ptr_relaxed(&ep->me_key)) {
                return cmp;
            }
            else {
                /* The dict was mutated, restart */
                return DKIX_KEY_CHANGED;
            }
        }
        else {
            Py_DECREF(startkey);
        }
    }
    return 0;
}

// Search non-Unicode key from Unicode table
static Py_ssize_t
unicodekeys_lookup_generic_threadsafe(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_unicode_generic_threadsafe);
}

static inline Py_ALWAYS_INLINE int
compare_unicode_unicode_threadsafe(PyDictObject *mp, PyDictKeysObject *dk,
                                   void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictUnicodeEntry *ep = &((PyDictUnicodeEntry *)ep0)[ix];
    PyObject *startkey = _Py_atomic_load_ptr_relaxed(&ep->me_key);
    if (startkey == key) {
        assert(PyUnicode_CheckExact(startkey));
        return 1;
    }
    if (startkey != NULL) {
        if (_Py_IsImmortal(startkey)) {
            assert(PyUnicode_CheckExact(startkey));
            return unicode_get_hash(startkey) == hash && unicode_eq(startkey, key);
        }
        else {
            if (!_Py_TryIncrefCompare(&ep->me_key, startkey)) {
                return DKIX_KEY_CHANGED;
            }
            assert(PyUnicode_CheckExact(startkey));
            if (unicode_get_hash(startkey) == hash && unicode_eq(startkey, key)) {
                Py_DECREF(startkey);
                return 1;
            }
            Py_DECREF(startkey);
        }
    }
    return 0;
}

static Py_ssize_t _Py_HOT_FUNCTION
unicodekeys_lookup_unicode_threadsafe(PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(NULL, dk, key, hash, compare_unicode_unicode_threadsafe);
}

static inline Py_ALWAYS_INLINE int
compare_generic_threadsafe(PyDictObject *mp, PyDictKeysObject *dk,
                           void *ep0, Py_ssize_t ix, PyObject *key, Py_hash_t hash)
{
    PyDictKeyEntry *ep = &((PyDictKeyEntry *)ep0)[ix];
    PyObject *startkey = _Py_atomic_load_ptr_relaxed(&ep->me_key);
    if (startkey == key) {
        return 1;
    }
    Py_ssize_t ep_hash = _Py_atomic_load_ssize_relaxed(&ep->me_hash);
    if (ep_hash == hash) {
        if (startkey == NULL || !_Py_TryIncrefCompare(&ep->me_key, startkey)) {
            return DKIX_KEY_CHANGED;
        }
        int cmp = PyObject_RichCompareBool(startkey, key, Py_EQ);
        Py_DECREF(startkey);
        if (cmp < 0) {
            return DKIX_ERROR;
        }
        if (dk == _Py_atomic_load_ptr_relaxed(&mp->ma_keys) &&
            startkey == _Py_atomic_load_ptr_relaxed(&ep->me_key)) {
            return cmp;
        }
        else {
            /* The dict was mutated, restart */
            return DKIX_KEY_CHANGED;
        }
    }
    return 0;
}

static Py_ssize_t
dictkeys_generic_lookup_threadsafe(PyDictObject *mp, PyDictKeysObject* dk, PyObject *key, Py_hash_t hash)
{
    return do_lookup(mp, dk, key, hash, compare_generic_threadsafe);
}

Py_ssize_t
_Py_dict_lookup_threadsafe(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr)
{
    PyDictKeysObject *dk;
    DictKeysKind kind;
    Py_ssize_t ix;
    PyObject *value;

    ensure_shared_on_read(mp);

    dk = _Py_atomic_load_ptr(&mp->ma_keys);
    kind = dk->dk_kind;

    if (kind != DICT_KEYS_GENERAL) {
        if (PyUnicode_CheckExact(key)) {
            ix = unicodekeys_lookup_unicode_threadsafe(dk, key, hash);
        }
        else {
            ix = unicodekeys_lookup_generic_threadsafe(mp, dk, key, hash);
        }
        if (ix == DKIX_KEY_CHANGED) {
            goto read_failed;
        }

        if (ix >= 0) {
            if (kind == DICT_KEYS_SPLIT) {
                PyDictValues *values = _Py_atomic_load_ptr(&mp->ma_values);
                if (values == NULL)
                    goto read_failed;

                uint8_t capacity = _Py_atomic_load_uint8_relaxed(&values->capacity);
                if (ix >= (Py_ssize_t)capacity)
                    goto read_failed;

                value = _Py_TryXGetRef(&values->values[ix]);
                if (value == NULL)
                    goto read_failed;

                if (values != _Py_atomic_load_ptr(&mp->ma_values)) {
                    Py_DECREF(value);
                    goto read_failed;
                }
            }
            else {
                value = _Py_TryXGetRef(&DK_UNICODE_ENTRIES(dk)[ix].me_value);
                if (value == NULL) {
                    goto read_failed;
                }

                if (dk != _Py_atomic_load_ptr(&mp->ma_keys)) {
                    Py_DECREF(value);
                    goto read_failed;
                }
            }
        }
        else {
            value = NULL;
        }
    }
    else {
        ix = dictkeys_generic_lookup_threadsafe(mp, dk, key, hash);
        if (ix == DKIX_KEY_CHANGED) {
            goto read_failed;
        }
        if (ix >= 0) {
            value = _Py_TryXGetRef(&DK_ENTRIES(dk)[ix].me_value);
            if (value == NULL)
                goto read_failed;

            if (dk != _Py_atomic_load_ptr(&mp->ma_keys)) {
                Py_DECREF(value);
                goto read_failed;
            }
        }
        else {
            value = NULL;
        }
    }

    *value_addr = value;
    return ix;

read_failed:
    // In addition to the normal races of the dict being modified the _Py_TryXGetRef
    // can all fail if they don't yet have a shared ref count.  That can happen here
    // or in the *_lookup_* helper.  In that case we need to take the lock to avoid
    // mutation and do a normal incref which will make them shared.
    Py_BEGIN_CRITICAL_SECTION(mp);
    ix = _Py_dict_lookup(mp, key, hash, &value);
    *value_addr = value;
    if (value != NULL) {
        assert(ix >= 0);
        _Py_NewRefWithLock(value);
    }
    Py_END_CRITICAL_SECTION();
    return ix;
}

Py_ssize_t
_Py_dict_lookup_threadsafe_stackref(PyDictObject *mp, PyObject *key, Py_hash_t hash, _PyStackRef *value_addr)
{
    PyDictKeysObject *dk = _Py_atomic_load_ptr(&mp->ma_keys);
    if (dk->dk_kind == DICT_KEYS_UNICODE && PyUnicode_CheckExact(key)) {
        Py_ssize_t ix = unicodekeys_lookup_unicode_threadsafe(dk, key, hash);
        if (ix == DKIX_EMPTY) {
            *value_addr = PyStackRef_NULL;
            return ix;
        }
        else if (ix >= 0) {
            PyObject **addr_of_value = &DK_UNICODE_ENTRIES(dk)[ix].me_value;
            PyObject *value = _Py_atomic_load_ptr(addr_of_value);
            if (value == NULL) {
                *value_addr = PyStackRef_NULL;
                return DKIX_EMPTY;
            }
            if (_PyObject_HasDeferredRefcount(value)) {
                *value_addr =  (_PyStackRef){ .bits = (uintptr_t)value | Py_TAG_DEFERRED };
                return ix;
            }
            if (_Py_TryIncrefCompare(addr_of_value, value)) {
                *value_addr = PyStackRef_FromPyObjectSteal(value);
                return ix;
            }
        }
    }

    PyObject *obj;
    Py_ssize_t ix = _Py_dict_lookup_threadsafe(mp, key, hash, &obj);
    if (ix >= 0 && obj != NULL) {
        *value_addr = PyStackRef_FromPyObjectSteal(obj);
    }
    else {
        *value_addr = PyStackRef_NULL;
    }
    return ix;
}

#else   // Py_GIL_DISABLED

Py_ssize_t
_Py_dict_lookup_threadsafe(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr)
{
    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, value_addr);
    Py_XNewRef(*value_addr);
    return ix;
}

Py_ssize_t
_Py_dict_lookup_threadsafe_stackref(PyDictObject *mp, PyObject *key, Py_hash_t hash, _PyStackRef *value_addr)
{
    PyObject *val;
    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &val);
    if (val == NULL) {
        *value_addr = PyStackRef_NULL;
    }
    else {
        *value_addr = PyStackRef_FromPyObjectNew(val);
    }
    return ix;
}

#endif

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

void
_PyDict_EnablePerThreadRefcounting(PyObject *op)
{
    assert(PyDict_Check(op));
#ifdef Py_GIL_DISABLED
    Py_ssize_t id = _PyObject_AssignUniqueId(op);
    if (id == _Py_INVALID_UNIQUE_ID) {
        return;
    }
    if ((uint64_t)id >= (uint64_t)DICT_UNIQUE_ID_MAX) {
        _PyObject_ReleaseUniqueId(id);
        return;
    }

    PyDictObject *mp = (PyDictObject *)op;
    assert((mp->_ma_watcher_tag >> DICT_UNIQUE_ID_SHIFT) == 0);
    mp->_ma_watcher_tag += (uint64_t)id << DICT_UNIQUE_ID_SHIFT;
#endif
}

static inline int
is_unusable_slot(Py_ssize_t ix)
{
#ifdef Py_GIL_DISABLED
    return ix >= 0 || ix == DKIX_DUMMY;
#else
    return ix >= 0;
#endif
}

/* Internal function to find slot for an item from its hash
   when it is known that the key is not present in the dict.
 */
static Py_ssize_t
find_empty_slot(PyDictKeysObject *keys, Py_hash_t hash)
{
    assert(keys != NULL);

    const size_t mask = DK_MASK(keys);
    size_t i = hash & mask;
    Py_ssize_t ix = dictkeys_get_index(keys, i);
    for (size_t perturb = hash; is_unusable_slot(ix);) {
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

static inline int
insert_combined_dict(PyInterpreterState *interp, PyDictObject *mp,
                     Py_hash_t hash, PyObject *key, PyObject *value)
{
    if (mp->ma_keys->dk_usable <= 0) {
        /* Need to resize. */
        if (insertion_resize(interp, mp, 1) < 0) {
            return -1;
        }
    }

    _PyDict_NotifyEvent(interp, PyDict_EVENT_ADDED, mp, key, value);
    FT_ATOMIC_STORE_UINT32_RELAXED(mp->ma_keys->dk_version, 0);

    Py_ssize_t hashpos = find_empty_slot(mp->ma_keys, hash);
    dictkeys_set_index(mp->ma_keys, hashpos, mp->ma_keys->dk_nentries);

    if (DK_IS_UNICODE(mp->ma_keys)) {
        PyDictUnicodeEntry *ep;
        ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
        STORE_KEY(ep, key);
        STORE_VALUE(ep, value);
    }
    else {
        PyDictKeyEntry *ep;
        ep = &DK_ENTRIES(mp->ma_keys)[mp->ma_keys->dk_nentries];
        STORE_KEY(ep, key);
        STORE_VALUE(ep, value);
        STORE_HASH(ep, hash);
    }
    STORE_KEYS_USABLE(mp->ma_keys, mp->ma_keys->dk_usable - 1);
    STORE_KEYS_NENTRIES(mp->ma_keys, mp->ma_keys->dk_nentries + 1);
    assert(mp->ma_keys->dk_usable >= 0);
    return 0;
}

static Py_ssize_t
insert_split_key(PyDictKeysObject *keys, PyObject *key, Py_hash_t hash)
{
    assert(PyUnicode_CheckExact(key));
    Py_ssize_t ix;


#ifdef Py_GIL_DISABLED
    ix = unicodekeys_lookup_unicode_threadsafe(keys, key, hash);
    if (ix >= 0) {
        return ix;
    }
#endif

    LOCK_KEYS(keys);
    ix = unicodekeys_lookup_unicode(keys, key, hash);
    if (ix == DKIX_EMPTY && keys->dk_usable > 0) {
        // Insert into new slot
        FT_ATOMIC_STORE_UINT32_RELAXED(keys->dk_version, 0);
        Py_ssize_t hashpos = find_empty_slot(keys, hash);
        ix = keys->dk_nentries;
        dictkeys_set_index(keys, hashpos, ix);
        PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(keys)[ix];
        STORE_SHARED_KEY(ep->me_key, Py_NewRef(key));
        split_keys_entry_added(keys);
    }
    assert (ix < SHARED_KEYS_MAX_SIZE);
    UNLOCK_KEYS(keys);
    return ix;
}

static void
insert_split_value(PyInterpreterState *interp, PyDictObject *mp, PyObject *key, PyObject *value, Py_ssize_t ix)
{
    assert(PyUnicode_CheckExact(key));
    ASSERT_DICT_LOCKED(mp);
    PyObject *old_value = mp->ma_values->values[ix];
    if (old_value == NULL) {
        _PyDict_NotifyEvent(interp, PyDict_EVENT_ADDED, mp, key, value);
        STORE_SPLIT_VALUE(mp, ix, Py_NewRef(value));
        _PyDictValues_AddToInsertionOrder(mp->ma_values, ix);
        STORE_USED(mp, mp->ma_used + 1);
    }
    else {
        _PyDict_NotifyEvent(interp, PyDict_EVENT_MODIFIED, mp, key, value);
        STORE_SPLIT_VALUE(mp, ix, Py_NewRef(value));
        // old_value should be DECREFed after GC track checking is done, if not, it could raise a segmentation fault,
        // when dict only holds the strong reference to value in ep->me_value.
        Py_DECREF(old_value);
    }
    ASSERT_CONSISTENT(mp);
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

    ASSERT_DICT_LOCKED(mp);

    if (DK_IS_UNICODE(mp->ma_keys) && !PyUnicode_CheckExact(key)) {
        if (insertion_resize(interp, mp, 0) < 0)
            goto Fail;
        assert(mp->ma_keys->dk_kind == DICT_KEYS_GENERAL);
    }

    if (_PyDict_HasSplitTable(mp)) {
        Py_ssize_t ix = insert_split_key(mp->ma_keys, key, hash);
        if (ix != DKIX_EMPTY) {
            insert_split_value(interp, mp, key, value, ix);
            Py_DECREF(key);
            Py_DECREF(value);
            return 0;
        }

        /* No space in shared keys. Resize and continue below. */
        if (insertion_resize(interp, mp, 1) < 0) {
            goto Fail;
        }
    }

    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR)
        goto Fail;

    if (ix == DKIX_EMPTY) {
        assert(!_PyDict_HasSplitTable(mp));
        /* Insert into new slot. */
        assert(old_value == NULL);
        if (insert_combined_dict(interp, mp, hash, key, value) < 0) {
            goto Fail;
        }
        STORE_USED(mp, mp->ma_used + 1);
        ASSERT_CONSISTENT(mp);
        return 0;
    }

    if (old_value != value) {
        _PyDict_NotifyEvent(interp, PyDict_EVENT_MODIFIED, mp, key, value);
        assert(old_value != NULL);
        assert(!_PyDict_HasSplitTable(mp));
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[ix];
            STORE_VALUE(ep, value);
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[ix];
            STORE_VALUE(ep, value);
        }
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

// Same as insertdict but specialized for ma_keys == Py_EMPTY_KEYS.
// Consumes key and value references.
static int
insert_to_emptydict(PyInterpreterState *interp, PyDictObject *mp,
                    PyObject *key, Py_hash_t hash, PyObject *value)
{
    assert(mp->ma_keys == Py_EMPTY_KEYS);
    ASSERT_DICT_LOCKED(mp);

    int unicode = PyUnicode_CheckExact(key);
    PyDictKeysObject *newkeys = new_keys_object(
            interp, PyDict_LOG_MINSIZE, unicode);
    if (newkeys == NULL) {
        Py_DECREF(key);
        Py_DECREF(value);
        return -1;
    }
    _PyDict_NotifyEvent(interp, PyDict_EVENT_ADDED, mp, key, value);

    /* We don't decref Py_EMPTY_KEYS here because it is immortal. */
    assert(mp->ma_values == NULL);

    size_t hashpos = (size_t)hash & (PyDict_MINSIZE-1);
    dictkeys_set_index(newkeys, hashpos, 0);
    if (unicode) {
        PyDictUnicodeEntry *ep = DK_UNICODE_ENTRIES(newkeys);
        ep->me_key = key;
        STORE_VALUE(ep, value);
    }
    else {
        PyDictKeyEntry *ep = DK_ENTRIES(newkeys);
        ep->me_key = key;
        ep->me_hash = hash;
        STORE_VALUE(ep, value);
    }
    STORE_USED(mp, mp->ma_used + 1);
    newkeys->dk_usable--;
    newkeys->dk_nentries++;
    // We store the keys last so no one can see them in a partially inconsistent
    // state so that we don't need to switch the keys to being shared yet for
    // the case where we're inserting from the non-owner thread.  We don't use
    // set_keys here because the transition from empty to non-empty is safe
    // as the empty keys will never be freed.
    FT_ATOMIC_STORE_PTR_RELEASE(mp->ma_keys, newkeys);
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

static void
invalidate_and_clear_inline_values(PyDictValues *values)
{
    assert(values->embedded);
    FT_ATOMIC_STORE_UINT8(values->valid, 0);
    for (int i = 0; i < values->capacity; i++) {
        FT_ATOMIC_STORE_PTR_RELEASE(values->values[i], NULL);
    }
}

/*
Restructure the table by allocating a new table and reinserting all
items again.  When entries have been deleted, the new table may
actually be smaller than the old one.
If a table is split (its keys and hashes are shared, its values are not),
then the values are temporarily copied into the table, it is resized as
a combined table, then the me_value slots in the old table are NULLed out.
After resizing, a table is always combined.

This function supports:
 - Unicode split -> Unicode combined or Generic
 - Unicode combined -> Unicode combined or Generic
 - Generic -> Generic
*/
static int
dictresize(PyInterpreterState *interp, PyDictObject *mp,
           uint8_t log2_newsize, int unicode)
{
    PyDictKeysObject *oldkeys, *newkeys;
    PyDictValues *oldvalues;

    ASSERT_DICT_LOCKED(mp);

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

    ensure_shared_on_resize(mp);
    /* NOTE: Current odict checks mp->ma_keys to detect resize happen.
     * So we can't reuse oldkeys even if oldkeys->dk_size == newsize.
     * TODO: Try reusing oldkeys when reimplement odict.
     */

    /* Allocate a new table. */
    newkeys = new_keys_object(interp, log2_newsize, unicode);
    if (newkeys == NULL) {
        return -1;
    }
    // New table must be large enough.
    assert(newkeys->dk_usable >= mp->ma_used);

    Py_ssize_t numentries = mp->ma_used;

    if (oldvalues != NULL) {
        LOCK_KEYS(oldkeys);
        PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
        /* Convert split table into new combined table.
         * We must incref keys; we can transfer values.
         */
        if (newkeys->dk_kind == DICT_KEYS_GENERAL) {
            // split -> generic
            PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);

            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                PyDictUnicodeEntry *ep = &oldentries[index];
                assert(oldvalues->values[index] != NULL);
                newentries[i].me_key = Py_NewRef(ep->me_key);
                newentries[i].me_hash = unicode_get_hash(ep->me_key);
                newentries[i].me_value = oldvalues->values[index];
            }
            build_indices_generic(newkeys, newentries, numentries);
        }
        else { // split -> combined unicode
            PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(newkeys);

            for (Py_ssize_t i = 0; i < numentries; i++) {
                int index = get_index_from_order(mp, i);
                PyDictUnicodeEntry *ep = &oldentries[index];
                assert(oldvalues->values[index] != NULL);
                newentries[i].me_key = Py_NewRef(ep->me_key);
                newentries[i].me_value = oldvalues->values[index];
            }
            build_indices_unicode(newkeys, newentries, numentries);
        }
        UNLOCK_KEYS(oldkeys);
        set_keys(mp, newkeys);
        dictkeys_decref(interp, oldkeys, IS_DICT_SHARED(mp));
        set_values(mp, NULL);
        if (oldvalues->embedded) {
            assert(oldvalues->embedded == 1);
            assert(oldvalues->valid == 1);
            invalidate_and_clear_inline_values(oldvalues);
        }
        else {
            free_values(oldvalues, IS_DICT_SHARED(mp));
        }
    }
    else {  // oldkeys is combined.
        if (oldkeys->dk_kind == DICT_KEYS_GENERAL) {
            // generic -> generic
            assert(newkeys->dk_kind == DICT_KEYS_GENERAL);
            PyDictKeyEntry *oldentries = DK_ENTRIES(oldkeys);
            PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);
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
            build_indices_generic(newkeys, newentries, numentries);
        }
        else {  // oldkeys is combined unicode
            PyDictUnicodeEntry *oldentries = DK_UNICODE_ENTRIES(oldkeys);
            if (unicode) { // combined unicode -> combined unicode
                PyDictUnicodeEntry *newentries = DK_UNICODE_ENTRIES(newkeys);
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
                build_indices_unicode(newkeys, newentries, numentries);
            }
            else { // combined unicode -> generic
                PyDictKeyEntry *newentries = DK_ENTRIES(newkeys);
                PyDictUnicodeEntry *ep = oldentries;
                for (Py_ssize_t i = 0; i < numentries; i++) {
                    while (ep->me_value == NULL)
                        ep++;
                    newentries[i].me_key = ep->me_key;
                    newentries[i].me_hash = unicode_get_hash(ep->me_key);
                    newentries[i].me_value = ep->me_value;
                    ep++;
                }
                build_indices_generic(newkeys, newentries, numentries);
            }
        }

        set_keys(mp, newkeys);

        if (oldkeys != Py_EMPTY_KEYS) {
#ifdef Py_REF_DEBUG
            _Py_DecRefTotal(_PyThreadState_GET());
#endif
            assert(oldkeys->dk_kind != DICT_KEYS_SPLIT);
            assert(oldkeys->dk_refcnt == 1);
            free_keys_object(oldkeys, IS_DICT_SHARED(mp));
        }
    }

    STORE_KEYS_USABLE(mp->ma_keys, mp->ma_keys->dk_usable - numentries);
    STORE_KEYS_NENTRIES(mp->ma_keys, numentries);
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
        if (setitem_lock_held((PyDictObject *)dict, key, value) < 0) {
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
static PyObject *
dict_getitem(PyObject *op, PyObject *key, const char *warnmsg)
{
    if (!PyDict_Check(op)) {
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)op;

    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        PyErr_FormatUnraisable(warnmsg);
        return NULL;
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
#ifdef Py_GIL_DISABLED
    ix = _Py_dict_lookup_threadsafe(mp, key, hash, &value);
    Py_XDECREF(value);
#else
    ix = _Py_dict_lookup(mp, key, hash, &value);
#endif

    /* Ignore any exception raised by the lookup */
    PyObject *exc2 = _PyErr_Occurred(tstate);
    if (exc2 && !PyErr_GivenExceptionMatches(exc2, PyExc_KeyError)) {
        PyErr_FormatUnraisable(warnmsg);
    }
    _PyErr_SetRaisedException(tstate, exc);

    assert(ix >= 0 || value == NULL);
    return value;  // borrowed reference
}

PyObject *
PyDict_GetItem(PyObject *op, PyObject *key)
{
    return dict_getitem(op, key,
            "Exception ignored in PyDict_GetItem(); consider using "
            "PyDict_GetItemRef() or PyDict_GetItemWithError()");
}

static void
dict_unhashable_type(PyObject *key)
{
    PyObject *exc = PyErr_GetRaisedException();
    assert(exc != NULL);
    if (!Py_IS_TYPE(exc, (PyTypeObject*)PyExc_TypeError)) {
        PyErr_SetRaisedException(exc);
        return;
    }

    PyErr_Format(PyExc_TypeError,
                 "cannot use '%T' as a dict key (%S)",
                 key, exc);
    Py_DECREF(exc);
}

Py_ssize_t
_PyDict_LookupIndex(PyDictObject *mp, PyObject *key)
{
    // TODO: Thread safety
    PyObject *value;
    assert(PyDict_CheckExact((PyObject*)mp));
    assert(PyUnicode_CheckExact(key));

    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return -1;
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

#ifdef Py_GIL_DISABLED
    ix = _Py_dict_lookup_threadsafe(mp, key, hash, &value);
    Py_XDECREF(value);
#else
    ix = _Py_dict_lookup(mp, key, hash, &value);
#endif
    assert(ix >= 0 || value == NULL);
    return value;  // borrowed reference
}

/* Gets an item and provides a new reference if the value is present.
 * Returns 1 if the key is present, 0 if the key is missing, and -1 if an
 * exception occurred.
*/
int
_PyDict_GetItemRef_KnownHash_LockHeld(PyDictObject *op, PyObject *key,
                                      Py_hash_t hash, PyObject **result)
{
    PyObject *value;
    Py_ssize_t ix = _Py_dict_lookup(op, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    if (ix == DKIX_ERROR) {
        *result = NULL;
        return -1;
    }
    if (value == NULL) {
        *result = NULL;
        return 0;  // missing key
    }
    *result = Py_NewRef(value);
    return 1;  // key is present
}

/* Gets an item and provides a new reference if the value is present.
 * Returns 1 if the key is present, 0 if the key is missing, and -1 if an
 * exception occurred.
*/
int
_PyDict_GetItemRef_KnownHash(PyDictObject *op, PyObject *key, Py_hash_t hash, PyObject **result)
{
    PyObject *value;
#ifdef Py_GIL_DISABLED
    Py_ssize_t ix = _Py_dict_lookup_threadsafe(op, key, hash, &value);
#else
    Py_ssize_t ix = _Py_dict_lookup(op, key, hash, &value);
#endif
    assert(ix >= 0 || value == NULL);
    if (ix == DKIX_ERROR) {
        *result = NULL;
        return -1;
    }
    if (value == NULL) {
        *result = NULL;
        return 0;  // missing key
    }
#ifdef Py_GIL_DISABLED
    *result = value;
#else
    *result = Py_NewRef(value);
#endif
    return 1;  // key is present
}

int
PyDict_GetItemRef(PyObject *op, PyObject *key, PyObject **result)
{
    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        *result = NULL;
        return -1;
    }

    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        *result = NULL;
        return -1;
    }

    return _PyDict_GetItemRef_KnownHash((PyDictObject *)op, key, hash, result);
}

int
_PyDict_GetItemRef_Unicode_LockHeld(PyDictObject *op, PyObject *key, PyObject **result)
{
    ASSERT_DICT_LOCKED(op);
    assert(PyUnicode_CheckExact(key));

    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        *result = NULL;
        return -1;
    }

    PyObject *value;
    Py_ssize_t ix = _Py_dict_lookup(op, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    if (ix == DKIX_ERROR) {
        *result = NULL;
        return -1;
    }
    if (value == NULL) {
        *result = NULL;
        return 0;  // missing key
    }
    *result = Py_NewRef(value);
    return 1;  // key is present
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
    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return NULL;
    }

#ifdef Py_GIL_DISABLED
    ix = _Py_dict_lookup_threadsafe(mp, key, hash, &value);
    Py_XDECREF(value);
#else
    ix = _Py_dict_lookup(mp, key, hash, &value);
#endif
    assert(ix >= 0 || value == NULL);
    return value;  // borrowed reference
}

PyObject *
_PyDict_GetItemWithError(PyObject *dp, PyObject *kv)
{
    assert(PyUnicode_CheckExact(kv));
    Py_hash_t hash = Py_TYPE(kv)->tp_hash(kv);
    if (hash == -1) {
        return NULL;
    }
    return _PyDict_GetItem_KnownHash(dp, kv, hash);  // borrowed reference
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
    return _PyDict_GetItem_KnownHash(dp, kv, hash);  // borrowed reference
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
 *
 * Returns a new reference.
 */
PyObject *
_PyDict_LoadGlobal(PyDictObject *globals, PyDictObject *builtins, PyObject *key)
{
    Py_ssize_t ix;
    Py_hash_t hash;
    PyObject *value;

    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        return NULL;
    }

    /* namespace 1: globals */
    ix = _Py_dict_lookup_threadsafe(globals, key, hash, &value);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix != DKIX_EMPTY && value != NULL)
        return value;

    /* namespace 2: builtins */
    ix = _Py_dict_lookup_threadsafe(builtins, key, hash, &value);
    assert(ix >= 0 || value == NULL);
    return value;
}

void
_PyDict_LoadGlobalStackRef(PyDictObject *globals, PyDictObject *builtins, PyObject *key, _PyStackRef *res)
{
    Py_ssize_t ix;
    Py_hash_t hash;

    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        *res = PyStackRef_NULL;
        return;
    }

    /* namespace 1: globals */
    ix = _Py_dict_lookup_threadsafe_stackref(globals, key, hash, res);
    if (ix == DKIX_ERROR) {
        return;
    }
    if (ix != DKIX_EMPTY && !PyStackRef_IsNull(*res)) {
        return;
    }

    /* namespace 2: builtins */
    ix = _Py_dict_lookup_threadsafe_stackref(builtins, key, hash, res);
    assert(ix >= 0 || PyStackRef_IsNull(*res));
}

PyObject *
_PyDict_LoadBuiltinsFromGlobals(PyObject *globals)
{
    if (!PyDict_Check(globals)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PyDictObject *mp = (PyDictObject *)globals;
    PyObject *key = &_Py_ID(__builtins__);
    Py_hash_t hash = unicode_get_hash(key);

    // Use the stackref variant to avoid reference count contention on the
    // builtins module in the free threading build. It's important not to
    // make any escaping calls between the lookup and the `PyStackRef_CLOSE()`
    // because the `ref` is not visible to the GC.
    _PyStackRef ref;
    Py_ssize_t ix = _Py_dict_lookup_threadsafe_stackref(mp, key, hash, &ref);
    if (ix == DKIX_ERROR) {
        return NULL;
    }
    if (PyStackRef_IsNull(ref)) {
        return Py_NewRef(PyEval_GetBuiltins());
    }
    PyObject *builtins = PyStackRef_AsPyObjectBorrow(ref);
    if (PyModule_Check(builtins)) {
        builtins = _PyModule_GetDict(builtins);
        assert(builtins != NULL);
    }
    _Py_INCREF_BUILTINS(builtins);
    PyStackRef_CLOSE(ref);
    return builtins;
}

/* Consumes references to key and value */
static int
setitem_take2_lock_held(PyDictObject *mp, PyObject *key, PyObject *value)
{
    ASSERT_DICT_LOCKED(mp);

    assert(key);
    assert(value);
    assert(PyDict_Check(mp));
    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        Py_DECREF(key);
        Py_DECREF(value);
        return -1;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();

    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return insert_to_emptydict(interp, mp, key, hash, value);
    }
    /* insertdict() handles any resizing that might be necessary */
    return insertdict(interp, mp, key, hash, value);
}

int
_PyDict_SetItem_Take2(PyDictObject *mp, PyObject *key, PyObject *value)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(mp);
    res = setitem_take2_lock_held(mp, key, value);
    Py_END_CRITICAL_SECTION();
    return res;
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

static int
setitem_lock_held(PyDictObject *mp, PyObject *key, PyObject *value)
{
    assert(key);
    assert(value);
    return setitem_take2_lock_held(mp,
                                   Py_NewRef(key), Py_NewRef(value));
}


int
_PyDict_SetItem_KnownHash_LockHeld(PyDictObject *mp, PyObject *key, PyObject *value,
                                   Py_hash_t hash)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (mp->ma_keys == Py_EMPTY_KEYS) {
        return insert_to_emptydict(interp, mp, Py_NewRef(key), hash, Py_NewRef(value));
    }
    /* insertdict() handles any resizing that might be necessary */
    return insertdict(interp, mp, Py_NewRef(key), hash, Py_NewRef(value));
}

int
_PyDict_SetItem_KnownHash(PyObject *op, PyObject *key, PyObject *value,
                          Py_hash_t hash)
{
    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    assert(key);
    assert(value);
    assert(hash != -1);

    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = _PyDict_SetItem_KnownHash_LockHeld((PyDictObject *)op, key, value, hash);
    Py_END_CRITICAL_SECTION();
    return res;
}

static void
delete_index_from_values(PyDictValues *values, Py_ssize_t ix)
{
    uint8_t *array = get_insertion_order_array(values);
    int size = values->size;
    assert(size <= values->capacity);
    int i;
    for (i = 0; array[i] != ix; i++) {
        assert(i < size);
    }
    assert(i < size);
    size--;
    for (; i < size; i++) {
        array[i] = array[i+1];
    }
    values->size = size;
}

static void
delitem_common(PyDictObject *mp, Py_hash_t hash, Py_ssize_t ix,
               PyObject *old_value)
{
    PyObject *old_key;

    ASSERT_DICT_LOCKED(mp);

    Py_ssize_t hashpos = lookdict_index(mp->ma_keys, hash, ix);
    assert(hashpos >= 0);

    STORE_USED(mp, mp->ma_used - 1);
    if (_PyDict_HasSplitTable(mp)) {
        assert(old_value == mp->ma_values->values[ix]);
        STORE_SPLIT_VALUE(mp, ix, NULL);
        assert(ix < SHARED_KEYS_MAX_SIZE);
        /* Update order */
        delete_index_from_values(mp->ma_values, ix);
        ASSERT_CONSISTENT(mp);
    }
    else {
        FT_ATOMIC_STORE_UINT32_RELAXED(mp->ma_keys->dk_version, 0);
        dictkeys_set_index(mp->ma_keys, hashpos, DKIX_DUMMY);
        if (DK_IS_UNICODE(mp->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(mp->ma_keys)[ix];
            old_key = ep->me_key;
            STORE_KEY(ep, NULL);
            STORE_VALUE(ep, NULL);
        }
        else {
            PyDictKeyEntry *ep = &DK_ENTRIES(mp->ma_keys)[ix];
            old_key = ep->me_key;
            STORE_KEY(ep, NULL);
            STORE_VALUE(ep, NULL);
            STORE_HASH(ep, 0);
        }
        Py_DECREF(old_key);
    }
    Py_DECREF(old_value);

    ASSERT_CONSISTENT(mp);
}

int
PyDict_DelItem(PyObject *op, PyObject *key)
{
    assert(key);
    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return -1;
    }

    return _PyDict_DelItem_KnownHash(op, key, hash);
}

static int
delitem_knownhash_lock_held(PyObject *op, PyObject *key, Py_hash_t hash)
{
    Py_ssize_t ix;
    PyDictObject *mp;
    PyObject *old_value;

    if (!PyDict_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }

    ASSERT_DICT_LOCKED(op);

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
    _PyDict_NotifyEvent(interp, PyDict_EVENT_DELETED, mp, key, NULL);
    delitem_common(mp, hash, ix, old_value);
    return 0;
}

int
_PyDict_DelItem_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = delitem_knownhash_lock_held(op, key, hash);
    Py_END_CRITICAL_SECTION();
    return res;
}

static int
delitemif_lock_held(PyObject *op, PyObject *key,
                    int (*predicate)(PyObject *value, void *arg),
                    void *arg)
{
    Py_ssize_t ix;
    PyDictObject *mp;
    Py_hash_t hash;
    PyObject *old_value;
    int res;

    ASSERT_DICT_LOCKED(op);

    assert(key);
    hash = PyObject_Hash(key);
    if (hash == -1)
        return -1;
    mp = (PyDictObject *)op;
    ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR) {
        return -1;
    }
    if (ix == DKIX_EMPTY || old_value == NULL) {
        return 0;
    }

    res = predicate(old_value, arg);
    if (res == -1)
        return -1;

    if (res > 0) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        _PyDict_NotifyEvent(interp, PyDict_EVENT_DELETED, mp, key, NULL);
        delitem_common(mp, hash, ix, old_value);
        return 1;
    } else {
        return 0;
    }
}
/* This function promises that the predicate -> deletion sequence is atomic
 * (i.e. protected by the GIL or the per-dict mutex in free threaded builds),
 * assuming the predicate itself doesn't release the GIL (or cause re-entrancy
 * which would release the per-dict mutex)
 */
int
_PyDict_DelItemIf(PyObject *op, PyObject *key,
                  int (*predicate)(PyObject *value, void *arg),
                  void *arg)
{
    assert(PyDict_Check(op));
    int res;
    Py_BEGIN_CRITICAL_SECTION(op);
    res = delitemif_lock_held(op, key, predicate, arg);
    Py_END_CRITICAL_SECTION();
    return res;
}

static void
clear_lock_held(PyObject *op)
{
    PyDictObject *mp;
    PyDictKeysObject *oldkeys;
    PyDictValues *oldvalues;
    Py_ssize_t i, n;

    ASSERT_DICT_LOCKED(op);

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
    _PyDict_NotifyEvent(interp, PyDict_EVENT_CLEARED, mp, NULL, NULL);
    // We don't inc ref empty keys because they're immortal
    ensure_shared_on_resize(mp);
    STORE_USED(mp, 0);
    if (oldvalues == NULL) {
        set_keys(mp, Py_EMPTY_KEYS);
        assert(oldkeys->dk_refcnt == 1);
        dictkeys_decref(interp, oldkeys, IS_DICT_SHARED(mp));
    }
    else {
        n = oldkeys->dk_nentries;
        for (i = 0; i < n; i++) {
            Py_CLEAR(oldvalues->values[i]);
        }
        if (oldvalues->embedded) {
            oldvalues->size = 0;
        }
        else {
            set_values(mp, NULL);
            set_keys(mp, Py_EMPTY_KEYS);
            free_values(oldvalues, IS_DICT_SHARED(mp));
            dictkeys_decref(interp, oldkeys, false);
        }
    }
    ASSERT_CONSISTENT(mp);
}

void
_PyDict_Clear_LockHeld(PyObject *op) {
    clear_lock_held(op);
}

void
PyDict_Clear(PyObject *op)
{
    Py_BEGIN_CRITICAL_SECTION(op);
    clear_lock_held(op);
    Py_END_CRITICAL_SECTION();
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
    if (_PyDict_HasSplitTable(mp)) {
        assert(mp->ma_used <= SHARED_KEYS_MAX_SIZE);
        if (i < 0 || i >= mp->ma_used)
            return 0;
        int index = get_index_from_order(mp, i);
        value = mp->ma_values->values[index];
        key = LOAD_SHARED_KEY(DK_UNICODE_ENTRIES(mp->ma_keys)[index].me_key);
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
int
_PyDict_Pop_KnownHash(PyDictObject *mp, PyObject *key, Py_hash_t hash,
                      PyObject **result)
{
    assert(PyDict_Check(mp));

    ASSERT_DICT_LOCKED(mp);

    if (mp->ma_used == 0) {
        if (result) {
            *result = NULL;
        }
        return 0;
    }

    PyObject *old_value;
    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &old_value);
    if (ix == DKIX_ERROR) {
        if (result) {
            *result = NULL;
        }
        return -1;
    }

    if (ix == DKIX_EMPTY || old_value == NULL) {
        if (result) {
            *result = NULL;
        }
        return 0;
    }

    assert(old_value != NULL);
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyDict_NotifyEvent(interp, PyDict_EVENT_DELETED, mp, key, NULL);
    delitem_common(mp, hash, ix, Py_NewRef(old_value));

    ASSERT_CONSISTENT(mp);
    if (result) {
        *result = old_value;
    }
    else {
        Py_DECREF(old_value);
    }
    return 1;
}

static int
pop_lock_held(PyObject *op, PyObject *key, PyObject **result)
{
    ASSERT_DICT_LOCKED(op);

    if (!PyDict_Check(op)) {
        if (result) {
            *result = NULL;
        }
        PyErr_BadInternalCall();
        return -1;
    }
    PyDictObject *dict = (PyDictObject *)op;

    if (dict->ma_used == 0) {
        if (result) {
            *result = NULL;
        }
        return 0;
    }

    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        if (result) {
            *result = NULL;
        }
        return -1;
    }
    return _PyDict_Pop_KnownHash(dict, key, hash, result);
}

int
PyDict_Pop(PyObject *op, PyObject *key, PyObject **result)
{
    int err;
    Py_BEGIN_CRITICAL_SECTION(op);
    err = pop_lock_held(op, key, result);
    Py_END_CRITICAL_SECTION();

    return err;
}


int
PyDict_PopString(PyObject *op, const char *key, PyObject **result)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        if (result != NULL) {
            *result = NULL;
        }
        return -1;
    }

    int res = PyDict_Pop(op, key_obj, result);
    Py_DECREF(key_obj);
    return res;
}


static PyObject *
dict_pop_default(PyObject *dict, PyObject *key, PyObject *default_value)
{
    PyObject *result;
    if (PyDict_Pop(dict, key, &result) == 0) {
        if (default_value != NULL) {
            return Py_NewRef(default_value);
        }
        _PyErr_SetKeyError(key);
        return NULL;
    }
    return result;
}

PyObject *
_PyDict_Pop(PyObject *dict, PyObject *key, PyObject *default_value)
{
    return dict_pop_default(dict, key, default_value);
}

static PyDictObject *
dict_dict_fromkeys(PyInterpreterState *interp, PyDictObject *mp,
                   PyObject *iterable, PyObject *value)
{
    PyObject *oldvalue;
    Py_ssize_t pos = 0;
    PyObject *key;
    Py_hash_t hash;
    int unicode = DK_IS_UNICODE(((PyDictObject*)iterable)->ma_keys);
    uint8_t new_size = Py_MAX(
        estimate_log2_keysize(PyDict_GET_SIZE(iterable)),
        DK_LOG_SIZE(mp->ma_keys));
    if (dictresize(interp, mp, new_size, unicode)) {
        Py_DECREF(mp);
        return NULL;
    }

    while (_PyDict_Next(iterable, &pos, &key, &oldvalue, &hash)) {
        if (insertdict(interp, mp,
                        Py_NewRef(key), hash, Py_NewRef(value))) {
            Py_DECREF(mp);
            return NULL;
        }
    }
    return mp;
}

static PyDictObject *
dict_set_fromkeys(PyInterpreterState *interp, PyDictObject *mp,
                  PyObject *iterable, PyObject *value)
{
    Py_ssize_t pos = 0;
    PyObject *key;
    Py_hash_t hash;
    uint8_t new_size = Py_MAX(
        estimate_log2_keysize(PySet_GET_SIZE(iterable)),
        DK_LOG_SIZE(mp->ma_keys));
    if (dictresize(interp, mp, new_size, 0)) {
        Py_DECREF(mp);
        return NULL;
    }

    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(iterable);
    while (_PySet_NextEntryRef(iterable, &pos, &key, &hash)) {
        if (insertdict(interp, mp, key, hash, Py_NewRef(value))) {
            Py_DECREF(mp);
            return NULL;
        }
    }
    return mp;
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


    if (PyDict_CheckExact(d)) {
        if (PyDict_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION2(d, iterable);
            d = (PyObject *)dict_dict_fromkeys(interp, mp, iterable, value);
            Py_END_CRITICAL_SECTION2();
            return d;
        }
        else if (PyAnySet_CheckExact(iterable)) {
            PyDictObject *mp = (PyDictObject *)d;

            Py_BEGIN_CRITICAL_SECTION2(d, iterable);
            d = (PyObject *)dict_set_fromkeys(interp, mp, iterable, value);
            Py_END_CRITICAL_SECTION2();
            return d;
        }
    }

    it = PyObject_GetIter(iterable);
    if (it == NULL){
        Py_DECREF(d);
        return NULL;
    }

    if (PyDict_CheckExact(d)) {
        Py_BEGIN_CRITICAL_SECTION(d);
        while ((key = PyIter_Next(it)) != NULL) {
            status = setitem_lock_held((PyDictObject *)d, key, value);
            Py_DECREF(key);
            if (status < 0) {
                assert(PyErr_Occurred());
                goto dict_iter_exit;
            }
        }
dict_iter_exit:;
        Py_END_CRITICAL_SECTION();
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
dict_dealloc(PyObject *self)
{
    PyDictObject *mp = (PyDictObject *)self;
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyObject_ResurrectStart(self);
    _PyDict_NotifyEvent(interp, PyDict_EVENT_DEALLOCATED, mp, NULL, NULL);
    if (_PyObject_ResurrectEnd(self)) {
        return;
    }
    PyDictValues *values = mp->ma_values;
    PyDictKeysObject *keys = mp->ma_keys;
    Py_ssize_t i, n;

    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(mp);
    if (values != NULL) {
        if (values->embedded == 0) {
            for (i = 0, n = values->capacity; i < n; i++) {
                Py_XDECREF(values->values[i]);
            }
            free_values(values, false);
        }
        dictkeys_decref(interp, keys, false);
    }
    else if (keys != NULL) {
        assert(keys->dk_refcnt == 1 || keys == Py_EMPTY_KEYS);
        dictkeys_decref(interp, keys, false);
    }
    if (Py_IS_TYPE(mp, &PyDict_Type)) {
        _Py_FREELIST_FREE(dicts, mp, Py_TYPE(mp)->tp_free);
    }
    else {
        Py_TYPE(mp)->tp_free((PyObject *)mp);
    }
}


static PyObject *
dict_repr_lock_held(PyObject *self)
{
    PyDictObject *mp = (PyDictObject *)self;
    PyObject *key = NULL, *value = NULL;
    ASSERT_DICT_LOCKED(mp);

    int res = Py_ReprEnter((PyObject *)mp);
    if (res != 0) {
        return (res > 0 ? PyUnicode_FromString("{...}") : NULL);
    }

    if (mp->ma_used == 0) {
        Py_ReprLeave((PyObject *)mp);
        return PyUnicode_FromString("{}");
    }

    // "{" + "1: 2" + ", 3: 4" * (len - 1) + "}"
    Py_ssize_t prealloc = 1 + 4 + 6 * (mp->ma_used - 1) + 1;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(prealloc);
    if (writer == NULL) {
        goto error;
    }

    if (PyUnicodeWriter_WriteChar(writer, '{') < 0) {
        goto error;
    }

    /* Do repr() on each key+value pair, and insert ": " between them.
       Note that repr may mutate the dict. */
    Py_ssize_t i = 0;
    int first = 1;
    while (_PyDict_Next((PyObject *)mp, &i, &key, &value, NULL)) {
        // Prevent repr from deleting key or value during key format.
        Py_INCREF(key);
        Py_INCREF(value);

        if (!first) {
            // Write ", "
            if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
                goto error;
            }
            if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) {
                goto error;
            }
        }
        first = 0;

        // Write repr(key)
        if (PyUnicodeWriter_WriteRepr(writer, key) < 0) {
            goto error;
        }

        // Write ": "
        if (PyUnicodeWriter_WriteChar(writer, ':') < 0) {
            goto error;
        }
        if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) {
            goto error;
        }

        // Write repr(value)
        if (PyUnicodeWriter_WriteRepr(writer, value) < 0) {
            goto error;
        }

        Py_CLEAR(key);
        Py_CLEAR(value);
    }

    if (PyUnicodeWriter_WriteChar(writer, '}') < 0) {
        goto error;
    }

    Py_ReprLeave((PyObject *)mp);

    return PyUnicodeWriter_Finish(writer);

error:
    Py_ReprLeave((PyObject *)mp);
    PyUnicodeWriter_Discard(writer);
    Py_XDECREF(key);
    Py_XDECREF(value);
    return NULL;
}

static PyObject *
dict_repr(PyObject *self)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(self);
    res = dict_repr_lock_held(self);
    Py_END_CRITICAL_SECTION();
    return res;
}

static Py_ssize_t
dict_length(PyObject *self)
{
    return FT_ATOMIC_LOAD_SSIZE_RELAXED(((PyDictObject *)self)->ma_used);
}

static PyObject *
dict_subscript(PyObject *self, PyObject *key)
{
    PyDictObject *mp = (PyDictObject *)self;
    Py_ssize_t ix;
    Py_hash_t hash;
    PyObject *value;

    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return NULL;
    }
    ix = _Py_dict_lookup_threadsafe(mp, key, hash, &value);
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
    return value;
}

static int
dict_ass_sub(PyObject *mp, PyObject *v, PyObject *w)
{
    if (w == NULL)
        return PyDict_DelItem(mp, v);
    else
        return PyDict_SetItem(mp, v, w);
}

static PyMappingMethods dict_as_mapping = {
    dict_length, /*mp_length*/
    dict_subscript, /*mp_subscript*/
    dict_ass_sub, /*mp_ass_subscript*/
};

static PyObject *
keys_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);

    if (dict == NULL || !PyDict_Check(dict)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)dict;
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

PyObject *
PyDict_Keys(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = keys_lock_held(dict);
    Py_END_CRITICAL_SECTION();

    return res;
}

static PyObject *
values_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);

    if (dict == NULL || !PyDict_Check(dict)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)dict;
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

PyObject *
PyDict_Values(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = values_lock_held(dict);
    Py_END_CRITICAL_SECTION();
    return res;
}

static PyObject *
items_lock_held(PyObject *dict)
{
    ASSERT_DICT_LOCKED(dict);

    if (dict == NULL || !PyDict_Check(dict)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyDictObject *mp = (PyDictObject *)dict;
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

PyObject *
PyDict_Items(PyObject *dict)
{
    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    res = items_lock_held(dict);
    Py_END_CRITICAL_SECTION();

    return res;
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
    int has_keys = PyObject_HasAttrWithError(arg, &_Py_ID(keys));
    if (has_keys < 0) {
        return -1;
    }
    if (has_keys) {
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

static int
merge_from_seq2_lock_held(PyObject *d, PyObject *seq2, int override)
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
        fast = PySequence_Fast(item, "object is not iterable");
        if (fast == NULL) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                _PyErr_FormatNote(
                    "Cannot convert dictionary update "
                    "sequence element #%zd to a sequence",
                    i);
            }
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
            if (setitem_lock_held((PyDictObject *)d, key, value) < 0) {
                Py_DECREF(key);
                Py_DECREF(value);
                goto Fail;
            }
        }
        else {
            if (dict_setdefault_ref_lock_held(d, key, value, NULL, 0) < 0) {
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

int
PyDict_MergeFromSeq2(PyObject *d, PyObject *seq2, int override)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(d);
    res = merge_from_seq2_lock_held(d, seq2, override);
    Py_END_CRITICAL_SECTION();

    return res;
}

static int
dict_dict_merge(PyInterpreterState *interp, PyDictObject *mp, PyDictObject *other, int override)
{
    ASSERT_DICT_LOCKED(mp);
    ASSERT_DICT_LOCKED(other);

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
        if (mp->ma_values == NULL &&
            other->ma_values == NULL &&
            other->ma_used == okeys->dk_nentries &&
            (DK_LOG_SIZE(okeys) == PyDict_LOG_MINSIZE ||
             USABLE_FRACTION(DK_SIZE(okeys)/2) < other->ma_used)
        ) {
            _PyDict_NotifyEvent(interp, PyDict_EVENT_CLONED, mp, (PyObject *)other, NULL);
            PyDictKeysObject *keys = clone_combined_dict_keys(other);
            if (keys == NULL)
                return -1;

            ensure_shared_on_resize(mp);
            dictkeys_decref(interp, mp->ma_keys, IS_DICT_SHARED(mp));
            set_keys(mp, keys);
            STORE_USED(mp, other->ma_used);
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
            err = _PyDict_Contains_KnownHash((PyObject *)mp, key, hash);
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
    return 0;
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
    int res = 0;
    if (PyDict_Check(b) && (Py_TYPE(b)->tp_iter == dict_iter)) {
        other = (PyDictObject*)b;
        int res;
        Py_BEGIN_CRITICAL_SECTION2(a, b);
        res = dict_dict_merge(interp, (PyDictObject *)a, other, override);
        ASSERT_CONSISTENT(a);
        Py_END_CRITICAL_SECTION2();
        return res;
    }
    else {
        /* Do it the generic, slower way */
        Py_BEGIN_CRITICAL_SECTION(a);
        PyObject *keys = PyMapping_Keys(b);
        PyObject *iter;
        PyObject *key, *value;
        int status;

        if (keys == NULL) {
            /* Docstring says this is equivalent to E.keys() so
             * if E doesn't have a .keys() method we want
             * AttributeError to percolate up.  Might as well
             * do the same for any other error.
             */
            res = -1;
            goto slow_exit;
        }

        iter = PyObject_GetIter(keys);
        Py_DECREF(keys);
        if (iter == NULL) {
            res = -1;
            goto slow_exit;
        }

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
                    res = -1;
                    goto slow_exit;
                }
            }
            value = PyObject_GetItem(b, key);
            if (value == NULL) {
                Py_DECREF(iter);
                Py_DECREF(key);
                res = -1;
                goto slow_exit;
            }
            status = setitem_lock_held(mp, key, value);
            Py_DECREF(key);
            Py_DECREF(value);
            if (status < 0) {
                Py_DECREF(iter);
                res = -1;
                goto slow_exit;
                return -1;
            }
        }
        Py_DECREF(iter);
        if (PyErr_Occurred()) {
            /* Iterator completed, via error */
            res = -1;
            goto slow_exit;
        }

slow_exit:
        ASSERT_CONSISTENT(a);
        Py_END_CRITICAL_SECTION();
        return res;
    }
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

/*[clinic input]
dict.copy

Return a shallow copy of the dict.
[clinic start generated code]*/

static PyObject *
dict_copy_impl(PyDictObject *self)
/*[clinic end generated code: output=ffb782cf970a5c39 input=73935f042b639de4]*/
{
    return PyDict_Copy((PyObject *)self);
}

/* Copies the values, but does not change the reference
 * counts of the objects in the array.
 * Return NULL, but does *not* set an exception on failure  */
static PyDictValues *
copy_values(PyDictValues *values)
{
    PyDictValues *newvalues = new_values(values->capacity);
    if (newvalues == NULL) {
        return NULL;
    }
    newvalues->size = values->size;
    uint8_t *values_order = get_insertion_order_array(values);
    uint8_t *new_values_order = get_insertion_order_array(newvalues);
    memcpy(new_values_order, values_order, values->capacity);
    for (int i = 0; i < values->capacity; i++) {
        newvalues->values[i] = values->values[i];
    }
    assert(newvalues->embedded == 0);
    return newvalues;
}

static PyObject *
copy_lock_held(PyObject *o)
{
    PyObject *copy;
    PyDictObject *mp;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    ASSERT_DICT_LOCKED(o);

    mp = (PyDictObject *)o;
    if (mp->ma_used == 0) {
        /* The dict is empty; just return a new dict. */
        return PyDict_New();
    }

    if (_PyDict_HasSplitTable(mp)) {
        PyDictObject *split_copy;
        PyDictValues *newvalues = copy_values(mp->ma_values);
        if (newvalues == NULL) {
            return PyErr_NoMemory();
        }
        split_copy = PyObject_GC_New(PyDictObject, &PyDict_Type);
        if (split_copy == NULL) {
            free_values(newvalues, false);
            return NULL;
        }
        for (size_t i = 0; i < newvalues->capacity; i++) {
            Py_XINCREF(newvalues->values[i]);
        }
        split_copy->ma_values = newvalues;
        split_copy->ma_keys = mp->ma_keys;
        split_copy->ma_used = mp->ma_used;
        split_copy->_ma_watcher_tag = 0;
        dictkeys_incref(mp->ma_keys);
        _PyObject_GC_TRACK(split_copy);
        return (PyObject *)split_copy;
    }

    if (Py_TYPE(mp)->tp_iter == dict_iter &&
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

PyObject *
PyDict_Copy(PyObject *o)
{
    if (o == NULL || !PyDict_Check(o)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION(o);

    res = copy_lock_held(o);

    Py_END_CRITICAL_SECTION();
    return res;
}

Py_ssize_t
PyDict_Size(PyObject *mp)
{
    if (mp == NULL || !PyDict_Check(mp)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return FT_ATOMIC_LOAD_SSIZE_RELAXED(((PyDictObject *)mp)->ma_used);
}

/* Return 1 if dicts equal, 0 if not, -1 if error.
 * Gets out as soon as any difference is detected.
 * Uses only Py_EQ comparison.
 */
static int
dict_equal_lock_held(PyDictObject *a, PyDictObject *b)
{
    Py_ssize_t i;

    ASSERT_DICT_LOCKED(a);
    ASSERT_DICT_LOCKED(b);

    if (a->ma_used != b->ma_used)
        /* can't be equal if # of entries differ */
        return 0;
    /* Same # of entries -- check all of 'em.  Exit early on any diff. */
    for (i = 0; i < LOAD_KEYS_NENTRIES(a->ma_keys); i++) {
        PyObject *key, *aval;
        Py_hash_t hash;
        if (DK_IS_UNICODE(a->ma_keys)) {
            PyDictUnicodeEntry *ep = &DK_UNICODE_ENTRIES(a->ma_keys)[i];
            key = ep->me_key;
            if (key == NULL) {
                continue;
            }
            hash = unicode_get_hash(key);
            if (_PyDict_HasSplitTable(a))
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

static int
dict_equal(PyDictObject *a, PyDictObject *b)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION2(a, b);
    res = dict_equal_lock_held(a, b);
    Py_END_CRITICAL_SECTION2();

    return res;
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
dict___contains___impl(PyDictObject *self, PyObject *key)
/*[clinic end generated code: output=1b314e6da7687dae input=fe1cb42ad831e820]*/
{
    int contains = PyDict_Contains((PyObject *)self, key);
    if (contains < 0) {
        return NULL;
    }
    if (contains) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
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

    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return NULL;
    }
    ix = _Py_dict_lookup_threadsafe(self, key, hash, &val);
    if (ix == DKIX_ERROR)
        return NULL;
    if (ix == DKIX_EMPTY || val == NULL) {
        val = Py_NewRef(default_value);
    }
    return val;
}

static int
dict_setdefault_ref_lock_held(PyObject *d, PyObject *key, PyObject *default_value,
                    PyObject **result, int incref_result)
{
    PyDictObject *mp = (PyDictObject *)d;
    PyObject *value;
    Py_hash_t hash;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    ASSERT_DICT_LOCKED(d);

    if (!PyDict_Check(d)) {
        PyErr_BadInternalCall();
        if (result) {
            *result = NULL;
        }
        return -1;
    }

    hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        if (result) {
            *result = NULL;
        }
        return -1;
    }

    if (mp->ma_keys == Py_EMPTY_KEYS) {
        if (insert_to_emptydict(interp, mp, Py_NewRef(key), hash,
                                Py_NewRef(default_value)) < 0) {
            if (result) {
                *result = NULL;
            }
            return -1;
        }
        if (result) {
            *result = incref_result ? Py_NewRef(default_value) : default_value;
        }
        return 0;
    }

    if (!PyUnicode_CheckExact(key) && DK_IS_UNICODE(mp->ma_keys)) {
        if (insertion_resize(interp, mp, 0) < 0) {
            if (result) {
                *result = NULL;
            }
            return -1;
        }
    }

    if (_PyDict_HasSplitTable(mp)) {
        Py_ssize_t ix = insert_split_key(mp->ma_keys, key, hash);
        if (ix != DKIX_EMPTY) {
            PyObject *value = mp->ma_values->values[ix];
            int already_present = value != NULL;
            if (!already_present) {
                insert_split_value(interp, mp, key, default_value, ix);
                value = default_value;
            }
            if (result) {
                *result = incref_result ? Py_NewRef(value) : value;
            }
            return already_present;
        }

        /* No space in shared keys. Resize and continue below. */
        if (insertion_resize(interp, mp, 1) < 0) {
            goto error;
        }
    }

    assert(!_PyDict_HasSplitTable(mp));

    Py_ssize_t ix = _Py_dict_lookup(mp, key, hash, &value);
    if (ix == DKIX_ERROR) {
        if (result) {
            *result = NULL;
        }
        return -1;
    }

    if (ix == DKIX_EMPTY) {
        assert(!_PyDict_HasSplitTable(mp));
        value = default_value;

        if (insert_combined_dict(interp, mp, hash, Py_NewRef(key), Py_NewRef(value)) < 0) {
            Py_DECREF(key);
            Py_DECREF(value);
            if (result) {
                *result = NULL;
            }
        }

        STORE_USED(mp, mp->ma_used + 1);
        assert(mp->ma_keys->dk_usable >= 0);
        ASSERT_CONSISTENT(mp);
        if (result) {
            *result = incref_result ? Py_NewRef(value) : value;
        }
        return 0;
    }

    assert(value != NULL);
    ASSERT_CONSISTENT(mp);
    if (result) {
        *result = incref_result ? Py_NewRef(value) : value;
    }
    return 1;

error:
    if (result) {
        *result = NULL;
    }
    return -1;
}

int
PyDict_SetDefaultRef(PyObject *d, PyObject *key, PyObject *default_value,
                     PyObject **result)
{
    int res;
    Py_BEGIN_CRITICAL_SECTION(d);
    res = dict_setdefault_ref_lock_held(d, key, default_value, result, 1);
    Py_END_CRITICAL_SECTION();
    return res;
}

PyObject *
PyDict_SetDefault(PyObject *d, PyObject *key, PyObject *defaultobj)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(d);
    dict_setdefault_ref_lock_held(d, key, defaultobj, &result, 0);
    Py_END_CRITICAL_SECTION();
    return result;
}

/*[clinic input]
@critical_section
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
/*[clinic end generated code: output=f8c1101ebf69e220 input=9237af9a0a224302]*/
{
    PyObject *val;
    dict_setdefault_ref_lock_held((PyObject *)self, key, default_value, &val, 1);
    return val;
}


/*[clinic input]
dict.clear

Remove all items from the dict.
[clinic start generated code]*/

static PyObject *
dict_clear_impl(PyDictObject *self)
/*[clinic end generated code: output=5139a830df00830a input=0bf729baba97a4c2]*/
{
    PyDict_Clear((PyObject *)self);
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
    return dict_pop_default((PyObject*)self, key, default_value);
}

/*[clinic input]
@critical_section
dict.popitem

Remove and return a (key, value) pair as a 2-tuple.

Pairs are returned in LIFO (last-in, first-out) order.
Raises KeyError if the dict is empty.
[clinic start generated code]*/

static PyObject *
dict_popitem_impl(PyDictObject *self)
/*[clinic end generated code: output=e65fcb04420d230d input=ef28b4da5f0f762e]*/
{
    Py_ssize_t i, j;
    PyObject *res;
    PyInterpreterState *interp = _PyInterpreterState_GET();

    ASSERT_DICT_LOCKED(self);

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
    if (_PyDict_HasSplitTable(self)) {
        if (dictresize(interp, self, DK_LOG_SIZE(self->ma_keys), 1) < 0) {
            Py_DECREF(res);
            return NULL;
        }
    }
    FT_ATOMIC_STORE_UINT32_RELAXED(self->ma_keys->dk_version, 0);

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
        _PyDict_NotifyEvent(interp, PyDict_EVENT_DELETED, self, key, NULL);
        hash = unicode_get_hash(key);
        value = ep0[i].me_value;
        STORE_KEY(&ep0[i], NULL);
        STORE_VALUE(&ep0[i], NULL);
    }
    else {
        PyDictKeyEntry *ep0 = DK_ENTRIES(self->ma_keys);
        i = self->ma_keys->dk_nentries - 1;
        while (i >= 0 && ep0[i].me_value == NULL) {
            i--;
        }
        assert(i >= 0);

        key = ep0[i].me_key;
        _PyDict_NotifyEvent(interp, PyDict_EVENT_DELETED, self, key, NULL);
        hash = ep0[i].me_hash;
        value = ep0[i].me_value;
        STORE_KEY(&ep0[i], NULL);
        STORE_HASH(&ep0[i], -1);
        STORE_VALUE(&ep0[i], NULL);
    }

    j = lookdict_index(self->ma_keys, hash, i);
    assert(j >= 0);
    assert(dictkeys_get_index(self->ma_keys, j) == i);
    dictkeys_set_index(self->ma_keys, j, DKIX_DUMMY);

    PyTuple_SET_ITEM(res, 0, key);
    PyTuple_SET_ITEM(res, 1, value);
    /* We can't dk_usable++ since there is DKIX_DUMMY in indices */
    STORE_KEYS_NENTRIES(self->ma_keys, i);
    STORE_USED(self, self->ma_used - 1);
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
        if (_PyDict_HasSplitTable(mp)) {
            if (!mp->ma_values->embedded) {
                for (i = 0; i < n; i++) {
                    Py_VISIT(mp->ma_values->values[i]);
                }
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

static Py_ssize_t
sizeof_lock_held(PyDictObject *mp)
{
    size_t res = _PyObject_SIZE(Py_TYPE(mp));
    if (_PyDict_HasSplitTable(mp)) {
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

Py_ssize_t
_PyDict_SizeOf(PyDictObject *mp)
{
    Py_ssize_t res;
    Py_BEGIN_CRITICAL_SECTION(mp);
    res = sizeof_lock_held(mp);
    Py_END_CRITICAL_SECTION();

    return res;
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

/*[clinic input]
dict.__sizeof__

Return the size of the dict in memory, in bytes.
[clinic start generated code]*/

static PyObject *
dict___sizeof___impl(PyDictObject *self)
/*[clinic end generated code: output=44279379b3824bda input=4fec4ddfc44a4d1a]*/
{
    return PyLong_FromSsize_t(_PyDict_SizeOf(self));
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

PyDoc_STRVAR(update__doc__,
"D.update([E, ]**F) -> None.  Update D from mapping/iterable E and F.\n\
If E is present and has a .keys() method, then does:  for k in E.keys(): D[k] = E[k]\n\
If E is present and lacks a .keys() method, then does:  for k, v in E: D[k] = v\n\
In either case, this is followed by: for k in F:  D[k] = F[k]");

/* Forward */

static PyMethodDef mapp_methods[] = {
    DICT___CONTAINS___METHODDEF
    {"__getitem__",     dict_subscript,                 METH_O | METH_COEXIST,
     getitem__doc__},
    DICT___SIZEOF___METHODDEF
    DICT_GET_METHODDEF
    DICT_SETDEFAULT_METHODDEF
    DICT_POP_METHODDEF
    DICT_POPITEM_METHODDEF
    DICT_KEYS_METHODDEF
    DICT_ITEMS_METHODDEF
    DICT_VALUES_METHODDEF
    {"update",          _PyCFunction_CAST(dict_update), METH_VARARGS | METH_KEYWORDS,
     update__doc__},
    DICT_FROMKEYS_METHODDEF
    DICT_CLEAR_METHODDEF
    DICT_COPY_METHODDEF
    DICT___REVERSED___METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL,              NULL}   /* sentinel */
};

/* Return 1 if `key` is in dict `op`, 0 if not, and -1 on error. */
int
PyDict_Contains(PyObject *op, PyObject *key)
{
    Py_hash_t hash = _PyObject_HashFast(key);
    if (hash == -1) {
        dict_unhashable_type(key);
        return -1;
    }

    return _PyDict_Contains_KnownHash(op, key, hash);
}

int
PyDict_ContainsString(PyObject *op, const char *key)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        return -1;
    }
    int res = PyDict_Contains(op, key_obj);
    Py_DECREF(key_obj);
    return res;
}

/* Internal version of PyDict_Contains used when the hash value is already known */
int
_PyDict_Contains_KnownHash(PyObject *op, PyObject *key, Py_hash_t hash)
{
    PyDictObject *mp = (PyDictObject *)op;
    PyObject *value;
    Py_ssize_t ix;

#ifdef Py_GIL_DISABLED
    ix = _Py_dict_lookup_threadsafe(mp, key, hash, &value);
#else
    ix = _Py_dict_lookup(mp, key, hash, &value);
#endif
    if (ix == DKIX_ERROR)
        return -1;
    if (ix != DKIX_EMPTY && value != NULL) {
#ifdef Py_GIL_DISABLED
        Py_DECREF(value);
#endif
        return 1;
    }
    return 0;
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
    d->_ma_watcher_tag = 0;
    // We don't inc ref empty keys because they're immortal
    assert((Py_EMPTY_KEYS)->dk_refcnt == _Py_DICT_IMMORTAL_INITIAL_REFCNT);
    d->ma_keys = Py_EMPTY_KEYS;
    d->ma_values = NULL;
    ASSERT_CONSISTENT(d);
    if (!_PyObject_GC_IS_TRACKED(d)) {
        _PyObject_GC_TRACK(d);
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
dict_iter(PyObject *self)
{
    PyDictObject *dict = (PyDictObject *)self;
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
    dict_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dict_repr,                                  /* tp_repr */
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
    dict_iter,                                  /* tp_iter */
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
    .tp_version_tag = _Py_TYPE_VERSION_DICT,
};

/* For backward compatibility with old dictionary interface */

PyObject *
PyDict_GetItemString(PyObject *v, const char *key)
{
    PyObject *kv, *rv;
    kv = PyUnicode_FromString(key);
    if (kv == NULL) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyDict_GetItemString(); consider using "
            "PyDict_GetItemStringRef()");
        return NULL;
    }
    rv = dict_getitem(v, kv,
            "Exception ignored in PyDict_GetItemString(); consider using "
            "PyDict_GetItemStringRef()");
    Py_DECREF(kv);
    return rv;  // borrowed reference
}

int
PyDict_GetItemStringRef(PyObject *v, const char *key, PyObject **result)
{
    PyObject *key_obj = PyUnicode_FromString(key);
    if (key_obj == NULL) {
        *result = NULL;
        return -1;
    }
    int res = PyDict_GetItemRef(v, key_obj, result);
    Py_DECREF(key_obj);
    return res;
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
    PyInterpreterState *interp = _PyInterpreterState_GET();
    _PyUnicode_InternImmortal(interp, &kv); /* XXX Should we really? */
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
    Py_ssize_t used;
    dictiterobject *di;
    di = PyObject_GC_New(dictiterobject, itertype);
    if (di == NULL) {
        return NULL;
    }
    di->di_dict = (PyDictObject*)Py_NewRef(dict);
    used = FT_ATOMIC_LOAD_SSIZE_RELAXED(dict->ma_used);
    di->di_used = used;
    di->len = used;
    if (itertype == &PyDictRevIterKey_Type ||
         itertype == &PyDictRevIterItem_Type ||
         itertype == &PyDictRevIterValue_Type) {
        if (_PyDict_HasSplitTable(dict)) {
            di->di_pos = used - 1;
        }
        else {
            di->di_pos = load_keys_nentries(dict) - 1;
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
dictiter_dealloc(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(di);
    Py_XDECREF(di->di_dict);
    Py_XDECREF(di->di_result);
    PyObject_GC_Del(di);
}

static int
dictiter_traverse(PyObject *self, visitproc visit, void *arg)
{
    dictiterobject *di = (dictiterobject *)self;
    Py_VISIT(di->di_dict);
    Py_VISIT(di->di_result);
    return 0;
}

static PyObject *
dictiter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    dictiterobject *di = (dictiterobject *)self;
    Py_ssize_t len = 0;
    if (di->di_dict != NULL && di->di_used == FT_ATOMIC_LOAD_SSIZE_RELAXED(di->di_dict->ma_used))
        len = FT_ATOMIC_LOAD_SSIZE_RELAXED(di->len);
    return PyLong_FromSize_t(len);
}

PyDoc_STRVAR(length_hint_doc,
             "Private method returning an estimate of len(list(it)).");

static PyObject *
dictiter_reduce(PyObject *di, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyMethodDef dictiter_methods[] = {
    {"__length_hint__", dictiter_len,                   METH_NOARGS,
     length_hint_doc},
     {"__reduce__",     dictiter_reduce,                METH_NOARGS,
     reduce_doc},
    {NULL,              NULL}           /* sentinel */
};

#ifdef Py_GIL_DISABLED

static int
dictiter_iternext_threadsafe(PyDictObject *d, PyObject *self,
                             PyObject **out_key, PyObject **out_value);

#else /* Py_GIL_DISABLED */

static PyObject*
dictiter_iternextkey_lock_held(PyDictObject *d, PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyObject *key;
    Py_ssize_t i;
    PyDictKeysObject *k;

    assert (PyDict_Check(d));
    ASSERT_DICT_LOCKED(d);

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    i = di->di_pos;
    k = d->ma_keys;
    assert(i >= 0);
    if (_PyDict_HasSplitTable(d)) {
        if (i >= d->ma_used)
            goto fail;
        int index = get_index_from_order(d, i);
        key = LOAD_SHARED_KEY(DK_UNICODE_ENTRIES(k)[index].me_key);
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

#endif  /* Py_GIL_DISABLED */

static PyObject*
dictiter_iternextkey(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;

    if (d == NULL)
        return NULL;

    PyObject *value;
#ifdef Py_GIL_DISABLED
    if (dictiter_iternext_threadsafe(d, self, &value, NULL) < 0) {
        value = NULL;
    }
#else
    value = dictiter_iternextkey_lock_held(d, self);
#endif

    return value;
}

PyTypeObject PyDictIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keyiterator",                         /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
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
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_iternextkey,                       /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};

#ifndef Py_GIL_DISABLED

static PyObject *
dictiter_iternextvalue_lock_held(PyDictObject *d, PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyObject *value;
    Py_ssize_t i;

    assert (PyDict_Check(d));
    ASSERT_DICT_LOCKED(d);

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return NULL;
    }

    i = di->di_pos;
    assert(i >= 0);
    if (_PyDict_HasSplitTable(d)) {
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

#endif  /* Py_GIL_DISABLED */

static PyObject *
dictiter_iternextvalue(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;

    if (d == NULL)
        return NULL;

    PyObject *value;
#ifdef Py_GIL_DISABLED
    if (dictiter_iternext_threadsafe(d, self, NULL, &value) < 0) {
        value = NULL;
    }
#else
    value = dictiter_iternextvalue_lock_held(d, self);
#endif

    return value;
}

PyTypeObject PyDictIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_valueiterator",                       /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
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
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_iternextvalue,                     /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};

static int
dictiter_iternextitem_lock_held(PyDictObject *d, PyObject *self,
                                PyObject **out_key, PyObject **out_value)
{
    dictiterobject *di = (dictiterobject *)self;
    PyObject *key, *value;
    Py_ssize_t i;

    assert (PyDict_Check(d));
    ASSERT_DICT_LOCKED(d);

    if (di->di_used != d->ma_used) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return -1;
    }

    i = FT_ATOMIC_LOAD_SSIZE_RELAXED(di->di_pos);

    assert(i >= 0);
    if (_PyDict_HasSplitTable(d)) {
        if (i >= d->ma_used)
            goto fail;
        int index = get_index_from_order(d, i);
        key = LOAD_SHARED_KEY(DK_UNICODE_ENTRIES(d->ma_keys)[index].me_key);
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
    if (out_key != NULL) {
        *out_key = Py_NewRef(key);
    }
    if (out_value != NULL) {
        *out_value = Py_NewRef(value);
    }
    return 0;

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
    return -1;
}

#ifdef Py_GIL_DISABLED

// Grabs the key and/or value from the provided locations and if successful
// returns them with an increased reference count.  If either one is unsuccessful
// nothing is incref'd and returns -1.
static int
acquire_key_value(PyObject **key_loc, PyObject *value, PyObject **value_loc,
                  PyObject **out_key, PyObject **out_value)
{
    if (out_key) {
        *out_key = _Py_TryXGetRef(key_loc);
        if (*out_key == NULL) {
            return -1;
        }
    }

    if (out_value) {
        if (!_Py_TryIncrefCompare(value_loc, value)) {
            if (out_key) {
                Py_DECREF(*out_key);
            }
            return -1;
        }
        *out_value = value;
    }

    return 0;
}

static int
dictiter_iternext_threadsafe(PyDictObject *d, PyObject *self,
                             PyObject **out_key, PyObject **out_value)
{
    int res;
    dictiterobject *di = (dictiterobject *)self;
    Py_ssize_t i;
    PyDictKeysObject *k;

    assert (PyDict_Check(d));

    if (di->di_used != _Py_atomic_load_ssize_relaxed(&d->ma_used)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dictionary changed size during iteration");
        di->di_used = -1; /* Make this state sticky */
        return -1;
    }

    ensure_shared_on_read(d);

    i = _Py_atomic_load_ssize_relaxed(&di->di_pos);
    k = _Py_atomic_load_ptr_acquire(&d->ma_keys);
    assert(i >= 0);
    if (_PyDict_HasSplitTable(d)) {
        PyDictValues *values = _Py_atomic_load_ptr_relaxed(&d->ma_values);
        if (values == NULL) {
            goto concurrent_modification;
        }

        Py_ssize_t used = (Py_ssize_t)_Py_atomic_load_uint8(&values->size);
        if (i >= used) {
            goto fail;
        }

        // We're racing against writes to the order from delete_index_from_values, but
        // single threaded can suffer from concurrent modification to those as well and
        // can have either duplicated or skipped attributes, so we strive to do no better
        // here.
        int index = get_index_from_order(d, i);
        PyObject *value = _Py_atomic_load_ptr(&values->values[index]);
        if (acquire_key_value(&DK_UNICODE_ENTRIES(k)[index].me_key, value,
                               &values->values[index], out_key, out_value) < 0) {
            goto try_locked;
        }
    }
    else {
        Py_ssize_t n = _Py_atomic_load_ssize_relaxed(&k->dk_nentries);
        if (DK_IS_UNICODE(k)) {
            PyDictUnicodeEntry *entry_ptr = &DK_UNICODE_ENTRIES(k)[i];
            PyObject *value;
            while (i < n &&
                  (value = _Py_atomic_load_ptr(&entry_ptr->me_value)) == NULL) {
                entry_ptr++;
                i++;
            }
            if (i >= n)
                goto fail;

            if (acquire_key_value(&entry_ptr->me_key, value,
                                   &entry_ptr->me_value, out_key, out_value) < 0) {
                goto try_locked;
            }
        }
        else {
            PyDictKeyEntry *entry_ptr = &DK_ENTRIES(k)[i];
            PyObject *value;
            while (i < n &&
                  (value = _Py_atomic_load_ptr(&entry_ptr->me_value)) == NULL) {
                entry_ptr++;
                i++;
            }

            if (i >= n)
                goto fail;

            if (acquire_key_value(&entry_ptr->me_key, value,
                                   &entry_ptr->me_value, out_key, out_value) < 0) {
                goto try_locked;
            }
        }
    }
    // We found an element (key), but did not expect it
    Py_ssize_t len;
    if ((len = _Py_atomic_load_ssize_relaxed(&di->len)) == 0) {
        goto concurrent_modification;
    }

    _Py_atomic_store_ssize_relaxed(&di->di_pos, i + 1);
    _Py_atomic_store_ssize_relaxed(&di->len, len - 1);
    return 0;

concurrent_modification:
    PyErr_SetString(PyExc_RuntimeError,
                    "dictionary keys changed during iteration");

fail:
    di->di_dict = NULL;
    Py_DECREF(d);
    return -1;

try_locked:
    Py_BEGIN_CRITICAL_SECTION(d);
    res = dictiter_iternextitem_lock_held(d, self, out_key, out_value);
    Py_END_CRITICAL_SECTION();
    return res;
}

#endif

static bool
has_unique_reference(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    return (_Py_IsOwnedByCurrentThread(op) &&
            op->ob_ref_local == 1 &&
            _Py_atomic_load_ssize_relaxed(&op->ob_ref_shared) == 0);
#else
    return Py_REFCNT(op) == 1;
#endif
}

static bool
acquire_iter_result(PyObject *result)
{
    if (has_unique_reference(result)) {
        Py_INCREF(result);
        return true;
    }
    return false;
}

static PyObject *
dictiter_iternextitem(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;

    if (d == NULL)
        return NULL;

    PyObject *key, *value;
#ifdef Py_GIL_DISABLED
    if (dictiter_iternext_threadsafe(d, self, &key, &value) == 0) {
#else
    if (dictiter_iternextitem_lock_held(d, self, &key, &value) == 0) {

#endif
        PyObject *result = di->di_result;
        if (acquire_iter_result(result)) {
            PyObject *oldkey = PyTuple_GET_ITEM(result, 0);
            PyObject *oldvalue = PyTuple_GET_ITEM(result, 1);
            PyTuple_SET_ITEM(result, 0, key);
            PyTuple_SET_ITEM(result, 1, value);
            Py_DECREF(oldkey);
            Py_DECREF(oldvalue);
            // bpo-42536: The GC may have untracked this result tuple. Since we're
            // recycling it, make sure it's tracked again:
            _PyTuple_Recycle(result);
        }
        else {
            result = PyTuple_New(2);
            if (result == NULL)
                return NULL;
            PyTuple_SET_ITEM(result, 0, key);
            PyTuple_SET_ITEM(result, 1, value);
        }
        return result;
    }
    return NULL;
}

PyTypeObject PyDictIterItem_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_itemiterator",                        /* tp_name */
    sizeof(dictiterobject),                     /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictiter_dealloc,                           /* tp_dealloc */
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
    dictiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    dictiter_iternextitem,                      /* tp_iternext */
    dictiter_methods,                           /* tp_methods */
    0,
};


/* dictreviter */

static PyObject *
dictreviter_iter_lock_held(PyDictObject *d, PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;

    assert (PyDict_Check(d));
    ASSERT_DICT_LOCKED(d);

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
    if (_PyDict_HasSplitTable(d)) {
        int index = get_index_from_order(d, i);
        key = LOAD_SHARED_KEY(DK_UNICODE_ENTRIES(k)[index].me_key);
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
            _PyTuple_Recycle(result);
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

static PyObject *
dictreviter_iternext(PyObject *self)
{
    dictiterobject *di = (dictiterobject *)self;
    PyDictObject *d = di->di_dict;

    if (d == NULL)
        return NULL;

    PyObject *value;
    Py_BEGIN_CRITICAL_SECTION(d);
    value = dictreviter_iter_lock_held(d, self);
    Py_END_CRITICAL_SECTION();

    return value;
}

PyTypeObject PyDictRevIterKey_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversekeyiterator",
    sizeof(dictiterobject),
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictreviter_iternext,
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
dictiter_reduce(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    dictiterobject *di = (dictiterobject *)self;
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
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictreviter_iternext,
    .tp_methods = dictiter_methods
};

PyTypeObject PyDictRevIterValue_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_reversevalueiterator",
    sizeof(dictiterobject),
    .tp_dealloc = dictiter_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = dictiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = dictreviter_iternext,
    .tp_methods = dictiter_methods
};

/***********************************************/
/* View objects for keys(), items(), values(). */
/***********************************************/

/* The instance lay-out is the same for all three; but the type differs. */

static void
dictview_dealloc(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    _PyObject_GC_UNTRACK(dv);
    Py_XDECREF(dv->dv_dict);
    PyObject_GC_Del(dv);
}

static int
dictview_traverse(PyObject *self, visitproc visit, void *arg)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    Py_VISIT(dv->dv_dict);
    return 0;
}

static Py_ssize_t
dictview_len(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    Py_ssize_t len = 0;
    if (dv->dv_dict != NULL)
        len = FT_ATOMIC_LOAD_SSIZE_RELAXED(dv->dv_dict->ma_used);
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
    {"mapping", dictview_mapping, NULL,
     PyDoc_STR("dictionary that this view refers to"), NULL},
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
dictview_repr(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
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
dictkeys_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterKey_Type);
}

static int
dictkeys_contains(PyObject *self, PyObject *obj)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL)
        return 0;
    return PyDict_Contains((PyObject *)dv->dv_dict, obj);
}

static PySequenceMethods dictkeys_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    dictkeys_contains,                  /* sq_contains */
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
dictitems_contains(PyObject *dv, PyObject *obj);

PyObject *
_PyDictView_Intersect(PyObject* self, PyObject *other)
{
    PyObject *result;
    PyObject *it;
    PyObject *key;
    Py_ssize_t len_self;
    int rv;
    objobjproc dict_contains;

    /* Python interpreter swaps parameters when dict view
       is on right side of & */
    if (!PyDictViewSet_Check(self)) {
        PyObject *tmp = other;
        other = self;
        self = tmp;
    }

    len_self = dictview_len(self);

    /* if other is a set and self is smaller than other,
       reuse set intersection logic */
    if (PySet_CheckExact(other) && len_self <= PyObject_Size(other)) {
        return PyObject_CallMethodObjArgs(
                other, &_Py_ID(intersection), self, NULL);
    }

    /* if other is another dict view, and it is bigger than self,
       swap them */
    if (PyDictViewSet_Check(other)) {
        Py_ssize_t len_other = dictview_len(other);
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
        rv = dict_contains(self, key);
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
dictitems_xor_lock_held(PyObject *d1, PyObject *d2)
{
    ASSERT_DICT_LOCKED(d1);
    ASSERT_DICT_LOCKED(d2);

    PyObject *temp_dict = copy_lock_held(d1);
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

static PyObject *
dictitems_xor(PyObject *self, PyObject *other)
{
    assert(PyDictItems_Check(self));
    assert(PyDictItems_Check(other));
    PyObject *d1 = (PyObject *)((_PyDictViewObject *)self)->dv_dict;
    PyObject *d2 = (PyObject *)((_PyDictViewObject *)other)->dv_dict;

    PyObject *res;
    Py_BEGIN_CRITICAL_SECTION2(d1, d2);
    res = dictitems_xor_lock_held(d1, d2);
    Py_END_CRITICAL_SECTION2();

    return res;
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
    dictviews_sub,                      /*nb_subtract*/
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
    _PyDictView_Intersect,              /*nb_and*/
    dictviews_xor,                      /*nb_xor*/
    dictviews_or,                       /*nb_or*/
};

static PyObject*
dictviews_isdisjoint(PyObject *self, PyObject *other)
{
    PyObject *it;
    PyObject *item = NULL;

    if (self == other) {
        if (dictview_len(self) == 0)
            Py_RETURN_TRUE;
        else
            Py_RETURN_FALSE;
    }

    /* Iterate over the shorter object (only if other is a set,
     * because PySequence_Contains may be expensive otherwise): */
    if (PyAnySet_Check(other) || PyDictViewSet_Check(other)) {
        Py_ssize_t len_self = dictview_len(self);
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

static PyObject* dictkeys_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_keys_doc,
"Return a reverse iterator over the dict keys.");

static PyMethodDef dictkeys_methods[] = {
    {"isdisjoint",      dictviews_isdisjoint,           METH_O,
     isdisjoint_doc},
    {"__reversed__",    dictkeys_reversed,              METH_NOARGS,
     reversed_keys_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictKeys_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_keys",                                /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
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
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictkeys_iter,                              /* tp_iter */
    0,                                          /* tp_iternext */
    dictkeys_methods,                           /* tp_methods */
    .tp_getset = dictview_getset,
};

/*[clinic input]
dict.keys

Return a set-like object providing a view on the dict's keys.
[clinic start generated code]*/

static PyObject *
dict_keys_impl(PyDictObject *self)
/*[clinic end generated code: output=aac2830c62990358 input=42f48a7a771212a7]*/
{
    return _PyDictView_New((PyObject *)self, &PyDictKeys_Type);
}

static PyObject *
dictkeys_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterKey_Type);
}

/*** dict_items ***/

static PyObject *
dictitems_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterItem_Type);
}

static int
dictitems_contains(PyObject *self, PyObject *obj)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    int result;
    PyObject *key, *value, *found;
    if (dv->dv_dict == NULL)
        return 0;
    if (!PyTuple_Check(obj) || PyTuple_GET_SIZE(obj) != 2)
        return 0;
    key = PyTuple_GET_ITEM(obj, 0);
    value = PyTuple_GET_ITEM(obj, 1);
    result = PyDict_GetItemRef((PyObject *)dv->dv_dict, key, &found);
    if (result == 1) {
        result = PyObject_RichCompareBool(found, value, Py_EQ);
        Py_DECREF(found);
    }
    return result;
}

static PySequenceMethods dictitems_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    dictitems_contains,                 /* sq_contains */
};

static PyObject* dictitems_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_items_doc,
"Return a reverse iterator over the dict items.");

static PyMethodDef dictitems_methods[] = {
    {"isdisjoint",      dictviews_isdisjoint,           METH_O,
     isdisjoint_doc},
    {"__reversed__",    dictitems_reversed,             METH_NOARGS,
     reversed_items_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictItems_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_items",                               /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
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
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    dictview_richcompare,                       /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictitems_iter,                             /* tp_iter */
    0,                                          /* tp_iternext */
    dictitems_methods,                          /* tp_methods */
    .tp_getset = dictview_getset,
};

/*[clinic input]
dict.items

Return a set-like object providing a view on the dict's items.
[clinic start generated code]*/

static PyObject *
dict_items_impl(PyDictObject *self)
/*[clinic end generated code: output=88c7db7150c7909a input=87c822872eb71f5a]*/
{
    return _PyDictView_New((PyObject *)self, &PyDictItems_Type);
}

static PyObject *
dictitems_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterItem_Type);
}

/*** dict_values ***/

static PyObject *
dictvalues_iter(PyObject *self)
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictIterValue_Type);
}

static PySequenceMethods dictvalues_as_sequence = {
    dictview_len,                       /* sq_length */
    0,                                  /* sq_concat */
    0,                                  /* sq_repeat */
    0,                                  /* sq_item */
    0,                                  /* sq_slice */
    0,                                  /* sq_ass_item */
    0,                                  /* sq_ass_slice */
    0,                                  /* sq_contains */
};

static PyObject* dictvalues_reversed(PyObject *dv, PyObject *Py_UNUSED(ignored));

PyDoc_STRVAR(reversed_values_doc,
"Return a reverse iterator over the dict values.");

static PyMethodDef dictvalues_methods[] = {
    {"__reversed__",    dictvalues_reversed,            METH_NOARGS,
     reversed_values_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyDictValues_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "dict_values",                              /* tp_name */
    sizeof(_PyDictViewObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    dictview_dealloc,                           /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    dictview_repr,                              /* tp_repr */
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
    dictview_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    dictvalues_iter,                            /* tp_iter */
    0,                                          /* tp_iternext */
    dictvalues_methods,                         /* tp_methods */
    .tp_getset = dictview_getset,
};

/*[clinic input]
dict.values

Return an object providing a view on the dict's values.
[clinic start generated code]*/

static PyObject *
dict_values_impl(PyDictObject *self)
/*[clinic end generated code: output=ce9f2e9e8a959dd4 input=b46944f85493b230]*/
{
    return _PyDictView_New((PyObject *)self, &PyDictValues_Type);
}

static PyObject *
dictvalues_reversed(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    _PyDictViewObject *dv = (_PyDictViewObject *)self;
    if (dv->dv_dict == NULL) {
        Py_RETURN_NONE;
    }
    return dictiter_new(dv->dv_dict, &PyDictRevIterValue_Type);
}


/* Returns NULL if cannot allocate a new PyDictKeysObject,
   but does not set an error */
PyDictKeysObject *
_PyDict_NewKeysForClass(PyHeapTypeObject *cls)
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
    if (cls->ht_type.tp_dict) {
        PyObject *attrs = PyDict_GetItem(cls->ht_type.tp_dict, &_Py_ID(__static_attributes__));
        if (attrs != NULL && PyTuple_Check(attrs)) {
            for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(attrs); i++) {
                PyObject *key = PyTuple_GET_ITEM(attrs, i);
                Py_hash_t hash;
                if (PyUnicode_CheckExact(key) && (hash = unicode_get_hash(key)) != -1) {
                    if (insert_split_key(keys, key, hash) == DKIX_EMPTY) {
                        break;
                    }
                }
            }
        }
    }
    return keys;
}

void
_PyObject_InitInlineValues(PyObject *obj, PyTypeObject *tp)
{
    assert(tp->tp_flags & Py_TPFLAGS_HEAPTYPE);
    assert(tp->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    assert(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictKeysObject *keys = CACHED_KEYS(tp);
    assert(keys != NULL);
    OBJECT_STAT_INC(inline_values);
#ifdef Py_GIL_DISABLED
    Py_ssize_t usable = _Py_atomic_load_ssize_relaxed(&keys->dk_usable);
    if (usable > 1) {
        LOCK_KEYS(keys);
        if (keys->dk_usable > 1) {
            _Py_atomic_store_ssize(&keys->dk_usable, keys->dk_usable - 1);
        }
        UNLOCK_KEYS(keys);
    }
#else
    if (keys->dk_usable > 1) {
        keys->dk_usable--;
    }
#endif
    size_t size = shared_keys_usable_size(keys);
    PyDictValues *values = _PyObject_InlineValues(obj);
    assert(size < 256);
    values->capacity = (uint8_t)size;
    values->size = 0;
    values->embedded = 1;
    values->valid = 1;
    for (size_t i = 0; i < size; i++) {
        values->values[i] = NULL;
    }
    _PyObject_ManagedDictPointer(obj)->dict = NULL;
}

static PyDictObject *
make_dict_from_instance_attributes(PyInterpreterState *interp,
                                   PyDictKeysObject *keys, PyDictValues *values)
{
    dictkeys_incref(keys);
    Py_ssize_t used = 0;
    size_t size = shared_keys_usable_size(keys);
    for (size_t i = 0; i < size; i++) {
        PyObject *val = values->values[i];
        if (val != NULL) {
            used += 1;
        }
    }
    PyDictObject *res = (PyDictObject *)new_dict(interp, keys, values, used, 0);
    return res;
}

PyDictObject *
_PyObject_MaterializeManagedDict_LockHeld(PyObject *obj)
{
    ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(obj);

    OBJECT_STAT_INC(dict_materialized_on_request);

    PyDictValues *values = _PyObject_InlineValues(obj);
    PyDictObject *dict;
    if (values->valid) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
        dict = make_dict_from_instance_attributes(interp, keys, values);
    }
    else {
        dict = (PyDictObject *)PyDict_New();
    }
    FT_ATOMIC_STORE_PTR_RELEASE(_PyObject_ManagedDictPointer(obj)->dict,
                                dict);
    return dict;
}

PyDictObject *
_PyObject_MaterializeManagedDict(PyObject *obj)
{
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict != NULL) {
        return dict;
    }

    Py_BEGIN_CRITICAL_SECTION(obj);

#ifdef Py_GIL_DISABLED
    dict = _PyObject_GetManagedDict(obj);
    if (dict != NULL) {
        // We raced with another thread creating the dict
        goto exit;
    }
#endif
    dict = _PyObject_MaterializeManagedDict_LockHeld(obj);

#ifdef Py_GIL_DISABLED
exit:
#endif
    Py_END_CRITICAL_SECTION();
    return dict;
}

int
_PyDict_SetItem_LockHeld(PyDictObject *dict, PyObject *name, PyObject *value)
{
    if (value == NULL) {
        Py_hash_t hash = _PyObject_HashFast(name);
        if (hash == -1) {
            dict_unhashable_type(name);
            return -1;
        }
        return delitem_knownhash_lock_held((PyObject *)dict, name, hash);
    } else {
        return setitem_lock_held(dict, name, value);
    }
}

// Called with either the object's lock or the dict's lock held
// depending on whether or not a dict has been materialized for
// the object.
static int
store_instance_attr_lock_held(PyObject *obj, PyDictValues *values,
                              PyObject *name, PyObject *value)
{
    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    assert(keys != NULL);
    assert(values != NULL);
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);
    Py_ssize_t ix = DKIX_EMPTY;
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    assert(dict == NULL || ((PyDictObject *)dict)->ma_values == values);
    if (PyUnicode_CheckExact(name)) {
        Py_hash_t hash = unicode_get_hash(name);
        if (hash == -1) {
            hash = PyUnicode_Type.tp_hash(name);
            assert(hash != -1);
        }

        ix = insert_split_key(keys, name, hash);

#ifdef Py_STATS
        if (ix == DKIX_EMPTY) {
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
        }
#endif
    }

    if (ix == DKIX_EMPTY) {
        int res;
        if (dict == NULL) {
            // Make the dict but don't publish it in the object
            // so that no one else will see it.
            dict = make_dict_from_instance_attributes(PyInterpreterState_Get(), keys, values);
            if (dict == NULL ||
                _PyDict_SetItem_LockHeld(dict, name, value) < 0) {
                Py_XDECREF(dict);
                return -1;
            }

            FT_ATOMIC_STORE_PTR_RELEASE(_PyObject_ManagedDictPointer(obj)->dict,
                                        (PyDictObject *)dict);
            return 0;
        }

        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(dict);

        res = _PyDict_SetItem_LockHeld(dict, name, value);
        return res;
    }

    PyObject *old_value = values->values[ix];
    if (old_value == NULL && value == NULL) {
        PyErr_Format(PyExc_AttributeError,
                        "'%.100s' object has no attribute '%U'",
                        Py_TYPE(obj)->tp_name, name);
        return -1;
    }

    if (dict) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        PyDict_WatchEvent event = (old_value == NULL ? PyDict_EVENT_ADDED :
                                   value == NULL ? PyDict_EVENT_DELETED :
                                   PyDict_EVENT_MODIFIED);
        _PyDict_NotifyEvent(interp, event, dict, name, value);
    }

    FT_ATOMIC_STORE_PTR_RELEASE(values->values[ix], Py_XNewRef(value));

    if (old_value == NULL) {
        _PyDictValues_AddToInsertionOrder(values, ix);
        if (dict) {
            assert(dict->ma_values == values);
            STORE_USED(dict, dict->ma_used + 1);
        }
    }
    else {
        if (value == NULL) {
            delete_index_from_values(values, ix);
            if (dict) {
                assert(dict->ma_values == values);
                STORE_USED(dict, dict->ma_used - 1);
            }
        }
        Py_DECREF(old_value);
    }
    return 0;
}

static inline int
store_instance_attr_dict(PyObject *obj, PyDictObject *dict, PyObject *name, PyObject *value)
{
    PyDictValues *values = _PyObject_InlineValues(obj);
    int res;
    Py_BEGIN_CRITICAL_SECTION(dict);
    if (dict->ma_values == values) {
        res = store_instance_attr_lock_held(obj, values, name, value);
    }
    else {
        res = _PyDict_SetItem_LockHeld(dict, name, value);
    }
    Py_END_CRITICAL_SECTION();
    return res;
}

int
_PyObject_StoreInstanceAttribute(PyObject *obj, PyObject *name, PyObject *value)
{
    PyDictValues *values = _PyObject_InlineValues(obj);
    if (!FT_ATOMIC_LOAD_UINT8(values->valid)) {
        PyDictObject *dict = _PyObject_GetManagedDict(obj);
        if (dict == NULL) {
            dict = (PyDictObject *)PyObject_GenericGetDict(obj, NULL);
            if (dict == NULL) {
                return -1;
            }
            int res = store_instance_attr_dict(obj, dict, name, value);
            Py_DECREF(dict);
            return res;
        }
        return store_instance_attr_dict(obj, dict, name, value);
    }

#ifdef Py_GIL_DISABLED
    // We have a valid inline values, at least for now...  There are two potential
    // races with having the values become invalid.  One is the dictionary
    // being detached from the object.  The other is if someone is inserting
    // into the dictionary directly and therefore causing it to resize.
    //
    // If we haven't materialized the dictionary yet we lock on the object, which
    // will also be used to prevent the dictionary from being materialized while
    // we're doing the insertion.  If we race and the dictionary gets created
    // then we'll need to release the object lock and lock the dictionary to
    // prevent resizing.
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        int res;
        Py_BEGIN_CRITICAL_SECTION(obj);
        dict = _PyObject_GetManagedDict(obj);

        if (dict == NULL) {
            res = store_instance_attr_lock_held(obj, values, name, value);
        }
        Py_END_CRITICAL_SECTION();

        if (dict == NULL) {
            return res;
        }
    }
    return store_instance_attr_dict(obj, dict, name, value);
#else
    return store_instance_attr_lock_held(obj, values, name, value);
#endif
}

/* Sanity check for managed dicts */
#if 0
#define CHECK(val) assert(val); if (!(val)) { return 0; }

int
_PyObject_ManagedDictValidityCheck(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    CHECK(tp->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyManagedDictPointer *managed_dict = _PyObject_ManagedDictPointer(obj);
    if (_PyManagedDictPointer_IsValues(*managed_dict)) {
        PyDictValues *values = _PyManagedDictPointer_GetValues(*managed_dict);
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
        if (managed_dict->dict != NULL) {
            CHECK(PyDict_Check(managed_dict->dict));
        }
    }
    return 1;
}
#endif

// Attempts to get an instance attribute from the inline values. Returns true
// if successful, or false if the caller needs to lookup in the dictionary.
bool
_PyObject_TryGetInstanceAttribute(PyObject *obj, PyObject *name, PyObject **attr)
{
    assert(PyUnicode_CheckExact(name));
    PyDictValues *values = _PyObject_InlineValues(obj);
    if (!FT_ATOMIC_LOAD_UINT8(values->valid)) {
        return false;
    }

    PyDictKeysObject *keys = CACHED_KEYS(Py_TYPE(obj));
    assert(keys != NULL);
    Py_ssize_t ix = _PyDictKeys_StringLookupSplit(keys, name);
    if (ix == DKIX_EMPTY) {
        *attr = NULL;
        return true;
    }

#ifdef Py_GIL_DISABLED
    PyObject *value = _Py_atomic_load_ptr_acquire(&values->values[ix]);
    if (value == NULL) {
        if (FT_ATOMIC_LOAD_UINT8(values->valid)) {
            *attr = NULL;
            return true;
        }
    }
    else if (_Py_TryIncrefCompare(&values->values[ix], value)) {
        *attr = value;
        return true;
    }

    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        // No dict, lock the object to prevent one from being
        // materialized...
        bool success = false;
        Py_BEGIN_CRITICAL_SECTION(obj);

        dict = _PyObject_GetManagedDict(obj);
        if (dict == NULL) {
            // Still no dict, we can read from the values
            assert(values->valid);
            value = values->values[ix];
            *attr = _Py_XNewRefWithLock(value);
            success = true;
        }

        Py_END_CRITICAL_SECTION();

        if (success) {
            return true;
        }
    }

    // We have a dictionary, we'll need to lock it to prevent
    // the values from being resized.
    assert(dict != NULL);

    bool success;
    Py_BEGIN_CRITICAL_SECTION(dict);

    if (dict->ma_values == values && FT_ATOMIC_LOAD_UINT8(values->valid)) {
        value = _Py_atomic_load_ptr_relaxed(&values->values[ix]);
        *attr = _Py_XNewRefWithLock(value);
        success = true;
    } else {
        // Caller needs to lookup from the dictionary
        success = false;
    }

    Py_END_CRITICAL_SECTION();

    return success;
#else
    PyObject *value = values->values[ix];
    *attr = Py_XNewRef(value);
    return true;
#endif
}

int
_PyObject_IsInstanceDictEmpty(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_dictoffset == 0) {
        return 1;
    }
    PyDictObject *dict;
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictValues *values = _PyObject_InlineValues(obj);
        if (FT_ATOMIC_LOAD_UINT8(values->valid)) {
            PyDictKeysObject *keys = CACHED_KEYS(tp);
            for (Py_ssize_t i = 0; i < keys->dk_nentries; i++) {
                if (FT_ATOMIC_LOAD_PTR_RELAXED(values->values[i]) != NULL) {
                    return 0;
                }
            }
            return 1;
        }
        dict = _PyObject_GetManagedDict(obj);
    }
    else if (tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) {
        dict = _PyObject_GetManagedDict(obj);
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        dict = (PyDictObject *)*dictptr;
    }
    if (dict == NULL) {
        return 1;
    }
    return FT_ATOMIC_LOAD_SSIZE_RELAXED(((PyDictObject *)dict)->ma_used) == 0;
}

int
PyObject_VisitManagedDict(PyObject *obj, visitproc visit, void *arg)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if((tp->tp_flags & Py_TPFLAGS_MANAGED_DICT) == 0) {
        return 0;
    }
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictValues *values = _PyObject_InlineValues(obj);
        if (values->valid) {
            for (Py_ssize_t i = 0; i < values->capacity; i++) {
                Py_VISIT(values->values[i]);
            }
            return 0;
        }
    }
    Py_VISIT(_PyObject_ManagedDictPointer(obj)->dict);
    return 0;
}

static void
clear_inline_values(PyDictValues *values)
{
    if (values->valid) {
        FT_ATOMIC_STORE_UINT8(values->valid, 0);
        for (Py_ssize_t i = 0; i < values->capacity; i++) {
            Py_CLEAR(values->values[i]);
        }
    }
}

static void
set_dict_inline_values(PyObject *obj, PyDictObject *new_dict)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(obj);

    PyDictValues *values = _PyObject_InlineValues(obj);

    Py_XINCREF(new_dict);
    FT_ATOMIC_STORE_PTR(_PyObject_ManagedDictPointer(obj)->dict, new_dict);

    clear_inline_values(values);
}

#ifdef Py_GIL_DISABLED

// Trys and sets the dictionary for an object in the easy case when our current
// dictionary is either completely not materialized or is a dictionary which
// does not point at the inline values.
static bool
try_set_dict_inline_only_or_other_dict(PyObject *obj, PyObject *new_dict, PyDictObject **cur_dict)
{
    bool replaced = false;
    Py_BEGIN_CRITICAL_SECTION(obj);

    PyDictObject *dict = *cur_dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        // We only have inline values, we can just completely replace them.
        set_dict_inline_values(obj, (PyDictObject *)new_dict);
        replaced = true;
        goto exit_lock;
    }

    if (FT_ATOMIC_LOAD_PTR_RELAXED(dict->ma_values) != _PyObject_InlineValues(obj)) {
        // We have a materialized dict which doesn't point at the inline values,
        // We get to simply swap dictionaries and free the old dictionary.
        FT_ATOMIC_STORE_PTR(_PyObject_ManagedDictPointer(obj)->dict,
                            (PyDictObject *)Py_XNewRef(new_dict));
        replaced = true;
        goto exit_lock;
    }
    else {
        // We have inline values, we need to lock the dict and the object
        // at the same time to safely dematerialize them. To do that while releasing
        // the object lock we need a strong reference to the current dictionary.
        Py_INCREF(dict);
    }
exit_lock:
    Py_END_CRITICAL_SECTION();
    return replaced;
}

// Replaces a dictionary that is probably the dictionary which has been
// materialized and points at the inline values. We could have raced
// and replaced it with another dictionary though.
static int
replace_dict_probably_inline_materialized(PyObject *obj, PyDictObject *inline_dict,
                                          PyDictObject *cur_dict, PyObject *new_dict)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(obj);

    if (cur_dict == inline_dict) {
        assert(FT_ATOMIC_LOAD_PTR_RELAXED(inline_dict->ma_values) == _PyObject_InlineValues(obj));

        int err = _PyDict_DetachFromObject(inline_dict, obj);
        if (err != 0) {
            assert(new_dict == NULL);
            return err;
        }
    }

    FT_ATOMIC_STORE_PTR(_PyObject_ManagedDictPointer(obj)->dict,
                        (PyDictObject *)Py_XNewRef(new_dict));
    return 0;
}

#endif

static void
decref_maybe_delay(PyObject *obj, bool delay)
{
    if (delay) {
        _PyObject_XDecRefDelayed(obj);
    }
    else {
        Py_XDECREF(obj);
    }
}

int
_PyObject_SetManagedDict(PyObject *obj, PyObject *new_dict)
{
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
#ifndef NDEBUG
    Py_BEGIN_CRITICAL_SECTION(obj);
    assert(_PyObject_InlineValuesConsistencyCheck(obj));
    Py_END_CRITICAL_SECTION();
#endif
    int err = 0;
    PyTypeObject *tp = Py_TYPE(obj);
    if (tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
#ifdef Py_GIL_DISABLED
        PyDictObject *prev_dict;
        if (!try_set_dict_inline_only_or_other_dict(obj, new_dict, &prev_dict)) {
            // We had a materialized dictionary which pointed at the inline
            // values. We need to lock both the object and the dict at the
            // same time to safely replace it. We can't merely lock the dictionary
            // while the object is locked because it could suspend the object lock.
            PyDictObject *cur_dict;

            assert(prev_dict != NULL);
            Py_BEGIN_CRITICAL_SECTION2(obj, prev_dict);

            // We could have had another thread race in between the call to
            // try_set_dict_inline_only_or_other_dict where we locked the object
            // and when we unlocked and re-locked the dictionary.
            cur_dict = _PyObject_GetManagedDict(obj);

            err = replace_dict_probably_inline_materialized(obj, prev_dict,
                                                            cur_dict, new_dict);

            Py_END_CRITICAL_SECTION2();

            // Decref for the dictionary we incref'd in try_set_dict_inline_only_or_other_dict
            // while the object was locked
            decref_maybe_delay((PyObject *)prev_dict, prev_dict != cur_dict);
            if (err != 0) {
                return err;
            }

            prev_dict = cur_dict;
        }

        if (prev_dict != NULL) {
            // decref for the dictionary that we replaced
            decref_maybe_delay((PyObject *)prev_dict, true);
        }

        return 0;
#else
        PyDictObject *dict = _PyObject_GetManagedDict(obj);
        if (dict == NULL) {
            set_dict_inline_values(obj, (PyDictObject *)new_dict);
            return 0;
        }
        if (_PyDict_DetachFromObject(dict, obj) == 0) {
            _PyObject_ManagedDictPointer(obj)->dict = (PyDictObject *)Py_XNewRef(new_dict);
            Py_DECREF(dict);
            return 0;
        }
        assert(new_dict == NULL);
        return -1;
#endif
    }
    else {
        PyDictObject *dict;

        Py_BEGIN_CRITICAL_SECTION(obj);

        dict = _PyObject_ManagedDictPointer(obj)->dict;

        FT_ATOMIC_STORE_PTR(_PyObject_ManagedDictPointer(obj)->dict,
                            (PyDictObject *)Py_XNewRef(new_dict));

        Py_END_CRITICAL_SECTION();
        decref_maybe_delay((PyObject *)dict, true);
    }
    assert(_PyObject_InlineValuesConsistencyCheck(obj));
    return err;
}

static int
detach_dict_from_object(PyDictObject *mp, PyObject *obj)
{
    assert(_PyObject_ManagedDictPointer(obj)->dict == mp);
    assert(_PyObject_InlineValuesConsistencyCheck(obj));

    if (FT_ATOMIC_LOAD_PTR_RELAXED(mp->ma_values) != _PyObject_InlineValues(obj)) {
        return 0;
    }

    // We could be called with an unlocked dict when the caller knows the
    // values are already detached, so we assert after inline values check.
    ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(mp);
    assert(mp->ma_values->embedded == 1);
    assert(mp->ma_values->valid == 1);
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES);

    PyDictValues *values = copy_values(mp->ma_values);

    if (values == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    mp->ma_values = values;

    invalidate_and_clear_inline_values(_PyObject_InlineValues(obj));

    assert(_PyObject_InlineValuesConsistencyCheck(obj));
    ASSERT_CONSISTENT(mp);
    return 0;
}


void
PyObject_ClearManagedDict(PyObject *obj)
{
    // This is called when the object is being freed or cleared
    // by the GC and therefore known to have no references.
    if (Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) {
        PyDictObject *dict = _PyObject_GetManagedDict(obj);
        if (dict == NULL) {
            // We have no materialized dictionary and inline values
            // that just need to be cleared.
            // No dict to clear, we're done
            clear_inline_values(_PyObject_InlineValues(obj));
            return;
        }
        else if (FT_ATOMIC_LOAD_PTR_RELAXED(dict->ma_values) ==
                    _PyObject_InlineValues(obj)) {
            // We have a materialized object which points at the inline
            // values. We need to materialize the keys. Nothing can modify
            // this object, but we need to lock the dictionary.
            int err;
            Py_BEGIN_CRITICAL_SECTION(dict);
            err = detach_dict_from_object(dict, obj);
            Py_END_CRITICAL_SECTION();

            if (err) {
                /* Must be out of memory */
                assert(PyErr_Occurred() == PyExc_MemoryError);
                PyErr_FormatUnraisable("Exception ignored while "
                                       "clearing an object managed dict");
                /* Clear the dict */
                Py_BEGIN_CRITICAL_SECTION(dict);
                PyInterpreterState *interp = _PyInterpreterState_GET();
                PyDictKeysObject *oldkeys = dict->ma_keys;
                set_keys(dict, Py_EMPTY_KEYS);
                dict->ma_values = NULL;
                dictkeys_decref(interp, oldkeys, IS_DICT_SHARED(dict));
                STORE_USED(dict, 0);
                clear_inline_values(_PyObject_InlineValues(obj));
                Py_END_CRITICAL_SECTION();
            }
        }
    }
    Py_CLEAR(_PyObject_ManagedDictPointer(obj)->dict);
}

int
_PyDict_DetachFromObject(PyDictObject *mp, PyObject *obj)
{
    ASSERT_WORLD_STOPPED_OR_OBJ_LOCKED(obj);

    return detach_dict_from_object(mp, obj);
}

static inline PyObject *
ensure_managed_dict(PyObject *obj)
{
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        PyTypeObject *tp = Py_TYPE(obj);
        if ((tp->tp_flags & Py_TPFLAGS_INLINE_VALUES) &&
            FT_ATOMIC_LOAD_UINT8(_PyObject_InlineValues(obj)->valid)) {
            dict = _PyObject_MaterializeManagedDict(obj);
        }
        else {
#ifdef Py_GIL_DISABLED
            // Check again that we're not racing with someone else creating the dict
            Py_BEGIN_CRITICAL_SECTION(obj);
            dict = _PyObject_GetManagedDict(obj);
            if (dict != NULL) {
                goto done;
            }
#endif
            dict = (PyDictObject *)new_dict_with_shared_keys(_PyInterpreterState_GET(),
                                                             CACHED_KEYS(tp));
            FT_ATOMIC_STORE_PTR_RELEASE(_PyObject_ManagedDictPointer(obj)->dict,
                                        (PyDictObject *)dict);

#ifdef Py_GIL_DISABLED
done:
            Py_END_CRITICAL_SECTION();
#endif
        }
    }
    return (PyObject *)dict;
}

static inline PyObject *
ensure_nonmanaged_dict(PyObject *obj, PyObject **dictptr)
{
    PyDictKeysObject *cached;

    PyObject *dict = FT_ATOMIC_LOAD_PTR_ACQUIRE(*dictptr);
    if (dict == NULL) {
#ifdef Py_GIL_DISABLED
        Py_BEGIN_CRITICAL_SECTION(obj);
        dict = *dictptr;
        if (dict != NULL) {
            goto done;
        }
#endif
        PyTypeObject *tp = Py_TYPE(obj);
        if (_PyType_HasFeature(tp, Py_TPFLAGS_HEAPTYPE) && (cached = CACHED_KEYS(tp))) {
            PyInterpreterState *interp = _PyInterpreterState_GET();
            assert(!_PyType_HasFeature(tp, Py_TPFLAGS_INLINE_VALUES));
            dict = new_dict_with_shared_keys(interp, cached);
        }
        else {
            dict = PyDict_New();
        }
        FT_ATOMIC_STORE_PTR_RELEASE(*dictptr, dict);
#ifdef Py_GIL_DISABLED
done:
        Py_END_CRITICAL_SECTION();
#endif
    }
    return dict;
}

PyObject *
PyObject_GenericGetDict(PyObject *obj, void *context)
{
    PyTypeObject *tp = Py_TYPE(obj);
    if (_PyType_HasFeature(tp, Py_TPFLAGS_MANAGED_DICT)) {
        return Py_XNewRef(ensure_managed_dict(obj));
    }
    else {
        PyObject **dictptr = _PyObject_ComputedDictPointer(obj);
        if (dictptr == NULL) {
            PyErr_SetString(PyExc_AttributeError,
                            "This object has no __dict__");
            return NULL;
        }

        return Py_XNewRef(ensure_nonmanaged_dict(obj, dictptr));
    }
}

int
_PyObjectDict_SetItem(PyTypeObject *tp, PyObject *obj, PyObject **dictptr,
                      PyObject *key, PyObject *value)
{
    PyObject *dict;
    int res;

    assert(dictptr != NULL);
    dict = ensure_nonmanaged_dict(obj, dictptr);
    if (dict == NULL) {
        return -1;
    }

    Py_BEGIN_CRITICAL_SECTION(dict);
    res = _PyDict_SetItem_LockHeld((PyDictObject *)dict, key, value);
    ASSERT_CONSISTENT(dict);
    Py_END_CRITICAL_SECTION();
    return res;
}

void
_PyDictKeys_DecRef(PyDictKeysObject *keys)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    dictkeys_decref(interp, keys, false);
}

static inline uint32_t
get_next_dict_keys_version(PyInterpreterState *interp)
{
#ifdef Py_GIL_DISABLED
    uint32_t v;
    do {
        v = _Py_atomic_load_uint32_relaxed(
            &interp->dict_state.next_keys_version);
        if (v == 0) {
            return 0;
        }
    } while (!_Py_atomic_compare_exchange_uint32(
        &interp->dict_state.next_keys_version, &v, v + 1));
#else
    if (interp->dict_state.next_keys_version == 0) {
        return 0;
    }
    uint32_t v = interp->dict_state.next_keys_version++;
#endif
    return v;
}

// In free-threaded builds the caller must ensure that the keys object is not
// being mutated concurrently by another thread.
uint32_t
_PyDictKeys_GetVersionForCurrentState(PyInterpreterState *interp,
                                      PyDictKeysObject *dictkeys)
{
    uint32_t dk_version = FT_ATOMIC_LOAD_UINT32_RELAXED(dictkeys->dk_version);
    if (dk_version != 0) {
        return dk_version;
    }
    dk_version = get_next_dict_keys_version(interp);
    FT_ATOMIC_STORE_UINT32_RELAXED(dictkeys->dk_version, dk_version);
    return dk_version;
}

uint32_t
_PyDict_GetKeysVersionForCurrentState(PyInterpreterState *interp,
                                      PyDictObject *dict)
{
    ASSERT_DICT_LOCKED((PyObject *) dict);
    uint32_t dk_version =
        _PyDictKeys_GetVersionForCurrentState(interp, dict->ma_keys);
    ensure_shared_on_keys_version_assignment(dict);
    return dk_version;
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
    ((PyDictObject*)dict)->_ma_watcher_tag |= (1LL << watcher_id);
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
    ((PyDictObject*)dict)->_ma_watcher_tag &= ~(1LL << watcher_id);
    return 0;
}

int
PyDict_AddWatcher(PyDict_WatchCallback callback)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();

    /* Start at 2, as 0 and 1 are reserved for CPython */
    for (int i = 2; i < DICT_MAX_WATCHERS; i++) {
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
                PyErr_FormatUnraisable(
                    "Exception ignored in %s watcher callback for <dict at %p>",
                    dict_event_name(event), mp);
            }
        }
        watcher_bits >>= 1;
    }
}

#ifndef NDEBUG
static int
_PyObject_InlineValuesConsistencyCheck(PyObject *obj)
{
    if ((Py_TYPE(obj)->tp_flags & Py_TPFLAGS_INLINE_VALUES) == 0) {
        return 1;
    }
    assert(Py_TYPE(obj)->tp_flags & Py_TPFLAGS_MANAGED_DICT);
    PyDictObject *dict = _PyObject_GetManagedDict(obj);
    if (dict == NULL) {
        return 1;
    }
    if (dict->ma_values == _PyObject_InlineValues(obj) ||
        _PyObject_InlineValues(obj)->valid == 0) {
        return 1;
    }
    assert(0);
    return 0;
}
#endif
