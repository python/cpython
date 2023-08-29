// This is the implementation of Python atomic operations using C++11 or C11
// atomics. Note that the pyatomic_gcc.h implementation is preferred for GCC
// compatible compilers, even if they support C++11 atomics.

#ifndef Py_ATOMIC_STD_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C++" {
#include <atomic>
}
#define  _Py_USING_STD   using namespace std;
#define  _Atomic(tp)    atomic<tp>
#else
#include <stdatomic.h>
#define  _Py_USING_STD
#endif


static inline int
_Py_atomic_add_int(int *ptr, int value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int)*)ptr, value);
}

static inline int8_t
_Py_atomic_add_int8(int8_t *ptr, int8_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int8_t)*)ptr, value);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *ptr, int16_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int16_t)*)ptr, value);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *ptr, int32_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int32_t)*)ptr, value);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *ptr, int64_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int64_t)*)ptr, value);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *ptr, intptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(intptr_t)*)ptr, value);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *ptr, unsigned int value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(unsigned int)*)ptr, value);
}

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint8_t)*)ptr, value);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint16_t)*)ptr, value);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint32_t)*)ptr, value);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint64_t)*)ptr, value);
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uintptr_t)*)ptr, value);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(Py_ssize_t)*)ptr, value);
}

static inline int
_Py_atomic_compare_exchange_int(int *ptr, int *expected, int desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int8(int8_t *ptr, int8_t *expected, int8_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int8_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *ptr, int16_t *expected, int16_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int16_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *ptr, int32_t *expected, int32_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int32_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *ptr, int64_t *expected, int64_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int64_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *ptr, intptr_t *expected, intptr_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(intptr_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *ptr, unsigned int *expected, unsigned int desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(unsigned int)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *ptr, uint8_t *expected, uint8_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint8_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *ptr, uint16_t *expected, uint16_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint16_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *ptr, uint32_t *expected, uint32_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint32_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *ptr, uint64_t *expected, uint64_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint64_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *ptr, uintptr_t *expected, uintptr_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uintptr_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t *expected, Py_ssize_t desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(Py_ssize_t)*)ptr,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_ptr(void *ptr, void *expected, void *desired)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(void *)*)ptr,
                                          (void **)expected, desired);
}


static inline int
_Py_atomic_exchange_int(int *ptr, int value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int)*)ptr, value);
}

static inline int8_t
_Py_atomic_exchange_int8(int8_t *ptr, int8_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int8_t)*)ptr, value);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *ptr, int16_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int16_t)*)ptr, value);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *ptr, int32_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int32_t)*)ptr, value);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *ptr, int64_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int64_t)*)ptr, value);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *ptr, intptr_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(intptr_t)*)ptr, value);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *ptr, unsigned int value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(unsigned int)*)ptr, value);
}

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint8_t)*)ptr, value);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint16_t)*)ptr, value);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint32_t)*)ptr, value);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint64_t)*)ptr, value);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uintptr_t)*)ptr, value);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(Py_ssize_t)*)ptr, value);
}

static inline void *
_Py_atomic_exchange_ptr(void *ptr, void *value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(void *)*)ptr, value);
}

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint8_t)*)ptr, value);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint16_t)*)ptr, value);
}


static inline uint32_t
_Py_atomic_and_uint32(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint32_t)*)ptr, value);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint64_t)*)ptr, value);
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uintptr_t)*)ptr, value);
}

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint8_t)*)ptr, value);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint16_t)*)ptr, value);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint32_t)*)ptr, value);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint64_t)*)ptr, value);
}

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uintptr_t)*)ptr, value);
}

static inline int
_Py_atomic_load_int(const int *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int)*)ptr);
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int8_t)*)ptr);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int16_t)*)ptr);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int32_t)*)ptr);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int64_t)*)ptr);
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(intptr_t)*)ptr);
}

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint8_t)*)ptr);
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint32_t)*)ptr);
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint32_t)*)ptr);
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint64_t)*)ptr);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uintptr_t)*)ptr);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(unsigned int)*)ptr);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(Py_ssize_t)*)ptr);
}

static inline void *
_Py_atomic_load_ptr(const void *ptr)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(void*)*)ptr);
}


static inline int
_Py_atomic_load_int_relaxed(const int *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int)*)ptr,
                                memory_order_relaxed);
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int8_t)*)ptr,
                                memory_order_relaxed);
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int16_t)*)ptr,
                                memory_order_relaxed);
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int32_t)*)ptr,
                                memory_order_relaxed);
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int64_t)*)ptr,
                                memory_order_relaxed);
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(intptr_t)*)ptr,
                                memory_order_relaxed);
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint8_t)*)ptr,
                                memory_order_relaxed);
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint16_t)*)ptr,
                                memory_order_relaxed);
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint32_t)*)ptr,
                                memory_order_relaxed);
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint64_t)*)ptr,
                                memory_order_relaxed);
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uintptr_t)*)ptr,
                                memory_order_relaxed);
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(unsigned int)*)ptr,
                                memory_order_relaxed);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(Py_ssize_t)*)ptr,
                                memory_order_relaxed);
}

static inline void *
_Py_atomic_load_ptr_relaxed(const void *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(void*)*)ptr,
                                memory_order_relaxed);
}

static inline void
_Py_atomic_store_int(int *ptr, int value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int)*)ptr, value);
}

static inline void
_Py_atomic_store_int8(int8_t *ptr, int8_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int8_t)*)ptr, value);
}

static inline void
_Py_atomic_store_int16(int16_t *ptr, int16_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int16_t)*)ptr, value);
}

static inline void
_Py_atomic_store_int32(int32_t *ptr, int32_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int32_t)*)ptr, value);
}

static inline void
_Py_atomic_store_int64(int64_t *ptr, int64_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int64_t)*)ptr, value);
}

static inline void
_Py_atomic_store_intptr(intptr_t *ptr, intptr_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(intptr_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uint8(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint8_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uint16(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint16_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uint32(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint32_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uint64(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint64_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uintptr_t)*)ptr, value);
}

static inline void
_Py_atomic_store_uint(unsigned int *ptr, unsigned int value)
{
    _Py_USING_STD
    atomic_store((_Atomic(unsigned int)*)ptr, value);
}

static inline void
_Py_atomic_store_ptr(void *ptr, void *value)
{
    _Py_USING_STD
    atomic_store((_Atomic(void*)*)ptr, value);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(Py_ssize_t)*)ptr, value);
}

static inline void
_Py_atomic_store_int_relaxed(int *ptr, int value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *ptr, int8_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int8_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *ptr, int16_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int16_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *ptr, int32_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int32_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *ptr, int64_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int64_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *ptr, intptr_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(intptr_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *ptr, uint8_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint8_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *ptr, uint16_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint16_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *ptr, uint32_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint32_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *ptr, uint64_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint64_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *ptr, uintptr_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uintptr_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *ptr, unsigned int value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(unsigned int)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ptr_relaxed(void *ptr, void *value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(void*)*)ptr, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *ptr, Py_ssize_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(Py_ssize_t)*)ptr, value,
                          memory_order_relaxed);
}

static inline void *
_Py_atomic_load_ptr_acquire(const void *ptr)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(void*)*)ptr,
                                memory_order_acquire);
}

static inline void
_Py_atomic_store_ptr_release(void *ptr, void *value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(void*)*)ptr, value,
                          memory_order_release);
}

 static inline void
_Py_atomic_fence_seq_cst(void)
{
    _Py_USING_STD
    atomic_thread_fence(memory_order_seq_cst);
}

 static inline void
_Py_atomic_fence_release(void)
{
    _Py_USING_STD
    atomic_thread_fence(memory_order_release);
}
