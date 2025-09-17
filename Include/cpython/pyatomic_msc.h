// This is the implementation of Python atomic operations for MSVC if the
// compiler does not support C11 or C++11 atomics.
//
// MSVC intrinsics are defined on char, short, long, __int64, and pointer
// types. Note that long and int are both 32-bits even on 64-bit Windows,
// so operations on int are cast to long.
//
// The volatile keyword has additional memory ordering semantics on MSVC. On
// x86 and x86-64, volatile accesses have acquire-release semantics. On ARM64,
// volatile accesses behave like C11's memory_order_relaxed.

#ifndef Py_ATOMIC_MSC_H
#  error "this header file must not be included directly"
#endif

#include <intrin.h>

#define _Py_atomic_ASSERT_ARG_TYPE(TYPE) \
    Py_BUILD_ASSERT(sizeof(*obj) == sizeof(TYPE))


// --- _Py_atomic_add --------------------------------------------------------

static inline int8_t
_Py_atomic_add_int8(int8_t *obj, int8_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(char);
    return (int8_t)_InterlockedExchangeAdd8((volatile char *)obj, (char)value);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *obj, int16_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(short);
    return (int16_t)_InterlockedExchangeAdd16((volatile short *)obj, (short)value);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *obj, int32_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(long);
    return (int32_t)_InterlockedExchangeAdd((volatile long *)obj, (long)value);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *obj, int64_t value)
{
#if defined(_M_X64) || defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    return (int64_t)_InterlockedExchangeAdd64((volatile __int64 *)obj, (__int64)value);
#else
    int64_t old_value = _Py_atomic_load_int64_relaxed(obj);
    for (;;) {
        int64_t new_value = old_value + value;
        if (_Py_atomic_compare_exchange_int64(obj, &old_value, new_value)) {
            return old_value;
        }
    }
#endif
}


static inline uint8_t
_Py_atomic_add_uint8(uint8_t *obj, uint8_t value)
{
    return (uint8_t)_Py_atomic_add_int8((int8_t *)obj, (int8_t)value);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *obj, uint16_t value)
{
    return (uint16_t)_Py_atomic_add_int16((int16_t *)obj, (int16_t)value);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *obj, uint32_t value)
{
    return (uint32_t)_Py_atomic_add_int32((int32_t *)obj, (int32_t)value);
}

static inline int
_Py_atomic_add_int(int *obj, int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return (int)_Py_atomic_add_int32((int32_t *)obj, (int32_t)value);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *obj, unsigned int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return (unsigned int)_Py_atomic_add_int32((int32_t *)obj, (int32_t)value);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *obj, uint64_t value)
{
    return (uint64_t)_Py_atomic_add_int64((int64_t *)obj, (int64_t)value);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *obj, intptr_t value)
{
#if SIZEOF_VOID_P == 8
    _Py_atomic_ASSERT_ARG_TYPE(int64_t);
    return (intptr_t)_Py_atomic_add_int64((int64_t *)obj, (int64_t)value);
#else
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return (intptr_t)_Py_atomic_add_int32((int32_t *)obj, (int32_t)value);
#endif
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(intptr_t);
    return (uintptr_t)_Py_atomic_add_intptr((intptr_t *)obj, (intptr_t)value);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(intptr_t);
    return (Py_ssize_t)_Py_atomic_add_intptr((intptr_t *)obj, (intptr_t)value);
}


// --- _Py_atomic_compare_exchange -------------------------------------------

static inline int
_Py_atomic_compare_exchange_int8(int8_t *obj, int8_t *expected, int8_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(char);
    int8_t initial = (int8_t)_InterlockedCompareExchange8(
                                       (volatile char *)obj,
                                       (char)value,
                                       (char)*expected);
    if (initial == *expected) {
        return 1;
    }
    *expected = initial;
    return 0;
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *obj, int16_t *expected, int16_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(short);
    int16_t initial = (int16_t)_InterlockedCompareExchange16(
                                       (volatile short *)obj,
                                       (short)value,
                                       (short)*expected);
    if (initial == *expected) {
        return 1;
    }
    *expected = initial;
    return 0;
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *obj, int32_t *expected, int32_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(long);
    int32_t initial = (int32_t)_InterlockedCompareExchange(
                                       (volatile long *)obj,
                                       (long)value,
                                       (long)*expected);
    if (initial == *expected) {
        return 1;
    }
    *expected = initial;
    return 0;
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *obj, int64_t *expected, int64_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    int64_t initial = (int64_t)_InterlockedCompareExchange64(
                                       (volatile __int64 *)obj,
                                       (__int64)value,
                                       (__int64)*expected);
    if (initial == *expected) {
        return 1;
    }
    *expected = initial;
    return 0;
}

static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *value)
{
    void *initial = _InterlockedCompareExchangePointer(
                                       (void**)obj,
                                       value,
                                       *(void**)expected);
    if (initial == *(void**)expected) {
        return 1;
    }
    *(void**)expected = initial;
    return 0;
}


static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t value)
{
    return _Py_atomic_compare_exchange_int8((int8_t *)obj,
                                            (int8_t *)expected,
                                            (int8_t)value);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *obj, uint16_t *expected, uint16_t value)
{
    return _Py_atomic_compare_exchange_int16((int16_t *)obj,
                                             (int16_t *)expected,
                                             (int16_t)value);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *obj, uint32_t *expected, uint32_t value)
{
    return _Py_atomic_compare_exchange_int32((int32_t *)obj,
                                             (int32_t *)expected,
                                             (int32_t)value);
}

static inline int
_Py_atomic_compare_exchange_int(int *obj, int *expected, int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return _Py_atomic_compare_exchange_int32((int32_t *)obj,
                                             (int32_t *)expected,
                                             (int32_t)value);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *obj, unsigned int *expected, unsigned int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return _Py_atomic_compare_exchange_int32((int32_t *)obj,
                                             (int32_t *)expected,
                                             (int32_t)value);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *obj, uint64_t *expected, uint64_t value)
{
    return _Py_atomic_compare_exchange_int64((int64_t *)obj,
                                             (int64_t *)expected,
                                             (int64_t)value);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *obj, intptr_t *expected, intptr_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return _Py_atomic_compare_exchange_ptr((void**)obj,
                                           (void**)expected,
                                           (void*)value);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *obj, uintptr_t *expected, uintptr_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return _Py_atomic_compare_exchange_ptr((void**)obj,
                                           (void**)expected,
                                           (void*)value);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *obj, Py_ssize_t *expected, Py_ssize_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return _Py_atomic_compare_exchange_ptr((void**)obj,
                                           (void**)expected,
                                           (void*)value);
}


// --- _Py_atomic_exchange ---------------------------------------------------

static inline int8_t
_Py_atomic_exchange_int8(int8_t *obj, int8_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(char);
    return (int8_t)_InterlockedExchange8((volatile char *)obj, (char)value);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *obj, int16_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(short);
    return (int16_t)_InterlockedExchange16((volatile short *)obj, (short)value);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *obj, int32_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(long);
    return (int32_t)_InterlockedExchange((volatile long *)obj, (long)value);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *obj, int64_t value)
{
#if defined(_M_X64) || defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    return (int64_t)_InterlockedExchange64((volatile __int64 *)obj, (__int64)value);
#else
    int64_t old_value = _Py_atomic_load_int64_relaxed(obj);
    for (;;) {
        if (_Py_atomic_compare_exchange_int64(obj, &old_value, value)) {
            return old_value;
        }
    }
#endif
}

static inline void*
_Py_atomic_exchange_ptr(void *obj, void *value)
{
    return (void*)_InterlockedExchangePointer((void * volatile *)obj, (void *)value);
}


static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *obj, uint8_t value)
{
    return (uint8_t)_Py_atomic_exchange_int8((int8_t *)obj,
                                             (int8_t)value);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *obj, uint16_t value)
{
    return (uint16_t)_Py_atomic_exchange_int16((int16_t *)obj,
                                               (int16_t)value);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *obj, uint32_t value)
{
    return (uint32_t)_Py_atomic_exchange_int32((int32_t *)obj,
                                               (int32_t)value);
}

static inline int
_Py_atomic_exchange_int(int *obj, int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return (int)_Py_atomic_exchange_int32((int32_t *)obj,
                                           (int32_t)value);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *obj, unsigned int value)
{
    _Py_atomic_ASSERT_ARG_TYPE(int32_t);
    return (unsigned int)_Py_atomic_exchange_int32((int32_t *)obj,
                                                   (int32_t)value);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *obj, uint64_t value)
{
    return (uint64_t)_Py_atomic_exchange_int64((int64_t *)obj,
                                               (int64_t)value);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *obj, intptr_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (intptr_t)_Py_atomic_exchange_ptr((void**)obj,
                                             (void*)value);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *obj, uintptr_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (uintptr_t)_Py_atomic_exchange_ptr((void**)obj,
                                              (void*)value);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (Py_ssize_t)_Py_atomic_exchange_ptr((void**)obj,
                                               (void*)value);
}


// --- _Py_atomic_and --------------------------------------------------------

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *obj, uint8_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(char);
    return (uint8_t)_InterlockedAnd8((volatile char *)obj, (char)value);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *obj, uint16_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(short);
    return (uint16_t)_InterlockedAnd16((volatile short *)obj, (short)value);
}

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *obj, uint32_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(long);
    return (uint32_t)_InterlockedAnd((volatile long *)obj, (long)value);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *obj, uint64_t value)
{
#if defined(_M_X64) || defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    return (uint64_t)_InterlockedAnd64((volatile __int64 *)obj, (__int64)value);
#else
    uint64_t old_value = _Py_atomic_load_uint64_relaxed(obj);
    for (;;) {
        uint64_t new_value = old_value & value;
        if (_Py_atomic_compare_exchange_uint64(obj, &old_value, new_value)) {
            return old_value;
        }
    }
#endif
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *obj, uintptr_t value)
{
#if SIZEOF_VOID_P == 8
    _Py_atomic_ASSERT_ARG_TYPE(uint64_t);
    return (uintptr_t)_Py_atomic_and_uint64((uint64_t *)obj,
                                            (uint64_t)value);
#else
    _Py_atomic_ASSERT_ARG_TYPE(uint32_t);
    return (uintptr_t)_Py_atomic_and_uint32((uint32_t *)obj,
                                            (uint32_t)value);
#endif
}


// --- _Py_atomic_or ---------------------------------------------------------

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *obj, uint8_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(char);
    return (uint8_t)_InterlockedOr8((volatile char *)obj, (char)value);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *obj, uint16_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(short);
    return (uint16_t)_InterlockedOr16((volatile short *)obj, (short)value);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *obj, uint32_t value)
{
    _Py_atomic_ASSERT_ARG_TYPE(long);
    return (uint32_t)_InterlockedOr((volatile long *)obj, (long)value);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *obj, uint64_t value)
{
#if defined(_M_X64) || defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    return (uint64_t)_InterlockedOr64((volatile __int64 *)obj, (__int64)value);
#else
    uint64_t old_value = _Py_atomic_load_uint64_relaxed(obj);
    for (;;) {
        uint64_t new_value = old_value | value;
        if (_Py_atomic_compare_exchange_uint64(obj, &old_value, new_value)) {
            return old_value;
        }
    }
#endif
}


static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *obj, uintptr_t value)
{
#if SIZEOF_VOID_P == 8
    _Py_atomic_ASSERT_ARG_TYPE(uint64_t);
    return (uintptr_t)_Py_atomic_or_uint64((uint64_t *)obj,
                                           (uint64_t)value);
#else
    _Py_atomic_ASSERT_ARG_TYPE(uint32_t);
    return (uintptr_t)_Py_atomic_or_uint32((uint32_t *)obj,
                                           (uint32_t)value);
#endif
}


// --- _Py_atomic_load -------------------------------------------------------

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(volatile uint8_t *)obj;
#elif defined(_M_ARM64)
    return (uint8_t)__ldar8((unsigned __int8 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint8"
#endif
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(volatile uint16_t *)obj;
#elif defined(_M_ARM64)
    return (uint16_t)__ldar16((unsigned __int16 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint16"
#endif
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(volatile uint32_t *)obj;
#elif defined(_M_ARM64)
    return (uint32_t)__ldar32((unsigned __int32 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint32"
#endif
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(volatile uint64_t *)obj;
#elif defined(_M_ARM64)
    return (uint64_t)__ldar64((unsigned __int64 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint64"
#endif
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *obj)
{
    return (int8_t)_Py_atomic_load_uint8((const uint8_t *)obj);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *obj)
{
    return (int16_t)_Py_atomic_load_uint16((const uint16_t *)obj);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *obj)
{
    return (int32_t)_Py_atomic_load_uint32((const uint32_t *)obj);
}

static inline int
_Py_atomic_load_int(const int *obj)
{
    _Py_atomic_ASSERT_ARG_TYPE(uint32_t);
    return (int)_Py_atomic_load_uint32((uint32_t *)obj);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *obj)
{
    _Py_atomic_ASSERT_ARG_TYPE(uint32_t);
    return (unsigned int)_Py_atomic_load_uint32((uint32_t *)obj);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *obj)
{
    return (int64_t)_Py_atomic_load_uint64((const uint64_t *)obj);
}

static inline void*
_Py_atomic_load_ptr(const void *obj)
{
#if SIZEOF_VOID_P == 8
    return (void*)_Py_atomic_load_uint64((const uint64_t *)obj);
#else
    return (void*)_Py_atomic_load_uint32((const uint32_t *)obj);
#endif
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *obj)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (intptr_t)_Py_atomic_load_ptr((void*)obj);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *obj)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (uintptr_t)_Py_atomic_load_ptr((void*)obj);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *obj)
{
    _Py_atomic_ASSERT_ARG_TYPE(void*);
    return (Py_ssize_t)_Py_atomic_load_ptr((void*)obj);
}


// --- _Py_atomic_load_relaxed -----------------------------------------------

static inline int
_Py_atomic_load_int_relaxed(const int *obj)
{
    return *(volatile int *)obj;
}

static inline char
_Py_atomic_load_char_relaxed(const char *obj)
{
    return *(volatile char *)obj;
}

static inline unsigned char
_Py_atomic_load_uchar_relaxed(const unsigned char *obj)
{
    return *(volatile unsigned char *)obj;
}

static inline short
_Py_atomic_load_short_relaxed(const short *obj)
{
    return *(volatile short *)obj;
}

static inline unsigned short
_Py_atomic_load_ushort_relaxed(const unsigned short *obj)
{
    return *(volatile unsigned short *)obj;
}

static inline long
_Py_atomic_load_long_relaxed(const long *obj)
{
    return *(volatile long *)obj;
}

static inline float
_Py_atomic_load_float_relaxed(const float *obj)
{
    return *(volatile float *)obj;
}

static inline double
_Py_atomic_load_double_relaxed(const double *obj)
{
    return *(volatile double *)obj;
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *obj)
{
    return *(volatile int8_t *)obj;
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *obj)
{
    return *(volatile int16_t *)obj;
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *obj)
{
    return *(volatile int32_t *)obj;
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *obj)
{
    return *(volatile int64_t *)obj;
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *obj)
{
    return *(volatile intptr_t *)obj;
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *obj)
{
    return *(volatile uint8_t *)obj;
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *obj)
{
    return *(volatile uint16_t *)obj;
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *obj)
{
    return *(volatile uint32_t *)obj;
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *obj)
{
    return *(volatile uint64_t *)obj;
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *obj)
{
    return *(volatile uintptr_t *)obj;
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *obj)
{
    return *(volatile unsigned int *)obj;
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *obj)
{
    return *(volatile Py_ssize_t *)obj;
}

static inline void*
_Py_atomic_load_ptr_relaxed(const void *obj)
{
    return *(void * volatile *)obj;
}

static inline unsigned long long
_Py_atomic_load_ullong_relaxed(const unsigned long long *obj)
{
    return *(volatile unsigned long long *)obj;
}

static inline long long
_Py_atomic_load_llong_relaxed(const long long *obj)
{
    return *(volatile long long *)obj;
}


// --- _Py_atomic_store ------------------------------------------------------

static inline void
_Py_atomic_store_int(int *obj, int value)
{
    (void)_Py_atomic_exchange_int(obj, value);
}

static inline void
_Py_atomic_store_int8(int8_t *obj, int8_t value)
{
    (void)_Py_atomic_exchange_int8(obj, value);
}

static inline void
_Py_atomic_store_int16(int16_t *obj, int16_t value)
{
    (void)_Py_atomic_exchange_int16(obj, value);
}

static inline void
_Py_atomic_store_int32(int32_t *obj, int32_t value)
{
    (void)_Py_atomic_exchange_int32(obj, value);
}

static inline void
_Py_atomic_store_int64(int64_t *obj, int64_t value)
{
    (void)_Py_atomic_exchange_int64(obj, value);
}

static inline void
_Py_atomic_store_intptr(intptr_t *obj, intptr_t value)
{
    (void)_Py_atomic_exchange_intptr(obj, value);
}

static inline void
_Py_atomic_store_uint8(uint8_t *obj, uint8_t value)
{
    (void)_Py_atomic_exchange_uint8(obj, value);
}

static inline void
_Py_atomic_store_uint16(uint16_t *obj, uint16_t value)
{
    (void)_Py_atomic_exchange_uint16(obj, value);
}

static inline void
_Py_atomic_store_uint32(uint32_t *obj, uint32_t value)
{
    (void)_Py_atomic_exchange_uint32(obj, value);
}

static inline void
_Py_atomic_store_uint64(uint64_t *obj, uint64_t value)
{
    (void)_Py_atomic_exchange_uint64(obj, value);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *obj, uintptr_t value)
{
    (void)_Py_atomic_exchange_uintptr(obj, value);
}

static inline void
_Py_atomic_store_uint(unsigned int *obj, unsigned int value)
{
    (void)_Py_atomic_exchange_uint(obj, value);
}

static inline void
_Py_atomic_store_ptr(void *obj, void *value)
{
    (void)_Py_atomic_exchange_ptr(obj, value);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *obj, Py_ssize_t value)
{
    (void)_Py_atomic_exchange_ssize(obj, value);
}


// --- _Py_atomic_store_relaxed ----------------------------------------------

static inline void
_Py_atomic_store_int_relaxed(int *obj, int value)
{
    *(volatile int *)obj = value;
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *obj, int8_t value)
{
    *(volatile int8_t *)obj = value;
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *obj, int16_t value)
{
    *(volatile int16_t *)obj = value;
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *obj, int32_t value)
{
    *(volatile int32_t *)obj = value;
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *obj, int64_t value)
{
    *(volatile int64_t *)obj = value;
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *obj, intptr_t value)
{
    *(volatile intptr_t *)obj = value;
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *obj, uint8_t value)
{
    *(volatile uint8_t *)obj = value;
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *obj, uint16_t value)
{
    *(volatile uint16_t *)obj = value;
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *obj, uint32_t value)
{
    *(volatile uint32_t *)obj = value;
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *obj, uint64_t value)
{
    *(volatile uint64_t *)obj = value;
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *obj, uintptr_t value)
{
    *(volatile uintptr_t *)obj = value;
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *obj, unsigned int value)
{
    *(volatile unsigned int *)obj = value;
}

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void* value)
{
    *(void * volatile *)obj = value;
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *obj, Py_ssize_t value)
{
    *(volatile Py_ssize_t *)obj = value;
}

static inline void
_Py_atomic_store_ullong_relaxed(unsigned long long *obj,
                                unsigned long long value)
{
    *(volatile unsigned long long *)obj = value;
}

static inline void
_Py_atomic_store_char_relaxed(char *obj, char value)
{
    *(volatile char *)obj = value;
}

static inline void
_Py_atomic_store_uchar_relaxed(unsigned char *obj, unsigned char value)
{
    *(volatile unsigned char *)obj = value;
}

static inline void
_Py_atomic_store_short_relaxed(short *obj, short value)
{
    *(volatile short *)obj = value;
}

static inline void
_Py_atomic_store_ushort_relaxed(unsigned short *obj, unsigned short value)
{
    *(volatile unsigned short *)obj = value;
}

static inline void
_Py_atomic_store_uint_release(unsigned int *obj, unsigned int value)
{
    *(volatile unsigned int *)obj = value;
}

static inline void
_Py_atomic_store_long_relaxed(long *obj, long value)
{
    *(volatile long *)obj = value;
}

static inline void
_Py_atomic_store_float_relaxed(float *obj, float value)
{
    *(volatile float *)obj = value;
}

static inline void
_Py_atomic_store_double_relaxed(double *obj, double value)
{
    *(volatile double *)obj = value;
}

static inline void
_Py_atomic_store_llong_relaxed(long long *obj, long long value)
{
    *(volatile long long *)obj = value;
}


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

static inline void *
_Py_atomic_load_ptr_acquire(const void *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(void * volatile *)obj;
#elif defined(_M_ARM64)
    return (void *)__ldar64((unsigned __int64 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_ptr_acquire"
#endif
}

static inline uintptr_t
_Py_atomic_load_uintptr_acquire(const uintptr_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(uintptr_t volatile *)obj;
#elif defined(_M_ARM64)
    return (uintptr_t)__ldar64((unsigned __int64 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uintptr_acquire"
#endif
}

static inline void
_Py_atomic_store_ptr_release(void *obj, void *value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(void * volatile *)obj = value;
#elif defined(_M_ARM64)
    __stlr64((unsigned __int64 volatile *)obj, (uintptr_t)value);
#else
#  error "no implementation of _Py_atomic_store_ptr_release"
#endif
}

static inline void
_Py_atomic_store_uintptr_release(uintptr_t *obj, uintptr_t value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(uintptr_t volatile *)obj = value;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(unsigned __int64);
    __stlr64((unsigned __int64 volatile *)obj, (unsigned __int64)value);
#else
#  error "no implementation of _Py_atomic_store_uintptr_release"
#endif
}

static inline void
_Py_atomic_store_int_release(int *obj, int value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(int volatile *)obj = value;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(unsigned __int32);
    __stlr32((unsigned __int32 volatile *)obj, (unsigned __int32)value);
#else
#  error "no implementation of _Py_atomic_store_int_release"
#endif
}

static inline void
_Py_atomic_store_ssize_release(Py_ssize_t *obj, Py_ssize_t value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(Py_ssize_t volatile *)obj = value;
#elif defined(_M_ARM64)
    __stlr64((unsigned __int64 volatile *)obj, (unsigned __int64)value);
#else
#  error "no implementation of _Py_atomic_store_ssize_release"
#endif
}

static inline int
_Py_atomic_load_int_acquire(const int *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(int volatile *)obj;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(unsigned __int32);
    return (int)__ldar32((unsigned __int32 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_int_acquire"
#endif
}

static inline void
_Py_atomic_store_uint32_release(uint32_t *obj, uint32_t value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(uint32_t volatile *)obj = value;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(unsigned __int32);
    __stlr32((unsigned __int32 volatile *)obj, (unsigned __int32)value);
#else
#  error "no implementation of _Py_atomic_store_uint32_release"
#endif
}

static inline void
_Py_atomic_store_uint64_release(uint64_t *obj, uint64_t value)
{
#if defined(_M_X64) || defined(_M_IX86)
    *(uint64_t volatile *)obj = value;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(unsigned __int64);
    __stlr64((unsigned __int64 volatile *)obj, (unsigned __int64)value);
#else
#  error "no implementation of _Py_atomic_store_uint64_release"
#endif
}

static inline uint64_t
_Py_atomic_load_uint64_acquire(const uint64_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(uint64_t volatile *)obj;
#elif defined(_M_ARM64)
    _Py_atomic_ASSERT_ARG_TYPE(__int64);
    return (uint64_t)__ldar64((unsigned __int64 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint64_acquire"
#endif
}

static inline uint32_t
_Py_atomic_load_uint32_acquire(const uint32_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(uint32_t volatile *)obj;
#elif defined(_M_ARM64)
    return (uint32_t)__ldar32((uint32_t volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_uint32_acquire"
#endif
}

static inline Py_ssize_t
_Py_atomic_load_ssize_acquire(const Py_ssize_t *obj)
{
#if defined(_M_X64) || defined(_M_IX86)
    return *(Py_ssize_t volatile *)obj;
#elif defined(_M_ARM64)
    return (Py_ssize_t)__ldar64((unsigned __int64 volatile *)obj);
#else
#  error "no implementation of _Py_atomic_load_ssize_acquire"
#endif
}

// --- _Py_atomic_fence ------------------------------------------------------

 static inline void
_Py_atomic_fence_seq_cst(void)
{
#if defined(_M_ARM64)
    __dmb(_ARM64_BARRIER_ISH);
#elif defined(_M_X64)
    __faststorefence();
#elif defined(_M_IX86)
    _mm_mfence();
#else
#  error "no implementation of _Py_atomic_fence_seq_cst"
#endif
}

 static inline void
_Py_atomic_fence_acquire(void)
{
#if defined(_M_ARM64)
    __dmb(_ARM64_BARRIER_ISHLD);
#elif defined(_M_X64) || defined(_M_IX86)
    _ReadBarrier();
#else
#  error "no implementation of _Py_atomic_fence_acquire"
#endif
}

 static inline void
_Py_atomic_fence_release(void)
{
#if defined(_M_ARM64)
    __dmb(_ARM64_BARRIER_ISH);
#elif defined(_M_X64) || defined(_M_IX86)
    _ReadWriteBarrier();
#else
#  error "no implementation of _Py_atomic_fence_release"
#endif
}

#undef _Py_atomic_ASSERT_ARG_TYPE
