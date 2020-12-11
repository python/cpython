#include "tommath_private.h"
#ifdef BN_S_MP_RAND_PLATFORM_C
/* LibTomMath, multiple-precision integer library -- Tom St Denis */
/* SPDX-License-Identifier: Unlicense */

/* First the OS-specific special cases
 * - *BSD
 * - Windows
 */
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
#define BN_S_READ_ARC4RANDOM_C
static mp_err s_read_arc4random(void *p, size_t n)
{
   arc4random_buf(p, n);
   return MP_OKAY;
}
#endif

#if defined(_WIN32) || defined(_WIN32_WCE)
#define BN_S_READ_WINCSP_C

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#ifdef _WIN32_WCE
#define UNDER_CE
#define ARM
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>

static mp_err s_read_wincsp(void *p, size_t n)
{
   static HCRYPTPROV hProv = 0;
   if (hProv == 0) {
      HCRYPTPROV h = 0;
      if (!CryptAcquireContext(&h, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                               (CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET)) &&
          !CryptAcquireContext(&h, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                               CRYPT_VERIFYCONTEXT | CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET)) {
         return MP_ERR;
      }
      hProv = h;
   }
   return CryptGenRandom(hProv, (DWORD)n, (BYTE *)p) == TRUE ? MP_OKAY : MP_ERR;
}
#endif /* WIN32 */

#if !defined(BN_S_READ_WINCSP_C) && defined(__linux__) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 25)
#define BN_S_READ_GETRANDOM_C
#include <sys/random.h>
#include <errno.h>

static mp_err s_read_getrandom(void *p, size_t n)
{
   char *q = (char *)p;
   while (n > 0u) {
      ssize_t ret = getrandom(q, n, 0);
      if (ret < 0) {
         if (errno == EINTR) {
            continue;
         }
         return MP_ERR;
      }
      q += ret;
      n -= (size_t)ret;
   }
   return MP_OKAY;
}
#endif
#endif

/* We assume all platforms besides windows provide "/dev/urandom".
 * In case yours doesn't, define MP_NO_DEV_URANDOM at compile-time.
 */
#if !defined(BN_S_READ_WINCSP_C) && !defined(MP_NO_DEV_URANDOM)
#define BN_S_READ_URANDOM_C
#ifndef MP_DEV_URANDOM
#define MP_DEV_URANDOM "/dev/urandom"
#endif
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

static mp_err s_read_urandom(void *p, size_t n)
{
   int fd;
   char *q = (char *)p;

   do {
      fd = open(MP_DEV_URANDOM, O_RDONLY);
   } while ((fd == -1) && (errno == EINTR));
   if (fd == -1) return MP_ERR;

   while (n > 0u) {
      ssize_t ret = read(fd, p, n);
      if (ret < 0) {
         if (errno == EINTR) {
            continue;
         }
         close(fd);
         return MP_ERR;
      }
      q += ret;
      n -= (size_t)ret;
   }

   close(fd);
   return MP_OKAY;
}
#endif

#if defined(MP_PRNG_ENABLE_LTM_RNG)
#define BN_S_READ_LTM_RNG
unsigned long (*ltm_rng)(unsigned char *out, unsigned long outlen, void (*callback)(void));
void (*ltm_rng_callback)(void);

static mp_err s_read_ltm_rng(void *p, size_t n)
{
   unsigned long res;
   if (ltm_rng == NULL) return MP_ERR;
   res = ltm_rng(p, n, ltm_rng_callback);
   if (res != n) return MP_ERR;
   return MP_OKAY;
}
#endif

mp_err s_read_arc4random(void *p, size_t n);
mp_err s_read_wincsp(void *p, size_t n);
mp_err s_read_getrandom(void *p, size_t n);
mp_err s_read_urandom(void *p, size_t n);
mp_err s_read_ltm_rng(void *p, size_t n);

mp_err s_mp_rand_platform(void *p, size_t n)
{
   mp_err err = MP_ERR;
   if ((err != MP_OKAY) && MP_HAS(S_READ_ARC4RANDOM)) err = s_read_arc4random(p, n);
   if ((err != MP_OKAY) && MP_HAS(S_READ_WINCSP))     err = s_read_wincsp(p, n);
   if ((err != MP_OKAY) && MP_HAS(S_READ_GETRANDOM))  err = s_read_getrandom(p, n);
   if ((err != MP_OKAY) && MP_HAS(S_READ_URANDOM))    err = s_read_urandom(p, n);
   if ((err != MP_OKAY) && MP_HAS(S_READ_LTM_RNG))    err = s_read_ltm_rng(p, n);
   return err;
}

#endif
