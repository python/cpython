#ifndef Py_INTERNAL_DICT_H
#define Py_INTERNAL_DICT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_dict_state.h"    // DICT_MAX_WATCHERS
#include "pycore_dict_struct.h"   // export PyDictKeysObject and DKIX_EMPTY
#include "pycore_interp.h"        // PyInterpreter.dict_state
#include "pycore_object.h"        // PyDictOrValues


// Unsafe flavor of PyDict_GetItemWithError(): no error checking
extern PyObject* _PyDict_GetItemWithError(PyObject *dp, PyObject *key);

extern int _PyDict_Contains_KnownHash(PyObject *, PyObject *, Py_hash_t);

extern int _PyDict_DelItemIf(PyObject *mp, PyObject *key,
                             int (*predicate)(PyObject *value));

extern int _PyDict_HasOnlyStringKeys(PyObject *mp);

extern void _PyDict_MaybeUntrack(PyObject *mp);

/* Like PyDict_Merge, but override can be 0, 1 or 2.  If override is 0,
   the first occurrence of a key wins, if override is 1, the last occurrence
   of a key wins, if override is 2, a KeyError with conflicting key as
   argument is raised.
*/
extern int _PyDict_MergeEx(PyObject *mp, PyObject *other, int override);

extern void _PyDict_DebugMallocStats(FILE *out);

/* runtime lifecycle */

extern void _PyDict_Fini(PyInterpreterState *interp);


/* other API */

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

extern Py_ssize_t _PyDict_LookupIndex(PyDictObject *, PyObject *);
extern Py_ssize_t _PyDictKeys_StringLookup(PyDictKeysObject* dictkeys, PyObject *key);
extern PyObject *_PyDict_LoadGlobal(PyDictObject *, PyDictObject *, PyObject *);

/* Consumes references to key and value */
extern int _PyDict_SetItem_Take2(PyDictObject *op, PyObject *key, PyObject *value);
extern int _PyObjectDict_SetItem(PyTypeObject *tp, PyObject **dictptr, PyObject *name, PyObject *value);

extern PyObject *_PyDict_Pop_KnownHash(PyObject *, PyObject *, Py_hash_t, PyObject *);

/* This must be no more than 250, for the prefix size to fit in one byte. */
#define SHARED_KEYS_MAX_SIZE 30
#define NEXT_LOG2_SHARED_KEYS_MAX_SIZE 6

#define DICT_VERSION_INCREMENT (1 << DICT_MAX_WATCHERS)
#define DICT_VERSION_MASK (DICT_VERSION_INCREMENT - 1)

#define DICT_NEXT_VERSION(INTERP) \
    ((INTERP)->dict_state.global_version += DICT_VERSION_INCREMENT)

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
    int watcher_bits = mp->ma_version_tag & DICT_VERSION_MASK;
    if (watcher_bits) {
        _PyDict_SendEvent(watcher_bits, event, mp, key, value);
        return DICT_NEXT_VERSION(interp) | watcher_bits;
    }
    return DICT_NEXT_VERSION(interp);
}

extern PyObject *_PyObject_MakeDictFromInstanceAttributes(PyObject *obj, PyDictValues *values);
extern bool _PyObject_MakeInstanceAttributesFromDict(PyObject *obj, PyDictOrValues *dorv);
extern PyObject *_PyDict_FromItems(
        PyObject *const *keys, Py_ssize_t keys_offset,
        PyObject *const *values, Py_ssize_t values_offset,
        Py_ssize_t length);

static inline void
_PyDictValues_AddToInsertionOrder(PyDictValues *values, Py_ssize_t ix)
{
    assert(ix < SHARED_KEYS_MAX_SIZE);
    uint8_t *size_ptr = ((uint8_t *)values)-2;
    int size = *size_ptr;
    assert(size+2 < ((uint8_t *)values)[-1]);
    size++;
    size_ptr[-size] = (uint8_t)ix;
    *size_ptr = size;
}

#ifdef __cplusplus
}
#endif
#endif   // !Py_INTERNAL_DICT_H
