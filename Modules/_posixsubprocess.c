/* Authors: Gregory P. Smith & Jeffrey Yasskin */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_fileutils.h"
#if defined(HAVE_PIPE2) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#if defined(HAVE_SYS_RESOURCE_H)
#include <sys/resource.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif /* HAVE_GRP_H */

#include "posixmodule.h"

#ifdef _Py_MEMORY_SANITIZER
# include <sanitizer/msan_interface.h>
#endif

#if defined(__ANDROID__) && __ANDROID_API__ < 21 && !defined(SYS_getdents64)
# include <sys/linux-syscalls.h>
# define SYS_getdents64  __NR_getdents64
#endif

#if defined(__linux__) && defined(HAVE_VFORK) && defined(HAVE_SIGNAL_H) && \
    defined(HAVE_PTHREAD_SIGMASK) && !defined(HAVE_BROKEN_PTHREAD_SIGMASK)
/* If this is ever expanded to non-Linux platforms, verify what calls are
 * allowed after vfork(). Ex: setsid() may be disallowed on macOS? */
# include <signal.h>
# define VFORK_USABLE 1
#endif

#if defined(__sun) && defined(__SVR4)
/* readdir64 is used to work around Solaris 9 bug 6395699. */
# define readdir readdir64
# define dirent dirent64
# if !defined(HAVE_DIRFD)
/* Some versions of Solaris lack dirfd(). */
#  define dirfd(dirp) ((dirp)->dd_fd)
#  define HAVE_DIRFD
# endif
#endif

#if defined(__FreeBSD__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__DragonFly__)
# define FD_DIR "/dev/fd"
#else
# define FD_DIR "/proc/self/fd"
#endif

#ifdef NGROUPS_MAX
#define MAX_GROUPS NGROUPS_MAX
#else
#define MAX_GROUPS 64
#endif

#define POSIX_CALL(call)   do { if ((call) == -1) goto error; } while (0)

static struct PyModuleDef _posixsubprocessmodule;

/* Convert ASCII to a positive int, no libc call. no overflow. -1 on error. */
static int
_pos_int_from_ascii(const char *name)
{
    int num = 0;
    while (*name >= '0' && *name <= '9') {
        num = num * 10 + (*name - '0');
        ++name;
    }
    if (*name)
        return -1;  /* Non digit found, not a number. */
    return num;
}


#if defined(__FreeBSD__) || defined(__DragonFly__)
/* When /dev/fd isn't mounted it is often a static directory populated
 * with 0 1 2 or entries for 0 .. 63 on FreeBSD, NetBSD, OpenBSD and DragonFlyBSD.
 * NetBSD and OpenBSD have a /proc fs available (though not necessarily
 * mounted) and do not have fdescfs for /dev/fd.  MacOS X has a devfs
 * that properly supports /dev/fd.
 */
static int
_is_fdescfs_mounted_on_dev_fd(void)
{
    struct stat dev_stat;
    struct stat dev_fd_stat;
    if (stat("/dev", &dev_stat) != 0)
        return 0;
    if (stat(FD_DIR, &dev_fd_stat) != 0)
        return 0;
    if (dev_stat.st_dev == dev_fd_stat.st_dev)
        return 0;  /* / == /dev == /dev/fd means it is static. #fail */
    return 1;
}
#endif


/* Returns 1 if there is a problem with fd_sequence, 0 otherwise. */
static int
_sanity_check_python_fd_sequence(PyObject *fd_sequence)
{
    Py_ssize_t seq_idx;
    long prev_fd = -1;
    for (seq_idx = 0; seq_idx < PyTuple_GET_SIZE(fd_sequence); ++seq_idx) {
        PyObject* py_fd = PyTuple_GET_ITEM(fd_sequence, seq_idx);
        long iter_fd;
        if (!PyLong_Check(py_fd)) {
            return 1;
        }
        iter_fd = PyLong_AsLong(py_fd);
        if (iter_fd < 0 || iter_fd <= prev_fd || iter_fd > INT_MAX) {
            /* Negative, overflow, unsorted, too big for a fd. */
            return 1;
        }
        prev_fd = iter_fd;
    }
    return 0;
}


/* Is fd found in the sorted Python Sequence? */
static int
_is_fd_in_sorted_fd_sequence(int fd, int *fd_sequence,
                             Py_ssize_t fd_sequence_len)
{
    /* Binary search. */
    Py_ssize_t search_min = 0;
    Py_ssize_t search_max = fd_sequence_len - 1;
    if (search_max < 0)
        return 0;
    do {
        long middle = (search_min + search_max) / 2;
        long middle_fd = fd_sequence[middle];
        if (fd == middle_fd)
            return 1;
        if (fd > middle_fd)
            search_min = middle + 1;
        else
            search_max = middle - 1;
    } while (search_min <= search_max);
    return 0;
}

/*
 * Do all the Python C API calls in the parent process to turn the pass_fds
 * "py_fds_to_keep" tuple into a C array.  The caller owns allocation and
 * freeing of the array.
 *
 * On error an unknown number of array elements may have been filled in.
 * A Python exception has been set when an error is returned.
 *
 * Returns: -1 on error, 0 on success.
 */
static int
convert_fds_to_keep_to_c(PyObject *py_fds_to_keep, int *c_fds_to_keep)
{
    Py_ssize_t i, len;

    len = PyTuple_GET_SIZE(py_fds_to_keep);
    for (i = 0; i < len; ++i) {
        PyObject* fdobj = PyTuple_GET_ITEM(py_fds_to_keep, i);
        long fd = PyLong_AsLong(fdobj);
        if (fd == -1 && PyErr_Occurred()) {
            return -1;
        }
        if (fd < 0 || fd > INT_MAX) {
            PyErr_SetString(PyExc_ValueError,
                            "fd out of range in fds_to_keep.");
            return -1;
        }
        c_fds_to_keep[i] = (int)fd;
    }
    return 0;
}


/* This function must be async-signal-safe as it is called from child_exec()
 * after fork() or vfork().
 */
static int
make_inheritable(int *c_fds_to_keep, Py_ssize_t len, int errpipe_write)
{
    Py_ssize_t i;

    for (i = 0; i < len; ++i) {
        int fd = c_fds_to_keep[i];
        if (fd == errpipe_write) {
            /* errpipe_write is part of fds_to_keep. It must be closed at
               exec(), but kept open in the child process until exec() is
               called. */
            continue;
        }
        if (_Py_set_inheritable_async_safe(fd, 1, NULL) < 0)
            return -1;
    }
    return 0;
}


/* Get the maximum file descriptor that could be opened by this process.
 * This function is async signal safe for use between fork() and exec().
 */
static long
safe_get_max_fd(void)
{
    long local_max_fd;
#if defined(__NetBSD__)
    local_max_fd = fcntl(0, F_MAXFD);
    if (local_max_fd >= 0)
        return local_max_fd;
#endif
#if defined(HAVE_SYS_RESOURCE_H) && defined(__OpenBSD__)
    struct rlimit rl;
    /* Not on the POSIX async signal safe functions list but likely
     * safe.  TODO - Someone should audit OpenBSD to make sure. */
    if (getrlimit(RLIMIT_NOFILE, &rl) >= 0)
        return (long) rl.rlim_max;
#endif
#ifdef _SC_OPEN_MAX
    local_max_fd = sysconf(_SC_OPEN_MAX);
    if (local_max_fd == -1)
#endif
        local_max_fd = 256;  /* Matches legacy Lib/subprocess.py behavior. */
    return local_max_fd;
}


/* Close all file descriptors in the given range except for those in
 * fds_to_keep by invoking closer on each subrange.
 *
 * If end_fd == -1, it's guessed via safe_get_max_fd(), but it isn't
 * possible to know for sure what the max fd to go up to is for
 * processes with the capability of raising their maximum, or in case
 * a process opened a high fd and then lowered its maximum.
 */
static int
_close_range_except(int start_fd,
                    int end_fd,
                    int *fds_to_keep,
                    Py_ssize_t fds_to_keep_len,
                    int (*closer)(int, int))
{
    if (end_fd == -1) {
        end_fd = Py_MIN(safe_get_max_fd(), INT_MAX);
    }
    Py_ssize_t keep_seq_idx;
    /* As fds_to_keep is sorted we can loop through the list closing
     * fds in between any in the keep list falling within our range. */
    for (keep_seq_idx = 0; keep_seq_idx < fds_to_keep_len; ++keep_seq_idx) {
        int keep_fd = fds_to_keep[keep_seq_idx];
        if (keep_fd < start_fd)
            continue;
        if (closer(start_fd, keep_fd - 1) != 0)
            return -1;
        start_fd = keep_fd + 1;
    }
    if (start_fd <= end_fd) {
        if (closer(start_fd, end_fd) != 0)
            return -1;
    }
    return 0;
}

#if defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)
/* It doesn't matter if d_name has room for NAME_MAX chars; we're using this
 * only to read a directory of short file descriptor number names.  The kernel
 * will return an error if we didn't give it enough space.  Highly Unlikely.
 * This structure is very old and stable: It will not change unless the kernel
 * chooses to break compatibility with all existing binaries.  Highly Unlikely.
 */
struct linux_dirent64 {
   unsigned long long d_ino;
   long long d_off;
   unsigned short d_reclen;     /* Length of this linux_dirent */
   unsigned char  d_type;
   char           d_name[256];  /* Filename (null-terminated) */
};

static int
_brute_force_closer(int first, int last)
{
    for (int i = first; i <= last; i++) {
        /* Ignore errors */
        (void)close(i);
    }
    return 0;
}

/* Close all open file descriptors in the range from start_fd and higher
 * Do not close any in the sorted fds_to_keep list.
 *
 * This version is async signal safe as it does not make any unsafe C library
 * calls, malloc calls or handle any locks.  It is _unfortunate_ to be forced
 * to resort to making a kernel system call directly but this is the ONLY api
 * available that does no harm.  opendir/readdir/closedir perform memory
 * allocation and locking so while they usually work they are not guaranteed
 * to (especially if you have replaced your malloc implementation).  A version
 * of this function that uses those can be found in the _maybe_unsafe variant.
 *
 * This is Linux specific because that is all I am ready to test it on.  It
 * should be easy to add OS specific dirent or dirent64 structures and modify
 * it with some cpp #define magic to work on other OSes as well if you want.
 */
static void
_close_open_fds_safe(int start_fd, int *fds_to_keep, Py_ssize_t fds_to_keep_len)
{
    int fd_dir_fd;

    fd_dir_fd = _Py_open_noraise(FD_DIR, O_RDONLY);
    if (fd_dir_fd == -1) {
        /* No way to get a list of open fds. */
        _close_range_except(start_fd, -1,
                            fds_to_keep, fds_to_keep_len,
                            _brute_force_closer);
        return;
    } else {
        char buffer[sizeof(struct linux_dirent64)];
        int bytes;
        while ((bytes = syscall(SYS_getdents64, fd_dir_fd,
                                (struct linux_dirent64 *)buffer,
                                sizeof(buffer))) > 0) {
            struct linux_dirent64 *entry;
            int offset;
#ifdef _Py_MEMORY_SANITIZER
            __msan_unpoison(buffer, bytes);
#endif
            for (offset = 0; offset < bytes; offset += entry->d_reclen) {
                int fd;
                entry = (struct linux_dirent64 *)(buffer + offset);
                if ((fd = _pos_int_from_ascii(entry->d_name)) < 0)
                    continue;  /* Not a number. */
                if (fd != fd_dir_fd && fd >= start_fd &&
                    !_is_fd_in_sorted_fd_sequence(fd, fds_to_keep,
                                                  fds_to_keep_len)) {
                    close(fd);
                }
            }
        }
        close(fd_dir_fd);
    }
}

#define _close_open_fds_fallback _close_open_fds_safe

#else  /* NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */

static int
_unsafe_closer(int first, int last)
{
    _Py_closerange(first, last);
    return 0;
}

/* Close all open file descriptors from start_fd and higher.
 * Do not close any in the sorted fds_to_keep tuple.
 *
 * This function violates the strict use of async signal safe functions. :(
 * It calls opendir(), readdir() and closedir().  Of these, the one most
 * likely to ever cause a problem is opendir() as it performs an internal
 * malloc().  Practically this should not be a problem.  The Java VM makes the
 * same calls between fork and exec in its own UNIXProcess_md.c implementation.
 *
 * readdir_r() is not used because it provides no benefit.  It is typically
 * implemented as readdir() followed by memcpy().  See also:
 *   http://womble.decadent.org.uk/readdir_r-advisory.html
 */
static void
_close_open_fds_maybe_unsafe(int start_fd, int *fds_to_keep,
                             Py_ssize_t fds_to_keep_len)
{
    DIR *proc_fd_dir;
#ifndef HAVE_DIRFD
    while (_is_fd_in_sorted_fd_sequence(start_fd, fds_to_keep,
                                        fds_to_keep_len)) {
        ++start_fd;
    }
    /* Close our lowest fd before we call opendir so that it is likely to
     * reuse that fd otherwise we might close opendir's file descriptor in
     * our loop.  This trick assumes that fd's are allocated on a lowest
     * available basis. */
    close(start_fd);
    ++start_fd;
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
    if (!_is_fdescfs_mounted_on_dev_fd())
        proc_fd_dir = NULL;
    else
#endif
        proc_fd_dir = opendir(FD_DIR);
    if (!proc_fd_dir) {
        /* No way to get a list of open fds. */
        _close_range_except(start_fd, -1, fds_to_keep, fds_to_keep_len,
                            _unsafe_closer);
    } else {
        struct dirent *dir_entry;
#ifdef HAVE_DIRFD
        int fd_used_by_opendir = dirfd(proc_fd_dir);
#else
        int fd_used_by_opendir = start_fd - 1;
#endif
        errno = 0;
        while ((dir_entry = readdir(proc_fd_dir))) {
            int fd;
            if ((fd = _pos_int_from_ascii(dir_entry->d_name)) < 0)
                continue;  /* Not a number. */
            if (fd != fd_used_by_opendir && fd >= start_fd &&
                !_is_fd_in_sorted_fd_sequence(fd, fds_to_keep,
                                              fds_to_keep_len)) {
                close(fd);
            }
            errno = 0;
        }
        if (errno) {
            /* readdir error, revert behavior. Highly Unlikely. */
            _close_range_except(start_fd, -1, fds_to_keep, fds_to_keep_len,
                                _unsafe_closer);
        }
        closedir(proc_fd_dir);
    }
}

#define _close_open_fds_fallback _close_open_fds_maybe_unsafe

#endif  /* else NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */

/* We can use close_range() library function only if it's known to be
 * async-signal-safe.
 *
 * On Linux, glibc explicitly documents it to be a thin wrapper over
 * the system call, and other C libraries are likely to follow glibc.
 */
#if defined(HAVE_CLOSE_RANGE) && \
    (defined(__linux__) || defined(__FreeBSD__))
#define HAVE_ASYNC_SAFE_CLOSE_RANGE

static int
_close_range_closer(int first, int last)
{
    return close_range(first, last, 0);
}
#endif

static void
_close_open_fds(int start_fd, int *fds_to_keep, Py_ssize_t fds_to_keep_len)
{
#ifdef HAVE_ASYNC_SAFE_CLOSE_RANGE
    if (_close_range_except(
            start_fd, INT_MAX, fds_to_keep, fds_to_keep_len,
            _close_range_closer) == 0) {
        return;
    }
#endif
    _close_open_fds_fallback(start_fd, fds_to_keep, fds_to_keep_len);
}

#ifdef VFORK_USABLE
/* Reset dispositions for all signals to SIG_DFL except for ignored
 * signals. This way we ensure that no signal handlers can run
 * after we unblock signals in a child created by vfork().
 */
static void
reset_signal_handlers(const sigset_t *child_sigmask)
{
    struct sigaction sa_dfl = {.sa_handler = SIG_DFL};
    for (int sig = 1; sig < _NSIG; sig++) {
        /* Dispositions for SIGKILL and SIGSTOP can't be changed. */
        if (sig == SIGKILL || sig == SIGSTOP) {
            continue;
        }

        /* There is no need to reset the disposition of signals that will
         * remain blocked across execve() since the kernel will do it. */
        if (sigismember(child_sigmask, sig) == 1) {
            continue;
        }

        struct sigaction sa;
        /* C libraries usually return EINVAL for signals used
         * internally (e.g. for thread cancellation), so simply
         * skip errors here. */
        if (sigaction(sig, NULL, &sa) == -1) {
            continue;
        }

        /* void *h works as these fields are both pointer types already. */
        void *h = (sa.sa_flags & SA_SIGINFO ? (void *)sa.sa_sigaction :
                                              (void *)sa.sa_handler);
        if (h == SIG_IGN || h == SIG_DFL) {
            continue;
        }

        /* This call can't reasonably fail, but if it does, terminating
         * the child seems to be too harsh, so ignore errors. */
        (void) sigaction(sig, &sa_dfl, NULL);
    }
}
#endif /* VFORK_USABLE */


/*
 * This function is code executed in the child process immediately after
 * (v)fork to set things up and call exec().
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 *
 * This restriction is documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/fork.html.
 *
 * If this function is called after vfork(), even more care must be taken.
 * The lack of preparations that C libraries normally take on fork(),
 * as well as sharing the address space with the parent, might make even
 * async-signal-safe functions vfork-unsafe. In particular, on Linux,
 * set*id() and setgroups() library functions must not be called, since
 * they have to interact with the library-level thread list and send
 * library-internal signals to implement per-process credentials semantics
 * required by POSIX but not supported natively on Linux. Another reason to
 * avoid this family of functions is that sharing an address space between
 * processes running with different privileges is inherently insecure.
 * See bpo-35823 for further discussion and references.
 *
 * In some C libraries, setrlimit() has the same thread list/signalling
 * behavior since resource limits were per-thread attributes before
 * Linux 2.6.10. Musl, as of 1.2.1, is known to have this issue
 * (https://www.openwall.com/lists/musl/2020/10/15/6).
 *
 * If vfork-unsafe functionality is desired after vfork(), consider using
 * syscall() to obtain it.
 */
Py_NO_INLINE static void
child_exec(char *const exec_array[],
           char *const argv[],
           char *const envp[],
           const char *cwd,
           int p2cread, int p2cwrite,
           int c2pread, int c2pwrite,
           int errread, int errwrite,
           int errpipe_read, int errpipe_write,
           int close_fds, int restore_signals,
           int call_setsid, pid_t pgid_to_set,
           int call_setgid, gid_t gid,
           int call_setgroups, size_t groups_size, const gid_t *groups,
           int call_setuid, uid_t uid, int child_umask,
           const void *child_sigmask,
           int *fds_to_keep, Py_ssize_t fds_to_keep_len,
           PyObject *preexec_fn,
           PyObject *preexec_fn_args_tuple)
{
    int i, saved_errno, reached_preexec = 0;
    PyObject *result;
    const char* err_msg = "";
    /* Buffer large enough to hold a hex integer.  We can't malloc. */
    char hex_errno[sizeof(saved_errno)*2+1];

    if (make_inheritable(fds_to_keep, fds_to_keep_len, errpipe_write) < 0)
        goto error;

    /* Close parent's pipe ends. */
    if (p2cwrite != -1)
        POSIX_CALL(close(p2cwrite));
    if (c2pread != -1)
        POSIX_CALL(close(c2pread));
    if (errread != -1)
        POSIX_CALL(close(errread));
    POSIX_CALL(close(errpipe_read));

    /* When duping fds, if there arises a situation where one of the fds is
       either 0, 1 or 2, it is possible that it is overwritten (#12607). */
    if (c2pwrite == 0) {
        POSIX_CALL(c2pwrite = dup(c2pwrite));
        /* issue32270 */
        if (_Py_set_inheritable_async_safe(c2pwrite, 0, NULL) < 0) {
            goto error;
        }
    }
    while (errwrite == 0 || errwrite == 1) {
        POSIX_CALL(errwrite = dup(errwrite));
        /* issue32270 */
        if (_Py_set_inheritable_async_safe(errwrite, 0, NULL) < 0) {
            goto error;
        }
    }

    /* Dup fds for child.
       dup2() removes the CLOEXEC flag but we must do it ourselves if dup2()
       would be a no-op (issue #10806). */
    if (p2cread == 0) {
        if (_Py_set_inheritable_async_safe(p2cread, 1, NULL) < 0)
            goto error;
    }
    else if (p2cread != -1)
        POSIX_CALL(dup2(p2cread, 0));  /* stdin */

    if (c2pwrite == 1) {
        if (_Py_set_inheritable_async_safe(c2pwrite, 1, NULL) < 0)
            goto error;
    }
    else if (c2pwrite != -1)
        POSIX_CALL(dup2(c2pwrite, 1));  /* stdout */

    if (errwrite == 2) {
        if (_Py_set_inheritable_async_safe(errwrite, 1, NULL) < 0)
            goto error;
    }
    else if (errwrite != -1)
        POSIX_CALL(dup2(errwrite, 2));  /* stderr */

    /* We no longer manually close p2cread, c2pwrite, and errwrite here as
     * _close_open_fds takes care when it is not already non-inheritable. */

    if (cwd)
        POSIX_CALL(chdir(cwd));

    if (child_umask >= 0)
        umask(child_umask);  /* umask() always succeeds. */

    if (restore_signals)
        _Py_RestoreSignals();

#ifdef VFORK_USABLE
    if (child_sigmask) {
        reset_signal_handlers(child_sigmask);
        if ((errno = pthread_sigmask(SIG_SETMASK, child_sigmask, NULL))) {
            goto error;
        }
    }
#endif

#ifdef HAVE_SETSID
    if (call_setsid)
        POSIX_CALL(setsid());
#endif

#ifdef HAVE_SETPGID
    if (pgid_to_set >= 0)
        POSIX_CALL(setpgid(0, pgid_to_set));
#endif

#ifdef HAVE_SETGROUPS
    if (call_setgroups)
        POSIX_CALL(setgroups(groups_size, groups));
#endif /* HAVE_SETGROUPS */

#ifdef HAVE_SETREGID
    if (call_setgid)
        POSIX_CALL(setregid(gid, gid));
#endif /* HAVE_SETREGID */

#ifdef HAVE_SETREUID
    if (call_setuid)
        POSIX_CALL(setreuid(uid, uid));
#endif /* HAVE_SETREUID */


    reached_preexec = 1;
    if (preexec_fn != Py_None && preexec_fn_args_tuple) {
        /* This is where the user has asked us to deadlock their program. */
        result = PyObject_Call(preexec_fn, preexec_fn_args_tuple, NULL);
        if (result == NULL) {
            /* Stringifying the exception or traceback would involve
             * memory allocation and thus potential for deadlock.
             * We've already faced potential deadlock by calling back
             * into Python in the first place, so it probably doesn't
             * matter but we avoid it to minimize the possibility. */
            err_msg = "Exception occurred in preexec_fn.";
            errno = 0;  /* We don't want to report an OSError. */
            goto error;
        }
        /* Py_DECREF(result); - We're about to exec so why bother? */
    }

    /* close FDs after executing preexec_fn, which might open FDs */
    if (close_fds) {
        /* TODO HP-UX could use pstat_getproc() if anyone cares about it. */
        _close_open_fds(3, fds_to_keep, fds_to_keep_len);
    }

    /* This loop matches the Lib/os.py _execvpe()'s PATH search when */
    /* given the executable_list generated by Lib/subprocess.py.     */
    saved_errno = 0;
    for (i = 0; exec_array[i] != NULL; ++i) {
        const char *executable = exec_array[i];
        if (envp) {
            execve(executable, argv, envp);
        } else {
            execv(executable, argv);
        }
        if (errno != ENOENT && errno != ENOTDIR && saved_errno == 0) {
            saved_errno = errno;
        }
    }
    /* Report the first exec error, not the last. */
    if (saved_errno)
        errno = saved_errno;

error:
    saved_errno = errno;
    /* Report the posix error to our parent process. */
    /* We ignore all write() return values as the total size of our writes is
       less than PIPEBUF and we cannot do anything about an error anyways.
       Use _Py_write_noraise() to retry write() if it is interrupted by a
       signal (fails with EINTR). */
    if (saved_errno) {
        char *cur;
        _Py_write_noraise(errpipe_write, "OSError:", 8);
        cur = hex_errno + sizeof(hex_errno);
        while (saved_errno != 0 && cur != hex_errno) {
            *--cur = Py_hexdigits[saved_errno % 16];
            saved_errno /= 16;
        }
        _Py_write_noraise(errpipe_write, cur, hex_errno + sizeof(hex_errno) - cur);
        _Py_write_noraise(errpipe_write, ":", 1);
        if (!reached_preexec) {
            /* Indicate to the parent that the error happened before exec(). */
            _Py_write_noraise(errpipe_write, "noexec", 6);
        }
        /* We can't call strerror(saved_errno).  It is not async signal safe.
         * The parent process will look the error message up. */
    } else {
        _Py_write_noraise(errpipe_write, "SubprocessError:0:", 18);
        _Py_write_noraise(errpipe_write, err_msg, strlen(err_msg));
    }
}


/* The main purpose of this wrapper function is to isolate vfork() from both
 * subprocess_fork_exec() and child_exec(). A child process created via
 * vfork() executes on the same stack as the parent process while the latter is
 * suspended, so this function should not be inlined to avoid compiler bugs
 * that might clobber data needed by the parent later. Additionally,
 * child_exec() should not be inlined to avoid spurious -Wclobber warnings from
 * GCC (see bpo-35823).
 */
Py_NO_INLINE static pid_t
do_fork_exec(char *const exec_array[],
             char *const argv[],
             char *const envp[],
             const char *cwd,
             int p2cread, int p2cwrite,
             int c2pread, int c2pwrite,
             int errread, int errwrite,
             int errpipe_read, int errpipe_write,
             int close_fds, int restore_signals,
             int call_setsid, pid_t pgid_to_set,
             int call_setgid, gid_t gid,
             int call_setgroups, size_t groups_size, const gid_t *groups,
             int call_setuid, uid_t uid, int child_umask,
             const void *child_sigmask,
             int *fds_to_keep, Py_ssize_t fds_to_keep_len,
             PyObject *preexec_fn,
             PyObject *preexec_fn_args_tuple)
{

    pid_t pid;

#ifdef VFORK_USABLE
    if (child_sigmask) {
        /* These are checked by our caller; verify them in debug builds. */
        assert(!call_setuid);
        assert(!call_setgid);
        assert(!call_setgroups);
        assert(preexec_fn == Py_None);

        pid = vfork();
        if (pid == -1) {
            /* If vfork() fails, fall back to using fork(). When it isn't
             * allowed in a process by the kernel, vfork can return -1
             * with errno EINVAL. https://bugs.python.org/issue47151. */
            pid = fork();
        }
    } else
#endif
    {
        pid = fork();
    }

    if (pid != 0) {
        return pid;
    }

    /* Child process.
     * See the comment above child_exec() for restrictions imposed on
     * the code below.
     */

    if (preexec_fn != Py_None) {
        /* We'll be calling back into Python later so we need to do this.
         * This call may not be async-signal-safe but neither is calling
         * back into Python.  The user asked us to use hope as a strategy
         * to avoid deadlock... */
        PyOS_AfterFork_Child();
    }

    child_exec(exec_array, argv, envp, cwd,
               p2cread, p2cwrite, c2pread, c2pwrite,
               errread, errwrite, errpipe_read, errpipe_write,
               close_fds, restore_signals, call_setsid, pgid_to_set,
               call_setgid, gid, call_setgroups, groups_size, groups,
               call_setuid, uid, child_umask, child_sigmask,
               fds_to_keep, fds_to_keep_len,
               preexec_fn, preexec_fn_args_tuple);
    _exit(255);
    return 0;  /* Dead code to avoid a potential compiler warning. */
}


static PyObject *
subprocess_fork_exec(PyObject *module, PyObject *args)
{
    PyObject *gc_module = NULL;
    PyObject *executable_list, *py_fds_to_keep;
    PyObject *env_list, *preexec_fn;
    PyObject *process_args, *converted_args = NULL, *fast_args = NULL;
    PyObject *preexec_fn_args_tuple = NULL;
    PyObject *groups_list;
    PyObject *uid_object, *gid_object;
    int p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite;
    int errpipe_read, errpipe_write, close_fds, restore_signals;
    int call_setsid;
    pid_t pgid_to_set = -1;
    int call_setgid = 0, call_setgroups = 0, call_setuid = 0;
    uid_t uid;
    gid_t gid, *groups = NULL;
    int child_umask;
    PyObject *cwd_obj, *cwd_obj2 = NULL;
    const char *cwd;
    pid_t pid = -1;
    int need_to_reenable_gc = 0;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;
    Py_ssize_t arg_num, num_groups = 0;
    int need_after_fork = 0;
    int saved_errno = 0;
    int allow_vfork;
    int *c_fds_to_keep = NULL;

    if (!PyArg_ParseTuple(
            args, "OOpO!OOiiiiiiiiii" _Py_PARSE_PID "OOOiOp:fork_exec",
            &process_args, &executable_list,
            &close_fds, &PyTuple_Type, &py_fds_to_keep,
            &cwd_obj, &env_list,
            &p2cread, &p2cwrite, &c2pread, &c2pwrite,
            &errread, &errwrite, &errpipe_read, &errpipe_write,
            &restore_signals, &call_setsid, &pgid_to_set,
            &gid_object, &groups_list, &uid_object, &child_umask,
            &preexec_fn, &allow_vfork))
        return NULL;

    if ((preexec_fn != Py_None) &&
            (PyInterpreterState_Get() != PyInterpreterState_Main())) {
        PyErr_SetString(PyExc_RuntimeError,
                        "preexec_fn not supported within subinterpreters");
        return NULL;
    }

    if (close_fds && errpipe_write < 3) {  /* precondition */
        PyErr_SetString(PyExc_ValueError, "errpipe_write must be >= 3");
        return NULL;
    }
    if (_sanity_check_python_fd_sequence(py_fds_to_keep)) {
        PyErr_SetString(PyExc_ValueError, "bad value(s) in fds_to_keep");
        return NULL;
    }

    PyInterpreterState *interp = PyInterpreterState_Get();
    const PyConfig *config = _PyInterpreterState_GetConfig(interp);
    if (config->_isolated_interpreter) {
        PyErr_SetString(PyExc_RuntimeError,
                        "subprocess not supported for isolated subinterpreters");
        return NULL;
    }

    /* We need to call gc.disable() when we'll be calling preexec_fn */
    if (preexec_fn != Py_None) {
        need_to_reenable_gc = PyGC_Disable();
    }

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array)
        goto cleanup;

    /* Convert args and env into appropriate arguments for exec() */
    /* These conversions are done in the parent process to avoid allocating
       or freeing memory in the child process. */
    if (process_args != Py_None) {
        Py_ssize_t num_args;
        /* Equivalent to:  */
        /*  tuple(PyUnicode_FSConverter(arg) for arg in process_args)  */
        fast_args = PySequence_Fast(process_args, "argv must be a tuple");
        if (fast_args == NULL)
            goto cleanup;
        num_args = PySequence_Fast_GET_SIZE(fast_args);
        converted_args = PyTuple_New(num_args);
        if (converted_args == NULL)
            goto cleanup;
        for (arg_num = 0; arg_num < num_args; ++arg_num) {
            PyObject *borrowed_arg, *converted_arg;
            if (PySequence_Fast_GET_SIZE(fast_args) != num_args) {
                PyErr_SetString(PyExc_RuntimeError, "args changed during iteration");
                goto cleanup;
            }
            borrowed_arg = PySequence_Fast_GET_ITEM(fast_args, arg_num);
            if (PyUnicode_FSConverter(borrowed_arg, &converted_arg) == 0)
                goto cleanup;
            PyTuple_SET_ITEM(converted_args, arg_num, converted_arg);
        }

        argv = _PySequence_BytesToCharpArray(converted_args);
        Py_CLEAR(converted_args);
        Py_CLEAR(fast_args);
        if (!argv)
            goto cleanup;
    }

    if (env_list != Py_None) {
        envp = _PySequence_BytesToCharpArray(env_list);
        if (!envp)
            goto cleanup;
    }

    if (cwd_obj != Py_None) {
        if (PyUnicode_FSConverter(cwd_obj, &cwd_obj2) == 0)
            goto cleanup;
        cwd = PyBytes_AsString(cwd_obj2);
    } else {
        cwd = NULL;
    }

    if (groups_list != Py_None) {
#ifdef HAVE_SETGROUPS
        Py_ssize_t i;
        gid_t gid;

        if (!PyList_Check(groups_list)) {
            PyErr_SetString(PyExc_TypeError,
                    "setgroups argument must be a list");
            goto cleanup;
        }
        num_groups = PySequence_Size(groups_list);

        if (num_groups < 0)
            goto cleanup;

        if (num_groups > MAX_GROUPS) {
            PyErr_SetString(PyExc_ValueError, "too many groups");
            goto cleanup;
        }

        if ((groups = PyMem_RawMalloc(num_groups * sizeof(gid_t))) == NULL) {
            PyErr_SetString(PyExc_MemoryError,
                    "failed to allocate memory for group list");
            goto cleanup;
        }

        for (i = 0; i < num_groups; i++) {
            PyObject *elem;
            elem = PySequence_GetItem(groups_list, i);
            if (!elem)
                goto cleanup;
            if (!PyLong_Check(elem)) {
                PyErr_SetString(PyExc_TypeError,
                                "groups must be integers");
                Py_DECREF(elem);
                goto cleanup;
            } else {
                if (!_Py_Gid_Converter(elem, &gid)) {
                    Py_DECREF(elem);
                    PyErr_SetString(PyExc_ValueError, "invalid group id");
                    goto cleanup;
                }
                groups[i] = gid;
            }
            Py_DECREF(elem);
        }
        call_setgroups = 1;

#else /* HAVE_SETGROUPS */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETGROUPS */
    }

    if (gid_object != Py_None) {
#ifdef HAVE_SETREGID
        if (!_Py_Gid_Converter(gid_object, &gid))
            goto cleanup;

        call_setgid = 1;

#else /* HAVE_SETREGID */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETREUID */
    }

    if (uid_object != Py_None) {
#ifdef HAVE_SETREUID
        if (!_Py_Uid_Converter(uid_object, &uid))
            goto cleanup;

        call_setuid = 1;

#else /* HAVE_SETREUID */
        PyErr_BadInternalCall();
        goto cleanup;
#endif /* HAVE_SETREUID */
    }

    Py_ssize_t fds_to_keep_len = PyTuple_GET_SIZE(py_fds_to_keep);
    c_fds_to_keep = PyMem_Malloc(fds_to_keep_len * sizeof(int));
    if (c_fds_to_keep == NULL) {
        PyErr_SetString(PyExc_MemoryError, "failed to malloc c_fds_to_keep");
        goto cleanup;
    }
    if (convert_fds_to_keep_to_c(py_fds_to_keep, c_fds_to_keep) < 0) {
        goto cleanup;
    }

    /* This must be the last thing done before fork() because we do not
     * want to call PyOS_BeforeFork() if there is any chance of another
     * error leading to the cleanup: code without calling fork(). */
    if (preexec_fn != Py_None) {
        preexec_fn_args_tuple = PyTuple_New(0);
        if (!preexec_fn_args_tuple)
            goto cleanup;
        PyOS_BeforeFork();
        need_after_fork = 1;
    }

    /* NOTE: When old_sigmask is non-NULL, do_fork_exec() may use vfork(). */
    const void *old_sigmask = NULL;
#ifdef VFORK_USABLE
    /* Use vfork() only if it's safe. See the comment above child_exec(). */
    sigset_t old_sigs;
    if (preexec_fn == Py_None && allow_vfork &&
        !call_setuid && !call_setgid && !call_setgroups) {
        /* Block all signals to ensure that no signal handlers are run in the
         * child process while it shares memory with us. Note that signals
         * used internally by C libraries won't be blocked by
         * pthread_sigmask(), but signal handlers installed by C libraries
         * normally service only signals originating from *within the process*,
         * so it should be sufficient to consider any library function that
         * might send such a signal to be vfork-unsafe and do not call it in
         * the child.
         */
        sigset_t all_sigs;
        sigfillset(&all_sigs);
        if ((saved_errno = pthread_sigmask(SIG_BLOCK, &all_sigs, &old_sigs))) {
            goto cleanup;
        }
        old_sigmask = &old_sigs;
    }
#endif

    pid = do_fork_exec(exec_array, argv, envp, cwd,
                       p2cread, p2cwrite, c2pread, c2pwrite,
                       errread, errwrite, errpipe_read, errpipe_write,
                       close_fds, restore_signals, call_setsid, pgid_to_set,
                       call_setgid, gid, call_setgroups, num_groups, groups,
                       call_setuid, uid, child_umask, old_sigmask,
                       c_fds_to_keep, fds_to_keep_len,
                       preexec_fn, preexec_fn_args_tuple);

    /* Parent (original) process */
    if (pid == -1) {
        /* Capture errno for the exception. */
        saved_errno = errno;
    }

#ifdef VFORK_USABLE
    if (old_sigmask) {
        /* vfork() semantics guarantees that the parent is blocked
         * until the child performs _exit() or execve(), so it is safe
         * to unblock signals once we're here.
         * Note that in environments where vfork() is implemented as fork(),
         * such as QEMU user-mode emulation, the parent won't be blocked,
         * but it won't share the address space with the child,
         * so it's still safe to unblock the signals.
         *
         * We don't handle errors here because this call can't fail
         * if valid arguments are given, and because there is no good
         * way for the caller to deal with a failure to restore
         * the thread signal mask. */
        (void) pthread_sigmask(SIG_SETMASK, old_sigmask, NULL);
    }
#endif

    if (need_after_fork)
        PyOS_AfterFork_Parent();

cleanup:
    if (c_fds_to_keep != NULL) {
        PyMem_Free(c_fds_to_keep);
    }

    if (saved_errno != 0) {
        errno = saved_errno;
        /* We can't call this above as PyOS_AfterFork_Parent() calls back
         * into Python code which would see the unreturned error. */
        PyErr_SetFromErrno(PyExc_OSError);
    }

    Py_XDECREF(preexec_fn_args_tuple);
    PyMem_RawFree(groups);
    Py_XDECREF(cwd_obj2);
    if (envp)
        _Py_FreeCharPArray(envp);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    if (argv)
        _Py_FreeCharPArray(argv);
    if (exec_array)
        _Py_FreeCharPArray(exec_array);

    if (need_to_reenable_gc) {
        PyGC_Enable();
    }
    Py_XDECREF(gc_module);

    return pid == -1 ? NULL : PyLong_FromPid(pid);
}


PyDoc_STRVAR(subprocess_fork_exec_doc,
"fork_exec(args, executable_list, close_fds, pass_fds, cwd, env,\n\
          p2cread, p2cwrite, c2pread, c2pwrite,\n\
          errread, errwrite, errpipe_read, errpipe_write,\n\
          restore_signals, call_setsid, pgid_to_set,\n\
          gid, groups_list, uid,\n\
          preexec_fn)\n\
\n\
Forks a child process, closes parent file descriptors as appropriate in the\n\
child and dups the few that are needed before calling exec() in the child\n\
process.\n\
\n\
If close_fds is true, close file descriptors 3 and higher, except those listed\n\
in the sorted tuple pass_fds.\n\
\n\
The preexec_fn, if supplied, will be called immediately before closing file\n\
descriptors and exec.\n\
WARNING: preexec_fn is NOT SAFE if your application uses threads.\n\
         It may trigger infrequent, difficult to debug deadlocks.\n\
\n\
If an error occurs in the child process before the exec, it is\n\
serialized and written to the errpipe_write fd per subprocess.py.\n\
\n\
Returns: the child process's PID.\n\
\n\
Raises: Only on an error in the parent process.\n\
");

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"A POSIX helper for the subprocess module.");

static PyMethodDef module_methods[] = {
    {"fork_exec", subprocess_fork_exec, METH_VARARGS, subprocess_fork_exec_doc},
    {NULL, NULL}  /* sentinel */
};

static PyModuleDef_Slot _posixsubprocess_slots[] = {
    {0, NULL}
};

static struct PyModuleDef _posixsubprocessmodule = {
        PyModuleDef_HEAD_INIT,
        .m_name = "_posixsubprocess",
        .m_doc = module_doc,
        .m_size = 0,
        .m_methods = module_methods,
        .m_slots = _posixsubprocess_slots,
};

PyMODINIT_FUNC
PyInit__posixsubprocess(void)
{
    return PyModuleDef_Init(&_posixsubprocessmodule);
}
