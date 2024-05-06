#ifndef Py_CPYTHON_DICTOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct _dictkeysobject PyDictKeysObject;
typedef struct _dictvalues PyDictValues;

/* The ma_values pointer is NULL for a combined table
 * or points to an array of PyObject* for a split table
 */
typedef struct {
    PyObject_HEAD

    /* Number of items in the dictionary */
    Py_ssize_t ma_used;

    /* Dictionary version: globally unique, value change each time
       the dictionary is modified */
#ifdef Py_BUILD_CORE
    /* Bits 0-7 are for dict watchers.
     * Bits 8-11 are for the watched mutation counter (used by tier2 optimization)
     * The remaining bits (12-63) are the actual version tag. */
    uint64_t ma_version_tag;
#else
    Py_DEPRECATED(3.12) uint64_t ma_version_tag;
#endif

    PyDictKeysObject *ma_keys;

    /* If ma_values is NULL, the table is "combined": keys and values
       are stored in ma_keys.

       If ma_values is not NULL, the table is split:
       keys are stored in ma_keys and values are stored in ma_values */
    PyDictValues *ma_values;
} PyDictObject;

PyAPI_FUNC(PyObject *) _PyDict_GetItem_KnownHash(PyObject *mp, PyObject *key,
                                                 Py_hash_t hash);
PyAPI_FUNC(PyObject *) _PyDict_GetItemStringWithError(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyDict_SetDefault(
    PyObject *mp, PyObject *key, PyObject *defaultobj);

// Inserts `key` with a value `default_value`, if `key` is not already present
// in the dictionary.  If `result` is not NULL, then the value associated
// with `key` is returned in `*result` (either the existing value, or the now
// inserted `default_value`).
// Returns:
//   -1 on error
//    0 if `key` was not present and `default_value` was inserted
//    1 if `key` was present and `default_value` was not inserted
PyAPI_FUNC(int) PyDict_SetDefaultRef(PyObject *mp, PyObject *key, PyObject *default_value, PyObject **result);

/* Get the number of items of a dictionary. */
static inline Py_ssize_t PyDict_GET_SIZE(PyObject *op) {
    PyDictObject *mp;
    assert(PyDict_Check(op));
    mp = _Py_CAST(PyDictObject*, op);
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_ssize_relaxed(&mp->ma_used);
#else
    return mp->ma_used;
#endif
}
#define PyDict_GET_SIZE(op) PyDict_GET_SIZE(_PyObject_CAST(op))

PyAPI_FUNC(int) PyDict_ContainsString(PyObject *mp, const char *key);

PyAPI_FUNC(PyObject *) _PyDict_NewPresized(Py_ssize_t minused);

PyAPI_FUNC(int) PyDict_Pop(PyObject *dict, PyObject *key, PyObject **result);
PyAPI_FUNC(int) PyDict_PopString(PyObject *dict, const char *key, PyObject **result);
PyAPI_FUNC(PyObject *) _PyDict_Pop(PyObject *dict, PyObject *key, PyObject *default_value);

/* Dictionary watchers */

#define PY_FOREACH_DICT_EVENT(V) \
    V(ADDED)                     \
    V(MODIFIED)                  \
    V(DELETED)                   \
    V(CLONED)                    \
    V(CLEARED)                   \
    V(DEALLOCATED)

typedef enum {
    #define PY_DEF_EVENT(EVENT) PyDict_EVENT_##EVENT,
    PY_FOREACH_DICT_EVENT(PY_DEF_EVENT)
    #undef PY_DEF_EVENT
} PyDict_WatchEvent;

// Callback to be invoked when a watched dict is cleared, dealloced, or modified.
// In clear/dealloc case, key and new_value will be NULL. Otherwise, new_value will be the
// new value for key, NULL if key is being deleted.
typedef int(*PyDict_WatchCallback)(PyDict_WatchEvent event, PyObject* dict, PyObject* key, PyObject* new_value);

// Register/unregister a dict-watcher callback
PyAPI_FUNC(int) PyDict_AddWatcher(PyDict_WatchCallback callback);
PyAPI_FUNC(int) PyDict_ClearWatcher(int watcher_id);

// Mark given dictionary as "watched" (callback will be called if it is modified)
PyAPI_FUNC(int) PyDict_Watch(int watcher_id, PyObject* dict);
PyAPI_FUNC(int) PyDict_Unwatch(int watcher_id, PyObject* dict);
