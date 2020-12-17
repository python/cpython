/*
 * tclWinPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between Windows and Unix. It should be the only
 *	file that contains #ifdefs to handle different flavors of OS.
 *
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TCLWINPORT
#define _TCLWINPORT

#if !defined(_WIN64) && defined(BUILD_tcl)
/* See [Bug 3354324]: file mtime sets wrong time */
#   define _USE_32BIT_TIME_T
#endif

/*
 * We must specify the lower version we intend to support.
 *
 * WINVER = 0x0500 means Windows 2000 and above
 */

#ifndef WINVER
#   define WINVER 0x0501
#endif
#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0501
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

/* Compatibility to older visual studio / windows platform SDK */
#if !defined(MAXULONG_PTR)
typedef DWORD DWORD_PTR;
typedef DWORD_PTR * PDWORD_PTR;
#endif

/*
 * Ask for the winsock function typedefs, also.
 */
#define INCL_WINSOCK_API_TYPEDEFS   1
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef HAVE_WSPIAPI_H
#   include <wspiapi.h>
#endif

#ifdef CHECK_UNICODE_CALLS
#   define _UNICODE
#   define UNICODE
#   define __TCHAR_DEFINED
    typedef float *_TCHAR;
#   define _TCHAR_DEFINED
    typedef float *TCHAR;
#endif /* CHECK_UNICODE_CALLS */

/*
 *  Pull in the typedef of TCHAR for windows.
 */
#include <tchar.h>
#ifndef _TCHAR_DEFINED
    /* Borland seems to forget to set this. */
    typedef _TCHAR TCHAR;
#   define _TCHAR_DEFINED
#endif
#if defined(_MSC_VER) && defined(__STDC__)
    /* VS2005 SP1 misses this. See [Bug #3110161] */
    typedef _TCHAR TCHAR;
#endif

/*
 *---------------------------------------------------------------------------
 * The following sets of #includes and #ifdefs are required to get Tcl to
 * compile under the windows compilers.
 *---------------------------------------------------------------------------
 */

#include <time.h>
#include <wchar.h>
#include <io.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <malloc.h>
#include <process.h>
#include <signal.h>
#include <limits.h>

#ifndef __GNUC__
#    define strncasecmp _strnicmp
#    define strcasecmp _stricmp
#endif

/*
 * Need to block out these includes for building extensions with MetroWerks
 * compiler for Win32.
 */

#ifndef __MWERKS__
#include <sys/stat.h>
#include <sys/timeb.h>
#   ifdef __BORLANDC__
#	include <utime.h>
#   else
#	include <sys/utime.h>
#   endif /* __BORLANDC__ */
#endif /* __MWERKS__ */

/*
 * The following defines redefine the Windows Socket errors as
 * BSD errors so Tcl_PosixError can do the right thing.
 */

#ifndef ENOTEMPTY
#   define ENOTEMPTY 	41	/* Directory not empty */
#endif
#ifndef EREMOTE
#   define EREMOTE	66	/* The object is remote */
#endif
#ifndef EPFNOSUPPORT
#   define EPFNOSUPPORT	96	/* Protocol family not supported */
#endif
#ifndef EADDRINUSE
#   define EADDRINUSE	100	/* Address already in use */
#endif
#ifndef EADDRNOTAVAIL
#   define EADDRNOTAVAIL 101	/* Can't assign requested address */
#endif
#ifndef EAFNOSUPPORT
#   define EAFNOSUPPORT	102	/* Address family not supported */
#endif
#ifndef EALREADY
#   define EALREADY	103	/* Operation already in progress */
#endif
#ifndef EBADMSG
#   define EBADMSG	104	/* Not a data message */
#endif
#ifndef ECANCELED
#   define ECANCELED	105	/* Canceled */
#endif
#ifndef ECONNABORTED
#   define ECONNABORTED	106	/* Software caused connection abort */
#endif
#ifndef ECONNREFUSED
#   define ECONNREFUSED	107	/* Connection refused */
#endif
#ifndef ECONNRESET
#   define ECONNRESET	108	/* Connection reset by peer */
#endif
#ifndef EDESTADDRREQ
#   define EDESTADDRREQ	109	/* Destination address required */
#endif
#ifndef EHOSTUNREACH
#   define EHOSTUNREACH	110	/* No route to host */
#endif
#ifndef EIDRM
#   define EIDRM	111	/* Identifier removed */
#endif
#ifndef EINPROGRESS
#   define EINPROGRESS	112	/* Operation now in progress */
#endif
#ifndef EISCONN
#   define EISCONN	113	/* Socket is already connected */
#endif
#ifndef ELOOP
#   define ELOOP	114	/* Symbolic link loop */
#endif
#ifndef EMSGSIZE
#   define EMSGSIZE	115	/* Message too long */
#endif
#ifndef ENETDOWN
#   define ENETDOWN	116	/* Network is down */
#endif
#ifndef ENETRESET
#   define ENETRESET	117	/* Network dropped connection on reset */
#endif
#ifndef ENETUNREACH
#   define ENETUNREACH	118	/* Network is unreachable */
#endif
#ifndef ENOBUFS
#   define ENOBUFS	119	/* No buffer space available */
#endif
#ifndef ENODATA
#   define ENODATA	120	/* No data available */
#endif
#ifndef ENOLINK
#   define ENOLINK	121	/* Link has be severed */
#endif
#ifndef ENOMSG
#   define ENOMSG	122	/* No message of desired type */
#endif
#ifndef ENOPROTOOPT
#   define ENOPROTOOPT	123	/* Protocol not available */
#endif
#ifndef ENOSR
#   define ENOSR	124	/* Out of stream resources */
#endif
#ifndef ENOSTR
#   define ENOSTR	125	/* Not a stream device */
#endif
#ifndef ENOTCONN
#   define ENOTCONN	126	/* Socket is not connected */
#endif
#ifndef ENOTRECOVERABLE
#   define ENOTRECOVERABLE	127	/* Not recoverable */
#endif
#ifndef ENOTSOCK
#   define ENOTSOCK	128	/* Socket operation on non-socket */
#endif
#ifndef ENOTSUP
#   define ENOTSUP	129	/* Operation not supported */
#endif
#ifndef EOPNOTSUPP
#   define EOPNOTSUPP	130	/* Operation not supported on socket */
#endif
#ifndef EOTHER
#   define EOTHER	131	/* Other error */
#endif
#ifndef EOVERFLOW
#   define EOVERFLOW	132	/* File too big */
#endif
#ifndef EOWNERDEAD
#   define EOWNERDEAD	133	/* Owner dead */
#endif
#ifndef EPROTO
#   define EPROTO	134	/* Protocol error */
#endif
#ifndef EPROTONOSUPPORT
#   define EPROTONOSUPPORT 135	/* Protocol not supported */
#endif
#ifndef EPROTOTYPE
#   define EPROTOTYPE	136	/* Protocol wrong type for socket */
#endif
#ifndef ETIME
#   define ETIME	137	/* Timer expired */
#endif
#ifndef ETIMEDOUT
#   define ETIMEDOUT	138	/* Connection timed out */
#endif
#ifndef ETXTBSY
#   define ETXTBSY	139	/* Text file or pseudo-device busy */
#endif
#ifndef EWOULDBLOCK
#   define EWOULDBLOCK	140	/* Operation would block */
#endif


/* Visual Studio doesn't have these, so just choose some high numbers */
#ifndef ESOCKTNOSUPPORT
#   define ESOCKTNOSUPPORT 240	/* Socket type not supported */
#endif
#ifndef ESHUTDOWN
#   define ESHUTDOWN	241	/* Can't send after socket shutdown */
#endif
#ifndef ETOOMANYREFS
#   define ETOOMANYREFS	242	/* Too many references: can't splice */
#endif
#ifndef EHOSTDOWN
#   define EHOSTDOWN	243	/* Host is down */
#endif
#ifndef EUSERS
#   define EUSERS	244	/* Too many users (for UFS) */
#endif
#ifndef EDQUOT
#   define EDQUOT	245	/* Disc quota exceeded */
#endif
#ifndef ESTALE
#   define ESTALE	246	/* Stale NFS file handle */
#endif

/*
 * Signals not known to the standard ANSI signal.h.  These are used
 * by Tcl_WaitPid() and generic/tclPosixStr.c
 */

#ifndef SIGTRAP
#   define SIGTRAP  5
#endif
#ifndef SIGBUS
#   define SIGBUS   10
#endif

/*
 * Supply definitions for macros to query wait status, if not already
 * defined in header files above.
 */

#if TCL_UNION_WAIT
#   define WAIT_STATUS_TYPE union wait
#else
#   define WAIT_STATUS_TYPE int
#endif /* TCL_UNION_WAIT */

#ifndef WIFEXITED
#   define WIFEXITED(stat)  (((*((int *) &(stat))) & 0xC0000000) == 0)
#endif

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat) (*((int *) &(stat)))
#endif

#ifndef WIFSIGNALED
#   define WIFSIGNALED(stat) ((*((int *) &(stat))) & 0xC0000000)
#endif

#ifndef WTERMSIG
#   define WTERMSIG(stat)    ((*((int *) &(stat))) & 0x7f)
#endif

#ifndef WIFSTOPPED
#   define WIFSTOPPED(stat)  0
#endif

#ifndef WSTOPSIG
#   define WSTOPSIG(stat)    (((*((int *) &(stat))) >> 8) & 0xff)
#endif

/*
 * Define constants for waitpid() system call if they aren't defined
 * by a system header file.
 */

#ifndef WNOHANG
#   define WNOHANG 1
#endif
#ifndef WUNTRACED
#   define WUNTRACED 2
#endif

/*
 * Define access mode constants if they aren't already defined.
 */

#ifndef F_OK
#    define F_OK 00
#endif
#ifndef X_OK
#    define X_OK 01
#endif
#ifndef W_OK
#    define W_OK 02
#endif
#ifndef R_OK
#    define R_OK 04
#endif

/*
 * Define macros to query file type bits, if they're not already
 * defined.
 */

#ifndef S_IFLNK
#   define S_IFLNK        0120000  /* Symbolic Link */
#endif

/*
 * Windows compilers do not define S_IFBLK. However, Tcl uses it in
 * GetTypeFromMode to identify blockSpecial devices based on the
 * value in the statsbuf st_mode field. We have no other way to pass this
 * from NativeStat on Windows so are forced to define it here.
 * The definition here is essentially what is seen on Linux and MingW.
 * XXX - the root problem is Tcl using Unix definitions instead of
 * abstracting the structure into a platform independent one. Sigh - perhaps
 * Tcl 9
 */
#ifndef S_IFBLK
#   define S_IFBLK (S_IFDIR | S_IFCHR)
#endif

#ifndef S_ISREG
#   ifdef S_IFREG
#       define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#   else
#       define S_ISREG(m) 0
#   endif
#endif /* !S_ISREG */
#ifndef S_ISDIR
#   ifdef S_IFDIR
#       define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#   else
#       define S_ISDIR(m) 0
#   endif
#endif /* !S_ISDIR */
#ifndef S_ISCHR
#   ifdef S_IFCHR
#       define S_ISCHR(m) (((m) & S_IFMT) == S_IFCHR)
#   else
#       define S_ISCHR(m) 0
#   endif
#endif /* !S_ISCHR */
#ifndef S_ISBLK
#   ifdef S_IFBLK
#       define S_ISBLK(m) (((m) & S_IFMT) == S_IFBLK)
#   else
#       define S_ISBLK(m) 0
#   endif
#endif /* !S_ISBLK */
#ifndef S_ISFIFO
#   ifdef S_IFIFO
#       define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#   else
#       define S_ISFIFO(m) 0
#   endif
#endif /* !S_ISFIFO */
#ifndef S_ISLNK
#   ifdef S_IFLNK
#       define S_ISLNK(m) (((m) & S_IFMT) == S_IFLNK)
#   else
#       define S_ISLNK(m) 0
#   endif
#endif /* !S_ISLNK */


/*
 * Define MAXPATHLEN in terms of MAXPATH if available
 */

#ifndef MAXPATH
#   define MAXPATH MAX_PATH
#endif /* MAXPATH */

#ifndef MAXPATHLEN
#   define MAXPATHLEN MAXPATH
#endif /* MAXPATHLEN */

/*
 * Define pid_t and uid_t if they're not already defined.
 */

#if ! TCL_PID_T
#   define pid_t int
#endif /* !TCL_PID_T */
#if ! TCL_UID_T
#   define uid_t int
#endif /* !TCL_UID_T */

/*
 * Visual C++ has some odd names for common functions, so we need to
 * define a few macros to handle them.  Also, it defines EDEADLOCK and
 * EDEADLK as the same value, which confuses Tcl_ErrnoId().
 */

#if defined(_MSC_VER) || defined(__MSVCRT__)
#   define environ _environ
#   if defined(_MSC_VER) && (_MSC_VER < 1600)
#	define hypot _hypot
#   endif
#   define exception _exception
#   undef EDEADLOCK
#   if defined(_MSC_VER) && (_MSC_VER >= 1700)
#	define timezone _timezone
#   endif
#endif /* _MSC_VER || __MSVCRT__ */

/*
 * Borland's timezone and environ functions.
 */

#ifdef  __BORLANDC__
#   define timezone _timezone
#   define environ  _environ
#endif /* __BORLANDC__ */

#ifdef __WATCOMC__
#   if !defined(__CHAR_SIGNED__)
#	error "You must use the -j switch to ensure char is signed."
#   endif
#endif


/*
 * MSVC 8.0 started to mark many standard C library functions depreciated
 * including the *printf family and others. Tell it to shut up.
 * (_MSC_VER is 1200 for VC6, 1300 or 1310 for vc7.net, 1400 for 8.0)
 */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
#   pragma warning(disable:4244)
#   pragma warning(disable:4267)
#   pragma warning(disable:4996)
#endif

/*
 *---------------------------------------------------------------------------
 * The following macros and declarations represent the interface between
 * generic and windows-specific parts of Tcl.  Some of the macros may
 * override functions declared in tclInt.h.
 *---------------------------------------------------------------------------
 */

/*
 * The default platform eol translation on Windows is TCL_TRANSLATE_CRLF:
 */

#define	TCL_PLATFORM_TRANSLATION	TCL_TRANSLATE_CRLF

/*
 * Declare dynamic loading extension macro.
 */

#define TCL_SHLIB_EXT ".dll"

/*
 * The following define ensures that we use the native putenv
 * implementation to modify the environment array.  This keeps
 * the C level environment in synch with the system level environment.
 */

#define USE_PUTENV		1
#define USE_PUTENV_FOR_UNSET	1

/*
 * Msvcrt's putenv() copies the string rather than takes ownership of it.
 */

#if defined(_MSC_VER) || defined(__MSVCRT__)
#   define HAVE_PUTENV_THAT_COPIES 1
#endif

/*
 * Older version of Mingw are known to lack a MWMO_ALERTABLE define.
 */
#if !defined(MWMO_ALERTABLE)
#   define MWMO_ALERTABLE 2
#endif

/*
 * The following defines wrap the system memory allocation routines for
 * use by tclAlloc.c.
 */

#define TclpSysAlloc(size, isBin)	((void*)HeapAlloc(GetProcessHeap(), \
					    (DWORD)0, (DWORD)size))
#define TclpSysFree(ptr)		(HeapFree(GetProcessHeap(), \
					    (DWORD)0, (HGLOBAL)ptr))
#define TclpSysRealloc(ptr, size)	((void*)HeapReAlloc(GetProcessHeap(), \
					    (DWORD)0, (LPVOID)ptr, (DWORD)size))

/* This type is not defined in the Windows headers */
#define socklen_t       int


/*
 * The following macros have trivial definitions, allowing generic code to
 * address platform-specific issues.
 */

#define TclpReleaseFile(file)	ckfree((char *) file)

/*
 * The following macros and declarations wrap the C runtime library
 * functions.
 */

#define TclpExit		exit

#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF
#endif /* INVALID_SET_FILE_POINTER */

#ifndef LABEL_SECURITY_INFORMATION
#   define LABEL_SECURITY_INFORMATION (0x00000010L)
#endif

#define Tcl_DirEntry void
#define TclDIR void

#endif /* _TCLWINPORT */
