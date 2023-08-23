#ifndef Py_ATOMIC_STD_H
#  error "this header file must not be included directly"
#endif

// This is the implementation of Python atomic operations using C++11 or C11
// atomics. Note that the pyatomic_gcc.h implementation is preferred for GCC
// compatible compilers, even if they support C++11 atomics.

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
_Py_atomic_add_int(int *address, int value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int)*)address, value);
}

static inline int8_t
_Py_atomic_add_int8(int8_t *address, int8_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int8_t)*)address, value);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *address, int16_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int16_t)*)address, value);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *address, int32_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int32_t)*)address, value);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *address, int64_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(int64_t)*)address, value);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *address, intptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(intptr_t)*)address, value);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *address, unsigned int value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(unsigned int)*)address, value);
}

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint8_t)*)address, value);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint16_t)*)address, value);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint32_t)*)address, value);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uint64_t)*)address, value);
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(uintptr_t)*)address, value);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    _Py_USING_STD
    return atomic_fetch_add((_Atomic(Py_ssize_t)*)address, value);
}

static inline int
_Py_atomic_compare_exchange_int(int *address, int expected, int value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_int8(int8_t *address, int8_t expected, int8_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int8_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *address, int16_t expected, int16_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int16_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *address, int32_t expected, int32_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int32_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *address, int64_t expected, int64_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(int64_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *address, intptr_t expected, intptr_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(intptr_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *address, unsigned int expected, unsigned int value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(unsigned int)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *address, uint8_t expected, uint8_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint8_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *address, uint16_t expected, uint16_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint16_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *address, uint32_t expected, uint32_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint32_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *address, uint64_t expected, uint64_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uint64_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *address, uintptr_t expected, uintptr_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(uintptr_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *address, Py_ssize_t expected, Py_ssize_t value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(Py_ssize_t)*)address, &expected, value);
}

static inline int
_Py_atomic_compare_exchange_ptr(void *address, void *expected, void *value)
{
    _Py_USING_STD
    return atomic_compare_exchange_strong((_Atomic(void *)*)address, &expected, value);
}


static inline int
_Py_atomic_exchange_int(int *address, int value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int)*)address, value);
}

static inline int8_t
_Py_atomic_exchange_int8(int8_t *address, int8_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int8_t)*)address, value);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *address, int16_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int16_t)*)address, value);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *address, int32_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int32_t)*)address, value);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *address, int64_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(int64_t)*)address, value);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *address, intptr_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(intptr_t)*)address, value);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *address, unsigned int value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(unsigned int)*)address, value);
}

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint8_t)*)address, value);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint16_t)*)address, value);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint32_t)*)address, value);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uint64_t)*)address, value);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(uintptr_t)*)address, value);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(Py_ssize_t)*)address, value);
}

static inline void *
_Py_atomic_exchange_ptr(void *address, void *value)
{
    _Py_USING_STD
    return atomic_exchange((_Atomic(void *)*)address, value);
}

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint8_t)*)address, value);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint16_t)*)address, value);
}


static inline uint32_t
_Py_atomic_and_uint32(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint32_t)*)address, value);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uint64_t)*)address, value);
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_and((_Atomic(uintptr_t)*)address, value);
}

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint8_t)*)address, value);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint16_t)*)address, value);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint32_t)*)address, value);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uint64_t)*)address, value);
}

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    return atomic_fetch_or((_Atomic(uintptr_t)*)address, value);
}

static inline int
_Py_atomic_load_int(const int *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int)*)address);
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int8_t)*)address);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int16_t)*)address);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int32_t)*)address);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(int64_t)*)address);
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(intptr_t)*)address);
}

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint8_t)*)address);
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint32_t)*)address);
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint32_t)*)address);
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uint64_t)*)address);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(uintptr_t)*)address);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(unsigned int)*)address);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(Py_ssize_t)*)address);
}

static inline void *
_Py_atomic_load_ptr(const void *address)
{
    _Py_USING_STD
    return atomic_load((const _Atomic(void*)*)address);
}


static inline int
_Py_atomic_load_int_relaxed(const int *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int)*)address, memory_order_relaxed);
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int8_t)*)address, memory_order_relaxed);
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int16_t)*)address, memory_order_relaxed);
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int32_t)*)address, memory_order_relaxed);
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(int64_t)*)address, memory_order_relaxed);
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(intptr_t)*)address, memory_order_relaxed);
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint8_t)*)address, memory_order_relaxed);
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint16_t)*)address, memory_order_relaxed);
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint32_t)*)address, memory_order_relaxed);
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uint64_t)*)address, memory_order_relaxed);
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(uintptr_t)*)address, memory_order_relaxed);
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(unsigned int)*)address, memory_order_relaxed);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(Py_ssize_t)*)address, memory_order_relaxed);
}

static inline void *
_Py_atomic_load_ptr_relaxed(const void *address)
{
    _Py_USING_STD
    return atomic_load_explicit((const _Atomic(void*)*)address, memory_order_relaxed);
}

static inline void
_Py_atomic_store_int(int *address, int value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int)*)address, value);
}

static inline void
_Py_atomic_store_int8(int8_t *address, int8_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int8_t)*)address, value);
}

static inline void
_Py_atomic_store_int16(int16_t *address, int16_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int16_t)*)address, value);
}

static inline void
_Py_atomic_store_int32(int32_t *address, int32_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int32_t)*)address, value);
}

static inline void
_Py_atomic_store_int64(int64_t *address, int64_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(int64_t)*)address, value);
}

static inline void
_Py_atomic_store_intptr(intptr_t *address, intptr_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(intptr_t)*)address, value);
}

static inline void
_Py_atomic_store_uint8(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint8_t)*)address, value);
}

static inline void
_Py_atomic_store_uint16(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint16_t)*)address, value);
}

static inline void
_Py_atomic_store_uint32(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint32_t)*)address, value);
}

static inline void
_Py_atomic_store_uint64(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uint64_t)*)address, value);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(uintptr_t)*)address, value);
}

static inline void
_Py_atomic_store_uint(unsigned int *address, unsigned int value)
{
    _Py_USING_STD
    atomic_store((_Atomic(unsigned int)*)address, value);
}

static inline void
_Py_atomic_store_ptr(void *address, void *value)
{
    _Py_USING_STD
    atomic_store((_Atomic(void*)*)address, value);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    _Py_USING_STD
    atomic_store((_Atomic(Py_ssize_t)*)address, value);
}

static inline void
_Py_atomic_store_int_relaxed(int *address, int value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *address, int8_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int8_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *address, int16_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int16_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *address, int32_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int32_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *address, int64_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(int64_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *address, intptr_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(intptr_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *address, uint8_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint8_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *address, uint16_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint16_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *address, uint32_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint32_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint64_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *address, uintptr_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uintptr_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *address, unsigned int value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(unsigned int)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_ptr_relaxed(void *address, void *value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(void*)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *address, Py_ssize_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(Py_ssize_t)*)address, value, memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint64_release(uint64_t *address, uint64_t value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(uint64_t)*)address, value, memory_order_release);
}

static inline void
_Py_atomic_store_ptr_release(void *address, void *value)
{
    _Py_USING_STD
    atomic_store_explicit((_Atomic(void*)*)address, value, memory_order_release);
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
