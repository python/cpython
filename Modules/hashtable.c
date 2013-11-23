/* The implementation of the hash table (_Py_hashtable_t) is based on the cfuhash
   project:
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
        (sizeof(_Py_hashtable_entry_t) + (HT)->data_size)

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
_Py_hashtable_hash_int(const void *key)
{
    return (Py_uhash_t)key;
}

Py_uhash_t
_Py_hashtable_hash_ptr(const void *key)
{
    return (Py_uhash_t)_Py_HashPointer((void *)key);
}

int
_Py_hashtable_compare_direct(const void *key, const _Py_hashtable_entry_t *entry)
{
    return entry->key == key;
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
_Py_hashtable_new_full(size_t data_size, size_t init_size,
                       _Py_hashtable_hash_func hash_func,
                       _Py_hashtable_compare_func compare_func,
                       _Py_hashtable_copy_data_func copy_data_func,
                       _Py_hashtable_free_data_func free_data_func,
                       _Py_hashtable_get_data_size_func get_data_size_func,
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
    ht->copy_data_func = copy_data_func;
    ht->free_data_func = free_data_func;
    ht->get_data_size_func = get_data_size_func;
    ht->alloc = alloc;
    return ht;
}

_Py_hashtable_t *
_Py_hashtable_new(size_t data_size,
                  _Py_hashtable_hash_func hash_func,
                  _Py_hashtable_compare_func compare_func)
{
    return _Py_hashtable_new_full(data_size, HASHTABLE_MIN_SIZE,
                                  hash_func, compare_func,
                                  NULL, NULL, NULL, NULL);
}

size_t
_Py_hashtable_size(_Py_hashtable_t *ht)
{
    size_t size;
    size_t hv;

    size = sizeof(_Py_hashtable_t);

    /* buckets */
    size += ht->num_buckets * sizeof(_Py_hashtable_entry_t *);

    /* entries */
    size += ht->entries * HASHTABLE_ITEM_SIZE(ht);

    /* data linked from entries */
    if (ht->get_data_size_func) {
        for (hv = 0; hv < ht->num_buckets; hv++) {
            _Py_hashtable_entry_t *entry;

            for (entry = TABLE_HEAD(ht, hv); entry; entry = ENTRY_NEXT(entry)) {
                void *data;

                data = _Py_HASHTABLE_ENTRY_DATA_AS_VOID_P(entry);
                size += ht->get_data_size_func(data);
            }
        }
    }
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
    printf("hash table %p: entries=%zu/%zu (%.0f%%), ",
           ht, ht->entries, ht->num_buckets, load * 100.0);
    if (nchains)
        printf("avg_chain_len=%.1f, ", (double)total_chain_len / nchains);
    printf("max_chain_len=%zu, %zu kB\n",
           max_chain_len, size / 1024);
}
#endif

/* Get an entry. Return NULL if the key does not exist. */
_Py_hashtable_entry_t *
_Py_hashtable_get_entry(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    key_hash = ht->hash_func(key);
    index = key_hash & (ht->num_buckets - 1);

    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(key, entry))
            break;
    }

    return entry;
}

static int
_hashtable_pop_entry(_Py_hashtable_t *ht, const void *key, void *data, size_t data_size)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry, *previous;

    key_hash = ht->hash_func(key);
    index = key_hash & (ht->num_buckets - 1);

    previous = NULL;
    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(key, entry))
            break;
        previous = entry;
    }

    if (entry == NULL)
        return 0;

    _Py_slist_remove(&ht->buckets[index], (_Py_slist_item_t *)previous,
                     (_Py_slist_item_t *)entry);
    ht->entries--;

    if (data != NULL)
        _Py_HASHTABLE_ENTRY_READ_DATA(ht, data, data_size, entry);
    ht->alloc.free(entry);

    if ((float)ht->entries / (float)ht->num_buckets < HASHTABLE_LOW)
        hashtable_rehash(ht);
    return 1;
}

/* Add a new entry to the hash. The key must not be present in the hash table.
   Return 0 on success, -1 on memory error. */
int
_Py_hashtable_set(_Py_hashtable_t *ht, const void *key,
                  void *data, size_t data_size)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    assert(data != NULL || data_size == 0);
#ifndef NDEBUG
    /* Don't write the assertion on a single line because it is interesting
       to know the duplicated entry if the assertion failed. The entry can
       be read using a debugger. */
    entry = _Py_hashtable_get_entry(ht, key);
    assert(entry == NULL);
#endif

    key_hash = ht->hash_func(key);
    index = key_hash & (ht->num_buckets - 1);

    entry = ht->alloc.malloc(HASHTABLE_ITEM_SIZE(ht));
    if (entry == NULL) {
        /* memory allocation failed */
        return -1;
    }

    entry->key = (void *)key;
    entry->key_hash = key_hash;

    assert(data_size == ht->data_size);
    memcpy(_PY_HASHTABLE_ENTRY_DATA(entry), data, data_size);

    _Py_slist_prepend(&ht->buckets[index], (_Py_slist_item_t*)entry);
    ht->entries++;

    if ((float)ht->entries / (float)ht->num_buckets > HASHTABLE_HIGH)
        hashtable_rehash(ht);
    return 0;
}

/* Get data from an entry. Copy entry data into data and return 1 if the entry
   exists, return 0 if the entry does not exist. */
int
_Py_hashtable_get(_Py_hashtable_t *ht, const void *key, void *data, size_t data_size)
{
    _Py_hashtable_entry_t *entry;

    assert(data != NULL);

    entry = _Py_hashtable_get_entry(ht, key);
    if (entry == NULL)
        return 0;
    _Py_HASHTABLE_ENTRY_READ_DATA(ht, data, data_size, entry);
    return 1;
}

int
_Py_hashtable_pop(_Py_hashtable_t *ht, const void *key, void *data, size_t data_size)
{
    assert(data != NULL);
    assert(ht->free_data_func == NULL);
    return _hashtable_pop_entry(ht, key, data, data_size);
}

/* Delete an entry. The entry must exist. */
void
_Py_hashtable_delete(_Py_hashtable_t *ht, const void *key)
{
#ifndef NDEBUG
    int found = _hashtable_pop_entry(ht, key, NULL, 0);
    assert(found);
#else
    (void)_hashtable_pop_entry(ht, key, NULL, 0);
#endif
}

/* Prototype for a pointer to a function to be called foreach
   key/value pair in the hash by hashtable_foreach().  Iteration
   stops if a non-zero value is returned. */
int
_Py_hashtable_foreach(_Py_hashtable_t *ht,
                      int (*func) (_Py_hashtable_entry_t *entry, void *arg),
                      void *arg)
{
    _Py_hashtable_entry_t *entry;
    size_t hv;

    for (hv = 0; hv < ht->num_buckets; hv++) {
        for (entry = TABLE_HEAD(ht, hv); entry; entry = ENTRY_NEXT(entry)) {
            int res = func(entry, arg);
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

void
_Py_hashtable_clear(_Py_hashtable_t *ht)
{
    _Py_hashtable_entry_t *entry, *next;
    size_t i;

    for (i=0; i < ht->num_buckets; i++) {
        for (entry = TABLE_HEAD(ht, i); entry != NULL; entry = next) {
            next = ENTRY_NEXT(entry);
            if (ht->free_data_func)
                ht->free_data_func(_Py_HASHTABLE_ENTRY_DATA_AS_VOID_P(entry));
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
            if (ht->free_data_func)
                ht->free_data_func(_Py_HASHTABLE_ENTRY_DATA_AS_VOID_P(entry));
            ht->alloc.free(entry);
            entry = entry_next;
        }
    }

    ht->alloc.free(ht->buckets);
    ht->alloc.free(ht);
}

/* Return a copy of the hash table */
_Py_hashtable_t *
_Py_hashtable_copy(_Py_hashtable_t *src)
{
    _Py_hashtable_t *dst;
    _Py_hashtable_entry_t *entry;
    size_t bucket;
    int err;
    void *data, *new_data;

    dst = _Py_hashtable_new_full(src->data_size, src->num_buckets,
                            src->hash_func, src->compare_func,
                            src->copy_data_func, src->free_data_func,
                            src->get_data_size_func, &src->alloc);
    if (dst == NULL)
        return NULL;

    for (bucket=0; bucket < src->num_buckets; bucket++) {
        entry = TABLE_HEAD(src, bucket);
        for (; entry; entry = ENTRY_NEXT(entry)) {
            if (src->copy_data_func) {
                data = _Py_HASHTABLE_ENTRY_DATA_AS_VOID_P(entry);
                new_data = src->copy_data_func(data);
                if (new_data != NULL)
                    err = _Py_hashtable_set(dst, entry->key,
                                        &new_data, src->data_size);
                else
                    err = 1;
            }
            else {
                data = _PY_HASHTABLE_ENTRY_DATA(entry);
                err = _Py_hashtable_set(dst, entry->key, data, src->data_size);
            }
            if (err) {
                _Py_hashtable_destroy(dst);
                return NULL;
            }
        }
    }
    return dst;
}

