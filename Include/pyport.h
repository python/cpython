#ifndef Py_PYPORT_H
#define Py_PYPORT_H

#include "pyconfig.h" /* include for defines */

/**************************************************************************
Symbols and macros to supply platform-independent interfaces to basic
C language & library operations whose spellings vary across platforms.

Please try to make documentation here as clear as possible:  by definition,
the stuff here is trying to illuminate C's darkest corners.

Config #defines referenced here:

SIGNED_RIGHT_SHIFT_ZERO_FILLS
Meaning:  To be defined iff i>>j does not extend the sign bit when i is a
          signed integral type and i < 0.
Used in:  Py_ARITHMETIC_RIGHT_SHIFT

Py_DEBUG
Meaning:  Extra checks compiled in for debug mode.
Used in:  Py_SAFE_DOWNCAST

HAVE_UINTPTR_T
Meaning:  The C9X type uintptr_t is supported by the compiler
Used in:  Py_uintptr_t

HAVE_LONG_LONG
Meaning:  The compiler supports the C type "long long"
Used in:  LONG_LONG

**************************************************************************/


/* For backward compatibility only. Obsolete, do not use. */
#define ANY void
#ifdef HAVE_PROTOTYPES
#define Py_PROTO(x) x
#else
#define Py_PROTO(x) ()
#endif
#ifndef Py_FPROTO
#define Py_FPROTO(x) Py_PROTO(x)
#endif

/* typedefs for some C9X-defined synonyms for integral types.
 *
 * The names in Python are exactly the same as the C9X names, except with a
 * Py_ prefix.  Until C9X is universally implemented, this is the only way
 * to ensure that Python gets reliable names that don't conflict with names
 * in non-Python code that are playing their own tricks to define the C9X
 * names.
 *
 * NOTE: don't go nuts here!  Python has no use for *most* of the C9X
 * integral synonyms.  Only define the ones we actually need.
 */

#ifdef HAVE_LONG_LONG
#ifndef LONG_LONG
#define LONG_LONG long long
#endif
#endif /* HAVE_LONG_LONG */

/* uintptr_t is the C9X name for an unsigned integral type such that a
 * legitimate void* can be cast to uintptr_t and then back to void* again
 * without loss of information.  Similarly for intptr_t, wrt a signed
 * integral type.
 */
#ifdef HAVE_UINTPTR_T
typedef uintptr_t	Py_uintptr_t;
typedef intptr_t	Py_intptr_t;

#elif SIZEOF_VOID_P <= SIZEOF_INT
typedef unsigned int	Py_uintptr_t;
typedef int		Py_intptr_t;

#elif SIZEOF_VOID_P <= SIZEOF_LONG
typedef unsigned long	Py_uintptr_t;
typedef long		Py_intptr_t;

#elif defined(HAVE_LONG_LONG) && (SIZEOF_VOID_P <= SIZEOF_LONG_LONG)
typedef unsigned LONG_LONG	Py_uintptr_t;
typedef LONG_LONG		Py_intptr_t;

#else
#   error "Python needs a typedef for Py_uintptr_t in pyport.h."
#endif /* HAVE_UINTPTR_T */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <math.h> /* Moved here from the math section, before extern "C" */

/********************************************
 * WRAPPER FOR <time.h> and/or <sys/time.h> *
 ********************************************/

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else /* !TIME_WITH_SYS_TIME */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */


/******************************
 * WRAPPER FOR <sys/select.h> *
 ******************************/

/* NB caller must include <sys/types.h> */

#ifdef HAVE_SYS_SELECT_H

#include <sys/select.h>

#else /* !HAVE_SYS_SELECT_H */

#ifdef USE_GUSI1
/* If we don't have sys/select the definition may be in unistd.h */
#include <GUSI.h>
#endif

#endif /* !HAVE_SYS_SELECT_H */

/*******************************
 * stat() and fstat() fiddling *
 *******************************/

/* We expect that stat and fstat exist on most systems.
 *  It's confirmed on Unix, Mac and Windows.
 *  If you don't have them, add
 *      #define DONT_HAVE_STAT
 * and/or
 *      #define DONT_HAVE_FSTAT
 * to your pyconfig.h. Python code beyond this should check HAVE_STAT and
 * HAVE_FSTAT instead.
 * Also
 *      #define DONT_HAVE_SYS_STAT_H
 * if <sys/stat.h> doesn't exist on your platform, and
 *      #define HAVE_STAT_H
 * if <stat.h> does (don't look at me -- ths mess is inherited).
 */
#ifndef DONT_HAVE_STAT
#define HAVE_STAT
#endif

#ifndef DONT_HAVE_FSTAT
#define HAVE_FSTAT
#endif

#ifdef RISCOS
#include <sys/types.h>
#endif

#ifndef DONT_HAVE_SYS_STAT_H
#include <sys/stat.h>
#elif defined(HAVE_STAT_H)
#include <stat.h>
#endif

#if defined(PYCC_VACPP)
/* VisualAge C/C++ Failed to Define MountType Field in sys/stat.h */
#define S_IFMT (S_IFDIR|S_IFCHR|S_IFREG)
#endif

#ifndef S_ISREG
#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif


#ifdef __cplusplus
/* Move this down here since some C++ #include's don't like to be included
   inside an extern "C" */
extern "C" {
#endif


/* Py_ARITHMETIC_RIGHT_SHIFT
 * C doesn't define whether a right-shift of a signed integer sign-extends
 * or zero-fills.  Here a macro to force sign extension:
 * Py_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J)
 *    Return I >> J, forcing sign extension.
 * Requirements:
 *    I is of basic signed type TYPE (char, short, int, long, or long long).
 *    TYPE is one of char, short, int, long, or long long, although long long
 *    must not be used except on platforms that support it.
 *    J is an integer >= 0 and strictly less than the number of bits in TYPE
 *    (because C doesn't define what happens for J outside that range either).
 * Caution:
 *    I may be evaluated more than once.
 */
#ifdef SIGNED_RIGHT_SHIFT_ZERO_FILLS
#define Py_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) \
	((I) < 0 ? ~((~(unsigned TYPE)(I)) >> (J)) : (I) >> (J))
#else
#define Py_ARITHMETIC_RIGHT_SHIFT(TYPE, I, J) ((I) >> (J))
#endif

/* Py_FORCE_EXPANSION(X)
 * "Simply" returns its argument.  However, macro expansions within the
 * argument are evaluated.  This unfortunate trickery is needed to get
 * token-pasting to work as desired in some cases.
 */
#define Py_FORCE_EXPANSION(X) X

/* Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW)
 * Cast VALUE to type NARROW from type WIDE.  In Py_DEBUG mode, this
 * assert-fails if any information is lost.
 * Caution:
 *    VALUE may be evaluated more than once.
 */
#ifdef Py_DEBUG
#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) \
	(assert((WIDE)(NARROW)(VALUE) == (VALUE)), (NARROW)(VALUE))
#else
#define Py_SAFE_DOWNCAST(VALUE, WIDE, NARROW) (NARROW)(VALUE)
#endif

/* Py_IS_INFINITY(X)
 * Return 1 if float or double arg is an infinity, else 0.
 * Caution:
 *    X is evaluated more than once.
 *    This implementation may set the underflow flag if |X| is very small;
 *    it really can't be implemented correctly (& easily) before C99.
 */
#define Py_IS_INFINITY(X) ((X) && (X)*0.5 == (X))

/* According to
 * http://www.cray.com/swpubs/manuals/SN-2194_2.0/html-SN-2194_2.0/x3138.htm
 * on some Cray systems HUGE_VAL is incorrectly (according to the C std)
 * defined to be the largest positive finite rather than infinity.  We need
 * the std-conforming infinity meaning (provided the platform has one!).
 *
 * Then, according to a bug report on SourceForge, defining Py_HUGE_VAL as
 * INFINITY caused internal compiler errors under BeOS using some version
 * of gcc.  Explicitly casting INFINITY to double made that problem go away.
 */
#ifdef INFINITY
#define Py_HUGE_VAL ((double)INFINITY)
#else
#define Py_HUGE_VAL HUGE_VAL
#endif

/* Py_OVERFLOWED(X)
 * Return 1 iff a libm function overflowed.  Set errno to 0 before calling
 * a libm function, and invoke this macro after, passing the function
 * result.
 * Caution:
 *    This isn't reliable.  C99 no longer requires libm to set errno under
 *	  any exceptional condition, but does require +- HUGE_VAL return
 *	  values on overflow.  A 754 box *probably* maps HUGE_VAL to a
 *	  double infinity, and we're cool if that's so, unless the input
 *	  was an infinity and an infinity is the expected result.  A C89
 *	  system sets errno to ERANGE, so we check for that too.  We're
 *	  out of luck if a C99 754 box doesn't map HUGE_VAL to +Inf, or
 *	  if the returned result is a NaN, or if a C89 box returns HUGE_VAL
 *	  in non-overflow cases.
 *    X is evaluated more than once.
 */
#define Py_OVERFLOWED(X) ((X) != 0.0 && (errno == ERANGE ||    \
					 (X) == Py_HUGE_VAL || \
					 (X) == -Py_HUGE_VAL))

/* Py_SET_ERANGE_ON_OVERFLOW(x)
 * If a libm function did not set errno, but it looks like the result
 * overflowed, set errno to ERANGE.  Set errno to 0 before calling a libm
 * function, and invoke this macro after, passing the function result.
 * Caution:
 *    This isn't reliable.  See Py_OVERFLOWED comments.
 *    X is evaluated more than once.
 */
#define Py_SET_ERANGE_IF_OVERFLOW(X) \
	do { \
		if (errno == 0 && ((X) == Py_HUGE_VAL ||  \
				   (X) == -Py_HUGE_VAL))  \
			errno = ERANGE; \
	} while(0)

/**************************************************************************
Prototypes that are missing from the standard include files on some systems
(and possibly only some versions of such systems.)

Please be conservative with adding new ones, document them and enclose them
in platform-specific #ifdefs.
**************************************************************************/

#ifdef SOLARIS
/* Unchecked */
extern int gethostname(char *, int);
#endif

#ifdef __BEOS__
/* Unchecked */
/* It's in the libs, but not the headers... - [cjh] */
int shutdown( int, int );
#endif

#ifdef HAVE__GETPTY
#include <sys/types.h>		/* we need to import mode_t */
extern char * _getpty(int *, int, mode_t, int);
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY)
#if !defined(HAVE_PTY_H) && !defined(HAVE_LIBUTIL_H)
/* BSDI does not supply a prototype for the 'openpty' and 'forkpty'
   functions, even though they are included in libutil. */
#include <termios.h>
extern int openpty(int *, int *, char *, struct termios *, struct winsize *);
extern int forkpty(int *, char *, struct termios *, struct winsize *);
#endif /* !defined(HAVE_PTY_H) && !defined(HAVE_LIBUTIL_H) */
#endif /* defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) */


/* These are pulled from various places. It isn't obvious on what platforms
   they are necessary, nor what the exact prototype should look like (which
   is likely to vary between platforms!) If you find you need one of these
   declarations, please move them to a platform-specific block and include
   proper prototypes. */
#if 0

/* From Modules/resource.c */
extern int getrusage();
extern int getpagesize();

/* From Python/sysmodule.c and Modules/posixmodule.c */
extern int fclose(FILE *);

/* From Modules/posixmodule.c */
extern int fdatasync(int);
/* XXX These are supposedly for SunOS4.1.3 but "shouldn't hurt elsewhere" */
extern int rename(const char *, const char *);
extern int pclose(FILE *);
extern int lstat(const char *, struct stat *);
extern int symlink(const char *, const char *);
extern int fsync(int fd);

#endif /* 0 */


/************************
 * WRAPPER FOR <math.h> *
 ************************/

#ifndef HAVE_HYPOT
extern double hypot(double, double);
#endif


/************************************
 * MALLOC COMPATIBILITY FOR pymem.h *
 ************************************/

#ifndef DL_IMPORT       /* declarations for DLL import */
#define DL_IMPORT(RTYPE) RTYPE
#endif

#ifdef MALLOC_ZERO_RETURNS_NULL
/* Allocate an extra byte if the platform malloc(0) returns NULL.
   Caution:  this bears no relation to whether realloc(p, 0) returns NULL
   when p != NULL.  Even on platforms where malloc(0) does not return NULL,
   realloc(p, 0) may act like free(p) and return NULL.  Examples include
   Windows, and Python's own obmalloc.c (as of 2-Mar-2002).  For whatever
   reason, our docs promise that PyMem_Realloc(p, 0) won't act like
   free(p) or return NULL, so realloc() calls may have to be hacked
   too, but MALLOC_ZERO_RETURNS_NULL's state is irrelevant to realloc (it
   needs a different hack).
*/
#define _PyMem_EXTRA 1
#else
#define _PyMem_EXTRA 0
#endif


/* If the fd manipulation macros aren't defined,
   here is a set that should do the job */

#if 0 /* disabled and probably obsolete */

#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

#ifndef FD_SET

typedef long fd_mask;

#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif /* howmany */

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset((char *)(p), '\0', sizeof(*(p)))

#endif /* FD_SET */

#endif /* fd manipulation macros */


/* limits.h constants that may be missing */

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef LONG_MAX
#if SIZEOF_LONG == 4
#define LONG_MAX 0X7FFFFFFFL
#elif SIZEOF_LONG == 8
#define LONG_MAX 0X7FFFFFFFFFFFFFFFL
#else
#error "could not set LONG_MAX in pyport.h"
#endif
#endif

#ifndef LONG_MIN
#define LONG_MIN (-LONG_MAX-1)
#endif

#ifndef LONG_BIT
#define LONG_BIT (8 * SIZEOF_LONG)
#endif

#if LONG_BIT != 8 * SIZEOF_LONG
/* 04-Oct-2000 LONG_BIT is apparently (mis)defined as 64 on some recent
 * 32-bit platforms using gcc.  We try to catch that here at compile-time
 * rather than waiting for integer multiplication to trigger bogus
 * overflows.
 */
#error "LONG_BIT definition appears wrong for platform (bad gcc/glibc config?)."
#endif

/*
 * Rename some functions for the Borland compiler
 */
#ifdef __BORLANDC__
#  include <io.h>
#  define _chsize chsize
#  define _setmode setmode
#endif

#ifdef __cplusplus
}
#endif

/*
 * Hide GCC attributes from compilers that don't support them.
 */
#if (!defined(__GNUC__) || __GNUC__ < 2 || \
     (__GNUC__ == 2 && __GNUC_MINOR__ < 7) || \
     defined(NEXT) ) && \
    !defined(RISCOS)
#define __attribute__(__x)
#endif

#endif /* Py_PYPORT_H */
