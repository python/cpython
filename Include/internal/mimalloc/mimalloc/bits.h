/* ----------------------------------------------------------------------------
Copyright (c) 2019-2026 Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
  Bit operation, and platform dependent definition (MI_INTPTR_SIZE etc)
---------------------------------------------------------------------------- */

#pragma once
#ifndef MI_BITS_H
#define MI_BITS_H

#include <stddef.h>   // size_t
#include <stdint.h>   // int64_t etc
#include <stdbool.h>  // bool
#include <limits.h>   // LONG_MAX

#if defined(__cplusplus)
#define mi_decl_externc         extern "C"
#define mi_init_struct_zero     { }
#else
#define mi_decl_externc
#define mi_init_struct_zero     { 0 }
#endif

// ------------------------------------------------------
// Size of a pointer.
// We assume that `sizeof(void*)==sizeof(intptr_t)`
// and it holds for all platforms we know of.
//
// However, the C standard only requires that:
//  p == (void*)((intptr_t)p))
// but we also need:
//  i == (intptr_t)((void*)i)
// or otherwise one might define an intptr_t type that is larger than a pointer...
//
// On CHERI, pointers are 128-bit capabilities but `INTPTR_MAX` is `INT64_MAX`
// (as only the address is an integer value), so we use `__SIZEOF_POINTER__` if available.
// ------------------------------------------------------

#if (INTPTR_MAX > INT64_MAX) || (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 16)
# define MI_INTPTR_SHIFT (4)  // 128-bit (as on CHERI for example)
#elif INTPTR_MAX == INT64_MAX
# define MI_INTPTR_SHIFT (3)
#elif INTPTR_MAX == INT32_MAX
# define MI_INTPTR_SHIFT (2)
#else
#error platform pointers must be 32, 64, or 128 bits
#endif

#if (INTPTR_MAX) > LONG_MAX
# define MI_PU(x)  x##ULL
#else
# define MI_PU(x)  x##UL
#endif

#if SIZE_MAX == UINT64_MAX
# define MI_SIZE_SHIFT (3)
typedef int64_t  mi_ssize_t;
#define MI_SSIZE_MAX  INT64_MAX
#elif SIZE_MAX == UINT32_MAX
# define MI_SIZE_SHIFT (2)
typedef int32_t  mi_ssize_t;
#define MI_SSIZE_MAX  INT32_MAX
#else
#error platform objects must be 32 or 64 bits in size
#endif

#if (SIZE_MAX/2) > LONG_MAX
# define MI_ZU(x)  x##ULL
#else
# define MI_ZU(x)  x##UL
#endif

#define MI_INTPTR_SIZE  (1<<MI_INTPTR_SHIFT)
#define MI_INTPTR_BITS  (MI_INTPTR_SIZE*8)

#define MI_SIZE_SIZE  (1<<MI_SIZE_SHIFT)
#define MI_SIZE_BITS  (MI_SIZE_SIZE*8)

#define MI_KiB     (MI_ZU(1024))
#define MI_MiB     (MI_KiB*MI_KiB)
#define MI_GiB     (MI_MiB*MI_KiB)

/* --------------------------------------------------------------------------------
  Architecture
-------------------------------------------------------------------------------- */

#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)  // consider arm64ec as arm64
#define MI_ARCH_ARM64     1
#elif defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_X64) || defined(_M_AMD64)
#define MI_ARCH_X64       1
#elif defined(__i386__) || defined(__i386) || defined(_M_IX86) || defined(_X86_) || defined(__X86__)
#define MI_ARCH_X86       1
#elif defined(__arm__) || defined(_ARM) || defined(_M_ARM)  || defined(_M_ARMT) || defined(__arm)
#define MI_ARCH_ARM32     1
#elif defined(__riscv) || defined(_M_RISCV)
#define MI_ARCH_RISCV     1
#if (LONG_MAX == INT32_MAX)
#define MI_ARCH_RISCV32   1
#else
#define MI_ARCH_RISCV64   1
#endif
#endif

#if defined(__CHERI_PURE_CAPABILITY__)
#define MI_CHERI          1
#endif

#if MI_ARCH_X64 && defined(__AVX2__)
#include <immintrin.h>
#elif MI_ARCH_ARM64 && MI_OPT_SIMD
#include <arm_neon.h>
#endif
#if defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
#include <intrin.h>
#endif

#if MI_ARCH_X64 && defined(__AVX2__) && !defined(__BMI2__) // avx2 implies bmi2
#define __BMI2__  1
#endif
#if MI_ARCH_X64 && (defined(__AVX2__) || defined(__BMI2__) || defined(__BMI__)) && !defined(__BMI1__) // bmi2 implies bmi1
#define __BMI1__  1
#endif
#if MI_ARCH_X64 && defined(__AVX2__) && !defined(__LZCNT__) // avx2 implies lzcnt
#define __LZCNT__  1
#endif

// Define big endian if needed
// #define MI_BIG_ENDIAN  1

// maximum virtual address bits in a user-space pointer
#if MI_DEFAULT_VIRTUAL_ADDRESS_BITS > 0
#define MI_MAX_VABITS     MI_DEFAULT_VIRTUAL_ADDRESS_BITS
#elif MI_ARCH_X64 && (MI_INTPTR_SIZE > 4)   // not on x32
#define MI_MAX_VABITS     (47)
#elif MI_INTPTR_SIZE > 4
#define MI_MAX_VABITS     (48)
#else
#define MI_MAX_VABITS     (32)
#endif

// the MI_MIN_VABITS determine how many bits of the address space are always committed in the page_map
#if MI_MAX_VABITS <= 32
#define MI_MIN_VABITS     (32)
#elif MI_MAX_VABITS <= 42
#define MI_MIN_VABITS     MI_MAX_VABITS
#else
#define MI_MIN_VABITS     (43)    /* 8 TiB */
#endif

// Always use the page map in `mi_free` on CHERI as we cannot derive the page meta-data from a (bounded) user pointer,
// and on WebAssembly as it cannot do large aligned allocations well (needed for aligned page meta-data).
#if (MI_CHERI || defined(__wasi__) || defined(__EMSCRIPTEN__)) && !defined(MI_FREE_USE_PAGEMAP)
#define MI_FREE_USE_PAGEMAP  1
#endif

// Probable OS page size (as statically known; the actual page size is determined at runtime).
// Can be set explicitly, e.g. `-DMI_OS_PROBABLE_PAGE_SIZE=65536` for arm64 Linux kernels with 64 KiB pages.
#if !defined(MI_OS_PROBABLE_PAGE_SIZE)
#if defined(__APPLE__)                     // check first as `PAGE_SIZE` may be the runtime `vm_page_size` here
#if MI_ARCH_ARM64
#define MI_OS_PROBABLE_PAGE_SIZE  (16*MI_KiB) // macOS, iOS, etc. on Apple silicon
#else
#define MI_OS_PROBABLE_PAGE_SIZE  (4*MI_KiB)
#endif
#elif defined(__wasm__) || defined(__EMSCRIPTEN__) || defined(__wasi__)
#define MI_OS_PROBABLE_PAGE_SIZE  (64*MI_KiB) // fixed WebAssembly page size
#elif defined(PAGE_SIZE)                   // only defined if constant (FreeBSD, musl on x86, bionic without flexible page sizes, ...)
#define MI_OS_PROBABLE_PAGE_SIZE  PAGE_SIZE
#elif defined(__linux__) && (defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__))
#define MI_OS_PROBABLE_PAGE_SIZE  (64*MI_KiB) // default for ppc64(le) distributions
#elif defined(__linux__) && defined(__loongarch__)
#define MI_OS_PROBABLE_PAGE_SIZE  (16*MI_KiB) // default for LoongArch kernels
#elif defined(__ANDROID__) && defined(__BIONIC_NO_PAGE_SIZE_MACRO)
#define MI_OS_PROBABLE_PAGE_SIZE  (16*MI_KiB) // built with flexible page size support (NDK r27+), so it can run with 16 KiB pages
#else
#define MI_OS_PROBABLE_PAGE_SIZE  (4*MI_KiB)
#endif
#endif

// Does the OS have guard pages?
#if !defined(MI_OS_HAS_GUARD_PAGES)
#if defined(__wasm__) || defined(__EMSCRIPTEN__) || defined(__wasi__)
#define MI_OS_HAS_GUARD_PAGES  0
#elif defined(__uClinux__)                 // no MMU
#define MI_OS_HAS_GUARD_PAGES  0
#else
#define MI_OS_HAS_GUARD_PAGES  1
#endif
#endif

/* --------------------------------------------------------------------------------
  Builtin's
-------------------------------------------------------------------------------- */

#ifndef __has_builtin
#define __has_builtin(x)  0
#endif

#define mi_builtin(name)        __builtin_##name
#define mi_has_builtin(name)    __has_builtin(__builtin_##name)

#if (LONG_MAX == INT32_MAX)
#define mi_builtin32(name)       mi_builtin(name##l)
#define mi_has_builtin32(name)   mi_has_builtin(name##l)
#else
#define mi_builtin32(name)       mi_builtin(name)
#define mi_has_builtin32(name)   mi_has_builtin(name)
#endif
#if (LONG_MAX == INT64_MAX)
#define mi_builtin64(name)       mi_builtin(name##l)
#define mi_has_builtin64(name)   mi_has_builtin(name##l)
#else
#define mi_builtin64(name)       mi_builtin(name##ll)
#define mi_has_builtin64(name)   mi_has_builtin(name##ll)
#endif

#if (MI_SIZE_BITS == 32)
#define mi_builtinz(name)        mi_builtin32(name)
#define mi_has_builtinz(name)    mi_has_builtin32(name)
#define mi_msc_builtinz(name)    name
#elif (MI_SIZE_BITS == 64)
#define mi_builtinz(name)        mi_builtin64(name)
#define mi_has_builtinz(name)    mi_has_builtin64(name)
#define mi_msc_builtinz(name)    name##64
#endif

/* --------------------------------------------------------------------------------
  Popcount and count trailing/leading zero's
-------------------------------------------------------------------------------- */

size_t _mi_popcount_generic(size_t x);
extern bool _mi_cpu_has_popcnt;

static inline size_t mi_popcount(size_t x) {
  #if mi_has_builtinz(popcount)
    return mi_builtinz(popcount)(x);
  #elif defined(_MSC_VER) && (MI_ARCH_ARM64 || MI_ARCH_ARM32)
    return mi_msc_builtinz(_CountOneBits)(x);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86)
    if (_mi_cpu_has_popcnt) { return mi_msc_builtinz(__popcnt)(x); }
                       else { return _mi_popcount_generic(x); }      // see issue #1291
  #elif MI_ARCH_X64 && defined(__BMI1__)
    #if (MI_SIZE_BITS == 64)
    return (size_t)_mm_popcnt_u64(x);
    #else
    return (size_t)_mm_popcnt_u32(x);  // x32
    #endif
  #else
    #define MI_HAS_FAST_POPCOUNT  0
    return (x<=1 ? x : _mi_popcount_generic(x));
  #endif
}

#ifndef MI_HAS_FAST_POPCOUNT
#define MI_HAS_FAST_POPCOUNT 1
#endif

size_t _mi_clz_generic(size_t x);
size_t _mi_ctz_generic(size_t x);

static inline size_t mi_ctz(size_t x) {
  #if defined(__GNUC__) && MI_ARCH_X64 && defined(__BMI1__) // on x64 tzcnt is defined for 0
    size_t r;
    __asm ("tzcnt\t%1, %0" : "=r"(r) : "r"(x) : "cc");
    return r;
  #elif defined(_MSC_VER) && MI_ARCH_X64 && defined(__BMI1__)
    return _tzcnt_u64(x);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
    unsigned long idx;
    return (mi_msc_builtinz(_BitScanForward)(&idx, x) ? (size_t)idx : MI_SIZE_BITS);
  #elif mi_has_builtinz(ctz)
    return (x!=0 ? (size_t)mi_builtinz(ctz)(x) : MI_SIZE_BITS);
  #elif defined(__GNUC__) && (MI_ARCH_X64 || MI_ARCH_X86)
    size_t r = MI_SIZE_BITS;  // bsf leaves destination unmodified if the argument is 0 (see <https://github.com/llvm/llvm-project/pull/102885>)
    __asm ("bsf\t%1, %0" : "+r"(r) : "r"(x) : "cc");
    return r;
  #elif MI_HAS_FAST_POPCOUNT
    return (x!=0 ? (mi_popcount(x^(x-1))-1) : MI_SIZE_BITS);
  #else
    #define MI_HAS_FAST_BITSCAN  0
    return (x!=0 ? _mi_ctz_generic(x) : MI_SIZE_BITS);
  #endif
}

static inline size_t mi_clz(size_t x) {
  #if defined(__GNUC__) && MI_ARCH_X64 && defined(__LZCNT__) // on x64 lzcnt is defined for 0
    size_t r;
    __asm ("lzcnt\t%1, %0" : "=r"(r) : "r"(x) : "cc");
    return r;
  #elif defined(_MSC_VER) && MI_ARCH_X64 && defined(__LZCNT__)
    return _lzcnt_u64(x);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
    unsigned long idx;
    return (mi_msc_builtinz(_BitScanReverse)(&idx, x) ? MI_SIZE_BITS - 1 - (size_t)idx : MI_SIZE_BITS);
  #elif mi_has_builtinz(clz)
    return (x!=0 ? (size_t)mi_builtinz(clz)(x) : MI_SIZE_BITS);
  #elif defined(__GNUC__) && (MI_ARCH_X64 || MI_ARCH_X86)
    if (x==0) return MI_SIZE_BITS;
    size_t r;
    __asm ("bsr\t%1, %0" : "=r"(r) : "r"(x) : "cc");
    return (MI_SIZE_BITS - 1 - r);
  #else
    #define MI_HAS_FAST_BITSCAN  0
    return (x!=0 ? _mi_clz_generic(x) : MI_SIZE_BITS);
  #endif
}

#ifndef MI_HAS_FAST_BITSCAN
#define MI_HAS_FAST_BITSCAN 1
#endif

/* --------------------------------------------------------------------------------
  find trailing/leading zero  (bit scan forward/reverse)
-------------------------------------------------------------------------------- */

// Bit scan forward: find the least significant bit that is set (i.e. count trailing zero's)
// return false if `x==0` (with `*idx` undefined) and true otherwise,
// with the `idx` is set to the bit index (`0 <= *idx < MI_BFIELD_BITS`).
static inline bool mi_bsf(size_t x, size_t* idx) {
  #if defined(__GNUC__) && MI_ARCH_X64 && defined(__BMI1__) && (!defined(__clang_major__) || __clang_major__ >= 9)
    // on x64 the carry flag is set on zero which gives better codegen
    bool is_zero;
    __asm ( "tzcnt\t%2, %1" : "=@ccc"(is_zero), "=r"(*idx) : "r"(x) : "cc" );
    return !is_zero;
  #elif 0 && defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
    unsigned long i;
    return (mi_msc_builtinz(_BitScanForward)(&i, x) ? (*idx = (size_t)i, true) : false);
  #else
    return (x!=0 ? (*idx = mi_ctz(x), true) : false);
  #endif
}

// Bit scan reverse: find the most significant bit that is set
// return false if `x==0` (with `*idx` undefined) and true otherwise,
// with the `idx` is set to the bit index (`0 <= *idx < MI_BFIELD_BITS`).
static inline bool mi_bsr(size_t x, size_t* idx) {
  #if 0 && defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
    unsigned long i;
    return (mi_msc_builtinz(_BitScanReverse)(&i, x) ? (*idx = (size_t)i, true) : false);
  #else
    return (x!=0 ? (*idx = MI_SIZE_BITS - 1 - mi_clz(x), true) : false);
  #endif
}


/* --------------------------------------------------------------------------------
  rotate
-------------------------------------------------------------------------------- */

static inline size_t mi_rotr(size_t x, size_t r) {
  #if (mi_has_builtin(rotateright64) && MI_SIZE_BITS==64)
    return mi_builtin(rotateright64)(x,r);
  #elif (mi_has_builtin(rotateright32) && MI_SIZE_BITS==32)
    return mi_builtin(rotateright32)(x,r);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_ARM64)
    return _rotr64(x, (int)r);
  #elif defined(_MSC_VER) && (MI_ARCH_X86 || MI_ARCH_ARM32)
    return _lrotr(x,(int)r);
  #else
    // The term `(-rshift)&(BITS-1)` is written instead of `BITS - rshift` to
    // avoid UB when `rshift==0`. See <https://blog.regehr.org/archives/1063>
    const unsigned int rshift = (unsigned int)(r) & (MI_SIZE_BITS-1);
    return ((x >> rshift) | (x << ((-rshift) & (MI_SIZE_BITS-1))));
  #endif
}

static inline size_t mi_rotl(size_t x, size_t r) {
  #if (mi_has_builtin(rotateleft64) && MI_SIZE_BITS==64)
    return mi_builtin(rotateleft64)(x,r);
  #elif (mi_has_builtin(rotateleft32) && MI_SIZE_BITS==32)
    return mi_builtin(rotateleft32)(x,r);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_ARM64)
    return _rotl64(x, (int)r);
  #elif defined(_MSC_VER) && (MI_ARCH_X86 || MI_ARCH_ARM32)
    return _lrotl(x, (int)r);
  #else
    // The term `(-rshift)&(BITS-1)` is written instead of `BITS - rshift` to
    // avoid UB when `rshift==0`. See <https://blog.regehr.org/archives/1063>
    const unsigned int rshift = (unsigned int)(r) & (MI_SIZE_BITS-1);
    return ((x << rshift) | (x >> ((-rshift) & (MI_SIZE_BITS-1))));
  #endif
}

static inline uint32_t mi_rotl32(uint32_t x, uint32_t r) {
  #if mi_has_builtin(rotateleft32)
    return mi_builtin(rotateleft32)(x,r);
  #elif defined(_MSC_VER) && (MI_ARCH_X64 || MI_ARCH_X86 || MI_ARCH_ARM64 || MI_ARCH_ARM32)
    return _lrotl(x, (int)r);
  #else
    // The term `(-rshift)&(BITS-1)` is written instead of `BITS - rshift` to
    // avoid UB when `rshift==0`. See <https://blog.regehr.org/archives/1063>
    const unsigned int rshift = (unsigned int)(r) & 31;
    return ((x << rshift) | (x >> ((-rshift) & 31)));
  #endif
}


// ------------------------------------------------------
// Raw addresses: the integer value of a pointer (without provenance).
// Usually `mi_addr_t == uintptr_t == size_t`, but on CHERI `uintptr_t` is a 128-bit capability
// while the address (`ptraddr_t`) is 64-bit. Use `mi_addr(p)` to get the address of a pointer
// (and derive pointers from an existing pointer to keep provenance).
// ------------------------------------------------------

#if defined(__PTRADDR_TYPE__)
typedef __PTRADDR_TYPE__  mi_addr_t;
#else
typedef uintptr_t         mi_addr_t;
#endif
#define MI_AU(x)          ((mi_addr_t)(x))

// Get the address part of a pointer (without provenance)
static inline mi_addr_t mi_addr(const void* p) {
  #if MI_CHERI
  return (mi_addr_t)__builtin_cheri_address_get(p);
  #else
  return (mi_addr_t)(uintptr_t)(p);
  #endif
}

// Derive a pointer with address `addr` from `p` (keeping the provenance of `p`)
static inline void* mi_ptr_with_addr(const void* p, mi_addr_t addr) {
  #if MI_CHERI
  return (void*)__builtin_cheri_address_set(p, addr);
  #else
  return (void*)((uintptr_t)p + (addr - mi_addr(p)));
  #endif
}

// A pointer from an address without provenance
static inline void* mi_ptr_from_addr(mi_addr_t addr) {
  return (void*)(uintptr_t)addr;
}

// ------------------------------------------------------
// Capability bounds and permissions (only on CHERI).
// Internally all blocks are derived from the page start which is bounded to the block area
// of the page, while pointers handed out to the user are bounded to their block and lose
// any non-data permissions.
// ------------------------------------------------------

#if MI_CHERI
#if defined(__has_include)
#if __has_include(<cheri/cherireg.h>)
#include <cheri/cherireg.h>     // CHERI_PERM_SW_VMEM on CheriBSD
#endif
#endif
#if defined(CHERI_PERM_SW_VMEM)
#define MI_CHERI_PERM_SW_VMEM   CHERI_PERM_SW_VMEM            // permission to (un)map memory
#elif defined(CHERI_PERM_CHERIABI_VMMAP)
#define MI_CHERI_PERM_SW_VMEM   CHERI_PERM_CHERIABI_VMMAP     // older CheriBSD
#else
#define MI_CHERI_PERM_SW_VMEM   0
#endif
// permissions that data pointers (user blocks and internal meta data) never need
#define MI_CHERI_PERMS_NOT_DATA (__CHERI_CAP_PERMISSION_PERMIT_EXECUTE__ | __CHERI_CAP_PERMISSION_PERMIT_SEAL__ | \
                                 __CHERI_CAP_PERMISSION_PERMIT_UNSEAL__  | __CHERI_CAP_PERMISSION_ACCESS_SYSTEM_REGISTERS__ | \
                                 MI_CHERI_PERM_SW_VMEM)
#endif

// The alignment needed for exactly representable bounds of `size` bytes (always a power of 2, and 1 if not on CHERI)
static inline size_t mi_bounds_align(size_t size) {
  #if MI_CHERI
  return ~(size_t)__builtin_cheri_representable_alignment_mask(size) + 1;
  #else
  MI_UNUSED(size);
  return 1;
  #endif
}

// Round `size` up such that bounds of that size can be exactly representable (`size` if not on CHERI)
static inline size_t mi_bounds_size(size_t size) {
  #if MI_CHERI
  return __builtin_cheri_round_representable_length(size);
  #else
  return size;
  #endif
}

// Restrict `p` to `size` bytes (rounded outward if that is not exactly representable)
static inline void* mi_ptr_bounded(void* p, size_t size) {
  #if MI_CHERI
  return __builtin_cheri_bounds_set(p, size);
  #else
  MI_UNUSED(size);
  return p;
  #endif
}

// Restrict a (non-NULL) pointer to `size` bytes and data permissions only
static inline void* mi_ptr_bounded_data(void* p, size_t size) {
  #if MI_CHERI
  return mi_ptr_bounded(__builtin_cheri_perms_and(p, ~(size_t)MI_CHERI_PERMS_NOT_DATA), size);
  #else
  MI_UNUSED(size);
  return p;
  #endif
}

// Re-derive `p` from the capability `auth` (when `p` lacks the bounds or permissions, e.g. a user pointer)
static inline void* mi_ptr_rederive(void* auth, void* p) {
  #if MI_CHERI
  return mi_ptr_with_addr(auth, mi_addr(p));
  #else
  MI_UNUSED(auth);
  return p;
  #endif
}

// Limit `size` to the bytes accessible through `p` (`size` if not on a system with provenance)
static inline size_t mi_ptr_bounds_limit(const void* p, size_t size) {
  #if MI_CHERI
  const size_t len = (size_t)__builtin_cheri_length_get(p);
  const size_t ofs = (size_t)__builtin_cheri_offset_get(p);
  const size_t avail = (ofs <= len ? len - ofs : 0);
  return (size <= avail ? size : avail);
  #else
  MI_UNUSED(p);
  return size;
  #endif
}


#endif // MI_BITS_H
