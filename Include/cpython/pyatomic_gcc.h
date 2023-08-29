// This is the implementation of Python atomic operations using GCC's built-in
// functions that match the C+11 memory model. This implementation is preferred
// for GCC compatible compilers, such as Clang. These functions are available
// in GCC 4.8+ without needing to compile with --std=c11 or --std=gnu11.

#ifndef Py_ATOMIC_GCC_H
#  error "this header file must not be included directly"
#endif

static inline int
_Py_atomic_add_int(int *ptr, int value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_add_int8(int8_t *ptr, int8_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *ptr, int16_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *ptr, int32_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *ptr, int64_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *ptr, intptr_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *ptr, unsigned int value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *ptr, uint8_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *ptr, uint16_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *ptr, uint64_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *ptr, uintptr_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    return __atomic_fetch_add(ptr, value, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_compare_exchange_int(int *ptr, int *expected, int desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int8(int8_t *ptr, int8_t *expected, int8_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *ptr, int16_t *expected, int16_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *ptr, int32_t *expected, int32_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *ptr, int64_t *expected, int64_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *ptr, intptr_t *expected, intptr_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *ptr, unsigned int *expected, unsigned int desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *ptr, uint8_t *expected, uint8_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *ptr, uint16_t *expected, uint16_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *ptr, uint32_t *expected, uint32_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *ptr, uint64_t *expected, uint64_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *ptr, uintptr_t *expected, uintptr_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t *expected, Py_ssize_t desired)
{
    return __atomic_compare_exchange_n(ptr, expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_ptr(void *ptr, void *expected, void *desired)
{
    return __atomic_compare_exchange_n((void **)ptr, (void **)expected, desired, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_exchange_int(int *ptr, int value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_exchange_int8(int8_t *ptr, int8_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *ptr, int16_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *ptr, int32_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *ptr, int64_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *ptr, intptr_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *ptr, unsigned int value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *ptr, uint8_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *ptr, uint16_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *ptr, uint32_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *ptr, uint64_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *ptr, uintptr_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    return __atomic_exchange_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void *
_Py_atomic_exchange_ptr(void *ptr, void *value)
{
    return __atomic_exchange_n((void **)ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *ptr, uint8_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *ptr, uint16_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *ptr, uint64_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *ptr, uintptr_t value)
{
    return __atomic_fetch_and(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *ptr, uint8_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *ptr, uint16_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *ptr, uint32_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *ptr, uint64_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *ptr, uintptr_t value)
{
    return __atomic_fetch_or(ptr, value, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_load_int(const int *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline void *
_Py_atomic_load_ptr(const void *ptr)
{
    return (void *)__atomic_load_n((void **)ptr, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_load_int_relaxed(const int *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *ptr)
{
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
}

static inline void *
_Py_atomic_load_ptr_relaxed(const void *ptr)
{
     return (void *)__atomic_load_n((const void **)ptr, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int(int *ptr, int value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int8(int8_t *ptr, int8_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int16(int16_t *ptr, int16_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int32(int32_t *ptr, int32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int64(int64_t *ptr, int64_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_intptr(intptr_t *ptr, intptr_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint8(uint8_t *ptr, uint8_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint16(uint16_t *ptr, uint16_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint32(uint32_t *ptr, uint32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint64(uint64_t *ptr, uint64_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *ptr, uintptr_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint(unsigned int *ptr, unsigned int value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_ptr(void *ptr, void *value)
{
    __atomic_store_n((void **)ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *ptr, Py_ssize_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int_relaxed(int *ptr, int value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *ptr, int8_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *ptr, int16_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *ptr, int32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *ptr, int64_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *ptr, intptr_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *ptr, uint8_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *ptr, uint16_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *ptr, uint32_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *ptr, uint64_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *ptr, uintptr_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *ptr, unsigned int value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_ptr_relaxed(void *ptr, void *value)
{
    __atomic_store_n((void **)ptr, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *ptr, Py_ssize_t value)
{
    __atomic_store_n(ptr, value, __ATOMIC_RELAXED);
}

static inline void *
_Py_atomic_load_ptr_acquire(const void *ptr)
{
    return (void *)__atomic_load_n((void **)ptr, __ATOMIC_ACQUIRE);
}

static inline void
_Py_atomic_store_ptr_release(void *ptr, void *value)
{
    __atomic_store_n((void **)ptr, value, __ATOMIC_RELEASE);
}

 static inline void
_Py_atomic_fence_seq_cst(void)
{
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
}

 static inline void
_Py_atomic_fence_release(void)
{
    __atomic_thread_fence(__ATOMIC_RELEASE);
}
