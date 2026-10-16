/* ----------------------------------------------------------------------------
Copyright (c) 2018-2026, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#pragma once
#ifndef MI_INTERNAL_LIBC_H
#define MI_INTERNAL_LIBC_H

#include "../types.h"

#include <string.h>  // memset, memcpy, memcmp
#include <stdarg.h>  // va_list, va_start, va_end, va_copy

int           _mi_vsnprintf(char* buf, size_t bufsize, const char* fmt, va_list args);
int           _mi_snprintf(char* buf, size_t buflen, const char* fmt, ...);
char          _mi_toupper(char c);
int           _mi_strnicmp(const char* s, const char* t, size_t n);
bool          _mi_strlcpy(char* dest, const char* src, size_t dest_size); // returns true if the entire src was copied
bool          _mi_strlcat(char* dest, const char* src, size_t dest_size); // returns true if the entire src was appended
size_t        _mi_strlen(const char* s);
size_t        _mi_strnlen(const char* s, size_t max_len);
char*         _mi_strnstr(char* s, size_t max_len, const char* pat);
const char*   _mi_strchr(const char* s, char c);
const char*   _mi_strrchr(const char* s, char c);
bool          _mi_streq(const char* s, const char* t);
int           _mi_getenv(const char* name, char* result, size_t result_size);
void          _mi_detect_cpu_features(void);


// minimum
static inline size_t mi_min(size_t x,  size_t y) { return (x <= y ? x : y); }

// maximum
static inline size_t mi_max(size_t x,  size_t y) { return (x >= y ? x : y); }

// Is `x` a power of two? (0 is considered a power of two)
static inline bool mi_is_power_of_two(uintptr_t x) {
  return ((x & (x - 1)) == 0);
}

// valid alignment values are as posix memalign: <https://en.cppreference.com/c/memory/aligned_alloc#Notes>
static inline bool mi_alignment_is_valid(size_t alignment) {
  return ((alignment!=0) && mi_is_power_of_two(alignment));
}

// Is a pointer aligned?
static inline bool mi_is_aligned(const void* p, size_t alignment) {
  return (alignment==0 || ((uintptr_t)p % alignment) == 0);
}

// Align upwards
static inline uintptr_t mi_align_up(uintptr_t sz, size_t alignment) {
  mi_assert_internal(alignment != 0);
  const uintptr_t mask = alignment - 1;
  if ((alignment & mask) == 0) {  // power of two?
    return ((sz + mask) & ~mask);
  }
  else {
    return (((sz + mask)/alignment)*alignment);
  }
}

// Align a pointer upwards
static inline void* mi_align_up_ptr(const void* p, size_t alignment) {
  return (void*)mi_align_up((uintptr_t)p, alignment);
}

// Align down
static inline uintptr_t mi_align_down(uintptr_t sz, size_t alignment) {
  mi_assert_internal(alignment != 0);
  const uintptr_t mask = alignment - 1;
  if ((alignment & mask) == 0) {  // power of two?
    return (sz & ~mask);
  }
  else {
    return ((sz/alignment)*alignment);
  }
}

// Align a pointer downwards
static inline void* mi_align_down_ptr(const void* p, size_t alignment) {
  return (void*)mi_align_down((uintptr_t)p, alignment);
}

// Divide upwards: `s <= mi_divide_up(s,d)*d < s+d`.
static inline uintptr_t mi_divide_up(uintptr_t size, size_t divider) {
  mi_assert_internal(divider != 0);
  return (divider == 0 ? size : ((size + divider - 1) / divider));
}


// clamp an integer
static inline size_t mi_clamp(size_t sz, size_t min, size_t max) {
  if (sz < min) return min;
  else if (sz > max) return max;
  else return sz;
}

// Overflow detecting multiply
#if __has_builtin(__builtin_umul_overflow) || (defined(__GNUC__) && (__GNUC__ >= 5))
#include <limits.h>      // UINT_MAX, ULONG_MAX
#if defined(_CLOCK_T)    // for Illumos
#undef _CLOCK_T
#endif
static inline bool mi_mul_overflow(size_t count, size_t size, size_t* total) {
  #if (SIZE_MAX == UINT_MAX)
    return __builtin_umul_overflow(count, size, (unsigned int *)total);
  #elif (SIZE_MAX == ULONG_MAX)
    return __builtin_umull_overflow(count, size, (unsigned long *)total);
  #else
    return __builtin_umulll_overflow(count, size, (unsigned long long *)total);
  #endif
}
#else /* __builtin_umul_overflow is unavailable */
static inline bool mi_mul_overflow(size_t count, size_t size, size_t* total) {
  *total = count*size;
  if mi_likely(((size|count)>>(4*MI_SIZE_SIZE))==0) {  // did size and count fit both in the lower half bits of a size_t?
    return false;
  }
  else {
    return (size!=0 && (SIZE_MAX / size) < count);
  }
}
#endif

// initialize a local variable to zero; use memset as compilers optimize constant sized memset's
#define _mi_memzero_var(x)  memset(&x,0,sizeof(x))

// Is memory zero initialized?
static inline bool mi_mem_is_zero(const void* p, size_t size) {
  for (size_t i = 0; i < size; i++) {
    if (((uint8_t*)p)[i] != 0) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------------
// Provide our own `mi_memcpy/set` for potential performance optimizations.
// ---------------------------------------------------------------------------------

static inline int mi_memcmp(const void* dst, const void* src, size_t n) {
  return memcmp(dst, src, n);
}

static inline void* mi_memcpy(void* dst, const void* src, size_t n) {
  return memcpy(dst, src, n);
}

static inline void* mi_memset(void* dst, int val, size_t n) {
  return memset(dst, val, n);
}

static inline void* mi_memset_backward(void* dst, int val, size_t n) {
  memset((uint8_t*)dst - n, val, n);
  return dst;
}

static inline void* mi_memzero(void* dst, size_t n) {
  return mi_memset(dst, 0, n);
}

static inline void* mi_memzero_backward(void* dst, size_t n) {
  return mi_memset_backward(dst, 0, n);
}

static inline void* mi_memcpy_aligned(void* dst, const void* src, size_t n) {
  // on gcc/clang we can provide a hint that the pointers are word aligned.
  mi_assert_internal(mi_is_aligned(dst,MI_SIZE_SIZE) && mi_is_aligned(src,MI_SIZE_SIZE));
  void* adst = mi_assume_aligned(dst, MI_SIZE_SIZE);
  const void* asrc = mi_assume_aligned(src, MI_SIZE_SIZE);
  return mi_memcpy(adst, asrc, n);
}

static inline void* mi_memset_aligned(void* dst, int val, size_t n) {
  mi_assert_internal(mi_is_aligned(dst,MI_SIZE_SIZE));
  void* adst = mi_assume_aligned(dst, MI_SIZE_SIZE);
  return mi_memset(adst, val, n);
}

static inline void* mi_memzero_aligned(void* dst, size_t n) {
  return mi_memset_aligned(dst, 0, n);
}


// Zero a block: blocks are always aligned with a positive bsize in machine-word bytes.
static mi_decl_forceinline void* mi_memzero_block(mi_block_t* dst, size_t bsize) {
  mi_assert_internal(bsize%MI_SIZE_SIZE == 0);
  mi_assert_internal(bsize > 0);
  mi_assert_internal(mi_is_aligned(dst,MI_SIZE_SIZE));
  mi_assert_internal(bsize < MI_MAX_ALIGN_SIZE || mi_is_aligned(dst,MI_MAX_ALIGN_SIZE));

  // fast memzero for small sizes based on overlapping writes (and assuming non-zero size_t-multiple size, and size_t aligned)
  // assumes constant memset(p,0,N) gets optimized to fast simd stores by the compiler
  // note: disabled on riscv for now as a constant memset is not always replaced correctly by current compilers.
  // (compile with -DMI_USE_MEMZERO16X=0 to disable this)
  #if (!defined(MI_USE_MEMZERO16X) && !MI_ARCH_RISCV) || (MI_USE_MEMZERO16X != 0) // 16x MI_SIZE_SIZE (128 bytes on 64-bit)
    if mi_unlikely(bsize < 2*MI_SIZE_SIZE) { // bsize < 16 (8)
      *((size_t*)dst) = 0;
      return dst;
    }
    mi_assert_internal(mi_is_aligned(dst,MI_MAX_ALIGN_SIZE));
    uint8_t* const start = (uint8_t*)mi_assume_aligned(dst, MI_MAX_ALIGN_SIZE);
    uint8_t* const end   = start + bsize;    // note: if bsize is always a multiple of 16 then end is always aligned as well (but due to padding this does not hold)
    if mi_likely(bsize < 8*MI_SIZE_SIZE) {   // bsize < 64 (32)
      const size_t ofs = (bsize>>1)&(2*MI_SIZE_SIZE); mi_assert_internal(bsize < 4*MI_SIZE_SIZE ? ofs==0 : ofs==2*MI_SIZE_SIZE);  // ofs == 16 (8)
      mi_memzero(start,            2*MI_SIZE_SIZE);
      mi_memzero(start+ofs,        2*MI_SIZE_SIZE);
      mi_memzero_backward(end-ofs, 2*MI_SIZE_SIZE);
      mi_memzero_backward(end,     2*MI_SIZE_SIZE);
      return dst;
    }
    if mi_likely(bsize <= 16*MI_SIZE_SIZE) {  // bsize < 128 (64)
      mi_memzero(start,        8*MI_SIZE_SIZE);
      mi_memzero_backward(end, 8*MI_SIZE_SIZE);
      return dst;
    }
  #endif
  // fallback to regular memset for larger sizes
  void* const wdst = mi_assume_aligned(dst,MI_SIZE_SIZE);
  return mi_memzero_aligned(wdst, bsize);
}

#endif
