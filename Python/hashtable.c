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
#include "pycore_hashtable.h"

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
        (sizeof(_Py_hashtable_entry_t) + (HT)->data_size)

#define ENTRY_READ_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((PDATA), _Py_HASHTABLE_ENTRY_PDATA(ENTRY), \
                  (DATA_SIZE)); \
    } while (0)

#define ENTRY_WRITE_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((void *)_Py_HASHTABLE_ENTRY_PDATA(ENTRY), \
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
_Py_hashtable_hash_ptr(const void *key)
{
    return (Py_uhash_t)_Py_HashPointerRaw(key);
}


int
_Py_hashtable_compare_direct(const void *key1, const void *key2)
{
    return (key1 == key2);
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
           (void *)ht, ht->entries, ht->num_buckets, load * 100.0);
    if (nchains)
        printf("avg_chain_len=%.1f, ", (double)total_chain_len / nchains);
    printf("max_chain_len=%" PY_FORMAT_SIZE_T "u, %" PY_FORMAT_SIZE_T "u KiB\n",
           max_chain_len, size / 1024);
}
#endif


_Py_hashtable_entry_t *
_Py_hashtable_get_entry_generic(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash = ht->hash_func(key);
    size_t index = key_hash & (ht->num_buckets - 1);
    _Py_hashtable_entry_t *entry = entry = TABLE_HEAD(ht, index);
    while (1) {
        if (entry == NULL) {
            return NULL;
        }
        if (entry->key_hash == key_hash && ht->compare_func(key, entry->key)) {
            break;
        }
        entry = ENTRY_NEXT(entry);
    }
    return entry;
}


static int
_Py_hashtable_pop_entry(_Py_hashtable_t *ht, const void *key,
                        void *data, size_t data_size)
{

    Py_uhash_t key_hash = ht->hash_func(key);
    size_t index = key_hash & (ht->num_buckets - 1);

    _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, index);
    _Py_hashtable_entry_t *previous = NULL;
    while (1) {
        if (entry == NULL) {
            // not found
            return 0;
        }
        if (entry->key_hash == key_hash && ht->compare_func(key, entry->key)) {
            break;
        }
        previous = entry;
        entry = ENTRY_NEXT(entry);
    }

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
_Py_hashtable_set(_Py_hashtable_t *ht, const void *key,
                  size_t data_size, const void *data)
{
    _Py_hashtable_entry_t *entry;

    assert(data != NULL || data_size == 0);
#ifndef NDEBUG
    /* Don't write the assertion on a single line because it is interesting
       to know the duplicated entry if the assertion failed. The entry can
       be read using a debugger. */
    entry = ht->get_entry_func(ht, key);
    assert(entry == NULL);
#endif

    Py_uhash_t key_hash = ht->hash_func(key);
    size_t index = key_hash & (ht->num_buckets - 1);

    entry = ht->alloc.malloc(HASHTABLE_ITEM_SIZE(ht));
    if (entry == NULL) {
        /* memory allocation failed */
        return -1;
    }

    entry->key_hash = key_hash;
    entry->key = (void *)key;
    if (data) {
        ENTRY_WRITE_PDATA(ht, entry, data_size, data);
    }

    _Py_slist_prepend(&ht->buckets[index], (_Py_slist_item_t*)entry);
    ht->entries++;

    if ((float)ht->entries / (float)ht->num_buckets > HASHTABLE_HIGH)
        hashtable_rehash(ht);
    return 0;
}


int
_Py_hashtable_get_generic(_Py_hashtable_t *ht, const void *key, void *data)
{
    assert(data != NULL);
    _Py_hashtable_entry_t *entry = ht->get_entry_func(ht, key);
    if (entry != NULL) {
        ENTRY_READ_PDATA(ht, entry, ht->data_size, data);
        return 1;
    }
    else {
        return 0;
    }
}


// Specialized for:
// hash_func == _Py_hashtable_hash_ptr
// compare_func == _Py_hashtable_compare_direct
_Py_hashtable_entry_t *
_Py_hashtable_get_entry_ptr(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash = _Py_hashtable_hash_ptr(key);
    size_t index = key_hash & (ht->num_buckets - 1);
    _Py_hashtable_entry_t *entry = entry = TABLE_HEAD(ht, index);
    while (1) {
        if (entry == NULL) {
            return NULL;
        }
        if (entry->key_hash == key_hash) {
            if (entry->key == key) {
                break;
            }
        }
        entry = ENTRY_NEXT(entry);
    }
    return entry;
}


// Specialized for:
// hash_func == _Py_hashtable_hash_ptr
// compare_func == _Py_hashtable_compare_direct
int
_Py_hashtable_get_ptr(_Py_hashtable_t *ht, const void *key, void *data)
{
    assert(data != NULL);
    _Py_hashtable_entry_t *entry = _Py_hashtable_get_entry_ptr(ht, key);
    if (entry != NULL) {
        ENTRY_READ_PDATA(ht, entry, ht->data_size, data);
        return 1;
    }
    else {
        return 0;
    }
}


int
_Py_hashtable_pop(_Py_hashtable_t *ht, const void *key,
                  size_t data_size, void *data)
{
    assert(data != NULL);
    return _Py_hashtable_pop_entry(ht, key, data, data_size);
}


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


            assert(ht->hash_func(entry->key) == entry->key_hash);
            next = ENTRY_NEXT(entry);
            entry_index = entry->key_hash & (new_size - 1);

            _Py_slist_prepend(&ht->buckets[entry_index], (_Py_slist_item_t*)entry);
        }
    }

    ht->alloc.free(old_buckets);
}


_Py_hashtable_t *
_Py_hashtable_new_full(size_t data_size, size_t init_size,
                       _Py_hashtable_hash_func hash_func,
                       _Py_hashtable_compare_func compare_func,
                       _Py_hashtable_destroy_func key_destroy_func,
                       _Py_hashtable_value_destroy_func value_destroy_func,
                       _Py_hashtable_allocator_t *allocator)
{
    _Py_hashtable_t *ht;
    size_t buckets_size;
    _Py_hashtable_allocator_t alloc;

    if (allocator == NULL) {
        alloc.malloc = PyMem_Malloc;
        alloc.free = PyMem_Free;
    }
    else {
        alloc = *allocator;
    }

    ht = (_Py_hashtable_t *)alloc.malloc(sizeof(_Py_hashtable_t));
    if (ht == NULL)
        return ht;

    ht->num_buckets = round_size(init_size);
    ht->entries = 0;
    ht->data_size = data_size;

    buckets_size = ht->num_buckets * sizeof(ht->buckets[0]);
    ht->buckets = alloc.malloc(buckets_size);
    if (ht->buckets == NULL) {
        alloc.free(ht);
        return NULL;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->get_func = _Py_hashtable_get_generic;
    ht->get_entry_func = _Py_hashtable_get_entry_generic;
    ht->hash_func = hash_func;
    ht->compare_func = compare_func;
    ht->key_destroy_func = key_destroy_func;
    ht->value_destroy_func = value_destroy_func;
    ht->alloc = alloc;
    if (ht->hash_func == _Py_hashtable_hash_ptr
        && ht->compare_func == _Py_hashtable_compare_direct)
    {
        ht->get_func = _Py_hashtable_get_ptr;
        ht->get_entry_func = _Py_hashtable_get_entry_ptr;
    }
    return ht;
}


_Py_hashtable_t *
_Py_hashtable_new(size_t data_size,
                  _Py_hashtable_hash_func hash_func,
                  _Py_hashtable_compare_func compare_func)
{
    return _Py_hashtable_new_full(data_size, HASHTABLE_MIN_SIZE,
                                  hash_func, compare_func,
                                  NULL, NULL, NULL);
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


static void
_Py_hashtable_destroy_entry(_Py_hashtable_t *ht, _Py_hashtable_entry_t *entry)
{
    if (ht->key_destroy_func) {
        ht->key_destroy_func(entry->key);
    }
    if (ht->value_destroy_func) {
        ht->value_destroy_func(ht, entry);
    }
    ht->alloc.free(entry);
}


void
_Py_hashtable_destroy(_Py_hashtable_t *ht)
{
    for (size_t i = 0; i < ht->num_buckets; i++) {
        _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, i);
        while (entry) {
            _Py_hashtable_entry_t *entry_next = ENTRY_NEXT(entry);
            _Py_hashtable_destroy_entry(ht, entry);
            entry = entry_next;
        }
    }

    ht->alloc.free(ht->buckets);
    ht->alloc.free(ht);
}
