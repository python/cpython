/* Authors: Gregory P. Smith & Jeffrey Yasskin */
#include "Python.h"
#if defined(HAVE_PIPE2) && !defined(_GNU_SOURCE)
# define _GNU_SOURCE
#endif
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H) && defined(__FreeBSD__)
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#if defined(sun)
/* readdir64 is used to work around Solaris 9 bug 6395699. */
# define readdir readdir64
# define dirent dirent64
# if !defined(HAVE_DIRFD)
/* Some versions of Solaris lack dirfd(). */
#  define dirfd(dirp) ((dirp)->dd_fd)
#  define HAVE_DIRFD
# endif
#endif

#if defined(__FreeBSD__) || (defined(__APPLE__) && defined(__MACH__))
# define FD_DIR "/dev/fd"
#else
# define FD_DIR "/proc/self/fd"
#endif

#define POSIX_CALL(call)   if ((call) == -1) goto error


/* Maximum file descriptor, initialized on module load. */
static long max_fd;


/* Given the gc module call gc.enable() and return 0 on success. */
static int
_enable_gc(PyObject *gc_module)
{
    PyObject *result;
    _Py_IDENTIFIER(enable);

    result = _PyObject_CallMethodId(gc_module, &PyId_enable, NULL);
    if (result == NULL)
        return 1;
    Py_DECREF(result);
    return 0;
}


/* Convert ASCII to a positive int, no libc call. no overflow. -1 on error. */
static int
_pos_int_from_ascii(char *name)
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


#if defined(__FreeBSD__)
/* When /dev/fd isn't mounted it is often a static directory populated
 * with 0 1 2 or entries for 0 .. 63 on FreeBSD, NetBSD and OpenBSD.
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
    Py_ssize_t seq_idx, seq_len = PySequence_Length(fd_sequence);
    long prev_fd = -1;
    for (seq_idx = 0; seq_idx < seq_len; ++seq_idx) {
        PyObject* py_fd = PySequence_Fast_GET_ITEM(fd_sequence, seq_idx);
        long iter_fd = PyLong_AsLong(py_fd);
        if (iter_fd < 0 || iter_fd < prev_fd || iter_fd > INT_MAX) {
            /* Negative, overflow, not a Long, unsorted, too big for a fd. */
            return 1;
        }
    }
    return 0;
}


/* Is fd found in the sorted Python Sequence? */
static int
_is_fd_in_sorted_fd_sequence(int fd, PyObject *fd_sequence)
{
    /* Binary search. */
    Py_ssize_t search_min = 0;
    Py_ssize_t search_max = PySequence_Length(fd_sequence) - 1;
    if (search_max < 0)
        return 0;
    do {
        long middle = (search_min + search_max) / 2;
        long middle_fd = PyLong_AsLong(
                PySequence_Fast_GET_ITEM(fd_sequence, middle));
        if (fd == middle_fd)
            return 1;
        if (fd > middle_fd)
            search_min = middle + 1;
        else
            search_max = middle - 1;
    } while (search_min <= search_max);
    return 0;
}


/* Close all file descriptors in the range start_fd inclusive to
 * end_fd exclusive except for those in py_fds_to_keep.  If the
 * range defined by [start_fd, end_fd) is large this will take a
 * long time as it calls close() on EVERY possible fd.
 */
static void
_close_fds_by_brute_force(int start_fd, int end_fd, PyObject *py_fds_to_keep)
{
    Py_ssize_t num_fds_to_keep = PySequence_Length(py_fds_to_keep);
    Py_ssize_t keep_seq_idx;
    int fd_num;
    /* As py_fds_to_keep is sorted we can loop through the list closing
     * fds inbetween any in the keep list falling within our range. */
    for (keep_seq_idx = 0; keep_seq_idx < num_fds_to_keep; ++keep_seq_idx) {
        PyObject* py_keep_fd = PySequence_Fast_GET_ITEM(py_fds_to_keep,
                                                        keep_seq_idx);
        int keep_fd = PyLong_AsLong(py_keep_fd);
        if (keep_fd < start_fd)
            continue;
        for (fd_num = start_fd; fd_num < keep_fd; ++fd_num) {
            while (close(fd_num) < 0 && errno == EINTR);
        }
        start_fd = keep_fd + 1;
    }
    if (start_fd <= end_fd) {
        for (fd_num = start_fd; fd_num < end_fd; ++fd_num) {
            while (close(fd_num) < 0 && errno == EINTR);
        }
    }
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

/* Close all open file descriptors in the range start_fd inclusive to end_fd
 * exclusive. Do not close any in the sorted py_fds_to_keep list.
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
_close_open_fd_range_safe(int start_fd, int end_fd, PyObject* py_fds_to_keep)
{
    int fd_dir_fd;
    if (start_fd >= end_fd)
        return;
#ifdef O_CLOEXEC
    fd_dir_fd = open(FD_DIR, O_RDONLY | O_CLOEXEC, 0);
#else
    fd_dir_fd = open(FD_DIR, O_RDONLY, 0);
#ifdef FD_CLOEXEC
    {
        int old = fcntl(fd_dir_fd, F_GETFD);
        if (old != -1)
            fcntl(fd_dir_fd, F_SETFD, old | FD_CLOEXEC);
    }
#endif
#endif
    if (fd_dir_fd == -1) {
        /* No way to get a list of open fds. */
        _close_fds_by_brute_force(start_fd, end_fd, py_fds_to_keep);
        return;
    } else {
        char buffer[sizeof(struct linux_dirent64)];
        int bytes;
        while ((bytes = syscall(SYS_getdents64, fd_dir_fd,
                                (struct linux_dirent64 *)buffer,
                                sizeof(buffer))) > 0) {
            struct linux_dirent64 *entry;
            int offset;
            for (offset = 0; offset < bytes; offset += entry->d_reclen) {
                int fd;
                entry = (struct linux_dirent64 *)(buffer + offset);
                if ((fd = _pos_int_from_ascii(entry->d_name)) < 0)
                    continue;  /* Not a number. */
                if (fd != fd_dir_fd && fd >= start_fd && fd < end_fd &&
                    !_is_fd_in_sorted_fd_sequence(fd, py_fds_to_keep)) {
                    while (close(fd) < 0 && errno == EINTR);
                }
            }
        }
        close(fd_dir_fd);
    }
}

#define _close_open_fd_range _close_open_fd_range_safe

#else  /* NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */


/* Close all open file descriptors in the range start_fd inclusive to end_fd
 * exclusive. Do not close any in the sorted py_fds_to_keep list.
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
_close_open_fd_range_maybe_unsafe(int start_fd, int end_fd,
                                  PyObject* py_fds_to_keep)
{
    DIR *proc_fd_dir;
#ifndef HAVE_DIRFD
    while (_is_fd_in_sorted_fd_sequence(start_fd, py_fds_to_keep) &&
           (start_fd < end_fd)) {
        ++start_fd;
    }
    if (start_fd >= end_fd)
        return;
    /* Close our lowest fd before we call opendir so that it is likely to
     * reuse that fd otherwise we might close opendir's file descriptor in
     * our loop.  This trick assumes that fd's are allocated on a lowest
     * available basis. */
    while (close(start_fd) < 0 && errno == EINTR);
    ++start_fd;
#endif
    if (start_fd >= end_fd)
        return;

#if defined(__FreeBSD__)
    if (!_is_fdescfs_mounted_on_dev_fd())
        proc_fd_dir = NULL;
    else
#endif
        proc_fd_dir = opendir(FD_DIR);
    if (!proc_fd_dir) {
        /* No way to get a list of open fds. */
        _close_fds_by_brute_force(start_fd, end_fd, py_fds_to_keep);
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
            if (fd != fd_used_by_opendir && fd >= start_fd && fd < end_fd &&
                !_is_fd_in_sorted_fd_sequence(fd, py_fds_to_keep)) {
                while (close(fd) < 0 && errno == EINTR);
            }
            errno = 0;
        }
        if (errno) {
            /* readdir error, revert behavior. Highly Unlikely. */
            _close_fds_by_brute_force(start_fd, end_fd, py_fds_to_keep);
        }
        closedir(proc_fd_dir);
    }
}

#define _close_open_fd_range _close_open_fd_range_maybe_unsafe

#endif  /* else NOT (defined(__linux__) && defined(HAVE_SYS_SYSCALL_H)) */


/*
 * This function is code executed in the child process immediately after fork
 * to set things up and call exec().
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 *
 * This restriction is documented at
 * http://www.opengroup.org/onlinepubs/009695399/functions/fork.html.
 */
static void
child_exec(char *const exec_array[],
           char *const argv[],
           char *const envp[],
           const char *cwd,
           int p2cread, int p2cwrite,
           int c2pread, int c2pwrite,
           int errread, int errwrite,
           int errpipe_read, int errpipe_write,
           int close_fds, int restore_signals,
           int call_setsid,
           PyObject *py_fds_to_keep,
           PyObject *preexec_fn,
           PyObject *preexec_fn_args_tuple)
{
    int i, saved_errno, unused, reached_preexec = 0;
    PyObject *result;
    const char* err_msg = "";
    /* Buffer large enough to hold a hex integer.  We can't malloc. */
    char hex_errno[sizeof(saved_errno)*2+1];

    /* Close parent's pipe ends. */
    if (p2cwrite != -1) {
        POSIX_CALL(close(p2cwrite));
    }
    if (c2pread != -1) {
        POSIX_CALL(close(c2pread));
    }
    if (errread != -1) {
        POSIX_CALL(close(errread));
    }
    POSIX_CALL(close(errpipe_read));

    /* When duping fds, if there arises a situation where one of the fds is
       either 0, 1 or 2, it is possible that it is overwritten (#12607). */
    if (c2pwrite == 0)
        POSIX_CALL(c2pwrite = dup(c2pwrite));
    if (errwrite == 0 || errwrite == 1)
        POSIX_CALL(errwrite = dup(errwrite));

    /* Dup fds for child.
       dup2() removes the CLOEXEC flag but we must do it ourselves if dup2()
       would be a no-op (issue #10806). */
    if (p2cread == 0) {
        int old = fcntl(p2cread, F_GETFD);
        if (old != -1)
            fcntl(p2cread, F_SETFD, old & ~FD_CLOEXEC);
    } else if (p2cread != -1) {
        POSIX_CALL(dup2(p2cread, 0));  /* stdin */
    }
    if (c2pwrite == 1) {
        int old = fcntl(c2pwrite, F_GETFD);
        if (old != -1)
            fcntl(c2pwrite, F_SETFD, old & ~FD_CLOEXEC);
    } else if (c2pwrite != -1) {
        POSIX_CALL(dup2(c2pwrite, 1));  /* stdout */
    }
    if (errwrite == 2) {
        int old = fcntl(errwrite, F_GETFD);
        if (old != -1)
            fcntl(errwrite, F_SETFD, old & ~FD_CLOEXEC);
    } else if (errwrite != -1) {
        POSIX_CALL(dup2(errwrite, 2));  /* stderr */
    }

    /* Close pipe fds.  Make sure we don't close the same fd more than */
    /* once, or standard fds. */
    if (p2cread > 2) {
        POSIX_CALL(close(p2cread));
    }
    if (c2pwrite > 2 && c2pwrite != p2cread) {
        POSIX_CALL(close(c2pwrite));
    }
    if (errwrite != c2pwrite && errwrite != p2cread && errwrite > 2) {
        POSIX_CALL(close(errwrite));
    }

    if (close_fds) {
        int local_max_fd = max_fd;
#if defined(__NetBSD__)
        local_max_fd = fcntl(0, F_MAXFD);
        if (local_max_fd < 0)
            local_max_fd = max_fd;
#endif
        /* TODO HP-UX could use pstat_getproc() if anyone cares about it. */
        _close_open_fd_range(3, local_max_fd, py_fds_to_keep);
    }

    if (cwd)
        POSIX_CALL(chdir(cwd));

    if (restore_signals)
        _Py_RestoreSignals();

#ifdef HAVE_SETSID
    if (call_setsid)
        POSIX_CALL(setsid());
#endif

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
     * less than PIPEBUF and we cannot do anything about an error anyways. */
    if (saved_errno) {
        char *cur;
        unused = write(errpipe_write, "OSError:", 8);
        cur = hex_errno + sizeof(hex_errno);
        while (saved_errno != 0 && cur > hex_errno) {
            *--cur = "0123456789ABCDEF"[saved_errno % 16];
            saved_errno /= 16;
        }
        unused = write(errpipe_write, cur, hex_errno + sizeof(hex_errno) - cur);
        unused = write(errpipe_write, ":", 1);
        if (!reached_preexec) {
            /* Indicate to the parent that the error happened before exec(). */
            unused = write(errpipe_write, "noexec", 6);
        }
        /* We can't call strerror(saved_errno).  It is not async signal safe.
         * The parent process will look the error message up. */
    } else {
        unused = write(errpipe_write, "RuntimeError:0:", 15);
        unused = write(errpipe_write, err_msg, strlen(err_msg));
    }
    if (unused) return;  /* silly? yes! avoids gcc compiler warning. */
}


static PyObject *
subprocess_fork_exec(PyObject* self, PyObject *args)
{
    PyObject *gc_module = NULL;
    PyObject *executable_list, *py_fds_to_keep;
    PyObject *env_list, *preexec_fn;
    PyObject *process_args, *converted_args = NULL, *fast_args = NULL;
    PyObject *preexec_fn_args_tuple = NULL;
    int p2cread, p2cwrite, c2pread, c2pwrite, errread, errwrite;
    int errpipe_read, errpipe_write, close_fds, restore_signals;
    int call_setsid;
    PyObject *cwd_obj, *cwd_obj2;
    const char *cwd;
    pid_t pid;
    int need_to_reenable_gc = 0;
    char *const *exec_array, *const *argv = NULL, *const *envp = NULL;
    Py_ssize_t arg_num;

    if (!PyArg_ParseTuple(
            args, "OOpOOOiiiiiiiiiiO:fork_exec",
            &process_args, &executable_list, &close_fds, &py_fds_to_keep,
            &cwd_obj, &env_list,
            &p2cread, &p2cwrite, &c2pread, &c2pwrite,
            &errread, &errwrite, &errpipe_read, &errpipe_write,
            &restore_signals, &call_setsid, &preexec_fn))
        return NULL;

    if (close_fds && errpipe_write < 3) {  /* precondition */
        PyErr_SetString(PyExc_ValueError, "errpipe_write must be >= 3");
        return NULL;
    }
    if (PySequence_Length(py_fds_to_keep) < 0) {
        PyErr_SetString(PyExc_ValueError, "cannot get length of fds_to_keep");
        return NULL;
    }
    if (_sanity_check_python_fd_sequence(py_fds_to_keep)) {
        PyErr_SetString(PyExc_ValueError, "bad value(s) in fds_to_keep");
        return NULL;
    }

    /* We need to call gc.disable() when we'll be calling preexec_fn */
    if (preexec_fn != Py_None) {
        PyObject *result;
        _Py_IDENTIFIER(isenabled);
        _Py_IDENTIFIER(disable);
        
        gc_module = PyImport_ImportModule("gc");
        if (gc_module == NULL)
            return NULL;
        result = _PyObject_CallMethodId(gc_module, &PyId_isenabled, NULL);
        if (result == NULL) {
            Py_DECREF(gc_module);
            return NULL;
        }
        need_to_reenable_gc = PyObject_IsTrue(result);
        Py_DECREF(result);
        if (need_to_reenable_gc == -1) {
            Py_DECREF(gc_module);
            return NULL;
        }
        result = _PyObject_CallMethodId(gc_module, &PyId_disable, NULL);
        if (result == NULL) {
            Py_DECREF(gc_module);
            return NULL;
        }
        Py_DECREF(result);
    }

    exec_array = _PySequence_BytesToCharpArray(executable_list);
    if (!exec_array) {
        Py_XDECREF(gc_module);
        return NULL;
    }

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

    if (preexec_fn != Py_None) {
        preexec_fn_args_tuple = PyTuple_New(0);
        if (!preexec_fn_args_tuple)
            goto cleanup;
        _PyImport_AcquireLock();
    }

    if (cwd_obj != Py_None) {
        if (PyUnicode_FSConverter(cwd_obj, &cwd_obj2) == 0)
            goto cleanup;
        cwd = PyBytes_AsString(cwd_obj2);
    } else {
        cwd = NULL;
        cwd_obj2 = NULL;
    }

    pid = fork();
    if (pid == 0) {
        /* Child process */
        /*
         * Code from here to _exit() must only use async-signal-safe functions,
         * listed at `man 7 signal` or
         * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
         */

        if (preexec_fn != Py_None) {
            /* We'll be calling back into Python later so we need to do this.
             * This call may not be async-signal-safe but neither is calling
             * back into Python.  The user asked us to use hope as a strategy
             * to avoid deadlock... */
            PyOS_AfterFork();
        }

        child_exec(exec_array, argv, envp, cwd,
                   p2cread, p2cwrite, c2pread, c2pwrite,
                   errread, errwrite, errpipe_read, errpipe_write,
                   close_fds, restore_signals, call_setsid,
                   py_fds_to_keep, preexec_fn, preexec_fn_args_tuple);
        _exit(255);
        return NULL;  /* Dead code to avoid a potential compiler warning. */
    }
    Py_XDECREF(cwd_obj2);

    if (pid == -1) {
        /* Capture the errno exception before errno can be clobbered. */
        PyErr_SetFromErrno(PyExc_OSError);
    }
    if (preexec_fn != Py_None &&
        _PyImport_ReleaseLock() < 0 && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError,
                        "not holding the import lock");
    }

    /* Parent process */
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    _Py_FreeCharPArray(exec_array);

    /* Reenable gc in the parent process (or if fork failed). */
    if (need_to_reenable_gc && _enable_gc(gc_module)) {
        Py_XDECREF(gc_module);
        return NULL;
    }
    Py_XDECREF(preexec_fn_args_tuple);
    Py_XDECREF(gc_module);

    if (pid == -1)
        return NULL;  /* fork() failed.  Exception set earlier. */

    return PyLong_FromPid(pid);

cleanup:
    if (envp)
        _Py_FreeCharPArray(envp);
    if (argv)
        _Py_FreeCharPArray(argv);
    _Py_FreeCharPArray(exec_array);
    Py_XDECREF(converted_args);
    Py_XDECREF(fast_args);
    Py_XDECREF(preexec_fn_args_tuple);

    /* Reenable gc if it was disabled. */
    if (need_to_reenable_gc)
        _enable_gc(gc_module);
    Py_XDECREF(gc_module);
    return NULL;
}


PyDoc_STRVAR(subprocess_fork_exec_doc,
"fork_exec(args, executable_list, close_fds, cwd, env,\n\
          p2cread, p2cwrite, c2pread, c2pwrite,\n\
          errread, errwrite, errpipe_read, errpipe_write,\n\
          restore_signals, call_setsid, preexec_fn)\n\
\n\
Forks a child process, closes parent file descriptors as appropriate in the\n\
child and dups the few that are needed before calling exec() in the child\n\
process.\n\
\n\
The preexec_fn, if supplied, will be called immediately before exec.\n\
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

PyDoc_STRVAR(subprocess_cloexec_pipe_doc,
"cloexec_pipe() -> (read_end, write_end)\n\n\
Create a pipe whose ends have the cloexec flag set.");

static PyObject *
subprocess_cloexec_pipe(PyObject *self, PyObject *noargs)
{
    int fds[2];
    int res;
#ifdef HAVE_PIPE2
    Py_BEGIN_ALLOW_THREADS
    res = pipe2(fds, O_CLOEXEC);
    Py_END_ALLOW_THREADS
    if (res != 0 && errno == ENOSYS)
    {
        {
#endif
        /* We hold the GIL which offers some protection from other code calling
         * fork() before the CLOEXEC flags have been set but we can't guarantee
         * anything without pipe2(). */
        long oldflags;

        res = pipe(fds);

        if (res == 0) {
            oldflags = fcntl(fds[0], F_GETFD, 0);
            if (oldflags < 0) res = oldflags;
        }
        if (res == 0)
            res = fcntl(fds[0], F_SETFD, oldflags | FD_CLOEXEC);

        if (res == 0) {
            oldflags = fcntl(fds[1], F_GETFD, 0);
            if (oldflags < 0) res = oldflags;
        }
        if (res == 0)
            res = fcntl(fds[1], F_SETFD, oldflags | FD_CLOEXEC);
#ifdef HAVE_PIPE2
        }
    }
#endif
    if (res != 0)
        return PyErr_SetFromErrno(PyExc_OSError);
    return Py_BuildValue("(ii)", fds[0], fds[1]);
}

/* module level code ********************************************************/

PyDoc_STRVAR(module_doc,
"A POSIX helper for the subprocess module.");


static PyMethodDef module_methods[] = {
    {"fork_exec", subprocess_fork_exec, METH_VARARGS, subprocess_fork_exec_doc},
    {"cloexec_pipe", subprocess_cloexec_pipe, METH_NOARGS, subprocess_cloexec_pipe_doc},
    {NULL, NULL}  /* sentinel */
};


static struct PyModuleDef _posixsubprocessmodule = {
	PyModuleDef_HEAD_INIT,
	"_posixsubprocess",
	module_doc,
	-1,  /* No memory is needed. */
	module_methods,
};

PyMODINIT_FUNC
PyInit__posixsubprocess(void)
{
#ifdef _SC_OPEN_MAX
    max_fd = sysconf(_SC_OPEN_MAX);
    if (max_fd == -1)
#endif
        max_fd = 256;  /* Matches Lib/subprocess.py */

    return PyModule_Create(&_posixsubprocessmodule);
}
