#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ffi.h>
#include "fficonfig.h"

#if defined HAVE_STDINT_H
#include <stdint.h>
#endif

#if defined HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#define MAX_ARGS 256

#define CHECK(x) !(x) ? abort() : 0

/* Define __UNUSED__ that also other compilers than gcc can run the tests.  */
#undef __UNUSED__
#if defined(__GNUC__)
#define __UNUSED__ __attribute__((__unused__))
#else
#define __UNUSED__
#endif

/* Prefer MAP_ANON(YMOUS) to /dev/zero, since we don't need to keep a
   file open.  */
#ifdef HAVE_MMAP_ANON
# undef HAVE_MMAP_DEV_ZERO

# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED -1
# endif
# if !defined (MAP_ANONYMOUS) && defined (MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif
# define USING_MMAP

#endif

#ifdef HAVE_MMAP_DEV_ZERO

# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED -1
# endif
# define USING_MMAP

#endif

/* MinGW kludge.  */
#ifdef _WIN64
#define PRIdLL "I64d"
#define PRIuLL "I64u"
#else
#define PRIdLL "lld"
#define PRIuLL "llu"
#endif

/* Tru64 UNIX kludge.  */
#if defined(__alpha__) && defined(__osf__)
/* Tru64 UNIX V4.0 doesn't support %lld/%lld, but long is 64-bit.  */
#undef PRIdLL
#define PRIdLL "ld"
#undef PRIuLL
#define PRIuLL "lu"
#define PRId64 "ld"
#define PRIu64 "lu"
#define PRIuPTR "lu"
#endif

/* PA HP-UX kludge.  */
#if defined(__hppa__) && defined(__hpux__) && !defined(PRIuPTR)
#define PRIuPTR "lu"
#endif

/* Solaris < 10 kludge.  */
#if defined(__sun__) && defined(__svr4__) && !defined(PRIuPTR)
#if defined(__arch64__) || defined (__x86_64__)
#define PRIuPTR "lu"
#else
#define PRIuPTR "u"
#endif
#endif

#ifdef USING_MMAP
static inline void *
allocate_mmap (size_t size)
{
  void *page;
#if defined (HAVE_MMAP_DEV_ZERO)
  static int dev_zero_fd = -1;
#endif

#ifdef HAVE_MMAP_DEV_ZERO
  if (dev_zero_fd == -1)
    {
      dev_zero_fd = open ("/dev/zero", O_RDONLY);
      if (dev_zero_fd == -1)
	{
	  perror ("open /dev/zero: %m");
	  exit (1);
	}
    }
#endif


#ifdef HAVE_MMAP_ANON
  page = mmap (NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
	       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
#ifdef HAVE_MMAP_DEV_ZERO
  page = mmap (NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC,
	       MAP_PRIVATE, dev_zero_fd, 0);
#endif

  if (page == (void *) MAP_FAILED)
    {
      perror ("virtual memory exhausted");
      exit (1);
    }

  return page;
}

#endif
