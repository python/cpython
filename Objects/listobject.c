/* List object implementation */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_critical_section.h"  // _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED()
#include "pycore_dict.h"          // _PyDictViewObject
#include "pycore_freelist.h"      // _Py_FREELIST_FREE(), _Py_FREELIST_POP()
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_interp.h"        // PyInterpreterState.list
#include "pycore_list.h"          // struct _Py_list_freelist, _PyListIterObject
#include "pycore_long.h"          // _PyLong_DigitCount
#include "pycore_modsupport.h"    // _PyArg_NoKwnames()
#include "pycore_object.h"        // _PyObject_GC_TRACK(), _PyDebugAllocatorStats()
#include "pycore_stackref.h"      // _Py_TryIncrefCompareStackRef()
#include "pycore_tuple.h"         // _PyTuple_FromArray()
#include "pycore_typeobject.h"    // _Py_TYPE_VERSION_LIST
#include "pycore_setobject.h"     // _PySet_NextEntry()
#include <stddef.h>

/*[clinic input]
class list "PyListObject *" "&PyList_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f9b222678f9f71e0]*/

#include "clinic/listobject.c.h"

_Py_DECLARE_STR(list_err, "list index out of range");

#ifdef Py_GIL_DISABLED
typedef struct {
    Py_ssize_t allocated;
    PyObject *ob_item[];
} _PyListArray;

static _PyListArray *
list_allocate_array(size_t capacity)
{
    if (capacity > PY_SSIZE_T_MAX/sizeof(PyObject*) - 1) {
        return NULL;
    }
    _PyListArray *array = PyMem_Malloc(sizeof(_PyListArray) + capacity * sizeof(PyObject *));
    if (array == NULL) {
        return NULL;
    }
    array->allocated = capacity;
    return array;
}

static Py_ssize_t
list_capacity(PyObject **items)
{
    _PyListArray *array = _Py_CONTAINER_OF(items, _PyListArray, ob_item);
    return array->allocated;
}
#endif

static void
free_list_items(PyObject** items, bool use_qsbr)
{
#ifdef Py_GIL_DISABLED
    _PyListArray *array = _Py_CONTAINER_OF(items, _PyListArray, ob_item);
    if (use_qsbr) {
        _PyMem_FreeDelayed(array);
    }
    else {
        PyMem_Free(array);
    }
#else
    PyMem_Free(items);
#endif
}

static void
ensure_shared_on_resize(PyListObject *self)
{
#ifdef Py_GIL_DISABLED
    // We can't use _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED here because
    // the `CALL_LIST_APPEND` bytecode handler may lock the list without
    // a critical section.
    assert(Py_REFCNT(self) == 1 || PyMutex_IsLocked(&_PyObject_CAST(self)->ob_mutex));

    // Ensure that the list array is freed using QSBR if we are not the
    // owning thread.
    if (!_Py_IsOwnedByCurrentThread((PyObject *)self) &&
        !_PyObject_GC_IS_SHARED(self))
    {
        _PyObject_GC_SET_SHARED(self);
    }
#endif
}

/* Ensure ob_item has room for at least newsize elements, and set
 * ob_size to newsize.  If newsize > ob_size on entry, the content
 * of the new slots at exit is undefined heap trash; it's the caller's
 * responsibility to overwrite them with sane values.
 * The number of allocated elements may grow, shrink, or stay the same.
 * Failure is impossible if newsize <= self.allocated on entry, although
 * that partly relies on an assumption that the system realloc() never
 * fails when passed a number of bytes <= the number of bytes last
 * allocated (the C standard doesn't guarantee this, but it's hard to
 * imagine a realloc implementation where it wouldn't be true).
 * Note that self->ob_item may change, and even if newsize is less
 * than ob_size on entry.
 */
static int
list_resize(PyListObject *self, Py_ssize_t newsize)
{
    size_t new_allocated, target_bytes;
    Py_ssize_t allocated = self->allocated;

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize falls lower than half
       the allocated size, then proceed with the realloc() to shrink the list.
    */
    if (allocated >= newsize && newsize >= (allocated >> 1)) {
        assert(self->ob_item != NULL || newsize == 0);
        Py_SET_SIZE(self, newsize);
        return 0;
    }

    /* This over-allocates proportional to the list size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * Add padding to make the allocated size multiple of 4.
     * The growth pattern is:  0, 4, 8, 16, 24, 32, 40, 52, 64, 76, ...
     * Note: new_allocated won't overflow because the largest possible value
     *       is PY_SSIZE_T_MAX * (9 / 8) + 6 which always fits in a size_t.
     */
    new_allocated = ((size_t)newsize + (newsize >> 3) + 6) & ~(size_t)3;
    /* Do not overallocate if the new size is closer to overallocated size
     * than to the old size.
     */
    if (newsize - Py_SIZE(self) > (Py_ssize_t)(new_allocated - newsize))
        new_allocated = ((size_t)newsize + 3) & ~(size_t)3;

    if (newsize == 0)
        new_allocated = 0;

    ensure_shared_on_resize(self);

#ifdef Py_GIL_DISABLED
    _PyListArray *array = list_allocate_array(new_allocated);
    if (array == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    PyObject **old_items = self->ob_item;
    if (self->ob_item) {
        if (new_allocated < (size_t)allocated) {
            target_bytes = new_allocated * sizeof(PyObject*);
        }
        else {
            target_bytes = allocated * sizeof(PyObject*);
        }
        memcpy(array->ob_item, self->ob_item, target_bytes);
    }
    if (new_allocated > (size_t)allocated) {
        memset(array->ob_item + allocated, 0, sizeof(PyObject *) * (new_allocated - allocated));
    }
     _Py_atomic_store_ptr_release(&self->ob_item, &array->ob_item);
    self->allocated = new_allocated;
    Py_SET_SIZE(self, newsize);
    if (old_items != NULL) {
        free_list_items(old_items, _PyObject_GC_IS_SHARED(self));
    }
#else
    PyObject **items;
    if (new_allocated <= (size_t)PY_SSIZE_T_MAX / sizeof(PyObject *)) {
        target_bytes = new_allocated * sizeof(PyObject *);
        items = (PyObject **)PyMem_Realloc(self->ob_item, target_bytes);
    }
    else {
        // integer overflow
        items = NULL;
    }
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
    Py_SET_SIZE(self, newsize);
    self->allocated = new_allocated;
#endif
    return 0;
}

static int
list_preallocate_exact(PyListObject *self, Py_ssize_t size)
{
    PyObject **items;
    assert(self->ob_item == NULL);
    assert(size > 0);

    /* Since the Python memory allocator has granularity of 16 bytes on 64-bit
     * platforms (8 on 32-bit), there is no benefit of allocating space for
     * the odd number of items, and there is no drawback of rounding the
     * allocated size up to the nearest even number.
     */
    size = (size + 1) & ~(size_t)1;
#ifdef Py_GIL_DISABLED
    _PyListArray *array = list_allocate_array(size);
    if (array == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    items = array->ob_item;
    memset(items, 0, size * sizeof(PyObject *));
#else
    items = PyMem_New(PyObject*, size);
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
#endif
    FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item, items);
    self->allocated = size;
    return 0;
}

/* Print summary info about the state of the optimized allocator */
void
_PyList_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyListObject",
                            _Py_FREELIST_SIZE(lists),
                           sizeof(PyListObject));
}

PyObject *
PyList_New(Py_ssize_t size)
{
    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    PyListObject *op = _Py_FREELIST_POP(PyListObject, lists);
    if (op == NULL) {
        op = PyObject_GC_New(PyListObject, &PyList_Type);
        if (op == NULL) {
            return NULL;
        }
    }
    if (size <= 0) {
        op->ob_item = NULL;
    }
    else {
#ifdef Py_GIL_DISABLED
        _PyListArray *array = list_allocate_array(size);
        if (array == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
        memset(&array->ob_item, 0, size * sizeof(PyObject *));
        op->ob_item = array->ob_item;
#else
        op->ob_item = (PyObject **) PyMem_Calloc(size, sizeof(PyObject *));
#endif
        if (op->ob_item == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
    }
    Py_SET_SIZE(op, size);
    op->allocated = size;
    _PyObject_GC_TRACK(op);
    return (PyObject *) op;
}

static PyObject *
list_new_prealloc(Py_ssize_t size)
{
    assert(size > 0);
    PyListObject *op = (PyListObject *) PyList_New(0);
    if (op == NULL) {
        return NULL;
    }
    assert(op->ob_item == NULL);
#ifdef Py_GIL_DISABLED
    _PyListArray *array = list_allocate_array(size);
    if (array == NULL) {
        Py_DECREF(op);
        return PyErr_NoMemory();
    }
    op->ob_item = array->ob_item;
#else
    op->ob_item = PyMem_New(PyObject *, size);
    if (op->ob_item == NULL) {
        Py_DECREF(op);
        return PyErr_NoMemory();
    }
#endif
    op->allocated = size;
    return (PyObject *) op;
}

Py_ssize_t
PyList_Size(PyObject *op)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    else {
        return PyList_GET_SIZE(op);
    }
}

static inline int
valid_index(Py_ssize_t i, Py_ssize_t limit)
{
    /* The cast to size_t lets us use just a single comparison
       to check whether i is in the range: 0 <= i < limit.

       See:  Section 14.2 "Bounds Checking" in the Agner Fog
       optimization manual found at:
       https://www.agner.org/optimize/optimizing_cpp.pdf
    */
    return (size_t) i < (size_t) limit;
}

#ifdef Py_GIL_DISABLED

static PyObject *
list_item_impl(PyListObject *self, Py_ssize_t idx)
{
    PyObject *item = NULL;
    Py_BEGIN_CRITICAL_SECTION(self);
    if (!_PyObject_GC_IS_SHARED(self)) {
        _PyObject_GC_SET_SHARED(self);
    }
    Py_ssize_t size = Py_SIZE(self);
    if (!valid_index(idx, size)) {
        goto exit;
    }
    item = _Py_NewRefWithLock(self->ob_item[idx]);
exit:
    Py_END_CRITICAL_SECTION();
    return item;
}

static inline PyObject*
list_get_item_ref(PyListObject *op, Py_ssize_t i)
{
    if (!_Py_IsOwnedByCurrentThread((PyObject *)op) && !_PyObject_GC_IS_SHARED(op)) {
        return list_item_impl(op, i);
    }
    // Need atomic operation for the getting size.
    Py_ssize_t size = PyList_GET_SIZE(op);
    if (!valid_index(i, size)) {
        return NULL;
    }
    PyObject **ob_item = _Py_atomic_load_ptr(&op->ob_item);
    if (ob_item == NULL) {
        return NULL;
    }
    Py_ssize_t cap = list_capacity(ob_item);
    assert(cap != -1);
    if (!valid_index(i, cap)) {
        return NULL;
    }
    PyObject *item = _Py_TryXGetRef(&ob_item[i]);
    if (item == NULL) {
        return list_item_impl(op, i);
    }
    return item;
}
#else
static inline PyObject*
list_get_item_ref(PyListObject *op, Py_ssize_t i)
{
    if (!valid_index(i, Py_SIZE(op))) {
        return NULL;
    }
    return Py_NewRef(PyList_GET_ITEM(op, i));
}
#endif

PyObject *
PyList_GetItem(PyObject *op, Py_ssize_t i)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (!valid_index(i, Py_SIZE(op))) {
        _Py_DECLARE_STR(list_err, "list index out of range");
        PyErr_SetObject(PyExc_IndexError, &_Py_STR(list_err));
        return NULL;
    }
    return ((PyListObject *)op) -> ob_item[i];
}

PyObject *
PyList_GetItemRef(PyObject *op, Py_ssize_t i)
{
    if (!PyList_Check(op)) {
        PyErr_SetString(PyExc_TypeError, "expected a list");
        return NULL;
    }
    PyObject *item = list_get_item_ref((PyListObject *)op, i);
    if (item == NULL) {
        _Py_DECLARE_STR(list_err, "list index out of range");
        PyErr_SetObject(PyExc_IndexError, &_Py_STR(list_err));
        return NULL;
    }
    return item;
}

PyObject *
_PyList_GetItemRef(PyListObject *list, Py_ssize_t i)
{
    return list_get_item_ref(list, i);
}

#ifdef Py_GIL_DISABLED
int
_PyList_GetItemRefNoLock(PyListObject *list, Py_ssize_t i, _PyStackRef *result)
{
    assert(_Py_IsOwnedByCurrentThread((PyObject *)list) ||
           _PyObject_GC_IS_SHARED(list));
    if (!valid_index(i, PyList_GET_SIZE(list))) {
        return 0;
    }
    PyObject **ob_item = _Py_atomic_load_ptr(&list->ob_item);
    if (ob_item == NULL) {
        return 0;
    }
    Py_ssize_t cap = list_capacity(ob_item);
    assert(cap != -1);
    if (!valid_index(i, cap)) {
        return 0;
    }
    PyObject *obj = _Py_atomic_load_ptr(&ob_item[i]);
    if (obj == NULL || !_Py_TryIncrefCompareStackRef(&ob_item[i], obj, result)) {
        return -1;
    }
    return 1;
}
#endif

int
PyList_SetItem(PyObject *op, Py_ssize_t i,
               PyObject *newitem)
{
    if (!PyList_Check(op)) {
        Py_XDECREF(newitem);
        PyErr_BadInternalCall();
        return -1;
    }
    int ret;
    PyListObject *self = ((PyListObject *)op);
    Py_BEGIN_CRITICAL_SECTION(self);
    if (!valid_index(i, Py_SIZE(self))) {
        Py_XDECREF(newitem);
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        ret = -1;
        goto end;
    }
    PyObject *tmp = self->ob_item[i];
    FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item[i], newitem);
    Py_XDECREF(tmp);
    ret = 0;
end:;
    Py_END_CRITICAL_SECTION();
    return ret;
}

static int
ins1(PyListObject *self, Py_ssize_t where, PyObject *v)
{
    Py_ssize_t i, n = Py_SIZE(self);
    PyObject **items;
    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }

    assert((size_t)n + 1 < PY_SSIZE_T_MAX);
    if (list_resize(self, n+1) < 0)
        return -1;

    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    items = self->ob_item;
    for (i = n; --i >= where; )
        FT_ATOMIC_STORE_PTR_RELAXED(items[i+1], items[i]);
    FT_ATOMIC_STORE_PTR_RELEASE(items[where], Py_NewRef(v));
    return 0;
}

int
PyList_Insert(PyObject *op, Py_ssize_t where, PyObject *newitem)
{
    if (!PyList_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    PyListObject *self = (PyListObject *)op;
    int err;
    Py_BEGIN_CRITICAL_SECTION(self);
    err = ins1(self, where, newitem);
    Py_END_CRITICAL_SECTION();
    return err;
}

/* internal, used by _PyList_AppendTakeRef */
int
_PyList_AppendTakeRefListResize(PyListObject *self, PyObject *newitem)
{
    Py_ssize_t len = Py_SIZE(self);
    assert(self->allocated == -1 || self->allocated == len);
    if (list_resize(self, len + 1) < 0) {
        Py_DECREF(newitem);
        return -1;
    }
    FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item[len], newitem);
    return 0;
}

int
PyList_Append(PyObject *op, PyObject *newitem)
{
    if (PyList_Check(op) && (newitem != NULL)) {
        int ret;
        Py_BEGIN_CRITICAL_SECTION(op);
        ret = _PyList_AppendTakeRef((PyListObject *)op, Py_NewRef(newitem));
        Py_END_CRITICAL_SECTION();
        return ret;
    }
    PyErr_BadInternalCall();
    return -1;
}

/* Methods */

static void
list_dealloc(PyObject *self)
{
    PyListObject *op = (PyListObject *)self;
    Py_ssize_t i;
    PyObject_GC_UnTrack(op);
    if (op->ob_item != NULL) {
        /* Do it backwards, for Christian Tismer.
           There's a simple test case where somehow this reduces
           thrashing when a *very* large list is created and
           immediately deleted. */
        i = Py_SIZE(op);
        while (--i >= 0) {
            Py_XDECREF(op->ob_item[i]);
        }
        free_list_items(op->ob_item, false);
        op->ob_item = NULL;
    }
    if (PyList_CheckExact(op)) {
        _Py_FREELIST_FREE(lists, op, PyObject_GC_Del);
    }
    else {
        PyObject_GC_Del(op);
    }
}

static PyObject *
list_repr_impl(PyListObject *v)
{
    int res = Py_ReprEnter((PyObject*)v);
    if (res != 0) {
        return (res > 0 ? PyUnicode_FromString("[...]") : NULL);
    }

    /* "[" + "1" + ", 2" * (len - 1) + "]" */
    Py_ssize_t prealloc = 1 + 1 + (2 + 1) * (Py_SIZE(v) - 1) + 1;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(prealloc);
    PyObject *item = NULL;
    if (writer == NULL) {
        goto error;
    }

    if (PyUnicodeWriter_WriteChar(writer, '[') < 0) {
        goto error;
    }

    /* Do repr() on each element.  Note that this may mutate the list,
       so must refetch the list size on each iteration. */
    for (Py_ssize_t i = 0; i < Py_SIZE(v); ++i) {
        /* Hold a strong reference since repr(item) can mutate the list */
        item = Py_NewRef(v->ob_item[i]);

        if (i > 0) {
            if (PyUnicodeWriter_WriteChar(writer, ',') < 0) {
                goto error;
            }
            if (PyUnicodeWriter_WriteChar(writer, ' ') < 0) {
                goto error;
            }
        }

        if (PyUnicodeWriter_WriteRepr(writer, item) < 0) {
            goto error;
        }
        Py_CLEAR(item);
    }

    if (PyUnicodeWriter_WriteChar(writer, ']') < 0) {
        goto error;
    }

    Py_ReprLeave((PyObject *)v);
    return PyUnicodeWriter_Finish(writer);

error:
    Py_XDECREF(item);
    PyUnicodeWriter_Discard(writer);
    Py_ReprLeave((PyObject *)v);
    return NULL;
}

static PyObject *
list_repr(PyObject *self)
{
    if (PyList_GET_SIZE(self) == 0) {
        return PyUnicode_FromString("[]");
    }
    PyListObject *v = (PyListObject *)self;
    PyObject *ret = NULL;
    Py_BEGIN_CRITICAL_SECTION(v);
    ret = list_repr_impl(v);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static Py_ssize_t
list_length(PyObject *a)
{
    return PyList_GET_SIZE(a);
}

static int
list_contains(PyObject *aa, PyObject *el)
{

    for (Py_ssize_t i = 0; ; i++) {
        PyObject *item = list_get_item_ref((PyListObject *)aa, i);
        if (item == NULL) {
            // out-of-bounds
            return 0;
        }
        int cmp = PyObject_RichCompareBool(item, el, Py_EQ);
        Py_DECREF(item);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}

static PyObject *
list_item(PyObject *aa, Py_ssize_t i)
{
    PyListObject *a = (PyListObject *)aa;
    if (!valid_index(i, PyList_GET_SIZE(a))) {
        PyErr_SetObject(PyExc_IndexError, &_Py_STR(list_err));
        return NULL;
    }
    PyObject *item;
#ifdef Py_GIL_DISABLED
    item = list_get_item_ref(a, i);
    if (item == NULL) {
        PyErr_SetObject(PyExc_IndexError, &_Py_STR(list_err));
        return NULL;
    }
#else
    item = Py_NewRef(a->ob_item[i]);
#endif
    return item;
}

static PyObject *
list_slice_lock_held(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    PyListObject *np;
    PyObject **src, **dest;
    Py_ssize_t i, len;
    len = ihigh - ilow;
    if (len <= 0) {
        return PyList_New(0);
    }
    np = (PyListObject *) list_new_prealloc(len);
    if (np == NULL)
        return NULL;

    src = a->ob_item + ilow;
    dest = np->ob_item;
    for (i = 0; i < len; i++) {
        PyObject *v = src[i];
        dest[i] = Py_NewRef(v);
    }
    Py_SET_SIZE(np, len);
    return (PyObject *)np;
}

PyObject *
PyList_GetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    if (!PyList_Check(a)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(a);
    if (ilow < 0) {
        ilow = 0;
    }
    else if (ilow > Py_SIZE(a)) {
        ilow = Py_SIZE(a);
    }
    if (ihigh < ilow) {
        ihigh = ilow;
    }
    else if (ihigh > Py_SIZE(a)) {
        ihigh = Py_SIZE(a);
    }
    ret = list_slice_lock_held((PyListObject *)a, ilow, ihigh);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static PyObject *
list_concat_lock_held(PyListObject *a, PyListObject *b)
{
    Py_ssize_t size;
    Py_ssize_t i;
    PyObject **src, **dest;
    PyListObject *np;
    assert((size_t)Py_SIZE(a) + (size_t)Py_SIZE(b) < PY_SSIZE_T_MAX);
    size = Py_SIZE(a) + Py_SIZE(b);
    if (size == 0) {
        return PyList_New(0);
    }
    np = (PyListObject *) list_new_prealloc(size);
    if (np == NULL) {
        return NULL;
    }
    src = a->ob_item;
    dest = np->ob_item;
    for (i = 0; i < Py_SIZE(a); i++) {
        PyObject *v = src[i];
        dest[i] = Py_NewRef(v);
    }
    src = b->ob_item;
    dest = np->ob_item + Py_SIZE(a);
    for (i = 0; i < Py_SIZE(b); i++) {
        PyObject *v = src[i];
        dest[i] = Py_NewRef(v);
    }
    Py_SET_SIZE(np, size);
    return (PyObject *)np;
}

static PyObject *
list_concat(PyObject *aa, PyObject *bb)
{
    if (!PyList_Check(bb)) {
        PyErr_Format(PyExc_TypeError,
                  "can only concatenate list (not \"%.200s\") to list",
                  Py_TYPE(bb)->tp_name);
        return NULL;
    }
    PyListObject *a = (PyListObject *)aa;
    PyListObject *b = (PyListObject *)bb;
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION2(a, b);
    ret = list_concat_lock_held(a, b);
    Py_END_CRITICAL_SECTION2();
    return ret;
}

static PyObject *
list_repeat_lock_held(PyListObject *a, Py_ssize_t n)
{
    const Py_ssize_t input_size = Py_SIZE(a);
    if (input_size == 0 || n <= 0)
        return PyList_New(0);
    assert(n > 0);

    if (input_size > PY_SSIZE_T_MAX / n)
        return PyErr_NoMemory();
    Py_ssize_t output_size = input_size * n;

    PyListObject *np = (PyListObject *) list_new_prealloc(output_size);
    if (np == NULL)
        return NULL;

    PyObject **dest = np->ob_item;
    if (input_size == 1) {
        PyObject *elem = a->ob_item[0];
        _Py_RefcntAdd(elem, n);
        PyObject **dest_end = dest + output_size;
        while (dest < dest_end) {
            *dest++ = elem;
        }
    }
    else {
        PyObject **src = a->ob_item;
        PyObject **src_end = src + input_size;
        while (src < src_end) {
            _Py_RefcntAdd(*src, n);
            *dest++ = *src++;
        }
        // TODO: _Py_memory_repeat calls are not safe for shared lists in
        // GIL_DISABLED builds. (See issue #129069)
        _Py_memory_repeat((char *)np->ob_item, sizeof(PyObject *)*output_size,
                                        sizeof(PyObject *)*input_size);
    }

    Py_SET_SIZE(np, output_size);
    return (PyObject *) np;
}

static PyObject *
list_repeat(PyObject *aa, Py_ssize_t n)
{
    PyObject *ret;
    PyListObject *a = (PyListObject *)aa;
    Py_BEGIN_CRITICAL_SECTION(a);
    ret = list_repeat_lock_held(a, n);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static void
list_clear_impl(PyListObject *a, bool is_resize)
{
    PyObject **items = a->ob_item;
    if (items == NULL) {
        return;
    }

    /* Because XDECREF can recursively invoke operations on
       this list, we make it empty first. */
    Py_ssize_t i = Py_SIZE(a);
    Py_SET_SIZE(a, 0);
    FT_ATOMIC_STORE_PTR_RELEASE(a->ob_item, NULL);
    a->allocated = 0;
    while (--i >= 0) {
        Py_XDECREF(items[i]);
    }
#ifdef Py_GIL_DISABLED
    if (is_resize) {
        ensure_shared_on_resize(a);
    }
    bool use_qsbr = is_resize && _PyObject_GC_IS_SHARED(a);
#else
    bool use_qsbr = false;
#endif
    free_list_items(items, use_qsbr);
    // Note that there is no guarantee that the list is actually empty
    // at this point, because XDECREF may have populated it indirectly again!
}

static void
list_clear(PyListObject *a)
{
    list_clear_impl(a, true);
}

static int
list_clear_slot(PyObject *self)
{
    list_clear_impl((PyListObject *)self, false);
    return 0;
}

/* a[ilow:ihigh] = v if v != NULL.
 * del a[ilow:ihigh] if v == NULL.
 *
 * Special speed gimmick:  when v is NULL and ihigh - ilow <= 8, it's
 * guaranteed the call cannot fail.
 */
static int
list_ass_slice_lock_held(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    /* Because [X]DECREF can recursively invoke list operations on
       this list, we must postpone all [X]DECREF activity until
       after the list is back in its canonical shape.  Therefore
       we must allocate an additional array, 'recycle', into which
       we temporarily copy the items that are deleted from the
       list. :-( */
    PyObject *recycle_on_stack[8];
    PyObject **recycle = recycle_on_stack; /* will allocate more if needed */
    PyObject **item;
    PyObject **vitem = NULL;
    PyObject *v_as_SF = NULL; /* PySequence_Fast(v) */
    Py_ssize_t n; /* # of elements in replacement list */
    Py_ssize_t norig; /* # of elements in list getting replaced */
    Py_ssize_t d; /* Change in size */
    Py_ssize_t k;
    size_t s;
    int result = -1;            /* guilty until proved innocent */
#define b ((PyListObject *)v)
    if (v == NULL)
        n = 0;
    else {
        v_as_SF = PySequence_Fast(v, "can only assign an iterable");
        if(v_as_SF == NULL)
            goto Error;
        n = PySequence_Fast_GET_SIZE(v_as_SF);
        vitem = PySequence_Fast_ITEMS(v_as_SF);
    }
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);

    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);

    norig = ihigh - ilow;
    assert(norig >= 0);
    d = n - norig;
    if (Py_SIZE(a) + d == 0) {
        Py_XDECREF(v_as_SF);
        list_clear(a);
        return 0;
    }
    item = a->ob_item;
    /* recycle the items that we are about to remove */
    s = norig * sizeof(PyObject *);
    /* If norig == 0, item might be NULL, in which case we may not memcpy from it. */
    if (s) {
        if (s > sizeof(recycle_on_stack)) {
            recycle = (PyObject **)PyMem_Malloc(s);
            if (recycle == NULL) {
                PyErr_NoMemory();
                goto Error;
            }
        }
        memcpy(recycle, &item[ilow], s);
    }

    if (d < 0) { /* Delete -d items */
        Py_ssize_t tail;
        tail = (Py_SIZE(a) - ihigh) * sizeof(PyObject *);
        // TODO: these memmove/memcpy calls are not safe for shared lists in
        // GIL_DISABLED builds. (See issue #129069)
        memmove(&item[ihigh+d], &item[ihigh], tail);
        if (list_resize(a, Py_SIZE(a) + d) < 0) {
            memmove(&item[ihigh], &item[ihigh+d], tail);
            memcpy(&item[ilow], recycle, s);
            goto Error;
        }
        item = a->ob_item;
    }
    else if (d > 0) { /* Insert d items */
        k = Py_SIZE(a);
        if (list_resize(a, k+d) < 0)
            goto Error;
        item = a->ob_item;
        // TODO: these memmove/memcpy calls are not safe for shared lists in
        // GIL_DISABLED builds. (See issue #129069)
        memmove(&item[ihigh+d], &item[ihigh],
            (k - ihigh)*sizeof(PyObject *));
    }
    for (k = 0; k < n; k++, ilow++) {
        PyObject *w = vitem[k];
        FT_ATOMIC_STORE_PTR_RELEASE(item[ilow], Py_XNewRef(w));
    }
    for (k = norig - 1; k >= 0; --k)
        Py_XDECREF(recycle[k]);
    result = 0;
 Error:
    if (recycle != recycle_on_stack)
        PyMem_Free(recycle);
    Py_XDECREF(v_as_SF);
    return result;
#undef b
}

static int
list_ass_slice(PyListObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    int ret;
    if (a == (PyListObject *)v) {
        Py_BEGIN_CRITICAL_SECTION(a);
        Py_ssize_t n = PyList_GET_SIZE(a);
        PyObject *copy = list_slice_lock_held(a, 0, n);
        if (copy == NULL) {
            ret = -1;
        }
        else {
            ret = list_ass_slice_lock_held(a, ilow, ihigh, copy);
            Py_DECREF(copy);
        }
        Py_END_CRITICAL_SECTION();
    }
    else if (v != NULL && PyList_CheckExact(v)) {
        Py_BEGIN_CRITICAL_SECTION2(a, v);
        ret = list_ass_slice_lock_held(a, ilow, ihigh, v);
        Py_END_CRITICAL_SECTION2();
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(a);
        ret = list_ass_slice_lock_held(a, ilow, ihigh, v);
        Py_END_CRITICAL_SECTION();
    }
    return ret;
}

int
PyList_SetSlice(PyObject *a, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    if (!PyList_Check(a)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return list_ass_slice((PyListObject *)a, ilow, ihigh, v);
}

static int
list_inplace_repeat_lock_held(PyListObject *self, Py_ssize_t n)
{
    Py_ssize_t input_size = PyList_GET_SIZE(self);
    if (input_size == 0 || n == 1) {
        return 0;
    }

    if (n < 1) {
        list_clear(self);
        return 0;
    }

    if (input_size > PY_SSIZE_T_MAX / n) {
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t output_size = input_size * n;

    if (list_resize(self, output_size) < 0) {
        return -1;
    }

    PyObject **items = self->ob_item;
    for (Py_ssize_t j = 0; j < input_size; j++) {
        _Py_RefcntAdd(items[j], n-1);
    }
    // TODO: _Py_memory_repeat calls are not safe for shared lists in
    // GIL_DISABLED builds. (See issue #129069)
    _Py_memory_repeat((char *)items, sizeof(PyObject *)*output_size,
                      sizeof(PyObject *)*input_size);
    return 0;
}

static PyObject *
list_inplace_repeat(PyObject *_self, Py_ssize_t n)
{
    PyObject *ret;
    PyListObject *self = (PyListObject *) _self;
    Py_BEGIN_CRITICAL_SECTION(self);
    if (list_inplace_repeat_lock_held(self, n) < 0) {
        ret = NULL;
    }
    else {
        ret = Py_NewRef(self);
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

static int
list_ass_item_lock_held(PyListObject *a, Py_ssize_t i, PyObject *v)
{
    if (!valid_index(i, Py_SIZE(a))) {
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
    PyObject *tmp = a->ob_item[i];
    if (v == NULL) {
        Py_ssize_t size = Py_SIZE(a);
        for (Py_ssize_t idx = i; idx < size - 1; idx++) {
            FT_ATOMIC_STORE_PTR_RELAXED(a->ob_item[idx], a->ob_item[idx + 1]);
        }
        Py_SET_SIZE(a, size - 1);
    }
    else {
        FT_ATOMIC_STORE_PTR_RELEASE(a->ob_item[i], Py_NewRef(v));
    }
    Py_DECREF(tmp);
    return 0;
}

static int
list_ass_item(PyObject *aa, Py_ssize_t i, PyObject *v)
{
    int ret;
    PyListObject *a = (PyListObject *)aa;
    Py_BEGIN_CRITICAL_SECTION(a);
    ret = list_ass_item_lock_held(a, i, v);
    Py_END_CRITICAL_SECTION();
    return ret;
}

/*[clinic input]
@critical_section
list.insert

    index: Py_ssize_t
    object: object
    /

Insert object before index.
[clinic start generated code]*/

static PyObject *
list_insert_impl(PyListObject *self, Py_ssize_t index, PyObject *object)
/*[clinic end generated code: output=7f35e32f60c8cb78 input=b1987ca998a4ae2d]*/
{
    if (ins1(self, index, object) == 0) {
        Py_RETURN_NONE;
    }
    return NULL;
}

/*[clinic input]
@critical_section
list.clear as py_list_clear

Remove all items from list.
[clinic start generated code]*/

static PyObject *
py_list_clear_impl(PyListObject *self)
/*[clinic end generated code: output=83726743807e3518 input=e285b7f09051a9ba]*/
{
    list_clear(self);
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
list.copy

Return a shallow copy of the list.
[clinic start generated code]*/

static PyObject *
list_copy_impl(PyListObject *self)
/*[clinic end generated code: output=ec6b72d6209d418e input=81c54b0c7bb4f73d]*/
{
    return list_slice_lock_held(self, 0, Py_SIZE(self));
}

/*[clinic input]
@critical_section
list.append

     object: object
     /

Append object to the end of the list.
[clinic start generated code]*/

static PyObject *
list_append_impl(PyListObject *self, PyObject *object)
/*[clinic end generated code: output=78423561d92ed405 input=122b0853de54004f]*/
{
    if (_PyList_AppendTakeRef(self, Py_NewRef(object)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
list_extend_fast(PyListObject *self, PyObject *iterable)
{
    Py_ssize_t n = PySequence_Fast_GET_SIZE(iterable);
    if (n == 0) {
        /* short circuit when iterable is empty */
        return 0;
    }

    Py_ssize_t m = Py_SIZE(self);
    // It should not be possible to allocate a list large enough to cause
    // an overflow on any relevant platform.
    assert(m < PY_SSIZE_T_MAX - n);
    if (self->ob_item == NULL) {
        if (list_preallocate_exact(self, n) < 0) {
            return -1;
        }
        Py_SET_SIZE(self, n);
    }
    else if (list_resize(self, m + n) < 0) {
        return -1;
    }

    // note that we may still have self == iterable here for the
    // situation a.extend(a), but the following code works
    // in that case too.  Just make sure to resize self
    // before calling PySequence_Fast_ITEMS.
    //
    // populate the end of self with iterable's items.
    PyObject **src = PySequence_Fast_ITEMS(iterable);
    PyObject **dest = self->ob_item + m;
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *o = src[i];
        FT_ATOMIC_STORE_PTR_RELEASE(dest[i], Py_NewRef(o));
    }
    return 0;
}

static int
list_extend_iter_lock_held(PyListObject *self, PyObject *iterable)
{
    PyObject *it = PyObject_GetIter(iterable);
    if (it == NULL) {
        return -1;
    }
    PyObject *(*iternext)(PyObject *) = *Py_TYPE(it)->tp_iternext;

    /* Guess a result list size. */
    Py_ssize_t n = PyObject_LengthHint(iterable, 8);
    if (n < 0) {
        Py_DECREF(it);
        return -1;
    }

    Py_ssize_t m = Py_SIZE(self);
    if (m > PY_SSIZE_T_MAX - n) {
        /* m + n overflowed; on the chance that n lied, and there really
         * is enough room, ignore it.  If n was telling the truth, we'll
         * eventually run out of memory during the loop.
         */
    }
    else if (self->ob_item == NULL) {
        if (n && list_preallocate_exact(self, n) < 0)
            goto error;
    }
    else {
        /* Make room. */
        if (list_resize(self, m + n) < 0) {
            goto error;
        }

        /* Make the list sane again. */
        Py_SET_SIZE(self, m);
    }

    /* Run iterator to exhaustion. */
    for (;;) {
        PyObject *item = iternext(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                if (PyErr_ExceptionMatches(PyExc_StopIteration))
                    PyErr_Clear();
                else
                    goto error;
            }
            break;
        }

        if (Py_SIZE(self) < self->allocated) {
            Py_ssize_t len = Py_SIZE(self);
            FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item[len], item);  // steals item ref
            Py_SET_SIZE(self, len + 1);
        }
        else {
            if (_PyList_AppendTakeRef(self, item) < 0)
                goto error;
        }
    }

    /* Cut back result list if initial guess was too large. */
    if (Py_SIZE(self) < self->allocated) {
        if (list_resize(self, Py_SIZE(self)) < 0)
            goto error;
    }

    Py_DECREF(it);
    return 0;

  error:
    Py_DECREF(it);
    return -1;
}

static int
list_extend_lock_held(PyListObject *self, PyObject *iterable)
{
    PyObject *seq = PySequence_Fast(iterable, "argument must be iterable");
    if (!seq) {
        return -1;
    }

    int res = list_extend_fast(self, seq);
    Py_DECREF(seq);
    return res;
}

static int
list_extend_set(PyListObject *self, PySetObject *other)
{
    Py_ssize_t m = Py_SIZE(self);
    Py_ssize_t n = PySet_GET_SIZE(other);
    Py_ssize_t r = m + n;
    if (r == 0) {
        return 0;
    }
    if (list_resize(self, r) < 0) {
        return -1;
    }

    assert(self->ob_item != NULL);
    /* populate the end of self with iterable's items */
    Py_ssize_t setpos = 0;
    Py_hash_t hash;
    PyObject *key;
    PyObject **dest = self->ob_item + m;
    while (_PySet_NextEntryRef((PyObject *)other, &setpos, &key, &hash)) {
        FT_ATOMIC_STORE_PTR_RELEASE(*dest, key);
        dest++;
    }
    Py_SET_SIZE(self, r);
    return 0;
}

static int
list_extend_dict(PyListObject *self, PyDictObject *dict, int which_item)
{
    // which_item: 0 for keys and 1 for values
    Py_ssize_t m = Py_SIZE(self);
    Py_ssize_t n = PyDict_GET_SIZE(dict);
    Py_ssize_t r = m + n;
    if (r == 0) {
        return 0;
    }
    if (list_resize(self, r) < 0) {
        return -1;
    }

    assert(self->ob_item != NULL);
    PyObject **dest = self->ob_item + m;
    Py_ssize_t pos = 0;
    PyObject *keyvalue[2];
    while (_PyDict_Next((PyObject *)dict, &pos, &keyvalue[0], &keyvalue[1], NULL)) {
        PyObject *obj = keyvalue[which_item];
        Py_INCREF(obj);
        FT_ATOMIC_STORE_PTR_RELEASE(*dest, obj);
        dest++;
    }

    Py_SET_SIZE(self, r);
    return 0;
}

static int
list_extend_dictitems(PyListObject *self, PyDictObject *dict)
{
    Py_ssize_t m = Py_SIZE(self);
    Py_ssize_t n = PyDict_GET_SIZE(dict);
    Py_ssize_t r = m + n;
    if (r == 0) {
        return 0;
    }
    if (list_resize(self, r) < 0) {
        return -1;
    }

    assert(self->ob_item != NULL);
    PyObject **dest = self->ob_item + m;
    Py_ssize_t pos = 0;
    Py_ssize_t i = 0;
    PyObject *key, *value;
    while (_PyDict_Next((PyObject *)dict, &pos, &key, &value, NULL)) {
        PyObject *item = PyTuple_Pack(2, key, value);
        if (item == NULL) {
            Py_SET_SIZE(self, m + i);
            return -1;
        }
        FT_ATOMIC_STORE_PTR_RELEASE(*dest, item);
        dest++;
        i++;
    }

    Py_SET_SIZE(self, r);
    return 0;
}

static int
_list_extend(PyListObject *self, PyObject *iterable)
{
    // Special case:
    // lists and tuples which can use PySequence_Fast ops
    int res = -1;
    if ((PyObject *)self == iterable) {
        Py_BEGIN_CRITICAL_SECTION(self);
        res = list_inplace_repeat_lock_held(self, 2);
        Py_END_CRITICAL_SECTION();
    }
    else if (PyList_CheckExact(iterable)) {
        Py_BEGIN_CRITICAL_SECTION2(self, iterable);
        res = list_extend_lock_held(self, iterable);
        Py_END_CRITICAL_SECTION2();
    }
    else if (PyTuple_CheckExact(iterable)) {
        Py_BEGIN_CRITICAL_SECTION(self);
        res = list_extend_lock_held(self, iterable);
        Py_END_CRITICAL_SECTION();
    }
    else if (PyAnySet_CheckExact(iterable)) {
        Py_BEGIN_CRITICAL_SECTION2(self, iterable);
        res = list_extend_set(self, (PySetObject *)iterable);
        Py_END_CRITICAL_SECTION2();
    }
    else if (PyDict_CheckExact(iterable)) {
        Py_BEGIN_CRITICAL_SECTION2(self, iterable);
        res = list_extend_dict(self, (PyDictObject *)iterable, 0 /*keys*/);
        Py_END_CRITICAL_SECTION2();
    }
    else if (Py_IS_TYPE(iterable, &PyDictKeys_Type)) {
        PyDictObject *dict = ((_PyDictViewObject *)iterable)->dv_dict;
        Py_BEGIN_CRITICAL_SECTION2(self, dict);
        res = list_extend_dict(self, dict, 0 /*keys*/);
        Py_END_CRITICAL_SECTION2();
    }
    else if (Py_IS_TYPE(iterable, &PyDictValues_Type)) {
        PyDictObject *dict = ((_PyDictViewObject *)iterable)->dv_dict;
        Py_BEGIN_CRITICAL_SECTION2(self, dict);
        res = list_extend_dict(self, dict, 1 /*values*/);
        Py_END_CRITICAL_SECTION2();
    }
    else if (Py_IS_TYPE(iterable, &PyDictItems_Type)) {
        PyDictObject *dict = ((_PyDictViewObject *)iterable)->dv_dict;
        Py_BEGIN_CRITICAL_SECTION2(self, dict);
        res = list_extend_dictitems(self, dict);
        Py_END_CRITICAL_SECTION2();
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(self);
        res = list_extend_iter_lock_held(self, iterable);
        Py_END_CRITICAL_SECTION();
    }
    return res;
}

/*[clinic input]
list.extend as list_extend

     iterable: object
     /

Extend list by appending elements from the iterable.
[clinic start generated code]*/

static PyObject *
list_extend_impl(PyListObject *self, PyObject *iterable)
/*[clinic end generated code: output=b0eba9e0b186d5ce input=979da7597a515791]*/
{
    if (_list_extend(self, iterable) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

PyObject *
_PyList_Extend(PyListObject *self, PyObject *iterable)
{
    return list_extend((PyObject*)self, iterable);
}

int
PyList_Extend(PyObject *self, PyObject *iterable)
{
    if (!PyList_Check(self)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return _list_extend((PyListObject*)self, iterable);
}


int
PyList_Clear(PyObject *self)
{
    if (!PyList_Check(self)) {
        PyErr_BadInternalCall();
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    list_clear((PyListObject*)self);
    Py_END_CRITICAL_SECTION();
    return 0;
}


static PyObject *
list_inplace_concat(PyObject *_self, PyObject *other)
{
    PyListObject *self = (PyListObject *)_self;
    if (_list_extend(self, other) < 0) {
        return NULL;
    }
    return Py_NewRef(self);
}

/*[clinic input]
@critical_section
list.pop

    index: Py_ssize_t = -1
    /

Remove and return item at index (default last).

Raises IndexError if list is empty or index is out of range.
[clinic start generated code]*/

static PyObject *
list_pop_impl(PyListObject *self, Py_ssize_t index)
/*[clinic end generated code: output=6bd69dcb3f17eca8 input=c269141068ae4b8f]*/
{
    PyObject *v;
    int status;

    if (Py_SIZE(self) == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty list");
        return NULL;
    }
    if (index < 0)
        index += Py_SIZE(self);
    if (!valid_index(index, Py_SIZE(self))) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }

    PyObject **items = self->ob_item;
    v = items[index];
    const Py_ssize_t size_after_pop = Py_SIZE(self) - 1;
    if (size_after_pop == 0) {
        Py_INCREF(v);
        list_clear(self);
        status = 0;
    }
    else {
        if ((size_after_pop - index) > 0) {
            memmove(&items[index], &items[index+1], (size_after_pop - index) * sizeof(PyObject *));
        }
        status = list_resize(self, size_after_pop);
    }
    if (status >= 0) {
        return v; // and v now owns the reference the list had
    }
    else {
        // list resize failed, need to restore
        memmove(&items[index+1], &items[index], (size_after_pop - index)* sizeof(PyObject *));
        items[index] = v;
        return NULL;
    }
}

/* Reverse a slice of a list in place, from lo up to (exclusive) hi. */
static void
reverse_slice(PyObject **lo, PyObject **hi)
{
    assert(lo && hi);

    --hi;
    while (lo < hi) {
        PyObject *t = *lo;
        *lo = *hi;
        *hi = t;
        ++lo;
        --hi;
    }
}

/* Lots of code for an adaptive, stable, natural mergesort.  There are many
 * pieces to this algorithm; read listsort.txt for overviews and details.
 */

/* A sortslice contains a pointer to an array of keys and a pointer to
 * an array of corresponding values.  In other words, keys[i]
 * corresponds with values[i].  If values == NULL, then the keys are
 * also the values.
 *
 * Several convenience routines are provided here, so that keys and
 * values are always moved in sync.
 */

typedef struct {
    PyObject **keys;
    PyObject **values;
} sortslice;

Py_LOCAL_INLINE(void)
sortslice_copy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j)
{
    s1->keys[i] = s2->keys[j];
    if (s1->values != NULL)
        s1->values[i] = s2->values[j];
}

Py_LOCAL_INLINE(void)
sortslice_copy_incr(sortslice *dst, sortslice *src)
{
    *dst->keys++ = *src->keys++;
    if (dst->values != NULL)
        *dst->values++ = *src->values++;
}

Py_LOCAL_INLINE(void)
sortslice_copy_decr(sortslice *dst, sortslice *src)
{
    *dst->keys-- = *src->keys--;
    if (dst->values != NULL)
        *dst->values-- = *src->values--;
}


Py_LOCAL_INLINE(void)
sortslice_memcpy(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                 Py_ssize_t n)
{
    memcpy(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memcpy(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_memmove(sortslice *s1, Py_ssize_t i, sortslice *s2, Py_ssize_t j,
                  Py_ssize_t n)
{
    memmove(&s1->keys[i], &s2->keys[j], sizeof(PyObject *) * n);
    if (s1->values != NULL)
        memmove(&s1->values[i], &s2->values[j], sizeof(PyObject *) * n);
}

Py_LOCAL_INLINE(void)
sortslice_advance(sortslice *slice, Py_ssize_t n)
{
    slice->keys += n;
    if (slice->values != NULL)
        slice->values += n;
}

/* Comparison function: ms->key_compare, which is set at run-time in
 * listsort_impl to optimize for various special cases.
 * Returns -1 on error, 1 if x < y, 0 if x >= y.
 */

#define ISLT(X, Y) (*(ms->key_compare))(X, Y, ms)

/* Compare X to Y via "<".  Goto "fail" if the comparison raises an
   error.  Else "k" is set to true iff X<Y, and an "if (k)" block is
   started.  It makes more sense in context <wink>.  X and Y are PyObject*s.
*/
#define IFLT(X, Y) if ((k = ISLT(X, Y)) < 0) goto fail;  \
           if (k)

/* The maximum number of entries in a MergeState's pending-runs stack.
 * For a list with n elements, this needs at most floor(log2(n)) + 1 entries
 * even if we didn't force runs to a minimal length.  So the number of bits
 * in a Py_ssize_t is plenty large enough for all cases.
 */
#define MAX_MERGE_PENDING (SIZEOF_SIZE_T * 8)

/* When we get into galloping mode, we stay there until both runs win less
 * often than MIN_GALLOP consecutive times.  See listsort.txt for more info.
 */
#define MIN_GALLOP 7

/* Avoid malloc for small temp arrays. */
#define MERGESTATE_TEMP_SIZE 256

/* The largest value of minrun. This must be a power of 2, and >= 1, so that
 * the compute_minrun() algorithm guarantees to return a result no larger than
 * this,
 */
#define MAX_MINRUN 64
#if ((MAX_MINRUN) < 1) || ((MAX_MINRUN) & ((MAX_MINRUN) - 1))
#error "MAX_MINRUN must be a power of 2, and >= 1"
#endif

/* One MergeState exists on the stack per invocation of mergesort.  It's just
 * a convenient way to pass state around among the helper functions.
 */
struct s_slice {
    sortslice base;
    Py_ssize_t len;   /* length of run */
    int power; /* node "level" for powersort merge strategy */
};

typedef struct s_MergeState MergeState;
struct s_MergeState {
    /* This controls when we get *into* galloping mode.  It's initialized
     * to MIN_GALLOP.  merge_lo and merge_hi tend to nudge it higher for
     * random data, and lower for highly structured data.
     */
    Py_ssize_t min_gallop;

    Py_ssize_t listlen;     /* len(input_list) - read only */
    PyObject **basekeys;    /* base address of keys array - read only */

    /* 'a' is temp storage to help with merges.  It contains room for
     * alloced entries.
     */
    sortslice a;        /* may point to temparray below */
    Py_ssize_t alloced;

    /* A stack of n pending runs yet to be merged.  Run #i starts at
     * address base[i] and extends for len[i] elements.  It's always
     * true (so long as the indices are in bounds) that
     *
     *     pending[i].base + pending[i].len == pending[i+1].base
     *
     * so we could cut the storage for this, but it's a minor amount,
     * and keeping all the info explicit simplifies the code.
     */
    int n;
    struct s_slice pending[MAX_MERGE_PENDING];

    /* 'a' points to this when possible, rather than muck with malloc. */
    PyObject *temparray[MERGESTATE_TEMP_SIZE];

    /* This is the function we will use to compare two keys,
     * even when none of our special cases apply and we have to use
     * safe_object_compare. */
    int (*key_compare)(PyObject *, PyObject *, MergeState *);

    /* This function is used by unsafe_object_compare to optimize comparisons
     * when we know our list is type-homogeneous but we can't assume anything else.
     * In the pre-sort check it is set equal to Py_TYPE(key)->tp_richcompare */
    PyObject *(*key_richcompare)(PyObject *, PyObject *, int);

    /* This function is used by unsafe_tuple_compare to compare the first elements
     * of tuples. It may be set to safe_object_compare, but the idea is that hopefully
     * we can assume more, and use one of the special-case compares. */
    int (*tuple_elem_compare)(PyObject *, PyObject *, MergeState *);
};

/* binarysort is the best method for sorting small arrays: it does few
   compares, but can do data movement quadratic in the number of elements.
   ss->keys is viewed as an array of n kays, a[:n]. a[:ok] is already sorted.
   Pass ok = 0 (or 1) if you don't know.
   It's sorted in-place, by a stable binary insertion sort. If ss->values
   isn't NULL, it's permuted in lockstap with ss->keys.
   On entry, must have n >= 1, and 0 <= ok <= n <= MAX_MINRUN.
   Return -1 if comparison raises an exception, else 0.
   Even in case of error, the output slice will be some permutation of
   the input (nothing is lost or duplicated).
*/
static int
binarysort(MergeState *ms, const sortslice *ss, Py_ssize_t n, Py_ssize_t ok)
{
    Py_ssize_t k; /* for IFLT macro expansion */
    PyObject ** const a = ss->keys;
    PyObject ** const v = ss->values;
    const bool has_values = v != NULL;
    PyObject *pivot;
    Py_ssize_t M;

    assert(0 <= ok && ok <= n && 1 <= n && n <= MAX_MINRUN);
    /* assert a[:ok] is sorted */
    if (! ok)
        ++ok;
    /* Regular insertion sort has average- and worst-case O(n**2) cost
       for both # of comparisons and number of bytes moved. But its branches
       are highly predictable, and it loves sorted input (n-1 compares and no
       data movement). This is significant in cases like sortperf.py's %sort,
       where an out-of-order element near the start of a run is moved into
       place slowly but then the remaining elements up to length minrun are
       generally at worst one slot away from their correct position (so only
       need 1 or 2 commpares to resolve). If comparisons are very fast (such
       as for a list of Python floats), the simple inner loop leaves it
       very competitive with binary insertion, despite that it does
       significantly more compares overall on random data.

       Binary insertion sort has worst, average, and best case O(n log n)
       cost for # of comparisons, but worst and average case O(n**2) cost
       for data movement. The more expensive comparisons, the more important
       the comparison advantage. But its branches are less predictable the
       more "randomish" the data, and that's so significant its worst case
       in real life is random input rather than reverse-ordered (which does
       about twice the data movement than random input does).

       Note that the number of bytes moved doesn't seem to matter. MAX_MINRUN
       of 64 is so small that the key and value pointers all fit in a corner
       of L1 cache, and moving things around in that is very fast. */
#if 0 // ordinary insertion sort.
    PyObject * vpivot = NULL;
    for (; ok < n; ++ok) {
        pivot = a[ok];
        if (has_values)
            vpivot = v[ok];
        for (M = ok - 1; M >= 0; --M) {
            k = ISLT(pivot, a[M]);
            if (k < 0) {
                a[M + 1] = pivot;
                if (has_values)
                    v[M + 1] = vpivot;
                goto fail;
            }
            else if (k) {
                a[M + 1] = a[M];
                if (has_values)
                    v[M + 1] = v[M];
            }
            else
                break;
        }
        a[M + 1] = pivot;
        if (has_values)
            v[M + 1] = vpivot;
    }
#else // binary insertion sort
    Py_ssize_t L, R;
    for (; ok < n; ++ok) {
        /* set L to where a[ok] belongs */
        L = 0;
        R = ok;
        pivot = a[ok];
        /* Slice invariants. vacuously true at the start:
         * all a[0:L]  <= pivot
         * all a[L:R]     unknown
         * all a[R:ok]  > pivot
         */
        assert(L < R);
        do {
            /* don't do silly ;-) things to prevent overflow when finding
               the midpoint; L and R are very far from filling a Py_ssize_t */
            M = (L + R) >> 1;
#if 1 // straightforward, but highly unpredictable branch on random data
            IFLT(pivot, a[M])
                R = M;
            else
                L = M + 1;
#else
            /* Try to get compiler to generate conditional move instructions
               instead. Works fine, but leaving it disabled for now because
               it's not yielding consistently faster sorts. Needs more
               investigation. More computation in the inner loop adds its own
               costs, which can be significant when compares are fast. */
            k = ISLT(pivot, a[M]);
            if (k < 0)
                goto fail;
            Py_ssize_t Mp1 = M + 1;
            R = k ? M : R;
            L = k ? L : Mp1;
#endif
        } while (L < R);
        assert(L == R);
        /* a[:L] holds all elements from a[:ok] <= pivot now, so pivot belongs
           at index L. Slide a[L:ok] to the right a slot to make room for it.
           Caution: using memmove is much slower under MSVC 5; we're not
           usually moving many slots. Years later: under Visual Studio 2022,
           memmove seems just slightly slower than doing it "by hand". */
        for (M = ok; M > L; --M)
            a[M] = a[M - 1];
        a[L] = pivot;
        if (has_values) {
            pivot = v[ok];
            for (M = ok; M > L; --M)
                v[M] = v[M - 1];
            v[L] = pivot;
        }
    }
#endif // pick binary or regular insertion sort
    return 0;

 fail:
    return -1;
}

static void
sortslice_reverse(sortslice *s, Py_ssize_t n)
{
    reverse_slice(s->keys, &s->keys[n]);
    if (s->values != NULL)
        reverse_slice(s->values, &s->values[n]);
}

/*
Return the length of the run beginning at slo->keys, spanning no more than
nremaining elements. The run beginning there may be ascending or descending,
but the function permutes it in place, if needed, so that it's always ascending
upon return.

Returns -1 in case of error.
*/
static Py_ssize_t
count_run(MergeState *ms, sortslice *slo, Py_ssize_t nremaining)
{
    Py_ssize_t k; /* used by IFLT macro expansion */
    Py_ssize_t n;
    PyObject ** const lo = slo->keys;

    /* In general, as things go on we've established that the slice starts
       with a monotone run of n elements, starting at lo. */

    /* We're n elements into the slice, and the most recent neq+1 elements are
     * all equal. This reverses them in-place, and resets neq for reuse.
     */
#define REVERSE_LAST_NEQ                        \
    if (neq) {                                  \
        sortslice slice = *slo;                 \
        ++neq;                                  \
        sortslice_advance(&slice, n - neq);     \
        sortslice_reverse(&slice, neq);         \
        neq = 0;                                \
    }

    /* Sticking to only __lt__ compares is confusing and error-prone. But in
     * this routine, almost all uses of IFLT can be captured by tiny macros
     * giving mnemonic names to the intent. Note that inline functions don't
     * work for this (IFLT expands to code including `goto fail`).
     */
#define IF_NEXT_LARGER  IFLT(lo[n-1], lo[n])
#define IF_NEXT_SMALLER IFLT(lo[n], lo[n-1])

    assert(nremaining);
    /* try ascending run first */
    for (n = 1; n < nremaining; ++n) {
        IF_NEXT_SMALLER
            break;
    }
    if (n == nremaining)
        return n;
    /* lo[n] is strictly less */
    /* If n is 1 now, then the first compare established it's a descending
     * run, so fall through to the descending case. But if n > 1, there are
     * n elements in an ascending run terminated by the strictly less lo[n].
     * If the first key < lo[n-1], *somewhere* along the way the sequence
     * increased, so we're done (there is no descending run).
     * Else first key >= lo[n-1], which implies that the entire ascending run
     * consists of equal elements. In that case, this is a descending run,
     * and we reverse the all-equal prefix in-place.
     */
    if (n > 1) {
        IFLT(lo[0], lo[n-1])
            return n;
        sortslice_reverse(slo, n);
    }
    ++n; /* in all cases it's been established that lo[n] has been resolved */

    /* Finish descending run. All-squal subruns are reversed in-place on the
     * fly. Their original order will be restored at the end by the whole-slice
     * reversal.
     */
    Py_ssize_t neq = 0;
    for ( ; n < nremaining; ++n) {
        IF_NEXT_SMALLER {
            /* This ends the most recent run of equal elements, but still in
             * the "descending" direction.
             */
            REVERSE_LAST_NEQ
        }
        else {
            IF_NEXT_LARGER /* descending run is over */
                break;
            else /* not x < y and not y < x implies x == y */
                ++neq;
        }
    }
    REVERSE_LAST_NEQ
    sortslice_reverse(slo, n); /* transform to ascending run */

    /* And after reversing, it's possible this can be extended by a
     * naturally increasing suffix; e.g., [3, 2, 3, 4, 1] makes an
     * ascending run from the first 4 elements.
     */
    for ( ; n < nremaining; ++n) {
        IF_NEXT_SMALLER
            break;
    }

    return n;
fail:
    return -1;

#undef REVERSE_LAST_NEQ
#undef IF_NEXT_SMALLER
#undef IF_NEXT_LARGER
}

/*
Locate the proper position of key in a sorted vector; if the vector contains
an element equal to key, return the position immediately to the left of
the leftmost equal element.  [gallop_right() does the same except returns
the position to the right of the rightmost equal element (if any).]

"a" is a sorted vector with n elements, starting at a[0].  n must be > 0.

"hint" is an index at which to begin the search, 0 <= hint < n.  The closer
hint is to the final result, the faster this runs.

The return value is the int k in 0..n such that

    a[k-1] < key <= a[k]

pretending that *(a-1) is minus infinity and a[n] is plus infinity.  IOW,
key belongs at index k; or, IOW, the first k elements of a should precede
key, and the last n-k should follow key.

Returns -1 on error.  See listsort.txt for info on the method.
*/
static Py_ssize_t
gallop_left(MergeState *ms, PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(*a, key) {
        /* a[hint] < key -- gallop right, until
         * a[hint + lastofs] < key <= a[hint + ofs]
         */
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(a[ofs], key) {
                lastofs = ofs;
                assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
                ofs = (ofs << 1) + 1;
            }
            else                /* key <= a[hint + ofs] */
                break;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to offsets relative to &a[0]. */
        lastofs += hint;
        ofs += hint;
    }
    else {
        /* key <= a[hint] -- gallop left, until
         * a[hint - ofs] < key <= a[hint - lastofs]
         */
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(*(a-ofs), key)
                break;
            /* key <= a[hint - ofs] */
            lastofs = ofs;
            assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
            ofs = (ofs << 1) + 1;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to positive offsets relative to &a[0]. */
        k = lastofs;
        lastofs = hint - ofs;
        ofs = hint - k;
    }
    a -= hint;

    assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
    /* Now a[lastofs] < key <= a[ofs], so key belongs somewhere to the
     * right of lastofs but no farther right than ofs.  Do a binary
     * search, with invariant a[lastofs-1] < key <= a[ofs].
     */
    ++lastofs;
    while (lastofs < ofs) {
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(a[m], key)
            lastofs = m+1;              /* a[m] < key */
        else
            ofs = m;                    /* key <= a[m] */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] < key <= a[ofs] */
    return ofs;

fail:
    return -1;
}

/*
Exactly like gallop_left(), except that if key already exists in a[0:n],
finds the position immediately to the right of the rightmost equal value.

The return value is the int k in 0..n such that

    a[k-1] <= key < a[k]

or -1 if error.

The code duplication is massive, but this is enough different given that
we're sticking to "<" comparisons that it's much harder to follow if
written as one routine with yet another "left or right?" flag.
*/
static Py_ssize_t
gallop_right(MergeState *ms, PyObject *key, PyObject **a, Py_ssize_t n, Py_ssize_t hint)
{
    Py_ssize_t ofs;
    Py_ssize_t lastofs;
    Py_ssize_t k;

    assert(key && a && n > 0 && hint >= 0 && hint < n);

    a += hint;
    lastofs = 0;
    ofs = 1;
    IFLT(key, *a) {
        /* key < a[hint] -- gallop left, until
         * a[hint - ofs] <= key < a[hint - lastofs]
         */
        const Py_ssize_t maxofs = hint + 1;             /* &a[0] is lowest */
        while (ofs < maxofs) {
            IFLT(key, *(a-ofs)) {
                lastofs = ofs;
                assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
                ofs = (ofs << 1) + 1;
            }
            else                /* a[hint - ofs] <= key */
                break;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to positive offsets relative to &a[0]. */
        k = lastofs;
        lastofs = hint - ofs;
        ofs = hint - k;
    }
    else {
        /* a[hint] <= key -- gallop right, until
         * a[hint + lastofs] <= key < a[hint + ofs]
        */
        const Py_ssize_t maxofs = n - hint;             /* &a[n-1] is highest */
        while (ofs < maxofs) {
            IFLT(key, a[ofs])
                break;
            /* a[hint + ofs] <= key */
            lastofs = ofs;
            assert(ofs <= (PY_SSIZE_T_MAX - 1) / 2);
            ofs = (ofs << 1) + 1;
        }
        if (ofs > maxofs)
            ofs = maxofs;
        /* Translate back to offsets relative to &a[0]. */
        lastofs += hint;
        ofs += hint;
    }
    a -= hint;

    assert(-1 <= lastofs && lastofs < ofs && ofs <= n);
    /* Now a[lastofs] <= key < a[ofs], so key belongs somewhere to the
     * right of lastofs but no farther right than ofs.  Do a binary
     * search, with invariant a[lastofs-1] <= key < a[ofs].
     */
    ++lastofs;
    while (lastofs < ofs) {
        Py_ssize_t m = lastofs + ((ofs - lastofs) >> 1);

        IFLT(key, a[m])
            ofs = m;                    /* key < a[m] */
        else
            lastofs = m+1;              /* a[m] <= key */
    }
    assert(lastofs == ofs);             /* so a[ofs-1] <= key < a[ofs] */
    return ofs;

fail:
    return -1;
}

/* Conceptually a MergeState's constructor. */
static void
merge_init(MergeState *ms, Py_ssize_t list_size, int has_keyfunc,
           sortslice *lo)
{
    assert(ms != NULL);
    if (has_keyfunc) {
        /* The temporary space for merging will need at most half the list
         * size rounded up.  Use the minimum possible space so we can use the
         * rest of temparray for other things.  In particular, if there is
         * enough extra space, listsort() will use it to store the keys.
         */
        ms->alloced = (list_size + 1) / 2;

        /* ms->alloced describes how many keys will be stored at
           ms->temparray, but we also need to store the values.  Hence,
           ms->alloced is capped at half of MERGESTATE_TEMP_SIZE. */
        if (MERGESTATE_TEMP_SIZE / 2 < ms->alloced)
            ms->alloced = MERGESTATE_TEMP_SIZE / 2;
        ms->a.values = &ms->temparray[ms->alloced];
    }
    else {
        ms->alloced = MERGESTATE_TEMP_SIZE;
        ms->a.values = NULL;
    }
    ms->a.keys = ms->temparray;
    ms->n = 0;
    ms->min_gallop = MIN_GALLOP;
    ms->listlen = list_size;
    ms->basekeys = lo->keys;
}

/* Free all the temp memory owned by the MergeState.  This must be called
 * when you're done with a MergeState, and may be called before then if
 * you want to free the temp memory early.
 */
static void
merge_freemem(MergeState *ms)
{
    assert(ms != NULL);
    if (ms->a.keys != ms->temparray) {
        PyMem_Free(ms->a.keys);
        ms->a.keys = NULL;
    }
}

/* Ensure enough temp memory for 'need' array slots is available.
 * Returns 0 on success and -1 if the memory can't be gotten.
 */
static int
merge_getmem(MergeState *ms, Py_ssize_t need)
{
    int multiplier;

    assert(ms != NULL);
    if (need <= ms->alloced)
        return 0;

    multiplier = ms->a.values != NULL ? 2 : 1;

    /* Don't realloc!  That can cost cycles to copy the old data, but
     * we don't care what's in the block.
     */
    merge_freemem(ms);
    if ((size_t)need > PY_SSIZE_T_MAX / sizeof(PyObject *) / multiplier) {
        PyErr_NoMemory();
        return -1;
    }
    ms->a.keys = (PyObject **)PyMem_Malloc(multiplier * need
                                          * sizeof(PyObject *));
    if (ms->a.keys != NULL) {
        ms->alloced = need;
        if (ms->a.values != NULL)
            ms->a.values = &ms->a.keys[need];
        return 0;
    }
    PyErr_NoMemory();
    return -1;
}
#define MERGE_GETMEM(MS, NEED) ((NEED) <= (MS)->alloced ? 0 :   \
                                merge_getmem(MS, NEED))

/* Merge the na elements starting at ssa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na <= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
static Py_ssize_t
merge_lo(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, na) < 0)
        return -1;
    sortslice_memcpy(&ms->a, 0, &ssa, 0, na);
    dest = ssa;
    ssa = ms->a;

    sortslice_copy_incr(&dest, &ssb);
    --nb;
    if (nb == 0)
        goto Succeed;
    if (na == 1)
        goto CopyB;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 1 && nb > 0);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_incr(&dest, &ssb);
                ++bcount;
                acount = 0;
                --nb;
                if (nb == 0)
                    goto Succeed;
                if (bcount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_incr(&dest, &ssa);
                ++acount;
                bcount = 0;
                --na;
                if (na == 1)
                    goto CopyB;
                if (acount >= min_gallop)
                    break;
            }
        }

        /* One run is winning so consistently that galloping may
         * be a huge win.  So try that, and continue galloping until
         * (if ever) neither run appears to be winning consistently
         * anymore.
         */
        ++min_gallop;
        do {
            assert(na > 1 && nb > 0);
            min_gallop -= min_gallop > 1;
            ms->min_gallop = min_gallop;
            k = gallop_right(ms, ssb.keys[0], ssa.keys, na, 0);
            acount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memcpy(&dest, 0, &ssa, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssa, k);
                na -= k;
                if (na == 1)
                    goto CopyB;
                /* na==0 is impossible now if the comparison
                 * function is consistent, but we can't assume
                 * that it is.
                 */
                if (na == 0)
                    goto Succeed;
            }
            sortslice_copy_incr(&dest, &ssb);
            --nb;
            if (nb == 0)
                goto Succeed;

            k = gallop_left(ms, ssa.keys[0], ssb.keys, nb, 0);
            bcount = k;
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_memmove(&dest, 0, &ssb, 0, k);
                sortslice_advance(&dest, k);
                sortslice_advance(&ssb, k);
                nb -= k;
                if (nb == 0)
                    goto Succeed;
            }
            sortslice_copy_incr(&dest, &ssa);
            --na;
            if (na == 1)
                goto CopyB;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (na)
        sortslice_memcpy(&dest, 0, &ssa, 0, na);
    return result;
CopyB:
    assert(na == 1 && nb > 0);
    /* The last element of ssa belongs at the end of the merge. */
    sortslice_memmove(&dest, 0, &ssb, 0, nb);
    sortslice_copy(&dest, nb, &ssa, 0);
    return 0;
}

/* Merge the na elements starting at pa with the nb elements starting at
 * ssb.keys = ssa.keys + na in a stable way, in-place.  na and nb must be > 0.
 * Must also have that ssa.keys[na-1] belongs at the end of the merge, and
 * should have na >= nb.  See listsort.txt for more info.  Return 0 if
 * successful, -1 if error.
 */
static Py_ssize_t
merge_hi(MergeState *ms, sortslice ssa, Py_ssize_t na,
         sortslice ssb, Py_ssize_t nb)
{
    Py_ssize_t k;
    sortslice dest, basea, baseb;
    int result = -1;            /* guilty until proved innocent */
    Py_ssize_t min_gallop;

    assert(ms && ssa.keys && ssb.keys && na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);
    if (MERGE_GETMEM(ms, nb) < 0)
        return -1;
    dest = ssb;
    sortslice_advance(&dest, nb-1);
    sortslice_memcpy(&ms->a, 0, &ssb, 0, nb);
    basea = ssa;
    baseb = ms->a;
    ssb.keys = ms->a.keys + nb - 1;
    if (ssb.values != NULL)
        ssb.values = ms->a.values + nb - 1;
    sortslice_advance(&ssa, na - 1);

    sortslice_copy_decr(&dest, &ssa);
    --na;
    if (na == 0)
        goto Succeed;
    if (nb == 1)
        goto CopyA;

    min_gallop = ms->min_gallop;
    for (;;) {
        Py_ssize_t acount = 0;          /* # of times A won in a row */
        Py_ssize_t bcount = 0;          /* # of times B won in a row */

        /* Do the straightforward thing until (if ever) one run
         * appears to win consistently.
         */
        for (;;) {
            assert(na > 0 && nb > 1);
            k = ISLT(ssb.keys[0], ssa.keys[0]);
            if (k) {
                if (k < 0)
                    goto Fail;
                sortslice_copy_decr(&dest, &ssa);
                ++acount;
                bcount = 0;
                --na;
                if (na == 0)
                    goto Succeed;
                if (acount >= min_gallop)
                    break;
            }
            else {
                sortslice_copy_decr(&dest, &ssb);
                ++bcount;
                acount = 0;
                --nb;
                if (nb == 1)
                    goto CopyA;
                if (bcount >= min_gallop)
                    break;
            }
        }

        /* One run is winning so consistently that galloping may
         * be a huge win.  So try that, and continue galloping until
         * (if ever) neither run appears to be winning consistently
         * anymore.
         */
        ++min_gallop;
        do {
            assert(na > 0 && nb > 1);
            min_gallop -= min_gallop > 1;
            ms->min_gallop = min_gallop;
            k = gallop_right(ms, ssb.keys[0], basea.keys, na, na-1);
            if (k < 0)
                goto Fail;
            k = na - k;
            acount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssa, -k);
                sortslice_memmove(&dest, 1, &ssa, 1, k);
                na -= k;
                if (na == 0)
                    goto Succeed;
            }
            sortslice_copy_decr(&dest, &ssb);
            --nb;
            if (nb == 1)
                goto CopyA;

            k = gallop_left(ms, ssa.keys[0], baseb.keys, nb, nb-1);
            if (k < 0)
                goto Fail;
            k = nb - k;
            bcount = k;
            if (k) {
                sortslice_advance(&dest, -k);
                sortslice_advance(&ssb, -k);
                sortslice_memcpy(&dest, 1, &ssb, 1, k);
                nb -= k;
                if (nb == 1)
                    goto CopyA;
                /* nb==0 is impossible now if the comparison
                 * function is consistent, but we can't assume
                 * that it is.
                 */
                if (nb == 0)
                    goto Succeed;
            }
            sortslice_copy_decr(&dest, &ssa);
            --na;
            if (na == 0)
                goto Succeed;
        } while (acount >= MIN_GALLOP || bcount >= MIN_GALLOP);
        ++min_gallop;           /* penalize it for leaving galloping mode */
        ms->min_gallop = min_gallop;
    }
Succeed:
    result = 0;
Fail:
    if (nb)
        sortslice_memcpy(&dest, -(nb-1), &baseb, 0, nb);
    return result;
CopyA:
    assert(nb == 1 && na > 0);
    /* The first element of ssb belongs at the front of the merge. */
    sortslice_memmove(&dest, 1-na, &ssa, 1-na, na);
    sortslice_advance(&dest, -na);
    sortslice_advance(&ssa, -na);
    sortslice_copy(&dest, 0, &ssb, 0);
    return 0;
}

/* Merge the two runs at stack indices i and i+1.
 * Returns 0 on success, -1 on error.
 */
static Py_ssize_t
merge_at(MergeState *ms, Py_ssize_t i)
{
    sortslice ssa, ssb;
    Py_ssize_t na, nb;
    Py_ssize_t k;

    assert(ms != NULL);
    assert(ms->n >= 2);
    assert(i >= 0);
    assert(i == ms->n - 2 || i == ms->n - 3);

    ssa = ms->pending[i].base;
    na = ms->pending[i].len;
    ssb = ms->pending[i+1].base;
    nb = ms->pending[i+1].len;
    assert(na > 0 && nb > 0);
    assert(ssa.keys + na == ssb.keys);

    /* Record the length of the combined runs; if i is the 3rd-last
     * run now, also slide over the last run (which isn't involved
     * in this merge).  The current run i+1 goes away in any case.
     */
    ms->pending[i].len = na + nb;
    if (i == ms->n - 3)
        ms->pending[i+1] = ms->pending[i+2];
    --ms->n;

    /* Where does b start in a?  Elements in a before that can be
     * ignored (already in place).
     */
    k = gallop_right(ms, *ssb.keys, ssa.keys, na, 0);
    if (k < 0)
        return -1;
    sortslice_advance(&ssa, k);
    na -= k;
    if (na == 0)
        return 0;

    /* Where does a end in b?  Elements in b after that can be
     * ignored (already in place).
     */
    nb = gallop_left(ms, ssa.keys[na-1], ssb.keys, nb, nb-1);
    if (nb <= 0)
        return nb;

    /* Merge what remains of the runs, using a temp array with
     * min(na, nb) elements.
     */
    if (na <= nb)
        return merge_lo(ms, ssa, na, ssb, nb);
    else
        return merge_hi(ms, ssa, na, ssb, nb);
}

/* Two adjacent runs begin at index s1. The first run has length n1, and
 * the second run (starting at index s1+n1) has length n2. The list has total
 * length n.
 * Compute the "power" of the first run. See listsort.txt for details.
 */
static int
powerloop(Py_ssize_t s1, Py_ssize_t n1, Py_ssize_t n2, Py_ssize_t n)
{
    int result = 0;
    assert(s1 >= 0);
    assert(n1 > 0 && n2 > 0);
    assert(s1 + n1 + n2 <= n);
    /* midpoints a and b:
     * a = s1 + n1/2
     * b = s1 + n1 + n2/2 = a + (n1 + n2)/2
     *
     * Those may not be integers, though, because of the "/2". So we work with
     * 2*a and 2*b instead, which are necessarily integers. It makes no
     * difference to the outcome, since the bits in the expansion of (2*i)/n
     * are merely shifted one position from those of i/n.
     */
    Py_ssize_t a = 2 * s1 + n1;  /* 2*a */
    Py_ssize_t b = a + n1 + n2;  /* 2*b */
    /* Emulate a/n and b/n one bit a time, until bits differ. */
    for (;;) {
        ++result;
        if (a >= n) {  /* both quotient bits are 1 */
            assert(b >= a);
            a -= n;
            b -= n;
        }
        else if (b >= n) {  /* a/n bit is 0, b/n bit is 1 */
            break;
        } /* else both quotient bits are 0 */
        assert(a < b && b < n);
        a <<= 1;
        b <<= 1;
    }
    return result;
}

/* The next run has been identified, of length n2.
 * If there's already a run on the stack, apply the "powersort" merge strategy:
 * compute the topmost run's "power" (depth in a conceptual binary merge tree)
 * and merge adjacent runs on the stack with greater power. See listsort.txt
 * for more info.
 *
 * It's the caller's responsibility to push the new run on the stack when this
 * returns.
 *
 * Returns 0 on success, -1 on error.
 */
static int
found_new_run(MergeState *ms, Py_ssize_t n2)
{
    assert(ms);
    if (ms->n) {
        assert(ms->n > 0);
        struct s_slice *p = ms->pending;
        Py_ssize_t s1 = p[ms->n - 1].base.keys - ms->basekeys; /* start index */
        Py_ssize_t n1 = p[ms->n - 1].len;
        int power = powerloop(s1, n1, n2, ms->listlen);
        while (ms->n > 1 && p[ms->n - 2].power > power) {
            if (merge_at(ms, ms->n - 2) < 0)
                return -1;
        }
        assert(ms->n < 2 || p[ms->n - 2].power < power);
        p[ms->n - 1].power = power;
    }
    return 0;
}

/* Regardless of invariants, merge all runs on the stack until only one
 * remains.  This is used at the end of the mergesort.
 *
 * Returns 0 on success, -1 on error.
 */
static int
merge_force_collapse(MergeState *ms)
{
    struct s_slice *p = ms->pending;

    assert(ms);
    while (ms->n > 1) {
        Py_ssize_t n = ms->n - 2;
        if (n > 0 && p[n-1].len < p[n+1].len)
            --n;
        if (merge_at(ms, n) < 0)
            return -1;
    }
    return 0;
}

/* Compute a good value for the minimum run length; natural runs shorter
 * than this are boosted artificially via binary insertion.
 *
 * If n < MAX_MINRUN return n (it's too small to bother with fancy stuff).
 * Else if n is an exact power of 2, return MAX_MINRUN / 2.
 * Else return an int k, MAX_MINRUN / 2 <= k <= MAX_MINRUN, such that n/k is
 * close to, but strictly less than, an exact power of 2.
 *
 * See listsort.txt for more info.
 */
static Py_ssize_t
merge_compute_minrun(Py_ssize_t n)
{
    Py_ssize_t r = 0;           /* becomes 1 if any 1 bits are shifted off */

    assert(n >= 0);
    while (n >= MAX_MINRUN) {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

/* Here we define custom comparison functions to optimize for the cases one commonly
 * encounters in practice: homogeneous lists, often of one of the basic types. */

/* This struct holds the comparison function and helper functions
 * selected in the pre-sort check. */

/* These are the special case compare functions.
 * ms->key_compare will always point to one of these: */

/* Heterogeneous compare: default, always safe to fall back on. */
static int
safe_object_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    /* No assumptions necessary! */
    return PyObject_RichCompareBool(v, w, Py_LT);
}

/* Homogeneous compare: safe for any two comparable objects of the same type.
 * (ms->key_richcompare is set to ob_type->tp_richcompare in the
 *  pre-sort check.)
 */
static int
unsafe_object_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyObject *res_obj; int res;

    /* No assumptions, because we check first: */
    if (Py_TYPE(v)->tp_richcompare != ms->key_richcompare)
        return PyObject_RichCompareBool(v, w, Py_LT);

    assert(ms->key_richcompare != NULL);
    res_obj = (*(ms->key_richcompare))(v, w, Py_LT);

    if (res_obj == Py_NotImplemented) {
        Py_DECREF(res_obj);
        return PyObject_RichCompareBool(v, w, Py_LT);
    }
    if (res_obj == NULL)
        return -1;

    if (PyBool_Check(res_obj)) {
        res = (res_obj == Py_True);
    }
    else {
        res = PyObject_IsTrue(res_obj);
    }
    Py_DECREF(res_obj);

    /* Note that we can't assert
     *     res == PyObject_RichCompareBool(v, w, Py_LT);
     * because of evil compare functions like this:
     *     lambda a, b:  int(random.random() * 3) - 1)
     * (which is actually in test_sort.py) */
    return res;
}

/* Latin string compare: safe for any two latin (one byte per char) strings. */
static int
unsafe_latin_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    Py_ssize_t len;
    int res;

    /* Modified from Objects/unicodeobject.c:unicode_compare, assuming: */
    assert(Py_IS_TYPE(v, &PyUnicode_Type));
    assert(Py_IS_TYPE(w, &PyUnicode_Type));
    assert(PyUnicode_KIND(v) == PyUnicode_KIND(w));
    assert(PyUnicode_KIND(v) == PyUnicode_1BYTE_KIND);

    len = Py_MIN(PyUnicode_GET_LENGTH(v), PyUnicode_GET_LENGTH(w));
    res = memcmp(PyUnicode_DATA(v), PyUnicode_DATA(w), len);

    res = (res != 0 ?
           res < 0 :
           PyUnicode_GET_LENGTH(v) < PyUnicode_GET_LENGTH(w));

    assert(res == PyObject_RichCompareBool(v, w, Py_LT));;
    return res;
}

/* Bounded int compare: compare any two longs that fit in a single machine word. */
static int
unsafe_long_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyLongObject *vl, *wl;
    intptr_t v0, w0;
    int res;

    /* Modified from Objects/longobject.c:long_compare, assuming: */
    assert(Py_IS_TYPE(v, &PyLong_Type));
    assert(Py_IS_TYPE(w, &PyLong_Type));
    assert(_PyLong_IsCompact((PyLongObject *)v));
    assert(_PyLong_IsCompact((PyLongObject *)w));

    vl = (PyLongObject*)v;
    wl = (PyLongObject*)w;

    v0 = _PyLong_CompactValue(vl);
    w0 = _PyLong_CompactValue(wl);

    res = v0 < w0;
    assert(res == PyObject_RichCompareBool(v, w, Py_LT));
    return res;
}

/* Float compare: compare any two floats. */
static int
unsafe_float_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    int res;

    /* Modified from Objects/floatobject.c:float_richcompare, assuming: */
    assert(Py_IS_TYPE(v, &PyFloat_Type));
    assert(Py_IS_TYPE(w, &PyFloat_Type));

    res = PyFloat_AS_DOUBLE(v) < PyFloat_AS_DOUBLE(w);
    assert(res == PyObject_RichCompareBool(v, w, Py_LT));
    return res;
}

/* Tuple compare: compare *any* two tuples, using
 * ms->tuple_elem_compare to compare the first elements, which is set
 * using the same pre-sort check as we use for ms->key_compare,
 * but run on the list [x[0] for x in L]. This allows us to optimize compares
 * on two levels (as long as [x[0] for x in L] is type-homogeneous.) The idea is
 * that most tuple compares don't involve x[1:]. */
static int
unsafe_tuple_compare(PyObject *v, PyObject *w, MergeState *ms)
{
    PyTupleObject *vt, *wt;
    Py_ssize_t i, vlen, wlen;
    int k;

    /* Modified from Objects/tupleobject.c:tuplerichcompare, assuming: */
    assert(Py_IS_TYPE(v, &PyTuple_Type));
    assert(Py_IS_TYPE(w, &PyTuple_Type));
    assert(Py_SIZE(v) > 0);
    assert(Py_SIZE(w) > 0);

    vt = (PyTupleObject *)v;
    wt = (PyTupleObject *)w;

    vlen = Py_SIZE(vt);
    wlen = Py_SIZE(wt);

    for (i = 0; i < vlen && i < wlen; i++) {
        k = PyObject_RichCompareBool(vt->ob_item[i], wt->ob_item[i], Py_EQ);
        if (k < 0)
            return -1;
        if (!k)
            break;
    }

    if (i >= vlen || i >= wlen)
        return vlen < wlen;

    if (i == 0)
        return ms->tuple_elem_compare(vt->ob_item[i], wt->ob_item[i], ms);
    else
        return PyObject_RichCompareBool(vt->ob_item[i], wt->ob_item[i], Py_LT);
}

/* An adaptive, stable, natural mergesort.  See listsort.txt.
 * Returns Py_None on success, NULL on error.  Even in case of error, the
 * list will be some permutation of its input state (nothing is lost or
 * duplicated).
 */
/*[clinic input]
@critical_section
list.sort

    *
    key as keyfunc: object = None
    reverse: bool = False

Sort the list in ascending order and return None.

The sort is in-place (i.e. the list itself is modified) and stable (i.e. the
order of two equal elements is maintained).

If a key function is given, apply it once to each list item and sort them,
ascending or descending, according to their function values.

The reverse flag can be set to sort in descending order.
[clinic start generated code]*/

static PyObject *
list_sort_impl(PyListObject *self, PyObject *keyfunc, int reverse)
/*[clinic end generated code: output=57b9f9c5e23fbe42 input=667bf25d0e3a3676]*/
{
    MergeState ms;
    Py_ssize_t nremaining;
    Py_ssize_t minrun;
    sortslice lo;
    Py_ssize_t saved_ob_size, saved_allocated;
    PyObject **saved_ob_item;
    PyObject **final_ob_item;
    PyObject *result = NULL;            /* guilty until proved innocent */
    Py_ssize_t i;
    PyObject **keys;

    assert(self != NULL);
    assert(PyList_Check(self));
    if (keyfunc == Py_None)
        keyfunc = NULL;

    /* The list is temporarily made empty, so that mutations performed
     * by comparison functions can't affect the slice of memory we're
     * sorting (allowing mutations during sorting is a core-dump
     * factory, since ob_item may change).
     */
    saved_ob_size = Py_SIZE(self);
    saved_ob_item = self->ob_item;
    saved_allocated = self->allocated;
    Py_SET_SIZE(self, 0);
    FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item, NULL);
    self->allocated = -1; /* any operation will reset it to >= 0 */

    if (keyfunc == NULL) {
        keys = NULL;
        lo.keys = saved_ob_item;
        lo.values = NULL;
    }
    else {
        if (saved_ob_size < MERGESTATE_TEMP_SIZE/2)
            /* Leverage stack space we allocated but won't otherwise use */
            keys = &ms.temparray[saved_ob_size+1];
        else {
            keys = PyMem_Malloc(sizeof(PyObject *) * saved_ob_size);
            if (keys == NULL) {
                PyErr_NoMemory();
                goto keyfunc_fail;
            }
        }

        for (i = 0; i < saved_ob_size ; i++) {
            keys[i] = PyObject_CallOneArg(keyfunc, saved_ob_item[i]);
            if (keys[i] == NULL) {
                for (i=i-1 ; i>=0 ; i--)
                    Py_DECREF(keys[i]);
                if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
                    PyMem_Free(keys);
                goto keyfunc_fail;
            }
        }

        lo.keys = keys;
        lo.values = saved_ob_item;
    }


    /* The pre-sort check: here's where we decide which compare function to use.
     * How much optimization is safe? We test for homogeneity with respect to
     * several properties that are expensive to check at compare-time, and
     * set ms appropriately. */
    if (saved_ob_size > 1) {
        /* Assume the first element is representative of the whole list. */
        int keys_are_in_tuples = (Py_IS_TYPE(lo.keys[0], &PyTuple_Type) &&
                                  Py_SIZE(lo.keys[0]) > 0);

        PyTypeObject* key_type = (keys_are_in_tuples ?
                                  Py_TYPE(PyTuple_GET_ITEM(lo.keys[0], 0)) :
                                  Py_TYPE(lo.keys[0]));

        int keys_are_all_same_type = 1;
        int strings_are_latin = 1;
        int ints_are_bounded = 1;

        /* Prove that assumption by checking every key. */
        for (i=0; i < saved_ob_size; i++) {

            if (keys_are_in_tuples &&
                !(Py_IS_TYPE(lo.keys[i], &PyTuple_Type) && Py_SIZE(lo.keys[i]) != 0)) {
                keys_are_in_tuples = 0;
                keys_are_all_same_type = 0;
                break;
            }

            /* Note: for lists of tuples, key is the first element of the tuple
             * lo.keys[i], not lo.keys[i] itself! We verify type-homogeneity
             * for lists of tuples in the if-statement directly above. */
            PyObject *key = (keys_are_in_tuples ?
                             PyTuple_GET_ITEM(lo.keys[i], 0) :
                             lo.keys[i]);

            if (!Py_IS_TYPE(key, key_type)) {
                keys_are_all_same_type = 0;
                /* If keys are in tuple we must loop over the whole list to make
                   sure all items are tuples */
                if (!keys_are_in_tuples) {
                    break;
                }
            }

            if (keys_are_all_same_type) {
                if (key_type == &PyLong_Type &&
                    ints_are_bounded &&
                    !_PyLong_IsCompact((PyLongObject *)key)) {

                    ints_are_bounded = 0;
                }
                else if (key_type == &PyUnicode_Type &&
                         strings_are_latin &&
                         PyUnicode_KIND(key) != PyUnicode_1BYTE_KIND) {

                        strings_are_latin = 0;
                    }
                }
            }

        /* Choose the best compare, given what we now know about the keys. */
        if (keys_are_all_same_type) {

            if (key_type == &PyUnicode_Type && strings_are_latin) {
                ms.key_compare = unsafe_latin_compare;
            }
            else if (key_type == &PyLong_Type && ints_are_bounded) {
                ms.key_compare = unsafe_long_compare;
            }
            else if (key_type == &PyFloat_Type) {
                ms.key_compare = unsafe_float_compare;
            }
            else if ((ms.key_richcompare = key_type->tp_richcompare) != NULL) {
                ms.key_compare = unsafe_object_compare;
            }
            else {
                ms.key_compare = safe_object_compare;
            }
        }
        else {
            ms.key_compare = safe_object_compare;
        }

        if (keys_are_in_tuples) {
            /* Make sure we're not dealing with tuples of tuples
             * (remember: here, key_type refers list [key[0] for key in keys]) */
            if (key_type == &PyTuple_Type) {
                ms.tuple_elem_compare = safe_object_compare;
            }
            else {
                ms.tuple_elem_compare = ms.key_compare;
            }

            ms.key_compare = unsafe_tuple_compare;
        }
    }
    /* End of pre-sort check: ms is now set properly! */

    merge_init(&ms, saved_ob_size, keys != NULL, &lo);

    nremaining = saved_ob_size;
    if (nremaining < 2)
        goto succeed;

    /* Reverse sort stability achieved by initially reversing the list,
    applying a stable forward sort, then reversing the final result. */
    if (reverse) {
        if (keys != NULL)
            reverse_slice(&keys[0], &keys[saved_ob_size]);
        reverse_slice(&saved_ob_item[0], &saved_ob_item[saved_ob_size]);
    }

    /* March over the array once, left to right, finding natural runs,
     * and extending short natural runs to minrun elements.
     */
    minrun = merge_compute_minrun(nremaining);
    do {
        Py_ssize_t n;

        /* Identify next run. */
        n = count_run(&ms, &lo, nremaining);
        if (n < 0)
            goto fail;
        /* If short, extend to min(minrun, nremaining). */
        if (n < minrun) {
            const Py_ssize_t force = nremaining <= minrun ?
                              nremaining : minrun;
            if (binarysort(&ms, &lo, force, n) < 0)
                goto fail;
            n = force;
        }
        /* Maybe merge pending runs. */
        assert(ms.n == 0 || ms.pending[ms.n -1].base.keys +
                            ms.pending[ms.n-1].len == lo.keys);
        if (found_new_run(&ms, n) < 0)
            goto fail;
        /* Push new run on stack. */
        assert(ms.n < MAX_MERGE_PENDING);
        ms.pending[ms.n].base = lo;
        ms.pending[ms.n].len = n;
        ++ms.n;
        /* Advance to find next run. */
        sortslice_advance(&lo, n);
        nremaining -= n;
    } while (nremaining);

    if (merge_force_collapse(&ms) < 0)
        goto fail;
    assert(ms.n == 1);
    assert(keys == NULL
           ? ms.pending[0].base.keys == saved_ob_item
           : ms.pending[0].base.keys == &keys[0]);
    assert(ms.pending[0].len == saved_ob_size);
    lo = ms.pending[0].base;

succeed:
    result = Py_None;
fail:
    if (keys != NULL) {
        for (i = 0; i < saved_ob_size; i++)
            Py_DECREF(keys[i]);
        if (saved_ob_size >= MERGESTATE_TEMP_SIZE/2)
            PyMem_Free(keys);
    }

    if (self->allocated != -1 && result != NULL) {
        /* The user mucked with the list during the sort,
         * and we don't already have another error to report.
         */
        PyErr_SetString(PyExc_ValueError, "list modified during sort");
        result = NULL;
    }

    if (reverse && saved_ob_size > 1)
        reverse_slice(saved_ob_item, saved_ob_item + saved_ob_size);

    merge_freemem(&ms);

keyfunc_fail:
    final_ob_item = self->ob_item;
    i = Py_SIZE(self);
    Py_SET_SIZE(self, saved_ob_size);
    FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item, saved_ob_item);
    FT_ATOMIC_STORE_SSIZE_RELAXED(self->allocated, saved_allocated);
    if (final_ob_item != NULL) {
        /* we cannot use list_clear() for this because it does not
           guarantee that the list is really empty when it returns */
        while (--i >= 0) {
            Py_XDECREF(final_ob_item[i]);
        }
#ifdef Py_GIL_DISABLED
        ensure_shared_on_resize(self);
        bool use_qsbr = _PyObject_GC_IS_SHARED(self);
#else
        bool use_qsbr = false;
#endif
        free_list_items(final_ob_item, use_qsbr);
    }
    return Py_XNewRef(result);
}
#undef IFLT
#undef ISLT

int
PyList_Sort(PyObject *v)
{
    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(v);
    v = list_sort_impl((PyListObject *)v, NULL, 0);
    Py_END_CRITICAL_SECTION();
    if (v == NULL)
        return -1;
    Py_DECREF(v);
    return 0;
}

/*[clinic input]
@critical_section
list.reverse

Reverse *IN PLACE*.
[clinic start generated code]*/

static PyObject *
list_reverse_impl(PyListObject *self)
/*[clinic end generated code: output=482544fc451abea9 input=04ac8e0c6a66e4d9]*/
{
    if (Py_SIZE(self) > 1)
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    Py_RETURN_NONE;
}

int
PyList_Reverse(PyObject *v)
{
    PyListObject *self = (PyListObject *)v;

    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    if (Py_SIZE(self) > 1) {
        reverse_slice(self->ob_item, self->ob_item + Py_SIZE(self));
    }
    Py_END_CRITICAL_SECTION()
    return 0;
}

PyObject *
PyList_AsTuple(PyObject *v)
{
    if (v == NULL || !PyList_Check(v)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    PyObject *ret;
    PyListObject *self = (PyListObject *)v;
    Py_BEGIN_CRITICAL_SECTION(self);
    ret = _PyTuple_FromArray(self->ob_item, Py_SIZE(v));
    Py_END_CRITICAL_SECTION();
    return ret;
}

PyObject *
_PyList_AsTupleAndClear(PyListObject *self)
{
    assert(self != NULL);
    PyObject *ret;
    if (self->ob_item == NULL) {
        return PyTuple_New(0);
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    PyObject **items = self->ob_item;
    Py_ssize_t size = Py_SIZE(self);
    self->ob_item = NULL;
    Py_SET_SIZE(self, 0);
    ret = _PyTuple_FromArraySteal(items, size);
    free_list_items(items, false);
    Py_END_CRITICAL_SECTION();
    return ret;
}

PyObject *
_PyList_FromStackRefStealOnSuccess(const _PyStackRef *src, Py_ssize_t n)
{
    if (n == 0) {
        return PyList_New(0);
    }

    PyListObject *list = (PyListObject *)PyList_New(n);
    if (list == NULL) {
        return NULL;
    }

    PyObject **dst = list->ob_item;
    for (Py_ssize_t i = 0; i < n; i++) {
        dst[i] = PyStackRef_AsPyObjectSteal(src[i]);
    }

    return (PyObject *)list;
}

/*[clinic input]
list.index

    value: object
    start: slice_index(accept={int}) = 0
    stop: slice_index(accept={int}, c_default="PY_SSIZE_T_MAX") = sys.maxsize
    /

Return first index of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
list_index_impl(PyListObject *self, PyObject *value, Py_ssize_t start,
                Py_ssize_t stop)
/*[clinic end generated code: output=ec51b88787e4e481 input=40ec5826303a0eb1]*/
{
    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0)
            start = 0;
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
        if (stop < 0)
            stop = 0;
    }
    for (Py_ssize_t i = start; i < stop; i++) {
        PyObject *obj = list_get_item_ref(self, i);
        if (obj == NULL) {
            // out-of-bounds
            break;
        }
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0)
            return PyLong_FromSsize_t(i);
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "list.index(x): x not in list");
    return NULL;
}

/*[clinic input]
list.count

     value: object
     /

Return number of occurrences of value.
[clinic start generated code]*/

static PyObject *
list_count_impl(PyListObject *self, PyObject *value)
/*[clinic end generated code: output=eff66f14aef2df86 input=3bdc3a5e6f749565]*/
{
    Py_ssize_t count = 0;
    for (Py_ssize_t i = 0; ; i++) {
        PyObject *obj = list_get_item_ref(self, i);
        if (obj == NULL) {
            // out-of-bounds
            break;
        }
        if (obj == value) {
           count++;
           Py_DECREF(obj);
           continue;
        }
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}

/*[clinic input]
@critical_section
list.remove

     value: object
     /

Remove first occurrence of value.

Raises ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
list_remove_impl(PyListObject *self, PyObject *value)
/*[clinic end generated code: output=b9b76a6633b18778 input=26c813dbb95aa93b]*/
{
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *obj = self->ob_item[i];
        Py_INCREF(obj);
        int cmp = PyObject_RichCompareBool(obj, value, Py_EQ);
        Py_DECREF(obj);
        if (cmp > 0) {
            if (list_ass_slice_lock_held(self, i, i+1, NULL) == 0)
                Py_RETURN_NONE;
            return NULL;
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "list.remove(x): x not in list");
    return NULL;
}

static int
list_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyListObject *o = (PyListObject *)self;
    Py_ssize_t i;

    for (i = Py_SIZE(o); --i >= 0; )
        Py_VISIT(o->ob_item[i]);
    return 0;
}

static PyObject *
list_richcompare_impl(PyObject *v, PyObject *w, int op)
{
    PyListObject *vl, *wl;
    Py_ssize_t i;

    if (!PyList_Check(v) || !PyList_Check(w))
        Py_RETURN_NOTIMPLEMENTED;

    vl = (PyListObject *)v;
    wl = (PyListObject *)w;

    if (Py_SIZE(vl) != Py_SIZE(wl) && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the lists differ */
        if (op == Py_EQ)
            Py_RETURN_FALSE;
        else
            Py_RETURN_TRUE;
    }

    /* Search for the first index where items are different */
    for (i = 0; i < Py_SIZE(vl) && i < Py_SIZE(wl); i++) {
        PyObject *vitem = vl->ob_item[i];
        PyObject *witem = wl->ob_item[i];
        if (vitem == witem) {
            continue;
        }

        Py_INCREF(vitem);
        Py_INCREF(witem);
        int k = PyObject_RichCompareBool(vitem, witem, Py_EQ);
        Py_DECREF(vitem);
        Py_DECREF(witem);
        if (k < 0)
            return NULL;
        if (!k)
            break;
    }

    if (i >= Py_SIZE(vl) || i >= Py_SIZE(wl)) {
        /* No more items to compare -- compare sizes */
        Py_RETURN_RICHCOMPARE(Py_SIZE(vl), Py_SIZE(wl), op);
    }

    /* We have an item that differs -- shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_RETURN_FALSE;
    }
    if (op == Py_NE) {
        Py_RETURN_TRUE;
    }

    /* Compare the final item again using the proper operator */
    PyObject *vitem = vl->ob_item[i];
    PyObject *witem = wl->ob_item[i];
    Py_INCREF(vitem);
    Py_INCREF(witem);
    PyObject *result = PyObject_RichCompare(vl->ob_item[i], wl->ob_item[i], op);
    Py_DECREF(vitem);
    Py_DECREF(witem);
    return result;
}

static PyObject *
list_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION2(v, w);
    ret = list_richcompare_impl(v, w, op);
    Py_END_CRITICAL_SECTION2()
    return ret;
}

/*[clinic input]
list.__init__

    iterable: object(c_default="NULL") = ()
    /

Built-in mutable sequence.

If no argument is given, the constructor creates a new empty list.
The argument must be an iterable if specified.
[clinic start generated code]*/

static int
list___init___impl(PyListObject *self, PyObject *iterable)
/*[clinic end generated code: output=0f3c21379d01de48 input=b3f3fe7206af8f6b]*/
{
    /* Verify list invariants established by PyType_GenericAlloc() */
    assert(0 <= Py_SIZE(self));
    assert(Py_SIZE(self) <= self->allocated || self->allocated == -1);
    assert(self->ob_item != NULL ||
           self->allocated == 0 || self->allocated == -1);

    /* Empty previous contents */
    if (self->ob_item != NULL) {
        Py_BEGIN_CRITICAL_SECTION(self);
        list_clear(self);
        Py_END_CRITICAL_SECTION();
    }
    if (iterable != NULL) {
        if (_list_extend(self, iterable) < 0) {
            return -1;
        }
    }
    return 0;
}

static PyObject *
list_vectorcall(PyObject *type, PyObject * const*args,
                size_t nargsf, PyObject *kwnames)
{
    if (!_PyArg_NoKwnames("list", kwnames)) {
        return NULL;
    }
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("list", nargs, 0, 1)) {
        return NULL;
    }

    PyObject *list = PyType_GenericAlloc(_PyType_CAST(type), 0);
    if (list == NULL) {
        return NULL;
    }
    if (nargs) {
        if (list___init___impl((PyListObject *)list, args[0])) {
            Py_DECREF(list);
            return NULL;
        }
    }
    return list;
}


/*[clinic input]
list.__sizeof__

Return the size of the list in memory, in bytes.
[clinic start generated code]*/

static PyObject *
list___sizeof___impl(PyListObject *self)
/*[clinic end generated code: output=3417541f95f9a53e input=b8030a5d5ce8a187]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    Py_ssize_t allocated = FT_ATOMIC_LOAD_SSIZE_RELAXED(self->allocated);
    res += (size_t)allocated * sizeof(void*);
    return PyLong_FromSize_t(res);
}

static PyObject *list_iter(PyObject *seq);
static PyObject *list_subscript(PyObject*, PyObject*);

static PyMethodDef list_methods[] = {
    {"__getitem__", list_subscript, METH_O|METH_COEXIST,
     PyDoc_STR("__getitem__($self, index, /)\n--\n\nReturn self[index].")},
    LIST___REVERSED___METHODDEF
    LIST___SIZEOF___METHODDEF
    PY_LIST_CLEAR_METHODDEF
    LIST_COPY_METHODDEF
    LIST_APPEND_METHODDEF
    LIST_INSERT_METHODDEF
    LIST_EXTEND_METHODDEF
    LIST_POP_METHODDEF
    LIST_REMOVE_METHODDEF
    LIST_INDEX_METHODDEF
    LIST_COUNT_METHODDEF
    LIST_REVERSE_METHODDEF
    LIST_SORT_METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS, PyDoc_STR("See PEP 585")},
    {NULL,              NULL}           /* sentinel */
};

static PySequenceMethods list_as_sequence = {
    list_length,                                /* sq_length */
    list_concat,                                /* sq_concat */
    list_repeat,                                /* sq_repeat */
    list_item,                                  /* sq_item */
    0,                                          /* sq_slice */
    list_ass_item,                              /* sq_ass_item */
    0,                                          /* sq_ass_slice */
    list_contains,                              /* sq_contains */
    list_inplace_concat,                        /* sq_inplace_concat */
    list_inplace_repeat,                        /* sq_inplace_repeat */
};

static inline PyObject *
list_slice_step_lock_held(PyListObject *a, Py_ssize_t start, Py_ssize_t step, Py_ssize_t len)
{
    PyListObject *np = (PyListObject *)list_new_prealloc(len);
    if (np == NULL) {
        return NULL;
    }
    size_t cur;
    Py_ssize_t i;
    PyObject **src = a->ob_item;
    PyObject **dest = np->ob_item;
    for (cur = start, i = 0; i < len;
            cur += (size_t)step, i++) {
        PyObject *v = src[cur];
        dest[i] = Py_NewRef(v);
    }
    Py_SET_SIZE(np, len);
    return (PyObject *)np;
}

static PyObject *
list_slice_wrap(PyListObject *aa, Py_ssize_t start, Py_ssize_t stop, Py_ssize_t step)
{
    PyObject *res = NULL;
    Py_BEGIN_CRITICAL_SECTION(aa);
    Py_ssize_t len = PySlice_AdjustIndices(Py_SIZE(aa), &start, &stop, step);
    if (len <= 0) {
        res = PyList_New(0);
    }
    else if (step == 1) {
        res = list_slice_lock_held(aa, start, stop);
    }
    else {
        res = list_slice_step_lock_held(aa, start, step, len);
    }
    Py_END_CRITICAL_SECTION();
    return res;
}

static inline PyObject*
list_slice_subscript(PyObject* self, PyObject* item)
{
    assert(PyList_Check(self));
    assert(PySlice_Check(item));
    Py_ssize_t start, stop, step;
    if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
        return NULL;
    }
    return list_slice_wrap((PyListObject *)self, start, stop, step);
}

PyObject *
_PyList_SliceSubscript(PyObject* _self, PyObject* item)
{
    return list_slice_subscript(_self, item);
}

static PyObject *
list_subscript(PyObject* _self, PyObject* item)
{
    PyListObject* self = (PyListObject*)_self;
    if (_PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_item((PyObject *)self, i);
    }
    else if (PySlice_Check(item)) {
        return list_slice_subscript(_self, item);
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     Py_TYPE(item)->tp_name);
        return NULL;
    }
}

static Py_ssize_t
adjust_slice_indexes(PyListObject *lst,
                     Py_ssize_t *start, Py_ssize_t *stop,
                     Py_ssize_t step)
{
    Py_ssize_t slicelength = PySlice_AdjustIndices(Py_SIZE(lst), start, stop,
                                                   step);

    /* Make sure s[5:2] = [..] inserts at the right place:
        before 5, not before 2. */
    if ((step < 0 && *start < *stop) ||
        (step > 0 && *start > *stop))
        *stop = *start;

    return slicelength;
}

static int
list_ass_subscript_lock_held(PyObject *_self, PyObject *item, PyObject *value)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(_self);

    PyListObject *self = (PyListObject *)_self;
    if (_PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += PyList_GET_SIZE(self);
        return list_ass_item_lock_held(self, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }

        if (value == NULL) {
            /* delete slice */
            PyObject **garbage;
            size_t cur;
            Py_ssize_t i;
            int res;

            Py_ssize_t slicelength = adjust_slice_indexes(self, &start, &stop,
                                                          step);

            if (step == 1)
                return list_ass_slice_lock_held(self, start, stop, value);

            if (slicelength <= 0)
                return 0;

            if (step < 0) {
                stop = start + 1;
                start = stop + step*(slicelength - 1) - 1;
                step = -step;
            }

            garbage = (PyObject**)
                PyMem_Malloc(slicelength*sizeof(PyObject*));
            if (!garbage) {
                PyErr_NoMemory();
                return -1;
            }

            /* drawing pictures might help understand these for
               loops. Basically, we memmove the parts of the
               list that are *not* part of the slice: step-1
               items for each item that is part of the slice,
               and then tail end of the list that was not
               covered by the slice */
            for (cur = start, i = 0;
                 cur < (size_t)stop;
                 cur += step, i++) {
                Py_ssize_t lim = step - 1;

                garbage[i] = PyList_GET_ITEM(self, cur);

                if (cur + step >= (size_t)Py_SIZE(self)) {
                    lim = Py_SIZE(self) - cur - 1;
                }

                memmove(self->ob_item + cur - i,
                    self->ob_item + cur + 1,
                    lim * sizeof(PyObject *));
            }
            cur = start + (size_t)slicelength * step;
            if (cur < (size_t)Py_SIZE(self)) {
                memmove(self->ob_item + cur - slicelength,
                    self->ob_item + cur,
                    (Py_SIZE(self) - cur) *
                     sizeof(PyObject *));
            }

            Py_SET_SIZE(self, Py_SIZE(self) - slicelength);
            res = list_resize(self, Py_SIZE(self));

            for (i = 0; i < slicelength; i++) {
                Py_DECREF(garbage[i]);
            }
            PyMem_Free(garbage);

            return res;
        }
        else {
            /* assign slice */
            PyObject *ins, *seq;
            PyObject **garbage, **seqitems, **selfitems;
            Py_ssize_t i;
            size_t cur;

            /* protect against a[::-1] = a */
            if (self == (PyListObject*)value) {
                seq = list_slice_lock_held((PyListObject *)value, 0,
                                            Py_SIZE(value));
            }
            else {
                seq = PySequence_Fast(value,
                                      "must assign iterable "
                                      "to extended slice");
            }
            if (!seq)
                return -1;

            Py_ssize_t slicelength = adjust_slice_indexes(self, &start, &stop,
                                                          step);

            if (step == 1) {
                int res = list_ass_slice_lock_held(self, start, stop, seq);
                Py_DECREF(seq);
                return res;
            }

            if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
                PyErr_Format(PyExc_ValueError,
                    "attempt to assign sequence of "
                    "size %zd to extended slice of "
                    "size %zd",
                         PySequence_Fast_GET_SIZE(seq),
                         slicelength);
                Py_DECREF(seq);
                return -1;
            }

            if (!slicelength) {
                Py_DECREF(seq);
                return 0;
            }

            garbage = (PyObject**)
                PyMem_Malloc(slicelength*sizeof(PyObject*));
            if (!garbage) {
                Py_DECREF(seq);
                PyErr_NoMemory();
                return -1;
            }

            selfitems = self->ob_item;
            seqitems = PySequence_Fast_ITEMS(seq);
            for (cur = start, i = 0; i < slicelength;
                 cur += (size_t)step, i++) {
                garbage[i] = selfitems[cur];
                ins = Py_NewRef(seqitems[i]);
                selfitems[cur] = ins;
            }

            for (i = 0; i < slicelength; i++) {
                Py_DECREF(garbage[i]);
            }

            PyMem_Free(garbage);
            Py_DECREF(seq);

            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "list indices must be integers or slices, not %.200s",
                     Py_TYPE(item)->tp_name);
        return -1;
    }
}

static int
list_ass_subscript(PyObject *self, PyObject *item, PyObject *value)
{
    int res;
#ifdef Py_GIL_DISABLED
    if (PySlice_Check(item) && value != NULL && PyList_CheckExact(value)) {
        Py_BEGIN_CRITICAL_SECTION2(self, value);
        res = list_ass_subscript_lock_held(self, item, value);
        Py_END_CRITICAL_SECTION2();
        return res;
    }
#endif
    Py_BEGIN_CRITICAL_SECTION(self);
    res = list_ass_subscript_lock_held(self, item, value);
    Py_END_CRITICAL_SECTION();
    return res;
}

static PyMappingMethods list_as_mapping = {
    list_length,
    list_subscript,
    list_ass_subscript
};

PyTypeObject PyList_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list",
    sizeof(PyListObject),
    0,
    list_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    list_repr,                                  /* tp_repr */
    0,                                          /* tp_as_number */
    &list_as_sequence,                          /* tp_as_sequence */
    &list_as_mapping,                           /* tp_as_mapping */
    PyObject_HashNotImplemented,                /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_LIST_SUBCLASS |
        _Py_TPFLAGS_MATCH_SELF | Py_TPFLAGS_SEQUENCE,  /* tp_flags */
    list___init____doc__,                       /* tp_doc */
    list_traverse,                              /* tp_traverse */
    list_clear_slot,                            /* tp_clear */
    list_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    list_iter,                                  /* tp_iter */
    0,                                          /* tp_iternext */
    list_methods,                               /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    list___init__,                              /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    .tp_vectorcall = list_vectorcall,
    .tp_version_tag = _Py_TYPE_VERSION_LIST,
};

/*********************** List Iterator **************************/

static void listiter_dealloc(PyObject *);
static int listiter_traverse(PyObject *, visitproc, void *);
static PyObject *listiter_next(PyObject *);
static PyObject *listiter_len(PyObject *, PyObject *);
static PyObject *listiter_reduce_general(void *_it, int forward);
static PyObject *listiter_reduce(PyObject *, PyObject *);
static PyObject *listiter_setstate(PyObject *, PyObject *state);

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");
PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");
PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef listiter_methods[] = {
    {"__length_hint__", listiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", listiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", listiter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_iterator",                            /* tp_name */
    sizeof(_PyListIterObject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    listiter_dealloc,               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    listiter_traverse,                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    listiter_next,                              /* tp_iternext */
    listiter_methods,                           /* tp_methods */
    0,                                          /* tp_members */
};


static PyObject *
list_iter(PyObject *seq)
{
    if (!PyList_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    _PyListIterObject *it = _Py_FREELIST_POP(_PyListIterObject, list_iters);
    if (it == NULL) {
        it = PyObject_GC_New(_PyListIterObject, &PyListIter_Type);
        if (it == NULL) {
            return NULL;
        }
    }
    it->it_index = 0;
    it->it_seq = (PyListObject *)Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

static void
listiter_dealloc(PyObject *self)
{
    _PyListIterObject *it = (_PyListIterObject *)self;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    assert(Py_IS_TYPE(self, &PyListIter_Type));
    _Py_FREELIST_FREE(list_iters, it, PyObject_GC_Del);
}

static int
listiter_traverse(PyObject *it, visitproc visit, void *arg)
{
    Py_VISIT(((_PyListIterObject *)it)->it_seq);
    return 0;
}

static PyObject *
listiter_next(PyObject *self)
{
    _PyListIterObject *it = (_PyListIterObject *)self;
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index < 0) {
        return NULL;
    }

    PyObject *item = list_get_item_ref(it->it_seq, index);
    if (item == NULL) {
        // out-of-bounds
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, -1);
#ifndef Py_GIL_DISABLED
        PyListObject *seq = it->it_seq;
        it->it_seq = NULL;
        Py_DECREF(seq);
#endif
        return NULL;
    }
    FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index + 1);
    return item;
}

static PyObject *
listiter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(self != NULL);
    _PyListIterObject *it = (_PyListIterObject *)self;
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index >= 0) {
        Py_ssize_t len = PyList_GET_SIZE(it->it_seq) - index;
        if (len >= 0)
            return PyLong_FromSsize_t(len);
    }
    return PyLong_FromLong(0);
}

static PyObject *
listiter_reduce(PyObject *it, PyObject *Py_UNUSED(ignored))
{
    return listiter_reduce_general(it, 1);
}

static PyObject *
listiter_setstate(PyObject *self, PyObject *state)
{
    _PyListIterObject *it = (_PyListIterObject *)self;
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < -1)
            index = -1;
        else if (index > PyList_GET_SIZE(it->it_seq))
            index = PyList_GET_SIZE(it->it_seq); /* iterator exhausted */
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index);
    }
    Py_RETURN_NONE;
}

/*********************** List Reverse Iterator **************************/

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyListObject *it_seq; /* Set to NULL when iterator is exhausted */
} listreviterobject;

static void listreviter_dealloc(PyObject *);
static int listreviter_traverse(PyObject *, visitproc, void *);
static PyObject *listreviter_next(PyObject *);
static PyObject *listreviter_len(PyObject *, PyObject *);
static PyObject *listreviter_reduce(PyObject *, PyObject *);
static PyObject *listreviter_setstate(PyObject *, PyObject *);

static PyMethodDef listreviter_methods[] = {
    {"__length_hint__", listreviter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__", listreviter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__", listreviter_setstate, METH_O, setstate_doc},
    {NULL,              NULL}           /* sentinel */
};

PyTypeObject PyListRevIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "list_reverseiterator",                     /* tp_name */
    sizeof(listreviterobject),                  /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    listreviter_dealloc,                        /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                                          /* tp_doc */
    listreviter_traverse,                       /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    PyObject_SelfIter,                          /* tp_iter */
    listreviter_next,                           /* tp_iternext */
    listreviter_methods,                /* tp_methods */
    0,
};

/*[clinic input]
list.__reversed__

Return a reverse iterator over the list.
[clinic start generated code]*/

static PyObject *
list___reversed___impl(PyListObject *self)
/*[clinic end generated code: output=b166f073208c888c input=eadb6e17f8a6a280]*/
{
    listreviterobject *it;

    it = PyObject_GC_New(listreviterobject, &PyListRevIter_Type);
    if (it == NULL)
        return NULL;
    assert(PyList_Check(self));
    it->it_index = PyList_GET_SIZE(self) - 1;
    it->it_seq = (PyListObject*)Py_NewRef(self);
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static void
listreviter_dealloc(PyObject *self)
{
    listreviterobject *it = (listreviterobject *)self;
    PyObject_GC_UnTrack(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
listreviter_traverse(PyObject *it, visitproc visit, void *arg)
{
    Py_VISIT(((listreviterobject *)it)->it_seq);
    return 0;
}

static PyObject *
listreviter_next(PyObject *self)
{
    listreviterobject *it = (listreviterobject *)self;
    assert(it != NULL);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    if (index < 0) {
        return NULL;
    }

    PyListObject *seq = it->it_seq;
    assert(PyList_Check(seq));
    PyObject *item = list_get_item_ref(seq, index);
    if (item != NULL) {
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index - 1);
        return item;
    }
    FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, -1);
#ifndef Py_GIL_DISABLED
    it->it_seq = NULL;
    Py_DECREF(seq);
#endif
    return NULL;
}

static PyObject *
listreviter_len(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    listreviterobject *it = (listreviterobject *)self;
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
    Py_ssize_t len = index + 1;
    if (it->it_seq == NULL || PyList_GET_SIZE(it->it_seq) < len)
        len = 0;
    return PyLong_FromSsize_t(len);
}

static PyObject *
listreviter_reduce(PyObject *it, PyObject *Py_UNUSED(ignored))
{
    return listiter_reduce_general(it, 0);
}

static PyObject *
listreviter_setstate(PyObject *self, PyObject *state)
{
    listreviterobject *it = (listreviterobject *)self;
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < -1)
            index = -1;
        else if (index > PyList_GET_SIZE(it->it_seq) - 1)
            index = PyList_GET_SIZE(it->it_seq) - 1;
        FT_ATOMIC_STORE_SSIZE_RELAXED(it->it_index, index);
    }
    Py_RETURN_NONE;
}

/* common pickling support */

static PyObject *
listiter_reduce_general(void *_it, int forward)
{
    PyObject *list;
    PyObject *iter;

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    if (forward) {
        iter = _PyEval_GetBuiltin(&_Py_ID(iter));
        _PyListIterObject *it = (_PyListIterObject *)_it;
        Py_ssize_t idx = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
        if (idx >= 0) {
            return Py_BuildValue("N(O)n", iter, it->it_seq, idx);
        }
    } else {
        iter = _PyEval_GetBuiltin(&_Py_ID(reversed));
        listreviterobject *it = (listreviterobject *)_it;
        Py_ssize_t idx = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->it_index);
        if (idx >= 0) {
            return Py_BuildValue("N(O)n", iter, it->it_seq, idx);
        }
    }
    /* empty iterator, create an empty list */
    list = PyList_New(0);
    if (list == NULL)
        return NULL;
    return Py_BuildValue("N(N)", iter, list);
}
