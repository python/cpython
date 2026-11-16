// Per-type method cache implementation

// The cache is used for method and attribute lookups on type objects.
// The stored names are always interned strings, and the
// stored values are borrowed references to the corresponding method or attribute object.
// For static types, the cache is stored on the per-interpreter managed_static_type_state,
// and for heap types the cache is stored in the `PyTypeObject._tp_cache` field.

#include "Python.h"
#include "pycore_typecache.h"
#include "pycore_interp.h"        // PyInterpreterState
#include "pycore_pymem.h"
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_typeobject.h"    // _PyStaticType_GetState()
#include "pycore_stats.h"         // OBJECT_STAT_INC


// The empty cache is statically allocated and shared across all the types,
// when a type is modified, the cache of type is set to the empty cache
// and when a cache entry is inserted to the empty cache, a new cache is
// allocated for the type and the entry is inserted to the new cache.
static struct type_cache empty_cache = {
    .mask = _Py_TYPECACHE_MINSIZE - 1,
    .version_tag = 0,
    .available = 0,
    .used = 0,
};

static inline uint32_t
cache_size(struct type_cache *cache)
{
    return cache->mask + 1;
}

static inline size_t
cache_nbytes(struct type_cache *cache)
{
    return offsetof(struct type_cache, hashtable)
        + (size_t)cache_size(cache) * sizeof(struct type_cache_entry);
}

static struct type_cache *
cache_allocate(uint32_t size)
{
    // size must be a power of two
    assert((size & (size - 1)) == 0);
    size_t nbytes = offsetof(struct type_cache, hashtable)
                    + (size_t)size * sizeof(struct type_cache_entry);
    struct type_cache *cache = PyMem_Calloc(1, nbytes);
    if (cache == NULL) {
        return NULL;
    }
    cache->mask = size - 1;
    // load factor of 0.75
    cache->available = size - (size >> 2);
    cache->used = 0;
    return cache;
}

static void
cache_free_delayed(struct type_cache *cache)
{
    if (cache == NULL || cache == &empty_cache) {
        return;
    }
#ifndef Py_GIL_DISABLED
    // On gil-enabled builds, the cache owns strong references to the interned strings,
    // so we need to decref them before freeing the cache memory.
    for (uint32_t i = 0; i < cache_size(cache); i++) {
        if (cache->hashtable[i].name != NULL) {
            Py_DECREF(cache->hashtable[i].name);
        }
    }
#endif
    // Delay the freeing of old cache for concurrent lock-free readers
    _PyMem_FreeDelayed(cache, cache_nbytes(cache));
}


static inline void **
cache_slot(PyTypeObject *type)
{
    if (type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN) {
        PyInterpreterState *interp = _PyInterpreterState_GET();
        managed_static_type_state *state = _PyStaticType_GetState(interp, type);
        assert(state != NULL);
        return &state->_tp_cache;
    }
    return &type->_tp_cache;
}

static inline struct type_cache *
cache_get(PyTypeObject *type)
{
    return (struct type_cache *)FT_ATOMIC_LOAD_PTR_ACQUIRE(*cache_slot(type));
}

static inline void
cache_set(PyTypeObject *type, struct type_cache *cache)
{
    FT_ATOMIC_STORE_PTR_RELEASE(*cache_slot(type), cache);
}

void
_PyTypeCache_InitType(PyTypeObject *type)
{
    *cache_slot(type) = &empty_cache;
}

static inline void
cache_insert(struct type_cache *cache, PyObject *name,
             PyObject *value)
{
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    for (;;) {
        if (cache->hashtable[index].name == NULL) {
#ifndef Py_GIL_DISABLED
            // On free-threading, all interned strings are immortal.
            Py_INCREF(name);
#endif
            cache->hashtable[index].value = value;
            FT_ATOMIC_STORE_PTR_RELEASE(cache->hashtable[index].name, name);
            cache->used++;
            cache->available--;
            return;
        }
        else if (cache->hashtable[index].name == name) {
            /* someone else added the entry before us. */
            return;
        }
        index = (index + 1) & cache->mask;
    }
}

static inline int
cache_resize(PyTypeObject *type, struct type_cache *cache)
{
    uint32_t old_size = cache_size(cache);
    uint32_t new_size;
    if (cache->used == 0) {
        // the cache is the empty cache, we need to allocate a new cache with the minimum size
        new_size = _Py_TYPECACHE_MINSIZE;
    }
    else {
        // double the cache size when resizing
        new_size = old_size * 2;
    }
    if (new_size > _Py_TYPECACHE_MAXSIZE) {
        // The new size is too big, don't resize and just return.
        OBJECT_STAT_INC(type_cache_too_big);
        return -1;
    }
    struct type_cache *new_cache = cache_allocate(new_size);
    if (new_cache == NULL) {
        return -1;
    }
    OBJECT_STAT_INC(type_cache_resizes);
    for (uint32_t i = 0; i < old_size; i++) {
        if (cache->hashtable[i].name != NULL) {
            cache_insert(new_cache, cache->hashtable[i].name, cache->hashtable[i].value);
        }
    }
    new_cache->version_tag = type->tp_version_tag;
    cache_set(type, new_cache);
    cache_free_delayed(cache);
    return 0;
}

// Insert a new entry to the type cache.
// The TYPE_LOCK should be held while calling this function.
void
_PyTypeCache_Insert(PyTypeObject *type, PyObject *name, PyObject *value)
{
    struct type_cache *cache = cache_get(type);
    // If the cache is full, resize it before inserting the new entry.
    // this also handles the case of empty cache where available is 0 but there are no entries.
    if (cache->available == 0) {
        if (cache_resize(type, cache) == -1) {
            // out of memory, don't cache the value
            return;
        }
        cache = cache_get(type);
        assert(cache->available > 0);
    }
    assert(cache->version_tag == type->tp_version_tag);
    cache_insert(cache, name, value);
}


// Lookup the given name in the type cache.
// The cache is lock-free so it is possible that cache becomes stale during the lookup,
// to prevent returning stale cache entry, the cache version is compared with the type version tag.
struct _PyTypeCacheLookupResult
_PyTypeCache_Lookup(PyTypeObject *type, PyObject *name)
{
    assert(PyUnicode_CheckExact(name) && PyUnicode_CHECK_INTERNED(name));
    struct _PyTypeCacheLookupResult miss = {PyStackRef_NULL, 0, 0};
    struct type_cache *cache = cache_get(type);
    if (cache == NULL) {
        return miss;
    }
    Py_hash_t hash = PyUnstable_Unicode_GET_CACHED_HASH(name);
    assert(hash != -1);
    uint32_t index = hash & cache->mask;
    _PyStackRef out_ref;
    for (;;) {
        PyObject *entry_name = FT_ATOMIC_LOAD_PTR_ACQUIRE(cache->hashtable[index].name);
        if (entry_name == name) {
#ifdef Py_GIL_DISABLED
            if (!_Py_TryXGetStackRef(&cache->hashtable[index].value, &out_ref)) {
                return miss;
            }
#else
            PyObject *v = cache->hashtable[index].value;
            out_ref = v ? PyStackRef_FromPyObjectNew(v) : PyStackRef_NULL;
#endif
            break;
        }
        else if (entry_name == NULL) {
            return miss;
        }
        index = (index + 1) & cache->mask;
    }
    // Check the cache version against the type version tag to maintain
    // consistency with find_name_in_mro and prevent stale cache reads
    if (cache->version_tag != FT_ATOMIC_LOAD_UINT_RELAXED(type->tp_version_tag)) {
        PyStackRef_XCLOSE(out_ref);
        return miss;
    }
    return (struct _PyTypeCacheLookupResult){out_ref, 1, cache->version_tag};
}

// Invalidate the type cache of the type.
// The cache is set to the empty cache and the old cache is freed with QSBR.
// The TYPE_LOCK should be held while calling this function.
void
_PyTypeCache_Invalidate(PyTypeObject *type)
{
    OBJECT_STAT_INC(type_cache_invalidations);
    struct type_cache *cache = cache_get(type);
    cache_set(type, &empty_cache);
    cache_free_delayed(cache);
}
