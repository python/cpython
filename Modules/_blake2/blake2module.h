#ifndef Py_BLAKE2MODULE_H
#define Py_BLAKE2MODULE_H

#ifdef HAVE_LIBB2
#include <blake2.h>

// copied from blake2-impl.h
static inline void
store32( void *dst, uint32_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy( dst, &w, sizeof( w ) );
#else
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
#endif
}

static inline void
store48( void *dst, uint64_t w )
{
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
}

static inline void
store64( void *dst, uint64_t w )
{
#if defined(NATIVE_LITTLE_ENDIAN)
  memcpy( dst, &w, sizeof( w ) );
#else
  uint8_t *p = ( uint8_t * )dst;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w; w >>= 8;
  *p++ = ( uint8_t )w;
#endif
}

static inline void
secure_zero_memory(void *v, size_t n)
{
#if defined(_WIN32) || defined(WIN32)
  SecureZeroMemory(v, n);
#elif defined(__hpux)
  static void *(*const volatile memset_v)(void *, int, size_t) = &memset;
  memset_v(v, 0, n);
#else
// prioritize first the general C11 call
#if defined(HAVE_MEMSET_S)
  memset_s(v, n, 0, n);
#elif defined(HAVE_EXPLICIT_BZERO)
  explicit_bzero(v, n);
#elif defined(HAVE_EXPLICIT_MEMSET)
  explicit_memset(v, 0, n);
#else
  memset(v, 0, n);
  __asm__ __volatile__("" :: "r"(v) : "memory");
#endif
#endif
}

#else
// use vendored copy of blake2

// Prefix all public blake2 symbols with PyBlake2_
#define blake2b            PyBlake2_blake2b
#define blake2b_compress   PyBlake2_blake2b_compress
#define blake2b_final      PyBlake2_blake2b_final
#define blake2b_init       PyBlake2_blake2b_init
#define blake2b_init_key   PyBlake2_blake2b_init_key
#define blake2b_init_param PyBlake2_blake2b_init_param
#define blake2b_update     PyBlake2_blake2b_update
#define blake2bp           PyBlake2_blake2bp
#define blake2bp_final     PyBlake2_blake2bp_final
#define blake2bp_init      PyBlake2_blake2bp_init
#define blake2bp_init_key  PyBlake2_blake2bp_init_key
#define blake2bp_update    PyBlake2_blake2bp_update
#define blake2s            PyBlake2_blake2s
#define blake2s_compress   PyBlake2_blake2s_compress
#define blake2s_final      PyBlake2_blake2s_final
#define blake2s_init       PyBlake2_blake2s_init
#define blake2s_init_key   PyBlake2_blake2s_init_key
#define blake2s_init_param PyBlake2_blake2s_init_param
#define blake2s_update     PyBlake2_blake2s_update
#define blake2sp           PyBlake2_blake2sp
#define blake2sp_final     PyBlake2_blake2sp_final
#define blake2sp_init      PyBlake2_blake2sp_init
#define blake2sp_init_key  PyBlake2_blake2sp_init_key
#define blake2sp_update    PyBlake2_blake2sp_update

#include "impl/blake2.h"
#include "impl/blake2-impl.h" /* for secure_zero_memory() and store48() */

#endif // HAVE_LIBB2

#endif // Py_BLAKE2MODULE_H
