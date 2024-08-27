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

#define PY_PROTOTYPE_ATOMIC_ADD(TYPENAME, SUFFIX)               \
    static inline TYPENAME                                      \
    _Py_atomic_add_ ## SUFFIX (TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_ADD(int,                            int)
PY_PROTOTYPE_ATOMIC_ADD(int8_t,                         int8)
PY_PROTOTYPE_ATOMIC_ADD(int16_t,                        int16)
PY_PROTOTYPE_ATOMIC_ADD(int32_t,                        int32)
PY_PROTOTYPE_ATOMIC_ADD(int64_t,                        int64)
PY_PROTOTYPE_ATOMIC_ADD(intptr_t,                       intptr)

PY_PROTOTYPE_ATOMIC_ADD(unsigned int,                   uint)
PY_PROTOTYPE_ATOMIC_ADD(uint8_t,                        uint8)
PY_PROTOTYPE_ATOMIC_ADD(uint16_t,                       uint16)
PY_PROTOTYPE_ATOMIC_ADD(uint32_t,                       uint32)
PY_PROTOTYPE_ATOMIC_ADD(uint64_t,                       uint64)
PY_PROTOTYPE_ATOMIC_ADD(uintptr_t,                      uintptr)
PY_PROTOTYPE_ATOMIC_ADD(Py_ssize_t,                     ssize)

#undef PY_PROTOTYPE_ATOMIC_ADD


// --- _Py_atomic_compare_exchange -------------------------------------------
// Performs an atomic compare-and-exchange.
//
// - If `*obj` and `*expected` are equal, store `desired` into `*obj`
//   and return 1 (success).
// - Otherwise, store the `*obj` current value into `*expected`
//   and return 0 (failure).
//
// These correspond to the C11 atomic_compare_exchange_strong() function.

#define PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(TYPENAME, SUFFIX)  \
    static inline int                                           \
    _Py_atomic_compare_exchange_ ## SUFFIX (TYPENAME *obj,      \
                                            TYPENAME *expected, \
                                            TYPENAME desired);

PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(int,               int)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(int8_t,            int8)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(int16_t,           int16)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(int32_t,           int32)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(int64_t,           int64)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(intptr_t,          intptr)

PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(unsigned int,      uint)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(uint8_t,           uint8)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(uint16_t,          uint16)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(uint32_t,          uint32)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(uint64_t,          uint64)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(uintptr_t,         uintptr)
PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE(Py_ssize_t,        ssize)

#undef PY_PROTOTYPE_ATOMIC_COMPARE_EXCHANGE

// NOTE: `obj` and `expected` are logically `void**` types, but we use `void*`
// so that we can pass types like `PyObject**` without a cast.
static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *value);


// --- _Py_atomic_exchange ---------------------------------------------------
// Atomically replaces `*obj` with `value` and returns the previous value of `*obj`.

#define PY_PROTOTYPE_ATOMIC_EXCHANGE(TYPENAME, SUFFIX)              \
    static inline TYPENAME                                          \
    _Py_atomic_exchange_ ## SUFFIX (TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_EXCHANGE(int,                       int)
PY_PROTOTYPE_ATOMIC_EXCHANGE(int8_t,                    int8)
PY_PROTOTYPE_ATOMIC_EXCHANGE(int16_t,                   int16)
PY_PROTOTYPE_ATOMIC_EXCHANGE(int32_t,                   int32)
PY_PROTOTYPE_ATOMIC_EXCHANGE(int64_t,                   int64)
PY_PROTOTYPE_ATOMIC_EXCHANGE(intptr_t,                  intptr)

PY_PROTOTYPE_ATOMIC_EXCHANGE(unsigned int,              uint)
PY_PROTOTYPE_ATOMIC_EXCHANGE(uint8_t,                   uint8)
PY_PROTOTYPE_ATOMIC_EXCHANGE(uint16_t,                  uint16)
PY_PROTOTYPE_ATOMIC_EXCHANGE(uint32_t,                  uint32)
PY_PROTOTYPE_ATOMIC_EXCHANGE(uint64_t,                  uint64)
PY_PROTOTYPE_ATOMIC_EXCHANGE(uintptr_t,                 uintptr)
PY_PROTOTYPE_ATOMIC_EXCHANGE(Py_ssize_t,                ssize)

#undef PY_PROTOTYPE_ATOMIC_EXCHANGE

static inline void *
_Py_atomic_exchange_ptr(void *obj, void *value);


// --- _Py_atomic_and --------------------------------------------------------
// Performs `*obj &= value` atomically and returns the previous value of `*obj`.

#define PY_PROTOTYPE_ATOMIC_AND(TYPENAME, SUFFIX)               \
    static inline TYPENAME                                      \
    _Py_atomic_and_ ## SUFFIX (TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_AND(uint8_t,                        uint8)
PY_PROTOTYPE_ATOMIC_AND(uint16_t,                       uint16)
PY_PROTOTYPE_ATOMIC_AND(uint32_t,                       uint32)
PY_PROTOTYPE_ATOMIC_AND(uint64_t,                       uint64)
PY_PROTOTYPE_ATOMIC_AND(uintptr_t,                      uintptr)

#undef PY_PROTOTYPE_ATOMIC_AND


// --- _Py_atomic_or ---------------------------------------------------------
// Performs `*obj |= value` atomically and returns the previous value of `*obj`.

#define PY_PROTOTYPE_ATOMIC_OR(TYPENAME, SUFFIX)                \
    static inline TYPENAME                                      \
    _Py_atomic_or_ ## SUFFIX (TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_OR(uint8_t,                         uint8)
PY_PROTOTYPE_ATOMIC_OR(uint16_t,                        uint16)
PY_PROTOTYPE_ATOMIC_OR(uint32_t,                        uint32)
PY_PROTOTYPE_ATOMIC_OR(uint64_t,                        uint64)
PY_PROTOTYPE_ATOMIC_OR(uintptr_t,                       uintptr)

#undef PY_PROTOTYPE_ATOMIC_OR


// --- _Py_atomic_load -------------------------------------------------------
// Atomically loads `*obj` (sequential consistency)

#define PY_PROTOTYPE_ATOMIC_LOAD(TYPENAME, SUFFIX)      \
    static inline TYPENAME                              \
    _Py_atomic_load_ ## SUFFIX (const TYPENAME *obj);

PY_PROTOTYPE_ATOMIC_LOAD(int,                           int)
PY_PROTOTYPE_ATOMIC_LOAD(int8_t,                        int8)
PY_PROTOTYPE_ATOMIC_LOAD(int16_t,                       int16)
PY_PROTOTYPE_ATOMIC_LOAD(int32_t,                       int32)
PY_PROTOTYPE_ATOMIC_LOAD(int64_t,                       int64)
PY_PROTOTYPE_ATOMIC_LOAD(intptr_t,                      intptr)

PY_PROTOTYPE_ATOMIC_LOAD(unsigned int,                  uint)
PY_PROTOTYPE_ATOMIC_LOAD(uint8_t,                       uint8)
PY_PROTOTYPE_ATOMIC_LOAD(uint16_t,                      uint16)
PY_PROTOTYPE_ATOMIC_LOAD(uint32_t,                      uint32)
PY_PROTOTYPE_ATOMIC_LOAD(uint64_t,                      uint64)
PY_PROTOTYPE_ATOMIC_LOAD(uintptr_t,                     uintptr)
PY_PROTOTYPE_ATOMIC_LOAD(Py_ssize_t,                    ssize)

#undef PY_PROTOTYPE_ATOMIC_LOAD

static inline void *
_Py_atomic_load_ptr(const void *obj);


// --- _Py_atomic_load_relaxed -----------------------------------------------
// Loads `*obj` (relaxed consistency, i.e., no ordering)

#define PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(TYPENAME, SUFFIX)          \
    static inline TYPENAME                                          \
    _Py_atomic_load_ ## SUFFIX ## _relaxed(const TYPENAME *obj);

PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(int,                   int)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(int8_t,                int8)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(int16_t,               int16)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(int32_t,               int32)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(int64_t,               int64)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(intptr_t,              intptr)

PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(unsigned int,          uint)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(unsigned long long,    ullong)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(uint8_t,               uint8)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(uint16_t,              uint16)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(uint32_t,              uint32)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(uint64_t,              uint64)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(uintptr_t,             uintptr)
PY_PROTOTYPE_ATOMIC_LOAD_RELAXED(Py_ssize_t,            ssize)

#undef PY_PROTOTYPE_ATOMIC_LOAD_RELAXED

static inline void *
_Py_atomic_load_ptr_relaxed(const void *obj);


// --- _Py_atomic_store ------------------------------------------------------
// Atomically performs `*obj = value` (sequential consistency)

#define PY_PROTOTYPE_ATOMIC_STORE(TYPENAME, SUFFIX)                 \
    static inline void                                              \
    _Py_atomic_store_ ## SUFFIX (TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_STORE(int,                          int)
PY_PROTOTYPE_ATOMIC_STORE(int8_t,                       int8)
PY_PROTOTYPE_ATOMIC_STORE(int16_t,                      int16)
PY_PROTOTYPE_ATOMIC_STORE(int32_t,                      int32)
PY_PROTOTYPE_ATOMIC_STORE(int64_t,                      int64)
PY_PROTOTYPE_ATOMIC_STORE(intptr_t,                     intptr)

PY_PROTOTYPE_ATOMIC_STORE(unsigned int,                 uint)
PY_PROTOTYPE_ATOMIC_STORE(uint8_t,                      uint8)
PY_PROTOTYPE_ATOMIC_STORE(uint16_t,                     uint16)
PY_PROTOTYPE_ATOMIC_STORE(uint32_t,                     uint32)
PY_PROTOTYPE_ATOMIC_STORE(uint64_t,                     uint64)
PY_PROTOTYPE_ATOMIC_STORE(uintptr_t,                    uintptr)
PY_PROTOTYPE_ATOMIC_STORE(Py_ssize_t,                   ssize)

#undef PY_PROTOTYPE_ATOMIC_STORE

static inline void
_Py_atomic_store_ptr(void *obj, void *value);


// --- _Py_atomic_store_relaxed ----------------------------------------------
// Stores `*obj = value` (relaxed consistency, i.e., no ordering)

#define PY_PROTOTYPE_ATOMIC_STORE_RELAXED(TYPENAME, SUFFIX)                 \
    static inline void                                                      \
    _Py_atomic_store_ ## SUFFIX ## _relaxed(TYPENAME *obj, TYPENAME value);

PY_PROTOTYPE_ATOMIC_STORE_RELAXED(int,                  int)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(int8_t,               int8)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(int16_t,              int16)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(int32_t,              int32)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(int64_t,              int64)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(intptr_t,             intptr)

PY_PROTOTYPE_ATOMIC_STORE_RELAXED(unsigned int,         uint)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(unsigned long long,   ullong)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(uint8_t,              uint8)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(uint16_t,             uint16)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(uint32_t,             uint32)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(uint64_t,             uint64)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(uintptr_t,            uintptr)
PY_PROTOTYPE_ATOMIC_STORE_RELAXED(Py_ssize_t,           ssize)

#undef PY_PROTOTYPE_ATOMIC_STORE_RELAXED

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void *value);


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

#define PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(TYPENAME, SUFFIX)   \
    static inline TYPENAME                                          \
    _Py_atomic_load_ ## SUFFIX ## _acquire(const TYPENAME *obj);    \
    static inline void                                              \
    _Py_atomic_store_ ## SUFFIX ## _release(TYPENAME *obj,          \
                                            TYPENAME value);

PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(int,            int)
PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(uint32_t,       uint32)
PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(uint64_t,       uint64)
PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(uintptr_t,      uintptr)
PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE(Py_ssize_t,     ssize)

#undef PY_PROTOTYPE_ATOMIC_ACQUIRE_AND_RELEASE

// Loads `*obj` (acquire operation)
static inline void *
_Py_atomic_load_ptr_acquire(const void *obj);

// Stores `*obj = value` (release operation)
static inline void
_Py_atomic_store_ptr_release(void *obj, void *value);


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
#  include "cpython/pyatomic_gcc.h"
#  undef Py_ATOMIC_GCC_H
#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)
#  define Py_ATOMIC_STD_H
#  include "cpython/pyatomic_std.h"
#  undef Py_ATOMIC_STD_H
#elif defined(_MSC_VER)
#  define Py_ATOMIC_MSC_H
#  include "cpython/pyatomic_msc.h"
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
