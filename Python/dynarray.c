/*
 * Dynamic array implementation
 */

#include "pycore_dynarray.h"

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

int
_PyDynArray_Append(_PyDynArray *array, void *item)
{
    _PyDynArray_ASSERT_VALID(array);
    array->items[array->length++] = item;
    if (array->length == array->capacity)
    {
        // Need to resize
        array->capacity *= 2;
        void **new_items = PyMem_RawRealloc(
                                            array->items,
                                            sizeof(void *) * array->capacity);
        if (new_items == NULL)
        {
            --array->length;
            return -1;
        }

        array->items = new_items;
    }
    return 0;
}

void
_PyDynArray_Clear(_PyDynArray *array)
{
    _PyDynArray_ASSERT_VALID(array);
    for (Py_ssize_t i = 0; i < array->length; ++i)
    {
        if (array->deallocator != NULL)
        {
            array->deallocator(array->items[i]);
        }
    }
    PyMem_RawFree(array->items);

    // It would be nice if others could reuse the allocation for another
    // dynarray later, so clear all the fields.
    array->items = NULL;
    array->length = 0;
    array->capacity = 0;
    array->deallocator = NULL;
}
