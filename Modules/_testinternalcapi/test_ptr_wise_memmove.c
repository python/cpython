// Tests for _Py_ptr_wise_atomic_memmove() in pycore_object.h

#include "parts.h"
#include "pycore_object.h"    // _Py_ptr_wise_atomic_memmove()
#include "pycore_gc.h"        // _PyObject_GC_SET_SHARED()

// Five distinguishable immortal singletons used as placeholder pointers.
// These require no reference-count management when stored in a raw array.
#define NPTRS 5
static PyObject *test_objs[NPTRS];

static void
setup_test_objs(void)
{
    test_objs[0] = Py_None;
    test_objs[1] = Py_True;
    test_objs[2] = Py_False;
    test_objs[3] = Py_Ellipsis;
    test_objs[4] = Py_NotImplemented;
}

// Fill buf[0..NPTRS-1] with test_objs in order.
static void
fill_buf(PyObject **buf)
{
    for (int i = 0; i < NPTRS; i++) {
        buf[i] = test_objs[i];
    }
}

// Return a fresh empty PyListObject whose GC SHARED bit is set in
// Py_GIL_DISABLED builds.  This forces _Py_ptr_wise_atomic_memmove()
// to take the atomic (non-fast) path so we can exercise all branches.
// In non-GIL builds the function always uses memmove, so no flag is needed.
static PyObject *
make_shared_container(void)
{
    PyObject *a = PyList_New(0);
    if (a == NULL) {
        return NULL;
    }
#ifdef Py_GIL_DISABLED
    _PyObject_GC_SET_SHARED(a);
#endif
    return a;
}

// Helper: create container, call the function, return it for cleanup.
// Returns NULL (with exception set) on allocation failure.
static PyObject *
call_memmove(PyObject **dest, PyObject **src, Py_ssize_t n)
{
    PyObject *a = make_shared_container();
    if (a != NULL) {
        _Py_ptr_wise_atomic_memmove(a, dest, src, n);
    }
    return a;
}


// dest < src: forward pointer-by-pointer copy.
// buf = [0,1,2,3,4], copy src=&buf[2] n=3 to dest=&buf[0]
// Expected result: buf = [2,3,4,3,4]
static PyObject *
test_memmove_dest_lt_src(PyObject *self, PyObject *Py_UNUSED(arg))
{
    setup_test_objs();

    PyObject *buf[NPTRS];
    fill_buf(buf);

    PyObject *a = call_memmove(&buf[0], &buf[2], 3);
    if (a == NULL) {
        return NULL;
    }

    assert(buf[0] == test_objs[2]);
    assert(buf[1] == test_objs[3]);
    assert(buf[2] == test_objs[4]);
    assert(buf[3] == test_objs[3]);  // unchanged
    assert(buf[4] == test_objs[4]);  // unchanged

    Py_DECREF(a);
    Py_RETURN_NONE;
}

// dest > src: backward pointer-by-pointer copy.
// buf = [0,1,2,3,4], copy src=&buf[0] n=3 to dest=&buf[2]
// Expected result: buf = [0,1,0,1,2]
static PyObject *
test_memmove_dest_gt_src(PyObject *self, PyObject *Py_UNUSED(arg))
{
    setup_test_objs();

    PyObject *buf[NPTRS];
    fill_buf(buf);

    PyObject *a = call_memmove(&buf[2], &buf[0], 3);
    if (a == NULL) {
        return NULL;
    }

    assert(buf[0] == test_objs[0]);  // unchanged
    assert(buf[1] == test_objs[1]);  // unchanged
    assert(buf[2] == test_objs[0]);
    assert(buf[3] == test_objs[1]);
    assert(buf[4] == test_objs[2]);

    Py_DECREF(a);
    Py_RETURN_NONE;
}

// dest == src: backward copy where every write is a no-op.
// buf = [0,1,2,3,4], copy src=&buf[1] n=3 to dest=&buf[1]
// Expected result: buf unchanged.
static PyObject *
test_memmove_dest_eq_src(PyObject *self, PyObject *Py_UNUSED(arg))
{
    setup_test_objs();

    PyObject *buf[NPTRS];
    fill_buf(buf);

    PyObject *a = call_memmove(&buf[1], &buf[1], 3);
    if (a == NULL) {
        return NULL;
    }

    for (int i = 0; i < NPTRS; i++) {
        assert(buf[i] == test_objs[i]);
    }

    Py_DECREF(a);
    Py_RETURN_NONE;
}

// Overlapping ranges, dest < src: forward copy is safe.
// buf = [0,1,2,3,4], copy src=&buf[2] n=3 to dest=&buf[1]
// Forward: buf[1]=buf[2]=2, buf[2]=buf[3]=3, buf[3]=buf[4]=4
// Expected result: buf = [0,2,3,4,4]
static PyObject *
test_memmove_overlapping(PyObject *self, PyObject *Py_UNUSED(arg))
{
    setup_test_objs();

    PyObject *buf[NPTRS];
    fill_buf(buf);

    PyObject *a = call_memmove(&buf[1], &buf[2], 3);
    if (a == NULL) {
        return NULL;
    }

    assert(buf[0] == test_objs[0]);  // unchanged
    assert(buf[1] == test_objs[2]);
    assert(buf[2] == test_objs[3]);
    assert(buf[3] == test_objs[4]);
    assert(buf[4] == test_objs[4]);  // unchanged

    Py_DECREF(a);
    Py_RETURN_NONE;
}

// Single owner fast path (GIL_DISABLED only): object owned by this thread
// and not GC-shared => memmove is used instead of atomic stores.
// In non-GIL builds this always takes the memmove path, so the test
// degenerates to a basic correctness check.
static PyObject *
test_memmove_single_owner(PyObject *self, PyObject *Py_UNUSED(arg))
{
    setup_test_objs();

    PyObject *a = PyList_New(0);
    if (a == NULL) {
        return NULL;
    }

#ifdef Py_GIL_DISABLED
    // A freelist-reused list may carry the SHARED bit set by a sibling test.
    // Clear it explicitly so this test exercises the single-owner fast path.
    _PyObject_CLEAR_GC_BITS(a, _PyGC_BITS_SHARED);
    assert(_Py_IsOwnedByCurrentThread(a) && !_PyObject_GC_IS_SHARED(a));
#endif

    PyObject *buf[NPTRS];
    fill_buf(buf);

    _Py_ptr_wise_atomic_memmove(a, &buf[0], &buf[2], 3);

    assert(buf[0] == test_objs[2]);
    assert(buf[1] == test_objs[3]);
    assert(buf[2] == test_objs[4]);

    Py_DECREF(a);
    Py_RETURN_NONE;
}


static PyMethodDef test_methods[] = {
    {"test_memmove_dest_lt_src",  test_memmove_dest_lt_src,  METH_NOARGS},
    {"test_memmove_dest_gt_src",  test_memmove_dest_gt_src,  METH_NOARGS},
    {"test_memmove_dest_eq_src",  test_memmove_dest_eq_src,  METH_NOARGS},
    {"test_memmove_overlapping",  test_memmove_overlapping,  METH_NOARGS},
    {"test_memmove_single_owner", test_memmove_single_owner, METH_NOARGS},
    {NULL},
};

int
_PyTestInternalCapi_Init_PtrWiseMemmove(PyObject *mod)
{
    return PyModule_AddFunctions(mod, test_methods);
}
