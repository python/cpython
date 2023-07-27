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

/* Forward declaration */
static int hashtable_rehash(_Py_hashtable_t *ht);

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
_Py_hashtable_size(const _Py_hashtable_t *ht)
{
    size_t size = sizeof(_Py_hashtable_t);
    /* buckets */
    size += ht->nbuckets * sizeof(_Py_hashtable_entry_t *);
    /* entries */
    size += ht->nentries * sizeof(_Py_hashtable_entry_t);
    return size;
}


size_t
_Py_hashtable_len(const _Py_hashtable_t *ht)
{
    return ht->nentries;
}


_Py_hashtable_entry_t *
_Py_hashtable_get_entry_generic(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash = ht->hash_func(key);
    size_t index = key_hash & (ht->nbuckets - 1);
    _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, index);
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


// Specialized for:
// hash_func == _Py_hashtable_hash_ptr
// compare_func == _Py_hashtable_compare_direct
static _Py_hashtable_entry_t *
_Py_hashtable_get_entry_ptr(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash = _Py_hashtable_hash_ptr(key);
    size_t index = key_hash & (ht->nbuckets - 1);
    _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, index);
    while (1) {
        if (entry == NULL) {
            return NULL;
        }
        // Compare directly keys (ignore entry->key_hash)
        if (entry->key == key) {
            break;
        }
        entry = ENTRY_NEXT(entry);
    }
    return entry;
}


void*
_Py_hashtable_steal(_Py_hashtable_t *ht, const void *key)
{
    Py_uhash_t key_hash = ht->hash_func(key);
    size_t index = key_hash & (ht->nbuckets - 1);

    _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, index);
    _Py_hashtable_entry_t *previous = NULL;
    while (1) {
        if (entry == NULL) {
            // not found
            return NULL;
        }
        if (entry->key_hash == key_hash && ht->compare_func(key, entry->key)) {
            break;
        }
        previous = entry;
        entry = ENTRY_NEXT(entry);
    }

    _Py_slist_remove(&ht->buckets[index], (_Py_slist_item_t *)previous,
                     (_Py_slist_item_t *)entry);
    ht->nentries--;

    void *value = entry->value;
    ht->alloc.free(entry);

    if ((float)ht->nentries / (float)ht->nbuckets < HASHTABLE_LOW) {
        // Ignore failure: error cannot be reported to the caller
        hashtable_rehash(ht);
    }
    return value;
}


int
_Py_hashtable_set(_Py_hashtable_t *ht, const void *key, void *value)
{
    _Py_hashtable_entry_t *entry;

#ifndef NDEBUG
    /* Don't write the assertion on a single line because it is interesting
       to know the duplicated entry if the assertion failed. The entry can
       be read using a debugger. */
    entry = ht->get_entry_func(ht, key);
    assert(entry == NULL);
#endif


    entry = ht->alloc.malloc(sizeof(_Py_hashtable_entry_t));
    if (entry == NULL) {
        /* memory allocation failed */
        return -1;
    }

    entry->key_hash = ht->hash_func(key);
    entry->key = (void *)key;
    entry->value = value;

    ht->nentries++;
    if ((float)ht->nentries / (float)ht->nbuckets > HASHTABLE_HIGH) {
        if (hashtable_rehash(ht) < 0) {
            ht->nentries--;
            ht->alloc.free(entry);
            return -1;
        }
    }

    size_t index = entry->key_hash & (ht->nbuckets - 1);
    _Py_slist_prepend(&ht->buckets[index], (_Py_slist_item_t*)entry);
    return 0;
}


void*
_Py_hashtable_get(_Py_hashtable_t *ht, const void *key)
{
    _Py_hashtable_entry_t *entry = ht->get_entry_func(ht, key);
    if (entry != NULL) {
        return entry->value;
    }
    else {
        return NULL;
    }
}


int
_Py_hashtable_foreach(_Py_hashtable_t *ht,
                      _Py_hashtable_foreach_func func,
                      void *user_data)
{
    for (size_t hv = 0; hv < ht->nbuckets; hv++) {
        _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, hv);
        while (entry != NULL) {
            int res = func(ht, entry->key, entry->value, user_data);
            if (res) {
                return res;
            }
            entry = ENTRY_NEXT(entry);
        }
    }
    return 0;
}


static int
hashtable_rehash(_Py_hashtable_t *ht)
{
    size_t new_size = round_size((size_t)(ht->nentries * HASHTABLE_REHASH_FACTOR));
    if (new_size == ht->nbuckets) {
        return 0;
    }

    size_t buckets_size = new_size * sizeof(ht->buckets[0]);
    _Py_slist_t *new_buckets = ht->alloc.malloc(buckets_size);
    if (new_buckets == NULL) {
        /* memory allocation failed */
        return -1;
    }
    memset(new_buckets, 0, buckets_size);

    for (size_t bucket = 0; bucket < ht->nbuckets; bucket++) {
        _Py_hashtable_entry_t *entry = BUCKETS_HEAD(ht->buckets[bucket]);
        while (entry != NULL) {
            assert(ht->hash_func(entry->key) == entry->key_hash);
            _Py_hashtable_entry_t *next = ENTRY_NEXT(entry);
            size_t entry_index = entry->key_hash & (new_size - 1);

            _Py_slist_prepend(&new_buckets[entry_index], (_Py_slist_item_t*)entry);

            entry = next;
        }
    }

    ht->alloc.free(ht->buckets);
    ht->nbuckets = new_size;
    ht->buckets = new_buckets;
    return 0;
}


_Py_hashtable_t *
_Py_hashtable_new_full(_Py_hashtable_hash_func hash_func,
                       _Py_hashtable_compare_func compare_func,
                       _Py_hashtable_destroy_func key_destroy_func,
                       _Py_hashtable_destroy_func value_destroy_func,
                       _Py_hashtable_allocator_t *allocator)
{
    _Py_hashtable_allocator_t alloc;
    if (allocator == NULL) {
        alloc.malloc = PyMem_Malloc;
        alloc.free = PyMem_Free;
    }
    else {
        alloc = *allocator;
    }

    _Py_hashtable_t *ht = (_Py_hashtable_t *)alloc.malloc(sizeof(_Py_hashtable_t));
    if (ht == NULL) {
        return ht;
    }

    ht->nbuckets = HASHTABLE_MIN_SIZE;
    ht->nentries = 0;

    size_t buckets_size = ht->nbuckets * sizeof(ht->buckets[0]);
    ht->buckets = alloc.malloc(buckets_size);
    if (ht->buckets == NULL) {
        alloc.free(ht);
        return NULL;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->get_entry_func = _Py_hashtable_get_entry_generic;
    ht->hash_func = hash_func;
    ht->compare_func = compare_func;
    ht->key_destroy_func = key_destroy_func;
    ht->value_destroy_func = value_destroy_func;
    ht->alloc = alloc;
    if (ht->hash_func == _Py_hashtable_hash_ptr
        && ht->compare_func == _Py_hashtable_compare_direct)
    {
        ht->get_entry_func = _Py_hashtable_get_entry_ptr;
    }
    return ht;
}


_Py_hashtable_t *
_Py_hashtable_new(_Py_hashtable_hash_func hash_func,
                  _Py_hashtable_compare_func compare_func)
{
    return _Py_hashtable_new_full(hash_func, compare_func,
                                  NULL, NULL, NULL);
}


static void
_Py_hashtable_destroy_entry(_Py_hashtable_t *ht, _Py_hashtable_entry_t *entry)
{
    if (ht->key_destroy_func) {
        ht->key_destroy_func(entry->key);
    }
    if (ht->value_destroy_func) {
        ht->value_destroy_func(entry->value);
    }
    ht->alloc.free(entry);
}


void
_Py_hashtable_clear(_Py_hashtable_t *ht)
{
    for (size_t i=0; i < ht->nbuckets; i++) {
        _Py_hashtable_entry_t *entry = TABLE_HEAD(ht, i);
        while (entry != NULL) {
            _Py_hashtable_entry_t *next = ENTRY_NEXT(entry);
            _Py_hashtable_destroy_entry(ht, entry);
            entry = next;
        }
        _Py_slist_init(&ht->buckets[i]);
    }
    ht->nentries = 0;
    // Ignore failure: clear function is not expected to fail
    // because of a memory allocation failure.
    (void)hashtable_rehash(ht);
}


void
_Py_hashtable_destroy(_Py_hashtable_t *ht)
{
    for (size_t i = 0; i < ht->nbuckets; i++) {
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
