#ifndef Py_INTERNAL_DYNARRAY_H
#define Py_INTERNAL_DYNARRAY_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h" // Py_ssize_t

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define _PyDynArray_DEFAULT_SIZE 16

/*
 * Deallocator for items on a _PyDynArray structure. A NULL pointer
 * will never be given to the deallocator.
 */
typedef void (*_PyDynArray_Deallocator)(void *);

/*
 * Internal only dynamic array for CPython.
 */
typedef struct {
    /*
     * The actual items in the dynamic array.
     * Don't access this field publicly to get
     * items--use _PyDynArray_GET_ITEM() instead.
     */
    void **items;
    /*
     * The length of the actual items array allocation.
     */
    Py_ssize_t capacity;
    /*
     * The number of items in the array.
     * Don't use this field publicly--use _PyDynArray_LENGTH()
     */
    Py_ssize_t length;
    /*
     * The deallocator, set by one of the initializer functions.
     * This may be NULL.
     */
    _PyDynArray_Deallocator deallocator;
} _PyDynArray;


static inline void
_PyDynArray_ASSERT_VALID(_PyDynArray *array)
{
    assert(array != NULL);
    assert(array->items != NULL);
}

static inline void
_PyDynArray_ASSERT_INDEX(_PyDynArray *array, Py_ssize_t index)
{
    // Ensure the index is valid
    assert(index >= 0);
    assert(index < array->length);
}

/*
 * Initialize a dynamic array with an initial size and deallocator.
 *
 * If the deallocator is NULL, then nothing happens to items upon
 * removal and upon array clearing.
 *
 * Returns -1 upon failure, 0 otherwise.
 */
PyAPI_FUNC(int)
_PyDynArray_InitWithSize(_PyDynArray *array,
                         _PyDynArray_Deallocator deallocator,
                         Py_ssize_t initial);

/*
 * Append to a dynamic array.
 *
 * Returns -1 upon failure, 0 otherwise.
 * If this fails, the deallocator is not ran on *item*.
 */
PyAPI_FUNC(int) _PyDynArray_Append(_PyDynArray *array, void *item);

/*
 * Clear all the fields on a dynamic array.
 *
 * Note that this does *not* free the actual dynamic array
 * structure--use _PyDynArray_Free() for that.
 *
 * It's safe to call _PyDynArray_Init() or InitWithSize() again
 * on the array after calling this.
 */
PyAPI_FUNC(void) _PyDynArray_Clear(_PyDynArray *array);

/*
 * Set a value at index in the array.
 *
 * If an item already exists at the target index, the deallocator
 * is called on it, if the array has one set.
 *
 * This cannot fail.
 */
PyAPI_FUNC(void)
_PyDynArray_Set(_PyDynArray *array, Py_ssize_t index, void *item);

/*
 * Remove the item at the index, and call the deallocator on it (if the array
 * has one set).
 *
 * This cannot fail.
 */
PyAPI_FUNC(void)
_PyDynArray_Remove(_PyDynArray *array, Py_ssize_t index);

/*
 * Remove the item at the index *without* deallocating it, and
 * return the item.
 *
 * This cannot fail.
 */
PyAPI_FUNC(void *)
_PyDynArray_Pop(_PyDynArray *array, Py_ssize_t index);

/*
 * Clear all the fields on a dynamic array, and then
 * free the dynamic array structure itself.
 *
 * The array must have been created by _PyDynArray_New()
 */
static inline void
_PyDynArray_Free(_PyDynArray *array)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_Clear(array);
    PyMem_RawFree(array);
}

/*
 * Equivalent to _PyDynArray_InitWithSize() with a default size of 16.
 *
 * Returns -1 upon failure, 0 otherwise.
 */
static inline int
_PyDynArray_Init(_PyDynArray *array, _PyDynArray_Deallocator deallocator)
{
    return _PyDynArray_InitWithSize(array, deallocator, _PyDynArray_DEFAULT_SIZE);
}

/*
 * Allocate and create a new dynamic array on the heap.
 *
 * The returned pointer should be freed with _PyDynArray_Free()
 * If this function fails, it returns NULL.
 */
static inline _PyDynArray *
_PyDynArray_NewWithSize(_PyDynArray_Deallocator deallocator, Py_ssize_t initial)
{
    _PyDynArray *array = PyMem_RawMalloc(sizeof(_PyDynArray));
    if (array == NULL)
    {
        return NULL;
    }

    if (_PyDynArray_InitWithSize(array, deallocator, initial) < 0)
    {
        PyMem_RawFree(array);
        return NULL;
    }

    _PyDynArray_ASSERT_VALID(array); // Sanity check
    return array;
}

/*
 * Equivalent to _PyDynArray_NewWithSize() with a size of 16.
 *
 * The returned array must be freed with _PyDynArray_Free().
 * Returns NULL on failure.
 */
static inline _PyDynArray *
_PyDynArray_New(_PyDynArray_Deallocator deallocator)
{
    return _PyDynArray_NewWithSize(deallocator, _PyDynArray_DEFAULT_SIZE);
}

/*
 * Get an item from the array. This cannot fail.
 *
 * If the index is not valid, this is undefined behavior.
 */
static inline void *
_PyDynArray_GET_ITEM(_PyDynArray *array, Py_ssize_t index)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_ASSERT_INDEX(array, index);
    return array->items[index];
}

/*
 * Get the length of the array. This cannot fail.
 */
static inline Py_ssize_t
_PyDynArray_LENGTH(_PyDynArray *array)
{
    _PyDynArray_ASSERT_VALID(array);
    return array->length;
}

/*
 * Pop the item at the end the array.
 * This function cannot fail.
 */
static inline void *
_PyDynArray_PopTop(_PyDynArray *array, Py_ssize_t index)
{
    return _PyDynArray_PopTop(array, _PyDynArray_LENGTH(array) - 1);
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_DYNARRAY_H */
