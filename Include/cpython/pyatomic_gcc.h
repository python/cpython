// This is the implementation of Python atomic operations using GCC's built-in
// functions that match the C+11 memory model. This implementation is preferred
// for GCC compatible compilers, such as Clang. These functions are available
// in GCC 4.8+ without needing to compile with --std=c11 or --std=gnu11.

#ifndef Py_ATOMIC_GCC_H
#  error "this header file must not be included directly"
#endif

static inline int
_Py_atomic_add_int(int *address, int value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_add_int8(int8_t *address, int8_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_add_int16(int16_t *address, int16_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_add_int32(int32_t *address, int32_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_add_int64(int64_t *address, int64_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *address, intptr_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_add_uint(unsigned int *address, unsigned int value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *address, uint8_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *address, uint16_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *address, uint32_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *address, uint64_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *address, uintptr_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    return __atomic_fetch_add(address, value, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_compare_exchange_int(int *address, int expected, int value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int8(int8_t *address, int8_t expected, int8_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int16(int16_t *address, int16_t expected, int16_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int32(int32_t *address, int32_t expected, int32_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_int64(int64_t *address, int64_t expected, int64_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *address, intptr_t expected, intptr_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *address, unsigned int expected, unsigned int value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *address, uint8_t expected, uint8_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *address, uint16_t expected, uint16_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *address, uint32_t expected, uint32_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *address, uint64_t expected, uint64_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *address, uintptr_t expected, uintptr_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *address, Py_ssize_t expected, Py_ssize_t value)
{
    return __atomic_compare_exchange_n(address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_compare_exchange_ptr(void *address, void *expected, void *value)
{
    return __atomic_compare_exchange_n((void **)address, &expected, value, 0,
                                       __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_exchange_int(int *address, int value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_exchange_int8(int8_t *address, int8_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_exchange_int16(int16_t *address, int16_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_exchange_int32(int32_t *address, int32_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_exchange_int64(int64_t *address, int64_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *address, intptr_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *address, unsigned int value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *address, uint8_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *address, uint16_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *address, uint32_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *address, uint64_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *address, uintptr_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    return __atomic_exchange_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void *
_Py_atomic_exchange_ptr(void *address, void *value)
{
    return __atomic_exchange_n((void **)address, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *address, uint8_t value)
{
    return __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *address, uint16_t value)
{
    return __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *address, uint32_t value)
{
    return __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *address, uint64_t value)
{
    return __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *address, uintptr_t value)
{
    return __atomic_fetch_and(address, value, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *address, uint8_t value)
{
    return __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *address, uint16_t value)
{
    return __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *address, uint32_t value)
{
    return __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *address, uint64_t value)
{
    return __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *address, uintptr_t value)
{
    return __atomic_fetch_or(address, value, __ATOMIC_SEQ_CST);
}

static inline int
_Py_atomic_load_int(const int *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline int8_t
_Py_atomic_load_int8(const int8_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline int16_t
_Py_atomic_load_int16(const int16_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline int32_t
_Py_atomic_load_int32(const int32_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline int64_t
_Py_atomic_load_int64(const int64_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *address)
{
    return __atomic_load_n(address, __ATOMIC_SEQ_CST);
}

static inline void *
_Py_atomic_load_ptr(const void *address)
{
    return (void *)__atomic_load_n((void **)address, __ATOMIC_SEQ_CST);
}


static inline int
_Py_atomic_load_int_relaxed(const int *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *address)
{
    return __atomic_load_n(address, __ATOMIC_RELAXED);
}

static inline void *
_Py_atomic_load_ptr_relaxed(const void *address)
{
     return (void *)__atomic_load_n((const void **)address, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int(int *address, int value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int8(int8_t *address, int8_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int16(int16_t *address, int16_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int32(int32_t *address, int32_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int64(int64_t *address, int64_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_intptr(intptr_t *address, intptr_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint8(uint8_t *address, uint8_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint16(uint16_t *address, uint16_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint32(uint32_t *address, uint32_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint64(uint64_t *address, uint64_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uintptr(uintptr_t *address, uintptr_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_uint(unsigned int *address, unsigned int value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_ptr(void *address, void *value)
{
    __atomic_store_n((void **)address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_ssize(Py_ssize_t *address, Py_ssize_t value)
{
    __atomic_store_n(address, value, __ATOMIC_SEQ_CST);
}

static inline void
_Py_atomic_store_int_relaxed(int *address, int value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int8_relaxed(int8_t *address, int8_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int16_relaxed(int16_t *address, int16_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int32_relaxed(int32_t *address, int32_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_int64_relaxed(int64_t *address, int64_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *address, intptr_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *address, uint8_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *address, uint16_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *address, uint32_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *address, uint64_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *address, uintptr_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *address, unsigned int value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_ptr_relaxed(void *address, void *value)
{
    __atomic_store_n((void **)address, value, __ATOMIC_RELAXED);
}

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *address, Py_ssize_t value)
{
    __atomic_store_n(address, value, __ATOMIC_RELAXED);
}

static inline void *
_Py_atomic_load_ptr_acquire(const void *address)
{
    return (void *)__atomic_load_n((void **)address, __ATOMIC_ACQUIRE);
}

static inline void
_Py_atomic_store_ptr_release(void *address, void *value)
{
    __atomic_store_n((void **)address, value, __ATOMIC_RELEASE);
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
