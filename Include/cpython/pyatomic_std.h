// This is the implementation of Python atomic operations using C++11 or C11
// atomics. Note that the pyatomic_gcc.h implementation is preferred for GCC
// compatible compilers, even if they support C++11 atomics.

#ifndef Py_ATOMIC_STD_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C++" {
#  include <atomic>
}
#  define _Py_USING_STD using namespace std
#  define _Atomic(tp) atomic<tp>
#else
#  define  _Py_USING_STD
#  include <stdatomic.h>
#endif


// --- _Py_atomic_add --------------------------------------------------------

static inline int
_Py_atomic_add_int(int *obj, int value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(int)*)obj, value);
}

static inline int8_t
_Py_atomic_add_int8(int8_t *obj, int8_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(int8_t)*)obj, value);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *obj, int16_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(int16_t)*)obj, value);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *obj, int32_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(int32_t)*)obj, value);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *obj, int64_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(int64_t)*)obj, value);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *obj, intptr_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(intptr_t)*)obj, value);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *obj, unsigned int value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(unsigned int)*)obj, value);
}

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(uint8_t)*)obj, value);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(uint16_t)*)obj, value);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(uint32_t)*)obj, value);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(uint64_t)*)obj, value);
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(uintptr_t)*)obj, value);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_USING_STD;
    return atomic_fetch_add((_Atomic(Py_ssize_t)*)obj, value);
}


// --- _Py_atomic_compare_exchange -------------------------------------------

static inline int
_Py_atomic_compare_exchange_int(int *obj, int *expected, int desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(int)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int8(int8_t *obj, int8_t *expected, int8_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(int8_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *obj, int16_t *expected, int16_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(int16_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *obj, int32_t *expected, int32_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(int32_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *obj, int64_t *expected, int64_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(int64_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *obj, intptr_t *expected, intptr_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(intptr_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *obj, unsigned int *expected, unsigned int desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(unsigned int)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uint8_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *obj, uint16_t *expected, uint16_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uint16_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *obj, uint32_t *expected, uint32_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uint32_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *obj, uint64_t *expected, uint64_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uint64_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *obj, uintptr_t *expected, uintptr_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(uintptr_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *obj, Py_ssize_t *expected, Py_ssize_t desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(Py_ssize_t)*)obj,
                                          expected, desired);
}

static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *desired)
{
    _Py_USING_STD;
    return atomic_compare_exchange_strong((_Atomic(void *)*)obj,
                                          (void **)expected, desired);
}


// --- _Py_atomic_exchange ---------------------------------------------------

static inline int
_Py_atomic_exchange_int(int *obj, int value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(int)*)obj, value);
}

static inline int8_t
_Py_atomic_exchange_int8(int8_t *obj, int8_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(int8_t)*)obj, value);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *obj, int16_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(int16_t)*)obj, value);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *obj, int32_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(int32_t)*)obj, value);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *obj, int64_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(int64_t)*)obj, value);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *obj, intptr_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(intptr_t)*)obj, value);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *obj, unsigned int value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(unsigned int)*)obj, value);
}

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(uint8_t)*)obj, value);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(uint16_t)*)obj, value);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(uint32_t)*)obj, value);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(uint64_t)*)obj, value);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(uintptr_t)*)obj, value);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(Py_ssize_t)*)obj, value);
}

static inline void*
_Py_atomic_exchange_ptr(void *obj, void *value)
{
    _Py_USING_STD;
    return atomic_exchange((_Atomic(void *)*)obj, value);
}


// --- _Py_atomic_and --------------------------------------------------------

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    return atomic_fetch_and((_Atomic(uint8_t)*)obj, value);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    return atomic_fetch_and((_Atomic(uint16_t)*)obj, value);
}

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    return atomic_fetch_and((_Atomic(uint32_t)*)obj, value);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    return atomic_fetch_and((_Atomic(uint64_t)*)obj, value);
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    return atomic_fetch_and((_Atomic(uintptr_t)*)obj, value);
}


// --- _Py_atomic_or ---------------------------------------------------------

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    return atomic_fetch_or((_Atomic(uint8_t)*)obj, value);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    return atomic_fetch_or((_Atomic(uint16_t)*)obj, value);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    return atomic_fetch_or((_Atomic(uint32_t)*)obj, value);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    return atomic_fetch_or((_Atomic(uint64_t)*)obj, value);
}

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    return atomic_fetch_or((_Atomic(uintptr_t)*)obj, value);
}


// --- _Py_atomic_load -------------------------------------------------------

static inline int
_Py_atomic_load_int(const int *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(int)*)obj);
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(int8_t)*)obj);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(int16_t)*)obj);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(int32_t)*)obj);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(int64_t)*)obj);
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(intptr_t)*)obj);
}

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(uint8_t)*)obj);
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(uint32_t)*)obj);
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(uint32_t)*)obj);
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(uint64_t)*)obj);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(uintptr_t)*)obj);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(unsigned int)*)obj);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(Py_ssize_t)*)obj);
}

static inline void*
_Py_atomic_load_ptr(const void *obj)
{
    _Py_USING_STD;
    return atomic_load((const _Atomic(void*)*)obj);
}


// --- _Py_atomic_load_relaxed -----------------------------------------------

static inline int
_Py_atomic_load_int_relaxed(const int *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int)*)obj,
                                memory_order_relaxed);
}

static inline char
_Py_atomic_load_char_relaxed(const char *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(char)*)obj,
                                memory_order_relaxed);
}

static inline unsigned char
_Py_atomic_load_uchar_relaxed(const unsigned char *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(unsigned char)*)obj,
                                memory_order_relaxed);
}

static inline short
_Py_atomic_load_short_relaxed(const short *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(short)*)obj,
                                memory_order_relaxed);
}

static inline unsigned short
_Py_atomic_load_ushort_relaxed(const unsigned short *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(unsigned short)*)obj,
                                memory_order_relaxed);
}

static inline long
_Py_atomic_load_long_relaxed(const long *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(long)*)obj,
                                memory_order_relaxed);
}

static inline float
_Py_atomic_load_float_relaxed(const float *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(float)*)obj,
                                memory_order_relaxed);
}

static inline double
_Py_atomic_load_double_relaxed(const double *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(double)*)obj,
                                memory_order_relaxed);
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int8_t)*)obj,
                                memory_order_relaxed);
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int16_t)*)obj,
                                memory_order_relaxed);
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int32_t)*)obj,
                                memory_order_relaxed);
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int64_t)*)obj,
                                memory_order_relaxed);
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(intptr_t)*)obj,
                                memory_order_relaxed);
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint8_t)*)obj,
                                memory_order_relaxed);
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint16_t)*)obj,
                                memory_order_relaxed);
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint32_t)*)obj,
                                memory_order_relaxed);
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint64_t)*)obj,
                                memory_order_relaxed);
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uintptr_t)*)obj,
                                memory_order_relaxed);
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(unsigned int)*)obj,
                                memory_order_relaxed);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(Py_ssize_t)*)obj,
                                memory_order_relaxed);
}

static inline void*
_Py_atomic_load_ptr_relaxed(const void *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(void*)*)obj,
                                memory_order_relaxed);
}

static inline unsigned long long
_Py_atomic_load_ullong_relaxed(const unsigned long long *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(unsigned long long)*)obj,
                                memory_order_relaxed);
}

static inline long long
_Py_atomic_load_llong_relaxed(const long long *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(long long)*)obj,
                                memory_order_relaxed);
}


// --- _Py_atomic_store ------------------------------------------------------

static inline void
_Py_atomic_store_int(int *obj, int value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(int)*)obj, value);
}

static inline void
_Py_atomic_store_int8(int8_t *obj, int8_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(int8_t)*)obj, value);
}

static inline void
_Py_atomic_store_int16(int16_t *obj, int16_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(int16_t)*)obj, value);
}

static inline void
_Py_atomic_store_int32(int32_t *obj, int32_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(int32_t)*)obj, value);
}

static inline void
_Py_atomic_store_int64(int64_t *obj, int64_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(int64_t)*)obj, value);
}

static inline void
_Py_atomic_store_intptr(intptr_t *obj, intptr_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(intptr_t)*)obj, value);
}

static inline void
_Py_atomic_store_uint8(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(uint8_t)*)obj, value);
}

static inline void
_Py_atomic_store_uint16(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(uint16_t)*)obj, value);
}

static inline void
_Py_atomic_store_uint32(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(uint32_t)*)obj, value);
}

static inline void
_Py_atomic_store_uint64(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(uint64_t)*)obj, value);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(uintptr_t)*)obj, value);
}

static inline void
_Py_atomic_store_uint(unsigned int *obj, unsigned int value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(unsigned int)*)obj, value);
}

static inline void
_Py_atomic_store_ptr(void *obj, void *value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(void*)*)obj, value);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_USING_STD;
    atomic_store((_Atomic(Py_ssize_t)*)obj, value);
}


// --- _Py_atomic_store_relaxed ----------------------------------------------

static inline void
_Py_atomic_store_int_relaxed(int *obj, int value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *obj, int8_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int8_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *obj, int16_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int16_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *obj, int32_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int32_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *obj, int64_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int64_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *obj, intptr_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(intptr_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *obj, uint8_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint8_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *obj, uint16_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint16_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint32_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint64_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uintptr_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *obj, unsigned int value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(unsigned int)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void *value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(void*)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(Py_ssize_t)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ullong_relaxed(unsigned long long *obj,
                                unsigned long long value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(unsigned long long)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_char_relaxed(char *obj, char value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(char)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uchar_relaxed(unsigned char *obj, unsigned char value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(unsigned char)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_short_relaxed(short *obj, short value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(short)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_ushort_relaxed(unsigned short *obj, unsigned short value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(unsigned short)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_uint_release(unsigned int *obj, unsigned int value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(unsigned int)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_long_relaxed(long *obj, long value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(long)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_float_relaxed(float *obj, float value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(float)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_double_relaxed(double *obj, double value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(double)*)obj, value,
                          memory_order_relaxed);
}

static inline void
_Py_atomic_store_llong_relaxed(long long *obj, long long value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(long long)*)obj, value,
                          memory_order_relaxed);
}


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

static inline void *
_Py_atomic_load_ptr_acquire(const void *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(void*)*)obj,
                                memory_order_acquire);
}

static inline uintptr_t
_Py_atomic_load_uintptr_acquire(const uintptr_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uintptr_t)*)obj,
                                memory_order_acquire);
}

static inline void
_Py_atomic_store_ptr_release(void *obj, void *value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(void*)*)obj, value,
                          memory_order_release);
}

static inline void
_Py_atomic_store_uintptr_release(uintptr_t *obj, uintptr_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uintptr_t)*)obj, value,
                          memory_order_release);
}

static inline void
_Py_atomic_store_int_release(int *obj, int value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(int)*)obj, value,
                          memory_order_release);
}

static inline void
_Py_atomic_store_ssize_release(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(Py_ssize_t)*)obj, value,
                          memory_order_release);
}

static inline int
_Py_atomic_load_int_acquire(const int *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(int)*)obj,
                                memory_order_acquire);
}

static inline void
_Py_atomic_store_uint32_release(uint32_t *obj, uint32_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint32_t)*)obj, value,
                          memory_order_release);
}

static inline void
_Py_atomic_store_uint64_release(uint64_t *obj, uint64_t value)
{
    _Py_USING_STD;
    atomic_store_explicit((_Atomic(uint64_t)*)obj, value,
                          memory_order_release);
}

static inline uint64_t
_Py_atomic_load_uint64_acquire(const uint64_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint64_t)*)obj,
                                memory_order_acquire);
}

static inline uint32_t
_Py_atomic_load_uint32_acquire(const uint32_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(uint32_t)*)obj,
                                memory_order_acquire);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_acquire(const Py_ssize_t *obj)
{
    _Py_USING_STD;
    return atomic_load_explicit((const _Atomic(Py_ssize_t)*)obj,
                                memory_order_acquire);
}


// --- _Py_atomic_fence ------------------------------------------------------

 static inline void
_Py_atomic_fence_seq_cst(void)
{
    _Py_USING_STD;
    atomic_thread_fence(memory_order_seq_cst);
}

 static inline void
_Py_atomic_fence_acquire(void)
{
    _Py_USING_STD;
    atomic_thread_fence(memory_order_acquire);
}

 static inline void
_Py_atomic_fence_release(void)
{
    _Py_USING_STD;
    atomic_thread_fence(memory_order_release);
}
