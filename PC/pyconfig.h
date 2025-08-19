#ifndef Py_CONFIG_H
#define Py_CONFIG_H

/* pyconfig.h.  NOT Generated automatically by configure.

This is a manually maintained version used for the Watcom,
Borland and Microsoft Visual C++ compilers.  It is a
standard part of the Python distribution.

WINDOWS DEFINES:
The code specific to Windows should be wrapped around one of
the following #defines

MS_WIN64 - Code specific to the MS Win64 API
MS_WIN32 - Code specific to the MS Win32 (and Win64) API (obsolete, this covers all supported APIs)
MS_WINDOWS - Code specific to Windows, but all versions.
Py_ENABLE_SHARED - Code if the Python core is built as a DLL.

Also note that neither "_M_IX86" or "_MSC_VER" should be used for
any purpose other than "Windows Intel x86 specific" and "Microsoft
compiler specific".  Therefore, these should be very rare.


NOTE: The following symbols are deprecated:
NT, USE_DL_EXPORT, USE_DL_IMPORT, DL_EXPORT, DL_IMPORT
MS_CORE_DLL.

WIN32 is still required for the locale module.

*/

/* Deprecated USE_DL_EXPORT macro - please use Py_BUILD_CORE */
#ifdef USE_DL_EXPORT
#       define Py_BUILD_CORE
#endif /* USE_DL_EXPORT */

/* Visual Studio 2005 introduces deprecation warnings for
   "insecure" and POSIX functions. The insecure functions should
   be replaced by *_s versions (according to Microsoft); the
   POSIX functions by _* versions (which, according to Microsoft,
   would be ISO C conforming). Neither renaming is feasible, so
   we just silence the warnings. */

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE 1
#endif

#define HAVE_IO_H
#define HAVE_SYS_UTIME_H
#define HAVE_TEMPNAM
#define HAVE_TMPFILE
#define HAVE_TMPNAM
#define HAVE_CLOCK
#define HAVE_STRERROR

#include <io.h>

#define HAVE_STRFTIME
#define DONT_HAVE_SIG_ALARM
#define DONT_HAVE_SIG_PAUSE
#define LONG_BIT        32
#define WORD_BIT 32

#define MS_WIN32 /* only support win32 and greater. */
#define MS_WINDOWS
#define NT_THREADS
#define WITH_THREAD
#ifndef NETSCAPE_PI
#define USE_SOCKET
#endif

#if defined(Py_BUILD_CORE) || defined(Py_BUILD_CORE_BUILTIN) || defined(Py_BUILD_CORE_MODULE)
#include <winapifamily.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define MS_WINDOWS_DESKTOP
#endif
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define MS_WINDOWS_APP
#endif
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_SYSTEM)
#define MS_WINDOWS_SYSTEM
#endif
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_GAMES)
#define MS_WINDOWS_GAMES
#endif

/* Define to 1 if you support windows console io */
#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM)
#define HAVE_WINDOWS_CONSOLE_IO 1
#endif
#endif /* Py_BUILD_CORE || Py_BUILD_CORE_BUILTIN || Py_BUILD_CORE_MODULE */

/* _DEBUG implies Py_DEBUG */
#ifdef _DEBUG
#  define Py_DEBUG 1
#endif

/* Define to 1 when compiling for experimental free-threaded builds */
#ifdef Py_GIL_DISABLED
/* We undefine if it was set to zero because all later checks are #ifdef.
 * Note that non-Windows builds do not do this, and so every effort should
 * be made to avoid defining the variable at all when not desired. However,
 * sysconfig.get_config_var always returns a 1 or a 0, and so it seems likely
 * that a build backend will define it with the value.
 */
#if Py_GIL_DISABLED == 0
#undef Py_GIL_DISABLED
#endif
#endif

/* Compiler specific defines */

/* ------------------------------------------------------------------------*/
/* Microsoft C defines _MSC_VER, as does clang-cl.exe */
#ifdef _MSC_VER

/* We want COMPILER to expand to a string containing _MSC_VER's *value*.
 * This is horridly tricky, because the stringization operator only works
 * on macro arguments, and doesn't evaluate macros passed *as* arguments.
 */
#define _Py_PASTE_VERSION(SUFFIX) \
        ("[MSC v." _Py_STRINGIZE(_MSC_VER) " " SUFFIX "]")
/* e.g., this produces, after compile-time string catenation,
 *      ("[MSC v.1900 64 bit (Intel)]")
 *
 * _Py_STRINGIZE(_MSC_VER) expands to
 * _Py_STRINGIZE1(_MSC_VER) and this second macro call is scanned
 *      again for macros and so further expands to
 * _Py_STRINGIZE1(1900) which then expands to
 * "1900"
 */
#define _Py_STRINGIZE(X) _Py_STRINGIZE1(X)
#define _Py_STRINGIZE1(X) #X

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

/* set the COMPILER and support tier
 *
 * win_amd64 MSVC (x86_64-pc-windows-msvc): 1
 * win32 MSVC (i686-pc-windows-msvc): 1
 * win_arm64 MSVC (aarch64-pc-windows-msvc): 3
 * other archs and ICC: 0
 */
#ifdef MS_WIN64
#if defined(_M_X64) || defined(_M_AMD64)
#if defined(__clang__)
#define COMPILER ("[Clang " __clang_version__ "] 64 bit (AMD64) with MSC v." _Py_STRINGIZE(_MSC_VER) " CRT]")
#define PY_SUPPORT_TIER 0
#elif defined(__INTEL_COMPILER)
#define COMPILER ("[ICC v." _Py_STRINGIZE(__INTEL_COMPILER) " 64 bit (amd64) with MSC v." _Py_STRINGIZE(_MSC_VER) " CRT]")
#define PY_SUPPORT_TIER 0
#else
#define COMPILER _Py_PASTE_VERSION("64 bit (AMD64)")
#define PY_SUPPORT_TIER 1
#endif /* __clang__ */
#define PYD_PLATFORM_TAG "win_amd64"
#elif defined(_M_ARM64)
#define COMPILER _Py_PASTE_VERSION("64 bit (ARM64)")
#define PY_SUPPORT_TIER 3
#define PYD_PLATFORM_TAG "win_arm64"
#else
#define COMPILER _Py_PASTE_VERSION("64 bit (Unknown)")
#define PY_SUPPORT_TIER 0
#endif
#endif /* MS_WIN64 */

/* set the version macros for the windows headers */
/* Python 3.13+ requires Windows 10 or greater */
#define Py_WINVER 0x0A00 /* _WIN32_WINNT_WIN10 */
#define Py_NTDDI NTDDI_WIN10

/* We only set these values when building Python - we don't want to force
   these values on extensions, as that will affect the prototypes and
   structures exposed in the Windows headers. Even when building Python, we
   allow a single source file to override this - they may need access to
   structures etc so it can optionally use new Windows features if it
   determines at runtime they are available.
*/
#if defined(Py_BUILD_CORE) || defined(Py_BUILD_CORE_BUILTIN) || defined(Py_BUILD_CORE_MODULE)
#ifndef NTDDI_VERSION
#define NTDDI_VERSION Py_NTDDI
#endif
#ifndef WINVER
#define WINVER Py_WINVER
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT Py_WINVER
#endif
#endif

/* _W64 is not defined for VC6 or eVC4 */
#ifndef _W64
#define _W64
#endif

/* Define like size_t, omitting the "unsigned" */
#ifdef MS_WIN64
typedef __int64 Py_ssize_t;
#   define PY_SSIZE_T_MAX LLONG_MAX
#else
typedef _W64 int Py_ssize_t;
#   define PY_SSIZE_T_MAX INT_MAX
#endif
#define HAVE_PY_SSIZE_T 1

#if defined(MS_WIN32) && !defined(MS_WIN64)
#if defined(_M_IX86)
#if defined(__clang__)
#define COMPILER ("[Clang " __clang_version__ "] 32 bit (Intel) with MSC v." _Py_STRINGIZE(_MSC_VER) " CRT]")
#define PY_SUPPORT_TIER 0
#elif defined(__INTEL_COMPILER)
#define COMPILER ("[ICC v." _Py_STRINGIZE(__INTEL_COMPILER) " 32 bit (Intel) with MSC v." _Py_STRINGIZE(_MSC_VER) " CRT]")
#define PY_SUPPORT_TIER 0
#else
#define COMPILER _Py_PASTE_VERSION("32 bit (Intel)")
#define PY_SUPPORT_TIER 1
#endif /* __clang__ */
#define PYD_PLATFORM_TAG "win32"
#elif defined(_M_ARM)
#define COMPILER _Py_PASTE_VERSION("32 bit (ARM)")
#define PYD_PLATFORM_TAG "win_arm32"
#define PY_SUPPORT_TIER 0
#else
#define COMPILER _Py_PASTE_VERSION("32 bit (Unknown)")
#define PY_SUPPORT_TIER 0
#endif
#endif /* MS_WIN32 && !MS_WIN64 */

typedef int pid_t;

/* define some ANSI types that are not defined in earlier Win headers */
#if _MSC_VER >= 1200
/* This file only exists in VC 6.0 or higher */
#include <basetsd.h>
#endif

#endif /* _MSC_VER */

/* ------------------------------------------------------------------------*/
/* mingw and mingw-w64 define __MINGW32__ */
#ifdef __MINGW32__

#ifdef _WIN64
#define MS_WIN64
#endif

#endif /* __MINGW32__*/

/* ------------------------------------------------------------------------*/
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

#define COMPILER "[gcc]"
#define PY_LONG_LONG long long
#define PY_LLONG_MIN LLONG_MIN
#define PY_LLONG_MAX LLONG_MAX
#define PY_ULLONG_MAX ULLONG_MAX
#endif /* GNUC */

/* ------------------------------------------------------------------------*/
/* lcc-win32 defines __LCC__ */
#if defined(__LCC__)
/* XXX These defines are likely incomplete, but should be easy to fix.
   They should be complete enough to build extension modules. */

#define COMPILER "[lcc-win32]"
typedef int pid_t;
/* __declspec() is supported here too - do nothing to get the defaults */

#endif /* LCC */

/* ------------------------------------------------------------------------*/
/* End of compilers - finish up */

#ifndef NO_STDIO_H
#       include <stdio.h>
#endif

/* 64 bit ints are usually spelt __int64 unless compiler has overridden */
#ifndef PY_LONG_LONG
#       define PY_LONG_LONG __int64
#       define PY_LLONG_MAX _I64_MAX
#       define PY_LLONG_MIN _I64_MIN
#       define PY_ULLONG_MAX _UI64_MAX
#endif

/* For Windows the Python core is in a DLL by default.  Test
Py_NO_ENABLE_SHARED to find out.  Also support MS_NO_COREDLL for b/w compat */
#if !defined(MS_NO_COREDLL) && !defined(Py_NO_ENABLE_SHARED)
#       define Py_ENABLE_SHARED 1 /* standard symbol for shared library */
#       define MS_COREDLL       /* deprecated old symbol */
#endif /* !MS_NO_COREDLL && ... */

/*  All windows compilers that use this header support __declspec */
#define HAVE_DECLSPEC_DLL

/* For an MSVC DLL, we can nominate the .lib files used by extensions */
#ifdef MS_COREDLL
#       if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
                /* not building the core - must be an ext */
#               if defined(_MSC_VER) && !defined(Py_NO_LINK_LIB)
                        /* So MSVC users need not specify the .lib
                        file in their Makefile */
                        /* Define Py_NO_LINK_LIB to build extension disabling pragma
                        based auto-linking.
                        This is relevant when using build-system generator (e.g CMake) where
                        the linking is explicitly handled */
#                       if defined(Py_GIL_DISABLED)
#                       if defined(Py_DEBUG)
#                               pragma comment(lib,"python315t_d.lib")
#                       elif defined(Py_LIMITED_API)
#                               pragma comment(lib,"python3t.lib")
#                       else
#                               pragma comment(lib,"python315t.lib")
#                       endif /* Py_DEBUG */
#                       else /* Py_GIL_DISABLED */
#                       if defined(Py_DEBUG)
#                               pragma comment(lib,"python315_d.lib")
#                       elif defined(Py_LIMITED_API)
#                               pragma comment(lib,"python3.lib")
#                       else
#                               pragma comment(lib,"python315.lib")
#                       endif /* Py_DEBUG */
#                       endif /* Py_GIL_DISABLED */
#               endif /* _MSC_VER && !Py_NO_LINK_LIB */
#       endif /* Py_BUILD_CORE */
#endif /* MS_COREDLL */

#ifdef MS_WIN64
/* maintain "win32" sys.platform for backward compatibility of Python code,
   the Win64 API should be close enough to the Win32 API to make this
   preferable */
#       define PLATFORM "win32"
#       define SIZEOF_VOID_P 8
#       define SIZEOF_TIME_T 8
#       define SIZEOF_OFF_T 4
#       define SIZEOF_FPOS_T 8
#       define SIZEOF_HKEY 8
#       define SIZEOF_SIZE_T 8
#       define ALIGNOF_SIZE_T 8
#       define ALIGNOF_MAX_ALIGN_T 8
/* configure.ac defines HAVE_LARGEFILE_SUPPORT iff
   sizeof(off_t) > sizeof(long), and sizeof(long long) >= sizeof(off_t).
   On Win64 the second condition is not true, but if fpos_t replaces off_t
   then this is true. The uses of HAVE_LARGEFILE_SUPPORT imply that Win64
   should define this. */
#       define HAVE_LARGEFILE_SUPPORT
#elif defined(MS_WIN32)
#       define PLATFORM "win32"
#       define HAVE_LARGEFILE_SUPPORT
#       define SIZEOF_VOID_P 4
#       define SIZEOF_OFF_T 4
#       define SIZEOF_FPOS_T 8
#       define SIZEOF_HKEY 4
#       define SIZEOF_SIZE_T 4
#       define ALIGNOF_SIZE_T 4
        /* MS VS2005 changes time_t to a 64-bit type on all platforms */
#       if defined(_MSC_VER) && _MSC_VER >= 1400
#       define SIZEOF_TIME_T 8
#       else
#       define SIZEOF_TIME_T 4
#       endif
#       define ALIGNOF_MAX_ALIGN_T 8
#endif

#ifdef MS_WIN32

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define ALIGNOF_LONG 4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_DOUBLE 8
#define SIZEOF_FLOAT 4

/* VC 7.1 has them and VC 6.0 does not.  VC 6.0 has a version number of 1200.
   Microsoft eMbedded Visual C++ 4.0 has a version number of 1201 and doesn't
   define these.
   If some compiler does not provide them, modify the #if appropriately. */
#if defined(_MSC_VER)
#if _MSC_VER > 1300
#define HAVE_UINTPTR_T 1
#define HAVE_INTPTR_T 1
#else
/* VC6, VS 2002 and eVC4 don't support the C99 LL suffix for 64-bit integer literals */
#define Py_LL(x) x##I64
#endif  /* _MSC_VER > 1300  */
#endif  /* _MSC_VER */

#endif

/* define signed and unsigned exact-width 32-bit and 64-bit types, used in the
   implementation of Python integers. */
#define PY_UINT32_T uint32_t
#define PY_UINT64_T uint64_t
#define PY_INT32_T int32_t
#define PY_INT64_T int64_t

/* Fairly standard from here! */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define to empty if the keyword does not work.  */
/* #define const  */

/* Define to 1 if you have the <conio.h> header file. */
#define HAVE_CONIO_H 1

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the declaration of `tzname', and to 0 if you don't.
   */
#define HAVE_DECL_TZNAME 1

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

/* Define if getpgrp() must be called as getpgrp(0)
   and (consequently) setpgrp() as setpgrp(0, 0). */
/* #undef GETPGRP_HAVE_ARGS */

/* Define this if your time.h defines altzone */
/* #define HAVE_ALTZONE */

/* Define if you have the putenv function.  */
#define HAVE_PUTENV

/* Define if your compiler supports function prototypes */
#define HAVE_PROTOTYPES

/* Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0). */
/* #undef SYS_SELECT_WITH_SYS_TIME */

/* Define if you want build the _decimal module using a coroutine-local rather
   than a thread-local context */
#define WITH_DECIMAL_CONTEXTVAR 1

/* Define if you want documentation strings in extension modules */
#define WITH_DOC_STRINGS 1

/* Define if you want to compile in rudimentary thread support */
/* #undef WITH_THREAD */

/* Define if you want to use the GNU readline library */
/* #define WITH_READLINE 1 */

/* Use Python's own small-block memory-allocator. */
#define WITH_PYMALLOC 1

/* Define if you want to compile in mimalloc memory allocator. */
#define WITH_MIMALLOC 1

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

/* Define to 1 if you have the `shutdown' function. */
#define HAVE_SHUTDOWN 1

/* Define if you have symlink.  */
/* #undef HAVE_SYMLINK */

/* Define if you have tcgetpgrp.  */
/* #undef HAVE_TCGETPGRP */

/* Define if you have tcsetpgrp.  */
/* #undef HAVE_TCSETPGRP */

/* Define if you have times.  */
/* #undef HAVE_TIMES */

/* Define to 1 if you have the `umask' function. */
#define HAVE_UMASK 1

/* Define if you have uname.  */
/* #undef HAVE_UNAME */

/* Define if you have waitpid.  */
/* #undef HAVE_WAITPID */

/* Define to 1 if you have the `wcsftime' function. */
#if defined(_MSC_VER) && _MSC_VER >= 1310
#define HAVE_WCSFTIME 1
#endif

/* Define to 1 if you have the `wcscoll' function. */
#define HAVE_WCSCOLL 1

/* Define to 1 if you have the `wcsxfrm' function. */
#define HAVE_WCSXFRM 1

/* Define if the zlib library has inflateCopy */
#define HAVE_ZLIB_COPY 1

/* Define if you have the <dlfcn.h> header file.  */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <process.h> header file. */
#define HAVE_PROCESS_H 1

/* Define to 1 if you have the <signal.h> header file. */
#define HAVE_SIGNAL_H 1

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <sys/audioio.h> header file.  */
/* #undef HAVE_SYS_AUDIOIO_H */

/* Define if you have the <sys/param.h> header file.  */
/* #define HAVE_SYS_PARAM_H 1 */

/* Define if you have the <sys/select.h> header file.  */
/* #define HAVE_SYS_SELECT_H 1 */

/* Define to 1 if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
/* #define HAVE_SYS_TIME_H 1 */

/* Define if you have the <sys/times.h> header file.  */
/* #define HAVE_SYS_TIMES_H 1 */

/* Define to 1 if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <sys/un.h> header file.  */
/* #define HAVE_SYS_UN_H 1 */

/* Define if you have the <sys/utime.h> header file.  */
/* #define HAVE_SYS_UTIME_H 1 */

/* Define if you have the <sys/utsname.h> header file.  */
/* #define HAVE_SYS_UTSNAME_H 1 */

/* Define if you have the <unistd.h> header file.  */
/* #define HAVE_UNISTD_H 1 */

/* Define if you have the <utime.h> header file.  */
/* #define HAVE_UTIME_H 1 */

/* Define if the compiler provides a wchar.h header file. */
#define HAVE_WCHAR_H 1

/* The size of `wchar_t', as computed by sizeof. */
#define SIZEOF_WCHAR_T 2

/* The size of `_Bool', as computed by sizeof. */
#define SIZEOF__BOOL 1

/* The size of `pid_t', as computed by sizeof. */
#define SIZEOF_PID_T SIZEOF_INT

/* Define if you have the dl library (-ldl).  */
/* #undef HAVE_LIBDL */

/* Define if you have the mpc library (-lmpc).  */
/* #undef HAVE_LIBMPC */

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

/* WinSock does not use a bitmask in select, and uses
   socket handles greater than FD_SETSIZE */
#define Py_SOCKET_FD_CAN_BE_GE_FD_SETSIZE

/* Define if C doubles are 64-bit IEEE 754 binary format, stored with the
   least significant byte first */
#define DOUBLE_IS_LITTLE_ENDIAN_IEEE754 1

/* Define to 1 if you have the `erf' function. */
#define HAVE_ERF 1

/* Define to 1 if you have the `erfc' function. */
#define HAVE_ERFC 1

// netdb.h functions (provided by winsock.h)
#define HAVE_GETHOSTNAME 1
#define HAVE_GETHOSTBYADDR 1
#define HAVE_GETHOSTBYNAME 1
#define HAVE_GETPROTOBYNAME 1
#define HAVE_GETSERVBYNAME 1
#define HAVE_GETSERVBYPORT 1
// sys/socket.h functions (provided by winsock.h)
#define HAVE_INET_PTON 1
#define HAVE_INET_NTOA 1
#define HAVE_ACCEPT 1
#define HAVE_BIND 1
#define HAVE_CONNECT 1
#define HAVE_GETSOCKNAME 1
#define HAVE_LISTEN 1
#define HAVE_RECVFROM 1
#define HAVE_SENDTO 1
#define HAVE_SETSOCKOPT 1
#define HAVE_SOCKET 1

/* Define to 1 if you have the `dup' function. */
#define HAVE_DUP 1

/* framework name */
#define _PYTHONFRAMEWORK ""

/* Define if libssl has X509_VERIFY_PARAM_set1_host and related function */
#define HAVE_X509_VERIFY_PARAM_SET1_HOST 1

// Truncate the thread name to 32766 characters.
#define _PYTHREAD_NAME_MAXLEN 32766

#endif /* !Py_CONFIG_H */
