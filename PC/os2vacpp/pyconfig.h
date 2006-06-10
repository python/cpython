#ifndef Py_CONFIG_H
#define Py_CONFIG_H

/**********************************************************************
 * pyconfig.h.  NOT Generated automatically by configure.
 *
 * This is a manually maintained version used for the IBM VisualAge
 * C/C++ compiler on the OS/2 platform.  It is a standard part of
 * the Python distribution.
 *
 * FILESYSTEM DEFINES:
 * The code specific to a particular way of naming files and
 * directory paths should be wrapped around one of the following
 * #defines:
 *
 *     DOSFILESYS   PCDOS-Style (for PCDOS, Windows and OS/2)
 *     MACFILESYS   Macintosh-Style
 *     UNIXFILESYS  Unix-Style
 *     AMIGAFILESYS AmigaDOS-Style
 * 
 * Because of the different compilers and operating systems in
 * use on the Intel platform, neither the compiler name nor
 * the operating system name is sufficient.
 *
 * OS/2 DEFINES:
 * The code specific to OS/2's Program API should be wrapped around
 *
 * __TOS_OS2__   Target Operating System, OS/2
 *
 * Any code specific to the compiler itself should be wrapped with
 *
 * __IBMC__      IBM C Compiler
 * __IBMCPP__    IBM C++ Compiler
 *
 * Note that since the VisualAge C/C++ compiler is also available
 * for the Windows platform, it may be necessary to use both a
 * __TOS_OS2__ and a __IBMC__ to select a very specific environment.
 *
 **********************************************************************/

/*
 * Some systems require special declarations for data items imported
 * or exported from dynamic link libraries.  Note that the definition
 * of DL_IMPORT covers both cases.  Define USE_DL_IMPORT for the client
 * of a DLL.  Define USE_DL_EXPORT when making a DLL.
 */

#include <io.h>

/* Configuration Options for Finding Modules */
#define PREFIX                 ""
#define EXEC_PREFIX            ""

/* Provide a default library so writers of extension modules
 * won't have to explicitly specify it anymore
 */
#pragma library("Python24.lib")

/***************************************************/
/*    32-Bit IBM VisualAge C/C++ v3.0 for OS/2     */
/*  (Convert Compiler Flags into Useful Switches)  */
/***************************************************/
#define PLATFORM    "os2"
#define COMPILER    "[VisualAge C/C++]"
#define PYOS_OS2    /* Define Indicator of Operating System */
#define PYCC_VACPP  /* Define Indicator of C Compiler */

  /* Platform Filesystem */
#define PYTHONPATH  ".;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
#define DOSFILESYS  /* OS/2 Uses the DOS File Naming Conventions */
/* #define IMPORT_8x3_NAMES (let's move up to long filenames) */

  /* Platform CPU-Mode Dependencies */
#define WORD_BIT                32 /* OS/2 is a 32-Bit Operating System */
#define LONG_BIT                32
#define SIZEOF_INT               4 /* Count of Bytes in an (int)            */
#define SIZEOF_LONG              4 /* Count of Bytes in a (long)            */
#define SIZEOF_VOID_P            4 /* Count of Bytes in a (void *)          */
/* #define HAVE_LONG_LONG     1 */ /* VAC++ does not support (long long)    */
/* #define SIZEOF_LONG_LONG   8 */ /* Count of Bytes in a (long long)       */

/* unicode definines */
#define Py_USING_UNICODE
#define PY_UNICODE_TYPE    wchar_t
#define Py_UNICODE_SIZE SIZEOF_SHORT

/* dynamic loading */
#define HAVE_DYNAMIC_LOADING 1

/* Define if type char is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
/* #undef __CHAR_UNSIGNED__ */
#endif

typedef int mode_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;

#if defined(__MULTI__)     /* If Compiler /Gt+ Multithread Option Enabled,  */
  #define WITH_THREAD            1 /* Enable Threading Throughout Python    */
  #define OS2_THREADS            1 /* And Use the OS/2 Flavor of Threads    */
/* #define _REENTRANT 1 */ /* Use thread-safe errno, h_errno, and other fns */
#endif

  /* Compiler Runtime Library Capabilities */
#include <ctype.h>
#include <direct.h>
/* #undef BAD_STATIC_FORWARD */ /* if compiler botches static fwd decls */

#define STDC_HEADERS             1 /* VAC++ is an ANSI C Compiler           */
#define HAVE_HYPOT               1 /* hypot()                               */
#define HAVE_PUTENV              1 /* putenv()                              */
/* #define VA_LIST_IS_ARRAY   1 */ /* if va_list is an array of some kind   */
/* #define HAVE_CONIO_H       1 */ /* #include <conio.h>                    */
#define HAVE_ERRNO_H             1 /* #include <errno.h>                    */
#define HAVE_SYS_STAT_H          1 /* #include <sys/stat.h>                 */
#define HAVE_SYS_TYPES_H         1 /* #include <sys/types.h>                */

  /* Variable-Arguments/Prototypes */
#define HAVE_PROTOTYPES          1 /* VAC++ supports C Function Prototypes  */
#define HAVE_STDARG_PROTOTYPES   1 /* Our <stdarg.h> has prototypes         */

  /* String/Memory/Locale Operations */
#define HAVE_MEMMOVE             1 /* memmove()                             */
#define HAVE_STRERROR            1 /* strerror()                            */
#define HAVE_SETLOCALE           1 /* setlocale()                           */
#define MALLOC_ZERO_RETURNS_NULL 1 /* Our malloc(0) returns a NULL ptr      */

  /* Signal Handling */
#define HAVE_SIGNAL_H            1 /* signal.h                              */
#define RETSIGTYPE            void /* Return type of handlers (int or void) */
/* #undef WANT_SIGFPE_HANDLER   */ /* Handle SIGFPE (see Include/pyfpe.h)   */
/* #define HAVE_ALARM         1 */ /* alarm()                               */
/* #define HAVE_SIGINTERRUPT  1 */ /* siginterrupt()                        */
/* #define HAVE_SIGRELSE      1 */ /* sigrelse()                            */
#define DONT_HAVE_SIG_ALARM      1
#define DONT_HAVE_SIG_PAUSE      1

  /* Clock/Time Support */
#define HAVE_FTIME               1 /* We have ftime() in <sys/timeb.h>      */
#define HAVE_CLOCK               1 /* clock()                               */
#define HAVE_STRFTIME            1 /* strftime()                            */
#define HAVE_MKTIME              1 /* mktime()                              */
#define HAVE_TZNAME              1 /* No tm_zone but do have tzname[]       */
#define HAVE_TIMES               1 /* #include <sys/times.h>                */
#define HAVE_SYS_UTIME_H         1 /* #include <sys/utime.h>                */
/* #define HAVE_UTIME_H       1 */ /* #include <utime.h>                    */
#define HAVE_SYS_TIME_H          1 /* #include <sys/time.h>                 */
/* #define TM_IN_SYS_TIME     1 */ /* <sys/time.h> declares struct tm       */
#define HAVE_GETTIMEOFDAY        1 /* gettimeofday()                        */
/* #define GETTIMEOFDAY_NO_TZ 1 */ /* gettimeofday() does not have 2nd arg  */
/* #define HAVE_TIMEGM        1 */ /* timegm()                              */
#define TIME_WITH_SYS_TIME       1 /* Mix <sys/time.h> and <time.h>         */
#define SYS_SELECT_WITH_SYS_TIME 1 /* Mix <sys/select.h> and <sys/time.h>   */
/* #define HAVE_ALTZONE       1 */ /* if <time.h> defines altzone           */

  /* Network/Sockets Support */
#define HAVE_SYS_SELECT_H       1 /* #include <sys/select.h>                */
#define BSD_SELECT              1 /* Use BSD versus OS/2 form of select()   */
#define HAVE_SELECT             1 /* select()                               */
#define HAVE_GETPEERNAME        1 /* getpeername()                          */
/* #undef HAVE_GETHOSTNAME_R 1 */ /* gethostname_r()                        */

  /* File I/O */
#define HAVE_DUP2                1 /* dup2()                                */
#define HAVE_EXECV               1 /* execv()                               */
#define HAVE_SETVBUF             1 /* setvbuf()                             */
#define HAVE_GETCWD              1 /* getcwd()                              */
#define HAVE_PIPE                1 /* pipe()     [OS/2-specific code added] */
#define HAVE_IO_H                1 /* #include <io.h>                       */
#define HAVE_FCNTL_H             1 /* #include <fcntl.h>                    */
#define HAVE_DIRECT_H            1 /* #include <direct.h>                   */
/* #define HAVE_FLOCK         1 */ /* flock()                               */
/* #define HAVE_TRUNCATE      1 */ /* truncate()                            */
/* #define HAVE_FTRUNCATE     1 */ /* ftruncate()                           */
/* #define HAVE_LSTAT         1 */ /* lstat()                               */
/* #define HAVE_DIRENT_H      1 */ /* #include <dirent.h>                   */
/* #define HAVE_OPENDIR       1 */ /* opendir()                             */

  /* Process Operations */
#define HAVE_PROCESS_H           1 /* #include <process.h>                  */
#define HAVE_GETPID              1 /* getpid()                              */
#define HAVE_SYSTEM              1 /* system()                              */
#define HAVE_WAIT                1 /* wait()                                */
#define HAVE_KILL                1 /* kill()     [OS/2-specific code added] */
#define HAVE_POPEN               1 /* popen()    [OS/2-specific code added] */
/* #define HAVE_GETPPID       1 */ /* getppid()                             */
/* #define HAVE_WAITPID       1 */ /* waitpid()                             */
/* #define HAVE_FORK          1 */ /* fork()                                */

  /* User/Group ID Queries */
/* #define HAVE_GETEGID       1 */
/* #define HAVE_GETEUID       1 */
/* #define HAVE_GETGID        1 */
/* #define HAVE_GETUID        1 */

  /* Unix-Specific */
/* #define HAVE_SYS_UN_H            1 /* #include <sys/un.h>                   */
/* #define HAVE_SYS_UTSNAME_H 1 */ /* #include <sys/utsname.h>              */
/* #define HAVE_SYS_WAIT_H    1 */ /* #include <sys/wait.h>                 */
/* #define HAVE_UNISTD_H      1 */ /* #include <unistd.h>                   */
/* #define HAVE_UNAME         1 */ /* uname ()                              */

/* Define if you want documentation strings in extension modules */
#define WITH_DOC_STRINGS 1

#ifdef USE_DL_EXPORT
  #define DL_IMPORT(RTYPE) RTYPE _System
#endif

#endif /* !Py_CONFIG_H */

