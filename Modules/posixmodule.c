
/* POSIX module implementation */

/* This file is also used for Windows NT/MS-Win.  In that case the
   module actually calls itself 'nt', not 'posix', and a few
   functions are either unimplemented or implemented differently.  The source
   assumes that for Windows NT, the macro 'MS_WINDOWS' is defined independent
   of the compiler used.  Different compilers define their own feature
   test macro, e.g. '_MSC_VER'. */



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
#ifndef MS_WINDOWS
#include "posixmodule.h"
#else
#include "winreparse.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

PyDoc_STRVAR(posix__doc__,
"This module provides access to operating system functionality that is\n\
standardized by the C Standard and the POSIX standard (a thinly\n\
disguised Unix interface).  Refer to the library manual and\n\
corresponding Unix manual entries for more information on calls.");


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

#if defined(HAVE_SYS_XATTR_H) && defined(__GLIBC__) && !defined(__FreeBSD_kernel__) && !defined(__GNU__)
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

#ifdef __hpux
#include <sys/mpctl.h>
#endif

#if defined(__DragonFly__) || \
    defined(__OpenBSD__)   || \
    defined(__FreeBSD__)   || \
    defined(__NetBSD__)    || \
    defined(__APPLE__)
#include <sys/sysctl.h>
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
#if defined(__WATCOMC__) && !defined(__QNX__)           /* Watcom compiler */
#define HAVE_OPENDIR    1
#define HAVE_SYSTEM     1
#include <process.h>
#else
#ifdef _MSC_VER         /* Microsoft compiler */
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
/* Unix functions that the configure script doesn't check for */
#define HAVE_EXECV      1
#define HAVE_FORK       1
#if defined(__USLC__) && defined(__SCO_VERSION__)       /* SCO UDK Compiler */
#define HAVE_FORK1      1
#endif
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
#endif  /* _MSC_VER */
#endif  /* ! __WATCOMC__ || __QNX__ */


/*[clinic input]
# one of the few times we lie about this name!
module os
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=94a0f0f978acae17]*/

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
extern int chmod(const char *, mode_t);
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
#ifndef IO_REPARSE_TAG_MOUNT_POINT
#define IO_REPARSE_TAG_MOUNT_POINT (0xA0000003L)
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
#ifdef MS_WINDOWS
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

#define DWORD_MAX 4294967295U


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


#ifndef MS_WINDOWS
PyObject *
_PyLong_FromUid(uid_t uid)
{
    if (uid == (uid_t)-1)
        return PyLong_FromLong(-1);
    return PyLong_FromUnsignedLong(uid);
}

PyObject *
_PyLong_FromGid(gid_t gid)
{
    if (gid == (gid_t)-1)
        return PyLong_FromLong(-1);
    return PyLong_FromUnsignedLong(gid);
}

int
_Py_Uid_Converter(PyObject *obj, void *p)
{
    uid_t uid;
    PyObject *index;
    int overflow;
    long result;
    unsigned long uresult;

    index = PyNumber_Index(obj);
    if (index == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "uid should be integer, not %.200s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }

    /*
     * Handling uid_t is complicated for two reasons:
     *  * Although uid_t is (always?) unsigned, it still
     *    accepts -1.
     *  * We don't know its size in advance--it may be
     *    bigger than an int, or it may be smaller than
     *    a long.
     *
     * So a bit of defensive programming is in order.
     * Start with interpreting the value passed
     * in as a signed long and see if it works.
     */

    result = PyLong_AsLongAndOverflow(index, &overflow);

    if (!overflow) {
        uid = (uid_t)result;

        if (result == -1) {
            if (PyErr_Occurred())
                goto fail;
            /* It's a legitimate -1, we're done. */
            goto success;
        }

        /* Any other negative number is disallowed. */
        if (result < 0)
            goto underflow;

        /* Ensure the value wasn't truncated. */
        if (sizeof(uid_t) < sizeof(long) &&
            (long)uid != result)
            goto underflow;
        goto success;
    }

    if (overflow < 0)
        goto underflow;

    /*
     * Okay, the value overflowed a signed long.  If it
     * fits in an *unsigned* long, it may still be okay,
     * as uid_t may be unsigned long on this platform.
     */
    uresult = PyLong_AsUnsignedLong(index);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            goto overflow;
        goto fail;
    }

    uid = (uid_t)uresult;

    /*
     * If uid == (uid_t)-1, the user actually passed in ULONG_MAX,
     * but this value would get interpreted as (uid_t)-1  by chown
     * and its siblings.   That's not what the user meant!  So we
     * throw an overflow exception instead.   (We already
     * handled a real -1 with PyLong_AsLongAndOverflow() above.)
     */
    if (uid == (uid_t)-1)
        goto overflow;

    /* Ensure the value wasn't truncated. */
    if (sizeof(uid_t) < sizeof(long) &&
        (unsigned long)uid != uresult)
        goto overflow;
    /* fallthrough */

success:
    Py_DECREF(index);
    *(uid_t *)p = uid;
    return 1;

underflow:
    PyErr_SetString(PyExc_OverflowError,
                    "uid is less than minimum");
    goto fail;

overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "uid is greater than maximum");
    /* fallthrough */

fail:
    Py_DECREF(index);
    return 0;
}

int
_Py_Gid_Converter(PyObject *obj, void *p)
{
    gid_t gid;
    PyObject *index;
    int overflow;
    long result;
    unsigned long uresult;

    index = PyNumber_Index(obj);
    if (index == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "gid should be integer, not %.200s",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }

    /*
     * Handling gid_t is complicated for two reasons:
     *  * Although gid_t is (always?) unsigned, it still
     *    accepts -1.
     *  * We don't know its size in advance--it may be
     *    bigger than an int, or it may be smaller than
     *    a long.
     *
     * So a bit of defensive programming is in order.
     * Start with interpreting the value passed
     * in as a signed long and see if it works.
     */

    result = PyLong_AsLongAndOverflow(index, &overflow);

    if (!overflow) {
        gid = (gid_t)result;

        if (result == -1) {
            if (PyErr_Occurred())
                goto fail;
            /* It's a legitimate -1, we're done. */
            goto success;
        }

        /* Any other negative number is disallowed. */
        if (result < 0) {
            goto underflow;
        }

        /* Ensure the value wasn't truncated. */
        if (sizeof(gid_t) < sizeof(long) &&
            (long)gid != result)
            goto underflow;
        goto success;
    }

    if (overflow < 0)
        goto underflow;

    /*
     * Okay, the value overflowed a signed long.  If it
     * fits in an *unsigned* long, it may still be okay,
     * as gid_t may be unsigned long on this platform.
     */
    uresult = PyLong_AsUnsignedLong(index);
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            goto overflow;
        goto fail;
    }

    gid = (gid_t)uresult;

    /*
     * If gid == (gid_t)-1, the user actually passed in ULONG_MAX,
     * but this value would get interpreted as (gid_t)-1  by chown
     * and its siblings.   That's not what the user meant!  So we
     * throw an overflow exception instead.   (We already
     * handled a real -1 with PyLong_AsLongAndOverflow() above.)
     */
    if (gid == (gid_t)-1)
        goto overflow;

    /* Ensure the value wasn't truncated. */
    if (sizeof(gid_t) < sizeof(long) &&
        (unsigned long)gid != uresult)
        goto overflow;
    /* fallthrough */

success:
    Py_DECREF(index);
    *(gid_t *)p = gid;
    return 1;

underflow:
    PyErr_SetString(PyExc_OverflowError,
                    "gid is less than minimum");
    goto fail;

overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "gid is greater than maximum");
    /* fallthrough */

fail:
    Py_DECREF(index);
    return 0;
}
#endif /* MS_WINDOWS */


#ifdef AT_FDCWD
/*
 * Why the (int) cast?  Solaris 10 defines AT_FDCWD as 0xffd19553 (-3041965);
 * without the int cast, the value gets interpreted as uint (4291925331),
 * which doesn't play nicely with all the initializer lines in this file that
 * look like this:
 *      int dir_fd = DEFAULT_DIR_FD;
 */
#define DEFAULT_DIR_FD (int)AT_FDCWD
#else
#define DEFAULT_DIR_FD (-100)
#endif

static int
_fd_converter(PyObject *o, int *p, const char *allowed)
{
    int overflow;
    long long_value;

    PyObject *index = PyNumber_Index(o);
    if (index == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "argument should be %s, not %.200s",
                     allowed, Py_TYPE(o)->tp_name);
        return 0;
    }

    long_value = PyLong_AsLongAndOverflow(index, &overflow);
    Py_DECREF(index);
    if (overflow > 0 || long_value > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "fd is greater than maximum");
        return 0;
    }
    if (overflow < 0 || long_value < INT_MIN) {
        PyErr_SetString(PyExc_OverflowError,
                        "fd is less than minimum");
        return 0;
    }

    *p = (int)long_value;
    return 1;
}

static int
dir_fd_converter(PyObject *o, void *p)
{
    if (o == Py_None) {
        *(int *)p = DEFAULT_DIR_FD;
        return 1;
    }
    return _fd_converter(o, (int *)p, "integer");
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
 *     (If path.function_name is NULL it omits the function name.)
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
    const char *function_name;
    const char *argument_name;
    int nullable;
    int allow_fd;
    wchar_t *wide;
    char *narrow;
    int fd;
    Py_ssize_t length;
    PyObject *object;
    PyObject *cleanup;
} path_t;

#define PATH_T_INITIALIZE(function_name, argument_name, nullable, allow_fd) \
    {function_name, argument_name, nullable, allow_fd, NULL, NULL, -1, 0, NULL, NULL}

static void
path_cleanup(path_t *path) {
    if (path->cleanup) {
        Py_CLEAR(path->cleanup);
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

        wide = PyUnicode_AsUnicodeAndSize(unicode, &length);
        if (!wide) {
            Py_DECREF(unicode);
            return 0;
        }
        if (length > 32767) {
            FORMAT_EXCEPTION(PyExc_ValueError, "%s too long for Windows");
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
        if (PyObject_CheckBuffer(o))
            bytes = PyBytes_FromObject(o);
        else
            bytes = NULL;
        if (!bytes) {
            PyErr_Clear();
            if (path->allow_fd) {
                int fd;
                int result = _fd_converter(o, &fd,
                        "string, bytes or integer");
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
    if (length > MAX_PATH-1) {
        FORMAT_EXCEPTION(PyExc_ValueError, "%s too long for Windows");
        Py_DECREF(bytes);
        return 0;
    }
#endif

    narrow = PyBytes_AS_STRING(bytes);
    if ((size_t)length != strlen(narrow)) {
        FORMAT_EXCEPTION(PyExc_ValueError, "embedded null character in %s");
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
dir_fd_unavailable(PyObject *o, void *p)
{
    int dir_fd;
    if (!dir_fd_converter(o, &dir_fd))
        return 0;
    if (dir_fd != DEFAULT_DIR_FD) {
        argument_unavailable_error(NULL, "dir_fd");
        return 0;
    }
    *(int *)p = dir_fd;
    return 1;
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

#ifdef MS_WINDOWS
    typedef PY_LONG_LONG Py_off_t;
#else
    typedef off_t Py_off_t;
#endif

static int
Py_off_t_converter(PyObject *arg, void *addr)
{
#ifdef HAVE_LARGEFILE_SUPPORT
    *((Py_off_t *)addr) = PyLong_AsLongLong(arg);
#else
    *((Py_off_t *)addr) = PyLong_AsLong(arg);
#endif
    if (PyErr_Occurred())
        return 0;
    return 1;
}

static PyObject *
PyLong_FromPy_off_t(Py_off_t offset)
{
#ifdef HAVE_LARGEFILE_SUPPORT
    return PyLong_FromLongLong(offset);
#else
    return PyLong_FromLong(offset);
#endif
}


#if defined _MSC_VER && _MSC_VER >= 1400
/* Microsoft CRT in VS2005 and higher will verify that a filehandle is
 * valid and raise an assertion if it isn't.
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
#if defined(WITH_NEXT_FRAMEWORK) || (defined(__APPLE__) && defined(Py_ENABLE_SHARED))
/* On Darwin/MacOSX a shared library or framework has no access to
** environ directly, we must obtain it with _NSGetEnviron(). See also
** man environ(7).
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

    d = PyDict_New();
    if (d == NULL)
        return NULL;
#if defined(WITH_NEXT_FRAMEWORK) || (defined(__APPLE__) && defined(Py_ENABLE_SHARED))
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
    return d;
}

/* Set a POSIX-specific error from errno, and return NULL */

static PyObject *
posix_error(void)
{
    return PyErr_SetFromErrno(PyExc_OSError);
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
win32_error_object(char* function, PyObject* filename)
{
    /* XXX - see win32_error for comments on 'function' */
    errno = GetLastError();
    if (filename)
        return PyErr_SetExcFromWindowsErrWithFilenameObject(
                    PyExc_OSError,
                    errno,
                    filename);
    else
        return PyErr_SetFromWindowsErr(errno);
}

#endif /* MS_WINDOWS */

static PyObject *
path_error(path_t *path)
{
#ifdef MS_WINDOWS
    return PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError,
                                                        0, path->object);
#else
    return PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path->object);
#endif
}


static PyObject *
path_error2(path_t *path, path_t *path2)
{
#ifdef MS_WINDOWS
    return PyErr_SetExcFromWindowsErrWithFilenameObjects(PyExc_OSError,
            0, path->object, path2->object);
#else
    return PyErr_SetFromErrnoWithFilenameObjects(PyExc_OSError,
        path->object, path2->object);
#endif
}


/* POSIX generic methods */

static int
fildes_converter(PyObject *o, void *p)
{
    int fd;
    int *pointer = (int *)p;
    fd = PyObject_AsFileDescriptor(o);
    if (fd < 0)
        return 0;
    if (!_PyVerify_fd(fd)) {
        posix_error();
        return 0;
    }
    *pointer = fd;
    return 1;
}

static PyObject *
posix_fildes_fd(int fd, int (*func)(int))
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = (*func)(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_INCREF(Py_None);
    return Py_None;
}


#ifdef MS_WINDOWS
/* This is a reimplementation of the C library's chdir function,
   but one that produces Win32 errors instead of DOS error codes.
   chdir is essentially a wrapper around SetCurrentDirectory; however,
   it also needs to set "magic" environment variables indicating
   the per-drive current directory, which are of the form =<drive>: */
static BOOL __stdcall
win32_chdir(LPCSTR path)
{
    char new_path[MAX_PATH];
    int result;
    char env[4] = "=x:";

    if(!SetCurrentDirectoryA(path))
        return FALSE;
    result = GetCurrentDirectoryA(Py_ARRAY_LENGTH(new_path), new_path);
    if (!result)
        return FALSE;
    /* In the ANSI API, there should not be any paths longer
       than MAX_PATH-1 (not including the final null character). */
    assert(result < Py_ARRAY_LENGTH(new_path));
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
    wchar_t _new_path[MAX_PATH], *new_path = _new_path;
    int result;
    wchar_t env[4] = L"=x:";

    if(!SetCurrentDirectoryW(path))
        return FALSE;
    result = GetCurrentDirectoryW(Py_ARRAY_LENGTH(new_path), new_path);
    if (!result)
        return FALSE;
    if (result > Py_ARRAY_LENGTH(new_path)) {
        new_path = PyMem_RawMalloc(result * sizeof(wchar_t));
        if (!new_path) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        result = GetCurrentDirectoryW(result, new_path);
        if (!result) {
            PyMem_RawFree(new_path);
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
        PyMem_RawFree(new_path);
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
#define HAVE_STRUCT_STAT_ST_FILE_ATTRIBUTES 1

struct win32_stat{
    unsigned long st_dev;
    __int64 st_ino;
    unsigned short st_mode;
    int st_nlink;
    int st_uid;
    int st_gid;
    unsigned long st_rdev;
    __int64 st_size;
    time_t st_atime;
    int st_atime_nsec;
    time_t st_mtime;
    int st_mtime_nsec;
    time_t st_ctime;
    int st_ctime_nsec;
    unsigned long st_file_attributes;
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
    result->st_dev = info->dwVolumeSerialNumber;
    result->st_rdev = result->st_dev;
    FILE_TIME_to_time_t_nsec(&info->ftCreationTime, &result->st_ctime, &result->st_ctime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastWriteTime, &result->st_mtime, &result->st_mtime_nsec);
    FILE_TIME_to_time_t_nsec(&info->ftLastAccessTime, &result->st_atime, &result->st_atime_nsec);
    result->st_nlink = info->nNumberOfLinks;
    result->st_ino = (((__int64)info->nFileIndexHigh)<<32) + info->nFileIndexLow;
    if (reparse_tag == IO_REPARSE_TAG_SYMLINK) {
        /* first clear the S_IFMT bits */
        result->st_mode ^= (result->st_mode & S_IFMT);
        /* now set the bits that make this a symlink */
        result->st_mode |= S_IFLNK;
    }
    result->st_file_attributes = info->dwFileAttributes;

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
static int has_GetFinalPathNameByHandle = -1;
static DWORD (CALLBACK *Py_GetFinalPathNameByHandleW)(HANDLE, LPWSTR, DWORD,
                                                      DWORD);
static int
check_GetFinalPathNameByHandle()
{
    HINSTANCE hKernel32;
    DWORD (CALLBACK *Py_GetFinalPathNameByHandleA)(HANDLE, LPSTR, DWORD,
                                                   DWORD);

    /* only recheck */
    if (-1 == has_GetFinalPathNameByHandle)
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

    buf = (wchar_t *)PyMem_Malloc((buf_size+1)*sizeof(wchar_t));
    if (!buf) {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    result_length = Py_GetFinalPathNameByHandleW(hdl,
                       buf, buf_size, VOLUME_NAME_DOS);

    if(!result_length) {
        PyMem_Free(buf);
        return FALSE;
    }

    if(!CloseHandle(hdl)) {
        PyMem_Free(buf);
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
           the symlink path again and not the actual final path. */
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
                PyMem_Free(target_path);
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
           the symlink path again and not the actual final path. */
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
                PyMem_Free(target_path);
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

    if (!_PyVerify_fd(file_number))
        h = INVALID_HANDLE_VALUE;
    else
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
#ifdef HAVE_STRUCT_STAT_ST_FILE_ATTRIBUTES
    {"st_file_attributes", "Windows file attribute bits"},
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

#ifdef HAVE_STRUCT_STAT_ST_FILE_ATTRIBUTES
#define ST_FILE_ATTRIBUTES_IDX (ST_BIRTHTIME_IDX+1)
#else
#define ST_FILE_ATTRIBUTES_IDX ST_BIRTHTIME_IDX
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
\n\
If value is True, future calls to stat() return floats; if it is False,\n\
future calls return ints.\n\
If value is omitted, return the current setting.\n");

/* AC 3.5: the public default value should be None, not ready for that yet */
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
#ifdef MS_WINDOWS
    PyStructSequence_SET_ITEM(v, 2, PyLong_FromUnsignedLong(st->st_dev));
#elif defined(HAVE_LONG_LONG)
    PyStructSequence_SET_ITEM(v, 2,
                              PyLong_FromLongLong((PY_LONG_LONG)st->st_dev));
#else
    PyStructSequence_SET_ITEM(v, 2, PyLong_FromLong((long)st->st_dev));
#endif
    PyStructSequence_SET_ITEM(v, 3, PyLong_FromLong((long)st->st_nlink));
#if defined(MS_WINDOWS)
    PyStructSequence_SET_ITEM(v, 4, PyLong_FromLong(0));
    PyStructSequence_SET_ITEM(v, 5, PyLong_FromLong(0));
#else
    PyStructSequence_SET_ITEM(v, 4, _PyLong_FromUid(st->st_uid));
    PyStructSequence_SET_ITEM(v, 5, _PyLong_FromGid(st->st_gid));
#endif
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
#ifdef HAVE_STRUCT_STAT_ST_FILE_ATTRIBUTES
    PyStructSequence_SET_ITEM(v, ST_FILE_ATTRIBUTES_IDX,
                              PyLong_FromUnsignedLong(st->st_file_attributes));
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

    if (result != 0) {
        return path_error(path);
    }

    return _pystat_fromstructstat(&st);
}

/*[python input]

for s in """

FACCESSAT
FCHMODAT
FCHOWNAT
FSTATAT
LINKAT
MKDIRAT
MKFIFOAT
MKNODAT
OPENAT
READLINKAT
SYMLINKAT
UNLINKAT

""".strip().split():
    s = s.strip()
    print("""
#ifdef HAVE_{s}
    #define {s}_DIR_FD_CONVERTER dir_fd_converter
#else
    #define {s}_DIR_FD_CONVERTER dir_fd_unavailable
#endif
""".rstrip().format(s=s))

for s in """

FCHDIR
FCHMOD
FCHOWN
FDOPENDIR
FEXECVE
FPATHCONF
FSTATVFS
FTRUNCATE

""".strip().split():
    s = s.strip()
    print("""
#ifdef HAVE_{s}
    #define PATH_HAVE_{s} 1
#else
    #define PATH_HAVE_{s} 0
#endif

""".rstrip().format(s=s))
[python start generated code]*/

#ifdef HAVE_FACCESSAT
    #define FACCESSAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define FACCESSAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_FCHMODAT
    #define FCHMODAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define FCHMODAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_FCHOWNAT
    #define FCHOWNAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define FCHOWNAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_FSTATAT
    #define FSTATAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define FSTATAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_LINKAT
    #define LINKAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define LINKAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_MKDIRAT
    #define MKDIRAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define MKDIRAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_MKFIFOAT
    #define MKFIFOAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define MKFIFOAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_MKNODAT
    #define MKNODAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define MKNODAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_OPENAT
    #define OPENAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define OPENAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_READLINKAT
    #define READLINKAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define READLINKAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_SYMLINKAT
    #define SYMLINKAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define SYMLINKAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_UNLINKAT
    #define UNLINKAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define UNLINKAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#ifdef HAVE_FCHDIR
    #define PATH_HAVE_FCHDIR 1
#else
    #define PATH_HAVE_FCHDIR 0
#endif

#ifdef HAVE_FCHMOD
    #define PATH_HAVE_FCHMOD 1
#else
    #define PATH_HAVE_FCHMOD 0
#endif

#ifdef HAVE_FCHOWN
    #define PATH_HAVE_FCHOWN 1
#else
    #define PATH_HAVE_FCHOWN 0
#endif

#ifdef HAVE_FDOPENDIR
    #define PATH_HAVE_FDOPENDIR 1
#else
    #define PATH_HAVE_FDOPENDIR 0
#endif

#ifdef HAVE_FEXECVE
    #define PATH_HAVE_FEXECVE 1
#else
    #define PATH_HAVE_FEXECVE 0
#endif

#ifdef HAVE_FPATHCONF
    #define PATH_HAVE_FPATHCONF 1
#else
    #define PATH_HAVE_FPATHCONF 0
#endif

#ifdef HAVE_FSTATVFS
    #define PATH_HAVE_FSTATVFS 1
#else
    #define PATH_HAVE_FSTATVFS 0
#endif

#ifdef HAVE_FTRUNCATE
    #define PATH_HAVE_FTRUNCATE 1
#else
    #define PATH_HAVE_FTRUNCATE 0
#endif
/*[python end generated code: output=4bd4f6f7d41267f1 input=80b4c890b6774ea5]*/


/*[python input]

class path_t_converter(CConverter):

    type = "path_t"
    impl_by_reference = True
    parse_by_reference = True

    converter = 'path_converter'

    def converter_init(self, *, allow_fd=False, nullable=False):
        # right now path_t doesn't support default values.
        # to support a default value, you'll need to override initialize().
        if self.default not in (unspecified, None):
            fail("Can't specify a default to the path_t converter!")

        if self.c_default not in (None, 'Py_None'):
            raise RuntimeError("Can't specify a c_default to the path_t converter!")

        self.nullable = nullable
        self.allow_fd = allow_fd

    def pre_render(self):
        def strify(value):
            if isinstance(value, str):
                return value
            return str(int(bool(value)))

        # add self.py_name here when merging with posixmodule conversion
        self.c_default = 'PATH_T_INITIALIZE("{}", "{}", {}, {})'.format(
            self.function.name,
            self.name,
            strify(self.nullable),
            strify(self.allow_fd),
            )

    def cleanup(self):
        return "path_cleanup(&" + self.name + ");\n"


class dir_fd_converter(CConverter):
    type = 'int'

    def converter_init(self, requires=None):
        if self.default in (unspecified, None):
            self.c_default = 'DEFAULT_DIR_FD'
        if isinstance(requires, str):
            self.converter = requires.upper() + '_DIR_FD_CONVERTER'
        else:
            self.converter = 'dir_fd_converter'

class fildes_converter(CConverter):
    type = 'int'
    converter = 'fildes_converter'

class uid_t_converter(CConverter):
    type = "uid_t"
    converter = '_Py_Uid_Converter'

class gid_t_converter(CConverter):
    type = "gid_t"
    converter = '_Py_Gid_Converter'

class FSConverter_converter(CConverter):
    type = 'PyObject *'
    converter = 'PyUnicode_FSConverter'
    def converter_init(self):
        if self.default is not unspecified:
            fail("FSConverter_converter does not support default values")
        self.c_default = 'NULL'

    def cleanup(self):
        return "Py_XDECREF(" + self.name + ");\n"

class pid_t_converter(CConverter):
    type = 'pid_t'
    format_unit = '" _Py_PARSE_PID "'

class idtype_t_converter(int_converter):
    type = 'idtype_t'

class id_t_converter(CConverter):
    type = 'id_t'
    format_unit = '" _Py_PARSE_PID "'

class Py_intptr_t_converter(CConverter):
    type = 'Py_intptr_t'
    format_unit = '" _Py_PARSE_INTPTR "'

class Py_off_t_converter(CConverter):
    type = 'Py_off_t'
    converter = 'Py_off_t_converter'

class Py_off_t_return_converter(long_return_converter):
    type = 'Py_off_t'
    conversion_fn = 'PyLong_FromPy_off_t'

class path_confname_converter(CConverter):
    type="int"
    converter="conv_path_confname"

class confstr_confname_converter(path_confname_converter):
    converter='conv_confstr_confname'

class sysconf_confname_converter(path_confname_converter):
    converter="conv_sysconf_confname"

class sched_param_converter(CConverter):
    type = 'struct sched_param'
    converter = 'convert_sched_param'
    impl_by_reference = True;

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=147ba8f52a05aca4]*/

/*[clinic input]

os.stat

    path : path_t(allow_fd=True)
        Path to be examined; can be string, bytes, or open-file-descriptor int.

    *

    dir_fd : dir_fd(requires='fstatat') = None
        If not None, it should be a file descriptor open to a directory,
        and path should be a relative string; path will then be relative to
        that directory.

    follow_symlinks: bool = True
        If False, and the last element of the path is a symbolic link,
        stat will examine the symbolic link itself instead of the file
        the link points to.

Perform a stat system call on the given path.

dir_fd and follow_symlinks may not be implemented
  on your platform.  If they are unavailable, using them will raise a
  NotImplementedError.

It's an error to use dir_fd or follow_symlinks when specifying path as
  an open file descriptor.

[clinic start generated code]*/

PyDoc_STRVAR(os_stat__doc__,
"stat($module, /, path, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Perform a stat system call on the given path.\n"
"\n"
"  path\n"
"    Path to be examined; can be string, bytes, or open-file-descriptor int.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be a relative string; path will then be relative to\n"
"    that directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    stat will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"dir_fd and follow_symlinks may not be implemented\n"
"  on your platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.\n"
"\n"
"It\'s an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.");

#define OS_STAT_METHODDEF    \
    {"stat", (PyCFunction)os_stat, METH_VARARGS|METH_KEYWORDS, os_stat__doc__},

static PyObject *
os_stat_impl(PyModuleDef *module, path_t *path, int dir_fd, int follow_symlinks);

static PyObject *
os_stat(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "dir_fd", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("stat", "path", 0, 1);
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|$O&p:stat", _keywords,
        path_converter, &path, FSTATAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks))
        goto exit;
    return_value = os_stat_impl(module, &path, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_stat_impl(PyModuleDef *module, path_t *path, int dir_fd, int follow_symlinks)
/*[clinic end generated code: output=0e9f9508fa0c0607 input=099d356c306fa24a]*/
{
    return posix_do_stat("stat", path, dir_fd, follow_symlinks);
}


/*[clinic input]
os.lstat

    path : path_t

    *

    dir_fd : dir_fd(requires='fstatat') = None

Perform a stat system call on the given path, without following symbolic links.

Like stat(), but do not follow symbolic links.
Equivalent to stat(path, follow_symlinks=False).
[clinic start generated code]*/

PyDoc_STRVAR(os_lstat__doc__,
"lstat($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Perform a stat system call on the given path, without following symbolic links.\n"
"\n"
"Like stat(), but do not follow symbolic links.\n"
"Equivalent to stat(path, follow_symlinks=False).");

#define OS_LSTAT_METHODDEF    \
    {"lstat", (PyCFunction)os_lstat, METH_VARARGS|METH_KEYWORDS, os_lstat__doc__},

static PyObject *
os_lstat_impl(PyModuleDef *module, path_t *path, int dir_fd);

static PyObject *
os_lstat(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("lstat", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|$O&:lstat", _keywords,
        path_converter, &path, FSTATAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_lstat_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_lstat_impl(PyModuleDef *module, path_t *path, int dir_fd)
/*[clinic end generated code: output=85702247224a2b1c input=0b7474765927b925]*/
{
    int follow_symlinks = 0;
    return posix_do_stat("lstat", path, dir_fd, follow_symlinks);
}


/*[clinic input]
os.access -> bool

    path: path_t(allow_fd=True)
        Path to be tested; can be string, bytes, or open-file-descriptor int.

    mode: int
        Operating-system mode bitfield.  Can be F_OK to test existence,
        or the inclusive-OR of R_OK, W_OK, and X_OK.

    *

    dir_fd : dir_fd(requires='faccessat') = None
        If not None, it should be a file descriptor open to a directory,
        and path should be relative; path will then be relative to that
        directory.

    effective_ids: bool = False
        If True, access will use the effective uid/gid instead of
        the real uid/gid.

    follow_symlinks: bool = True
        If False, and the last element of the path is a symbolic link,
        access will examine the symbolic link itself instead of the file
        the link points to.

Use the real uid/gid to test for access to a path.

{parameters}
dir_fd, effective_ids, and follow_symlinks may not be implemented
  on your platform.  If they are unavailable, using them will raise a
  NotImplementedError.

Note that most operations will use the effective uid/gid, therefore this
  routine can be used in a suid/sgid environment to test if the invoking user
  has the specified access to the path.

[clinic start generated code]*/

PyDoc_STRVAR(os_access__doc__,
"access($module, /, path, mode, *, dir_fd=None, effective_ids=False,\n"
"       follow_symlinks=True)\n"
"--\n"
"\n"
"Use the real uid/gid to test for access to a path.\n"
"\n"
"  path\n"
"    Path to be tested; can be string, bytes, or open-file-descriptor int.\n"
"  mode\n"
"    Operating-system mode bitfield.  Can be F_OK to test existence,\n"
"    or the inclusive-OR of R_OK, W_OK, and X_OK.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  effective_ids\n"
"    If True, access will use the effective uid/gid instead of\n"
"    the real uid/gid.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    access will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"dir_fd, effective_ids, and follow_symlinks may not be implemented\n"
"  on your platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.\n"
"\n"
"Note that most operations will use the effective uid/gid, therefore this\n"
"  routine can be used in a suid/sgid environment to test if the invoking user\n"
"  has the specified access to the path.");

#define OS_ACCESS_METHODDEF    \
    {"access", (PyCFunction)os_access, METH_VARARGS|METH_KEYWORDS, os_access__doc__},

static int
os_access_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd, int effective_ids, int follow_symlinks);

static PyObject *
os_access(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", "dir_fd", "effective_ids", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("access", "path", 0, 1);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int effective_ids = 0;
    int follow_symlinks = 1;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&i|$O&pp:access", _keywords,
        path_converter, &path, &mode, FACCESSAT_DIR_FD_CONVERTER, &dir_fd, &effective_ids, &follow_symlinks))
        goto exit;
    _return_value = os_access_impl(module, &path, mode, dir_fd, effective_ids, follow_symlinks);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static int
os_access_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd, int effective_ids, int follow_symlinks)
/*[clinic end generated code: output=dfd404666906f012 input=b75a756797af45ec]*/
{
    int return_value;

#ifdef MS_WINDOWS
    DWORD attr;
#else
    int result;
#endif

#ifndef HAVE_FACCESSAT
    if (follow_symlinks_specified("access", follow_symlinks))
        return -1;

    if (effective_ids) {
        argument_unavailable_error("access", "effective_ids");
        return -1;
    }
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path->wide != NULL)
        attr = GetFileAttributesW(path->wide);
    else
        attr = GetFileAttributesA(path->narrow);
    Py_END_ALLOW_THREADS

    /*
     * Access is possible if
     *   * we didn't get a -1, and
     *     * write access wasn't requested,
     *     * or the file isn't read-only,
     *     * or it's a directory.
     * (Directories cannot be read-only on Windows.)
    */
    return_value = (attr != INVALID_FILE_ATTRIBUTES) &&
            (!(mode & 2) ||
            !(attr & FILE_ATTRIBUTE_READONLY) ||
            (attr & FILE_ATTRIBUTE_DIRECTORY));
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
        result = faccessat(dir_fd, path->narrow, mode, flags);
    }
    else
#endif
        result = access(path->narrow, mode);
    Py_END_ALLOW_THREADS
    return_value = !result;
#endif

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
/*[clinic input]
os.ttyname -> DecodeFSDefault

    fd: int
        Integer file descriptor handle.

    /

Return the name of the terminal device connected to 'fd'.
[clinic start generated code]*/

PyDoc_STRVAR(os_ttyname__doc__,
"ttyname($module, fd, /)\n"
"--\n"
"\n"
"Return the name of the terminal device connected to \'fd\'.\n"
"\n"
"  fd\n"
"    Integer file descriptor handle.");

#define OS_TTYNAME_METHODDEF    \
    {"ttyname", (PyCFunction)os_ttyname, METH_VARARGS, os_ttyname__doc__},

static char *
os_ttyname_impl(PyModuleDef *module, int fd);

static PyObject *
os_ttyname(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    char *_return_value;

    if (!PyArg_ParseTuple(args,
        "i:ttyname",
        &fd))
        goto exit;
    _return_value = os_ttyname_impl(module, fd);
    if (_return_value == NULL)
        goto exit;
    return_value = PyUnicode_DecodeFSDefault(_return_value);

exit:
    return return_value;
}

static char *
os_ttyname_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=cee7bc4cffec01a2 input=5f72ca83e76b3b45]*/
{
    char *ret;

    ret = ttyname(fd);
    if (ret == NULL)
        posix_error();
    return ret;
}
#endif

#ifdef HAVE_CTERMID
/*[clinic input]
os.ctermid

Return the name of the controlling terminal for this process.
[clinic start generated code]*/

PyDoc_STRVAR(os_ctermid__doc__,
"ctermid($module, /)\n"
"--\n"
"\n"
"Return the name of the controlling terminal for this process.");

#define OS_CTERMID_METHODDEF    \
    {"ctermid", (PyCFunction)os_ctermid, METH_NOARGS, os_ctermid__doc__},

static PyObject *
os_ctermid_impl(PyModuleDef *module);

static PyObject *
os_ctermid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_ctermid_impl(module);
}

static PyObject *
os_ctermid_impl(PyModuleDef *module)
/*[clinic end generated code: output=277bf7964ec2d782 input=3b87fdd52556382d]*/
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
#endif /* HAVE_CTERMID */


/*[clinic input]
os.chdir

    path: path_t(allow_fd='PATH_HAVE_FCHDIR')

Change the current working directory to the specified path.

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.
[clinic start generated code]*/

PyDoc_STRVAR(os_chdir__doc__,
"chdir($module, /, path)\n"
"--\n"
"\n"
"Change the current working directory to the specified path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_CHDIR_METHODDEF    \
    {"chdir", (PyCFunction)os_chdir, METH_VARARGS|METH_KEYWORDS, os_chdir__doc__},

static PyObject *
os_chdir_impl(PyModuleDef *module, path_t *path);

static PyObject *
os_chdir(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    path_t path = PATH_T_INITIALIZE("chdir", "path", 0, PATH_HAVE_FCHDIR);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:chdir", _keywords,
        path_converter, &path))
        goto exit;
    return_value = os_chdir_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_chdir_impl(PyModuleDef *module, path_t *path)
/*[clinic end generated code: output=cc07592dd23ca9e0 input=1a4a15b4d12cb15d]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path->wide)
        result = win32_wchdir(path->wide);
    else
        result = win32_chdir(path->narrow);
    result = !result; /* on unix, success = 0, on windows, success = !0 */
#else
#ifdef HAVE_FCHDIR
    if (path->fd != -1)
        result = fchdir(path->fd);
    else
#endif
        result = chdir(path->narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result) {
        return path_error(path);
    }

    Py_RETURN_NONE;
}


#ifdef HAVE_FCHDIR
/*[clinic input]
os.fchdir

    fd: fildes

Change to the directory of the given file descriptor.

fd must be opened on a directory, not a file.
Equivalent to os.chdir(fd).

[clinic start generated code]*/

PyDoc_STRVAR(os_fchdir__doc__,
"fchdir($module, /, fd)\n"
"--\n"
"\n"
"Change to the directory of the given file descriptor.\n"
"\n"
"fd must be opened on a directory, not a file.\n"
"Equivalent to os.chdir(fd).");

#define OS_FCHDIR_METHODDEF    \
    {"fchdir", (PyCFunction)os_fchdir, METH_VARARGS|METH_KEYWORDS, os_fchdir__doc__},

static PyObject *
os_fchdir_impl(PyModuleDef *module, int fd);

static PyObject *
os_fchdir(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:fchdir", _keywords,
        fildes_converter, &fd))
        goto exit;
    return_value = os_fchdir_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_fchdir_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=9f6dbc89b2778834 input=18e816479a2fa985]*/
{
    return posix_fildes_fd(fd, fchdir);
}
#endif /* HAVE_FCHDIR */


/*[clinic input]
os.chmod

    path: path_t(allow_fd='PATH_HAVE_FCHMOD')
        Path to be modified.  May always be specified as a str or bytes.
        On some platforms, path may also be specified as an open file descriptor.
        If this functionality is unavailable, using it raises an exception.

    mode: int
        Operating-system mode bitfield.

    *

    dir_fd : dir_fd(requires='fchmodat') = None
        If not None, it should be a file descriptor open to a directory,
        and path should be relative; path will then be relative to that
        directory.

    follow_symlinks: bool = True
        If False, and the last element of the path is a symbolic link,
        chmod will modify the symbolic link itself instead of the file
        the link points to.

Change the access permissions of a file.

It is an error to use dir_fd or follow_symlinks when specifying path as
  an open file descriptor.
dir_fd and follow_symlinks may not be implemented on your platform.
  If they are unavailable, using them will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_chmod__doc__,
"chmod($module, /, path, mode, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Change the access permissions of a file.\n"
"\n"
"  path\n"
"    Path to be modified.  May always be specified as a str or bytes.\n"
"    On some platforms, path may also be specified as an open file descriptor.\n"
"    If this functionality is unavailable, using it raises an exception.\n"
"  mode\n"
"    Operating-system mode bitfield.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    chmod will modify the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"It is an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.\n"
"dir_fd and follow_symlinks may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_CHMOD_METHODDEF    \
    {"chmod", (PyCFunction)os_chmod, METH_VARARGS|METH_KEYWORDS, os_chmod__doc__},

static PyObject *
os_chmod_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd, int follow_symlinks);

static PyObject *
os_chmod(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", "dir_fd", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("chmod", "path", 0, PATH_HAVE_FCHMOD);
    int mode;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&i|$O&p:chmod", _keywords,
        path_converter, &path, &mode, FCHMODAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks))
        goto exit;
    return_value = os_chmod_impl(module, &path, mode, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_chmod_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd, int follow_symlinks)
/*[clinic end generated code: output=1e9db031aea46422 input=7f1618e5e15cc196]*/
{
    int result;

#ifdef MS_WINDOWS
    DWORD attr;
#endif

#ifdef HAVE_FCHMODAT
    int fchmodat_nofollow_unsupported = 0;
#endif

#if !(defined(HAVE_FCHMODAT) || defined(HAVE_LCHMOD))
    if (follow_symlinks_specified("chmod", follow_symlinks))
        return NULL;
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path->wide)
        attr = GetFileAttributesW(path->wide);
    else
        attr = GetFileAttributesA(path->narrow);
    if (attr == INVALID_FILE_ATTRIBUTES)
        result = 0;
    else {
        if (mode & _S_IWRITE)
            attr &= ~FILE_ATTRIBUTE_READONLY;
        else
            attr |= FILE_ATTRIBUTE_READONLY;
        if (path->wide)
            result = SetFileAttributesW(path->wide, attr);
        else
            result = SetFileAttributesA(path->narrow, attr);
    }
    Py_END_ALLOW_THREADS

    if (!result) {
        return path_error(path);
    }
#else /* MS_WINDOWS */
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FCHMOD
    if (path->fd != -1)
        result = fchmod(path->fd, mode);
    else
#endif
#ifdef HAVE_LCHMOD
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = lchmod(path->narrow, mode);
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
        result = fchmodat(dir_fd, path->narrow, mode,
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
        result = chmod(path->narrow, mode);
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
        return path_error(path);
    }
#endif

    Py_RETURN_NONE;
}


#ifdef HAVE_FCHMOD
/*[clinic input]
os.fchmod

    fd: int
    mode: int

Change the access permissions of the file given by file descriptor fd.

Equivalent to os.chmod(fd, mode).
[clinic start generated code]*/

PyDoc_STRVAR(os_fchmod__doc__,
"fchmod($module, /, fd, mode)\n"
"--\n"
"\n"
"Change the access permissions of the file given by file descriptor fd.\n"
"\n"
"Equivalent to os.chmod(fd, mode).");

#define OS_FCHMOD_METHODDEF    \
    {"fchmod", (PyCFunction)os_fchmod, METH_VARARGS|METH_KEYWORDS, os_fchmod__doc__},

static PyObject *
os_fchmod_impl(PyModuleDef *module, int fd, int mode);

static PyObject *
os_fchmod(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", "mode", NULL};
    int fd;
    int mode;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "ii:fchmod", _keywords,
        &fd, &mode))
        goto exit;
    return_value = os_fchmod_impl(module, fd, mode);

exit:
    return return_value;
}

static PyObject *
os_fchmod_impl(PyModuleDef *module, int fd, int mode)
/*[clinic end generated code: output=3c19fbfd724a8e0f input=8ab11975ca01ee5b]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = fchmod(fd, mode);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHMOD */


#ifdef HAVE_LCHMOD
/*[clinic input]
os.lchmod

    path: path_t
    mode: int

Change the access permissions of a file, without following symbolic links.

If path is a symlink, this affects the link itself rather than the target.
Equivalent to chmod(path, mode, follow_symlinks=False)."
[clinic start generated code]*/

PyDoc_STRVAR(os_lchmod__doc__,
"lchmod($module, /, path, mode)\n"
"--\n"
"\n"
"Change the access permissions of a file, without following symbolic links.\n"
"\n"
"If path is a symlink, this affects the link itself rather than the target.\n"
"Equivalent to chmod(path, mode, follow_symlinks=False).\"");

#define OS_LCHMOD_METHODDEF    \
    {"lchmod", (PyCFunction)os_lchmod, METH_VARARGS|METH_KEYWORDS, os_lchmod__doc__},

static PyObject *
os_lchmod_impl(PyModuleDef *module, path_t *path, int mode);

static PyObject *
os_lchmod(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", NULL};
    path_t path = PATH_T_INITIALIZE("lchmod", "path", 0, 0);
    int mode;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&i:lchmod", _keywords,
        path_converter, &path, &mode))
        goto exit;
    return_value = os_lchmod_impl(module, &path, mode);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_lchmod_impl(PyModuleDef *module, path_t *path, int mode)
/*[clinic end generated code: output=2849977d65f8c68c input=90c5663c7465d24f]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = lchmod(path->narrow, mode);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        path_error(path);
        return NULL;
    }
    Py_RETURN_NONE;
}
#endif /* HAVE_LCHMOD */


#ifdef HAVE_CHFLAGS
/*[clinic input]
os.chflags

    path: path_t
    flags: unsigned_long(bitwise=True)
    follow_symlinks: bool=True

Set file flags.

If follow_symlinks is False, and the last element of the path is a symbolic
  link, chflags will change flags on the symbolic link itself instead of the
  file the link points to.
follow_symlinks may not be implemented on your platform.  If it is
unavailable, using it will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_chflags__doc__,
"chflags($module, /, path, flags, follow_symlinks=True)\n"
"--\n"
"\n"
"Set file flags.\n"
"\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, chflags will change flags on the symbolic link itself instead of the\n"
"  file the link points to.\n"
"follow_symlinks may not be implemented on your platform.  If it is\n"
"unavailable, using it will raise a NotImplementedError.");

#define OS_CHFLAGS_METHODDEF    \
    {"chflags", (PyCFunction)os_chflags, METH_VARARGS|METH_KEYWORDS, os_chflags__doc__},

static PyObject *
os_chflags_impl(PyModuleDef *module, path_t *path, unsigned long flags, int follow_symlinks);

static PyObject *
os_chflags(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "flags", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("chflags", "path", 0, 0);
    unsigned long flags;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&k|p:chflags", _keywords,
        path_converter, &path, &flags, &follow_symlinks))
        goto exit;
    return_value = os_chflags_impl(module, &path, flags, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_chflags_impl(PyModuleDef *module, path_t *path, unsigned long flags, int follow_symlinks)
/*[clinic end generated code: output=2767927bf071e3cf input=0327e29feb876236]*/
{
    int result;

#ifndef HAVE_LCHFLAGS
    if (follow_symlinks_specified("chflags", follow_symlinks))
        return NULL;
#endif

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_LCHFLAGS
    if (!follow_symlinks)
        result = lchflags(path->narrow, flags);
    else
#endif
        result = chflags(path->narrow, flags);
    Py_END_ALLOW_THREADS

    if (result)
        return path_error(path);

    Py_RETURN_NONE;
}
#endif /* HAVE_CHFLAGS */


#ifdef HAVE_LCHFLAGS
/*[clinic input]
os.lchflags

    path: path_t
    flags: unsigned_long(bitwise=True)

Set file flags.

This function will not follow symbolic links.
Equivalent to chflags(path, flags, follow_symlinks=False).
[clinic start generated code]*/

PyDoc_STRVAR(os_lchflags__doc__,
"lchflags($module, /, path, flags)\n"
"--\n"
"\n"
"Set file flags.\n"
"\n"
"This function will not follow symbolic links.\n"
"Equivalent to chflags(path, flags, follow_symlinks=False).");

#define OS_LCHFLAGS_METHODDEF    \
    {"lchflags", (PyCFunction)os_lchflags, METH_VARARGS|METH_KEYWORDS, os_lchflags__doc__},

static PyObject *
os_lchflags_impl(PyModuleDef *module, path_t *path, unsigned long flags);

static PyObject *
os_lchflags(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "flags", NULL};
    path_t path = PATH_T_INITIALIZE("lchflags", "path", 0, 0);
    unsigned long flags;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&k:lchflags", _keywords,
        path_converter, &path, &flags))
        goto exit;
    return_value = os_lchflags_impl(module, &path, flags);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_lchflags_impl(PyModuleDef *module, path_t *path, unsigned long flags)
/*[clinic end generated code: output=bb93b6b8a5e45aa7 input=f9f82ea8b585ca9d]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = lchflags(path->narrow, flags);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        return path_error(path);
    }
    Py_RETURN_NONE;
}
#endif /* HAVE_LCHFLAGS */


#ifdef HAVE_CHROOT
/*[clinic input]
os.chroot
    path: path_t

Change root directory to path.

[clinic start generated code]*/

PyDoc_STRVAR(os_chroot__doc__,
"chroot($module, /, path)\n"
"--\n"
"\n"
"Change root directory to path.");

#define OS_CHROOT_METHODDEF    \
    {"chroot", (PyCFunction)os_chroot, METH_VARARGS|METH_KEYWORDS, os_chroot__doc__},

static PyObject *
os_chroot_impl(PyModuleDef *module, path_t *path);

static PyObject *
os_chroot(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    path_t path = PATH_T_INITIALIZE("chroot", "path", 0, 0);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:chroot", _keywords,
        path_converter, &path))
        goto exit;
    return_value = os_chroot_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_chroot_impl(PyModuleDef *module, path_t *path)
/*[clinic end generated code: output=15b1256cbe4f24a1 input=14822965652c3dc3]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = chroot(path->narrow);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return path_error(path);
    Py_RETURN_NONE;
}
#endif /* HAVE_CHROOT */


#ifdef HAVE_FSYNC
/*[clinic input]
os.fsync

    fd: fildes

Force write of fd to disk.
[clinic start generated code]*/

PyDoc_STRVAR(os_fsync__doc__,
"fsync($module, /, fd)\n"
"--\n"
"\n"
"Force write of fd to disk.");

#define OS_FSYNC_METHODDEF    \
    {"fsync", (PyCFunction)os_fsync, METH_VARARGS|METH_KEYWORDS, os_fsync__doc__},

static PyObject *
os_fsync_impl(PyModuleDef *module, int fd);

static PyObject *
os_fsync(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:fsync", _keywords,
        fildes_converter, &fd))
        goto exit;
    return_value = os_fsync_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_fsync_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=59f32d3a0b360133 input=21c3645c056967f2]*/
{
    return posix_fildes_fd(fd, fsync);
}
#endif /* HAVE_FSYNC */


#ifdef HAVE_SYNC
/*[clinic input]
os.sync

Force write of everything to disk.
[clinic start generated code]*/

PyDoc_STRVAR(os_sync__doc__,
"sync($module, /)\n"
"--\n"
"\n"
"Force write of everything to disk.");

#define OS_SYNC_METHODDEF    \
    {"sync", (PyCFunction)os_sync, METH_NOARGS, os_sync__doc__},

static PyObject *
os_sync_impl(PyModuleDef *module);

static PyObject *
os_sync(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_sync_impl(module);
}

static PyObject *
os_sync_impl(PyModuleDef *module)
/*[clinic end generated code: output=526c495683d0bb38 input=84749fe5e9b404ff]*/
{
    Py_BEGIN_ALLOW_THREADS
    sync();
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}
#endif /* HAVE_SYNC */


#ifdef HAVE_FDATASYNC
#ifdef __hpux
extern int fdatasync(int); /* On HP-UX, in libc but not in unistd.h */
#endif

/*[clinic input]
os.fdatasync

    fd: fildes

Force write of fd to disk without forcing update of metadata.
[clinic start generated code]*/

PyDoc_STRVAR(os_fdatasync__doc__,
"fdatasync($module, /, fd)\n"
"--\n"
"\n"
"Force write of fd to disk without forcing update of metadata.");

#define OS_FDATASYNC_METHODDEF    \
    {"fdatasync", (PyCFunction)os_fdatasync, METH_VARARGS|METH_KEYWORDS, os_fdatasync__doc__},

static PyObject *
os_fdatasync_impl(PyModuleDef *module, int fd);

static PyObject *
os_fdatasync(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:fdatasync", _keywords,
        fildes_converter, &fd))
        goto exit;
    return_value = os_fdatasync_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_fdatasync_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=2335fdfd37c92180 input=bc74791ee54dd291]*/
{
    return posix_fildes_fd(fd, fdatasync);
}
#endif /* HAVE_FDATASYNC */


#ifdef HAVE_CHOWN
/*[clinic input]
os.chown

    path : path_t(allow_fd='PATH_HAVE_FCHOWN')
        Path to be examined; can be string, bytes, or open-file-descriptor int.

    uid: uid_t

    gid: gid_t

    *

    dir_fd : dir_fd(requires='fchownat') = None
        If not None, it should be a file descriptor open to a directory,
        and path should be relative; path will then be relative to that
        directory.

    follow_symlinks: bool = True
        If False, and the last element of the path is a symbolic link,
        stat will examine the symbolic link itself instead of the file
        the link points to.

Change the owner and group id of path to the numeric uid and gid.\

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.
If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, chown will modify the symbolic link itself instead of the file the
  link points to.
It is an error to use dir_fd or follow_symlinks when specifying path as
  an open file descriptor.
dir_fd and follow_symlinks may not be implemented on your platform.
  If they are unavailable, using them will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_chown__doc__,
"chown($module, /, path, uid, gid, *, dir_fd=None, follow_symlinks=True)\n"
"--\n"
"\n"
"Change the owner and group id of path to the numeric uid and gid.\\\n"
"\n"
"  path\n"
"    Path to be examined; can be string, bytes, or open-file-descriptor int.\n"
"  dir_fd\n"
"    If not None, it should be a file descriptor open to a directory,\n"
"    and path should be relative; path will then be relative to that\n"
"    directory.\n"
"  follow_symlinks\n"
"    If False, and the last element of the path is a symbolic link,\n"
"    stat will examine the symbolic link itself instead of the file\n"
"    the link points to.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, chown will modify the symbolic link itself instead of the file the\n"
"  link points to.\n"
"It is an error to use dir_fd or follow_symlinks when specifying path as\n"
"  an open file descriptor.\n"
"dir_fd and follow_symlinks may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_CHOWN_METHODDEF    \
    {"chown", (PyCFunction)os_chown, METH_VARARGS|METH_KEYWORDS, os_chown__doc__},

static PyObject *
os_chown_impl(PyModuleDef *module, path_t *path, uid_t uid, gid_t gid, int dir_fd, int follow_symlinks);

static PyObject *
os_chown(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "uid", "gid", "dir_fd", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("chown", "path", 0, PATH_HAVE_FCHOWN);
    uid_t uid;
    gid_t gid;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&O&|$O&p:chown", _keywords,
        path_converter, &path, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid, FCHOWNAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks))
        goto exit;
    return_value = os_chown_impl(module, &path, uid, gid, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_chown_impl(PyModuleDef *module, path_t *path, uid_t uid, gid_t gid, int dir_fd, int follow_symlinks)
/*[clinic end generated code: output=22f011e3b4f9ff49 input=a61cc35574814d5d]*/
{
    int result;

#if !(defined(HAVE_LCHOWN) || defined(HAVE_FCHOWNAT))
    if (follow_symlinks_specified("chown", follow_symlinks))
        return NULL;
#endif
    if (dir_fd_and_fd_invalid("chown", dir_fd, path->fd) ||
        fd_and_follow_symlinks_invalid("chown", path->fd, follow_symlinks))
        return NULL;

#ifdef __APPLE__
    /*
     * This is for Mac OS X 10.3, which doesn't have lchown.
     * (But we still have an lchown symbol because of weak-linking.)
     * It doesn't have fchownat either.  So there's no possibility
     * of a graceful failover.
     */
    if ((!follow_symlinks) && (lchown == NULL)) {
        follow_symlinks_specified("chown", follow_symlinks);
        return NULL;
    }
#endif

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FCHOWN
    if (path->fd != -1)
        result = fchown(path->fd, uid, gid);
    else
#endif
#ifdef HAVE_LCHOWN
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = lchown(path->narrow, uid, gid);
    else
#endif
#ifdef HAVE_FCHOWNAT
    if ((dir_fd != DEFAULT_DIR_FD) || (!follow_symlinks))
        result = fchownat(dir_fd, path->narrow, uid, gid,
                          follow_symlinks ? 0 : AT_SYMLINK_NOFOLLOW);
    else
#endif
        result = chown(path->narrow, uid, gid);
    Py_END_ALLOW_THREADS

    if (result)
        return path_error(path);

    Py_RETURN_NONE;
}
#endif /* HAVE_CHOWN */


#ifdef HAVE_FCHOWN
/*[clinic input]
os.fchown

    fd: int
    uid: uid_t
    gid: gid_t

Change the owner and group id of the file specified by file descriptor.

Equivalent to os.chown(fd, uid, gid).

[clinic start generated code]*/

PyDoc_STRVAR(os_fchown__doc__,
"fchown($module, /, fd, uid, gid)\n"
"--\n"
"\n"
"Change the owner and group id of the file specified by file descriptor.\n"
"\n"
"Equivalent to os.chown(fd, uid, gid).");

#define OS_FCHOWN_METHODDEF    \
    {"fchown", (PyCFunction)os_fchown, METH_VARARGS|METH_KEYWORDS, os_fchown__doc__},

static PyObject *
os_fchown_impl(PyModuleDef *module, int fd, uid_t uid, gid_t gid);

static PyObject *
os_fchown(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", "uid", "gid", NULL};
    int fd;
    uid_t uid;
    gid_t gid;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "iO&O&:fchown", _keywords,
        &fd, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid))
        goto exit;
    return_value = os_fchown_impl(module, fd, uid, gid);

exit:
    return return_value;
}

static PyObject *
os_fchown_impl(PyModuleDef *module, int fd, uid_t uid, gid_t gid)
/*[clinic end generated code: output=687781cb7d8974d6 input=3af544ba1b13a0d7]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = fchown(fd, uid, gid);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FCHOWN */


#ifdef HAVE_LCHOWN
/*[clinic input]
os.lchown

    path : path_t
    uid: uid_t
    gid: gid_t

Change the owner and group id of path to the numeric uid and gid.

This function will not follow symbolic links.
Equivalent to os.chown(path, uid, gid, follow_symlinks=False).
[clinic start generated code]*/

PyDoc_STRVAR(os_lchown__doc__,
"lchown($module, /, path, uid, gid)\n"
"--\n"
"\n"
"Change the owner and group id of path to the numeric uid and gid.\n"
"\n"
"This function will not follow symbolic links.\n"
"Equivalent to os.chown(path, uid, gid, follow_symlinks=False).");

#define OS_LCHOWN_METHODDEF    \
    {"lchown", (PyCFunction)os_lchown, METH_VARARGS|METH_KEYWORDS, os_lchown__doc__},

static PyObject *
os_lchown_impl(PyModuleDef *module, path_t *path, uid_t uid, gid_t gid);

static PyObject *
os_lchown(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "uid", "gid", NULL};
    path_t path = PATH_T_INITIALIZE("lchown", "path", 0, 0);
    uid_t uid;
    gid_t gid;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&O&:lchown", _keywords,
        path_converter, &path, _Py_Uid_Converter, &uid, _Py_Gid_Converter, &gid))
        goto exit;
    return_value = os_lchown_impl(module, &path, uid, gid);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_lchown_impl(PyModuleDef *module, path_t *path, uid_t uid, gid_t gid)
/*[clinic end generated code: output=bf25fdb0d25130e2 input=b1c6014d563a7161]*/
{
    int res;
    Py_BEGIN_ALLOW_THREADS
    res = lchown(path->narrow, uid, gid);
    Py_END_ALLOW_THREADS
    if (res < 0) {
        return path_error(path);
    }
    Py_RETURN_NONE;
}
#endif /* HAVE_LCHOWN */


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
        len = GetCurrentDirectoryW(Py_ARRAY_LENGTH(wbuf), wbuf);
        /* If the buffer is large enough, len does not include the
           terminating \0. If the buffer is too small, len includes
           the space needed for the terminator. */
        if (len >= Py_ARRAY_LENGTH(wbuf)) {
            wbuf2 = PyMem_RawMalloc(len * sizeof(wchar_t));
            if (wbuf2)
                len = GetCurrentDirectoryW(len, wbuf2);
        }
        Py_END_ALLOW_THREADS
        if (!wbuf2) {
            PyErr_NoMemory();
            return NULL;
        }
        if (!len) {
            if (wbuf2 != wbuf)
                PyMem_RawFree(wbuf2);
            return PyErr_SetFromWindowsErr(0);
        }
        resobj = PyUnicode_FromWideChar(wbuf2, len);
        if (wbuf2 != wbuf)
            PyMem_RawFree(wbuf2);
        return resobj;
    }

    if (win32_warn_bytes_api())
        return NULL;
#endif

    Py_BEGIN_ALLOW_THREADS
    res = getcwd(buf, sizeof buf);
    Py_END_ALLOW_THREADS
    if (res == NULL)
        return posix_error();
    if (use_bytes)
        return PyBytes_FromStringAndSize(buf, strlen(buf));
    return PyUnicode_DecodeFSDefault(buf);
}


/*[clinic input]
os.getcwd

Return a unicode string representing the current working directory.
[clinic start generated code]*/

PyDoc_STRVAR(os_getcwd__doc__,
"getcwd($module, /)\n"
"--\n"
"\n"
"Return a unicode string representing the current working directory.");

#define OS_GETCWD_METHODDEF    \
    {"getcwd", (PyCFunction)os_getcwd, METH_NOARGS, os_getcwd__doc__},

static PyObject *
os_getcwd_impl(PyModuleDef *module);

static PyObject *
os_getcwd(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getcwd_impl(module);
}

static PyObject *
os_getcwd_impl(PyModuleDef *module)
/*[clinic end generated code: output=d70b281db5c78ff7 input=f069211bb70e3d39]*/
{
    return posix_getcwd(0);
}


/*[clinic input]
os.getcwdb

Return a bytes string representing the current working directory.
[clinic start generated code]*/

PyDoc_STRVAR(os_getcwdb__doc__,
"getcwdb($module, /)\n"
"--\n"
"\n"
"Return a bytes string representing the current working directory.");

#define OS_GETCWDB_METHODDEF    \
    {"getcwdb", (PyCFunction)os_getcwdb, METH_NOARGS, os_getcwdb__doc__},

static PyObject *
os_getcwdb_impl(PyModuleDef *module);

static PyObject *
os_getcwdb(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getcwdb_impl(module);
}

static PyObject *
os_getcwdb_impl(PyModuleDef *module)
/*[clinic end generated code: output=75da47f2d75f9166 input=f6f6a378dad3d9cb]*/
{
    return posix_getcwd(1);
}


#if ((!defined(HAVE_LINK)) && defined(MS_WINDOWS))
#define HAVE_LINK 1
#endif

#ifdef HAVE_LINK
/*[clinic input]

os.link

    src : path_t
    dst : path_t
    *
    src_dir_fd : dir_fd = None
    dst_dir_fd : dir_fd = None
    follow_symlinks: bool = True

Create a hard link to a file.

If either src_dir_fd or dst_dir_fd is not None, it should be a file
  descriptor open to a directory, and the respective path string (src or dst)
  should be relative; the path will then be relative to that directory.
If follow_symlinks is False, and the last element of src is a symbolic
  link, link will create a link to the symbolic link itself instead of the
  file the link points to.
src_dir_fd, dst_dir_fd, and follow_symlinks may not be implemented on your
  platform.  If they are unavailable, using them will raise a
  NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_link__doc__,
"link($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None,\n"
"     follow_symlinks=True)\n"
"--\n"
"\n"
"Create a hard link to a file.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of src is a symbolic\n"
"  link, link will create a link to the symbolic link itself instead of the\n"
"  file the link points to.\n"
"src_dir_fd, dst_dir_fd, and follow_symlinks may not be implemented on your\n"
"  platform.  If they are unavailable, using them will raise a\n"
"  NotImplementedError.");

#define OS_LINK_METHODDEF    \
    {"link", (PyCFunction)os_link, METH_VARARGS|METH_KEYWORDS, os_link__doc__},

static PyObject *
os_link_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd, int follow_symlinks);

static PyObject *
os_link(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", "follow_symlinks", NULL};
    path_t src = PATH_T_INITIALIZE("link", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("link", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|$O&O&p:link", _keywords,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd, &follow_symlinks))
        goto exit;
    return_value = os_link_impl(module, &src, &dst, src_dir_fd, dst_dir_fd, follow_symlinks);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

static PyObject *
os_link_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd, int follow_symlinks)
/*[clinic end generated code: output=53477662fe02e183 input=b0095ebbcbaa7e04]*/
{
#ifdef MS_WINDOWS
    BOOL result;
#else
    int result;
#endif

#ifndef HAVE_LINKAT
    if ((src_dir_fd != DEFAULT_DIR_FD) || (dst_dir_fd != DEFAULT_DIR_FD)) {
        argument_unavailable_error("link", "src_dir_fd and dst_dir_fd");
        return NULL;
    }
#endif

    if ((src->narrow && dst->wide) || (src->wide && dst->narrow)) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "link: src and dst must be the same type");
        return NULL;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (src->wide)
        result = CreateHardLinkW(dst->wide, src->wide, NULL);
    else
        result = CreateHardLinkA(dst->narrow, src->narrow, NULL);
    Py_END_ALLOW_THREADS

    if (!result)
        return path_error2(src, dst);
#else
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_LINKAT
    if ((src_dir_fd != DEFAULT_DIR_FD) ||
        (dst_dir_fd != DEFAULT_DIR_FD) ||
        (!follow_symlinks))
        result = linkat(src_dir_fd, src->narrow,
            dst_dir_fd, dst->narrow,
            follow_symlinks ? AT_SYMLINK_FOLLOW : 0);
    else
#endif
        result = link(src->narrow, dst->narrow);
    Py_END_ALLOW_THREADS

    if (result)
        return path_error2(src, dst);
#endif

    Py_RETURN_NONE;
}
#endif


#if defined(MS_WINDOWS) && !defined(HAVE_OPENDIR)
static PyObject *
_listdir_windows_no_opendir(path_t *path, PyObject *list)
{
    PyObject *v;
    HANDLE hFindFile = INVALID_HANDLE_VALUE;
    BOOL result;
    WIN32_FIND_DATA FileData;
    char namebuf[MAX_PATH+4]; /* Overallocate for "\*.*" */
    char *bufptr = namebuf;
    /* only claim to have space for MAX_PATH */
    Py_ssize_t len = Py_ARRAY_LENGTH(namebuf)-4;
    PyObject *po = NULL;
    wchar_t *wnamebuf = NULL;

    if (!path->narrow) {
        WIN32_FIND_DATAW wFileData;
        wchar_t *po_wchars;

        if (!path->wide) { /* Default arg: "." */
            po_wchars = L".";
            len = 1;
        } else {
            po_wchars = path->wide;
            len = wcslen(path->wide);
        }
        /* The +5 is so we can append "\\*.*\0" */
        wnamebuf = PyMem_Malloc((len + 5) * sizeof(wchar_t));
        if (!wnamebuf) {
            PyErr_NoMemory();
            goto exit;
        }
        wcscpy(wnamebuf, po_wchars);
        if (len > 0) {
            wchar_t wch = wnamebuf[len-1];
            if (wch != SEP && wch != ALTSEP && wch != L':')
                wnamebuf[len++] = SEP;
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
            list = path_error(path);
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
                list = path_error(path);
                goto exit;
            }
        } while (result == TRUE);

        goto exit;
    }
    strcpy(namebuf, path->narrow);
    len = path->length;
    if (len > 0) {
        char ch = namebuf[len-1];
        if (ch != '\\' && ch != '/' && ch != ':')
            namebuf[len++] = '\\';
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
        list = path_error(path);
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
            list = path_error(path);
            goto exit;
        }
    } while (result == TRUE);

exit:
    if (hFindFile != INVALID_HANDLE_VALUE) {
        if (FindClose(hFindFile) == FALSE) {
            if (list != NULL) {
                Py_DECREF(list);
                list = path_error(path);
            }
        }
    }
    PyMem_Free(wnamebuf);

    return list;
}  /* end of _listdir_windows_no_opendir */

#else  /* thus POSIX, ie: not (MS_WINDOWS and not HAVE_OPENDIR) */

static PyObject *
_posix_listdir(path_t *path, PyObject *list)
{
    PyObject *v;
    DIR *dirp = NULL;
    struct dirent *ep;
    int return_str; /* if false, return bytes */
#ifdef HAVE_FDOPENDIR
    int fd = -1;
#endif

    errno = 0;
#ifdef HAVE_FDOPENDIR
    if (path->fd != -1) {
        /* closedir() closes the FD, so we duplicate it */
        fd = _Py_dup(path->fd);
        if (fd == -1)
            return NULL;

        return_str = 1;

        Py_BEGIN_ALLOW_THREADS
        dirp = fdopendir(fd);
        Py_END_ALLOW_THREADS
    }
    else
#endif
    {
        char *name;
        if (path->narrow) {
            name = path->narrow;
            /* only return bytes if they specified a bytes object */
            return_str = !(PyBytes_Check(path->object));
        }
        else {
            name = ".";
            return_str = 1;
        }

        Py_BEGIN_ALLOW_THREADS
        dirp = opendir(name);
        Py_END_ALLOW_THREADS
    }

    if (dirp == NULL) {
        list = path_error(path);
#ifdef HAVE_FDOPENDIR
        if (fd != -1) {
            Py_BEGIN_ALLOW_THREADS
            close(fd);
            Py_END_ALLOW_THREADS
        }
#endif
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
                list = path_error(path);
                goto exit;
            }
        }
        if (ep->d_name[0] == '.' &&
            (NAMLEN(ep) == 1 ||
             (ep->d_name[1] == '.' && NAMLEN(ep) == 2)))
            continue;
        if (return_str)
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
#ifdef HAVE_FDOPENDIR
        if (fd > -1)
            rewinddir(dirp);
#endif
        closedir(dirp);
        Py_END_ALLOW_THREADS
    }

    return list;
}  /* end of _posix_listdir */
#endif  /* which OS */


/*[clinic input]
os.listdir

    path : path_t(nullable=True, allow_fd='PATH_HAVE_FDOPENDIR') = None

Return a list containing the names of the files in the directory.

path can be specified as either str or bytes.  If path is bytes,
  the filenames returned will also be bytes; in all other circumstances
  the filenames returned will be str.
If path is None, uses the path='.'.
On some platforms, path may also be specified as an open file descriptor;\
  the file descriptor must refer to a directory.
  If this functionality is unavailable, using it raises NotImplementedError.

The list is in arbitrary order.  It does not include the special
entries '.' and '..' even if they are present in the directory.


[clinic start generated code]*/

PyDoc_STRVAR(os_listdir__doc__,
"listdir($module, /, path=None)\n"
"--\n"
"\n"
"Return a list containing the names of the files in the directory.\n"
"\n"
"path can be specified as either str or bytes.  If path is bytes,\n"
"  the filenames returned will also be bytes; in all other circumstances\n"
"  the filenames returned will be str.\n"
"If path is None, uses the path=\'.\'.\n"
"On some platforms, path may also be specified as an open file descriptor;\\\n"
"  the file descriptor must refer to a directory.\n"
"  If this functionality is unavailable, using it raises NotImplementedError.\n"
"\n"
"The list is in arbitrary order.  It does not include the special\n"
"entries \'.\' and \'..\' even if they are present in the directory.");

#define OS_LISTDIR_METHODDEF    \
    {"listdir", (PyCFunction)os_listdir, METH_VARARGS|METH_KEYWORDS, os_listdir__doc__},

static PyObject *
os_listdir_impl(PyModuleDef *module, path_t *path);

static PyObject *
os_listdir(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    path_t path = PATH_T_INITIALIZE("listdir", "path", 1, PATH_HAVE_FDOPENDIR);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|O&:listdir", _keywords,
        path_converter, &path))
        goto exit;
    return_value = os_listdir_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_listdir_impl(PyModuleDef *module, path_t *path)
/*[clinic end generated code: output=e159bd9be6909018 input=09e300416e3cd729]*/
{
#if defined(MS_WINDOWS) && !defined(HAVE_OPENDIR)
    return _listdir_windows_no_opendir(path, NULL);
#else
    return _posix_listdir(path, NULL);
#endif
}

#ifdef MS_WINDOWS
/* A helper function for abspath on win32 */
/* AC 3.5: probably just convert to using path converter */
static PyObject *
posix__getfullpathname(PyObject *self, PyObject *args)
{
    const char *path;
    char outbuf[MAX_PATH];
    char *temp;
    PyObject *po;

    if (PyArg_ParseTuple(args, "U|:_getfullpathname", &po))
    {
        wchar_t *wpath;
        wchar_t woutbuf[MAX_PATH], *woutbufp = woutbuf;
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
            woutbufp = PyMem_Malloc(result * sizeof(wchar_t));
            if (!woutbufp)
                return PyErr_NoMemory();
            result = GetFullPathNameW(wpath, result, woutbufp, &wtemp);
        }
        if (result)
            v = PyUnicode_FromWideChar(woutbufp, wcslen(woutbufp));
        else
            v = win32_error_object("GetFullPathNameW", po);
        if (woutbufp != woutbuf)
            PyMem_Free(woutbufp);
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
}


/*[clinic input]
os._getfinalpathname

    path: unicode
    /

A helper function for samepath on windows.
[clinic start generated code]*/

PyDoc_STRVAR(os__getfinalpathname__doc__,
"_getfinalpathname($module, path, /)\n"
"--\n"
"\n"
"A helper function for samepath on windows.");

#define OS__GETFINALPATHNAME_METHODDEF    \
    {"_getfinalpathname", (PyCFunction)os__getfinalpathname, METH_VARARGS, os__getfinalpathname__doc__},

static PyObject *
os__getfinalpathname_impl(PyModuleDef *module, PyObject *path);

static PyObject *
os__getfinalpathname(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyArg_ParseTuple(args,
        "U:_getfinalpathname",
        &path))
        goto exit;
    return_value = os__getfinalpathname_impl(module, path);

exit:
    return return_value;
}

static PyObject *
os__getfinalpathname_impl(PyModuleDef *module, PyObject *path)
/*[clinic end generated code: output=4563c6eacf1b0881 input=71d5e89334891bf4]*/
{
    HANDLE hFile;
    int buf_size;
    wchar_t *target_path;
    int result_length;
    PyObject *result;
    wchar_t *path_wchar;

    path_wchar = PyUnicode_AsUnicode(path);
    if (path_wchar == NULL)
        return NULL;

    if(!check_GetFinalPathNameByHandle()) {
        /* If the OS doesn't have GetFinalPathNameByHandle, return a
           NotImplementedError. */
        return PyErr_Format(PyExc_NotImplementedError,
            "GetFinalPathNameByHandle not available on this platform");
    }

    hFile = CreateFileW(
        path_wchar,
        0, /* desired access */
        0, /* share mode */
        NULL, /* security attributes */
        OPEN_EXISTING,
        /* FILE_FLAG_BACKUP_SEMANTICS is required to open a directory */
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);

    if(hFile == INVALID_HANDLE_VALUE)
        return win32_error_object("CreateFileW", path);

    /* We have a good handle to the target, use it to determine the
       target path name. */
    buf_size = Py_GetFinalPathNameByHandleW(hFile, 0, 0, VOLUME_NAME_NT);

    if(!buf_size)
        return win32_error_object("GetFinalPathNameByHandle", path);

    target_path = (wchar_t *)PyMem_Malloc((buf_size+1)*sizeof(wchar_t));
    if(!target_path)
        return PyErr_NoMemory();

    result_length = Py_GetFinalPathNameByHandleW(hFile, target_path,
                                                 buf_size, VOLUME_NAME_DOS);
    if(!result_length)
        return win32_error_object("GetFinalPathNamyByHandle", path);

    if(!CloseHandle(hFile))
        return win32_error_object("CloseHandle", path);

    target_path[result_length] = 0;
    result = PyUnicode_FromWideChar(target_path, result_length);
    PyMem_Free(target_path);
    return result;
}

PyDoc_STRVAR(posix__isdir__doc__,
"Return true if the pathname refers to an existing directory.");

/* AC 3.5: convert using path converter */
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


/*[clinic input]
os._getvolumepathname

    path: unicode

A helper function for ismount on Win32.
[clinic start generated code]*/

PyDoc_STRVAR(os__getvolumepathname__doc__,
"_getvolumepathname($module, /, path)\n"
"--\n"
"\n"
"A helper function for ismount on Win32.");

#define OS__GETVOLUMEPATHNAME_METHODDEF    \
    {"_getvolumepathname", (PyCFunction)os__getvolumepathname, METH_VARARGS|METH_KEYWORDS, os__getvolumepathname__doc__},

static PyObject *
os__getvolumepathname_impl(PyModuleDef *module, PyObject *path);

static PyObject *
os__getvolumepathname(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    PyObject *path;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "U:_getvolumepathname", _keywords,
        &path))
        goto exit;
    return_value = os__getvolumepathname_impl(module, path);

exit:
    return return_value;
}

static PyObject *
os__getvolumepathname_impl(PyModuleDef *module, PyObject *path)
/*[clinic end generated code: output=ac0833b6d6da7657 input=7eacadc40acbda6b]*/
{
    PyObject *result;
    wchar_t *path_wchar, *mountpath=NULL;
    size_t buflen;
    BOOL ret;

    path_wchar = PyUnicode_AsUnicodeAndSize(path, &buflen);
    if (path_wchar == NULL)
        return NULL;
    buflen += 1;

    /* Volume path should be shorter than entire path */
    buflen = Py_MAX(buflen, MAX_PATH);

    if (buflen > DWORD_MAX) {
        PyErr_SetString(PyExc_OverflowError, "path too long");
        return NULL;
    }

    mountpath = (wchar_t *)PyMem_Malloc(buflen * sizeof(wchar_t));
    if (mountpath == NULL)
        return PyErr_NoMemory();

    Py_BEGIN_ALLOW_THREADS
    ret = GetVolumePathNameW(path_wchar, mountpath,
                             Py_SAFE_DOWNCAST(buflen, size_t, DWORD));
    Py_END_ALLOW_THREADS

    if (!ret) {
        result = win32_error_object("_getvolumepathname", path);
        goto exit;
    }
    result = PyUnicode_FromWideChar(mountpath, wcslen(mountpath));

exit:
    PyMem_Free(mountpath);
    return result;
}

#endif /* MS_WINDOWS */


/*[clinic input]
os.mkdir

    path : path_t

    mode: int = 0o777

    *

    dir_fd : dir_fd(requires='mkdirat') = None

# "mkdir(path, mode=0o777, *, dir_fd=None)\n\n\

Create a directory.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.

The mode argument is ignored on Windows.
[clinic start generated code]*/

PyDoc_STRVAR(os_mkdir__doc__,
"mkdir($module, /, path, mode=511, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a directory.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.\n"
"\n"
"The mode argument is ignored on Windows.");

#define OS_MKDIR_METHODDEF    \
    {"mkdir", (PyCFunction)os_mkdir, METH_VARARGS|METH_KEYWORDS, os_mkdir__doc__},

static PyObject *
os_mkdir_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkdir(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("mkdir", "path", 0, 0);
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|i$O&:mkdir", _keywords,
        path_converter, &path, &mode, MKDIRAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_mkdir_impl(module, &path, mode, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_mkdir_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd)
/*[clinic end generated code: output=55c6ef2bc1b207e6 input=e965f68377e9b1ce]*/
{
    int result;

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path->wide)
        result = CreateDirectoryW(path->wide, NULL);
    else
        result = CreateDirectoryA(path->narrow, NULL);
    Py_END_ALLOW_THREADS

    if (!result)
        return path_error(path);
#else
    Py_BEGIN_ALLOW_THREADS
#if HAVE_MKDIRAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mkdirat(dir_fd, path->narrow, mode);
    else
#endif
#if ( defined(__WATCOMC__) || defined(PYCC_VACPP) ) && !defined(__QNX__)
        result = mkdir(path->narrow);
#else
        result = mkdir(path->narrow, mode);
#endif
    Py_END_ALLOW_THREADS
    if (result < 0)
        return path_error(path);
#endif
    Py_RETURN_NONE;
}


/* sys/resource.h is needed for at least: wait3(), wait4(), broken nice. */
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif


#ifdef HAVE_NICE
/*[clinic input]
os.nice

    increment: int
    /

Add increment to the priority of process and return the new priority.
[clinic start generated code]*/

PyDoc_STRVAR(os_nice__doc__,
"nice($module, increment, /)\n"
"--\n"
"\n"
"Add increment to the priority of process and return the new priority.");

#define OS_NICE_METHODDEF    \
    {"nice", (PyCFunction)os_nice, METH_VARARGS, os_nice__doc__},

static PyObject *
os_nice_impl(PyModuleDef *module, int increment);

static PyObject *
os_nice(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int increment;

    if (!PyArg_ParseTuple(args,
        "i:nice",
        &increment))
        goto exit;
    return_value = os_nice_impl(module, increment);

exit:
    return return_value;
}

static PyObject *
os_nice_impl(PyModuleDef *module, int increment)
/*[clinic end generated code: output=c360dc2a3bd8e3d0 input=864be2d402a21da2]*/
{
    int value;

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
/*[clinic input]
os.getpriority

    which: int
    who: int

Return program scheduling priority.
[clinic start generated code]*/

PyDoc_STRVAR(os_getpriority__doc__,
"getpriority($module, /, which, who)\n"
"--\n"
"\n"
"Return program scheduling priority.");

#define OS_GETPRIORITY_METHODDEF    \
    {"getpriority", (PyCFunction)os_getpriority, METH_VARARGS|METH_KEYWORDS, os_getpriority__doc__},

static PyObject *
os_getpriority_impl(PyModuleDef *module, int which, int who);

static PyObject *
os_getpriority(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"which", "who", NULL};
    int which;
    int who;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "ii:getpriority", _keywords,
        &which, &who))
        goto exit;
    return_value = os_getpriority_impl(module, which, who);

exit:
    return return_value;
}

static PyObject *
os_getpriority_impl(PyModuleDef *module, int which, int who)
/*[clinic end generated code: output=81639cf765f05dae input=9be615d40e2544ef]*/
{
    int retval;

    errno = 0;
    retval = getpriority(which, who);
    if (errno != 0)
        return posix_error();
    return PyLong_FromLong((long)retval);
}
#endif /* HAVE_GETPRIORITY */


#ifdef HAVE_SETPRIORITY
/*[clinic input]
os.setpriority

    which: int
    who: int
    priority: int

Set program scheduling priority.
[clinic start generated code]*/

PyDoc_STRVAR(os_setpriority__doc__,
"setpriority($module, /, which, who, priority)\n"
"--\n"
"\n"
"Set program scheduling priority.");

#define OS_SETPRIORITY_METHODDEF    \
    {"setpriority", (PyCFunction)os_setpriority, METH_VARARGS|METH_KEYWORDS, os_setpriority__doc__},

static PyObject *
os_setpriority_impl(PyModuleDef *module, int which, int who, int priority);

static PyObject *
os_setpriority(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"which", "who", "priority", NULL};
    int which;
    int who;
    int priority;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "iii:setpriority", _keywords,
        &which, &who, &priority))
        goto exit;
    return_value = os_setpriority_impl(module, which, who, priority);

exit:
    return return_value;
}

static PyObject *
os_setpriority_impl(PyModuleDef *module, int which, int who, int priority)
/*[clinic end generated code: output=ddad62651fb2120c input=710ccbf65b9dc513]*/
{
    int retval;

    retval = setpriority(which, who, priority);
    if (retval == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETPRIORITY */


static PyObject *
internal_rename(path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd, int is_replace)
{
    char *function_name = is_replace ? "replace" : "rename";
    int dir_fd_specified;

#ifdef MS_WINDOWS
    BOOL result;
    int flags = is_replace ? MOVEFILE_REPLACE_EXISTING : 0;
#else
    int result;
#endif

    dir_fd_specified = (src_dir_fd != DEFAULT_DIR_FD) ||
                       (dst_dir_fd != DEFAULT_DIR_FD);
#ifndef HAVE_RENAMEAT
    if (dir_fd_specified) {
        argument_unavailable_error(function_name, "src_dir_fd and dst_dir_fd");
        return NULL;
    }
#endif

    if ((src->narrow && dst->wide) || (src->wide && dst->narrow)) {
        PyErr_Format(PyExc_ValueError,
                     "%s: src and dst must be the same type", function_name);
        return NULL;
    }

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (src->wide)
        result = MoveFileExW(src->wide, dst->wide, flags);
    else
        result = MoveFileExA(src->narrow, dst->narrow, flags);
    Py_END_ALLOW_THREADS

    if (!result)
        return path_error2(src, dst);

#else
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_RENAMEAT
    if (dir_fd_specified)
        result = renameat(src_dir_fd, src->narrow, dst_dir_fd, dst->narrow);
    else
#endif
        result = rename(src->narrow, dst->narrow);
    Py_END_ALLOW_THREADS

    if (result)
        return path_error2(src, dst);
#endif
    Py_RETURN_NONE;
}


/*[clinic input]
os.rename

    src : path_t
    dst : path_t
    *
    src_dir_fd : dir_fd = None
    dst_dir_fd : dir_fd = None

Rename a file or directory.

If either src_dir_fd or dst_dir_fd is not None, it should be a file
  descriptor open to a directory, and the respective path string (src or dst)
  should be relative; the path will then be relative to that directory.
src_dir_fd and dst_dir_fd, may not be implemented on your platform.
  If they are unavailable, using them will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_rename__doc__,
"rename($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n"
"--\n"
"\n"
"Rename a file or directory.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_RENAME_METHODDEF    \
    {"rename", (PyCFunction)os_rename, METH_VARARGS|METH_KEYWORDS, os_rename__doc__},

static PyObject *
os_rename_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd);

static PyObject *
os_rename(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    path_t src = PATH_T_INITIALIZE("rename", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("rename", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|$O&O&:rename", _keywords,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd))
        goto exit;
    return_value = os_rename_impl(module, &src, &dst, src_dir_fd, dst_dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

static PyObject *
os_rename_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd)
/*[clinic end generated code: output=c936bdc81f460a1e input=faa61c847912c850]*/
{
    return internal_rename(src, dst, src_dir_fd, dst_dir_fd, 0);
}


/*[clinic input]
os.replace = os.rename

Rename a file or directory, overwriting the destination.

If either src_dir_fd or dst_dir_fd is not None, it should be a file
  descriptor open to a directory, and the respective path string (src or dst)
  should be relative; the path will then be relative to that directory.
src_dir_fd and dst_dir_fd, may not be implemented on your platform.
  If they are unavailable, using them will raise a NotImplementedError."
[clinic start generated code]*/

PyDoc_STRVAR(os_replace__doc__,
"replace($module, /, src, dst, *, src_dir_fd=None, dst_dir_fd=None)\n"
"--\n"
"\n"
"Rename a file or directory, overwriting the destination.\n"
"\n"
"If either src_dir_fd or dst_dir_fd is not None, it should be a file\n"
"  descriptor open to a directory, and the respective path string (src or dst)\n"
"  should be relative; the path will then be relative to that directory.\n"
"src_dir_fd and dst_dir_fd, may not be implemented on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.\"");

#define OS_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)os_replace, METH_VARARGS|METH_KEYWORDS, os_replace__doc__},

static PyObject *
os_replace_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd);

static PyObject *
os_replace(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"src", "dst", "src_dir_fd", "dst_dir_fd", NULL};
    path_t src = PATH_T_INITIALIZE("replace", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("replace", "dst", 0, 0);
    int src_dir_fd = DEFAULT_DIR_FD;
    int dst_dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|$O&O&:replace", _keywords,
        path_converter, &src, path_converter, &dst, dir_fd_converter, &src_dir_fd, dir_fd_converter, &dst_dir_fd))
        goto exit;
    return_value = os_replace_impl(module, &src, &dst, src_dir_fd, dst_dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

static PyObject *
os_replace_impl(PyModuleDef *module, path_t *src, path_t *dst, int src_dir_fd, int dst_dir_fd)
/*[clinic end generated code: output=224e4710d290d171 input=25515dfb107c8421]*/
{
    return internal_rename(src, dst, src_dir_fd, dst_dir_fd, 1);
}


/*[clinic input]
os.rmdir

    path: path_t
    *
    dir_fd: dir_fd(requires='unlinkat') = None

Remove a directory.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_rmdir__doc__,
"rmdir($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a directory.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_RMDIR_METHODDEF    \
    {"rmdir", (PyCFunction)os_rmdir, METH_VARARGS|METH_KEYWORDS, os_rmdir__doc__},

static PyObject *
os_rmdir_impl(PyModuleDef *module, path_t *path, int dir_fd);

static PyObject *
os_rmdir(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("rmdir", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|$O&:rmdir", _keywords,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_rmdir_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_rmdir_impl(PyModuleDef *module, path_t *path, int dir_fd)
/*[clinic end generated code: output=70b9fdbe3bee0591 input=38c8b375ca34a7e2]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path->wide)
        result = RemoveDirectoryW(path->wide);
    else
        result = RemoveDirectoryA(path->narrow);
    result = !result; /* Windows, success=1, UNIX, success=0 */
#else
#ifdef HAVE_UNLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = unlinkat(dir_fd, path->narrow, AT_REMOVEDIR);
    else
#endif
        result = rmdir(path->narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result)
        return path_error(path);

    Py_RETURN_NONE;
}


#ifdef HAVE_SYSTEM
#ifdef MS_WINDOWS
/*[clinic input]
os.system -> long

    command: Py_UNICODE

Execute the command in a subshell.
[clinic start generated code]*/

PyDoc_STRVAR(os_system__doc__,
"system($module, /, command)\n"
"--\n"
"\n"
"Execute the command in a subshell.");

#define OS_SYSTEM_METHODDEF    \
    {"system", (PyCFunction)os_system, METH_VARARGS|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyModuleDef *module, Py_UNICODE *command);

static PyObject *
os_system(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"command", NULL};
    Py_UNICODE *command;
    long _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "u:system", _keywords,
        &command))
        goto exit;
    _return_value = os_system_impl(module, command);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

static long
os_system_impl(PyModuleDef *module, Py_UNICODE *command)
/*[clinic end generated code: output=29fe699c0b2e9d38 input=303f5ce97df606b0]*/
{
    long result;
    Py_BEGIN_ALLOW_THREADS
    result = _wsystem(command);
    Py_END_ALLOW_THREADS
    return result;
}
#else /* MS_WINDOWS */
/*[clinic input]
os.system -> long

    command: FSConverter

Execute the command in a subshell.
[clinic start generated code]*/

PyDoc_STRVAR(os_system__doc__,
"system($module, /, command)\n"
"--\n"
"\n"
"Execute the command in a subshell.");

#define OS_SYSTEM_METHODDEF    \
    {"system", (PyCFunction)os_system, METH_VARARGS|METH_KEYWORDS, os_system__doc__},

static long
os_system_impl(PyModuleDef *module, PyObject *command);

static PyObject *
os_system(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"command", NULL};
    PyObject *command = NULL;
    long _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:system", _keywords,
        PyUnicode_FSConverter, &command))
        goto exit;
    _return_value = os_system_impl(module, command);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong(_return_value);

exit:
    /* Cleanup for command */
    Py_XDECREF(command);

    return return_value;
}

static long
os_system_impl(PyModuleDef *module, PyObject *command)
/*[clinic end generated code: output=5be9f3c40ead3bad input=86a58554ba6094af]*/
{
    long result;
    char *bytes = PyBytes_AsString(command);
    Py_BEGIN_ALLOW_THREADS
    result = system(bytes);
    Py_END_ALLOW_THREADS
    return result;
}
#endif
#endif /* HAVE_SYSTEM */


/*[clinic input]
os.umask

    mask: int
    /

Set the current numeric umask and return the previous umask.
[clinic start generated code]*/

PyDoc_STRVAR(os_umask__doc__,
"umask($module, mask, /)\n"
"--\n"
"\n"
"Set the current numeric umask and return the previous umask.");

#define OS_UMASK_METHODDEF    \
    {"umask", (PyCFunction)os_umask, METH_VARARGS, os_umask__doc__},

static PyObject *
os_umask_impl(PyModuleDef *module, int mask);

static PyObject *
os_umask(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int mask;

    if (!PyArg_ParseTuple(args,
        "i:umask",
        &mask))
        goto exit;
    return_value = os_umask_impl(module, mask);

exit:
    return return_value;
}

static PyObject *
os_umask_impl(PyModuleDef *module, int mask)
/*[clinic end generated code: output=90048b39d2d4a961 input=ab6bfd9b24d8a7e8]*/
{
    int i = (int)umask(mask);
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
                /* IO_REPARSE_TAG_SYMLINK if it is a symlink and
                   IO_REPARSE_TAG_MOUNT_POINT if it is a junction point. */
                is_link = find_data.dwReserved0 == IO_REPARSE_TAG_SYMLINK ||
                          find_data.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT;
                FindClose(find_data_handle);
            }
        }
    }

    if (is_directory && is_link)
        return RemoveDirectoryW(lpFileName);

    return DeleteFileW(lpFileName);
}
#endif /* MS_WINDOWS */


/*[clinic input]
os.unlink

    path: path_t
    *
    dir_fd: dir_fd(requires='unlinkat')=None

Remove a file (same as remove()).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_unlink__doc__,
"unlink($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a file (same as remove()).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_UNLINK_METHODDEF    \
    {"unlink", (PyCFunction)os_unlink, METH_VARARGS|METH_KEYWORDS, os_unlink__doc__},

static PyObject *
os_unlink_impl(PyModuleDef *module, path_t *path, int dir_fd);

static PyObject *
os_unlink(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("unlink", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|$O&:unlink", _keywords,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_unlink_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_unlink_impl(PyModuleDef *module, path_t *path, int dir_fd)
/*[clinic end generated code: output=59a6e66d67ff2e75 input=d7bcde2b1b2a2552]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path->wide)
        result = Py_DeleteFileW(path->wide);
    else
        result = DeleteFileA(path->narrow);
    result = !result; /* Windows, success=1, UNIX, success=0 */
#else
#ifdef HAVE_UNLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = unlinkat(dir_fd, path->narrow, 0);
    else
#endif /* HAVE_UNLINKAT */
        result = unlink(path->narrow);
#endif
    Py_END_ALLOW_THREADS

    if (result)
        return path_error(path);

    Py_RETURN_NONE;
}


/*[clinic input]
os.remove = os.unlink

Remove a file (same as unlink()).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_remove__doc__,
"remove($module, /, path, *, dir_fd=None)\n"
"--\n"
"\n"
"Remove a file (same as unlink()).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_REMOVE_METHODDEF    \
    {"remove", (PyCFunction)os_remove, METH_VARARGS|METH_KEYWORDS, os_remove__doc__},

static PyObject *
os_remove_impl(PyModuleDef *module, path_t *path, int dir_fd);

static PyObject *
os_remove(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("remove", "path", 0, 0);
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|$O&:remove", _keywords,
        path_converter, &path, UNLINKAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_remove_impl(module, &path, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_remove_impl(PyModuleDef *module, path_t *path, int dir_fd)
/*[clinic end generated code: output=cb170cf1e195b8ed input=e05c5ab55cd30983]*/
{
    return os_unlink_impl(module, path, dir_fd);
}


static PyStructSequence_Field uname_result_fields[] = {
    {"sysname",    "operating system name"},
    {"nodename",   "name of machine on network (implementation-defined)"},
    {"release",    "operating system release"},
    {"version",    "operating system version"},
    {"machine",    "hardware identifier"},
    {NULL}
};

PyDoc_STRVAR(uname_result__doc__,
"uname_result: Result from os.uname().\n\n\
This object may be accessed either as a tuple of\n\
  (sysname, nodename, release, version, machine),\n\
or via the attributes sysname, nodename, release, version, and machine.\n\
\n\
See os.uname for more information.");

static PyStructSequence_Desc uname_result_desc = {
    "uname_result", /* name */
    uname_result__doc__, /* doc */
    uname_result_fields,
    5
};

static PyTypeObject UnameResultType;


#ifdef HAVE_UNAME
/*[clinic input]
os.uname

Return an object identifying the current operating system.

The object behaves like a named tuple with the following fields:
  (sysname, nodename, release, version, machine)

[clinic start generated code]*/

PyDoc_STRVAR(os_uname__doc__,
"uname($module, /)\n"
"--\n"
"\n"
"Return an object identifying the current operating system.\n"
"\n"
"The object behaves like a named tuple with the following fields:\n"
"  (sysname, nodename, release, version, machine)");

#define OS_UNAME_METHODDEF    \
    {"uname", (PyCFunction)os_uname, METH_NOARGS, os_uname__doc__},

static PyObject *
os_uname_impl(PyModuleDef *module);

static PyObject *
os_uname(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_uname_impl(module);
}

static PyObject *
os_uname_impl(PyModuleDef *module)
/*[clinic end generated code: output=459a86521ff5041c input=e68bd246db3043ed]*/
{
    struct utsname u;
    int res;
    PyObject *value;

    Py_BEGIN_ALLOW_THREADS
    res = uname(&u);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();

    value = PyStructSequence_New(&UnameResultType);
    if (value == NULL)
        return NULL;

#define SET(i, field) \
    { \
    PyObject *o = PyUnicode_DecodeFSDefault(field); \
    if (!o) { \
        Py_DECREF(value); \
        return NULL; \
    } \
    PyStructSequence_SET_ITEM(value, i, o); \
    } \

    SET(0, u.sysname);
    SET(1, u.nodename);
    SET(2, u.release);
    SET(3, u.version);
    SET(4, u.machine);

#undef SET

    return value;
}
#endif /* HAVE_UNAME */



typedef struct {
    int    now;
    time_t atime_s;
    long   atime_ns;
    time_t mtime_s;
    long   mtime_ns;
} utime_t;

/*
 * these macros assume that "ut" is a pointer to a utime_t
 * they also intentionally leak the declaration of a pointer named "time"
 */
#define UTIME_TO_TIMESPEC \
    struct timespec ts[2]; \
    struct timespec *time; \
    if (ut->now) \
        time = NULL; \
    else { \
        ts[0].tv_sec = ut->atime_s; \
        ts[0].tv_nsec = ut->atime_ns; \
        ts[1].tv_sec = ut->mtime_s; \
        ts[1].tv_nsec = ut->mtime_ns; \
        time = ts; \
    } \

#define UTIME_TO_TIMEVAL \
    struct timeval tv[2]; \
    struct timeval *time; \
    if (ut->now) \
        time = NULL; \
    else { \
        tv[0].tv_sec = ut->atime_s; \
        tv[0].tv_usec = ut->atime_ns / 1000; \
        tv[1].tv_sec = ut->mtime_s; \
        tv[1].tv_usec = ut->mtime_ns / 1000; \
        time = tv; \
    } \

#define UTIME_TO_UTIMBUF \
    struct utimbuf u; \
    struct utimbuf *time; \
    if (ut->now) \
        time = NULL; \
    else { \
        u.actime = ut->atime_s; \
        u.modtime = ut->mtime_s; \
        time = &u; \
    }

#define UTIME_TO_TIME_T \
    time_t timet[2]; \
    time_t *time; \
    if (ut->now) \
        time = NULL; \
    else { \
        timet[0] = ut->atime_s; \
        timet[1] = ut->mtime_s; \
        time = timet; \
    } \


#define UTIME_HAVE_DIR_FD (defined(HAVE_FUTIMESAT) || defined(HAVE_UTIMENSAT))

#if UTIME_HAVE_DIR_FD

static int
utime_dir_fd(utime_t *ut, int dir_fd, char *path, int follow_symlinks)
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

    #define FUTIMENSAT_DIR_FD_CONVERTER dir_fd_converter
#else
    #define FUTIMENSAT_DIR_FD_CONVERTER dir_fd_unavailable
#endif

#define UTIME_HAVE_FD (defined(HAVE_FUTIMES) || defined(HAVE_FUTIMENS))

#if UTIME_HAVE_FD

static int
utime_fd(utime_t *ut, int fd)
{
#ifdef HAVE_FUTIMENS
    UTIME_TO_TIMESPEC;
    return futimens(fd, time);
#else
    UTIME_TO_TIMEVAL;
    return futimes(fd, time);
#endif
}

    #define PATH_UTIME_HAVE_FD 1
#else
    #define PATH_UTIME_HAVE_FD 0
#endif


#define UTIME_HAVE_NOFOLLOW_SYMLINKS \
        (defined(HAVE_UTIMENSAT) || defined(HAVE_LUTIMES))

#if UTIME_HAVE_NOFOLLOW_SYMLINKS

static int
utime_nofollow_symlinks(utime_t *ut, char *path)
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
utime_default(utime_t *ut, char *path)
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


/*[clinic input]
os.utime

    path: path_t(allow_fd='PATH_UTIME_HAVE_FD')
    times: object = NULL
    *
    ns: object = NULL
    dir_fd: dir_fd(requires='futimensat') = None
    follow_symlinks: bool=True

# "utime(path, times=None, *, ns=None, dir_fd=None, follow_symlinks=True)\n\

Set the access and modified time of path.

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.

If times is not None, it must be a tuple (atime, mtime);
    atime and mtime should be expressed as float seconds since the epoch.
If ns is not None, it must be a tuple (atime_ns, mtime_ns);
    atime_ns and mtime_ns should be expressed as integer nanoseconds
    since the epoch.
If both times and ns are None, utime uses the current time.
Specifying tuples for both times and ns is an error.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, utime will modify the symbolic link itself instead of the file the
  link points to.
It is an error to use dir_fd or follow_symlinks when specifying path
  as an open file descriptor.
dir_fd and follow_symlinks may not be available on your platform.
  If they are unavailable, using them will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_utime__doc__,
"utime($module, /, path, times=None, *, ns=None, dir_fd=None,\n"
"      follow_symlinks=True)\n"
"--\n"
"\n"
"Set the access and modified time of path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.\n"
"\n"
"If times is not None, it must be a tuple (atime, mtime);\n"
"    atime and mtime should be expressed as float seconds since the epoch.\n"
"If ns is not None, it must be a tuple (atime_ns, mtime_ns);\n"
"    atime_ns and mtime_ns should be expressed as integer nanoseconds\n"
"    since the epoch.\n"
"If both times and ns are None, utime uses the current time.\n"
"Specifying tuples for both times and ns is an error.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, utime will modify the symbolic link itself instead of the file the\n"
"  link points to.\n"
"It is an error to use dir_fd or follow_symlinks when specifying path\n"
"  as an open file descriptor.\n"
"dir_fd and follow_symlinks may not be available on your platform.\n"
"  If they are unavailable, using them will raise a NotImplementedError.");

#define OS_UTIME_METHODDEF    \
    {"utime", (PyCFunction)os_utime, METH_VARARGS|METH_KEYWORDS, os_utime__doc__},

static PyObject *
os_utime_impl(PyModuleDef *module, path_t *path, PyObject *times, PyObject *ns, int dir_fd, int follow_symlinks);

static PyObject *
os_utime(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "times", "ns", "dir_fd", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("utime", "path", 0, PATH_UTIME_HAVE_FD);
    PyObject *times = NULL;
    PyObject *ns = NULL;
    int dir_fd = DEFAULT_DIR_FD;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|O$OO&p:utime", _keywords,
        path_converter, &path, &times, &ns, FUTIMENSAT_DIR_FD_CONVERTER, &dir_fd, &follow_symlinks))
        goto exit;
    return_value = os_utime_impl(module, &path, times, ns, dir_fd, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_utime_impl(PyModuleDef *module, path_t *path, PyObject *times, PyObject *ns, int dir_fd, int follow_symlinks)
/*[clinic end generated code: output=891489c35cc68c5d input=1f18c17d5941aa82]*/
{
#ifdef MS_WINDOWS
    HANDLE hFile;
    FILETIME atime, mtime;
#else
    int result;
#endif

    PyObject *return_value = NULL;
    utime_t utime;

    memset(&utime, 0, sizeof(utime_t));

    if (times && (times != Py_None) && ns) {
        PyErr_SetString(PyExc_ValueError,
                     "utime: you may specify either 'times'"
                     " or 'ns' but not both");
        goto exit;
    }

    if (times && (times != Py_None)) {
        time_t a_sec, m_sec;
        long a_nsec, m_nsec;
        if (!PyTuple_CheckExact(times) || (PyTuple_Size(times) != 2)) {
            PyErr_SetString(PyExc_TypeError,
                         "utime: 'times' must be either"
                         " a tuple of two ints or None");
            goto exit;
        }
        utime.now = 0;
        if (_PyTime_ObjectToTimespec(PyTuple_GET_ITEM(times, 0),
                                     &a_sec, &a_nsec, _PyTime_ROUND_DOWN) == -1 ||
            _PyTime_ObjectToTimespec(PyTuple_GET_ITEM(times, 1),
                                     &m_sec, &m_nsec, _PyTime_ROUND_DOWN) == -1) {
            goto exit;
        }
        utime.atime_s = a_sec;
        utime.atime_ns = a_nsec;
        utime.mtime_s = m_sec;
        utime.mtime_ns = m_nsec;
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

    if (path_and_dir_fd_invalid("utime", path, dir_fd) ||
        dir_fd_and_fd_invalid("utime", dir_fd, path->fd) ||
        fd_and_follow_symlinks_invalid("utime", path->fd, follow_symlinks))
        goto exit;

#if !defined(HAVE_UTIMENSAT)
    if ((dir_fd != DEFAULT_DIR_FD) && (!follow_symlinks)) {
        PyErr_SetString(PyExc_ValueError,
                     "utime: cannot use dir_fd and follow_symlinks "
                     "together on this platform");
        goto exit;
    }
#endif

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (path->wide)
        hFile = CreateFileW(path->wide, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
    else
        hFile = CreateFileA(path->narrow, FILE_WRITE_ATTRIBUTES, 0,
                            NULL, OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS, NULL);
    Py_END_ALLOW_THREADS
    if (hFile == INVALID_HANDLE_VALUE) {
        path_error(path);
        goto exit;
    }

    if (utime.now) {
        GetSystemTimeAsFileTime(&mtime);
        atime = mtime;
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
        PyErr_SetFromWindowsErr(0);
        goto exit;
    }
#else /* MS_WINDOWS */
    Py_BEGIN_ALLOW_THREADS

#if UTIME_HAVE_NOFOLLOW_SYMLINKS
    if ((!follow_symlinks) && (dir_fd == DEFAULT_DIR_FD))
        result = utime_nofollow_symlinks(&utime, path->narrow);
    else
#endif

#if UTIME_HAVE_DIR_FD
    if ((dir_fd != DEFAULT_DIR_FD) || (!follow_symlinks))
        result = utime_dir_fd(&utime, dir_fd, path->narrow, follow_symlinks);
    else
#endif

#if UTIME_HAVE_FD
    if (path->fd != -1)
        result = utime_fd(&utime, path->fd);
    else
#endif

    result = utime_default(&utime, path->narrow);

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
#ifdef MS_WINDOWS
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
#endif
    return return_value;
}

/* Process operations */


/*[clinic input]
os._exit

    status: int

Exit to the system with specified status, without normal exit processing.
[clinic start generated code]*/

PyDoc_STRVAR(os__exit__doc__,
"_exit($module, /, status)\n"
"--\n"
"\n"
"Exit to the system with specified status, without normal exit processing.");

#define OS__EXIT_METHODDEF    \
    {"_exit", (PyCFunction)os__exit, METH_VARARGS|METH_KEYWORDS, os__exit__doc__},

static PyObject *
os__exit_impl(PyModuleDef *module, int status);

static PyObject *
os__exit(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:_exit", _keywords,
        &status))
        goto exit;
    return_value = os__exit_impl(module, status);

exit:
    return return_value;
}

static PyObject *
os__exit_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=4f9858c4cc2dcb89 input=5e6d57556b0c4a62]*/
{
    _exit(status);
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
    if (!*out) {
        PyErr_NoMemory();
        return 0;
    }
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
    if (!keys)
        goto error;
    vals = PyMapping_Values(env);
    if (!vals)
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
/*[clinic input]
os.execv

    path: FSConverter
        Path of executable file.
    argv: object
        Tuple or list of strings.
    /

Execute an executable path with arguments, replacing current process.
[clinic start generated code]*/

PyDoc_STRVAR(os_execv__doc__,
"execv($module, path, argv, /)\n"
"--\n"
"\n"
"Execute an executable path with arguments, replacing current process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.");

#define OS_EXECV_METHODDEF    \
    {"execv", (PyCFunction)os_execv, METH_VARARGS, os_execv__doc__},

static PyObject *
os_execv_impl(PyModuleDef *module, PyObject *path, PyObject *argv);

static PyObject *
os_execv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *path = NULL;
    PyObject *argv;

    if (!PyArg_ParseTuple(args,
        "O&O:execv",
        PyUnicode_FSConverter, &path, &argv))
        goto exit;
    return_value = os_execv_impl(module, path, argv);

exit:
    /* Cleanup for path */
    Py_XDECREF(path);

    return return_value;
}

static PyObject *
os_execv_impl(PyModuleDef *module, PyObject *path, PyObject *argv)
/*[clinic end generated code: output=b0f5f2caa6097edc input=96041559925e5229]*/
{
    char *path_char;
    char **argvlist;
    Py_ssize_t argc;

    /* execv has two arguments: (path, argv), where
       argv is a list or tuple of strings. */

    path_char = PyBytes_AsString(path);
    if (!PyList_Check(argv) && !PyTuple_Check(argv)) {
        PyErr_SetString(PyExc_TypeError,
                        "execv() arg 2 must be a tuple or list");
        return NULL;
    }
    argc = PySequence_Size(argv);
    if (argc < 1) {
        PyErr_SetString(PyExc_ValueError, "execv() arg 2 must not be empty");
        return NULL;
    }

    argvlist = parse_arglist(argv, &argc);
    if (argvlist == NULL) {
        return NULL;
    }

    execv(path_char, argvlist);

    /* If we get here it's definitely an error */

    free_string_array(argvlist, argc);
    return posix_error();
}


/*[clinic input]
os.execve

    path: path_t(allow_fd='PATH_HAVE_FEXECVE')
        Path of executable file.
    argv: object
        Tuple or list of strings.
    env: object
        Dictionary of strings mapping to strings.

Execute an executable path with arguments, replacing current process.
[clinic start generated code]*/

PyDoc_STRVAR(os_execve__doc__,
"execve($module, /, path, argv, env)\n"
"--\n"
"\n"
"Execute an executable path with arguments, replacing current process.\n"
"\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.");

#define OS_EXECVE_METHODDEF    \
    {"execve", (PyCFunction)os_execve, METH_VARARGS|METH_KEYWORDS, os_execve__doc__},

static PyObject *
os_execve_impl(PyModuleDef *module, path_t *path, PyObject *argv, PyObject *env);

static PyObject *
os_execve(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "argv", "env", NULL};
    path_t path = PATH_T_INITIALIZE("execve", "path", 0, PATH_HAVE_FEXECVE);
    PyObject *argv;
    PyObject *env;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&OO:execve", _keywords,
        path_converter, &path, &argv, &env))
        goto exit;
    return_value = os_execve_impl(module, &path, argv, env);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_execve_impl(PyModuleDef *module, path_t *path, PyObject *argv, PyObject *env)
/*[clinic end generated code: output=fb283760f5d15ab7 input=626804fa092606d9]*/
{
    char **argvlist = NULL;
    char **envlist;
    Py_ssize_t argc, envc;

    /* execve has three arguments: (path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

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
    if (path->fd > -1)
        fexecve(path->fd, argvlist, envlist);
    else
#endif
        execve(path->narrow, argvlist, envlist);

    /* If we get here it's definitely an error */

    path_error(path);

    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail:
    if (argvlist)
        free_string_array(argvlist, argc);
    return NULL;
}
#endif /* HAVE_EXECV */


#ifdef HAVE_SPAWNV
/*[clinic input]
os.spawnv

    mode: int
        Mode of process creation.
    path: FSConverter
        Path of executable file.
    argv: object
        Tuple or list of strings.
    /

Execute the program specified by path in a new process.
[clinic start generated code]*/

PyDoc_STRVAR(os_spawnv__doc__,
"spawnv($module, mode, path, argv, /)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  mode\n"
"    Mode of process creation.\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.");

#define OS_SPAWNV_METHODDEF    \
    {"spawnv", (PyCFunction)os_spawnv, METH_VARARGS, os_spawnv__doc__},

static PyObject *
os_spawnv_impl(PyModuleDef *module, int mode, PyObject *path, PyObject *argv);

static PyObject *
os_spawnv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int mode;
    PyObject *path = NULL;
    PyObject *argv;

    if (!PyArg_ParseTuple(args,
        "iO&O:spawnv",
        &mode, PyUnicode_FSConverter, &path, &argv))
        goto exit;
    return_value = os_spawnv_impl(module, mode, path, argv);

exit:
    /* Cleanup for path */
    Py_XDECREF(path);

    return return_value;
}

static PyObject *
os_spawnv_impl(PyModuleDef *module, int mode, PyObject *path, PyObject *argv)
/*[clinic end generated code: output=dfee6be062e780e3 input=042c91dfc1e6debc]*/
{
    char *path_char;
    char **argvlist;
    int i;
    Py_ssize_t argc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);

    /* spawnv has three arguments: (mode, path, argv), where
       argv is a list or tuple of strings. */

    path_char = PyBytes_AsString(path);
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
        return NULL;
    }

    argvlist = PyMem_NEW(char *, argc+1);
    if (argvlist == NULL) {
        return PyErr_NoMemory();
    }
    for (i = 0; i < argc; i++) {
        if (!fsconvert_strdup((*getitem)(argv, i),
                              &argvlist[i])) {
            free_string_array(argvlist, i);
            PyErr_SetString(
                PyExc_TypeError,
                "spawnv() arg 2 must contain only strings");
            return NULL;
        }
    }
    argvlist[argc] = NULL;

    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnv(mode, path_char, argvlist);
    Py_END_ALLOW_THREADS

    free_string_array(argvlist, argc);

    if (spawnval == -1)
        return posix_error();
    else
        return Py_BuildValue(_Py_PARSE_INTPTR, spawnval);
}


/*[clinic input]
os.spawnve

    mode: int
        Mode of process creation.
    path: FSConverter
        Path of executable file.
    argv: object
        Tuple or list of strings.
    env: object
        Dictionary of strings mapping to strings.
    /

Execute the program specified by path in a new process.
[clinic start generated code]*/

PyDoc_STRVAR(os_spawnve__doc__,
"spawnve($module, mode, path, argv, env, /)\n"
"--\n"
"\n"
"Execute the program specified by path in a new process.\n"
"\n"
"  mode\n"
"    Mode of process creation.\n"
"  path\n"
"    Path of executable file.\n"
"  argv\n"
"    Tuple or list of strings.\n"
"  env\n"
"    Dictionary of strings mapping to strings.");

#define OS_SPAWNVE_METHODDEF    \
    {"spawnve", (PyCFunction)os_spawnve, METH_VARARGS, os_spawnve__doc__},

static PyObject *
os_spawnve_impl(PyModuleDef *module, int mode, PyObject *path, PyObject *argv, PyObject *env);

static PyObject *
os_spawnve(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int mode;
    PyObject *path = NULL;
    PyObject *argv;
    PyObject *env;

    if (!PyArg_ParseTuple(args,
        "iO&OO:spawnve",
        &mode, PyUnicode_FSConverter, &path, &argv, &env))
        goto exit;
    return_value = os_spawnve_impl(module, mode, path, argv, env);

exit:
    /* Cleanup for path */
    Py_XDECREF(path);

    return return_value;
}

static PyObject *
os_spawnve_impl(PyModuleDef *module, int mode, PyObject *path, PyObject *argv, PyObject *env)
/*[clinic end generated code: output=6f7df38473f63c7c input=02362fd937963f8f]*/
{
    char *path_char;
    char **argvlist;
    char **envlist;
    PyObject *res = NULL;
    Py_ssize_t argc, i, envc;
    Py_intptr_t spawnval;
    PyObject *(*getitem)(PyObject *, Py_ssize_t);
    Py_ssize_t lastarg = 0;

    /* spawnve has four arguments: (mode, path, argv, env), where
       argv is a list or tuple of strings and env is a dictionary
       like posix.environ. */

    path_char = PyBytes_AsString(path);
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

    if (mode == _OLD_P_OVERLAY)
        mode = _P_OVERLAY;

    Py_BEGIN_ALLOW_THREADS
    spawnval = _spawnve(mode, path_char, argvlist, envlist);
    Py_END_ALLOW_THREADS

    if (spawnval == -1)
        (void) posix_error();
    else
        res = Py_BuildValue(_Py_PARSE_INTPTR, spawnval);

    while (--envc >= 0)
        PyMem_DEL(envlist[envc]);
    PyMem_DEL(envlist);
  fail_1:
    free_string_array(argvlist, lastarg);
  fail_0:
    return res;
}

#endif /* HAVE_SPAWNV */


#ifdef HAVE_FORK1
/*[clinic input]
os.fork1

Fork a child process with a single multiplexed (i.e., not bound) thread.

Return 0 to child process and PID of child to parent process.
[clinic start generated code]*/

PyDoc_STRVAR(os_fork1__doc__,
"fork1($module, /)\n"
"--\n"
"\n"
"Fork a child process with a single multiplexed (i.e., not bound) thread.\n"
"\n"
"Return 0 to child process and PID of child to parent process.");

#define OS_FORK1_METHODDEF    \
    {"fork1", (PyCFunction)os_fork1, METH_NOARGS, os_fork1__doc__},

static PyObject *
os_fork1_impl(PyModuleDef *module);

static PyObject *
os_fork1(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_fork1_impl(module);
}

static PyObject *
os_fork1_impl(PyModuleDef *module)
/*[clinic end generated code: output=fa04088d6bc02efa input=12db02167893926e]*/
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
#endif /* HAVE_FORK1 */


#ifdef HAVE_FORK
/*[clinic input]
os.fork

Fork a child process.

Return 0 to child process and PID of child to parent process.
[clinic start generated code]*/

PyDoc_STRVAR(os_fork__doc__,
"fork($module, /)\n"
"--\n"
"\n"
"Fork a child process.\n"
"\n"
"Return 0 to child process and PID of child to parent process.");

#define OS_FORK_METHODDEF    \
    {"fork", (PyCFunction)os_fork, METH_NOARGS, os_fork__doc__},

static PyObject *
os_fork_impl(PyModuleDef *module);

static PyObject *
os_fork(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_fork_impl(module);
}

static PyObject *
os_fork_impl(PyModuleDef *module)
/*[clinic end generated code: output=b3c8e6bdc11eedc6 input=13c956413110eeaa]*/
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
#endif /* HAVE_FORK */


#ifdef HAVE_SCHED_H
#ifdef HAVE_SCHED_GET_PRIORITY_MAX
/*[clinic input]
os.sched_get_priority_max

    policy: int

Get the maximum scheduling priority for policy.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_get_priority_max__doc__,
"sched_get_priority_max($module, /, policy)\n"
"--\n"
"\n"
"Get the maximum scheduling priority for policy.");

#define OS_SCHED_GET_PRIORITY_MAX_METHODDEF    \
    {"sched_get_priority_max", (PyCFunction)os_sched_get_priority_max, METH_VARARGS|METH_KEYWORDS, os_sched_get_priority_max__doc__},

static PyObject *
os_sched_get_priority_max_impl(PyModuleDef *module, int policy);

static PyObject *
os_sched_get_priority_max(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"policy", NULL};
    int policy;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:sched_get_priority_max", _keywords,
        &policy))
        goto exit;
    return_value = os_sched_get_priority_max_impl(module, policy);

exit:
    return return_value;
}

static PyObject *
os_sched_get_priority_max_impl(PyModuleDef *module, int policy)
/*[clinic end generated code: output=a580a52f25238c1f input=2097b7998eca6874]*/
{
    int max;

    max = sched_get_priority_max(policy);
    if (max < 0)
        return posix_error();
    return PyLong_FromLong(max);
}


/*[clinic input]
os.sched_get_priority_min

    policy: int

Get the minimum scheduling priority for policy.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_get_priority_min__doc__,
"sched_get_priority_min($module, /, policy)\n"
"--\n"
"\n"
"Get the minimum scheduling priority for policy.");

#define OS_SCHED_GET_PRIORITY_MIN_METHODDEF    \
    {"sched_get_priority_min", (PyCFunction)os_sched_get_priority_min, METH_VARARGS|METH_KEYWORDS, os_sched_get_priority_min__doc__},

static PyObject *
os_sched_get_priority_min_impl(PyModuleDef *module, int policy);

static PyObject *
os_sched_get_priority_min(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"policy", NULL};
    int policy;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:sched_get_priority_min", _keywords,
        &policy))
        goto exit;
    return_value = os_sched_get_priority_min_impl(module, policy);

exit:
    return return_value;
}

static PyObject *
os_sched_get_priority_min_impl(PyModuleDef *module, int policy)
/*[clinic end generated code: output=bad8ba10e7d0e977 input=21bc8fa0d70983bf]*/
{
    int min = sched_get_priority_min(policy);
    if (min < 0)
        return posix_error();
    return PyLong_FromLong(min);
}
#endif /* HAVE_SCHED_GET_PRIORITY_MAX */


#ifdef HAVE_SCHED_SETSCHEDULER
/*[clinic input]
os.sched_getscheduler
    pid: pid_t
    /

Get the scheduling policy for the process identifiedy by pid.

Passing 0 for pid returns the scheduling policy for the calling process.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_getscheduler__doc__,
"sched_getscheduler($module, pid, /)\n"
"--\n"
"\n"
"Get the scheduling policy for the process identifiedy by pid.\n"
"\n"
"Passing 0 for pid returns the scheduling policy for the calling process.");

#define OS_SCHED_GETSCHEDULER_METHODDEF    \
    {"sched_getscheduler", (PyCFunction)os_sched_getscheduler, METH_VARARGS, os_sched_getscheduler__doc__},

static PyObject *
os_sched_getscheduler_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_sched_getscheduler(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID ":sched_getscheduler",
        &pid))
        goto exit;
    return_value = os_sched_getscheduler_impl(module, pid);

exit:
    return return_value;
}

static PyObject *
os_sched_getscheduler_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=e0d6244207b1d828 input=5f14cfd1f189e1a0]*/
{
    int policy;

    policy = sched_getscheduler(pid);
    if (policy < 0)
        return posix_error();
    return PyLong_FromLong(policy);
}
#endif /* HAVE_SCHED_SETSCHEDULER */


#if defined(HAVE_SCHED_SETSCHEDULER) || defined(HAVE_SCHED_SETPARAM)
/*[clinic input]
class os.sched_param "PyObject *" "&SchedParamType"

@classmethod
os.sched_param.__new__

    sched_priority: object
        A scheduling parameter.

Current has only one field: sched_priority");
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_param__doc__,
"sched_param(sched_priority)\n"
"--\n"
"\n"
"Current has only one field: sched_priority\");\n"
"\n"
"  sched_priority\n"
"    A scheduling parameter.");

static PyObject *
os_sched_param_impl(PyTypeObject *type, PyObject *sched_priority);

static PyObject *
os_sched_param(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"sched_priority", NULL};
    PyObject *sched_priority;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O:sched_param", _keywords,
        &sched_priority))
        goto exit;
    return_value = os_sched_param_impl(type, sched_priority);

exit:
    return return_value;
}

static PyObject *
os_sched_param_impl(PyTypeObject *type, PyObject *sched_priority)
/*[clinic end generated code: output=d3791e345f7fe573 input=73a4c22f7071fc62]*/
{
    PyObject *res;

    res = PyStructSequence_New(type);
    if (!res)
        return NULL;
    Py_INCREF(sched_priority);
    PyStructSequence_SET_ITEM(res, 0, sched_priority);
    return res;
}


static PyStructSequence_Field sched_param_fields[] = {
    {"sched_priority", "the scheduling priority"},
    {0}
};

static PyStructSequence_Desc sched_param_desc = {
    "sched_param", /* name */
    os_sched_param__doc__, /* doc */
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
#endif /* defined(HAVE_SCHED_SETSCHEDULER) || defined(HAVE_SCHED_SETPARAM) */


#ifdef HAVE_SCHED_SETSCHEDULER
/*[clinic input]
os.sched_setscheduler

    pid: pid_t
    policy: int
    param: sched_param
    /

Set the scheduling policy for the process identified by pid.

If pid is 0, the calling process is changed.
param is an instance of sched_param.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_setscheduler__doc__,
"sched_setscheduler($module, pid, policy, param, /)\n"
"--\n"
"\n"
"Set the scheduling policy for the process identified by pid.\n"
"\n"
"If pid is 0, the calling process is changed.\n"
"param is an instance of sched_param.");

#define OS_SCHED_SETSCHEDULER_METHODDEF    \
    {"sched_setscheduler", (PyCFunction)os_sched_setscheduler, METH_VARARGS, os_sched_setscheduler__doc__},

static PyObject *
os_sched_setscheduler_impl(PyModuleDef *module, pid_t pid, int policy, struct sched_param *param);

static PyObject *
os_sched_setscheduler(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int policy;
    struct sched_param param;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "iO&:sched_setscheduler",
        &pid, &policy, convert_sched_param, &param))
        goto exit;
    return_value = os_sched_setscheduler_impl(module, pid, policy, &param);

exit:
    return return_value;
}

static PyObject *
os_sched_setscheduler_impl(PyModuleDef *module, pid_t pid, int policy, struct sched_param *param)
/*[clinic end generated code: output=36abdb73f81c224f input=c581f9469a5327dd]*/
{
    /*
    ** sched_setscheduler() returns 0 in Linux, but the previous
    ** scheduling policy under Solaris/Illumos, and others.
    ** On error, -1 is returned in all Operating Systems.
    */
    if (sched_setscheduler(pid, policy, param) == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#endif  /* HAVE_SCHED_SETSCHEDULER*/


#ifdef HAVE_SCHED_SETPARAM
/*[clinic input]
os.sched_getparam
    pid: pid_t
    /

Returns scheduling parameters for the process identified by pid.

If pid is 0, returns parameters for the calling process.
Return value is an instance of sched_param.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_getparam__doc__,
"sched_getparam($module, pid, /)\n"
"--\n"
"\n"
"Returns scheduling parameters for the process identified by pid.\n"
"\n"
"If pid is 0, returns parameters for the calling process.\n"
"Return value is an instance of sched_param.");

#define OS_SCHED_GETPARAM_METHODDEF    \
    {"sched_getparam", (PyCFunction)os_sched_getparam, METH_VARARGS, os_sched_getparam__doc__},

static PyObject *
os_sched_getparam_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_sched_getparam(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID ":sched_getparam",
        &pid))
        goto exit;
    return_value = os_sched_getparam_impl(module, pid);

exit:
    return return_value;
}

static PyObject *
os_sched_getparam_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=b33acc8db004a8c9 input=18a1ef9c2efae296]*/
{
    struct sched_param param;
    PyObject *result;
    PyObject *priority;

    if (sched_getparam(pid, &param))
        return posix_error();
    result = PyStructSequence_New(&SchedParamType);
    if (!result)
        return NULL;
    priority = PyLong_FromLong(param.sched_priority);
    if (!priority) {
        Py_DECREF(result);
        return NULL;
    }
    PyStructSequence_SET_ITEM(result, 0, priority);
    return result;
}


/*[clinic input]
os.sched_setparam
    pid: pid_t
    param: sched_param
    /

Set scheduling parameters for the process identified by pid.

If pid is 0, sets parameters for the calling process.
param should be an instance of sched_param.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_setparam__doc__,
"sched_setparam($module, pid, param, /)\n"
"--\n"
"\n"
"Set scheduling parameters for the process identified by pid.\n"
"\n"
"If pid is 0, sets parameters for the calling process.\n"
"param should be an instance of sched_param.");

#define OS_SCHED_SETPARAM_METHODDEF    \
    {"sched_setparam", (PyCFunction)os_sched_setparam, METH_VARARGS, os_sched_setparam__doc__},

static PyObject *
os_sched_setparam_impl(PyModuleDef *module, pid_t pid, struct sched_param *param);

static PyObject *
os_sched_setparam(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    struct sched_param param;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "O&:sched_setparam",
        &pid, convert_sched_param, &param))
        goto exit;
    return_value = os_sched_setparam_impl(module, pid, &param);

exit:
    return return_value;
}

static PyObject *
os_sched_setparam_impl(PyModuleDef *module, pid_t pid, struct sched_param *param)
/*[clinic end generated code: output=488bdf5bcbe0d4e8 input=6b8d6dfcecdc21bd]*/
{
    if (sched_setparam(pid, param))
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SCHED_SETPARAM */


#ifdef HAVE_SCHED_RR_GET_INTERVAL
/*[clinic input]
os.sched_rr_get_interval -> double
    pid: pid_t
    /

Return the round-robin quantum for the process identified by pid, in seconds.

Value returned is a float.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_rr_get_interval__doc__,
"sched_rr_get_interval($module, pid, /)\n"
"--\n"
"\n"
"Return the round-robin quantum for the process identified by pid, in seconds.\n"
"\n"
"Value returned is a float.");

#define OS_SCHED_RR_GET_INTERVAL_METHODDEF    \
    {"sched_rr_get_interval", (PyCFunction)os_sched_rr_get_interval, METH_VARARGS, os_sched_rr_get_interval__doc__},

static double
os_sched_rr_get_interval_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_sched_rr_get_interval(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    double _return_value;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID ":sched_rr_get_interval",
        &pid))
        goto exit;
    _return_value = os_sched_rr_get_interval_impl(module, pid);
    if ((_return_value == -1.0) && PyErr_Occurred())
        goto exit;
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}

static double
os_sched_rr_get_interval_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=5b3b8d1f27fb2c0a input=2a973da15cca6fae]*/
{
    struct timespec interval;
    if (sched_rr_get_interval(pid, &interval)) {
        posix_error();
        return -1.0;
    }
    return (double)interval.tv_sec + 1e-9*interval.tv_nsec;
}
#endif /* HAVE_SCHED_RR_GET_INTERVAL */


/*[clinic input]
os.sched_yield

Voluntarily relinquish the CPU.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_yield__doc__,
"sched_yield($module, /)\n"
"--\n"
"\n"
"Voluntarily relinquish the CPU.");

#define OS_SCHED_YIELD_METHODDEF    \
    {"sched_yield", (PyCFunction)os_sched_yield, METH_NOARGS, os_sched_yield__doc__},

static PyObject *
os_sched_yield_impl(PyModuleDef *module);

static PyObject *
os_sched_yield(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_sched_yield_impl(module);
}

static PyObject *
os_sched_yield_impl(PyModuleDef *module)
/*[clinic end generated code: output=9d2e5f29f1370324 input=e54d6f98189391d4]*/
{
    if (sched_yield())
        return posix_error();
    Py_RETURN_NONE;
}

#ifdef HAVE_SCHED_SETAFFINITY
/* The minimum number of CPUs allocated in a cpu_set_t */
static const int NCPUS_START = sizeof(unsigned long) * CHAR_BIT;

/*[clinic input]
os.sched_setaffinity
    pid: pid_t
    mask : object
    /

Set the CPU affinity of the process identified by pid to mask.

mask should be an iterable of integers identifying CPUs.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_setaffinity__doc__,
"sched_setaffinity($module, pid, mask, /)\n"
"--\n"
"\n"
"Set the CPU affinity of the process identified by pid to mask.\n"
"\n"
"mask should be an iterable of integers identifying CPUs.");

#define OS_SCHED_SETAFFINITY_METHODDEF    \
    {"sched_setaffinity", (PyCFunction)os_sched_setaffinity, METH_VARARGS, os_sched_setaffinity__doc__},

static PyObject *
os_sched_setaffinity_impl(PyModuleDef *module, pid_t pid, PyObject *mask);

static PyObject *
os_sched_setaffinity(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    PyObject *mask;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "O:sched_setaffinity",
        &pid, &mask))
        goto exit;
    return_value = os_sched_setaffinity_impl(module, pid, mask);

exit:
    return return_value;
}

static PyObject *
os_sched_setaffinity_impl(PyModuleDef *module, pid_t pid, PyObject *mask)
/*[clinic end generated code: output=5199929738130196 input=a0791a597c7085ba]*/
{
    int ncpus;
    size_t setsize;
    cpu_set_t *cpu_set = NULL;
    PyObject *iterator = NULL, *item;

    iterator = PyObject_GetIter(mask);
    if (iterator == NULL)
        return NULL;

    ncpus = NCPUS_START;
    setsize = CPU_ALLOC_SIZE(ncpus);
    cpu_set = CPU_ALLOC(ncpus);
    if (cpu_set == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    CPU_ZERO_S(setsize, cpu_set);

    while ((item = PyIter_Next(iterator))) {
        long cpu;
        if (!PyLong_Check(item)) {
            PyErr_Format(PyExc_TypeError,
                        "expected an iterator of ints, "
                        "but iterator yielded %R",
                        Py_TYPE(item));
            Py_DECREF(item);
            goto error;
        }
        cpu = PyLong_AsLong(item);
        Py_DECREF(item);
        if (cpu < 0) {
            if (!PyErr_Occurred())
                PyErr_SetString(PyExc_ValueError, "negative CPU number");
            goto error;
        }
        if (cpu > INT_MAX - 1) {
            PyErr_SetString(PyExc_OverflowError, "CPU number too large");
            goto error;
        }
        if (cpu >= ncpus) {
            /* Grow CPU mask to fit the CPU number */
            int newncpus = ncpus;
            cpu_set_t *newmask;
            size_t newsetsize;
            while (newncpus <= cpu) {
                if (newncpus > INT_MAX / 2)
                    newncpus = cpu + 1;
                else
                    newncpus = newncpus * 2;
            }
            newmask = CPU_ALLOC(newncpus);
            if (newmask == NULL) {
                PyErr_NoMemory();
                goto error;
            }
            newsetsize = CPU_ALLOC_SIZE(newncpus);
            CPU_ZERO_S(newsetsize, newmask);
            memcpy(newmask, cpu_set, setsize);
            CPU_FREE(cpu_set);
            setsize = newsetsize;
            cpu_set = newmask;
            ncpus = newncpus;
        }
        CPU_SET_S(cpu, setsize, cpu_set);
    }
    Py_CLEAR(iterator);

    if (sched_setaffinity(pid, setsize, cpu_set)) {
        posix_error();
        goto error;
    }
    CPU_FREE(cpu_set);
    Py_RETURN_NONE;

error:
    if (cpu_set)
        CPU_FREE(cpu_set);
    Py_XDECREF(iterator);
    return NULL;
}


/*[clinic input]
os.sched_getaffinity
    pid: pid_t
    /

Return the affinity of the process identified by pid.

The affinity is returned as a set of CPU identifiers.
[clinic start generated code]*/

PyDoc_STRVAR(os_sched_getaffinity__doc__,
"sched_getaffinity($module, pid, /)\n"
"--\n"
"\n"
"Return the affinity of the process identified by pid.\n"
"\n"
"The affinity is returned as a set of CPU identifiers.");

#define OS_SCHED_GETAFFINITY_METHODDEF    \
    {"sched_getaffinity", (PyCFunction)os_sched_getaffinity, METH_VARARGS, os_sched_getaffinity__doc__},

static PyObject *
os_sched_getaffinity_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_sched_getaffinity(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID ":sched_getaffinity",
        &pid))
        goto exit;
    return_value = os_sched_getaffinity_impl(module, pid);

exit:
    return return_value;
}

static PyObject *
os_sched_getaffinity_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=7b273b0fca9830f0 input=eaf161936874b8a1]*/
{
    int cpu, ncpus, count;
    size_t setsize;
    cpu_set_t *mask = NULL;
    PyObject *res = NULL;

    ncpus = NCPUS_START;
    while (1) {
        setsize = CPU_ALLOC_SIZE(ncpus);
        mask = CPU_ALLOC(ncpus);
        if (mask == NULL)
            return PyErr_NoMemory();
        if (sched_getaffinity(pid, setsize, mask) == 0)
            break;
        CPU_FREE(mask);
        if (errno != EINVAL)
            return posix_error();
        if (ncpus > INT_MAX / 2) {
            PyErr_SetString(PyExc_OverflowError, "could not allocate "
                            "a large enough CPU set");
            return NULL;
        }
        ncpus = ncpus * 2;
    }

    res = PySet_New(NULL);
    if (res == NULL)
        goto error;
    for (cpu = 0, count = CPU_COUNT_S(setsize, mask); count; cpu++) {
        if (CPU_ISSET_S(cpu, setsize, mask)) {
            PyObject *cpu_num = PyLong_FromLong(cpu);
            --count;
            if (cpu_num == NULL)
                goto error;
            if (PySet_Add(res, cpu_num)) {
                Py_DECREF(cpu_num);
                goto error;
            }
            Py_DECREF(cpu_num);
        }
    }
    CPU_FREE(mask);
    return res;

error:
    if (mask)
        CPU_FREE(mask);
    Py_XDECREF(res);
    return NULL;
}

#endif /* HAVE_SCHED_SETAFFINITY */

#endif /* HAVE_SCHED_H */

#ifndef OS_SCHED_GET_PRIORITY_MAX_METHODDEF
#define OS_SCHED_GET_PRIORITY_MAX_METHODDEF
#endif  /* OS_SCHED_GET_PRIORITY_MAX_METHODDEF */

#ifndef OS_SCHED_GET_PRIORITY_MIN_METHODDEF
#define OS_SCHED_GET_PRIORITY_MIN_METHODDEF
#endif  /* OS_SCHED_GET_PRIORITY_MIN_METHODDEF */

#ifndef OS_SCHED_GETSCHEDULER_METHODDEF
#define OS_SCHED_GETSCHEDULER_METHODDEF
#endif  /* OS_SCHED_GETSCHEDULER_METHODDEF */

#ifndef OS_SCHED_SETSCHEDULER_METHODDEF
#define OS_SCHED_SETSCHEDULER_METHODDEF
#endif  /* OS_SCHED_SETSCHEDULER_METHODDEF */

#ifndef OS_SCHED_GETPARAM_METHODDEF
#define OS_SCHED_GETPARAM_METHODDEF
#endif  /* OS_SCHED_GETPARAM_METHODDEF */

#ifndef OS_SCHED_SETPARAM_METHODDEF
#define OS_SCHED_SETPARAM_METHODDEF
#endif  /* OS_SCHED_SETPARAM_METHODDEF */

#ifndef OS_SCHED_RR_GET_INTERVAL_METHODDEF
#define OS_SCHED_RR_GET_INTERVAL_METHODDEF
#endif  /* OS_SCHED_RR_GET_INTERVAL_METHODDEF */

#ifndef OS_SCHED_YIELD_METHODDEF
#define OS_SCHED_YIELD_METHODDEF
#endif  /* OS_SCHED_YIELD_METHODDEF */

#ifndef OS_SCHED_SETAFFINITY_METHODDEF
#define OS_SCHED_SETAFFINITY_METHODDEF
#endif  /* OS_SCHED_SETAFFINITY_METHODDEF */

#ifndef OS_SCHED_GETAFFINITY_METHODDEF
#define OS_SCHED_GETAFFINITY_METHODDEF
#endif  /* OS_SCHED_GETAFFINITY_METHODDEF */


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
/*[clinic input]
os.openpty

Open a pseudo-terminal.

Return a tuple of (master_fd, slave_fd) containing open file descriptors
for both the master and slave ends.
[clinic start generated code]*/

PyDoc_STRVAR(os_openpty__doc__,
"openpty($module, /)\n"
"--\n"
"\n"
"Open a pseudo-terminal.\n"
"\n"
"Return a tuple of (master_fd, slave_fd) containing open file descriptors\n"
"for both the master and slave ends.");

#define OS_OPENPTY_METHODDEF    \
    {"openpty", (PyCFunction)os_openpty, METH_NOARGS, os_openpty__doc__},

static PyObject *
os_openpty_impl(PyModuleDef *module);

static PyObject *
os_openpty(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_openpty_impl(module);
}

static PyObject *
os_openpty_impl(PyModuleDef *module)
/*[clinic end generated code: output=b12d3c1735468464 input=f3d99fd99e762907]*/
{
    int master_fd = -1, slave_fd = -1;
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
        goto posix_error;

    if (_Py_set_inheritable(master_fd, 0, NULL) < 0)
        goto error;
    if (_Py_set_inheritable(slave_fd, 0, NULL) < 0)
        goto error;

#elif defined(HAVE__GETPTY)
    slave_name = _getpty(&master_fd, O_RDWR, 0666, 0);
    if (slave_name == NULL)
        goto posix_error;
    if (_Py_set_inheritable(master_fd, 0, NULL) < 0)
        goto error;

    slave_fd = _Py_open(slave_name, O_RDWR);
    if (slave_fd < 0)
        goto posix_error;

#else
    master_fd = open(DEV_PTY_FILE, O_RDWR | O_NOCTTY); /* open master */
    if (master_fd < 0)
        goto posix_error;

    sig_saved = PyOS_setsig(SIGCHLD, SIG_DFL);

    /* change permission of slave */
    if (grantpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        goto posix_error;
    }

    /* unlock slave */
    if (unlockpt(master_fd) < 0) {
        PyOS_setsig(SIGCHLD, sig_saved);
        goto posix_error;
    }

    PyOS_setsig(SIGCHLD, sig_saved);

    slave_name = ptsname(master_fd); /* get name of slave */
    if (slave_name == NULL)
        goto posix_error;

    slave_fd = _Py_open(slave_name, O_RDWR | O_NOCTTY); /* open slave */
    if (slave_fd < 0)
        goto posix_error;

    if (_Py_set_inheritable(master_fd, 0, NULL) < 0)
        goto posix_error;

#if !defined(__CYGWIN__) && !defined(HAVE_DEV_PTC)
    ioctl(slave_fd, I_PUSH, "ptem"); /* push ptem */
    ioctl(slave_fd, I_PUSH, "ldterm"); /* push ldterm */
#ifndef __hpux
    ioctl(slave_fd, I_PUSH, "ttcompat"); /* push ttcompat */
#endif /* __hpux */
#endif /* HAVE_CYGWIN */
#endif /* HAVE_OPENPTY */

    return Py_BuildValue("(ii)", master_fd, slave_fd);

posix_error:
    posix_error();
#if defined(HAVE_OPENPTY) || defined(HAVE__GETPTY)
error:
#endif
    if (master_fd != -1)
        close(master_fd);
    if (slave_fd != -1)
        close(slave_fd);
    return NULL;
}
#endif /* defined(HAVE_OPENPTY) || defined(HAVE__GETPTY) || defined(HAVE_DEV_PTMX) */


#ifdef HAVE_FORKPTY
/*[clinic input]
os.forkpty

Fork a new process with a new pseudo-terminal as controlling tty.

Returns a tuple of (pid, master_fd).
Like fork(), return pid of 0 to the child process,
and pid of child to the parent process.
To both, return fd of newly opened pseudo-terminal.
[clinic start generated code]*/

PyDoc_STRVAR(os_forkpty__doc__,
"forkpty($module, /)\n"
"--\n"
"\n"
"Fork a new process with a new pseudo-terminal as controlling tty.\n"
"\n"
"Returns a tuple of (pid, master_fd).\n"
"Like fork(), return pid of 0 to the child process,\n"
"and pid of child to the parent process.\n"
"To both, return fd of newly opened pseudo-terminal.");

#define OS_FORKPTY_METHODDEF    \
    {"forkpty", (PyCFunction)os_forkpty, METH_NOARGS, os_forkpty__doc__},

static PyObject *
os_forkpty_impl(PyModuleDef *module);

static PyObject *
os_forkpty(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_forkpty_impl(module);
}

static PyObject *
os_forkpty_impl(PyModuleDef *module)
/*[clinic end generated code: output=d4f82958d2ed5cad input=f1f7f4bae3966010]*/
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
#endif /* HAVE_FORKPTY */


#ifdef HAVE_GETEGID
/*[clinic input]
os.getegid

Return the current process's effective group id.
[clinic start generated code]*/

PyDoc_STRVAR(os_getegid__doc__,
"getegid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s effective group id.");

#define OS_GETEGID_METHODDEF    \
    {"getegid", (PyCFunction)os_getegid, METH_NOARGS, os_getegid__doc__},

static PyObject *
os_getegid_impl(PyModuleDef *module);

static PyObject *
os_getegid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getegid_impl(module);
}

static PyObject *
os_getegid_impl(PyModuleDef *module)
/*[clinic end generated code: output=fd12c346fa41cccb input=1596f79ad1107d5d]*/
{
    return _PyLong_FromGid(getegid());
}
#endif /* HAVE_GETEGID */


#ifdef HAVE_GETEUID
/*[clinic input]
os.geteuid

Return the current process's effective user id.
[clinic start generated code]*/

PyDoc_STRVAR(os_geteuid__doc__,
"geteuid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s effective user id.");

#define OS_GETEUID_METHODDEF    \
    {"geteuid", (PyCFunction)os_geteuid, METH_NOARGS, os_geteuid__doc__},

static PyObject *
os_geteuid_impl(PyModuleDef *module);

static PyObject *
os_geteuid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_geteuid_impl(module);
}

static PyObject *
os_geteuid_impl(PyModuleDef *module)
/*[clinic end generated code: output=03d98e07f4bc03d4 input=4644c662d3bd9f19]*/
{
    return _PyLong_FromUid(geteuid());
}
#endif /* HAVE_GETEUID */


#ifdef HAVE_GETGID
/*[clinic input]
os.getgid

Return the current process's group id.
[clinic start generated code]*/

PyDoc_STRVAR(os_getgid__doc__,
"getgid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s group id.");

#define OS_GETGID_METHODDEF    \
    {"getgid", (PyCFunction)os_getgid, METH_NOARGS, os_getgid__doc__},

static PyObject *
os_getgid_impl(PyModuleDef *module);

static PyObject *
os_getgid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getgid_impl(module);
}

static PyObject *
os_getgid_impl(PyModuleDef *module)
/*[clinic end generated code: output=07b0356121b8098d input=58796344cd87c0f6]*/
{
    return _PyLong_FromGid(getgid());
}
#endif /* HAVE_GETGID */


/*[clinic input]
os.getpid

Return the current process id.
[clinic start generated code]*/

PyDoc_STRVAR(os_getpid__doc__,
"getpid($module, /)\n"
"--\n"
"\n"
"Return the current process id.");

#define OS_GETPID_METHODDEF    \
    {"getpid", (PyCFunction)os_getpid, METH_NOARGS, os_getpid__doc__},

static PyObject *
os_getpid_impl(PyModuleDef *module);

static PyObject *
os_getpid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getpid_impl(module);
}

static PyObject *
os_getpid_impl(PyModuleDef *module)
/*[clinic end generated code: output=d63a01a3cebc573d input=5a9a00f0ab68aa00]*/
{
    return PyLong_FromPid(getpid());
}

#ifdef HAVE_GETGROUPLIST

/* AC 3.5: funny apple logic below */
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

#ifdef __APPLE__
    if (!PyArg_ParseTuple(args, "si:getgrouplist", &user, &basegid))
        return NULL;
#else
    if (!PyArg_ParseTuple(args, "sO&:getgrouplist", &user,
                          _Py_Gid_Converter, &basegid))
        return NULL;
#endif

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
#ifdef __APPLE__
        PyObject *o = PyLong_FromUnsignedLong((unsigned long)groups[i]);
#else
        PyObject *o = _PyLong_FromGid(groups[i]);
#endif
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
#endif /* HAVE_GETGROUPLIST */


#ifdef HAVE_GETGROUPS
/*[clinic input]
os.getgroups

Return list of supplemental group IDs for the process.
[clinic start generated code]*/

PyDoc_STRVAR(os_getgroups__doc__,
"getgroups($module, /)\n"
"--\n"
"\n"
"Return list of supplemental group IDs for the process.");

#define OS_GETGROUPS_METHODDEF    \
    {"getgroups", (PyCFunction)os_getgroups, METH_NOARGS, os_getgroups__doc__},

static PyObject *
os_getgroups_impl(PyModuleDef *module);

static PyObject *
os_getgroups(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getgroups_impl(module);
}

static PyObject *
os_getgroups_impl(PyModuleDef *module)
/*[clinic end generated code: output=d9a3559b2e6f4ab8 input=d3f109412e6a155c]*/
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

#ifdef __APPLE__
    /* Issue #17557: As of OS X 10.8, getgroups(2) no longer raises EINVAL if
     * there are more groups than can fit in grouplist.  Therefore, on OS X
     * always first call getgroups with length 0 to get the actual number
     * of groups.
     */
    n = getgroups(0, NULL);
    if (n < 0) {
        return posix_error();
    } else if (n <= MAX_GROUPS) {
        /* groups will fit in existing array */
        alt_grouplist = grouplist;
    } else {
        alt_grouplist = PyMem_Malloc(n * sizeof(gid_t));
        if (alt_grouplist == NULL) {
            errno = EINVAL;
            return posix_error();
        }
    }

    n = getgroups(n, alt_grouplist);
    if (n == -1) {
        if (alt_grouplist != grouplist) {
            PyMem_Free(alt_grouplist);
        }
        return posix_error();
    }
#else
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
#endif

    result = PyList_New(n);
    if (result != NULL) {
        int i;
        for (i = 0; i < n; ++i) {
            PyObject *o = _PyLong_FromGid(alt_grouplist[i]);
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
#endif /* HAVE_GETGROUPS */

#ifdef HAVE_INITGROUPS
PyDoc_STRVAR(posix_initgroups__doc__,
"initgroups(username, gid) -> None\n\n\
Call the system initgroups() to initialize the group access list with all of\n\
the groups of which the specified username is a member, plus the specified\n\
group id.");

/* AC 3.5: funny apple logic */
static PyObject *
posix_initgroups(PyObject *self, PyObject *args)
{
    PyObject *oname;
    char *username;
    int res;
#ifdef __APPLE__
    int gid;
#else
    gid_t gid;
#endif

#ifdef __APPLE__
    if (!PyArg_ParseTuple(args, "O&i:initgroups",
                          PyUnicode_FSConverter, &oname,
                          &gid))
#else
    if (!PyArg_ParseTuple(args, "O&O&:initgroups",
                          PyUnicode_FSConverter, &oname,
                          _Py_Gid_Converter, &gid))
#endif
        return NULL;
    username = PyBytes_AS_STRING(oname);

    res = initgroups(username, gid);
    Py_DECREF(oname);
    if (res == -1)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_INCREF(Py_None);
    return Py_None;
}
#endif /* HAVE_INITGROUPS */


#ifdef HAVE_GETPGID
/*[clinic input]
os.getpgid

    pid: pid_t

Call the system call getpgid(), and return the result.
[clinic start generated code]*/

PyDoc_STRVAR(os_getpgid__doc__,
"getpgid($module, /, pid)\n"
"--\n"
"\n"
"Call the system call getpgid(), and return the result.");

#define OS_GETPGID_METHODDEF    \
    {"getpgid", (PyCFunction)os_getpgid, METH_VARARGS|METH_KEYWORDS, os_getpgid__doc__},

static PyObject *
os_getpgid_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_getpgid(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"pid", NULL};
    pid_t pid;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "" _Py_PARSE_PID ":getpgid", _keywords,
        &pid))
        goto exit;
    return_value = os_getpgid_impl(module, pid);

exit:
    return return_value;
}

static PyObject *
os_getpgid_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=3db4ed686179160d input=39d710ae3baaf1c7]*/
{
    pid_t pgid = getpgid(pid);
    if (pgid < 0)
        return posix_error();
    return PyLong_FromPid(pgid);
}
#endif /* HAVE_GETPGID */


#ifdef HAVE_GETPGRP
/*[clinic input]
os.getpgrp

Return the current process group id.
[clinic start generated code]*/

PyDoc_STRVAR(os_getpgrp__doc__,
"getpgrp($module, /)\n"
"--\n"
"\n"
"Return the current process group id.");

#define OS_GETPGRP_METHODDEF    \
    {"getpgrp", (PyCFunction)os_getpgrp, METH_NOARGS, os_getpgrp__doc__},

static PyObject *
os_getpgrp_impl(PyModuleDef *module);

static PyObject *
os_getpgrp(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getpgrp_impl(module);
}

static PyObject *
os_getpgrp_impl(PyModuleDef *module)
/*[clinic end generated code: output=3b0d3663ea054277 input=6846fb2bb9a3705e]*/
{
#ifdef GETPGRP_HAVE_ARG
    return PyLong_FromPid(getpgrp(0));
#else /* GETPGRP_HAVE_ARG */
    return PyLong_FromPid(getpgrp());
#endif /* GETPGRP_HAVE_ARG */
}
#endif /* HAVE_GETPGRP */


#ifdef HAVE_SETPGRP
/*[clinic input]
os.setpgrp

Make the current process the leader of its process group.
[clinic start generated code]*/

PyDoc_STRVAR(os_setpgrp__doc__,
"setpgrp($module, /)\n"
"--\n"
"\n"
"Make the current process the leader of its process group.");

#define OS_SETPGRP_METHODDEF    \
    {"setpgrp", (PyCFunction)os_setpgrp, METH_NOARGS, os_setpgrp__doc__},

static PyObject *
os_setpgrp_impl(PyModuleDef *module);

static PyObject *
os_setpgrp(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_setpgrp_impl(module);
}

static PyObject *
os_setpgrp_impl(PyModuleDef *module)
/*[clinic end generated code: output=8fbb0ee29ef6fb2d input=1f0619fcb5731e7e]*/
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


/*[clinic input]
os.getppid

Return the parent's process id.

If the parent process has already exited, Windows machines will still
return its id; others systems will return the id of the 'init' process (1).
[clinic start generated code]*/

PyDoc_STRVAR(os_getppid__doc__,
"getppid($module, /)\n"
"--\n"
"\n"
"Return the parent\'s process id.\n"
"\n"
"If the parent process has already exited, Windows machines will still\n"
"return its id; others systems will return the id of the \'init\' process (1).");

#define OS_GETPPID_METHODDEF    \
    {"getppid", (PyCFunction)os_getppid, METH_NOARGS, os_getppid__doc__},

static PyObject *
os_getppid_impl(PyModuleDef *module);

static PyObject *
os_getppid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getppid_impl(module);
}

static PyObject *
os_getppid_impl(PyModuleDef *module)
/*[clinic end generated code: output=9ff3b387781edf3a input=e637cb87539c030e]*/
{
#ifdef MS_WINDOWS
    return win32_getppid();
#else
    return PyLong_FromPid(getppid());
#endif
}
#endif /* HAVE_GETPPID */


#ifdef HAVE_GETLOGIN
/*[clinic input]
os.getlogin

Return the actual login name.
[clinic start generated code]*/

PyDoc_STRVAR(os_getlogin__doc__,
"getlogin($module, /)\n"
"--\n"
"\n"
"Return the actual login name.");

#define OS_GETLOGIN_METHODDEF    \
    {"getlogin", (PyCFunction)os_getlogin, METH_NOARGS, os_getlogin__doc__},

static PyObject *
os_getlogin_impl(PyModuleDef *module);

static PyObject *
os_getlogin(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getlogin_impl(module);
}

static PyObject *
os_getlogin_impl(PyModuleDef *module)
/*[clinic end generated code: output=ab6211dab104cbb2 input=2a21ab1e917163df]*/
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
/*[clinic input]
os.getuid

Return the current process's user id.
[clinic start generated code]*/

PyDoc_STRVAR(os_getuid__doc__,
"getuid($module, /)\n"
"--\n"
"\n"
"Return the current process\'s user id.");

#define OS_GETUID_METHODDEF    \
    {"getuid", (PyCFunction)os_getuid, METH_NOARGS, os_getuid__doc__},

static PyObject *
os_getuid_impl(PyModuleDef *module);

static PyObject *
os_getuid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getuid_impl(module);
}

static PyObject *
os_getuid_impl(PyModuleDef *module)
/*[clinic end generated code: output=77e0dcf2e37d1e89 input=b53c8b35f110a516]*/
{
    return _PyLong_FromUid(getuid());
}
#endif /* HAVE_GETUID */


#ifdef MS_WINDOWS
#define HAVE_KILL
#endif /* MS_WINDOWS */

#ifdef HAVE_KILL
/*[clinic input]
os.kill

    pid: pid_t
    signal: Py_ssize_t
    /

Kill a process with a signal.
[clinic start generated code]*/

PyDoc_STRVAR(os_kill__doc__,
"kill($module, pid, signal, /)\n"
"--\n"
"\n"
"Kill a process with a signal.");

#define OS_KILL_METHODDEF    \
    {"kill", (PyCFunction)os_kill, METH_VARARGS, os_kill__doc__},

static PyObject *
os_kill_impl(PyModuleDef *module, pid_t pid, Py_ssize_t signal);

static PyObject *
os_kill(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    Py_ssize_t signal;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "n:kill",
        &pid, &signal))
        goto exit;
    return_value = os_kill_impl(module, pid, signal);

exit:
    return return_value;
}

static PyObject *
os_kill_impl(PyModuleDef *module, pid_t pid, Py_ssize_t signal)
/*[clinic end generated code: output=2f5c77920ed575e6 input=61a36b86ca275ab9]*/
#ifndef MS_WINDOWS
{
    if (kill(pid, (int)signal) == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#else /* !MS_WINDOWS */
{
    PyObject *result;
    DWORD sig = (DWORD)signal;
    DWORD err;
    HANDLE handle;

    /* Console processes which share a common console can be sent CTRL+C or
       CTRL+BREAK events, provided they handle said events. */
    if (sig == CTRL_C_EVENT || sig == CTRL_BREAK_EVENT) {
        if (GenerateConsoleCtrlEvent(sig, (DWORD)pid) == 0) {
            err = GetLastError();
            PyErr_SetFromWindowsErr(err);
        }
        else
            Py_RETURN_NONE;
    }

    /* If the signal is outside of what GenerateConsoleCtrlEvent can use,
       attempt to open and terminate the process. */
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid);
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
#endif /* !MS_WINDOWS */
#endif /* HAVE_KILL */


#ifdef HAVE_KILLPG
/*[clinic input]
os.killpg

    pgid: pid_t
    signal: int
    /

Kill a process group with a signal.
[clinic start generated code]*/

PyDoc_STRVAR(os_killpg__doc__,
"killpg($module, pgid, signal, /)\n"
"--\n"
"\n"
"Kill a process group with a signal.");

#define OS_KILLPG_METHODDEF    \
    {"killpg", (PyCFunction)os_killpg, METH_VARARGS, os_killpg__doc__},

static PyObject *
os_killpg_impl(PyModuleDef *module, pid_t pgid, int signal);

static PyObject *
os_killpg(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pgid;
    int signal;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "i:killpg",
        &pgid, &signal))
        goto exit;
    return_value = os_killpg_impl(module, pgid, signal);

exit:
    return return_value;
}

static PyObject *
os_killpg_impl(PyModuleDef *module, pid_t pgid, int signal)
/*[clinic end generated code: output=0e05215d1c007e01 input=38b5449eb8faec19]*/
{
    /* XXX some man pages make the `pgid` parameter an int, others
       a pid_t. Since getpgrp() returns a pid_t, we assume killpg should
       take the same type. Moreover, pid_t is always at least as wide as
       int (else compilation of this module fails), which is safe. */
    if (killpg(pgid, signal) == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_KILLPG */


#ifdef HAVE_PLOCK
#ifdef HAVE_SYS_LOCK_H
#include <sys/lock.h>
#endif

/*[clinic input]
os.plock
    op: int
    /

Lock program segments into memory.");
[clinic start generated code]*/

PyDoc_STRVAR(os_plock__doc__,
"plock($module, op, /)\n"
"--\n"
"\n"
"Lock program segments into memory.\");");

#define OS_PLOCK_METHODDEF    \
    {"plock", (PyCFunction)os_plock, METH_VARARGS, os_plock__doc__},

static PyObject *
os_plock_impl(PyModuleDef *module, int op);

static PyObject *
os_plock(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int op;

    if (!PyArg_ParseTuple(args,
        "i:plock",
        &op))
        goto exit;
    return_value = os_plock_impl(module, op);

exit:
    return return_value;
}

static PyObject *
os_plock_impl(PyModuleDef *module, int op)
/*[clinic end generated code: output=2744fe4b6e5f4dbc input=e6e5e348e1525f60]*/
{
    if (plock(op) == -1)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_PLOCK */


#ifdef HAVE_SETUID
/*[clinic input]
os.setuid

    uid: uid_t
    /

Set the current process's user id.
[clinic start generated code]*/

PyDoc_STRVAR(os_setuid__doc__,
"setuid($module, uid, /)\n"
"--\n"
"\n"
"Set the current process\'s user id.");

#define OS_SETUID_METHODDEF    \
    {"setuid", (PyCFunction)os_setuid, METH_VARARGS, os_setuid__doc__},

static PyObject *
os_setuid_impl(PyModuleDef *module, uid_t uid);

static PyObject *
os_setuid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    uid_t uid;

    if (!PyArg_ParseTuple(args,
        "O&:setuid",
        _Py_Uid_Converter, &uid))
        goto exit;
    return_value = os_setuid_impl(module, uid);

exit:
    return return_value;
}

static PyObject *
os_setuid_impl(PyModuleDef *module, uid_t uid)
/*[clinic end generated code: output=aea344bc22ccf400 input=c921a3285aa22256]*/
{
    if (setuid(uid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETUID */


#ifdef HAVE_SETEUID
/*[clinic input]
os.seteuid

    euid: uid_t
    /

Set the current process's effective user id.
[clinic start generated code]*/

PyDoc_STRVAR(os_seteuid__doc__,
"seteuid($module, euid, /)\n"
"--\n"
"\n"
"Set the current process\'s effective user id.");

#define OS_SETEUID_METHODDEF    \
    {"seteuid", (PyCFunction)os_seteuid, METH_VARARGS, os_seteuid__doc__},

static PyObject *
os_seteuid_impl(PyModuleDef *module, uid_t euid);

static PyObject *
os_seteuid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    uid_t euid;

    if (!PyArg_ParseTuple(args,
        "O&:seteuid",
        _Py_Uid_Converter, &euid))
        goto exit;
    return_value = os_seteuid_impl(module, euid);

exit:
    return return_value;
}

static PyObject *
os_seteuid_impl(PyModuleDef *module, uid_t euid)
/*[clinic end generated code: output=6e824cce4f3b8a5d input=ba93d927e4781aa9]*/
{
    if (seteuid(euid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETEUID */


#ifdef HAVE_SETEGID
/*[clinic input]
os.setegid

    egid: gid_t
    /

Set the current process's effective group id.
[clinic start generated code]*/

PyDoc_STRVAR(os_setegid__doc__,
"setegid($module, egid, /)\n"
"--\n"
"\n"
"Set the current process\'s effective group id.");

#define OS_SETEGID_METHODDEF    \
    {"setegid", (PyCFunction)os_setegid, METH_VARARGS, os_setegid__doc__},

static PyObject *
os_setegid_impl(PyModuleDef *module, gid_t egid);

static PyObject *
os_setegid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    gid_t egid;

    if (!PyArg_ParseTuple(args,
        "O&:setegid",
        _Py_Gid_Converter, &egid))
        goto exit;
    return_value = os_setegid_impl(module, egid);

exit:
    return return_value;
}

static PyObject *
os_setegid_impl(PyModuleDef *module, gid_t egid)
/*[clinic end generated code: output=80a32263a4d56a9c input=4080526d0ccd6ce3]*/
{
    if (setegid(egid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETEGID */


#ifdef HAVE_SETREUID
/*[clinic input]
os.setreuid

    ruid: uid_t
    euid: uid_t
    /

Set the current process's real and effective user ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_setreuid__doc__,
"setreuid($module, ruid, euid, /)\n"
"--\n"
"\n"
"Set the current process\'s real and effective user ids.");

#define OS_SETREUID_METHODDEF    \
    {"setreuid", (PyCFunction)os_setreuid, METH_VARARGS, os_setreuid__doc__},

static PyObject *
os_setreuid_impl(PyModuleDef *module, uid_t ruid, uid_t euid);

static PyObject *
os_setreuid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;

    if (!PyArg_ParseTuple(args,
        "O&O&:setreuid",
        _Py_Uid_Converter, &ruid, _Py_Uid_Converter, &euid))
        goto exit;
    return_value = os_setreuid_impl(module, ruid, euid);

exit:
    return return_value;
}

static PyObject *
os_setreuid_impl(PyModuleDef *module, uid_t ruid, uid_t euid)
/*[clinic end generated code: output=d7f226f943dad739 input=0ca8978de663880c]*/
{
    if (setreuid(ruid, euid) < 0) {
        return posix_error();
    } else {
        Py_INCREF(Py_None);
        return Py_None;
    }
}
#endif /* HAVE_SETREUID */


#ifdef HAVE_SETREGID
/*[clinic input]
os.setregid

    rgid: gid_t
    egid: gid_t
    /

Set the current process's real and effective group ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_setregid__doc__,
"setregid($module, rgid, egid, /)\n"
"--\n"
"\n"
"Set the current process\'s real and effective group ids.");

#define OS_SETREGID_METHODDEF    \
    {"setregid", (PyCFunction)os_setregid, METH_VARARGS, os_setregid__doc__},

static PyObject *
os_setregid_impl(PyModuleDef *module, gid_t rgid, gid_t egid);

static PyObject *
os_setregid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;

    if (!PyArg_ParseTuple(args,
        "O&O&:setregid",
        _Py_Gid_Converter, &rgid, _Py_Gid_Converter, &egid))
        goto exit;
    return_value = os_setregid_impl(module, rgid, egid);

exit:
    return return_value;
}

static PyObject *
os_setregid_impl(PyModuleDef *module, gid_t rgid, gid_t egid)
/*[clinic end generated code: output=a82d9ab70f8e6562 input=c59499f72846db78]*/
{
    if (setregid(rgid, egid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETREGID */


#ifdef HAVE_SETGID
/*[clinic input]
os.setgid
    gid: gid_t
    /

Set the current process's group id.
[clinic start generated code]*/

PyDoc_STRVAR(os_setgid__doc__,
"setgid($module, gid, /)\n"
"--\n"
"\n"
"Set the current process\'s group id.");

#define OS_SETGID_METHODDEF    \
    {"setgid", (PyCFunction)os_setgid, METH_VARARGS, os_setgid__doc__},

static PyObject *
os_setgid_impl(PyModuleDef *module, gid_t gid);

static PyObject *
os_setgid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    gid_t gid;

    if (!PyArg_ParseTuple(args,
        "O&:setgid",
        _Py_Gid_Converter, &gid))
        goto exit;
    return_value = os_setgid_impl(module, gid);

exit:
    return return_value;
}

static PyObject *
os_setgid_impl(PyModuleDef *module, gid_t gid)
/*[clinic end generated code: output=08287886db435f23 input=27d30c4059045dc6]*/
{
    if (setgid(gid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETGID */


#ifdef HAVE_SETGROUPS
/*[clinic input]
os.setgroups

    groups: object
    /

Set the groups of the current process to list.
[clinic start generated code]*/

PyDoc_STRVAR(os_setgroups__doc__,
"setgroups($module, groups, /)\n"
"--\n"
"\n"
"Set the groups of the current process to list.");

#define OS_SETGROUPS_METHODDEF    \
    {"setgroups", (PyCFunction)os_setgroups, METH_O, os_setgroups__doc__},

static PyObject *
os_setgroups(PyModuleDef *module, PyObject *groups)
/*[clinic end generated code: output=0b8de65d5b3cda94 input=fa742ca3daf85a7e]*/
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
            if (!_Py_Gid_Converter(elem, &grouplist[i])) {
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
/*[clinic input]
os.wait3

    options: int
Wait for completion of a child process.

Returns a tuple of information about the child process:
  (pid, status, rusage)
[clinic start generated code]*/

PyDoc_STRVAR(os_wait3__doc__,
"wait3($module, /, options)\n"
"--\n"
"\n"
"Wait for completion of a child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"  (pid, status, rusage)");

#define OS_WAIT3_METHODDEF    \
    {"wait3", (PyCFunction)os_wait3, METH_VARARGS|METH_KEYWORDS, os_wait3__doc__},

static PyObject *
os_wait3_impl(PyModuleDef *module, int options);

static PyObject *
os_wait3(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"options", NULL};
    int options;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:wait3", _keywords,
        &options))
        goto exit;
    return_value = os_wait3_impl(module, options);

exit:
    return return_value;
}

static PyObject *
os_wait3_impl(PyModuleDef *module, int options)
/*[clinic end generated code: output=1f2a63b6a93cbb57 input=8ac4c56956b61710]*/
{
    pid_t pid;
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    Py_BEGIN_ALLOW_THREADS
    pid = wait3(&status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT3 */


#ifdef HAVE_WAIT4
/*[clinic input]

os.wait4

    pid: pid_t
    options: int

Wait for completion of a specific child process.

Returns a tuple of information about the child process:
  (pid, status, rusage)
[clinic start generated code]*/

PyDoc_STRVAR(os_wait4__doc__,
"wait4($module, /, pid, options)\n"
"--\n"
"\n"
"Wait for completion of a specific child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"  (pid, status, rusage)");

#define OS_WAIT4_METHODDEF    \
    {"wait4", (PyCFunction)os_wait4, METH_VARARGS|METH_KEYWORDS, os_wait4__doc__},

static PyObject *
os_wait4_impl(PyModuleDef *module, pid_t pid, int options);

static PyObject *
os_wait4(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"pid", "options", NULL};
    pid_t pid;
    int options;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "" _Py_PARSE_PID "i:wait4", _keywords,
        &pid, &options))
        goto exit;
    return_value = os_wait4_impl(module, pid, options);

exit:
    return return_value;
}

static PyObject *
os_wait4_impl(PyModuleDef *module, pid_t pid, int options)
/*[clinic end generated code: output=20dfb05289d37dc6 input=d11deed0750600ba]*/
{
    struct rusage ru;
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    Py_BEGIN_ALLOW_THREADS
    pid = wait4(pid, &status, options, &ru);
    Py_END_ALLOW_THREADS

    return wait_helper(pid, WAIT_STATUS_INT(status), &ru);
}
#endif /* HAVE_WAIT4 */


#if defined(HAVE_WAITID) && !defined(__APPLE__)
/*[clinic input]
os.waitid

    idtype: idtype_t
        Must be one of be P_PID, P_PGID or P_ALL.
    id: id_t
        The id to wait on.
    options: int
        Constructed from the ORing of one or more of WEXITED, WSTOPPED
        or WCONTINUED and additionally may be ORed with WNOHANG or WNOWAIT.
    /

Returns the result of waiting for a process or processes.

Returns either waitid_result or None if WNOHANG is specified and there are
no children in a waitable state.
[clinic start generated code]*/

PyDoc_STRVAR(os_waitid__doc__,
"waitid($module, idtype, id, options, /)\n"
"--\n"
"\n"
"Returns the result of waiting for a process or processes.\n"
"\n"
"  idtype\n"
"    Must be one of be P_PID, P_PGID or P_ALL.\n"
"  id\n"
"    The id to wait on.\n"
"  options\n"
"    Constructed from the ORing of one or more of WEXITED, WSTOPPED\n"
"    or WCONTINUED and additionally may be ORed with WNOHANG or WNOWAIT.\n"
"\n"
"Returns either waitid_result or None if WNOHANG is specified and there are\n"
"no children in a waitable state.");

#define OS_WAITID_METHODDEF    \
    {"waitid", (PyCFunction)os_waitid, METH_VARARGS, os_waitid__doc__},

static PyObject *
os_waitid_impl(PyModuleDef *module, idtype_t idtype, id_t id, int options);

static PyObject *
os_waitid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    idtype_t idtype;
    id_t id;
    int options;

    if (!PyArg_ParseTuple(args,
        "i" _Py_PARSE_PID "i:waitid",
        &idtype, &id, &options))
        goto exit;
    return_value = os_waitid_impl(module, idtype, id, options);

exit:
    return return_value;
}

static PyObject *
os_waitid_impl(PyModuleDef *module, idtype_t idtype, id_t id, int options)
/*[clinic end generated code: output=fb44bf97f01021b2 input=d8e7f76e052b7920]*/
{
    PyObject *result;
    int res;
    siginfo_t si;
    si.si_pid = 0;

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
    PyStructSequence_SET_ITEM(result, 1, _PyLong_FromUid(si.si_uid));
    PyStructSequence_SET_ITEM(result, 2, PyLong_FromLong((long)(si.si_signo)));
    PyStructSequence_SET_ITEM(result, 3, PyLong_FromLong((long)(si.si_status)));
    PyStructSequence_SET_ITEM(result, 4, PyLong_FromLong((long)(si.si_code)));
    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}
#endif /* defined(HAVE_WAITID) && !defined(__APPLE__) */


#if defined(HAVE_WAITPID)
/*[clinic input]
os.waitpid
    pid: pid_t
    options: int
    /

Wait for completion of a given child process.

Returns a tuple of information regarding the child process:
    (pid, status)

The options argument is ignored on Windows.
[clinic start generated code]*/

PyDoc_STRVAR(os_waitpid__doc__,
"waitpid($module, pid, options, /)\n"
"--\n"
"\n"
"Wait for completion of a given child process.\n"
"\n"
"Returns a tuple of information regarding the child process:\n"
"    (pid, status)\n"
"\n"
"The options argument is ignored on Windows.");

#define OS_WAITPID_METHODDEF    \
    {"waitpid", (PyCFunction)os_waitpid, METH_VARARGS, os_waitpid__doc__},

static PyObject *
os_waitpid_impl(PyModuleDef *module, pid_t pid, int options);

static PyObject *
os_waitpid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int options;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "i:waitpid",
        &pid, &options))
        goto exit;
    return_value = os_waitpid_impl(module, pid, options);

exit:
    return return_value;
}

static PyObject *
os_waitpid_impl(PyModuleDef *module, pid_t pid, int options)
/*[clinic end generated code: output=095a6b00af70b7ac input=0bf1666b8758fda3]*/
{
    WAIT_TYPE status;
    WAIT_STATUS_INT(status) = 0;

    Py_BEGIN_ALLOW_THREADS
    pid = waitpid(pid, &status, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return posix_error();

    return Py_BuildValue("Ni", PyLong_FromPid(pid), WAIT_STATUS_INT(status));
}
#elif defined(HAVE_CWAIT)
/* MS C has a variant of waitpid() that's usable for most purposes. */
/*[clinic input]
os.waitpid
    pid: Py_intptr_t
    options: int
    /

Wait for completion of a given process.

Returns a tuple of information regarding the process:
    (pid, status << 8)

The options argument is ignored on Windows.
[clinic start generated code]*/

PyDoc_STRVAR(os_waitpid__doc__,
"waitpid($module, pid, options, /)\n"
"--\n"
"\n"
"Wait for completion of a given process.\n"
"\n"
"Returns a tuple of information regarding the process:\n"
"    (pid, status << 8)\n"
"\n"
"The options argument is ignored on Windows.");

#define OS_WAITPID_METHODDEF    \
    {"waitpid", (PyCFunction)os_waitpid, METH_VARARGS, os_waitpid__doc__},

static PyObject *
os_waitpid_impl(PyModuleDef *module, Py_intptr_t pid, int options);

static PyObject *
os_waitpid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_intptr_t pid;
    int options;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_INTPTR "i:waitpid",
        &pid, &options))
        goto exit;
    return_value = os_waitpid_impl(module, pid, options);

exit:
    return return_value;
}

static PyObject *
os_waitpid_impl(PyModuleDef *module, Py_intptr_t pid, int options)
/*[clinic end generated code: output=c20b95b15ad44a3a input=444c8f51cca5b862]*/
{
    int status;

    Py_BEGIN_ALLOW_THREADS
    pid = _cwait(&status, pid, options);
    Py_END_ALLOW_THREADS
    if (pid == -1)
        return posix_error();

    /* shift the status left a byte so this is more like the POSIX waitpid */
    return Py_BuildValue(_Py_PARSE_INTPTR "i", pid, status << 8);
}
#endif


#ifdef HAVE_WAIT
/*[clinic input]
os.wait

Wait for completion of a child process.

Returns a tuple of information about the child process:
    (pid, status)
[clinic start generated code]*/

PyDoc_STRVAR(os_wait__doc__,
"wait($module, /)\n"
"--\n"
"\n"
"Wait for completion of a child process.\n"
"\n"
"Returns a tuple of information about the child process:\n"
"    (pid, status)");

#define OS_WAIT_METHODDEF    \
    {"wait", (PyCFunction)os_wait, METH_NOARGS, os_wait__doc__},

static PyObject *
os_wait_impl(PyModuleDef *module);

static PyObject *
os_wait(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_wait_impl(module);
}

static PyObject *
os_wait_impl(PyModuleDef *module)
/*[clinic end generated code: output=2a83a9d164e7e6a8 input=03b0182d4a4700ce]*/
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
#endif /* HAVE_WAIT */


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

/* AC 3.5: merge win32 and not together */
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
    path.function_name = "readlink";
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O&|$O&:readlink", keywords,
                          path_converter, &path,
                          READLINKAT_DIR_FD_CONVERTER, &dir_fd))
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
        return_value = path_error(&path);
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



#ifdef HAVE_SYMLINK

#if defined(MS_WINDOWS)

/* Grab CreateSymbolicLinkW dynamically from kernel32 */
static DWORD (CALLBACK *Py_CreateSymbolicLinkW)(LPWSTR, LPWSTR, DWORD) = NULL;
static DWORD (CALLBACK *Py_CreateSymbolicLinkA)(LPSTR, LPSTR, DWORD) = NULL;

static int
check_CreateSymbolicLink(void)
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

/* Remove the last portion of the path */
static void
_dirnameW(WCHAR *path)
{
    WCHAR *ptr;

    /* walk the path from the end until a backslash is encountered */
    for(ptr = path + wcslen(path); ptr != path; ptr--) {
        if (*ptr == L'\\' || *ptr == L'/')
            break;
    }
    *ptr = 0;
}

/* Remove the last portion of the path */
static void
_dirnameA(char *path)
{
    char *ptr;

    /* walk the path from the end until a backslash is encountered */
    for(ptr = path + strlen(path); ptr != path; ptr--) {
        if (*ptr == '\\' || *ptr == '/')
            break;
    }
    *ptr = 0;
}

/* Is this path absolute? */
static int
_is_absW(const WCHAR *path)
{
    return path[0] == L'\\' || path[0] == L'/' || path[1] == L':';

}

/* Is this path absolute? */
static int
_is_absA(const char *path)
{
    return path[0] == '\\' || path[0] == '/' || path[1] == ':';

}

/* join root and rest with a backslash */
static void
_joinW(WCHAR *dest_path, const WCHAR *root, const WCHAR *rest)
{
    size_t root_len;

    if (_is_absW(rest)) {
        wcscpy(dest_path, rest);
        return;
    }

    root_len = wcslen(root);

    wcscpy(dest_path, root);
    if(root_len) {
        dest_path[root_len] = L'\\';
        root_len++;
    }
    wcscpy(dest_path+root_len, rest);
}

/* join root and rest with a backslash */
static void
_joinA(char *dest_path, const char *root, const char *rest)
{
    size_t root_len;

    if (_is_absA(rest)) {
        strcpy(dest_path, rest);
        return;
    }

    root_len = strlen(root);

    strcpy(dest_path, root);
    if(root_len) {
        dest_path[root_len] = '\\';
        root_len++;
    }
    strcpy(dest_path+root_len, rest);
}

/* Return True if the path at src relative to dest is a directory */
static int
_check_dirW(WCHAR *src, WCHAR *dest)
{
    WIN32_FILE_ATTRIBUTE_DATA src_info;
    WCHAR dest_parent[MAX_PATH];
    WCHAR src_resolved[MAX_PATH] = L"";

    /* dest_parent = os.path.dirname(dest) */
    wcscpy(dest_parent, dest);
    _dirnameW(dest_parent);
    /* src_resolved = os.path.join(dest_parent, src) */
    _joinW(src_resolved, dest_parent, src);
    return (
        GetFileAttributesExW(src_resolved, GetFileExInfoStandard, &src_info)
        && src_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
    );
}

/* Return True if the path at src relative to dest is a directory */
static int
_check_dirA(char *src, char *dest)
{
    WIN32_FILE_ATTRIBUTE_DATA src_info;
    char dest_parent[MAX_PATH];
    char src_resolved[MAX_PATH] = "";

    /* dest_parent = os.path.dirname(dest) */
    strcpy(dest_parent, dest);
    _dirnameA(dest_parent);
    /* src_resolved = os.path.join(dest_parent, src) */
    _joinA(src_resolved, dest_parent, src);
    return (
        GetFileAttributesExA(src_resolved, GetFileExInfoStandard, &src_info)
        && src_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY
    );
}
#endif


/*[clinic input]
os.symlink
    src: path_t
    dst: path_t
    target_is_directory: bool = False
    *
    dir_fd: dir_fd(requires='symlinkat')=None

# "symlink(src, dst, target_is_directory=False, *, dir_fd=None)\n\n\

Create a symbolic link pointing to src named dst.

target_is_directory is required on Windows if the target is to be
  interpreted as a directory.  (On Windows, symlink requires
  Windows 6.0 or greater, and raises a NotImplementedError otherwise.)
  target_is_directory is ignored on non-Windows platforms.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.

[clinic start generated code]*/

PyDoc_STRVAR(os_symlink__doc__,
"symlink($module, /, src, dst, target_is_directory=False, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a symbolic link pointing to src named dst.\n"
"\n"
"target_is_directory is required on Windows if the target is to be\n"
"  interpreted as a directory.  (On Windows, symlink requires\n"
"  Windows 6.0 or greater, and raises a NotImplementedError otherwise.)\n"
"  target_is_directory is ignored on non-Windows platforms.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_SYMLINK_METHODDEF    \
    {"symlink", (PyCFunction)os_symlink, METH_VARARGS|METH_KEYWORDS, os_symlink__doc__},

static PyObject *
os_symlink_impl(PyModuleDef *module, path_t *src, path_t *dst, int target_is_directory, int dir_fd);

static PyObject *
os_symlink(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"src", "dst", "target_is_directory", "dir_fd", NULL};
    path_t src = PATH_T_INITIALIZE("symlink", "src", 0, 0);
    path_t dst = PATH_T_INITIALIZE("symlink", "dst", 0, 0);
    int target_is_directory = 0;
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|p$O&:symlink", _keywords,
        path_converter, &src, path_converter, &dst, &target_is_directory, SYMLINKAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_symlink_impl(module, &src, &dst, target_is_directory, dir_fd);

exit:
    /* Cleanup for src */
    path_cleanup(&src);
    /* Cleanup for dst */
    path_cleanup(&dst);

    return return_value;
}

static PyObject *
os_symlink_impl(PyModuleDef *module, path_t *src, path_t *dst, int target_is_directory, int dir_fd)
/*[clinic end generated code: output=1a31e6d88aafe9b6 input=e820ec4472547bc3]*/
{
#ifdef MS_WINDOWS
    DWORD result;
#else
    int result;
#endif

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

    if ((src->narrow && dst->wide) || (src->wide && dst->narrow)) {
        PyErr_SetString(PyExc_ValueError,
            "symlink: src and dst must be the same type");
        return NULL;
    }

#ifdef MS_WINDOWS

    Py_BEGIN_ALLOW_THREADS
    if (dst->wide) {
        /* if src is a directory, ensure target_is_directory==1 */
        target_is_directory |= _check_dirW(src->wide, dst->wide);
        result = Py_CreateSymbolicLinkW(dst->wide, src->wide,
                                        target_is_directory);
    }
    else {
        /* if src is a directory, ensure target_is_directory==1 */
        target_is_directory |= _check_dirA(src->narrow, dst->narrow);
        result = Py_CreateSymbolicLinkA(dst->narrow, src->narrow,
                                        target_is_directory);
    }
    Py_END_ALLOW_THREADS

    if (!result)
        return path_error2(src, dst);

#else

    Py_BEGIN_ALLOW_THREADS
#if HAVE_SYMLINKAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = symlinkat(src->narrow, dir_fd, dst->narrow);
    else
#endif
        result = symlink(src->narrow, dst->narrow);
    Py_END_ALLOW_THREADS

    if (result)
        return path_error2(src, dst);
#endif

    Py_RETURN_NONE;
}
#endif /* HAVE_SYMLINK */




static PyStructSequence_Field times_result_fields[] = {
    {"user",    "user time"},
    {"system",   "system time"},
    {"children_user",    "user time of children"},
    {"children_system",    "system time of children"},
    {"elapsed",    "elapsed time since an arbitrary point in the past"},
    {NULL}
};

PyDoc_STRVAR(times_result__doc__,
"times_result: Result from os.times().\n\n\
This object may be accessed either as a tuple of\n\
  (user, system, children_user, children_system, elapsed),\n\
or via the attributes user, system, children_user, children_system,\n\
and elapsed.\n\
\n\
See os.times for more information.");

static PyStructSequence_Desc times_result_desc = {
    "times_result", /* name */
    times_result__doc__, /* doc */
    times_result_fields,
    5
};

static PyTypeObject TimesResultType;

#ifdef MS_WINDOWS
#define HAVE_TIMES  /* mandatory, for the method table */
#endif

#ifdef HAVE_TIMES

static PyObject *
build_times_result(double user, double system,
    double children_user, double children_system,
    double elapsed)
{
    PyObject *value = PyStructSequence_New(&TimesResultType);
    if (value == NULL)
        return NULL;

#define SET(i, field) \
    { \
    PyObject *o = PyFloat_FromDouble(field); \
    if (!o) { \
        Py_DECREF(value); \
        return NULL; \
    } \
    PyStructSequence_SET_ITEM(value, i, o); \
    } \

    SET(0, user);
    SET(1, system);
    SET(2, children_user);
    SET(3, children_system);
    SET(4, elapsed);

#undef SET

    return value;
}


#ifndef MS_WINDOWS
#define NEED_TICKS_PER_SECOND
static long ticks_per_second = -1;
#endif /* MS_WINDOWS */

/*[clinic input]
os.times

Return a collection containing process timing information.

The object returned behaves like a named tuple with these fields:
  (utime, stime, cutime, cstime, elapsed_time)
All fields are floating point numbers.
[clinic start generated code]*/

PyDoc_STRVAR(os_times__doc__,
"times($module, /)\n"
"--\n"
"\n"
"Return a collection containing process timing information.\n"
"\n"
"The object returned behaves like a named tuple with these fields:\n"
"  (utime, stime, cutime, cstime, elapsed_time)\n"
"All fields are floating point numbers.");

#define OS_TIMES_METHODDEF    \
    {"times", (PyCFunction)os_times, METH_NOARGS, os_times__doc__},

static PyObject *
os_times_impl(PyModuleDef *module);

static PyObject *
os_times(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_times_impl(module);
}

static PyObject *
os_times_impl(PyModuleDef *module)
/*[clinic end generated code: output=b86896d031a9b768 input=2bf9df3d6ab2e48b]*/
#ifdef MS_WINDOWS
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
    return build_times_result(
        (double)(user.dwHighDateTime*429.4967296 +
                 user.dwLowDateTime*1e-7),
        (double)(kernel.dwHighDateTime*429.4967296 +
                 kernel.dwLowDateTime*1e-7),
        (double)0,
        (double)0,
        (double)0);
}
#else /* MS_WINDOWS */
{


    struct tms t;
    clock_t c;
    errno = 0;
    c = times(&t);
    if (c == (clock_t) -1)
        return posix_error();
    return build_times_result(
                         (double)t.tms_utime / ticks_per_second,
                         (double)t.tms_stime / ticks_per_second,
                         (double)t.tms_cutime / ticks_per_second,
                         (double)t.tms_cstime / ticks_per_second,
                         (double)c / ticks_per_second);
}
#endif /* MS_WINDOWS */
#endif /* HAVE_TIMES */


#ifdef HAVE_GETSID
/*[clinic input]
os.getsid

    pid: pid_t
    /

Call the system call getsid(pid) and return the result.
[clinic start generated code]*/

PyDoc_STRVAR(os_getsid__doc__,
"getsid($module, pid, /)\n"
"--\n"
"\n"
"Call the system call getsid(pid) and return the result.");

#define OS_GETSID_METHODDEF    \
    {"getsid", (PyCFunction)os_getsid, METH_VARARGS, os_getsid__doc__},

static PyObject *
os_getsid_impl(PyModuleDef *module, pid_t pid);

static PyObject *
os_getsid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID ":getsid",
        &pid))
        goto exit;
    return_value = os_getsid_impl(module, pid);

exit:
    return return_value;
}

static PyObject *
os_getsid_impl(PyModuleDef *module, pid_t pid)
/*[clinic end generated code: output=ea8390f395f4e0e1 input=eeb2b923a30ce04e]*/
{
    int sid;
    sid = getsid(pid);
    if (sid < 0)
        return posix_error();
    return PyLong_FromLong((long)sid);
}
#endif /* HAVE_GETSID */


#ifdef HAVE_SETSID
/*[clinic input]
os.setsid

Call the system call setsid().
[clinic start generated code]*/

PyDoc_STRVAR(os_setsid__doc__,
"setsid($module, /)\n"
"--\n"
"\n"
"Call the system call setsid().");

#define OS_SETSID_METHODDEF    \
    {"setsid", (PyCFunction)os_setsid, METH_NOARGS, os_setsid__doc__},

static PyObject *
os_setsid_impl(PyModuleDef *module);

static PyObject *
os_setsid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_setsid_impl(module);
}

static PyObject *
os_setsid_impl(PyModuleDef *module)
/*[clinic end generated code: output=2a9a1435d8d764d5 input=5fff45858e2f0776]*/
{
    if (setsid() < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETSID */


#ifdef HAVE_SETPGID
/*[clinic input]
os.setpgid

    pid: pid_t
    pgrp: pid_t
    /

Call the system call setpgid(pid, pgrp).
[clinic start generated code]*/

PyDoc_STRVAR(os_setpgid__doc__,
"setpgid($module, pid, pgrp, /)\n"
"--\n"
"\n"
"Call the system call setpgid(pid, pgrp).");

#define OS_SETPGID_METHODDEF    \
    {"setpgid", (PyCFunction)os_setpgid, METH_VARARGS, os_setpgid__doc__},

static PyObject *
os_setpgid_impl(PyModuleDef *module, pid_t pid, pid_t pgrp);

static PyObject *
os_setpgid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    pid_t pgrp;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_PID "" _Py_PARSE_PID ":setpgid",
        &pid, &pgrp))
        goto exit;
    return_value = os_setpgid_impl(module, pid, pgrp);

exit:
    return return_value;
}

static PyObject *
os_setpgid_impl(PyModuleDef *module, pid_t pid, pid_t pgrp)
/*[clinic end generated code: output=7ad79b725f890e1f input=fceb395eca572e1a]*/
{
    if (setpgid(pid, pgrp) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETPGID */


#ifdef HAVE_TCGETPGRP
/*[clinic input]
os.tcgetpgrp

    fd: int
    /

Return the process group associated with the terminal specified by fd.
[clinic start generated code]*/

PyDoc_STRVAR(os_tcgetpgrp__doc__,
"tcgetpgrp($module, fd, /)\n"
"--\n"
"\n"
"Return the process group associated with the terminal specified by fd.");

#define OS_TCGETPGRP_METHODDEF    \
    {"tcgetpgrp", (PyCFunction)os_tcgetpgrp, METH_VARARGS, os_tcgetpgrp__doc__},

static PyObject *
os_tcgetpgrp_impl(PyModuleDef *module, int fd);

static PyObject *
os_tcgetpgrp(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;

    if (!PyArg_ParseTuple(args,
        "i:tcgetpgrp",
        &fd))
        goto exit;
    return_value = os_tcgetpgrp_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_tcgetpgrp_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=abcf52ed4c8d22cb input=7f6c18eac10ada86]*/
{
    pid_t pgid = tcgetpgrp(fd);
    if (pgid < 0)
        return posix_error();
    return PyLong_FromPid(pgid);
}
#endif /* HAVE_TCGETPGRP */


#ifdef HAVE_TCSETPGRP
/*[clinic input]
os.tcsetpgrp

    fd: int
    pgid: pid_t
    /

Set the process group associated with the terminal specified by fd.
[clinic start generated code]*/

PyDoc_STRVAR(os_tcsetpgrp__doc__,
"tcsetpgrp($module, fd, pgid, /)\n"
"--\n"
"\n"
"Set the process group associated with the terminal specified by fd.");

#define OS_TCSETPGRP_METHODDEF    \
    {"tcsetpgrp", (PyCFunction)os_tcsetpgrp, METH_VARARGS, os_tcsetpgrp__doc__},

static PyObject *
os_tcsetpgrp_impl(PyModuleDef *module, int fd, pid_t pgid);

static PyObject *
os_tcsetpgrp(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    pid_t pgid;

    if (!PyArg_ParseTuple(args,
        "i" _Py_PARSE_PID ":tcsetpgrp",
        &fd, &pgid))
        goto exit;
    return_value = os_tcsetpgrp_impl(module, fd, pgid);

exit:
    return return_value;
}

static PyObject *
os_tcsetpgrp_impl(PyModuleDef *module, int fd, pid_t pgid)
/*[clinic end generated code: output=76f9bb8fd00f20f5 input=5bdc997c6a619020]*/
{
    if (tcsetpgrp(fd, pgid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_TCSETPGRP */

/* Functions acting on file descriptors */

#ifdef O_CLOEXEC
extern int _Py_open_cloexec_works;
#endif


/*[clinic input]
os.open -> int
    path: path_t
    flags: int
    mode: int = 0o777
    *
    dir_fd: dir_fd(requires='openat') = None

# "open(path, flags, mode=0o777, *, dir_fd=None)\n\n\

Open a file for low level IO.  Returns a file descriptor (integer).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_open__doc__,
"open($module, /, path, flags, mode=511, *, dir_fd=None)\n"
"--\n"
"\n"
"Open a file for low level IO.  Returns a file descriptor (integer).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_OPEN_METHODDEF    \
    {"open", (PyCFunction)os_open, METH_VARARGS|METH_KEYWORDS, os_open__doc__},

static int
os_open_impl(PyModuleDef *module, path_t *path, int flags, int mode, int dir_fd);

static PyObject *
os_open(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "flags", "mode", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("open", "path", 0, 0);
    int flags;
    int mode = 511;
    int dir_fd = DEFAULT_DIR_FD;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&i|i$O&:open", _keywords,
        path_converter, &path, &flags, &mode, OPENAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    _return_value = os_open_impl(module, &path, flags, mode, dir_fd);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static int
os_open_impl(PyModuleDef *module, path_t *path, int flags, int mode, int dir_fd)
/*[clinic end generated code: output=05b68fc4ed5e29c9 input=ad8623b29acd2934]*/
{
    int fd;

#ifdef O_CLOEXEC
    int *atomic_flag_works = &_Py_open_cloexec_works;
#elif !defined(MS_WINDOWS)
    int *atomic_flag_works = NULL;
#endif

#ifdef MS_WINDOWS
    flags |= O_NOINHERIT;
#elif defined(O_CLOEXEC)
    flags |= O_CLOEXEC;
#endif

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (path->wide)
        fd = _wopen(path->wide, flags, mode);
    else
#endif
#ifdef HAVE_OPENAT
    if (dir_fd != DEFAULT_DIR_FD)
        fd = openat(dir_fd, path->narrow, flags, mode);
    else
#endif
        fd = open(path->narrow, flags, mode);
    Py_END_ALLOW_THREADS

    if (fd == -1) {
        PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path->object);
        return -1;
    }

#ifndef MS_WINDOWS
    if (_Py_set_inheritable(fd, 0, atomic_flag_works) < 0) {
        close(fd);
        return -1;
    }
#endif

    return fd;
}


/*[clinic input]
os.close

    fd: int

Close a file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_close__doc__,
"close($module, /, fd)\n"
"--\n"
"\n"
"Close a file descriptor.");

#define OS_CLOSE_METHODDEF    \
    {"close", (PyCFunction)os_close, METH_VARARGS|METH_KEYWORDS, os_close__doc__},

static PyObject *
os_close_impl(PyModuleDef *module, int fd);

static PyObject *
os_close(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:close", _keywords,
        &fd))
        goto exit;
    return_value = os_close_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_close_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=927004e29ad55808 input=2bc42451ca5c3223]*/
{
    int res;
    if (!_PyVerify_fd(fd))
        return posix_error();
    Py_BEGIN_ALLOW_THREADS
    res = close(fd);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();
    Py_RETURN_NONE;
}


/*[clinic input]
os.closerange

    fd_low: int
    fd_high: int
    /

Closes all file descriptors in [fd_low, fd_high), ignoring errors.
[clinic start generated code]*/

PyDoc_STRVAR(os_closerange__doc__,
"closerange($module, fd_low, fd_high, /)\n"
"--\n"
"\n"
"Closes all file descriptors in [fd_low, fd_high), ignoring errors.");

#define OS_CLOSERANGE_METHODDEF    \
    {"closerange", (PyCFunction)os_closerange, METH_VARARGS, os_closerange__doc__},

static PyObject *
os_closerange_impl(PyModuleDef *module, int fd_low, int fd_high);

static PyObject *
os_closerange(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd_low;
    int fd_high;

    if (!PyArg_ParseTuple(args,
        "ii:closerange",
        &fd_low, &fd_high))
        goto exit;
    return_value = os_closerange_impl(module, fd_low, fd_high);

exit:
    return return_value;
}

static PyObject *
os_closerange_impl(PyModuleDef *module, int fd_low, int fd_high)
/*[clinic end generated code: output=0a929ece386811c3 input=5855a3d053ebd4ec]*/
{
    int i;
    Py_BEGIN_ALLOW_THREADS
    for (i = fd_low; i < fd_high; i++)
        if (_PyVerify_fd(i))
            close(i);
    Py_END_ALLOW_THREADS
    Py_RETURN_NONE;
}


/*[clinic input]
os.dup -> int

    fd: int
    /

Return a duplicate of a file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_dup__doc__,
"dup($module, fd, /)\n"
"--\n"
"\n"
"Return a duplicate of a file descriptor.");

#define OS_DUP_METHODDEF    \
    {"dup", (PyCFunction)os_dup, METH_VARARGS, os_dup__doc__},

static int
os_dup_impl(PyModuleDef *module, int fd);

static PyObject *
os_dup(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:dup",
        &fd))
        goto exit;
    _return_value = os_dup_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_dup_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=75943e057b25e1bd input=6f10f7ea97f7852a]*/
{
    return _Py_dup(fd);
}


/*[clinic input]
os.dup2
    fd: int
    fd2: int
    inheritable: bool=True

Duplicate file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_dup2__doc__,
"dup2($module, /, fd, fd2, inheritable=True)\n"
"--\n"
"\n"
"Duplicate file descriptor.");

#define OS_DUP2_METHODDEF    \
    {"dup2", (PyCFunction)os_dup2, METH_VARARGS|METH_KEYWORDS, os_dup2__doc__},

static PyObject *
os_dup2_impl(PyModuleDef *module, int fd, int fd2, int inheritable);

static PyObject *
os_dup2(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", "fd2", "inheritable", NULL};
    int fd;
    int fd2;
    int inheritable = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "ii|p:dup2", _keywords,
        &fd, &fd2, &inheritable))
        goto exit;
    return_value = os_dup2_impl(module, fd, fd2, inheritable);

exit:
    return return_value;
}

static PyObject *
os_dup2_impl(PyModuleDef *module, int fd, int fd2, int inheritable)
/*[clinic end generated code: output=531e482dd11a99a0 input=76e96f511be0352f]*/
{
    int res;
#if defined(HAVE_DUP3) && \
    !(defined(HAVE_FCNTL_H) && defined(F_DUP2FD_CLOEXEC))
    /* dup3() is available on Linux 2.6.27+ and glibc 2.9 */
    int dup3_works = -1;
#endif

    if (!_PyVerify_fd_dup2(fd, fd2))
        return posix_error();

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    res = dup2(fd, fd2);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();

    /* Character files like console cannot be make non-inheritable */
    if (!inheritable && _Py_set_inheritable(fd2, 0, NULL) < 0) {
        close(fd2);
        return NULL;
    }

#elif defined(HAVE_FCNTL_H) && defined(F_DUP2FD_CLOEXEC)
    Py_BEGIN_ALLOW_THREADS
    if (!inheritable)
        res = fcntl(fd, F_DUP2FD_CLOEXEC, fd2);
    else
        res = dup2(fd, fd2);
    Py_END_ALLOW_THREADS
    if (res < 0)
        return posix_error();

#else

#ifdef HAVE_DUP3
    if (!inheritable && dup3_works != 0) {
        Py_BEGIN_ALLOW_THREADS
        res = dup3(fd, fd2, O_CLOEXEC);
        Py_END_ALLOW_THREADS
        if (res < 0) {
            if (dup3_works == -1)
                dup3_works = (errno != ENOSYS);
            if (dup3_works)
                return posix_error();
        }
    }

    if (inheritable || dup3_works == 0)
    {
#endif
        Py_BEGIN_ALLOW_THREADS
        res = dup2(fd, fd2);
        Py_END_ALLOW_THREADS
        if (res < 0)
            return posix_error();

        if (!inheritable && _Py_set_inheritable(fd2, 0, NULL) < 0) {
            close(fd2);
            return NULL;
        }
#ifdef HAVE_DUP3
    }
#endif

#endif

    Py_RETURN_NONE;
}


#ifdef HAVE_LOCKF
/*[clinic input]
os.lockf

    fd: int
        An open file descriptor.
    command: int
        One of F_LOCK, F_TLOCK, F_ULOCK or F_TEST.
    length: Py_off_t
        The number of bytes to lock, starting at the current position.
    /

Apply, test or remove a POSIX lock on an open file descriptor.

[clinic start generated code]*/

PyDoc_STRVAR(os_lockf__doc__,
"lockf($module, fd, command, length, /)\n"
"--\n"
"\n"
"Apply, test or remove a POSIX lock on an open file descriptor.\n"
"\n"
"  fd\n"
"    An open file descriptor.\n"
"  command\n"
"    One of F_LOCK, F_TLOCK, F_ULOCK or F_TEST.\n"
"  length\n"
"    The number of bytes to lock, starting at the current position.");

#define OS_LOCKF_METHODDEF    \
    {"lockf", (PyCFunction)os_lockf, METH_VARARGS, os_lockf__doc__},

static PyObject *
os_lockf_impl(PyModuleDef *module, int fd, int command, Py_off_t length);

static PyObject *
os_lockf(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int command;
    Py_off_t length;

    if (!PyArg_ParseTuple(args,
        "iiO&:lockf",
        &fd, &command, Py_off_t_converter, &length))
        goto exit;
    return_value = os_lockf_impl(module, fd, command, length);

exit:
    return return_value;
}

static PyObject *
os_lockf_impl(PyModuleDef *module, int fd, int command, Py_off_t length)
/*[clinic end generated code: output=1b28346ac7335c0f input=65da41d2106e9b79]*/
{
    int res;

    Py_BEGIN_ALLOW_THREADS
    res = lockf(fd, command, length);
    Py_END_ALLOW_THREADS

    if (res < 0)
        return posix_error();

    Py_RETURN_NONE;
}
#endif /* HAVE_LOCKF */


/*[clinic input]
os.lseek -> Py_off_t

    fd: int
    position: Py_off_t
    how: int
    /

Set the position of a file descriptor.  Return the new position.

Return the new cursor position in number of bytes
relative to the beginning of the file.
[clinic start generated code]*/

PyDoc_STRVAR(os_lseek__doc__,
"lseek($module, fd, position, how, /)\n"
"--\n"
"\n"
"Set the position of a file descriptor.  Return the new position.\n"
"\n"
"Return the new cursor position in number of bytes\n"
"relative to the beginning of the file.");

#define OS_LSEEK_METHODDEF    \
    {"lseek", (PyCFunction)os_lseek, METH_VARARGS, os_lseek__doc__},

static Py_off_t
os_lseek_impl(PyModuleDef *module, int fd, Py_off_t position, int how);

static PyObject *
os_lseek(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t position;
    int how;
    Py_off_t _return_value;

    if (!PyArg_ParseTuple(args,
        "iO&i:lseek",
        &fd, Py_off_t_converter, &position, &how))
        goto exit;
    _return_value = os_lseek_impl(module, fd, position, how);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromPy_off_t(_return_value);

exit:
    return return_value;
}

static Py_off_t
os_lseek_impl(PyModuleDef *module, int fd, Py_off_t position, int how)
/*[clinic end generated code: output=88cfc146f55667af input=902654ad3f96a6d3]*/
{
    Py_off_t result;

    if (!_PyVerify_fd(fd)) {
        posix_error();
        return -1;
    }
#ifdef SEEK_SET
    /* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
    switch (how) {
        case 0: how = SEEK_SET; break;
        case 1: how = SEEK_CUR; break;
        case 2: how = SEEK_END; break;
    }
#endif /* SEEK_END */

    if (PyErr_Occurred())
        return -1;

    if (!_PyVerify_fd(fd)) {
        posix_error();
        return -1;
    }
    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    result = _lseeki64(fd, position, how);
#else
    result = lseek(fd, position, how);
#endif
    Py_END_ALLOW_THREADS
    if (result < 0)
        posix_error();

    return result;
}


/*[clinic input]
os.read
    fd: int
    length: Py_ssize_t
    /

Read from a file descriptor.  Returns a bytes object.
[clinic start generated code]*/

PyDoc_STRVAR(os_read__doc__,
"read($module, fd, length, /)\n"
"--\n"
"\n"
"Read from a file descriptor.  Returns a bytes object.");

#define OS_READ_METHODDEF    \
    {"read", (PyCFunction)os_read, METH_VARARGS, os_read__doc__},

static PyObject *
os_read_impl(PyModuleDef *module, int fd, Py_ssize_t length);

static PyObject *
os_read(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_ssize_t length;

    if (!PyArg_ParseTuple(args,
        "in:read",
        &fd, &length))
        goto exit;
    return_value = os_read_impl(module, fd, length);

exit:
    return return_value;
}

static PyObject *
os_read_impl(PyModuleDef *module, int fd, Py_ssize_t length)
/*[clinic end generated code: output=1f3bc27260a24968 input=1df2eaa27c0bf1d3]*/
{
    Py_ssize_t n;
    PyObject *buffer;

    if (length < 0) {
        errno = EINVAL;
        return posix_error();
    }
    if (!_PyVerify_fd(fd))
        return posix_error();

#ifdef MS_WINDOWS
    #define READ_CAST (int)
    if (length > INT_MAX)
        length = INT_MAX;
#else
    #define READ_CAST
#endif

    buffer = PyBytes_FromStringAndSize((char *)NULL, length);
    if (buffer == NULL)
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    n = read(fd, PyBytes_AS_STRING(buffer), READ_CAST length);
    Py_END_ALLOW_THREADS

    if (n < 0) {
        Py_DECREF(buffer);
        return posix_error();
    }

    if (n != length)
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
        return -1;
    }

    *buf = PyMem_New(Py_buffer, cnt);
    if (*buf == NULL) {
        PyMem_Del(*iov);
        PyErr_NoMemory();
        return -1;
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
    return -1;
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
/*[clinic input]
os.readv -> Py_ssize_t

    fd: int
    buffers: object
    /

Read from a file descriptor fd into an iterable of buffers.

The buffers should be mutable buffers accepting bytes.
readv will transfer data into each buffer until it is full
and then move on to the next buffer in the sequence to hold
the rest of the data.

readv returns the total number of bytes read,
which may be less than the total capacity of all the buffers.
[clinic start generated code]*/

PyDoc_STRVAR(os_readv__doc__,
"readv($module, fd, buffers, /)\n"
"--\n"
"\n"
"Read from a file descriptor fd into an iterable of buffers.\n"
"\n"
"The buffers should be mutable buffers accepting bytes.\n"
"readv will transfer data into each buffer until it is full\n"
"and then move on to the next buffer in the sequence to hold\n"
"the rest of the data.\n"
"\n"
"readv returns the total number of bytes read,\n"
"which may be less than the total capacity of all the buffers.");

#define OS_READV_METHODDEF    \
    {"readv", (PyCFunction)os_readv, METH_VARARGS, os_readv__doc__},

static Py_ssize_t
os_readv_impl(PyModuleDef *module, int fd, PyObject *buffers);

static PyObject *
os_readv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!PyArg_ParseTuple(args,
        "iO:readv",
        &fd, &buffers))
        goto exit;
    _return_value = os_readv_impl(module, fd, buffers);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

static Py_ssize_t
os_readv_impl(PyModuleDef *module, int fd, PyObject *buffers)
/*[clinic end generated code: output=72748b1c32a6e2a1 input=e679eb5dbfa0357d]*/
{
    int cnt;
    Py_ssize_t n;
    struct iovec *iov;
    Py_buffer *buf;

    if (!PySequence_Check(buffers)) {
        PyErr_SetString(PyExc_TypeError,
            "readv() arg 2 must be a sequence");
        return -1;
    }

    cnt = PySequence_Size(buffers);

    if (iov_setup(&iov, &buf, buffers, cnt, PyBUF_WRITABLE) < 0)
        return -1;

    Py_BEGIN_ALLOW_THREADS
    n = readv(fd, iov, cnt);
    Py_END_ALLOW_THREADS

    iov_cleanup(iov, buf, cnt);
    if (n < 0) {
        posix_error();
        return -1;
    }

    return n;
}
#endif /* HAVE_READV */


#ifdef HAVE_PREAD
/*[clinic input]
# TODO length should be size_t!  but Python doesn't support parsing size_t yet.
os.pread

    fd: int
    length: int
    offset: Py_off_t
    /

Read a number of bytes from a file descriptor starting at a particular offset.

Read length bytes from file descriptor fd, starting at offset bytes from
the beginning of the file.  The file offset remains unchanged.
[clinic start generated code]*/

PyDoc_STRVAR(os_pread__doc__,
"pread($module, fd, length, offset, /)\n"
"--\n"
"\n"
"Read a number of bytes from a file descriptor starting at a particular offset.\n"
"\n"
"Read length bytes from file descriptor fd, starting at offset bytes from\n"
"the beginning of the file.  The file offset remains unchanged.");

#define OS_PREAD_METHODDEF    \
    {"pread", (PyCFunction)os_pread, METH_VARARGS, os_pread__doc__},

static PyObject *
os_pread_impl(PyModuleDef *module, int fd, int length, Py_off_t offset);

static PyObject *
os_pread(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int length;
    Py_off_t offset;

    if (!PyArg_ParseTuple(args,
        "iiO&:pread",
        &fd, &length, Py_off_t_converter, &offset))
        goto exit;
    return_value = os_pread_impl(module, fd, length, offset);

exit:
    return return_value;
}

static PyObject *
os_pread_impl(PyModuleDef *module, int fd, int length, Py_off_t offset)
/*[clinic end generated code: output=7b62bf6c06e20ae8 input=084948dcbaa35d4c]*/
{
    Py_ssize_t n;
    PyObject *buffer;

    if (length < 0) {
        errno = EINVAL;
        return posix_error();
    }
    buffer = PyBytes_FromStringAndSize((char *)NULL, length);
    if (buffer == NULL)
        return NULL;
    if (!_PyVerify_fd(fd)) {
        Py_DECREF(buffer);
        return posix_error();
    }
    Py_BEGIN_ALLOW_THREADS
    n = pread(fd, PyBytes_AS_STRING(buffer), length, offset);
    Py_END_ALLOW_THREADS
    if (n < 0) {
        Py_DECREF(buffer);
        return posix_error();
    }
    if (n != length)
        _PyBytes_Resize(&buffer, n);
    return buffer;
}
#endif /* HAVE_PREAD */


/*[clinic input]
os.write -> Py_ssize_t

    fd: int
    data: Py_buffer
    /

Write a bytes object to a file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_write__doc__,
"write($module, fd, data, /)\n"
"--\n"
"\n"
"Write a bytes object to a file descriptor.");

#define OS_WRITE_METHODDEF    \
    {"write", (PyCFunction)os_write, METH_VARARGS, os_write__doc__},

static Py_ssize_t
os_write_impl(PyModuleDef *module, int fd, Py_buffer *data);

static PyObject *
os_write(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t _return_value;

    if (!PyArg_ParseTuple(args,
        "iy*:write",
        &fd, &data))
        goto exit;
    _return_value = os_write_impl(module, fd, &data);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

static Py_ssize_t
os_write_impl(PyModuleDef *module, int fd, Py_buffer *data)
/*[clinic end generated code: output=aeb96acfdd4d5112 input=3207e28963234f3c]*/
{
    Py_ssize_t size;
    Py_ssize_t len = data->len;

    if (!_PyVerify_fd(fd)) {
        posix_error();
        return -1;
    }

    Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
    if (len > INT_MAX)
        len = INT_MAX;
    size = write(fd, data->buf, (int)len);
#else
    size = write(fd, data->buf, len);
#endif
    Py_END_ALLOW_THREADS
    if (size < 0) {
        posix_error();
        return -1;
    }
    return size;
}

#ifdef HAVE_SENDFILE
PyDoc_STRVAR(posix_sendfile__doc__,
"sendfile(out, in, offset, nbytes) -> byteswritten\n\
sendfile(out, in, offset, nbytes, headers=None, trailers=None, flags=0)\n\
            -> byteswritten\n\
Copy nbytes bytes from file descriptor in to file descriptor out.");

/* AC 3.5: don't bother converting, has optional group*/
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
    static char *keywords[] = {"out", "in",
                                "offset", "count",
                                "headers", "trailers", "flags", NULL};

    sf.headers = NULL;
    sf.trailers = NULL;

#ifdef __APPLE__
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "iiO&O&|OOi:sendfile",
        keywords, &out, &in, Py_off_t_converter, &offset, Py_off_t_converter, &sbytes,
#else
    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "iiO&n|OOi:sendfile",
        keywords, &out, &in, Py_off_t_converter, &offset, &len,
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
                (i = iov_setup(&(sf.headers), &hbuf,
                                headers, sf.hdr_cnt, PyBUF_SIMPLE)) < 0)
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
                (i = iov_setup(&(sf.trailers), &tbuf,
                                trailers, sf.trl_cnt, PyBUF_SIMPLE)) < 0)
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
    if (!Py_off_t_converter(offobj, &offset))
        return NULL;
    Py_BEGIN_ALLOW_THREADS
    ret = sendfile(out, in, &offset, count);
    Py_END_ALLOW_THREADS
    if (ret < 0)
        return posix_error();
    return Py_BuildValue("n", ret);
#endif
}
#endif /* HAVE_SENDFILE */


/*[clinic input]
os.fstat

    fd : int

Perform a stat system call on the given file descriptor.

Like stat(), but for an open file descriptor.
Equivalent to os.stat(fd).
[clinic start generated code]*/

PyDoc_STRVAR(os_fstat__doc__,
"fstat($module, /, fd)\n"
"--\n"
"\n"
"Perform a stat system call on the given file descriptor.\n"
"\n"
"Like stat(), but for an open file descriptor.\n"
"Equivalent to os.stat(fd).");

#define OS_FSTAT_METHODDEF    \
    {"fstat", (PyCFunction)os_fstat, METH_VARARGS|METH_KEYWORDS, os_fstat__doc__},

static PyObject *
os_fstat_impl(PyModuleDef *module, int fd);

static PyObject *
os_fstat(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:fstat", _keywords,
        &fd))
        goto exit;
    return_value = os_fstat_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_fstat_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=dae4a9678c7bd881 input=27e0e0ebbe5600c9]*/
{
    STRUCT_STAT st;
    int res;

    Py_BEGIN_ALLOW_THREADS
    res = FSTAT(fd, &st);
    Py_END_ALLOW_THREADS
    if (res != 0) {
#ifdef MS_WINDOWS
        return PyErr_SetFromWindowsErr(0);
#else
        return posix_error();
#endif
    }

    return _pystat_fromstructstat(&st);
}


/*[clinic input]
os.isatty -> bool
    fd: int
    /

Return True if the fd is connected to a terminal.

Return True if the file descriptor is an open file descriptor
connected to the slave end of a terminal.
[clinic start generated code]*/

PyDoc_STRVAR(os_isatty__doc__,
"isatty($module, fd, /)\n"
"--\n"
"\n"
"Return True if the fd is connected to a terminal.\n"
"\n"
"Return True if the file descriptor is an open file descriptor\n"
"connected to the slave end of a terminal.");

#define OS_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)os_isatty, METH_VARARGS, os_isatty__doc__},

static int
os_isatty_impl(PyModuleDef *module, int fd);

static PyObject *
os_isatty(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:isatty",
        &fd))
        goto exit;
    _return_value = os_isatty_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_isatty_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=4bfadbfe22715097 input=08ce94aa1eaf7b5e]*/
{
    if (!_PyVerify_fd(fd))
        return 0;
    return isatty(fd);
}


#ifdef HAVE_PIPE
/*[clinic input]
os.pipe

Create a pipe.

Returns a tuple of two file descriptors:
  (read_fd, write_fd)
[clinic start generated code]*/

PyDoc_STRVAR(os_pipe__doc__,
"pipe($module, /)\n"
"--\n"
"\n"
"Create a pipe.\n"
"\n"
"Returns a tuple of two file descriptors:\n"
"  (read_fd, write_fd)");

#define OS_PIPE_METHODDEF    \
    {"pipe", (PyCFunction)os_pipe, METH_NOARGS, os_pipe__doc__},

static PyObject *
os_pipe_impl(PyModuleDef *module);

static PyObject *
os_pipe(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_pipe_impl(module);
}

static PyObject *
os_pipe_impl(PyModuleDef *module)
/*[clinic end generated code: output=0da2479f2266e774 input=02535e8c8fa6c4d4]*/
{
    int fds[2];
#ifdef MS_WINDOWS
    HANDLE read, write;
    SECURITY_ATTRIBUTES attr;
    BOOL ok;
#else
    int res;
#endif

#ifdef MS_WINDOWS
    attr.nLength = sizeof(attr);
    attr.lpSecurityDescriptor = NULL;
    attr.bInheritHandle = FALSE;

    Py_BEGIN_ALLOW_THREADS
    ok = CreatePipe(&read, &write, &attr, 0);
    if (ok) {
        fds[0] = _open_osfhandle((Py_intptr_t)read, _O_RDONLY);
        fds[1] = _open_osfhandle((Py_intptr_t)write, _O_WRONLY);
        if (fds[0] == -1 || fds[1] == -1) {
            CloseHandle(read);
            CloseHandle(write);
            ok = 0;
        }
    }
    Py_END_ALLOW_THREADS

    if (!ok)
        return PyErr_SetFromWindowsErr(0);
#else

#ifdef HAVE_PIPE2
    Py_BEGIN_ALLOW_THREADS
    res = pipe2(fds, O_CLOEXEC);
    Py_END_ALLOW_THREADS

    if (res != 0 && errno == ENOSYS)
    {
#endif
        Py_BEGIN_ALLOW_THREADS
        res = pipe(fds);
        Py_END_ALLOW_THREADS

        if (res == 0) {
            if (_Py_set_inheritable(fds[0], 0, NULL) < 0) {
                close(fds[0]);
                close(fds[1]);
                return NULL;
            }
            if (_Py_set_inheritable(fds[1], 0, NULL) < 0) {
                close(fds[0]);
                close(fds[1]);
                return NULL;
            }
        }
#ifdef HAVE_PIPE2
    }
#endif

    if (res != 0)
        return PyErr_SetFromErrno(PyExc_OSError);
#endif /* !MS_WINDOWS */
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}
#endif  /* HAVE_PIPE */


#ifdef HAVE_PIPE2
/*[clinic input]
os.pipe2

    flags: int
    /

Create a pipe with flags set atomically.

Returns a tuple of two file descriptors:
  (read_fd, write_fd)

flags can be constructed by ORing together one or more of these values:
O_NONBLOCK, O_CLOEXEC.
[clinic start generated code]*/

PyDoc_STRVAR(os_pipe2__doc__,
"pipe2($module, flags, /)\n"
"--\n"
"\n"
"Create a pipe with flags set atomically.\n"
"\n"
"Returns a tuple of two file descriptors:\n"
"  (read_fd, write_fd)\n"
"\n"
"flags can be constructed by ORing together one or more of these values:\n"
"O_NONBLOCK, O_CLOEXEC.");

#define OS_PIPE2_METHODDEF    \
    {"pipe2", (PyCFunction)os_pipe2, METH_VARARGS, os_pipe2__doc__},

static PyObject *
os_pipe2_impl(PyModuleDef *module, int flags);

static PyObject *
os_pipe2(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int flags;

    if (!PyArg_ParseTuple(args,
        "i:pipe2",
        &flags))
        goto exit;
    return_value = os_pipe2_impl(module, flags);

exit:
    return return_value;
}

static PyObject *
os_pipe2_impl(PyModuleDef *module, int flags)
/*[clinic end generated code: output=9e27c799ce19220b input=f261b6e7e63c6817]*/
{
    int fds[2];
    int res;

    res = pipe2(fds, flags);
    if (res != 0)
        return posix_error();
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}
#endif /* HAVE_PIPE2 */


#ifdef HAVE_WRITEV
/*[clinic input]
os.writev -> Py_ssize_t
    fd: int
    buffers: object
    /

Iterate over buffers, and write the contents of each to a file descriptor.

Returns the total number of bytes written.
buffers must be a sequence of bytes-like objects.
[clinic start generated code]*/

PyDoc_STRVAR(os_writev__doc__,
"writev($module, fd, buffers, /)\n"
"--\n"
"\n"
"Iterate over buffers, and write the contents of each to a file descriptor.\n"
"\n"
"Returns the total number of bytes written.\n"
"buffers must be a sequence of bytes-like objects.");

#define OS_WRITEV_METHODDEF    \
    {"writev", (PyCFunction)os_writev, METH_VARARGS, os_writev__doc__},

static Py_ssize_t
os_writev_impl(PyModuleDef *module, int fd, PyObject *buffers);

static PyObject *
os_writev(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *buffers;
    Py_ssize_t _return_value;

    if (!PyArg_ParseTuple(args,
        "iO:writev",
        &fd, &buffers))
        goto exit;
    _return_value = os_writev_impl(module, fd, buffers);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

static Py_ssize_t
os_writev_impl(PyModuleDef *module, int fd, PyObject *buffers)
/*[clinic end generated code: output=591c662dccbe4951 input=5b8d17fe4189d2fe]*/
{
    int cnt;
    Py_ssize_t result;
    struct iovec *iov;
    Py_buffer *buf;

    if (!PySequence_Check(buffers)) {
        PyErr_SetString(PyExc_TypeError,
            "writev() arg 2 must be a sequence");
        return -1;
    }
    cnt = PySequence_Size(buffers);

    if (iov_setup(&iov, &buf, buffers, cnt, PyBUF_SIMPLE) < 0) {
        return -1;
    }

    Py_BEGIN_ALLOW_THREADS
    result = writev(fd, iov, cnt);
    Py_END_ALLOW_THREADS

    iov_cleanup(iov, buf, cnt);
    if (result < 0)
        posix_error();

    return result;
}
#endif /* HAVE_WRITEV */


#ifdef HAVE_PWRITE
/*[clinic input]
os.pwrite -> Py_ssize_t

    fd: int
    buffer: Py_buffer
    offset: Py_off_t
    /

Write bytes to a file descriptor starting at a particular offset.

Write buffer to fd, starting at offset bytes from the beginning of
the file.  Returns the number of bytes writte.  Does not change the
current file offset.
[clinic start generated code]*/

PyDoc_STRVAR(os_pwrite__doc__,
"pwrite($module, fd, buffer, offset, /)\n"
"--\n"
"\n"
"Write bytes to a file descriptor starting at a particular offset.\n"
"\n"
"Write buffer to fd, starting at offset bytes from the beginning of\n"
"the file.  Returns the number of bytes writte.  Does not change the\n"
"current file offset.");

#define OS_PWRITE_METHODDEF    \
    {"pwrite", (PyCFunction)os_pwrite, METH_VARARGS, os_pwrite__doc__},

static Py_ssize_t
os_pwrite_impl(PyModuleDef *module, int fd, Py_buffer *buffer, Py_off_t offset);

static PyObject *
os_pwrite(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_buffer buffer = {NULL, NULL};
    Py_off_t offset;
    Py_ssize_t _return_value;

    if (!PyArg_ParseTuple(args,
        "iy*O&:pwrite",
        &fd, &buffer, Py_off_t_converter, &offset))
        goto exit;
    _return_value = os_pwrite_impl(module, fd, &buffer, offset);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    /* Cleanup for buffer */
    if (buffer.obj)
       PyBuffer_Release(&buffer);

    return return_value;
}

static Py_ssize_t
os_pwrite_impl(PyModuleDef *module, int fd, Py_buffer *buffer, Py_off_t offset)
/*[clinic end generated code: output=ec9cc5b2238e96a7 input=19903f1b3dd26377]*/
{
    Py_ssize_t size;

    if (!_PyVerify_fd(fd)) {
        posix_error();
        return -1;
    }

    Py_BEGIN_ALLOW_THREADS
    size = pwrite(fd, buffer->buf, (size_t)buffer->len, offset);
    Py_END_ALLOW_THREADS

    if (size < 0)
        posix_error();
    return size;
}
#endif /* HAVE_PWRITE */


#ifdef HAVE_MKFIFO
/*[clinic input]
os.mkfifo

    path: path_t
    mode: int=0o666
    *
    dir_fd: dir_fd(requires='mkfifoat')=None

Create a "fifo" (a POSIX named pipe).

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_mkfifo__doc__,
"mkfifo($module, /, path, mode=438, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a \"fifo\" (a POSIX named pipe).\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_MKFIFO_METHODDEF    \
    {"mkfifo", (PyCFunction)os_mkfifo, METH_VARARGS|METH_KEYWORDS, os_mkfifo__doc__},

static PyObject *
os_mkfifo_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd);

static PyObject *
os_mkfifo(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("mkfifo", "path", 0, 0);
    int mode = 438;
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|i$O&:mkfifo", _keywords,
        path_converter, &path, &mode, MKFIFOAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_mkfifo_impl(module, &path, mode, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_mkfifo_impl(PyModuleDef *module, path_t *path, int mode, int dir_fd)
/*[clinic end generated code: output=b3321927546893d0 input=73032e98a36e0e19]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_MKFIFOAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mkfifoat(dir_fd, path->narrow, mode);
    else
#endif
        result = mkfifo(path->narrow, mode);
    Py_END_ALLOW_THREADS

    if (result < 0)
        return posix_error();

    Py_RETURN_NONE;
}
#endif /* HAVE_MKFIFO */


#if defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV)
/*[clinic input]
os.mknod

    path: path_t
    mode: int=0o600
    device: int=0
    *
    dir_fd: dir_fd(requires='mknodat')=None

Create a node in the file system.

Create a node in the file system (file, device special file or named pipe)
at path.  mode specifies both the permissions to use and the
type of node to be created, being combined (bitwise OR) with one of
S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO.  If S_IFCHR or S_IFBLK is set on mode,
device defines the newly created device special file (probably using
os.makedev()).  Otherwise device is ignored.

If dir_fd is not None, it should be a file descriptor open to a directory,
  and path should be relative; path will then be relative to that directory.
dir_fd may not be implemented on your platform.
  If it is unavailable, using it will raise a NotImplementedError.
[clinic start generated code]*/

PyDoc_STRVAR(os_mknod__doc__,
"mknod($module, /, path, mode=384, device=0, *, dir_fd=None)\n"
"--\n"
"\n"
"Create a node in the file system.\n"
"\n"
"Create a node in the file system (file, device special file or named pipe)\n"
"at path.  mode specifies both the permissions to use and the\n"
"type of node to be created, being combined (bitwise OR) with one of\n"
"S_IFREG, S_IFCHR, S_IFBLK, and S_IFIFO.  If S_IFCHR or S_IFBLK is set on mode,\n"
"device defines the newly created device special file (probably using\n"
"os.makedev()).  Otherwise device is ignored.\n"
"\n"
"If dir_fd is not None, it should be a file descriptor open to a directory,\n"
"  and path should be relative; path will then be relative to that directory.\n"
"dir_fd may not be implemented on your platform.\n"
"  If it is unavailable, using it will raise a NotImplementedError.");

#define OS_MKNOD_METHODDEF    \
    {"mknod", (PyCFunction)os_mknod, METH_VARARGS|METH_KEYWORDS, os_mknod__doc__},

static PyObject *
os_mknod_impl(PyModuleDef *module, path_t *path, int mode, int device, int dir_fd);

static PyObject *
os_mknod(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "mode", "device", "dir_fd", NULL};
    path_t path = PATH_T_INITIALIZE("mknod", "path", 0, 0);
    int mode = 384;
    int device = 0;
    int dir_fd = DEFAULT_DIR_FD;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&|ii$O&:mknod", _keywords,
        path_converter, &path, &mode, &device, MKNODAT_DIR_FD_CONVERTER, &dir_fd))
        goto exit;
    return_value = os_mknod_impl(module, &path, mode, device, dir_fd);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_mknod_impl(PyModuleDef *module, path_t *path, int mode, int device, int dir_fd)
/*[clinic end generated code: output=c688739c15ca7bbb input=30e02126aba9732e]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_MKNODAT
    if (dir_fd != DEFAULT_DIR_FD)
        result = mknodat(dir_fd, path->narrow, mode, device);
    else
#endif
        result = mknod(path->narrow, mode, device);
    Py_END_ALLOW_THREADS

    if (result < 0)
        return posix_error();

    Py_RETURN_NONE;
}
#endif /* defined(HAVE_MKNOD) && defined(HAVE_MAKEDEV) */


#ifdef HAVE_DEVICE_MACROS
/*[clinic input]
os.major -> unsigned_int

    device: int
    /

Extracts a device major number from a raw device number.
[clinic start generated code]*/

PyDoc_STRVAR(os_major__doc__,
"major($module, device, /)\n"
"--\n"
"\n"
"Extracts a device major number from a raw device number.");

#define OS_MAJOR_METHODDEF    \
    {"major", (PyCFunction)os_major, METH_VARARGS, os_major__doc__},

static unsigned int
os_major_impl(PyModuleDef *module, int device);

static PyObject *
os_major(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int device;
    unsigned int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:major",
        &device))
        goto exit;
    _return_value = os_major_impl(module, device);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

static unsigned int
os_major_impl(PyModuleDef *module, int device)
/*[clinic end generated code: output=52e6743300dcf4ad input=ea48820b7e10d310]*/
{
    return major(device);
}


/*[clinic input]
os.minor -> unsigned_int

    device: int
    /

Extracts a device minor number from a raw device number.
[clinic start generated code]*/

PyDoc_STRVAR(os_minor__doc__,
"minor($module, device, /)\n"
"--\n"
"\n"
"Extracts a device minor number from a raw device number.");

#define OS_MINOR_METHODDEF    \
    {"minor", (PyCFunction)os_minor, METH_VARARGS, os_minor__doc__},

static unsigned int
os_minor_impl(PyModuleDef *module, int device);

static PyObject *
os_minor(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int device;
    unsigned int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:minor",
        &device))
        goto exit;
    _return_value = os_minor_impl(module, device);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

static unsigned int
os_minor_impl(PyModuleDef *module, int device)
/*[clinic end generated code: output=aebe4bd7f455b755 input=089733ebbf9754e8]*/
{
    return minor(device);
}


/*[clinic input]
os.makedev -> unsigned_int

    major: int
    minor: int
    /

Composes a raw device number from the major and minor device numbers.
[clinic start generated code]*/

PyDoc_STRVAR(os_makedev__doc__,
"makedev($module, major, minor, /)\n"
"--\n"
"\n"
"Composes a raw device number from the major and minor device numbers.");

#define OS_MAKEDEV_METHODDEF    \
    {"makedev", (PyCFunction)os_makedev, METH_VARARGS, os_makedev__doc__},

static unsigned int
os_makedev_impl(PyModuleDef *module, int major, int minor);

static PyObject *
os_makedev(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int major;
    int minor;
    unsigned int _return_value;

    if (!PyArg_ParseTuple(args,
        "ii:makedev",
        &major, &minor))
        goto exit;
    _return_value = os_makedev_impl(module, major, minor);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    return return_value;
}

static unsigned int
os_makedev_impl(PyModuleDef *module, int major, int minor)
/*[clinic end generated code: output=5cb79d9c9eac58b0 input=f55bf7cffb028a08]*/
{
    return makedev(major, minor);
}
#endif /* HAVE_DEVICE_MACROS */


#ifdef HAVE_FTRUNCATE
/*[clinic input]
os.ftruncate

    fd: int
    length: Py_off_t
    /

Truncate a file, specified by file descriptor, to a specific length.
[clinic start generated code]*/

PyDoc_STRVAR(os_ftruncate__doc__,
"ftruncate($module, fd, length, /)\n"
"--\n"
"\n"
"Truncate a file, specified by file descriptor, to a specific length.");

#define OS_FTRUNCATE_METHODDEF    \
    {"ftruncate", (PyCFunction)os_ftruncate, METH_VARARGS, os_ftruncate__doc__},

static PyObject *
os_ftruncate_impl(PyModuleDef *module, int fd, Py_off_t length);

static PyObject *
os_ftruncate(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t length;

    if (!PyArg_ParseTuple(args,
        "iO&:ftruncate",
        &fd, Py_off_t_converter, &length))
        goto exit;
    return_value = os_ftruncate_impl(module, fd, length);

exit:
    return return_value;
}

static PyObject *
os_ftruncate_impl(PyModuleDef *module, int fd, Py_off_t length)
/*[clinic end generated code: output=62326766cb9b76bf input=63b43641e52818f2]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
    result = ftruncate(fd, length);
    Py_END_ALLOW_THREADS
    if (result < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_FTRUNCATE */


#ifdef HAVE_TRUNCATE
/*[clinic input]
os.truncate
    path: path_t(allow_fd='PATH_HAVE_FTRUNCATE')
    length: Py_off_t

Truncate a file, specified by path, to a specific length.

On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.
[clinic start generated code]*/

PyDoc_STRVAR(os_truncate__doc__,
"truncate($module, /, path, length)\n"
"--\n"
"\n"
"Truncate a file, specified by path, to a specific length.\n"
"\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)os_truncate, METH_VARARGS|METH_KEYWORDS, os_truncate__doc__},

static PyObject *
os_truncate_impl(PyModuleDef *module, path_t *path, Py_off_t length);

static PyObject *
os_truncate(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "length", NULL};
    path_t path = PATH_T_INITIALIZE("truncate", "path", 0, PATH_HAVE_FTRUNCATE);
    Py_off_t length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&:truncate", _keywords,
        path_converter, &path, Py_off_t_converter, &length))
        goto exit;
    return_value = os_truncate_impl(module, &path, length);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_truncate_impl(PyModuleDef *module, path_t *path, Py_off_t length)
/*[clinic end generated code: output=6bd76262d2e027c6 input=77229cf0b50a9b77]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FTRUNCATE
    if (path->fd != -1)
        result = ftruncate(path->fd, length);
    else
#endif
        result = truncate(path->narrow, length);
    Py_END_ALLOW_THREADS
    if (result < 0)
        return path_error(path);

    Py_RETURN_NONE;
}
#endif /* HAVE_TRUNCATE */


/* Issue #22396: On 32-bit AIX platform, the prototypes of os.posix_fadvise()
   and os.posix_fallocate() in system headers are wrong if _LARGE_FILES is
   defined, which is the case in Python on AIX. AIX bug report:
   http://www-01.ibm.com/support/docview.wss?uid=isg1IV56170 */
#if defined(_AIX) && defined(_LARGE_FILES) && !defined(__64BIT__)
#  define POSIX_FADVISE_AIX_BUG
#endif


#if defined(HAVE_POSIX_FALLOCATE) && !defined(POSIX_FADVISE_AIX_BUG)
/*[clinic input]
os.posix_fallocate

    fd: int
    offset: Py_off_t
    length: Py_off_t
    /

Ensure a file has allocated at least a particular number of bytes on disk.

Ensure that the file specified by fd encompasses a range of bytes
starting at offset bytes from the beginning and continuing for length bytes.
[clinic start generated code]*/

PyDoc_STRVAR(os_posix_fallocate__doc__,
"posix_fallocate($module, fd, offset, length, /)\n"
"--\n"
"\n"
"Ensure a file has allocated at least a particular number of bytes on disk.\n"
"\n"
"Ensure that the file specified by fd encompasses a range of bytes\n"
"starting at offset bytes from the beginning and continuing for length bytes.");

#define OS_POSIX_FALLOCATE_METHODDEF    \
    {"posix_fallocate", (PyCFunction)os_posix_fallocate, METH_VARARGS, os_posix_fallocate__doc__},

static PyObject *
os_posix_fallocate_impl(PyModuleDef *module, int fd, Py_off_t offset, Py_off_t length);

static PyObject *
os_posix_fallocate(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t offset;
    Py_off_t length;

    if (!PyArg_ParseTuple(args,
        "iO&O&:posix_fallocate",
        &fd, Py_off_t_converter, &offset, Py_off_t_converter, &length))
        goto exit;
    return_value = os_posix_fallocate_impl(module, fd, offset, length);

exit:
    return return_value;
}

static PyObject *
os_posix_fallocate_impl(PyModuleDef *module, int fd, Py_off_t offset, Py_off_t length)
/*[clinic end generated code: output=0cd702d2065c79db input=d7a2ef0ab2ca52fb]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
    result = posix_fallocate(fd, offset, length);
    Py_END_ALLOW_THREADS
    if (result != 0) {
        errno = result;
        return posix_error();
    }
    Py_RETURN_NONE;
}
#endif /* HAVE_POSIX_FALLOCATE) && !POSIX_FADVISE_AIX_BUG */


#if defined(HAVE_POSIX_FADVISE) && !defined(POSIX_FADVISE_AIX_BUG)
/*[clinic input]
os.posix_fadvise

    fd: int
    offset: Py_off_t
    length: Py_off_t
    advice: int
    /

Announce an intention to access data in a specific pattern.

Announce an intention to access data in a specific pattern, thus allowing
the kernel to make optimizations.
The advice applies to the region of the file specified by fd starting at
offset and continuing for length bytes.
advice is one of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,
POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED, or
POSIX_FADV_DONTNEED.
[clinic start generated code]*/

PyDoc_STRVAR(os_posix_fadvise__doc__,
"posix_fadvise($module, fd, offset, length, advice, /)\n"
"--\n"
"\n"
"Announce an intention to access data in a specific pattern.\n"
"\n"
"Announce an intention to access data in a specific pattern, thus allowing\n"
"the kernel to make optimizations.\n"
"The advice applies to the region of the file specified by fd starting at\n"
"offset and continuing for length bytes.\n"
"advice is one of POSIX_FADV_NORMAL, POSIX_FADV_SEQUENTIAL,\n"
"POSIX_FADV_RANDOM, POSIX_FADV_NOREUSE, POSIX_FADV_WILLNEED, or\n"
"POSIX_FADV_DONTNEED.");

#define OS_POSIX_FADVISE_METHODDEF    \
    {"posix_fadvise", (PyCFunction)os_posix_fadvise, METH_VARARGS, os_posix_fadvise__doc__},

static PyObject *
os_posix_fadvise_impl(PyModuleDef *module, int fd, Py_off_t offset, Py_off_t length, int advice);

static PyObject *
os_posix_fadvise(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    Py_off_t offset;
    Py_off_t length;
    int advice;

    if (!PyArg_ParseTuple(args,
        "iO&O&i:posix_fadvise",
        &fd, Py_off_t_converter, &offset, Py_off_t_converter, &length, &advice))
        goto exit;
    return_value = os_posix_fadvise_impl(module, fd, offset, length, advice);

exit:
    return return_value;
}

static PyObject *
os_posix_fadvise_impl(PyModuleDef *module, int fd, Py_off_t offset, Py_off_t length, int advice)
/*[clinic end generated code: output=dad93f32c04dd4f7 input=0fbe554edc2f04b5]*/
{
    int result;

    Py_BEGIN_ALLOW_THREADS
    result = posix_fadvise(fd, offset, length, advice);
    Py_END_ALLOW_THREADS
    if (result != 0) {
        errno = result;
        return posix_error();
    }
    Py_RETURN_NONE;
}
#endif /* HAVE_POSIX_FADVISE && !POSIX_FADVISE_AIX_BUG */

#ifdef HAVE_PUTENV

/* Save putenv() parameters as values here, so we can collect them when they
 * get re-set with another call for the same key. */
static PyObject *posix_putenv_garbage;

static void
posix_putenv_garbage_setitem(PyObject *name, PyObject *value)
{
    /* Install the first arg and newstr in posix_putenv_garbage;
     * this will cause previous value to be collected.  This has to
     * happen after the real putenv() call because the old value
     * was still accessible until then. */
    if (PyDict_SetItem(posix_putenv_garbage, name, value))
        /* really not much we can do; just leak */
        PyErr_Clear();
    else
        Py_DECREF(value);
}


#ifdef MS_WINDOWS
/*[clinic input]
os.putenv

    name: unicode
    value: unicode
    /

Change or add an environment variable.
[clinic start generated code]*/

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)os_putenv, METH_VARARGS, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyModuleDef *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *name;
    PyObject *value;

    if (!PyArg_ParseTuple(args,
        "UU:putenv",
        &name, &value))
        goto exit;
    return_value = os_putenv_impl(module, name, value);

exit:
    return return_value;
}

static PyObject *
os_putenv_impl(PyModuleDef *module, PyObject *name, PyObject *value)
/*[clinic end generated code: output=5ce9ef9b15606e7e input=ba586581c2e6105f]*/
{
    wchar_t *env;

    PyObject *unicode = PyUnicode_FromFormat("%U=%U", name, value);
    if (unicode == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    if (_MAX_ENV < PyUnicode_GET_LENGTH(unicode)) {
        PyErr_Format(PyExc_ValueError,
                     "the environment variable is longer than %u characters",
                     _MAX_ENV);
        goto error;
    }

    env = PyUnicode_AsUnicode(unicode);
    if (env == NULL)
        goto error;
    if (_wputenv(env)) {
        posix_error();
        goto error;
    }

    posix_putenv_garbage_setitem(name, unicode);
    Py_RETURN_NONE;

error:
    Py_DECREF(unicode);
    return NULL;
}
#else /* MS_WINDOWS */
/*[clinic input]
os.putenv

    name: FSConverter
    value: FSConverter
    /

Change or add an environment variable.
[clinic start generated code]*/

PyDoc_STRVAR(os_putenv__doc__,
"putenv($module, name, value, /)\n"
"--\n"
"\n"
"Change or add an environment variable.");

#define OS_PUTENV_METHODDEF    \
    {"putenv", (PyCFunction)os_putenv, METH_VARARGS, os_putenv__doc__},

static PyObject *
os_putenv_impl(PyModuleDef *module, PyObject *name, PyObject *value);

static PyObject *
os_putenv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *name = NULL;
    PyObject *value = NULL;

    if (!PyArg_ParseTuple(args,
        "O&O&:putenv",
        PyUnicode_FSConverter, &name, PyUnicode_FSConverter, &value))
        goto exit;
    return_value = os_putenv_impl(module, name, value);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);
    /* Cleanup for value */
    Py_XDECREF(value);

    return return_value;
}

static PyObject *
os_putenv_impl(PyModuleDef *module, PyObject *name, PyObject *value)
/*[clinic end generated code: output=85ab223393dc7afd input=a97bc6152f688d31]*/
{
    PyObject *bytes = NULL;
    char *env;
    char *name_string = PyBytes_AsString(name);
    char *value_string = PyBytes_AsString(value);

    bytes = PyBytes_FromFormat("%s=%s", name_string, value_string);
    if (bytes == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    env = PyBytes_AS_STRING(bytes);
    if (putenv(env)) {
        Py_DECREF(bytes);
        return posix_error();
    }

    posix_putenv_garbage_setitem(name, bytes);
    Py_RETURN_NONE;
}
#endif /* MS_WINDOWS */
#endif /* HAVE_PUTENV */


#ifdef HAVE_UNSETENV
/*[clinic input]
os.unsetenv
    name: FSConverter
    /

Delete an environment variable.
[clinic start generated code]*/

PyDoc_STRVAR(os_unsetenv__doc__,
"unsetenv($module, name, /)\n"
"--\n"
"\n"
"Delete an environment variable.");

#define OS_UNSETENV_METHODDEF    \
    {"unsetenv", (PyCFunction)os_unsetenv, METH_VARARGS, os_unsetenv__doc__},

static PyObject *
os_unsetenv_impl(PyModuleDef *module, PyObject *name);

static PyObject *
os_unsetenv(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *name = NULL;

    if (!PyArg_ParseTuple(args,
        "O&:unsetenv",
        PyUnicode_FSConverter, &name))
        goto exit;
    return_value = os_unsetenv_impl(module, name);

exit:
    /* Cleanup for name */
    Py_XDECREF(name);

    return return_value;
}

static PyObject *
os_unsetenv_impl(PyModuleDef *module, PyObject *name)
/*[clinic end generated code: output=91318c995f9a0767 input=2bb5288a599c7107]*/
{
#ifndef HAVE_BROKEN_UNSETENV
    int err;
#endif

#ifdef HAVE_BROKEN_UNSETENV
    unsetenv(PyBytes_AS_STRING(name));
#else
    err = unsetenv(PyBytes_AS_STRING(name));
    if (err)
        return posix_error();
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
    Py_RETURN_NONE;
}
#endif /* HAVE_UNSETENV */


/*[clinic input]
os.strerror

    code: int
    /

Translate an error code to a message string.
[clinic start generated code]*/

PyDoc_STRVAR(os_strerror__doc__,
"strerror($module, code, /)\n"
"--\n"
"\n"
"Translate an error code to a message string.");

#define OS_STRERROR_METHODDEF    \
    {"strerror", (PyCFunction)os_strerror, METH_VARARGS, os_strerror__doc__},

static PyObject *
os_strerror_impl(PyModuleDef *module, int code);

static PyObject *
os_strerror(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int code;

    if (!PyArg_ParseTuple(args,
        "i:strerror",
        &code))
        goto exit;
    return_value = os_strerror_impl(module, code);

exit:
    return return_value;
}

static PyObject *
os_strerror_impl(PyModuleDef *module, int code)
/*[clinic end generated code: output=8665c70bb2ca4720 input=75a8673d97915a91]*/
{
    char *message = strerror(code);
    if (message == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "strerror() argument out of range");
        return NULL;
    }
    return PyUnicode_DecodeLocale(message, "surrogateescape");
}


#ifdef HAVE_SYS_WAIT_H
#ifdef WCOREDUMP
/*[clinic input]
os.WCOREDUMP -> bool

    status: int
    /

Return True if the process returning status was dumped to a core file.
[clinic start generated code]*/

PyDoc_STRVAR(os_WCOREDUMP__doc__,
"WCOREDUMP($module, status, /)\n"
"--\n"
"\n"
"Return True if the process returning status was dumped to a core file.");

#define OS_WCOREDUMP_METHODDEF    \
    {"WCOREDUMP", (PyCFunction)os_WCOREDUMP, METH_VARARGS, os_WCOREDUMP__doc__},

static int
os_WCOREDUMP_impl(PyModuleDef *module, int status);

static PyObject *
os_WCOREDUMP(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int status;
    int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:WCOREDUMP",
        &status))
        goto exit;
    _return_value = os_WCOREDUMP_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WCOREDUMP_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=e04d55c09c299828 input=8b05e7ab38528d04]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WCOREDUMP(wait_status);
}
#endif /* WCOREDUMP */


#ifdef WIFCONTINUED
/*[clinic input]
os.WIFCONTINUED -> bool

    status: int

Return True if a particular process was continued from a job control stop.

Return True if the process returning status was continued from a
job control stop.
[clinic start generated code]*/

PyDoc_STRVAR(os_WIFCONTINUED__doc__,
"WIFCONTINUED($module, /, status)\n"
"--\n"
"\n"
"Return True if a particular process was continued from a job control stop.\n"
"\n"
"Return True if the process returning status was continued from a\n"
"job control stop.");

#define OS_WIFCONTINUED_METHODDEF    \
    {"WIFCONTINUED", (PyCFunction)os_WIFCONTINUED, METH_VARARGS|METH_KEYWORDS, os_WIFCONTINUED__doc__},

static int
os_WIFCONTINUED_impl(PyModuleDef *module, int status);

static PyObject *
os_WIFCONTINUED(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WIFCONTINUED", _keywords,
        &status))
        goto exit;
    _return_value = os_WIFCONTINUED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WIFCONTINUED_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=9c4e6105a4520ab5 input=e777e7d38eb25bd9]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WIFCONTINUED(wait_status);
}
#endif /* WIFCONTINUED */


#ifdef WIFSTOPPED
/*[clinic input]
os.WIFSTOPPED -> bool

    status: int

Return True if the process returning status was stopped.
[clinic start generated code]*/

PyDoc_STRVAR(os_WIFSTOPPED__doc__,
"WIFSTOPPED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status was stopped.");

#define OS_WIFSTOPPED_METHODDEF    \
    {"WIFSTOPPED", (PyCFunction)os_WIFSTOPPED, METH_VARARGS|METH_KEYWORDS, os_WIFSTOPPED__doc__},

static int
os_WIFSTOPPED_impl(PyModuleDef *module, int status);

static PyObject *
os_WIFSTOPPED(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WIFSTOPPED", _keywords,
        &status))
        goto exit;
    _return_value = os_WIFSTOPPED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WIFSTOPPED_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=e0de2da8ec9593ff input=043cb7f1289ef904]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WIFSTOPPED(wait_status);
}
#endif /* WIFSTOPPED */


#ifdef WIFSIGNALED
/*[clinic input]
os.WIFSIGNALED -> bool

    status: int

Return True if the process returning status was terminated by a signal.
[clinic start generated code]*/

PyDoc_STRVAR(os_WIFSIGNALED__doc__,
"WIFSIGNALED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status was terminated by a signal.");

#define OS_WIFSIGNALED_METHODDEF    \
    {"WIFSIGNALED", (PyCFunction)os_WIFSIGNALED, METH_VARARGS|METH_KEYWORDS, os_WIFSIGNALED__doc__},

static int
os_WIFSIGNALED_impl(PyModuleDef *module, int status);

static PyObject *
os_WIFSIGNALED(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WIFSIGNALED", _keywords,
        &status))
        goto exit;
    _return_value = os_WIFSIGNALED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WIFSIGNALED_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=f14d106558f406be input=d55ba7cc9ce5dc43]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WIFSIGNALED(wait_status);
}
#endif /* WIFSIGNALED */


#ifdef WIFEXITED
/*[clinic input]
os.WIFEXITED -> bool

    status: int

Return True if the process returning status exited via the exit() system call.
[clinic start generated code]*/

PyDoc_STRVAR(os_WIFEXITED__doc__,
"WIFEXITED($module, /, status)\n"
"--\n"
"\n"
"Return True if the process returning status exited via the exit() system call.");

#define OS_WIFEXITED_METHODDEF    \
    {"WIFEXITED", (PyCFunction)os_WIFEXITED, METH_VARARGS|METH_KEYWORDS, os_WIFEXITED__doc__},

static int
os_WIFEXITED_impl(PyModuleDef *module, int status);

static PyObject *
os_WIFEXITED(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WIFEXITED", _keywords,
        &status))
        goto exit;
    _return_value = os_WIFEXITED_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WIFEXITED_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=2f76087d53721255 input=d63775a6791586c0]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WIFEXITED(wait_status);
}
#endif /* WIFEXITED */


#ifdef WEXITSTATUS
/*[clinic input]
os.WEXITSTATUS -> int

    status: int

Return the process return code from status.
[clinic start generated code]*/

PyDoc_STRVAR(os_WEXITSTATUS__doc__,
"WEXITSTATUS($module, /, status)\n"
"--\n"
"\n"
"Return the process return code from status.");

#define OS_WEXITSTATUS_METHODDEF    \
    {"WEXITSTATUS", (PyCFunction)os_WEXITSTATUS, METH_VARARGS|METH_KEYWORDS, os_WEXITSTATUS__doc__},

static int
os_WEXITSTATUS_impl(PyModuleDef *module, int status);

static PyObject *
os_WEXITSTATUS(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WEXITSTATUS", _keywords,
        &status))
        goto exit;
    _return_value = os_WEXITSTATUS_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WEXITSTATUS_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=13b6c270e2a326b1 input=e1fb4944e377585b]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WEXITSTATUS(wait_status);
}
#endif /* WEXITSTATUS */


#ifdef WTERMSIG
/*[clinic input]
os.WTERMSIG -> int

    status: int

Return the signal that terminated the process that provided the status value.
[clinic start generated code]*/

PyDoc_STRVAR(os_WTERMSIG__doc__,
"WTERMSIG($module, /, status)\n"
"--\n"
"\n"
"Return the signal that terminated the process that provided the status value.");

#define OS_WTERMSIG_METHODDEF    \
    {"WTERMSIG", (PyCFunction)os_WTERMSIG, METH_VARARGS|METH_KEYWORDS, os_WTERMSIG__doc__},

static int
os_WTERMSIG_impl(PyModuleDef *module, int status);

static PyObject *
os_WTERMSIG(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WTERMSIG", _keywords,
        &status))
        goto exit;
    _return_value = os_WTERMSIG_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WTERMSIG_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=bf1fd4b002d0a9ed input=727fd7f84ec3f243]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WTERMSIG(wait_status);
}
#endif /* WTERMSIG */


#ifdef WSTOPSIG
/*[clinic input]
os.WSTOPSIG -> int

    status: int

Return the signal that stopped the process that provided the status value.
[clinic start generated code]*/

PyDoc_STRVAR(os_WSTOPSIG__doc__,
"WSTOPSIG($module, /, status)\n"
"--\n"
"\n"
"Return the signal that stopped the process that provided the status value.");

#define OS_WSTOPSIG_METHODDEF    \
    {"WSTOPSIG", (PyCFunction)os_WSTOPSIG, METH_VARARGS|METH_KEYWORDS, os_WSTOPSIG__doc__},

static int
os_WSTOPSIG_impl(PyModuleDef *module, int status);

static PyObject *
os_WSTOPSIG(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"status", NULL};
    int status;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:WSTOPSIG", _keywords,
        &status))
        goto exit;
    _return_value = os_WSTOPSIG_impl(module, status);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_WSTOPSIG_impl(PyModuleDef *module, int status)
/*[clinic end generated code: output=92e1647d29ee0549 input=46ebf1d1b293c5c1]*/
{
    WAIT_TYPE wait_status;
    WAIT_STATUS_INT(wait_status) = status;
    return WSTOPSIG(wait_status);
}
#endif /* WSTOPSIG */
#endif /* HAVE_SYS_WAIT_H */


#ifndef OS_WCOREDUMP_METHODDEF
#define OS_WCOREDUMP_METHODDEF
#endif /* OS_WCOREDUMP_METHODDEF */

#ifndef OS_WIFCONTINUED_METHODDEF
#define OS_WIFCONTINUED_METHODDEF
#endif /* OS_WIFCONTINUED_METHODDEF */

#ifndef OS_WIFSTOPPED_METHODDEF
#define OS_WIFSTOPPED_METHODDEF
#endif /* OS_WIFSTOPPED_METHODDEF */

#ifndef OS_WIFSIGNALED_METHODDEF
#define OS_WIFSIGNALED_METHODDEF
#endif /* OS_WIFSIGNALED_METHODDEF */

#ifndef OS_WIFEXITED_METHODDEF
#define OS_WIFEXITED_METHODDEF
#endif /* OS_WIFEXITED_METHODDEF */

#ifndef OS_WEXITSTATUS_METHODDEF
#define OS_WEXITSTATUS_METHODDEF
#endif /* OS_WEXITSTATUS_METHODDEF */

#ifndef OS_WTERMSIG_METHODDEF
#define OS_WTERMSIG_METHODDEF
#endif /* OS_WTERMSIG_METHODDEF */

#ifndef OS_WSTOPSIG_METHODDEF
#define OS_WSTOPSIG_METHODDEF
#endif /* OS_WSTOPSIG_METHODDEF */


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
    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}


/*[clinic input]
os.fstatvfs
    fd: int
    /

Perform an fstatvfs system call on the given fd.

Equivalent to statvfs(fd).
[clinic start generated code]*/

PyDoc_STRVAR(os_fstatvfs__doc__,
"fstatvfs($module, fd, /)\n"
"--\n"
"\n"
"Perform an fstatvfs system call on the given fd.\n"
"\n"
"Equivalent to statvfs(fd).");

#define OS_FSTATVFS_METHODDEF    \
    {"fstatvfs", (PyCFunction)os_fstatvfs, METH_VARARGS, os_fstatvfs__doc__},

static PyObject *
os_fstatvfs_impl(PyModuleDef *module, int fd);

static PyObject *
os_fstatvfs(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;

    if (!PyArg_ParseTuple(args,
        "i:fstatvfs",
        &fd))
        goto exit;
    return_value = os_fstatvfs_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_fstatvfs_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=0e32bf07f946ec0d input=d8122243ac50975e]*/
{
    int result;
    struct statvfs st;

    Py_BEGIN_ALLOW_THREADS
    result = fstatvfs(fd, &st);
    Py_END_ALLOW_THREADS
    if (result != 0)
        return posix_error();

    return _pystatvfs_fromstructstatvfs(st);
}
#endif /* defined(HAVE_FSTATVFS) && defined(HAVE_SYS_STATVFS_H) */


#if defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H)
#include <sys/statvfs.h>
/*[clinic input]
os.statvfs

    path: path_t(allow_fd='PATH_HAVE_FSTATVFS')

Perform a statvfs system call on the given path.

path may always be specified as a string.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.
[clinic start generated code]*/

PyDoc_STRVAR(os_statvfs__doc__,
"statvfs($module, /, path)\n"
"--\n"
"\n"
"Perform a statvfs system call on the given path.\n"
"\n"
"path may always be specified as a string.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_STATVFS_METHODDEF    \
    {"statvfs", (PyCFunction)os_statvfs, METH_VARARGS|METH_KEYWORDS, os_statvfs__doc__},

static PyObject *
os_statvfs_impl(PyModuleDef *module, path_t *path);

static PyObject *
os_statvfs(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    path_t path = PATH_T_INITIALIZE("statvfs", "path", 0, PATH_HAVE_FSTATVFS);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&:statvfs", _keywords,
        path_converter, &path))
        goto exit;
    return_value = os_statvfs_impl(module, &path);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_statvfs_impl(PyModuleDef *module, path_t *path)
/*[clinic end generated code: output=00ff54983360b446 input=3f5c35791c669bd9]*/
{
    int result;
    struct statvfs st;

    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FSTATVFS
    if (path->fd != -1) {
#ifdef __APPLE__
        /* handle weak-linking on Mac OS X 10.3 */
        if (fstatvfs == NULL) {
            fd_specified("statvfs", path->fd);
            return NULL;
        }
#endif
        result = fstatvfs(path->fd, &st);
    }
    else
#endif
        result = statvfs(path->narrow, &st);
    Py_END_ALLOW_THREADS

    if (result) {
        return path_error(path);
    }

    return _pystatvfs_fromstructstatvfs(st);
}
#endif /* defined(HAVE_STATVFS) && defined(HAVE_SYS_STATVFS_H) */


#ifdef MS_WINDOWS
/*[clinic input]
os._getdiskusage

    path: Py_UNICODE

Return disk usage statistics about the given path as a (total, free) tuple.
[clinic start generated code]*/

PyDoc_STRVAR(os__getdiskusage__doc__,
"_getdiskusage($module, /, path)\n"
"--\n"
"\n"
"Return disk usage statistics about the given path as a (total, free) tuple.");

#define OS__GETDISKUSAGE_METHODDEF    \
    {"_getdiskusage", (PyCFunction)os__getdiskusage, METH_VARARGS|METH_KEYWORDS, os__getdiskusage__doc__},

static PyObject *
os__getdiskusage_impl(PyModuleDef *module, Py_UNICODE *path);

static PyObject *
os__getdiskusage(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", NULL};
    Py_UNICODE *path;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "u:_getdiskusage", _keywords,
        &path))
        goto exit;
    return_value = os__getdiskusage_impl(module, path);

exit:
    return return_value;
}

static PyObject *
os__getdiskusage_impl(PyModuleDef *module, Py_UNICODE *path)
/*[clinic end generated code: output=054c972179b13708 input=6458133aed893c78]*/
{
    BOOL retval;
    ULARGE_INTEGER _, total, free;

    Py_BEGIN_ALLOW_THREADS
    retval = GetDiskFreeSpaceExW(path, &_, &total, &free);
    Py_END_ALLOW_THREADS
    if (retval == 0)
        return PyErr_SetFromWindowsErr(0);

    return Py_BuildValue("(LL)", total.QuadPart, free.QuadPart);
}
#endif /* MS_WINDOWS */


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
/*[clinic input]
os.fpathconf -> long

    fd: int
    name: path_confname
    /

Return the configuration limit name for the file descriptor fd.

If there is no limit, return -1.
[clinic start generated code]*/

PyDoc_STRVAR(os_fpathconf__doc__,
"fpathconf($module, fd, name, /)\n"
"--\n"
"\n"
"Return the configuration limit name for the file descriptor fd.\n"
"\n"
"If there is no limit, return -1.");

#define OS_FPATHCONF_METHODDEF    \
    {"fpathconf", (PyCFunction)os_fpathconf, METH_VARARGS, os_fpathconf__doc__},

static long
os_fpathconf_impl(PyModuleDef *module, int fd, int name);

static PyObject *
os_fpathconf(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int name;
    long _return_value;

    if (!PyArg_ParseTuple(args,
        "iO&:fpathconf",
        &fd, conv_path_confname, &name))
        goto exit;
    _return_value = os_fpathconf_impl(module, fd, name);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

static long
os_fpathconf_impl(PyModuleDef *module, int fd, int name)
/*[clinic end generated code: output=3bf04b40e0523a8c input=5942a024d3777810]*/
{
    long limit;

    errno = 0;
    limit = fpathconf(fd, name);
    if (limit == -1 && errno != 0)
        posix_error();

    return limit;
}
#endif /* HAVE_FPATHCONF */


#ifdef HAVE_PATHCONF
/*[clinic input]
os.pathconf -> long
    path: path_t(allow_fd='PATH_HAVE_FPATHCONF')
    name: path_confname

Return the configuration limit name for the file or directory path.

If there is no limit, return -1.
On some platforms, path may also be specified as an open file descriptor.
  If this functionality is unavailable, using it raises an exception.
[clinic start generated code]*/

PyDoc_STRVAR(os_pathconf__doc__,
"pathconf($module, /, path, name)\n"
"--\n"
"\n"
"Return the configuration limit name for the file or directory path.\n"
"\n"
"If there is no limit, return -1.\n"
"On some platforms, path may also be specified as an open file descriptor.\n"
"  If this functionality is unavailable, using it raises an exception.");

#define OS_PATHCONF_METHODDEF    \
    {"pathconf", (PyCFunction)os_pathconf, METH_VARARGS|METH_KEYWORDS, os_pathconf__doc__},

static long
os_pathconf_impl(PyModuleDef *module, path_t *path, int name);

static PyObject *
os_pathconf(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "name", NULL};
    path_t path = PATH_T_INITIALIZE("pathconf", "path", 0, PATH_HAVE_FPATHCONF);
    int name;
    long _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&:pathconf", _keywords,
        path_converter, &path, conv_path_confname, &name))
        goto exit;
    _return_value = os_pathconf_impl(module, &path, name);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong(_return_value);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static long
os_pathconf_impl(PyModuleDef *module, path_t *path, int name)
/*[clinic end generated code: output=1a53e125b6cf63e4 input=bc3e2a985af27e5e]*/
{
    long limit;

    errno = 0;
#ifdef HAVE_FPATHCONF
    if (path->fd != -1)
        limit = fpathconf(path->fd, name);
    else
#endif
        limit = pathconf(path->narrow, name);
    if (limit == -1 && errno != 0) {
        if (errno == EINVAL)
            /* could be a path or name problem */
            posix_error();
        else
            path_error(path);
    }

    return limit;
}
#endif /* HAVE_PATHCONF */

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


/*[clinic input]
os.confstr

    name: confstr_confname
    /

Return a string-valued system configuration variable.
[clinic start generated code]*/

PyDoc_STRVAR(os_confstr__doc__,
"confstr($module, name, /)\n"
"--\n"
"\n"
"Return a string-valued system configuration variable.");

#define OS_CONFSTR_METHODDEF    \
    {"confstr", (PyCFunction)os_confstr, METH_VARARGS, os_confstr__doc__},

static PyObject *
os_confstr_impl(PyModuleDef *module, int name);

static PyObject *
os_confstr(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int name;

    if (!PyArg_ParseTuple(args,
        "O&:confstr",
        conv_confstr_confname, &name))
        goto exit;
    return_value = os_confstr_impl(module, name);

exit:
    return return_value;
}

static PyObject *
os_confstr_impl(PyModuleDef *module, int name)
/*[clinic end generated code: output=3f5e8aba9f8e3174 input=18fb4d0567242e65]*/
{
    PyObject *result = NULL;
    char buffer[255];
    size_t len;

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

    if (len >= sizeof(buffer)) {
        size_t len2;
        char *buf = PyMem_Malloc(len);
        if (buf == NULL)
            return PyErr_NoMemory();
        len2 = confstr(name, buf, len);
        assert(len == len2);
        result = PyUnicode_DecodeFSDefaultAndSize(buf, len-1);
        PyMem_Free(buf);
    }
    else
        result = PyUnicode_DecodeFSDefaultAndSize(buffer, len-1);
    return result;
}
#endif /* HAVE_CONFSTR */


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


/*[clinic input]
os.sysconf -> long
    name: sysconf_confname
    /

Return an integer-valued system configuration variable.
[clinic start generated code]*/

PyDoc_STRVAR(os_sysconf__doc__,
"sysconf($module, name, /)\n"
"--\n"
"\n"
"Return an integer-valued system configuration variable.");

#define OS_SYSCONF_METHODDEF    \
    {"sysconf", (PyCFunction)os_sysconf, METH_VARARGS, os_sysconf__doc__},

static long
os_sysconf_impl(PyModuleDef *module, int name);

static PyObject *
os_sysconf(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int name;
    long _return_value;

    if (!PyArg_ParseTuple(args,
        "O&:sysconf",
        conv_sysconf_confname, &name))
        goto exit;
    _return_value = os_sysconf_impl(module, name);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

static long
os_sysconf_impl(PyModuleDef *module, int name)
/*[clinic end generated code: output=7b06dfdc472431e4 input=279e3430a33f29e4]*/
{
    long value;

    errno = 0;
    value = sysconf(name);
    if (value == -1 && errno != 0)
        posix_error();
    return value;
}
#endif /* HAVE_SYSCONF */


/* This code is used to ensure that the tables of configuration value names
 * are in sorted order as required by conv_confname(), and also to build
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


/*[clinic input]
os.abort

Abort the interpreter immediately.

This function 'dumps core' or otherwise fails in the hardest way possible
on the hosting operating system.  This function never returns.
[clinic start generated code]*/

PyDoc_STRVAR(os_abort__doc__,
"abort($module, /)\n"
"--\n"
"\n"
"Abort the interpreter immediately.\n"
"\n"
"This function \'dumps core\' or otherwise fails in the hardest way possible\n"
"on the hosting operating system.  This function never returns.");

#define OS_ABORT_METHODDEF    \
    {"abort", (PyCFunction)os_abort, METH_NOARGS, os_abort__doc__},

static PyObject *
os_abort_impl(PyModuleDef *module);

static PyObject *
os_abort(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_abort_impl(module);
}

static PyObject *
os_abort_impl(PyModuleDef *module)
/*[clinic end generated code: output=cded2cc8c5453d3a input=cf2c7d98bc504047]*/
{
    abort();
    /*NOTREACHED*/
    Py_FatalError("abort() called from Python code didn't abort!");
    return NULL;
}

#ifdef MS_WINDOWS
/* AC 3.5: change to path_t? but that might change exceptions */
PyDoc_STRVAR(win32_startfile__doc__,
"startfile(filepath [, operation])\n\
\n\
Start a file with its associated application.\n\
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
#endif /* MS_WINDOWS */


#ifdef HAVE_GETLOADAVG
/*[clinic input]
os.getloadavg

Return average recent system load information.

Return the number of processes in the system run queue averaged over
the last 1, 5, and 15 minutes as a tuple of three floats.
Raises OSError if the load average was unobtainable.
[clinic start generated code]*/

PyDoc_STRVAR(os_getloadavg__doc__,
"getloadavg($module, /)\n"
"--\n"
"\n"
"Return average recent system load information.\n"
"\n"
"Return the number of processes in the system run queue averaged over\n"
"the last 1, 5, and 15 minutes as a tuple of three floats.\n"
"Raises OSError if the load average was unobtainable.");

#define OS_GETLOADAVG_METHODDEF    \
    {"getloadavg", (PyCFunction)os_getloadavg, METH_NOARGS, os_getloadavg__doc__},

static PyObject *
os_getloadavg_impl(PyModuleDef *module);

static PyObject *
os_getloadavg(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getloadavg_impl(module);
}

static PyObject *
os_getloadavg_impl(PyModuleDef *module)
/*[clinic end generated code: output=67593a92457d55af input=3d6d826b76d8a34e]*/
{
    double loadavg[3];
    if (getloadavg(loadavg, 3)!=3) {
        PyErr_SetString(PyExc_OSError, "Load averages are unobtainable");
        return NULL;
    } else
        return Py_BuildValue("ddd", loadavg[0], loadavg[1], loadavg[2]);
}
#endif /* HAVE_GETLOADAVG */


/*[clinic input]
os.device_encoding
    fd: int

Return a string describing the encoding of a terminal's file descriptor.

The file descriptor must be attached to a terminal.
If the device is not a terminal, return None.
[clinic start generated code]*/

PyDoc_STRVAR(os_device_encoding__doc__,
"device_encoding($module, /, fd)\n"
"--\n"
"\n"
"Return a string describing the encoding of a terminal\'s file descriptor.\n"
"\n"
"The file descriptor must be attached to a terminal.\n"
"If the device is not a terminal, return None.");

#define OS_DEVICE_ENCODING_METHODDEF    \
    {"device_encoding", (PyCFunction)os_device_encoding, METH_VARARGS|METH_KEYWORDS, os_device_encoding__doc__},

static PyObject *
os_device_encoding_impl(PyModuleDef *module, int fd);

static PyObject *
os_device_encoding(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i:device_encoding", _keywords,
        &fd))
        goto exit;
    return_value = os_device_encoding_impl(module, fd);

exit:
    return return_value;
}

static PyObject *
os_device_encoding_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=e9f8274d42f5cce3 input=9e1d4a42b66df312]*/
{
    return _Py_device_encoding(fd);
}


#ifdef HAVE_SETRESUID
/*[clinic input]
os.setresuid

    ruid: uid_t
    euid: uid_t
    suid: uid_t
    /

Set the current process's real, effective, and saved user ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_setresuid__doc__,
"setresuid($module, ruid, euid, suid, /)\n"
"--\n"
"\n"
"Set the current process\'s real, effective, and saved user ids.");

#define OS_SETRESUID_METHODDEF    \
    {"setresuid", (PyCFunction)os_setresuid, METH_VARARGS, os_setresuid__doc__},

static PyObject *
os_setresuid_impl(PyModuleDef *module, uid_t ruid, uid_t euid, uid_t suid);

static PyObject *
os_setresuid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    uid_t ruid;
    uid_t euid;
    uid_t suid;

    if (!PyArg_ParseTuple(args,
        "O&O&O&:setresuid",
        _Py_Uid_Converter, &ruid, _Py_Uid_Converter, &euid, _Py_Uid_Converter, &suid))
        goto exit;
    return_value = os_setresuid_impl(module, ruid, euid, suid);

exit:
    return return_value;
}

static PyObject *
os_setresuid_impl(PyModuleDef *module, uid_t ruid, uid_t euid, uid_t suid)
/*[clinic end generated code: output=2e3457cfe7cd1f94 input=9e33cb79a82792f3]*/
{
    if (setresuid(ruid, euid, suid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETRESUID */


#ifdef HAVE_SETRESGID
/*[clinic input]
os.setresgid

    rgid: gid_t
    egid: gid_t
    sgid: gid_t
    /

Set the current process's real, effective, and saved group ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_setresgid__doc__,
"setresgid($module, rgid, egid, sgid, /)\n"
"--\n"
"\n"
"Set the current process\'s real, effective, and saved group ids.");

#define OS_SETRESGID_METHODDEF    \
    {"setresgid", (PyCFunction)os_setresgid, METH_VARARGS, os_setresgid__doc__},

static PyObject *
os_setresgid_impl(PyModuleDef *module, gid_t rgid, gid_t egid, gid_t sgid);

static PyObject *
os_setresgid(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    gid_t rgid;
    gid_t egid;
    gid_t sgid;

    if (!PyArg_ParseTuple(args,
        "O&O&O&:setresgid",
        _Py_Gid_Converter, &rgid, _Py_Gid_Converter, &egid, _Py_Gid_Converter, &sgid))
        goto exit;
    return_value = os_setresgid_impl(module, rgid, egid, sgid);

exit:
    return return_value;
}

static PyObject *
os_setresgid_impl(PyModuleDef *module, gid_t rgid, gid_t egid, gid_t sgid)
/*[clinic end generated code: output=8a7ee6c1f2482362 input=33e9e0785ef426b1]*/
{
    if (setresgid(rgid, egid, sgid) < 0)
        return posix_error();
    Py_RETURN_NONE;
}
#endif /* HAVE_SETRESGID */


#ifdef HAVE_GETRESUID
/*[clinic input]
os.getresuid

Return a tuple of the current process's real, effective, and saved user ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_getresuid__doc__,
"getresuid($module, /)\n"
"--\n"
"\n"
"Return a tuple of the current process\'s real, effective, and saved user ids.");

#define OS_GETRESUID_METHODDEF    \
    {"getresuid", (PyCFunction)os_getresuid, METH_NOARGS, os_getresuid__doc__},

static PyObject *
os_getresuid_impl(PyModuleDef *module);

static PyObject *
os_getresuid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getresuid_impl(module);
}

static PyObject *
os_getresuid_impl(PyModuleDef *module)
/*[clinic end generated code: output=d0786686a6ef1320 input=41ccfa8e1f6517ad]*/
{
    uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) < 0)
        return posix_error();
    return Py_BuildValue("(NNN)", _PyLong_FromUid(ruid),
                                  _PyLong_FromUid(euid),
                                  _PyLong_FromUid(suid));
}
#endif /* HAVE_GETRESUID */


#ifdef HAVE_GETRESGID
/*[clinic input]
os.getresgid

Return a tuple of the current process's real, effective, and saved group ids.
[clinic start generated code]*/

PyDoc_STRVAR(os_getresgid__doc__,
"getresgid($module, /)\n"
"--\n"
"\n"
"Return a tuple of the current process\'s real, effective, and saved group ids.");

#define OS_GETRESGID_METHODDEF    \
    {"getresgid", (PyCFunction)os_getresgid, METH_NOARGS, os_getresgid__doc__},

static PyObject *
os_getresgid_impl(PyModuleDef *module);

static PyObject *
os_getresgid(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_getresgid_impl(module);
}

static PyObject *
os_getresgid_impl(PyModuleDef *module)
/*[clinic end generated code: output=05249ac795fa759f input=517e68db9ca32df6]*/
{
    gid_t rgid, egid, sgid;
    if (getresgid(&rgid, &egid, &sgid) < 0)
        return posix_error();
    return Py_BuildValue("(NNN)", _PyLong_FromGid(rgid),
                                  _PyLong_FromGid(egid),
                                  _PyLong_FromGid(sgid));
}
#endif /* HAVE_GETRESGID */


#ifdef USE_XATTRS
/*[clinic input]
os.getxattr

    path: path_t(allow_fd=True)
    attribute: path_t
    *
    follow_symlinks: bool = True

Return the value of extended attribute attribute on path.

path may be either a string or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, getxattr will examine the symbolic link itself instead of the file
  the link points to.

[clinic start generated code]*/

PyDoc_STRVAR(os_getxattr__doc__,
"getxattr($module, /, path, attribute, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return the value of extended attribute attribute on path.\n"
"\n"
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, getxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_GETXATTR_METHODDEF    \
    {"getxattr", (PyCFunction)os_getxattr, METH_VARARGS|METH_KEYWORDS, os_getxattr__doc__},

static PyObject *
os_getxattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, int follow_symlinks);

static PyObject *
os_getxattr(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("getxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("getxattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|$p:getxattr", _keywords,
        path_converter, &path, path_converter, &attribute, &follow_symlinks))
        goto exit;
    return_value = os_getxattr_impl(module, &path, &attribute, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);

    return return_value;
}

static PyObject *
os_getxattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, int follow_symlinks)
/*[clinic end generated code: output=bbc9454fe2b9ea86 input=8c8ea3bab78d89c2]*/
{
    Py_ssize_t i;
    PyObject *buffer = NULL;

    if (fd_and_follow_symlinks_invalid("getxattr", path->fd, follow_symlinks))
        return NULL;

    for (i = 0; ; i++) {
        void *ptr;
        ssize_t result;
        static Py_ssize_t buffer_sizes[] = {128, XATTR_SIZE_MAX, 0};
        Py_ssize_t buffer_size = buffer_sizes[i];
        if (!buffer_size) {
            path_error(path);
            return NULL;
        }
        buffer = PyBytes_FromStringAndSize(NULL, buffer_size);
        if (!buffer)
            return NULL;
        ptr = PyBytes_AS_STRING(buffer);

        Py_BEGIN_ALLOW_THREADS;
        if (path->fd >= 0)
            result = fgetxattr(path->fd, attribute->narrow, ptr, buffer_size);
        else if (follow_symlinks)
            result = getxattr(path->narrow, attribute->narrow, ptr, buffer_size);
        else
            result = lgetxattr(path->narrow, attribute->narrow, ptr, buffer_size);
        Py_END_ALLOW_THREADS;

        if (result < 0) {
            Py_DECREF(buffer);
            if (errno == ERANGE)
                continue;
            path_error(path);
            return NULL;
        }

        if (result != buffer_size) {
            /* Can only shrink. */
            _PyBytes_Resize(&buffer, result);
        }
        break;
    }

    return buffer;
}


/*[clinic input]
os.setxattr

    path: path_t(allow_fd=True)
    attribute: path_t
    value: Py_buffer
    flags: int = 0
    *
    follow_symlinks: bool = True

Set extended attribute attribute on path to value.

path may be either a string or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, setxattr will modify the symbolic link itself instead of the file
  the link points to.

[clinic start generated code]*/

PyDoc_STRVAR(os_setxattr__doc__,
"setxattr($module, /, path, attribute, value, flags=0, *,\n"
"         follow_symlinks=True)\n"
"--\n"
"\n"
"Set extended attribute attribute on path to value.\n"
"\n"
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, setxattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_SETXATTR_METHODDEF    \
    {"setxattr", (PyCFunction)os_setxattr, METH_VARARGS|METH_KEYWORDS, os_setxattr__doc__},

static PyObject *
os_setxattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, Py_buffer *value, int flags, int follow_symlinks);

static PyObject *
os_setxattr(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "attribute", "value", "flags", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("setxattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("setxattr", "attribute", 0, 0);
    Py_buffer value = {NULL, NULL};
    int flags = 0;
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&y*|i$p:setxattr", _keywords,
        path_converter, &path, path_converter, &attribute, &value, &flags, &follow_symlinks))
        goto exit;
    return_value = os_setxattr_impl(module, &path, &attribute, &value, flags, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);
    /* Cleanup for value */
    if (value.obj)
       PyBuffer_Release(&value);

    return return_value;
}

static PyObject *
os_setxattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, Py_buffer *value, int flags, int follow_symlinks)
/*[clinic end generated code: output=2ff845d8e024b218 input=f0d26833992015c2]*/
{
    ssize_t result;

    if (fd_and_follow_symlinks_invalid("setxattr", path->fd, follow_symlinks))
        return NULL;

    Py_BEGIN_ALLOW_THREADS;
    if (path->fd > -1)
        result = fsetxattr(path->fd, attribute->narrow,
                           value->buf, value->len, flags);
    else if (follow_symlinks)
        result = setxattr(path->narrow, attribute->narrow,
                           value->buf, value->len, flags);
    else
        result = lsetxattr(path->narrow, attribute->narrow,
                           value->buf, value->len, flags);
    Py_END_ALLOW_THREADS;

    if (result) {
        path_error(path);
        return NULL;
    }

    Py_RETURN_NONE;
}


/*[clinic input]
os.removexattr

    path: path_t(allow_fd=True)
    attribute: path_t
    *
    follow_symlinks: bool = True

Remove extended attribute attribute on path.

path may be either a string or an open file descriptor.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, removexattr will modify the symbolic link itself instead of the file
  the link points to.

[clinic start generated code]*/

PyDoc_STRVAR(os_removexattr__doc__,
"removexattr($module, /, path, attribute, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Remove extended attribute attribute on path.\n"
"\n"
"path may be either a string or an open file descriptor.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, removexattr will modify the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_REMOVEXATTR_METHODDEF    \
    {"removexattr", (PyCFunction)os_removexattr, METH_VARARGS|METH_KEYWORDS, os_removexattr__doc__},

static PyObject *
os_removexattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, int follow_symlinks);

static PyObject *
os_removexattr(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "attribute", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("removexattr", "path", 0, 1);
    path_t attribute = PATH_T_INITIALIZE("removexattr", "attribute", 0, 0);
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "O&O&|$p:removexattr", _keywords,
        path_converter, &path, path_converter, &attribute, &follow_symlinks))
        goto exit;
    return_value = os_removexattr_impl(module, &path, &attribute, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);
    /* Cleanup for attribute */
    path_cleanup(&attribute);

    return return_value;
}

static PyObject *
os_removexattr_impl(PyModuleDef *module, path_t *path, path_t *attribute, int follow_symlinks)
/*[clinic end generated code: output=8dfc715bf607c4cf input=cdb54834161e3329]*/
{
    ssize_t result;

    if (fd_and_follow_symlinks_invalid("removexattr", path->fd, follow_symlinks))
        return NULL;

    Py_BEGIN_ALLOW_THREADS;
    if (path->fd > -1)
        result = fremovexattr(path->fd, attribute->narrow);
    else if (follow_symlinks)
        result = removexattr(path->narrow, attribute->narrow);
    else
        result = lremovexattr(path->narrow, attribute->narrow);
    Py_END_ALLOW_THREADS;

    if (result) {
        return path_error(path);
    }

    Py_RETURN_NONE;
}


/*[clinic input]
os.listxattr

    path: path_t(allow_fd=True, nullable=True) = None
    *
    follow_symlinks: bool = True

Return a list of extended attributes on path.

path may be either None, a string, or an open file descriptor.
if path is None, listxattr will examine the current directory.
If follow_symlinks is False, and the last element of the path is a symbolic
  link, listxattr will examine the symbolic link itself instead of the file
  the link points to.
[clinic start generated code]*/

PyDoc_STRVAR(os_listxattr__doc__,
"listxattr($module, /, path=None, *, follow_symlinks=True)\n"
"--\n"
"\n"
"Return a list of extended attributes on path.\n"
"\n"
"path may be either None, a string, or an open file descriptor.\n"
"if path is None, listxattr will examine the current directory.\n"
"If follow_symlinks is False, and the last element of the path is a symbolic\n"
"  link, listxattr will examine the symbolic link itself instead of the file\n"
"  the link points to.");

#define OS_LISTXATTR_METHODDEF    \
    {"listxattr", (PyCFunction)os_listxattr, METH_VARARGS|METH_KEYWORDS, os_listxattr__doc__},

static PyObject *
os_listxattr_impl(PyModuleDef *module, path_t *path, int follow_symlinks);

static PyObject *
os_listxattr(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "follow_symlinks", NULL};
    path_t path = PATH_T_INITIALIZE("listxattr", "path", 1, 1);
    int follow_symlinks = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|O&$p:listxattr", _keywords,
        path_converter, &path, &follow_symlinks))
        goto exit;
    return_value = os_listxattr_impl(module, &path, follow_symlinks);

exit:
    /* Cleanup for path */
    path_cleanup(&path);

    return return_value;
}

static PyObject *
os_listxattr_impl(PyModuleDef *module, path_t *path, int follow_symlinks)
/*[clinic end generated code: output=3104cafda1a3d887 input=08cca53ac0b07c13]*/
{
    Py_ssize_t i;
    PyObject *result = NULL;
    const char *name;
    char *buffer = NULL;

    if (fd_and_follow_symlinks_invalid("listxattr", path->fd, follow_symlinks))
        goto exit;

    name = path->narrow ? path->narrow : ".";

    for (i = 0; ; i++) {
        char *start, *trace, *end;
        ssize_t length;
        static Py_ssize_t buffer_sizes[] = { 256, XATTR_LIST_MAX, 0 };
        Py_ssize_t buffer_size = buffer_sizes[i];
        if (!buffer_size) {
            /* ERANGE */
            path_error(path);
            break;
        }
        buffer = PyMem_MALLOC(buffer_size);
        if (!buffer) {
            PyErr_NoMemory();
            break;
        }

        Py_BEGIN_ALLOW_THREADS;
        if (path->fd > -1)
            length = flistxattr(path->fd, buffer, buffer_size);
        else if (follow_symlinks)
            length = listxattr(name, buffer, buffer_size);
        else
            length = llistxattr(name, buffer, buffer_size);
        Py_END_ALLOW_THREADS;

        if (length < 0) {
            if (errno == ERANGE) {
                PyMem_FREE(buffer);
                buffer = NULL;
                continue;
            }
            path_error(path);
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
    if (buffer)
        PyMem_FREE(buffer);
    return result;
}
#endif /* USE_XATTRS */


/*[clinic input]
os.urandom

    size: Py_ssize_t
    /

Return a bytes object containing random bytes suitable for cryptographic use.
[clinic start generated code]*/

PyDoc_STRVAR(os_urandom__doc__,
"urandom($module, size, /)\n"
"--\n"
"\n"
"Return a bytes object containing random bytes suitable for cryptographic use.");

#define OS_URANDOM_METHODDEF    \
    {"urandom", (PyCFunction)os_urandom, METH_VARARGS, os_urandom__doc__},

static PyObject *
os_urandom_impl(PyModuleDef *module, Py_ssize_t size);

static PyObject *
os_urandom(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args,
        "n:urandom",
        &size))
        goto exit;
    return_value = os_urandom_impl(module, size);

exit:
    return return_value;
}

static PyObject *
os_urandom_impl(PyModuleDef *module, Py_ssize_t size)
/*[clinic end generated code: output=5dbff582cab94cb9 input=4067cdb1b6776c29]*/
{
    PyObject *bytes;
    int result;

    if (size < 0)
        return PyErr_Format(PyExc_ValueError,
                            "negative argument not allowed");
    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return NULL;

    result = _PyOS_URandom(PyBytes_AS_STRING(bytes),
                        PyBytes_GET_SIZE(bytes));
    if (result == -1) {
        Py_DECREF(bytes);
        return NULL;
    }
    return bytes;
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
/* AC 3.5: fd should accept None */
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


/*[clinic input]
os.cpu_count

Return the number of CPUs in the system; return None if indeterminable.
[clinic start generated code]*/

PyDoc_STRVAR(os_cpu_count__doc__,
"cpu_count($module, /)\n"
"--\n"
"\n"
"Return the number of CPUs in the system; return None if indeterminable.");

#define OS_CPU_COUNT_METHODDEF    \
    {"cpu_count", (PyCFunction)os_cpu_count, METH_NOARGS, os_cpu_count__doc__},

static PyObject *
os_cpu_count_impl(PyModuleDef *module);

static PyObject *
os_cpu_count(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return os_cpu_count_impl(module);
}

static PyObject *
os_cpu_count_impl(PyModuleDef *module)
/*[clinic end generated code: output=92e2a4a729eb7740 input=d55e2f8f3823a628]*/
{
    int ncpu = 0;
#ifdef MS_WINDOWS
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    ncpu = sysinfo.dwNumberOfProcessors;
#elif defined(__hpux)
    ncpu = mpctl(MPC_GETNUMSPUS, NULL, NULL);
#elif defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
    ncpu = sysconf(_SC_NPROCESSORS_ONLN);
#elif defined(__DragonFly__) || \
      defined(__OpenBSD__)   || \
      defined(__FreeBSD__)   || \
      defined(__NetBSD__)    || \
      defined(__APPLE__)
    int mib[2];
    size_t len = sizeof(ncpu);
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    if (sysctl(mib, 2, &ncpu, &len, NULL, 0) != 0)
        ncpu = 0;
#endif
    if (ncpu >= 1)
        return PyLong_FromLong(ncpu);
    else
        Py_RETURN_NONE;
}


/*[clinic input]
os.get_inheritable -> bool

    fd: int
    /

Get the close-on-exe flag of the specified file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_get_inheritable__doc__,
"get_inheritable($module, fd, /)\n"
"--\n"
"\n"
"Get the close-on-exe flag of the specified file descriptor.");

#define OS_GET_INHERITABLE_METHODDEF    \
    {"get_inheritable", (PyCFunction)os_get_inheritable, METH_VARARGS, os_get_inheritable__doc__},

static int
os_get_inheritable_impl(PyModuleDef *module, int fd);

static PyObject *
os_get_inheritable(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int _return_value;

    if (!PyArg_ParseTuple(args,
        "i:get_inheritable",
        &fd))
        goto exit;
    _return_value = os_get_inheritable_impl(module, fd);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_get_inheritable_impl(PyModuleDef *module, int fd)
/*[clinic end generated code: output=261d1dd2b0dbdc35 input=89ac008dc9ab6b95]*/
{
    if (!_PyVerify_fd(fd)){
        posix_error();
        return -1;
    }

    return _Py_get_inheritable(fd);
}


/*[clinic input]
os.set_inheritable
    fd: int
    inheritable: int
    /

Set the inheritable flag of the specified file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_set_inheritable__doc__,
"set_inheritable($module, fd, inheritable, /)\n"
"--\n"
"\n"
"Set the inheritable flag of the specified file descriptor.");

#define OS_SET_INHERITABLE_METHODDEF    \
    {"set_inheritable", (PyCFunction)os_set_inheritable, METH_VARARGS, os_set_inheritable__doc__},

static PyObject *
os_set_inheritable_impl(PyModuleDef *module, int fd, int inheritable);

static PyObject *
os_set_inheritable(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int fd;
    int inheritable;

    if (!PyArg_ParseTuple(args,
        "ii:set_inheritable",
        &fd, &inheritable))
        goto exit;
    return_value = os_set_inheritable_impl(module, fd, inheritable);

exit:
    return return_value;
}

static PyObject *
os_set_inheritable_impl(PyModuleDef *module, int fd, int inheritable)
/*[clinic end generated code: output=64dfe5e15c906539 input=9ceaead87a1e2402]*/
{
    if (!_PyVerify_fd(fd))
        return posix_error();

    if (_Py_set_inheritable(fd, inheritable, NULL) < 0)
        return NULL;
    Py_RETURN_NONE;
}


#ifdef MS_WINDOWS
/*[clinic input]
os.get_handle_inheritable -> bool
    handle: Py_intptr_t
    /

Get the close-on-exe flag of the specified file descriptor.
[clinic start generated code]*/

PyDoc_STRVAR(os_get_handle_inheritable__doc__,
"get_handle_inheritable($module, handle, /)\n"
"--\n"
"\n"
"Get the close-on-exe flag of the specified file descriptor.");

#define OS_GET_HANDLE_INHERITABLE_METHODDEF    \
    {"get_handle_inheritable", (PyCFunction)os_get_handle_inheritable, METH_VARARGS, os_get_handle_inheritable__doc__},

static int
os_get_handle_inheritable_impl(PyModuleDef *module, Py_intptr_t handle);

static PyObject *
os_get_handle_inheritable(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_intptr_t handle;
    int _return_value;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_INTPTR ":get_handle_inheritable",
        &handle))
        goto exit;
    _return_value = os_get_handle_inheritable_impl(module, handle);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyBool_FromLong((long)_return_value);

exit:
    return return_value;
}

static int
os_get_handle_inheritable_impl(PyModuleDef *module, Py_intptr_t handle)
/*[clinic end generated code: output=d5bf9d86900bf457 input=5f7759443aae3dc5]*/
{
    DWORD flags;

    if (!GetHandleInformation((HANDLE)handle, &flags)) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }

    return flags & HANDLE_FLAG_INHERIT;
}


/*[clinic input]
os.set_handle_inheritable
    handle: Py_intptr_t
    inheritable: bool
    /

Set the inheritable flag of the specified handle.
[clinic start generated code]*/

PyDoc_STRVAR(os_set_handle_inheritable__doc__,
"set_handle_inheritable($module, handle, inheritable, /)\n"
"--\n"
"\n"
"Set the inheritable flag of the specified handle.");

#define OS_SET_HANDLE_INHERITABLE_METHODDEF    \
    {"set_handle_inheritable", (PyCFunction)os_set_handle_inheritable, METH_VARARGS, os_set_handle_inheritable__doc__},

static PyObject *
os_set_handle_inheritable_impl(PyModuleDef *module, Py_intptr_t handle, int inheritable);

static PyObject *
os_set_handle_inheritable(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_intptr_t handle;
    int inheritable;

    if (!PyArg_ParseTuple(args,
        "" _Py_PARSE_INTPTR "p:set_handle_inheritable",
        &handle, &inheritable))
        goto exit;
    return_value = os_set_handle_inheritable_impl(module, handle, inheritable);

exit:
    return return_value;
}

static PyObject *
os_set_handle_inheritable_impl(PyModuleDef *module, Py_intptr_t handle, int inheritable)
/*[clinic end generated code: output=ee5fcc6d9f0d4f8b input=e64b2b2730469def]*/
{
    DWORD flags = inheritable ? HANDLE_FLAG_INHERIT : 0;
    if (!SetHandleInformation((HANDLE)handle, HANDLE_FLAG_INHERIT, flags)) {
        PyErr_SetFromWindowsErr(0);
        return NULL;
    }
    Py_RETURN_NONE;
}
#endif /* MS_WINDOWS */

#ifndef MS_WINDOWS
PyDoc_STRVAR(get_blocking__doc__,
    "get_blocking(fd) -> bool\n" \
    "\n" \
    "Get the blocking mode of the file descriptor:\n" \
    "False if the O_NONBLOCK flag is set, True if the flag is cleared.");

static PyObject*
posix_get_blocking(PyObject *self, PyObject *args)
{
    int fd;
    int blocking;

    if (!PyArg_ParseTuple(args, "i:get_blocking", &fd))
        return NULL;

    if (!_PyVerify_fd(fd))
        return posix_error();

    blocking = _Py_get_blocking(fd);
    if (blocking < 0)
        return NULL;
    return PyBool_FromLong(blocking);
}

PyDoc_STRVAR(set_blocking__doc__,
    "set_blocking(fd, blocking)\n" \
    "\n" \
    "Set the blocking mode of the specified file descriptor.\n" \
    "Set the O_NONBLOCK flag if blocking is False,\n" \
    "clear the O_NONBLOCK flag otherwise.");

static PyObject*
posix_set_blocking(PyObject *self, PyObject *args)
{
    int fd, blocking;

    if (!PyArg_ParseTuple(args, "ii:set_blocking", &fd, &blocking))
        return NULL;

    if (!_PyVerify_fd(fd))
        return posix_error();

    if (_Py_set_blocking(fd, blocking) < 0)
        return NULL;
    Py_RETURN_NONE;
}
#endif   /* !MS_WINDOWS */


/*[clinic input]
dump buffer
[clinic start generated code]*/

#ifndef OS_TTYNAME_METHODDEF
    #define OS_TTYNAME_METHODDEF
#endif /* !defined(OS_TTYNAME_METHODDEF) */

#ifndef OS_CTERMID_METHODDEF
    #define OS_CTERMID_METHODDEF
#endif /* !defined(OS_CTERMID_METHODDEF) */

#ifndef OS_FCHDIR_METHODDEF
    #define OS_FCHDIR_METHODDEF
#endif /* !defined(OS_FCHDIR_METHODDEF) */

#ifndef OS_FCHMOD_METHODDEF
    #define OS_FCHMOD_METHODDEF
#endif /* !defined(OS_FCHMOD_METHODDEF) */

#ifndef OS_LCHMOD_METHODDEF
    #define OS_LCHMOD_METHODDEF
#endif /* !defined(OS_LCHMOD_METHODDEF) */

#ifndef OS_CHFLAGS_METHODDEF
    #define OS_CHFLAGS_METHODDEF
#endif /* !defined(OS_CHFLAGS_METHODDEF) */

#ifndef OS_LCHFLAGS_METHODDEF
    #define OS_LCHFLAGS_METHODDEF
#endif /* !defined(OS_LCHFLAGS_METHODDEF) */

#ifndef OS_CHROOT_METHODDEF
    #define OS_CHROOT_METHODDEF
#endif /* !defined(OS_CHROOT_METHODDEF) */

#ifndef OS_FSYNC_METHODDEF
    #define OS_FSYNC_METHODDEF
#endif /* !defined(OS_FSYNC_METHODDEF) */

#ifndef OS_SYNC_METHODDEF
    #define OS_SYNC_METHODDEF
#endif /* !defined(OS_SYNC_METHODDEF) */

#ifndef OS_FDATASYNC_METHODDEF
    #define OS_FDATASYNC_METHODDEF
#endif /* !defined(OS_FDATASYNC_METHODDEF) */

#ifndef OS_CHOWN_METHODDEF
    #define OS_CHOWN_METHODDEF
#endif /* !defined(OS_CHOWN_METHODDEF) */

#ifndef OS_FCHOWN_METHODDEF
    #define OS_FCHOWN_METHODDEF
#endif /* !defined(OS_FCHOWN_METHODDEF) */

#ifndef OS_LCHOWN_METHODDEF
    #define OS_LCHOWN_METHODDEF
#endif /* !defined(OS_LCHOWN_METHODDEF) */

#ifndef OS_LINK_METHODDEF
    #define OS_LINK_METHODDEF
#endif /* !defined(OS_LINK_METHODDEF) */

#ifndef OS__GETFINALPATHNAME_METHODDEF
    #define OS__GETFINALPATHNAME_METHODDEF
#endif /* !defined(OS__GETFINALPATHNAME_METHODDEF) */

#ifndef OS__GETVOLUMEPATHNAME_METHODDEF
    #define OS__GETVOLUMEPATHNAME_METHODDEF
#endif /* !defined(OS__GETVOLUMEPATHNAME_METHODDEF) */

#ifndef OS_NICE_METHODDEF
    #define OS_NICE_METHODDEF
#endif /* !defined(OS_NICE_METHODDEF) */

#ifndef OS_GETPRIORITY_METHODDEF
    #define OS_GETPRIORITY_METHODDEF
#endif /* !defined(OS_GETPRIORITY_METHODDEF) */

#ifndef OS_SETPRIORITY_METHODDEF
    #define OS_SETPRIORITY_METHODDEF
#endif /* !defined(OS_SETPRIORITY_METHODDEF) */

#ifndef OS_SYSTEM_METHODDEF
    #define OS_SYSTEM_METHODDEF
#endif /* !defined(OS_SYSTEM_METHODDEF) */

#ifndef OS_SYSTEM_METHODDEF
    #define OS_SYSTEM_METHODDEF
#endif /* !defined(OS_SYSTEM_METHODDEF) */

#ifndef OS_UNAME_METHODDEF
    #define OS_UNAME_METHODDEF
#endif /* !defined(OS_UNAME_METHODDEF) */

#ifndef OS_EXECV_METHODDEF
    #define OS_EXECV_METHODDEF
#endif /* !defined(OS_EXECV_METHODDEF) */

#ifndef OS_EXECVE_METHODDEF
    #define OS_EXECVE_METHODDEF
#endif /* !defined(OS_EXECVE_METHODDEF) */

#ifndef OS_SPAWNV_METHODDEF
    #define OS_SPAWNV_METHODDEF
#endif /* !defined(OS_SPAWNV_METHODDEF) */

#ifndef OS_SPAWNVE_METHODDEF
    #define OS_SPAWNVE_METHODDEF
#endif /* !defined(OS_SPAWNVE_METHODDEF) */

#ifndef OS_FORK1_METHODDEF
    #define OS_FORK1_METHODDEF
#endif /* !defined(OS_FORK1_METHODDEF) */

#ifndef OS_FORK_METHODDEF
    #define OS_FORK_METHODDEF
#endif /* !defined(OS_FORK_METHODDEF) */

#ifndef OS_SCHED_GET_PRIORITY_MAX_METHODDEF
    #define OS_SCHED_GET_PRIORITY_MAX_METHODDEF
#endif /* !defined(OS_SCHED_GET_PRIORITY_MAX_METHODDEF) */

#ifndef OS_SCHED_GET_PRIORITY_MIN_METHODDEF
    #define OS_SCHED_GET_PRIORITY_MIN_METHODDEF
#endif /* !defined(OS_SCHED_GET_PRIORITY_MIN_METHODDEF) */

#ifndef OS_SCHED_GETSCHEDULER_METHODDEF
    #define OS_SCHED_GETSCHEDULER_METHODDEF
#endif /* !defined(OS_SCHED_GETSCHEDULER_METHODDEF) */

#ifndef OS_SCHED_SETSCHEDULER_METHODDEF
    #define OS_SCHED_SETSCHEDULER_METHODDEF
#endif /* !defined(OS_SCHED_SETSCHEDULER_METHODDEF) */

#ifndef OS_SCHED_GETPARAM_METHODDEF
    #define OS_SCHED_GETPARAM_METHODDEF
#endif /* !defined(OS_SCHED_GETPARAM_METHODDEF) */

#ifndef OS_SCHED_SETPARAM_METHODDEF
    #define OS_SCHED_SETPARAM_METHODDEF
#endif /* !defined(OS_SCHED_SETPARAM_METHODDEF) */

#ifndef OS_SCHED_RR_GET_INTERVAL_METHODDEF
    #define OS_SCHED_RR_GET_INTERVAL_METHODDEF
#endif /* !defined(OS_SCHED_RR_GET_INTERVAL_METHODDEF) */

#ifndef OS_SCHED_YIELD_METHODDEF
    #define OS_SCHED_YIELD_METHODDEF
#endif /* !defined(OS_SCHED_YIELD_METHODDEF) */

#ifndef OS_SCHED_SETAFFINITY_METHODDEF
    #define OS_SCHED_SETAFFINITY_METHODDEF
#endif /* !defined(OS_SCHED_SETAFFINITY_METHODDEF) */

#ifndef OS_SCHED_GETAFFINITY_METHODDEF
    #define OS_SCHED_GETAFFINITY_METHODDEF
#endif /* !defined(OS_SCHED_GETAFFINITY_METHODDEF) */

#ifndef OS_OPENPTY_METHODDEF
    #define OS_OPENPTY_METHODDEF
#endif /* !defined(OS_OPENPTY_METHODDEF) */

#ifndef OS_FORKPTY_METHODDEF
    #define OS_FORKPTY_METHODDEF
#endif /* !defined(OS_FORKPTY_METHODDEF) */

#ifndef OS_GETEGID_METHODDEF
    #define OS_GETEGID_METHODDEF
#endif /* !defined(OS_GETEGID_METHODDEF) */

#ifndef OS_GETEUID_METHODDEF
    #define OS_GETEUID_METHODDEF
#endif /* !defined(OS_GETEUID_METHODDEF) */

#ifndef OS_GETGID_METHODDEF
    #define OS_GETGID_METHODDEF
#endif /* !defined(OS_GETGID_METHODDEF) */

#ifndef OS_GETGROUPS_METHODDEF
    #define OS_GETGROUPS_METHODDEF
#endif /* !defined(OS_GETGROUPS_METHODDEF) */

#ifndef OS_GETPGID_METHODDEF
    #define OS_GETPGID_METHODDEF
#endif /* !defined(OS_GETPGID_METHODDEF) */

#ifndef OS_GETPGRP_METHODDEF
    #define OS_GETPGRP_METHODDEF
#endif /* !defined(OS_GETPGRP_METHODDEF) */

#ifndef OS_SETPGRP_METHODDEF
    #define OS_SETPGRP_METHODDEF
#endif /* !defined(OS_SETPGRP_METHODDEF) */

#ifndef OS_GETPPID_METHODDEF
    #define OS_GETPPID_METHODDEF
#endif /* !defined(OS_GETPPID_METHODDEF) */

#ifndef OS_GETLOGIN_METHODDEF
    #define OS_GETLOGIN_METHODDEF
#endif /* !defined(OS_GETLOGIN_METHODDEF) */

#ifndef OS_GETUID_METHODDEF
    #define OS_GETUID_METHODDEF
#endif /* !defined(OS_GETUID_METHODDEF) */

#ifndef OS_KILL_METHODDEF
    #define OS_KILL_METHODDEF
#endif /* !defined(OS_KILL_METHODDEF) */

#ifndef OS_KILLPG_METHODDEF
    #define OS_KILLPG_METHODDEF
#endif /* !defined(OS_KILLPG_METHODDEF) */

#ifndef OS_PLOCK_METHODDEF
    #define OS_PLOCK_METHODDEF
#endif /* !defined(OS_PLOCK_METHODDEF) */

#ifndef OS_SETUID_METHODDEF
    #define OS_SETUID_METHODDEF
#endif /* !defined(OS_SETUID_METHODDEF) */

#ifndef OS_SETEUID_METHODDEF
    #define OS_SETEUID_METHODDEF
#endif /* !defined(OS_SETEUID_METHODDEF) */

#ifndef OS_SETEGID_METHODDEF
    #define OS_SETEGID_METHODDEF
#endif /* !defined(OS_SETEGID_METHODDEF) */

#ifndef OS_SETREUID_METHODDEF
    #define OS_SETREUID_METHODDEF
#endif /* !defined(OS_SETREUID_METHODDEF) */

#ifndef OS_SETREGID_METHODDEF
    #define OS_SETREGID_METHODDEF
#endif /* !defined(OS_SETREGID_METHODDEF) */

#ifndef OS_SETGID_METHODDEF
    #define OS_SETGID_METHODDEF
#endif /* !defined(OS_SETGID_METHODDEF) */

#ifndef OS_SETGROUPS_METHODDEF
    #define OS_SETGROUPS_METHODDEF
#endif /* !defined(OS_SETGROUPS_METHODDEF) */

#ifndef OS_WAIT3_METHODDEF
    #define OS_WAIT3_METHODDEF
#endif /* !defined(OS_WAIT3_METHODDEF) */

#ifndef OS_WAIT4_METHODDEF
    #define OS_WAIT4_METHODDEF
#endif /* !defined(OS_WAIT4_METHODDEF) */

#ifndef OS_WAITID_METHODDEF
    #define OS_WAITID_METHODDEF
#endif /* !defined(OS_WAITID_METHODDEF) */

#ifndef OS_WAITPID_METHODDEF
    #define OS_WAITPID_METHODDEF
#endif /* !defined(OS_WAITPID_METHODDEF) */

#ifndef OS_WAITPID_METHODDEF
    #define OS_WAITPID_METHODDEF
#endif /* !defined(OS_WAITPID_METHODDEF) */

#ifndef OS_WAIT_METHODDEF
    #define OS_WAIT_METHODDEF
#endif /* !defined(OS_WAIT_METHODDEF) */

#ifndef OS_SYMLINK_METHODDEF
    #define OS_SYMLINK_METHODDEF
#endif /* !defined(OS_SYMLINK_METHODDEF) */

#ifndef OS_TIMES_METHODDEF
    #define OS_TIMES_METHODDEF
#endif /* !defined(OS_TIMES_METHODDEF) */

#ifndef OS_GETSID_METHODDEF
    #define OS_GETSID_METHODDEF
#endif /* !defined(OS_GETSID_METHODDEF) */

#ifndef OS_SETSID_METHODDEF
    #define OS_SETSID_METHODDEF
#endif /* !defined(OS_SETSID_METHODDEF) */

#ifndef OS_SETPGID_METHODDEF
    #define OS_SETPGID_METHODDEF
#endif /* !defined(OS_SETPGID_METHODDEF) */

#ifndef OS_TCGETPGRP_METHODDEF
    #define OS_TCGETPGRP_METHODDEF
#endif /* !defined(OS_TCGETPGRP_METHODDEF) */

#ifndef OS_TCSETPGRP_METHODDEF
    #define OS_TCSETPGRP_METHODDEF
#endif /* !defined(OS_TCSETPGRP_METHODDEF) */

#ifndef OS_LOCKF_METHODDEF
    #define OS_LOCKF_METHODDEF
#endif /* !defined(OS_LOCKF_METHODDEF) */

#ifndef OS_READV_METHODDEF
    #define OS_READV_METHODDEF
#endif /* !defined(OS_READV_METHODDEF) */

#ifndef OS_PREAD_METHODDEF
    #define OS_PREAD_METHODDEF
#endif /* !defined(OS_PREAD_METHODDEF) */

#ifndef OS_PIPE_METHODDEF
    #define OS_PIPE_METHODDEF
#endif /* !defined(OS_PIPE_METHODDEF) */

#ifndef OS_PIPE2_METHODDEF
    #define OS_PIPE2_METHODDEF
#endif /* !defined(OS_PIPE2_METHODDEF) */

#ifndef OS_WRITEV_METHODDEF
    #define OS_WRITEV_METHODDEF
#endif /* !defined(OS_WRITEV_METHODDEF) */

#ifndef OS_PWRITE_METHODDEF
    #define OS_PWRITE_METHODDEF
#endif /* !defined(OS_PWRITE_METHODDEF) */

#ifndef OS_MKFIFO_METHODDEF
    #define OS_MKFIFO_METHODDEF
#endif /* !defined(OS_MKFIFO_METHODDEF) */

#ifndef OS_MKNOD_METHODDEF
    #define OS_MKNOD_METHODDEF
#endif /* !defined(OS_MKNOD_METHODDEF) */

#ifndef OS_MAJOR_METHODDEF
    #define OS_MAJOR_METHODDEF
#endif /* !defined(OS_MAJOR_METHODDEF) */

#ifndef OS_MINOR_METHODDEF
    #define OS_MINOR_METHODDEF
#endif /* !defined(OS_MINOR_METHODDEF) */

#ifndef OS_MAKEDEV_METHODDEF
    #define OS_MAKEDEV_METHODDEF
#endif /* !defined(OS_MAKEDEV_METHODDEF) */

#ifndef OS_FTRUNCATE_METHODDEF
    #define OS_FTRUNCATE_METHODDEF
#endif /* !defined(OS_FTRUNCATE_METHODDEF) */

#ifndef OS_TRUNCATE_METHODDEF
    #define OS_TRUNCATE_METHODDEF
#endif /* !defined(OS_TRUNCATE_METHODDEF) */

#ifndef OS_POSIX_FALLOCATE_METHODDEF
    #define OS_POSIX_FALLOCATE_METHODDEF
#endif /* !defined(OS_POSIX_FALLOCATE_METHODDEF) */

#ifndef OS_POSIX_FADVISE_METHODDEF
    #define OS_POSIX_FADVISE_METHODDEF
#endif /* !defined(OS_POSIX_FADVISE_METHODDEF) */

#ifndef OS_PUTENV_METHODDEF
    #define OS_PUTENV_METHODDEF
#endif /* !defined(OS_PUTENV_METHODDEF) */

#ifndef OS_PUTENV_METHODDEF
    #define OS_PUTENV_METHODDEF
#endif /* !defined(OS_PUTENV_METHODDEF) */

#ifndef OS_UNSETENV_METHODDEF
    #define OS_UNSETENV_METHODDEF
#endif /* !defined(OS_UNSETENV_METHODDEF) */

#ifndef OS_WCOREDUMP_METHODDEF
    #define OS_WCOREDUMP_METHODDEF
#endif /* !defined(OS_WCOREDUMP_METHODDEF) */

#ifndef OS_WIFCONTINUED_METHODDEF
    #define OS_WIFCONTINUED_METHODDEF
#endif /* !defined(OS_WIFCONTINUED_METHODDEF) */

#ifndef OS_WIFSTOPPED_METHODDEF
    #define OS_WIFSTOPPED_METHODDEF
#endif /* !defined(OS_WIFSTOPPED_METHODDEF) */

#ifndef OS_WIFSIGNALED_METHODDEF
    #define OS_WIFSIGNALED_METHODDEF
#endif /* !defined(OS_WIFSIGNALED_METHODDEF) */

#ifndef OS_WIFEXITED_METHODDEF
    #define OS_WIFEXITED_METHODDEF
#endif /* !defined(OS_WIFEXITED_METHODDEF) */

#ifndef OS_WEXITSTATUS_METHODDEF
    #define OS_WEXITSTATUS_METHODDEF
#endif /* !defined(OS_WEXITSTATUS_METHODDEF) */

#ifndef OS_WTERMSIG_METHODDEF
    #define OS_WTERMSIG_METHODDEF
#endif /* !defined(OS_WTERMSIG_METHODDEF) */

#ifndef OS_WSTOPSIG_METHODDEF
    #define OS_WSTOPSIG_METHODDEF
#endif /* !defined(OS_WSTOPSIG_METHODDEF) */

#ifndef OS_FSTATVFS_METHODDEF
    #define OS_FSTATVFS_METHODDEF
#endif /* !defined(OS_FSTATVFS_METHODDEF) */

#ifndef OS_STATVFS_METHODDEF
    #define OS_STATVFS_METHODDEF
#endif /* !defined(OS_STATVFS_METHODDEF) */

#ifndef OS__GETDISKUSAGE_METHODDEF
    #define OS__GETDISKUSAGE_METHODDEF
#endif /* !defined(OS__GETDISKUSAGE_METHODDEF) */

#ifndef OS_FPATHCONF_METHODDEF
    #define OS_FPATHCONF_METHODDEF
#endif /* !defined(OS_FPATHCONF_METHODDEF) */

#ifndef OS_PATHCONF_METHODDEF
    #define OS_PATHCONF_METHODDEF
#endif /* !defined(OS_PATHCONF_METHODDEF) */

#ifndef OS_CONFSTR_METHODDEF
    #define OS_CONFSTR_METHODDEF
#endif /* !defined(OS_CONFSTR_METHODDEF) */

#ifndef OS_SYSCONF_METHODDEF
    #define OS_SYSCONF_METHODDEF
#endif /* !defined(OS_SYSCONF_METHODDEF) */

#ifndef OS_GETLOADAVG_METHODDEF
    #define OS_GETLOADAVG_METHODDEF
#endif /* !defined(OS_GETLOADAVG_METHODDEF) */

#ifndef OS_SETRESUID_METHODDEF
    #define OS_SETRESUID_METHODDEF
#endif /* !defined(OS_SETRESUID_METHODDEF) */

#ifndef OS_SETRESGID_METHODDEF
    #define OS_SETRESGID_METHODDEF
#endif /* !defined(OS_SETRESGID_METHODDEF) */

#ifndef OS_GETRESUID_METHODDEF
    #define OS_GETRESUID_METHODDEF
#endif /* !defined(OS_GETRESUID_METHODDEF) */

#ifndef OS_GETRESGID_METHODDEF
    #define OS_GETRESGID_METHODDEF
#endif /* !defined(OS_GETRESGID_METHODDEF) */

#ifndef OS_GETXATTR_METHODDEF
    #define OS_GETXATTR_METHODDEF
#endif /* !defined(OS_GETXATTR_METHODDEF) */

#ifndef OS_SETXATTR_METHODDEF
    #define OS_SETXATTR_METHODDEF
#endif /* !defined(OS_SETXATTR_METHODDEF) */

#ifndef OS_REMOVEXATTR_METHODDEF
    #define OS_REMOVEXATTR_METHODDEF
#endif /* !defined(OS_REMOVEXATTR_METHODDEF) */

#ifndef OS_LISTXATTR_METHODDEF
    #define OS_LISTXATTR_METHODDEF
#endif /* !defined(OS_LISTXATTR_METHODDEF) */

#ifndef OS_GET_HANDLE_INHERITABLE_METHODDEF
    #define OS_GET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_GET_HANDLE_INHERITABLE_METHODDEF) */

#ifndef OS_SET_HANDLE_INHERITABLE_METHODDEF
    #define OS_SET_HANDLE_INHERITABLE_METHODDEF
#endif /* !defined(OS_SET_HANDLE_INHERITABLE_METHODDEF) */
/*[clinic end generated code: output=52a6140b0b052ce6 input=524ce2e021e4eba6]*/


static PyMethodDef posix_methods[] = {

    OS_STAT_METHODDEF
    OS_ACCESS_METHODDEF
    OS_TTYNAME_METHODDEF
    OS_CHDIR_METHODDEF
    OS_CHFLAGS_METHODDEF
    OS_CHMOD_METHODDEF
    OS_FCHMOD_METHODDEF
    OS_LCHMOD_METHODDEF
    OS_CHOWN_METHODDEF
    OS_FCHOWN_METHODDEF
    OS_LCHOWN_METHODDEF
    OS_LCHFLAGS_METHODDEF
    OS_CHROOT_METHODDEF
    OS_CTERMID_METHODDEF
    OS_GETCWD_METHODDEF
    OS_GETCWDB_METHODDEF
    OS_LINK_METHODDEF
    OS_LISTDIR_METHODDEF
    OS_LSTAT_METHODDEF
    OS_MKDIR_METHODDEF
    OS_NICE_METHODDEF
    OS_GETPRIORITY_METHODDEF
    OS_SETPRIORITY_METHODDEF
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
    OS_RENAME_METHODDEF
    OS_REPLACE_METHODDEF
    OS_RMDIR_METHODDEF
    {"stat_float_times", stat_float_times, METH_VARARGS, stat_float_times__doc__},
    OS_SYMLINK_METHODDEF
    OS_SYSTEM_METHODDEF
    OS_UMASK_METHODDEF
    OS_UNAME_METHODDEF
    OS_UNLINK_METHODDEF
    OS_REMOVE_METHODDEF
    OS_UTIME_METHODDEF
    OS_TIMES_METHODDEF
    OS__EXIT_METHODDEF
    OS_EXECV_METHODDEF
    OS_EXECVE_METHODDEF
    OS_SPAWNV_METHODDEF
    OS_SPAWNVE_METHODDEF
    OS_FORK1_METHODDEF
    OS_FORK_METHODDEF
    OS_SCHED_GET_PRIORITY_MAX_METHODDEF
    OS_SCHED_GET_PRIORITY_MIN_METHODDEF
    OS_SCHED_GETPARAM_METHODDEF
    OS_SCHED_GETSCHEDULER_METHODDEF
    OS_SCHED_RR_GET_INTERVAL_METHODDEF
    OS_SCHED_SETPARAM_METHODDEF
    OS_SCHED_SETSCHEDULER_METHODDEF
    OS_SCHED_YIELD_METHODDEF
    OS_SCHED_SETAFFINITY_METHODDEF
    OS_SCHED_GETAFFINITY_METHODDEF
    OS_OPENPTY_METHODDEF
    OS_FORKPTY_METHODDEF
    OS_GETEGID_METHODDEF
    OS_GETEUID_METHODDEF
    OS_GETGID_METHODDEF
#ifdef HAVE_GETGROUPLIST
    {"getgrouplist",    posix_getgrouplist, METH_VARARGS, posix_getgrouplist__doc__},
#endif
    OS_GETGROUPS_METHODDEF
    OS_GETPID_METHODDEF
    OS_GETPGRP_METHODDEF
    OS_GETPPID_METHODDEF
    OS_GETUID_METHODDEF
    OS_GETLOGIN_METHODDEF
    OS_KILL_METHODDEF
    OS_KILLPG_METHODDEF
    OS_PLOCK_METHODDEF
#ifdef MS_WINDOWS
    {"startfile",       win32_startfile, METH_VARARGS, win32_startfile__doc__},
#endif
    OS_SETUID_METHODDEF
    OS_SETEUID_METHODDEF
    OS_SETREUID_METHODDEF
    OS_SETGID_METHODDEF
    OS_SETEGID_METHODDEF
    OS_SETREGID_METHODDEF
    OS_SETGROUPS_METHODDEF
#ifdef HAVE_INITGROUPS
    {"initgroups",      posix_initgroups, METH_VARARGS, posix_initgroups__doc__},
#endif /* HAVE_INITGROUPS */
    OS_GETPGID_METHODDEF
    OS_SETPGRP_METHODDEF
    OS_WAIT_METHODDEF
    OS_WAIT3_METHODDEF
    OS_WAIT4_METHODDEF
    OS_WAITID_METHODDEF
    OS_WAITPID_METHODDEF
    OS_GETSID_METHODDEF
    OS_SETSID_METHODDEF
    OS_SETPGID_METHODDEF
    OS_TCGETPGRP_METHODDEF
    OS_TCSETPGRP_METHODDEF
    OS_OPEN_METHODDEF
    OS_CLOSE_METHODDEF
    OS_CLOSERANGE_METHODDEF
    OS_DEVICE_ENCODING_METHODDEF
    OS_DUP_METHODDEF
    OS_DUP2_METHODDEF
    OS_LOCKF_METHODDEF
    OS_LSEEK_METHODDEF
    OS_READ_METHODDEF
    OS_READV_METHODDEF
    OS_PREAD_METHODDEF
    OS_WRITE_METHODDEF
    OS_WRITEV_METHODDEF
    OS_PWRITE_METHODDEF
#ifdef HAVE_SENDFILE
    {"sendfile",        (PyCFunction)posix_sendfile, METH_VARARGS | METH_KEYWORDS,
                            posix_sendfile__doc__},
#endif
    OS_FSTAT_METHODDEF
    OS_ISATTY_METHODDEF
    OS_PIPE_METHODDEF
    OS_PIPE2_METHODDEF
    OS_MKFIFO_METHODDEF
    OS_MKNOD_METHODDEF
    OS_MAJOR_METHODDEF
    OS_MINOR_METHODDEF
    OS_MAKEDEV_METHODDEF
    OS_FTRUNCATE_METHODDEF
    OS_TRUNCATE_METHODDEF
    OS_POSIX_FALLOCATE_METHODDEF
    OS_POSIX_FADVISE_METHODDEF
    OS_PUTENV_METHODDEF
    OS_UNSETENV_METHODDEF
    OS_STRERROR_METHODDEF
    OS_FCHDIR_METHODDEF
    OS_FSYNC_METHODDEF
    OS_SYNC_METHODDEF
    OS_FDATASYNC_METHODDEF
    OS_WCOREDUMP_METHODDEF
    OS_WIFCONTINUED_METHODDEF
    OS_WIFSTOPPED_METHODDEF
    OS_WIFSIGNALED_METHODDEF
    OS_WIFEXITED_METHODDEF
    OS_WEXITSTATUS_METHODDEF
    OS_WTERMSIG_METHODDEF
    OS_WSTOPSIG_METHODDEF
    OS_FSTATVFS_METHODDEF
    OS_STATVFS_METHODDEF
    OS_CONFSTR_METHODDEF
    OS_SYSCONF_METHODDEF
    OS_FPATHCONF_METHODDEF
    OS_PATHCONF_METHODDEF
    OS_ABORT_METHODDEF
#ifdef MS_WINDOWS
    {"_getfullpathname",        posix__getfullpathname, METH_VARARGS, NULL},
    {"_isdir",                  posix__isdir, METH_VARARGS, posix__isdir__doc__},
#endif
    OS__GETDISKUSAGE_METHODDEF
    OS__GETFINALPATHNAME_METHODDEF
    OS__GETVOLUMEPATHNAME_METHODDEF
    OS_GETLOADAVG_METHODDEF
    OS_URANDOM_METHODDEF
    OS_SETRESUID_METHODDEF
    OS_SETRESGID_METHODDEF
    OS_GETRESUID_METHODDEF
    OS_GETRESGID_METHODDEF

    OS_GETXATTR_METHODDEF
    OS_SETXATTR_METHODDEF
    OS_REMOVEXATTR_METHODDEF
    OS_LISTXATTR_METHODDEF

#if defined(TERMSIZE_USE_CONIO) || defined(TERMSIZE_USE_IOCTL)
    {"get_terminal_size", get_terminal_size, METH_VARARGS, termsize__doc__},
#endif
    OS_CPU_COUNT_METHODDEF
    OS_GET_INHERITABLE_METHODDEF
    OS_SET_INHERITABLE_METHODDEF
    OS_GET_HANDLE_INHERITABLE_METHODDEF
    OS_SET_HANDLE_INHERITABLE_METHODDEF
#ifndef MS_WINDOWS
    {"get_blocking", posix_get_blocking, METH_VARARGS, get_blocking__doc__},
    {"set_blocking", posix_set_blocking, METH_VARARGS, set_blocking__doc__},
#endif
    {NULL,              NULL}            /* Sentinel */
};


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
all_ins(PyObject *m)
{
#ifdef F_OK
    if (PyModule_AddIntMacro(m, F_OK)) return -1;
#endif
#ifdef R_OK
    if (PyModule_AddIntMacro(m, R_OK)) return -1;
#endif
#ifdef W_OK
    if (PyModule_AddIntMacro(m, W_OK)) return -1;
#endif
#ifdef X_OK
    if (PyModule_AddIntMacro(m, X_OK)) return -1;
#endif
#ifdef NGROUPS_MAX
    if (PyModule_AddIntMacro(m, NGROUPS_MAX)) return -1;
#endif
#ifdef TMP_MAX
    if (PyModule_AddIntMacro(m, TMP_MAX)) return -1;
#endif
#ifdef WCONTINUED
    if (PyModule_AddIntMacro(m, WCONTINUED)) return -1;
#endif
#ifdef WNOHANG
    if (PyModule_AddIntMacro(m, WNOHANG)) return -1;
#endif
#ifdef WUNTRACED
    if (PyModule_AddIntMacro(m, WUNTRACED)) return -1;
#endif
#ifdef O_RDONLY
    if (PyModule_AddIntMacro(m, O_RDONLY)) return -1;
#endif
#ifdef O_WRONLY
    if (PyModule_AddIntMacro(m, O_WRONLY)) return -1;
#endif
#ifdef O_RDWR
    if (PyModule_AddIntMacro(m, O_RDWR)) return -1;
#endif
#ifdef O_NDELAY
    if (PyModule_AddIntMacro(m, O_NDELAY)) return -1;
#endif
#ifdef O_NONBLOCK
    if (PyModule_AddIntMacro(m, O_NONBLOCK)) return -1;
#endif
#ifdef O_APPEND
    if (PyModule_AddIntMacro(m, O_APPEND)) return -1;
#endif
#ifdef O_DSYNC
    if (PyModule_AddIntMacro(m, O_DSYNC)) return -1;
#endif
#ifdef O_RSYNC
    if (PyModule_AddIntMacro(m, O_RSYNC)) return -1;
#endif
#ifdef O_SYNC
    if (PyModule_AddIntMacro(m, O_SYNC)) return -1;
#endif
#ifdef O_NOCTTY
    if (PyModule_AddIntMacro(m, O_NOCTTY)) return -1;
#endif
#ifdef O_CREAT
    if (PyModule_AddIntMacro(m, O_CREAT)) return -1;
#endif
#ifdef O_EXCL
    if (PyModule_AddIntMacro(m, O_EXCL)) return -1;
#endif
#ifdef O_TRUNC
    if (PyModule_AddIntMacro(m, O_TRUNC)) return -1;
#endif
#ifdef O_BINARY
    if (PyModule_AddIntMacro(m, O_BINARY)) return -1;
#endif
#ifdef O_TEXT
    if (PyModule_AddIntMacro(m, O_TEXT)) return -1;
#endif
#ifdef O_XATTR
    if (PyModule_AddIntMacro(m, O_XATTR)) return -1;
#endif
#ifdef O_LARGEFILE
    if (PyModule_AddIntMacro(m, O_LARGEFILE)) return -1;
#endif
#ifdef O_SHLOCK
    if (PyModule_AddIntMacro(m, O_SHLOCK)) return -1;
#endif
#ifdef O_EXLOCK
    if (PyModule_AddIntMacro(m, O_EXLOCK)) return -1;
#endif
#ifdef O_EXEC
    if (PyModule_AddIntMacro(m, O_EXEC)) return -1;
#endif
#ifdef O_SEARCH
    if (PyModule_AddIntMacro(m, O_SEARCH)) return -1;
#endif
#ifdef O_PATH
    if (PyModule_AddIntMacro(m, O_PATH)) return -1;
#endif
#ifdef O_TTY_INIT
    if (PyModule_AddIntMacro(m, O_TTY_INIT)) return -1;
#endif
#ifdef O_TMPFILE
    if (PyModule_AddIntMacro(m, O_TMPFILE)) return -1;
#endif
#ifdef PRIO_PROCESS
    if (PyModule_AddIntMacro(m, PRIO_PROCESS)) return -1;
#endif
#ifdef PRIO_PGRP
    if (PyModule_AddIntMacro(m, PRIO_PGRP)) return -1;
#endif
#ifdef PRIO_USER
    if (PyModule_AddIntMacro(m, PRIO_USER)) return -1;
#endif
#ifdef O_CLOEXEC
    if (PyModule_AddIntMacro(m, O_CLOEXEC)) return -1;
#endif
#ifdef O_ACCMODE
    if (PyModule_AddIntMacro(m, O_ACCMODE)) return -1;
#endif


#ifdef SEEK_HOLE
    if (PyModule_AddIntMacro(m, SEEK_HOLE)) return -1;
#endif
#ifdef SEEK_DATA
    if (PyModule_AddIntMacro(m, SEEK_DATA)) return -1;
#endif

/* MS Windows */
#ifdef O_NOINHERIT
    /* Don't inherit in child processes. */
    if (PyModule_AddIntMacro(m, O_NOINHERIT)) return -1;
#endif
#ifdef _O_SHORT_LIVED
    /* Optimize for short life (keep in memory). */
    /* MS forgot to define this one with a non-underscore form too. */
    if (PyModule_AddIntConstant(m, "O_SHORT_LIVED", _O_SHORT_LIVED)) return -1;
#endif
#ifdef O_TEMPORARY
    /* Automatically delete when last handle is closed. */
    if (PyModule_AddIntMacro(m, O_TEMPORARY)) return -1;
#endif
#ifdef O_RANDOM
    /* Optimize for random access. */
    if (PyModule_AddIntMacro(m, O_RANDOM)) return -1;
#endif
#ifdef O_SEQUENTIAL
    /* Optimize for sequential access. */
    if (PyModule_AddIntMacro(m, O_SEQUENTIAL)) return -1;
#endif

/* GNU extensions. */
#ifdef O_ASYNC
    /* Send a SIGIO signal whenever input or output
       becomes available on file descriptor */
    if (PyModule_AddIntMacro(m, O_ASYNC)) return -1;
#endif
#ifdef O_DIRECT
    /* Direct disk access. */
    if (PyModule_AddIntMacro(m, O_DIRECT)) return -1;
#endif
#ifdef O_DIRECTORY
    /* Must be a directory.      */
    if (PyModule_AddIntMacro(m, O_DIRECTORY)) return -1;
#endif
#ifdef O_NOFOLLOW
    /* Do not follow links.      */
    if (PyModule_AddIntMacro(m, O_NOFOLLOW)) return -1;
#endif
#ifdef O_NOLINKS
    /* Fails if link count of the named file is greater than 1 */
    if (PyModule_AddIntMacro(m, O_NOLINKS)) return -1;
#endif
#ifdef O_NOATIME
    /* Do not update the access time. */
    if (PyModule_AddIntMacro(m, O_NOATIME)) return -1;
#endif

    /* These come from sysexits.h */
#ifdef EX_OK
    if (PyModule_AddIntMacro(m, EX_OK)) return -1;
#endif /* EX_OK */
#ifdef EX_USAGE
    if (PyModule_AddIntMacro(m, EX_USAGE)) return -1;
#endif /* EX_USAGE */
#ifdef EX_DATAERR
    if (PyModule_AddIntMacro(m, EX_DATAERR)) return -1;
#endif /* EX_DATAERR */
#ifdef EX_NOINPUT
    if (PyModule_AddIntMacro(m, EX_NOINPUT)) return -1;
#endif /* EX_NOINPUT */
#ifdef EX_NOUSER
    if (PyModule_AddIntMacro(m, EX_NOUSER)) return -1;
#endif /* EX_NOUSER */
#ifdef EX_NOHOST
    if (PyModule_AddIntMacro(m, EX_NOHOST)) return -1;
#endif /* EX_NOHOST */
#ifdef EX_UNAVAILABLE
    if (PyModule_AddIntMacro(m, EX_UNAVAILABLE)) return -1;
#endif /* EX_UNAVAILABLE */
#ifdef EX_SOFTWARE
    if (PyModule_AddIntMacro(m, EX_SOFTWARE)) return -1;
#endif /* EX_SOFTWARE */
#ifdef EX_OSERR
    if (PyModule_AddIntMacro(m, EX_OSERR)) return -1;
#endif /* EX_OSERR */
#ifdef EX_OSFILE
    if (PyModule_AddIntMacro(m, EX_OSFILE)) return -1;
#endif /* EX_OSFILE */
#ifdef EX_CANTCREAT
    if (PyModule_AddIntMacro(m, EX_CANTCREAT)) return -1;
#endif /* EX_CANTCREAT */
#ifdef EX_IOERR
    if (PyModule_AddIntMacro(m, EX_IOERR)) return -1;
#endif /* EX_IOERR */
#ifdef EX_TEMPFAIL
    if (PyModule_AddIntMacro(m, EX_TEMPFAIL)) return -1;
#endif /* EX_TEMPFAIL */
#ifdef EX_PROTOCOL
    if (PyModule_AddIntMacro(m, EX_PROTOCOL)) return -1;
#endif /* EX_PROTOCOL */
#ifdef EX_NOPERM
    if (PyModule_AddIntMacro(m, EX_NOPERM)) return -1;
#endif /* EX_NOPERM */
#ifdef EX_CONFIG
    if (PyModule_AddIntMacro(m, EX_CONFIG)) return -1;
#endif /* EX_CONFIG */
#ifdef EX_NOTFOUND
    if (PyModule_AddIntMacro(m, EX_NOTFOUND)) return -1;
#endif /* EX_NOTFOUND */

    /* statvfs */
#ifdef ST_RDONLY
    if (PyModule_AddIntMacro(m, ST_RDONLY)) return -1;
#endif /* ST_RDONLY */
#ifdef ST_NOSUID
    if (PyModule_AddIntMacro(m, ST_NOSUID)) return -1;
#endif /* ST_NOSUID */

       /* GNU extensions */
#ifdef ST_NODEV
    if (PyModule_AddIntMacro(m, ST_NODEV)) return -1;
#endif /* ST_NODEV */
#ifdef ST_NOEXEC
    if (PyModule_AddIntMacro(m, ST_NOEXEC)) return -1;
#endif /* ST_NOEXEC */
#ifdef ST_SYNCHRONOUS
    if (PyModule_AddIntMacro(m, ST_SYNCHRONOUS)) return -1;
#endif /* ST_SYNCHRONOUS */
#ifdef ST_MANDLOCK
    if (PyModule_AddIntMacro(m, ST_MANDLOCK)) return -1;
#endif /* ST_MANDLOCK */
#ifdef ST_WRITE
    if (PyModule_AddIntMacro(m, ST_WRITE)) return -1;
#endif /* ST_WRITE */
#ifdef ST_APPEND
    if (PyModule_AddIntMacro(m, ST_APPEND)) return -1;
#endif /* ST_APPEND */
#ifdef ST_NOATIME
    if (PyModule_AddIntMacro(m, ST_NOATIME)) return -1;
#endif /* ST_NOATIME */
#ifdef ST_NODIRATIME
    if (PyModule_AddIntMacro(m, ST_NODIRATIME)) return -1;
#endif /* ST_NODIRATIME */
#ifdef ST_RELATIME
    if (PyModule_AddIntMacro(m, ST_RELATIME)) return -1;
#endif /* ST_RELATIME */

    /* FreeBSD sendfile() constants */
#ifdef SF_NODISKIO
    if (PyModule_AddIntMacro(m, SF_NODISKIO)) return -1;
#endif
#ifdef SF_MNOWAIT
    if (PyModule_AddIntMacro(m, SF_MNOWAIT)) return -1;
#endif
#ifdef SF_SYNC
    if (PyModule_AddIntMacro(m, SF_SYNC)) return -1;
#endif

    /* constants for posix_fadvise */
#ifdef POSIX_FADV_NORMAL
    if (PyModule_AddIntMacro(m, POSIX_FADV_NORMAL)) return -1;
#endif
#ifdef POSIX_FADV_SEQUENTIAL
    if (PyModule_AddIntMacro(m, POSIX_FADV_SEQUENTIAL)) return -1;
#endif
#ifdef POSIX_FADV_RANDOM
    if (PyModule_AddIntMacro(m, POSIX_FADV_RANDOM)) return -1;
#endif
#ifdef POSIX_FADV_NOREUSE
    if (PyModule_AddIntMacro(m, POSIX_FADV_NOREUSE)) return -1;
#endif
#ifdef POSIX_FADV_WILLNEED
    if (PyModule_AddIntMacro(m, POSIX_FADV_WILLNEED)) return -1;
#endif
#ifdef POSIX_FADV_DONTNEED
    if (PyModule_AddIntMacro(m, POSIX_FADV_DONTNEED)) return -1;
#endif

    /* constants for waitid */
#if defined(HAVE_SYS_WAIT_H) && defined(HAVE_WAITID)
    if (PyModule_AddIntMacro(m, P_PID)) return -1;
    if (PyModule_AddIntMacro(m, P_PGID)) return -1;
    if (PyModule_AddIntMacro(m, P_ALL)) return -1;
#endif
#ifdef WEXITED
    if (PyModule_AddIntMacro(m, WEXITED)) return -1;
#endif
#ifdef WNOWAIT
    if (PyModule_AddIntMacro(m, WNOWAIT)) return -1;
#endif
#ifdef WSTOPPED
    if (PyModule_AddIntMacro(m, WSTOPPED)) return -1;
#endif
#ifdef CLD_EXITED
    if (PyModule_AddIntMacro(m, CLD_EXITED)) return -1;
#endif
#ifdef CLD_DUMPED
    if (PyModule_AddIntMacro(m, CLD_DUMPED)) return -1;
#endif
#ifdef CLD_TRAPPED
    if (PyModule_AddIntMacro(m, CLD_TRAPPED)) return -1;
#endif
#ifdef CLD_CONTINUED
    if (PyModule_AddIntMacro(m, CLD_CONTINUED)) return -1;
#endif

    /* constants for lockf */
#ifdef F_LOCK
    if (PyModule_AddIntMacro(m, F_LOCK)) return -1;
#endif
#ifdef F_TLOCK
    if (PyModule_AddIntMacro(m, F_TLOCK)) return -1;
#endif
#ifdef F_ULOCK
    if (PyModule_AddIntMacro(m, F_ULOCK)) return -1;
#endif
#ifdef F_TEST
    if (PyModule_AddIntMacro(m, F_TEST)) return -1;
#endif

#ifdef HAVE_SPAWNV
    if (PyModule_AddIntConstant(m, "P_WAIT", _P_WAIT)) return -1;
    if (PyModule_AddIntConstant(m, "P_NOWAIT", _P_NOWAIT)) return -1;
    if (PyModule_AddIntConstant(m, "P_OVERLAY", _OLD_P_OVERLAY)) return -1;
    if (PyModule_AddIntConstant(m, "P_NOWAITO", _P_NOWAITO)) return -1;
    if (PyModule_AddIntConstant(m, "P_DETACH", _P_DETACH)) return -1;
#endif

#ifdef HAVE_SCHED_H
    if (PyModule_AddIntMacro(m, SCHED_OTHER)) return -1;
    if (PyModule_AddIntMacro(m, SCHED_FIFO)) return -1;
    if (PyModule_AddIntMacro(m, SCHED_RR)) return -1;
#ifdef SCHED_SPORADIC
    if (PyModule_AddIntMacro(m, SCHED_SPORADIC) return -1;
#endif
#ifdef SCHED_BATCH
    if (PyModule_AddIntMacro(m, SCHED_BATCH)) return -1;
#endif
#ifdef SCHED_IDLE
    if (PyModule_AddIntMacro(m, SCHED_IDLE)) return -1;
#endif
#ifdef SCHED_RESET_ON_FORK
    if (PyModule_AddIntMacro(m, SCHED_RESET_ON_FORK)) return -1;
#endif
#ifdef SCHED_SYS
    if (PyModule_AddIntMacro(m, SCHED_SYS)) return -1;
#endif
#ifdef SCHED_IA
    if (PyModule_AddIntMacro(m, SCHED_IA)) return -1;
#endif
#ifdef SCHED_FSS
    if (PyModule_AddIntMacro(m, SCHED_FSS)) return -1;
#endif
#ifdef SCHED_FX
    if (PyModule_AddIntConstant(m, "SCHED_FX", SCHED_FSS)) return -1;
#endif
#endif

#ifdef USE_XATTRS
    if (PyModule_AddIntMacro(m, XATTR_CREATE)) return -1;
    if (PyModule_AddIntMacro(m, XATTR_REPLACE)) return -1;
    if (PyModule_AddIntMacro(m, XATTR_SIZE_MAX)) return -1;
#endif

#ifdef RTLD_LAZY
    if (PyModule_AddIntMacro(m, RTLD_LAZY)) return -1;
#endif
#ifdef RTLD_NOW
    if (PyModule_AddIntMacro(m, RTLD_NOW)) return -1;
#endif
#ifdef RTLD_GLOBAL
    if (PyModule_AddIntMacro(m, RTLD_GLOBAL)) return -1;
#endif
#ifdef RTLD_LOCAL
    if (PyModule_AddIntMacro(m, RTLD_LOCAL)) return -1;
#endif
#ifdef RTLD_NODELETE
    if (PyModule_AddIntMacro(m, RTLD_NODELETE)) return -1;
#endif
#ifdef RTLD_NOLOAD
    if (PyModule_AddIntMacro(m, RTLD_NOLOAD)) return -1;
#endif
#ifdef RTLD_DEEPBIND
    if (PyModule_AddIntMacro(m, RTLD_DEEPBIND)) return -1;
#endif

    return 0;
}


#ifdef MS_WINDOWS
#define INITFUNC PyInit_nt
#define MODNAME "nt"

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

#ifdef HAVE_FCHOWNAT
    "HAVE_FCHOWNAT",
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

#ifdef HAVE_PUTENV
    if (posix_putenv_garbage == NULL)
        posix_putenv_garbage = PyDict_New();
#endif

    if (!initialized) {
#if defined(HAVE_WAITID) && !defined(__APPLE__)
        waitid_result_desc.name = MODNAME ".waitid_result";
        if (PyStructSequence_InitType2(&WaitidResultType, &waitid_result_desc) < 0)
            return NULL;
#endif

        stat_result_desc.name = "os.stat_result"; /* see issue #19209 */
        stat_result_desc.fields[7].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[8].name = PyStructSequence_UnnamedField;
        stat_result_desc.fields[9].name = PyStructSequence_UnnamedField;
        if (PyStructSequence_InitType2(&StatResultType, &stat_result_desc) < 0)
            return NULL;
        structseq_new = StatResultType.tp_new;
        StatResultType.tp_new = statresult_new;

        statvfs_result_desc.name = "os.statvfs_result"; /* see issue #19209 */
        if (PyStructSequence_InitType2(&StatVFSResultType,
                                       &statvfs_result_desc) < 0)
            return NULL;
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
        if (PyStructSequence_InitType2(&SchedParamType, &sched_param_desc) < 0)
            return NULL;
        SchedParamType.tp_new = os_sched_param;
#endif

        /* initialize TerminalSize_info */
        if (PyStructSequence_InitType2(&TerminalSizeType,
                                       &TerminalSize_desc) < 0)
            return NULL;
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

    times_result_desc.name = MODNAME ".times_result";
    if (PyStructSequence_InitType2(&TimesResultType, &times_result_desc) < 0)
        return NULL;
    PyModule_AddObject(m, "times_result", (PyObject *)&TimesResultType);

    uname_result_desc.name = MODNAME ".uname_result";
    if (PyStructSequence_InitType2(&UnameResultType, &uname_result_desc) < 0)
        return NULL;
    PyModule_AddObject(m, "uname_result", (PyObject *)&UnameResultType);

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

    Py_INCREF(&TerminalSizeType);
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
