/* Leave this blank line here -- autoheader needs it! */


/* Define for AIX if your compiler is a genuine IBM xlC/xlC_r
   and you want support for AIX C++ shared extension modules. */
#undef AIX_GENUINE_CPLUSPLUS

/* Define if your <unistd.h> contains bad prototypes for exec*()
   (as it does on SGI IRIX 4.x) */
#undef BAD_EXEC_PROTOTYPES

/* Define if your compiler botches static forward declarations
   (as it does on SCI ODT 3.0) */
#undef BAD_STATIC_FORWARD

/* Define this if you have BeOS threads */
#undef BEOS_THREADS

/* Define if you have the Mach cthreads package */
#undef C_THREADS

/* Define to `long' if <time.h> doesn't define.  */
#undef clock_t

/* Defined on Solaris to see additional function prototypes. */
#undef __EXTENSIONS__

/* Define if getpgrp() must be called as getpgrp(0). */
#undef GETPGRP_HAVE_ARG

/* Define if gettimeofday() does not have second (timezone) argument
   This is the case on Motorola V4 (R40V4.2) */
#undef GETTIMEOFDAY_NO_TZ

/* Define this if your time.h defines altzone */
#undef HAVE_ALTZONE

/* Defined when any dynamic module loading is enabled */
#undef HAVE_DYNAMIC_LOADING

/* Define this if you have flockfile(), getc_unlocked(), and funlockfile() */
#undef HAVE_GETC_UNLOCKED

/* Define this if you have gethostbyname() */
#undef HAVE_GETHOSTBYNAME

/* Define this if you have some version of gethostbyname_r() */
#undef HAVE_GETHOSTBYNAME_R

/* Define this if you have the 3-arg version of gethostbyname_r() */
#undef HAVE_GETHOSTBYNAME_R_3_ARG

/* Define this if you have the 5-arg version of gethostbyname_r() */
#undef HAVE_GETHOSTBYNAME_R_5_ARG

/* Define this if you have the 6-arg version of gethostbyname_r() */
#undef HAVE_GETHOSTBYNAME_R_6_ARG

/* Defined to enable large file support when an off_t is bigger than a long
   and long long is available and at least as big as an off_t. You may need
   to add some flags for configuration and compilation to enable this mode.
   E.g, for Solaris 2.7:
   CFLAGS="-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64" OPT="-O2 $CFLAGS" \
	configure
*/
#undef HAVE_LARGEFILE_SUPPORT

/* Define this if you have the type long long */
#undef HAVE_LONG_LONG

/* Define if your compiler supports function prototypes */
#undef HAVE_PROTOTYPES

/* Define if you have GNU PTH threads */
#undef HAVE_PTH

/* Define if you have readline 4.2 */
#undef HAVE_RL_COMPLETION_MATCHES

/* Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h> */
#undef HAVE_STDARG_PROTOTYPES

/* Define if you have termios available */
#undef HAVE_TERMIOS_H

/* Define this if you have the type uintptr_t */
#undef HAVE_UINTPTR_T

/* Define if you have a useable wchar_t type defined in wchar.h; useable
   means wchar_t must be 16-bit unsigned type. (see
   Include/unicodeobject.h). */
#undef HAVE_USABLE_WCHAR_T

/* Define if the compiler provides a wchar.h header file. */
#undef HAVE_WCHAR_H

/* Define if malloc(0) returns a NULL pointer */
#undef MALLOC_ZERO_RETURNS_NULL

/* Define if you have POSIX threads */
#undef _POSIX_THREADS

/* Define if you want to build an interpreter with many run-time checks  */
#undef Py_DEBUG

/* Define to force use of thread-safe errno, h_errno, and other functions */
#undef _REENTRANT

/* Define if setpgrp() must be called as setpgrp(0, 0). */
#undef SETPGRP_HAVE_ARG

/* Define to empty if the keyword does not work.  */
#undef signed

/* Define if i>>j for signed int i does not extend the sign bit
   when i < 0
*/
#undef SIGNED_RIGHT_SHIFT_ZERO_FILLS

/* The number of bytes in an off_t. */
#undef SIZEOF_OFF_T

/* The number of bytes in a time_t. */
#undef SIZEOF_TIME_T

/* The number of bytes in a pthread_t. */
#undef SIZEOF_PTHREAD_T

/* sizeof(void *) */
#undef SIZEOF_VOID_P

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef socklen_t

/* Define for SOLARIS 2.x */
#undef SOLARIS

/* Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0). */
#undef SYS_SELECT_WITH_SYS_TIME

/* Define if a va_list is an array of some kind */
#undef VA_LIST_IS_ARRAY

/* Define to empty if the keyword does not work.  */
#undef volatile

/* Define if you want SIGFPE handled (see Include/pyfpe.h). */
#undef WANT_SIGFPE_HANDLER

/* Define if you want wctype.h functions to be used instead of the
   one supplied by Python itself. (see Include/unicodectype.h). */
#undef WANT_WCTYPE_FUNCTIONS

/* Define if you want to compile in cycle garbage collection */
#undef WITH_CYCLE_GC

/* Define if you want to emulate SGI (IRIX 4) dynamic linking.
   This is rumoured to work on VAX (Ultrix), Sun3 (SunOS 3.4),
   Sequent Symmetry (Dynix), and Atari ST.
   This requires the "dl-dld" library,
   ftp://ftp.cwi.nl/pub/dynload/dl-dld-1.1.tar.Z,
   as well as the "GNU dld" library,
   ftp://ftp.cwi.nl/pub/dynload/dld-3.2.3.tar.Z.
   Don't bother on SunOS 4 or 5, they already have dynamic linking using
   shared libraries */ 
#undef WITH_DL_DLD

/* Define if you want to use the new-style (Openstep, Rhapsody, MacOS)
   dynamic linker (dyld) instead of the old-style (NextStep) dynamic
   linker (rld). Dyld is necessary to support frameworks. */
#undef WITH_DYLD

/* Define if you want to use BSD db. */
#undef WITH_LIBDB

/* Define if you want to use ndbm. */
#undef WITH_LIBNDBM

/* Define if you want to compile in Python-specific mallocs */
#undef WITH_PYMALLOC

/* Define if you want to produce an OpenStep/Rhapsody framework
   (shared library plus accessory files). */
#undef WITH_NEXT_FRAMEWORK

/* Define if you want to use SGI (IRIX 4) dynamic linking.
   This requires the "dl" library by Jack Jansen,
   ftp://ftp.cwi.nl/pub/dynload/dl-1.6.tar.Z.
   Don't bother on IRIX 5, it already has dynamic linking using SunOS
   style shared libraries */ 
#undef WITH_SGI_DL

/* Define if you want to compile in rudimentary thread support */
#undef WITH_THREAD


/* Leave that blank line there-- autoheader needs it! */

@BOTTOM@

#ifdef __CYGWIN__
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#else
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#endif
