/*
 * C Extension module to smoke test pyatomic.h API.
 *
 * This only tests basic functionality, not any synchronizing ordering.
 */

#include "parts.h"

// We define atomic bitwise operations on these types
#define FOR_BITWISE_TYPES(V)    \
    V(uint8, uint8_t)           \
    V(uint16, uint16_t)         \
    V(uint32, uint32_t)         \
    V(uint64, uint64_t)         \
    V(uintptr, uintptr_t)

// We define atomic addition on these types
#define FOR_ARITHMETIC_TYPES(V) \
    FOR_BITWISE_TYPES(V)        \
    V(int, int)                 \
    V(uint, unsigned int)       \
    V(int8, int8_t)             \
    V(int16, int16_t)           \
    V(int32, int32_t)           \
    V(int64, int64_t)           \
    V(intptr, intptr_t)         \
    V(ssize, Py_ssize_t)

// We define atomic load, store, exchange, and compare_exchange on these types
#define FOR_ALL_TYPES(V)        \
    FOR_ARITHMETIC_TYPES(V)     \
    V(ptr, void*)

#define IMPL_TEST_ADD(suffix, dtype) \
static PyObject * \
test_atomic_add_##suffix(PyObject *self, PyObject *obj) { \
    dtype x = 0; \
    assert(_Py_atomic_add_##suffix(&x, 1) == 0); \
    assert(x == 1); \
    assert(_Py_atomic_add_##suffix(&x, 2) == 1); \
    assert(x == 3); \
    assert(_Py_atomic_add_##suffix(&x, -2) == 3); \
    assert(x == 1); \
    assert(_Py_atomic_add_##suffix(&x, -1) == 1); \
    assert(x == 0); \
    assert(_Py_atomic_add_##suffix(&x, -1) == 0); \
    assert(x == (dtype)-1); \
    assert(_Py_atomic_add_##suffix(&x, -2) == (dtype)-1); \
    assert(x == (dtype)-3); \
    assert(_Py_atomic_add_##suffix(&x, 2) == (dtype)-3); \
    assert(x == (dtype)-1); \
    Py_RETURN_NONE; \
}
FOR_ARITHMETIC_TYPES(IMPL_TEST_ADD)

#define IMPL_TEST_COMPARE_EXCHANGE(suffix, dtype) \
static PyObject * \
test_atomic_compare_exchange_##suffix(PyObject *self, PyObject *obj) { \
    dtype x = (dtype)0; \
    dtype y = (dtype)1; \
    dtype z = (dtype)2; \
    assert(_Py_atomic_compare_exchange_##suffix(&x, &y, z) == 0); \
    assert(x == 0); \
    assert(y == 0); \
    assert(_Py_atomic_compare_exchange_##suffix(&x, &y, z) == 1); \
    assert(x == z); \
    assert(y == 0); \
    assert(_Py_atomic_compare_exchange_##suffix(&x, &y, z) == 0); \
    assert(x == z); \
    assert(y == z); \
    Py_RETURN_NONE; \
}
FOR_ALL_TYPES(IMPL_TEST_COMPARE_EXCHANGE)

#define IMPL_TEST_EXCHANGE(suffix, dtype) \
static PyObject * \
test_atomic_exchange_##suffix(PyObject *self, PyObject *obj) { \
    dtype x = (dtype)0; \
    dtype y = (dtype)1; \
    dtype z = (dtype)2; \
    assert(_Py_atomic_exchange_##suffix(&x, y) == (dtype)0); \
    assert(x == (dtype)1); \
    assert(_Py_atomic_exchange_##suffix(&x, z) == (dtype)1); \
    assert(x == (dtype)2); \
    assert(_Py_atomic_exchange_##suffix(&x, y) == (dtype)2); \
    assert(x == (dtype)1); \
    Py_RETURN_NONE; \
}
FOR_ALL_TYPES(IMPL_TEST_EXCHANGE)

#define IMPL_TEST_LOAD_STORE(suffix, dtype) \
static PyObject * \
test_atomic_load_store_##suffix(PyObject *self, PyObject *obj) { \
    dtype x = (dtype)0; \
    dtype y = (dtype)1; \
    dtype z = (dtype)2; \
    assert(_Py_atomic_load_##suffix(&x) == (dtype)0); \
    assert(x == (dtype)0); \
    _Py_atomic_store_##suffix(&x, y); \
    assert(_Py_atomic_load_##suffix(&x) == (dtype)1); \
    assert(x == (dtype)1); \
    _Py_atomic_store_##suffix##_relaxed(&x, z); \
    assert(_Py_atomic_load_##suffix##_relaxed(&x) == (dtype)2); \
    assert(x == (dtype)2); \
    Py_RETURN_NONE; \
}
FOR_ALL_TYPES(IMPL_TEST_LOAD_STORE)

#define IMPL_TEST_AND_OR(suffix, dtype) \
static PyObject * \
test_atomic_and_or_##suffix(PyObject *self, PyObject *obj) { \
    dtype x = (dtype)0; \
    dtype y = (dtype)1; \
    dtype z = (dtype)3; \
    assert(_Py_atomic_or_##suffix(&x, z) == (dtype)0); \
    assert(x == (dtype)3); \
    assert(_Py_atomic_and_##suffix(&x, y) == (dtype)3); \
    assert(x == (dtype)1); \
    Py_RETURN_NONE; \
}
FOR_BITWISE_TYPES(IMPL_TEST_AND_OR)

static PyObject *
test_atomic_fences(PyObject *self, PyObject *obj) {
    // Just make sure that the fences compile. We are not
    // testing any synchronizing ordering.
    _Py_atomic_fence_seq_cst();
    _Py_atomic_fence_release();
    Py_RETURN_NONE;
}

static PyObject *
test_atomic_release_acquire(PyObject *self, PyObject *obj) {
    void *x = NULL;
    void *y = &y;
    assert(_Py_atomic_load_ptr_acquire(&x) == NULL);
    _Py_atomic_store_ptr_release(&x, y);
    assert(x == y);
    assert(_Py_atomic_load_ptr_acquire(&x) == y);
    Py_RETURN_NONE;
}

static PyObject *
test_atomic_load_store_int_release_acquire(PyObject *self, PyObject *obj) { \
    int x = 0;
    int y = 1;
    int z = 2;
    assert(_Py_atomic_load_int_acquire(&x) == 0);
    _Py_atomic_store_int_release(&x, y);
    assert(x == y);
    assert(_Py_atomic_load_int_acquire(&x) == y);
    _Py_atomic_store_int_release(&x, z);
    assert(x == z);
    assert(_Py_atomic_load_int_acquire(&x) == z);
    Py_RETURN_NONE;
}

// NOTE: all tests should start with "test_atomic_" to be included
// in test_pyatomic.py

#define BIND_TEST_ADD(suffix, dtype) \
    {"test_atomic_add_" #suffix, test_atomic_add_##suffix, METH_NOARGS},
#define BIND_TEST_COMPARE_EXCHANGE(suffix, dtype) \
    {"test_atomic_compare_exchange_" #suffix, test_atomic_compare_exchange_##suffix, METH_NOARGS},
#define BIND_TEST_EXCHANGE(suffix, dtype) \
    {"test_atomic_exchange_" #suffix, test_atomic_exchange_##suffix, METH_NOARGS},
#define BIND_TEST_LOAD_STORE(suffix, dtype) \
    {"test_atomic_load_store_" #suffix, test_atomic_load_store_##suffix, METH_NOARGS},
#define BIND_TEST_AND_OR(suffix, dtype) \
    {"test_atomic_and_or_" #suffix, test_atomic_and_or_##suffix, METH_NOARGS},

static PyMethodDef test_methods[] = {
    FOR_ARITHMETIC_TYPES(BIND_TEST_ADD)
    FOR_ALL_TYPES(BIND_TEST_COMPARE_EXCHANGE)
    FOR_ALL_TYPES(BIND_TEST_EXCHANGE)
    FOR_ALL_TYPES(BIND_TEST_LOAD_STORE)
    FOR_BITWISE_TYPES(BIND_TEST_AND_OR)
    {"test_atomic_fences", test_atomic_fences, METH_NOARGS},
    {"test_atomic_release_acquire", test_atomic_release_acquire, METH_NOARGS},
    {"test_atomic_load_store_int_release_acquire", test_atomic_load_store_int_release_acquire, METH_NOARGS},
    {NULL, NULL} /* sentinel */
};

int
_PyTestCapi_Init_PyAtomic(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
