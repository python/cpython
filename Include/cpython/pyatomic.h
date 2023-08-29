// This header provides cross-platform low-level atomic operations
// similar to C11 atomics.
//
// Operations are sequentially consistent unless they have a suffix indicating
// otherwise. If in doubt, prefer the sequentially consistent operations.
//
// The "_relaxed" suffix for load and store operations indicates the "relaxed"
// memory order. They don't provide synchronization, but (roughly speaking)
// guarantee somewhat sane behavior for races instead of undefined behavior.
// In practice, they correspond to "normal" hardware load and store
// instructions, so they are almost as inexpensive as plain loads and stores
// in C.
//
// Note that atomic read-modify-write operations like _Py_atomic_add_* return
// the previous value of the atomic variable, not the new value.
//
// See https://en.cppreference.com/w/c/atomic for more information on C11
// atomics.
// See https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2055r0.pdf
// "A Relaxed Guide to memory_order_relaxed" for discussion of and common usage
// or relaxed atomics.

#ifndef Py_ATOMIC_H
#define Py_ATOMIC_H

// Atomically adds `value` to `ptr` and returns the previous value
static inline int
_Py_atomic_add_int(int *ptr, int value);

static inline int8_t
_Py_atomic_add_int8(int8_t *ptr, int8_t value);

static inline int16_t
_Py_atomic_add_int16(int16_t *ptr, int16_t value);

static inline int32_t
_Py_atomic_add_int32(int32_t *ptr, int32_t value);

static inline int64_t
_Py_atomic_add_int64(int64_t *ptr, int64_t value);

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *ptr, intptr_t value);

static inline unsigned int
_Py_atomic_add_uint(unsigned int *ptr, unsigned int value);

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *ptr, uint8_t value);

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *ptr, uint16_t value);

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *ptr, uint32_t value);

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *ptr, uint64_t value);

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *ptr, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *ptr, Py_ssize_t value);

// Performs an atomic compare-and-exchange. If `*ptr` and `*expected` are
// equal, then `desired` is stored in `*ptr`. Otherwise `*expected` is updated
// with the current value of `*ptr`. Returns 1 on success and 0 on failure.
// These correspond to the "strong" variations of the C11
// atomic_compare_exchange_* functions.
static inline int
_Py_atomic_compare_exchange_int(int *ptr, int *expected, int desired);

static inline int
_Py_atomic_compare_exchange_int8(int8_t *ptr, int8_t *expected, int8_t desired);

static inline int
_Py_atomic_compare_exchange_int16(int16_t *ptr, int16_t *expected, int16_t desired);

static inline int
_Py_atomic_compare_exchange_int32(int32_t *ptr, int32_t *expected, int32_t desired);

static inline int
_Py_atomic_compare_exchange_int64(int64_t *ptr, int64_t *expected, int64_t desired);

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *ptr, intptr_t *expected, intptr_t desired);

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *ptr, unsigned int *expected, unsigned int desired);

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *ptr, uint8_t *expected, uint8_t desired);

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *ptr, uint16_t *expected, uint16_t desired);

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *ptr, uint32_t *expected, uint32_t desired);

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *ptr, uint64_t *expected, uint64_t desired);

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *ptr, uintptr_t *expected, uintptr_t desired);

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t *expected, Py_ssize_t desired);

// NOTE: `ptr` and `expected` are logically `void**` types, but we use `void*`
// so that we can pass types like `PyObject**` without a cast.
static inline int
_Py_atomic_compare_exchange_ptr(void *ptr, void *expected, void *value);

// Atomically replaces `*ptr` with `value` and returns the previous value of `*ptr`.
static inline int
_Py_atomic_exchange_int(int *ptr, int value);

static inline int8_t
_Py_atomic_exchange_int8(int8_t *ptr, int8_t value);

static inline int16_t
_Py_atomic_exchange_int16(int16_t *ptr, int16_t value);

static inline int32_t
_Py_atomic_exchange_int32(int32_t *ptr, int32_t value);

static inline int64_t
_Py_atomic_exchange_int64(int64_t *ptr, int64_t value);

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *ptr, intptr_t value);

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *ptr, unsigned int value);

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *ptr, uint8_t value);

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *ptr, uint16_t value);

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *ptr, uint32_t value);

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *ptr, uint64_t value);

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *ptr, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *ptr, Py_ssize_t value);

static inline void *
_Py_atomic_exchange_ptr(void *ptr, void *value);

// Performs `*ptr &= value` atomically and returns the previous value of `*ptr`.
static inline uint8_t
_Py_atomic_and_uint8(uint8_t *ptr, uint8_t value);

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *ptr, uint16_t value);

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *ptr, uint32_t value);

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *ptr, uint64_t value);

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *ptr, uintptr_t value);

// Performs `*ptr |= value` atomically and returns the previous value of `*ptr`.
static inline uint8_t
_Py_atomic_or_uint8(uint8_t *ptr, uint8_t value);

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *ptr, uint16_t value);

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *ptr, uint32_t value);

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *ptr, uint64_t value);

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *ptr, uintptr_t value);

// Atomically loads `*ptr` (sequential consistency)
static inline int
_Py_atomic_load_int(const int *ptr);

static inline int8_t
_Py_atomic_load_int8(const int8_t *ptr);

static inline int16_t
_Py_atomic_load_int16(const int16_t *ptr);

static inline int32_t
_Py_atomic_load_int32(const int32_t *ptr);

static inline int64_t
_Py_atomic_load_int64(const int64_t *ptr);

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *ptr);

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *ptr);

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *ptr);

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *ptr);

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *ptr);

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *ptr);

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *ptr);

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *ptr);

static inline void *
_Py_atomic_load_ptr(const void *ptr);

// Loads `*ptr` (relaxed consistency, i.e., no ordering)
static inline int
_Py_atomic_load_int_relaxed(const int *ptr);

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *ptr);

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *ptr);

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *ptr);

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *ptr);

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *ptr);

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *ptr);

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *ptr);

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *ptr);

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *ptr);

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *ptr);

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *ptr);

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *ptr);

static inline void *
_Py_atomic_load_ptr_relaxed(const void *ptr);

// Atomically performs `*ptr = value` (sequential consistency)
static inline void
_Py_atomic_store_int(int *ptr, int value);

static inline void
_Py_atomic_store_int8(int8_t *ptr, int8_t value);

static inline void
_Py_atomic_store_int16(int16_t *ptr, int16_t value);

static inline void
_Py_atomic_store_int32(int32_t *ptr, int32_t value);

static inline void
_Py_atomic_store_int64(int64_t *ptr, int64_t value);

static inline void
_Py_atomic_store_intptr(intptr_t *ptr, intptr_t value);

static inline void
_Py_atomic_store_uint8(uint8_t *ptr, uint8_t value);

static inline void
_Py_atomic_store_uint16(uint16_t *ptr, uint16_t value);

static inline void
_Py_atomic_store_uint32(uint32_t *ptr, uint32_t value);

static inline void
_Py_atomic_store_uint64(uint64_t *ptr, uint64_t value);

static inline void
_Py_atomic_store_uintptr(uintptr_t *ptr, uintptr_t value);

static inline void
_Py_atomic_store_uint(unsigned int *ptr, unsigned int value);

static inline void
_Py_atomic_store_ptr(void *ptr, void *value);

static inline void
_Py_atomic_store_ssize(Py_ssize_t* ptr, Py_ssize_t value);

// Stores `*ptr = value` (relaxed consistency, i.e., no ordering)
static inline void
_Py_atomic_store_int_relaxed(int *ptr, int value);

static inline void
_Py_atomic_store_int8_relaxed(int8_t *ptr, int8_t value);

static inline void
_Py_atomic_store_int16_relaxed(int16_t *ptr, int16_t value);

static inline void
_Py_atomic_store_int32_relaxed(int32_t *ptr, int32_t value);

static inline void
_Py_atomic_store_int64_relaxed(int64_t *ptr, int64_t value);

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *ptr, intptr_t value);

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t* ptr, uint8_t value);

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *ptr, uint16_t value);

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *ptr, uint32_t value);

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *ptr, uint64_t value);

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *ptr, uintptr_t value);

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *ptr, unsigned int value);

static inline void
_Py_atomic_store_ptr_relaxed(void *ptr, void *value);

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *ptr, Py_ssize_t value);


// Loads `*ptr` (acquire operation)
static inline void *
_Py_atomic_load_ptr_acquire(const void *ptr);

// Stores `*ptr = value` (release operation)
static inline void
_Py_atomic_store_ptr_release(void *ptr, void *value);


// Sequential consistency fence. C11 fences have complex semantics. When
// possible, use the atomic operations on variables defined above, which
// generally do not require explicit use of a fence.
// See https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence
static inline void _Py_atomic_fence_seq_cst(void);

// Release fence
static inline void _Py_atomic_fence_release(void);


#ifndef _Py_USE_GCC_BUILTIN_ATOMICS
#  if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
#    define _Py_USE_GCC_BUILTIN_ATOMICS 1
#  elif defined(__clang__)
#    if __has_builtin(__atomic_load)
#      define _Py_USE_GCC_BUILTIN_ATOMICS 1
#    endif
#  endif
#endif

#if _Py_USE_GCC_BUILTIN_ATOMICS
#  define Py_ATOMIC_GCC_H
#  include "cpython/pyatomic_gcc.h"
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define Py_ATOMIC_STD_H
#  include "cpython/pyatomic_std.h"
#elif defined(_MSC_VER)
#  define Py_ATOMIC_MSC_H
#  include "cpython/pyatomic_msc.h"
#else
#  error "no available pyatomic implementation for this platform/compiler"
#endif

#endif  /* Py_ATOMIC_H */

