/***********************************************************
Copyright (c) 2000, BeOpen.com.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

#ifndef Py_PYPORT_H
#define Py_PYPORT_H

#include "config.h" /* include for defines */

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
**************************************************************************/


#define ANY void /* For API compatibility only. Obsolete, do not use. */

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

/* On the 68K Mac, when using CFM (Code Fragment Manager),
   <math.h> requires special treatment -- we need to surround it with
   #pragma lib_export off / on...
   This is because MathLib.o is a static library, and exporting its
   symbols doesn't quite work...
   XXX Not sure now...  Seems to be something else going on as well... */

#ifndef HAVE_HYPOT
extern double hypot(double, double);
#ifdef MWERKS_BEFORE_PRO4
#define hypot we_dont_want_faulty_hypot_decl
#endif
#endif

#ifndef HAVE_HYPOT
#ifdef __MWERKS__
#undef hypot
#endif
#endif

#if defined(USE_MSL) && defined(__MC68K__)
/* CodeWarrior MSL 2.1.1 has weird define overrides that don't work
** when you take the address of math functions. If I interpret the
** ANSI C standard correctly this is illegal, but I haven't been able
** to convince the MetroWerks folks of this...
*/
#undef acos
#undef asin
#undef atan
#undef atan2
#undef ceil
#undef cos
#undef cosh
#undef exp
#undef fabs
#undef floor
#undef fmod
#undef log
#undef log10
#undef pow
#undef sin
#undef sinh
#undef sqrt
#undef tan
#undef tanh
#define acos acosd
#define asin asind
#define atan atand
#define atan2 atan2d
#define ceil ceild
#define cos cosd
#define cosh coshd
#define exp expd
#define fabs fabsd
#define floor floord
#define fmod fmodd
#define log logd
#define log10 log10d
#define pow powd
#define sin sind
#define sinh sinhd
#define sqrt sqrtd
#define tan tand
#define tanh tanhd
#endif 


/************************************
 * MALLOC COMPATIBILITY FOR pymem.h *
 ************************************/

#ifndef DL_IMPORT       /* declarations for DLL import */
#define DL_IMPORT(RTYPE) RTYPE
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifdef MALLOC_ZERO_RETURNS_NULL
/* XXX Always allocate one extra byte, since some malloc's return NULL
   XXX for malloc(0) or realloc(p, 0). */
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


#ifdef __cplusplus
}
#endif

#endif /* Py_PYPORT_H */
