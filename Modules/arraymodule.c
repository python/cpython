/* Array object implementation */

/* An array is a uniform list -- all items have the same type.
   The item type is restricted to simple C types like int or float */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_bytesobject.h"   // _PyBytes_RepeatBuffer
#include "pycore_call.h"          // _PyObject_CallMethod()
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_floatobject.h"   // _PY_FLOAT_BIG_ENDIAN
#include "pycore_gc.h"            // _PyObject_GC_IS_SHARED()
#include "pycore_modsupport.h"    // _PyArg_NoKeywords()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyObject_GC_TRACK()
#include "pycore_pyatomic_ft_wrappers.h"
#include "pycore_pymem.h"         // _PyMem_FreeDelayed()
#include "pycore_tuple.h"         // _PyTuple_FromPairSteal
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include <stddef.h>               // offsetof()
#include <stdbool.h>

/*[clinic input]
module array
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7d1b8d7f5958fd83]*/

struct arrayobject; /* Forward */
static struct PyModuleDef arraymodule;

/* All possible arraydescr values are defined in the vector "descriptors"
 * below.  That's defined later because the appropriate get and set
 * functions aren't visible yet.
 */
struct arraydescr {
    char typecode[3];  // big enough to store "Zd\0"
    int itemsize;
    PyObject * (*getitem)(const char *, Py_ssize_t);
    int (*pack)(PyObject *, char *);
    int (*compareitems)(const void *, const void *, Py_ssize_t);
    int is_integer_type;
    int is_signed;
};

typedef struct arrayobject {
    PyObject_VAR_HEAD
    char *ob_item;
    Py_ssize_t allocated;
    const struct arraydescr *ob_descr;
    PyObject *weakreflist; /* List of weak references */
    Py_ssize_t ob_exports;  /* Number of exported buffers */
} arrayobject;

#ifdef Py_GIL_DISABLED
// As in listobject.c, the capacity is also stored in front of the items.  A
// thread that reads without the lock checks its index against the capacity
// of the buffer that it loaded.  The items keep the alignment that
// PyMem_Malloc() gives them in the default build.
#if SIZEOF_VOID_P > 4
#  define ARRAY_ITEMS_ALIGNMENT 16
#else
#  define ARRAY_ITEMS_ALIGNMENT 8
#endif

typedef struct {
    Py_ssize_t allocated;
    _Py_ALIGNED_DEF(ARRAY_ITEMS_ALIGNMENT, char) ob_item[];
} _PyArrayArray;

#define ARRAY_HEADER_SIZE sizeof(_PyArrayArray)
#else
#define ARRAY_HEADER_SIZE 0
#endif

typedef struct {
    PyObject_HEAD
    Py_ssize_t index;
    arrayobject *ao;
} arrayiterobject;

typedef struct {
    PyTypeObject *ArrayType;
    PyTypeObject *ArrayIterType;

    PyObject *array_reconstructor;

    PyObject *str_read;
    PyObject *str_write;
    PyObject *str___dict__;
    PyObject *str_iter;
    PyObject *typecodes;
} array_state;

static inline Py_ssize_t
array_get_size(PyObject *op)
{
    return FT_ATOMIC_LOAD_SSIZE_ACQUIRE(_PyVarObject_CAST(op)->ob_size);
}
#define array_get_size(op) array_get_size(_PyObject_CAST(op))

/* Forward declaration. */
static PyObject *array_array_frombytes(PyObject *self, PyObject *bytes);

static array_state *
get_array_state(PyObject *module)
{
    return (array_state *)_PyModule_GetState(module);
}

#define find_array_state_by_type(tp) \
    (get_array_state(PyType_GetModuleByDef(tp, &arraymodule)))
#define get_array_state_by_class(cls) \
    (get_array_state(PyType_GetModule(cls)))

#define arrayobject_CAST(op)        ((arrayobject *)(op))
#define arrayiterobject_CAST(op)    ((arrayiterobject *)(op))

enum machine_format_code {
    UNKNOWN_FORMAT = -1,
    /* UNKNOWN_FORMAT is used to indicate that the machine format for an
     * array type code cannot be interpreted. When this occurs, a list of
     * Python objects is used to represent the content of the array
     * instead of using the memory content of the array directly. In that
     * case, the array_reconstructor mechanism is bypassed completely, and
     * the standard array constructor is used instead.
     */

    UNSIGNED_INT8 = 0,
    SIGNED_INT8 = 1,
    UNSIGNED_INT16_LE = 2,
    UNSIGNED_INT16_BE = 3,
    SIGNED_INT16_LE = 4,
    SIGNED_INT16_BE = 5,
    UNSIGNED_INT32_LE = 6,
    UNSIGNED_INT32_BE = 7,
    SIGNED_INT32_LE = 8,
    SIGNED_INT32_BE = 9,
    UNSIGNED_INT64_LE = 10,
    UNSIGNED_INT64_BE = 11,
    SIGNED_INT64_LE = 12,
    SIGNED_INT64_BE = 13,
    IEEE_754_FLOAT_LE = 14,
    IEEE_754_FLOAT_BE = 15,
    IEEE_754_DOUBLE_LE = 16,
    IEEE_754_DOUBLE_BE = 17,
    UTF16_LE = 18,
    UTF16_BE = 19,
    UTF32_LE = 20,
    UTF32_BE = 21,
    IEEE_754_FLOAT_COMPLEX_LE = 22,
    IEEE_754_FLOAT_COMPLEX_BE = 23,
    IEEE_754_DOUBLE_COMPLEX_LE = 24,
    IEEE_754_DOUBLE_COMPLEX_BE = 25,
    IEEE_754_FLOAT16_LE = 26,
    IEEE_754_FLOAT16_BE = 27
};
#define MACHINE_FORMAT_CODE_MIN 0
#define MACHINE_FORMAT_CODE_MAX 27


/*
 * Must come after arrayobject, arrayiterobject,
 * and enum machine_code_type definitions.
 */
#include "clinic/arraymodule.c.h"

#define array_Check(op, state) PyObject_TypeCheck(op, state->ArrayType)

// True if a buffer for `size` items can be allocated.
static inline bool
array_size_valid(size_t size, int itemsize)
{
    return size <= ((size_t)PY_SSIZE_T_MAX - ARRAY_HEADER_SIZE) / itemsize;
}

// Allocate an uninitialized buffer for `capacity` items.
static char *
array_items_alloc(Py_ssize_t capacity, int itemsize)
{
    assert(capacity > 0);
    assert(array_size_valid(capacity, itemsize));
#ifdef Py_GIL_DISABLED
    _PyArrayArray *array = PyMem_Malloc(
        sizeof(_PyArrayArray) + (size_t)capacity * itemsize);
    if (array == NULL) {
        return NULL;
    }
    array->allocated = capacity;
    return array->ob_item;
#else
    return PyMem_Malloc((size_t)capacity * itemsize);
#endif
}

static void
array_items_free(char *items, int itemsize, bool use_qsbr)
{
#ifdef Py_GIL_DISABLED
    if (items == NULL) {
        return;
    }
    _PyArrayArray *array = _Py_CONTAINER_OF(items, _PyArrayArray, ob_item);
    if (use_qsbr) {
        size_t size = (sizeof(_PyArrayArray)
                       + (size_t)array->allocated * itemsize);
        _PyMem_FreeDelayed(array, size);
    }
    else {
        PyMem_Free(array);
    }
#else
    PyMem_Free(items);
#endif
}

#ifdef Py_GIL_DISABLED
static inline Py_ssize_t
array_capacity(char *items)
{
    _PyArrayArray *array = _Py_CONTAINER_OF(items, _PyArrayArray, ob_item);
    return array->allocated;
}
#endif

// Memory ordering in the free-threaded build.
//
// Single items are read without the lock (getarrayitem_threadsafe(), the
// iterator) and, for arrays that are shared or owned by the current
// thread, also written without it (setarrayitem_threadsafe()).  So:
//
// - `ob_item` is published with a release store and read with an acquire
//   load.  Old buffers of a shared array are freed with QSBR.
// - `ob_size` uses release stores (array_set_size()) and acquire loads
//   (array_get_size()).  Items are written before a larger size is
//   published, so a reader that sees the new size also sees the items.
// - At any time the first `ob_size` items of the current buffer are
//   initialized.  A lock-free access loads `ob_item`, then `ob_size`, and
//   then checks that `ob_item` is unchanged, so its size belongs to its
//   buffer.  QSBR keeps the address of the old buffer from being reused in
//   between.
// - Each item is loaded with acquire and stored with release.  Items of
//   16 bytes ('Zd') are accessed as two 8-byte halves and can tear.
// - Bulk copies use per-item atomic accesses unless the array is owned
//   by the current thread and not shared (array_is_exclusive()).
// - `allocated` is only accessed with the lock held.
//
// A lock-free item store that races with a resize can be lost: it may land
// in the old buffer after it was copied.  This is accepted for speed.

// Load or store one item of the given size.  The switch gives the compiler
// a constant size for each case, so no call to memcpy() is made.

static inline void
array_load_raw(const char *src, void *dst, int size)
{
    switch (size) {
#ifdef Py_GIL_DISABLED
    // There are no 8- and 16-bit acquire loads; a relaxed load followed
    // by an acquire fence gives the same ordering.
    case 1: {
        uint8_t v = _Py_atomic_load_uint8_relaxed((const uint8_t *)src);
        _Py_atomic_fence_acquire();
        memcpy(dst, &v, 1);
        return;
    }
    case 2: {
        uint16_t v = _Py_atomic_load_uint16_relaxed((const uint16_t *)src);
        _Py_atomic_fence_acquire();
        memcpy(dst, &v, 2);
        return;
    }
    case 4: {
        uint32_t v = _Py_atomic_load_uint32_acquire((const uint32_t *)src);
        memcpy(dst, &v, 4);
        return;
    }
    case 8: {
        uint64_t v = _Py_atomic_load_uint64_acquire((const uint64_t *)src);
        memcpy(dst, &v, 8);
        return;
    }
    case 16: {
        uint64_t v[2];
        v[0] = _Py_atomic_load_uint64_acquire((const uint64_t *)src);
        v[1] = _Py_atomic_load_uint64_acquire((const uint64_t *)src + 1);
        memcpy(dst, v, 16);
        return;
    }
#else
    case 1: memcpy(dst, src, 1); return;
    case 2: memcpy(dst, src, 2); return;
    case 4: memcpy(dst, src, 4); return;
    case 8: memcpy(dst, src, 8); return;
    case 16: memcpy(dst, src, 16); return;
#endif
    }
    Py_UNREACHABLE();
}

static inline void
array_store_raw(char *dst, const void *src, int size)
{
    switch (size) {
#ifdef Py_GIL_DISABLED
    case 1: {
        uint8_t v;
        memcpy(&v, src, 1);
        _Py_atomic_store_uint8_release((uint8_t *)dst, v);
        return;
    }
    case 2: {
        // No 16-bit release store; use a release fence and a relaxed store.
        uint16_t v;
        memcpy(&v, src, 2);
        _Py_atomic_fence_release();
        _Py_atomic_store_uint16_relaxed((uint16_t *)dst, v);
        return;
    }
    case 4: {
        uint32_t v;
        memcpy(&v, src, 4);
        _Py_atomic_store_uint32_release((uint32_t *)dst, v);
        return;
    }
    case 8: {
        uint64_t v;
        memcpy(&v, src, 8);
        _Py_atomic_store_uint64_release((uint64_t *)dst, v);
        return;
    }
    case 16: {
        uint64_t v[2];
        memcpy(v, src, 16);
        _Py_atomic_store_uint64_release((uint64_t *)dst, v[0]);
        _Py_atomic_store_uint64_release((uint64_t *)dst + 1, v[1]);
        return;
    }
#else
    case 1: memcpy(dst, src, 1); return;
    case 2: memcpy(dst, src, 2); return;
    case 4: memcpy(dst, src, 4); return;
    case 8: memcpy(dst, src, 8); return;
    case 16: memcpy(dst, src, 16); return;
#endif
    }
    Py_UNREACHABLE();
}

// True if no other thread can access the items without the lock.
static inline bool
array_is_exclusive(arrayobject *a)
{
#ifdef Py_GIL_DISABLED
    return (_Py_IsOwnedByCurrentThread((PyObject *)a) &&
            !_PyObject_GC_IS_SHARED(a));
#else
    return true;
#endif
}

// Copy n items of the given size; dst and src may overlap.  Use per-item
// atomic accesses unless `exclusive` is true.
static void
array_copy_raw(char *dst, const char *src, Py_ssize_t n, int itemsize,
               bool exclusive)
{
    if (n <= 0 || dst == src) {
        return;
    }
#ifdef Py_GIL_DISABLED
    if (!exclusive) {
        char buf[16];
        if (dst < src) {
            for (Py_ssize_t i = 0; i < n; i++) {
                array_load_raw(src + i * itemsize, buf, itemsize);
                array_store_raw(dst + i * itemsize, buf, itemsize);
            }
        }
        else {
            for (Py_ssize_t i = n; --i >= 0; ) {
                array_load_raw(src + i * itemsize, buf, itemsize);
                array_store_raw(dst + i * itemsize, buf, itemsize);
            }
        }
        return;
    }
#endif
    memmove(dst, src, n * itemsize);
}

static inline void
array_set_size(arrayobject *self, Py_ssize_t size)
{
    FT_ATOMIC_STORE_SSIZE_RELEASE(((PyVarObject *)self)->ob_size, size);
}

static void
ensure_shared_on_resize(arrayobject *self)
{
#ifdef Py_GIL_DISABLED
    // Ensure that the array data is freed using QSBR if we are not the
    // owning thread.
    if (!_Py_IsOwnedByCurrentThread((PyObject *)self) &&
        !_PyObject_GC_IS_SHARED(self))
    {
        _PyObject_GC_SET_SHARED(self);
    }
#endif
}

// Resize the storage for newsize items.  If publish is false and the array
// grows, ob_size is left unchanged: the caller writes the new items and
// then calls array_set_size().
static int
array_resize_ex(arrayobject *self, Py_ssize_t newsize, bool publish)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    assert(newsize >= 0);
    Py_ssize_t size = Py_SIZE(self);
    assert(publish || newsize >= size);

    if (self->ob_exports > 0 && newsize != size) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }

    /* Bypass realloc() when a previous overallocation is large enough
       to accommodate the newsize.  If the newsize is 16 smaller than the
       current size, then proceed with the realloc() to shrink the array.
    */

    char *items = self->ob_item;
    int itemsize = self->ob_descr->itemsize;

    if (items != NULL &&
        self->allocated >= newsize &&
        size < newsize + 16) {
        if (publish) {
            array_set_size(self, newsize);
        }
        return 0;
    }

    ensure_shared_on_resize(self);

    if (newsize == 0) {
        array_set_size(self, 0);
        FT_ATOMIC_STORE_PTR_RELEASE(self->ob_item, NULL);
        self->allocated = 0;
        array_items_free(items, itemsize, !array_is_exclusive(self));
        return 0;
    }

    /* This over-allocates proportional to the array size, making room
     * for additional growth.  The over-allocation is mild, but is
     * enough to give linear-time amortized behavior over a long
     * sequence of appends() in the presence of a poorly-performing
     * system realloc().
     * The growth pattern is:  0, 4, 8, 16, 25, 34, 46, 56, 67, 79, ...
     * Note, the pattern starts out the same as for lists but then
     * grows at a smaller rate so that larger arrays only overallocate
     * by about 1/16th -- this is done because arrays are presumed to be more
     * memory critical.
     */

    size_t _new_size = (size_t)(newsize >> 4) + (size < 8 ? 3 : 7) + newsize;

    if (!array_size_valid(_new_size, itemsize)) {
        PyErr_NoMemory();
        return -1;
    }

#ifdef Py_GIL_DISABLED
    char *newitems = array_items_alloc(_new_size, itemsize);
    if (newitems == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    bool exclusive = array_is_exclusive(self);
    if (newsize < size) {
        // Publish the smaller size before the smaller buffer, so that the
        // first `ob_size` items of the current buffer are always valid.
        array_set_size(self, newsize);
    }
    array_copy_raw(newitems, items, Py_MIN(size, newsize), itemsize,
                   exclusive);
    _Py_atomic_store_ptr_release(&self->ob_item, newitems);
    array_items_free(items, itemsize, !exclusive);
#else
    items = PyMem_Realloc(items, _new_size * itemsize);
    if (items == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->ob_item = items;
#endif
    self->allocated = _new_size;

    if (publish) {
        array_set_size(self, newsize);
    }
    return 0;
}

static int
array_resize(arrayobject *self, Py_ssize_t newsize)
{
    return array_resize_ex(self, newsize, true);
}

// Make room for newsize >= Py_SIZE(self) items without publishing the new
// size.
static int
array_reserve(arrayobject *self, Py_ssize_t newsize)
{
    return array_resize_ex(self, newsize, false);
}

// Get and Pack functions for each type.
// A Get function takes a char* to the items and an integer index, returning
// the array value at that index wrapped in an appropriate PyObject*.  It does
// not check that the index is in bounds.
// A Pack function converts a PyObject* to the raw C value and writes it to a
// local buffer of itemsize bytes.  It returns 0 on success, or -1 with an
// exception set.  It may call user code (__index__, __float__, ...), so it is
// called before the array buffer is loaded; the caller then checks the index
// and stores the item with array_store_raw().

#define DEFINE_ITEM_LOAD(NAME, T) \
    static inline T \
    NAME##_load(const char *items, Py_ssize_t i) \
    { \
        T x; \
        array_load_raw(items + i * (Py_ssize_t)sizeof(T), &x, sizeof(T)); \
        return x; \
    }

DEFINE_ITEM_LOAD(schar, signed char)
DEFINE_ITEM_LOAD(uchar, unsigned char)
DEFINE_ITEM_LOAD(ucs4, Py_UCS4)
DEFINE_ITEM_LOAD(short, short)
DEFINE_ITEM_LOAD(ushort, unsigned short)
DEFINE_ITEM_LOAD(int, int)
DEFINE_ITEM_LOAD(uint, unsigned int)
DEFINE_ITEM_LOAD(long, long)
DEFINE_ITEM_LOAD(ulong, unsigned long)
DEFINE_ITEM_LOAD(longlong, long long)
DEFINE_ITEM_LOAD(ulonglong, unsigned long long)
DEFINE_ITEM_LOAD(float, float)
DEFINE_ITEM_LOAD(double, double)

static PyObject *
b_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(schar_load(items, i));
}

static int
b_pack(PyObject *v, char *buf)
{
    short x;
    /* PyArg_Parse's 'b' formatter is for an unsigned char, therefore
       must use the next size up that is signed ('h') and manually do
       the overflow checking */
    if (!PyArg_Parse(v, "h;array item must be integer", &x))
        return -1;
    if (x < -128) {
        PyErr_SetString(PyExc_OverflowError,
            "signed char is less than minimum");
        return -1;
    }
    else if (x > 127) {
        PyErr_SetString(PyExc_OverflowError,
            "signed char is greater than maximum");
        return -1;
    }
    *(signed char *)buf = (signed char)x;
    return 0;
}

static PyObject *
BB_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(uchar_load(items, i));
}

static int
BB_pack(PyObject *v, char *buf)
{
    unsigned char x;
    /* 'B' == unsigned char, maps to PyArg_Parse's 'b' formatter */
    if (!PyArg_Parse(v, "b;array item must be integer", &x))
        return -1;
    *(unsigned char *)buf = x;
    return 0;
}

static PyObject *
w_getitem(const char *items, Py_ssize_t i)
{
    return PyUnicode_FromOrdinal(ucs4_load(items, i));
}

static int
w_pack(PyObject *v, char *buf)
{
    if (!PyUnicode_Check(v)) {
        PyErr_Format(PyExc_TypeError,
                     "array item must be a unicode character, not %T",
                     v);
        return -1;
    }

    if (PyUnicode_GET_LENGTH(v) != 1) {
        PyErr_Format(PyExc_TypeError,
                     "array item must be a unicode character, "
                     "not a string of length %zd",
                     PyUnicode_GET_LENGTH(v));
        return -1;
    }

    Py_UCS4 x = PyUnicode_READ_CHAR(v, 0);
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
h_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(short_load(items, i));
}

static int
h_pack(PyObject *v, char *buf)
{
    short x;
    /* 'h' == signed short, maps to PyArg_Parse's 'h' formatter */
    if (!PyArg_Parse(v, "h;array item must be integer", &x))
        return -1;
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
HH_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(ushort_load(items, i));
}

static int
HH_pack(PyObject *v, char *buf)
{
    int x;
    /* PyArg_Parse's 'h' formatter is for a signed short, therefore
       must use the next size up and manually do the overflow checking */
    if (!PyArg_Parse(v, "i;array item must be integer", &x))
        return -1;
    else if (x < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned short is less than minimum");
        return -1;
    }
    else if (x > USHRT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
            "unsigned short is greater than maximum");
        return -1;
    }
    unsigned short y = (unsigned short)x;
    memcpy(buf, &y, sizeof(y));
    return 0;
}

static PyObject *
i_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(int_load(items, i));
}

static int
i_pack(PyObject *v, char *buf)
{
    int x;
    /* 'i' == signed int, maps to PyArg_Parse's 'i' formatter */
    if (!PyArg_Parse(v, "i;array item must be integer", &x))
        return -1;
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
II_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromUnsignedLong(uint_load(items, i));
}

static int
II_pack(PyObject *v, char *buf)
{
    unsigned long x;
    int do_decref = 0; /* if nb_int was called */

    if (!PyLong_Check(v)) {
        Py_INCREF(v);
        PyObject *res = _PyNumber_Index(v);
        Py_DECREF(v);
        if (NULL == res) {
            return -1;
        }
        v = res;
        do_decref = 1;
    }
    x = PyLong_AsUnsignedLong(v);
    if (do_decref) {
        Py_DECREF(v);
    }
    if (x == (unsigned long)-1 && PyErr_Occurred()) {
        return -1;
    }
    if (x > UINT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "unsigned int is greater than maximum");
        return -1;
    }
    unsigned int y = (unsigned int)x;
    memcpy(buf, &y, sizeof(y));
    return 0;
}

static PyObject *
l_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLong(long_load(items, i));
}

static int
l_pack(PyObject *v, char *buf)
{
    long x;
    if (!PyArg_Parse(v, "l;array item must be integer", &x))
        return -1;
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
LL_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromUnsignedLong(ulong_load(items, i));
}

static int
LL_pack(PyObject *v, char *buf)
{
    unsigned long x;
    int do_decref = 0; /* if nb_int was called */

    if (!PyLong_Check(v)) {
        Py_INCREF(v);
        PyObject *res = _PyNumber_Index(v);
        Py_DECREF(v);
        if (NULL == res) {
            return -1;
        }
        v = res;
        do_decref = 1;
    }
    x = PyLong_AsUnsignedLong(v);
    if (do_decref) {
        Py_DECREF(v);
    }
    if (x == (unsigned long)-1 && PyErr_Occurred()) {
        return -1;
    }
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
q_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromLongLong(longlong_load(items, i));
}

static int
q_pack(PyObject *v, char *buf)
{
    long long x;
    if (!PyArg_Parse(v, "L;array item must be integer", &x))
        return -1;
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
QQ_getitem(const char *items, Py_ssize_t i)
{
    return PyLong_FromUnsignedLongLong(ulonglong_load(items, i));
}

static int
QQ_pack(PyObject *v, char *buf)
{
    unsigned long long x;
    int do_decref = 0; /* if nb_int was called */

    if (!PyLong_Check(v)) {
        Py_INCREF(v);
        PyObject *res = _PyNumber_Index(v);
        Py_DECREF(v);
        if (NULL == res) {
            return -1;
        }
        v = res;
        do_decref = 1;
    }
    x = PyLong_AsUnsignedLongLong(v);
    if (do_decref) {
        Py_DECREF(v);
    }
    if (x == (unsigned long long)-1 && PyErr_Occurred()) {
        return -1;
    }
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
e_getitem(const char *items, Py_ssize_t i)
{
    char b[2];
    array_load_raw(items + sizeof(short)*i, b, 2);
    double x = PyFloat_Unpack2(b, PY_LITTLE_ENDIAN);
    return PyFloat_FromDouble(x);
}

static int
e_pack(PyObject *v, char *buf)
{
    double x;
    if (!PyArg_Parse(v, "d;array item must be float", &x)) {
        return -1;
    }
    return PyFloat_Pack2(x, buf, PY_LITTLE_ENDIAN);
}

static PyObject *
f_getitem(const char *items, Py_ssize_t i)
{
    return PyFloat_FromDouble((double)float_load(items, i));
}

static int
f_pack(PyObject *v, char *buf)
{
    double x;
    if (!PyArg_Parse(v, "d;array item must be float", &x))
        return -1;
    return PyFloat_Pack4(x, buf, PY_LITTLE_ENDIAN);
}

static PyObject *
d_getitem(const char *items, Py_ssize_t i)
{
    return PyFloat_FromDouble(double_load(items, i));
}

static int
d_pack(PyObject *v, char *buf)
{
    double x;
    if (!PyArg_Parse(v, "d;array item must be float", &x))
        return -1;
    memcpy(buf, &x, sizeof(x));
    return 0;
}

static PyObject *
cf_getitem(const char *items, Py_ssize_t i)
{
    float f[2];

    array_load_raw(items + i*sizeof(f), f, sizeof(f));
    return PyComplex_FromDoubles(f[0], f[1]);
}

static int
cf_pack(PyObject *v, char *buf)
{
    Py_complex x;

    if (!PyArg_Parse(v, "D;array item must be complex", &x)) {
        return -1;
    }
    if (PyFloat_Pack4(x.real, buf, PY_LITTLE_ENDIAN)) {
        return -1;
    }
    return PyFloat_Pack4(x.imag, buf + sizeof(float), PY_LITTLE_ENDIAN);
}

static PyObject *
cd_getitem(const char *items, Py_ssize_t i)
{
    double f[2];

    array_load_raw(items + i*sizeof(f), f, sizeof(f));
    return PyComplex_FromDoubles(f[0], f[1]);
}

static int
cd_pack(PyObject *v, char *buf)
{
    Py_complex x;
    double f[2];

    if (!PyArg_Parse(v, "D;array item must be complex", &x)) {
        return -1;
    }
    f[0] = x.real;
    f[1] = x.imag;
    memcpy(buf, f, sizeof(f));
    return 0;
}

#define DEFINE_COMPAREITEMS(code, name, type) \
    static int \
    code##_compareitems(const void *lhs, const void *rhs, Py_ssize_t length) \
    { \
        for (Py_ssize_t i = 0; i < length; ++i) { \
            type a = name##_load(lhs, i); \
            type b = name##_load(rhs, i); \
            if (a != b) \
                return a < b ? -1 : 1; \
        } \
        return 0; \
    }

DEFINE_COMPAREITEMS(b, schar, signed char)
DEFINE_COMPAREITEMS(BB, uchar, unsigned char)
DEFINE_COMPAREITEMS(w, ucs4, Py_UCS4)
DEFINE_COMPAREITEMS(h, short, short)
DEFINE_COMPAREITEMS(HH, ushort, unsigned short)
DEFINE_COMPAREITEMS(i, int, int)
DEFINE_COMPAREITEMS(II, uint, unsigned int)
DEFINE_COMPAREITEMS(l, long, long)
DEFINE_COMPAREITEMS(LL, ulong, unsigned long)
DEFINE_COMPAREITEMS(q, longlong, long long)
DEFINE_COMPAREITEMS(QQ, ulonglong, unsigned long long)

/* Description of types.
 *
 * Don't forget to update typecode_to_mformat_code() if you add a new
 * typecode.
 */
static const struct arraydescr descriptors[] = {
    {"b", 1, b_getitem, b_pack, b_compareitems, 1, 1},
    {"B", 1, BB_getitem, BB_pack, BB_compareitems, 1, 0},
    {"w", sizeof(Py_UCS4), w_getitem, w_pack, w_compareitems, 0, 0,},
    {"h", sizeof(short), h_getitem, h_pack, h_compareitems, 1, 1},
    {"H", sizeof(short), HH_getitem, HH_pack, HH_compareitems, 1, 0},
    {"i", sizeof(int), i_getitem, i_pack, i_compareitems, 1, 1},
    {"I", sizeof(int), II_getitem, II_pack, II_compareitems, 1, 0},
    {"l", sizeof(long), l_getitem, l_pack, l_compareitems, 1, 1},
    {"L", sizeof(long), LL_getitem, LL_pack, LL_compareitems, 1, 0},
    {"q", sizeof(long long), q_getitem, q_pack, q_compareitems, 1, 1},
    {"Q", sizeof(long long), QQ_getitem, QQ_pack, QQ_compareitems, 1, 0},
    {"e", sizeof(short), e_getitem, e_pack, NULL, 0, 0},
    {"f", sizeof(float), f_getitem, f_pack, NULL, 0, 0},
    {"d", sizeof(double), d_getitem, d_pack, NULL, 0, 0},
    {"Zf", 2*sizeof(float), cf_getitem, cf_pack, NULL, 0, 0},
    {"Zd", 2*sizeof(double), cd_getitem, cd_pack, NULL, 0, 0},
    {"", 0, 0, 0, 0, 0, 0} /* Sentinel */
};

/****************************************************************************
Implementations of array object methods.
****************************************************************************/
/*[clinic input]
class array.array "arrayobject *" "ArrayType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a5c29edf59f176a3]*/

static PyObject *
newarrayobject_untracked(PyTypeObject *type, Py_ssize_t size,
                         const struct arraydescr *descr)
{
    arrayobject *op;

    if (size < 0) {
        PyErr_BadInternalCall();
        return NULL;
    }

    /* Check for overflow */
    if (!array_size_valid(size, descr->itemsize)) {
        return PyErr_NoMemory();
    }
    op = (arrayobject *) _PyType_AllocNoTrack(type, 0);
    if (op == NULL) {
        return NULL;
    }
    op->ob_descr = descr;
    op->allocated = size;
    op->weakreflist = NULL;
    Py_SET_SIZE(op, size);
    if (size <= 0) {
        op->ob_item = NULL;
    }
    else {
        op->ob_item = array_items_alloc(size, descr->itemsize);
        if (op->ob_item == NULL) {
            Py_DECREF(op);
            return PyErr_NoMemory();
        }
    }
    op->ob_exports = 0;
    return (PyObject *) op;
}

static int
valid_index(Py_ssize_t i, Py_ssize_t limit)
{
    return (size_t) i < (size_t) limit;
}

static PyObject *
newarrayobject(PyTypeObject *type, Py_ssize_t size, const struct arraydescr *descr)
{
    PyObject *op = newarrayobject_untracked(type, size, descr);
    if (op != NULL) {
        _PyObject_GC_TRACK(op);
    }
    return op;
}

/* Get item i; the caller holds the lock or owns a private array. */
static PyObject *
getarrayitem(arrayobject *ap, Py_ssize_t i)
{
#ifndef NDEBUG
    array_state *state = find_array_state_by_type(Py_TYPE(ap));
    assert(array_Check(ap, state));
    assert(valid_index(i, Py_SIZE(ap)));
#endif
    return (*ap->ob_descr->getitem)(ap->ob_item, i);
}

static void
array_mark_shared(PyObject *op)
{
#ifdef Py_GIL_DISABLED
    if (!_PyObject_GC_IS_SHARED(op)) {
        _PyObject_GC_SET_SHARED(op);
    }
#endif
}

// The getarrayitem_*() functions below return a new reference to item i.
// They return NULL without an exception set if i is out of range, and NULL
// with an exception set if the item object could not be created.

#ifdef Py_GIL_DISABLED

static PyObject *
getarrayitem_locked(PyObject *op, Py_ssize_t i)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    array_mark_shared(op);
    if (!valid_index(i, Py_SIZE(op)))  {
        ret = NULL;
    }
    else {
        ret = getarrayitem(arrayobject_CAST(op), i);
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

// Return true if item i of `items` can be accessed without the lock.
// `items` and then `size` were loaded from the array, in that order.  The
// size only belongs to the buffer if the buffer was not replaced in between;
// otherwise item i may be past the end or not yet written.
static inline bool
array_items_valid(arrayobject *ap, char *items, Py_ssize_t size, Py_ssize_t i)
{
    assert(valid_index(i, size));
    return (items != NULL &&
            valid_index(i, array_capacity(items)) &&
            items == _Py_atomic_load_ptr_acquire(&ap->ob_item));
}

static PyObject *
getarrayitem_threadsafe(PyObject *op, Py_ssize_t i)
{
    if (!_Py_IsOwnedByCurrentThread(op) && !_PyObject_GC_IS_SHARED(op)) {
        return getarrayitem_locked(op, i);
    }
    arrayobject *ap = arrayobject_CAST(op);
    char *items = _Py_atomic_load_ptr_acquire(&ap->ob_item);
    Py_ssize_t size = array_get_size(op);
    if (!valid_index(i, size)) {
        return NULL;
    }
    if (!array_items_valid(ap, items, size, i)) {
        return getarrayitem_locked(op, i);
    }
    return (*ap->ob_descr->getitem)(items, i);
}

#else // Py_GIL_DISABLED

static inline PyObject *
getarrayitem_threadsafe(PyObject *op, Py_ssize_t i)
{
    if (!valid_index(i, Py_SIZE(op))) {
        return NULL;
    }
    return getarrayitem(arrayobject_CAST(op), i);
}

#endif // Py_GIL_DISABLED

// Store a packed item at index i; the caller checked the index.
static inline void
array_store_item(arrayobject *ap, Py_ssize_t i, const char *buf)
{
    int itemsize = ap->ob_descr->itemsize;
    array_store_raw(ap->ob_item + i * itemsize, buf, itemsize);
}

static int
setarrayitem_packed_locked(PyObject *op, Py_ssize_t i, const char *buf)
{
    int ret = 0;
    Py_BEGIN_CRITICAL_SECTION(op);
    array_mark_shared(op);
    if (!valid_index(i, Py_SIZE(op)))  {
        PyErr_SetString(PyExc_IndexError,
                        "array assignment index out of range");
        ret = -1;
    }
    else {
        array_store_item(arrayobject_CAST(op), i, buf);
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

// a[i] = v.  The item is converted before the buffer is loaded, since the
// conversion can run user code that resizes the array.
static int
setarrayitem_threadsafe(PyObject *op, Py_ssize_t i, PyObject *v)
{
    arrayobject *ap = arrayobject_CAST(op);
    if (!valid_index(i, array_get_size(op))) {
        PyErr_SetString(PyExc_IndexError,
                        "array assignment index out of range");
        return -1;
    }
    _Py_ALIGNED_DEF(8, char) buf[16];
    if ((*ap->ob_descr->pack)(v, buf) < 0) {
        return -1;
    }
#ifdef Py_GIL_DISABLED
    if (_Py_IsOwnedByCurrentThread(op) || _PyObject_GC_IS_SHARED(op)) {
        char *items = _Py_atomic_load_ptr_acquire(&ap->ob_item);
        Py_ssize_t size = array_get_size(op);
        if (!valid_index(i, size)) {
            PyErr_SetString(PyExc_IndexError,
                            "array assignment index out of range");
            return -1;
        }
        if (array_items_valid(ap, items, size, i)) {
            int itemsize = ap->ob_descr->itemsize;
            array_store_raw(items + i * itemsize, buf, itemsize);
            return 0;
        }
    }
#endif
    return setarrayitem_packed_locked(op, i, buf);
}

static int
ins1(arrayobject *self, Py_ssize_t where, PyObject *v)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    if (v == NULL) {
        PyErr_BadInternalCall();
        return -1;
    }
    _Py_ALIGNED_DEF(8, char) buf[16];
    if ((*self->ob_descr->pack)(v, buf) < 0) {
        return -1;
    }

    // pack() can run user code, so read the size after it.
    Py_ssize_t n = Py_SIZE(self);
    if (array_reserve(self, n+1) == -1)
        return -1;
    if (where < 0) {
        where += n;
        if (where < 0)
            where = 0;
    }
    if (where > n)
        where = n;
    int itemsize = self->ob_descr->itemsize;
    char *items = self->ob_item;
    /* appends don't need to move items */
    if (where != n) {
        array_copy_raw(items + (where+1)*itemsize, items + where*itemsize,
                       n - where, itemsize, array_is_exclusive(self));
    }
    array_store_item(self, where, buf);
    array_set_size(self, n+1);
    return 0;
}

/* Methods */
static void
array_dealloc(PyObject *op)
{
    PyTypeObject *tp = Py_TYPE(op);
    PyObject_GC_UnTrack(op);

    arrayobject *self = arrayobject_CAST(op);
    if (self->ob_exports > 0) {
        PyErr_FormatUnraisable("deallocated array object has exported buffers");
    }
    FT_CLEAR_WEAKREFS(op, self->weakreflist);
    array_items_free(self->ob_item, self->ob_descr->itemsize, false);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static PyObject *
array_richcompare_lock_held(PyObject *v, PyObject *w, int op)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(v);
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(w);
    array_state *state = find_array_state_by_type(Py_TYPE(v));
    arrayobject *va, *wa;
    PyObject *vi = NULL;
    PyObject *wi = NULL;
    Py_ssize_t i, k;
    PyObject *res;

    if (!array_Check(v, state) || !array_Check(w, state))
        Py_RETURN_NOTIMPLEMENTED;

    va = (arrayobject *)v;
    wa = (arrayobject *)w;

    if (Py_SIZE(va) != Py_SIZE(wa) && (op == Py_EQ || op == Py_NE)) {
        /* Shortcut: if the lengths differ, the arrays differ */
        if (op == Py_EQ)
            res = Py_False;
        else
            res = Py_True;
        return Py_NewRef(res);
    }

    if (va->ob_descr == wa->ob_descr && va->ob_descr->compareitems != NULL) {
        /* Fast path:
           arrays with same types can have their buffers compared directly */
        Py_ssize_t common_length = Py_MIN(Py_SIZE(va), Py_SIZE(wa));
        int result = va->ob_descr->compareitems(va->ob_item, wa->ob_item,
                                                common_length);
        if (result == 0)
            goto compare_sizes;

        int cmp;
        switch (op) {
        case Py_LT: cmp = result < 0; break;
        case Py_LE: cmp = result <= 0; break;
        case Py_EQ: cmp = result == 0; break;
        case Py_NE: cmp = result != 0; break;
        case Py_GT: cmp = result > 0; break;
        case Py_GE: cmp = result >= 0; break;
        default: return NULL; /* cannot happen */
        }
        PyObject *res = cmp ? Py_True : Py_False;
        return Py_NewRef(res);
    }


    /* Search for the first index where items are different */
    k = 1;
    for (i = 0; i < Py_SIZE(va) && i < Py_SIZE(wa); i++) {
        vi = getarrayitem(va, i);
        if (vi == NULL) {
            return NULL;
        }
        wi = getarrayitem(wa, i);
        if (wi == NULL) {
            Py_DECREF(vi);
            return NULL;
        }
        k = PyObject_RichCompareBool(vi, wi, Py_EQ);
        if (k == 0)
            break; /* Keeping vi and wi alive! */
        Py_DECREF(vi);
        Py_DECREF(wi);
        if (k < 0)
            return NULL;
    }

    if (k) {
        /* No more items to compare -- compare sizes */
        compare_sizes: ;
        Py_ssize_t vs = Py_SIZE(va);
        Py_ssize_t ws = Py_SIZE(wa);
        int cmp;
        switch (op) {
        case Py_LT: cmp = vs <  ws; break;
        case Py_LE: cmp = vs <= ws; break;
        /* If the lengths were not equal,
           the earlier fast-path check would have caught that. */
        case Py_EQ: assert(vs == ws); cmp = 1; break;
        case Py_NE: assert(vs == ws); cmp = 0; break;
        case Py_GT: cmp = vs >  ws; break;
        case Py_GE: cmp = vs >= ws; break;
        default: return NULL; /* cannot happen */
        }
        if (cmp)
            res = Py_True;
        else
            res = Py_False;
        return Py_NewRef(res);
    }

    /* We have an item that differs.  First, shortcuts for EQ/NE */
    if (op == Py_EQ) {
        res = Py_NewRef(Py_False);
    }
    else if (op == Py_NE) {
        res = Py_NewRef(Py_True);
    }
    else {
        /* Compare the final item again using the proper operator */
        res = PyObject_RichCompare(vi, wi, op);
    }
    Py_DECREF(vi);
    Py_DECREF(wi);
    return res;
}

static PyObject *
array_richcompare(PyObject *v, PyObject *w, int op)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION2(v, w);
    ret = array_richcompare_lock_held(v, w, op);
    Py_END_CRITICAL_SECTION2();
    return ret;
}

static Py_ssize_t
array_length(PyObject *op)
{
    arrayobject *self = arrayobject_CAST(op);
    return array_get_size(self);
}


static PyObject *
array_item(PyObject *op, Py_ssize_t i)
{
    PyObject *item = getarrayitem_threadsafe(op, i);
    if (item == NULL && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_IndexError, "array index out of range");
    }
    return item;
}


static PyObject *
array_slice(arrayobject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(a);
    array_state *state = find_array_state_by_type(Py_TYPE(a));
    arrayobject *np;

    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    np = (arrayobject *) newarrayobject_untracked(state->ArrayType, ihigh - ilow, a->ob_descr);
    if (np == NULL)
        return NULL;
    if (ihigh > ilow) {
        int itemsize = a->ob_descr->itemsize;
        array_copy_raw(np->ob_item, a->ob_item + ilow * itemsize,
                       ihigh - ilow, itemsize, array_is_exclusive(a));
    }
    _PyObject_GC_TRACK(np);
    return (PyObject *)np;
}

/*[clinic input]
@critical_section
array.array.clear

Remove all items from the array.
[clinic start generated code]*/

static PyObject *
array_array_clear_impl(arrayobject *self)
/*[clinic end generated code: output=5efe0417062210a9 input=1c9dfcc80f5b6731]*/
{
    if (array_resize(self, 0) == -1) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
array.array.__copy__

Return a copy of the array.
[clinic start generated code]*/

static PyObject *
array_array___copy___impl(arrayobject *self)
/*[clinic end generated code: output=dec7c3f925d9619e input=7622f8f9489472d5]*/
{
    return array_slice(self, 0, Py_SIZE(self));
}

/*[clinic input]
@critical_section
array.array.__deepcopy__

    unused: object
    /

Return a copy of the array.
[clinic start generated code]*/

static PyObject *
array_array___deepcopy___impl(arrayobject *self, PyObject *unused)
/*[clinic end generated code: output=703b4c412feaaf31 input=1a29f718f5b8a1dc]*/
{
    return array_array___copy___impl(self);
}

static PyObject *
array_concat_lock_held(PyObject *op, PyObject *bb)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(bb);
    arrayobject *a = arrayobject_CAST(op);
    array_state *state = find_array_state_by_type(Py_TYPE(a));
    Py_ssize_t size;
    arrayobject *np;
    if (!array_Check(bb, state)) {
        PyErr_Format(PyExc_TypeError,
             "can only append array (not \"%.200s\") to array",
                 Py_TYPE(bb)->tp_name);
        return NULL;
    }
#define b ((arrayobject *)bb)
    if (a->ob_descr != b->ob_descr) {
        PyErr_BadArgument();
        return NULL;
    }
    if (Py_SIZE(a) > PY_SSIZE_T_MAX - Py_SIZE(b)) {
        return PyErr_NoMemory();
    }
    size = Py_SIZE(a) + Py_SIZE(b);
    np = (arrayobject *) newarrayobject_untracked(state->ArrayType, size, a->ob_descr);
    if (np == NULL) {
        return NULL;
    }
    int itemsize = a->ob_descr->itemsize;
    if (Py_SIZE(a) > 0) {
        array_copy_raw(np->ob_item, a->ob_item, Py_SIZE(a),
                       itemsize, array_is_exclusive(a));
    }
    if (Py_SIZE(b) > 0) {
        array_copy_raw(np->ob_item + Py_SIZE(a) * itemsize,
                       b->ob_item, Py_SIZE(b),
                       itemsize, array_is_exclusive(b));
    }
    _PyObject_GC_TRACK(np);
    return (PyObject *)np;
#undef b
}

static PyObject *
array_concat(PyObject *op, PyObject *bb)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION2(op, bb);
    ret = array_concat_lock_held(op, bb);
    Py_END_CRITICAL_SECTION2();
    return ret;
}

static PyObject *
array_repeat_lock_held(PyObject *op, Py_ssize_t n)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    arrayobject *a = arrayobject_CAST(op);
    array_state *state = find_array_state_by_type(Py_TYPE(a));

    if (n < 0)
        n = 0;
    const Py_ssize_t array_length = Py_SIZE(a);
    if ((array_length != 0) && (n > PY_SSIZE_T_MAX / array_length)) {
        return PyErr_NoMemory();
    }
    Py_ssize_t size = array_length * n;
    arrayobject* np = (arrayobject *) newarrayobject_untracked(state->ArrayType, size, a->ob_descr);
    if (np == NULL)
        return NULL;
    if (size == 0) {
        _PyObject_GC_TRACK(np);
        return (PyObject *)np;
    }

    const Py_ssize_t oldbytes = array_length * a->ob_descr->itemsize;
    const Py_ssize_t newbytes = oldbytes * n;
    // Copy one repetition from a, then repeat within the private np.
    array_copy_raw(np->ob_item, a->ob_item, array_length,
                   a->ob_descr->itemsize, array_is_exclusive(a));
    _PyBytes_RepeatBuffer(np->ob_item, newbytes, np->ob_item,
                          oldbytes);

    _PyObject_GC_TRACK(np);
    return (PyObject *)np;
}

static PyObject *
array_repeat(PyObject *op, Py_ssize_t n)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_repeat_lock_held(op, n);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static int
array_del_slice(arrayobject *a, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(a);
    char *item;
    Py_ssize_t d; /* Change in size */
    if (ilow < 0)
        ilow = 0;
    else if (ilow > Py_SIZE(a))
        ilow = Py_SIZE(a);
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > Py_SIZE(a))
        ihigh = Py_SIZE(a);
    item = a->ob_item;
    d = ihigh-ilow;
    /* Issue #4509: If the array has exported buffers and the slice
       assignment would change the size of the array, fail early to make
       sure we don't modify it. */
    if (d != 0 && a->ob_exports > 0) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }
    if (d > 0) { /* Delete d items */
        int itemsize = a->ob_descr->itemsize;
        array_copy_raw(item + ilow * itemsize, item + ihigh * itemsize,
                       Py_SIZE(a) - ihigh, itemsize, array_is_exclusive(a));
        if (array_resize(a, Py_SIZE(a) - d) == -1)
            return -1;
    }
    return 0;
}

static int
array_ass_item(PyObject *op, Py_ssize_t i, PyObject *v)
{
    if (v != NULL) {
        return setarrayitem_threadsafe(op, i, v);
    }
    int ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    if (!valid_index(i, Py_SIZE(op))) {
        PyErr_SetString(PyExc_IndexError,
                        "array assignment index out of range");
        ret = -1;
    }
    else {
        ret = array_del_slice(arrayobject_CAST(op), i, i+1);
    }
    Py_END_CRITICAL_SECTION();
    return ret;
}

static int
array_iter_extend(arrayobject *self, PyObject *bb)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    PyObject *it, *v;

    it = PyObject_GetIter(bb);
    if (it == NULL)
        return -1;

    while ((v = PyIter_Next(it)) != NULL) {
        if (ins1(self, Py_SIZE(self), v) != 0) {
            Py_DECREF(v);
            Py_DECREF(it);
            return -1;
        }
        Py_DECREF(v);
    }
    Py_DECREF(it);
    if (PyErr_Occurred())
        return -1;
    return 0;
}

static int
array_do_extend_lock_held(array_state *state, arrayobject *self, PyObject *bb)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(self);
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(bb);
    Py_ssize_t size, oldsize, bbsize;

    if (!array_Check(bb, state))
        return array_iter_extend(self, bb);
#define b ((arrayobject *)bb)
    if (self->ob_descr != b->ob_descr) {
        PyErr_SetString(PyExc_TypeError,
                     "can only extend with array of same kind");
        return -1;
    }
    if ((Py_SIZE(self) > PY_SSIZE_T_MAX - Py_SIZE(b)) ||
        !array_size_valid(Py_SIZE(self) + Py_SIZE(b),
                          self->ob_descr->itemsize))
    {
        PyErr_NoMemory();
        return -1;
    }
    oldsize = Py_SIZE(self);
    /* Get the size of bb before resizing the array since bb could be self. */
    bbsize = Py_SIZE(bb);
    size = oldsize + bbsize;
    if (array_reserve(self, size) == -1)
        return -1;
    if (bbsize > 0) {
        int itemsize = self->ob_descr->itemsize;
        array_copy_raw(self->ob_item + oldsize * itemsize,
                       b->ob_item, bbsize, itemsize,
                       array_is_exclusive(self) && array_is_exclusive(b));
    }
    array_set_size(self, size);

    return 0;
#undef b
}

static int
array_do_extend(array_state *state, arrayobject *self, PyObject *bb)
{
    int ret;
    Py_BEGIN_CRITICAL_SECTION2(self, bb);
    ret = array_do_extend_lock_held(state, self, bb);
    Py_END_CRITICAL_SECTION2();
    return ret;
}

static PyObject *
array_inplace_concat(PyObject *op, PyObject *bb)
{
    arrayobject *self = arrayobject_CAST(op);
    array_state *state = find_array_state_by_type(Py_TYPE(self));

    if (!array_Check(bb, state)) {
        PyErr_Format(PyExc_TypeError,
            "can only extend array with array (not \"%.200s\")",
            Py_TYPE(bb)->tp_name);
        return NULL;
    }
    if (array_do_extend(state, self, bb) == -1)
        return NULL;
    return Py_NewRef(self);
}

static PyObject *
array_inplace_repeat_lock_held(PyObject *op, Py_ssize_t n)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    arrayobject *self = arrayobject_CAST(op);
    const Py_ssize_t array_size = Py_SIZE(self);

    if (array_size > 0 && n != 1 ) {
        if (n < 0)
            n = 0;
        // XXX This check below does nothing useful???
        if ((self->ob_descr->itemsize != 0) &&
            (array_size > PY_SSIZE_T_MAX / self->ob_descr->itemsize)) {
            return PyErr_NoMemory();
        }
        Py_ssize_t size = array_size * self->ob_descr->itemsize;
        if (n > 0 && size > PY_SSIZE_T_MAX / n) {
            return PyErr_NoMemory();
        }
        if (n == 0) {
            if (array_resize(self, 0) == -1)
                return NULL;
            return Py_NewRef(self);
        }
        if (array_reserve(self, n * array_size) == -1)
            return NULL;

        char *items = self->ob_item;
        if (array_is_exclusive(self)) {
            _PyBytes_RepeatBuffer(items, n*size, items, size);
        }
        else {
            int itemsize = self->ob_descr->itemsize;
            for (Py_ssize_t k = 1; k < n; k++) {
                array_copy_raw(items + k * size, items, array_size,
                               itemsize, false);
            }
        }
        array_set_size(self, n * array_size);
    }
    return Py_NewRef(self);
}

static PyObject *
array_inplace_repeat(PyObject *op, Py_ssize_t n)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_inplace_repeat_lock_held(op, n);
    Py_END_CRITICAL_SECTION();
    return ret;
}


/*[clinic input]
@critical_section
array.array.count

    v: object
    /

Return number of occurrences of v in the array.
[clinic start generated code]*/

static PyObject *
array_array_count_impl(arrayobject *self, PyObject *v)
/*[clinic end generated code: output=93ead26a2affb739 input=c12c0042c1d0e27e]*/
{
    Py_ssize_t count = 0;
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *selfi;
        int cmp;

        selfi = getarrayitem(self, i);
        if (selfi == NULL)
            return NULL;
        cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0)
            count++;
        else if (cmp < 0)
            return NULL;
    }
    return PyLong_FromSsize_t(count);
}


/*[clinic input]
@critical_section
array.array.index

    v: object
    start: slice_index(accept={int}) = 0
    stop: slice_index(accept={int}, c_default="PY_SSIZE_T_MAX") = sys.maxsize
    /

Return index of first occurrence of v in the array.

Raise ValueError if the value is not present.
[clinic start generated code]*/

static PyObject *
array_array_index_impl(arrayobject *self, PyObject *v, Py_ssize_t start,
                       Py_ssize_t stop)
/*[clinic end generated code: output=c45e777880c99f52 input=fa32ac8ec22175d6]*/
{
    if (start < 0) {
        start += Py_SIZE(self);
        if (start < 0) {
            start = 0;
        }
    }
    if (stop < 0) {
        stop += Py_SIZE(self);
    }
    // Use Py_SIZE() for every iteration in case the array is mutated
    // during PyObject_RichCompareBool()
    for (Py_ssize_t i = start; i < stop && i < Py_SIZE(self); i++) {
        PyObject *selfi;
        int cmp;

        selfi = getarrayitem(self, i);
        if (selfi == NULL)
            return NULL;
        cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0) {
            return PyLong_FromSsize_t(i);
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "array.index(x): x not in array");
    return NULL;
}

static int
array_contains_lock_held(PyObject *op, PyObject *v)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    Py_ssize_t i;
    int cmp;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(op); i++) {
        arrayobject *ap = (arrayobject *)op;
        PyObject *opi = getarrayitem(ap, i);
        if (opi == NULL)
            return -1;
        cmp = PyObject_RichCompareBool(opi, v, Py_EQ);
        Py_DECREF(opi);
    }
    return cmp;
}

static int
array_contains(PyObject *op, PyObject *v)
{
    int ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_contains_lock_held(op, v);
    Py_END_CRITICAL_SECTION();
    return ret;
}

/*[clinic input]
@critical_section
array.array.remove

    v: object
    /

Remove the first occurrence of v in the array.
[clinic start generated code]*/

static PyObject *
array_array_remove_impl(arrayobject *self, PyObject *v)
/*[clinic end generated code: output=f2a24e288ecb2a35 input=78bef3fd40e62f7a]*/
{
    Py_ssize_t i;

    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *selfi;
        int cmp;

        selfi = getarrayitem(self, i);
        if (selfi == NULL)
            return NULL;
        cmp = PyObject_RichCompareBool(selfi, v, Py_EQ);
        Py_DECREF(selfi);
        if (cmp > 0) {
            if (array_del_slice(self, i, i+1) != 0)
                return NULL;
            Py_RETURN_NONE;
        }
        else if (cmp < 0)
            return NULL;
    }
    PyErr_SetString(PyExc_ValueError, "array.remove(x): x not in array");
    return NULL;
}

/*[clinic input]
@critical_section
array.array.pop

    i: Py_ssize_t = -1
    /

Return the i-th element and delete it from the array.

i defaults to -1.
[clinic start generated code]*/

static PyObject *
array_array_pop_impl(arrayobject *self, Py_ssize_t i)
/*[clinic end generated code: output=bc1f0c54fe5308e4 input=c69a7f1f8c570e2f]*/
{
    PyObject *v;

    if (Py_SIZE(self) == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty array");
        return NULL;
    }
    if (i < 0)
        i += Py_SIZE(self);
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    v = getarrayitem(self, i);
    if (v == NULL)
        return NULL;
    if (array_del_slice(self, i, i+1) != 0) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
}

/*[clinic input]
array.array.extend

    cls: defining_class
    bb: object
    /

Append items to the end of the array.
[clinic start generated code]*/

static PyObject *
array_array_extend_impl(arrayobject *self, PyTypeObject *cls, PyObject *bb)
/*[clinic end generated code: output=e65eb7588f0bc266 input=8eb6817ec4d2cb62]*/
{
    array_state *state = get_array_state_by_class(cls);

    if (array_do_extend(state, self, bb) == -1)
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
array.array.insert

    i: Py_ssize_t
    v: object
    /

Insert a new item v into the array before position i.
[clinic start generated code]*/

static PyObject *
array_array_insert_impl(arrayobject *self, Py_ssize_t i, PyObject *v)
/*[clinic end generated code: output=5a3648e278348564 input=3c922bbd81462978]*/
{
    if (ins1(self, i, v) != 0)
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
@permit_long_summary
array.array.buffer_info

Return a tuple (address, length) giving the current memory address and the length in items of the buffer used to hold array's contents.

The length should be multiplied by the itemsize attribute to
calculate the buffer length in bytes.
[clinic start generated code]*/

static PyObject *
array_array_buffer_info_impl(arrayobject *self)
/*[clinic end generated code: output=9b2a4ec3ae7e98e7 input=05c248e0009ce284]*/
{
    PyObject* item1 = PyLong_FromVoidPtr(self->ob_item);
    if (item1 == NULL) {
        return NULL;
    }
    PyObject* item2 = PyLong_FromSsize_t(Py_SIZE(self));
    if (item2 == NULL) {
        Py_DECREF(item1);
        return NULL;
    }

    return _PyTuple_FromPairSteal(item1, item2);
}

/*[clinic input]
@critical_section
array.array.append

    v: object
    /

Append new value v to the end of the array.
[clinic start generated code]*/

static PyObject *
array_array_append_impl(arrayobject *self, PyObject *v)
/*[clinic end generated code: output=2f1e8cbad70c2a8b input=9cdd897c66a40c3f]*/
{
    if (ins1(self, Py_SIZE(self), v) != 0)
        return NULL;
    Py_RETURN_NONE;
}

#ifdef Py_GIL_DISABLED
// Byteswap an array that other threads can read without the lock.
static void
array_byteswap_shared(arrayobject *self)
{
    int itemsize = self->ob_descr->itemsize;
    // Complex types swap the real and imaginary parts separately.
    int part = itemsize;
    if (itemsize == 1) {
        return;
    }
    if (self->ob_descr->typecode[0] == 'Z') {
        part = itemsize / 2;
    }
    char *items = self->ob_item;
    for (Py_ssize_t i = 0; i < Py_SIZE(self); i++) {
        _Py_ALIGNED_DEF(8, char) buf[16];
        char *p = items + i * itemsize;
        array_load_raw(p, buf, itemsize);
        for (int k = 0; k < itemsize; k += part) {
            for (int lo = k, hi = k + part - 1; lo < hi; lo++, hi--) {
                char t = buf[lo];
                buf[lo] = buf[hi];
                buf[hi] = t;
            }
        }
        array_store_raw(p, buf, itemsize);
    }
}
#endif

/*[clinic input]
@critical_section
array.array.byteswap

Byteswap all items of the array.

If the items in the array are not 1, 2, 4, 8 or 16 bytes in size,
RuntimeError is raised.  Note, that for complex types the order of
components (the real part, followed by imaginary part) is preserved.
[clinic start generated code]*/

static PyObject *
array_array_byteswap_impl(arrayobject *self)
/*[clinic end generated code: output=5f8236cbdf0d90b5 input=91e114b82e32c2b4]*/
{
    char *p;
    Py_ssize_t i;

#ifdef Py_GIL_DISABLED
    if (!array_is_exclusive(self)) {
        array_byteswap_shared(self);
        Py_RETURN_NONE;
    }
#endif

    switch (self->ob_descr->itemsize) {
    case 1:
        break;
    case 2:
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 2) {
            char p0 = p[0];
            p[0] = p[1];
            p[1] = p0;
        }
        break;
    case 4:
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 4) {
            char p0 = p[0];
            char p1 = p[1];
            p[0] = p[3];
            p[1] = p[2];
            p[2] = p1;
            p[3] = p0;
        }
        break;
    case 8:
        if (strcmp(self->ob_descr->typecode, "Zf") != 0) {
            for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 8) {
                char p0 = p[0];
                char p1 = p[1];
                char p2 = p[2];
                char p3 = p[3];
                p[0] = p[7];
                p[1] = p[6];
                p[2] = p[5];
                p[3] = p[4];
                p[4] = p3;
                p[5] = p2;
                p[6] = p1;
                p[7] = p0;
            }
        }
        else {
            for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 8) {
                char t0 = p[0];
                char t1 = p[1];
                p[0] = p[3];
                p[1] = p[2];
                p[2] = t1;
                p[3] = t0;
                t0 = p[4];
                t1 = p[5];
                p[4] = p[7];
                p[5] = p[6];
                p[6] = t1;
                p[7] = t0;
            }
        }
        break;
    case 16:
        assert(strcmp(self->ob_descr->typecode, "Zd") == 0);
        for (p = self->ob_item, i = Py_SIZE(self); --i >= 0; p += 16) {
            char t0 = p[0];
            char t1 = p[1];
            char t2 = p[2];
            char t3 = p[3];
            p[0] = p[7];
            p[1] = p[6];
            p[2] = p[5];
            p[3] = p[4];
            p[4] = t3;
            p[5] = t2;
            p[6] = t1;
            p[7] = t0;
            t0 = p[8];
            t1 = p[9];
            t2 = p[10];
            t3 = p[11];
            p[8] = p[15];
            p[9] = p[14];
            p[10] = p[13];
            p[11] = p[12];
            p[12] = t3;
            p[13] = t2;
            p[14] = t1;
            p[15] = t0;
        }
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError,
                   "don't know how to byteswap this array type");
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
array.array.reverse

Reverse the order of the items in the array.
[clinic start generated code]*/

static PyObject *
array_array_reverse_impl(arrayobject *self)
/*[clinic end generated code: output=c04868b36f6f4089 input=e3947e98aed068ed]*/
{
    int itemsize = self->ob_descr->itemsize;
    Py_ssize_t n = Py_SIZE(self);
    char *items = self->ob_item;
    /* little buffers to hold items while swapping */
    _Py_ALIGNED_DEF(8, char) a[16];
    _Py_ALIGNED_DEF(8, char) b[16];
    assert((size_t)itemsize <= sizeof(a));

    for (Py_ssize_t i = 0, j = n - 1; i < j; i++, j--) {
        array_load_raw(items + i * itemsize, a, itemsize);
        array_load_raw(items + j * itemsize, b, itemsize);
        array_store_raw(items + i * itemsize, b, itemsize);
        array_store_raw(items + j * itemsize, a, itemsize);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
@permit_long_summary
array.array.fromfile

    cls: defining_class
    f: object
    n: Py_ssize_t(allow_negative=False)
    /

Read n objects from the file object f and append them to the end of the array.
[clinic start generated code]*/

static PyObject *
array_array_fromfile_impl(arrayobject *self, PyTypeObject *cls, PyObject *f,
                          Py_ssize_t n)
/*[clinic end generated code: output=83a667080b345ebc input=db46b06ac1b6de87]*/
{
    PyObject *b, *res;
    int itemsize = self->ob_descr->itemsize;
    Py_ssize_t nbytes;
    int not_enough_bytes;

    if (!array_size_valid(n, itemsize)) {
        PyErr_NoMemory();
        return NULL;
    }

    array_state *state = get_array_state_by_class(cls);
    assert(state != NULL);

    nbytes = n * itemsize;

    b = _PyObject_CallMethod(f, state->str_read, "n", nbytes);
    if (b == NULL)
        return NULL;

    if (!PyBytes_Check(b)) {
        PyErr_SetString(PyExc_TypeError,
                        "read() didn't return bytes");
        Py_DECREF(b);
        return NULL;
    }

    not_enough_bytes = (PyBytes_GET_SIZE(b) != nbytes);

    res = array_array_frombytes((PyObject *)self, b);
    Py_DECREF(b);
    if (res == NULL)
        return NULL;

    if (not_enough_bytes) {
        PyErr_SetString(PyExc_EOFError,
                        "read() didn't return enough bytes");
        Py_DECREF(res);
        return NULL;
    }

    return res;
}

/*[clinic input]
@critical_section
array.array.tofile

    cls: defining_class
    f: object
    /

Write all items (as machine values) to the file object f.
[clinic start generated code]*/

static PyObject *
array_array_tofile_impl(arrayobject *self, PyTypeObject *cls, PyObject *f)
/*[clinic end generated code: output=4560c628d9c18bc2 input=a26bc66df57864dd]*/
{
    int itemsize = self->ob_descr->itemsize;
    /* Write about 64K at a time, in whole items */
    /* XXX Make the block size settable */
    Py_ssize_t BLOCKITEMS = Py_MAX(1, 64*1024 / itemsize);

    array_state *state = get_array_state_by_class(cls);
    assert(state != NULL);

    // f.write() can run user code that resizes the array, so recheck the
    // size for each block.
    for (Py_ssize_t start = 0; start < Py_SIZE(self); start += BLOCKITEMS) {
        Py_ssize_t n = Py_MIN(BLOCKITEMS, Py_SIZE(self) - start);
        PyObject *bytes, *res;

        bytes = PyBytes_FromStringAndSize(NULL, n * itemsize);
        if (bytes == NULL)
            return NULL;
        array_copy_raw(PyBytes_AS_STRING(bytes),
                       self->ob_item + start * itemsize,
                       n, itemsize, array_is_exclusive(self));
        res = PyObject_CallMethodOneArg(f, state->str_write, bytes);
        Py_DECREF(bytes);
        if (res == NULL)
            return NULL;
        Py_DECREF(res); /* drop write result */
    }

    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section self list
array.array.fromlist

    list: object
    /

Append items to array from list.
[clinic start generated code]*/

static PyObject *
array_array_fromlist_impl(arrayobject *self, PyObject *list)
/*[clinic end generated code: output=6c23733a68dd68df input=c7c056aaf85d997a]*/
{
    Py_ssize_t n;

    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "arg must be list");
        return NULL;
    }
    n = PyList_GET_SIZE(list);
    if (n == 0) {
        Py_RETURN_NONE;
    }
    int itemsize = self->ob_descr->itemsize;
    if (!array_size_valid(n, itemsize)) {
        return PyErr_NoMemory();
    }
    /* Convert all items first: pack() can run user code. */
    char *buf = PyMem_Malloc(n * itemsize);
    if (buf == NULL) {
        return PyErr_NoMemory();
    }
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *v = PyList_GET_ITEM(list, i);
        if ((*self->ob_descr->pack)(v, buf + i * itemsize) != 0) {
            goto error;
        }
        if (n != PyList_GET_SIZE(list)) {
            PyErr_SetString(PyExc_RuntimeError,
                            "list changed size during iteration");
            goto error;
        }
    }
    Py_ssize_t old_size = Py_SIZE(self);
    if (n > PY_SSIZE_T_MAX - old_size ||
        !array_size_valid(old_size + n, itemsize)) {
        PyErr_NoMemory();
        goto error;
    }
    if (array_reserve(self, old_size + n) == -1) {
        goto error;
    }
    array_copy_raw(self->ob_item + old_size * itemsize, buf, n,
                   itemsize, array_is_exclusive(self));
    array_set_size(self, old_size + n);
    PyMem_Free(buf);
    Py_RETURN_NONE;

error:
    PyMem_Free(buf);
    return NULL;
}

/*[clinic input]
@critical_section
array.array.tolist

Convert array to an ordinary list with the same items.
[clinic start generated code]*/

static PyObject *
array_array_tolist_impl(arrayobject *self)
/*[clinic end generated code: output=00b60cc9eab8ef89 input=4543fdbac475c52c]*/
{
    PyObject *list = PyList_New(Py_SIZE(self));
    Py_ssize_t i;

    if (list == NULL)
        return NULL;
    for (i = 0; i < Py_SIZE(self); i++) {
        PyObject *v = getarrayitem(self, i);
        if (v == NULL)
            goto error;
        PyList_SET_ITEM(list, i, v);
    }
    return list;

error:
    Py_DECREF(list);
    return NULL;
}


/*[clinic input]
@critical_section
@permit_long_summary
array.array.frombytes

    buffer: Py_buffer
    /

Appends items from the string, interpreting it as an array of machine values, as if it had been read from a file using the fromfile() method.
[clinic start generated code]*/

static PyObject *
array_array_frombytes_impl(arrayobject *self, Py_buffer *buffer)
/*[clinic end generated code: output=d9842c8f7510a516 input=db8cf498a38b0210]*/
{
    int itemsize = self->ob_descr->itemsize;
    Py_ssize_t n;
    if (buffer->itemsize != 1) {
        PyErr_SetString(PyExc_TypeError, "a bytes-like object is required");
        return NULL;
    }
    n = buffer->len;
    if (n % itemsize != 0) {
        PyErr_SetString(PyExc_ValueError,
                   "bytes length not a multiple of item size");
        return NULL;
    }
    n = n / itemsize;
    if (n > 0) {
        Py_ssize_t old_size = Py_SIZE(self);
        if ((n > PY_SSIZE_T_MAX - old_size) ||
            !array_size_valid(old_size + n, itemsize)) {
                return PyErr_NoMemory();
        }
        if (array_reserve(self, old_size + n) == -1) {
            return NULL;
        }
        char *dst = self->ob_item + old_size * itemsize;
        if (array_is_exclusive(self)) {
            memcpy(dst, buffer->buf, n * itemsize);
        }
        else {
            // buffer->buf may not be aligned; copy through a temporary.
            char *tmp = PyMem_Malloc(n * itemsize);
            if (tmp == NULL) {
                return PyErr_NoMemory();
            }
            memcpy(tmp, buffer->buf, n * itemsize);
            array_copy_raw(dst, tmp, n, itemsize, false);
            PyMem_Free(tmp);
        }
        array_set_size(self, old_size + n);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
@permit_long_summary
array.array.tobytes

Convert the array to an array of machine values and return the bytes representation.
[clinic start generated code]*/

static PyObject *
array_array_tobytes_impl(arrayobject *self)
/*[clinic end generated code: output=87318e4edcdc2bb6 input=15e6fd44fed32743]*/
{
    Py_ssize_t n = Py_SIZE(self);
    int itemsize = self->ob_descr->itemsize;
    PyObject *bytes = PyBytes_FromStringAndSize(NULL, n * itemsize);
    if (bytes == NULL) {
        return NULL;
    }
    if (n > 0) {
        array_copy_raw(PyBytes_AS_STRING(bytes), self->ob_item, n,
                       itemsize, array_is_exclusive(self));
    }
    return bytes;
}

/*[clinic input]
@critical_section
array.array.fromunicode

    ustr: unicode
    /

Extends this array with data from the unicode string ustr.

The array must be a unicode type array; otherwise a ValueError is
raised.  Use array.frombytes(ustr.encode(...)) to append Unicode
data to an array of some other type.
[clinic start generated code]*/

static PyObject *
array_array_fromunicode_impl(arrayobject *self, PyObject *ustr)
/*[clinic end generated code: output=24359f5e001a7f2b input=f92d1b5af53561b0]*/
{
    const char *typecode = self->ob_descr->typecode;
    if (strcmp(typecode, "w") != 0) {
        PyErr_SetString(PyExc_ValueError,
            "fromunicode() may only be called on "
            "unicode type ('w') arrays");
        return NULL;
    }

    Py_ssize_t ustr_length = PyUnicode_GetLength(ustr);
    Py_ssize_t old_size = Py_SIZE(self);
    Py_ssize_t new_size = old_size + ustr_length;

    if (new_size < 0 || (size_t)new_size > PY_SSIZE_T_MAX / sizeof(Py_UCS4)) {
        return PyErr_NoMemory();
    }
    if (ustr_length == 0) {
        Py_RETURN_NONE;
    }
    if (array_reserve(self, new_size) == -1) {
        return NULL;
    }

    Py_UCS4 *dst = ((Py_UCS4 *)self->ob_item) + old_size;
    if (array_is_exclusive(self)) {
        // must not fail
        Py_UCS4 *u = PyUnicode_AsUCS4(ustr, dst, ustr_length, 0);
        assert(u != NULL);
        (void)u; // Suppress unused_variable warning.
    }
    else {
        Py_UCS4 *tmp = PyUnicode_AsUCS4Copy(ustr);
        if (tmp == NULL) {
            return NULL;
        }
        array_copy_raw((char *)dst, (char *)tmp, ustr_length,
                       sizeof(Py_UCS4), false);
        PyMem_Free(tmp);
    }
    array_set_size(self, new_size);

    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
array.array.tounicode

Extends this array with data from the unicode string ustr.

Convert the array to a unicode string.  The array must be a unicode
type array; otherwise a ValueError is raised.  Use
array.tobytes().decode() to obtain a unicode string from an array of
some other type.
[clinic start generated code]*/

static PyObject *
array_array_tounicode_impl(arrayobject *self)
/*[clinic end generated code: output=08e442378336e1ef input=0bcded7ac2cde2af]*/
{
    const char *typecode = self->ob_descr->typecode;
    if (strcmp(typecode, "w") != 0) {
        PyErr_SetString(PyExc_ValueError,
             "tounicode() may only be called on unicode type ('w') arrays");
        return NULL;
    }
    int byteorder = 0; // native byteorder
    Py_ssize_t n = Py_SIZE(self);
    if (n == 0 || array_is_exclusive(self)) {
        return PyUnicode_DecodeUTF32((const char *)self->ob_item,
                                     n * 4, NULL, &byteorder);
    }
    char *tmp = PyMem_Malloc(n * 4);
    if (tmp == NULL) {
        return PyErr_NoMemory();
    }
    array_copy_raw(tmp, self->ob_item, n, 4, false);
    PyObject *res = PyUnicode_DecodeUTF32(tmp, n * 4, NULL, &byteorder);
    PyMem_Free(tmp);
    return res;
}

/*[clinic input]
@critical_section
array.array.__sizeof__

Size of the array in memory, in bytes.
[clinic start generated code]*/

static PyObject *
array_array___sizeof___impl(arrayobject *self)
/*[clinic end generated code: output=d8e1c61ebbe3eaed input=e88bfd334166dcb7]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    res += (size_t)self->allocated * (size_t)self->ob_descr->itemsize;
    return PyLong_FromSize_t(res);
}


/*********************** Pickling support ************************/

static const struct mformatdescr {
    size_t size;
    int is_signed;
    int is_big_endian;
} mformat_descriptors[] = {
    {1, 0, 0},                  /* 0: UNSIGNED_INT8 */
    {1, 1, 0},                  /* 1: SIGNED_INT8 */
    {2, 0, 0},                  /* 2: UNSIGNED_INT16_LE */
    {2, 0, 1},                  /* 3: UNSIGNED_INT16_BE */
    {2, 1, 0},                  /* 4: SIGNED_INT16_LE */
    {2, 1, 1},                  /* 5: SIGNED_INT16_BE */
    {4, 0, 0},                  /* 6: UNSIGNED_INT32_LE */
    {4, 0, 1},                  /* 7: UNSIGNED_INT32_BE */
    {4, 1, 0},                  /* 8: SIGNED_INT32_LE */
    {4, 1, 1},                  /* 9: SIGNED_INT32_BE */
    {8, 0, 0},                  /* 10: UNSIGNED_INT64_LE */
    {8, 0, 1},                  /* 11: UNSIGNED_INT64_BE */
    {8, 1, 0},                  /* 12: SIGNED_INT64_LE */
    {8, 1, 1},                  /* 13: SIGNED_INT64_BE */
    {4, 0, 0},                  /* 14: IEEE_754_FLOAT_LE */
    {4, 0, 1},                  /* 15: IEEE_754_FLOAT_BE */
    {8, 0, 0},                  /* 16: IEEE_754_DOUBLE_LE */
    {8, 0, 1},                  /* 17: IEEE_754_DOUBLE_BE */
    {4, 0, 0},                  /* 18: UTF16_LE */
    {4, 0, 1},                  /* 19: UTF16_BE */
    {8, 0, 0},                  /* 20: UTF32_LE */
    {8, 0, 1},                  /* 21: UTF32_BE */
    {8, 0, 0},                  /* 22: IEEE_754_FLOAT_COMPLEX_LE */
    {8, 0, 1},                  /* 23: IEEE_754_FLOAT_COMPLEX_BE */
    {16, 0, 0},                 /* 24: IEEE_754_DOUBLE_COMPLEX_LE */
    {16, 0, 1},                 /* 25: IEEE_754_DOUBLE_COMPLEX_BE */
    {2, 0, 0},                  /* 26: IEEE_754_FLOAT16_LE */
    {2, 0, 1}                   /* 27: IEEE_754_FLOAT16_BE */
};


/*
 * Internal: This function is used to find the machine format of a given
 * array type code. This returns UNKNOWN_FORMAT when the machine format cannot
 * be found.
 */
static enum machine_format_code
typecode_to_mformat_code(const char *typecode)
{
    const int is_big_endian = PY_BIG_ENDIAN;

    size_t intsize;
    int is_signed;

    switch (typecode[0]) {
    case 'b':
        return SIGNED_INT8;
    case 'B':
        return UNSIGNED_INT8;

    case 'w':
        return UTF32_LE + is_big_endian;

    case 'e':
        return _PY_FLOAT_BIG_ENDIAN ? IEEE_754_FLOAT16_BE : IEEE_754_FLOAT16_LE;

    case 'f':
        return _PY_FLOAT_BIG_ENDIAN ? IEEE_754_FLOAT_BE : IEEE_754_FLOAT_LE;

    case 'd':
        return _PY_FLOAT_BIG_ENDIAN ? IEEE_754_DOUBLE_BE : IEEE_754_DOUBLE_LE;

    case 'Z': {
        switch (typecode[1]) {
        case 'f':
            return _PY_FLOAT_BIG_ENDIAN ? \
                IEEE_754_FLOAT_COMPLEX_BE : IEEE_754_FLOAT_COMPLEX_LE;

        case 'd':
            return _PY_FLOAT_BIG_ENDIAN ? \
                IEEE_754_DOUBLE_COMPLEX_BE : IEEE_754_DOUBLE_COMPLEX_LE;

        default:
            return UNKNOWN_FORMAT;
        }
    }

    /* Integers */
    case 'h':
        intsize = sizeof(short);
        is_signed = 1;
        break;
    case 'H':
        intsize = sizeof(short);
        is_signed = 0;
        break;
    case 'i':
        intsize = sizeof(int);
        is_signed = 1;
        break;
    case 'I':
        intsize = sizeof(int);
        is_signed = 0;
        break;
    case 'l':
        intsize = sizeof(long);
        is_signed = 1;
        break;
    case 'L':
        intsize = sizeof(long);
        is_signed = 0;
        break;
    case 'q':
        intsize = sizeof(long long);
        is_signed = 1;
        break;
    case 'Q':
        intsize = sizeof(long long);
        is_signed = 0;
        break;
    default:
        return UNKNOWN_FORMAT;
    }
    switch (intsize) {
    case 2:
        return UNSIGNED_INT16_LE + is_big_endian + (2 * is_signed);
    case 4:
        return UNSIGNED_INT32_LE + is_big_endian + (2 * is_signed);
    case 8:
        return UNSIGNED_INT64_LE + is_big_endian + (2 * is_signed);
    default:
        return UNKNOWN_FORMAT;
    }
}

/* Forward declaration. */
static PyObject *array_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

/*
 * Internal: This function wraps the array constructor--i.e., array_new()--to
 * allow the creation of array objects from C code without having to deal
 * directly the tuple argument of array_new(). The typecode argument is a
 * string, like "i" or "f" for example, representing an array
 * type code. The items argument is a bytes or a list object from which
 * contains the initial value of the array.
 *
 * On success, this functions returns the array object created. Otherwise,
 * NULL is returned to indicate a failure.
 */
static PyObject *
make_array(PyTypeObject *arraytype, const char *typecode, PyObject *items)
{
    PyObject *new_args;
    PyObject *array_obj;
    PyObject *typecode_obj;

    assert(arraytype != NULL);
    assert(items != NULL);

    typecode_obj = PyUnicode_FromString(typecode);
    if (typecode_obj == NULL)
        return NULL;

    new_args = _PyTuple_FromPairSteal(typecode_obj, Py_NewRef(items));
    if (new_args == NULL) {
        return NULL;
    }

    array_obj = array_new(arraytype, new_args, NULL);
    Py_DECREF(new_args);
    if (array_obj == NULL)
        return NULL;

    return array_obj;
}

/*
 * This functions is a special constructor used when unpickling an array. It
 * provides a portable way to rebuild an array from its memory representation.
 */
/*[clinic input]
array._array_reconstructor

    arraytype: object(type="PyTypeObject *")
    typecode: str
    mformat_code: int(type="enum machine_format_code")
    items: object
    /

Internal. Used for pickling support.
[clinic start generated code]*/

static PyObject *
array__array_reconstructor_impl(PyObject *module, PyTypeObject *arraytype,
                                const char *typecode,
                                enum machine_format_code mformat_code,
                                PyObject *items)
/*[clinic end generated code: output=723e3813e0a18b7b input=9f1c331baae742a6]*/
{
    array_state *state = get_array_state(module);
    PyObject *converted_items;
    PyObject *result;
    const struct arraydescr *descr;

    if (!PyType_Check(arraytype)) {
        PyErr_Format(PyExc_TypeError,
            "first argument must be a type object, not %.200s",
            Py_TYPE(arraytype)->tp_name);
        return NULL;
    }
    if (!PyType_IsSubtype(arraytype, state->ArrayType)) {
        PyErr_Format(PyExc_TypeError,
            "%.200s is not a subtype of %.200s",
            arraytype->tp_name, state->ArrayType->tp_name);
        return NULL;
    }
    for (descr = descriptors; descr->typecode[0] != 0; descr++) {
        if (strcmp(descr->typecode, typecode) == 0) {
            break;
        }
    }
    if (descr->typecode[0] == 0) {
        PyErr_SetString(PyExc_ValueError,
                        "second argument must be a valid type code");
        return NULL;
    }
    if (mformat_code < MACHINE_FORMAT_CODE_MIN ||
        mformat_code > MACHINE_FORMAT_CODE_MAX) {
        PyErr_SetString(PyExc_ValueError,
            "third argument must be a valid machine format code.");
        return NULL;
    }
    if (!PyBytes_Check(items)) {
        PyErr_Format(PyExc_TypeError,
            "fourth argument should be bytes, not %.200s",
            Py_TYPE(items)->tp_name);
        return NULL;
    }

    /* Fast path: No decoding has to be done. */
    if (mformat_code == typecode_to_mformat_code(typecode) ||
        mformat_code == UNKNOWN_FORMAT) {
        return make_array(arraytype, typecode, items);
    }

    /* Slow path: Decode the byte string according to the given machine
     * format code. This occurs when the computer unpickling the array
     * object is architecturally different from the one that pickled the
     * array.
     */
    if (Py_SIZE(items) % mformat_descriptors[mformat_code].size != 0) {
        PyErr_SetString(PyExc_ValueError,
                        "string length not a multiple of item size");
        return NULL;
    }
    switch (mformat_code) {
    case IEEE_754_FLOAT16_LE:
    case IEEE_754_FLOAT16_BE: {
        Py_ssize_t i;
        int le = (mformat_code == IEEE_754_FLOAT16_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 2;
        const char *memstr = PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pyfloat = PyFloat_FromDouble(
                PyFloat_Unpack2(&memstr[i * 2], le));
            if (pyfloat == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pyfloat);
        }
        break;
    }
    case IEEE_754_FLOAT_LE:
    case IEEE_754_FLOAT_BE: {
        Py_ssize_t i;
        int le = (mformat_code == IEEE_754_FLOAT_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 4;
        const char *memstr = PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pyfloat = PyFloat_FromDouble(
                PyFloat_Unpack4(&memstr[i * 4], le));
            if (pyfloat == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pyfloat);
        }
        break;
    }
    case IEEE_754_DOUBLE_LE:
    case IEEE_754_DOUBLE_BE: {
        Py_ssize_t i;
        int le = (mformat_code == IEEE_754_DOUBLE_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 8;
        const char *memstr = PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pyfloat = PyFloat_FromDouble(
                PyFloat_Unpack8(&memstr[i * 8], le));
            if (pyfloat == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pyfloat);
        }
        break;
    }
    case IEEE_754_FLOAT_COMPLEX_LE:
    case IEEE_754_FLOAT_COMPLEX_BE: {
        Py_ssize_t i;
        int le = (mformat_code == IEEE_754_FLOAT_COMPLEX_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 8;
        const char *memstr = PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL) {
            return NULL;
        }
        for (i = 0; i < itemcount; i++) {
            PyObject *pycomplex = PyComplex_FromDoubles(
                PyFloat_Unpack4(&memstr[i * 8], le),
                PyFloat_Unpack4(&memstr[i * 8] + 4, le));
            if (pycomplex == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pycomplex);
        }
        break;
    }
    case IEEE_754_DOUBLE_COMPLEX_LE:
    case IEEE_754_DOUBLE_COMPLEX_BE: {
        Py_ssize_t i;
        int le = (mformat_code == IEEE_754_DOUBLE_COMPLEX_LE) ? 1 : 0;
        Py_ssize_t itemcount = Py_SIZE(items) / 16;
        const char *memstr = PyBytes_AS_STRING(items);

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL) {
            return NULL;
        }
        for (i = 0; i < itemcount; i++) {
            PyObject *pycomplex = PyComplex_FromDoubles(
                PyFloat_Unpack8(&memstr[i * 16], le),
                PyFloat_Unpack8(&memstr[i * 16] + 8, le));
            if (pycomplex == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pycomplex);
        }
        break;
    }
    case UTF16_LE:
    case UTF16_BE: {
        int byteorder = (mformat_code == UTF16_LE) ? -1 : 1;
        converted_items = PyUnicode_DecodeUTF16(
            PyBytes_AS_STRING(items), Py_SIZE(items),
            "strict", &byteorder);
        if (converted_items == NULL)
            return NULL;
        break;
    }
    case UTF32_LE:
    case UTF32_BE: {
        int byteorder = (mformat_code == UTF32_LE) ? -1 : 1;
        converted_items = PyUnicode_DecodeUTF32(
            PyBytes_AS_STRING(items), Py_SIZE(items),
            "strict", &byteorder);
        if (converted_items == NULL)
            return NULL;
        break;
    }

    case UNSIGNED_INT8:
    case SIGNED_INT8:
    case UNSIGNED_INT16_LE:
    case UNSIGNED_INT16_BE:
    case SIGNED_INT16_LE:
    case SIGNED_INT16_BE:
    case UNSIGNED_INT32_LE:
    case UNSIGNED_INT32_BE:
    case SIGNED_INT32_LE:
    case SIGNED_INT32_BE:
    case UNSIGNED_INT64_LE:
    case UNSIGNED_INT64_BE:
    case SIGNED_INT64_LE:
    case SIGNED_INT64_BE: {
        Py_ssize_t i;
        const struct mformatdescr mf_descr =
            mformat_descriptors[mformat_code];
        Py_ssize_t itemcount = Py_SIZE(items) / mf_descr.size;
        const unsigned char *memstr =
            (unsigned char *)PyBytes_AS_STRING(items);
        const struct arraydescr *descr;

        /* If possible, try to pack array's items using a data type
         * that fits better. This may result in an array with narrower
         * or wider elements.
         *
         * For example, if a 32-bit machine pickles an L-code array of
         * unsigned longs, then the array will be unpickled by 64-bit
         * machine as an I-code array of unsigned ints.
         *
         * XXX: Is it possible to write a unit test for this?
         */
        for (descr = descriptors; descr->typecode[0] != 0; descr++) {
            if (descr->is_integer_type &&
                (size_t)descr->itemsize == mf_descr.size &&
                descr->is_signed == mf_descr.is_signed)
            {
                typecode = descr->typecode;
            }
        }

        converted_items = PyList_New(itemcount);
        if (converted_items == NULL)
            return NULL;
        for (i = 0; i < itemcount; i++) {
            PyObject *pylong;

            pylong = _PyLong_FromByteArray(
                &memstr[i * mf_descr.size],
                mf_descr.size,
                !mf_descr.is_big_endian,
                mf_descr.is_signed);
            if (pylong == NULL) {
                Py_DECREF(converted_items);
                return NULL;
            }
            PyList_SET_ITEM(converted_items, i, pylong);
        }
        break;
    }
    case UNKNOWN_FORMAT:
        /* Impossible, but needed to shut up GCC about the unhandled
         * enumeration value.
         */
    default:
        PyErr_BadArgument();
        return NULL;
    }

    result = make_array(arraytype, typecode, converted_items);
    Py_DECREF(converted_items);
    return result;
}

/*[clinic input]
@critical_section
array.array.__reduce_ex__

    cls: defining_class
    value: object
    /

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
array_array___reduce_ex___impl(arrayobject *self, PyTypeObject *cls,
                               PyObject *value)
/*[clinic end generated code: output=4958ee5d79452ad5 input=18c90a4cad7ac527]*/
{
    PyObject *dict;
    PyObject *result;
    PyObject *array_str;
    const char *typecode = self->ob_descr->typecode;
    int mformat_code;
    long protocol;

    array_state *state = get_array_state_by_class(cls);
    assert(state != NULL);

    PyObject *array_reconstructor =
        FT_ATOMIC_LOAD_PTR_ACQUIRE(state->array_reconstructor);
    if (array_reconstructor == NULL) {
        array_reconstructor = PyImport_ImportModuleAttrString(
            "array", "_array_reconstructor");
        if (array_reconstructor == NULL) {
            return NULL;
        }
#ifdef Py_GIL_DISABLED
        PyObject *expected = NULL;
        if (!_Py_atomic_compare_exchange_ptr(&state->array_reconstructor,
                                             &expected, array_reconstructor)) {
            // Another thread stored it first.
            Py_DECREF(array_reconstructor);
            array_reconstructor = expected;
        }
#else
        state->array_reconstructor = array_reconstructor;
#endif
    }

    if (!PyLong_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__reduce_ex__ argument should be an integer");
        return NULL;
    }
    protocol = PyLong_AsLong(value);
    if (protocol == -1 && PyErr_Occurred())
        return NULL;

    if (PyObject_GetOptionalAttr((PyObject *)self, state->str___dict__, &dict) < 0) {
        return NULL;
    }
    if (dict == NULL) {
        dict = Py_NewRef(Py_None);
    }

    mformat_code = typecode_to_mformat_code(typecode);
    if (mformat_code == UNKNOWN_FORMAT || protocol < 3) {
        /* Convert the array to a list if we got something weird
         * (e.g., non-IEEE floats), or we are pickling the array using
         * a Python 2.x compatible protocol.
         *
         * It is necessary to use a list representation for Python 2.x
         * compatible pickle protocol, since Python 2's str objects
         * are unpickled as unicode by Python 3. Thus it is impossible
         * to make arrays unpicklable by Python 3 by using their memory
         * representation, unless we resort to ugly hacks such as
         * coercing unicode objects to bytes in array_reconstructor.
         */
        PyObject *list;
        list = array_array_tolist_impl(self);
        if (list == NULL) {
            Py_DECREF(dict);
            return NULL;
        }
        result = Py_BuildValue(
            "O(sO)O", Py_TYPE(self), typecode, list, dict);
        Py_DECREF(list);
        Py_DECREF(dict);
        return result;
    }

    array_str = array_array_tobytes_impl(self);
    if (array_str == NULL) {
        Py_DECREF(dict);
        return NULL;
    }

    assert(array_reconstructor != NULL);
    result = Py_BuildValue(
        "O(OsiN)O", array_reconstructor, Py_TYPE(self), typecode,
        mformat_code, array_str, dict);
    Py_DECREF(dict);
    return result;
}

static PyObject *
array_get_typecode(PyObject *op, void *Py_UNUSED(closure))
{
    arrayobject *a = arrayobject_CAST(op);
    const char *typecode = a->ob_descr->typecode;
    return PyUnicode_FromString(typecode);
}

static PyObject *
array_get_itemsize(PyObject *op, void *Py_UNUSED(closure))
{
    arrayobject *a = arrayobject_CAST(op);
    return PyLong_FromLong((long)a->ob_descr->itemsize);
}

static PyGetSetDef array_getsets [] = {
    {"typecode", array_get_typecode, NULL,
     "the typecode character used to create the array"},
    {"itemsize", array_get_itemsize, NULL,
     "the size, in bytes, of one array item"},
    {NULL}
};

static PyMethodDef array_methods[] = {
    ARRAY_ARRAY_APPEND_METHODDEF
    ARRAY_ARRAY_BUFFER_INFO_METHODDEF
    ARRAY_ARRAY_BYTESWAP_METHODDEF
    ARRAY_ARRAY_CLEAR_METHODDEF
    ARRAY_ARRAY___COPY___METHODDEF
    ARRAY_ARRAY_COUNT_METHODDEF
    ARRAY_ARRAY___DEEPCOPY___METHODDEF
    ARRAY_ARRAY_EXTEND_METHODDEF
    ARRAY_ARRAY_FROMFILE_METHODDEF
    ARRAY_ARRAY_FROMLIST_METHODDEF
    ARRAY_ARRAY_FROMBYTES_METHODDEF
    ARRAY_ARRAY_FROMUNICODE_METHODDEF
    ARRAY_ARRAY_INDEX_METHODDEF
    ARRAY_ARRAY_INSERT_METHODDEF
    ARRAY_ARRAY_POP_METHODDEF
    ARRAY_ARRAY___REDUCE_EX___METHODDEF
    ARRAY_ARRAY_REMOVE_METHODDEF
    ARRAY_ARRAY_REVERSE_METHODDEF
    ARRAY_ARRAY_TOFILE_METHODDEF
    ARRAY_ARRAY_TOLIST_METHODDEF
    ARRAY_ARRAY_TOBYTES_METHODDEF
    ARRAY_ARRAY_TOUNICODE_METHODDEF
    ARRAY_ARRAY___SIZEOF___METHODDEF
    {"__class_getitem__", Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("Arrays are generic over the type of their elements")},
    {NULL, NULL}  /* sentinel */
};

static PyObject *
array_repr_lock_held(PyObject *op)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    const char *typecode;
    PyObject *s, *v = NULL;
    Py_ssize_t len;
    arrayobject *a = arrayobject_CAST(op);

    len = Py_SIZE(a);
    typecode = a->ob_descr->typecode;
    if (len == 0) {
        return PyUnicode_FromFormat("%s('%s')",
                                    _PyType_Name(Py_TYPE(a)), typecode);
    }
    if (strcmp(typecode, "w") == 0) {
        v = array_array_tounicode_impl(a);
    } else {
        v = array_array_tolist_impl(a);
    }
    if (v == NULL)
        return NULL;

    s = PyUnicode_FromFormat("%s('%s', %R)",
                             _PyType_Name(Py_TYPE(a)), typecode, v);
    Py_DECREF(v);
    return s;
}

static PyObject *
array_repr(PyObject *op)
{
    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_repr_lock_held(op);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static PyObject*
array_subscr_slice_lock_held(PyObject *op, PyObject *item)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    arrayobject *self = arrayobject_CAST(op);
    array_state *state = find_array_state_by_type(Py_TYPE(self));

    if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, i;
        size_t cur;
        PyObject* result;
        arrayobject* ar;
        int itemsize = self->ob_descr->itemsize;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);

        if (slicelength <= 0) {
            return newarrayobject(state->ArrayType, 0, self->ob_descr);
        }
        else if (step == 1) {
            PyObject *result = newarrayobject_untracked(state->ArrayType, slicelength, self->ob_descr);
            if (result == NULL)
                return NULL;
            array_copy_raw(((arrayobject *)result)->ob_item,
                           self->ob_item + start * itemsize,
                           slicelength, itemsize, array_is_exclusive(self));
            _PyObject_GC_TRACK(result);
            return result;
        }
        else {
            result = newarrayobject_untracked(state->ArrayType, slicelength, self->ob_descr);
            if (!result) return NULL;

            ar = (arrayobject*)result;

            bool exclusive = array_is_exclusive(self);
            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                array_copy_raw(ar->ob_item + i*itemsize,
                               self->ob_item + cur*itemsize,
                               1, itemsize, exclusive);
            }

            _PyObject_GC_TRACK(result);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "array indices must be integers");
        return NULL;
    }
}

static PyObject *
array_subscr(PyObject *op, PyObject *item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i==-1 && PyErr_Occurred()) {
            return NULL;
        }
        Py_ssize_t size = array_get_size(op);
        if (i < 0) {
            i += size;
        }
        return array_item(op, i);
    }

    PyObject *ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_subscr_slice_lock_held(op, item);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static int
array_ass_subscr_lock_held(PyObject *op, PyObject* item, PyObject* value)
{
    array_state* state = find_array_state_by_type(Py_TYPE(op));
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
#ifdef Py_DEBUG
    if (value != NULL && array_Check(value, state)) {
        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(value);
    }
#endif
    arrayobject *self = arrayobject_CAST(op);
    Py_ssize_t start, stop, step, slicelength, needed;
    arrayobject* other;
    int itemsize;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);

        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += Py_SIZE(self);
        if (i < 0 || i >= Py_SIZE(self)) {
            PyErr_SetString(PyExc_IndexError,
                "array assignment index out of range");
            return -1;
        }
        // array_ass_subscr() handles a[i] = value without the lock.
        assert(value == NULL);
        /* Fall through to slice assignment */
        start = i;
        stop = i + 1;
        step = 1;
        slicelength = 1;
    }
    else if (PySlice_Check(item)) {
        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelength = PySlice_AdjustIndices(Py_SIZE(self), &start, &stop,
                                            step);
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "array indices must be integers");
        return -1;
    }
    if (value == NULL) {
        other = NULL;
        needed = 0;
    }
    else if (array_Check(value, state)) {
        other = (arrayobject *)value;
        needed = Py_SIZE(other);
        if (self == other) {
            /* Special case "self[i:j] = self" -- copy self first */
            int ret;
            value = array_slice(other, 0, needed);
            if (value == NULL)
                return -1;
            ret = array_ass_subscr_lock_held(op, item, value);
            Py_DECREF(value);
            return ret;
        }
        if (other->ob_descr != self->ob_descr) {
            PyErr_BadArgument();
            return -1;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
         "can only assign array (not \"%.200s\") to array slice",
                         Py_TYPE(value)->tp_name);
        return -1;
    }
    itemsize = self->ob_descr->itemsize;
    /* for 'a[2:1] = ...', the insertion point is 'start', not 'stop' */
    if ((step > 0 && stop < start) ||
        (step < 0 && stop > start))
        stop = start;

    /* Issue #4509: If the array has exported buffers and the slice
       assignment would change the size of the array, fail early to make
       sure we don't modify it. */
    if ((needed == 0 || slicelength != needed) && self->ob_exports > 0) {
        PyErr_SetString(PyExc_BufferError,
            "cannot resize an array that is exporting buffers");
        return -1;
    }

    bool exclusive = array_is_exclusive(self);
    if (step == 1) {
        Py_ssize_t size = Py_SIZE(self);
        Py_ssize_t newsize = size + needed - slicelength;
        if (slicelength > needed) {
            array_copy_raw(self->ob_item + (start + needed) * itemsize,
                           self->ob_item + stop * itemsize,
                           size - stop, itemsize, exclusive);
            if (array_resize(self, newsize) < 0)
                return -1;
        }
        else if (slicelength < needed) {
            if (array_reserve(self, newsize) < 0)
                return -1;
            array_copy_raw(self->ob_item + (start + needed) * itemsize,
                           self->ob_item + stop * itemsize,
                           size - stop, itemsize, exclusive);
        }
        if (needed > 0) {
            array_copy_raw(self->ob_item + start * itemsize,
                           other->ob_item, needed, itemsize,
                           exclusive && array_is_exclusive(other));
        }
        if (newsize > size) {
            array_set_size(self, newsize);
        }
        return 0;
    }
    else if (needed == 0) {
        /* Delete slice */
        size_t cur;
        Py_ssize_t i;

        if (step < 0) {
            stop = start + 1;
            start = stop + step * (slicelength - 1) - 1;
            step = -step;
        }
        for (cur = start, i = 0; i < slicelength;
             cur += step, i++) {
            Py_ssize_t lim = step - 1;

            if (cur + step >= (size_t)Py_SIZE(self))
                lim = Py_SIZE(self) - cur - 1;
            array_copy_raw(self->ob_item + (cur - i) * itemsize,
                           self->ob_item + (cur + 1) * itemsize,
                           lim, itemsize, exclusive);
        }
        cur = start + (size_t)slicelength * step;
        if (cur < (size_t)Py_SIZE(self)) {
            array_copy_raw(self->ob_item + (cur-slicelength) * itemsize,
                           self->ob_item + cur * itemsize,
                           Py_SIZE(self) - cur, itemsize, exclusive);
        }
        if (array_resize(self, Py_SIZE(self) - slicelength) < 0)
            return -1;
        return 0;
    }
    else {
        size_t cur;
        Py_ssize_t i;

        if (needed != slicelength) {
            PyErr_Format(PyExc_ValueError,
                "attempt to assign array of size %zd "
                "to extended slice of size %zd",
                needed, slicelength);
            return -1;
        }
        exclusive = exclusive && array_is_exclusive(other);
        for (cur = start, i = 0; i < slicelength;
             cur += step, i++) {
            array_copy_raw(self->ob_item + cur * itemsize,
                           other->ob_item + i * itemsize,
                           1, itemsize, exclusive);
        }
        return 0;
    }
}

static int
array_ass_subscr(PyObject *op, PyObject* item, PyObject* value)
{
    int ret;

    if (value != NULL) {
        if (PyIndex_Check(item)) {
            // a[i] = value
            Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
            if (i == -1 && PyErr_Occurred())
                return -1;
            if (i < 0)
                i += array_get_size(op);
            return setarrayitem_threadsafe(op, i, value);
        }
        array_state* state = find_array_state_by_type(Py_TYPE(op));
        if (array_Check(value, state)) {
            Py_BEGIN_CRITICAL_SECTION2(op, value);
            ret = array_ass_subscr_lock_held(op, item, value);
            Py_END_CRITICAL_SECTION2();
            return ret;
        }
    }

    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_ass_subscr_lock_held(op, item, value);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static const _Py_ALIGNED_DEF(ALIGNOF_MAX_ALIGN_T, char) emptybuf[] = "";

static int
array_buffer_getbuf_lock_held(PyObject *op, Py_buffer *view, int flags)
{
    _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(op);
    arrayobject *self = arrayobject_CAST(op);
    if (view == NULL) {
        PyErr_SetString(PyExc_BufferError,
            "array_buffer_getbuf: view==NULL argument is obsolete");
        return -1;
    }

    view->buf = (void *)self->ob_item;
    view->obj = Py_NewRef(self);
    if (view->buf == NULL)
        view->buf = (void *)emptybuf;
    view->len = Py_SIZE(self) * self->ob_descr->itemsize;
    view->readonly = 0;
    view->ndim = 1;
    view->itemsize = self->ob_descr->itemsize;
    view->suboffsets = NULL;
    view->shape = NULL;
    if ((flags & PyBUF_ND)==PyBUF_ND) {
        view->shape = &((PyVarObject*)self)->ob_size;
    }
    view->strides = NULL;
    if ((flags & PyBUF_STRIDES)==PyBUF_STRIDES)
        view->strides = &(view->itemsize);
    view->format = NULL;
    view->internal = NULL;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
        view->format = (char *)self->ob_descr->typecode;
    }

    self->ob_exports++;
    return 0;
}

static int
array_buffer_getbuf(PyObject *op, Py_buffer *view, int flags)
{
    int ret;
    Py_BEGIN_CRITICAL_SECTION(op);
    ret = array_buffer_getbuf_lock_held(op, view, flags);
    Py_END_CRITICAL_SECTION();
    return ret;
}

static void
array_buffer_relbuf(PyObject *op, Py_buffer *Py_UNUSED(view))
{
    Py_BEGIN_CRITICAL_SECTION(op);
    arrayobject *self = arrayobject_CAST(op);
    self->ob_exports--;
    assert(self->ob_exports >= 0);
    Py_END_CRITICAL_SECTION();
}

static PyObject *
array_new_internal_lock_held(PyTypeObject *type, PyObject *initial,
                             const char *s)
{
#ifdef Py_DEBUG
    if (initial != NULL) {
        _Py_CRITICAL_SECTION_ASSERT_OBJECT_LOCKED(initial);
    }
#endif
    array_state *state = find_array_state_by_type(type);
    PyObject *it = NULL;
    const struct arraydescr *descr;
    bool is_unicode = (strcmp(s, "w") == 0);

    if (!(initial == NULL || PyList_Check(initial)
          || PyByteArray_Check(initial)
          || PyBytes_Check(initial)
          || PyTuple_Check(initial)
          || (is_unicode && PyUnicode_Check(initial))
          || (array_Check(initial, state)
              && strcmp(s, ((arrayobject*)initial)->ob_descr->typecode) == 0))) {
        it = PyObject_GetIter(initial);
        if (it == NULL)
            return NULL;
        /* We set initial to NULL so that the subsequent code
           will create an empty array of the appropriate type
           and afterwards we can use array_iter_extend to populate
           the array.
        */
        initial = NULL;
    }
    for (descr = descriptors; descr->typecode[0] != 0; descr++) {
        if (strcmp(descr->typecode, s) == 0) {
            PyObject *a;
            Py_ssize_t len;

            if (initial == NULL)
                len = 0;
            else if (PyList_Check(initial))
                len = PyList_GET_SIZE(initial);
            else if (PyTuple_Check(initial) || array_Check(initial, state))
                len = Py_SIZE(initial);
            else
                len = 0;

            a = newarrayobject_untracked(type, len, descr);
            if (a == NULL) {
                Py_XDECREF(it);
                return NULL;
            }

            if (len > 0 && !array_Check(initial, state)) {
                Py_ssize_t i;
                arrayobject *ap = arrayobject_CAST(a);
                for (i = 0; i < len; i++) {
                    PyObject *v =
                        PySequence_GetItem(initial, i);
                    if (v == NULL) {
                        Py_DECREF(a);
                        Py_XDECREF(it);
                        return NULL;
                    }
                    _Py_ALIGNED_DEF(8, char) buf[16];
                    if ((*descr->pack)(v, buf) != 0) {
                        Py_DECREF(v);
                        Py_DECREF(a);
                        Py_XDECREF(it);
                        return NULL;
                    }
                    // ap is private, so no lock is needed.
                    array_store_item(ap, i, buf);
                    Py_DECREF(v);
                }
            }
            else if (initial != NULL && (PyByteArray_Check(initial) ||
                               PyBytes_Check(initial))) {
                PyObject *v;
                v = array_array_frombytes((PyObject *)a, initial);
                if (v == NULL) {
                    Py_DECREF(a);
                    Py_XDECREF(it);
                    return NULL;
                }
                Py_DECREF(v);
            }
            else if (initial != NULL && PyUnicode_Check(initial))  {
                assert(strcmp(descr->typecode, "w") == 0);
                Py_ssize_t n = PyUnicode_GET_LENGTH(initial);
                if (n > 0) {
                    arrayobject *self = (arrayobject *)a;
                    assert(self->ob_item == NULL);
                    self->ob_item = array_items_alloc(n, sizeof(Py_UCS4));
                    if (self->ob_item == NULL) {
                        Py_DECREF(a);
                        Py_XDECREF(it);
                        return PyErr_NoMemory();
                    }
                    Py_UCS4 *u = PyUnicode_AsUCS4(initial,
                                                  (Py_UCS4 *)self->ob_item,
                                                  n, 0);
                    assert(u != NULL);
                    (void)u;
                    Py_SET_SIZE(self, n);
                    self->allocated = n;
                }
            }
            else if (initial != NULL && array_Check(initial, state) && len > 0) {
                arrayobject *self = (arrayobject *)a;
                arrayobject *other = (arrayobject *)initial;
                array_copy_raw(self->ob_item, other->ob_item, len,
                               other->ob_descr->itemsize,
                               array_is_exclusive(other));
            }
            if (it != NULL) {
                if (array_iter_extend((arrayobject *)a, it) == -1) {
                    Py_DECREF(it);
                    Py_DECREF(a);
                    return NULL;
                }
                Py_DECREF(it);
            }
            // Track only once fully built.
            _PyObject_GC_TRACK(a);
            return a;
        }
    }
    Py_XDECREF(it);

    PyObject *sep = PyUnicode_FromString(", ");
    if (sep == NULL) {
        return NULL;
    }
    PyObject *msg = PyObject_CallMethod(sep, "join", "(O)", state->typecodes);
    Py_DECREF(sep);
    if (msg == NULL) {
        return NULL;
    }
    PyErr_Format(PyExc_ValueError, "bad typecode (must be: %S)", msg);
    Py_DECREF(msg);
    return NULL;
}

static PyObject *
array_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    array_state *state = find_array_state_by_type(type);
    const char *s;
    PyObject *initial = NULL;

    if ((type == state->ArrayType ||
         type->tp_init == state->ArrayType->tp_init) &&
        !_PyArg_NoKeywords("array.array", kwds))
        return NULL;

    if (!PyArg_ParseTuple(args, "s|O:array", &s, &initial))
        return NULL;

    if (PySys_Audit("array.__new__", "sO",
                    s, initial ? initial : Py_None) < 0) {
        return NULL;
    }

    bool is_unicode = (strcmp(s, "w") == 0);

    if (initial && !is_unicode) {
        if (PyUnicode_Check(initial)) {
            PyErr_Format(PyExc_TypeError, "cannot use a str to initialize "
                         "an array with typecode '%s'", s);
            return NULL;
        }
        else if (array_Check(initial, state)) {
            const char *is = ((arrayobject*)initial)->ob_descr->typecode;
            if (strcmp(is, "w") == 0) {
                PyErr_Format(PyExc_TypeError, "cannot use a unicode array to "
                            "initialize an array with typecode '%s'", s);
                return NULL;
            }
        }
    }

    PyObject *ret;
    if (initial == NULL) {
        ret = array_new_internal_lock_held(type, initial, s);
    }
    else {
        Py_BEGIN_CRITICAL_SECTION(initial);
        ret = array_new_internal_lock_held(type, initial, s);
        Py_END_CRITICAL_SECTION();
    }
    return ret;
}


PyDoc_STRVAR(module_doc,
"This module defines an object type which can efficiently represent\n\
an array of basic values: characters, integers, floating-point\n\
numbers, complex numbers.  Arrays are sequence types and behave very much like lists,\n\
except that the type of objects stored in them is constrained.\n");

PyDoc_STRVAR(arraytype_doc,
"array(typecode [, initializer]) -> array\n\
\n\
Return a new array whose items are restricted by typecode, and\n\
initialized from the optional initializer value, which must be a list,\n\
string or iterable over elements of the appropriate type.\n\
\n\
Arrays represent basic values and behave very much like lists, except\n\
the type of objects stored in them is constrained. The type is specified\n\
at object creation time by using a type code, which is a string.\n\
The following type codes are defined:\n\
\n\
    Type code   C Type             Minimum size in bytes\n\
    'b'         signed integer     1\n\
    'B'         unsigned integer   1\n\
    'w'         Py_UCS4            4\n\
    'h'         signed integer     2\n\
    'H'         unsigned integer   2\n\
    'i'         signed integer     2\n\
    'I'         unsigned integer   2\n\
    'l'         signed integer     4\n\
    'L'         unsigned integer   4\n\
    'q'         signed integer     8 (see note)\n\
    'Q'         unsigned integer   8 (see note)\n\
    'e'         16-bit IEEE floats 2\n\
    'f'         floating-point     4\n\
    'd'         floating-point     8\n\
    'Zf'        float complex      8\n\
    'Zd'        double complex     16\n\
\n\
NOTE: The 'q' and 'Q' type codes are only available if the platform\n\
C compiler used to build Python supports 'long long', or, on Windows,\n\
'__int64'.\n\
\n\
Methods:\n\
\n\
append() -- append a new item to the end of the array\n\
buffer_info() -- return information giving the current memory info\n\
byteswap() -- byteswap all the items of the array\n\
count() -- return number of occurrences of an object\n\
extend() -- extend array by appending multiple elements from an iterable\n\
fromfile() -- read items from a file object\n\
fromlist() -- append items from the list\n\
frombytes() -- append items from the string\n\
index() -- return index of first occurrence of an object\n\
insert() -- insert a new item into the array at a provided position\n\
pop() -- remove and return item (default last)\n\
remove() -- remove first occurrence of an object\n\
reverse() -- reverse the order of the items in the array\n\
tofile() -- write all items to a file object\n\
tolist() -- return the array converted to an ordinary list\n\
tobytes() -- return the array converted to a string\n\
\n\
Attributes:\n\
\n\
typecode -- the typecode character used to create the array\n\
itemsize -- the length in bytes of one array item\n\
");

static PyObject *array_iter(PyObject *op);

static struct PyMemberDef array_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(arrayobject, weakreflist), Py_READONLY},
    {NULL},
};

static PyType_Slot array_slots[] = {
    {Py_tp_dealloc, array_dealloc},
    {Py_tp_repr, array_repr},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)arraytype_doc},
    {Py_tp_richcompare, array_richcompare},
    {Py_tp_iter, array_iter},
    {Py_tp_methods, array_methods},
    {Py_tp_members, array_members},
    {Py_tp_getset, array_getsets},
    {Py_tp_alloc, _PyType_AllocNoTrack},
    {Py_tp_new, array_new},
    {Py_tp_traverse, _PyObject_VisitType},

    /* as sequence */
    {Py_sq_length, array_length},
    {Py_sq_concat, array_concat},
    {Py_sq_repeat, array_repeat},
    {Py_sq_item, array_item},
    {Py_sq_ass_item, array_ass_item},
    {Py_sq_contains, array_contains},
    {Py_sq_inplace_concat, array_inplace_concat},
    {Py_sq_inplace_repeat, array_inplace_repeat},

    /* as mapping */
    {Py_mp_length, array_length},
    {Py_mp_subscript, array_subscr},
    {Py_mp_ass_subscript, array_ass_subscr},

    /* as buffer */
    {Py_bf_getbuffer, array_buffer_getbuf},
    {Py_bf_releasebuffer, array_buffer_relbuf},

    {0, NULL},
};

static PyType_Spec array_spec = {
    .name = "array.array",
    .basicsize = sizeof(arrayobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_SEQUENCE),
    .slots = array_slots,
};

/*********************** Array Iterator **************************/

/*[clinic input]
class array.arrayiterator "arrayiterobject *" "find_array_state_by_type(type)->ArrayIterType"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=fb46d5ef98dd95ff]*/

static PyObject *
array_iter(PyObject *op)
{
    arrayobject *ao = arrayobject_CAST(op);
    array_state *state = find_array_state_by_type(Py_TYPE(ao));
    arrayiterobject *it;

    if (!array_Check(ao, state)) {
        PyErr_BadInternalCall();
        return NULL;
    }

    it = PyObject_GC_New(arrayiterobject, state->ArrayIterType);
    if (it == NULL)
        return NULL;

    it->ao = (arrayobject*)Py_NewRef(ao);
    it->index = 0;  // -1 indicates exhausted
    PyObject_GC_Track(it);
    return (PyObject *)it;
}

static PyObject *
arrayiter_next(PyObject *op)
{
    arrayiterobject *it = arrayiterobject_CAST(op);
    assert(it != NULL);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(it->index);
    if (index < 0) {
        return NULL;
    }
    arrayobject *ao = it->ao;
#ifndef NDEBUG
    array_state *state = find_array_state_by_type(Py_TYPE(it));
    assert(PyObject_TypeCheck(it, state->ArrayIterType));
    assert(array_Check(ao, state));
#endif
#ifdef Py_GIL_DISABLED
    PyObject *ret = getarrayitem_threadsafe((PyObject *)ao, index);
    if (ret != NULL || PyErr_Occurred()) {
        // A failed conversion skips the item, it does not end the iteration.
        _Py_atomic_store_ssize_relaxed(&it->index, index + 1);
        return ret;
    }
    _Py_atomic_store_ssize_relaxed(&it->index, -1);
#else
    if (index < Py_SIZE(ao)) {
        it->index = index + 1;
        return getarrayitem(ao, index);
    }
    it->index = -1;
    Py_CLEAR(it->ao);
#endif
    return NULL;
}

static void
arrayiter_dealloc(PyObject *op)
{
    arrayiterobject *it = arrayiterobject_CAST(op);
    PyTypeObject *tp = Py_TYPE(it);
    PyObject_GC_UnTrack(it);
    Py_XDECREF(it->ao);
    PyObject_GC_Del(it);
    Py_DECREF(tp);
}

static int
arrayiter_traverse(PyObject *op, visitproc visit, void *arg)
{
    arrayiterobject *it = arrayiterobject_CAST(op);
    Py_VISIT(Py_TYPE(it));
    Py_VISIT(it->ao);
    return 0;
}

/*[clinic input]
array.arrayiterator.__reduce__

    cls: defining_class
    /

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
array_arrayiterator___reduce___impl(arrayiterobject *self, PyTypeObject *cls)
/*[clinic end generated code: output=4b032417a2c8f5e6 input=ac64e65a87ad452e]*/
{
    array_state *state = get_array_state_by_class(cls);
    assert(state != NULL);
    PyObject *func = _PyEval_GetBuiltin(state->str_iter);
    Py_ssize_t index = FT_ATOMIC_LOAD_SSIZE_RELAXED(self->index);
    if (index >= 0) {
        return Py_BuildValue("N(O)n", func, self->ao, index);
    }
    return Py_BuildValue("N(())", func);
}

/*[clinic input]
array.arrayiterator.__setstate__

    state: object
    /

Set state information for unpickling.
[clinic start generated code]*/

static PyObject *
array_arrayiterator___setstate___impl(arrayiterobject *self, PyObject *state)
/*[clinic end generated code: output=d7837ae4ac1fd8b9 input=f47d5ceda19e787b]*/
{
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred()) {
        return NULL;
    }
    if (FT_ATOMIC_LOAD_SSIZE_RELAXED(self->index) >= 0) {
        if (index < -1) {
            index = -1;
        }
        else {
            Py_ssize_t size = array_get_size(self->ao);
            if (index > size) {
                index = size; /* iterator at end */
            }
        }
        FT_ATOMIC_STORE_SSIZE_RELAXED(self->index, index);
    }
    Py_RETURN_NONE;
}

static PyMethodDef arrayiter_methods[] = {
    ARRAY_ARRAYITERATOR___REDUCE___METHODDEF
    ARRAY_ARRAYITERATOR___SETSTATE___METHODDEF
    {NULL, NULL} /* sentinel */
};

static PyType_Slot arrayiter_slots[] = {
    {Py_tp_dealloc, arrayiter_dealloc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, arrayiter_traverse},
    {Py_tp_iter, PyObject_SelfIter},
    {Py_tp_iternext, arrayiter_next},
    {Py_tp_methods, arrayiter_methods},
    {0, NULL},
};

static PyType_Spec arrayiter_spec = {
    .name = "array.arrayiterator",
    .basicsize = sizeof(arrayiterobject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = arrayiter_slots,
};


/*********************** Install Module **************************/

static int
arraymodule_traverse(PyObject *module, visitproc visit, void *arg)
{
    array_state *state = get_array_state(module);
    Py_VISIT(state->ArrayType);
    Py_VISIT(state->ArrayIterType);
    Py_VISIT(state->array_reconstructor);
    Py_VISIT(state->typecodes);
    return 0;
}

static int
arraymodule_clear(PyObject *module)
{
    array_state *state = get_array_state(module);
    Py_CLEAR(state->ArrayType);
    Py_CLEAR(state->ArrayIterType);
    Py_CLEAR(state->array_reconstructor);
    Py_CLEAR(state->str_read);
    Py_CLEAR(state->str_write);
    Py_CLEAR(state->str___dict__);
    Py_CLEAR(state->str_iter);
    Py_CLEAR(state->typecodes);
    return 0;
}

static void
arraymodule_free(void *module)
{
    (void)arraymodule_clear((PyObject *)module);
}

/* No functions in array module. */
static PyMethodDef a_methods[] = {
    ARRAY__ARRAY_RECONSTRUCTOR_METHODDEF
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

#define CREATE_TYPE(module, type, spec)                                  \
do {                                                                     \
    type = (PyTypeObject *)PyType_FromModuleAndSpec(module, spec, NULL); \
    if (type == NULL) {                                                  \
        return -1;                                                       \
    }                                                                    \
} while (0)

#define ADD_INTERNED(state, string)                      \
do {                                                     \
    PyObject *tmp = PyUnicode_InternFromString(#string); \
    if (tmp == NULL) {                                   \
        return -1;                                       \
    }                                                    \
    state->str_ ## string = tmp;                         \
} while (0)

static int
arraymodule_modexec(PyObject *m)
{
    array_state *state = get_array_state(m);
    PyObject *typecodes;
    const struct arraydescr *descr;

    state->array_reconstructor = NULL;
    /* Add interned strings */
    ADD_INTERNED(state, read);
    ADD_INTERNED(state, write);
    ADD_INTERNED(state, __dict__);
    ADD_INTERNED(state, iter);

    CREATE_TYPE(m, state->ArrayType, &array_spec);
    CREATE_TYPE(m, state->ArrayIterType, &arrayiter_spec);
    Py_SET_TYPE(state->ArrayIterType, &PyType_Type);

    // Older undocumented alias:
    if (PyModule_AddObjectRef(m, "ArrayType",
                              (PyObject *)state->ArrayType) < 0) {
        return -1;
    }

    PyObject *mutablesequence = PyImport_ImportModuleAttrString(
            "collections.abc", "MutableSequence");
    if (!mutablesequence) {
        return -1;
    }
    PyObject *res = PyObject_CallMethod(mutablesequence, "register", "O",
                                        (PyObject *)state->ArrayType);
    Py_DECREF(mutablesequence);
    if (!res) {
        return -1;
    }
    Py_DECREF(res);

    if (PyModule_AddType(m, state->ArrayType) < 0) {
        return -1;
    }

    typecodes = PyList_New(0);
    if (typecodes == NULL) {
        return -1;
    }
    for (descr = descriptors; descr->typecode[0] != 0; descr++) {
        PyObject *typecode = PyUnicode_DecodeASCII(descr->typecode, strlen(descr->typecode), NULL);
        if (typecode == NULL) {
            Py_DECREF(typecodes);
            return -1;
        }
        int res = PyList_Append(typecodes, typecode);
        Py_DECREF(typecode);
        if (res < 0) {
            Py_DECREF(typecodes);
            return -1;
        }
    }
    PyObject *tuple = PyList_AsTuple(typecodes);
    Py_DECREF(typecodes);
    if (tuple == NULL) {
        return -1;
    }
    state->typecodes = Py_NewRef(tuple);
    if (PyModule_Add(m, "typecodes", tuple) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot arrayslots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, arraymodule_modexec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};


static struct PyModuleDef arraymodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "array",
    .m_size = sizeof(array_state),
    .m_doc = module_doc,
    .m_methods = a_methods,
    .m_slots = arrayslots,
    .m_traverse = arraymodule_traverse,
    .m_clear = arraymodule_clear,
    .m_free = arraymodule_free,
};


PyMODINIT_FUNC
PyInit_array(void)
{
    return PyModuleDef_Init(&arraymodule);
}
