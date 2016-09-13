#ifndef Py_HASHTABLE_H
#define Py_HASHTABLE_H
/* The whole API is private */
#ifndef Py_LIMITED_API

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

    /* key (key_size bytes) and then data (data_size bytes) follows */
} _Py_hashtable_entry_t;

#define _Py_HASHTABLE_ENTRY_PKEY(ENTRY) \
        ((const void *)((char *)(ENTRY) \
                        + sizeof(_Py_hashtable_entry_t)))

#define _Py_HASHTABLE_ENTRY_PDATA(TABLE, ENTRY) \
        ((const void *)((char *)(ENTRY) \
                        + sizeof(_Py_hashtable_entry_t) \
                        + (TABLE)->key_size))

/* Get a key value from pkey: use memcpy() rather than a pointer dereference
   to avoid memory alignment issues. */
#define _Py_HASHTABLE_READ_KEY(TABLE, PKEY, DST_KEY) \
    do { \
        assert(sizeof(DST_KEY) == (TABLE)->key_size); \
        memcpy(&(DST_KEY), (PKEY), sizeof(DST_KEY)); \
    } while (0)

#define _Py_HASHTABLE_ENTRY_READ_KEY(TABLE, ENTRY, KEY) \
    do { \
        assert(sizeof(KEY) == (TABLE)->key_size); \
        memcpy(&(KEY), _Py_HASHTABLE_ENTRY_PKEY(ENTRY), sizeof(KEY)); \
    } while (0)

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

typedef Py_uhash_t (*_Py_hashtable_hash_func) (struct _Py_hashtable_t *ht,
                                               const void *pkey);
typedef int (*_Py_hashtable_compare_func) (struct _Py_hashtable_t *ht,
                                           const void *pkey,
                                           const _Py_hashtable_entry_t *he);

typedef struct {
    /* allocate a memory block */
    void* (*malloc) (size_t size);

    /* release a memory block */
    void (*free) (void *ptr);
} _Py_hashtable_allocator_t;


/* _Py_hashtable: table */

typedef struct _Py_hashtable_t {
    size_t num_buckets;
    size_t entries; /* Total number of entries in the table. */
    _Py_slist_t *buckets;
    size_t key_size;
    size_t data_size;

    _Py_hashtable_hash_func hash_func;
    _Py_hashtable_compare_func compare_func;
    _Py_hashtable_allocator_t alloc;
} _Py_hashtable_t;

/* hash a pointer (void*) */
PyAPI_FUNC(Py_uhash_t) _Py_hashtable_hash_ptr(
    struct _Py_hashtable_t *ht,
    const void *pkey);

/* comparison using memcmp() */
PyAPI_FUNC(int) _Py_hashtable_compare_direct(
    _Py_hashtable_t *ht,
    const void *pkey,
    const _Py_hashtable_entry_t *entry);

PyAPI_FUNC(_Py_hashtable_t *) _Py_hashtable_new(
    size_t key_size,
    size_t data_size,
    _Py_hashtable_hash_func hash_func,
    _Py_hashtable_compare_func compare_func);

PyAPI_FUNC(_Py_hashtable_t *) _Py_hashtable_new_full(
    size_t key_size,
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
    size_t key_size,
    const void *pkey,
    size_t data_size,
    const void *data);

#define _Py_HASHTABLE_SET(TABLE, KEY, DATA) \
    _Py_hashtable_set(TABLE, sizeof(KEY), &(KEY), sizeof(DATA), &(DATA))

#define _Py_HASHTABLE_SET_NODATA(TABLE, KEY) \
    _Py_hashtable_set(TABLE, sizeof(KEY), &(KEY), 0, NULL)


/* Get an entry.
   Return NULL if the key does not exist.

   Don't call directly this function, but use _Py_HASHTABLE_GET_ENTRY()
   macro */
PyAPI_FUNC(_Py_hashtable_entry_t*) _Py_hashtable_get_entry(
    _Py_hashtable_t *ht,
    size_t key_size,
    const void *pkey);

#define _Py_HASHTABLE_GET_ENTRY(TABLE, KEY) \
    _Py_hashtable_get_entry(TABLE, sizeof(KEY), &(KEY))


/* Get data from an entry. Copy entry data into data and return 1 if the entry
   exists, return 0 if the entry does not exist.

   Don't call directly this function, but use _Py_HASHTABLE_GET() macro */
PyAPI_FUNC(int) _Py_hashtable_get(
    _Py_hashtable_t *ht,
    size_t key_size,
    const void *pkey,
    size_t data_size,
    void *data);

#define _Py_HASHTABLE_GET(TABLE, KEY, DATA) \
    _Py_hashtable_get(TABLE, sizeof(KEY), &(KEY), sizeof(DATA), &(DATA))


/* Don't call directly this function, but use _Py_HASHTABLE_POP() macro */
PyAPI_FUNC(int) _Py_hashtable_pop(
    _Py_hashtable_t *ht,
    size_t key_size,
    const void *pkey,
    size_t data_size,
    void *data);

#define _Py_HASHTABLE_POP(TABLE, KEY, DATA) \
    _Py_hashtable_pop(TABLE, sizeof(KEY), &(KEY), sizeof(DATA), &(DATA))


#endif   /* Py_LIMITED_API */
#endif
