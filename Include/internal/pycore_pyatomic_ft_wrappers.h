// This header file provides wrappers around the atomic operations found in
// `pyatomic.h` that are only atomic in free-threaded builds.
//
// These are intended to be used in places where atomics are required in
// free-threaded builds, but not in the default build, and we don't want to
// introduce the potential performance overhead of an atomic operation in the
// default build.
//
// All usages of these macros should be replaced with unconditionally atomic or
// non-atomic versions, and this file should be removed, once the dust settles
// on free threading.
#ifndef Py_ATOMIC_FT_WRAPPERS_H
#define Py_ATOMIC_FT_WRAPPERS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_GIL_DISABLED
#define FT_ATOMIC_LOAD_PTR(value) _Py_atomic_load_ptr(&value)
#define FT_ATOMIC_STORE_PTR(value, new_value) _Py_atomic_store_ptr(&value, new_value)
#define FT_ATOMIC_LOAD_SSIZE(value) _Py_atomic_load_ssize(&value)
#define FT_ATOMIC_LOAD_SSIZE_ACQUIRE(value) \
    _Py_atomic_load_ssize_acquire(&value)
#define FT_ATOMIC_LOAD_SSIZE_RELAXED(value) \
    _Py_atomic_load_ssize_relaxed(&value)
#define FT_ATOMIC_STORE_PTR(value, new_value) \
    _Py_atomic_store_ptr(&value, new_value)
#define FT_ATOMIC_LOAD_PTR_ACQUIRE(value) \
    _Py_atomic_load_ptr_acquire(&value)
#define FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(value) \
    _Py_atomic_load_uintptr_acquire(&value)
#define FT_ATOMIC_LOAD_PTR_RELAXED(value) \
    _Py_atomic_load_ptr_relaxed(&value)
#define FT_ATOMIC_LOAD_UINT8(value) \
    _Py_atomic_load_uint8(&value)
#define FT_ATOMIC_STORE_UINT8(value, new_value) \
    _Py_atomic_store_uint8(&value, new_value)
#define FT_ATOMIC_LOAD_UINT8_RELAXED(value) \
    _Py_atomic_load_uint8_relaxed(&value)
#define FT_ATOMIC_LOAD_UINT16_RELAXED(value) \
    _Py_atomic_load_uint16_relaxed(&value)
#define FT_ATOMIC_LOAD_UINT32_RELAXED(value) \
    _Py_atomic_load_uint32_relaxed(&value)
#define FT_ATOMIC_LOAD_ULONG_RELAXED(value) \
    _Py_atomic_load_ulong_relaxed(&value)
#define FT_ATOMIC_STORE_PTR_RELAXED(value, new_value) \
    _Py_atomic_store_ptr_relaxed(&value, new_value)
#define FT_ATOMIC_STORE_PTR_RELEASE(value, new_value) \
    _Py_atomic_store_ptr_release(&value, new_value)
#define FT_ATOMIC_STORE_UINTPTR_RELEASE(value, new_value) \
    _Py_atomic_store_uintptr_release(&value, new_value)
#define FT_ATOMIC_STORE_SSIZE_RELAXED(value, new_value) \
    _Py_atomic_store_ssize_relaxed(&value, new_value)
#define FT_ATOMIC_STORE_UINT8_RELAXED(value, new_value) \
    _Py_atomic_store_uint8_relaxed(&value, new_value)
#define FT_ATOMIC_STORE_UINT16_RELAXED(value, new_value) \
    _Py_atomic_store_uint16_relaxed(&value, new_value)
#define FT_ATOMIC_STORE_UINT32_RELAXED(value, new_value) \
    _Py_atomic_store_uint32_relaxed(&value, new_value)

#else
#define FT_ATOMIC_LOAD_PTR(value) value
#define FT_ATOMIC_STORE_PTR(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_SSIZE(value) value
#define FT_ATOMIC_LOAD_SSIZE_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_SSIZE_RELAXED(value) value
#define FT_ATOMIC_STORE_PTR(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_PTR_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_UINTPTR_ACQUIRE(value) value
#define FT_ATOMIC_LOAD_PTR_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT8(value) value
#define FT_ATOMIC_STORE_UINT8(value, new_value) value = new_value
#define FT_ATOMIC_LOAD_UINT8_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT16_RELAXED(value) value
#define FT_ATOMIC_LOAD_UINT32_RELAXED(value) value
#define FT_ATOMIC_LOAD_ULONG_RELAXED(value) value
#define FT_ATOMIC_STORE_PTR_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_PTR_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINTPTR_RELEASE(value, new_value) value = new_value
#define FT_ATOMIC_STORE_SSIZE_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT8_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT16_RELAXED(value, new_value) value = new_value
#define FT_ATOMIC_STORE_UINT32_RELAXED(value, new_value) value = new_value

#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_ATOMIC_FT_WRAPPERS_H */
