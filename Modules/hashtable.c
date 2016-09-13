/* The implementation of the hash table (_Py_hashtable_t) is based on the
   cfuhash project:
   http://sourceforge.net/projects/libcfu/

   Copyright of cfuhash:
   ----------------------------------
   Creation date: 2005-06-24 21:22:40
   Authors: Don
   Change log:

   Copyright (c) 2005 Don Owens
   All rights reserved.

   This code is released under the BSD license:

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     * Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

     * Redistributions in binary form must reproduce the above
       copyright notice, this list of conditions and the following
       disclaimer in the documentation and/or other materials provided
       with the distribution.

     * Neither the name of the author nor the names of its
       contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
   OF THE POSSIBILITY OF SUCH DAMAGE.
   ----------------------------------
*/

#include "Python.h"
#include "hashtable.h"

#define HASHTABLE_MIN_SIZE 16
#define HASHTABLE_HIGH 0.50
#define HASHTABLE_LOW 0.10
#define HASHTABLE_REHASH_FACTOR 2.0 / (HASHTABLE_LOW + HASHTABLE_HIGH)

#define BUCKETS_HEAD(SLIST) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_HEAD(&(SLIST)))
#define TABLE_HEAD(HT, BUCKET) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_HEAD(&(HT)->buckets[BUCKET]))
#define ENTRY_NEXT(ENTRY) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_ITEM_NEXT(ENTRY))
#define HASHTABLE_ITEM_SIZE(HT) \
        (sizeof(_Py_hashtable_entry_t) + (HT)->key_size + (HT)->data_size)

#define ENTRY_READ_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((PDATA), _Py_HASHTABLE_ENTRY_PDATA(TABLE, (ENTRY)), \
                  (DATA_SIZE)); \
    } while (0)

#define ENTRY_WRITE_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((void *)_Py_HASHTABLE_ENTRY_PDATA((TABLE), (ENTRY)), \
                  (PDATA), (DATA_SIZE)); \
    } while (0)

/* Forward declaration */
static void hashtable_rehash(_Py_hashtable_t *ht);

static void
_Py_slist_init(_Py_slist_t *list)
{
    list->head = NULL;
}


static void
_Py_slist_prepend(_Py_slist_t *list, _Py_slist_item_t *item)
{
    item->next = list->head;
    list->head = item;
}


static void
_Py_slist_remove(_Py_slist_t *list, _Py_slist_item_t *previous,
                 _Py_slist_item_t *item)
{
    if (previous != NULL)
        previous->next = item->next;
    else
        list->head = item->next;
}


Py_uhash_t
_Py_hashtable_hash_ptr(struct _Py_hashtable_t *ht, const void *pkey)
{
    void *key;

    _Py_HASHTABLE_READ_KEY(ht, pkey, key);
    return (Py_uhash_t)_Py_HashPointer(key);
}


int
_Py_hashtable_compare_direct(_Py_hashtable_t *ht, const void *pkey,
                             const _Py_hashtable_entry_t *entry)
{
    const void *pkey2 = _Py_HASHTABLE_ENTRY_PKEY(entry);
    return (memcmp(pkey, pkey2, ht->key_size) == 0);
}


/* makes sure the real size of the buckets array is a power of 2 */
static size_t
round_size(size_t s)
{
    size_t i;
    if (s < HASHTABLE_MIN_SIZE)
        return HASHTABLE_MIN_SIZE;
    i = 1;
    while (i < s)
        i <<= 1;
    return i;
}


_Py_hashtable_t *
_Py_hashtable_new_full(size_t key_size, size_t data_size,
                       size_t init_size,
                       _Py_hashtable_hash_func hash_func,
                       _Py_hashtable_compare_func compare_func,
                       _Py_hashtable_allocator_t *allocator)
{
    _Py_hashtable_t *ht;
    size_t buckets_size;
    _Py_hashtable_allocator_t alloc;

    if (allocator == NULL) {
        alloc.malloc = PyMem_RawMalloc;
        alloc.free = PyMem_RawFree;
    }
    else
        alloc = *allocator;

    ht = (_Py_hashtable_t *)alloc.malloc(sizeof(_Py_hashtable_t));
    if (ht == NULL)
        return ht;

    ht->num_buckets = round_size(init_size);
    ht->entries = 0;
    ht->key_size = key_size;
    ht->data_size = data_size;

    buckets_size = ht->num_buckets * sizeof(ht->buckets[0]);
    ht->buckets = alloc.malloc(buckets_size);
    if (ht->buckets == NULL) {
        alloc.free(ht);
        return NULL;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->hash_func = hash_func;
    ht->compare_func = compare_func;
    ht->alloc = alloc;
    return ht;
}


_Py_hashtable_t *
_Py_hashtable_new(size_t key_size, size_t data_size,
                  _Py_hashtable_hash_func hash_func,
                  _Py_hashtable_compare_func compare_func)
{
    return _Py_hashtable_new_full(key_size, data_size,
                                  HASHTABLE_MIN_SIZE,
                                  hash_func, compare_func,
                                  NULL);
}


size_t
_Py_hashtable_size(_Py_hashtable_t *ht)
{
    size_t size;

    size = sizeof(_Py_hashtable_t);

    /* buckets */
    size += ht->num_buckets * sizeof(_Py_hashtable_entry_t *);

    /* entries */
    size += ht->entries * HASHTABLE_ITEM_SIZE(ht);

    return size;
}


#ifdef Py_DEBUG
void
_Py_hashtable_print_stats(_Py_hashtable_t *ht)
{
    size_t size;
    size_t chain_len, max_chain_len, total_chain_len, nchains;
    _Py_hashtable_entry_t *entry;
    size_t hv;
    double load;

    size = _Py_hashtable_size(ht);

    load = (double)ht->entries / ht->num_buckets;

    max_chain_len = 0;
    total_chain_len = 0;
    nchains = 0;
    for (hv = 0; hv < ht->num_buckets; hv++) {
        entry = TABLE_HEAD(ht, hv);
        if (entry != NULL) {
            chain_len = 0;
            for (; entry; entry = ENTRY_NEXT(entry)) {
                chain_len++;
            }
            if (chain_len > max_chain_len)
                max_chain_len = chain_len;
            total_chain_len += chain_len;
            nchains++;
        }
    }
    printf("hash table %p: entries=%"
           PY_FORMAT_SIZE_T "u/%" PY_FORMAT_SIZE_T "u (%.0f%%), ",
           ht, ht->entries, ht->num_buckets, load * 100.0);
    if (nchains)
        printf("avg_chain_len=%.1f, ", (double)total_chain_len / nchains);
    printf("max_chain_len=%" PY_FORMAT_SIZE_T "u, %" PY_FORMAT_SIZE_T "u kB\n",
           max_chain_len, size / 1024);
}
#endif


_Py_hashtable_entry_t *
_Py_hashtable_get_entry(_Py_hashtable_t *ht,
                        size_t key_size, const void *pkey)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    assert(key_size == ht->key_size);

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(ht, pkey, entry))
            break;
    }

    return entry;
}


static int
_Py_hashtable_pop_entry(_Py_hashtable_t *ht, size_t key_size, const void *pkey,
                        void *data, size_t data_size)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry, *previous;

    assert(key_size == ht->key_size);

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    previous = NULL;
    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(ht, pkey, entry))
            break;
        previous = entry;
    }

    if (entry == NULL)
        return 0;

    _Py_slist_remove(&ht->buckets[index], (_Py_slist_item_t *)previous,
                     (_Py_slist_item_t *)entry);
    ht->entries--;

    if (data != NULL)
        ENTRY_READ_PDATA(ht, entry, data_size, data);
    ht->alloc.free(entry);

    if ((float)ht->entries / (float)ht->num_buckets < HASHTABLE_LOW)
        hashtable_rehash(ht);
    return 1;
}


int
_Py_hashtable_set(_Py_hashtable_t *ht, size_t key_size, const void *pkey,
                  size_t data_size, const void *data)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    assert(key_size == ht->key_size);

    assert(data != NULL || data_size == 0);
#ifndef NDEBUG
    /* Don't write the assertion on a single line because it is interesting
       to know the duplicated entry if the assertion failed. The entry can
       be read using a debugger. */
    entry = _Py_hashtable_get_entry(ht, key_size, pkey);
    assert(entry == NULL);
#endif

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    entry = ht->alloc.malloc(HASHTABLE_ITEM_SIZE(ht));
    if (entry == NULL) {
        /* memory allocation failed */
        return -1;
    }

    entry->key_hash = key_hash;
    memcpy((void *)_Py_HASHTABLE_ENTRY_PKEY(entry), pkey, ht->key_size);
    if (data)
        ENTRY_WRITE_PDATA(ht, entry, data_size, data);

    _Py_slist_prepend(&ht->buckets[index], (_Py_slist_item_t*)entry);
    ht->entries++;

    if ((float)ht->entries / (float)ht->num_buckets > HASHTABLE_HIGH)
        hashtable_rehash(ht);
    return 0;
}


int
_Py_hashtable_get(_Py_hashtable_t *ht, size_t key_size,const void *pkey,
                  size_t data_size, void *data)
{
    _Py_hashtable_entry_t *entry;

    assert(data != NULL);

    entry = _Py_hashtable_get_entry(ht, key_size, pkey);
    if (entry == NULL)
        return 0;
    ENTRY_READ_PDATA(ht, entry, data_size, data);
    return 1;
}


int
_Py_hashtable_pop(_Py_hashtable_t *ht, size_t key_size, const void *pkey,
                  size_t data_size, void *data)
{
    assert(data != NULL);
    return _Py_hashtable_pop_entry(ht, key_size, pkey, data, data_size);
}


/* Code commented since the function is not needed in Python */
#if 0
void
_Py_hashtable_delete(_Py_hashtable_t *ht, size_t key_size, const void *pkey)
{
#ifndef NDEBUG
    int found = _Py_hashtable_pop_entry(ht, key_size, pkey, NULL, 0);
    assert(found);
#else
    (void)_Py_hashtable_pop_entry(ht, key_size, pkey, NULL, 0);
#endif
}
#endif


int
_Py_hashtable_foreach(_Py_hashtable_t *ht,
                      _Py_hashtable_foreach_func func,
                      void *arg)
{
    _Py_hashtable_entry_t *entry;
    size_t hv;

    for (hv = 0; hv < ht->num_buckets; hv++) {
        for (entry = TABLE_HEAD(ht, hv); entry; entry = ENTRY_NEXT(entry)) {
            int res = func(ht, entry, arg);
            if (res)
                return res;
        }
    }
    return 0;
}


static void
hashtable_rehash(_Py_hashtable_t *ht)
{
    size_t buckets_size, new_size, bucket;
    _Py_slist_t *old_buckets = NULL;
    size_t old_num_buckets;

    new_size = round_size((size_t)(ht->entries * HASHTABLE_REHASH_FACTOR));
    if (new_size == ht->num_buckets)
        return;

    old_num_buckets = ht->num_buckets;

    buckets_size = new_size * sizeof(ht->buckets[0]);
    old_buckets = ht->buckets;
    ht->buckets = ht->alloc.malloc(buckets_size);
    if (ht->buckets == NULL) {
        /* cancel rehash on memory allocation failure */
        ht->buckets = old_buckets ;
        /* memory allocation failed */
        return;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->num_buckets = new_size;

    for (bucket = 0; bucket < old_num_buckets; bucket++) {
        _Py_hashtable_entry_t *entry, *next;
        for (entry = BUCKETS_HEAD(old_buckets[bucket]); entry != NULL; entry = next) {
            size_t entry_index;


            assert(ht->hash_func(ht, _Py_HASHTABLE_ENTRY_PKEY(entry)) == entry->key_hash);
            next = ENTRY_NEXT(entry);
            entry_index = entry->key_hash & (new_size - 1);

            _Py_slist_prepend(&ht->buckets[entry_index], (_Py_slist_item_t*)entry);
        }
    }

    ht->alloc.free(old_buckets);
}


void
_Py_hashtable_clear(_Py_hashtable_t *ht)
{
    _Py_hashtable_entry_t *entry, *next;
    size_t i;

    for (i=0; i < ht->num_buckets; i++) {
        for (entry = TABLE_HEAD(ht, i); entry != NULL; entry = next) {
            next = ENTRY_NEXT(entry);
            ht->alloc.free(entry);
        }
        _Py_slist_init(&ht->buckets[i]);
    }
    ht->entries = 0;
    hashtable_rehash(ht);
}


void
_Py_hashtable_destroy(_Py_hashtable_t *ht)
{
    size_t i;

    for (i = 0; i < ht->num_buckets; i++) {
        _Py_slist_item_t *entry = ht->buckets[i].head;
        while (entry) {
            _Py_slist_item_t *entry_next = entry->next;
            ht->alloc.free(entry);
            entry = entry_next;
        }
    }

    ht->alloc.free(ht->buckets);
    ht->alloc.free(ht);
}


_Py_hashtable_t *
_Py_hashtable_copy(_Py_hashtable_t *src)
{
    const size_t key_size = src->key_size;
    const size_t data_size = src->data_size;
    _Py_hashtable_t *dst;
    _Py_hashtable_entry_t *entry;
    size_t bucket;
    int err;

    dst = _Py_hashtable_new_full(key_size, data_size,
                                 src->num_buckets,
                                 src->hash_func,
                                 src->compare_func,
                                 &src->alloc);
    if (dst == NULL)
        return NULL;

    for (bucket=0; bucket < src->num_buckets; bucket++) {
        entry = TABLE_HEAD(src, bucket);
        for (; entry; entry = ENTRY_NEXT(entry)) {
            const void *pkey = _Py_HASHTABLE_ENTRY_PKEY(entry);
            const void *pdata = _Py_HASHTABLE_ENTRY_PDATA(src, entry);
            err = _Py_hashtable_set(dst, key_size, pkey, data_size, pdata);
            if (err) {
                _Py_hashtable_destroy(dst);
                return NULL;
            }
        }
    }
    return dst;
}
