#include "Python.h"

#include "pycore_index_pool.h"
#include "pycore_lock.h"

#include <stdbool.h>

#ifdef Py_GIL_DISABLED

static inline void
swap(int32_t *values, Py_ssize_t i, Py_ssize_t j)
{
    int32_t tmp = values[i];
    values[i] = values[j];
    values[j] = tmp;
}

static bool
heap_try_swap(_PyIndexHeap *heap, Py_ssize_t i, Py_ssize_t j)
{
    if (i < 0 || i >= heap->size) {
        return 0;
    }
    if (j < 0 || j >= heap->size) {
        return 0;
    }
    if (i <= j) {
        if (heap->values[i] <= heap->values[j]) {
            return 0;
        }
    }
    else if (heap->values[j] <= heap->values[i]) {
        return 0;
    }
    swap(heap->values, i, j);
    return 1;
}

static inline Py_ssize_t
parent(Py_ssize_t i)
{
    return (i - 1) / 2;
}

static inline Py_ssize_t
left_child(Py_ssize_t i)
{
    return 2 * i + 1;
}

static inline Py_ssize_t
right_child(Py_ssize_t i)
{
    return 2 * i + 2;
}

static void
heap_add(_PyIndexHeap *heap, int32_t val)
{
    assert(heap->size < heap->capacity);
    // Add val to end
    heap->values[heap->size] = val;
    heap->size++;
    // Sift up
    for (Py_ssize_t cur = heap->size - 1; cur > 0; cur = parent(cur)) {
        if (!heap_try_swap(heap, cur, parent(cur))) {
            break;
        }
    }
}

static Py_ssize_t
heap_min_child(_PyIndexHeap *heap, Py_ssize_t i)
{
    if (left_child(i) < heap->size) {
        if (right_child(i) < heap->size) {
            Py_ssize_t lval = heap->values[left_child(i)];
            Py_ssize_t rval = heap->values[right_child(i)];
            return lval < rval ? left_child(i) : right_child(i);
        }
        return left_child(i);
    }
    else if (right_child(i) < heap->size) {
        return right_child(i);
    }
    return -1;
}

static int32_t
heap_pop(_PyIndexHeap *heap)
{
    assert(heap->size > 0);
    // Pop smallest and replace with the last element
    int32_t result = heap->values[0];
    heap->values[0] = heap->values[heap->size - 1];
    heap->size--;
    // Sift down
    for (Py_ssize_t cur = 0; cur < heap->size;) {
        Py_ssize_t min_child = heap_min_child(heap, cur);
        if (min_child > -1 && heap_try_swap(heap, cur, min_child)) {
            cur = min_child;
        }
        else {
            break;
        }
    }
    return result;
}

static int
heap_ensure_capacity(_PyIndexHeap *heap, Py_ssize_t limit)
{
    assert(limit > 0);
    if (heap->capacity > limit) {
        return 0;
    }
    Py_ssize_t new_capacity = heap->capacity ? heap->capacity : 1024;
    while (new_capacity && new_capacity < limit) {
        new_capacity <<= 1;
    }
    if (!new_capacity) {
        return -1;
    }
    int32_t *new_values = PyMem_RawCalloc(new_capacity, sizeof(int32_t));
    if (new_values == NULL) {
        return -1;
    }
    if (heap->values != NULL) {
        memcpy(new_values, heap->values, heap->capacity);
        PyMem_RawFree(heap->values);
    }
    heap->values = new_values;
    heap->capacity = new_capacity;
    return 0;
}

static void
heap_fini(_PyIndexHeap *heap)
{
    if (heap->values != NULL) {
        PyMem_RawFree(heap->values);
        heap->values = NULL;
    }
    heap->size = -1;
    heap->capacity = -1;
}

#define LOCK_POOL(pool) PyMutex_LockFlags(&pool->mutex, _Py_LOCK_DONT_DETACH)
#define UNLOCK_POOL(pool) PyMutex_Unlock(&pool->mutex)

int32_t
_PyIndexPool_AllocIndex(_PyIndexPool *pool)
{
    LOCK_POOL(pool);
    int32_t index;
    _PyIndexHeap *free_indices = &pool->free_indices;
    if (free_indices->size == 0) {
        // No free indices. Make sure the heap can always store all of the
        // indices that have been allocated to avoid having to allocate memory
        // (which can fail) when freeing an index. Freeing indices happens when
        // threads are being destroyed, which makes error handling awkward /
        // impossible. This arrangement shifts handling of allocation failures
        // to when indices are allocated, which happens at thread creation,
        // where we are better equipped to deal with failure.
        if (heap_ensure_capacity(free_indices, pool->next_index + 1) < 0) {
            UNLOCK_POOL(pool);
            PyErr_NoMemory();
            return -1;
        }
        index = pool->next_index++;
    }
    else {
        index = heap_pop(free_indices);
    }
    UNLOCK_POOL(pool);
    return index;
}

void
_PyIndexPool_FreeIndex(_PyIndexPool *pool, int32_t index)
{
    LOCK_POOL(pool);
    heap_add(&pool->free_indices, index);
    UNLOCK_POOL(pool);
}

void
_PyIndexPool_Fini(_PyIndexPool *pool)
{
    heap_fini(&pool->free_indices);
}

#endif  // Py_GIL_DISABLED
