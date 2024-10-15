/*
 * Dynamic array implementation
 */

#include "pycore_dynarray.h"

_PyDynArray *
_PyDynArray_InitWithSize(_PyDynArray *array,
                         Py_ssize_t initial,
                         _PyDynArray_Deallocator deallocator)
{
    assert(array != NULL);
    assert(initial > 0);
    void **items = PyMem_RawCalloc(sizeof(void *), initial);
    if (items == NULL)
    {
        PyMem_RawFree(array);
        return NULL;
    }

    array->capacity = initial;
    array->items = items;
    array->length = 0;
    array->deallocator = deallocator;

    return array;
}

int
_PyDynArray_Append(_PyDynArray *array, void *item)
{
    assert(array != NULL);
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
    assert(array != NULL);
    assert(array->deallocator != NULL);
    for (Py_ssize_t i = 0; i < array->length; ++i)
    {
        array->deallocator(array->items[i]);
    }
    PyMem_RawFree(array->items);

    // It would be nice if others could reuse the allocation for another
    // dynarray later, so clear all the fields.
    array->items = NULL;
    array->length = 0;
    array->capacity = 0;
    array->deallocator = NULL;
}
