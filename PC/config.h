#ifndef Py_CONFIG_H
#define Py_CONFIG_H

/* config.h.  NOT Generated automatically by configure.

This is a manually maintained version used for the Watcom,
Borland and and Microsoft Visual C++ compilers.  It is a
standard part of the Python distribution.

WINDOWS DEFINES:
The code specific to Windows should be wrapped around one of
the following #defines

MS_WIN64 - Code specific to the MS Win64 API
MS_WIN32 - Code specific to the MS Win32 (and Win64) API
MS_WIN16 - Code specific to the old 16 bit Windows API.
MS_WINDOWS - Code specific to Windows, but all versions.
MS_COREDLL - Code if the Python core is built as a DLL.

Note that the old defines "NT" and "WIN32" are still supported, but
will soon be dropped.

Also note that neither "_M_IX86" or "_MSC_VER" should be used for
any purpose other than "Windows Intel x86 specific" and "Microsoft
compiler specific".  Therefore, these should be very rare.

*/


/*
 Some systems require special declarations for data items imported
 or exported from dynamic link libraries.  Note that the definition
 of DL_IMPORT covers both cases.  Define USE_DL_IMPORT for the client
 of a DLL.  Define USE_DL_EXPORT when making a DLL.
*/

#include <io.h>
#define HAVE_LIMITS_H
#define HAVE_HYPOT
#define DONT_HAVE_SIG_ALARM
#define DONT_HAVE_SIG_PAUSE
#define LONG_BIT	32
#define PREFIX ""
#define EXEC_PREFIX ""

/* Microsoft C defines _MSC_VER */
#ifdef _MSC_VER

/* MSVC defines _WINxx to differentiate the windows platform types

   Note that for compatibility reasons _WIN32 is defined on Win32
   *and* on Win64. For the same reasons, in Python, MS_WIN32 is
   defined on Win32 *and* Win64. Win32 only code must therefore be
   guarded as follows:
   	#if defined(MS_WIN32) && !defined(MS_WIN64)
*/
#ifdef _WIN64
#define MS_WIN64
#endif
#ifdef _WIN32
#define NT	/* NT is obsolete - please use MS_WIN32 instead */
#define MS_WIN32
#endif
#ifdef _WIN16
#define MS_WIN16
#endif
#define MS_WINDOWS

/* set the COMPILER */
#ifdef MS_WIN64
#ifdef _M_IX86
#define COMPILER "[MSC 64 bit (Intel)]"
#elif defined(_M_ALPHA)
#define COMPILER "[MSC 64 bit (Alpha)]"
#else
#define COMPILER "[MSC 64 bit (Unknown)]"
#endif
#endif /* MS_WIN64 */

#if defined(MS_WIN32) && !defined(MS_WIN64)
#ifdef _M_IX86
#define COMPILER "[MSC 32 bit (Intel)]"
#elif defined(_M_ALPHA)
#define COMPILER "[MSC 32 bit (Alpha)]"
#else
#define COMPILER "[MSC (Unknown)]"
#endif
#endif /* MS_WIN32 && !MS_WIN64 */

#endif /* _MSC_VER */

#if defined(_MSC_VER) && _MSC_VER > 850
/* Start of defines for MS_WIN32 using VC++ 2.0 and up */

/* For NT the Python core is in a DLL by default.  Test the
standard macro MS_COREDLL to find out.  If you have an exception
you must define MS_NO_COREDLL (do not test this macro) */
#ifndef MS_NO_COREDLL
#define MS_COREDLL	/* Python core is in a DLL */
#ifndef USE_DL_EXPORT
#define USE_DL_IMPORT
#endif /* !USE_DL_EXPORT */
#endif /* !MS_NO_COREDLL */

#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
typedef int pid_t;
#define WORD_BIT 32
#pragma warning(disable:4113)
#define hypot _hypot
#include <stdio.h>
#define HAVE_CLOCK
#define HAVE_STRFTIME
#define HAVE_STRERROR
#define NT_THREADS
#define WITH_THREAD
#ifndef NETSCAPE_PI
#define USE_SOCKET
#endif
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#endif
#ifdef USE_DL_EXPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif

#define HAVE_LONG_LONG 1
#define LONG_LONG __int64
#endif /* _MSC_VER && > 850 */

#if defined(_MSC_VER) && _MSC_VER <= 850 /* presume this implies Win16 */
/* Start of defines for 16-bit Windows using VC++ 1.5 */
#define COMPILER "[MSC 16-bit]"
#define PYTHONPATH ".;.\\lib;.\\lib\\plat-win;.\\lib\\dos-8x3"
#define IMPORT_8x3_NAMES
typedef int pid_t;
#define WORD_BIT 16
#define SIZEOF_INT 2
#define SIZEOF_LONG 4
#define SIZEOF_VOID_P 4
#pragma warning(disable:4113)
#define memcpy memmove	/* memcpy dangerous pointer wrap in Win 3.1 */
#define hypot _hypot
#define SIGINT	2
#include <stdio.h>
/* Windows 3.1 will not tolerate any console io in a dll */
#ifdef _USRDLL
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif
#define stdin	((FILE *)0)
#define stdout	((FILE *)1)
#define stderr	((FILE *)2)
#define fflush	Py_fflush
int Py_fflush(FILE *);
#define fgets	Py_fgets
char *Py_fgets(char *, int, FILE *);
#define fileno	Py_fileno
int Py_fileno(FILE *);
#define fprintf	Py_fprintf
int Py_fprintf(FILE *, const char *, ...);
#define	printf	Py_printf
int Py_printf(const char *, ...);
#define sscanf	Py_sscanf
int Py_sscanf(const char *, const char *, ...);
clock_t clock();
void _exit(int);
void exit(int);
int sscanf(const char *, const char *, ...);
#ifdef __cplusplus
}
#endif
#endif /* _USRDLL */
#ifndef NETSCAPE_PI
/* use sockets, but not in a Netscape dll */
#define USE_SOCKET
#endif
#endif /* MS_WIN16 */

/* The Watcom compiler defines __WATCOMC__ */
#ifdef __WATCOMC__
#define COMPILER "[Watcom]"
#define PYTHONPATH ".;.\\lib;.\\lib\\plat-win;.\\lib\\dos-8x3"
#define IMPORT_8x3_NAMES
#include <ctype.h>
#include <direct.h>
typedef int mode_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
#if defined(__NT__)
#define NT	/* NT is obsolete - please use MS_WIN32 instead */
#define MS_WIN32
#define MS_WINDOWS
#define NT_THREADS
#define USE_SOCKET
#define WITH_THREAD
#elif defined(__WINDOWS__)
#define MS_WIN16
#define MS_WINDOWS
#endif
#ifdef M_I386
#define WORD_BIT 32
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_VOID_P 4
#else
#define WORD_BIT 16
#define SIZEOF_INT 2
#define SIZEOF_LONG 4
#define SIZEOF_VOID_P 4
#endif
#define VA_LIST_IS_ARRAY
#define HAVE_CLOCK
#define HAVE_STRFTIME
#ifdef USE_DL_EXPORT
#define DL_IMPORT(RTYPE) RTYPE __export
#endif
#endif /* __WATCOMC__ */

/* The Borland compiler defines __BORLANDC__ */
/* XXX These defines are likely incomplete, but should be easy to fix. */
#ifdef __BORLANDC__
#define COMPILER "[Borland]"
#define HAVE_CLOCK
#define HAVE_STRFTIME

#ifdef _WIN32

/* tested with BCC 5.5 (__BORLANDC__ >= 0x0550)
 */
#define NT	/* NT is obsolete - please use MS_WIN32 instead */
#define MS_WIN32
#define MS_WINDOWS

/* For NT the Python core is in a DLL by default.  Test the
standard macro MS_COREDLL to find out.  If you have an exception
you must define MS_NO_COREDLL (do not test this macro) */
#ifndef MS_NO_COREDLL
#define MS_COREDLL	/* Python core is in a DLL */
#ifndef USE_DL_EXPORT
#define USE_DL_IMPORT
#endif /* !USE_DL_EXPORT */
#endif /* !MS_NO_COREDLL */

#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
typedef int pid_t;
#define WORD_BIT 32
#include <stdio.h>
#define HAVE_STRERROR
#define NT_THREADS
#define WITH_THREAD
#ifndef NETSCAPE_PI
#define USE_SOCKET
#endif
/* BCC55 seems to understand __declspec(dllimport), it is used in its
   own header files (winnt.h, ...) */
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#endif
#ifdef USE_DL_EXPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif

#define HAVE_LONG_LONG 1
#define LONG_LONG __int64

#else /* !_WIN32 */
/* XXX These defines are likely incomplete, but should be easy to fix. */

#define PYTHONPATH ".;.\\lib;.\\lib\\plat-win;.\\lib\\dos-8x3"
#define IMPORT_8x3_NAMES
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE)  RTYPE __import
#endif

#endif /* !_WIN32 */

#endif /* BORLANDC */

/* egcs/gnu-win32 defines __GNUC__ and _WIN32 */
#if defined(__GNUC__) && defined(_WIN32)
/* XXX These defines are likely incomplete, but should be easy to fix. 
   They should be complete enough to build extension modules. */
/* Suggested by Rene Liebscher <R.Liebscher@gmx.de> to avoid a GCC 2.91.*
   bug that requires structure imports.  More recent versions of the
   compiler don't exhibit this bug.
*/
#if (__GNUC__==2) && (__GNUC_MINOR__<=91)
#warning "Please use an up-to-date version of gcc! (>2.91 recommended)"
#endif

#define NT	/* NT is obsolete - please use MS_WIN32 instead */
#define MS_WIN32
#define MS_WINDOWS

/* For NT the Python core is in a DLL by default.  Test the
standard macro MS_COREDLL to find out.  If you have an exception
you must define MS_NO_COREDLL (do not test this macro) */
#ifndef MS_NO_COREDLL
#define MS_COREDLL	/* Python core is in a DLL */
#ifndef USE_DL_EXPORT
#define USE_DL_IMPORT
#endif /* !USE_DL_EXPORT */
#endif /* !MS_NO_COREDLL */

#define COMPILER "[gcc]"
#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
#define WORD_BIT 32
#define hypot _hypot
#include <stdio.h>
#define HAVE_CLOCK
#define HAVE_STRFTIME
#define HAVE_STRERROR
#define NT_THREADS
#define WITH_THREAD
#ifndef NETSCAPE_PI
#define USE_SOCKET
#endif
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#endif
#ifdef USE_DL_EXPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif

#define HAVE_LONG_LONG 1
#define LONG_LONG long long 
#endif /* GNUC */

/* lcc-win32 defines __LCC__ */

#if defined(__LCC__)
/* XXX These defines are likely incomplete, but should be easy to fix. 
   They should be complete enough to build extension modules. */

#define NT	/* NT is obsolete - please use MS_WIN32 instead */
#define MS_WIN32
#define MS_WINDOWS

/* For NT the Python core is in a DLL by default.  Test the
standard macro MS_COREDLL to find out.  If you have an exception
you must define MS_NO_COREDLL (do not test this macro) */
#ifndef MS_NO_COREDLL
#define MS_COREDLL	/* Python core is in a DLL */
#ifndef USE_DL_EXPORT
#define USE_DL_IMPORT
#endif /* !USE_DL_EXPORT */
#endif /* !MS_NO_COREDLL */

#define COMPILER "[lcc-win32]"
#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
typedef int pid_t;
#define WORD_BIT 32
#include <stdio.h>
#define HAVE_CLOCK
#define HAVE_STRFTIME
#define HAVE_STRERROR
#define NT_THREADS
#define WITH_THREAD
#ifndef NETSCAPE_PI
#define USE_SOCKET
#endif
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#endif
#ifdef USE_DL_EXPORT
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif

#define HAVE_LONG_LONG 1
#define LONG_LONG __int64
#endif /* LCC */

/* End of compilers - finish up */

/* define some ANSI types that are not defined in earlier Win headers */
#if _MSC_VER >= 1200 /* This file only exists in VC 6.0 or higher */
#include <basetsd.h>
#endif
#if defined(MS_WINDOWS) && !defined(MS_WIN64)
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif

#if defined(MS_WIN64)
/* maintain "win32" sys.platform for backward compatibility of Python code,
   the Win64 API should be close enough to the Win32 API to make this
   preferable */
#	define PLATFORM "win32"
#	define SIZEOF_VOID_P 8
#	define SIZEOF_TIME_T 8
#	define SIZEOF_OFF_T 4
#	define SIZEOF_FPOS_T 8
#	define SIZEOF_HKEY 8
/* configure.in defines HAVE_LARGEFILE_SUPPORT iff HAVE_LONG_LONG,
   sizeof(off_t) > sizeof(long), and sizeof(LONG_LONG) >= sizeof(off_t).
   On Win64 the second condition is not true, but if fpos_t replaces off_t
   then this is true. The uses of HAVE_LARGEFILE_SUPPORT imply that Win64
   should define this. */
#	define HAVE_LARGEFILE_SUPPORT
#elif defined(MS_WIN32)
#	define PLATFORM "win32"
#	ifdef _M_ALPHA
#		define SIZEOF_VOID_P 8
#		define SIZEOF_TIME_T 8
#	else
#		define SIZEOF_VOID_P 4
#		define SIZEOF_TIME_T 4
#		define SIZEOF_OFF_T 4
#		define SIZEOF_FPOS_T 8
#		define SIZEOF_HKEY 4
#	endif
#elif defined(MS_WIN16)
#	define PLATFORM "win16"
#else
#	define PLATFORM "dos"
#endif


#ifdef MS_WIN32

#if !defined(USE_DL_EXPORT) && defined(_MSC_VER)
/* So nobody using MSVC needs to specify the .lib in their Makefile any
   more (other compilers will still need to do so, but that's taken care
   of by the Distutils, so it's not a problem). */
#ifdef _DEBUG
#pragma comment(lib,"python20_d.lib")
#else
#pragma comment(lib,"python20.lib")
#endif
#endif /* USE_DL_EXPORT */

#ifdef _DEBUG
#define Py_DEBUG
#endif

#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8

/* EXPERIMENTAL FEATURE: When CHECK_IMPORT_CASE is defined, check case of
   imported modules against case of file; this causes "import String" to fail
   with a NameError exception when it finds "string.py".  Normally, you set
   the environment variable PYTHONCASEOK (to anything) to disable this
   feature; to permanently disable it, #undef it here.  This only works on
   case-preserving filesystems; otherwise you definitely want it off. */
#define CHECK_IMPORT_CASE
#endif

/* Fairly standard from here! */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define to empty if the keyword does not work.  */
/* #define const  */

/* Define if you have dirent.h.  */
/* #define DIRENT 1 */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
/* #undef GETGROUPS_T */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if your struct tm has tm_zone.  */
/* #undef HAVE_TM_ZONE */

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#define HAVE_TZNAME

/* Define if on MINIX.  */
/* #undef _MINIX */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define if you don't have dirent.h, but have ndir.h.  */
/* #undef NDIR */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
/* #undef _POSIX_1_SOURCE */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
#define socklen_t int

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you don't have dirent.h, but have sys/dir.h.  */
/* #undef SYSDIR */

/* Define if you don't have dirent.h, but have sys/ndir.h.  */
/* #undef SYSNDIR */

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
/* #undef TIME_WITH_SYS_TIME */

/* Define if your <sys/time.h> declares struct tm.  */
/* #define TM_IN_SYS_TIME 1 */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define if the closedir function returns void instead of int.  */
/* #undef VOID_CLOSEDIR */

/* Define if your <unistd.h> contains bad prototypes for exec*()
   (as it does on SGI IRIX 4.x) */
/* #undef BAD_EXEC_PROTOTYPES */

/* Define if your compiler botches static forward declarations
   (as it does on SCI ODT 3.0) */
#define BAD_STATIC_FORWARD 1

/* Define if getpgrp() must be called as getpgrp(0)
   and (consequently) setpgrp() as setpgrp(0, 0). */
/* #undef GETPGRP_HAVE_ARGS */

/* Define this if your time.h defines altzone */
/* #define HAVE_ALTZONE */

/* Define if you have the putenv function.  */
#ifdef MS_WIN32
/* Does this exist on Win16? */
#define HAVE_PUTENV
#endif

/* Define if your compiler supports function prototypes */
#define HAVE_PROTOTYPES

/* Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0). */
/* #undef SYS_SELECT_WITH_SYS_TIME */

/* Define if you want to use SGI (IRIX 4) dynamic linking.
   This requires the "dl" library by Jack Jansen,
   ftp://ftp.cwi.nl/pub/dynload/dl-1.6.tar.Z.
   Don't bother on IRIX 5, it already has dynamic linking using SunOS
   style shared libraries */
/* #undef WITH_SGI_DL */

/* Define if you want to emulate SGI (IRIX 4) dynamic linking.
   This is rumoured to work on VAX (Ultrix), Sun3 (SunOS 3.4),
   Sequent Symmetry (Dynix), and Atari ST.
   This requires the "dl-dld" library,
   ftp://ftp.cwi.nl/pub/dynload/dl-dld-1.1.tar.Z,
   as well as the "GNU dld" library,
   ftp://ftp.cwi.nl/pub/dynload/dld-3.2.3.tar.Z.
   Don't bother on SunOS 4 or 5, they already have dynamic linking using
   shared libraries */
/* #undef WITH_DL_DLD */

/* Define if you want to compile in rudimentary thread support */
/* #undef WITH_THREAD */

/* Define if you want to use the GNU readline library */
/* #define WITH_READLINE 1 */

/* Define if you want cycle garbage collection */
#define WITH_CYCLE_GC 1

/* Define if you have clock.  */
/* #define HAVE_CLOCK */

/* Define when any dynamic module loading is enabled */
#define HAVE_DYNAMIC_LOADING

/* Define if you have ftime.  */
#define HAVE_FTIME

/* Define if you have getpeername.  */
#define HAVE_GETPEERNAME

/* Define if you have getpgrp.  */
/* #undef HAVE_GETPGRP */

/* Define if you have getpid.  */
#define HAVE_GETPID

/* Define if you have gettimeofday.  */
/* #undef HAVE_GETTIMEOFDAY */

/* Define if you have getwd.  */
/* #undef HAVE_GETWD */

/* Define if you have lstat.  */
/* #undef HAVE_LSTAT */

/* Define if you have the mktime function.  */
#define HAVE_MKTIME

/* Define if you have nice.  */
/* #undef HAVE_NICE */

/* Define if you have readlink.  */
/* #undef HAVE_READLINK */

/* Define if you have select.  */
/* #undef HAVE_SELECT */

/* Define if you have setpgid.  */
/* #undef HAVE_SETPGID */

/* Define if you have setpgrp.  */
/* #undef HAVE_SETPGRP */

/* Define if you have setsid.  */
/* #undef HAVE_SETSID */

/* Define if you have setvbuf.  */
#define HAVE_SETVBUF

/* Define if you have siginterrupt.  */
/* #undef HAVE_SIGINTERRUPT */

/* Define if you have symlink.  */
/* #undef HAVE_SYMLINK */

/* Define if you have tcgetpgrp.  */
/* #undef HAVE_TCGETPGRP */

/* Define if you have tcsetpgrp.  */
/* #undef HAVE_TCSETPGRP */

/* Define if you have times.  */
/* #undef HAVE_TIMES */

/* Define if you have uname.  */
/* #undef HAVE_UNAME */

/* Define if you have waitpid.  */
/* #undef HAVE_WAITPID */

/* Define if you have the <dlfcn.h> header file.  */
/* #undef HAVE_DLFCN_H */

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdarg.h> prototypes.  */
#define HAVE_STDARG_PROTOTYPES

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <sys/audioio.h> header file.  */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define if you have the <sys/param.h> header file.  */
/* #define HAVE_SYS_PARAM_H 1 */

/* Define if you have the <sys/select.h> header file.  */
/* #define HAVE_SYS_SELECT_H 1 */

/* Define if you have the <sys/time.h> header file.  */
/* #define HAVE_SYS_TIME_H 1 */

/* Define if you have the <sys/times.h> header file.  */
/* #define HAVE_SYS_TIMES_H 1 */

/* Define if you have the <sys/un.h> header file.  */
/* #define HAVE_SYS_UN_H 1 */

/* Define if you have the <sys/utime.h> header file.  */
#define HAVE_SYS_UTIME_H 1

/* Define if you have the <sys/utsname.h> header file.  */
/* #define HAVE_SYS_UTSNAME_H 1 */

/* Define if you have the <thread.h> header file.  */
/* #undef HAVE_THREAD_H */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Define if you have the <utime.h> header file.  */
/* #define HAVE_UTIME_H 1 */

/* Define if you have the dl library (-ldl).  */
/* #undef HAVE_LIBDL */

/* Define if you have the mpc library (-lmpc).  */
/* #undef HAVE_LIBMPC */

/* Define if you have the nsl library (-lnsl).  */
#define HAVE_LIBNSL 1

/* Define if you have the seq library (-lseq).  */
/* #undef HAVE_LIBSEQ */

/* Define if you have the socket library (-lsocket).  */
#define HAVE_LIBSOCKET 1

/* Define if you have the sun library (-lsun).  */
/* #undef HAVE_LIBSUN */

/* Define if you have the termcap library (-ltermcap).  */
/* #undef HAVE_LIBTERMCAP */

/* Define if you have the termlib library (-ltermlib).  */
/* #undef HAVE_LIBTERMLIB */

/* Define if you have the thread library (-lthread).  */
/* #undef HAVE_LIBTHREAD */
#endif /* !Py_CONFIG_H */
