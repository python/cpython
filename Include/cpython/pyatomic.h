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

// Atomically adds `value` to `address` and returns the previous value
static inline int
_Py_atomic_add_int(int *address, int value);

static inline int8_t
_Py_atomic_add_int8(int8_t *address, int8_t value);

static inline int16_t
_Py_atomic_add_int16(int16_t *address, int16_t value);

static inline int32_t
_Py_atomic_add_int32(int32_t *address, int32_t value);

static inline int64_t
_Py_atomic_add_int64(int64_t *address, int64_t value);

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *address, intptr_t value);

static inline unsigned int
_Py_atomic_add_uint(unsigned int *address, unsigned int value);

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *address, uint8_t value);

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *address, uint16_t value);

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *address, uint32_t value);

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *address, uint64_t value);

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *address, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *address, Py_ssize_t value);

// Performs an atomic compare-and-exchange. If `*address` and `expected` are equal,
// then `value` is stored in `*address`. Returns 1 on success and 0 on failure.
// These correspond to the "strong" variations of the C11 atomic_compare_exchange_* functions.
static inline int
_Py_atomic_compare_exchange_int(int *address, int expected, int value);

static inline int
_Py_atomic_compare_exchange_int8(int8_t *address, int8_t expected, int8_t value);

static inline int
_Py_atomic_compare_exchange_int16(int16_t *address, int16_t expected, int16_t value);

static inline int
_Py_atomic_compare_exchange_int32(int32_t *address, int32_t expected, int32_t value);

static inline int
_Py_atomic_compare_exchange_int64(int64_t *address, int64_t expected, int64_t value);

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *address, intptr_t expected, intptr_t value);

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *address, unsigned int expected, unsigned int value);

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *address, uint8_t expected, uint8_t value);

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *address, uint16_t expected, uint16_t value);

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *address, uint32_t expected, uint32_t value);

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *address, uint64_t expected, uint64_t value);

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *address, uintptr_t expected, uintptr_t value);

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *address, Py_ssize_t expected, Py_ssize_t value);

static inline int
_Py_atomic_compare_exchange_ptr(void *address, void *expected, void *value);

// Atomically replaces `*address` with `value` and returns the previous value of `*address`.
static inline int
_Py_atomic_exchange_int(int *address, int value);

static inline int8_t
_Py_atomic_exchange_int8(int8_t *address, int8_t value);

static inline int16_t
_Py_atomic_exchange_int16(int16_t *address, int16_t value);

static inline int32_t
_Py_atomic_exchange_int32(int32_t *address, int32_t value);

static inline int64_t
_Py_atomic_exchange_int64(int64_t *address, int64_t value);

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *address, intptr_t value);

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *address, unsigned int value);

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *address, uint8_t value);

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *address, uint16_t value);

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *address, uint32_t value);

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *address, uint64_t value);

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *address, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *address, Py_ssize_t value);

static inline void *
_Py_atomic_exchange_ptr(void *address, void *value);

// Performs `*address &= value` atomically and returns the previous value of `*address`.
static inline uint8_t
_Py_atomic_and_uint8(uint8_t *address, uint8_t value);

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *address, uint16_t value);

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *address, uint32_t value);

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *address, uint64_t value);

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *address, uintptr_t value);

// Performs `*address |= value` atomically and returns the previous value of `*address`.
static inline uint8_t
_Py_atomic_or_uint8(uint8_t *address, uint8_t value);

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *address, uint16_t value);

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *address, uint32_t value);

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *address, uint64_t value);

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *address, uintptr_t value);

// Atomically loads `*address` (sequential consistency)
static inline int
_Py_atomic_load_int(const int *address);

static inline int8_t
_Py_atomic_load_int8(const int8_t *address);

static inline int16_t
_Py_atomic_load_int16(const int16_t *address);

static inline int32_t
_Py_atomic_load_int32(const int32_t *address);

static inline int64_t
_Py_atomic_load_int64(const int64_t *address);

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *address);

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *address);

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *address);

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *address);

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *address);

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *address);

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *address);

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *address);

static inline void *
_Py_atomic_load_ptr(const void *address);

// Loads `*address` (relaxed consistency, i.e., no ordering)
static inline int
_Py_atomic_load_int_relaxed(const int *address);

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *address);

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *address);

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *address);

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *address);

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *address);

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *address);

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *address);

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *address);

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *address);

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *address);

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *address);

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *address);

static inline void *
_Py_atomic_load_ptr_relaxed(const void *address);

// Atomically performs `*address = value` (sequential consistency)
static inline void
_Py_atomic_store_int(int *address, int value);

static inline void
_Py_atomic_store_int8(int8_t *address, int8_t value);

static inline void
_Py_atomic_store_int16(int16_t *address, int16_t value);

static inline void
_Py_atomic_store_int32(int32_t *address, int32_t value);

static inline void
_Py_atomic_store_int64(int64_t *address, int64_t value);

static inline void
_Py_atomic_store_intptr(intptr_t *address, intptr_t value);

static inline void
_Py_atomic_store_uint8(uint8_t *address, uint8_t value);

static inline void
_Py_atomic_store_uint16(uint16_t *address, uint16_t value);

static inline void
_Py_atomic_store_uint32(uint32_t *address, uint32_t value);

static inline void
_Py_atomic_store_uint64(uint64_t *address, uint64_t value);

static inline void
_Py_atomic_store_uintptr(uintptr_t *address, uintptr_t value);

static inline void
_Py_atomic_store_uint(unsigned int *address, unsigned int value);

static inline void
_Py_atomic_store_ptr(void *address, void *value);

static inline void
_Py_atomic_store_ssize(Py_ssize_t* address, Py_ssize_t value);

// Stores `*address = value` (relaxed consistency, i.e., no ordering)
static inline void
_Py_atomic_store_int_relaxed(int *address, int value);

static inline void
_Py_atomic_store_int8_relaxed(int8_t *address, int8_t value);

static inline void
_Py_atomic_store_int16_relaxed(int16_t *address, int16_t value);

static inline void
_Py_atomic_store_int32_relaxed(int32_t *address, int32_t value);

static inline void
_Py_atomic_store_int64_relaxed(int64_t *address, int64_t value);

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *address, intptr_t value);

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t* address, uint8_t value);

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *address, uint16_t value);

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *address, uint32_t value);

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *address, uint64_t value);

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *address, uintptr_t value);

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *address, unsigned int value);

static inline void
_Py_atomic_store_ptr_relaxed(void *address, void *value);

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *address, Py_ssize_t value);

// Stores `*address = value` (release operation)
static inline void
_Py_atomic_store_uint64_release(uint64_t *address, uint64_t value);

static inline void
_Py_atomic_store_ptr_release(void *address, void *value);


// Sequential consistency fence
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

