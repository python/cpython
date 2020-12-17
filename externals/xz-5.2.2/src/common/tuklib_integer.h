///////////////////////////////////////////////////////////////////////////////
//
/// \file       tuklib_integer.h
/// \brief      Various integer and bit operations
///
/// This file provides macros or functions to do some basic integer and bit
/// operations.
///
/// Native endian inline functions (XX = 16, 32, or 64):
///   - Unaligned native endian reads: readXXne(ptr)
///   - Unaligned native endian writes: writeXXne(ptr, num)
///   - Aligned native endian reads: aligned_readXXne(ptr)
///   - Aligned native endian writes: aligned_writeXXne(ptr, num)
///
/// Endianness-converting integer operations (these can be macros!)
/// (XX = 16, 32, or 64; Y = b or l):
///   - Byte swapping: bswapXX(num)
///   - Byte order conversions to/from native (byteswaps if Y isn't
///     the native endianness): convXXYe(num)
///   - Unaligned reads (16/32-bit only): readXXYe(ptr)
///   - Unaligned writes (16/32-bit only): writeXXYe(ptr, num)
///   - Aligned reads: aligned_readXXYe(ptr)
///   - Aligned writes: aligned_writeXXYe(ptr, num)
///
/// Since the above can macros, the arguments should have no side effects
/// because they may be evaluated more than once.
///
/// Bit scan operations for non-zero 32-bit integers (inline functions):
///   - Bit scan reverse (find highest non-zero bit): bsr32(num)
///   - Count leading zeros: clz32(num)
///   - Count trailing zeros: ctz32(num)
///   - Bit scan forward (simply an alias for ctz32()): bsf32(num)
///
/// The above bit scan operations return 0-31. If num is zero,
/// the result is undefined.
//
//  Authors:    Lasse Collin
//              Joachim Henke
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TUKLIB_INTEGER_H
#define TUKLIB_INTEGER_H

#include "tuklib_common.h"
#include <string.h>

// Newer Intel C compilers require immintrin.h for _bit_scan_reverse()
// and such functions.
#if defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 1500)
#	include <immintrin.h>
#endif


///////////////////
// Byte swapping //
///////////////////

#if defined(HAVE___BUILTIN_BSWAPXX)
	// GCC >= 4.8 and Clang
#	define bswap16(n) __builtin_bswap16(n)
#	define bswap32(n) __builtin_bswap32(n)
#	define bswap64(n) __builtin_bswap64(n)

#elif defined(HAVE_BYTESWAP_H)
	// glibc, uClibc, dietlibc
#	include <byteswap.h>
#	ifdef HAVE_BSWAP_16
#		define bswap16(num) bswap_16(num)
#	endif
#	ifdef HAVE_BSWAP_32
#		define bswap32(num) bswap_32(num)
#	endif
#	ifdef HAVE_BSWAP_64
#		define bswap64(num) bswap_64(num)
#	endif

#elif defined(HAVE_SYS_ENDIAN_H)
	// *BSDs and Darwin
#	include <sys/endian.h>

#elif defined(HAVE_SYS_BYTEORDER_H)
	// Solaris
#	include <sys/byteorder.h>
#	ifdef BSWAP_16
#		define bswap16(num) BSWAP_16(num)
#	endif
#	ifdef BSWAP_32
#		define bswap32(num) BSWAP_32(num)
#	endif
#	ifdef BSWAP_64
#		define bswap64(num) BSWAP_64(num)
#	endif
#	ifdef BE_16
#		define conv16be(num) BE_16(num)
#	endif
#	ifdef BE_32
#		define conv32be(num) BE_32(num)
#	endif
#	ifdef BE_64
#		define conv64be(num) BE_64(num)
#	endif
#	ifdef LE_16
#		define conv16le(num) LE_16(num)
#	endif
#	ifdef LE_32
#		define conv32le(num) LE_32(num)
#	endif
#	ifdef LE_64
#		define conv64le(num) LE_64(num)
#	endif
#endif

#ifndef bswap16
#	define bswap16(n) (uint16_t)( \
		  (((n) & 0x00FFU) << 8) \
		| (((n) & 0xFF00U) >> 8) \
	)
#endif

#ifndef bswap32
#	define bswap32(n) (uint32_t)( \
		  (((n) & UINT32_C(0x000000FF)) << 24) \
		| (((n) & UINT32_C(0x0000FF00)) << 8) \
		| (((n) & UINT32_C(0x00FF0000)) >> 8) \
		| (((n) & UINT32_C(0xFF000000)) >> 24) \
	)
#endif

#ifndef bswap64
#	define bswap64(n) (uint64_t)( \
		  (((n) & UINT64_C(0x00000000000000FF)) << 56) \
		| (((n) & UINT64_C(0x000000000000FF00)) << 40) \
		| (((n) & UINT64_C(0x0000000000FF0000)) << 24) \
		| (((n) & UINT64_C(0x00000000FF000000)) << 8) \
		| (((n) & UINT64_C(0x000000FF00000000)) >> 8) \
		| (((n) & UINT64_C(0x0000FF0000000000)) >> 24) \
		| (((n) & UINT64_C(0x00FF000000000000)) >> 40) \
		| (((n) & UINT64_C(0xFF00000000000000)) >> 56) \
	)
#endif

// Define conversion macros using the basic byte swapping macros.
#ifdef WORDS_BIGENDIAN
#	ifndef conv16be
#		define conv16be(num) ((uint16_t)(num))
#	endif
#	ifndef conv32be
#		define conv32be(num) ((uint32_t)(num))
#	endif
#	ifndef conv64be
#		define conv64be(num) ((uint64_t)(num))
#	endif
#	ifndef conv16le
#		define conv16le(num) bswap16(num)
#	endif
#	ifndef conv32le
#		define conv32le(num) bswap32(num)
#	endif
#	ifndef conv64le
#		define conv64le(num) bswap64(num)
#	endif
#else
#	ifndef conv16be
#		define conv16be(num) bswap16(num)
#	endif
#	ifndef conv32be
#		define conv32be(num) bswap32(num)
#	endif
#	ifndef conv64be
#		define conv64be(num) bswap64(num)
#	endif
#	ifndef conv16le
#		define conv16le(num) ((uint16_t)(num))
#	endif
#	ifndef conv32le
#		define conv32le(num) ((uint32_t)(num))
#	endif
#	ifndef conv64le
#		define conv64le(num) ((uint64_t)(num))
#	endif
#endif


////////////////////////////////
// Unaligned reads and writes //
////////////////////////////////

// The traditional way of casting e.g. *(const uint16_t *)uint8_pointer
// is bad even if the uint8_pointer is properly aligned because this kind
// of casts break strict aliasing rules and result in undefined behavior.
// With unaligned pointers it's even worse: compilers may emit vector
// instructions that require aligned pointers even if non-vector
// instructions work with unaligned pointers.
//
// Using memcpy() is the standard compliant way to do unaligned access.
// Many modern compilers inline it so there is no function call overhead.
// For those compilers that don't handle the memcpy() method well, the
// old casting method (that violates strict aliasing) can be requested at
// build time. A third method, casting to a packed struct, would also be
// an option but isn't provided to keep things simpler (it's already a mess).
// Hopefully this is flexible enough in practice.

static inline uint16_t
read16ne(const uint8_t *buf)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	return *(const uint16_t *)buf;
#else
	uint16_t num;
	memcpy(&num, buf, sizeof(num));
	return num;
#endif
}


static inline uint32_t
read32ne(const uint8_t *buf)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	return *(const uint32_t *)buf;
#else
	uint32_t num;
	memcpy(&num, buf, sizeof(num));
	return num;
#endif
}


static inline uint64_t
read64ne(const uint8_t *buf)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	return *(const uint64_t *)buf;
#else
	uint64_t num;
	memcpy(&num, buf, sizeof(num));
	return num;
#endif
}


static inline void
write16ne(uint8_t *buf, uint16_t num)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	*(uint16_t *)buf = num;
#else
	memcpy(buf, &num, sizeof(num));
#endif
	return;
}


static inline void
write32ne(uint8_t *buf, uint32_t num)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	*(uint32_t *)buf = num;
#else
	memcpy(buf, &num, sizeof(num));
#endif
	return;
}


static inline void
write64ne(uint8_t *buf, uint64_t num)
{
#if defined(TUKLIB_FAST_UNALIGNED_ACCESS) \
		&& defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING)
	*(uint64_t *)buf = num;
#else
	memcpy(buf, &num, sizeof(num));
#endif
	return;
}


static inline uint16_t
read16be(const uint8_t *buf)
{
#if defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
	uint16_t num = read16ne(buf);
	return conv16be(num);
#else
	uint16_t num = ((uint16_t)buf[0] << 8) | (uint16_t)buf[1];
	return num;
#endif
}


static inline uint16_t
read16le(const uint8_t *buf)
{
#if !defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
	uint16_t num = read16ne(buf);
	return conv16le(num);
#else
	uint16_t num = ((uint16_t)buf[0]) | ((uint16_t)buf[1] << 8);
	return num;
#endif
}


static inline uint32_t
read32be(const uint8_t *buf)
{
#if defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
	uint32_t num = read32ne(buf);
	return conv32be(num);
#else
	uint32_t num = (uint32_t)buf[0] << 24;
	num |= (uint32_t)buf[1] << 16;
	num |= (uint32_t)buf[2] << 8;
	num |= (uint32_t)buf[3];
	return num;
#endif
}


static inline uint32_t
read32le(const uint8_t *buf)
{
#if !defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
	uint32_t num = read32ne(buf);
	return conv32le(num);
#else
	uint32_t num = (uint32_t)buf[0];
	num |= (uint32_t)buf[1] << 8;
	num |= (uint32_t)buf[2] << 16;
	num |= (uint32_t)buf[3] << 24;
	return num;
#endif
}


// NOTE: Possible byte swapping must be done in a macro to allow the compiler
// to optimize byte swapping of constants when using glibc's or *BSD's
// byte swapping macros. The actual write is done in an inline function
// to make type checking of the buf pointer possible.
#if defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
#	define write16be(buf, num) write16ne(buf, conv16be(num))
#	define write32be(buf, num) write32ne(buf, conv32be(num))
#endif

#if !defined(WORDS_BIGENDIAN) || defined(TUKLIB_FAST_UNALIGNED_ACCESS)
#	define write16le(buf, num) write16ne(buf, conv16le(num))
#	define write32le(buf, num) write32ne(buf, conv32le(num))
#endif


#ifndef write16be
static inline void
write16be(uint8_t *buf, uint16_t num)
{
	buf[0] = (uint8_t)(num >> 8);
	buf[1] = (uint8_t)num;
	return;
}
#endif


#ifndef write16le
static inline void
write16le(uint8_t *buf, uint16_t num)
{
	buf[0] = (uint8_t)num;
	buf[1] = (uint8_t)(num >> 8);
	return;
}
#endif


#ifndef write32be
static inline void
write32be(uint8_t *buf, uint32_t num)
{
	buf[0] = (uint8_t)(num >> 24);
	buf[1] = (uint8_t)(num >> 16);
	buf[2] = (uint8_t)(num >> 8);
	buf[3] = (uint8_t)num;
	return;
}
#endif


#ifndef write32le
static inline void
write32le(uint8_t *buf, uint32_t num)
{
	buf[0] = (uint8_t)num;
	buf[1] = (uint8_t)(num >> 8);
	buf[2] = (uint8_t)(num >> 16);
	buf[3] = (uint8_t)(num >> 24);
	return;
}
#endif


//////////////////////////////
// Aligned reads and writes //
//////////////////////////////

// Separate functions for aligned reads and writes are provided since on
// strict-align archs aligned access is much faster than unaligned access.
//
// Just like in the unaligned case, memcpy() is needed to avoid
// strict aliasing violations. However, on archs that don't support
// unaligned access the compiler cannot know that the pointers given
// to memcpy() are aligned which results in slow code. As of C11 there is
// no standard way to tell the compiler that we know that the address is
// aligned but some compilers have language extensions to do that. With
// such language extensions the memcpy() method gives excellent results.
//
// What to do on a strict-align system when no known language extentensions
// are available? Falling back to byte-by-byte access would be safe but ruin
// optimizations that have been made specifically with aligned access in mind.
// As a compromise, aligned reads will fall back to non-compliant type punning
// but aligned writes will be byte-by-byte, that is, fast reads are preferred
// over fast writes. This obviously isn't great but hopefully it's a working
// compromise for now.
//
// __builtin_assume_aligned is support by GCC >= 4.7 and clang >= 3.6.
#ifdef HAVE___BUILTIN_ASSUME_ALIGNED
#	define tuklib_memcpy_aligned(dest, src, size) \
		memcpy(dest, __builtin_assume_aligned(src, size), size)
#else
#	define tuklib_memcpy_aligned(dest, src, size) \
		memcpy(dest, src, size)
#	ifndef TUKLIB_FAST_UNALIGNED_ACCESS
#		define TUKLIB_USE_UNSAFE_ALIGNED_READS 1
#	endif
#endif


static inline uint16_t
aligned_read16ne(const uint8_t *buf)
{
#if defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING) \
		|| defined(TUKLIB_USE_UNSAFE_ALIGNED_READS)
	return *(const uint16_t *)buf;
#else
	uint16_t num;
	tuklib_memcpy_aligned(&num, buf, sizeof(num));
	return num;
#endif
}


static inline uint32_t
aligned_read32ne(const uint8_t *buf)
{
#if defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING) \
		|| defined(TUKLIB_USE_UNSAFE_ALIGNED_READS)
	return *(const uint32_t *)buf;
#else
	uint32_t num;
	tuklib_memcpy_aligned(&num, buf, sizeof(num));
	return num;
#endif
}


static inline uint64_t
aligned_read64ne(const uint8_t *buf)
{
#if defined(TUKLIB_USE_UNSAFE_TYPE_PUNNING) \
		|| defined(TUKLIB_USE_UNSAFE_ALIGNED_READS)
	return *(const uint64_t *)buf;
#else
	uint64_t num;
	tuklib_memcpy_aligned(&num, buf, sizeof(num));
	return num;
#endif
}


static inline void
aligned_write16ne(uint8_t *buf, uint16_t num)
{
#ifdef TUKLIB_USE_UNSAFE_TYPE_PUNNING
	*(uint16_t *)buf = num;
#else
	tuklib_memcpy_aligned(buf, &num, sizeof(num));
#endif
	return;
}


static inline void
aligned_write32ne(uint8_t *buf, uint32_t num)
{
#ifdef TUKLIB_USE_UNSAFE_TYPE_PUNNING
	*(uint32_t *)buf = num;
#else
	tuklib_memcpy_aligned(buf, &num, sizeof(num));
#endif
	return;
}


static inline void
aligned_write64ne(uint8_t *buf, uint64_t num)
{
#ifdef TUKLIB_USE_UNSAFE_TYPE_PUNNING
	*(uint64_t *)buf = num;
#else
	tuklib_memcpy_aligned(buf, &num, sizeof(num));
#endif
	return;
}


static inline uint16_t
aligned_read16be(const uint8_t *buf)
{
	uint16_t num = aligned_read16ne(buf);
	return conv16be(num);
}


static inline uint16_t
aligned_read16le(const uint8_t *buf)
{
	uint16_t num = aligned_read16ne(buf);
	return conv16le(num);
}


static inline uint32_t
aligned_read32be(const uint8_t *buf)
{
	uint32_t num = aligned_read32ne(buf);
	return conv32be(num);
}


static inline uint32_t
aligned_read32le(const uint8_t *buf)
{
	uint32_t num = aligned_read32ne(buf);
	return conv32le(num);
}


static inline uint64_t
aligned_read64be(const uint8_t *buf)
{
	uint64_t num = aligned_read64ne(buf);
	return conv64be(num);
}


static inline uint64_t
aligned_read64le(const uint8_t *buf)
{
	uint64_t num = aligned_read64ne(buf);
	return conv64le(num);
}


// These need to be macros like in the unaligned case.
#define aligned_write16be(buf, num) aligned_write16ne((buf), conv16be(num))
#define aligned_write16le(buf, num) aligned_write16ne((buf), conv16le(num))
#define aligned_write32be(buf, num) aligned_write32ne((buf), conv32be(num))
#define aligned_write32le(buf, num) aligned_write32ne((buf), conv32le(num))
#define aligned_write64be(buf, num) aligned_write64ne((buf), conv64be(num))
#define aligned_write64le(buf, num) aligned_write64ne((buf), conv64le(num))


////////////////////
// Bit operations //
////////////////////

static inline uint32_t
bsr32(uint32_t n)
{
	// Check for ICC first, since it tends to define __GNUC__ too.
#if defined(__INTEL_COMPILER)
	return _bit_scan_reverse(n);

#elif TUKLIB_GNUC_REQ(3, 4) && UINT_MAX == UINT32_MAX
	// GCC >= 3.4 has __builtin_clz(), which gives good results on
	// multiple architectures. On x86, __builtin_clz() ^ 31U becomes
	// either plain BSR (so the XOR gets optimized away) or LZCNT and
	// XOR (if -march indicates that SSE4a instructions are supported).
	return (uint32_t)__builtin_clz(n) ^ 31U;

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	uint32_t i;
	__asm__("bsrl %1, %0" : "=r" (i) : "rm" (n));
	return i;

#elif defined(_MSC_VER)
	unsigned long i;
	_BitScanReverse(&i, n);
	return i;

#else
	uint32_t i = 31;

	if ((n & 0xFFFF0000) == 0) {
		n <<= 16;
		i = 15;
	}

	if ((n & 0xFF000000) == 0) {
		n <<= 8;
		i -= 8;
	}

	if ((n & 0xF0000000) == 0) {
		n <<= 4;
		i -= 4;
	}

	if ((n & 0xC0000000) == 0) {
		n <<= 2;
		i -= 2;
	}

	if ((n & 0x80000000) == 0)
		--i;

	return i;
#endif
}


static inline uint32_t
clz32(uint32_t n)
{
#if defined(__INTEL_COMPILER)
	return _bit_scan_reverse(n) ^ 31U;

#elif TUKLIB_GNUC_REQ(3, 4) && UINT_MAX == UINT32_MAX
	return (uint32_t)__builtin_clz(n);

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	uint32_t i;
	__asm__("bsrl %1, %0\n\t"
		"xorl $31, %0"
		: "=r" (i) : "rm" (n));
	return i;

#elif defined(_MSC_VER)
	unsigned long i;
	_BitScanReverse(&i, n);
	return i ^ 31U;

#else
	uint32_t i = 0;

	if ((n & 0xFFFF0000) == 0) {
		n <<= 16;
		i = 16;
	}

	if ((n & 0xFF000000) == 0) {
		n <<= 8;
		i += 8;
	}

	if ((n & 0xF0000000) == 0) {
		n <<= 4;
		i += 4;
	}

	if ((n & 0xC0000000) == 0) {
		n <<= 2;
		i += 2;
	}

	if ((n & 0x80000000) == 0)
		++i;

	return i;
#endif
}


static inline uint32_t
ctz32(uint32_t n)
{
#if defined(__INTEL_COMPILER)
	return _bit_scan_forward(n);

#elif TUKLIB_GNUC_REQ(3, 4) && UINT_MAX >= UINT32_MAX
	return (uint32_t)__builtin_ctz(n);

#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
	uint32_t i;
	__asm__("bsfl %1, %0" : "=r" (i) : "rm" (n));
	return i;

#elif defined(_MSC_VER)
	unsigned long i;
	_BitScanForward(&i, n);
	return i;

#else
	uint32_t i = 0;

	if ((n & 0x0000FFFF) == 0) {
		n >>= 16;
		i = 16;
	}

	if ((n & 0x000000FF) == 0) {
		n >>= 8;
		i += 8;
	}

	if ((n & 0x0000000F) == 0) {
		n >>= 4;
		i += 4;
	}

	if ((n & 0x00000003) == 0) {
		n >>= 2;
		i += 2;
	}

	if ((n & 0x00000001) == 0)
		++i;

	return i;
#endif
}

#define bsf32 ctz32

#endif
