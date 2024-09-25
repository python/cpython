// This is the implementation of Python atomic operations using GCC's built-in
// functions that match the C+11 memory model. This implementation is preferred
// for GCC compatible compilers, such as Clang. These functions are available
// in GCC 4.8+ without needing to compile with --std=c11 or --std=gnu11.

#ifndef Py_ATOMIC_GCC_H
#  error "this header file must not be included directly"
#endif


// --- _Py_atomic_add --------------------------------------------------------

static inline int
_Py_atomic_add_int(int *obj, int value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline int8_t
_Py_atomic_add_int8(int8_t *obj, int8_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline int16_t
_Py_atomic_add_int16(int16_t *obj, int16_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline int32_t
_Py_atomic_add_int32(int32_t *obj, int32_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline int64_t
_Py_atomic_add_int64(int64_t *obj, int64_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *obj, intptr_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline unsigned int
_Py_atomic_add_uint(unsigned int *obj, unsigned int value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *obj, uint8_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *obj, uint16_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *obj, uint32_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *obj, uint64_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *obj, uintptr_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *obj, Py_ssize_t value)
{ return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_compare_exchange -------------------------------------------

static inline int
_Py_atomic_compare_exchange_int(int *obj, int *expected, int desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_int8(int8_t *obj, int8_t *expected, int8_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_int16(int16_t *obj, int16_t *expected, int16_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_int32(int32_t *obj, int32_t *expected, int32_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_int64(int64_t *obj, int64_t *expected, int64_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *obj, intptr_t *expected, intptr_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *obj, unsigned int *expected, unsigned int desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *obj, uint16_t *expected, uint16_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *obj, uint32_t *expected, uint32_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *obj, uint64_t *expected, uint64_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *obj, uintptr_t *expected, uintptr_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *obj, Py_ssize_t *expected, Py_ssize_t desired)
{ return __atomic_compare_exchange_n(obj, expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }

static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *desired)
{ return __atomic_compare_exchange_n((void **)obj, (void **)expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_exchange ---------------------------------------------------

static inline int
_Py_atomic_exchange_int(int *obj, int value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline int8_t
_Py_atomic_exchange_int8(int8_t *obj, int8_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline int16_t
_Py_atomic_exchange_int16(int16_t *obj, int16_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline int32_t
_Py_atomic_exchange_int32(int32_t *obj, int32_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline int64_t
_Py_atomic_exchange_int64(int64_t *obj, int64_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *obj, intptr_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *obj, unsigned int value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *obj, uint8_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *obj, uint16_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *obj, uint32_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *obj, uint64_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *obj, uintptr_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *obj, Py_ssize_t value)
{ return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void *
_Py_atomic_exchange_ptr(void *obj, void *value)
{ return __atomic_exchange_n((void **)obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_and --------------------------------------------------------

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *obj, uint8_t value)
{ return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *obj, uint16_t value)
{ return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *obj, uint32_t value)
{ return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *obj, uint64_t value)
{ return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *obj, uintptr_t value)
{ return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_or ---------------------------------------------------------

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *obj, uint8_t value)
{ return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *obj, uint16_t value)
{ return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *obj, uint32_t value)
{ return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *obj, uint64_t value)
{ return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *obj, uintptr_t value)
{ return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_load -------------------------------------------------------

static inline int
_Py_atomic_load_int(const int *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline int8_t
_Py_atomic_load_int8(const int8_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline int16_t
_Py_atomic_load_int16(const int16_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline int32_t
_Py_atomic_load_int32(const int32_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline int64_t
_Py_atomic_load_int64(const int64_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

static inline void *
_Py_atomic_load_ptr(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_load_relaxed -----------------------------------------------

static inline int
_Py_atomic_load_int_relaxed(const int *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }

static inline void *
_Py_atomic_load_ptr_relaxed(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_RELAXED); }

static inline unsigned long long
_Py_atomic_load_ullong_relaxed(const unsigned long long *obj)
{ return __atomic_load_n(obj, __ATOMIC_RELAXED); }


// --- _Py_atomic_store ------------------------------------------------------

static inline void
_Py_atomic_store_int(int *obj, int value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_int8(int8_t *obj, int8_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_int16(int16_t *obj, int16_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_int32(int32_t *obj, int32_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_int64(int64_t *obj, int64_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_intptr(intptr_t *obj, intptr_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uint8(uint8_t *obj, uint8_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uint16(uint16_t *obj, uint16_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uint32(uint32_t *obj, uint32_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uint64(uint64_t *obj, uint64_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uintptr(uintptr_t *obj, uintptr_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_uint(unsigned int *obj, unsigned int value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_ptr(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_SEQ_CST); }

static inline void
_Py_atomic_store_ssize(Py_ssize_t *obj, Py_ssize_t value)
{ __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_store_relaxed ----------------------------------------------

static inline void
_Py_atomic_store_int_relaxed(int *obj, int value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_int8_relaxed(int8_t *obj, int8_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_int16_relaxed(int16_t *obj, int16_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_int32_relaxed(int32_t *obj, int32_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_int64_relaxed(int64_t *obj, int64_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *obj, intptr_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t *obj, uint8_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *obj, uint16_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *obj, uint32_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *obj, uint64_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *obj, uintptr_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *obj, unsigned int value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *obj, Py_ssize_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

static inline void
_Py_atomic_store_ullong_relaxed(unsigned long long *obj,
                                unsigned long long value)
{ __atomic_store_n(obj, value, __ATOMIC_RELAXED); }


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

static inline void *
_Py_atomic_load_ptr_acquire(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_ACQUIRE); }

static inline uintptr_t
_Py_atomic_load_uintptr_acquire(const uintptr_t *obj)
{ return (uintptr_t)__atomic_load_n(obj, __ATOMIC_ACQUIRE); }

static inline void
_Py_atomic_store_ptr_release(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_RELEASE); }

static inline void
_Py_atomic_store_uintptr_release(uintptr_t *obj, uintptr_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

static inline void
_Py_atomic_store_int_release(int *obj, int value)
{ __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

static inline void
_Py_atomic_store_ssize_release(Py_ssize_t *obj, Py_ssize_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

static inline int
_Py_atomic_load_int_acquire(const int *obj)
{ return __atomic_load_n(obj, __ATOMIC_ACQUIRE); }

static inline void
_Py_atomic_store_uint32_release(uint32_t *obj, uint32_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

static inline void
_Py_atomic_store_uint64_release(uint64_t *obj, uint64_t value)
{ __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

static inline uint64_t
_Py_atomic_load_uint64_acquire(const uint64_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_ACQUIRE); }

static inline uint32_t
_Py_atomic_load_uint32_acquire(const uint32_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_ACQUIRE); }

static inline Py_ssize_t
_Py_atomic_load_ssize_acquire(const Py_ssize_t *obj)
{ return __atomic_load_n(obj, __ATOMIC_ACQUIRE); }

// --- _Py_atomic_fence ------------------------------------------------------

static inline void
_Py_atomic_fence_seq_cst(void)
{ __atomic_thread_fence(__ATOMIC_SEQ_CST); }

 static inline void
_Py_atomic_fence_acquire(void)
{ __atomic_thread_fence(__ATOMIC_ACQUIRE); }

 static inline void
_Py_atomic_fence_release(void)
{ __atomic_thread_fence(__ATOMIC_RELEASE); }
