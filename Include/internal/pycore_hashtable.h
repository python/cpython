#ifndef Py_INTERNAL_HASHTABLE_H
#define Py_INTERNAL_HASHTABLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Single linked list */

typedef struct _Py_slist_item_s {
    struct _Py_slist_item_s *next;
} _Py_slist_item_t;

typedef struct {
    _Py_slist_item_t *head;
} _Py_slist_t;

#define _Py_SLIST_ITEM_NEXT(ITEM) (((_Py_slist_item_t *)ITEM)->next)

#define _Py_SLIST_HEAD(SLIST) (((_Py_slist_t *)SLIST)->head)


/* _Py_hashtable: table entry */

typedef struct {
    /* used by _Py_hashtable_t.buckets to link entries */
    _Py_slist_item_t _Py_slist_item;

    Py_uhash_t key_hash;
    void *key;
    /* data (data_size bytes) follows */
} _Py_hashtable_entry_t;

#define _Py_HASHTABLE_ENTRY_PDATA(TABLE, ENTRY) \
        ((const void *)((char *)(ENTRY) \
                        + sizeof(_Py_hashtable_entry_t)))

#define _Py_HASHTABLE_ENTRY_READ_DATA(TABLE, ENTRY, DATA) \
    do { \
        assert(sizeof(DATA) == (TABLE)->data_size); \
        memcpy(&(DATA), _Py_HASHTABLE_ENTRY_PDATA(TABLE, (ENTRY)), \
                  sizeof(DATA)); \
    } while (0)

#define _Py_HASHTABLE_ENTRY_WRITE_DATA(TABLE, ENTRY, DATA) \
    do { \
        assert(sizeof(DATA) == (TABLE)->data_size); \
        memcpy((void *)_Py_HASHTABLE_ENTRY_PDATA((TABLE), (ENTRY)), \
                  &(DATA), sizeof(DATA)); \
    } while (0)


/* _Py_hashtable: prototypes */

/* Forward declaration */
struct _Py_hashtable_t;
typedef struct _Py_hashtable_t _Py_hashtable_t;

typedef Py_uhash_t (*_Py_hashtable_hash_func) (const void *key);
typedef int (*_Py_hashtable_compare_func) (const void *key1, const void *key2);
typedef _Py_hashtable_entry_t* (*_Py_hashtable_get_entry_func)(_Py_hashtable_t *ht,
                                                               const void *key);
typedef int (*_Py_hashtable_get_func) (_Py_hashtable_t *ht,
                                       const void *key, void *data);

typedef struct {
    /* allocate a memory block */
    void* (*malloc) (size_t size);

    /* release a memory block */
    void (*free) (void *ptr);
} _Py_hashtable_allocator_t;


/* _Py_hashtable: table */
struct _Py_hashtable_t {
    size_t num_buckets;
    size_t entries; /* Total number of entries in the table. */
    _Py_slist_t *buckets;
    size_t data_size;

    _Py_hashtable_get_func get_func;
    _Py_hashtable_get_entry_func get_entry_func;
    _Py_hashtable_hash_func hash_func;
    _Py_hashtable_compare_func compare_func;
    _Py_hashtable_allocator_t alloc;
};

/* hash a pointer (void*) */
PyAPI_FUNC(Py_uhash_t) _Py_hashtable_hash_ptr(const void *key);

/* comparison using memcmp() */
PyAPI_FUNC(int) _Py_hashtable_compare_direct(
    const void *key1,
    const void *key2);

PyAPI_FUNC(_Py_hashtable_t *) _Py_hashtable_new(
    size_t data_size,
    _Py_hashtable_hash_func hash_func,
    _Py_hashtable_compare_func compare_func);

PyAPI_FUNC(_Py_hashtable_t *) _Py_hashtable_new_full(
    size_t data_size,
    size_t init_size,
    _Py_hashtable_hash_func hash_func,
    _Py_hashtable_compare_func compare_func,
    _Py_hashtable_allocator_t *allocator);

PyAPI_FUNC(void) _Py_hashtable_destroy(_Py_hashtable_t *ht);

/* Return a copy of the hash table */
PyAPI_FUNC(_Py_hashtable_t *) _Py_hashtable_copy(_Py_hashtable_t *src);

PyAPI_FUNC(void) _Py_hashtable_clear(_Py_hashtable_t *ht);

typedef int (*_Py_hashtable_foreach_func) (_Py_hashtable_t *ht,
                                           _Py_hashtable_entry_t *entry,
                                           void *arg);

/* Call func() on each entry of the hashtable.
   Iteration stops if func() result is non-zero, in this case it's the result
   of the call. Otherwise, the function returns 0. */
PyAPI_FUNC(int) _Py_hashtable_foreach(
    _Py_hashtable_t *ht,
    _Py_hashtable_foreach_func func,
    void *arg);

PyAPI_FUNC(size_t) _Py_hashtable_size(_Py_hashtable_t *ht);

/* Add a new entry to the hash. The key must not be present in the hash table.
   Return 0 on success, -1 on memory error.

   Don't call directly this function,
   but use _Py_HASHTABLE_SET() and _Py_HASHTABLE_SET_NODATA() macros */
PyAPI_FUNC(int) _Py_hashtable_set(
    _Py_hashtable_t *ht,
    const void *key,
    size_t data_size,
    const void *data);

#define _Py_HASHTABLE_SET(TABLE, KEY, DATA) \
    _Py_hashtable_set(TABLE, (KEY), sizeof(DATA), &(DATA))

#define _Py_HASHTABLE_SET_NODATA(TABLE, KEY) \
    _Py_hashtable_set(TABLE, (KEY), 0, NULL)


/* Get an entry.
   Return NULL if the key does not exist.

   Don't call directly this function, but use _Py_HASHTABLE_GET_ENTRY()
   macro */
static inline _Py_hashtable_entry_t *
_Py_hashtable_get_entry(_Py_hashtable_t *ht, const void *key)
{
    return ht->get_entry_func(ht, key);
}

#define _Py_HASHTABLE_GET_ENTRY(TABLE, KEY) \
    _Py_hashtable_get_entry(TABLE, (const void *)(KEY))


/* Get data from an entry. Copy entry data into data and return 1 if the entry
   exists, return 0 if the entry does not exist.

   Don't call directly this function, but use _Py_HASHTABLE_GET() macro */
static inline int
_Py_hashtable_get(_Py_hashtable_t *ht, const void *key,
                  size_t data_size, void *data)
{
    assert(data_size == ht->data_size);
    return ht->get_func(ht, key, data);
}

#define _Py_HASHTABLE_GET(TABLE, KEY, DATA) \
    _Py_hashtable_get(TABLE, (KEY), sizeof(DATA), &(DATA))


/* Don't call directly this function, but use _Py_HASHTABLE_POP() macro */
PyAPI_FUNC(int) _Py_hashtable_pop(
    _Py_hashtable_t *ht,
    const void *key,
    size_t data_size,
    void *data);

#define _Py_HASHTABLE_POP(TABLE, KEY, DATA) \
    _Py_hashtable_pop(TABLE, (KEY), sizeof(DATA), &(DATA))


#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_HASHTABLE_H */
