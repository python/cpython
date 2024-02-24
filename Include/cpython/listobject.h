#ifndef Py_CPYTHON_LISTOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* Vector of pointers to list elements.  list[0] is ob_item[0], etc. */
    PyObject **ob_item;

    /* ob_item contains space for 'allocated' elements.  The number
     * currently in use is ob_size.
     * Invariants:
     *     0 <= ob_size <= allocated
     *     len(list) == ob_size
     *     ob_item == NULL implies ob_size == allocated == 0
     * list.sort() temporarily sets allocated to -1 to detect mutations.
     *
     * Items must normally not be NULL, except during construction when
     * the list is not yet visible outside the function that builds it.
     */
    Py_ssize_t allocated;
} PyListObject;

/* Cast argument to PyListObject* type. */
#define _PyList_CAST(op) \
    (assert(PyList_Check(op)), _Py_CAST(PyListObject*, (op)))

// Macros and static inline functions, trading safety for speed

static inline Py_ssize_t PyList_GET_SIZE(PyObject *op) {
    PyListObject *list = _PyList_CAST(op);
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_ssize_relaxed(&(_PyVarObject_CAST(list)->ob_size));
#else
    return Py_SIZE(list);
#endif
}
#define PyList_GET_SIZE(op) PyList_GET_SIZE(_PyObject_CAST(op))

#define PyList_GET_ITEM(op, index) (_PyList_CAST(op)->ob_item[(index)])

static inline void
PyList_SET_ITEM(PyObject *op, Py_ssize_t index, PyObject *value) {
    PyListObject *list = _PyList_CAST(op);
    assert(0 <= index);
    assert(index < list->allocated);
    list->ob_item[index] = value;
}
#define PyList_SET_ITEM(op, index, value) \
    PyList_SET_ITEM(_PyObject_CAST(op), (index), _PyObject_CAST(value))

PyAPI_FUNC(int) PyList_Extend(PyObject *self, PyObject *iterable);
PyAPI_FUNC(int) PyList_Clear(PyObject *self);
