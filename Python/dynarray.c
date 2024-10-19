/*
 * Dynamic array implementation.
 */

#include "pycore_dynarray.h"
#include "pycore_pymem.h" // _PyMem_RawStrdup

static inline void
call_deallocator_maybe(_PyDynArray *array, Py_ssize_t index)
{
    if (array->deallocator != NULL && array->items[index] != NULL)
    {
        array->deallocator(array->items[index]);
    }
}

int
_PyDynArray_InitWithSize(_PyDynArray *array,
                         _PyDynArray_Deallocator deallocator,
                         Py_ssize_t initial)
{
    assert(array != NULL);
    assert(initial > 0);
    void **items = PyMem_RawCalloc(sizeof(void *), initial);
    if (items == NULL)
    {
        return -1;
    }

    array->capacity = initial;
    array->items = items;
    array->length = 0;
    array->deallocator = deallocator;

    return 0;
}

static int
resize_if_needed(_PyDynArray *array)
{
    if (array->length == array->capacity)
    {
        // Need to resize
        array->capacity *= 2;
        void **new_items = PyMem_RawRealloc(
                                            array->items,
                                            sizeof(void *) * array->capacity);
        if (new_items == NULL)
        {
            return -1;
        }

        array->items = new_items;
    }

    return 0;
}

int
_PyDynArray_Append(_PyDynArray *array, void *item)
{
    _PyDynArray_ASSERT_VALID(array);
    array->items[array->length++] = item;
    if (resize_if_needed(array) < 0)
    {
        array->items[--array->length] = NULL;
        return -1;
    }
    return 0;
}

int
_PyDynArray_Insert(_PyDynArray *array, Py_ssize_t index, void *item)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_ASSERT_INDEX(array, index);
    ++array->length;
    if (resize_if_needed(array) < 0)
    {
        // Grow the array beforehand, otherwise it's
        // going to be a mess putting it back together if
        // allocation fails.
        --array->length;
        return -1;
    }

    for (Py_ssize_t i = array->length - 1; i > index; --i)
    {
        array->items[i] = array->items[i - 1];
    }

    array->items[index] = item;
    return 0;
}

void
_PyDynArray_Set(_PyDynArray *array, Py_ssize_t index, void *item)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_ASSERT_INDEX(array, index);
    call_deallocator_maybe(array, index);
    array->items[index] = item;
}

static void
remove_no_dealloc(_PyDynArray *array, Py_ssize_t index)
{
    for (Py_ssize_t i = index; i < array->length - 1; ++i)
    {
        array->items[i] = array->items[i + 1];
    }
    --array->length;
}

void
_PyDynArray_Remove(_PyDynArray *array, Py_ssize_t index)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_ASSERT_INDEX(array, index);
    call_deallocator_maybe(array, index);
    remove_no_dealloc(array, index);
}

void *
_PyDynArray_Pop(_PyDynArray *array, Py_ssize_t index)
{
    _PyDynArray_ASSERT_VALID(array);
    _PyDynArray_ASSERT_INDEX(array, index);
    void *item = array->items[index];
    remove_no_dealloc(array, index);
    return item;
}

void
_PyDynArray_Clear(_PyDynArray *array)
{
    _PyDynArray_ASSERT_VALID(array);
    for (Py_ssize_t i = 0; i < array->length; ++i)
    {
        call_deallocator_maybe(array, i);
    }
    PyMem_RawFree(array->items);

    // It would be nice if others could reuse the allocation for another
    // dynarray later, so clear all the fields.
    array->items = NULL;
    array->length = 0;
    array->capacity = 0;
    array->deallocator = NULL;
}
