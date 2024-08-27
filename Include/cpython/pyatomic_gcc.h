// This is the implementation of Python atomic operations using GCC's built-in
// functions that match the C+11 memory model. This implementation is preferred
// for GCC compatible compilers, such as Clang. These functions are available
// in GCC 4.8+ without needing to compile with --std=c11 or --std=gnu11.

#ifndef Py_ATOMIC_GCC_H
#  error "this header file must not be included directly"
#endif


// --- _Py_atomic_add --------------------------------------------------------

#define PY_GENERATE_ATOMIC_ADD(TYPENAME, SUFFIX)                    \
    static inline TYPENAME                                          \
    _Py_atomic_add_ ## SUFFIX (TYPENAME *obj, TYPENAME value)       \
    { return __atomic_fetch_add(obj, value, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_ADD(int,                                 int)
PY_GENERATE_ATOMIC_ADD(int8_t,                              int8)
PY_GENERATE_ATOMIC_ADD(int16_t,                             int16)
PY_GENERATE_ATOMIC_ADD(int32_t,                             int32)
PY_GENERATE_ATOMIC_ADD(int64_t,                             int64)
PY_GENERATE_ATOMIC_ADD(intptr_t,                            intptr)

PY_GENERATE_ATOMIC_ADD(unsigned int,                        uint)
PY_GENERATE_ATOMIC_ADD(uint8_t,                             uint8)
PY_GENERATE_ATOMIC_ADD(uint16_t,                            uint16)
PY_GENERATE_ATOMIC_ADD(uint32_t,                            uint32)
PY_GENERATE_ATOMIC_ADD(uint64_t,                            uint64)
PY_GENERATE_ATOMIC_ADD(uintptr_t,                           uintptr)
PY_GENERATE_ATOMIC_ADD(Py_ssize_t,                          ssize)

#undef PY_GENERATE_ATOMIC_ADD


// --- _Py_atomic_compare_exchange -------------------------------------------

#define PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(TYPENAME, SUFFIX)       \
    static inline int                                               \
    _Py_atomic_compare_exchange_ ## SUFFIX (TYPENAME *obj,          \
                                            TYPENAME *expected,     \
                                            TYPENAME desired)       \
    { return __atomic_compare_exchange_n(obj, expected, desired, 0, \
                                         __ATOMIC_SEQ_CST,          \
                                         __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(int,                    int)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(int8_t,                 int8)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(int16_t,                int16)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(int32_t,                int32)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(int64_t,                int64)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(intptr_t,               intptr)

PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(unsigned int,           uint)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(uint8_t,                uint8)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(uint16_t,               uint16)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(uint32_t,               uint32)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(uint64_t,               uint64)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(uintptr_t,              uintptr)
PY_GENERATE_ATOMIC_COMPARE_EXCHANGE(Py_ssize_t,             ssize)

#undef PY_GENERATE_ATOMIC_COMPARE_EXCHANGE

static inline int
_Py_atomic_compare_exchange_ptr(void *obj, void *expected, void *desired)
{ return __atomic_compare_exchange_n((void **)obj, (void **)expected, desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_exchange ---------------------------------------------------

#define PY_GENERATE_ATOMIC_EXCHANGE(TYPENAME, SUFFIX)               \
    static inline TYPENAME                                          \
    _Py_atomic_exchange_ ## SUFFIX (TYPENAME *obj, TYPENAME value)  \
    { return __atomic_exchange_n(obj, value, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_EXCHANGE(int,                            int)
PY_GENERATE_ATOMIC_EXCHANGE(int8_t,                         int8)
PY_GENERATE_ATOMIC_EXCHANGE(int16_t,                        int16)
PY_GENERATE_ATOMIC_EXCHANGE(int32_t,                        int32)
PY_GENERATE_ATOMIC_EXCHANGE(int64_t,                        int64)
PY_GENERATE_ATOMIC_EXCHANGE(intptr_t,                       intptr)

PY_GENERATE_ATOMIC_EXCHANGE(unsigned int,                   uint)
PY_GENERATE_ATOMIC_EXCHANGE(uint8_t,                        uint8)
PY_GENERATE_ATOMIC_EXCHANGE(uint16_t,                       uint16)
PY_GENERATE_ATOMIC_EXCHANGE(uint32_t,                       uint32)
PY_GENERATE_ATOMIC_EXCHANGE(uint64_t,                       uint64)
PY_GENERATE_ATOMIC_EXCHANGE(uintptr_t,                      uintptr)
PY_GENERATE_ATOMIC_EXCHANGE(Py_ssize_t,                     ssize)

#undef PY_GENERATE_ATOMIC_EXCHANGE

static inline void *
_Py_atomic_exchange_ptr(void *obj, void *value)
{ return __atomic_exchange_n((void **)obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_and --------------------------------------------------------

#define PY_GENERATE_ATOMIC_AND(TYPENAME, SUFFIX)                    \
    static inline TYPENAME                                          \
    _Py_atomic_and_ ## SUFFIX (TYPENAME *obj, TYPENAME value)       \
    { return __atomic_fetch_and(obj, value, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_AND(uint8_t,                             uint8)
PY_GENERATE_ATOMIC_AND(uint16_t,                            uint16)
PY_GENERATE_ATOMIC_AND(uint32_t,                            uint32)
PY_GENERATE_ATOMIC_AND(uint64_t,                            uint64)
PY_GENERATE_ATOMIC_AND(uintptr_t,                           uintptr)

#undef PY_GENERATE_ATOMIC_AND


// --- _Py_atomic_or ---------------------------------------------------------

#define PY_GENERATE_ATOMIC_OR(TYPENAME, SUFFIX)                     \
    static inline TYPENAME                                          \
    _Py_atomic_or_ ## SUFFIX (TYPENAME *obj, TYPENAME value)        \
    { return __atomic_fetch_or(obj, value, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_OR(uint8_t,                              uint8)
PY_GENERATE_ATOMIC_OR(uint16_t,                             uint16)
PY_GENERATE_ATOMIC_OR(uint32_t,                             uint32)
PY_GENERATE_ATOMIC_OR(uint64_t,                             uint64)
PY_GENERATE_ATOMIC_OR(uintptr_t,                            uintptr)

#undef PY_GENERATE_ATOMIC_OR


// --- _Py_atomic_load -------------------------------------------------------

#define PY_GENERATE_ATOMIC_LOAD(TYPENAME, SUFFIX)       \
    static inline TYPENAME                              \
    _Py_atomic_load_ ## SUFFIX (const TYPENAME *obj)    \
    { return __atomic_load_n(obj, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_LOAD(int,                                int)
PY_GENERATE_ATOMIC_LOAD(int8_t,                             int8)
PY_GENERATE_ATOMIC_LOAD(int16_t,                            int16)
PY_GENERATE_ATOMIC_LOAD(int32_t,                            int32)
PY_GENERATE_ATOMIC_LOAD(int64_t,                            int64)
PY_GENERATE_ATOMIC_LOAD(intptr_t,                           intptr)

PY_GENERATE_ATOMIC_LOAD(unsigned int,                       uint)
PY_GENERATE_ATOMIC_LOAD(uint8_t,                            uint8)
PY_GENERATE_ATOMIC_LOAD(uint16_t,                           uint16)
PY_GENERATE_ATOMIC_LOAD(uint32_t,                           uint32)
PY_GENERATE_ATOMIC_LOAD(uint64_t,                           uint64)
PY_GENERATE_ATOMIC_LOAD(uintptr_t,                          uintptr)
PY_GENERATE_ATOMIC_LOAD(Py_ssize_t,                         ssize)

#undef PY_GENERATE_ATOMIC_LOAD

static inline void *
_Py_atomic_load_ptr(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_load_relaxed -----------------------------------------------

#define PY_GENERATE_ATOMIC_LOAD_RELAXED(TYPENAME, SUFFIX)       \
    static inline TYPENAME                                      \
    _Py_atomic_load_ ## SUFFIX ## _relaxed(const TYPENAME *obj) \
    { return __atomic_load_n(obj, __ATOMIC_RELAXED); }

PY_GENERATE_ATOMIC_LOAD_RELAXED(int,                        int)
PY_GENERATE_ATOMIC_LOAD_RELAXED(int8_t,                     int8)
PY_GENERATE_ATOMIC_LOAD_RELAXED(int16_t,                    int16)
PY_GENERATE_ATOMIC_LOAD_RELAXED(int32_t,                    int32)
PY_GENERATE_ATOMIC_LOAD_RELAXED(int64_t,                    int64)
PY_GENERATE_ATOMIC_LOAD_RELAXED(intptr_t,                   intptr)

PY_GENERATE_ATOMIC_LOAD_RELAXED(unsigned int,               uint)
PY_GENERATE_ATOMIC_LOAD_RELAXED(unsigned long long,         ullong)
PY_GENERATE_ATOMIC_LOAD_RELAXED(uint8_t,                    uint8)
PY_GENERATE_ATOMIC_LOAD_RELAXED(uint16_t,                   uint16)
PY_GENERATE_ATOMIC_LOAD_RELAXED(uint32_t,                   uint32)
PY_GENERATE_ATOMIC_LOAD_RELAXED(uint64_t,                   uint64)
PY_GENERATE_ATOMIC_LOAD_RELAXED(uintptr_t,                  uintptr)
PY_GENERATE_ATOMIC_LOAD_RELAXED(Py_ssize_t,                 ssize)

#undef PY_GENERATE_ATOMIC_LOAD_RELAXED

static inline void *
_Py_atomic_load_ptr_relaxed(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_RELAXED); }


// --- _Py_atomic_store ------------------------------------------------------

#define PY_GENERATE_ATOMIC_STORE(TYPENAME, SUFFIX)              \
    static inline void                                          \
    _Py_atomic_store_ ## SUFFIX (TYPENAME *obj, TYPENAME value) \
    { return __atomic_store_n(obj, value, __ATOMIC_SEQ_CST); }

PY_GENERATE_ATOMIC_STORE(int,                               int)
PY_GENERATE_ATOMIC_STORE(int8_t,                            int8)
PY_GENERATE_ATOMIC_STORE(int16_t,                           int16)
PY_GENERATE_ATOMIC_STORE(int32_t,                           int32)
PY_GENERATE_ATOMIC_STORE(int64_t,                           int64)
PY_GENERATE_ATOMIC_STORE(intptr_t,                          intptr)

PY_GENERATE_ATOMIC_STORE(unsigned int,                      uint)
PY_GENERATE_ATOMIC_STORE(uint8_t,                           uint8)
PY_GENERATE_ATOMIC_STORE(uint16_t,                          uint16)
PY_GENERATE_ATOMIC_STORE(uint32_t,                          uint32)
PY_GENERATE_ATOMIC_STORE(uint64_t,                          uint64)
PY_GENERATE_ATOMIC_STORE(uintptr_t,                         uintptr)
PY_GENERATE_ATOMIC_STORE(Py_ssize_t,                        ssize)

#undef PY_GENERATE_ATOMIC_STORE

static inline void
_Py_atomic_store_ptr(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_SEQ_CST); }


// --- _Py_atomic_store_relaxed ----------------------------------------------

#define PY_GENERATE_ATOMIC_STORE_RELAXED(TYPENAME, SUFFIX)                  \
    static inline void                                                      \
    _Py_atomic_store_ ## SUFFIX ## _relaxed(TYPENAME *obj, TYPENAME value)  \
    { return __atomic_store_n(obj, value, __ATOMIC_RELAXED); }

PY_GENERATE_ATOMIC_STORE_RELAXED(int,                       int)
PY_GENERATE_ATOMIC_STORE_RELAXED(int8_t,                    int8)
PY_GENERATE_ATOMIC_STORE_RELAXED(int16_t,                   int16)
PY_GENERATE_ATOMIC_STORE_RELAXED(int32_t,                   int32)
PY_GENERATE_ATOMIC_STORE_RELAXED(int64_t,                   int64)
PY_GENERATE_ATOMIC_STORE_RELAXED(intptr_t,                  intptr)

PY_GENERATE_ATOMIC_STORE_RELAXED(unsigned int,              uint)
PY_GENERATE_ATOMIC_STORE_RELAXED(unsigned long long,        ullong)
PY_GENERATE_ATOMIC_STORE_RELAXED(uint8_t,                   uint8)
PY_GENERATE_ATOMIC_STORE_RELAXED(uint16_t,                  uint16)
PY_GENERATE_ATOMIC_STORE_RELAXED(uint32_t,                  uint32)
PY_GENERATE_ATOMIC_STORE_RELAXED(uint64_t,                  uint64)
PY_GENERATE_ATOMIC_STORE_RELAXED(uintptr_t,                 uintptr)
PY_GENERATE_ATOMIC_STORE_RELAXED(Py_ssize_t,                ssize)

#undef PY_GENERATE_ATOMIC_STORE_RELAXED

static inline void
_Py_atomic_store_ptr_relaxed(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_RELAXED); }


// --- _Py_atomic_load_ptr_acquire / _Py_atomic_store_ptr_release ------------

#define PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(TYPENAME, SUFFIX)            \
    static inline TYPENAME                                                  \
    _Py_atomic_load_ ## SUFFIX ## _acquire(const TYPENAME* obj)             \
    { return __atomic_load_n(obj, __ATOMIC_ACQUIRE); }                      \
    static inline void                                                      \
    _Py_atomic_store_ ## SUFFIX ## _release(TYPENAME *obj, TYPENAME value)  \
    { return __atomic_store_n(obj, value, __ATOMIC_RELEASE); }

PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(int,                 int)
PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(uint32_t,            uint32)
PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(uint64_t,            uint64)
PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(uintptr_t,           uintptr)
PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE(Py_ssize_t,          ssize)

#undef PY_GENERATE_ATOMIC_ACQUIRE_AND_RELEASE

static inline void *
_Py_atomic_load_ptr_acquire(const void *obj)
{ return (void *)__atomic_load_n((void * const *)obj, __ATOMIC_ACQUIRE); }

static inline void
_Py_atomic_store_ptr_release(void *obj, void *value)
{ __atomic_store_n((void **)obj, value, __ATOMIC_RELEASE); }


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
