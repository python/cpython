/* RISCOS/pyconfig.h: Python configuration for RISC OS  */

#ifndef Py_PYCONFIG_H
#define Py_PYCONFIG_H

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

/* Define if type char is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
#undef __CHAR_UNSIGNED__
#endif

/* Define to empty if the keyword does not work.  */
#undef const

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef gid_t

/* Define if your struct tm has tm_zone.  */
#undef HAVE_TM_ZONE

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
#undef HAVE_TZNAME

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef mode_t

/* Define to `long' if <sys/types.h> doesn't define.  */
#undef off_t

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef pid_t

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
#undef _POSIX_1_SOURCE

/* Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
#undef size_t

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if your <sys/time.h> declares struct tm.  */
#define TM_IN_SYS_TIME 1

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef uid_t

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
#undef WORDS_BIGENDIAN

/* Define for AIX if your compiler is a genuine IBM xlC/xlC_r
   and you want support for AIX C++ shared extension modules. */
#undef AIX_GENUINE_CPLUSPLUS

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

/* This must be set to 64 on some systems to enable large file support */
#undef _FILE_OFFSET_BITS

/* Define if getpgrp() must be called as getpgrp(0). */
#undef GETPGRP_HAVE_ARG

/* Define if gettimeofday() does not have second (timezone) argument
   This is the case on Motorola V4 (R40V4.2) */
#undef GETTIMEOFDAY_NO_TZ

/* Define this if your time.h defines altzone */
#undef HAVE_ALTZONE

/* Define if --enable-ipv6 is specified */
#undef ENABLE_IPV6

/* Define if sockaddr has sa_len member */
#undef HAVE_SOCKADDR_SA_LEN

/* struct addrinfo (netdb.h) */
#undef HAVE_ADDRINFO

/* struct sockaddr_storage (sys/socket.h) */
#undef HAVE_SOCKADDR_STORAGE

/* Defined when any dynamic module loading is enabled */
#define HAVE_DYNAMIC_LOADING 1

/* Define this if you have flockfile(), getc_unlocked(), and funlockfile() */
#undef HAVE_GETC_UNLOCKED

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
#define HAVE_LONG_LONG

/* Define if your compiler supports function prototypes */
#define HAVE_PROTOTYPES 1

/* Define if you have GNU PTH threads */
#undef HAVE_PTH

/* Define if you have readline 4.2 */
#undef HAVE_RL_COMPLETION_MATCHES

/* Define if your compiler supports variable length function prototypes
   (e.g. void fprintf(FILE *, char *, ...);) *and* <stdarg.h> */
#define HAVE_STDARG_PROTOTYPES 1

/* Define this if you have the type uintptr_t */
#undef HAVE_UINTPTR_T

/* Define if you have a useable wchar_t type defined in wchar.h; useable
   means wchar_t must be 16-bit unsigned type. (see
   Include/unicodeobject.h). */
#undef HAVE_USABLE_WCHAR_T

/* Define if the compiler provides a wchar.h header file. */
#undef HAVE_WCHAR_H

/* This must be defined on some systems to enable large file support */
#undef _LARGEFILE_SOURCE

/* Define if you want to have a Unicode type. */
#define Py_USING_UNICODE 1

/* Define as the integral type used for Unicode representation. */
#define PY_UNICODE_TYPE unsigned short

/* Define as the size of the unicode type. */
#define Py_UNICODE_SIZE 2

/* Define if nice() returns success/failure instead of the new priority. */
#undef HAVE_BROKEN_NICE

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
#define SIZEOF_OFF_T 4

/* The number of bytes in a time_t. */
#define SIZEOF_TIME_T 4

/* The number of bytes in a pthread_t. */
#undef SIZEOF_PTHREAD_T

/* Define to `int' if <sys/types.h> doesn't define.  */
#define socklen_t int

/* Define if  you can safely include both <sys/select.h> and <sys/time.h>
   (which you can't on SCO ODT 3.0). */
#undef SYS_SELECT_WITH_SYS_TIME

/* Define if a va_list is an array of some kind */
#define VA_LIST_IS_ARRAY 1

/* Define to empty if the keyword does not work.  */
#undef volatile

/* Define if you want SIGFPE handled (see Include/pyfpe.h). */
#undef WANT_SIGFPE_HANDLER

/* Define if you want wctype.h functions to be used instead of the
   one supplied by Python itself. (see Include/unicodectype.h). */
#undef WANT_WCTYPE_FUNCTIONS

/* Define if you want documentation strings in extension modules */
#define WITH_DOC_STRINGS 1

/* Define if you want to use the new-style (Openstep, Rhapsody, MacOS)
   dynamic linker (dyld) instead of the old-style (NextStep) dynamic
   linker (rld). Dyld is necessary to support frameworks. */
#undef WITH_DYLD

/* Define if you want to compile in Python-specific mallocs */
#define WITH_PYMALLOC 1

/* Define if you want to produce an OpenStep/Rhapsody framework
   (shared library plus accessory files). */
#undef WITH_NEXT_FRAMEWORK

/* Define if you want to use MacPython modules on MacOSX in unix-Python */
#undef USE_TOOLBOX_OBJECT_GLUE

/* Define if you want to compile in rudimentary thread support */
#undef WITH_THREAD

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a fpos_t.  */
#undef SIZEOF_FPOS_T

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a uintptr_t.  */
#undef SIZEOF_UINTPTR_T

/* The number of bytes in a void *.  */
#define SIZEOF_VOID_P 4

/* The number of bytes in a wchar_t.  */
#undef SIZEOF_WCHAR_T

/* Define if you have the _getpty function.  */
#undef HAVE__GETPTY

/* Define if you have the alarm function.  */
#undef HAVE_ALARM

/* Define if you have the chown function.  */
#undef HAVE_CHOWN

/* Define if you have the clock function.  */
#define HAVE_CLOCK 1

/* Define if you have the confstr function.  */
#undef HAVE_CONFSTR

/* Define if you have the ctermid function.  */
#undef HAVE_CTERMID

/* Define if you have the ctermid_r function.  */
#undef HAVE_CTERMID_R

/* Define if you have the dlopen function.  */
#undef HAVE_DLOPEN

/* Define if you have the dup2 function.  */
#undef HAVE_DUP2

/* Define if you have the execv function.  */
#undef HAVE_EXECV

/* Define if you have the fdatasync function.  */
#undef HAVE_FDATASYNC

/* Define if you have the flock function.  */
#undef HAVE_FLOCK

/* Define if you have the fork function.  */
#undef HAVE_FORK

/* Define if you have the forkpty function.  */
#undef HAVE_FORKPTY

/* Define if you have the fpathconf function.  */
#undef HAVE_FPATHCONF

/* Define if you have the fseek64 function.  */
#undef HAVE_FSEEK64

/* Define if you have the fseeko function.  */
#undef HAVE_FSEEKO

/* Define if you have the fstatvfs function.  */
#undef HAVE_FSTATVFS

/* Define if you have the fsync function.  */
#undef HAVE_FSYNC

/* Define if you have the ftell64 function.  */
#undef HAVE_FTELL64

/* Define if you have the ftello function.  */
#undef HAVE_FTELLO

/* Define if you have the ftime function.  */
#undef HAVE_FTIME

/* Define if you have the ftruncate function.  */
#undef HAVE_FTRUNCATE

/* Define if you have the gai_strerror function.  */
#undef HAVE_GAI_STRERROR

/* Define if you have the getaddrinfo function.  */
#undef HAVE_GETADDRINFO

/* Define if you have the getcwd function.  */
#undef HAVE_GETCWD

/* Define if you have the getgroups function.  */
#undef HAVE_GETGROUPS

/* Define if you have the gethostbyname function.  */
#undef HAVE_GETHOSTBYNAME

/* Define if you have the getlogin function.  */
#undef HAVE_GETLOGIN

/* Define if you have the getnameinfo function.  */
#undef HAVE_GETNAMEINFO

/* Define if you have the getpeername function.  */
#define HAVE_GETPEERNAME

/* Define if you have the getpgid function.  */
#undef HAVE_GETPGID

/* Define if you have the getpgrp function.  */
#undef HAVE_GETPGRP

/* Define if you have the getpid function.  */
#undef HAVE_GETPID

/* Define if you have the getpriority function.  */
#undef HAVE_GETPRIORITY

/* Define if you have the getpwent function.  */
#undef HAVE_GETPWENT

/* Define if you have the gettimeofday function.  */
#undef HAVE_GETTIMEOFDAY

/* Define if you have the getwd function.  */
#undef HAVE_GETWD

/* Define if you have the hstrerror function.  */
#undef HAVE_HSTRERROR

/* Define if you have the hypot function.  */
#define HAVE_HYPOT

/* Define if you have the inet_pton function.  */
#define HAVE_INET_PTON 1

/* Define if you have the kill function.  */
#undef HAVE_KILL

/* Define if you have the link function.  */
#undef HAVE_LINK

/* Define if you have the lstat function.  */
#undef HAVE_LSTAT

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the mkfifo function.  */
#undef HAVE_MKFIFO

/* Define if you have the mktime function.  */
#define HAVE_MKTIME 1

/* Define if you have the mremap function.  */
#undef HAVE_MREMAP

/* Define if you have the nice function.  */
#undef HAVE_NICE

/* Define if you have the openpty function.  */
#undef HAVE_OPENPTY

/* Define if you have the pathconf function.  */
#undef HAVE_PATHCONF

/* Define if you have the pause function.  */
#undef HAVE_PAUSE

/* Define if you have the plock function.  */
#undef HAVE_PLOCK

/* Define if you have the poll function.  */
#undef HAVE_POLL

/* Define if you have the pthread_init function.  */
#undef HAVE_PTHREAD_INIT

/* Define if you have the putenv function.  */
#undef HAVE_PUTENV

/* Define if you have the readlink function.  */
#undef HAVE_READLINK

/* Define if you have the select function.  */
#undef HAVE_SELECT

/* Define if you have the setegid function.  */
#undef HAVE_SETEGID

/* Define if you have the seteuid function.  */
#undef HAVE_SETEUID

/* Define if you have the setgid function.  */
#undef HAVE_SETGID

/* Define if you have the setlocale function.  */
#define HAVE_SETLOCALE 1

/* Define if you have the setpgid function.  */
#undef HAVE_SETPGID

/* Define if you have the setpgrp function.  */
#undef HAVE_SETPGRP

/* Define if you have the setregid function.  */
#undef HAVE_SETREGID

/* Define if you have the setreuid function.  */
#undef HAVE_SETREUID

/* Define if you have the setsid function.  */
#undef HAVE_SETSID

/* Define if you have the setuid function.  */
#undef HAVE_SETUID

/* Define if you have the setvbuf function.  */
#undef HAVE_SETVBUF

/* Define if you have the sigaction function.  */
#undef HAVE_SIGACTION

/* Define if you have the siginterrupt function.  */
#undef HAVE_SIGINTERRUPT

/* Define if you have the sigrelse function.  */
#undef HAVE_SIGRELSE

/* Define if you have the snprintf function.  */
#undef HAVE_SNPRINTF

/* Define if you have the statvfs function.  */
#undef HAVE_STATVFS

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have the symlink function.  */
#undef HAVE_SYMLINK

/* Define if you have the sysconf function.  */
#undef HAVE_SYSCONF

/* Define if you have the tcgetpgrp function.  */
#undef HAVE_TCGETPGRP

/* Define if you have the tcsetpgrp function.  */
#undef HAVE_TCSETPGRP

/* Define if you have the tempnam function.  */
#undef HAVE_TEMPNAM

/* Define if you have the timegm function.  */
#undef HAVE_TIMEGM

/* Define if you have the times function.  */
#undef HAVE_TIMES

/* Define if you have the tmpfile function.  */
#undef HAVE_TMPFILE

/* Define if you have the tmpnam function.  */
#undef HAVE_TMPNAM

/* Define if you have the tmpnam_r function.  */
#undef HAVE_TMPNAM_R

/* Define if you have the truncate function.  */
#undef HAVE_TRUNCATE

/* Define if you have the uname function.  */
#undef HAVE_UNAME

/* Define if you have the waitpid function.  */
#undef HAVE_WAITPID

/* Define if you have the <conio.h> header file. */
#undef HAVE_CONIO_H

/* Define if you have the <db.h> header file.  */
#undef HAVE_DB_H

/* Define if you have the <db1/ndbm.h> header file.  */
#undef HAVE_DB1_NDBM_H

/* Define if you have the <db_185.h> header file.  */
#undef HAVE_DB_185_H

/* Define if you have the <direct.h> header file. */
#undef HAVE_DIRECT_H

/* Define if you have the <dirent.h> header file.  */
#undef HAVE_DIRENT_H

/* Define if you have the <dlfcn.h> header file.  */
#undef HAVE_DLFCN_H

/* Define if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#undef HAVE_FCNTL_H

/* Define if you have the <gdbm/ndbm.h> header file.  */
#undef HAVE_GDBM_NDBM_H

/* Define if you have the <io.h> header file.  */
#undef HAVE_IO_H

/* Define if you have the <langinfo.h> header file.  */
#undef HAVE_LANGINFO_H

/* Define if you have the <libutil.h> header file.  */
#undef HAVE_LIBUTIL_H

/* Define if you have the <ncurses.h> header file.  */
#undef HAVE_NCURSES_H

/* Define if you have the <ndbm.h> header file.  */
#undef HAVE_NDBM_H

/* Define if you have the <ndir.h> header file.  */
#undef HAVE_NDIR_H

/* Define if you have the <netpacket/packet.h> header file.  */
#undef HAVE_NETPACKET_PACKET_H

/* Define if you have the <poll.h> header file.  */
#undef HAVE_POLL_H

/* Define if you have the <process.h> header file.  */
#undef HAVE_PROCESS_H

/* Define if you have the <pthread.h> header file.  */
#undef HAVE_PTHREAD_H

/* Define if you have the <pty.h> header file.  */
#undef HAVE_PTY_H

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H

/* Define if you have the <sys/audioio.h> header file.  */
#undef HAVE_SYS_AUDIOIO_H

/* Define if you have the <sys/dir.h> header file.  */
#undef HAVE_SYS_DIR_H

/* Define if you have the <sys/file.h> header file.  */
#undef HAVE_SYS_FILE_H

/* Define if you have the <sys/lock.h> header file.  */
#undef HAVE_SYS_LOCK_H

/* Define if you have the <sys/modem.h> header file.  */
#undef HAVE_SYS_MODEM_H

/* Define if you have the <sys/ndir.h> header file.  */
#undef HAVE_SYS_NDIR_H

/* Define if you have the <sys/param.h> header file.  */
#undef HAVE_SYS_PARAM_H

/* Define if you have the <sys/poll.h> header file.  */
#undef HAVE_SYS_POLL_H

/* Define if you have the <sys/resource.h> header file.  */
#undef HAVE_SYS_RESOURCE_H

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <sys/socket.h> header file.  */
#undef HAVE_SYS_SOCKET_H

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <sys/times.h> header file.  */
#undef HAVE_SYS_TIMES_H

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <sys/un.h> header file.  */
#undef HAVE_SYS_UN_H

/* Define if you have the <sys/utsname.h> header file.  */
#undef HAVE_SYS_UTSNAME_H

/* Define if you have the <sys/wait.h> header file.  */
#undef HAVE_SYS_WAIT_H

/* Define if you have the <termios.h> header file.  */
#undef HAVE_TERMIOS_H

/* Define if you have the <thread.h> header file.  */
#undef HAVE_THREAD_H

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the <utime.h> header file.  */
#undef HAVE_UTIME_H

/* Define if you have the dl library (-ldl).  */
#undef HAVE_LIBDL

/* Define if you have the dld library (-ldld).  */
#undef HAVE_LIBDLD

/* Define if you have the ieee library (-lieee).  */
#undef HAVE_LIBIEEE

#ifdef __CYGWIN__
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#else
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#endif

/* Define the macros needed if on a UnixWare 7.x system. */
#if defined(__USLC__) && defined(__SCO_VERSION__)
#define STRICT_SYSV_CURSES /* Don't use ncurses extensions */
#endif


#define DONT_HAVE_FSTAT 1
#define DONT_HAVE_STAT  1

#define PLATFORM "riscos"

#endif /* Py_PYCONFIG_H */
