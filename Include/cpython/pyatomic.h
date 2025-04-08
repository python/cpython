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
//
// Functions with pseudo Python code:
//
//   def _Py_atomic_load(obj):
//        return obj  # sequential consistency
//
//   def _Py_atomic_load_relaxed(obj):
//       return obj  # relaxed consistency
//
//   def _Py_atomic_store(obj, value):
//       obj = value  # sequential consistency
//
//   def _Py_atomic_store_relaxed(obj, value):
//       obj = value  # relaxed consistency
//
//   def _Py_atomic_exchange(obj, value):
//       # sequential consistency
//       old_obj = obj
//       obj = value
//       return old_obj
//
//   def _Py_atomic_compare_exchange(obj, expected, desired):
//       # sequential consistency
//       if obj == expected:
//           obj = desired
//           return True
//       else:
//           expected = obj
//           return False
//
//   def _Py_atomic_add(obj, value):
//       # sequential consistency
//       old_obj = obj
//       obj += value
//       return old_obj
//
//   def _Py_atomic_and(obj, value):
//       # sequential consistency
//       old_obj = obj
//       obj &= value
//       return old_obj
//
//   def _Py_atomic_or(obj, value):
//       # sequential consistency
//       old_obj = obj
//       obj |= value
//       return old_obj
//
// Other functions:
//
//   def _Py_atomic_load_ptr_acquire(obj):
//       return obj  # acquire
//
//   def _Py_atomic_store_ptr_release(obj):
//       return obj  # release
//
//   def _Py_atomic_fence_seq_cst():
//       # sequential consistency
//       ...
//
//   def _Py_atomic_fence_release():
//       # release
//       ...

#ifndef Py_CPYTHON_ATOMIC_H
#  error "this header file must not be included directly"
#endif

// --- _Py_atomic_add --------------------------------------------------------
// Atomically adds `value` to `obj` and returns the previous value

static inline int
_Py_atomic_add_int(int *obj, int value);

static inline int8_t
_Py_atomic_add_int8(int8_t *obj, int8_t value);

static inline int16_t
_Py_atomic_add_int16(int16_t *obj, int16_t value);

static inline int32_t
_Py_atomic_add_int32(int32_t *obj, int32_t value);

static inline int64_t
_Py_atomic_add_int64(int64_t *obj, int64_t value);

static inline intptr_t
_Py_atomic_add_intptr(intptr_t *obj, intptr_t value);

static inline unsigned int
_Py_atomic_add_uint(unsigned int *obj, unsigned int value);

static inline uint8_t
_Py_atomic_add_uint8(uint8_t *obj, uint8_t value);

static inline uint16_t
_Py_atomic_add_uint16(uint16_t *obj, uint16_t value);

static inline uint32_t
_Py_atomic_add_uint32(uint32_t *obj, uint32_t value);

static inline uint64_t
_Py_atomic_add_uint64(uint64_t *obj, uint64_t value);

static inline uintptr_t
_Py_atomic_add_uintptr(uintptr_t *obj, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_add_ssize(Py_ssize_t *obj, Py_ssize_t value);


// --- _Py_atomic_compare_exchange -------------------------------------------
// Performs an atomic compare-and-exchange.
//
// - If `*obj` and `*expected` are equal, store `desired` into `*obj`
//   and return 1 (success).
// - Otherwise, store the `*obj` current value into `*expected`
//   and return 0 (failure).
//
// These correspond to the C11 atomic_compare_exchange_strong() function.

static inline int
_Py_atomic_compare_exchange_int(int *obj, int *expected, int desired);

static inline int
_Py_atomic_compare_exchange_int8(int8_t *obj, int8_t *expected, int8_t desired);

static inline int
_Py_atomic_compare_exchange_int16(int16_t *obj, int16_t *expected, int16_t desired);

static inline int
_Py_atomic_compare_exchange_int32(int32_t *obj, int32_t *expected, int32_t desired);

static inline int
_Py_atomic_compare_exchange_int64(int64_t *obj, int64_t *expected, int64_t desired);

static inline int
_Py_atomic_compare_exchange_intptr(intptr_t *obj, intptr_t *expected, intptr_t desired);

static inline int
_Py_atomic_compare_exchange_uint(unsigned int *obj, unsigned int *expected, unsigned int desired);

static inline int
_Py_atomic_compare_exchange_uint8(uint8_t *obj, uint8_t *expected, uint8_t desired);

static inline int
_Py_atomic_compare_exchange_uint16(uint16_t *obj, uint16_t *expected, uint16_t desired);

static inline int
_Py_atomic_compare_exchange_uint32(uint32_t *obj, uint32_t *expected, uint32_t desired);

static inline int
_Py_atomic_compare_exchange_uint64(uint64_t *obj, uint64_t *expected, uint64_t desired);

static inline int
_Py_atomic_compare_exchange_uintptr(uintptr_t *obj, uintptr_t *expected, uintptr_t desired);

static inline int
_Py_atomic_compare_exchange_ssize(Py_ssize_t *obj, Py_ssize_t *expected, Py_ssize_t desired);

// NOTE: `obj` and `expected` are logically `void**` types, but we use `void*`
// so that we can pass types like `PyObject**` without a cast.
static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *value);


// --- _Py_atomic_exchange ---------------------------------------------------
// Atomically replaces `*obj` with `value` and returns the previous value of `*obj`.

static inline int
_Py_atomic_exchange_int(int *obj, int value);

static inline int8_t
_Py_atomic_exchange_int8(int8_t *obj, int8_t value);

static inline int16_t
_Py_atomic_exchange_int16(int16_t *obj, int16_t value);

static inline int32_t
_Py_atomic_exchange_int32(int32_t *obj, int32_t value);

static inline int64_t
_Py_atomic_exchange_int64(int64_t *obj, int64_t value);

static inline intptr_t
_Py_atomic_exchange_intptr(intptr_t *obj, intptr_t value);

static inline unsigned int
_Py_atomic_exchange_uint(unsigned int *obj, unsigned int value);

static inline uint8_t
_Py_atomic_exchange_uint8(uint8_t *obj, uint8_t value);

static inline uint16_t
_Py_atomic_exchange_uint16(uint16_t *obj, uint16_t value);

static inline uint32_t
_Py_atomic_exchange_uint32(uint32_t *obj, uint32_t value);

static inline uint64_t
_Py_atomic_exchange_uint64(uint64_t *obj, uint64_t value);

static inline uintptr_t
_Py_atomic_exchange_uintptr(uintptr_t *obj, uintptr_t value);

static inline Py_ssize_t
_Py_atomic_exchange_ssize(Py_ssize_t *obj, Py_ssize_t value);

static inline void *
_Py_atomic_exchange_ptr(void *obj, void *value);


// --- _Py_atomic_and --------------------------------------------------------
// Performs `*obj &= value` atomically and returns the previous value of `*obj`.

static inline uint8_t
_Py_atomic_and_uint8(uint8_t *obj, uint8_t value);

static inline uint16_t
_Py_atomic_and_uint16(uint16_t *obj, uint16_t value);

static inline uint32_t
_Py_atomic_and_uint32(uint32_t *obj, uint32_t value);

static inline uint64_t
_Py_atomic_and_uint64(uint64_t *obj, uint64_t value);

static inline uintptr_t
_Py_atomic_and_uintptr(uintptr_t *obj, uintptr_t value);


// --- _Py_atomic_or ---------------------------------------------------------
// Performs `*obj |= value` atomically and returns the previous value of `*obj`.

static inline uint8_t
_Py_atomic_or_uint8(uint8_t *obj, uint8_t value);

static inline uint16_t
_Py_atomic_or_uint16(uint16_t *obj, uint16_t value);

static inline uint32_t
_Py_atomic_or_uint32(uint32_t *obj, uint32_t value);

static inline uint64_t
_Py_atomic_or_uint64(uint64_t *obj, uint64_t value);

static inline uintptr_t
_Py_atomic_or_uintptr(uintptr_t *obj, uintptr_t value);


// --- _Py_atomic_load -------------------------------------------------------
// Atomically loads `*obj` (sequential consistency)

static inline int
_Py_atomic_load_int(const int *obj);

static inline int8_t
_Py_atomic_load_int8(const int8_t *obj);

static inline int16_t
_Py_atomic_load_int16(const int16_t *obj);

static inline int32_t
_Py_atomic_load_int32(const int32_t *obj);

static inline int64_t
_Py_atomic_load_int64(const int64_t *obj);

static inline intptr_t
_Py_atomic_load_intptr(const intptr_t *obj);

static inline uint8_t
_Py_atomic_load_uint8(const uint8_t *obj);

static inline uint16_t
_Py_atomic_load_uint16(const uint16_t *obj);

static inline uint32_t
_Py_atomic_load_uint32(const uint32_t *obj);

static inline uint64_t
_Py_atomic_load_uint64(const uint64_t *obj);

static inline uintptr_t
_Py_atomic_load_uintptr(const uintptr_t *obj);

static inline unsigned int
_Py_atomic_load_uint(const unsigned int *obj);

static inline Py_ssize_t
_Py_atomic_load_ssize(const Py_ssize_t *obj);

static inline void *
_Py_atomic_load_ptr(const void *obj);


// --- _Py_atomic_load_relaxed -----------------------------------------------
// Loads `*obj` (relaxed consistency, i.e., no ordering)

static inline int
_Py_atomic_load_int_relaxed(const int *obj);

static inline int8_t
_Py_atomic_load_int8_relaxed(const int8_t *obj);

static inline int16_t
_Py_atomic_load_int16_relaxed(const int16_t *obj);

static inline int32_t
_Py_atomic_load_int32_relaxed(const int32_t *obj);

static inline int64_t
_Py_atomic_load_int64_relaxed(const int64_t *obj);

static inline intptr_t
_Py_atomic_load_intptr_relaxed(const intptr_t *obj);

static inline uint8_t
_Py_atomic_load_uint8_relaxed(const uint8_t *obj);

static inline uint16_t
_Py_atomic_load_uint16_relaxed(const uint16_t *obj);

static inline uint32_t
_Py_atomic_load_uint32_relaxed(const uint32_t *obj);

static inline uint64_t
_Py_atomic_load_uint64_relaxed(const uint64_t *obj);

static inline uintptr_t
_Py_atomic_load_uintptr_relaxed(const uintptr_t *obj);

static inline unsigned int
_Py_atomic_load_uint_relaxed(const unsigned int *obj);

static inline Py_ssize_t
_Py_atomic_load_ssize_relaxed(const Py_ssize_t *obj);

static inline void *
_Py_atomic_load_ptr_relaxed(const void *obj);

static inline unsigned long long
_Py_atomic_load_ullong_relaxed(const unsigned long long *obj);

// --- _Py_atomic_store ------------------------------------------------------
// Atomically performs `*obj = value` (sequential consistency)

static inline void
_Py_atomic_store_int(int *obj, int value);

static inline void
_Py_atomic_store_int8(int8_t *obj, int8_t value);

static inline void
_Py_atomic_store_int16(int16_t *obj, int16_t value);

static inline void
_Py_atomic_store_int32(int32_t *obj, int32_t value);

static inline void
_Py_atomic_store_int64(int64_t *obj, int64_t value);

static inline void
_Py_atomic_store_intptr(intptr_t *obj, intptr_t value);

static inline void
_Py_atomic_store_uint8(uint8_t *obj, uint8_t value);

static inline void
_Py_atomic_store_uint16(uint16_t *obj, uint16_t value);

static inline void
_Py_atomic_store_uint32(uint32_t *obj, uint32_t value);

static inline void
_Py_atomic_store_uint64(uint64_t *obj, uint64_t value);

static inline void
_Py_atomic_store_uintptr(uintptr_t *obj, uintptr_t value);

static inline void
_Py_atomic_store_uint(unsigned int *obj, unsigned int value);

static inline void
_Py_atomic_store_ptr(void *obj, void *value);

static inline void
_Py_atomic_store_ssize(Py_ssize_t* obj, Py_ssize_t value);


// --- _Py_atomic_store_relaxed ----------------------------------------------
// Stores `*obj = value` (relaxed consistency, i.e., no ordering)

static inline void
_Py_atomic_store_int_relaxed(int *obj, int value);

static inline void
_Py_atomic_store_int8_relaxed(int8_t *obj, int8_t value);

static inline void
_Py_atomic_store_int16_relaxed(int16_t *obj, int16_t value);

static inline void
_Py_atomic_store_int32_relaxed(int32_t *obj, int32_t value);

static inline void
_Py_atomic_store_int64_relaxed(int64_t *obj, int64_t value);

static inline void
_Py_atomic_store_intptr_relaxed(intptr_t *obj, intptr_t value);

static inline void
_Py_atomic_store_uint8_relaxed(uint8_t* obj, uint8_t value);

static inline void
_Py_atomic_store_uint16_relaxed(uint16_t *obj, uint16_t value);

static inline void
_Py_atomic_store_uint32_relaxed(uint32_t *obj, uint32_t value);

static inline void
_Py_atomic_store_uint64_relaxed(uint64_t *obj, uint64_t value);

static inline void
_Py_atomic_store_uintptr_relaxed(uintptr_t *obj, uintptr_t value);

static inline void
_Py_atomic_store_uint_relaxed(unsigned int *obj, unsigned int value);

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void *value);

static inline void
_Py_atomic_store_ssize_relaxed(Py_ssize_t *obj, Py_ssize_t value);

static inline void
_Py_atomic_store_ullong_relaxed(unsigned long long *obj,
                                unsigned long long value);


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

// Loads `*obj` (acquire operation)
static inline void *
_Py_atomic_load_ptr_acquire(const void *obj);

static inline uintptr_t
_Py_atomic_load_uintptr_acquire(const uintptr_t *obj);

// Stores `*obj = value` (release operation)
static inline void
_Py_atomic_store_ptr_release(void *obj, void *value);

static inline void
_Py_atomic_store_uintptr_release(uintptr_t *obj, uintptr_t value);

static inline void
_Py_atomic_store_ssize_release(Py_ssize_t *obj, Py_ssize_t value);

static inline void
_Py_atomic_store_int_release(int *obj, int value);

static inline int
_Py_atomic_load_int_acquire(const int *obj);

static inline void
_Py_atomic_store_uint32_release(uint32_t *obj, uint32_t value);

static inline void
_Py_atomic_store_uint64_release(uint64_t *obj, uint64_t value);

static inline uint64_t
_Py_atomic_load_uint64_acquire(const uint64_t *obj);

static inline uint32_t
_Py_atomic_load_uint32_acquire(const uint32_t *obj);

static inline Py_ssize_t
_Py_atomic_load_ssize_acquire(const Py_ssize_t *obj);




// --- _Py_atomic_fence ------------------------------------------------------

// Sequential consistency fence. C11 fences have complex semantics. When
// possible, use the atomic operations on variables defined above, which
// generally do not require explicit use of a fence.
// See https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence
static inline void _Py_atomic_fence_seq_cst(void);

// Acquire fence
static inline void _Py_atomic_fence_acquire(void);

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
#  include "pyatomic_gcc.h"
#  undef Py_ATOMIC_GCC_H
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define Py_ATOMIC_STD_H
#  include "pyatomic_std.h"
#  undef Py_ATOMIC_STD_H
#elif defined(_MSC_VER)
#  define Py_ATOMIC_MSC_H
#  include "pyatomic_msc.h"
#  undef Py_ATOMIC_MSC_H
#else
#  error "no available pyatomic implementation for this platform/compiler"
#endif


// --- aliases ---------------------------------------------------------------

#if SIZEOF_LONG == 8
# define _Py_atomic_load_ulong(p) \
    _Py_atomic_load_uint64((uint64_t *)p)
# define _Py_atomic_load_ulong_relaxed(p) \
    _Py_atomic_load_uint64_relaxed((uint64_t *)p)
# define _Py_atomic_store_ulong(p, v) \
    _Py_atomic_store_uint64((uint64_t *)p, v)
# define _Py_atomic_store_ulong_relaxed(p, v) \
    _Py_atomic_store_uint64_relaxed((uint64_t *)p, v)
#elif SIZEOF_LONG == 4
# define _Py_atomic_load_ulong(p) \
    _Py_atomic_load_uint32((uint32_t *)p)
# define _Py_atomic_load_ulong_relaxed(p) \
    _Py_atomic_load_uint32_relaxed((uint32_t *)p)
# define _Py_atomic_store_ulong(p, v) \
    _Py_atomic_store_uint32((uint32_t *)p, v)
# define _Py_atomic_store_ulong_relaxed(p, v) \
    _Py_atomic_store_uint32_relaxed((uint32_t *)p, v)
#else
# error "long must be 4 or 8 bytes in size"
#endif  // SIZEOF_LONG
