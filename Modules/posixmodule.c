
/* POSIX module implementation */

/* This file is also used for Windows NT/MS-Win and OS/2.  In that case the
   module actually calls itself 'nt' or 'os2', not 'posix', and a few
   functions are either unimplemented or implemented differently.  The source
   assumes that for Windows NT, the macro 'MS_WINDOWS' is defined independent
   of the compiler used.  Different compilers define their own feature
   test macro, e.g. '__BORLANDC__' or '_MSC_VER'.  For OS/2, the compiler
   independent macro PYOS_OS2 should be defined.  On OS/2 the default
   compiler is assumed to be IBM's VisualAge C++ (VACPP).  PYCC_GCC is used
   as the compiler specific macro for the EMX port of gcc to OS/2. */

#ifdef __APPLE__
   /*
    * Step 1 of support for weak-linking a number of symbols existing on
    * OSX 10.4 and later, see the comment in the #ifdef __APPLE__ block
    * at the end of this file for more information.
    */
#  pragma weak lchown
#  pragma weak statvfs
#  pragma weak fstatvfs

#endif /* __APPLE__ */

#define PY_SSIZE_T_CLEAN

#include "Python.h"

#if defined(__VMS)
#    error "PEP 11: VMS is now unsupported, code will be removed in Python 3.4"
#    include <unixio.h>
#endif /* defined(__VMS) */

#ifdef __cplusplus
extern "C" {
#endif

PyDoc_STRVAR(posix__doc__,
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the POSIX standard (a thinly\n\
disguised Unix interface).  Refer to the library manual and\n\
corresponding Unix manual entries for more information on calls.");


#if defined(PYOS_OS2)
#error "PEP 11: OS/2 is now unsupported, code will be removed in Python 3.4"
#define  INCL_DOS
#define  INCL_DOSERRORS
#define  INCL_DOSPROCESS
#define  INCL_NOPMAPI
#include <os2.h>
#if defined(PYCC_GCC)
#include <ctype.h>
#include <io.h>
#include <stdio.h>
#include <process.h>
#endif
#include "osdefs.h"
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* For WNOHANG */
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif /* HAVE_SYSEXITS_H */

#ifdef HAVE_SYS_LOADAVG_H
#include <sys/loadavg.h>
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#if !defined(CPU_ALLOC) && defined(HAVE_SCHED_SETAFFINITY)
#undef HAVE_SCHED_SETAFFINITY
#endif

#if defined(HAVE_SYS_XATTR_H) && defined(__GLIBC__)
#define USE_XATTRS
#endif

#ifdef USE_XATTRS
#include <sys/xattr.h>
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#if defined(MS_WINDOWS)
#  define TERMSIZE_USE_CONIO
#elif defined(HAVE_SYS_IOCTL_H)
#  include <sys/ioctl.h>
#  if defined(HAVE_TERMIOS_H)
#    include <termios.h>
#  endif
#  if defined(TIOCGWINSZ)
#    define TERMSIZE_USE_IOCTL
#  endif
#endif /* MS_WINDOWS */

/* Various compilers have only certain posix functions */
/* XXX Gosh I wish these were all moved into pyconfig.h */
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <process.h>
#else
#if defined(__WATCOMC__) && !defined(__QNX__)           /* Watcom compiler */
#define HAVE_GETCWD     1
#define HAVE_OPENDIR    1
#define HAVE_SYSTEM     1
#if defined(__OS2__)
#define HAVE_EXECV      1
#define HAVE_WAIT       1
#endif
#include <process.h>
#else
#ifdef __BORLANDC__             /* Borland compiler */
#define HAVE_EXECV      1
#define HAVE_GETCWD     1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#define HAVE_SYSTEM     1
#define HAVE_WAIT       1
#else
#ifdef _MSC_VER         /* Microsoft compiler */
#define HAVE_GETCWD     1
#define HAVE_GETPPID    1
#define HAVE_GETLOGIN   1
#define HAVE_SPAWNV     1
#define HAVE_EXECV      1
#define HAVE_PIPE       1
#define HAVE_SYSTEM     1
#define HAVE_CWAIT      1
#define HAVE_FSYNC      1
#define fsync _commit
#else
#if defined(PYOS_OS2) && defined(PYCC_GCC) || defined(__VMS)
/* Everything needed is defined in PC/os2emx/pyconfig.h or vms/pyconfig.h */
#else                   /* all other compilers */
/* Unix functions that the configure script doesn't check for */
#define HAVE_EXECV      1
#define HAVE_FORK       1
#if defined(__USLC__) && defined(__SCO_VERSION__)       /* SCO UDK Compiler */
#define HAVE_FORK1      1
#endif
#define HAVE_GETCWD     1
#define HAVE_GETEGID    1
#define HAVE_GETEUID    1
#define HAVE_GETGID     1
#define HAVE_GETPPID    1
#define HAVE_GETUID     1
#define HAVE_KILL       1
#define HAVE_OPENDIR    1
#define HAVE_PIPE       1
#define HAVE_SYSTEM     1
#define HAVE_WAIT       1
#define HAVE_TTYNAME    1
#endif  /* PYOS_OS2 && PYCC_GCC && __VMS */
#endif  /* _MSC_VER */
#endif  /* __BORLANDC__ */
#endif  /* ! __WATCOMC__ || __QNX__ */
#endif /* ! __IBMC__ */




#ifndef _MSC_VER

#if defined(__sgi)&&_COMPILER_VERSION>=700
/* declare ctermid_r if compiling with MIPSPro 7.x in ANSI C mode
   (default) */
extern char        *ctermid_r(char *);
#endif

#ifndef HAVE_UNISTD_H
#if defined(PYCC_VACPP)
extern int mkdir(char *);
#else
#if ( defined(__WATCOMC__) || defined(_MSC_VER) ) && !defined(__QNX__)
extern int mkdir(const char *);
#else
extern int mkdir(const char *, mode_t);
#endif
#endif
#if defined(__IBMC__) || defined(__IBMCPP__)
extern int chdir(char *);
extern int rmdir(char *);
#else
extern int chdir(const char *);
extern int rmdir(const char *);
#endif
#ifdef __BORLANDC__
extern int chmod(const char *, int);
#else
extern int chmod(const char *, mode_t);
#endif
/*#ifdef HAVE_FCHMOD
extern int fchmod(int, mode_t);
#endif*/
/*#ifdef HAVE_LCHMOD
extern int lchmod(const char *, mode_t);
#endif*/
extern int chown(const char *, uid_t, gid_t);
extern char *getcwd(char *, int);
extern char *strerror(int);
extern int link(const char *, const char *);
extern int rename(const char *, const char *);
extern int stat(const char *, struct stat *);
extern int unlink(const char *);
#ifdef HAVE_SYMLINK
extern int symlink(const char *, const char *);
#endif /* HAVE_SYMLINK */
#ifdef HAVE_LSTAT
extern int lstat(const char *, struct stat *);
#endif /* HAVE_LSTAT */
#endif /* !HAVE_UNISTD_H */

#endif /* !_MSC_VER */

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif /* HAVE_UTIME_H */

#ifdef HAVE_SYS_UTIME_H
#include <sys/utime.h>
#define HAVE_UTIME_H /* pretend we do for the rest of this file */
#endif /* HAVE_SYS_UTIME_H */

#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif /* HAVE_SYS_TIMES_H */

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif /* HAVE_SYS_PARAM_H */

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif /* HAVE_SYS_UTSNAME_H */

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#if defined(__WATCOMC__) && !defined(__QNX__)
#include <direct.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#endif
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#ifdef _MSC_VER
#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#ifdef HAVE_PROCESS_H
#include <process.h>
#endif
#ifndef VOLUME_NAME_DOS
#define VOLUME_NAME_DOS 0x0
#endif
#ifndef VOLUME_NAME_NT
#define VOLUME_NAME_NT  0x2
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#endif
#include "osdefs.h"
#include <malloc.h>
#include <windows.h>
#include <shellapi.h>   /* for ShellExecute() */
#include <lmcons.h>     /* for UNLEN */
#ifdef SE_CREATE_SYMBOLIC_LINK_NAME /* Available starting with Vista */
#define HAVE_SYMLINK
static int win32_can_symlink = 0;
#endif
#endif /* _MSC_VER */

#if defined(PYCC_VACPP) && defined(PYOS_OS2)
#include <io.h>
#endif /* OS2 */

#ifndef MAXPATHLEN
#if defined(PATH_MAX) && PATH_MAX > 1024
#define MAXPATHLEN PATH_MAX
#else
#define MAXPATHLEN 1024
#endif
#endif /* MAXPATHLEN */

#ifdef UNION_WAIT
/* Emulate some macros on systems that have a union instead of macros */

#ifndef WIFEXITED
#define WIFEXITED(u_wait) (!(u_wait).w_termsig && !(u_wait).w_coredump)
#endif

#ifndef WEXITSTATUS
#define WEXITSTATUS(u_wait) (WIFEXITED(u_wait)?((u_wait).w_retcode):-1)
#endif

#ifndef WTERMSIG
#define WTERMSIG(u_wait) ((u_wait).w_termsig)
#endif

#define WAIT_TYPE union wait
#define WAIT_STATUS_INT(s) (s.w_status)

#else /* !UNION_WAIT */
#define WAIT_TYPE int
#define WAIT_STATUS_INT(s) (s)
#endif /* UNION_WAIT */

/* Don't use the "_r" form if we don't need it (also, won't have a
   prototype for it, at least on Solaris -- maybe others as well?). */
#if defined(HAVE_CTERMID_R) && defined(WITH_THREAD)
#define USE_CTERMID_R
#endif

/* choose the appropriate stat and fstat functions and return structs */
#undef STAT
#undef FSTAT
#undef STRUCT_STAT
#if defined(MS_WIN64) || defined(MS_WINDOWS)
#       define STAT win32_stat
#       define LSTAT win32_lstat
#       define FSTAT win32_fstat
#       define STRUCT_STAT struct win32_stat
#else
#       define STAT stat
#       define LSTAT lstat
#       define FSTAT fstat
#       define STRUCT_STAT struct stat
#endif

#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#else
#if defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_SYS_MKDEV_H)
#include <sys/mkdev.h>
#endif
#endif


#ifdef MS_WINDOWS
static int
win32_warn_bytes_api()
{
    return PyErr_WarnEx(PyExc_DeprecationWarning,
        "The Windows bytes API has been deprecated, "
        "use Unicode filenames instead",
        1);
}
#endif


#ifdef AT_FDCWD
#define DEFAULT_DIR_FD AT_FDCWD
#else
#define DEFAULT_DIR_FD (-100)
#endif

static int
_fd_converter(PyObject *o, int *p, int default_value) {
    long long_value;
    if (o == Py_None) {
        *p = default_value;
        return 1;
    }
    if (PyFloat_Check(o)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        return 0;
    }
    long_value = PyLong_AsLong(o);
    if (long_value == -1 && PyErr_Occurred())
        return 0;
    if (long_value > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "signed integer is greater than maximum");
        return 0;
    }
    if (long_value < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError,
                        "signed integer is less than minimum");
        return 0;
    }
    *p = (int)long_value;
    return 1;
}

static int
dir_fd_converter(PyObject *o, void *p) {
    return _fd_converter(o, (int *)p, DEFAULT_DIR_FD);
}



/*
 * A PyArg_ParseTuple "converter" function
 * that handles filesystem paths in the manner
 * preferred by the os module.
 *
 * path_converter accepts (Unicode) strings and their
 * subclasses, and bytes and their subclasses.  What
 * it does with the argument depends on the platform:
 *
 *   * On Windows, if we get a (Unicode) string we
 *     extract the wchar_t * and return it; if we get
 *     bytes we extract the char * and return that.
 *
 *   * On all other platforms, strings are encoded
 *     to bytes using PyUnicode_FSConverter, then we
 *     extract the char * from the bytes object and
 *     return that.
 *
 * path_converter also optionally accepts signed
 * integers (representing open file descriptors) instead
 * of path strings.
 *
 * Input fields:
 *   path.nullable
 *     If nonzero, the path is permitted to be None.
 *   path.allow_fd
 *     If nonzero, the path is permitted to be a file handle
 *     (a signed int) instead of a string.
 *   path.function_name
 *     If non-NULL, path_converter will use that as the name
 *     of the function in error messages.
 *     (If path.argument_name is NULL it omits the function name.)
 *   path.argument_name
 *     If non-NULL, path_converter will use that as the name
 *     of the parameter in error messages.
 *     (If path.argument_name is NULL it uses "path".)
 *
 * Output fields:
 *   path.wide
 *     Points to the path if it was expressed as Unicode
 *     and was not encoded.  (Only used on Windows.)
 *   path.narrow
 *     Points to the path if it was expressed as bytes,
 *     or it was Unicode and was encoded to bytes.
 *   path.fd
 *     Contains a file descriptor if path.accept_fd was true
 *     and the caller provided a signed integer instead of any
 *     sort of string.
 *
 *     WARNING: if your "path" parameter is optional, and is
 *     unspecified, path_converter will never get called.
 *     So if you set allow_fd, you *MUST* initialize path.fd = -1
 *     yourself!
 *   path.length
 *     The length of the path in characters, if specified as
 *     a string.
 *   path.object
 *     The original object passed in.
 *   path.cleanup
 *     For internal use only.  May point to a temporary object.
 *     (Pay no attention to the man behind the curtain.)
 *
 *   At most one of path.wide or path.narrow will be non-NULL.
 *   If path was None and path.nullable was set,
 *     or if path was an integer and path.allow_fd was set,
 *     both path.wide and path.narrow will be NULL
 *     and path.length will be 0.
 *   
 *   path_converter takes care to not write to the path_t
 *   unless it's successful.  However it must reset the
 *   "cleanup" field each time it's called.
 *
 * Use as follows:
 *      path_t path;
 *      memset(&path, 0, sizeof(path));
 *      PyArg_ParseTuple(args, "O&", path_converter, &path);
 *      // ... use values from path ...
 *      path_cleanup(&path);
 *
 * (Note that if PyArg_Parse fails you don't need to call
 * path_cleanup().  However it is safe to do so.)
 */
typedef struct {
    char *function_name;
    char *argument_name;
    int nullable;
    int allow_fd;
    wchar_t *wide;
    char *narrow;
    int fd;
    Py_ssize_t length;
    PyObject *object;
    PyObject *cleanup;
} path_t;

static void
path_cleanup(path_t *path) {
    if (path->cleanup) {
        Py_DECREF(path->cleanup);
        path->cleanup = NULL;
    }
}

static int
path_converter(PyObject *o, void *p) {
    path_t *path = (path_t *)p;
    PyObject *unicode, *bytes;
    Py_ssize_t length;
    char *narrow;

#define FORMAT_EXCEPTION(exc, fmt) \
    PyErr_Format(exc, "%s%s" fmt, \
        path->function_name ? path->function_name : "", \
        path->function_name ? ": "                : "", \
        path->argument_name ? path->argument_name : "path")

    /* Py_CLEANUP_SUPPORTED support */
    if (o == NULL) {
        path_cleanup(path);
        return 1;
    }

    /* ensure it's always safe to call path_cleanup() */
    path->cleanup = NULL;

    if (o == Py_None) {
        if (!path->nullable) {
            FORMAT_EXCEPTION(PyExc_TypeError,
                             "can't specify None for %s argument");
            return 0;
        }
        path->wide = NULL;
        path->narrow = NULL;
        path->length = 0;
        path->object = o;
        path->fd = -1;
        return 1;
    }

    unicode = PyUnicode_FromObject(o);
    if (unicode) {
#ifdef MS_WINDOWS
        wchar_t *wide;
        length = PyUnicode_GET_SIZE(unicode);
        if (length > 32767) {
            FORMAT_EXCEPTION(PyExc_ValueError, "%s too long for Windows");
            Py_DECREF(unicode);
            return 0;
        }

        wide = PyUnicode_AsUnicode(unicode);
        if (!wide) {
            Py_DECREF(unicode);
            return 0;
        }

        path->wide = wide;
        path->narrow = NULL;
        path->length = length;
        path->object = o;
        path->fd = -1;
        path->cleanup = unicode;
        return Py_CLEANUP_SUPPORTED;
#else
        int converted = PyUnicode_FSConverter(unicode, &bytes);
        Py_DECREF(unicode);
        if (!converted)
            bytes = NULL;
#endif
    }
    else {
        PyErr_Clear();
        bytes = PyBytes_FromObject(o);
        if (!bytes) {
            PyErr_Clear();
            if (path->allow_fd) {
                int fd;
                /*
                 * note: _fd_converter always permits None.
                 * but we've already done our None check.
                 * so o cannot be None at this point.
                 */
                int result = _fd_converter(o, &fd, -1);
                if (result) {
                    path->wide = NULL;
                    path->narrow = NULL;
                    path->length = 0;
                    path->object = o;
                    path->fd = fd;
                    return result;
                }
            }
        }
    }

    if (!bytes) {
        if (!PyErr_Occurred())
            FORMAT_EXCEPTION(PyExc_TypeError, "illegal type for %s parameter");
        return 0;
    }

#ifdef MS_WINDOWS
    if (win32_warn_bytes_api()) {
        Py_DECREF(bytes);
        return 0;
    }
#endif

    length = PyBytes_GET_SIZE(bytes);
#ifdef MS_WINDOWS
    if (length > MAX_PATH) {
        FORMAT_EXCEPTION(PyExc_ValueError, "%s too long for Windows");
        Py_DECREF(bytes);
        return 0;
    }
#endif

    narrow = PyBytes_AS_STRING(bytes);
    if (length != strlen(narrow)) {
        FORMAT_EXCEPTION(PyExc_ValueError, "embedded NUL character in %s");
        Py_DECREF(bytes);
        return 0;
    }

    path->wide = NULL;
    path->narrow = narrow;
    path->length = length;
    path->object = o;
    path->fd = -1;
    path->cleanup = bytes;
    return Py_CLEANUP_SUPPORTED;
}

static void
argument_unavailable_error(char *function_name, char *argument_name) {
    PyErr_Format(PyExc_NotImplementedError,
        "%s%s%s unavailable on this platform",
        (function_name != NULL) ? function_name : "",
        (function_name != NULL) ? ": ": "",
        argument_name);
}

static int
dir_fd_unavailable(PyObject *o, void *p) {
    int *dir_fd = (int *)p;
    int return_value = _fd_converter(o, dir_fd, DEFAULT_DIR_FD);
    if (!return_value)
        return 0;
    if (*dir_fd == DEFAULT_DIR_FD)
        return 1;
    argument_unavailable_error(NULL, "dir_fd");
    return 0;
}

static int
fd_specified(char *function_name, int fd) {
    if (fd == -1)
        return 0;

    argument_unavailable_error(function_name, "fd");
    return 1;
}

static int
follow_symlinks_specified(char *function_name, int follow_symlinks) {
    if (follow_symlinks)
        return 0;

    argument_unavailable_error(function_name, "follow_symlinks");
    return 1;
}

static int
path_and_dir_fd_invalid(char *function_name, path_t *path, int dir_fd) {
    if (!path->narrow && !path->wide && (dir_fd != DEFAULT_DIR_FD)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: can't specify dir_fd without matching path",
                     function_name);
        return 1;
    }
    return 0;
}

static int
dir_fd_and_fd_invalid(char *function_name, int dir_fd, int fd) {
    if ((dir_fd != DEFAULT_DIR_FD) && (fd != -1)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: can't specify both dir_fd and fd",
                     function_name);
        return 1;
    }
    return 0;
}

static int
fd_and_follow_symlinks_invalid(char *function_name, int fd,
                               int follow_symlinks) {
    if ((fd > 0) && (!follow_symlinks)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: cannot use fd and follow_symlinks together",
                     function_name);
        return 1;
    }
    return 0;
}

static int
dir_fd_and_follow_symlinks_invalid(char *function_name, int dir_fd,
                                   int follow_symlinks) {
    if ((dir_fd != DEFAULT_DIR_FD) && (!follow_symlinks)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: cannot use dir_fd and follow_symlinks together",
                     function_name);
        return 1;
    }
    return 0;
}

/* A helper used by a number of POSIX-only functions */
#ifndef MS_WINDOWS
static int
_parse_off_t(PyObject* arg, void* addr)
{
#if !defined(HAVE_LARGEFILE_SUPPORT)
    *((off_t*)addr) = PyLong_AsLong(arg);
#else
    *((off_t*)addr) = PyLong_AsLongLong(arg);
#endif
    if (PyErr_Occurred())
        return 0;
    return 1;
}
#endif

#if defined _MSC_VER && _MSC_VER >= 1400
/* Microsoft CRT in VS2005 and higher will verify that a filehandle is
 * valid and throw an assertion if it isn't.
 * Normally, an invalid fd is likely to be a C program error and therefore
 * an assertion can be useful, but it does contradict the POSIX standard
 * which for write(2) states:
 *    "Otherwise, -1 shall be returned and errno set to indicate the error."
 *    "[EBADF] The fildes argument is not a valid file descriptor open for
 *     writing."
 * Furthermore, python allows the user to enter any old integer
 * as a fd and should merely raise a python exception on error.
 * The Microsoft CRT doesn't provide an official way to check for the
 * validity of a file descriptor, but we can emulate its internal behaviour
 * by using the exported __pinfo data member and knowledge of the
 * internal structures involved.
 * The structures below must be updated for each version of visual studio
 * according to the file internal.h in the CRT source, until MS comes
 * up with a less hacky way to do this.
 * (all of this is to avoid globally modifying the CRT behaviour using
 * _set_invalid_parameter_handler() and _CrtSetReportMode())
 */
/* The actual size of the structure is determined at runtime.
 * Only the first items must be present.
 */
typedef struct {
    intptr_t osfhnd;
    char osfile;
} my_ioinfo;

extern __declspec(dllimport) char * __pioinfo[];
#define IOINFO_L2E 5
#define IOINFO_ARRAY_ELTS   (1 << IOINFO_L2E)
#define IOINFO_ARRAYS 64
#define _NHANDLE_           (IOINFO_ARRAYS * IOINFO_ARRAY_ELTS)
#define FOPEN 0x01
#define _NO_CONSOLE_FILENO (intptr_t)-2

/* This function emulates what the windows CRT does to validate file handles */
int
_PyVerify_fd(int fd)
{
    const int i1 = fd >> IOINFO_L2E;
    const int i2 = fd & ((1 << IOINFO_L2E) - 1);

    static size_t sizeof_ioinfo = 0;

    /* Determine the actual size of the ioinfo structure,
     * as used by the CRT loaded in memory
     */
    if (sizeof_ioinfo == 0 && __pioinfo[0] != NULL) {
        sizeof_ioinfo = _msize(__pioinfo[0]) / IOINFO_ARRAY_ELTS;
    }
    if (sizeof_ioinfo == 0) {
        /* This should not happen... */
        goto fail;
    }

    /* See that it isn't a special CLEAR fileno */
    if (fd != _NO_CONSOLE_FILENO) {
        /* Microsoft CRT would check that 0<=fd<_nhandle but we can't do that.  Instead
         * we check pointer validity and other info
         */
        if (0 <= i1 && i1 < IOINFO_ARRAYS && __pioinfo[i1] != NULL) {
            /* finally, check that the file is open */
            my_ioinfo* info = (my_ioinfo*)(__pioinfo[i1] + i2 * sizeof_ioinfo);
            if (info->osfile & FOPEN) {
                return 1;
            }
        }
    }
  fail:
    errno = EBADF;
    return 0;
}

/* the special case of checking dup2.  The target fd must be in a sensible range */
static int
_PyVerify_fd_dup2(int fd1, int fd2)
{
    if (!_PyVerify_fd(fd1))
        return 0;
    if (fd2 == _NO_CONSOLE_FILENO)
        return 0;
    if ((unsigned)fd2 < _NHANDLE_)
        return 1;
    else
        return 0;
}
#else
/* dummy version. _PyVerify_fd() is already defined in fileobject.h */
#define _PyVerify_fd_dup2(A, B) (1)
#endif

#ifdef MS_WINDOWS
/* The following structure was copied from
   http://msdn.microsoft.com/en-us/library/ms791514.aspx as the required
   include doesn't seem to be present in the Windows SDK (at least as included
   with Visual Studio Express). */
typedef struct _REPARSE_DATA_BUFFER {
    ULONG ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG Flags;
            WCHAR PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct {
            USHORT SubstituteNameOffset;
            USHORT  SubstituteNameLength;
            USHORT  PrintNameOffset;
            USHORT  PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;

        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER,\
                                                      GenericReparseBuffer)
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  ( 16 * 1024 )

static int
win32_get_reparse_tag(HANDLE reparse_point_handle, ULONG *reparse_tag)
{
    char target_buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER *)target_buffer;
    DWORD n_bytes_returned;

    if (0 == DeviceIoControl(
        reparse_point_handle,
        FSCTL_GET_REPARSE_POINT,
        NULL, 0, /* in buffer */
        target_buffer, sizeof(target_buffer),
        &n_bytes_returned,
        NULL)) /* we're not using OVERLAPPED_IO */
        return FALSE;

    if (reparse_tag)
        *reparse_tag = rdb->ReparseTag;

    return TRUE;
}

#endif /* MS_WINDOWS */

/* Return a dictionary corresponding to the POSIX environment table */
#ifdef WITH_NEXT_FRAMEWORK
/* On Darwin/MacOSX a shared library or framework has no access to
** environ directly, we must obtain it with _NSGetEnviron().
*/
#include <crt_externs.h>
static char **environ;
#elif !defined(_MSC_VER) && ( !defined(__WATCOMC__) || defined(__QNX__) )
extern char **environ;
#endif /* !_MSC_VER */

static PyObject *
convertenviron(void)
{
    PyObject *d;
#ifdef MS_WINDOWS
    wchar_t **e;
#else
    char **e;
#endif
#if defined(PYOS_OS2)
    APIRET rc;
    char   buffer[1024]; /* OS/2 Provides a Documented Max of 1024 Chars */
#endif

    d = PyDict_New();
    if (d == NULL)
        return NULL;
#ifdef WITH_NEXT_FRAMEWORK
    if (environ == NULL)
        environ = *_NSGetEnviron();
#endif
#ifdef MS_WINDOWS
    /* _wenviron must be initialized in this way if the program is started
       through main() instead of wmain(). */
    _wgetenv(L"");
    if (_wenviron == NULL)
        return d;
    /* This part ignores errors */
    for (e = _wenviron; *e != NULL; e++) {
        PyObject *k;
        PyObject *v;
        wchar_t *p = wcschr(*e, L'=');
        if (p == NULL)
            continue;
        k = PyUnicode_FromWideChar(*e, (Py_ssize_t)(p-*e));
        if (k == NULL) {
            PyErr_Clear();
            continue;
        }
        v = PyUnicode_FromWideChar(p+1, wcslen(p+1));
        if (v == NULL) {
            PyErr_Clear();
            Py_DECREF(k);
            continue;
        }
        if (PyDict_GetItem(d, k) == NULL) {
            if (PyDict_SetItem(d, k, v) != 0)
                PyErr_Clear();
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
#else
    if (environ == NULL)
        return d;
    /* This part ignores errors */
    for (e = environ; *e != NULL; e++) {
        PyObject *k;
        PyObject *v;
        char *p = strchr(*e, '=');
        if (p == NULL)
            continue;
        k = PyBytes_FromStringAndSize(*e, (int)(p-*e));
        if (k == NULL) {
            PyErr_Clear();
            continue;
        }
        v = PyBytes_FromStringAndSize(p+1, strlen(p+1));
        if (v == NULL) {
            PyErr_Clear();
            Py_DECREF(k);
            continue;
        }
        if (PyDict_GetItem(d, k) == NULL) {
            if (PyDict_SetItem(d, k, v) != 0)
                PyErr_Clear();
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
#endif
#if defined(PYOS_OS2)
    rc = DosQueryExtLIBPATH(buffer, BEGIN_LIBPATH);
    if (rc == NO_ERROR) { /* (not a type, envname is NOT 'BEGIN_LIBPATH') */
        PyObject *v = PyBytes_FromString(buffer);
        PyDict_SetItemString(d, "BEGINLIBPATH", v);
        Py_DECREF(v);
    }
    rc = DosQueryExtLIBPATH(buffer, END_LIBPATH);
    if (rc == NO_ERROR) { /* (not a typo, envname is NOT 'END_LIBPATH') */
        PyObject *v = PyBytes_FromString(buffer);
        PyDict_SetItemString(d, "ENDLIBPATH", v);
        Py_DECREF(v);
    }
#endif
    return d;
}

/* Set a POSIX-specific error from errno, and return NULL */

static PyObject *
posix_error(void)
{
    return PyErr_SetFromErrno(PyExc_OSError);
}
static PyObject *
posix_error_with_filename(char* name)
{
    return PyErr_SetFromErrnoWithFilename(PyExc_OSError, name);
}


static PyObject *
posix_error_with_allocated_filename(PyObject* name)
{
    PyObject *name_str, *rc;
    name_str = PyUnicode_DecodeFSDefaultAndSize(PyBytes_AsString(name),
                                                PyBytes_GET_SIZE(name));
    Py_DECREF(name);
    rc = PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError,
                                              name_str);
    Py_XDECREF(name_str);
    return rc;
}

#ifdef MS_WINDOWS
static PyObject *
win32_error(char* function, const char* filename)
{
    /* XXX We should pass the function name along in the future.
       (winreg.c also wants to pass the function name.)
       This would however require an additional param to the
       Windows error object, which is non-trivial.
    */
    errno = GetLastError();
    if (filename)
        return PyErr_SetFromWindowsErrWithFilename(errno, filename);
    else
        return PyErr_SetFromWindowsErr(errno);
}

static PyObject *
win32_error_unicode(char* function, wchar_t* filename)
{
    /* XXX - see win32_error for comments on 'function' */
    errno = GetLastError();
    if (filename)
        return PyErr_SetFromWindowsErrWithUnicodeFilename(errno, filename);
    else
        return PyErr_SetFromWindowsErr(errno);
}

static PyObject *
win32_error_object(char* function, PyObject* filename)
{
    /* XXX - see win32_error for comments on 'function' */
    errno = GetLastError();
    if (filename)
        return PyErr_SetExcFromWindowsErrWithFilenameObject(
                    PyExc_WindowsError,
                    errno,
                    filename);
    else
        return PyErr_SetFromWindowsErr(errno);
}

#endif /* MS_WINDOWS */

/*
 * Some functions return Win32 errors, others only ever use posix_error
 * (this is for backwards compatibility with exceptions)
 */
static PyObject *
path_posix_error(char *function_name, path_t *path)
{
    if (path->narrow)
        return posix_error_with_filename(path->narrow);
    return posix_error();
}

static PyObject *
path_error(char *function_name, path_t *path)
{
#ifdef MS_WINDOWS
    if (path->narrow)
        return win32_error(function_name, path->narrow);
    if (path->wide)
        return win32_error_unicode(function_name, path->wide);
    return win32_error(function_name, NULL);
#else
    return path_posix_error(function_name, path);
#endif
}

#if defined(PYOS_OS2)
/**********************************************************************
 *         Helper Function to Trim and Format OS/2 Messages
 **********************************************************************/
static void
os2_formatmsg(char *msgbuf, int msglen, char *reason)
{
    msgbuf[msglen] = '\0'; /* OS/2 Doesn't Guarantee a Terminator */

    if (strlen(msgbuf) > 0) { /* If Non-Empty Msg, Trim CRLF */
        char *lastc = &msgbuf[ strlen(msgbuf)-1 ];

        while (lastc > msgbuf && isspace(Py_CHARMASK(*lastc)))
            *lastc-- = '\0'; /* Trim Trailing Whitespace (CRLF) */
    }

    /* Add Optional Reason Text */
    if (reason) {
        strcat(msgbuf, " : ");
        strcat(msgbuf, reason);
    }
}

/**********************************************************************
 *             Decode an OS/2 Operating System Error Code
 *
 * A convenience function to lookup an OS/2 error code and return a
 * text message we can use to raise a Python exception.
 *
 * Notes:
 *   The messages for errors returned from the OS/2 kernel reside in
 *   the file OSO001.MSG in the \OS2 directory hierarchy.
 *
 **********************************************************************/
static char *
os2_strerror(char *msgbuf, int msgbuflen, int errorcode, char *reason)
{
    APIRET rc;
    ULONG  msglen;

    /* Retrieve Kernel-Related Error Message from OSO001.MSG File */
    Py_BEGIN_ALLOW_THREADS
    rc = DosGetMessage(NULL, 0, msgbuf, msgbuflen,
                       errorcode, "oso001.msg", &msglen);
    Py_END_ALLOW_THREADS

    if (rc == NO_ERROR)
        os2_formatmsg(msgbuf, msglen, reason);
    else
        PyOS_snprintf(msgbuf, msgbuflen,
                      "unknown OS error #%d", errorcode);

    return msgbuf;
}

/* Set an OS/2-specific error and return NULL.  OS/2 kernel
   errors are not in a global variable e.g. 'errno' nor are
   they congruent with posix error numbers. */

static PyObject *
os2_error(int code)
{
    char text[1024];
    PyObject *v;

    os2_strerror(text, sizeof(text), code, "");

    v = Py_BuildValue("(is)", code, text);
    if (v != NULL) {
        PyErr_SetObject(PyExc_OSError, v);
        Py_DECREF(v);
    }
    return NULL; /* Signal to Python that an Exception is Pending */
}

#endif /* OS2 */

/* POSIX generic methods */

static PyObject *
posix_fildes(PyObject *fdobj, int (*func)(int))
{
    int fd;
    int res;
    fd = PyObject_AsFileDescriptor(fdobj);
    if (fd < 0)
        return NULL;
    if (!_PyVerify_fd(fd))
        return posix_error();
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
posix_1str(PyObject *args, char *format, int (*func)(const char*))
{
    PyObject *opath1 = NULL;
    char *path1;
    int res;
    if (!PyArg_ParseTuple(args, format,
                          PyUnicode_FSConverter, &opath1))
        return NULL;
    path1 = PyBytes_AsString(opath1);
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(path1);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error_with_allocated_filename(opath1);
    Py_DECREF(opath1);
    Py_INCREF(Py_None);
    return Py_None;
}


#ifdef MS_WINDOWS
static PyObject*
win32_1str(PyObject* args, char* func,
           char* format, BOOL (__stdcall *funcA)(LPCSTR),
           char* wformat, BOOL (__stdcall *funcW)(LPWSTR))
{
    PyObject *uni;
    const char *ansi;
    BOOL result;

    if (PyArg_ParseTuple(args, wformat, &uni))
    {
        wchar_t *wstr = PyUnicode_AsUnicode(uni);
        if (wstr == NULL)
            return NULL;
        Py_BEGIN_ALLOW_THREADS
        result = funcW(wstr);
        Py_END_ALLOW_THREADS
        if (!result)
            return win32_error_object(func, uni);
        Py_INCREF(Py_None);
        return Py_None;
    }
    PyErr_Clear();

    if (!PyArg_ParseTuple(args, format, &ansi))
        return NULL;
    if (win32_warn_bytes_api())
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    result = funcA(ansi);
    Py_END_ALLOW_THREADS
    if (!result)
        return win32_error(func, ansi);
    Py_INCREF(Py_None);
    return Py_None;

}

/* This is a reimplementation of the C library's chdir function,
   but one that produces Win32 errors instead of DOS error codes.
   chdir is essentially a wrapper around SetCurrentDirectory; however,
   it also needs to set "magic" environment variables indicating
   the per-drive current directory, which are of the form =<drive>: */
static BOOL __stdcall
win32_chdir(LPCSTR path)
{
    char new_path[MAX_PATH+1];
    int result;
    char env[4] = "=x:";

    if(!SetCurrentDirectoryA(path))
        return FALSE;
    result = GetCurrentDirectoryA(MAX_PATH+1, new_path);
    if (!result)
        return FALSE;
    /* In the ANSI API, there should not be any paths longer
       than MAX_PATH. */
    assert(result <= MAX_PATH+1);
    if (strncmp(new_path, "\\\\", 2) == 0 ||
        strncmp(new_path, "//", 2) == 0)
        /* UNC path, nothing to do. */
        return TRUE;
    env[1] = new_path[0];
    return SetEnvironmentVariableA(env, new_path);
}

/* The Unicode version differs from the ANSI version
   since the current directory might exceed MAX_PATH characters */
static BOOL __stdcall
win32_wchdir(LPCWSTR path)
{
    wchar_t _new_path[MAX_PATH+1], *new_path = _new_path;
    int result;
    wchar_t env[4] = L"=x:";

    if(!SetCurrentDirectoryW(path))
        return FALSE;
    result = GetCurrentDirectoryW(MAX_PATH+1, new_path);
    if (!result)
        return FALSE;
    if (result > MAX_PATH+1) {
        new_path = malloc(result * sizeof(wchar_t));
        if (!new_path) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        result = GetCurrentDirectoryW(result, new_path);
        if (!result) {
            free(new_path);
            return FALSE;
        }
    }
    if (wcsncmp(new_path, L"\\\\", 2) == 0 ||
        wcsncmp(new_path, L"//", 2) == 0)
        /* UNC path, nothing to do. */
        return TRUE;
    env[1] = new_path[0];
    result = SetEnvironmentVariableW(env, new_path);
    if (new_path != _new_path)
        free(new_path);
    return result;
}
#endif

#ifdef MS_WINDOWS
/* The CRT of Windows has a number of flaws wrt. its stat() implementation:
   - time stamps are restricted to second resolution
   - file modification times suffer from forth-and-back conversions between
     UTC and local time
   Therefore, we implement our own stat, based on the Win32 API directly.
*/
#define HAVE_STAT_NSEC 1

struct win32_stat{
    int st_dev;
    __int64 st_ino;
    unsigned short st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    int st_rdev;
    __int64 st_size;
    time_t st_atime;
    int st_atime_nsec;
    time_t st_mtime;
    int st_mtime_nsec;
    time_t st_ctime;
    int st_ctime_nsec;
};

static __int64 secs_between_epochs = 11644473600; /* Seconds between 1.1.1601 and 1.1.1970 */

static void
FILE_TIME_to_time_t_nsec(FILETIME *in_ptr, time_t *time_out, int* nsec_out)
{
    /* XXX endianness. Shouldn't matter, as all Windows implementations are little-endian */
    /* Cannot simply cast and dereference in_ptr,
       since it might not be aligned properly */
    __int64 in;
    memcpy(&in, in_ptr, sizeof(in));
    *nsec_out = (int)(in % 10000000) * 100; /* FILETIME is in units of 100 nsec. */
    *time_out = Py_SAFE_DOWNCAST((in / 10000000) - secs_between_epochs, __int64, time_t);
}

static void
time_t_to_FILE_TIME(time_t time_in, int nsec_in, FILETIME *out_ptr)
{
    /* XXX endianness */
    __int64 out;
    out = time_in + secs_between_epochs;
    out = out * 10000000 + nsec_in / 100;
    memcpy(out_ptr, &out, sizeof(out));
}

/* Below, we *know* that ugo+r is 0444 */
#if _S_IREAD != 0400
#error Unsupported C library
#endif
static int
attributes_to_mode(DWORD attr)
{
    int m = 0;
    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        m |= _S_IFDIR | 0111; /* IFEXEC for user,group,other */
    else
        m |= _S_IFREG;
    if (attr & FILE_ATTRIBUTE_READONLY)
        m |= 0444;
    else
        m |= 0666;
    return m;
}

static int
attribute_data_to_stat(BY_HANDLE_FILE_INFORMATION *info, ULONG reparse_tag, struct win32_stat *result)
{
    memset(result, 0, sizeof(*result));
    result->st_mode = attributes_to_mode(info->dwFileAttributes);
    result->st_size = (((__int64)info->nFileSizeHigh)<<32) + info->nFileSizeLow;
    FILE_TIME_to_time_t_nsec(&info->ftCreationTime, &result->st_ctime, &result->st_ctime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastWriteTime, &result->st_mtime, &result->st_mtime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastAccessTime, &result->st_atime, &result->st_atime_nsec);
    result->st_nlink = info->nNumberOfLinks;
    result->st_ino = (((__int64)info->nFileIndexHigh)<<32) + info->nFileIndexLow;
    if (reparse_tag == IO_REPARSE_TAG_SYMLINK) {
        /* first clear the S_IFMT bits */
        result->st_mode ^= (result->st_mode & 0170000);
        /* now set the bits that make this a symlink */
        result->st_mode |= 0120000;
    }

    return 0;
}

static BOOL
attributes_from_dir(LPCSTR pszFile, BY_HANDLE_FILE_INFORMATION *info, ULONG *reparse_tag)
{
    HANDLE hFindFile;
    WIN32_FIND_DATAA FileData;
    hFindFile = FindFirstFileA(pszFile, &FileData);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(hFindFile);
    memset(info, 0, sizeof(*info));
    *reparse_tag = 0;
    info->dwFileAttributes = FileData.dwFileAttributes;
    info->ftCreationTime   = FileData.ftCreationTime;
    info->ftLastAccessTime = FileData.ftLastAccessTime;
    info->ftLastWriteTime  = FileData.ftLastWriteTime;
    info->nFileSizeHigh    = FileData.nFileSizeHigh;
    info->nFileSizeLow     = FileData.nFileSizeLow;
/*  info->nNumberOfLinks   = 1; */
    if (FileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        *reparse_tag = FileData.dwReserved0;
    return TRUE;
}

static BOOL
attributes_from_dir_w(LPCWSTR pszFile, BY_HANDLE_FILE_INFORMATION *info, ULONG *reparse_tag)
{
    HANDLE hFindFile;
    WIN32_FIND_DATAW FileData;
    hFindFile = FindFirstFileW(pszFile, &FileData);
    if (hFindFile == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(hFindFile);
    memset(info, 0, sizeof(*info));
    *reparse_tag = 0;
    info->dwFileAttributes = FileData.dwFileAttributes;
    info->ftCreationTime   = FileData.ftCreationTime;
    info->ftLastAccessTime = FileData.ftLastAccessTime;
    info->ftLastWriteTime  = FileData.ftLastWriteTime;
    info->nFileSizeHigh    = FileData.nFileSizeHigh;
    info->nFileSizeLow     = FileData.nFileSizeLow;
/*  info->nNumberOfLinks   = 1; */
    if (FileData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        *reparse_tag = FileData.dwReserved0;
    return TRUE;
}

/* Grab GetFinalPathNameByHandle dynamically from kernel32 */
static int has_GetFinalPathNameByHandle = 0;
static DWORD (CALLBACK *Py_GetFinalPathNameByHandleW)(HANDLE, LPWSTR, DWORD,
                                                      DWORD);
static int
check_GetFinalPathNameByHandle()
{
    HINSTANCE hKernel32;
    DWORD (CALLBACK *Py_GetFinalPathNameByHandleA)(HANDLE, LPSTR, DWORD,
                                                   DWORD);

    /* only recheck */
    if (!has_GetFinalPathNameByHandle)
    {
        hKernel32 = GetModuleHandleW(L"KERNEL32");
        *(FARPROC*)&Py_GetFinalPathNameByHandleA = GetProcAddress(hKernel32,
                                                "GetFinalPathNameByHandleA");
        *(FARPROC*)&Py_GetFinalPathNameByHandleW = GetProcAddress(hKernel32,
                                                "GetFinalPathNameByHandleW");
        has_GetFinalPathNameByHandle = Py_GetFinalPathNameByHandleA &&
                                       Py_GetFinalPathNameByHandleW;
    }
    return has_GetFinalPathNameByHandle;
}

static BOOL
get_target_path(HANDLE hdl, wchar_t **target_path)
{
    int buf_size, result_length;
    wchar_t *buf;

    /* We have a good handle to the target, use it to determine
       the target path name (then we'll call lstat on it). */
    buf_size = Py_GetFinalPathNameByHandleW(hdl, 0, 0,
                                            VOLUME_NAME_DOS);
    if(!buf_size)
        return FALSE;

    buf = (wchar_t *)malloc((buf_size+1)*sizeof(wchar_t));
    if (!buf) {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    result_length = Py_GetFinalPathNameByHandleW(hdl,
                       buf, buf_size, VOLUME_NAME_DOS);

    if(!result_length) {
        free(buf);
        return FALSE;
    }

    if(!CloseHandle(hdl)) {
        free(buf);
        return FALSE;
    }

    buf[result_length] = 0;

    *target_path = buf;
    return TRUE;
}

static int
win32_xstat_impl_w(const wchar_t *path, struct win32_stat *result,
                   BOOL traverse);
static int
win32_xstat_impl(const char *path, struct win32_stat *result,
                 BOOL traverse)
{
    int code;
    HANDLE hFile, hFile2;
    BY_HANDLE_FILE_INFORMATION info;
    ULONG reparse_tag = 0;
    wchar_t *target_path;
    const char *dot;

    if(!check_GetFinalPathNameByHandle()) {
        /* If the OS doesn't have GetFinalPathNameByHandle, don't
           traverse reparse point. */
        traverse = FALSE;
    }

    hFile = CreateFileA(
        path,
        FILE_READ_ATTRIBUTES, /* desired access */
        0, /* share mode */
        NULL, /* security attributes */
        OPEN_EXISTING,
        /* FILE_FLAG_BACKUP_SEMANTICS is required to open a directory */
        /* FILE_FLAG_OPEN_REPARSE_POINT does not follow the symlink.
           Because of this, calls like GetFinalPathNameByHandle will return
           the symlink path agin and not the actual final path. */
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS|
            FILE_FLAG_OPEN_REPARSE_POINT,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        /* Either the target doesn't exist, or we don't have access to
           get a handle to it. If the former, we need to return an error.
           If the latter, we can use attributes_from_dir. */
        if (GetLastError() != ERROR_SHARING_VIOLATION)
            return -1;
        /* Could not get attributes on open file. Fall back to
           reading the directory. */
        if (!attributes_from_dir(path, &info, &reparse_tag))
            /* Very strange. This should not fail now */
            return -1;
        if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (traverse) {
                /* Should traverse, but could not open reparse point handle */
                SetLastError(ERROR_SHARING_VIOLATION);
                return -1;
            }
        }
    } else {
        if (!GetFileInformationByHandle(hFile, &info)) {
            CloseHandle(hFile);
            return -1;
        }
        if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (!win32_get_reparse_tag(hFile, &reparse_tag))
                return -1;

            /* Close the outer open file handle now that we're about to
               reopen it with different flags. */
            if (!CloseHandle(hFile))
                return -1;

            if (traverse) {
                /* In order to call GetFinalPathNameByHandle we need to open
                   the file without the reparse handling flag set. */
                hFile2 = CreateFileA(
                           path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
                if (hFile2 == INVALID_HANDLE_VALUE)
                    return -1;

                if (!get_target_path(hFile2, &target_path))
                    return -1;

                code = win32_xstat_impl_w(target_path, result, FALSE);
                free(target_path);
                return code;
            }
        } else
            CloseHandle(hFile);
    }
    attribute_data_to_stat(&info, reparse_tag, result);

    /* Set S_IEXEC if it is an .exe, .bat, ... */
    dot = strrchr(path, '.');
    if (dot) {
        if (stricmp(dot, ".bat") == 0 || stricmp(dot, ".cmd") == 0 ||
            stricmp(dot, ".exe") == 0 || stricmp(dot, ".com") == 0)
            result->st_mode |= 0111;
    }
    return 0;
}

static int
win32_xstat_impl_w(const wchar_t *path, struct win32_stat *result,
                   BOOL traverse)
{
    int code;
    HANDLE hFile, hFile2;
    BY_HANDLE_FILE_INFORMATION info;
    ULONG reparse_tag = 0;
    wchar_t *target_path;
    const wchar_t *dot;

    if(!check_GetFinalPathNameByHandle()) {
        /* If the OS doesn't have GetFinalPathNameByHandle, don't
           traverse reparse point. */
        traverse = FALSE;
    }

    hFile = CreateFileW(
        path,
        FILE_READ_ATTRIBUTES, /* desired access */
        0, /* share mode */
        NULL, /* security attributes */
        OPEN_EXISTING,
        /* FILE_FLAG_BACKUP_SEMANTICS is required to open a directory */
        /* FILE_FLAG_OPEN_REPARSE_POINT does not follow the symlink.
           Because of this, calls like GetFinalPathNameByHandle will return
           the symlink path agin and not the actual final path. */
        FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS|
            FILE_FLAG_OPEN_REPARSE_POINT,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        /* Either the target doesn't exist, or we don't have access to
           get a handle to it. If the former, we need to return an error.
           If the latter, we can use attributes_from_dir. */
        if (GetLastError() != ERROR_SHARING_VIOLATION)
            return -1;
        /* Could not get attributes on open file. Fall back to
           reading the directory. */
        if (!attributes_from_dir_w(path, &info, &reparse_tag))
            /* Very strange. This should not fail now */
            return -1;
        if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (traverse) {
                /* Should traverse, but could not open reparse point handle */
                SetLastError(ERROR_SHARING_VIOLATION);
                return -1;
            }
        }
    } else {
        if (!GetFileInformationByHandle(hFile, &info)) {
            CloseHandle(hFile);
            return -1;
        }
        if (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            if (!win32_get_reparse_tag(hFile, &reparse_tag))
                return -1;

            /* Close the outer open file handle now that we're about to
               reopen it with different flags. */
            if (!CloseHandle(hFile))
                return -1;

            if (traverse) {
                /* In order to call GetFinalPathNameByHandle we need to open
                   the file without the reparse handling flag set. */
                hFile2 = CreateFileW(
                           path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                           NULL, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);
                if (hFile2 == INVALID_HANDLE_VALUE)
                    return -1;

                if (!get_target_path(hFile2, &target_path))
                    return -1;

                code = win32_xstat_impl_w(target_path, result, FALSE);
                free(target_path);
                return code;
            }
        } else
            CloseHandle(hFile);
    }
    attribute_data_to_stat(&info, reparse_tag, result);

    /* Set S_IEXEC if it is an .exe, .bat, ... */
    dot = wcsrchr(path, '.');
    if (dot) {
        if (_wcsicmp(dot, L".bat") == 0 || _wcsicmp(dot, L".cmd") == 0 ||
            _wcsicmp(dot, L".exe") == 0 || _wcsicmp(dot, L".com") == 0)
            result->st_mode |= 0111;
    }
    return 0;
}

static int
win32_xstat(const char *path, struct win32_stat *result, BOOL traverse)
{
    /* Protocol violation: we explicitly clear errno, instead of
       setting it to a POSIX error. Callers should use GetLastError. */
    int code = win32_xstat_impl(path, result, traverse);
    errno = 0;
    return code;
}

static int
win32_xstat_w(const wchar_t *path, struct win32_stat *result, BOOL traverse)
{
    /* Protocol violation: we explicitly clear errno, instead of
       setting it to a POSIX error. Callers should use GetLastError. */
    int code = win32_xstat_impl_w(path, result, traverse);
    errno = 0;
    return code;
}
/* About the following functions: win32_lstat_w, win32_stat, win32_stat_w

   In Posix, stat automatically traverses symlinks and returns the stat
   structure for the target.  In Windows, the equivalent GetFileAttributes by
   default does not traverse symlinks and instead returns attributes for
   the symlink.

   Therefore, win32_lstat will get the attributes traditionally, and
   win32_stat will first explicitly resolve the symlink target and then will
   call win32_lstat on that result.

   The _w represent Unicode equivalents of the aforementioned ANSI functions. */

static int
win32_lstat(const char* path, struct win32_stat *result)
{
    return win32_xstat(path, result, FALSE);
}

static int
win32_lstat_w(const wchar_t* path, struct win32_stat *result)
{
    return win32_xstat_w(path, result, FALSE);
}

static int
win32_stat(const char* path, struct win32_stat *result)
{
    return win32_xstat(path, result, TRUE);
}

static int
win32_stat_w(const wchar_t* path, struct win32_stat *result)
{
    return win32_xstat_w(path, result, TRUE);
}

static int
win32_fstat(int file_number, struct win32_stat *result)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int type;

    h = (HANDLE)_get_osfhandle(file_number);

    /* Protocol violation: we explicitly clear errno, instead of
       setting it to a POSIX error. Callers should use GetLastError. */
    errno = 0;

    if (h == INVALID_HANDLE_VALUE) {
        /* This is really a C library error (invalid file handle).
           We set the Win32 error to the closes one matching. */
        SetLastError(ERROR_INVALID_HANDLE);
        return -1;
    }
    memset(result, 0, sizeof(*result));

    type = GetFileType(h);
    if (type == FILE_TYPE_UNKNOWN) {
        DWORD error = GetLastError();
        if (error != 0) {
            return -1;
        }
        /* else: valid but unknown file */
    }

    if (type != FILE_TYPE_DISK) {
        if (type == FILE_TYPE_CHAR)
            result->st_mode = _S_IFCHR;
        else if (type == FILE_TYPE_PIPE)
            result->st_mode = _S_IFIFO;
        return 0;
    }

    if (!GetFileInformationByHandle(h, &info)) {
        return -1;
    }

    attribute_data_to_stat(&info, 0, result);
    /* specific to fstat() */
    result->st_ino = (((__int64)info.nFileIndexHigh)<<32) + info.nFileIndexLow;
    return 0;
}

#endif /* MS_WINDOWS */

PyDoc_STRVAR(stat_result__doc__,
"stat_result: Result from stat, fstat, or lstat.\n\n\
This object may be accessed either as a tuple of\n\
  (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)\n\
or via the attributes st_mode, st_ino, st_dev, st_nlink, st_uid, and so on.\n\
\n\
Posix/windows: If your platform supports st_blksize, st_blocks, st_rdev,\n\
or st_flags, they are available as attributes only.\n\
\n\
See os.stat for more information.");

static PyStructSequence_Field stat_result_fields[] = {
    {"st_mode",    "protection bits"},
    {"st_ino",     "inode"},
    {"st_dev",     "device"},
    {"st_nlink",   "number of hard links"},
    {"st_uid",     "user ID of owner"},
    {"st_gid",     "group ID of owner"},
    {"st_size",    "total size, in bytes"},
    /* The NULL is replaced with PyStructSequence_UnnamedField later. */
    {NULL,   "integer time of last access"},
    {NULL,   "integer time of last modification"},
    {NULL,   "integer time of last change"},
    {"st_atime",   "time of last access"},
    {"st_mtime",   "time of last modification"},
    {"st_ctime",   "time of last change"},
    {"st_atime_ns",   "time of last access in nanoseconds"},
    {"st_mtime_ns",   "time of last modification in nanoseconds"},
    {"st_ctime_ns",   "time of last change in nanoseconds"},
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    {"st_blksize", "blocksize for filesystem I/O"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    {"st_blocks",  "number of blocks allocated"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    {"st_rdev",    "device type (if inode device)"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    {"st_flags",   "user defined flags for file"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    {"st_gen",    "generation number"},
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {"st_birthtime",   "time of creation"},
#endif
    {0}
};

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
#define ST_BLKSIZE_IDX 16
#else
#define ST_BLKSIZE_IDX 15
#endif

#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
#define ST_BLOCKS_IDX (ST_BLKSIZE_IDX+1)
#else
#define ST_BLOCKS_IDX ST_BLKSIZE_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_RDEV
#define ST_RDEV_IDX (ST_BLOCKS_IDX+1)
#else
#define ST_RDEV_IDX ST_BLOCKS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_FLAGS
#define ST_FLAGS_IDX (ST_RDEV_IDX+1)
#else
#define ST_FLAGS_IDX ST_RDEV_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_GEN
#define ST_GEN_IDX (ST_FLAGS_IDX+1)
#else
#define ST_GEN_IDX ST_FLAGS_IDX
#endif

#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
#define ST_BIRTHTIME_IDX (ST_GEN_IDX+1)
#else
#define ST_BIRTHTIME_IDX ST_GEN_IDX
#endif

static PyStructSequence_Desc stat_result_desc = {
    "stat_result", /* name */
    stat_result__doc__, /* doc */
    stat_result_fields,
    10
};

PyDoc_STRVAR(statvfs_result__doc__,
"statvfs_result: Result from statvfs or fstatvfs.\n\n\
This object may be accessed either as a tuple of\n\
  (bsize, frsize, blocks, bfree, bavail, files, ffree, favail, flag, namemax),\n\
or via the attributes f_bsize, f_frsize, f_blocks, f_bfree, and so on.\n\
\n\
See os.statvfs for more information.");

static PyStructSequence_Field statvfs_result_fields[] = {
    {"f_bsize",  },
    {"f_frsize", },
    {"f_blocks", },
    {"f_bfree",  },
    {"f_bavail", },
    {"f_files",  },
    {"f_ffree",  },
    {"f_favail", },
    {"f_flag",   },
    {"f_namemax",},
    {0}
};

static PyStructSequence_Desc statvfs_result_desc = {
    "statvfs_result", /* name */
    statvfs_result__doc__, /* doc */
    statvfs_result_fields,
    10
};

#if defined(HAVE_WAITID) && !defined(__APPLE__)
PyDoc_STRVAR(waitid_result__doc__,
"waitid_result: Result from waitid.\n\n\
This object may be accessed either as a tuple of\n\
  (si_pid, si_uid, si_signo, si_status, si_code),\n\
or via the attributes si_pid, si_uid, and so on.\n\
\n\
See os.waitid for more information.");

static PyStructSequence_Field waitid_result_fields[] = {
    {"si_pid",  },
    {"si_uid", },
    {"si_signo", },
    {"si_status",  },
    {"si_code", },
    {0}
};

static PyStructSequence_Desc waitid_result_desc = {
    "waitid_result", /* name */
    waitid_result__doc__, /* doc */
    waitid_result_fields,
    5
};
static PyTypeObject WaitidResultType;
#endif

static int initialized;
static PyTypeObject StatResultType;
static PyTypeObject StatVFSResultType;
#if defined(HAVE_SCHED_SETPARAM) || defined(HAVE_SCHED_SETSCHEDULER)
static PyTypeObject SchedParamType;
#endif
static newfunc structseq_new;

static PyObject *
statresult_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyStructSequence *result;
    int i;

    result = (PyStructSequence*)structseq_new(type, args, kwds);
    if (!result)
        return NULL;
    /* If we have been initialized from a tuple,
       st_?time might be set to None. Initialize it
       from the int slots.  */
    for (i = 7; i <= 9; i++) {
        if (result->ob_item[i+3] == Py_None) {
            Py_DECREF(Py_None);
            Py_INCREF(result->ob_item[i]);
            result->ob_item[i+3] = result->ob_item[i];
        }
    }
    return (PyObject*)result;
}



/* If true, st_?time is float. */
static int _stat_float_times = 1;

PyDoc_STRVAR(stat_float_times__doc__,
"stat_float_times([newval]) -> oldval\n\n\
Determine whether os.[lf]stat represents time stamps as float objects.\n\
If newval is True, future calls to stat() return floats, if it is False,\n\
future calls return ints. \n\
If newval is omitted, return the current setting.\n");

static PyObject*
stat_float_times(PyObject* self, PyObject *args)
{
    int newval = -1;
    if (!PyArg_ParseTuple(args, "|i:stat_float_times", &newval))
        return NULL;
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "stat_float_times() is deprecated",
                     1))
        return NULL;
    if (newval == -1)
        /* Return old value */
        return PyBool_FromLong(_stat_float_times);
    _stat_float_times = newval;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *billion = NULL;

static void
fill_time(PyObject *v, int index, time_t sec, unsigned long nsec)
{
    PyObject *s = _PyLong_FromTime_t(sec);
    PyObject *ns_fractional = PyLong_FromUnsignedLong(nsec);
    PyObject *s_in_ns = NULL;
    PyObject *ns_total = NULL;
    PyObject *float_s = NULL;

    if (!(s && ns_fractional))
        goto exit;

    s_in_ns = PyNumber_Multiply(s, billion);
    if (!s_in_ns)
        goto exit;

    ns_total = PyNumber_Add(s_in_ns, ns_fractional);
    if (!ns_total)
        goto exit;

    if (_stat_float_times) {
        float_s = PyFloat_FromDouble(sec + 1e-9*nsec);
        if (!float_s)
            goto exit;
    }
    else {
        float_s = s;
        Py_INCREF(float_s);
    }

    PyStructSequence_SET_ITEM(v, index, s);
    PyStructSequence_SET_ITEM(v, index+3, float_s);
    PyStructSequence_SET_ITEM(v, index+6, ns_total);
    s = NULL;
    float_s = NULL;
    ns_total = NULL;
exit:
    Py_XDECREF(s);
    Py_XDECREF(ns_fractional);
    Py_XDECREF(s_in_ns);
    Py_XDECREF(ns_total);
    Py_XDECREF(float_s);
}

/* pack a system stat C structure into the Python stat tuple
   (used by posix_stat() and posix_fstat()) */
static PyObject*
_pystat_fromstructstat(STRUCT_STAT *st)
{
    unsigned long ansec, mnsec, cnsec;
    PyObject *v = PyStructSequence_New(&StatResultType);
    if (v == NULL)
        return NULL;

    PyStructSequence_SET_ITEM(v, 0, PyLong_FromLong((long)st->st_mode));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 1,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_ino));
#else
    PyStructSequence_SET_ITEM(v, 1, PyLong_FromLong((long)st->st_ino));
#endif
#if defined(HAVE_LONG_LONG) && !defined(MS_WINDOWS)
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
    PyStructSequence_SET_ITEM(v, 2, PyLong_FromLong((long)st->st_dev));
#endif
    PyStructSequence_SET_ITEM(v, 3, PyLong_FromLong((long)st->st_nlink));
    PyStructSequence_SET_ITEM(v, 4, PyLong_FromLong((long)st->st_uid));
    PyStructSequence_SET_ITEM(v, 5, PyLong_FromLong((long)st->st_gid));
#ifdef HAVE_LARGEFILE_SUPPORT
    PyStructSequence_SET_ITEM(v, 6,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_size));
#else
    PyStructSequence_SET_ITEM(v, 6, PyLong_FromLong(st->st_size));
#endif

#if defined(HAVE_STAT_TV_NSEC)
    ansec = st->st_atim.tv_nsec;
    mnsec = st->st_mtim.tv_nsec;
    cnsec = st->st_ctim.tv_nsec;
#elif defined(HAVE_STAT_TV_NSEC2)
    ansec = st->st_atimespec.tv_nsec;
    mnsec = st->st_mtimespec.tv_nsec;
    cnsec = st->st_ctimespec.tv_nsec;
#elif defined(HAVE_STAT_NSEC)
    ansec = st->st_atime_nsec;
    mnsec = st->st_mtime_nsec;
    cnsec = st->st_ctime_nsec;
#else
    ansec = mnsec = cnsec = 0;
#endif
    fill_time(v, 7, st->st_atime, ansec);
    fill_time(v, 8, st->st_mtime, mnsec);
    fill_time(v, 9, st->st_ctime, cnsec);

#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    PyStructSequence_SET_ITEM(v, ST_BLKSIZE_IDX,
                              PyLong_FromLong((long)st->st_blksize));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BLOCKS
    PyStructSequence_SET_ITEM(v, ST_BLOCKS_IDX,
                              PyLong_FromLong((long)st->st_blocks));
#endif
#ifdef HAVE_STRUCT_STAT_ST_RDEV
    PyStructSequence_SET_ITEM(v, ST_RDEV_IDX,
                              PyLong_FromLong((long)st->st_rdev));
#endif
#ifdef HAVE_STRUCT_STAT_ST_GEN
    PyStructSequence_SET_ITEM(v, ST_GEN_IDX,
                              PyLong_FromLong((long)st->st_gen));
#endif
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIME
    {
      PyObject *val;
      unsigned long bsec,bnsec;
      bsec = (long)st->st_birthtime;
#ifdef HAVE_STAT_TV_NSEC2
      bnsec = st->st_birthtimespec.tv_nsec;
#else
      bnsec = 0;
#endif
      if (_stat_float_times) {
        val = PyFloat_FromDouble(bsec + 1e-9*bnsec);
      } else {
        val = PyLong_FromLong((long)bsec);
      }
      PyStructSequence_SET_ITEM(v, ST_BIRTHTIME_IDX,
                                val);
    }
#endif
#ifdef HAVE_STRUCT_STAT_ST_FLAGS
    PyStructSequence_SET_ITEM(v, ST_FLAGS_IDX,
                              PyLong_FromLong((long)st->st_flags));
#endif

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}

/* POSIX methods */


static PyObject *
posix_do_stat(char *function_name, path_t *path,
              int dir_fd, int follow_symlinks)
{
    STRUCT_STAT st;
    int result;

#if !defined(MS_WINDOWS) && !defined(HAVE_FSTATAT) && !defined(HAVE_LSTAT)
    if (follow_symlinks_specified(function_name, follow_symlinks))
        return NULL;
#endif

    if (path_and_dir_fd_invalid("stat", path, dir_fd) ||
        dir_fd_and_fd_invalid("stat", dir_fd, path->fd) ||
        fd_and_follow_symlinks_invalid("stat", path->fd, follow_symlinks))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    if (path->fd != -1)
        result = FSTAT(path->fd, &st);
    else
#ifdef MS_WINDOWS
    if (path->wide) {
        if (follow_symlinks)
            result = win32_stat_w(path->wide, &st);
        else
            result = win32_lstat_w(path->wide, &st);
    }
    else
#endif
#if defined(HAVE_LSTAT) || defined(MS_WINDOWS)
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = LSTAT(path->narrow, &st);
    else
#endif
#ifdef HAVE_FSTATAT
    if ((dir_fd != DEFAULT_DIR_FD) || !follow_symlinks)
        result = fstatat(dir_fd, path->narrow, &st,
                         follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW);
    else
#endif
        result = STAT(path->narrow, &st);
    Py_END_ALLOW_THREADS

    if (result != 0)
        return path_error("stat", path);

    return _pystat_fromstructstat(&st);
}

PyDoc_STRVAR(posix_stat__doc__,
"stat(path, *, dir_fd=None, follow_symlinks=True) -> stat result\n\n\
Perform a stat system call on the given path.\n\
\n\
path may be specified as either a string or as an open file descriptor.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
  dir_fd may not be supported on your platform; if it is unavailable, using\n\
  it will raise a NotImplementedError.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, stat will examine the symbolic link itself instead of the file the\n\
  link points to.\n\
It is an error to use dir_fd or follow_symlinks when specifying path as\n\
  an open file descriptor.");

static PyObject *
posix_stat(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"path", "dir_fd", "follow_symlinks", NULL};
    path_t path;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;
    PyObject *return_value;

    memset(&path, 0, sizeof(path));
    path.allow_fd = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&p:stat", keywords,
        path_converter, &path,
#ifdef HAVE_FSTATAT
        dir_fd_converter, &dir_fd,
#else
        dir_fd_unavailable, &dir_fd,
#endif
        &follow_symlinks))
        return NULL;
    return_value = posix_do_stat("stat", &path, dir_fd, follow_symlinks);
    path_cleanup(&path);
    return return_value;
}

PyDoc_STRVAR(posix_lstat__doc__,
"lstat(path, *, dir_fd=None) -> stat result\n\n\
Like stat(), but do not follow symbolic links.\n\
Equivalent to stat(path, follow_symlinks=False).");

static PyObject *
posix_lstat(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"path", "dir_fd", NULL};
    path_t path;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 0;
    PyObject *return_value;

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&:lstat", keywords,
        path_converter, &path,
#ifdef HAVE_FSTATAT
        dir_fd_converter, &dir_fd
#else
        dir_fd_unavailable, &dir_fd
#endif
        ))
        return NULL;
    return_value = posix_do_stat("stat", &path, dir_fd, follow_symlinks);
    path_cleanup(&path);
    return return_value;
}

PyDoc_STRVAR(posix_access__doc__,
"access(path, mode, *, dir_fd=None, effective_ids=False,\
 follow_symlinks=True)\n\n\
Use the real uid/gid to test for access to a path.  Returns True if granted,\n\
False otherwise.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
If effective_ids is True, access will use the effective uid/gid instead of\n\
  the real uid/gid.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, access will examine the symbolic link itself instead of the file the\n\
  link points to.\n\
dir_fd, effective_ids, and follow_symlinks may not be implemented\n\
  on your platform.  If they are unavailable, using them will raise a\n\
  NotImplementedError.\n\
\n\
Note that most operations will use the effective uid/gid, therefore this\n\
  routine can be used in a suid/sgid environment to test if the invoking user\n\
  has the specified access to the path.\n\
The mode argument can be F_OK to test existence, or the inclusive-OR\n\
  of R_OK, W_OK, and X_OK.");

static PyObject *
posix_access(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"path", "mode", "dir_fd", "effective_ids",
                                "follow_symlinks", NULL};
    path_t path;
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int effective_ids = 0;
    int follow_symlinks = 1;
    PyObject *return_value = NULL;

#ifdef MS_WINDOWS
    DWORD attr;
#else
    int result;
#endif

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&i|$O&pp:access", keywords,
        path_converter, &path, &mode,
#ifdef HAVE_FACCESSAT
        dir_fd_converter, &dir_fd,
#else
        dir_fd_unavailable, &dir_fd,
#endif
        &effective_ids, &follow_symlinks))
        return NULL;

#ifndef HAVE_FACCESSAT
    if (follow_symlinks_specified("access", follow_symlinks))
        goto exit;

    if (effective_ids) {
        argument_unavailable_error("access", "effective_ids");
        goto exit;
    }
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path.wide != NULL)
        attr = GetFileAttributesW(path.wide);
    else
        attr = GetFileAttributesA(path.narrow);
    Py_END_ALLOW_THREADS

    /*
     * Access is possible if 
     *   * we didn't get a -1, and
     *     * write access wasn't requested,
     *     * or the file isn't read-only,
     *     * or it's a directory.
     * (Directories cannot be read-only on Windows.)
    */
    return_value = PyBool_FromLong(
        (attr != 0xFFFFFFFF) &&
            (!(mode & 2) ||
            !(attr & FILE_ATTRIBUTE_READONLY) ||
            (attr & FILE_ATTRIBUTE_DIRECTORY)));
#else

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FACCESSAT
    if ((dir_fd != DEFAULT_DIR_FD) ||
        effective_ids ||
        !follow_symlinks) {
        int flags = 0;
        if (!follow_symlinks)
            flags |= AT_SYMLINK_NOFOLLOW;
        if (effective_ids)
            flags |= AT_EACCESS;
        result = faccessat(dir_fd, path.narrow, mode, flags);
    }
    else
#endif
        result = access(path.narrow, mode);
    Py_END_ALLOW_THREADS
    return_value = PyBool_FromLong(!result);
#endif

#ifndef HAVE_FACCESSAT
exit:
#endif
    path_cleanup(&path);
    return return_value;
}

#ifndef F_OK
#define F_OK 0
#endif
#ifndef R_OK
#define R_OK 4
#endif
#ifndef W_OK
#define W_OK 2
#endif
#ifndef X_OK
#define X_OK 1
#endif

#ifdef HAVE_TTYNAME
PyDoc_STRVAR(posix_ttyname__doc__,
"ttyname(fd) -> string\n\n\
Return the name of the terminal device connected to 'fd'.");

static PyObject *
posix_ttyname(PyObject *self, PyObject *args)
{
    int id;
    char *ret;

    if (!PyArg_ParseTuple(args, "i:ttyname", &id))
        return NULL;

#if defined(__VMS)
    /* file descriptor 0 only, the default input device (stdin) */
    if (id == 0) {
        ret = ttyname();
    }
    else {
        ret = NULL;
    }
#else
    ret = ttyname(id);
#endif
    if (ret == NULL)
        return posix_error();
    return PyUnicode_DecodeFSDefault(ret);
}
#endif

#ifdef HAVE_CTERMID
PyDoc_STRVAR(posix_ctermid__doc__,
"ctermid() -> string\n\n\
Return the name of the controlling terminal for this process.");

static PyObject *
posix_ctermid(PyObject *self, PyObject *noargs)
{
    char *ret;
    char buffer[L_ctermid];

#ifdef USE_CTERMID_R
    ret = ctermid_r(buffer);
#else
    ret = ctermid(buffer);
#endif
    if (ret == NULL)
        return posix_error();
    return PyUnicode_DecodeFSDefault(buffer);
}
#endif

PyDoc_STRVAR(posix_chdir__doc__,
"chdir(path)\n\n\
Change the current working directory to the specified path.\n\
\n\
path may always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.");

static PyObject *
posix_chdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", NULL};

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FCHDIR
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&:chdir", keywords,
        path_converter, &path
        ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path.wide)
        result = win32_wchdir(path.wide);
    else
        result = win32_chdir(path.narrow);
	result = !result; /* on unix, success = 0, on windows, success = !0 */
#elif defined(PYOS_OS2) && defined(PYCC_GCC)
    result = _chdir2(path.narrow);
#else
#ifdef HAVE_FCHDIR
    if (path.fd != -1)
        result = fchdir(path.fd);
    else
#endif
        result = chdir(path.narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error("chdir", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);
    
exit:
    path_cleanup(&path);
    return return_value;
}

#ifdef HAVE_FCHDIR
PyDoc_STRVAR(posix_fchdir__doc__,
"fchdir(fd)\n\n\
Change to the directory of the given file descriptor.  fd must be\n\
opened on a directory, not a file.  Equivalent to os.chdir(fd).");

static PyObject *
posix_fchdir(PyObject *self, PyObject *fdobj)
{
    return posix_fildes(fdobj, fchdir);
}
#endif /* HAVE_FCHDIR */


PyDoc_STRVAR(posix_chmod__doc__,
"chmod(path, mode, *, dir_fd=None, follow_symlinks=True)\n\n\
Change the access permissions of a file.\n\
\n\
path may always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, chmod will modify the symbolic link itself instead of the file the\n\
  link points to.\n\
It is an error to use dir_fd or follow_symlinks when specifying path as\n\
  an open file descriptor.\n\
dir_fd and follow_symlinks may not be implemented on your platform.\n\
  If they are unavailable, using them will raise a NotImplementedError.");

static PyObject *
posix_chmod(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "mode", "dir_fd",
                               "follow_symlinks", NULL};

#ifdef MS_WINDOWS
    DWORD attr;
#endif

#ifdef HAVE_FCHMODAT
    int fchmodat_nofollow_unsupported = 0;
#endif

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FCHMOD
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&i|$O&p:chmod", keywords,
        path_converter, &path,
        &mode,
#ifdef HAVE_FCHMODAT
        dir_fd_converter, &dir_fd,
#else
        dir_fd_unavailable, &dir_fd,
#endif
        &follow_symlinks))
        return NULL;

#if !(defined(HAVE_FCHMODAT) || defined(HAVE_LCHMOD))
    if (follow_symlinks_specified("chmod", follow_symlinks))
        goto exit;
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path.wide)
        attr = GetFileAttributesW(path.wide);
    else
        attr = GetFileAttributesA(path.narrow);
    if (attr == 0xFFFFFFFF)
        result = 0;
    else {
        if (mode & _S_IWRITE)
            attr &= ~FILE_ATTRIBUTE_READONLY;
        else
            attr |= FILE_ATTRIBUTE_READONLY;
        if (path.wide)
            result = SetFileAttributesW(path.wide, attr);
        else
            result = SetFileAttributesA(path.narrow, attr);
    }
    Py_END_ALLOW_THREADS

    if (!result) {
        return_value = win32_error_object("chmod", path.object);
        goto exit;
    }
#else /* MS_WINDOWS */
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FCHMOD
    if (path.fd != -1)
        result = fchmod(path.fd, mode);
    else
#endif
#ifdef HAVE_LCHMOD
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = lchmod(path.narrow, mode);
    else
#endif
#ifdef HAVE_FCHMODAT
    if ((dir_fd != DEFAULT_DIR_FD) || !follow_symlinks) {
        /*
         * fchmodat() doesn't currently support AT_SYMLINK_NOFOLLOW!
         * The documentation specifically shows how to use it,
         * and then says it isn't implemented yet.
         * (true on linux with glibc 2.15, and openindiana 3.x)
         *
         * Once it is supported, os.chmod will automatically
         * support dir_fd and follow_symlinks=False.  (Hopefully.)
         * Until then, we need to be careful what exception we raise.
         */
        result = fchmodat(dir_fd, path.narrow, mode,
                          follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW);
        /*
         * But wait!  We can't throw the exception without allowing threads,
         * and we can't do that in this nested scope.  (Macro trickery, sigh.)
         */
        fchmodat_nofollow_unsupported =
                         result &&
                         ((errno == ENOTSUP) || (errno == EOPNOTSUPP)) &&
                         !follow_symlinks;
    }
    else
#endif
        result = chmod(path.narrow, mode);
    Py_END_ALLOW_THREADS

    if (result) {
#ifdef HAVE_FCHMODAT
        if (fchmodat_nofollow_unsupported) {
            if (dir_fd != DEFAULT_DIR_FD)
                dir_fd_and_follow_symlinks_invalid("chmod",
                                                   dir_fd, follow_symlinks);
            else
                follow_symlinks_specified("chmod", follow_symlinks);
        }
        else
#endif
            return_value = path_error("chmod", &path);
        goto exit;
    }
#endif

    Py_INCREF(Py_None);
    return_value = Py_None;
exit:
    path_cleanup(&path);
    return return_value;
}


#ifdef HAVE_FCHMOD
PyDoc_STRVAR(posix_fchmod__doc__,
"fchmod(fd, mode)\n\n\
Change the access permissions of the file given by file\n\
descriptor fd.  Equivalent to os.chmod(fd, mode).");

static PyObject *
posix_fchmod(PyObject *self, PyObject *args)
{
    int fd, mode, res;
    if (!PyArg_ParseTuple(args, "ii:fchmod", &fd, &mode))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fchmod(fd, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHMOD */

#ifdef HAVE_LCHMOD
PyDoc_STRVAR(posix_lchmod__doc__,
"lchmod(path, mode)\n\n\
Change the access permissions of a file. If path is a symlink, this\n\
affects the link itself rather than the target.\n\
Equivalent to chmod(path, mode, follow_symlinks=False).");

static PyObject *
posix_lchmod(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    int i;
    int res;
    if (!PyArg_ParseTuple(args, "O&i:lchmod", PyUnicode_FSConverter,
                          &opath, &i))
        return NULL;
    path = PyBytes_AsString(opath);
    Py_BEGIN_ALLOW_THREADS
    res = lchmod(path, i);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error_with_allocated_filename(opath);
    Py_DECREF(opath);
    Py_RETURN_NONE;
}
#endif /* HAVE_LCHMOD */


#ifdef HAVE_CHFLAGS
PyDoc_STRVAR(posix_chflags__doc__,
"chflags(path, flags, *, follow_symlinks=True)\n\n\
Set file flags.\n\
\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, chflags will change flags on the symbolic link itself instead of the\n\
  file the link points to.\n\
follow_symlinks may not be implemented on your platform.  If it is\n\
unavailable, using it will raise a NotImplementedError.");

static PyObject *
posix_chflags(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    unsigned long flags;
    int follow_symlinks = 1;
    int result;
    PyObject *return_value;
    static char *keywords[] = {"path", "flags", "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&k|$i:chflags", keywords,
                          path_converter, &path,
                          &flags, &follow_symlinks))
        return NULL;

#ifndef HAVE_LCHFLAGS
    if (follow_symlinks_specified("chflags", follow_symlinks))
        goto exit;
#endif

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_LCHFLAGS
    if (!follow_symlinks)
        result = lchflags(path.narrow, flags);
    else
#endif
        result = chflags(path.narrow, flags);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_posix_error("chflags", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&path);
    return return_value;
}
#endif /* HAVE_CHFLAGS */

#ifdef HAVE_LCHFLAGS
PyDoc_STRVAR(posix_lchflags__doc__,
"lchflags(path, flags)\n\n\
Set file flags.\n\
This function will not follow symbolic links.\n\
Equivalent to chflags(path, flags, follow_symlinks=False).");

static PyObject *
posix_lchflags(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    unsigned long flags;
    int res;
    if (!PyArg_ParseTuple(args, "O&k:lchflags",
                          PyUnicode_FSConverter, &opath, &flags))
        return NULL;
    path = PyBytes_AsString(opath);
    Py_BEGIN_ALLOW_THREADS
    res = lchflags(path, flags);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error_with_allocated_filename(opath);
    Py_DECREF(opath);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_LCHFLAGS */

#ifdef HAVE_CHROOT
PyDoc_STRVAR(posix_chroot__doc__,
"chroot(path)\n\n\
Change root directory to path.");

static PyObject *
posix_chroot(PyObject *self, PyObject *args)
{
    return posix_1str(args, "O&:chroot", chroot);
}
#endif

#ifdef HAVE_FSYNC
PyDoc_STRVAR(posix_fsync__doc__,
"fsync(fildes)\n\n\
force write of file with filedescriptor to disk.");

static PyObject *
posix_fsync(PyObject *self, PyObject *fdobj)
{
    return posix_fildes(fdobj, fsync);
}
#endif /* HAVE_FSYNC */

#ifdef HAVE_SYNC
PyDoc_STRVAR(posix_sync__doc__,
"sync()\n\n\
Force write of everything to disk.");

static PyObject *
posix_sync(PyObject *self, PyObject *noargs)
{
    Py_BEGIN_ALLOW_THREADS
    sync();
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_FDATASYNC

#ifdef __hpux
extern int fdatasync(int); /* On HP-UX, in libc but not in unistd.h */
#endif

PyDoc_STRVAR(posix_fdatasync__doc__,
"fdatasync(fildes)\n\n\
force write of file with filedescriptor to disk.\n\
 does not force update of metadata.");

static PyObject *
posix_fdatasync(PyObject *self, PyObject *fdobj)
{
    return posix_fildes(fdobj, fdatasync);
}
#endif /* HAVE_FDATASYNC */


#ifdef HAVE_CHOWN
PyDoc_STRVAR(posix_chown__doc__,
"chown(path, uid, gid, *, dir_fd=None, follow_symlinks=True)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
\n\
path may always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, chown will modify the symbolic link itself instead of the file the\n\
  link points to.\n\
It is an error to use dir_fd or follow_symlinks when specifying path as\n\
  an open file descriptor.\n\
dir_fd and follow_symlinks may not be implemented on your platform.\n\
  If they are unavailable, using them will raise a NotImplementedError.");

static PyObject *
posix_chown(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    long uid_l, gid_l;
    uid_t uid;
    gid_t gid;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "uid", "gid", "dir_fd",
                               "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FCHOWN
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&ll|$O&p:chown", keywords,
                                     path_converter, &path,
                                     &uid_l, &gid_l,
#ifdef HAVE_FCHOWNAT
                                     dir_fd_converter, &dir_fd,
#else
                                     dir_fd_unavailable, &dir_fd,
#endif
                                     &follow_symlinks))
        return NULL;

#if !(defined(HAVE_LCHOWN) || defined(HAVE_FCHOWNAT))
    if (follow_symlinks_specified("chown", follow_symlinks))
        goto exit;
#endif
    if (dir_fd_and_fd_invalid("chown", dir_fd, path.fd) ||
        fd_and_follow_symlinks_invalid("chown", path.fd, follow_symlinks))
        goto exit;

#ifdef __APPLE__
    /*
     * This is for Mac OS X 10.3, which doesn't have lchown.
     * (But we still have an lchown symbol because of weak-linking.)
     * It doesn't have fchownat either.  So there's no possibility
     * of a graceful failover.
     */    
    if ((!follow_symlinks) && (lchown == NULL)) {
        follow_symlinks_specified("chown", follow_symlinks);
        goto exit;
    }
#endif

    Py_BEGIN_ALLOW_THREADS
    uid = (uid_t)uid_l;
    gid = (uid_t)gid_l;
#ifdef HAVE_FCHOWN
    if (path.fd != -1)
        result = fchown(path.fd, uid, gid);
    else
#endif
#ifdef HAVE_LCHOWN
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = lchown(path.narrow, uid, gid);
    else
#endif
#ifdef HAVE_FCHOWNAT
    if ((dir_fd != DEFAULT_DIR_FD) || (!follow_symlinks))
        result = fchownat(dir_fd, path.narrow, uid, gid,
                          follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW);
    else
#endif
        result = chown(path.narrow, uid, gid);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_posix_error("chown", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&path);
    return return_value;
}
#endif /* HAVE_CHOWN */

#ifdef HAVE_FCHOWN
PyDoc_STRVAR(posix_fchown__doc__,
"fchown(fd, uid, gid)\n\n\
Change the owner and group id of the file given by file descriptor\n\
fd to the numeric uid and gid.  Equivalent to os.chown(fd, uid, gid).");

static PyObject *
posix_fchown(PyObject *self, PyObject *args)
{
    int fd;
    long uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "ill:fchown", &fd, &uid, &gid))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fchown(fd, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHOWN */

#ifdef HAVE_LCHOWN
PyDoc_STRVAR(posix_lchown__doc__,
"lchown(path, uid, gid)\n\n\
Change the owner and group id of path to the numeric uid and gid.\n\
This function will not follow symbolic links.\n\
Equivalent to os.chown(path, uid, gid, follow_symlinks=False).");

static PyObject *
posix_lchown(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    long uid, gid;
    int res;
    if (!PyArg_ParseTuple(args, "O&ll:lchown",
                          PyUnicode_FSConverter, &opath,
                          &uid, &gid))
        return NULL;
    path = PyBytes_AsString(opath);
    Py_BEGIN_ALLOW_THREADS
    res = lchown(path, (uid_t) uid, (gid_t) gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error_with_allocated_filename(opath);
    Py_DECREF(opath);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_LCHOWN */


#ifdef HAVE_GETCWD
static PyObject *
posix_getcwd(int use_bytes)
{
    char buf[1026];
    char *res;

#ifdef MS_WINDOWS
    if (!use_bytes) {
        wchar_t wbuf[1026];
        wchar_t *wbuf2 = wbuf;
        PyObject *resobj;
        DWORD len;
        Py_BEGIN_ALLOW_THREADS
        len = GetCurrentDirectoryW(sizeof wbuf/ sizeof wbuf[0], wbuf);
        /* If the buffer is large enough, len does not include the
           terminating \0. If the buffer is too small, len includes
           the space needed for the terminator. */
        if (len >= sizeof wbuf/ sizeof wbuf[0]) {
            wbuf2 = malloc(len * sizeof(wchar_t));
            if (wbuf2)
                len = GetCurrentDirectoryW(len, wbuf2);
        }
        Py_END_ALLOW_THREADS
        if (!wbuf2) {
            PyErr_NoMemory();
            return NULL;
        }
        if (!len) {
            if (wbuf2 != wbuf) free(wbuf2);
            return win32_error("getcwdu", NULL);
        }
        resobj = PyUnicode_FromWideChar(wbuf2, len);
        if (wbuf2 != wbuf) free(wbuf2);
        return resobj;
    }

    if (win32_warn_bytes_api())
        return NULL;
#endif

    Py_BEGIN_ALLOW_THREADS
#if defined(PYOS_OS2) && defined(PYCC_GCC)
    res = _getcwd2(buf, sizeof buf);
#else
    res = getcwd(buf, sizeof buf);
#endif
    Py_END_ALLOW_THREADS
    if (res == NULL)
        return posix_error();
    if (use_bytes)
        return PyBytes_FromStringAndSize(buf, strlen(buf));
    return PyUnicode_DecodeFSDefault(buf);
}

PyDoc_STRVAR(posix_getcwd__doc__,
"getcwd() -> path\n\n\
Return a unicode string representing the current working directory.");

static PyObject *
posix_getcwd_unicode(PyObject *self)
{
    return posix_getcwd(0);
}

PyDoc_STRVAR(posix_getcwdb__doc__,
"getcwdb() -> path\n\n\
Return a bytes string representing the current working directory.");

static PyObject *
posix_getcwd_bytes(PyObject *self)
{
    return posix_getcwd(1);
}
#endif

#if ((!defined(HAVE_LINK)) && defined(MS_WINDOWS))
#define HAVE_LINK 1
#endif

#ifdef HAVE_LINK
PyDoc_STRVAR(posix_link__doc__,
"link(src, dst, *, src_dir_fd=None, dst_dir_fd=None, follow_symlinks=True)\n\n\
Create a hard link to a file.\n\
\n\
If either src_dir_fd or dst_dir_fd is not None, it should be a file\n\
  descriptor open to a directory, and the respective path string (src or dst)\n\
  should be relative; the path will then be relative to that directory.\n\
If follow_symlinks is False, and the last element of src is a symbolic\n\
  link, link will create a link to the symbolic link itself instead of the\n\
  file the link points to.\n\
src_dir_fd, dst_dir_fd, and follow_symlinks may not be implemented on your\n\
  platform.  If they are unavailable, using them will raise a\n\
  NotImplementedError.");

static PyObject *
posix_link(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t src, dst;
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;
    PyObject *return_value = NULL;
    static char *keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd",
                               "follow_symlinks", NULL};
#ifdef MS_WINDOWS
    BOOL result;
#else
    int result;
#endif

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&|O&O&p:link", keywords,
            path_converter, &src,
            path_converter, &dst,
            dir_fd_converter, &src_dir_fd,
            dir_fd_converter, &dst_dir_fd,
            &follow_symlinks))
        return NULL;

#ifndef HAVE_LINKAT
    if ((src_dir_fd != DEFAULT_DIR_FD) || (dst_dir_fd != DEFAULT_DIR_FD)) {
        argument_unavailable_error("link", "src_dir_fd and dst_dir_fd");
        goto exit;
    }
#endif

    if ((src.narrow && dst.wide) || (src.wide && dst.narrow)) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "link: src and dst must be the same type");
        goto exit;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (src.wide)
        result = CreateHardLinkW(dst.wide, src.wide, NULL);
    else
        result = CreateHardLinkA(dst.narrow, src.narrow, NULL);
    Py_END_ALLOW_THREADS

    if (!result) {
        return_value = win32_error_object("link", dst.object);
        goto exit;
    }
#else
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_LINKAT
    if ((src_dir_fd != DEFAULT_DIR_FD) ||
        (dst_dir_fd != DEFAULT_DIR_FD) ||
        (!follow_symlinks))
        result = linkat(src_dir_fd, src.narrow,
            dst_dir_fd, dst.narrow,
            follow_symlinks ? AT_SYMLINK_FOLLOW : 0);
    else
#endif
        result = link(src.narrow, dst.narrow);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error("link", &dst);
        goto exit;
    }
#endif

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&src);
    path_cleanup(&dst);
    return return_value;
}
#endif



PyDoc_STRVAR(posix_listdir__doc__,
"listdir(path='.') -> list_of_strings\n\n\
Return a list containing the names of the entries in the directory.\n\
\n\
The list is in arbitrary order.  It does not include the special\n\
entries '.' and '..' even if they are present in the directory.\n\
\n\
path can always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises NotImplementedError.");

static PyObject *
posix_listdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    PyObject *list = NULL;
    static char *keywords[] = {"path", NULL};
    int fd = -1;

#if defined(MS_WINDOWS) && !defined(HAVE_OPENDIR)
    PyObject *v;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    BOOL result;
    WIN32_FIND_DATA FileData;
    char namebuf[MAX_PATH+5]; /* Overallocate for \\*.*\0 */
    char *bufptr = namebuf;
    /* only claim to have space for MAX_PATH */
    Py_ssize_t len = sizeof(namebuf)-5;
    PyObject *po = NULL;
    wchar_t *wnamebuf = NULL;
#elif defined(PYOS_OS2)
#ifndef MAX_PATH
#define MAX_PATH    CCHMAXPATH
#endif
    char *pt;
    PyObject *v;
    char namebuf[MAX_PATH+5];
    HDIR  hdir = 1;
    ULONG srchcnt = 1;
    FILEFINDBUF3   ep;
    APIRET rc;
#else
    PyObject *v;
    DIR *dirp = NULL;
    struct dirent *ep;
    int arg_is_unicode = 1;    
#endif

    memset(&path, 0, sizeof(path));
    path.nullable = 1;
#ifdef HAVE_FDOPENDIR
    path.allow_fd = 1;
    path.fd = -1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O&:listdir", keywords,
        path_converter, &path
        ))
        return NULL;

    /* XXX Should redo this putting the (now four) versions of opendir
       in separate files instead of having them all here... */
#if defined(MS_WINDOWS) && !defined(HAVE_OPENDIR)
    if (!path.narrow) {
        WIN32_FIND_DATAW wFileData;
        wchar_t *po_wchars;

        if (!path.wide) { /* Default arg: "." */
            po_wchars = L".";
            len = 1;
        } else {
            po_wchars = path.wide;
            len = wcslen(path.wide);
        }
        /* The +5 is so we can append "\\*.*\0" */
        wnamebuf = malloc((len + 5) * sizeof(wchar_t));
        if (!wnamebuf) {
            PyErr_NoMemory();
            goto exit;
        }
        wcscpy(wnamebuf, po_wchars);
        if (len > 0) {
            wchar_t wch = wnamebuf[len-1];
            if (wch != L'/' && wch != L'\\' && wch != L':')
                wnamebuf[len++] = L'\\';
            wcscpy(wnamebuf + len, L"*.*");
        }
        if ((list = PyList_New(0)) == NULL) {
            goto exit;
        }
        Py_BEGIN_ALLOW_THREADS
        hFindFile = FindFirstFileW(wnamebuf, &wFileData);
        Py_END_ALLOW_THREADS
        if (hFindFile == INVALID_HANDLE_VALUE) {
            int error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
                goto exit;
            Py_DECREF(list);
            list = NULL;
            win32_error_unicode("FindFirstFileW", wnamebuf);
            goto exit;
        }
        do {
            /* Skip over . and .. */
            if (wcscmp(wFileData.cFileName, L".") != 0 &&
                wcscmp(wFileData.cFileName, L"..") != 0) {
                v = PyUnicode_FromWideChar(wFileData.cFileName,
                                           wcslen(wFileData.cFileName));
                if (v == NULL) {
                    Py_DECREF(list);
                    list = NULL;
                    break;
                }
                if (PyList_Append(list, v) != 0) {
                    Py_DECREF(v);
                    Py_DECREF(list);
                    list = NULL;
                    break;
                }
                Py_DECREF(v);
            }
            Py_BEGIN_ALLOW_THREADS
            result = FindNextFileW(hFindFile, &wFileData);
            Py_END_ALLOW_THREADS
            /* FindNextFile sets error to ERROR_NO_MORE_FILES if
               it got to the end of the directory. */
            if (!result && GetLastError() != ERROR_NO_MORE_FILES) {
                Py_DECREF(list);
                list = win32_error_unicode("FindNextFileW", wnamebuf);
                goto exit;
            }
        } while (result == TRUE);

        goto exit;
    }
    strcpy(namebuf, path.narrow);
    len = path.length;
    if (len > 0) {
        char ch = namebuf[len-1];
        if (ch != SEP && ch != ALTSEP && ch != ':')
            namebuf[len++] = '/';
        strcpy(namebuf + len, "*.*");
    }

    if ((list = PyList_New(0)) == NULL)
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    hFindFile = FindFirstFile(namebuf, &FileData);
    Py_END_ALLOW_THREADS
    if (hFindFile == INVALID_HANDLE_VALUE) {
        int error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
            goto exit;
        Py_DECREF(list);
        list = win32_error("FindFirstFile", namebuf);
        goto exit;
    }
    do {
        /* Skip over . and .. */
        if (strcmp(FileData.cFileName, ".") != 0 &&
            strcmp(FileData.cFileName, "..") != 0) {
            v = PyBytes_FromString(FileData.cFileName);
            if (v == NULL) {
                Py_DECREF(list);
                list = NULL;
                break;
            }
            if (PyList_Append(list, v) != 0) {
                Py_DECREF(v);
                Py_DECREF(list);
                list = NULL;
                break;
            }
            Py_DECREF(v);
        }
        Py_BEGIN_ALLOW_THREADS
        result = FindNextFile(hFindFile, &FileData);
        Py_END_ALLOW_THREADS
        /* FindNextFile sets error to ERROR_NO_MORE_FILES if
           it got to the end of the directory. */
        if (!result && GetLastError() != ERROR_NO_MORE_FILES) {
            Py_DECREF(list);
            list = win32_error("FindNextFile", namebuf);
            goto exit;
        }
    } while (result == TRUE);

exit:
    if (hFindFile != INVALID_HANDLE_VALUE) {
        if (FindClose(hFindFile) == FALSE) {
            if (list != NULL) {
                Py_DECREF(list);
                list = win32_error_object("FindClose", path.object);
            }
        }
    }
    if (wnamebuf)
        free(wnamebuf);
    path_cleanup(&path);

    return list;

#elif defined(PYOS_OS2)
    if (path.length >= MAX_PATH) {
        PyErr_SetString(PyExc_ValueError, "path too long");
        goto exit;
    }
    strcpy(namebuf, path.narrow);
    for (pt = namebuf; *pt; pt++)
        if (*pt == ALTSEP)
            *pt = SEP;
    if (namebuf[len-1] != SEP)
        namebuf[len++] = SEP;
    strcpy(namebuf + len, "*.*");

    if ((list = PyList_New(0)) == NULL) {
        goto exit;
    }

    rc = DosFindFirst(namebuf,         /* Wildcard Pattern to Match */
                      &hdir,           /* Handle to Use While Search Directory */
                      FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_DIRECTORY,
                      &ep, sizeof(ep), /* Structure to Receive Directory Entry */
                      &srchcnt,        /* Max and Actual Count of Entries Per Iteration */
                      FIL_STANDARD);   /* Format of Entry (EAs or Not) */

    if (rc != NO_ERROR) {
        errno = ENOENT;
        Py_DECREF(list);
        list = posix_error_with_filename(path.narrow);
        goto exit;
    }

    if (srchcnt > 0) { /* If Directory is NOT Totally Empty, */
        do {
            if (ep.achName[0] == '.'
            && (ep.achName[1] == '\0' || (ep.achName[1] == '.' && ep.achName[2] == '\0')))
                continue; /* Skip Over "." and ".." Names */

            strcpy(namebuf, ep.achName);

            /* Leave Case of Name Alone -- In Native Form */
            /* (Removed Forced Lowercasing Code) */

            v = PyBytes_FromString(namebuf);
            if (v == NULL) {
                Py_DECREF(list);
                list = NULL;
                break;
            }
            if (PyList_Append(list, v) != 0) {
                Py_DECREF(v);
                Py_DECREF(list);
                list = NULL;
                break;
            }
            Py_DECREF(v);
        } while (DosFindNext(hdir, &ep, sizeof(ep), &srchcnt) == NO_ERROR && srchcnt > 0);
    }

exit:
    path_cleanup(&path);

    return list;
#else

    errno = 0;
    /* v is never read, so it does not need to be initialized yet. */
    if (path.narrow && !PyArg_ParseTuple(args, "U:listdir", &v)) {
        arg_is_unicode = 0;
        PyErr_Clear();
    }
#ifdef HAVE_FDOPENDIR
    if (path.fd != -1) {
        /* closedir() closes the FD, so we duplicate it */
        Py_BEGIN_ALLOW_THREADS
        fd = dup(path.fd);
        Py_END_ALLOW_THREADS

        if (fd == -1) {
            list = posix_error();
            goto exit;
        }

        Py_BEGIN_ALLOW_THREADS
        dirp = fdopendir(fd);
        Py_END_ALLOW_THREADS
    }
    else
#endif
    {
        char *name = path.narrow ? path.narrow : ".";
        Py_BEGIN_ALLOW_THREADS
        dirp = opendir(name);
        Py_END_ALLOW_THREADS
    }

    if (dirp == NULL) {
        list = path_error("listdir", &path);
        goto exit;
    }
    if ((list = PyList_New(0)) == NULL) {
        goto exit;
    }
    for (;;) {
        errno = 0;
        Py_BEGIN_ALLOW_THREADS
        ep = readdir(dirp);
        Py_END_ALLOW_THREADS
        if (ep == NULL) {
            if (errno == 0) {
                break;
            } else {
                Py_DECREF(list);
                list = path_error("listdir", &path);
                goto exit;
            }
        }
        if (ep->d_name[0] == '.' &&
            (NAMLEN(ep) == 1 ||
             (ep->d_name[1] == '.' && NAMLEN(ep) == 2)))
            continue;
        if (arg_is_unicode)
            v = PyUnicode_DecodeFSDefaultAndSize(ep->d_name, NAMLEN(ep));
        else
            v = PyBytes_FromStringAndSize(ep->d_name, NAMLEN(ep));
        if (v == NULL) {
            Py_CLEAR(list);
            break;
        }
        if (PyList_Append(list, v) != 0) {
            Py_DECREF(v);
            Py_CLEAR(list);
            break;
        }
        Py_DECREF(v);
    }

exit:
    if (dirp != NULL) {
        Py_BEGIN_ALLOW_THREADS
        if (fd > -1)
            rewinddir(dirp);
        closedir(dirp);
        Py_END_ALLOW_THREADS
    }

    path_cleanup(&path);

    return list;

#endif /* which OS */
}  /* end of posix_listdir */

#ifdef MS_WINDOWS
/* A helper function for abspath on win32 */
static PyObject *
posix__getfullpathname(PyObject *self, PyObject *args)
{
    const char *path;
    char outbuf[MAX_PATH*2];
    char *temp;
    PyObject *po;

    if (PyArg_ParseTuple(args, "U|:_getfullpathname", &po))
    {
        wchar_t *wpath;
        wchar_t woutbuf[MAX_PATH*2], *woutbufp = woutbuf;
        wchar_t *wtemp;
        DWORD result;
        PyObject *v;

        wpath = PyUnicode_AsUnicode(po);
        if (wpath == NULL)
            return NULL;
        result = GetFullPathNameW(wpath,
                                  Py_ARRAY_LENGTH(woutbuf),
                                  woutbuf, &wtemp);
        if (result > Py_ARRAY_LENGTH(woutbuf)) {
            woutbufp = malloc(result * sizeof(wchar_t));
            if (!woutbufp)
                return PyErr_NoMemory();
            result = GetFullPathNameW(wpath, result, woutbufp, &wtemp);
        }
        if (result)
            v = PyUnicode_FromWideChar(woutbufp, wcslen(woutbufp));
        else
            v = win32_error_object("GetFullPathNameW", po);
        if (woutbufp != woutbuf)
            free(woutbufp);
        return v;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();

    if (!PyArg_ParseTuple (args, "y:_getfullpathname",
                           &path))
        return NULL;
    if (win32_warn_bytes_api())
        return NULL;
    if (!GetFullPathName(path, Py_ARRAY_LENGTH(outbuf),
                         outbuf, &temp)) {
        win32_error("GetFullPathName", path);
        return NULL;
    }
    if (PyUnicode_Check(PyTuple_GetItem(args, 0))) {
        return PyUnicode_Decode(outbuf, strlen(outbuf),
                                Py_FileSystemDefaultEncoding, NULL);
    }
    return PyBytes_FromString(outbuf);
} /* end of posix__getfullpathname */



/* A helper function for samepath on windows */
static PyObject *
posix__getfinalpathname(PyObject *self, PyObject *args)
{
    HANDLE hFile;
    int buf_size;
    wchar_t *target_path;
    int result_length;
    PyObject *po, *result;
    wchar_t *path;

    if (!PyArg_ParseTuple(args, "U|:_getfinalpathname", &po))
        return NULL;
    path = PyUnicode_AsUnicode(po);
    if (path == NULL)
        return NULL;

    if(!check_GetFinalPathNameByHandle()) {
        /* If the OS doesn't have GetFinalPathNameByHandle, return a
           NotImplementedError. */
        return PyErr_Format(PyExc_NotImplementedError,
            "GetFinalPathNameByHandle not available on this platform");
    }

    hFile = CreateFileW(
        path,
        0, /* desired access */
        0, /* share mode */
        NULL, /* security attributes */
        OPEN_EXISTING,
        /* FILE_FLAG_BACKUP_SEMANTICS is required to open a directory */
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
        return win32_error_object("CreateFileW", po);

    /* We have a good handle to the target, use it to determine the
       target path name. */
    buf_size = Py_GetFinalPathNameByHandleW(hFile, 0, 0, VOLUME_NAME_NT);

    if(!buf_size)
        return win32_error_object("GetFinalPathNameByHandle", po);

    target_path = (wchar_t *)malloc((buf_size+1)*sizeof(wchar_t));
    if(!target_path)
        return PyErr_NoMemory();

    result_length = Py_GetFinalPathNameByHandleW(hFile, target_path,
                                                 buf_size, VOLUME_NAME_DOS);
    if(!result_length)
        return win32_error_object("GetFinalPathNamyByHandle", po);

    if(!CloseHandle(hFile))
        return win32_error_object("CloseHandle", po);

    target_path[result_length] = 0;
    result = PyUnicode_FromWideChar(target_path, result_length);
    free(target_path);
    return result;

} /* end of posix__getfinalpathname */

static PyObject *
posix__getfileinformation(PyObject *self, PyObject *args)
{
    HANDLE hFile;
    BY_HANDLE_FILE_INFORMATION info;
    int fd;

    if (!PyArg_ParseTuple(args, "i:_getfileinformation", &fd))
        return NULL;

    if (!_PyVerify_fd(fd))
        return posix_error();

    hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE)
        return posix_error();

    if (!GetFileInformationByHandle(hFile, &info))
        return win32_error("_getfileinformation", NULL);

    return Py_BuildValue("iii", info.dwVolumeSerialNumber,
                                info.nFileIndexHigh,
                                info.nFileIndexLow);
}

PyDoc_STRVAR(posix__isdir__doc__,
"Return true if the pathname refers to an existing directory.");

static PyObject *
posix__isdir(PyObject *self, PyObject *args)
{
    const char *path;
    PyObject *po;
    DWORD attributes;

    if (PyArg_ParseTuple(args, "U|:_isdir", &po)) {
        wchar_t *wpath = PyUnicode_AsUnicode(po);
        if (wpath == NULL)
            return NULL;

        attributes = GetFileAttributesW(wpath);
        if (attributes == INVALID_FILE_ATTRIBUTES)
            Py_RETURN_FALSE;
        goto check;
    }
    /* Drop the argument parsing error as narrow strings
       are also valid. */
    PyErr_Clear();

    if (!PyArg_ParseTuple(args, "y:_isdir", &path))
        return NULL;
    if (win32_warn_bytes_api())
        return NULL;
    attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
        Py_RETURN_FALSE;

check:
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}
#endif /* MS_WINDOWS */

PyDoc_STRVAR(posix_mkdir__doc__,
"mkdir(path, mode=0o777, *, dir_fd=None)\n\n\
Create a directory.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.\n\
\n\
The mode argument is ignored on Windows.");

static PyObject *
posix_mkdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int mode = 0777;
    int dir_fd = DEFAULT_DIR_FD;
    static char *keywords[] = {"path", "mode", "dir_fd", NULL};
    PyObject *return_value = NULL;
    int result;

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|i$O&:mkdir", keywords,
        path_converter, &path, &mode,
#ifdef HAVE_MKDIRAT
        dir_fd_converter, &dir_fd
#else
        dir_fd_unavailable, &dir_fd
#endif
        ))
        return NULL;

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path.wide)
        result = CreateDirectoryW(path.wide, NULL);
    else
        result = CreateDirectoryA(path.narrow, NULL);
    Py_END_ALLOW_THREADS

    if (!result) {
        return_value = win32_error_object("mkdir", path.object);
        goto exit;
    }
#else
    Py_BEGIN_ALLOW_THREADS
#if HAVE_MKDIRAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mkdirat(dir_fd, path.narrow, mode);
    else
#endif
#if ( defined(__WATCOMC__) || defined(PYCC_VACPP) ) && !defined(__QNX__)
        result = mkdir(path.narrow);
#else
        result = mkdir(path.narrow, mode);
#endif
    Py_END_ALLOW_THREADS
    if (result < 0) {
        return_value = path_error("mkdir", &path);
        goto exit;
    }
#endif
    return_value = Py_None;
    Py_INCREF(Py_None);
exit:
    path_cleanup(&path);
    return return_value;
}


/* sys/resource.h is needed for at least: wait3(), wait4(), broken nice. */
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif


#ifdef HAVE_NICE
PyDoc_STRVAR(posix_nice__doc__,
"nice(inc) -> new_priority\n\n\
Decrease the priority of process by inc and return the new priority.");

static PyObject *
posix_nice(PyObject *self, PyObject *args)
{
    int increment, value;

    if (!PyArg_ParseTuple(args, "i:nice", &increment))
        return NULL;

    /* There are two flavours of 'nice': one that returns the new
       priority (as required by almost all standards out there) and the
       Linux/FreeBSD/BSDI one, which returns '0' on success and advices
       the use of getpriority() to get the new priority.

       If we are of the nice family that returns the new priority, we
       need to clear errno before the call, and check if errno is filled
       before calling posix_error() on a returnvalue of -1, because the
       -1 may be the actual new priority! */

    errno = 0;
    value = nice(increment);
#if defined(HAVE_BROKEN_NICE) && defined(HAVE_GETPRIORITY)
    if (value == 0)
        value = getpriority(PRIO_PROCESS, 0);
#endif
    if (value == -1 && errno != 0)
        /* either nice() or getpriority() returned an error */
        return posix_error();
    return PyLong_FromLong((long) value);
}
#endif /* HAVE_NICE */


#ifdef HAVE_GETPRIORITY
PyDoc_STRVAR(posix_getpriority__doc__,
"getpriority(which, who) -> current_priority\n\n\
Get program scheduling priority.");

static PyObject *
posix_getpriority(PyObject *self, PyObject *args)
{
    int which, who, retval;

    if (!PyArg_ParseTuple(args, "ii", &which, &who))
        return NULL;
    errno = 0;
    retval = getpriority(which, who);
    if (errno != 0)
        return posix_error();
    return PyLong_FromLong((long)retval);
}
#endif /* HAVE_GETPRIORITY */


#ifdef HAVE_SETPRIORITY
PyDoc_STRVAR(posix_setpriority__doc__,
"setpriority(which, who, prio) -> None\n\n\
Set program scheduling priority.");

static PyObject *
posix_setpriority(PyObject *self, PyObject *args)
{
    int which, who, prio, retval;

    if (!PyArg_ParseTuple(args, "iii", &which, &who, &prio))
        return NULL;
    retval = setpriority(which, who, prio);
    if (retval == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETPRIORITY */


static PyObject *
internal_rename(PyObject *args, PyObject *kwargs, int is_replace)
{
    char *function_name = is_replace ? "replace" : "rename";
    path_t src;
    path_t dst;
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;
    int dir_fd_specified;
    PyObject *return_value = NULL;
    char format[24];
    static char *keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};

#ifdef MS_WINDOWS
    BOOL result;
    int flags = is_replace ? MOVEFILE_REPLACE_EXISTING : 0;
#else
    int result;
#endif

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    strcpy(format, "O&O&|$O&O&:");
    strcat(format, function_name);
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, format, keywords,
        path_converter, &src,
        path_converter, &dst,
        dir_fd_converter, &src_dir_fd,
        dir_fd_converter, &dst_dir_fd))
        return NULL;

    dir_fd_specified = (src_dir_fd != DEFAULT_DIR_FD) ||
                       (dst_dir_fd != DEFAULT_DIR_FD);
#ifndef HAVE_RENAMEAT
    if (dir_fd_specified) {
        argument_unavailable_error(function_name, "src_dir_fd and dst_dir_fd");
        goto exit;
    }
#endif

    if ((src.narrow && dst.wide) || (src.wide && dst.narrow)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: src and dst must be the same type", function_name);
        goto exit;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (src.wide)
        result = MoveFileExW(src.wide, dst.wide, flags);
    else
        result = MoveFileExA(src.narrow, dst.narrow, flags);
    Py_END_ALLOW_THREADS

    if (!result) {
        return_value = win32_error_object(function_name, dst.object);
        goto exit;
    }

#else
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_RENAMEAT
    if (dir_fd_specified)
        result = renameat(src_dir_fd, src.narrow, dst_dir_fd, dst.narrow);
    else
#endif
        result = rename(src.narrow, dst.narrow);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error(function_name, &dst);
        goto exit;
    }
#endif

    Py_INCREF(Py_None);
    return_value = Py_None;
exit:
    path_cleanup(&src);
    path_cleanup(&dst);
    return return_value;
}

PyDoc_STRVAR(posix_rename__doc__,
"rename(src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n\n\
Rename a file or directory.\n\
\n\
If either src_dir_fd or dst_dir_fd is not None, it should be a file\n\
  descriptor open to a directory, and the respective path string (src or dst)\n\
  should be relative; the path will then be relative to that directory.\n\
src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n\
  If they are unavailable, using them will raise a NotImplementedError.");

static PyObject *
posix_rename(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return internal_rename(args, kwargs, 0);
}

PyDoc_STRVAR(posix_replace__doc__,
"replace(src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n\n\
Rename a file or directory, overwriting the destination.\n\
\n\
If either src_dir_fd or dst_dir_fd is not None, it should be a file\n\
  descriptor open to a directory, and the respective path string (src or dst)\n\
  should be relative; the path will then be relative to that directory.\n\
src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n\
  If they are unavailable, using them will raise a NotImplementedError.");

static PyObject *
posix_replace(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return internal_rename(args, kwargs, 1);
}

PyDoc_STRVAR(posix_rmdir__doc__,
"rmdir(path, *, dir_fd=None)\n\n\
Remove a directory.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

static PyObject *
posix_rmdir(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int dir_fd = DEFAULT_DIR_FD;
    static char *keywords[] = {"path", "dir_fd", NULL};
    int result;
    PyObject *return_value = NULL;

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&:rmdir", keywords,
            path_converter, &path,
#ifdef HAVE_UNLINKAT
            dir_fd_converter, &dir_fd
#else
            dir_fd_unavailable, &dir_fd
#endif
            ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path.wide)
        result = RemoveDirectoryW(path.wide);
    else
        result = RemoveDirectoryA(path.narrow);
    result = !result; /* Windows, success=1, UNIX, success=0 */
#else
#ifdef HAVE_UNLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = unlinkat(dir_fd, path.narrow, AT_REMOVEDIR);
    else
#endif
        result = rmdir(path.narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error("rmdir", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&path);
    return return_value;
}


#ifdef HAVE_SYSTEM
PyDoc_STRVAR(posix_system__doc__,
"system(command) -> exit_status\n\n\
Execute the command (a string) in a subshell.");

static PyObject *
posix_system(PyObject *self, PyObject *args)
{
    long sts;
#ifdef MS_WINDOWS
    wchar_t *command;
    if (!PyArg_ParseTuple(args, "u:system", &command))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    sts = _wsystem(command);
    Py_END_ALLOW_THREADS
#else
    PyObject *command_obj;
    char *command;
    if (!PyArg_ParseTuple(args, "O&:system",
                          PyUnicode_FSConverter, &command_obj))
        return NULL;

    command = PyBytes_AsString(command_obj);
    Py_BEGIN_ALLOW_THREADS
    sts = system(command);
    Py_END_ALLOW_THREADS
    Py_DECREF(command_obj);
#endif
    return PyLong_FromLong(sts);
}
#endif


PyDoc_STRVAR(posix_umask__doc__,
"umask(new_mask) -> old_mask\n\n\
Set the current numeric umask and return the previous umask.");

static PyObject *
posix_umask(PyObject *self, PyObject *args)
{
    int i;
    if (!PyArg_ParseTuple(args, "i:umask", &i))
        return NULL;
    i = (int)umask(i);
    if (i < 0)
        return posix_error();
    return PyLong_FromLong((long)i);
}

#ifdef MS_WINDOWS

/* override the default DeleteFileW behavior so that directory
symlinks can be removed with this function, the same as with
Unix symlinks */
BOOL WINAPI Py_DeleteFileW(LPCWSTR lpFileName)
{
    WIN32_FILE_ATTRIBUTE_DATA info;
    WIN32_FIND_DATAW find_data;
    HANDLE find_data_handle;
    int is_directory = 0;
    int is_link = 0;

    if (GetFileAttributesExW(lpFileName, GetFileExInfoStandard, &info)) {
        is_directory = info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

        /* Get WIN32_FIND_DATA structure for the path to determine if
           it is a symlink */
        if(is_directory &&
           info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            find_data_handle = FindFirstFileW(lpFileName, &find_data);

            if(find_data_handle != INVALID_HANDLE_VALUE) {
                is_link = find_data.dwReserved0 == IO_REPARSE_TAG_SYMLINK;
                FindClose(find_data_handle);
            }
        }
    }

    if (is_directory && is_link)
        return RemoveDirectoryW(lpFileName);

    return DeleteFileW(lpFileName);
}
#endif /* MS_WINDOWS */

PyDoc_STRVAR(posix_unlink__doc__,
"unlink(path, *, dir_fd=None)\n\n\
Remove a file (same as remove()).\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

PyDoc_STRVAR(posix_remove__doc__,
"remove(path, *, dir_fd=None)\n\n\
Remove a file (same as unlink()).\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

static PyObject *
posix_unlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int dir_fd = DEFAULT_DIR_FD;
    static char *keywords[] = {"path", "dir_fd", NULL};
    int result;
    PyObject *return_value = NULL;

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&:unlink", keywords,
            path_converter, &path,
#ifdef HAVE_UNLINKAT
            dir_fd_converter, &dir_fd
#else
            dir_fd_unavailable, &dir_fd
#endif
            ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path.wide)
        result = Py_DeleteFileW(path.wide);
    else
        result = DeleteFileA(path.narrow);
    result = !result; /* Windows, success=1, UNIX, success=0 */
#else
#ifdef HAVE_UNLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = unlinkat(dir_fd, path.narrow, 0);
    else
#endif /* HAVE_UNLINKAT */
        result = unlink(path.narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error("unlink", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&path);
    return return_value;
}


#ifdef HAVE_UNAME
PyDoc_STRVAR(posix_uname__doc__,
"uname() -> (sysname, nodename, release, version, machine)\n\n\
Return a tuple identifying the current operating system.");

static PyObject *
posix_uname(PyObject *self, PyObject *noargs)
{
    struct utsname u;
    int res;

    Py_BEGIN_ALLOW_THREADS
    res = uname(&u);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    return Py_BuildValue("(sssss)",
                         u.sysname,
                         u.nodename,
                         u.release,
                         u.version,
                         u.machine);
}
#endif /* HAVE_UNAME */


PyDoc_STRVAR(posix_utime__doc__,
"utime(path, times=None, *, ns=None, dir_fd=None, follow_symlinks=True)\n\
Set the access and modified time of path.\n\
\n\
path may always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.\n\
\n\
If times is not None, it must be a tuple (atime, mtime);\n\
    atime and mtime should be expressed as float seconds since the epoch.\n\
If ns is not None, it must be a tuple (atime_ns, mtime_ns);\n\
    atime_ns and mtime_ns should be expressed as integer nanoseconds\n\
    since the epoch.\n\
If both times and ns are None, utime uses the current time.\n\
Specifying tuples for both times and ns is an error.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, utime will modify the symbolic link itself instead of the file the\n\
  link points to.\n\
It is an error to use dir_fd or follow_symlinks when specifying path\n\
  as an open file descriptor.\n\
dir_fd and follow_symlinks may not be available on your platform.\n\
  If they are unavailable, using them will raise a NotImplementedError.");

typedef struct {
    int    now;
    time_t atime_s;
    long   atime_ns;
    time_t mtime_s;
    long   mtime_ns;
} utime_t;

/*
 * these macros assume that "utime" is a pointer to a utime_t
 * they also intentionally leak the declaration of a pointer named "time"
 */
#define UTIME_TO_TIMESPEC \
    struct timespec ts[2]; \
    struct timespec *time; \
    if (utime->now) \
        time = NULL; \
    else { \
        ts[0].tv_sec = utime->atime_s; \
        ts[0].tv_nsec = utime->atime_ns; \
        ts[1].tv_sec = utime->mtime_s; \
        ts[1].tv_nsec = utime->mtime_ns; \
        time = ts; \
    } \

#define UTIME_TO_TIMEVAL \
    struct timeval tv[2]; \
    struct timeval *time; \
    if (utime->now) \
        time = NULL; \
    else { \
        tv[0].tv_sec = utime->atime_s; \
        tv[0].tv_usec = utime->atime_ns / 1000; \
        tv[1].tv_sec = utime->mtime_s; \
        tv[1].tv_usec = utime->mtime_ns / 1000; \
        time = tv; \
    } \

#define UTIME_TO_UTIMBUF \
    struct utimbuf u[2]; \
    struct utimbuf *time; \
    if (utime->now) \
        time = NULL; \
    else { \
        u.actime = utime->atime_s; \
        u.modtime = utime->mtime_s; \
        time = u; \
    }

#define UTIME_TO_TIME_T \
    time_t timet[2]; \
    struct timet time; \
    if (utime->now) \
        time = NULL; \
    else { \
        timet[0] = utime->atime_s; \
        timet[1] = utime->mtime_s; \
        time = &timet; \
    } \


#define UTIME_HAVE_DIR_FD (defined(HAVE_FUTIMESAT) || defined(HAVE_UTIMENSAT))

#if UTIME_HAVE_DIR_FD

static int
utime_dir_fd(utime_t *utime, int dir_fd, char *path, int follow_symlinks)
{
#ifdef HAVE_UTIMENSAT
    int flags = follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW;
    UTIME_TO_TIMESPEC;
    return utimensat(dir_fd, path, time, flags);
#elif defined(HAVE_FUTIMESAT)
    UTIME_TO_TIMEVAL;
    /*
     * follow_symlinks will never be false here;
     * we only allow !follow_symlinks and dir_fd together
     * if we have utimensat()
     */
    assert(follow_symlinks);
    return futimesat(dir_fd, path, time);
#endif
}

#endif

#define UTIME_HAVE_FD (defined(HAVE_FUTIMES) || defined(HAVE_FUTIMENS))

#if UTIME_HAVE_FD

static int
utime_fd(utime_t *utime, int fd)
{
#ifdef HAVE_FUTIMENS
    UTIME_TO_TIMESPEC;
    return futimens(fd, time);
#else
    UTIME_TO_TIMEVAL;
    return futimes(fd, time);
#endif
}

#endif


#define UTIME_HAVE_NOFOLLOW_SYMLINKS \
        (defined(HAVE_UTIMENSAT) || defined(HAVE_LUTIMES))

#if UTIME_HAVE_NOFOLLOW_SYMLINKS

static int
utime_nofollow_symlinks(utime_t *utime, char *path)
{
#ifdef HAVE_UTIMENSAT
    UTIME_TO_TIMESPEC;
    return utimensat(DEFAULT_DIR_FD, path, time, AT_SYMLINK_NOFOLLOW);
#else
    UTIME_TO_TIMEVAL;
    return lutimes(path, time);
#endif
}

#endif

#ifndef MS_WINDOWS

static int
utime_default(utime_t *utime, char *path)
{
#ifdef HAVE_UTIMENSAT
    UTIME_TO_TIMESPEC;
    return utimensat(DEFAULT_DIR_FD, path, time, 0);
#elif defined(HAVE_UTIMES)
    UTIME_TO_TIMEVAL;
    return utimes(path, time);
#elif defined(HAVE_UTIME_H)
    UTIME_TO_UTIMBUF;
    return utime(path, time);
#else
    UTIME_TO_TIME_T;
    return utime(path, time);
#endif
}

#endif

static int
split_py_long_to_s_and_ns(PyObject *py_long, time_t *s, long *ns)
{
    int result = 0;
    PyObject *divmod;
    divmod = PyNumber_Divmod(py_long, billion);
    if (!divmod)
        goto exit;
    *s = _PyLong_AsTime_t(PyTuple_GET_ITEM(divmod, 0));
    if ((*s == -1) && PyErr_Occurred())
        goto exit;
    *ns = PyLong_AsLong(PyTuple_GET_ITEM(divmod, 1));
    if ((*ns == -1) && PyErr_Occurred())
        goto exit;

    result = 1;
exit:
    Py_XDECREF(divmod);
    return result;
}

static PyObject *
posix_utime(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    PyObject *times = NULL;
    PyObject *ns = NULL;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;
    char *keywords[] = {"path", "times", "ns", "dir_fd",
                        "follow_symlinks", NULL};

    utime_t utime;

#ifdef MS_WINDOWS
    HANDLE hFile;
    FILETIME atime, mtime;
#else
    int result;
#endif

    PyObject *return_value = NULL;

    memset(&path, 0, sizeof(path));
#if UTIME_HAVE_FD
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
            "O&|O$OO&p:utime", keywords,
            path_converter, &path,
            &times, &ns,
#if UTIME_HAVE_DIR_FD
            dir_fd_converter, &dir_fd,
#else
            dir_fd_unavailable, &dir_fd,
#endif
            &follow_symlinks
            ))
        return NULL;

    if (times && (times != Py_None) && ns) {
        PyErr_SetString(PyExc_ValueError,
                     "utime: you may specify either 'times'"
                     " or 'ns' but not both");
        goto exit;
    }

    if (times && (times != Py_None)) {
        if (!PyTuple_CheckExact(times) || (PyTuple_Size(times) != 2)) {
            PyErr_SetString(PyExc_TypeError,
                         "utime: 'times' must be either"
                         " a tuple of two ints or None");
            goto exit;
        }
        utime.now = 0;
        if (_PyTime_ObjectToTimespec(PyTuple_GET_ITEM(times, 0),
                                     &utime.atime_s, &utime.atime_ns) == -1 ||
            _PyTime_ObjectToTimespec(PyTuple_GET_ITEM(times, 1),
                                     &utime.mtime_s, &utime.mtime_ns) == -1) {
            goto exit;
        }
    }
    else if (ns) {
        if (!PyTuple_CheckExact(ns) || (PyTuple_Size(ns) != 2)) {
            PyErr_SetString(PyExc_TypeError,
                         "utime: 'ns' must be a tuple of two ints");
            goto exit;
        }
        utime.now = 0;
        if (!split_py_long_to_s_and_ns(PyTuple_GET_ITEM(ns, 0),
                                      &utime.atime_s, &utime.atime_ns) ||
            !split_py_long_to_s_and_ns(PyTuple_GET_ITEM(ns, 1),
                                       &utime.mtime_s, &utime.mtime_ns)) {
            goto exit;
        }
    }
    else {
        /* times and ns are both None/unspecified. use "now". */
        utime.now = 1;
    }

#if !UTIME_HAVE_NOFOLLOW_SYMLINKS
    if (follow_symlinks_specified("utime", follow_symlinks))
        goto exit;
#endif

    if (path_and_dir_fd_invalid("utime", &path, dir_fd) ||
        dir_fd_and_fd_invalid("utime", dir_fd, path.fd) ||
        fd_and_follow_symlinks_invalid("utime", path.fd, follow_symlinks))
        goto exit;

#if !defined(HAVE_UTIMENSAT)
    if ((dir_fd != DEFAULT_DIR_FD) && (!follow_symlinks)) {
        PyErr_SetString(PyExc_RuntimeError,
                     "utime: cannot use dir_fd and follow_symlinks "
                     "together on this platform");
        goto exit;
    }
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path.wide)
        hFile = CreateFileW(path.wide, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
    else
        hFile = CreateFileA(path.narrow, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
    Py_END_ALLOW_THREADS
    if (hFile == INVALID_HANDLE_VALUE) {
        win32_error_object("utime", path.object);
        goto exit;
    }

    if (utime.now) {
        SYSTEMTIME now;
        GetSystemTime(&now);
        if (!SystemTimeToFileTime(&now, &mtime) ||
            !SystemTimeToFileTime(&now, &atime)) {
            win32_error("utime", NULL);
            goto exit;
        }
    }
    else {
        time_t_to_FILE_TIME(utime.atime_s, utime.atime_ns, &atime);
        time_t_to_FILE_TIME(utime.mtime_s, utime.mtime_ns, &mtime);
    }
    if (!SetFileTime(hFile, NULL, &atime, &mtime)) {
        /* Avoid putting the file name into the error here,
           as that may confuse the user into believing that
           something is wrong with the file, when it also
           could be the time stamp that gives a problem. */
        win32_error("utime", NULL);
        goto exit;
    }
#else /* MS_WINDOWS */
    Py_BEGIN_ALLOW_THREADS

#if UTIME_HAVE_NOFOLLOW_SYMLINKS
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = utime_nofollow_symlinks(&utime, path.narrow);
    else
#endif

#if UTIME_HAVE_DIR_FD
    if ((dir_fd != DEFAULT_DIR_FD) || (!follow_symlinks))
        result = utime_dir_fd(&utime, dir_fd, path.narrow, follow_symlinks);
    else
#endif

#if UTIME_HAVE_FD
    if (path.fd != -1)
        result = utime_fd(&utime, path.fd);
    else
#endif

    result = utime_default(&utime, path.narrow);

    Py_END_ALLOW_THREADS

    if (result < 0) {
        /* see previous comment about not putting filename in error here */
        return_value = posix_error();
        goto exit;
    }

#endif /* MS_WINDOWS */

    Py_INCREF(Py_None);
    return_value = Py_None;

exit:
    path_cleanup(&path);
#ifdef MS_WINDOWS
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
#endif
    return return_value;
}

/* Process operations */

PyDoc_STRVAR(posix__exit__doc__,
"_exit(status)\n\n\
Exit to the system with specified status, without normal exit processing.");

static PyObject *
posix__exit(PyObject *self, PyObject *args)
{
    int sts;
    if (!PyArg_ParseTuple(args, "i:_exit", &sts))
        return NULL;
    _exit(sts);
    return NULL; /* Make gcc -Wall happy */
}

#if defined(HAVE_EXECV) || defined(HAVE_SPAWNV)
static void
free_string_array(char **array, Py_ssize_t count)
{
    Py_ssize_t i;
    for (i = 0; i < count; i++)
        PyMem_Free(array[i]);
    PyMem_DEL(array);
}

static
int fsconvert_strdup(PyObject *o, char**out)
{
    PyObject *bytes;
    Py_ssize_t size;
    if (!PyUnicode_FSConverter(o, &bytes))
        return 0;
    size = PyBytes_GET_SIZE(bytes);
    *out = PyMem_Malloc(size+1);
    if (!*out)
        return 0;
    memcpy(*out, PyBytes_AsString(bytes), size+1);
    Py_DECREF(bytes);
    return 1;
}
#endif

#if defined(HAVE_EXECV) || defined (HAVE_FEXECVE)
static char**
parse_envlist(PyObject* env, Py_ssize_t *envc_ptr)
{
    char **envlist;
    Py_ssize_t i, pos, envc;
    PyObject *keys=NULL, *vals=NULL;
    PyObject *key, *val, *key2, *val2;
    char *p, *k, *v;
    size_t len;

    i = PyMapping_Size(env);
    if (i < 0)
        return NULL;
    envlist = PyMem_NEW(char *, i + 1);
    if (envlist == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    envc = 0;
    keys = PyMapping_Keys(env);
    vals = PyMapping_Values(env);
    if (!keys || !vals)
        goto error;
    if (!PyList_Check(keys) || !PyList_Check(vals)) {
        PyErr_Format(PyExc_TypeError,
                     "env.keys() or env.values() is not a list");
        goto error;
    }

    for (pos = 0; pos < i; pos++) {
        key = PyList_GetItem(keys, pos);
        val = PyList_GetItem(vals, pos);
        if (!key || !val)
            goto error;

        if (PyUnicode_FSConverter(key, &key2) == 0)
            goto error;
        if (PyUnicode_FSConverter(val, &val2) == 0) {
            Py_DECREF(key2);
            goto error;
        }

#if defined(PYOS_OS2)
        /* Omit Pseudo-Env Vars that Would Confuse Programs if Passed On */
        if (stricmp(k, "BEGINLIBPATH") != 0 && stricmp(k, "ENDLIBPATH") != 0) {
#endif
        k = PyBytes_AsString(key2);
        v = PyBytes_AsString(val2);
        len = PyBytes_GET_SIZE(key2) + PyBytes_GET_SIZE(val2) + 2;

        p = PyMem_NEW(char, len);
        if (p == NULL) {
            PyErr_NoMemory();
            Py_DECREF(key2);
            Py_DECREF(val2);
            goto error;
        }
        PyOS_snprintf(p, len, "%s=%s", k, v);
        envlist[envc++] = p;
        Py_DECREF(key2);
        Py_DECREF(val2);
#if defined(PYOS_OS2)
        }
#endif
    }
    Py_DECREF(vals);
    Py_DECREF(keys);

    envlist[envc] = 0;
    *envc_ptr = envc;
    return envlist;

error:
    Py_XDECREF(keys);
    Py_XDECREF(vals);
    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
    return NULL;
}

static char**
parse_arglist(PyObject* argv, Py_ssize_t *argc)
{
    int i;
    char **argvlist = PyMem_NEW(char *, *argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    for (i = 0; i < *argc; i++) {
        PyObject* item = PySequence_ITEM(argv, i);
        if (item == NULL)
            goto fail;
        if (!fsconvert_strdup(item, &argvlist[i])) {
            Py_DECREF(item);
            goto fail;
        }
        Py_DECREF(item);
    }
    argvlist[*argc] = NULL;
    return argvlist;
fail:
    *argc = i;
    free_string_array(argvlist, *argc);
    return NULL;
}
#endif

#ifdef HAVE_EXECV
PyDoc_STRVAR(posix_execv__doc__,
"execv(path, args)\n\n\
Execute an executable path with arguments, replacing current process.\n\
\n\
    path: path of executable file\n\
    args: tuple or list of strings");

static PyObject *
posix_execv(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    PyObject *argv;
    char **argvlist;
    Py_ssize_t argc;

    /* execv has two arguments: (path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "O&O:execv",
                          PyUnicode_FSConverter,
                          &opath, &argv))
        return NULL;
    path = PyBytes_AsString(opath);
    if (!PyList_Check(argv) && !PyTuple_Check(argv)) {
        PyErr_SetString(PyExc_TypeError,
                        "execv() arg 2 must be a tuple or list");
        Py_DECREF(opath);
        return NULL;
    }
    argc = PySequence_Size(argv);
    if (argc < 1) {
        PyErr_SetString(PyExc_ValueError, "execv() arg 2 must not be empty");
        Py_DECREF(opath);
        return NULL;
    }

    argvlist = parse_arglist(argv, &argc);
    if (argvlist == NULL) {
        Py_DECREF(opath);
        return NULL;
    }

    execv(path, argvlist);

    /* If we get here it's definitely an error */

    free_string_array(argvlist, argc);
    Py_DECREF(opath);
    return posix_error();
}

PyDoc_STRVAR(posix_execve__doc__,
"execve(path, args, env)\n\n\
Execute a path with arguments and environment, replacing current process.\n\
\n\
    path: path of executable file\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings\n\
\n\
On some platforms, you may specify an open file descriptor for path;\n\
  execve will execute the program the file descriptor is open to.\n\
  If this functionality is unavailable, using it raises NotImplementedError.");

static PyObject *
posix_execve(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    PyObject *argv, *env;
    char **argvlist = NULL;
    char **envlist;
    Py_ssize_t argc, envc;
    static char *keywords[] = {"path", "argv", "environment", NULL};

    /* execve has three arguments: (path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FEXECVE
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&OO:execve", keywords,
                          path_converter, &path,
                          &argv, &env
                          ))
        return NULL;

    if (!PyList_Check(argv) && !PyTuple_Check(argv)) {
        PyErr_SetString(PyExc_TypeError,
                        "execve: argv must be a tuple or list");
        goto fail;
    }
    argc = PySequence_Size(argv);
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "execve: environment must be a mapping object");
        goto fail;
    }

    argvlist = parse_arglist(argv, &argc);
    if (argvlist == NULL) {
        goto fail;
    }

    envlist = parse_envlist(env, &envc);
    if (envlist == NULL)
        goto fail;

#ifdef HAVE_FEXECVE
    if (path.fd > -1)
        fexecve(path.fd, argvlist, envlist);
    else
#endif
        execve(path.narrow, argvlist, envlist);

    /* If we get here it's definitely an error */

    path_posix_error("execve", &path);

    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail:
    if (argvlist)
        free_string_array(argvlist, argc);
    path_cleanup(&path);
    return NULL;
}
#endif /* HAVE_EXECV */


#ifdef HAVE_SPAWNV
PyDoc_STRVAR(posix_spawnv__doc__,
"spawnv(mode, path, args)\n\n\
Execute the program 'path' in a new process.\n\
\n\
    mode: mode of process creation\n\
    path: path of executable file\n\
    args: tuple or list of strings");

static PyObject *
posix_spawnv(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    PyObject *argv;
    char **argvlist;
    int mode, i;
    Py_ssize_t argc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* spawnv has three arguments: (mode, path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "iO&O:spawnv", &mode,
                          PyUnicode_FSConverter,
                          &opath, &argv))
        return NULL;
    path = PyBytes_AsString(opath);
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnv() arg 2 must be a tuple or list");
        Py_DECREF(opath);
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        Py_DECREF(opath);
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!fsconvert_strdup((*getitem)(argv, i),
                              &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(
                PyExc_TypeError,
                "spawnv() arg 2 must contain only strings");
            Py_DECREF(opath);
            return NULL;
        }
    }
    argvlist[argc] = NULL;

#if defined(PYOS_OS2) && defined(PYCC_GCC)
    Py_BEGIN_ALLOW_THREADS
    spawnval = spawnv(mode, path, argvlist);
    Py_END_ALLOW_THREADS
#else
    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnv(mode, path, argvlist);
    Py_END_ALLOW_THREADS
#endif

    free_string_array(argvlist, argc);
    Py_DECREF(opath);

    if (spawnval == -1)
        return posix_error();
    else
#if SIZEOF_LONG == SIZEOF_VOID_P
        return Py_BuildValue("l", (long) spawnval);
#else
        return Py_BuildValue("L", (PY_LONG_LONG) spawnval);
#endif
}


PyDoc_STRVAR(posix_spawnve__doc__,
"spawnve(mode, path, args, env)\n\n\
Execute the program 'path' in a new process.\n\
\n\
    mode: mode of process creation\n\
    path: path of executable file\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings");

static PyObject *
posix_spawnve(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    PyObject *argv, *env;
    char **argvlist;
    char **envlist;
    PyObject *res = NULL;
    int mode;
    Py_ssize_t argc, i, envc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    Py_ssize_t lastarg = 0;

    /* spawnve has four arguments: (mode, path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    if (!PyArg_ParseTuple(args, "iO&OO:spawnve", &mode,
                          PyUnicode_FSConverter,
                          &opath, &argv, &env))
        return NULL;
    path = PyBytes_AsString(opath);
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnve() arg 2 must be a tuple or list");
        goto fail_0;
    }
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnve() arg 3 must be a mapping object");
        goto fail_0;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        goto fail_0;
    }
    for (i = 0; i < argc; i++) {
        if (!fsconvert_strdup((*getitem)(argv, i),
                              &argvlist[i]))
        {
            lastarg = i;
            goto fail_1;
        }
    }
    lastarg = argc;
    argvlist[argc] = NULL;

    envlist = parse_envlist(env, &envc);
    if (envlist == NULL)
        goto fail_1;

#if defined(PYOS_OS2) && defined(PYCC_GCC)
    Py_BEGIN_ALLOW_THREADS
    spawnval = spawnve(mode, path, argvlist, envlist);
    Py_END_ALLOW_THREADS
#else
    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnve(mode, path, argvlist, envlist);
    Py_END_ALLOW_THREADS
#endif

    if (spawnval == -1)
        (void) posix_error();
    else
#if SIZEOF_LONG == SIZEOF_VOID_P
        res = Py_BuildValue("l", (long) spawnval);
#else
        res = Py_BuildValue("L", (PY_LONG_LONG) spawnval);
#endif

    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
  fail_0:
    Py_DECREF(opath);
    return res;
}

/* OS/2 supports spawnvp & spawnvpe natively */
#if defined(PYOS_OS2)
PyDoc_STRVAR(posix_spawnvp__doc__,
"spawnvp(mode, file, args)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
    mode: mode of process creation\n\
    file: executable file name\n\
    args: tuple or list of strings");

static PyObject *
posix_spawnvp(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    PyObject *argv;
    char **argvlist;
    int mode, i, argc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* spawnvp has three arguments: (mode, path, argv), where
       argv is a list or tuple of strings. */

    if (!PyArg_ParseTuple(args, "iO&O:spawnvp", &mode,
                          PyUnicode_FSConverter,
                          &opath, &argv))
        return NULL;
    path = PyBytes_AsString(opath);
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvp() arg 2 must be a tuple or list");
        Py_DECREF(opath);
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        Py_DECREF(opath);
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!fsconvert_strdup((*getitem)(argv, i),
                              &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(
                PyExc_TypeError,
                "spawnvp() arg 2 must contain only strings");
            Py_DECREF(opath);
            return NULL;
        }
    }
    argvlist[argc] = NULL;

    Py_BEGIN_ALLOW_THREADS
#if defined(PYCC_GCC)
    spawnval = spawnvp(mode, path, argvlist);
#else
    spawnval = _spawnvp(mode, path, argvlist);
#endif
    Py_END_ALLOW_THREADS

    free_string_array(argvlist, argc);
    Py_DECREF(opath);

    if (spawnval == -1)
        return posix_error();
    else
        return Py_BuildValue("l", (long) spawnval);
}


PyDoc_STRVAR(posix_spawnvpe__doc__,
"spawnvpe(mode, file, args, env)\n\n\
Execute the program 'file' in a new process, using the environment\n\
search path to find the file.\n\
\n\
    mode: mode of process creation\n\
    file: executable file name\n\
    args: tuple or list of arguments\n\
    env: dictionary of strings mapping to strings");

static PyObject *
posix_spawnvpe(PyObject *self, PyObject *args)
{
    PyObject *opath;
    char *path;
    PyObject *argv, *env;
    char **argvlist;
    char **envlist;
    PyObject *res=NULL;
    int mode;
    Py_ssize_t argc, i, envc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    int lastarg = 0;

    /* spawnvpe has four arguments: (mode, path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    if (!PyArg_ParseTuple(args, "ietOO:spawnvpe", &mode,
                          PyUnicode_FSConverter,
                          &opath, &argv, &env))
        return NULL;
    path = PyBytes_AsString(opath);
    if (PyList_Check(argv)) {
        argc = PyList_Size(argv);
        getitem = PyList_GetItem;
    }
    else if (PyTuple_Check(argv)) {
        argc = PyTuple_Size(argv);
        getitem = PyTuple_GetItem;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvpe() arg 2 must be a tuple or list");
        goto fail_0;
    }
    if (!PyMapping_Check(env)) {
        PyErr_SetString(PyExc_TypeError,
                        "spawnvpe() arg 3 must be a mapping object");
        goto fail_0;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        PyErr_NoMemory();
        goto fail_0;
    }
    for (i = 0; i < argc; i++) {
        if (!fsconvert_strdup((*getitem)(argv, i),
                              &argvlist[i]))
        {
            lastarg = i;
            goto fail_1;
        }
    }
    lastarg = argc;
    argvlist[argc] = NULL;

    envlist = parse_envlist(env, &envc);
    if (envlist == NULL)
        goto fail_1;

    Py_BEGIN_ALLOW_THREADS
#if defined(PYCC_GCC)
    spawnval = spawnvpe(mode, path, argvlist, envlist);
#else
    spawnval = _spawnvpe(mode, path, argvlist, envlist);
#endif
    Py_END_ALLOW_THREADS

    if (spawnval == -1)
        (void) posix_error();
    else
        res = Py_BuildValue("l", (long) spawnval);

    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
  fail_0:
    Py_DECREF(opath);
    return res;
}
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */


#ifdef HAVE_FORK1
PyDoc_STRVAR(posix_fork1__doc__,
"fork1() -> pid\n\n\
Fork a child process with a single multiplexed (i.e., not bound) thread.\n\
\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
posix_fork1(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    int result = 0;
    _PyImport_AcquireLock();
    pid = fork1();
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return posix_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return PyLong_FromPid(pid);
}
#endif


#ifdef HAVE_FORK
PyDoc_STRVAR(posix_fork__doc__,
"fork() -> pid\n\n\
Fork a child process.\n\
Return 0 to child process and PID of child to parent process.");

static PyObject *
posix_fork(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    int result = 0;
    _PyImport_AcquireLock();
    pid = fork();
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return posix_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return PyLong_FromPid(pid);
}
#endif

#ifdef HAVE_SCHED_H

#ifdef HAVE_SCHED_GET_PRIORITY_MAX

PyDoc_STRVAR(posix_sched_get_priority_max__doc__,
"sched_get_priority_max(policy)\n\n\
Get the maximum scheduling priority for *policy*.");

static PyObject *
posix_sched_get_priority_max(PyObject *self, PyObject *args)
{
    int policy, max;

    if (!PyArg_ParseTuple(args, "i:sched_get_priority_max", &policy))
        return NULL;
    max = sched_get_priority_max(policy);
    if (max < 0)
        return posix_error();
    return PyLong_FromLong(max);
}

PyDoc_STRVAR(posix_sched_get_priority_min__doc__,
"sched_get_priority_min(policy)\n\n\
Get the minimum scheduling priority for *policy*.");

static PyObject *
posix_sched_get_priority_min(PyObject *self, PyObject *args)
{
    int policy, min;

    if (!PyArg_ParseTuple(args, "i:sched_get_priority_min", &policy))
        return NULL;
    min = sched_get_priority_min(policy);
    if (min < 0)
        return posix_error();
    return PyLong_FromLong(min);
}

#endif /* HAVE_SCHED_GET_PRIORITY_MAX */

#ifdef HAVE_SCHED_SETSCHEDULER

PyDoc_STRVAR(posix_sched_getscheduler__doc__,
"sched_getscheduler(pid)\n\n\
Get the scheduling policy for the process with a PID of *pid*.\n\
Passing a PID of 0 returns the scheduling policy for the calling process.");

static PyObject *
posix_sched_getscheduler(PyObject *self, PyObject *args)
{
    pid_t pid;
    int policy;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID ":sched_getscheduler", &pid))
        return NULL;
    policy = sched_getscheduler(pid);
    if (policy < 0)
        return posix_error();
    return PyLong_FromLong(policy);
}

#endif

#if defined(HAVE_SCHED_SETSCHEDULER) || defined(HAVE_SCHED_SETPARAM)

static PyObject *
sched_param_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *res, *priority;
    static char *kwlist[] = {"sched_priority", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:sched_param", kwlist, &priority))
        return NULL;
    res = PyStructSequence_New(type);
    if (!res)
        return NULL;
    Py_INCREF(priority);
    PyStructSequence_SET_ITEM(res, 0, priority);
    return res;
}

PyDoc_STRVAR(sched_param__doc__,
"sched_param(sched_priority): A scheduling parameter.\n\n\
Current has only one field: sched_priority");

static PyStructSequence_Field sched_param_fields[] = {
    {"sched_priority", "the scheduling priority"},
    {0}
};

static PyStructSequence_Desc sched_param_desc = {
    "sched_param", /* name */
    sched_param__doc__, /* doc */
    sched_param_fields,
    1
};

static int
convert_sched_param(PyObject *param, struct sched_param *res)
{
    long priority;

    if (Py_TYPE(param) != &SchedParamType) {
        PyErr_SetString(PyExc_TypeError, "must have a sched_param object");
        return 0;
    }
    priority = PyLong_AsLong(PyStructSequence_GET_ITEM(param, 0));
    if (priority == -1 && PyErr_Occurred())
        return 0;
    if (priority > INT_MAX || priority < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError, "sched_priority out of range");
        return 0;
    }
    res->sched_priority = Py_SAFE_DOWNCAST(priority, long, int);
    return 1;
}

#endif

#ifdef HAVE_SCHED_SETSCHEDULER

PyDoc_STRVAR(posix_sched_setscheduler__doc__,
"sched_setscheduler(pid, policy, param)\n\n\
Set the scheduling policy, *policy*, for *pid*.\n\
If *pid* is 0, the calling process is changed.\n\
*param* is an instance of sched_param.");

static PyObject *
posix_sched_setscheduler(PyObject *self, PyObject *args)
{
    pid_t pid;
    int policy;
    struct sched_param param;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "iO&:sched_setscheduler",
                          &pid, &policy, &convert_sched_param, &param))
        return NULL;

    /*
    ** sched_setscheduler() returns 0 in Linux, but the previous
    ** scheduling policy under Solaris/Illumos, and others.
    ** On error, -1 is returned in all Operating Systems.
    */
    if (sched_setscheduler(pid, policy, &param) == -1)
        return posix_error();
    Py_RETURN_NONE;
}

#endif

#ifdef HAVE_SCHED_SETPARAM

PyDoc_STRVAR(posix_sched_getparam__doc__,
"sched_getparam(pid) -> sched_param\n\n\
Returns scheduling parameters for the process with *pid* as an instance of the\n\
sched_param class. A PID of 0 means the calling process.");

static PyObject *
posix_sched_getparam(PyObject *self, PyObject *args)
{
    pid_t pid;
    struct sched_param param;
    PyObject *res, *priority;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID ":sched_getparam", &pid))
        return NULL;
    if (sched_getparam(pid, &param))
        return posix_error();
    res = PyStructSequence_New(&SchedParamType);
    if (!res)
        return NULL;
    priority = PyLong_FromLong(param.sched_priority);
    if (!priority) {
        Py_DECREF(res);
        return NULL;
    }
    PyStructSequence_SET_ITEM(res, 0, priority);
    return res;
}

PyDoc_STRVAR(posix_sched_setparam__doc__,
"sched_setparam(pid, param)\n\n\
Set scheduling parameters for a process with PID *pid*.\n\
A PID of 0 means the calling process.");

static PyObject *
posix_sched_setparam(PyObject *self, PyObject *args)
{
    pid_t pid;
    struct sched_param param;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "O&:sched_setparam",
                          &pid, &convert_sched_param, &param))
        return NULL;
    if (sched_setparam(pid, &param))
        return posix_error();
    Py_RETURN_NONE;
}

#endif

#ifdef HAVE_SCHED_RR_GET_INTERVAL

PyDoc_STRVAR(posix_sched_rr_get_interval__doc__,
"sched_rr_get_interval(pid) -> float\n\n\
Return the round-robin quantum for the process with PID *pid* in seconds.");

static PyObject *
posix_sched_rr_get_interval(PyObject *self, PyObject *args)
{
    pid_t pid;
    struct timespec interval;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID ":sched_rr_get_interval", &pid))
        return NULL;
    if (sched_rr_get_interval(pid, &interval))
        return posix_error();
    return PyFloat_FromDouble((double)interval.tv_sec + 1e-9*interval.tv_nsec);
}

#endif

PyDoc_STRVAR(posix_sched_yield__doc__,
"sched_yield()\n\n\
Voluntarily relinquish the CPU.");

static PyObject *
posix_sched_yield(PyObject *self, PyObject *noargs)
{
    if (sched_yield())
        return posix_error();
    Py_RETURN_NONE;
}

#ifdef HAVE_SCHED_SETAFFINITY

typedef struct {
    PyObject_HEAD;
    Py_ssize_t size;
    int ncpus;
    cpu_set_t *set;
} Py_cpu_set;

static PyTypeObject cpu_set_type;

static void
cpu_set_dealloc(Py_cpu_set *set)
{
    assert(set->set);
    CPU_FREE(set->set);
    Py_TYPE(set)->tp_free(set);
}

static Py_cpu_set *
make_new_cpu_set(PyTypeObject *type, Py_ssize_t size)
{
    Py_cpu_set *set;

    if (size < 0) {
        PyErr_SetString(PyExc_ValueError, "negative size");
        return NULL;
    }
    set = (Py_cpu_set *)type->tp_alloc(type, 0);
    if (!set)
        return NULL;
    set->ncpus = size;
    set->size = CPU_ALLOC_SIZE(size);
    set->set = CPU_ALLOC(size);
    if (!set->set) {
        type->tp_free(set);
        PyErr_NoMemory();
        return NULL;
    }
    CPU_ZERO_S(set->size, set->set);
    return set;
}

static PyObject *
cpu_set_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    int size;

    if (!_PyArg_NoKeywords("cpu_set()", kwargs) ||
        !PyArg_ParseTuple(args, "i:cpu_set", &size))
        return NULL;
    return (PyObject *)make_new_cpu_set(type, size);
}

static PyObject *
cpu_set_repr(Py_cpu_set *set)
{
    return PyUnicode_FromFormat("<cpu_set with %li entries>", set->ncpus);
}

static Py_ssize_t
cpu_set_len(Py_cpu_set *set)
{
    return set->ncpus;
}

static int
_get_cpu(Py_cpu_set *set, const char *requester, PyObject *args)
{
    int cpu;
    if (!PyArg_ParseTuple(args, requester, &cpu))
        return -1;
    if (cpu < 0) {
        PyErr_SetString(PyExc_ValueError, "cpu < 0 not valid");
        return -1;
    }
    if (cpu >= set->ncpus) {
        PyErr_SetString(PyExc_ValueError, "cpu too large for set");
        return -1;
    }
    return cpu;
}

PyDoc_STRVAR(cpu_set_set_doc,
"cpu_set.set(i)\n\n\
Add CPU *i* to the set.");

static PyObject *
cpu_set_set(Py_cpu_set *set, PyObject *args)
{
    int cpu = _get_cpu(set, "i|set", args);
    if (cpu == -1)
        return NULL;
    CPU_SET_S(cpu, set->size, set->set);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(cpu_set_count_doc,
"cpu_set.count() -> int\n\n\
Return the number of CPUs active in the set.");

static PyObject *
cpu_set_count(Py_cpu_set *set, PyObject *noargs)
{
    return PyLong_FromLong(CPU_COUNT_S(set->size, set->set));
}

PyDoc_STRVAR(cpu_set_clear_doc,
"cpu_set.clear(i)\n\n\
Remove CPU *i* from the set.");

static PyObject *
cpu_set_clear(Py_cpu_set *set, PyObject *args)
{
    int cpu = _get_cpu(set, "i|clear", args);
    if (cpu == -1)
        return NULL;
    CPU_CLR_S(cpu, set->size, set->set);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(cpu_set_isset_doc,
"cpu_set.isset(i) -> bool\n\n\
Test if CPU *i* is in the set.");

static PyObject *
cpu_set_isset(Py_cpu_set *set, PyObject *args)
{
    int cpu = _get_cpu(set, "i|isset", args);
    if (cpu == -1)
        return NULL;
    if (CPU_ISSET_S(cpu, set->size, set->set))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

PyDoc_STRVAR(cpu_set_zero_doc,
"cpu_set.zero()\n\n\
Clear the cpu_set.");

static PyObject *
cpu_set_zero(Py_cpu_set *set, PyObject *noargs)
{
    CPU_ZERO_S(set->size, set->set);
    Py_RETURN_NONE;
}

static PyObject *
cpu_set_richcompare(Py_cpu_set *set, Py_cpu_set *other, int op)
{
    int eq;

    if ((op != Py_EQ && op != Py_NE) || Py_TYPE(other) != &cpu_set_type)
        Py_RETURN_NOTIMPLEMENTED;

    eq = set->ncpus == other->ncpus && CPU_EQUAL_S(set->size, set->set, other->set);
    if ((op == Py_EQ) ? eq : !eq)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

#define CPU_SET_BINOP(name, op) \
    static PyObject * \
    do_cpu_set_##name(Py_cpu_set *left, Py_cpu_set *right, Py_cpu_set *res) { \
        if (res) { \
            Py_INCREF(res); \
        } \
        else { \
            res = make_new_cpu_set(&cpu_set_type, left->ncpus); \
            if (!res) \
                return NULL; \
        } \
        if (Py_TYPE(right) != &cpu_set_type || left->ncpus != right->ncpus) { \
            Py_DECREF(res); \
            Py_RETURN_NOTIMPLEMENTED; \
        } \
        assert(left->size == right->size && right->size == res->size); \
        op(res->size, res->set, left->set, right->set); \
        return (PyObject *)res; \
    } \
    static PyObject * \
    cpu_set_##name(Py_cpu_set *left, Py_cpu_set *right) { \
        return do_cpu_set_##name(left, right, NULL); \
    } \
    static PyObject * \
    cpu_set_i##name(Py_cpu_set *left, Py_cpu_set *right) { \
        return do_cpu_set_##name(left, right, left); \
    } \

CPU_SET_BINOP(and, CPU_AND_S)
CPU_SET_BINOP(or, CPU_OR_S)
CPU_SET_BINOP(xor, CPU_XOR_S)
#undef CPU_SET_BINOP

PyDoc_STRVAR(cpu_set_doc,
"cpu_set(size)\n\n\
Create an empty mask of CPUs.");

static PyNumberMethods cpu_set_as_number = {
    0,                                  /*nb_add*/
    0,                                  /*nb_subtract*/
    0,                                  /*nb_multiply*/
    0,                                  /*nb_remainder*/
    0,                                  /*nb_divmod*/
    0,                                  /*nb_power*/
    0,                                  /*nb_negative*/
    0,                                  /*nb_positive*/
    0,                                  /*nb_absolute*/
    0,                                  /*nb_bool*/
    0,                                  /*nb_invert*/
    0,                                  /*nb_lshift*/
    0,                                  /*nb_rshift*/
    (binaryfunc)cpu_set_and,            /*nb_and*/
    (binaryfunc)cpu_set_xor,            /*nb_xor*/
    (binaryfunc)cpu_set_or,             /*nb_or*/
    0,                                  /*nb_int*/
    0,                                  /*nb_reserved*/
    0,                                  /*nb_float*/
    0,                                  /*nb_inplace_add*/
    0,                                  /*nb_inplace_subtract*/
    0,                                  /*nb_inplace_multiply*/
    0,                                  /*nb_inplace_remainder*/
    0,                                  /*nb_inplace_power*/
    0,                                  /*nb_inplace_lshift*/
    0,                                  /*nb_inplace_rshift*/
    (binaryfunc)cpu_set_iand,           /*nb_inplace_and*/
    (binaryfunc)cpu_set_ixor,           /*nb_inplace_xor*/
    (binaryfunc)cpu_set_ior,            /*nb_inplace_or*/
};

static PySequenceMethods cpu_set_as_sequence = {
    (lenfunc)cpu_set_len,                            /* sq_length */
};

static PyMethodDef cpu_set_methods[] = {
    {"clear", (PyCFunction)cpu_set_clear, METH_VARARGS, cpu_set_clear_doc},
    {"count", (PyCFunction)cpu_set_count, METH_NOARGS, cpu_set_count_doc},
    {"isset", (PyCFunction)cpu_set_isset, METH_VARARGS, cpu_set_isset_doc},
    {"set", (PyCFunction)cpu_set_set, METH_VARARGS, cpu_set_set_doc},
    {"zero", (PyCFunction)cpu_set_zero, METH_NOARGS, cpu_set_zero_doc},
    {NULL, NULL}   /* sentinel */
};

static PyTypeObject cpu_set_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "posix.cpu_set",                    /* tp_name */
    sizeof(Py_cpu_set),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)cpu_set_dealloc,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    (reprfunc)cpu_set_repr,             /* tp_repr */
    &cpu_set_as_number,                 /* tp_as_number */
    &cpu_set_as_sequence,               /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    PyObject_HashNotImplemented,        /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    cpu_set_doc,                        /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    (richcmpfunc)cpu_set_richcompare,   /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    cpu_set_methods,                    /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    cpu_set_new,                        /* tp_new */
    PyObject_Del,                       /* tp_free */
};

PyDoc_STRVAR(posix_sched_setaffinity__doc__,
"sched_setaffinity(pid, cpu_set)\n\n\
Set the affinity of the process with PID *pid* to *cpu_set*.");

static PyObject *
posix_sched_setaffinity(PyObject *self, PyObject *args)
{
    pid_t pid;
    Py_cpu_set *cpu_set;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "O!:sched_setaffinity",
                          &pid, &cpu_set_type, &cpu_set))
        return NULL;
    if (sched_setaffinity(pid, cpu_set->size, cpu_set->set))
        return posix_error();
    Py_RETURN_NONE;
}

PyDoc_STRVAR(posix_sched_getaffinity__doc__,
"sched_getaffinity(pid, ncpus) -> cpu_set\n\n\
Return the affinity of the process with PID *pid*.\n\
The returned cpu_set will be of size *ncpus*.");

static PyObject *
posix_sched_getaffinity(PyObject *self, PyObject *args)
{
    pid_t pid;
    int ncpus;
    Py_cpu_set *res;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:sched_getaffinity",
                          &pid, &ncpus))
        return NULL;
    res = make_new_cpu_set(&cpu_set_type, ncpus);
    if (!res)
        return NULL;
    if (sched_getaffinity(pid, res->size, res->set)) {
        Py_DECREF(res);
        return posix_error();
    }
    return (PyObject *)res;
}

#endif /* HAVE_SCHED_SETAFFINITY */

#endif /* HAVE_SCHED_H */

/* AIX uses /dev/ptc but is otherwise the same as /dev/ptmx */
/* IRIX has both /dev/ptc and /dev/ptmx, use ptmx */
#if defined(HAVE_DEV_PTC) && !defined(HAVE_DEV_PTMX)
#define DEV_PTY_FILE "/dev/ptc"
#define HAVE_DEV_PTMX
#else
#define DEV_PTY_FILE "/dev/ptmx"
#endif

#if defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX)
#ifdef HAVE_PTY_H
#include <pty.h>
#else
#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#else
#ifdef HAVE_UTIL_H
#include <util.h>
#endif /* HAVE_UTIL_H */
#endif /* HAVE_LIBUTIL_H */
#endif /* HAVE_PTY_H */
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif
#endif /* defined(HAVE_OPENPTY) || defined(HAVE_FORKPTY) || defined(HAVE_DEV_PTMX */

#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
PyDoc_STRVAR(posix_openpty__doc__,
"openpty() -> (master_fd, slave_fd)\n\n\
Open a pseudo-terminal, returning open fd's for both master and slave end.\n");

static PyObject *
posix_openpty(PyObject *self, PyObject *noargs)
{
    int master_fd, slave_fd;
#ifndef HAVE_OPENPTY
    char * slave_name;
#endif
#if defined(HAVE_DEV_PTMX) && !defined(HAVE_OPENPTY) && !defined(HAVE__GETPTY)
    PyOS_sighandler_t sig_saved;
#ifdef sun
    extern char *ptsname(int fildes);
#endif
#endif

#ifdef HAVE_OPENPTY
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) != 0)
        return posix_error();
#elif defined(HAVE__GETPTY)
    slave_name = _getpty(&master_fd, O_RDWR, 0666, 0);
    if (slave_name == NULL)
        return posix_error();

    slave_fd = open(slave_name, O_RDWR);
    if (slave_fd < 0)
        return posix_error();
#else
    master_fd = open(DEV_PTY_FILE, O_RDWR | O_NOCTTY); /* open master */
    if (master_fd < 0)
        return posix_error();
    sig_saved = PyOS_setsig(SIGCHLD, SIG_DFL);
    /* change permission of slave */
    if (grantpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        return posix_error();
    }
    /* unlock slave */
    if (unlockpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        return posix_error();
    }
    PyOS_setsig(SIGCHLD, sig_saved);
    slave_name = ptsname(master_fd); /* get name of slave */
    if (slave_name == NULL)
        return posix_error();
    slave_fd = open(slave_name, O_RDWR | O_NOCTTY); /* open slave */
    if (slave_fd < 0)
        return posix_error();
#if !defined(__CYGWIN__) && !defined(HAVE_DEV_PTC)
    ioctl(slave_fd, I_PUSH, "ptem"); /* push ptem */
    ioctl(slave_fd, I_PUSH, "ldterm"); /* push ldterm */
#ifndef __hpux
    ioctl(slave_fd, I_PUSH, "ttcompat"); /* push ttcompat */
#endif /* __hpux */
#endif /* HAVE_CYGWIN */
#endif /* HAVE_OPENPTY */

    return Py_BuildValue("(ii)", master_fd, slave_fd);

}
#endif /* defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX) */

#ifdef HAVE_FORKPTY
PyDoc_STRVAR(posix_forkpty__doc__,
"forkpty() -> (pid, master_fd)\n\n\
Fork a new process with a new pseudo-terminal as controlling tty.\n\n\
Like fork(), return 0 as pid to child process, and PID of child to parent.\n\
To both, return fd of newly opened pseudo-terminal.\n");

static PyObject *
posix_forkpty(PyObject *self, PyObject *noargs)
{
    int master_fd = -1, result = 0;
    pid_t pid;

    _PyImport_AcquireLock();
    pid = forkpty(&master_fd, NULL, NULL, NULL);
    if (pid == 0) {
        /* child: this clobbers and resets the import lock. */
        PyOS_AfterFork();
    } else {
        /* parent: release the import lock. */
        result = _PyImport_ReleaseLock();
    }
    if (pid == -1)
        return posix_error();
    if (result < 0) {
        /* Don't clobber the OSError if the fork failed. */
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
        return NULL;
    }
    return Py_BuildValue("(Ni)", PyLong_FromPid(pid), master_fd);
}
#endif


#ifdef HAVE_GETEGID
PyDoc_STRVAR(posix_getegid__doc__,
"getegid() -> egid\n\n\
Return the current process's effective group id.");

static PyObject *
posix_getegid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromLong((long)getegid());
}
#endif


#ifdef HAVE_GETEUID
PyDoc_STRVAR(posix_geteuid__doc__,
"geteuid() -> euid\n\n\
Return the current process's effective user id.");

static PyObject *
posix_geteuid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromLong((long)geteuid());
}
#endif


#ifdef HAVE_GETGID
PyDoc_STRVAR(posix_getgid__doc__,
"getgid() -> gid\n\n\
Return the current process's group id.");

static PyObject *
posix_getgid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromLong((long)getgid());
}
#endif


PyDoc_STRVAR(posix_getpid__doc__,
"getpid() -> pid\n\n\
Return the current process id");

static PyObject *
posix_getpid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromPid(getpid());
}

#ifdef HAVE_GETGROUPLIST
PyDoc_STRVAR(posix_getgrouplist__doc__,
"getgrouplist(user, group) -> list of groups to which a user belongs\n\n\
Returns a list of groups to which a user belongs.\n\n\
    user: username to lookup\n\
    group: base group id of the user");

static PyObject *
posix_getgrouplist(PyObject *self, PyObject *args)
{
#ifdef NGROUPS_MAX
#define MAX_GROUPS NGROUPS_MAX
#else
    /* defined to be 16 on Solaris7, so this should be a small number */
#define MAX_GROUPS 64
#endif

    const char *user;
    int i, ngroups;
    PyObject *list;
#ifdef __APPLE__
    int *groups, basegid;
#else
    gid_t *groups, basegid;
#endif
    ngroups = MAX_GROUPS;

    if (!PyArg_ParseTuple(args, "si", &user, &basegid))
        return NULL;

#ifdef __APPLE__
    groups = PyMem_Malloc(ngroups * sizeof(int));
#else
    groups = PyMem_Malloc(ngroups * sizeof(gid_t));
#endif
    if (groups == NULL)
        return PyErr_NoMemory();

    if (getgrouplist(user, basegid, groups, &ngroups) == -1) {
        PyMem_Del(groups);
        return posix_error();
    }

    list = PyList_New(ngroups);
    if (list == NULL) {
        PyMem_Del(groups);
        return NULL;
    }

    for (i = 0; i < ngroups; i++) {
        PyObject *o = PyLong_FromUnsignedLong((unsigned long)groups[i]);
        if (o == NULL) {
            Py_DECREF(list);
            PyMem_Del(groups);
            return NULL;
        }
        PyList_SET_ITEM(list, i, o);
    }

    PyMem_Del(groups);

    return list;
}
#endif

#ifdef HAVE_GETGROUPS
PyDoc_STRVAR(posix_getgroups__doc__,
"getgroups() -> list of group IDs\n\n\
Return list of supplemental group IDs for the process.");

static PyObject *
posix_getgroups(PyObject *self, PyObject *noargs)
{
    PyObject *result = NULL;

#ifdef NGROUPS_MAX
#define MAX_GROUPS NGROUPS_MAX
#else
    /* defined to be 16 on Solaris7, so this should be a small number */
#define MAX_GROUPS 64
#endif
    gid_t grouplist[MAX_GROUPS];

    /* On MacOSX getgroups(2) can return more than MAX_GROUPS results
     * This is a helper variable to store the intermediate result when
     * that happens.
     *
     * To keep the code readable the OSX behaviour is unconditional,
     * according to the POSIX spec this should be safe on all unix-y
     * systems.
     */
    gid_t* alt_grouplist = grouplist;
    int n;

    n = getgroups(MAX_GROUPS, grouplist);
    if (n < 0) {
        if (errno == EINVAL) {
            n = getgroups(0, NULL);
            if (n == -1) {
                return posix_error();
            }
            if (n == 0) {
                /* Avoid malloc(0) */
                alt_grouplist = grouplist;
            } else {
                alt_grouplist = PyMem_Malloc(n * sizeof(gid_t));
                if (alt_grouplist == NULL) {
                    errno = EINVAL;
                    return posix_error();
                }
                n = getgroups(n, alt_grouplist);
                if (n == -1) {
                    PyMem_Free(alt_grouplist);
                    return posix_error();
                }
            }
        } else {
            return posix_error();
        }
    }
    result = PyList_New(n);
    if (result != NULL) {
        int i;
        for (i = 0; i < n; ++i) {
            PyObject *o = PyLong_FromLong((long)alt_grouplist[i]);
            if (o == NULL) {
                Py_DECREF(result);
                result = NULL;
                break;
            }
            PyList_SET_ITEM(result, i, o);
        }
    }

    if (alt_grouplist != grouplist) {
        PyMem_Free(alt_grouplist);
    }

    return result;
}
#endif

#ifdef HAVE_INITGROUPS
PyDoc_STRVAR(posix_initgroups__doc__,
"initgroups(username, gid) -> None\n\n\
Call the system initgroups() to initialize the group access list with all of\n\
the groups of which the specified username is a member, plus the specified\n\
group id.");

static PyObject *
posix_initgroups(PyObject *self, PyObject *args)
{
    PyObject *oname;
    char *username;
    int res;
    long gid;

    if (!PyArg_ParseTuple(args, "O&l:initgroups",
                          PyUnicode_FSConverter, &oname, &gid))
        return NULL;
    username = PyBytes_AS_STRING(oname);

    res = initgroups(username, (gid_t) gid);
    Py_DECREF(oname);
    if (res == -1)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_GETPGID
PyDoc_STRVAR(posix_getpgid__doc__,
"getpgid(pid) -> pgid\n\n\
Call the system call getpgid().");

static PyObject *
posix_getpgid(PyObject *self, PyObject *args)
{
    pid_t pid, pgid;
    if (!PyArg_ParseTuple(args, _Py_PARSE_PID ":getpgid", &pid))
        return NULL;
    pgid = getpgid(pid);
    if (pgid < 0)
        return posix_error();
    return PyLong_FromPid(pgid);
}
#endif /* HAVE_GETPGID */


#ifdef HAVE_GETPGRP
PyDoc_STRVAR(posix_getpgrp__doc__,
"getpgrp() -> pgrp\n\n\
Return the current process group id.");

static PyObject *
posix_getpgrp(PyObject *self, PyObject *noargs)
{
#ifdef GETPGRP_HAVE_ARG
    return PyLong_FromPid(getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
    return PyLong_FromPid(getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}
#endif /* HAVE_GETPGRP */


#ifdef HAVE_SETPGRP
PyDoc_STRVAR(posix_setpgrp__doc__,
"setpgrp()\n\n\
Make this process the process group leader.");

static PyObject *
posix_setpgrp(PyObject *self, PyObject *noargs)
{
#ifdef SETPGRP_HAVE_ARG
    if (setpgrp(0, 0) < 0)
#else /* SETPGRP_HAVE_ARG */
    if (setpgrp() < 0)
#endif /* SETPGRP_HAVE_ARG */
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}

#endif /* HAVE_SETPGRP */

#ifdef HAVE_GETPPID

#ifdef MS_WINDOWS
#include <tlhelp32.h>

static PyObject*
win32_getppid()
{
    HANDLE snapshot;
    pid_t mypid;
    PyObject* result = NULL;
    BOOL have_record;
    PROCESSENTRY32 pe;

    mypid = getpid(); /* This function never fails */

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return PyErr_SetFromWindowsErr(GetLastError());

    pe.dwSize = sizeof(pe);
    have_record = Process32First(snapshot, &pe);
    while (have_record) {
        if (mypid == (pid_t)pe.th32ProcessID) {
            /* We could cache the ulong value in a static variable. */
            result = PyLong_FromPid((pid_t)pe.th32ParentProcessID);
            break;
        }

        have_record = Process32Next(snapshot, &pe);
    }

    /* If our loop exits and our pid was not found (result will be NULL)
     * then GetLastError will return ERROR_NO_MORE_FILES. This is an
     * error anyway, so let's raise it. */
    if (!result)
        result = PyErr_SetFromWindowsErr(GetLastError());

    CloseHandle(snapshot);

    return result;
}
#endif /*MS_WINDOWS*/

PyDoc_STRVAR(posix_getppid__doc__,
"getppid() -> ppid\n\n\
Return the parent's process id.  If the parent process has already exited,\n\
Windows machines will still return its id; others systems will return the id\n\
of the 'init' process (1).");

static PyObject *
posix_getppid(PyObject *self, PyObject *noargs)
{
#ifdef MS_WINDOWS
    return win32_getppid();
#else
    return PyLong_FromPid(getppid());
#endif
}
#endif /* HAVE_GETPPID */


#ifdef HAVE_GETLOGIN
PyDoc_STRVAR(posix_getlogin__doc__,
"getlogin() -> string\n\n\
Return the actual login name.");

static PyObject *
posix_getlogin(PyObject *self, PyObject *noargs)
{
    PyObject *result = NULL;
#ifdef MS_WINDOWS
    wchar_t user_name[UNLEN + 1];
    DWORD num_chars = Py_ARRAY_LENGTH(user_name);

    if (GetUserNameW(user_name, &num_chars)) {
        /* num_chars is the number of unicode chars plus null terminator */
        result = PyUnicode_FromWideChar(user_name, num_chars - 1);
    }
    else
        result = PyErr_SetFromWindowsErr(GetLastError());
#else
    char *name;
    int old_errno = errno;

    errno = 0;
    name = getlogin();
    if (name == NULL) {
        if (errno)
            posix_error();
        else
            PyErr_SetString(PyExc_OSError, "unable to determine login name");
    }
    else
        result = PyUnicode_DecodeFSDefault(name);
    errno = old_errno;
#endif
    return result;
}
#endif /* HAVE_GETLOGIN */

#ifdef HAVE_GETUID
PyDoc_STRVAR(posix_getuid__doc__,
"getuid() -> uid\n\n\
Return the current process's user id.");

static PyObject *
posix_getuid(PyObject *self, PyObject *noargs)
{
    return PyLong_FromLong((long)getuid());
}
#endif


#ifdef HAVE_KILL
PyDoc_STRVAR(posix_kill__doc__,
"kill(pid, sig)\n\n\
Kill a process with a signal.");

static PyObject *
posix_kill(PyObject *self, PyObject *args)
{
    pid_t pid;
    int sig;
    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:kill", &pid, &sig))
        return NULL;
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
    if (sig == XCPT_SIGNAL_INTR || sig == XCPT_SIGNAL_BREAK) {
        APIRET rc;
        if ((rc = DosSendSignalException(pid, sig)) != NO_ERROR)
            return os2_error(rc);

    } else if (sig == XCPT_SIGNAL_KILLPROC) {
        APIRET rc;
        if ((rc = DosKillProcess(DKP_PROCESS, pid)) != NO_ERROR)
            return os2_error(rc);

    } else
        return NULL; /* Unrecognized Signal Requested */
#else
    if (kill(pid, sig) == -1)
        return posix_error();
#endif
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_KILLPG
PyDoc_STRVAR(posix_killpg__doc__,
"killpg(pgid, sig)\n\n\
Kill a process group with a signal.");

static PyObject *
posix_killpg(PyObject *self, PyObject *args)
{
    int sig;
    pid_t pgid;
    /* XXX some man pages make the `pgid` parameter an int, others
       a pid_t. Since getpgrp() returns a pid_t, we assume killpg should
       take the same type. Moreover, pid_t is always at least as wide as
       int (else compilation of this module fails), which is safe. */
    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:killpg", &pgid, &sig))
        return NULL;
    if (killpg(pgid, sig) == -1)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef MS_WINDOWS
PyDoc_STRVAR(win32_kill__doc__,
"kill(pid, sig)\n\n\
Kill a process with a signal.");

static PyObject *
win32_kill(PyObject *self, PyObject *args)
{
    PyObject *result;
    DWORD pid, sig, err;
    HANDLE handle;

    if (!PyArg_ParseTuple(args, "kk:kill", &pid, &sig))
        return NULL;

    /* Console processes which share a common console can be sent CTRL+C or
       CTRL+BREAK events, provided they handle said events. */
    if (sig == CTRL_C_EVENT || sig == CTRL_BREAK_EVENT) {
        if (GenerateConsoleCtrlEvent(sig, pid) == 0) {
            err = GetLastError();
            PyErr_SetFromWindowsErr(err);
        }
        else
            Py_RETURN_NONE;
    }

    /* If the signal is outside of what GenerateConsoleCtrlEvent can use,
       attempt to open and terminate the process. */
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (handle == NULL) {
        err = GetLastError();
        return PyErr_SetFromWindowsErr(err);
    }

    if (TerminateProcess(handle, sig) == 0) {
        err = GetLastError();
        result = PyErr_SetFromWindowsErr(err);
    } else {
        Py_INCREF(Py_None);
        result = Py_None;
    }

    CloseHandle(handle);
    return result;
}
#endif /* MS_WINDOWS */

#ifdef HAVE_PLOCK

#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

PyDoc_STRVAR(posix_plock__doc__,
"plock(op)\n\n\
Lock program segments into memory.");

static PyObject *
posix_plock(PyObject *self, PyObject *args)
{
    int op;
    if (!PyArg_ParseTuple(args, "i:plock", &op))
        return NULL;
    if (plock(op) == -1)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_SETUID
PyDoc_STRVAR(posix_setuid__doc__,
"setuid(uid)\n\n\
Set the current process's user id.");

static PyObject *
posix_setuid(PyObject *self, PyObject *args)
{
    long uid_arg;
    uid_t uid;
    if (!PyArg_ParseTuple(args, "l:setuid", &uid_arg))
        return NULL;
    uid = uid_arg;
    if (uid != uid_arg) {
        PyErr_SetString(PyExc_OverflowError, "user id too big");
        return NULL;
    }
    if (setuid(uid) < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETUID */


#ifdef HAVE_SETEUID
PyDoc_STRVAR(posix_seteuid__doc__,
"seteuid(uid)\n\n\
Set the current process's effective user id.");

static PyObject *
posix_seteuid (PyObject *self, PyObject *args)
{
    long euid_arg;
    uid_t euid;
    if (!PyArg_ParseTuple(args, "l", &euid_arg))
        return NULL;
    euid = euid_arg;
    if (euid != euid_arg) {
        PyErr_SetString(PyExc_OverflowError, "user id too big");
        return NULL;
    }
    if (seteuid(euid) < 0) {
        return posix_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif /* HAVE_SETEUID */

#ifdef HAVE_SETEGID
PyDoc_STRVAR(posix_setegid__doc__,
"setegid(gid)\n\n\
Set the current process's effective group id.");

static PyObject *
posix_setegid (PyObject *self, PyObject *args)
{
    long egid_arg;
    gid_t egid;
    if (!PyArg_ParseTuple(args, "l", &egid_arg))
        return NULL;
    egid = egid_arg;
    if (egid != egid_arg) {
        PyErr_SetString(PyExc_OverflowError, "group id too big");
        return NULL;
    }
    if (setegid(egid) < 0) {
        return posix_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif /* HAVE_SETEGID */

#ifdef HAVE_SETREUID
PyDoc_STRVAR(posix_setreuid__doc__,
"setreuid(ruid, euid)\n\n\
Set the current process's real and effective user ids.");

static PyObject *
posix_setreuid (PyObject *self, PyObject *args)
{
    long ruid_arg, euid_arg;
    uid_t ruid, euid;
    if (!PyArg_ParseTuple(args, "ll", &ruid_arg, &euid_arg))
        return NULL;
    if (ruid_arg == -1)
        ruid = (uid_t)-1;  /* let the compiler choose how -1 fits */
    else
        ruid = ruid_arg;  /* otherwise, assign from our long */
    if (euid_arg == -1)
        euid = (uid_t)-1;
    else
        euid = euid_arg;
    if ((euid_arg != -1 && euid != euid_arg) ||
        (ruid_arg != -1 && ruid != ruid_arg)) {
        PyErr_SetString(PyExc_OverflowError, "user id too big");
        return NULL;
    }
    if (setreuid(ruid, euid) < 0) {
        return posix_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif /* HAVE_SETREUID */

#ifdef HAVE_SETREGID
PyDoc_STRVAR(posix_setregid__doc__,
"setregid(rgid, egid)\n\n\
Set the current process's real and effective group ids.");

static PyObject *
posix_setregid (PyObject *self, PyObject *args)
{
    long rgid_arg, egid_arg;
    gid_t rgid, egid;
    if (!PyArg_ParseTuple(args, "ll", &rgid_arg, &egid_arg))
        return NULL;
    if (rgid_arg == -1)
        rgid = (gid_t)-1;  /* let the compiler choose how -1 fits */
    else
        rgid = rgid_arg;  /* otherwise, assign from our long */
    if (egid_arg == -1)
        egid = (gid_t)-1;
    else
        egid = egid_arg;
    if ((egid_arg != -1 && egid != egid_arg) ||
        (rgid_arg != -1 && rgid != rgid_arg)) {
        PyErr_SetString(PyExc_OverflowError, "group id too big");
        return NULL;
    }
    if (setregid(rgid, egid) < 0) {
        return posix_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif /* HAVE_SETREGID */

#ifdef HAVE_SETGID
PyDoc_STRVAR(posix_setgid__doc__,
"setgid(gid)\n\n\
Set the current process's group id.");

static PyObject *
posix_setgid(PyObject *self, PyObject *args)
{
    long gid_arg;
    gid_t gid;
    if (!PyArg_ParseTuple(args, "l:setgid", &gid_arg))
        return NULL;
    gid = gid_arg;
    if (gid != gid_arg) {
        PyErr_SetString(PyExc_OverflowError, "group id too big");
        return NULL;
    }
    if (setgid(gid) < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETGID */

#ifdef HAVE_SETGROUPS
PyDoc_STRVAR(posix_setgroups__doc__,
"setgroups(list)\n\n\
Set the groups of the current process to list.");

static PyObject *
posix_setgroups(PyObject *self, PyObject *groups)
{
    int i, len;
    gid_t grouplist[MAX_GROUPS];

    if (!PySequence_Check(groups)) {
        PyErr_SetString(PyExc_TypeError, "setgroups argument must be a sequence");
        return NULL;
    }
    len = PySequence_Size(groups);
    if (len > MAX_GROUPS) {
        PyErr_SetString(PyExc_ValueError, "too many groups");
        return NULL;
    }
    for(i = 0; i < len; i++) {
        PyObject *elem;
        elem = PySequence_GetItem(groups, i);
        if (!elem)
            return NULL;
        if (!PyLong_Check(elem)) {
            PyErr_SetString(PyExc_TypeError,
                            "groups must be integers");
            Py_DECREF(elem);
            return NULL;
        } else {
            unsigned long x = PyLong_AsUnsignedLong(elem);
            if (PyErr_Occurred()) {
                PyErr_SetString(PyExc_TypeError,
                                "group id too big");
                Py_DECREF(elem);
                return NULL;
            }
            grouplist[i] = x;
            /* read back the value to see if it fitted in gid_t */
            if (grouplist[i] != x) {
                PyErr_SetString(PyExc_TypeError,
                                "group id too big");
                Py_DECREF(elem);
                return NULL;
            }
        }
        Py_DECREF(elem);
    }

    if (setgroups(len, grouplist) < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETGROUPS */

#if defined(HAVE_WAIT3) || defined(HAVE_WAIT4)
static PyObject *
wait_helper(pid_t pid, int status, struct rusage *ru)
{
    PyObject *result;
    static PyObject *struct_rusage;
    _Py_IDENTIFIER(struct_rusage);

    if (pid == -1)
        return posix_error();

    if (struct_rusage == NULL) {
        PyObject *m = PyImport_ImportModuleNoBlock("resource");
        if (m == NULL)
            return NULL;
        struct_rusage = _PyObject_GetAttrId(m, &PyId_struct_rusage);
        Py_DECREF(m);
        if (struct_rusage == NULL)
            return NULL;
    }

    /* XXX(nnorwitz): Copied (w/mods) from resource.c, there should be only one. */
    result = PyStructSequence_New((PyTypeObject*) struct_rusage);
    if (!result)
        return NULL;

#ifndef doubletime
#define doubletime(TV) ((double)(TV).tv_sec + (TV).tv_usec * 0.000001)
#endif

    PyStructSequence_SET_ITEM(result, 0,
                              PyFloat_FromDouble(doubletime(ru->ru_utime)));
    PyStructSequence_SET_ITEM(result, 1,
                              PyFloat_FromDouble(doubletime(ru->ru_stime)));
#define SET_INT(result, index, value)\
        PyStructSequence_SET_ITEM(result, index, PyLong_FromLong(value))
    SET_INT(result, 2, ru->ru_maxrss);
    SET_INT(result, 3, ru->ru_ixrss);
    SET_INT(result, 4, ru->ru_idrss);
    SET_INT(result, 5, ru->ru_isrss);
    SET_INT(result, 6, ru->ru_minflt);
    SET_INT(result, 7, ru->ru_majflt);
    SET_INT(result, 8, ru->ru_nswap);
    SET_INT(result, 9, ru->ru_inblock);
    SET_INT(result, 10, ru->ru_oublock);
    SET_INT(result, 11, ru->ru_msgsnd);
    SET_INT(result, 12, ru->ru_msgrcv);
    SET_INT(result, 13, ru->ru_nsignals);
    SET_INT(result, 14, ru->ru_nvcsw);
    SET_INT(result, 15, ru->ru_nivcsw);
#undef SET_INT

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return Py_BuildValue("NiN", PyLong_FromPid(pid), status, result);
}
#endif /* HAVE_WAIT3 || HAVE_WAIT4 */

#ifdef HAVE_WAIT3
PyDoc_STRVAR(posix_wait3__doc__,
"wait3(options) -> (pid, status, rusage)\n\n\
Wait for completion of a child process.");

static PyObject *
posix_wait3(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:wait3", &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    pid = wait3(&status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT3 */

#ifdef HAVE_WAIT4
PyDoc_STRVAR(posix_wait4__doc__,
"wait4(pid, options) -> (pid, status, rusage)\n\n\
Wait for completion of a given child process.");

static PyObject *
posix_wait4(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:wait4", &pid, &options))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    pid = wait4(pid, &status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT4 */

#if defined(HAVE_WAITID) && !defined(__APPLE__)
PyDoc_STRVAR(posix_waitid__doc__,
"waitid(idtype, id, options) -> waitid_result\n\n\
Wait for the completion of one or more child processes.\n\n\
idtype can be P_PID, P_PGID or P_ALL.\n\
id specifies the pid to wait on.\n\
options is constructed from the ORing of one or more of WEXITED, WSTOPPED\n\
or WCONTINUED and additionally may be ORed with WNOHANG or WNOWAIT.\n\
Returns either waitid_result or None if WNOHANG is specified and there are\n\
no children in a waitable state.");

static PyObject *
posix_waitid(PyObject *self, PyObject *args)
{
    PyObject *result;
    idtype_t idtype;
    id_t id;
    int options, res;
    siginfo_t si;
    si.si_pid = 0;
    if (!PyArg_ParseTuple(args, "i" _Py_PARSE_PID "i:waitid", &idtype, &id, &options))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = waitid(idtype, id, &si, options);
    Py_END_ALLOW_THREADS
    if (res == -1)
        return posix_error();

    if (si.si_pid == 0)
        Py_RETURN_NONE;

    result = PyStructSequence_New(&WaitidResultType);
    if (!result)
        return NULL;

    PyStructSequence_SET_ITEM(result, 0, PyLong_FromPid(si.si_pid));
    PyStructSequence_SET_ITEM(result, 1, PyLong_FromPid(si.si_uid));
    PyStructSequence_SET_ITEM(result, 2, PyLong_FromLong((long)(si.si_signo)));
    PyStructSequence_SET_ITEM(result, 3, PyLong_FromLong((long)(si.si_status)));
    PyStructSequence_SET_ITEM(result, 4, PyLong_FromLong((long)(si.si_code)));
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}
#endif

#ifdef HAVE_WAITPID
PyDoc_STRVAR(posix_waitpid__doc__,
"waitpid(pid, options) -> (pid, status)\n\n\
Wait for completion of a given child process.");

static PyObject *
posix_waitpid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int options;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:waitpid", &pid, &options))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    pid = waitpid(pid, &status, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return posix_error();

    return Py_BuildValue("Ni", PyLong_FromPid(pid), WAIT_STATUS_INT(status));
}

#elif defined(HAVE_CWAIT)

/* MS C has a variant of waitpid() that's usable for most purposes. */
PyDoc_STRVAR(posix_waitpid__doc__,
"waitpid(pid, options) -> (pid, status << 8)\n\n"
"Wait for completion of a given process.  options is ignored on Windows.");

static PyObject *
posix_waitpid(PyObject *self, PyObject *args)
{
    Py_intptr_t pid;
    int status, options;

    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:waitpid", &pid, &options))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    pid = _cwait(&status, pid, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return posix_error();

    /* shift the status left a byte so this is more like the POSIX waitpid */
    return Py_BuildValue("Ni", PyLong_FromPid(pid), status << 8);
}
#endif /* HAVE_WAITPID || HAVE_CWAIT */

#ifdef HAVE_WAIT
PyDoc_STRVAR(posix_wait__doc__,
"wait() -> (pid, status)\n\n\
Wait for completion of a child process.");

static PyObject *
posix_wait(PyObject *self, PyObject *noargs)
{
    pid_t pid;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    Py_BEGIN_ALLOW_THREADS
    pid = wait(&status);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return posix_error();

    return Py_BuildValue("Ni", PyLong_FromPid(pid), WAIT_STATUS_INT(status));
}
#endif


#if defined(HAVE_READLINK) || defined(MS_WINDOWS)
PyDoc_STRVAR(readlink__doc__,
"readlink(path, *, dir_fd=None) -> path\n\n\
Return a string representing the path to which the symbolic link points.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");
#endif

#ifdef HAVE_READLINK

static PyObject *
posix_readlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int dir_fd = DEFAULT_DIR_FD;
    char buffer[MAXPATHLEN];
    ssize_t length;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "dir_fd", NULL};

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&:readlink", keywords,
                          path_converter, &path,
#ifdef HAVE_READLINKAT
                          dir_fd_converter, &dir_fd
#else
                          dir_fd_unavailable, &dir_fd
#endif
                          ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_READLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        length = readlinkat(dir_fd, path.narrow, buffer, sizeof(buffer));
    else
#endif
        length = readlink(path.narrow, buffer, sizeof(buffer));
    Py_END_ALLOW_THREADS

    if (length < 0) {
        return_value = path_posix_error("readlink", &path);
        goto exit;
    }

    if (PyUnicode_Check(path.object))
        return_value = PyUnicode_DecodeFSDefaultAndSize(buffer, length);
    else
        return_value = PyBytes_FromStringAndSize(buffer, length);
exit:
    path_cleanup(&path);
    return return_value;
}


#endif /* HAVE_READLINK */


#ifdef HAVE_SYMLINK
PyDoc_STRVAR(posix_symlink__doc__,
"symlink(src, dst, target_is_directory=False, *, dir_fd=None)\n\n\
Create a symbolic link pointing to src named dst.\n\n\
target_is_directory is required on Windows if the target is to be\n\
  interpreted as a directory.  (On Windows, symlink requires\n\
  Windows 6.0 or greater, and raises a NotImplementedError otherwise.)\n\
  target_is_directory is ignored on non-Windows platforms.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

#if defined(MS_WINDOWS)

/* Grab CreateSymbolicLinkW dynamically from kernel32 */
static DWORD (CALLBACK *Py_CreateSymbolicLinkW)(LPWSTR, LPWSTR, DWORD) = NULL;
static DWORD (CALLBACK *Py_CreateSymbolicLinkA)(LPSTR, LPSTR, DWORD) = NULL;
static int
check_CreateSymbolicLink()
{
    HINSTANCE hKernel32;
    /* only recheck */
    if (Py_CreateSymbolicLinkW && Py_CreateSymbolicLinkA)
        return 1;
    hKernel32 = GetModuleHandleW(L"KERNEL32");
    *(FARPROC*)&Py_CreateSymbolicLinkW = GetProcAddress(hKernel32,
                                                        "CreateSymbolicLinkW");
    *(FARPROC*)&Py_CreateSymbolicLinkA = GetProcAddress(hKernel32,
                                                        "CreateSymbolicLinkA");
    return (Py_CreateSymbolicLinkW && Py_CreateSymbolicLinkA);
}

#endif

static PyObject *
posix_symlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t src;
    path_t dst;
    int dir_fd = DEFAULT_DIR_FD;
    int target_is_directory = 0;
    static char *keywords[] = {"src", "dst", "target_is_directory",
                               "dir_fd", NULL};
    PyObject *return_value;
#ifdef MS_WINDOWS
    DWORD result;
#else
    int result;
#endif

    memset(&src, 0, sizeof(src));
    src.argument_name = "src";
    memset(&dst, 0, sizeof(dst));
    dst.argument_name = "dst";

#ifdef MS_WINDOWS
    if (!check_CreateSymbolicLink()) {
        PyErr_SetString(PyExc_NotImplementedError,
            "CreateSymbolicLink functions not found");
		return NULL;
	}
    if (!win32_can_symlink) {
        PyErr_SetString(PyExc_OSError, "symbolic link privilege not held");
		return NULL;
	}
#endif

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&|i$O&:symlink",
            keywords,
            path_converter, &src,
            path_converter, &dst,
            &target_is_directory,
#ifdef HAVE_SYMLINKAT
            dir_fd_converter, &dir_fd
#else
            dir_fd_unavailable, &dir_fd
#endif
            ))
        return NULL;

    if ((src.narrow && dst.wide) || (src.wide && dst.narrow)) {
        PyErr_SetString(PyExc_ValueError,
            "symlink: src and dst must be the same type");
        return_value = NULL;
        goto exit;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (dst.wide)
        result = Py_CreateSymbolicLinkW(dst.wide, src.wide,
                                        target_is_directory);
    else
        result = Py_CreateSymbolicLinkA(dst.narrow, src.narrow,
                                        target_is_directory);
    Py_END_ALLOW_THREADS

    if (!result) {
        return_value = win32_error_object("symlink", src.object);
        goto exit;
    }

#else

    Py_BEGIN_ALLOW_THREADS
#if HAVE_SYMLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = symlinkat(src.narrow, dir_fd, dst.narrow);
    else
#endif
        result = symlink(src.narrow, dst.narrow);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_error("symlink", &dst);
        goto exit;
    }
#endif

    return_value = Py_None;
    Py_INCREF(Py_None);
    goto exit; /* silence "unused label" warning */
exit:
    path_cleanup(&src);
    path_cleanup(&dst);
    return return_value;
}

#endif /* HAVE_SYMLINK */


#if !defined(HAVE_READLINK) && defined(MS_WINDOWS)

static PyObject *
win_readlink(PyObject *self, PyObject *args, PyObject *kwargs)
{
    wchar_t *path;
    DWORD n_bytes_returned;
    DWORD io_result;
    PyObject *po, *result;
	int dir_fd;
    HANDLE reparse_point_handle;

    char target_buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER *)target_buffer;
    wchar_t *print_name;

    static char *keywords[] = {"path", "dir_fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "U|$O&:readlink", keywords,
                          &po,
                          dir_fd_unavailable, &dir_fd
                          ))
        return NULL;

    path = PyUnicode_AsUnicode(po);
    if (path == NULL)
        return NULL;

    /* First get a handle to the reparse point */
    Py_BEGIN_ALLOW_THREADS
    reparse_point_handle = CreateFileW(
        path,
        0,
        0,
        0,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,
        0);
    Py_END_ALLOW_THREADS

    if (reparse_point_handle==INVALID_HANDLE_VALUE)
        return win32_error_object("readlink", po);

    Py_BEGIN_ALLOW_THREADS
    /* New call DeviceIoControl to read the reparse point */
    io_result = DeviceIoControl(
        reparse_point_handle,
        FSCTL_GET_REPARSE_POINT,
        0, 0, /* in buffer */
        target_buffer, sizeof(target_buffer),
        &n_bytes_returned,
        0 /* we're not using OVERLAPPED_IO */
        );
    CloseHandle(reparse_point_handle);
    Py_END_ALLOW_THREADS

    if (io_result==0)
        return win32_error_object("readlink", po);

    if (rdb->ReparseTag != IO_REPARSE_TAG_SYMLINK)
    {
        PyErr_SetString(PyExc_ValueError,
                "not a symbolic link");
        return NULL;
    }
    print_name = rdb->SymbolicLinkReparseBuffer.PathBuffer +
                 rdb->SymbolicLinkReparseBuffer.PrintNameOffset;

    result = PyUnicode_FromWideChar(print_name,
                    rdb->SymbolicLinkReparseBuffer.PrintNameLength/2);
    return result;
}

#endif /* !defined(HAVE_READLINK) && defined(MS_WINDOWS) */


#ifdef HAVE_TIMES
#if defined(PYCC_VACPP) && defined(PYOS_OS2)
static long
system_uptime(void)
{
    ULONG     value = 0;

    Py_BEGIN_ALLOW_THREADS
    DosQuerySysInfo(QSV_MS_COUNT, QSV_MS_COUNT, &value, sizeof(value));
    Py_END_ALLOW_THREADS

    return value;
}

static PyObject *
posix_times(PyObject *self, PyObject *noargs)
{
    /* Currently Only Uptime is Provided -- Others Later */
    return Py_BuildValue("ddddd",
                         (double)0 /* t.tms_utime / HZ */,
                         (double)0 /* t.tms_stime / HZ */,
                         (double)0 /* t.tms_cutime / HZ */,
                         (double)0 /* t.tms_cstime / HZ */,
                         (double)system_uptime() / 1000);
}
#else /* not OS2 */
#define NEED_TICKS_PER_SECOND
static long ticks_per_second = -1;
static PyObject *
posix_times(PyObject *self, PyObject *noargs)
{
    struct tms t;
    clock_t c;
    errno = 0;
    c = times(&t);
    if (c == (clock_t) -1)
        return posix_error();
    return Py_BuildValue("ddddd",
                         (double)t.tms_utime / ticks_per_second,
                         (double)t.tms_stime / ticks_per_second,
                         (double)t.tms_cutime / ticks_per_second,
                         (double)t.tms_cstime / ticks_per_second,
                         (double)c / ticks_per_second);
}
#endif /* not OS2 */
#endif /* HAVE_TIMES */


#ifdef MS_WINDOWS
#define HAVE_TIMES      /* so the method table will pick it up */
static PyObject *
posix_times(PyObject *self, PyObject *noargs)
{
    FILETIME create, exit, kernel, user;
    HANDLE hProc;
    hProc = GetCurrentProcess();
    GetProcessTimes(hProc, &create, &exit, &kernel, &user);
    /* The fields of a FILETIME structure are the hi and lo part
       of a 64-bit value expressed in 100 nanosecond units.
       1e7 is one second in such units; 1e-7 the inverse.
       429.4967296 is 2**32 / 1e7 or 2**32 * 1e-7.
    */
    return Py_BuildValue(
        "ddddd",
        (double)(user.dwHighDateTime*429.4967296 +
                 user.dwLowDateTime*1e-7),
        (double)(kernel.dwHighDateTime*429.4967296 +
                 kernel.dwLowDateTime*1e-7),
        (double)0,
        (double)0,
        (double)0);
}
#endif /* MS_WINDOWS */

#ifdef HAVE_TIMES
PyDoc_STRVAR(posix_times__doc__,
"times() -> (utime, stime, cutime, cstime, elapsed_time)\n\n\
Return a tuple of floating point numbers indicating process times.");
#endif


#ifdef HAVE_GETSID
PyDoc_STRVAR(posix_getsid__doc__,
"getsid(pid) -> sid\n\n\
Call the system call getsid().");

static PyObject *
posix_getsid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int sid;
    if (!PyArg_ParseTuple(args, _Py_PARSE_PID ":getsid", &pid))
        return NULL;
    sid = getsid(pid);
    if (sid < 0)
        return posix_error();
    return PyLong_FromLong((long)sid);
}
#endif /* HAVE_GETSID */


#ifdef HAVE_SETSID
PyDoc_STRVAR(posix_setsid__doc__,
"setsid()\n\n\
Call the system call setsid().");

static PyObject *
posix_setsid(PyObject *self, PyObject *noargs)
{
    if (setsid() < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETSID */

#ifdef HAVE_SETPGID
PyDoc_STRVAR(posix_setpgid__doc__,
"setpgid(pid, pgrp)\n\n\
Call the system call setpgid().");

static PyObject *
posix_setpgid(PyObject *self, PyObject *args)
{
    pid_t pid;
    int pgrp;
    if (!PyArg_ParseTuple(args, _Py_PARSE_PID "i:setpgid", &pid, &pgrp))
        return NULL;
    if (setpgid(pid, pgrp) < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_SETPGID */


#ifdef HAVE_TCGETPGRP
PyDoc_STRVAR(posix_tcgetpgrp__doc__,
"tcgetpgrp(fd) -> pgid\n\n\
Return the process group associated with the terminal given by a fd.");

static PyObject *
posix_tcgetpgrp(PyObject *self, PyObject *args)
{
    int fd;
    pid_t pgid;
    if (!PyArg_ParseTuple(args, "i:tcgetpgrp", &fd))
        return NULL;
    pgid = tcgetpgrp(fd);
    if (pgid < 0)
        return posix_error();
    return PyLong_FromPid(pgid);
}
#endif /* HAVE_TCGETPGRP */


#ifdef HAVE_TCSETPGRP
PyDoc_STRVAR(posix_tcsetpgrp__doc__,
"tcsetpgrp(fd, pgid)\n\n\
Set the process group associated with the terminal given by a fd.");

static PyObject *
posix_tcsetpgrp(PyObject *self, PyObject *args)
{
    int fd;
    pid_t pgid;
    if (!PyArg_ParseTuple(args, "i" _Py_PARSE_PID ":tcsetpgrp", &fd, &pgid))
        return NULL;
    if (tcsetpgrp(fd, pgid) < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

PyDoc_STRVAR(posix_open__doc__,
"open(path, flags, mode=0o777, *, dir_fd=None)\n\n\
Open a file for low level IO.  Returns a file handle (integer).\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

static PyObject *
posix_open(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int flags;
    int mode = 0777;
    int dir_fd = DEFAULT_DIR_FD;
    int fd;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "flags", "mode", "dir_fd", NULL};

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&i|i$O&:open", keywords,
        path_converter, &path,
        &flags, &mode,
#ifdef HAVE_OPENAT
        dir_fd_converter, &dir_fd
#else
        dir_fd_unavailable, &dir_fd
#endif
        ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path.wide)
        fd = _wopen(path.wide, flags, mode);
    else
#endif
#ifdef HAVE_OPENAT
    if (dir_fd != DEFAULT_DIR_FD)
        fd = openat(dir_fd, path.narrow, flags, mode);
    else
#endif
        fd = open(path.narrow, flags, mode);
    Py_END_ALLOW_THREADS

    if (fd == -1) {
#ifdef MS_WINDOWS
        /* force use of posix_error here for exact backwards compatibility */
        if (path.wide)
            return_value = posix_error();
        else
#endif
        return_value = path_error("open", &path);
        goto exit;
    }

    return_value = PyLong_FromLong((long)fd);

exit:
    path_cleanup(&path);
    return return_value;
}

PyDoc_STRVAR(posix_close__doc__,
"close(fd)\n\n\
Close a file descriptor (for low level IO).");

static PyObject *
posix_close(PyObject *self, PyObject *args)
{
    int fd, res;
    if (!PyArg_ParseTuple(args, "i:close", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return posix_error();
    Py_BEGIN_ALLOW_THREADS
    res = close(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(posix_closerange__doc__,
"closerange(fd_low, fd_high)\n\n\
Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

static PyObject *
posix_closerange(PyObject *self, PyObject *args)
{
    int fd_from, fd_to, i;
    if (!PyArg_ParseTuple(args, "ii:closerange", &fd_from, &fd_to))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    for (i = fd_from; i < fd_to; i++)
        if (_PyVerify_fd(i))
            close(i);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


PyDoc_STRVAR(posix_dup__doc__,
"dup(fd) -> fd2\n\n\
Return a duplicate of a file descriptor.");

static PyObject *
posix_dup(PyObject *self, PyObject *args)
{
    int fd;
    if (!PyArg_ParseTuple(args, "i:dup", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return posix_error();
    fd = dup(fd);
    if (fd < 0)
        return posix_error();
    return PyLong_FromLong((long)fd);
}


PyDoc_STRVAR(posix_dup2__doc__,
"dup2(old_fd, new_fd)\n\n\
Duplicate file descriptor.");

static PyObject *
posix_dup2(PyObject *self, PyObject *args)
{
    int fd, fd2, res;
    if (!PyArg_ParseTuple(args, "ii:dup2", &fd, &fd2))
        return NULL;
    if (!_PyVerify_fd_dup2(fd, fd2))
        return posix_error();
    res = dup2(fd, fd2);
    if (res < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef HAVE_LOCKF
PyDoc_STRVAR(posix_lockf__doc__,
"lockf(fd, cmd, len)\n\n\
Apply, test or remove a POSIX lock on an open file descriptor.\n\n\
fd is an open file descriptor.\n\
cmd specifies the command to use - one of F_LOCK, F_TLOCK, F_ULOCK or\n\
F_TEST.\n\
len specifies the section of the file to lock.");

static PyObject *
posix_lockf(PyObject *self, PyObject *args)
{
    int fd, cmd, res;
    off_t len;
    if (!PyArg_ParseTuple(args, "iiO&:lockf",
            &fd, &cmd, _parse_off_t, &len))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = lockf(fd, cmd, len);
    Py_END_ALLOW_THREADS

    if (res < 0)
        return posix_error();

    Py_RETURN_NONE;
}
#endif


PyDoc_STRVAR(posix_lseek__doc__,
"lseek(fd, pos, how) -> newpos\n\n\
Set the current position of a file descriptor.\n\
Return the new cursor position in bytes, starting from the beginning.");

static PyObject *
posix_lseek(PyObject *self, PyObject *args)
{
    int fd, how;
#if defined(MS_WIN64) || defined(MS_WINDOWS)
    PY_LONG_LONG pos, res;
#else
    off_t pos, res;
#endif
    PyObject *posobj;
    if (!PyArg_ParseTuple(args, "iOi:lseek", &fd, &posobj, &how))
        return NULL;
#ifdef SEEK_SET
    /* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
    switch (how) {
    case 0: how = SEEK_SET; break;
    case 1: how = SEEK_CUR; break;
    case 2: how = SEEK_END; break;
    }
#endif /* SEEK_END */

#if !defined(HAVE_LARGEFILE_SUPPORT)
    pos = PyLong_AsLong(posobj);
#else
    pos = PyLong_AsLongLong(posobj);
#endif
    if (PyErr_Occurred())
        return NULL;

    if (!_PyVerify_fd(fd))
        return posix_error();
    Py_BEGIN_ALLOW_THREADS
#if defined(MS_WIN64) || defined(MS_WINDOWS)
    res = _lseeki64(fd, pos, how);
#else
    res = lseek(fd, pos, how);
#endif
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();

#if !defined(HAVE_LARGEFILE_SUPPORT)
    return PyLong_FromLong(res);
#else
    return PyLong_FromLongLong(res);
#endif
}


PyDoc_STRVAR(posix_read__doc__,
"read(fd, buffersize) -> string\n\n\
Read a file descriptor.");

static PyObject *
posix_read(PyObject *self, PyObject *args)
{
    int fd, size;
    Py_ssize_t n;
    PyObject *buffer;
    if (!PyArg_ParseTuple(args, "ii:read", &fd, &size))
        return NULL;
    if (size < 0) {
        errno = EINVAL;
        return posix_error();
    }
    buffer = PyBytes_FromStringAndSize((char *)NULL, size);
    if (buffer == NULL)
        return NULL;
    if (!_PyVerify_fd(fd)) {
        Py_DECREF(buffer);
        return posix_error();
    }
    Py_BEGIN_ALLOW_THREADS
    n = read(fd, PyBytes_AS_STRING(buffer), size);
    Py_END_ALLOW_THREADS
    if (n < 0) {
        Py_DECREF(buffer);
        return posix_error();
    }
    if (n != size)
        _PyBytes_Resize(&buffer, n);
    return buffer;
}

#if (defined(HAVE_SENDFILE) && (defined(__FreeBSD__) || defined(__DragonFly__) \
    || defined(__APPLE__))) || defined(HAVE_READV) || defined(HAVE_WRITEV)
static Py_ssize_t
iov_setup(struct iovec **iov, Py_buffer **buf, PyObject *seq, int cnt, int type)
{
    int i, j;
    Py_ssize_t blen, total = 0;

    *iov = PyMem_New(struct iovec, cnt);
    if (*iov == NULL) {
        PyErr_NoMemory();
        return total;
    }

    *buf = PyMem_New(Py_buffer, cnt);
    if (*buf == NULL) {
        PyMem_Del(*iov);
        PyErr_NoMemory();
        return total;
    }

    for (i = 0; i < cnt; i++) {
        PyObject *item = PySequence_GetItem(seq, i);
        if (item == NULL)
            goto fail;
        if (PyObject_GetBuffer(item, &(*buf)[i], type) == -1) {
            Py_DECREF(item);
            goto fail;
        }
        Py_DECREF(item);
        (*iov)[i].iov_base = (*buf)[i].buf;
        blen = (*buf)[i].len;
        (*iov)[i].iov_len = blen;
        total += blen;
    }
    return total;

fail:
    PyMem_Del(*iov);
    for (j = 0; j < i; j++) {
        PyBuffer_Release(&(*buf)[j]);
    }
    PyMem_Del(*buf);
    return 0;
}

static void
iov_cleanup(struct iovec *iov, Py_buffer *buf, int cnt)
{
    int i;
    PyMem_Del(iov);
    for (i = 0; i < cnt; i++) {
        PyBuffer_Release(&buf[i]);
    }
    PyMem_Del(buf);
}
#endif

#ifdef HAVE_READV
PyDoc_STRVAR(posix_readv__doc__,
"readv(fd, buffers) -> bytesread\n\n\
Read from a file descriptor into a number of writable buffers. buffers\n\
is an arbitrary sequence of writable buffers.\n\
Returns the total number of bytes read.");

static PyObject *
posix_readv(PyObject *self, PyObject *args)
{
    int fd, cnt;
    Py_ssize_t n;
    PyObject *seq;
    struct iovec *iov;
    Py_buffer *buf;

    if (!PyArg_ParseTuple(args, "iO:readv", &fd, &seq))
        return NULL;
    if (!PySequence_Check(seq)) {
        PyErr_SetString(PyExc_TypeError,
            "readv() arg 2 must be a sequence");
        return NULL;
    }
    cnt = PySequence_Size(seq);

    if (!iov_setup(&iov, &buf, seq, cnt, PyBUF_WRITABLE))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    n = readv(fd, iov, cnt);
    Py_END_ALLOW_THREADS

    iov_cleanup(iov, buf, cnt);
    return PyLong_FromSsize_t(n);
}
#endif

#ifdef HAVE_PREAD
PyDoc_STRVAR(posix_pread__doc__,
"pread(fd, buffersize, offset) -> string\n\n\
Read from a file descriptor, fd, at a position of offset. It will read up\n\
to buffersize number of bytes. The file offset remains unchanged.");

static PyObject *
posix_pread(PyObject *self, PyObject *args)
{
    int fd, size;
    off_t offset;
    Py_ssize_t n;
    PyObject *buffer;
    if (!PyArg_ParseTuple(args, "iiO&:pread", &fd, &size, _parse_off_t, &offset))
        return NULL;

    if (size < 0) {
        errno = EINVAL;
        return posix_error();
    }
    buffer = PyBytes_FromStringAndSize((char *)NULL, size);
    if (buffer == NULL)
        return NULL;
    if (!_PyVerify_fd(fd)) {
        Py_DECREF(buffer);
        return posix_error();
    }
    Py_BEGIN_ALLOW_THREADS
    n = pread(fd, PyBytes_AS_STRING(buffer), size, offset);
    Py_END_ALLOW_THREADS
    if (n < 0) {
        Py_DECREF(buffer);
        return posix_error();
    }
    if (n != size)
        _PyBytes_Resize(&buffer, n);
    return buffer;
}
#endif

PyDoc_STRVAR(posix_write__doc__,
"write(fd, string) -> byteswritten\n\n\
Write a string to a file descriptor.");

static PyObject *
posix_write(PyObject *self, PyObject *args)
{
    Py_buffer pbuf;
    int fd;
    Py_ssize_t size, len;

    if (!PyArg_ParseTuple(args, "iy*:write", &fd, &pbuf))
        return NULL;
    if (!_PyVerify_fd(fd)) {
        PyBuffer_Release(&pbuf);
        return posix_error();
    }
    len = pbuf.len;
    Py_BEGIN_ALLOW_THREADS
#if defined(MS_WIN64) || defined(MS_WINDOWS)
    if (len > INT_MAX)
        len = INT_MAX;
    size = write(fd, pbuf.buf, (int)len);
#else
    size = write(fd, pbuf.buf, len);
#endif
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&pbuf);
    if (size < 0)
        return posix_error();
    return PyLong_FromSsize_t(size);
}

#ifdef HAVE_SENDFILE
PyDoc_STRVAR(posix_sendfile__doc__,
"sendfile(out, in, offset, nbytes) -> byteswritten\n\
sendfile(out, in, offset, nbytes, headers=None, trailers=None, flags=0)\n\
            -> byteswritten\n\
Copy nbytes bytes from file descriptor in to file descriptor out.");

static PyObject *
posix_sendfile(PyObject *self, PyObject *args, PyObject *kwdict)
{
    int in, out;
    Py_ssize_t ret;
    off_t offset;

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__APPLE__)
#ifndef __APPLE__
    Py_ssize_t len;
#endif
    PyObject *headers = NULL, *trailers = NULL;
    Py_buffer *hbuf, *tbuf;
    off_t sbytes;
    struct sf_hdtr sf;
    int flags = 0;
    sf.headers = NULL;
    sf.trailers = NULL;
    static char *keywords[] = {"out", "in",
                                "offset", "count",
                                "headers", "trailers", "flags", NULL};

#ifdef __APPLE__
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "iiO&O&|OOi:sendfile",
        keywords, &out, &in, _parse_off_t, &offset, _parse_off_t, &sbytes,
#else
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "iiO&n|OOi:sendfile",
        keywords, &out, &in, _parse_off_t, &offset, &len,
#endif
                &headers, &trailers, &flags))
            return NULL;
    if (headers != NULL) {
        if (!PySequence_Check(headers)) {
            PyErr_SetString(PyExc_TypeError,
                "sendfile() headers must be a sequence or None");
            return NULL;
        } else {
            Py_ssize_t i = 0; /* Avoid uninitialized warning */
            sf.hdr_cnt = PySequence_Size(headers);
            if (sf.hdr_cnt > 0 &&
                !(i = iov_setup(&(sf.headers), &hbuf,
                                headers, sf.hdr_cnt, PyBUF_SIMPLE)))
                return NULL;
#ifdef __APPLE__
            sbytes += i;
#endif
        }
    }
    if (trailers != NULL) {
        if (!PySequence_Check(trailers)) {
            PyErr_SetString(PyExc_TypeError,
                "sendfile() trailers must be a sequence or None");
            return NULL;
        } else {
            Py_ssize_t i = 0; /* Avoid uninitialized warning */
            sf.trl_cnt = PySequence_Size(trailers);
            if (sf.trl_cnt > 0 &&
                !(i = iov_setup(&(sf.trailers), &tbuf,
                                trailers, sf.trl_cnt, PyBUF_SIMPLE)))
                return NULL;
#ifdef __APPLE__
            sbytes += i;
#endif
        }
    }

    Py_BEGIN_ALLOW_THREADS
#ifdef __APPLE__
    ret = sendfile(in, out, offset, &sbytes, &sf, flags);
#else
    ret = sendfile(in, out, offset, len, &sf, &sbytes, flags);
#endif
    Py_END_ALLOW_THREADS

    if (sf.headers != NULL)
        iov_cleanup(sf.headers, hbuf, sf.hdr_cnt);
    if (sf.trailers != NULL)
        iov_cleanup(sf.trailers, tbuf, sf.trl_cnt);

    if (ret < 0) {
        if ((errno == EAGAIN) || (errno == EBUSY)) {
            if (sbytes != 0) {
                // some data has been sent
                goto done;
            }
            else {
                // no data has been sent; upper application is supposed
                // to retry on EAGAIN or EBUSY
                return posix_error();
            }
        }
        return posix_error();
    }
    goto done;

done:
    #if !defined(HAVE_LARGEFILE_SUPPORT)
        return Py_BuildValue("l", sbytes);
    #else
        return Py_BuildValue("L", sbytes);
    #endif

#else
    Py_ssize_t count;
    PyObject *offobj;
    static char *keywords[] = {"out", "in",
                                "offset", "count", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "iiOn:sendfile",
            keywords, &out, &in, &offobj, &count))
        return NULL;
#ifdef linux
    if (offobj == Py_None) {
        Py_BEGIN_ALLOW_THREADS
        ret = sendfile(out, in, NULL, count);
        Py_END_ALLOW_THREADS
        if (ret < 0)
            return posix_error();
        return Py_BuildValue("n", ret);
    }
#endif
    if (!_parse_off_t(offobj, &offset))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    ret = sendfile(out, in, &offset, count);
    Py_END_ALLOW_THREADS
    if (ret < 0)
        return posix_error();
    return Py_BuildValue("n", ret);
#endif
}
#endif

PyDoc_STRVAR(posix_fstat__doc__,
"fstat(fd) -> stat result\n\n\
Like stat(), but for an open file descriptor.\n\
Equivalent to stat(fd=fd).");

static PyObject *
posix_fstat(PyObject *self, PyObject *args)
{
    int fd;
    STRUCT_STAT st;
    int res;
    if (!PyArg_ParseTuple(args, "i:fstat", &fd))
        return NULL;
#ifdef __VMS
    /* on OpenVMS we must ensure that all bytes are written to the file */
    fsync(fd);
#endif
    if (!_PyVerify_fd(fd))
        return posix_error();
    Py_BEGIN_ALLOW_THREADS
    res = FSTAT(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0) {
#ifdef MS_WINDOWS
        return win32_error("fstat", NULL);
#else
        return posix_error();
#endif
    }

    return _pystat_fromstructstat(&st);
}

PyDoc_STRVAR(posix_isatty__doc__,
"isatty(fd) -> bool\n\n\
Return True if the file descriptor 'fd' is an open file descriptor\n\
connected to the slave end of a terminal.");

static PyObject *
posix_isatty(PyObject *self, PyObject *args)
{
    int fd;
    if (!PyArg_ParseTuple(args, "i:isatty", &fd))
        return NULL;
    if (!_PyVerify_fd(fd))
        return PyBool_FromLong(0);
    return PyBool_FromLong(isatty(fd));
}

#ifdef HAVE_PIPE
PyDoc_STRVAR(posix_pipe__doc__,
"pipe() -> (read_end, write_end)\n\n\
Create a pipe.");

static PyObject *
posix_pipe(PyObject *self, PyObject *noargs)
{
#if defined(PYOS_OS2)
    HFILE read, write;
    APIRET rc;

    rc = DosCreatePipe( &read, &write, 4096);
    if (rc != NO_ERROR)
        return os2_error(rc);

    return Py_BuildValue("(ii)", read, write);
#else
#if !defined(MS_WINDOWS)
    int fds[2];
    int res;
    res = pipe(fds);
    if (res != 0)
        return posix_error();
    return Py_BuildValue("(ii)", fds[0], fds[1]);
#else /* MS_WINDOWS */
    HANDLE read, write;
    int read_fd, write_fd;
    BOOL ok;
    ok = CreatePipe(&read, &write, NULL, 0);
    if (!ok)
        return win32_error("CreatePipe", NULL);
    read_fd = _open_osfhandle((Py_intptr_t)read, 0);
    write_fd = _open_osfhandle((Py_intptr_t)write, 1);
    return Py_BuildValue("(ii)", read_fd, write_fd);
#endif /* MS_WINDOWS */
#endif
}
#endif  /* HAVE_PIPE */

#ifdef HAVE_PIPE2
PyDoc_STRVAR(posix_pipe2__doc__,
"pipe2(flags) -> (read_end, write_end)\n\n\
Create a pipe with flags set atomically.\n\
flags can be constructed by ORing together one or more of these values:\n\
O_NONBLOCK, O_CLOEXEC.\n\
");

static PyObject *
posix_pipe2(PyObject *self, PyObject *arg)
{
    int flags;
    int fds[2];
    int res;

    flags = PyLong_AsLong(arg);
    if (flags == -1 && PyErr_Occurred())
        return NULL;

    res = pipe2(fds, flags);
    if (res != 0)
        return posix_error();
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}
#endif /* HAVE_PIPE2 */

#ifdef HAVE_WRITEV
PyDoc_STRVAR(posix_writev__doc__,
"writev(fd, buffers) -> byteswritten\n\n\
Write the contents of buffers to a file descriptor, where buffers is an\n\
arbitrary sequence of buffers.\n\
Returns the total bytes written.");

static PyObject *
posix_writev(PyObject *self, PyObject *args)
{
    int fd, cnt;
    Py_ssize_t res;
    PyObject *seq;
    struct iovec *iov;
    Py_buffer *buf;
    if (!PyArg_ParseTuple(args, "iO:writev", &fd, &seq))
        return NULL;
    if (!PySequence_Check(seq)) {
        PyErr_SetString(PyExc_TypeError,
            "writev() arg 2 must be a sequence");
        return NULL;
    }
    cnt = PySequence_Size(seq);

    if (!iov_setup(&iov, &buf, seq, cnt, PyBUF_SIMPLE)) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    res = writev(fd, iov, cnt);
    Py_END_ALLOW_THREADS

    iov_cleanup(iov, buf, cnt);
    return PyLong_FromSsize_t(res);
}
#endif

#ifdef HAVE_PWRITE
PyDoc_STRVAR(posix_pwrite__doc__,
"pwrite(fd, string, offset) -> byteswritten\n\n\
Write string to a file descriptor, fd, from offset, leaving the file\n\
offset unchanged.");

static PyObject *
posix_pwrite(PyObject *self, PyObject *args)
{
    Py_buffer pbuf;
    int fd;
    off_t offset;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "iy*O&:pwrite", &fd, &pbuf, _parse_off_t, &offset))
        return NULL;

    if (!_PyVerify_fd(fd)) {
        PyBuffer_Release(&pbuf);
        return posix_error();
    }
    Py_BEGIN_ALLOW_THREADS
    size = pwrite(fd, pbuf.buf, (size_t)pbuf.len, offset);
    Py_END_ALLOW_THREADS
    PyBuffer_Release(&pbuf);
    if (size < 0)
        return posix_error();
    return PyLong_FromSsize_t(size);
}
#endif

#ifdef HAVE_MKFIFO
PyDoc_STRVAR(posix_mkfifo__doc__,
"mkfifo(path, mode=0o666, *, dir_fd=None)\n\n\
Create a FIFO (a POSIX named pipe).\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");

static PyObject *
posix_mkfifo(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int mode = 0666;
    int dir_fd = DEFAULT_DIR_FD;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "mode", "dir_fd", NULL};

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|i$O&:mkfifo", keywords,
        path_converter, &path,
        &mode,
#ifdef HAVE_MKFIFOAT
        dir_fd_converter, &dir_fd
#else
        dir_fd_unavailable, &dir_fd
#endif
        ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_MKFIFOAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mkfifoat(dir_fd, path.narrow, mode);
    else
#endif
        result = mkfifo(path.narrow, mode);
    Py_END_ALLOW_THREADS

    if (result < 0) {
        return_value = posix_error();
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);

exit:
    path_cleanup(&path);
    return return_value;
}
#endif

#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
PyDoc_STRVAR(posix_mknod__doc__,
"mknod(filename, mode=0o600, device=0, *, dir_fd=None)\n\n\
Create a filesystem node (file, device special file or named pipe)\n\
named filename. mode specifies both the permissions to use and the\n\
type of node to be created, being combined (bitwise OR) with one of\n\
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO. For S_IFCHR and S_IFBLK,\n\
device defines the newly created device special file (probably using\n\
os.makedev()), otherwise it is ignored.\n\
\n\
If dir_fd is not None, it should be a file descriptor open to a directory,\n\
  and path should be relative; path will then be relative to that directory.\n\
dir_fd may not be implemented on your platform.\n\
  If it is unavailable, using it will raise a NotImplementedError.");


static PyObject *
posix_mknod(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int mode = 0666;
    int device = 0;
    int dir_fd = DEFAULT_DIR_FD;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "mode", "device", "dir_fd", NULL};

    memset(&path, 0, sizeof(path));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|ii$O&:mknod", keywords,
        path_converter, &path,
        &mode, &device,
#ifdef HAVE_MKNODAT
        dir_fd_converter, &dir_fd
#else
        dir_fd_unavailable, &dir_fd
#endif
        ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_MKNODAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mknodat(dir_fd, path.narrow, mode, device);
    else
#endif
        result = mknod(path.narrow, mode, device);
    Py_END_ALLOW_THREADS

    if (result < 0) {
        return_value = posix_error();
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(Py_None);
    
exit:
    path_cleanup(&path);
    return return_value;
}
#endif

#ifdef HAVE_DEVICE_MACROS
PyDoc_STRVAR(posix_major__doc__,
"major(device) -> major number\n\
Extracts a device major number from a raw device number.");

static PyObject *
posix_major(PyObject *self, PyObject *args)
{
    int device;
    if (!PyArg_ParseTuple(args, "i:major", &device))
        return NULL;
    return PyLong_FromLong((long)major(device));
}

PyDoc_STRVAR(posix_minor__doc__,
"minor(device) -> minor number\n\
Extracts a device minor number from a raw device number.");

static PyObject *
posix_minor(PyObject *self, PyObject *args)
{
    int device;
    if (!PyArg_ParseTuple(args, "i:minor", &device))
        return NULL;
    return PyLong_FromLong((long)minor(device));
}

PyDoc_STRVAR(posix_makedev__doc__,
"makedev(major, minor) -> device number\n\
Composes a raw device number from the major and minor device numbers.");

static PyObject *
posix_makedev(PyObject *self, PyObject *args)
{
    int major, minor;
    if (!PyArg_ParseTuple(args, "ii:makedev", &major, &minor))
        return NULL;
    return PyLong_FromLong((long)makedev(major, minor));
}
#endif /* device macros */


#ifdef HAVE_FTRUNCATE
PyDoc_STRVAR(posix_ftruncate__doc__,
"ftruncate(fd, length)\n\n\
Truncate a file to a specified length.");

static PyObject *
posix_ftruncate(PyObject *self, PyObject *args)
{
    int fd;
    off_t length;
    int res;

    if (!PyArg_ParseTuple(args, "iO&:ftruncate", &fd, _parse_off_t, &length))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = ftruncate(fd, length);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_TRUNCATE
PyDoc_STRVAR(posix_truncate__doc__,
"truncate(path, length)\n\n\
Truncate the file given by path to length bytes.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.");

static PyObject *
posix_truncate(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    off_t length;
    int res;
    PyObject *result = NULL;
    static char *keywords[] = {"path", "length", NULL};

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FTRUNCATE
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&:truncate", keywords,
                                     path_converter, &path,
                                     _parse_off_t, &length))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FTRUNCATE
    if (path.fd != -1)
        res = ftruncate(path.fd, length);
    else
#endif
        res = truncate(path.narrow, length);
    Py_END_ALLOW_THREADS
    if (res < 0)
        result = path_posix_error("truncate", &path);
    else {
        Py_INCREF(Py_None);
        result = Py_None;
    }
    path_cleanup(&path);
    return result;
}
#endif

#ifdef HAVE_POSIX_FALLOCATE
PyDoc_STRVAR(posix_posix_fallocate__doc__,
"posix_fallocate(fd, offset, len)\n\n\
Ensures that enough disk space is allocated for the file specified by fd\n\
starting from offset and continuing for len bytes.");

static PyObject *
posix_posix_fallocate(PyObject *self, PyObject *args)
{
    off_t len, offset;
    int res, fd;

    if (!PyArg_ParseTuple(args, "iO&O&:posix_fallocate",
            &fd, _parse_off_t, &offset, _parse_off_t, &len))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = posix_fallocate(fd, offset, len);
    Py_END_ALLOW_THREADS
    if (res != 0) {
        errno = res;
        return posix_error();
    }
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_POSIX_FADVISE
PyDoc_STRVAR(posix_posix_fadvise__doc__,
"posix_fadvise(fd, offset, len, advice)\n\n\
Announces an intention to access data in a specific pattern thus allowing\n\
the kernel to make optimizations.\n\
The advice applies to the region of the file specified by fd starting at\n\
offset and continuing for len bytes.\n\
advice is one of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,\n\
POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED or\n\
POSIX_FADV_DONTNEED.");

static PyObject *
posix_posix_fadvise(PyObject *self, PyObject *args)
{
    off_t len, offset;
    int res, fd, advice;

    if (!PyArg_ParseTuple(args, "iO&O&i:posix_fadvise",
            &fd, _parse_off_t, &offset, _parse_off_t, &len, &advice))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    res = posix_fadvise(fd, offset, len, advice);
    Py_END_ALLOW_THREADS
    if (res != 0) {
        errno = res;
        return posix_error();
    }
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_PUTENV
PyDoc_STRVAR(posix_putenv__doc__,
"putenv(key, value)\n\n\
Change or add an environment variable.");

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *posix_putenv_garbage;

static PyObject *
posix_putenv(PyObject *self, PyObject *args)
{
    PyObject *newstr = NULL;
#ifdef MS_WINDOWS
    PyObject *os1, *os2;
    wchar_t *newenv;

    if (!PyArg_ParseTuple(args,
                          "UU:putenv",
                          &os1, &os2))
        return NULL;

    newstr = PyUnicode_FromFormat("%U=%U", os1, os2);
    if (newstr == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    if (_MAX_ENV < PyUnicode_GET_LENGTH(newstr)) {
        PyErr_Format(PyExc_ValueError,
                     "the environment variable is longer than %u characters",
                     _MAX_ENV);
        goto error;
    }

    newenv = PyUnicode_AsUnicode(newstr);
    if (newenv == NULL)
        goto error;
    if (_wputenv(newenv)) {
        posix_error();
        goto error;
    }
#else
    PyObject *os1, *os2;
    char *s1, *s2;
    char *newenv;

    if (!PyArg_ParseTuple(args,
                          "O&O&:putenv",
                          PyUnicode_FSConverter, &os1,
                          PyUnicode_FSConverter, &os2))
        return NULL;
    s1 = PyBytes_AsString(os1);
    s2 = PyBytes_AsString(os2);

    newstr = PyBytes_FromFormat("%s=%s", s1, s2);
    if (newstr == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    newenv = PyBytes_AS_STRING(newstr);
    if (putenv(newenv)) {
        posix_error();
        goto error;
    }
#endif

    /* Install the first arg and newstr in posix_putenv_garbage;
     * this will cause previous value to be collected.  This has to
     * happen after the real putenv() call because the old value
     * was still accessible until then. */
    if (PyDict_SetItem(posix_putenv_garbage, os1, newstr)) {
        /* really not much we can do; just leak */
        PyErr_Clear();
    }
    else {
        Py_DECREF(newstr);
    }

#ifndef MS_WINDOWS
    Py_DECREF(os1);
    Py_DECREF(os2);
#endif
    Py_RETURN_NONE;

error:
#ifndef MS_WINDOWS
    Py_DECREF(os1);
    Py_DECREF(os2);
#endif
    Py_XDECREF(newstr);
    return NULL;
}
#endif /* putenv */

#ifdef HAVE_UNSETENV
PyDoc_STRVAR(posix_unsetenv__doc__,
"unsetenv(key)\n\n\
Delete an environment variable.");

static PyObject *
posix_unsetenv(PyObject *self, PyObject *args)
{
    PyObject *name;
#ifndef HAVE_BROKEN_UNSETENV
    int err;
#endif

    if (!PyArg_ParseTuple(args, "O&:unsetenv",

                          PyUnicode_FSConverter, &name))
        return NULL;

#ifdef HAVE_BROKEN_UNSETENV
    unsetenv(PyBytes_AS_STRING(name));
#else
    err = unsetenv(PyBytes_AS_STRING(name));
    if (err) {
        Py_DECREF(name);
        return posix_error();
    }
#endif

    /* Remove the key from posix_putenv_garbage;
     * this will cause it to be collected.  This has to
     * happen after the real unsetenv() call because the
     * old value was still accessible until then.
     */
    if (PyDict_DelItem(posix_putenv_garbage, name)) {
        /* really not much we can do; just leak */
        PyErr_Clear();
    }
    Py_DECREF(name);
    Py_RETURN_NONE;
}
#endif /* unsetenv */

PyDoc_STRVAR(posix_strerror__doc__,
"strerror(code) -> string\n\n\
Translate an error code to a message string.");

static PyObject *
posix_strerror(PyObject *self, PyObject *args)
{
    int code;
    char *message;
    if (!PyArg_ParseTuple(args, "i:strerror", &code))
        return NULL;
    message = strerror(code);
    if (message == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "strerror() argument out of range");
        return NULL;
    }
    return PyUnicode_DecodeLocale(message, "surrogateescape");
}


#ifdef HAVE_SYS_WAIT_H

#ifdef WCOREDUMP
PyDoc_STRVAR(posix_WCOREDUMP__doc__,
"WCOREDUMP(status) -> bool\n\n\
Return True if the process returning 'status' was dumped to a core file.");

static PyObject *
posix_WCOREDUMP(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WCOREDUMP", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WCOREDUMP(status));
}
#endif /* WCOREDUMP */

#ifdef WIFCONTINUED
PyDoc_STRVAR(posix_WIFCONTINUED__doc__,
"WIFCONTINUED(status) -> bool\n\n\
Return True if the process returning 'status' was continued from a\n\
job control stop.");

static PyObject *
posix_WIFCONTINUED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WCONTINUED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFCONTINUED(status));
}
#endif /* WIFCONTINUED */

#ifdef WIFSTOPPED
PyDoc_STRVAR(posix_WIFSTOPPED__doc__,
"WIFSTOPPED(status) -> bool\n\n\
Return True if the process returning 'status' was stopped.");

static PyObject *
posix_WIFSTOPPED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFSTOPPED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFSTOPPED(status));
}
#endif /* WIFSTOPPED */

#ifdef WIFSIGNALED
PyDoc_STRVAR(posix_WIFSIGNALED__doc__,
"WIFSIGNALED(status) -> bool\n\n\
Return True if the process returning 'status' was terminated by a signal.");

static PyObject *
posix_WIFSIGNALED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFSIGNALED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFSIGNALED(status));
}
#endif /* WIFSIGNALED */

#ifdef WIFEXITED
PyDoc_STRVAR(posix_WIFEXITED__doc__,
"WIFEXITED(status) -> bool\n\n\
Return true if the process returning 'status' exited using the exit()\n\
system call.");

static PyObject *
posix_WIFEXITED(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WIFEXITED", &WAIT_STATUS_INT(status)))
        return NULL;

    return PyBool_FromLong(WIFEXITED(status));
}
#endif /* WIFEXITED */

#ifdef WEXITSTATUS
PyDoc_STRVAR(posix_WEXITSTATUS__doc__,
"WEXITSTATUS(status) -> integer\n\n\
Return the process return code from 'status'.");

static PyObject *
posix_WEXITSTATUS(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WEXITSTATUS", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WEXITSTATUS(status));
}
#endif /* WEXITSTATUS */

#ifdef WTERMSIG
PyDoc_STRVAR(posix_WTERMSIG__doc__,
"WTERMSIG(status) -> integer\n\n\
Return the signal that terminated the process that provided the 'status'\n\
value.");

static PyObject *
posix_WTERMSIG(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WTERMSIG", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WTERMSIG(status));
}
#endif /* WTERMSIG */

#ifdef WSTOPSIG
PyDoc_STRVAR(posix_WSTOPSIG__doc__,
"WSTOPSIG(status) -> integer\n\n\
Return the signal that stopped the process that provided\n\
the 'status' value.");

static PyObject *
posix_WSTOPSIG(PyObject *self, PyObject *args)
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    if (!PyArg_ParseTuple(args, "i:WSTOPSIG", &WAIT_STATUS_INT(status)))
        return NULL;

    return Py_BuildValue("i", WSTOPSIG(status));
}
#endif /* WSTOPSIG */

#endif /* HAVE_SYS_WAIT_H */


#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
#ifdef _SCO_DS
/* SCO OpenServer 5.0 and later requires _SVID3 before it reveals the
   needed definitions in sys/statvfs.h */
#define _SVID3
#endif
#include <sys/statvfs.h>

static PyObject*
_pystatvfs_fromstructstatvfs(struct statvfs st) {
    PyObject *v = PyStructSequence_New(&StatVFSResultType);
    if (v == NULL)
        return NULL;

#if !defined(HAVE_LARGEFILE_SUPPORT)
    PyStructSequence_SET_ITEM(v, 0, PyLong_FromLong((long) st.f_bsize));
    PyStructSequence_SET_ITEM(v, 1, PyLong_FromLong((long) st.f_frsize));
    PyStructSequence_SET_ITEM(v, 2, PyLong_FromLong((long) st.f_blocks));
    PyStructSequence_SET_ITEM(v, 3, PyLong_FromLong((long) st.f_bfree));
    PyStructSequence_SET_ITEM(v, 4, PyLong_FromLong((long) st.f_bavail));
    PyStructSequence_SET_ITEM(v, 5, PyLong_FromLong((long) st.f_files));
    PyStructSequence_SET_ITEM(v, 6, PyLong_FromLong((long) st.f_ffree));
    PyStructSequence_SET_ITEM(v, 7, PyLong_FromLong((long) st.f_favail));
    PyStructSequence_SET_ITEM(v, 8, PyLong_FromLong((long) st.f_flag));
    PyStructSequence_SET_ITEM(v, 9, PyLong_FromLong((long) st.f_namemax));
#else
    PyStructSequence_SET_ITEM(v, 0, PyLong_FromLong((long) st.f_bsize));
    PyStructSequence_SET_ITEM(v, 1, PyLong_FromLong((long) st.f_frsize));
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_blocks));
    PyStructSequence_SET_ITEM(v, 3,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_bfree));
    PyStructSequence_SET_ITEM(v, 4,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_bavail));
    PyStructSequence_SET_ITEM(v, 5,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_files));
    PyStructSequence_SET_ITEM(v, 6,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_ffree));
    PyStructSequence_SET_ITEM(v, 7,
                              PyLong_FromLongLong((PY_LONG_LONG) st.f_favail));
    PyStructSequence_SET_ITEM(v, 8, PyLong_FromLong((long) st.f_flag));
    PyStructSequence_SET_ITEM(v, 9, PyLong_FromLong((long) st.f_namemax));
#endif

    return v;
}

PyDoc_STRVAR(posix_fstatvfs__doc__,
"fstatvfs(fd) -> statvfs result\n\n\
Perform an fstatvfs system call on the given fd.\n\
Equivalent to statvfs(fd).");

static PyObject *
posix_fstatvfs(PyObject *self, PyObject *args)
{
    int fd, res;
    struct statvfs st;

    if (!PyArg_ParseTuple(args, "i:fstatvfs", &fd))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    res = fstatvfs(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0)
        return posix_error();

    return _pystatvfs_fromstructstatvfs(st);
}
#endif /* HAVE_FSTATVFS && HAVE_SYS_STATVFS_H */


#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>

PyDoc_STRVAR(posix_statvfs__doc__,
"statvfs(path)\n\n\
Perform a statvfs system call on the given path.\n\
\n\
path may always be specified as a string.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.");

static PyObject *
posix_statvfs(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *keywords[] = {"path", NULL};
    path_t path;
    int result;
    PyObject *return_value = NULL;
    struct statvfs st;

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FSTATVFS
    path.allow_fd = 1;
#endif
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&:statvfs", keywords,
        path_converter, &path
        ))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FSTATVFS
    if (path.fd != -1) {
#ifdef __APPLE__
        /* handle weak-linking on Mac OS X 10.3 */
        if (fstatvfs == NULL) {
            fd_specified("statvfs", path.fd);
            goto exit;
        }
#endif
        result = fstatvfs(path.fd, &st);
    }
    else
#endif
        result = statvfs(path.narrow, &st);
    Py_END_ALLOW_THREADS

    if (result) {
        return_value = path_posix_error("statvfs", &path);
        goto exit;
    }

    return_value = _pystatvfs_fromstructstatvfs(st);

exit:
    path_cleanup(&path);
    return return_value;
}
#endif /* HAVE_STATVFS */

#ifdef MS_WINDOWS
PyDoc_STRVAR(win32__getdiskusage__doc__,
"_getdiskusage(path) -> (total, free)\n\n\
Return disk usage statistics about the given path as (total, free) tuple.");

static PyObject *
win32__getdiskusage(PyObject *self, PyObject *args)
{
    BOOL retval;
    ULARGE_INTEGER _, total, free;
    const wchar_t *path;

    if (! PyArg_ParseTuple(args, "u", &path))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    retval = GetDiskFreeSpaceExW(path, &_, &total, &free);
    Py_END_ALLOW_THREADS
    if (retval == 0)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue("(LL)", total.QuadPart, free.QuadPart);
}
#endif


/* This is used for fpathconf(), pathconf(), confstr() and sysconf().
 * It maps strings representing configuration variable names to
 * integer values, allowing those functions to be called with the
 * magic names instead of polluting the module's namespace with tons of
 * rarely-used constants.  There are three separate tables that use
 * these definitions.
 *
 * This code is always included, even if none of the interfaces that
 * need it are included.  The #if hackery needed to avoid it would be
 * sufficiently pervasive that it's not worth the loss of readability.
 */
struct constdef {
    char *name;
    long value;
};

static int
conv_confname(PyObject *arg, int *valuep, struct constdef *table,
              size_t tablesize)
{
    if (PyLong_Check(arg)) {
        *valuep = PyLong_AS_LONG(arg);
        return 1;
    }
    else {
        /* look up the value in the table using a binary search */
        size_t lo = 0;
        size_t mid;
        size_t hi = tablesize;
        int cmp;
        const char *confname;
        if (!PyUnicode_Check(arg)) {
            PyErr_SetString(PyExc_TypeError,
                "configuration names must be strings or integers");
            return 0;
        }
        confname = _PyUnicode_AsString(arg);
        if (confname == NULL)
            return 0;
        while (lo < hi) {
            mid = (lo + hi) / 2;
            cmp = strcmp(confname, table[mid].name);
            if (cmp < 0)
                hi = mid;
            else if (cmp > 0)
                lo = mid + 1;
            else {
                *valuep = table[mid].value;
                return 1;
            }
        }
        PyErr_SetString(PyExc_ValueError, "unrecognized configuration name");
        return 0;
    }
}


#if defined(HAVE_FPATHCONF) || defined(HAVE_PATHCONF)
static struct constdef  posix_constants_pathconf[] = {
#ifdef _PC_ABI_AIO_XFER_MAX
    {"PC_ABI_AIO_XFER_MAX",     _PC_ABI_AIO_XFER_MAX},
#endif
#ifdef _PC_ABI_ASYNC_IO
    {"PC_ABI_ASYNC_IO", _PC_ABI_ASYNC_IO},
#endif
#ifdef _PC_ASYNC_IO
    {"PC_ASYNC_IO",     _PC_ASYNC_IO},
#endif
#ifdef _PC_CHOWN_RESTRICTED
    {"PC_CHOWN_RESTRICTED",     _PC_CHOWN_RESTRICTED},
#endif
#ifdef _PC_FILESIZEBITS
    {"PC_FILESIZEBITS", _PC_FILESIZEBITS},
#endif
#ifdef _PC_LAST
    {"PC_LAST", _PC_LAST},
#endif
#ifdef _PC_LINK_MAX
    {"PC_LINK_MAX",     _PC_LINK_MAX},
#endif
#ifdef _PC_MAX_CANON
    {"PC_MAX_CANON",    _PC_MAX_CANON},
#endif
#ifdef _PC_MAX_INPUT
    {"PC_MAX_INPUT",    _PC_MAX_INPUT},
#endif
#ifdef _PC_NAME_MAX
    {"PC_NAME_MAX",     _PC_NAME_MAX},
#endif
#ifdef _PC_NO_TRUNC
    {"PC_NO_TRUNC",     _PC_NO_TRUNC},
#endif
#ifdef _PC_PATH_MAX
    {"PC_PATH_MAX",     _PC_PATH_MAX},
#endif
#ifdef _PC_PIPE_BUF
    {"PC_PIPE_BUF",     _PC_PIPE_BUF},
#endif
#ifdef _PC_PRIO_IO
    {"PC_PRIO_IO",      _PC_PRIO_IO},
#endif
#ifdef _PC_SOCK_MAXBUF
    {"PC_SOCK_MAXBUF",  _PC_SOCK_MAXBUF},
#endif
#ifdef _PC_SYNC_IO
    {"PC_SYNC_IO",      _PC_SYNC_IO},
#endif
#ifdef _PC_VDISABLE
    {"PC_VDISABLE",     _PC_VDISABLE},
#endif
#ifdef _PC_ACL_ENABLED
    {"PC_ACL_ENABLED",  _PC_ACL_ENABLED},
#endif
#ifdef _PC_MIN_HOLE_SIZE
    {"PC_MIN_HOLE_SIZE",    _PC_MIN_HOLE_SIZE},
#endif
#ifdef _PC_ALLOC_SIZE_MIN
    {"PC_ALLOC_SIZE_MIN",   _PC_ALLOC_SIZE_MIN},
#endif
#ifdef _PC_REC_INCR_XFER_SIZE
    {"PC_REC_INCR_XFER_SIZE",   _PC_REC_INCR_XFER_SIZE},
#endif
#ifdef _PC_REC_MAX_XFER_SIZE
    {"PC_REC_MAX_XFER_SIZE",    _PC_REC_MAX_XFER_SIZE},
#endif
#ifdef _PC_REC_MIN_XFER_SIZE
    {"PC_REC_MIN_XFER_SIZE",    _PC_REC_MIN_XFER_SIZE},
#endif
#ifdef _PC_REC_XFER_ALIGN
    {"PC_REC_XFER_ALIGN",   _PC_REC_XFER_ALIGN},
#endif
#ifdef _PC_SYMLINK_MAX
    {"PC_SYMLINK_MAX",  _PC_SYMLINK_MAX},
#endif
#ifdef _PC_XATTR_ENABLED
    {"PC_XATTR_ENABLED",    _PC_XATTR_ENABLED},
#endif
#ifdef _PC_XATTR_EXISTS
    {"PC_XATTR_EXISTS", _PC_XATTR_EXISTS},
#endif
#ifdef _PC_TIMESTAMP_RESOLUTION
    {"PC_TIMESTAMP_RESOLUTION", _PC_TIMESTAMP_RESOLUTION},
#endif
};

static int
conv_path_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, posix_constants_pathconf,
                         sizeof(posix_constants_pathconf)
                           / sizeof(struct constdef));
}
#endif

#ifdef HAVE_FPATHCONF
PyDoc_STRVAR(posix_fpathconf__doc__,
"fpathconf(fd, name) -> integer\n\n\
Return the configuration limit name for the file descriptor fd.\n\
If there is no limit, return -1.");

static PyObject *
posix_fpathconf(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int name, fd;

    if (PyArg_ParseTuple(args, "iO&:fpathconf", &fd,
                         conv_path_confname, &name)) {
        long limit;

        errno = 0;
        limit = fpathconf(fd, name);
        if (limit == -1 && errno != 0)
            posix_error();
        else
            result = PyLong_FromLong(limit);
    }
    return result;
}
#endif


#ifdef HAVE_PATHCONF
PyDoc_STRVAR(posix_pathconf__doc__,
"pathconf(path, name) -> integer\n\n\
Return the configuration limit name for the file or directory path.\n\
If there is no limit, return -1.\n\
On some platforms, path may also be specified as an open file descriptor.\n\
  If this functionality is unavailable, using it raises an exception.");

static PyObject *
posix_pathconf(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    PyObject *result = NULL;
    int name;
    static char *keywords[] = {"path", "name", NULL};

    memset(&path, 0, sizeof(path));
#ifdef HAVE_FPATHCONF
    path.allow_fd = 1;
#endif
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&:pathconf", keywords,
                                    path_converter, &path,
                                    conv_path_confname, &name)) {
    long limit;

    errno = 0;
#ifdef HAVE_FPATHCONF
    if (path.fd != -1)
        limit = fpathconf(path.fd, name);
    else
#endif
        limit = pathconf(path.narrow, name);
    if (limit == -1 && errno != 0) {
        if (errno == EINVAL)
            /* could be a path or name problem */
            posix_error();
        else
            result = path_posix_error("pathconf", &path);
    }
    else
        result = PyLong_FromLong(limit);
    }
    path_cleanup(&path);
    return result;
}
#endif

#ifdef HAVE_CONFSTR
static struct constdef posix_constants_confstr[] = {
#ifdef _CS_ARCHITECTURE
    {"CS_ARCHITECTURE", _CS_ARCHITECTURE},
#endif
#ifdef _CS_GNU_LIBC_VERSION
    {"CS_GNU_LIBC_VERSION",     _CS_GNU_LIBC_VERSION},
#endif
#ifdef _CS_GNU_LIBPTHREAD_VERSION
    {"CS_GNU_LIBPTHREAD_VERSION",       _CS_GNU_LIBPTHREAD_VERSION},
#endif
#ifdef _CS_HOSTNAME
    {"CS_HOSTNAME",     _CS_HOSTNAME},
#endif
#ifdef _CS_HW_PROVIDER
    {"CS_HW_PROVIDER",  _CS_HW_PROVIDER},
#endif
#ifdef _CS_HW_SERIAL
    {"CS_HW_SERIAL",    _CS_HW_SERIAL},
#endif
#ifdef _CS_INITTAB_NAME
    {"CS_INITTAB_NAME", _CS_INITTAB_NAME},
#endif
#ifdef _CS_LFS64_CFLAGS
    {"CS_LFS64_CFLAGS", _CS_LFS64_CFLAGS},
#endif
#ifdef _CS_LFS64_LDFLAGS
    {"CS_LFS64_LDFLAGS",        _CS_LFS64_LDFLAGS},
#endif
#ifdef _CS_LFS64_LIBS
    {"CS_LFS64_LIBS",   _CS_LFS64_LIBS},
#endif
#ifdef _CS_LFS64_LINTFLAGS
    {"CS_LFS64_LINTFLAGS",      _CS_LFS64_LINTFLAGS},
#endif
#ifdef _CS_LFS_CFLAGS
    {"CS_LFS_CFLAGS",   _CS_LFS_CFLAGS},
#endif
#ifdef _CS_LFS_LDFLAGS
    {"CS_LFS_LDFLAGS",  _CS_LFS_LDFLAGS},
#endif
#ifdef _CS_LFS_LIBS
    {"CS_LFS_LIBS",     _CS_LFS_LIBS},
#endif
#ifdef _CS_LFS_LINTFLAGS
    {"CS_LFS_LINTFLAGS",        _CS_LFS_LINTFLAGS},
#endif
#ifdef _CS_MACHINE
    {"CS_MACHINE",      _CS_MACHINE},
#endif
#ifdef _CS_PATH
    {"CS_PATH", _CS_PATH},
#endif
#ifdef _CS_RELEASE
    {"CS_RELEASE",      _CS_RELEASE},
#endif
#ifdef _CS_SRPC_DOMAIN
    {"CS_SRPC_DOMAIN",  _CS_SRPC_DOMAIN},
#endif
#ifdef _CS_SYSNAME
    {"CS_SYSNAME",      _CS_SYSNAME},
#endif
#ifdef _CS_VERSION
    {"CS_VERSION",      _CS_VERSION},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_CFLAGS
    {"CS_XBS5_ILP32_OFF32_CFLAGS",      _CS_XBS5_ILP32_OFF32_CFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LDFLAGS
    {"CS_XBS5_ILP32_OFF32_LDFLAGS",     _CS_XBS5_ILP32_OFF32_LDFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LIBS
    {"CS_XBS5_ILP32_OFF32_LIBS",        _CS_XBS5_ILP32_OFF32_LIBS},
#endif
#ifdef _CS_XBS5_ILP32_OFF32_LINTFLAGS
    {"CS_XBS5_ILP32_OFF32_LINTFLAGS",   _CS_XBS5_ILP32_OFF32_LINTFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_CFLAGS
    {"CS_XBS5_ILP32_OFFBIG_CFLAGS",     _CS_XBS5_ILP32_OFFBIG_CFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LDFLAGS
    {"CS_XBS5_ILP32_OFFBIG_LDFLAGS",    _CS_XBS5_ILP32_OFFBIG_LDFLAGS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LIBS
    {"CS_XBS5_ILP32_OFFBIG_LIBS",       _CS_XBS5_ILP32_OFFBIG_LIBS},
#endif
#ifdef _CS_XBS5_ILP32_OFFBIG_LINTFLAGS
    {"CS_XBS5_ILP32_OFFBIG_LINTFLAGS",  _CS_XBS5_ILP32_OFFBIG_LINTFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_CFLAGS
    {"CS_XBS5_LP64_OFF64_CFLAGS",       _CS_XBS5_LP64_OFF64_CFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LDFLAGS
    {"CS_XBS5_LP64_OFF64_LDFLAGS",      _CS_XBS5_LP64_OFF64_LDFLAGS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LIBS
    {"CS_XBS5_LP64_OFF64_LIBS", _CS_XBS5_LP64_OFF64_LIBS},
#endif
#ifdef _CS_XBS5_LP64_OFF64_LINTFLAGS
    {"CS_XBS5_LP64_OFF64_LINTFLAGS",    _CS_XBS5_LP64_OFF64_LINTFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_CFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_CFLAGS",     _CS_XBS5_LPBIG_OFFBIG_CFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LDFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_LDFLAGS",    _CS_XBS5_LPBIG_OFFBIG_LDFLAGS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LIBS
    {"CS_XBS5_LPBIG_OFFBIG_LIBS",       _CS_XBS5_LPBIG_OFFBIG_LIBS},
#endif
#ifdef _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS
    {"CS_XBS5_LPBIG_OFFBIG_LINTFLAGS",  _CS_XBS5_LPBIG_OFFBIG_LINTFLAGS},
#endif
#ifdef _MIPS_CS_AVAIL_PROCESSORS
    {"MIPS_CS_AVAIL_PROCESSORS",        _MIPS_CS_AVAIL_PROCESSORS},
#endif
#ifdef _MIPS_CS_BASE
    {"MIPS_CS_BASE",    _MIPS_CS_BASE},
#endif
#ifdef _MIPS_CS_HOSTID
    {"MIPS_CS_HOSTID",  _MIPS_CS_HOSTID},
#endif
#ifdef _MIPS_CS_HW_NAME
    {"MIPS_CS_HW_NAME", _MIPS_CS_HW_NAME},
#endif
#ifdef _MIPS_CS_NUM_PROCESSORS
    {"MIPS_CS_NUM_PROCESSORS",  _MIPS_CS_NUM_PROCESSORS},
#endif
#ifdef _MIPS_CS_OSREL_MAJ
    {"MIPS_CS_OSREL_MAJ",       _MIPS_CS_OSREL_MAJ},
#endif
#ifdef _MIPS_CS_OSREL_MIN
    {"MIPS_CS_OSREL_MIN",       _MIPS_CS_OSREL_MIN},
#endif
#ifdef _MIPS_CS_OSREL_PATCH
    {"MIPS_CS_OSREL_PATCH",     _MIPS_CS_OSREL_PATCH},
#endif
#ifdef _MIPS_CS_OS_NAME
    {"MIPS_CS_OS_NAME", _MIPS_CS_OS_NAME},
#endif
#ifdef _MIPS_CS_OS_PROVIDER
    {"MIPS_CS_OS_PROVIDER",     _MIPS_CS_OS_PROVIDER},
#endif
#ifdef _MIPS_CS_PROCESSORS
    {"MIPS_CS_PROCESSORS",      _MIPS_CS_PROCESSORS},
#endif
#ifdef _MIPS_CS_SERIAL
    {"MIPS_CS_SERIAL",  _MIPS_CS_SERIAL},
#endif
#ifdef _MIPS_CS_VENDOR
    {"MIPS_CS_VENDOR",  _MIPS_CS_VENDOR},
#endif
};

static int
conv_confstr_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, posix_constants_confstr,
                         sizeof(posix_constants_confstr)
                           / sizeof(struct constdef));
}

PyDoc_STRVAR(posix_confstr__doc__,
"confstr(name) -> string\n\n\
Return a string-valued system configuration variable.");

static PyObject *
posix_confstr(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int name;
    char buffer[255];
    int len;

    if (!PyArg_ParseTuple(args, "O&:confstr", conv_confstr_confname, &name))
        return NULL;

    errno = 0;
    len = confstr(name, buffer, sizeof(buffer));
    if (len == 0) {
        if (errno) {
            posix_error();
            return NULL;
        }
        else {
            Py_RETURN_NONE;
        }
    }

    if ((unsigned int)len >= sizeof(buffer)) {
        char *buf = PyMem_Malloc(len);
        if (buf == NULL)
            return PyErr_NoMemory();
        confstr(name, buf, len);
        result = PyUnicode_DecodeFSDefaultAndSize(buf, len-1);
        PyMem_Free(buf);
    }
    else
        result = PyUnicode_DecodeFSDefaultAndSize(buffer, len-1);
    return result;
}
#endif


#ifdef HAVE_SYSCONF
static struct constdef posix_constants_sysconf[] = {
#ifdef _SC_2_CHAR_TERM
    {"SC_2_CHAR_TERM",  _SC_2_CHAR_TERM},
#endif
#ifdef _SC_2_C_BIND
    {"SC_2_C_BIND",     _SC_2_C_BIND},
#endif
#ifdef _SC_2_C_DEV
    {"SC_2_C_DEV",      _SC_2_C_DEV},
#endif
#ifdef _SC_2_C_VERSION
    {"SC_2_C_VERSION",  _SC_2_C_VERSION},
#endif
#ifdef _SC_2_FORT_DEV
    {"SC_2_FORT_DEV",   _SC_2_FORT_DEV},
#endif
#ifdef _SC_2_FORT_RUN
    {"SC_2_FORT_RUN",   _SC_2_FORT_RUN},
#endif
#ifdef _SC_2_LOCALEDEF
    {"SC_2_LOCALEDEF",  _SC_2_LOCALEDEF},
#endif
#ifdef _SC_2_SW_DEV
    {"SC_2_SW_DEV",     _SC_2_SW_DEV},
#endif
#ifdef _SC_2_UPE
    {"SC_2_UPE",        _SC_2_UPE},
#endif
#ifdef _SC_2_VERSION
    {"SC_2_VERSION",    _SC_2_VERSION},
#endif
#ifdef _SC_ABI_ASYNCHRONOUS_IO
    {"SC_ABI_ASYNCHRONOUS_IO",  _SC_ABI_ASYNCHRONOUS_IO},
#endif
#ifdef _SC_ACL
    {"SC_ACL",  _SC_ACL},
#endif
#ifdef _SC_AIO_LISTIO_MAX
    {"SC_AIO_LISTIO_MAX",       _SC_AIO_LISTIO_MAX},
#endif
#ifdef _SC_AIO_MAX
    {"SC_AIO_MAX",      _SC_AIO_MAX},
#endif
#ifdef _SC_AIO_PRIO_DELTA_MAX
    {"SC_AIO_PRIO_DELTA_MAX",   _SC_AIO_PRIO_DELTA_MAX},
#endif
#ifdef _SC_ARG_MAX
    {"SC_ARG_MAX",      _SC_ARG_MAX},
#endif
#ifdef _SC_ASYNCHRONOUS_IO
    {"SC_ASYNCHRONOUS_IO",      _SC_ASYNCHRONOUS_IO},
#endif
#ifdef _SC_ATEXIT_MAX
    {"SC_ATEXIT_MAX",   _SC_ATEXIT_MAX},
#endif
#ifdef _SC_AUDIT
    {"SC_AUDIT",        _SC_AUDIT},
#endif
#ifdef _SC_AVPHYS_PAGES
    {"SC_AVPHYS_PAGES", _SC_AVPHYS_PAGES},
#endif
#ifdef _SC_BC_BASE_MAX
    {"SC_BC_BASE_MAX",  _SC_BC_BASE_MAX},
#endif
#ifdef _SC_BC_DIM_MAX
    {"SC_BC_DIM_MAX",   _SC_BC_DIM_MAX},
#endif
#ifdef _SC_BC_SCALE_MAX
    {"SC_BC_SCALE_MAX", _SC_BC_SCALE_MAX},
#endif
#ifdef _SC_BC_STRING_MAX
    {"SC_BC_STRING_MAX",        _SC_BC_STRING_MAX},
#endif
#ifdef _SC_CAP
    {"SC_CAP",  _SC_CAP},
#endif
#ifdef _SC_CHARCLASS_NAME_MAX
    {"SC_CHARCLASS_NAME_MAX",   _SC_CHARCLASS_NAME_MAX},
#endif
#ifdef _SC_CHAR_BIT
    {"SC_CHAR_BIT",     _SC_CHAR_BIT},
#endif
#ifdef _SC_CHAR_MAX
    {"SC_CHAR_MAX",     _SC_CHAR_MAX},
#endif
#ifdef _SC_CHAR_MIN
    {"SC_CHAR_MIN",     _SC_CHAR_MIN},
#endif
#ifdef _SC_CHILD_MAX
    {"SC_CHILD_MAX",    _SC_CHILD_MAX},
#endif
#ifdef _SC_CLK_TCK
    {"SC_CLK_TCK",      _SC_CLK_TCK},
#endif
#ifdef _SC_COHER_BLKSZ
    {"SC_COHER_BLKSZ",  _SC_COHER_BLKSZ},
#endif
#ifdef _SC_COLL_WEIGHTS_MAX
    {"SC_COLL_WEIGHTS_MAX",     _SC_COLL_WEIGHTS_MAX},
#endif
#ifdef _SC_DCACHE_ASSOC
    {"SC_DCACHE_ASSOC", _SC_DCACHE_ASSOC},
#endif
#ifdef _SC_DCACHE_BLKSZ
    {"SC_DCACHE_BLKSZ", _SC_DCACHE_BLKSZ},
#endif
#ifdef _SC_DCACHE_LINESZ
    {"SC_DCACHE_LINESZ",        _SC_DCACHE_LINESZ},
#endif
#ifdef _SC_DCACHE_SZ
    {"SC_DCACHE_SZ",    _SC_DCACHE_SZ},
#endif
#ifdef _SC_DCACHE_TBLKSZ
    {"SC_DCACHE_TBLKSZ",        _SC_DCACHE_TBLKSZ},
#endif
#ifdef _SC_DELAYTIMER_MAX
    {"SC_DELAYTIMER_MAX",       _SC_DELAYTIMER_MAX},
#endif
#ifdef _SC_EQUIV_CLASS_MAX
    {"SC_EQUIV_CLASS_MAX",      _SC_EQUIV_CLASS_MAX},
#endif
#ifdef _SC_EXPR_NEST_MAX
    {"SC_EXPR_NEST_MAX",        _SC_EXPR_NEST_MAX},
#endif
#ifdef _SC_FSYNC
    {"SC_FSYNC",        _SC_FSYNC},
#endif
#ifdef _SC_GETGR_R_SIZE_MAX
    {"SC_GETGR_R_SIZE_MAX",     _SC_GETGR_R_SIZE_MAX},
#endif
#ifdef _SC_GETPW_R_SIZE_MAX
    {"SC_GETPW_R_SIZE_MAX",     _SC_GETPW_R_SIZE_MAX},
#endif
#ifdef _SC_ICACHE_ASSOC
    {"SC_ICACHE_ASSOC", _SC_ICACHE_ASSOC},
#endif
#ifdef _SC_ICACHE_BLKSZ
    {"SC_ICACHE_BLKSZ", _SC_ICACHE_BLKSZ},
#endif
#ifdef _SC_ICACHE_LINESZ
    {"SC_ICACHE_LINESZ",        _SC_ICACHE_LINESZ},
#endif
#ifdef _SC_ICACHE_SZ
    {"SC_ICACHE_SZ",    _SC_ICACHE_SZ},
#endif
#ifdef _SC_INF
    {"SC_INF",  _SC_INF},
#endif
#ifdef _SC_INT_MAX
    {"SC_INT_MAX",      _SC_INT_MAX},
#endif
#ifdef _SC_INT_MIN
    {"SC_INT_MIN",      _SC_INT_MIN},
#endif
#ifdef _SC_IOV_MAX
    {"SC_IOV_MAX",      _SC_IOV_MAX},
#endif
#ifdef _SC_IP_SECOPTS
    {"SC_IP_SECOPTS",   _SC_IP_SECOPTS},
#endif
#ifdef _SC_JOB_CONTROL
    {"SC_JOB_CONTROL",  _SC_JOB_CONTROL},
#endif
#ifdef _SC_KERN_POINTERS
    {"SC_KERN_POINTERS",        _SC_KERN_POINTERS},
#endif
#ifdef _SC_KERN_SIM
    {"SC_KERN_SIM",     _SC_KERN_SIM},
#endif
#ifdef _SC_LINE_MAX
    {"SC_LINE_MAX",     _SC_LINE_MAX},
#endif
#ifdef _SC_LOGIN_NAME_MAX
    {"SC_LOGIN_NAME_MAX",       _SC_LOGIN_NAME_MAX},
#endif
#ifdef _SC_LOGNAME_MAX
    {"SC_LOGNAME_MAX",  _SC_LOGNAME_MAX},
#endif
#ifdef _SC_LONG_BIT
    {"SC_LONG_BIT",     _SC_LONG_BIT},
#endif
#ifdef _SC_MAC
    {"SC_MAC",  _SC_MAC},
#endif
#ifdef _SC_MAPPED_FILES
    {"SC_MAPPED_FILES", _SC_MAPPED_FILES},
#endif
#ifdef _SC_MAXPID
    {"SC_MAXPID",       _SC_MAXPID},
#endif
#ifdef _SC_MB_LEN_MAX
    {"SC_MB_LEN_MAX",   _SC_MB_LEN_MAX},
#endif
#ifdef _SC_MEMLOCK
    {"SC_MEMLOCK",      _SC_MEMLOCK},
#endif
#ifdef _SC_MEMLOCK_RANGE
    {"SC_MEMLOCK_RANGE",        _SC_MEMLOCK_RANGE},
#endif
#ifdef _SC_MEMORY_PROTECTION
    {"SC_MEMORY_PROTECTION",    _SC_MEMORY_PROTECTION},
#endif
#ifdef _SC_MESSAGE_PASSING
    {"SC_MESSAGE_PASSING",      _SC_MESSAGE_PASSING},
#endif
#ifdef _SC_MMAP_FIXED_ALIGNMENT
    {"SC_MMAP_FIXED_ALIGNMENT", _SC_MMAP_FIXED_ALIGNMENT},
#endif
#ifdef _SC_MQ_OPEN_MAX
    {"SC_MQ_OPEN_MAX",  _SC_MQ_OPEN_MAX},
#endif
#ifdef _SC_MQ_PRIO_MAX
    {"SC_MQ_PRIO_MAX",  _SC_MQ_PRIO_MAX},
#endif
#ifdef _SC_NACLS_MAX
    {"SC_NACLS_MAX",    _SC_NACLS_MAX},
#endif
#ifdef _SC_NGROUPS_MAX
    {"SC_NGROUPS_MAX",  _SC_NGROUPS_MAX},
#endif
#ifdef _SC_NL_ARGMAX
    {"SC_NL_ARGMAX",    _SC_NL_ARGMAX},
#endif
#ifdef _SC_NL_LANGMAX
    {"SC_NL_LANGMAX",   _SC_NL_LANGMAX},
#endif
#ifdef _SC_NL_MSGMAX
    {"SC_NL_MSGMAX",    _SC_NL_MSGMAX},
#endif
#ifdef _SC_NL_NMAX
    {"SC_NL_NMAX",      _SC_NL_NMAX},
#endif
#ifdef _SC_NL_SETMAX
    {"SC_NL_SETMAX",    _SC_NL_SETMAX},
#endif
#ifdef _SC_NL_TEXTMAX
    {"SC_NL_TEXTMAX",   _SC_NL_TEXTMAX},
#endif
#ifdef _SC_NPROCESSORS_CONF
    {"SC_NPROCESSORS_CONF",     _SC_NPROCESSORS_CONF},
#endif
#ifdef _SC_NPROCESSORS_ONLN
    {"SC_NPROCESSORS_ONLN",     _SC_NPROCESSORS_ONLN},
#endif
#ifdef _SC_NPROC_CONF
    {"SC_NPROC_CONF",   _SC_NPROC_CONF},
#endif
#ifdef _SC_NPROC_ONLN
    {"SC_NPROC_ONLN",   _SC_NPROC_ONLN},
#endif
#ifdef _SC_NZERO
    {"SC_NZERO",        _SC_NZERO},
#endif
#ifdef _SC_OPEN_MAX
    {"SC_OPEN_MAX",     _SC_OPEN_MAX},
#endif
#ifdef _SC_PAGESIZE
    {"SC_PAGESIZE",     _SC_PAGESIZE},
#endif
#ifdef _SC_PAGE_SIZE
    {"SC_PAGE_SIZE",    _SC_PAGE_SIZE},
#endif
#ifdef _SC_PASS_MAX
    {"SC_PASS_MAX",     _SC_PASS_MAX},
#endif
#ifdef _SC_PHYS_PAGES
    {"SC_PHYS_PAGES",   _SC_PHYS_PAGES},
#endif
#ifdef _SC_PII
    {"SC_PII",  _SC_PII},
#endif
#ifdef _SC_PII_INTERNET
    {"SC_PII_INTERNET", _SC_PII_INTERNET},
#endif
#ifdef _SC_PII_INTERNET_DGRAM
    {"SC_PII_INTERNET_DGRAM",   _SC_PII_INTERNET_DGRAM},
#endif
#ifdef _SC_PII_INTERNET_STREAM
    {"SC_PII_INTERNET_STREAM",  _SC_PII_INTERNET_STREAM},
#endif
#ifdef _SC_PII_OSI
    {"SC_PII_OSI",      _SC_PII_OSI},
#endif
#ifdef _SC_PII_OSI_CLTS
    {"SC_PII_OSI_CLTS", _SC_PII_OSI_CLTS},
#endif
#ifdef _SC_PII_OSI_COTS
    {"SC_PII_OSI_COTS", _SC_PII_OSI_COTS},
#endif
#ifdef _SC_PII_OSI_M
    {"SC_PII_OSI_M",    _SC_PII_OSI_M},
#endif
#ifdef _SC_PII_SOCKET
    {"SC_PII_SOCKET",   _SC_PII_SOCKET},
#endif
#ifdef _SC_PII_XTI
    {"SC_PII_XTI",      _SC_PII_XTI},
#endif
#ifdef _SC_POLL
    {"SC_POLL", _SC_POLL},
#endif
#ifdef _SC_PRIORITIZED_IO
    {"SC_PRIORITIZED_IO",       _SC_PRIORITIZED_IO},
#endif
#ifdef _SC_PRIORITY_SCHEDULING
    {"SC_PRIORITY_SCHEDULING",  _SC_PRIORITY_SCHEDULING},
#endif
#ifdef _SC_REALTIME_SIGNALS
    {"SC_REALTIME_SIGNALS",     _SC_REALTIME_SIGNALS},
#endif
#ifdef _SC_RE_DUP_MAX
    {"SC_RE_DUP_MAX",   _SC_RE_DUP_MAX},
#endif
#ifdef _SC_RTSIG_MAX
    {"SC_RTSIG_MAX",    _SC_RTSIG_MAX},
#endif
#ifdef _SC_SAVED_IDS
    {"SC_SAVED_IDS",    _SC_SAVED_IDS},
#endif
#ifdef _SC_SCHAR_MAX
    {"SC_SCHAR_MAX",    _SC_SCHAR_MAX},
#endif
#ifdef _SC_SCHAR_MIN
    {"SC_SCHAR_MIN",    _SC_SCHAR_MIN},
#endif
#ifdef _SC_SELECT
    {"SC_SELECT",       _SC_SELECT},
#endif
#ifdef _SC_SEMAPHORES
    {"SC_SEMAPHORES",   _SC_SEMAPHORES},
#endif
#ifdef _SC_SEM_NSEMS_MAX
    {"SC_SEM_NSEMS_MAX",        _SC_SEM_NSEMS_MAX},
#endif
#ifdef _SC_SEM_VALUE_MAX
    {"SC_SEM_VALUE_MAX",        _SC_SEM_VALUE_MAX},
#endif
#ifdef _SC_SHARED_MEMORY_OBJECTS
    {"SC_SHARED_MEMORY_OBJECTS",        _SC_SHARED_MEMORY_OBJECTS},
#endif
#ifdef _SC_SHRT_MAX
    {"SC_SHRT_MAX",     _SC_SHRT_MAX},
#endif
#ifdef _SC_SHRT_MIN
    {"SC_SHRT_MIN",     _SC_SHRT_MIN},
#endif
#ifdef _SC_SIGQUEUE_MAX
    {"SC_SIGQUEUE_MAX", _SC_SIGQUEUE_MAX},
#endif
#ifdef _SC_SIGRT_MAX
    {"SC_SIGRT_MAX",    _SC_SIGRT_MAX},
#endif
#ifdef _SC_SIGRT_MIN
    {"SC_SIGRT_MIN",    _SC_SIGRT_MIN},
#endif
#ifdef _SC_SOFTPOWER
    {"SC_SOFTPOWER",    _SC_SOFTPOWER},
#endif
#ifdef _SC_SPLIT_CACHE
    {"SC_SPLIT_CACHE",  _SC_SPLIT_CACHE},
#endif
#ifdef _SC_SSIZE_MAX
    {"SC_SSIZE_MAX",    _SC_SSIZE_MAX},
#endif
#ifdef _SC_STACK_PROT
    {"SC_STACK_PROT",   _SC_STACK_PROT},
#endif
#ifdef _SC_STREAM_MAX
    {"SC_STREAM_MAX",   _SC_STREAM_MAX},
#endif
#ifdef _SC_SYNCHRONIZED_IO
    {"SC_SYNCHRONIZED_IO",      _SC_SYNCHRONIZED_IO},
#endif
#ifdef _SC_THREADS
    {"SC_THREADS",      _SC_THREADS},
#endif
#ifdef _SC_THREAD_ATTR_STACKADDR
    {"SC_THREAD_ATTR_STACKADDR",        _SC_THREAD_ATTR_STACKADDR},
#endif
#ifdef _SC_THREAD_ATTR_STACKSIZE
    {"SC_THREAD_ATTR_STACKSIZE",        _SC_THREAD_ATTR_STACKSIZE},
#endif
#ifdef _SC_THREAD_DESTRUCTOR_ITERATIONS
    {"SC_THREAD_DESTRUCTOR_ITERATIONS", _SC_THREAD_DESTRUCTOR_ITERATIONS},
#endif
#ifdef _SC_THREAD_KEYS_MAX
    {"SC_THREAD_KEYS_MAX",      _SC_THREAD_KEYS_MAX},
#endif
#ifdef _SC_THREAD_PRIORITY_SCHEDULING
    {"SC_THREAD_PRIORITY_SCHEDULING",   _SC_THREAD_PRIORITY_SCHEDULING},
#endif
#ifdef _SC_THREAD_PRIO_INHERIT
    {"SC_THREAD_PRIO_INHERIT",  _SC_THREAD_PRIO_INHERIT},
#endif
#ifdef _SC_THREAD_PRIO_PROTECT
    {"SC_THREAD_PRIO_PROTECT",  _SC_THREAD_PRIO_PROTECT},
#endif
#ifdef _SC_THREAD_PROCESS_SHARED
    {"SC_THREAD_PROCESS_SHARED",        _SC_THREAD_PROCESS_SHARED},
#endif
#ifdef _SC_THREAD_SAFE_FUNCTIONS
    {"SC_THREAD_SAFE_FUNCTIONS",        _SC_THREAD_SAFE_FUNCTIONS},
#endif
#ifdef _SC_THREAD_STACK_MIN
    {"SC_THREAD_STACK_MIN",     _SC_THREAD_STACK_MIN},
#endif
#ifdef _SC_THREAD_THREADS_MAX
    {"SC_THREAD_THREADS_MAX",   _SC_THREAD_THREADS_MAX},
#endif
#ifdef _SC_TIMERS
    {"SC_TIMERS",       _SC_TIMERS},
#endif
#ifdef _SC_TIMER_MAX
    {"SC_TIMER_MAX",    _SC_TIMER_MAX},
#endif
#ifdef _SC_TTY_NAME_MAX
    {"SC_TTY_NAME_MAX", _SC_TTY_NAME_MAX},
#endif
#ifdef _SC_TZNAME_MAX
    {"SC_TZNAME_MAX",   _SC_TZNAME_MAX},
#endif
#ifdef _SC_T_IOV_MAX
    {"SC_T_IOV_MAX",    _SC_T_IOV_MAX},
#endif
#ifdef _SC_UCHAR_MAX
    {"SC_UCHAR_MAX",    _SC_UCHAR_MAX},
#endif
#ifdef _SC_UINT_MAX
    {"SC_UINT_MAX",     _SC_UINT_MAX},
#endif
#ifdef _SC_UIO_MAXIOV
    {"SC_UIO_MAXIOV",   _SC_UIO_MAXIOV},
#endif
#ifdef _SC_ULONG_MAX
    {"SC_ULONG_MAX",    _SC_ULONG_MAX},
#endif
#ifdef _SC_USHRT_MAX
    {"SC_USHRT_MAX",    _SC_USHRT_MAX},
#endif
#ifdef _SC_VERSION
    {"SC_VERSION",      _SC_VERSION},
#endif
#ifdef _SC_WORD_BIT
    {"SC_WORD_BIT",     _SC_WORD_BIT},
#endif
#ifdef _SC_XBS5_ILP32_OFF32
    {"SC_XBS5_ILP32_OFF32",     _SC_XBS5_ILP32_OFF32},
#endif
#ifdef _SC_XBS5_ILP32_OFFBIG
    {"SC_XBS5_ILP32_OFFBIG",    _SC_XBS5_ILP32_OFFBIG},
#endif
#ifdef _SC_XBS5_LP64_OFF64
    {"SC_XBS5_LP64_OFF64",      _SC_XBS5_LP64_OFF64},
#endif
#ifdef _SC_XBS5_LPBIG_OFFBIG
    {"SC_XBS5_LPBIG_OFFBIG",    _SC_XBS5_LPBIG_OFFBIG},
#endif
#ifdef _SC_XOPEN_CRYPT
    {"SC_XOPEN_CRYPT",  _SC_XOPEN_CRYPT},
#endif
#ifdef _SC_XOPEN_ENH_I18N
    {"SC_XOPEN_ENH_I18N",       _SC_XOPEN_ENH_I18N},
#endif
#ifdef _SC_XOPEN_LEGACY
    {"SC_XOPEN_LEGACY", _SC_XOPEN_LEGACY},
#endif
#ifdef _SC_XOPEN_REALTIME
    {"SC_XOPEN_REALTIME",       _SC_XOPEN_REALTIME},
#endif
#ifdef _SC_XOPEN_REALTIME_THREADS
    {"SC_XOPEN_REALTIME_THREADS",       _SC_XOPEN_REALTIME_THREADS},
#endif
#ifdef _SC_XOPEN_SHM
    {"SC_XOPEN_SHM",    _SC_XOPEN_SHM},
#endif
#ifdef _SC_XOPEN_UNIX
    {"SC_XOPEN_UNIX",   _SC_XOPEN_UNIX},
#endif
#ifdef _SC_XOPEN_VERSION
    {"SC_XOPEN_VERSION",        _SC_XOPEN_VERSION},
#endif
#ifdef _SC_XOPEN_XCU_VERSION
    {"SC_XOPEN_XCU_VERSION",    _SC_XOPEN_XCU_VERSION},
#endif
#ifdef _SC_XOPEN_XPG2
    {"SC_XOPEN_XPG2",   _SC_XOPEN_XPG2},
#endif
#ifdef _SC_XOPEN_XPG3
    {"SC_XOPEN_XPG3",   _SC_XOPEN_XPG3},
#endif
#ifdef _SC_XOPEN_XPG4
    {"SC_XOPEN_XPG4",   _SC_XOPEN_XPG4},
#endif
};

static int
conv_sysconf_confname(PyObject *arg, int *valuep)
{
    return conv_confname(arg, valuep, posix_constants_sysconf,
                         sizeof(posix_constants_sysconf)
                           / sizeof(struct constdef));
}

PyDoc_STRVAR(posix_sysconf__doc__,
"sysconf(name) -> integer\n\n\
Return an integer-valued system configuration variable.");

static PyObject *
posix_sysconf(PyObject *self, PyObject *args)
{
    PyObject *result = NULL;
    int name;

    if (PyArg_ParseTuple(args, "O&:sysconf", conv_sysconf_confname, &name)) {
        int value;

        errno = 0;
        value = sysconf(name);
        if (value == -1 && errno != 0)
            posix_error();
        else
            result = PyLong_FromLong(value);
    }
    return result;
}
#endif


/* This code is used to ensure that the tables of configuration value names
 * are in sorted order as required by conv_confname(), and also to build the
 * the exported dictionaries that are used to publish information about the
 * names available on the host platform.
 *
 * Sorting the table at runtime ensures that the table is properly ordered
 * when used, even for platforms we're not able to test on.  It also makes
 * it easier to add additional entries to the tables.
 */

static int
cmp_constdefs(const void *v1,  const void *v2)
{
    const struct constdef *c1 =
    (const struct constdef *) v1;
    const struct constdef *c2 =
    (const struct constdef *) v2;

    return strcmp(c1->name, c2->name);
}

static int
setup_confname_table(struct constdef *table, size_t tablesize,
                     char *tablename, PyObject *module)
{
    PyObject *d = NULL;
    size_t i;

    qsort(table, tablesize, sizeof(struct constdef), cmp_constdefs);
    d = PyDict_New();
    if (d == NULL)
        return -1;

    for (i=0; i < tablesize; ++i) {
        PyObject *o = PyLong_FromLong(table[i].value);
        if (o == NULL || PyDict_SetItemString(d, table[i].name, o) == -1) {
            Py_XDECREF(o);
            Py_DECREF(d);
            return -1;
        }
        Py_DECREF(o);
    }
    return PyModule_AddObject(module, tablename, d);
}

/* Return -1 on failure, 0 on success. */
static int
setup_confname_tables(PyObject *module)
{
#if defined(HAVE_FPATHCONF) || defined(HAVE_PATHCONF)
    if (setup_confname_table(posix_constants_pathconf,
                             sizeof(posix_constants_pathconf)
                               / sizeof(struct constdef),
                             "pathconf_names", module))
        return -1;
#endif
#ifdef HAVE_CONFSTR
    if (setup_confname_table(posix_constants_confstr,
                             sizeof(posix_constants_confstr)
                               / sizeof(struct constdef),
                             "confstr_names", module))
        return -1;
#endif
#ifdef HAVE_SYSCONF
    if (setup_confname_table(posix_constants_sysconf,
                             sizeof(posix_constants_sysconf)
                               / sizeof(struct constdef),
                             "sysconf_names", module))
        return -1;
#endif
    return 0;
}


PyDoc_STRVAR(posix_abort__doc__,
"abort() -> does not return!\n\n\
Abort the interpreter immediately.  This 'dumps core' or otherwise fails\n\
in the hardest way possible on the hosting operating system.");

static PyObject *
posix_abort(PyObject *self, PyObject *noargs)
{
    abort();
    /*NOTREACHED*/
    Py_FatalError("abort() called from Python code didn't abort!");
    return NULL;
}

#ifdef MS_WINDOWS
PyDoc_STRVAR(win32_startfile__doc__,
"startfile(filepath [, operation]) - Start a file with its associated\n\
application.\n\
\n\
When \"operation\" is not specified or \"open\", this acts like\n\
double-clicking the file in Explorer, or giving the file name as an\n\
argument to the DOS \"start\" command: the file is opened with whatever\n\
application (if any) its extension is associated.\n\
When another \"operation\" is given, it specifies what should be done with\n\
the file.  A typical operation is \"print\".\n\
\n\
startfile returns as soon as the associated application is launched.\n\
There is no option to wait for the application to close, and no way\n\
to retrieve the application's exit status.\n\
\n\
The filepath is relative to the current directory.  If you want to use\n\
an absolute path, make sure the first character is not a slash (\"/\");\n\
the underlying Win32 ShellExecute function doesn't work if it is.");

static PyObject *
win32_startfile(PyObject *self, PyObject *args)
{
    PyObject *ofilepath;
    char *filepath;
    char *operation = NULL;
    wchar_t *wpath, *woperation;
    HINSTANCE rc;

    PyObject *unipath, *uoperation = NULL;
    if (!PyArg_ParseTuple(args, "U|s:startfile",
                          &unipath, &operation)) {
        PyErr_Clear();
        goto normal;
    }

    if (operation) {
        uoperation = PyUnicode_DecodeASCII(operation,
                                           strlen(operation), NULL);
        if (!uoperation) {
            PyErr_Clear();
            operation = NULL;
            goto normal;
        }
    }

    wpath = PyUnicode_AsUnicode(unipath);
    if (wpath == NULL)
        goto normal;
    if (uoperation) {
        woperation = PyUnicode_AsUnicode(uoperation);
        if (woperation == NULL)
            goto normal;
    }
    else
        woperation = NULL;

    Py_BEGIN_ALLOW_THREADS
    rc = ShellExecuteW((HWND)0, woperation, wpath,
                       NULL, NULL, SW_SHOWNORMAL);
    Py_END_ALLOW_THREADS

    Py_XDECREF(uoperation);
    if (rc <= (HINSTANCE)32) {
        win32_error_object("startfile", unipath);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;

normal:
    if (!PyArg_ParseTuple(args, "O&|s:startfile",
                          PyUnicode_FSConverter, &ofilepath,
                          &operation))
        return NULL;
    if (win32_warn_bytes_api()) {
        Py_DECREF(ofilepath);
        return NULL;
    }
    filepath = PyBytes_AsString(ofilepath);
    Py_BEGIN_ALLOW_THREADS
    rc = ShellExecute((HWND)0, operation, filepath,
                      NULL, NULL, SW_SHOWNORMAL);
    Py_END_ALLOW_THREADS
    if (rc <= (HINSTANCE)32) {
        PyObject *errval = win32_error("startfile", filepath);
        Py_DECREF(ofilepath);
        return errval;
    }
    Py_DECREF(ofilepath);
    Py_INCREF(Py_None);
    return Py_None;
}
#endif

#ifdef HAVE_GETLOADAVG
PyDoc_STRVAR(posix_getloadavg__doc__,
"getloadavg() -> (float, float, float)\n\n\
Return the number of processes in the system run queue averaged over\n\
the last 1, 5, and 15 minutes or raises OSError if the load average\n\
was unobtainable");

static PyObject *
posix_getloadavg(PyObject *self, PyObject *noargs)
{
    double loadavg[3];
    if (getloadavg(loadavg, 3)!=3) {
        PyErr_SetString(PyExc_OSError, "Load averages are unobtainable");
        return NULL;
    } else
        return Py_BuildValue("ddd", loadavg[0], loadavg[1], loadavg[2]);
}
#endif

PyDoc_STRVAR(device_encoding__doc__,
"device_encoding(fd) -> str\n\n\
Return a string describing the encoding of the device\n\
if the output is a terminal; else return None.");

static PyObject *
device_encoding(PyObject *self, PyObject *args)
{
    int fd;

    if (!PyArg_ParseTuple(args, "i:device_encoding", &fd))
        return NULL;

    return _Py_device_encoding(fd);
}

#ifdef __VMS
/* Use openssl random routine */
#include <openssl/rand.h>
PyDoc_STRVAR(vms_urandom__doc__,
"urandom(n) -> str\n\n\
Return n random bytes suitable for cryptographic use.");

static PyObject*
vms_urandom(PyObject *self, PyObject *args)
{
    int howMany;
    PyObject* result;

    /* Read arguments */
    if (! PyArg_ParseTuple(args, "i:urandom", &howMany))
        return NULL;
    if (howMany < 0)
        return PyErr_Format(PyExc_ValueError,
                            "negative argument not allowed");

    /* Allocate bytes */
    result = PyBytes_FromStringAndSize(NULL, howMany);
    if (result != NULL) {
        /* Get random data */
        if (RAND_pseudo_bytes((unsigned char*)
                              PyBytes_AS_STRING(result),
                              howMany) < 0) {
            Py_DECREF(result);
            return PyErr_Format(PyExc_ValueError,
                                "RAND_pseudo_bytes");
        }
    }
    return result;
}
#endif

#ifdef HAVE_SETRESUID
PyDoc_STRVAR(posix_setresuid__doc__,
"setresuid(ruid, euid, suid)\n\n\
Set the current process's real, effective, and saved user ids.");

static PyObject*
posix_setresuid (PyObject *self, PyObject *args)
{
    /* We assume uid_t is no larger than a long. */
    long ruid, euid, suid;
    if (!PyArg_ParseTuple(args, "lll", &ruid, &euid, &suid))
        return NULL;
    if (setresuid(ruid, euid, suid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_SETRESGID
PyDoc_STRVAR(posix_setresgid__doc__,
"setresgid(rgid, egid, sgid)\n\n\
Set the current process's real, effective, and saved group ids.");

static PyObject*
posix_setresgid (PyObject *self, PyObject *args)
{
    /* We assume uid_t is no larger than a long. */
    long rgid, egid, sgid;
    if (!PyArg_ParseTuple(args, "lll", &rgid, &egid, &sgid))
        return NULL;
    if (setresgid(rgid, egid, sgid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_GETRESUID
PyDoc_STRVAR(posix_getresuid__doc__,
"getresuid() -> (ruid, euid, suid)\n\n\
Get tuple of the current process's real, effective, and saved user ids.");

static PyObject*
posix_getresuid (PyObject *self, PyObject *noargs)
{
    uid_t ruid, euid, suid;
    long l_ruid, l_euid, l_suid;
    if (getresuid(&ruid, &euid, &suid) < 0)
        return posix_error();
    /* Force the values into long's as we don't know the size of uid_t. */
    l_ruid = ruid;
    l_euid = euid;
    l_suid = suid;
    return Py_BuildValue("(lll)", l_ruid, l_euid, l_suid);
}
#endif

#ifdef HAVE_GETRESGID
PyDoc_STRVAR(posix_getresgid__doc__,
"getresgid() -> (rgid, egid, sgid)\n\n\
Get tuple of the current process's real, effective, and saved group ids.");

static PyObject*
posix_getresgid (PyObject *self, PyObject *noargs)
{
    uid_t rgid, egid, sgid;
    long l_rgid, l_egid, l_sgid;
    if (getresgid(&rgid, &egid, &sgid) < 0)
        return posix_error();
    /* Force the values into long's as we don't know the size of uid_t. */
    l_rgid = rgid;
    l_egid = egid;
    l_sgid = sgid;
    return Py_BuildValue("(lll)", l_rgid, l_egid, l_sgid);
}
#endif

#ifdef USE_XATTRS

PyDoc_STRVAR(posix_getxattr__doc__,
"getxattr(path, attribute, *, follow_symlinks=True) -> value\n\n\
Return the value of extended attribute attribute on path.\n\
\n\
path may be either a string or an open file descriptor.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, getxattr will examine the symbolic link itself instead of the file\n\
  the link points to.");

static PyObject *
posix_getxattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    path_t attribute;
    int follow_symlinks = 1;
    PyObject *buffer = NULL;
    int i;
    static char *keywords[] = {"path", "attribute", "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
    memset(&attribute, 0, sizeof(attribute));
    path.allow_fd = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&|$p:getxattr", keywords,
        path_converter, &path,
        path_converter, &attribute,
        &follow_symlinks))
        return NULL;

    if (fd_and_follow_symlinks_invalid("getxattr", path.fd, follow_symlinks))
        goto exit;

    for (i = 0; ; i++) {
        void *ptr;
        ssize_t result;
        static Py_ssize_t buffer_sizes[] = {128, XATTR_SIZE_MAX, 0};
        Py_ssize_t buffer_size = buffer_sizes[i];
        if (!buffer_size) {
            path_error("getxattr", &path);
            goto exit;
        }
        buffer = PyBytes_FromStringAndSize(NULL, buffer_size);
        if (!buffer)
            goto exit;
        ptr = PyBytes_AS_STRING(buffer);

        Py_BEGIN_ALLOW_THREADS;
        if (path.fd >= 0)
            result = fgetxattr(path.fd, attribute.narrow, ptr, buffer_size);
        else if (follow_symlinks)
            result = getxattr(path.narrow, attribute.narrow, ptr, buffer_size);
        else
            result = lgetxattr(path.narrow, attribute.narrow, ptr, buffer_size);
        Py_END_ALLOW_THREADS;

        if (result < 0) {
            Py_DECREF(buffer);
            buffer = NULL;
            if (errno == ERANGE)
                continue;
            path_error("getxattr", &path);
            goto exit;
        }

        if (result != buffer_size) {
            /* Can only shrink. */
            _PyBytes_Resize(&buffer, result);
        }
        break;
    }

exit:
    path_cleanup(&path);
    path_cleanup(&attribute);
    return buffer;
}

PyDoc_STRVAR(posix_setxattr__doc__,
"setxattr(path, attribute, value, flags=0, *, follow_symlinks=True)\n\n\
Set extended attribute attribute on path to value.\n\
path may be either a string or an open file descriptor.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, setxattr will modify the symbolic link itself instead of the file\n\
  the link points to.");

static PyObject *
posix_setxattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    path_t attribute;
    Py_buffer value;
    int flags = 0;
    int follow_symlinks = 1;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "attribute", "value",
                               "flags", "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
    path.allow_fd = 1;
    memset(&attribute, 0, sizeof(attribute));
    memset(&value, 0, sizeof(value));
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&y*|i$p:setxattr",
        keywords,
        path_converter, &path,
        path_converter, &attribute,
        &value, &flags,
        &follow_symlinks))
        return NULL;

    if (fd_and_follow_symlinks_invalid("setxattr", path.fd, follow_symlinks))
        goto exit;

    Py_BEGIN_ALLOW_THREADS;
    if (path.fd > -1)
        result = fsetxattr(path.fd, attribute.narrow,
                           value.buf, value.len, flags);
    else if (follow_symlinks)
        result = setxattr(path.narrow, attribute.narrow,
                           value.buf, value.len, flags);
    else
        result = lsetxattr(path.narrow, attribute.narrow,
                           value.buf, value.len, flags);
    Py_END_ALLOW_THREADS;

    if (result) {
        return_value = path_error("setxattr", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(return_value);

exit:
    path_cleanup(&path);
    path_cleanup(&attribute);
    PyBuffer_Release(&value);

    return return_value;
}

PyDoc_STRVAR(posix_removexattr__doc__,
"removexattr(path, attribute, *, follow_symlinks=True)\n\n\
Remove extended attribute attribute on path.\n\
path may be either a string or an open file descriptor.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, removexattr will modify the symbolic link itself instead of the file\n\
  the link points to.");

static PyObject *
posix_removexattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    path_t attribute;
    int follow_symlinks = 1;
    int result;
    PyObject *return_value = NULL;
    static char *keywords[] = {"path", "attribute", "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
    memset(&attribute, 0, sizeof(attribute));
    path.allow_fd = 1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&O&|$p:removexattr",
                                     keywords,
                                     path_converter, &path,
                                     path_converter, &attribute,
                                     &follow_symlinks))
        return NULL;

    if (fd_and_follow_symlinks_invalid("removexattr", path.fd, follow_symlinks))
        goto exit;

    Py_BEGIN_ALLOW_THREADS;
    if (path.fd > -1)
        result = fremovexattr(path.fd, attribute.narrow);
    else if (follow_symlinks)
        result = removexattr(path.narrow, attribute.narrow);
    else
        result = lremovexattr(path.narrow, attribute.narrow);
    Py_END_ALLOW_THREADS;

    if (result) {
        return_value = path_error("removexattr", &path);
        goto exit;
    }

    return_value = Py_None;
    Py_INCREF(return_value);

exit:
    path_cleanup(&path);
    path_cleanup(&attribute);

    return return_value;
}

PyDoc_STRVAR(posix_listxattr__doc__,
"listxattr(path='.', *, follow_symlinks=True)\n\n\
Return a list of extended attributes on path.\n\
\n\
path may be either None, a string, or an open file descriptor.\n\
if path is None, listxattr will examine the current directory.\n\
If follow_symlinks is False, and the last element of the path is a symbolic\n\
  link, listxattr will examine the symbolic link itself instead of the file\n\
  the link points to.");

static PyObject *
posix_listxattr(PyObject *self, PyObject *args, PyObject *kwargs)
{
    path_t path;
    int follow_symlinks = 1;
    Py_ssize_t i;
    PyObject *result = NULL;
    char *buffer = NULL;
    char *name;
    static char *keywords[] = {"path", "follow_symlinks", NULL};

    memset(&path, 0, sizeof(path));
    path.allow_fd = 1;
    path.fd = -1;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O&$p:listxattr", keywords,
        path_converter, &path,
        &follow_symlinks))
        return NULL;

    if (fd_and_follow_symlinks_invalid("listxattr", path.fd, follow_symlinks))
        goto exit;

    name = path.narrow ? path.narrow : ".";
    for (i = 0; ; i++) {
        char *start, *trace, *end;
        ssize_t length;
        static Py_ssize_t buffer_sizes[] = { 256, XATTR_LIST_MAX, 0 };
        Py_ssize_t buffer_size = buffer_sizes[i];
        if (!buffer_size) {
            // ERANGE
            path_error("listxattr", &path);
            break;
        }
        buffer = PyMem_MALLOC(buffer_size);
        if (!buffer) {
            PyErr_NoMemory();
            break;
        }

        Py_BEGIN_ALLOW_THREADS;
        if (path.fd > -1)
            length = flistxattr(path.fd, buffer, buffer_size);
        else if (follow_symlinks)
            length = listxattr(name, buffer, buffer_size);
        else
            length = llistxattr(name, buffer, buffer_size);
        Py_END_ALLOW_THREADS;

        if (length < 0) {
            if (errno == ERANGE)
                continue;
            path_error("listxattr", &path);
            break;
        }

        result = PyList_New(0);
        if (!result) {
            goto exit;
        }

        end = buffer + length;
        for (trace = start = buffer; trace != end; trace++) {
            if (!*trace) {
                int error;
                PyObject *attribute = PyUnicode_DecodeFSDefaultAndSize(start,
                                                                 trace - start);
                if (!attribute) {
                    Py_DECREF(result);
                    result = NULL;
                    goto exit;
                }
                error = PyList_Append(result, attribute);
                Py_DECREF(attribute);
                if (error) {
                    Py_DECREF(result);
                    result = NULL;
                    goto exit;
                }
                start = trace + 1;
            }
        }
    break;
    }
exit:
    path_cleanup(&path);
    if (buffer)
        PyMem_FREE(buffer);
    return result;
}

#endif /* USE_XATTRS */


PyDoc_STRVAR(posix_urandom__doc__,
"urandom(n) -> str\n\n\
Return n random bytes suitable for cryptographic use.");

static PyObject *
posix_urandom(PyObject *self, PyObject *args)
{
    Py_ssize_t size;
    PyObject *result;
    int ret;

     /* Read arguments */
    if (!PyArg_ParseTuple(args, "n:urandom", &size))
        return NULL;
    if (size < 0)
        return PyErr_Format(PyExc_ValueError,
                            "negative argument not allowed");
    result = PyBytes_FromStringAndSize(NULL, size);
    if (result == NULL)
        return NULL;

    ret = _PyOS_URandom(PyBytes_AS_STRING(result),
                        PyBytes_GET_SIZE(result));
    if (ret == -1) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

/* Terminal size querying */

static PyTypeObject TerminalSizeType;

PyDoc_STRVAR(TerminalSize_docstring,
    "A tuple of (columns, lines) for holding terminal window size");

static PyStructSequence_Field TerminalSize_fields[] = {
    {"columns", "width of the terminal window in characters"},
    {"lines", "height of the terminal window in characters"},
    {NULL, NULL}
};

static PyStructSequence_Desc TerminalSize_desc = {
    "os.terminal_size",
    TerminalSize_docstring,
    TerminalSize_fields,
    2,
};

#if defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL)
PyDoc_STRVAR(termsize__doc__,
    "Return the size of the terminal window as (columns, lines).\n"        \
    "\n"                                                                   \
    "The optional argument fd (default standard output) specifies\n"       \
    "which file descriptor should be queried.\n"                           \
    "\n"                                                                   \
    "If the file descriptor is not connected to a terminal, an OSError\n"  \
    "is thrown.\n"                                                         \
    "\n"                                                                   \
    "This function will only be defined if an implementation is\n"         \
    "available for this system.\n"                                         \
    "\n"                                                                   \
    "shutil.get_terminal_size is the high-level function which should \n"  \
    "normally be used, os.get_terminal_size is the low-level implementation.");

static PyObject*
get_terminal_size(PyObject *self, PyObject *args)
{
    int columns, lines;
    PyObject *termsize;

    int fd = fileno(stdout);
    /* Under some conditions stdout may not be connected and
     * fileno(stdout) may point to an invalid file descriptor. For example
     * GUI apps don't have valid standard streams by default.
     *
     * If this happens, and the optional fd argument is not present,
     * the ioctl below will fail returning EBADF. This is what we want.
     */

    if (!PyArg_ParseTuple(args, "|i", &fd))
        return NULL;

#ifdef TERMSIZE_USE_IOCTL
    {
        struct winsize w;
        if (ioctl(fd, TIOCGWINSZ, &w))
            return PyErr_SetFromErrno(PyExc_OSError);
        columns = w.ws_col;
        lines = w.ws_row;
    }
#endif /* TERMSIZE_USE_IOCTL */

#ifdef TERMSIZE_USE_CONIO
    {
        DWORD nhandle;
        HANDLE handle;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        switch (fd) {
        case 0: nhandle = STD_INPUT_HANDLE;
            break;
        case 1: nhandle = STD_OUTPUT_HANDLE;
            break;
        case 2: nhandle = STD_ERROR_HANDLE;
            break;
        default:
            return PyErr_Format(PyExc_ValueError, "bad file descriptor");
        }
        handle = GetStdHandle(nhandle);
        if (handle == NULL)
            return PyErr_Format(PyExc_OSError, "handle cannot be retrieved");
        if (handle == INVALID_HANDLE_VALUE)
            return PyErr_SetFromWindowsErr(0);

        if (!GetConsoleScreenBufferInfo(handle, &csbi))
            return PyErr_SetFromWindowsErr(0);

        columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        lines = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#endif /* TERMSIZE_USE_CONIO */

    termsize = PyStructSequence_New(&TerminalSizeType);
    if (termsize == NULL)
        return NULL;
    PyStructSequence_SET_ITEM(termsize, 0, PyLong_FromLong(columns));
    PyStructSequence_SET_ITEM(termsize, 1, PyLong_FromLong(lines));
    if (PyErr_Occurred()) {
        Py_DECREF(termsize);
        return NULL;
    }
    return termsize;
}
#endif /* defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL) */


static PyMethodDef posix_methods[] = {
    {"access",          (PyCFunction)posix_access,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_access__doc__},
#ifdef HAVE_TTYNAME
    {"ttyname",         posix_ttyname, METH_VARARGS, posix_ttyname__doc__},
#endif
    {"chdir",           (PyCFunction)posix_chdir,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_chdir__doc__},
#ifdef HAVE_CHFLAGS
    {"chflags",         (PyCFunction)posix_chflags,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_chflags__doc__},
#endif /* HAVE_CHFLAGS */
    {"chmod",           (PyCFunction)posix_chmod,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_chmod__doc__},
#ifdef HAVE_FCHMOD
    {"fchmod",          posix_fchmod, METH_VARARGS, posix_fchmod__doc__},
#endif /* HAVE_FCHMOD */
#ifdef HAVE_CHOWN
    {"chown",           (PyCFunction)posix_chown,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_chown__doc__},
#endif /* HAVE_CHOWN */
#ifdef HAVE_LCHMOD
    {"lchmod",          posix_lchmod, METH_VARARGS, posix_lchmod__doc__},
#endif /* HAVE_LCHMOD */
#ifdef HAVE_FCHOWN
    {"fchown",          posix_fchown, METH_VARARGS, posix_fchown__doc__},
#endif /* HAVE_FCHOWN */
#ifdef HAVE_LCHFLAGS
    {"lchflags",        posix_lchflags, METH_VARARGS, posix_lchflags__doc__},
#endif /* HAVE_LCHFLAGS */
#ifdef HAVE_LCHOWN
    {"lchown",          posix_lchown, METH_VARARGS, posix_lchown__doc__},
#endif /* HAVE_LCHOWN */
#ifdef HAVE_CHROOT
    {"chroot",          posix_chroot, METH_VARARGS, posix_chroot__doc__},
#endif
#ifdef HAVE_CTERMID
    {"ctermid",         posix_ctermid, METH_NOARGS, posix_ctermid__doc__},
#endif
#ifdef HAVE_GETCWD
    {"getcwd",          (PyCFunction)posix_getcwd_unicode,
    METH_NOARGS, posix_getcwd__doc__},
    {"getcwdb",         (PyCFunction)posix_getcwd_bytes,
    METH_NOARGS, posix_getcwdb__doc__},
#endif
#if defined(HAVE_LINK) || defined(MS_WINDOWS)
    {"link",            (PyCFunction)posix_link,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_link__doc__},
#endif /* HAVE_LINK */
    {"listdir",         (PyCFunction)posix_listdir,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_listdir__doc__},
    {"lstat",           (PyCFunction)posix_lstat,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_lstat__doc__},
    {"mkdir",           (PyCFunction)posix_mkdir,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_mkdir__doc__},
#ifdef HAVE_NICE
    {"nice",            posix_nice, METH_VARARGS, posix_nice__doc__},
#endif /* HAVE_NICE */
#ifdef HAVE_GETPRIORITY
    {"getpriority",     posix_getpriority, METH_VARARGS, posix_getpriority__doc__},
#endif /* HAVE_GETPRIORITY */
#ifdef HAVE_SETPRIORITY
    {"setpriority",     posix_setpriority, METH_VARARGS, posix_setpriority__doc__},
#endif /* HAVE_SETPRIORITY */
#ifdef HAVE_READLINK
    {"readlink",        (PyCFunction)posix_readlink,
                        METH_VARARGS | METH_KEYWORDS,
                        readlink__doc__},
#endif /* HAVE_READLINK */
#if !defined(HAVE_READLINK) && defined(MS_WINDOWS)
    {"readlink",        (PyCFunction)win_readlink,
                        METH_VARARGS | METH_KEYWORDS,
                        readlink__doc__},
#endif /* !defined(HAVE_READLINK) && defined(MS_WINDOWS) */
    {"rename",          (PyCFunction)posix_rename,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_rename__doc__},
    {"replace",         (PyCFunction)posix_replace,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_replace__doc__},
    {"rmdir",           (PyCFunction)posix_rmdir,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_rmdir__doc__},
    {"stat",            (PyCFunction)posix_stat,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_stat__doc__},
    {"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
#if defined(HAVE_SYMLINK)
    {"symlink",         (PyCFunction)posix_symlink,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_symlink__doc__},
#endif /* HAVE_SYMLINK */
#ifdef HAVE_SYSTEM
    {"system",          posix_system, METH_VARARGS, posix_system__doc__},
#endif
    {"umask",           posix_umask, METH_VARARGS, posix_umask__doc__},
#ifdef HAVE_UNAME
    {"uname",           posix_uname, METH_NOARGS, posix_uname__doc__},
#endif /* HAVE_UNAME */
    {"unlink",          (PyCFunction)posix_unlink,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_unlink__doc__},
    {"remove",          (PyCFunction)posix_unlink,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_remove__doc__},
    {"utime",           (PyCFunction)posix_utime,
                        METH_VARARGS | METH_KEYWORDS, posix_utime__doc__},
#ifdef HAVE_TIMES
    {"times",           posix_times, METH_NOARGS, posix_times__doc__},
#endif /* HAVE_TIMES */
    {"_exit",           posix__exit, METH_VARARGS, posix__exit__doc__},
#ifdef HAVE_EXECV
    {"execv",           posix_execv, METH_VARARGS, posix_execv__doc__},
    {"execve",          (PyCFunction)posix_execve,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_execve__doc__},
#endif /* HAVE_EXECV */
#ifdef HAVE_SPAWNV
    {"spawnv",          posix_spawnv, METH_VARARGS, posix_spawnv__doc__},
    {"spawnve",         posix_spawnve, METH_VARARGS, posix_spawnve__doc__},
#if defined(PYOS_OS2)
    {"spawnvp",         posix_spawnvp, METH_VARARGS, posix_spawnvp__doc__},
    {"spawnvpe",        posix_spawnvpe, METH_VARARGS, posix_spawnvpe__doc__},
#endif /* PYOS_OS2 */
#endif /* HAVE_SPAWNV */
#ifdef HAVE_FORK1
    {"fork1",       posix_fork1, METH_NOARGS, posix_fork1__doc__},
#endif /* HAVE_FORK1 */
#ifdef HAVE_FORK
    {"fork",            posix_fork, METH_NOARGS, posix_fork__doc__},
#endif /* HAVE_FORK */
#ifdef HAVE_SCHED_H
#ifdef HAVE_SCHED_GET_PRIORITY_MAX
    {"sched_get_priority_max", posix_sched_get_priority_max, METH_VARARGS, posix_sched_get_priority_max__doc__},
    {"sched_get_priority_min", posix_sched_get_priority_min, METH_VARARGS, posix_sched_get_priority_min__doc__},
#endif
#ifdef HAVE_SCHED_SETPARAM
    {"sched_getparam", posix_sched_getparam, METH_VARARGS, posix_sched_getparam__doc__},
#endif
#ifdef HAVE_SCHED_SETSCHEDULER
    {"sched_getscheduler", posix_sched_getscheduler, METH_VARARGS, posix_sched_getscheduler__doc__},
#endif
#ifdef HAVE_SCHED_RR_GET_INTERVAL
    {"sched_rr_get_interval", posix_sched_rr_get_interval, METH_VARARGS, posix_sched_rr_get_interval__doc__},
#endif
#ifdef HAVE_SCHED_SETPARAM
    {"sched_setparam", posix_sched_setparam, METH_VARARGS, posix_sched_setparam__doc__},
#endif
#ifdef HAVE_SCHED_SETSCHEDULER
    {"sched_setscheduler", posix_sched_setscheduler, METH_VARARGS, posix_sched_setscheduler__doc__},
#endif
    {"sched_yield",     posix_sched_yield, METH_NOARGS, posix_sched_yield__doc__},
#ifdef HAVE_SCHED_SETAFFINITY
    {"sched_setaffinity", posix_sched_setaffinity, METH_VARARGS, posix_sched_setaffinity__doc__},
    {"sched_getaffinity", posix_sched_getaffinity, METH_VARARGS, posix_sched_getaffinity__doc__},
#endif
#endif /* HAVE_SCHED_H */
#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX)
    {"openpty",         posix_openpty, METH_NOARGS, posix_openpty__doc__},
#endif /* HAVE_OPENPTY || HAVE__GETPTY || HAVE_DEV_PTMX */
#ifdef HAVE_FORKPTY
    {"forkpty",         posix_forkpty, METH_NOARGS, posix_forkpty__doc__},
#endif /* HAVE_FORKPTY */
#ifdef HAVE_GETEGID
    {"getegid",         posix_getegid, METH_NOARGS, posix_getegid__doc__},
#endif /* HAVE_GETEGID */
#ifdef HAVE_GETEUID
    {"geteuid",         posix_geteuid, METH_NOARGS, posix_geteuid__doc__},
#endif /* HAVE_GETEUID */
#ifdef HAVE_GETGID
    {"getgid",          posix_getgid, METH_NOARGS, posix_getgid__doc__},
#endif /* HAVE_GETGID */
#ifdef HAVE_GETGROUPLIST
    {"getgrouplist",    posix_getgrouplist, METH_VARARGS, posix_getgrouplist__doc__},
#endif
#ifdef HAVE_GETGROUPS
    {"getgroups",       posix_getgroups, METH_NOARGS, posix_getgroups__doc__},
#endif
    {"getpid",          posix_getpid, METH_NOARGS, posix_getpid__doc__},
#ifdef HAVE_GETPGRP
    {"getpgrp",         posix_getpgrp, METH_NOARGS, posix_getpgrp__doc__},
#endif /* HAVE_GETPGRP */
#ifdef HAVE_GETPPID
    {"getppid",         posix_getppid, METH_NOARGS, posix_getppid__doc__},
#endif /* HAVE_GETPPID */
#ifdef HAVE_GETUID
    {"getuid",          posix_getuid, METH_NOARGS, posix_getuid__doc__},
#endif /* HAVE_GETUID */
#ifdef HAVE_GETLOGIN
    {"getlogin",        posix_getlogin, METH_NOARGS, posix_getlogin__doc__},
#endif
#ifdef HAVE_KILL
    {"kill",            posix_kill, METH_VARARGS, posix_kill__doc__},
#endif /* HAVE_KILL */
#ifdef HAVE_KILLPG
    {"killpg",          posix_killpg, METH_VARARGS, posix_killpg__doc__},
#endif /* HAVE_KILLPG */
#ifdef HAVE_PLOCK
    {"plock",           posix_plock, METH_VARARGS, posix_plock__doc__},
#endif /* HAVE_PLOCK */
#ifdef MS_WINDOWS
    {"startfile",       win32_startfile, METH_VARARGS, win32_startfile__doc__},
    {"kill",    win32_kill, METH_VARARGS, win32_kill__doc__},
#endif
#ifdef HAVE_SETUID
    {"setuid",          posix_setuid, METH_VARARGS, posix_setuid__doc__},
#endif /* HAVE_SETUID */
#ifdef HAVE_SETEUID
    {"seteuid",         posix_seteuid, METH_VARARGS, posix_seteuid__doc__},
#endif /* HAVE_SETEUID */
#ifdef HAVE_SETEGID
    {"setegid",         posix_setegid, METH_VARARGS, posix_setegid__doc__},
#endif /* HAVE_SETEGID */
#ifdef HAVE_SETREUID
    {"setreuid",        posix_setreuid, METH_VARARGS, posix_setreuid__doc__},
#endif /* HAVE_SETREUID */
#ifdef HAVE_SETREGID
    {"setregid",        posix_setregid, METH_VARARGS, posix_setregid__doc__},
#endif /* HAVE_SETREGID */
#ifdef HAVE_SETGID
    {"setgid",          posix_setgid, METH_VARARGS, posix_setgid__doc__},
#endif /* HAVE_SETGID */
#ifdef HAVE_SETGROUPS
    {"setgroups",       posix_setgroups, METH_O, posix_setgroups__doc__},
#endif /* HAVE_SETGROUPS */
#ifdef HAVE_INITGROUPS
    {"initgroups",      posix_initgroups, METH_VARARGS, posix_initgroups__doc__},
#endif /* HAVE_INITGROUPS */
#ifdef HAVE_GETPGID
    {"getpgid",         posix_getpgid, METH_VARARGS, posix_getpgid__doc__},
#endif /* HAVE_GETPGID */
#ifdef HAVE_SETPGRP
    {"setpgrp",         posix_setpgrp, METH_NOARGS, posix_setpgrp__doc__},
#endif /* HAVE_SETPGRP */
#ifdef HAVE_WAIT
    {"wait",            posix_wait, METH_NOARGS, posix_wait__doc__},
#endif /* HAVE_WAIT */
#ifdef HAVE_WAIT3
    {"wait3",           posix_wait3, METH_VARARGS, posix_wait3__doc__},
#endif /* HAVE_WAIT3 */
#ifdef HAVE_WAIT4
    {"wait4",           posix_wait4, METH_VARARGS, posix_wait4__doc__},
#endif /* HAVE_WAIT4 */
#if defined(HAVE_WAITID) && !defined(__APPLE__)
    {"waitid",          posix_waitid, METH_VARARGS, posix_waitid__doc__},
#endif
#if defined(HAVE_WAITPID) || defined(HAVE_CWAIT)
    {"waitpid",         posix_waitpid, METH_VARARGS, posix_waitpid__doc__},
#endif /* HAVE_WAITPID */
#ifdef HAVE_GETSID
    {"getsid",          posix_getsid, METH_VARARGS, posix_getsid__doc__},
#endif /* HAVE_GETSID */
#ifdef HAVE_SETSID
    {"setsid",          posix_setsid, METH_NOARGS, posix_setsid__doc__},
#endif /* HAVE_SETSID */
#ifdef HAVE_SETPGID
    {"setpgid",         posix_setpgid, METH_VARARGS, posix_setpgid__doc__},
#endif /* HAVE_SETPGID */
#ifdef HAVE_TCGETPGRP
    {"tcgetpgrp",       posix_tcgetpgrp, METH_VARARGS, posix_tcgetpgrp__doc__},
#endif /* HAVE_TCGETPGRP */
#ifdef HAVE_TCSETPGRP
    {"tcsetpgrp",       posix_tcsetpgrp, METH_VARARGS, posix_tcsetpgrp__doc__},
#endif /* HAVE_TCSETPGRP */
    {"open",            (PyCFunction)posix_open,\
                        METH_VARARGS | METH_KEYWORDS,
                        posix_open__doc__},
    {"close",           posix_close, METH_VARARGS, posix_close__doc__},
    {"closerange",      posix_closerange, METH_VARARGS, posix_closerange__doc__},
    {"device_encoding", device_encoding, METH_VARARGS, device_encoding__doc__},
    {"dup",             posix_dup, METH_VARARGS, posix_dup__doc__},
    {"dup2",            posix_dup2, METH_VARARGS, posix_dup2__doc__},
#ifdef HAVE_LOCKF
    {"lockf",           posix_lockf, METH_VARARGS, posix_lockf__doc__},
#endif
    {"lseek",           posix_lseek, METH_VARARGS, posix_lseek__doc__},
    {"read",            posix_read, METH_VARARGS, posix_read__doc__},
#ifdef HAVE_READV
    {"readv",           posix_readv, METH_VARARGS, posix_readv__doc__},
#endif
#ifdef HAVE_PREAD
    {"pread",           posix_pread, METH_VARARGS, posix_pread__doc__},
#endif
    {"write",           posix_write, METH_VARARGS, posix_write__doc__},
#ifdef HAVE_WRITEV
    {"writev",          posix_writev, METH_VARARGS, posix_writev__doc__},
#endif
#ifdef HAVE_PWRITE
    {"pwrite",          posix_pwrite, METH_VARARGS, posix_pwrite__doc__},
#endif
#ifdef HAVE_SENDFILE
    {"sendfile",        (PyCFunction)posix_sendfile, METH_VARARGS | METH_KEYWORDS,
                            posix_sendfile__doc__},
#endif
    {"fstat",           posix_fstat, METH_VARARGS, posix_fstat__doc__},
    {"isatty",          posix_isatty, METH_VARARGS, posix_isatty__doc__},
#ifdef HAVE_PIPE
    {"pipe",            posix_pipe, METH_NOARGS, posix_pipe__doc__},
#endif
#ifdef HAVE_PIPE2
    {"pipe2",           posix_pipe2, METH_O, posix_pipe2__doc__},
#endif
#ifdef HAVE_MKFIFO
    {"mkfifo",          (PyCFunction)posix_mkfifo,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_mkfifo__doc__},
#endif
#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
    {"mknod",           (PyCFunction)posix_mknod,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_mknod__doc__},
#endif
#ifdef HAVE_DEVICE_MACROS
    {"major",           posix_major, METH_VARARGS, posix_major__doc__},
    {"minor",           posix_minor, METH_VARARGS, posix_minor__doc__},
    {"makedev",         posix_makedev, METH_VARARGS, posix_makedev__doc__},
#endif
#ifdef HAVE_FTRUNCATE
    {"ftruncate",       posix_ftruncate, METH_VARARGS, posix_ftruncate__doc__},
#endif
#ifdef HAVE_TRUNCATE
    {"truncate",        (PyCFunction)posix_truncate,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_truncate__doc__},
#endif
#ifdef HAVE_POSIX_FALLOCATE
    {"posix_fallocate", posix_posix_fallocate, METH_VARARGS, posix_posix_fallocate__doc__},
#endif
#ifdef HAVE_POSIX_FADVISE
    {"posix_fadvise",   posix_posix_fadvise, METH_VARARGS, posix_posix_fadvise__doc__},
#endif
#ifdef HAVE_PUTENV
    {"putenv",          posix_putenv, METH_VARARGS, posix_putenv__doc__},
#endif
#ifdef HAVE_UNSETENV
    {"unsetenv",        posix_unsetenv, METH_VARARGS, posix_unsetenv__doc__},
#endif
    {"strerror",        posix_strerror, METH_VARARGS, posix_strerror__doc__},
#ifdef HAVE_FCHDIR
    {"fchdir",          posix_fchdir, METH_O, posix_fchdir__doc__},
#endif
#ifdef HAVE_FSYNC
    {"fsync",       posix_fsync, METH_O, posix_fsync__doc__},
#endif
#ifdef HAVE_SYNC
    {"sync",        posix_sync, METH_NOARGS, posix_sync__doc__},
#endif
#ifdef HAVE_FDATASYNC
    {"fdatasync",   posix_fdatasync,  METH_O, posix_fdatasync__doc__},
#endif
#ifdef HAVE_SYS_WAIT_H
#ifdef WCOREDUMP
    {"WCOREDUMP",       posix_WCOREDUMP, METH_VARARGS, posix_WCOREDUMP__doc__},
#endif /* WCOREDUMP */
#ifdef WIFCONTINUED
    {"WIFCONTINUED",posix_WIFCONTINUED, METH_VARARGS, posix_WIFCONTINUED__doc__},
#endif /* WIFCONTINUED */
#ifdef WIFSTOPPED
    {"WIFSTOPPED",      posix_WIFSTOPPED, METH_VARARGS, posix_WIFSTOPPED__doc__},
#endif /* WIFSTOPPED */
#ifdef WIFSIGNALED
    {"WIFSIGNALED",     posix_WIFSIGNALED, METH_VARARGS, posix_WIFSIGNALED__doc__},
#endif /* WIFSIGNALED */
#ifdef WIFEXITED
    {"WIFEXITED",       posix_WIFEXITED, METH_VARARGS, posix_WIFEXITED__doc__},
#endif /* WIFEXITED */
#ifdef WEXITSTATUS
    {"WEXITSTATUS",     posix_WEXITSTATUS, METH_VARARGS, posix_WEXITSTATUS__doc__},
#endif /* WEXITSTATUS */
#ifdef WTERMSIG
    {"WTERMSIG",        posix_WTERMSIG, METH_VARARGS, posix_WTERMSIG__doc__},
#endif /* WTERMSIG */
#ifdef WSTOPSIG
    {"WSTOPSIG",        posix_WSTOPSIG, METH_VARARGS, posix_WSTOPSIG__doc__},
#endif /* WSTOPSIG */
#endif /* HAVE_SYS_WAIT_H */
#if defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H)
    {"fstatvfs",        posix_fstatvfs, METH_VARARGS, posix_fstatvfs__doc__},
#endif
#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
    {"statvfs",         (PyCFunction)posix_statvfs,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_statvfs__doc__},
#endif
#ifdef HAVE_CONFSTR
    {"confstr",         posix_confstr, METH_VARARGS, posix_confstr__doc__},
#endif
#ifdef HAVE_SYSCONF
    {"sysconf",         posix_sysconf, METH_VARARGS, posix_sysconf__doc__},
#endif
#ifdef HAVE_FPATHCONF
    {"fpathconf",       posix_fpathconf, METH_VARARGS, posix_fpathconf__doc__},
#endif
#ifdef HAVE_PATHCONF
    {"pathconf",        (PyCFunction)posix_pathconf,
                        METH_VARARGS | METH_KEYWORDS,
                        posix_pathconf__doc__},
#endif
    {"abort",           posix_abort, METH_NOARGS, posix_abort__doc__},
#ifdef MS_WINDOWS
    {"_getfullpathname",        posix__getfullpathname, METH_VARARGS, NULL},
    {"_getfinalpathname",       posix__getfinalpathname, METH_VARARGS, NULL},
    {"_getfileinformation",     posix__getfileinformation, METH_VARARGS, NULL},
    {"_isdir",                  posix__isdir, METH_VARARGS, posix__isdir__doc__},
    {"_getdiskusage",           win32__getdiskusage, METH_VARARGS, win32__getdiskusage__doc__},
#endif
#ifdef HAVE_GETLOADAVG
    {"getloadavg",      posix_getloadavg, METH_NOARGS, posix_getloadavg__doc__},
#endif
    {"urandom",         posix_urandom,   METH_VARARGS, posix_urandom__doc__},
#ifdef HAVE_SETRESUID
    {"setresuid",       posix_setresuid, METH_VARARGS, posix_setresuid__doc__},
#endif
#ifdef HAVE_SETRESGID
    {"setresgid",       posix_setresgid, METH_VARARGS, posix_setresgid__doc__},
#endif
#ifdef HAVE_GETRESUID
    {"getresuid",       posix_getresuid, METH_NOARGS, posix_getresuid__doc__},
#endif
#ifdef HAVE_GETRESGID
    {"getresgid",       posix_getresgid, METH_NOARGS, posix_getresgid__doc__},
#endif

#ifdef USE_XATTRS
    {"setxattr", (PyCFunction)posix_setxattr,
                 METH_VARARGS | METH_KEYWORDS,
                 posix_setxattr__doc__},
    {"getxattr", (PyCFunction)posix_getxattr,
                 METH_VARARGS | METH_KEYWORDS,
                 posix_getxattr__doc__},
    {"removexattr", (PyCFunction)posix_removexattr,
                 METH_VARARGS | METH_KEYWORDS,
                 posix_removexattr__doc__},
    {"listxattr", (PyCFunction)posix_listxattr,
                 METH_VARARGS | METH_KEYWORDS,
                 posix_listxattr__doc__},
#endif
#if defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL)
    {"get_terminal_size", get_terminal_size, METH_VARARGS, termsize__doc__},
#endif
    {NULL,              NULL}            /* Sentinel */
};


static int
ins(PyObject *module, char *symbol, long value)
{
    return PyModule_AddIntConstant(module, symbol, value);
}

#if defined(PYOS_OS2)
/* Insert Platform-Specific Constant Values (Strings & Numbers) of Common Use */
static int insertvalues(PyObject *module)
{
    APIRET    rc;
    ULONG     values[QSV_MAX+1];
    PyObject *v;
    char     *ver, tmp[50];

    Py_BEGIN_ALLOW_THREADS
    rc = DosQuerySysInfo(1L, QSV_MAX, &values[1], sizeof(ULONG) * QSV_MAX);
    Py_END_ALLOW_THREADS

    if (rc != NO_ERROR) {
        os2_error(rc);
        return -1;
    }

    if (ins(module, "meminstalled", values[QSV_TOTPHYSMEM])) return -1;
    if (ins(module, "memkernel",    values[QSV_TOTRESMEM])) return -1;
    if (ins(module, "memvirtual",   values[QSV_TOTAVAILMEM])) return -1;
    if (ins(module, "maxpathlen",   values[QSV_MAX_PATH_LENGTH])) return -1;
    if (ins(module, "maxnamelen",   values[QSV_MAX_COMP_LENGTH])) return -1;
    if (ins(module, "revision",     values[QSV_VERSION_REVISION])) return -1;
    if (ins(module, "timeslice",    values[QSV_MIN_SLICE])) return -1;

    switch (values[QSV_VERSION_MINOR]) {
    case 0:  ver = "2.00"; break;
    case 10: ver = "2.10"; break;
    case 11: ver = "2.11"; break;
    case 30: ver = "3.00"; break;
    case 40: ver = "4.00"; break;
    case 50: ver = "5.00"; break;
    default:
        PyOS_snprintf(tmp, sizeof(tmp),
                      "%d-%d", values[QSV_VERSION_MAJOR],
                      values[QSV_VERSION_MINOR]);
        ver = &tmp[0];
    }

    /* Add Indicator of the Version of the Operating System */
    if (PyModule_AddStringConstant(module, "version", tmp) < 0)
        return -1;

    /* Add Indicator of Which Drive was Used to Boot the System */
    tmp[0] = 'A' + values[QSV_BOOT_DRIVE] - 1;
    tmp[1] = ':';
    tmp[2] = '\0';

    return PyModule_AddStringConstant(module, "bootdrive", tmp);
}
#endif

#if defined(HAVE_SYMLINK) && defined(MS_WINDOWS)
static int
enable_symlink()
{
    HANDLE tok;
    TOKEN_PRIVILEGES tok_priv;
    LUID luid;
    int meth_idx = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &tok))
        return 0;

    if (!LookupPrivilegeValue(NULL, SE_CREATE_SYMBOLIC_LINK_NAME, &luid))
        return 0;

    tok_priv.PrivilegeCount = 1;
    tok_priv.Privileges[0].Luid = luid;
    tok_priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(tok, FALSE, &tok_priv,
                               sizeof(TOKEN_PRIVILEGES),
                               (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL))
        return 0;

    /* ERROR_NOT_ALL_ASSIGNED returned when the privilege can't be assigned. */
    return GetLastError() == ERROR_NOT_ALL_ASSIGNED ? 0 : 1;
}
#endif /* defined(HAVE_SYMLINK) && defined(MS_WINDOWS) */

static int
all_ins(PyObject *d)
{
#ifdef F_OK
    if (ins(d, "F_OK", (long)F_OK)) return -1;
#endif
#ifdef R_OK
    if (ins(d, "R_OK", (long)R_OK)) return -1;
#endif
#ifdef W_OK
    if (ins(d, "W_OK", (long)W_OK)) return -1;
#endif
#ifdef X_OK
    if (ins(d, "X_OK", (long)X_OK)) return -1;
#endif
#ifdef NGROUPS_MAX
    if (ins(d, "NGROUPS_MAX", (long)NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
    if (ins(d, "TMP_MAX", (long)TMP_MAX)) return -1;
#endif
#ifdef WCONTINUED
    if (ins(d, "WCONTINUED", (long)WCONTINUED)) return -1;
#endif
#ifdef WNOHANG
    if (ins(d, "WNOHANG", (long)WNOHANG)) return -1;
#endif
#ifdef WUNTRACED
    if (ins(d, "WUNTRACED", (long)WUNTRACED)) return -1;
#endif
#ifdef O_RDONLY
    if (ins(d, "O_RDONLY", (long)O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
    if (ins(d, "O_WRONLY", (long)O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
    if (ins(d, "O_RDWR", (long)O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
    if (ins(d, "O_NDELAY", (long)O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
    if (ins(d, "O_NONBLOCK", (long)O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
    if (ins(d, "O_APPEND", (long)O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
    if (ins(d, "O_DSYNC", (long)O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
    if (ins(d, "O_RSYNC", (long)O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
    if (ins(d, "O_SYNC", (long)O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
    if (ins(d, "O_NOCTTY", (long)O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
    if (ins(d, "O_CREAT", (long)O_CREAT)) return -1;
#endif
#ifdef O_EXCL
    if (ins(d, "O_EXCL", (long)O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
    if (ins(d, "O_TRUNC", (long)O_TRUNC)) return -1;
#endif
#ifdef O_BINARY
    if (ins(d, "O_BINARY", (long)O_BINARY)) return -1;
#endif
#ifdef O_TEXT
    if (ins(d, "O_TEXT", (long)O_TEXT)) return -1;
#endif
#ifdef O_XATTR
    if (ins(d, "O_XATTR", (long)O_XATTR)) return -1;
#endif
#ifdef O_LARGEFILE
    if (ins(d, "O_LARGEFILE", (long)O_LARGEFILE)) return -1;
#endif
#ifdef O_SHLOCK
    if (ins(d, "O_SHLOCK", (long)O_SHLOCK)) return -1;
#endif
#ifdef O_EXLOCK
    if (ins(d, "O_EXLOCK", (long)O_EXLOCK)) return -1;
#endif
#ifdef O_EXEC
    if (ins(d, "O_EXEC", (long)O_EXEC)) return -1;
#endif
#ifdef O_SEARCH
    if (ins(d, "O_SEARCH", (long)O_SEARCH)) return -1;
#endif
#ifdef O_TTY_INIT
    if (ins(d, "O_TTY_INIT", (long)O_TTY_INIT)) return -1;
#endif
#ifdef PRIO_PROCESS
    if (ins(d, "PRIO_PROCESS", (long)PRIO_PROCESS)) return -1;
#endif
#ifdef PRIO_PGRP
    if (ins(d, "PRIO_PGRP", (long)PRIO_PGRP)) return -1;
#endif
#ifdef PRIO_USER
    if (ins(d, "PRIO_USER", (long)PRIO_USER)) return -1;
#endif
#ifdef O_CLOEXEC
    if (ins(d, "O_CLOEXEC", (long)O_CLOEXEC)) return -1;
#endif
#ifdef O_ACCMODE
    if (ins(d, "O_ACCMODE", (long)O_ACCMODE)) return -1;
#endif


#ifdef SEEK_HOLE
    if (ins(d, "SEEK_HOLE", (long)SEEK_HOLE)) return -1;
#endif
#ifdef SEEK_DATA
    if (ins(d, "SEEK_DATA", (long)SEEK_DATA)) return -1;
#endif

/* MS Windows */
#ifdef O_NOINHERIT
    /* Don't inherit in child processes. */
    if (ins(d, "O_NOINHERIT", (long)O_NOINHERIT)) return -1;
#endif
#ifdef _O_SHORT_LIVED
    /* Optimize for short life (keep in memory). */
    /* MS forgot to define this one with a non-underscore form too. */
    if (ins(d, "O_SHORT_LIVED", (long)_O_SHORT_LIVED)) return -1;
#endif
#ifdef O_TEMPORARY
    /* Automatically delete when last handle is closed. */
    if (ins(d, "O_TEMPORARY", (long)O_TEMPORARY)) return -1;
#endif
#ifdef O_RANDOM
    /* Optimize for random access. */
    if (ins(d, "O_RANDOM", (long)O_RANDOM)) return -1;
#endif
#ifdef O_SEQUENTIAL
    /* Optimize for sequential access. */
    if (ins(d, "O_SEQUENTIAL", (long)O_SEQUENTIAL)) return -1;
#endif

/* GNU extensions. */
#ifdef O_ASYNC
    /* Send a SIGIO signal whenever input or output
       becomes available on file descriptor */
    if (ins(d, "O_ASYNC", (long)O_ASYNC)) return -1;
#endif
#ifdef O_DIRECT
    /* Direct disk access. */
    if (ins(d, "O_DIRECT", (long)O_DIRECT)) return -1;
#endif
#ifdef O_DIRECTORY
    /* Must be a directory.      */
    if (ins(d, "O_DIRECTORY", (long)O_DIRECTORY)) return -1;
#endif
#ifdef O_NOFOLLOW
    /* Do not follow links.      */
    if (ins(d, "O_NOFOLLOW", (long)O_NOFOLLOW)) return -1;
#endif
#ifdef O_NOLINKS
    /* Fails if link count of the named file is greater than 1 */
    if (ins(d, "O_NOLINKS", (long)O_NOLINKS)) return -1;
#endif
#ifdef O_NOATIME
    /* Do not update the access time. */
    if (ins(d, "O_NOATIME", (long)O_NOATIME)) return -1;
#endif

    /* These come from sysexits.h */
#ifdef EX_OK
    if (ins(d, "EX_OK", (long)EX_OK)) return -1;
#endif /* EX_OK */
#ifdef EX_USAGE
    if (ins(d, "EX_USAGE", (long)EX_USAGE)) return -1;
#endif /* EX_USAGE */
#ifdef EX_DATAERR
    if (ins(d, "EX_DATAERR", (long)EX_DATAERR)) return -1;
#endif /* EX_DATAERR */
#ifdef EX_NOINPUT
    if (ins(d, "EX_NOINPUT", (long)EX_NOINPUT)) return -1;
#endif /* EX_NOINPUT */
#ifdef EX_NOUSER
    if (ins(d, "EX_NOUSER", (long)EX_NOUSER)) return -1;
#endif /* EX_NOUSER */
#ifdef EX_NOHOST
    if (ins(d, "EX_NOHOST", (long)EX_NOHOST)) return -1;
#endif /* EX_NOHOST */
#ifdef EX_UNAVAILABLE
    if (ins(d, "EX_UNAVAILABLE", (long)EX_UNAVAILABLE)) return -1;
#endif /* EX_UNAVAILABLE */
#ifdef EX_SOFTWARE
    if (ins(d, "EX_SOFTWARE", (long)EX_SOFTWARE)) return -1;
#endif /* EX_SOFTWARE */
#ifdef EX_OSERR
    if (ins(d, "EX_OSERR", (long)EX_OSERR)) return -1;
#endif /* EX_OSERR */
#ifdef EX_OSFILE
    if (ins(d, "EX_OSFILE", (long)EX_OSFILE)) return -1;
#endif /* EX_OSFILE */
#ifdef EX_CANTCREAT
    if (ins(d, "EX_CANTCREAT", (long)EX_CANTCREAT)) return -1;
#endif /* EX_CANTCREAT */
#ifdef EX_IOERR
    if (ins(d, "EX_IOERR", (long)EX_IOERR)) return -1;
#endif /* EX_IOERR */
#ifdef EX_TEMPFAIL
    if (ins(d, "EX_TEMPFAIL", (long)EX_TEMPFAIL)) return -1;
#endif /* EX_TEMPFAIL */
#ifdef EX_PROTOCOL
    if (ins(d, "EX_PROTOCOL", (long)EX_PROTOCOL)) return -1;
#endif /* EX_PROTOCOL */
#ifdef EX_NOPERM
    if (ins(d, "EX_NOPERM", (long)EX_NOPERM)) return -1;
#endif /* EX_NOPERM */
#ifdef EX_CONFIG
    if (ins(d, "EX_CONFIG", (long)EX_CONFIG)) return -1;
#endif /* EX_CONFIG */
#ifdef EX_NOTFOUND
    if (ins(d, "EX_NOTFOUND", (long)EX_NOTFOUND)) return -1;
#endif /* EX_NOTFOUND */

    /* statvfs */
#ifdef ST_RDONLY
    if (ins(d, "ST_RDONLY", (long)ST_RDONLY)) return -1;
#endif /* ST_RDONLY */
#ifdef ST_NOSUID
    if (ins(d, "ST_NOSUID", (long)ST_NOSUID)) return -1;
#endif /* ST_NOSUID */

    /* FreeBSD sendfile() constants */
#ifdef SF_NODISKIO
    if (ins(d, "SF_NODISKIO", (long)SF_NODISKIO)) return -1;
#endif
#ifdef SF_MNOWAIT
    if (ins(d, "SF_MNOWAIT", (long)SF_MNOWAIT)) return -1;
#endif
#ifdef SF_SYNC
    if (ins(d, "SF_SYNC", (long)SF_SYNC)) return -1;
#endif

    /* constants for posix_fadvise */
#ifdef POSIX_FADV_NORMAL
    if (ins(d, "POSIX_FADV_NORMAL", (long)POSIX_FADV_NORMAL)) return -1;
#endif
#ifdef POSIX_FADV_SEQUENTIAL
    if (ins(d, "POSIX_FADV_SEQUENTIAL", (long)POSIX_FADV_SEQUENTIAL)) return -1;
#endif
#ifdef POSIX_FADV_RANDOM
    if (ins(d, "POSIX_FADV_RANDOM", (long)POSIX_FADV_RANDOM)) return -1;
#endif
#ifdef POSIX_FADV_NOREUSE
    if (ins(d, "POSIX_FADV_NOREUSE", (long)POSIX_FADV_NOREUSE)) return -1;
#endif
#ifdef POSIX_FADV_WILLNEED
    if (ins(d, "POSIX_FADV_WILLNEED", (long)POSIX_FADV_WILLNEED)) return -1;
#endif
#ifdef POSIX_FADV_DONTNEED
    if (ins(d, "POSIX_FADV_DONTNEED", (long)POSIX_FADV_DONTNEED)) return -1;
#endif

    /* constants for waitid */
#if defined(HAVE_SYS_WAIT_H) && defined(HAVE_WAITID)
    if (ins(d, "P_PID", (long)P_PID)) return -1;
    if (ins(d, "P_PGID", (long)P_PGID)) return -1;
    if (ins(d, "P_ALL", (long)P_ALL)) return -1;
#endif
#ifdef WEXITED
    if (ins(d, "WEXITED", (long)WEXITED)) return -1;
#endif
#ifdef WNOWAIT
    if (ins(d, "WNOWAIT", (long)WNOWAIT)) return -1;
#endif
#ifdef WSTOPPED
    if (ins(d, "WSTOPPED", (long)WSTOPPED)) return -1;
#endif
#ifdef CLD_EXITED
    if (ins(d, "CLD_EXITED", (long)CLD_EXITED)) return -1;
#endif
#ifdef CLD_DUMPED
    if (ins(d, "CLD_DUMPED", (long)CLD_DUMPED)) return -1;
#endif
#ifdef CLD_TRAPPED
    if (ins(d, "CLD_TRAPPED", (long)CLD_TRAPPED)) return -1;
#endif
#ifdef CLD_CONTINUED
    if (ins(d, "CLD_CONTINUED", (long)CLD_CONTINUED)) return -1;
#endif

    /* constants for lockf */
#ifdef F_LOCK
    if (ins(d, "F_LOCK", (long)F_LOCK)) return -1;
#endif
#ifdef F_TLOCK
    if (ins(d, "F_TLOCK", (long)F_TLOCK)) return -1;
#endif
#ifdef F_ULOCK
    if (ins(d, "F_ULOCK", (long)F_ULOCK)) return -1;
#endif
#ifdef F_TEST
    if (ins(d, "F_TEST", (long)F_TEST)) return -1;
#endif

#ifdef HAVE_SPAWNV
#if defined(PYOS_OS2) && defined(PYCC_GCC)
    if (ins(d, "P_WAIT", (long)P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)P_OVERLAY)) return -1;
    if (ins(d, "P_DEBUG", (long)P_DEBUG)) return -1;
    if (ins(d, "P_SESSION", (long)P_SESSION)) return -1;
    if (ins(d, "P_DETACH", (long)P_DETACH)) return -1;
    if (ins(d, "P_PM", (long)P_PM)) return -1;
    if (ins(d, "P_DEFAULT", (long)P_DEFAULT)) return -1;
    if (ins(d, "P_MINIMIZE", (long)P_MINIMIZE)) return -1;
    if (ins(d, "P_MAXIMIZE", (long)P_MAXIMIZE)) return -1;
    if (ins(d, "P_FULLSCREEN", (long)P_FULLSCREEN)) return -1;
    if (ins(d, "P_WINDOWED", (long)P_WINDOWED)) return -1;
    if (ins(d, "P_FOREGROUND", (long)P_FOREGROUND)) return -1;
    if (ins(d, "P_BACKGROUND", (long)P_BACKGROUND)) return -1;
    if (ins(d, "P_NOCLOSE", (long)P_NOCLOSE)) return -1;
    if (ins(d, "P_NOSESSION", (long)P_NOSESSION)) return -1;
    if (ins(d, "P_QUOTE", (long)P_QUOTE)) return -1;
    if (ins(d, "P_TILDE", (long)P_TILDE)) return -1;
    if (ins(d, "P_UNRELATED", (long)P_UNRELATED)) return -1;
    if (ins(d, "P_DEBUGDESC", (long)P_DEBUGDESC)) return -1;
#else
    if (ins(d, "P_WAIT", (long)_P_WAIT)) return -1;
    if (ins(d, "P_NOWAIT", (long)_P_NOWAIT)) return -1;
    if (ins(d, "P_OVERLAY", (long)_OLD_P_OVERLAY)) return -1;
    if (ins(d, "P_NOWAITO", (long)_P_NOWAITO)) return -1;
    if (ins(d, "P_DETACH", (long)_P_DETACH)) return -1;
#endif
#endif

#ifdef HAVE_SCHED_H
    if (ins(d, "SCHED_OTHER", (long)SCHED_OTHER)) return -1;
    if (ins(d, "SCHED_FIFO", (long)SCHED_FIFO)) return -1;
    if (ins(d, "SCHED_RR", (long)SCHED_RR)) return -1;
#ifdef SCHED_SPORADIC
    if (ins(d, "SCHED_SPORADIC", (long)SCHED_SPORADIC) return -1;
#endif
#ifdef SCHED_BATCH
    if (ins(d, "SCHED_BATCH", (long)SCHED_BATCH)) return -1;
#endif
#ifdef SCHED_IDLE
    if (ins(d, "SCHED_IDLE", (long)SCHED_IDLE)) return -1;
#endif
#ifdef SCHED_RESET_ON_FORK
    if (ins(d, "SCHED_RESET_ON_FORK", (long)SCHED_RESET_ON_FORK)) return -1;
#endif
#ifdef SCHED_SYS
    if (ins(d, "SCHED_SYS", (long)SCHED_SYS)) return -1;
#endif
#ifdef SCHED_IA
    if (ins(d, "SCHED_IA", (long)SCHED_IA)) return -1;
#endif
#ifdef SCHED_FSS
    if (ins(d, "SCHED_FSS", (long)SCHED_FSS)) return -1;
#endif
#ifdef SCHED_FX
    if (ins(d, "SCHED_FX", (long)SCHED_FSS)) return -1;
#endif
#endif

#ifdef USE_XATTRS
    if (ins(d, "XATTR_CREATE", (long)XATTR_CREATE)) return -1;
    if (ins(d, "XATTR_REPLACE", (long)XATTR_REPLACE)) return -1;
    if (ins(d, "XATTR_SIZE_MAX", (long)XATTR_SIZE_MAX)) return -1;
#endif

#ifdef RTLD_LAZY
    if (PyModule_AddIntMacro(d, RTLD_LAZY)) return -1;
#endif
#ifdef RTLD_NOW
    if (PyModule_AddIntMacro(d, RTLD_NOW)) return -1;
#endif
#ifdef RTLD_GLOBAL
    if (PyModule_AddIntMacro(d, RTLD_GLOBAL)) return -1;
#endif
#ifdef RTLD_LOCAL
    if (PyModule_AddIntMacro(d, RTLD_LOCAL)) return -1;
#endif
#ifdef RTLD_NODELETE
    if (PyModule_AddIntMacro(d, RTLD_NODELETE)) return -1;
#endif
#ifdef RTLD_NOLOAD
    if (PyModule_AddIntMacro(d, RTLD_NOLOAD)) return -1;
#endif
#ifdef RTLD_DEEPBIND
    if (PyModule_AddIntMacro(d, RTLD_DEEPBIND)) return -1;
#endif

#if defined(PYOS_OS2)
    if (insertvalues(d)) return -1;
#endif
    return 0;
}


#if (defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__)) && !defined(__QNX__)
#define INITFUNC PyInit_nt
#define MODNAME "nt"

#elif defined(PYOS_OS2)
#define INITFUNC PyInit_os2
#define MODNAME "os2"

#else
#define INITFUNC PyInit_posix
#define MODNAME "posix"
#endif

static struct PyModuleDef posixmodule = {
    PyModuleDef_HEAD_INIT,
    MODNAME,
    posix__doc__,
    -1,
    posix_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


static char *have_functions[] = {

#ifdef HAVE_FACCESSAT
    "HAVE_FACCESSAT",
#endif

#ifdef HAVE_FCHDIR
    "HAVE_FCHDIR",
#endif

#ifdef HAVE_FCHMOD
    "HAVE_FCHMOD",
#endif

#ifdef HAVE_FCHMODAT
    "HAVE_FCHMODAT",
#endif

#ifdef HAVE_FCHOWN
    "HAVE_FCHOWN",
#endif

#ifdef HAVE_FEXECVE
    "HAVE_FEXECVE",
#endif

#ifdef HAVE_FDOPENDIR
    "HAVE_FDOPENDIR",
#endif

#ifdef HAVE_FPATHCONF
    "HAVE_FPATHCONF",
#endif

#ifdef HAVE_FSTATAT
    "HAVE_FSTATAT",
#endif

#ifdef HAVE_FSTATVFS
    "HAVE_FSTATVFS",
#endif

#ifdef HAVE_FTRUNCATE
    "HAVE_FTRUNCATE",
#endif

#ifdef HAVE_FUTIMENS
    "HAVE_FUTIMENS",
#endif

#ifdef HAVE_FUTIMES
    "HAVE_FUTIMES",
#endif

#ifdef HAVE_FUTIMESAT
    "HAVE_FUTIMESAT",
#endif

#ifdef HAVE_LINKAT
    "HAVE_LINKAT",
#endif

#ifdef HAVE_LCHFLAGS
    "HAVE_LCHFLAGS",
#endif

#ifdef HAVE_LCHMOD
    "HAVE_LCHMOD",
#endif

#ifdef HAVE_LCHOWN
    "HAVE_LCHOWN",
#endif

#ifdef HAVE_LSTAT
    "HAVE_LSTAT",
#endif

#ifdef HAVE_LUTIMES
    "HAVE_LUTIMES",
#endif

#ifdef HAVE_MKDIRAT
    "HAVE_MKDIRAT",
#endif

#ifdef HAVE_MKFIFOAT
    "HAVE_MKFIFOAT",
#endif

#ifdef HAVE_MKNODAT
    "HAVE_MKNODAT",
#endif

#ifdef HAVE_OPENAT
    "HAVE_OPENAT",
#endif

#ifdef HAVE_READLINKAT
    "HAVE_READLINKAT",
#endif

#ifdef HAVE_RENAMEAT
    "HAVE_RENAMEAT",
#endif

#ifdef HAVE_SYMLINKAT
    "HAVE_SYMLINKAT",
#endif

#ifdef HAVE_UNLINKAT
    "HAVE_UNLINKAT",
#endif

#ifdef HAVE_UTIMENSAT
    "HAVE_UTIMENSAT",
#endif

#ifdef MS_WINDOWS
    "MS_WINDOWS",
#endif

    NULL
};


PyMODINIT_FUNC
INITFUNC(void)
{
    PyObject *m, *v;
    PyObject *list;
    char **trace;

#if defined(HAVE_SYMLINK) && defined(MS_WINDOWS)
    win32_can_symlink = enable_symlink();
#endif

    m = PyModule_Create(&posixmodule);
    if (m == NULL)
        return NULL;

    /* Initialize environ dictionary */
    v = convertenviron();
    Py_XINCREF(v);
    if (v == NULL || PyModule_AddObject(m, "environ", v) != 0)
        return NULL;
    Py_DECREF(v);

    if (all_ins(m))
        return NULL;

    if (setup_confname_tables(m))
        return NULL;

    Py_INCREF(PyExc_OSError);
    PyModule_AddObject(m, "error", PyExc_OSError);

#ifdef HAVE_SCHED_SETAFFINITY
    if (PyType_Ready(&cpu_set_type) < 0)
        return NULL;
    Py_INCREF(&cpu_set_type);
    PyModule_AddObject(m, "cpu_set", (PyObject *)&cpu_set_type);
#endif

#ifdef HAVE_PUTENV
    if (posix_putenv_garbage == NULL)
        posix_putenv_garbage = PyDict_New();
#endif

    if (!initialized) {
#if defined(HAVE_WAITID) && !defined(__APPLE__)
        waitid_result_desc.name = MODNAME ".waitid_result";
        PyStructSequence_InitType(&WaitidResultType, &waitid_result_desc);
#endif

        stat_result_desc.name = MODNAME ".stat_result";
        stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
        PyStructSequence_InitType(&StatResultType, &stat_result_desc);
        structseq_new = StatResultType.tp_new;
        StatResultType.tp_new = statresult_new;

        statvfs_result_desc.name = MODNAME ".statvfs_result";
        PyStructSequence_InitType(&StatVFSResultType, &statvfs_result_desc);
#ifdef NEED_TICKS_PER_SECOND
#  if defined(HAVE_SYSCONF) && defined(_SC_CLK_TCK)
        ticks_per_second = sysconf(_SC_CLK_TCK);
#  elif defined(HZ)
        ticks_per_second = HZ;
#  else
        ticks_per_second = 60; /* magic fallback value; may be bogus */
#  endif
#endif

#if defined(HAVE_SCHED_SETPARAM) || defined(HAVE_SCHED_SETSCHEDULER)
        sched_param_desc.name = MODNAME ".sched_param";
        PyStructSequence_InitType(&SchedParamType, &sched_param_desc);
        SchedParamType.tp_new = sched_param_new;
#endif

        /* initialize TerminalSize_info */
        PyStructSequence_InitType(&TerminalSizeType, &TerminalSize_desc);
        Py_INCREF(&TerminalSizeType);
    }
#if defined(HAVE_WAITID) && !defined(__APPLE__)
    Py_INCREF((PyObject*) &WaitidResultType);
    PyModule_AddObject(m, "waitid_result", (PyObject*) &WaitidResultType);
#endif
    Py_INCREF((PyObject*) &StatResultType);
    PyModule_AddObject(m, "stat_result", (PyObject*) &StatResultType);
    Py_INCREF((PyObject*) &StatVFSResultType);
    PyModule_AddObject(m, "statvfs_result",
                       (PyObject*) &StatVFSResultType);

#if defined(HAVE_SCHED_SETPARAM) || defined(HAVE_SCHED_SETSCHEDULER)
    Py_INCREF(&SchedParamType);
    PyModule_AddObject(m, "sched_param", (PyObject *)&SchedParamType);
#endif

#ifdef __APPLE__
    /*
     * Step 2 of weak-linking support on Mac OS X.
     *
     * The code below removes functions that are not available on the
     * currently active platform.
     *
     * This block allow one to use a python binary that was build on
     * OSX 10.4 on OSX 10.3, without losing access to new APIs on
     * OSX 10.4.
     */
#ifdef HAVE_FSTATVFS
    if (fstatvfs == NULL) {
        if (PyObject_DelAttrString(m, "fstatvfs") == -1) {
            return NULL;
        }
    }
#endif /* HAVE_FSTATVFS */

#ifdef HAVE_STATVFS
    if (statvfs == NULL) {
        if (PyObject_DelAttrString(m, "statvfs") == -1) {
            return NULL;
        }
    }
#endif /* HAVE_STATVFS */

# ifdef HAVE_LCHOWN
    if (lchown == NULL) {
        if (PyObject_DelAttrString(m, "lchown") == -1) {
            return NULL;
        }
    }
#endif /* HAVE_LCHOWN */


#endif /* __APPLE__ */

    PyModule_AddObject(m, "terminal_size", (PyObject*) &TerminalSizeType);

    billion = PyLong_FromLong(1000000000);
    if (!billion)
        return NULL;

    /* suppress "function not used" warnings */
    {
    int ignored;
    fd_specified("", -1);
    follow_symlinks_specified("", 1);
    dir_fd_and_follow_symlinks_invalid("chmod", DEFAULT_DIR_FD, 1);
    dir_fd_converter(Py_None, &ignored);
    dir_fd_unavailable(Py_None, &ignored);
    }

    /*
     * provide list of locally available functions
     * so os.py can populate support_* lists
     */
    list = PyList_New(0);
    if (!list)
        return NULL;
    for (trace = have_functions; *trace; trace++) {
        PyObject *unicode = PyUnicode_DecodeASCII(*trace, strlen(*trace), NULL);
        if (!unicode)
            return NULL;
        if (PyList_Append(list, unicode))
            return NULL;
        Py_DECREF(unicode);
    }
    PyModule_AddObject(m, "_have_functions", list);

    initialized = 1;

    return m;

}

#ifdef __cplusplus
}
#endif
