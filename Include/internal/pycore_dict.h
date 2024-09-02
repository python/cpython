#ifndef Py_INTERNAL_DICT_H
#define Py_INTERNAL_DICT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist.h"             // _PyFreeListState
#include "pycore_identifier.h"           // _Py_Identifier
#include "pycore_object.h"               // PyManagedDictPointer
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_SSIZE_ACQUIRE

// Unsafe flavor of PyDict_GetItemWithError(): no error checking
extern PyObject* _PyDict_GetItemWithError(PyObject *dp, PyObject *key);

// Delete an item from a dict if a predicate is true
// Returns -1 on error, 1 if the item was deleted, 0 otherwise
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_DelItemIf(PyObject *mp, PyObject *key,
                                  int (*predicate)(PyObject *value, void *arg),
                                  void *arg);

// "KnownHash" variants
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key,
                                          PyObject *item, Py_hash_t hash);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_DelItem_KnownHash(PyObject *mp, PyObject *key,
                                          Py_hash_t hash);
extern int _PyDict_Contains_KnownHash(PyObject *, PyObject *, Py_hash_t);

// "Id" variants
extern PyObject* _PyDict_GetItemIdWithError(PyObject *dp,
                                            _Py_Identifier *key);
extern int _PyDict_ContainsId(PyObject *, _Py_Identifier *);
extern int _PyDict_SetItemId(PyObject *dp, _Py_Identifier *key, PyObject *item);
extern int _PyDict_DelItemId(PyObject *mp, _Py_Identifier *key);

extern int _PyDict_Next(
    PyObject *mp, Py_ssize_t *pos, PyObject **key, PyObject **value, Py_hash_t *hash);

extern int _PyDict_HasOnlyStringKeys(PyObject *mp);

extern void _PyDict_MaybeUntrack(PyObject *mp);

// Export for '_ctypes' shared extension
PyAPI_FUNC(Py_ssize_t) _PyDict_SizeOf(PyDictObject *);

#define _PyDict_HasSplitTable(d) ((d)->ma_values != NULL)

/* Like PyDict_Merge, but override can be 0, 1 or 2.  If override is 0,
   the first occurrence of a key wins, if override is 1, the last occurrence
   of a key wins, if override is 2, a KeyError with conflicting key as
   argument is raised.
*/
PyAPI_FUNC(int) _PyDict_MergeEx(PyObject *mp, PyObject *other, int override);

extern void _PyDict_DebugMallocStats(FILE *out);


/* _PyDictView */

typedef struct {
    PyObject_HEAD
    PyDictObject *dv_dict;
} _PyDictViewObject;

extern PyObject* _PyDictView_New(PyObject *, PyTypeObject *);
extern PyObject* _PyDictView_Intersect(PyObject* self, PyObject *other);

/* other API */

typedef struct {
    /* Cached hash code of me_key. */
    Py_hash_t me_hash;
    PyObject *me_key;
    PyObject *me_value; /* This field is only meaningful for combined tables */
} PyDictKeyEntry;

typedef struct {
    PyObject *me_key;   /* The key must be Unicode and have hash. */
    PyObject *me_value; /* This field is only meaningful for combined tables */
} PyDictUnicodeEntry;

extern PyDictKeysObject *_PyDict_NewKeysForClass(void);
extern PyObject *_PyDict_FromKeys(PyObject *, PyObject *, PyObject *);

/* Gets a version number unique to the current state of the keys of dict, if possible.
 * Returns the version number, or zero if it was not possible to get a version number. */
extern uint32_t _PyDictKeys_GetVersionForCurrentState(
        PyInterpreterState *interp, PyDictKeysObject *dictkeys);

extern size_t _PyDict_KeysSize(PyDictKeysObject *keys);

extern void _PyDictKeys_DecRef(PyDictKeysObject *keys);

/* _Py_dict_lookup() returns index of entry which can be used like DK_ENTRIES(dk)[index].
 * -1 when no entry found, -3 when compare raises error.
 */
extern Py_ssize_t _Py_dict_lookup(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr);
extern Py_ssize_t _Py_dict_lookup_threadsafe(PyDictObject *mp, PyObject *key, Py_hash_t hash, PyObject **value_addr);

extern Py_ssize_t _PyDict_LookupIndex(PyDictObject *, PyObject *);
extern Py_ssize_t _PyDictKeys_StringLookup(PyDictKeysObject* dictkeys, PyObject *key);
PyAPI_FUNC(PyObject *)_PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);

/* Consumes references to key and value */
PyAPI_FUNC(int) _PyDict_SetItem_Take2(PyDictObject *op, PyObject *key, PyObject *value);
extern int _PyDict_SetItem_LockHeld(PyDictObject *dict, PyObject *name, PyObject *value);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash_LockHeld(PyDictObject *mp, PyObject *key,
                                                   PyObject *value, Py_hash_t hash);
// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyDict_GetItemRef_KnownHash_LockHeld(PyDictObject *op, PyObject *key, Py_hash_t hash, PyObject **result);
extern int _PyDict_GetItemRef_KnownHash(PyDictObject *op, PyObject *key, Py_hash_t hash, PyObject **result);
extern int _PyDict_GetItemRef_Unicode_LockHeld(PyDictObject *op, PyObject *key, PyObject **result);
extern int _PyObjectDict_SetItem(PyTypeObject *tp, PyObject *obj, PyObject **dictptr, PyObject *name, PyObject *value);

extern int _PyDict_Pop_KnownHash(
    PyDictObject *dict,
    PyObject *key,
    Py_hash_t hash,
    PyObject **result);

#define DKIX_EMPTY (-1)
#define DKIX_DUMMY (-2)  /* Used internally */
#define DKIX_ERROR (-3)
#define DKIX_KEY_CHANGED (-4) /* Used internally */

typedef enum {
    DICT_KEYS_GENERAL = 0,
    DICT_KEYS_UNICODE = 1,
    DICT_KEYS_SPLIT = 2
} DictKeysKind;

/* See dictobject.c for actual layout of DictKeysObject */
struct _dictkeysobject {
    Py_ssize_t dk_refcnt;

    /* Size of the hash table (dk_indices). It must be a power of 2. */
    uint8_t dk_log2_size;

    /* Size of the hash table (dk_indices) by bytes. */
    uint8_t dk_log2_index_bytes;

    /* Kind of keys */
    uint8_t dk_kind;

#ifdef Py_GIL_DISABLED
    /* Lock used to protect shared keys */
    PyMutex dk_mutex;
#endif

    /* Version number -- Reset to 0 by any modification to keys */
    uint32_t dk_version;

    /* Number of usable entries in dk_entries. */
    Py_ssize_t dk_usable;

    /* Number of used entries in dk_entries. */
    Py_ssize_t dk_nentries;


    /* Actual hash table of dk_size entries. It holds indices in dk_entries,
       or DKIX_EMPTY(-1) or DKIX_DUMMY(-2).

       Indices must be: 0 <= indice < USABLE_FRACTION(dk_size).

       The size in bytes of an indice depends on dk_size:

       - 1 byte if dk_size <= 0xff (char*)
       - 2 bytes if dk_size <= 0xffff (int16_t*)
       - 4 bytes if dk_size <= 0xffffffff (int32_t*)
       - 8 bytes otherwise (int64_t*)

       Dynamically sized, SIZEOF_VOID_P is minimum. */
    char dk_indices[];  /* char is required to avoid strict aliasing. */

    /* "PyDictKeyEntry or PyDictUnicodeEntry dk_entries[USABLE_FRACTION(DK_SIZE(dk))];" array follows:
       see the DK_ENTRIES() / DK_UNICODE_ENTRIES() functions below */
};

/* This must be no more than 250, for the prefix size to fit in one byte. */
#define SHARED_KEYS_MAX_SIZE 30
#define NEXT_LOG2_SHARED_KEYS_MAX_SIZE 6

/* Layout of dict values:
 *
 * The PyObject *values are preceded by an array of bytes holding
 * the insertion order and size.
 * [-1] = prefix size. [-2] = used size. size[-2-n...] = insertion order.
 */
struct _dictvalues {
    uint8_t capacity;
    uint8_t size;
    uint8_t embedded;
    uint8_t valid;
    PyObject *values[1];
};

#define DK_LOG_SIZE(dk)  _Py_RVALUE((dk)->dk_log2_size)
#if SIZEOF_VOID_P > 4
#define DK_SIZE(dk)      (((int64_t)1)<<DK_LOG_SIZE(dk))
#else
#define DK_SIZE(dk)      (1<<DK_LOG_SIZE(dk))
#endif

static inline void* _DK_ENTRIES(PyDictKeysObject *dk) {
    int8_t *indices = (int8_t*)(dk->dk_indices);
    size_t index = (size_t)1 << dk->dk_log2_index_bytes;
    return (&indices[index]);
}

static inline PyDictKeyEntry* DK_ENTRIES(PyDictKeysObject *dk) {
    assert(dk->dk_kind == DICT_KEYS_GENERAL);
    return (PyDictKeyEntry*)_DK_ENTRIES(dk);
}
static inline PyDictUnicodeEntry* DK_UNICODE_ENTRIES(PyDictKeysObject *dk) {
    assert(dk->dk_kind != DICT_KEYS_GENERAL);
    return (PyDictUnicodeEntry*)_DK_ENTRIES(dk);
}

#define DK_IS_UNICODE(dk) ((dk)->dk_kind != DICT_KEYS_GENERAL)

#define DICT_VERSION_INCREMENT (1 << (DICT_MAX_WATCHERS + DICT_WATCHED_MUTATION_BITS))
#define DICT_WATCHER_MASK ((1 << DICT_MAX_WATCHERS) - 1)
#define DICT_WATCHER_AND_MODIFICATION_MASK ((1 << (DICT_MAX_WATCHERS + DICT_WATCHED_MUTATION_BITS)) - 1)

#ifdef Py_GIL_DISABLED

#define THREAD_LOCAL_DICT_VERSION_COUNT 256
#define THREAD_LOCAL_DICT_VERSION_BATCH THREAD_LOCAL_DICT_VERSION_COUNT * DICT_VERSION_INCREMENT

static inline uint64_t
dict_next_version(PyInterpreterState *interp)
{
    PyThreadState *tstate = PyThreadState_GET();
    uint64_t cur_progress = (tstate->dict_global_version &
                            (THREAD_LOCAL_DICT_VERSION_BATCH - 1));
    if (cur_progress == 0) {
        uint64_t next = _Py_atomic_add_uint64(&interp->dict_state.global_version,
                                              THREAD_LOCAL_DICT_VERSION_BATCH);
        tstate->dict_global_version = next;
    }
    return tstate->dict_global_version += DICT_VERSION_INCREMENT;
}

#define DICT_NEXT_VERSION(INTERP) dict_next_version(INTERP)

#else
#define DICT_NEXT_VERSION(INTERP) \
    ((INTERP)->dict_state.global_version += DICT_VERSION_INCREMENT)
#endif

void
_PyDict_SendEvent(int watcher_bits,
                  PyDict_WatchEvent event,
                  PyDictObject *mp,
                  PyObject *key,
                  PyObject *value);

static inline uint64_t
_PyDict_NotifyEvent(PyInterpreterState *interp,
                    PyDict_WatchEvent event,
                    PyDictObject *mp,
                    PyObject *key,
                    PyObject *value)
{
    assert(Py_REFCNT((PyObject*)mp) > 0);
    int watcher_bits = mp->ma_version_tag & DICT_WATCHER_MASK;
    if (watcher_bits) {
        RARE_EVENT_STAT_INC(watched_dict_modification);
        _PyDict_SendEvent(watcher_bits, event, mp, key, value);
    }
    return DICT_NEXT_VERSION(interp) | (mp->ma_version_tag & DICT_WATCHER_AND_MODIFICATION_MASK);
}

extern PyDictObject *_PyObject_MaterializeManagedDict(PyObject *obj);

PyAPI_FUNC(PyObject *)_PyDict_FromItems(
        PyObject *const *keys, Py_ssize_t keys_offset,
        PyObject *const *values, Py_ssize_t values_offset,
        Py_ssize_t length);

static inline uint8_t *
get_insertion_order_array(PyDictValues *values)
{
    return (uint8_t *)&values->values[values->capacity];
}

static inline void
_PyDictValues_AddToInsertionOrder(PyDictValues *values, Py_ssize_t ix)
{
    assert(ix < SHARED_KEYS_MAX_SIZE);
    int size = values->size;
    uint8_t *array = get_insertion_order_array(values);
    assert(size < values->capacity);
    assert(((uint8_t)ix) == ix);
    array[size] = (uint8_t)ix;
    values->size = size+1;
}

static inline size_t
shared_keys_usable_size(PyDictKeysObject *keys)
{
    // dk_usable will decrease for each instance that is created and each
    // value that is added.  dk_nentries will increase for each value that
    // is added.  We want to always return the right value or larger.
    // We therefore increase dk_nentries first and we decrease dk_usable
    // second, and conversely here we read dk_usable first and dk_entries
    // second (to avoid the case where we read entries before the increment
    // and read usable after the decrement)
    Py_ssize_t dk_usable = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_usable);
    Py_ssize_t dk_nentries = FT_ATOMIC_LOAD_SSIZE_ACQUIRE(keys->dk_nentries);
    return dk_nentries + dk_usable;
}

static inline size_t
_PyInlineValuesSize(PyTypeObject *tp)
{
    PyDictKeysObject *keys = ((PyHeapTypeObject*)tp)->ht_cached_keys;
    assert(keys != NULL);
    size_t size = shared_keys_usable_size(keys);
    size_t prefix_size = _Py_SIZE_ROUND_UP(size, sizeof(PyObject *));
    assert(prefix_size < 256);
    return prefix_size + (size + 1) * sizeof(PyObject *);
}

int
_PyDict_DetachFromObject(PyDictObject *dict, PyObject *obj);

PyDictObject *_PyObject_MaterializeManagedDict_LockHeld(PyObject *);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DICT_H */
